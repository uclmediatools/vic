/*
 * FILE:    mbus.c
 * AUTHORS: Colin Perkins
 * 
 * Copyright (c) 1997-99 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config_unix.h"
#include "config_win32.h"
#include "debug.h"
#include "memory.h"
#include "net_udp.h"
#include "hmac.h"
#include "qfDES.h"
#include "base64.h"
#include "gettimeofday.h"
#include "mbus.h"
#include "mbus_config.h"

#define MBUS_BUF_SIZE	  1500
#define MBUS_ACK_BUF_SIZE 1500
#define MBUS_MAX_ADDR	    10
#define MBUS_MAX_PD	    10
#define MBUS_MAX_QLEN	    50 /* Number of messages we can queue with mbus_qmsg() */

#ifdef NEED_VSNPRINTF
static int vsnprintf(char *s, int buf_size, const char *format, va_list ap)
{
	/* Quick hack replacement for vsnprintf... note that this */
	/* doesn't check for buffer overflows, and so is open to  */
	/* many really nasty attacks!                             */
	UNUSED(buf_size);
        return vsprintf(s,format,ap);
}
#endif

struct mbus_msg {
	struct mbus_msg	*next;
	struct timeval	 time;	/* Time the message was sent, to trigger a retransmit */
	struct timeval	 ts;	/* Time the message was composed, the timestamp in the packet header */
	char		*dest;
	int		 reliable;
	int		 complete;	/* Indicates that we've finished adding cmds to this message */
	int		 seqnum;
	int		 retransmit_count;
	int		 message_size;
	int		 num_cmds;
	char		*cmd_list[MBUS_MAX_QLEN];
	char		*arg_list[MBUS_MAX_QLEN];
};

struct mbus {
	socket_udp	 	 *s;
	int		 	  num_addr;
	char		 	*addr[MBUS_MAX_ADDR];		/* Addresses we respond to. 					*/
	int		 	  max_other_addr;
	int		 	  num_other_addr;
	char			**other_addr;			/* Addresses of other entities on the mbus. 			*/
	char		 	 *parse_buffer[MBUS_MAX_PD];	/* Temporary storage for parsing mbus commands 			*/
	char		 	 *parse_bufend[MBUS_MAX_PD];	/* End of space allocated for parsing, to check for overflows 	*/
	int		 	  parse_depth;
	int		 	  seqnum;
	struct mbus_msg	 	 *cmd_queue;			/* Queue of messages waiting to be sent */
	struct mbus_msg	 	 *waiting_ack;			/* The last reliable message sent, if we have not yet got the ACK */
	char		 	 *hashkey;
	int		 	  hashkeylen;
	char		 	 *encrkey;
	int		 	  encrkeylen;
	struct timeval	 	  last_heartbeat;		/* Last time we sent a heartbeat message */
	struct mbus_config	 *cfg;
	void (*cmd_handler)(char *src, char *cmd, char *arg, void *dat);
	void (*err_handler)(int seqnum, int reason);
};

static int mbus_addr_match(char *a, char *b)
{
	/* Compare the addresses "a" and "b". These may optionally be */
	/* surrounded by "(" and ")" and may have an arbitrary amount */
	/* of white space between components of the addresses. There  */
	/* is a match if every word of address b is in address a.     */
	/* NOTE: The strings passed to this function are stored for   */
	/* later use and MUST NOT be modified by this routine.        */
	char	*y, c;

	assert(a != NULL);
	assert(b != NULL);

	/* Skip leading whitespace and '('... */
	while (isspace((unsigned char)*a) || (*a == '(')) a++;
	while (isspace((unsigned char)*b) || (*b == '(')) b++;

	while ((*b != '\0') && (*b != ')')) {
		while (isspace((unsigned char)*b)) b++;
		for (y = b; ((*y != ' ') && (*y != ')') && (*y != '\0')); y++) {
			/* do nothing */
		}
		c = *y;
		*y = '\0';
		if (strstr(a, b) == NULL) {
			/* ...this word not found */
			*y = c;
			return FALSE;
		}
		*y = c;
		b = y;
	}		
	return TRUE;
}

static void store_other_addr(struct mbus *m, char *a)
{
	/* This takes the address a and ensures it is stored in the   */
	/* m->other_addr field of the mbus structure. The other_addr  */
	/* field should probably be a hash table, but for now we hope */
	/* that there are not too many entities on the mbus, so the   */
	/* list is small.                                             */
	int	i;

	for (i = 0; i < m->num_other_addr; i++) {
		if (mbus_addr_match(m->other_addr[i], a)) {
			/* Already in the list... */
			return;
		}
	}

	if (m->num_other_addr == m->max_other_addr) {
		/* Expand the list... */
		m->max_other_addr *= 2;
		m->other_addr = (char **) xrealloc(m->other_addr, m->max_other_addr * sizeof(char *));
	}
	m->other_addr[m->num_other_addr++] = xstrdup(a);
}

int mbus_addr_valid(struct mbus *m, char *addr)
{
	int	i;

	for (i = 0; i < m->num_other_addr; i++) {
		if (mbus_addr_match(m->other_addr[i], addr)) {
			return TRUE;
		}
	}
	return FALSE;
}

/* The tx_* functions are used to build an mbus message up in the */
/* tx_buffer, and to add authentication and encryption before the */
/* message is sent.                                               */
static char	 tx_cryptbuf[MBUS_BUF_SIZE];
static char	 tx_buffer[MBUS_BUF_SIZE];
static char	*tx_bufpos;

#define MBUS_AUTH_LEN 16

static void tx_header(int seqnum, int ts, char reliable, char *src, char *dst, int ackseq)
{
	memset(tx_buffer,   0, MBUS_BUF_SIZE);
	memset(tx_buffer, ' ', MBUS_AUTH_LEN);
	tx_bufpos = tx_buffer + MBUS_AUTH_LEN;
	sprintf(tx_bufpos, "\nmbus/1.0 %6d %9d %c (%s) %s ", seqnum, ts, reliable, src, dst);
	tx_bufpos += 33 + strlen(src) + strlen(dst);
	if (ackseq == -1) {
		sprintf(tx_bufpos, "()\n");
		tx_bufpos += 3;
	} else {
		sprintf(tx_bufpos, "(%6d)\n", ackseq);
		tx_bufpos += 9;
	}
}

static void tx_add_command(char *cmnd, char *args)
{
	sprintf(tx_bufpos, "%s (%s)\n", cmnd, args);
	tx_bufpos += strlen(cmnd) + strlen(args) + 4;
}

static void tx_send(struct mbus *m)
{
	char		digest[16];
	int		len;
	unsigned char	initVec[8] = {0,0,0,0,0,0,0,0};

	while (((tx_bufpos - tx_buffer) % 8) != 0) {
		/* Pad to a multiple of 8 bytes, so the encryption can work... */
		*(tx_bufpos++) = '\0';
	}
	*tx_bufpos = '\0';
	len = tx_bufpos - tx_buffer;

	if (m->hashkey != NULL) {
		/* Authenticate... */
		hmac_md5(tx_buffer + MBUS_AUTH_LEN+1, strlen(tx_buffer) - (MBUS_AUTH_LEN+1), m->hashkey, m->hashkeylen, digest);
		base64encode(digest, 12, tx_buffer, MBUS_AUTH_LEN);
	}
	if (m->encrkey != NULL) {
		/* Encrypt... */
		memset(tx_cryptbuf, 0, MBUS_BUF_SIZE);
		memcpy(tx_cryptbuf, tx_buffer, len);
		assert((len % 8) == 0);
		assert(len < MBUS_BUF_SIZE);
		assert(m->encrkeylen == 8);
		qfDES_CBC_e(m->encrkey, tx_cryptbuf, len, initVec);
		memcpy(tx_buffer, tx_cryptbuf, len);
	}
	udp_send(m->s, tx_buffer, len);
}

static void resend(struct mbus *m, struct mbus_msg *curr) 
{
	/* Don't need to check for buffer overflows: this was done in mbus_send() when */
	/* this message was first transmitted. If it was okay then, it's okay now.     */
	int	 i;

	tx_header(curr->seqnum, curr->ts.tv_sec, (char)(curr->reliable?'R':'U'), m->addr[0], curr->dest, -1);
	for (i = 0; i < curr->num_cmds; i++) {
		tx_add_command(curr->cmd_list[i], curr->arg_list[i]);
	}
	tx_send(m);
	curr->retransmit_count++;
}

void mbus_retransmit(struct mbus *m)
{
	struct mbus_msg	*curr = m->waiting_ack;
	struct timeval	time;
	long		diff;

	if (!mbus_waiting_ack(m)) {
		return;
	}

	gettimeofday(&time, NULL);

	/* diff is time in milliseconds that the message has been awaiting an ACK */
	diff = ((time.tv_sec * 1000) + (time.tv_usec / 1000)) - ((curr->time.tv_sec * 1000) + (curr->time.tv_usec / 1000));
	if (diff > 10000) {
		debug_msg("Reliable mbus message failed!\n");
		if (m->err_handler == NULL) {
			abort();
		}
		m->err_handler(curr->seqnum, MBUS_MESSAGE_LOST);
		return;
	} 
	/* Note: We only send one retransmission each time, to avoid
	 * overflowing the receiver with a burst of requests...
	 */
	if ((diff > 750) && (curr->retransmit_count == 2)) {
		resend(m, curr);
		return;
	} 
	if ((diff > 500) && (curr->retransmit_count == 1)) {
		resend(m, curr);
		return;
	} 
	if ((diff > 250) && (curr->retransmit_count == 0)) {
		resend(m, curr);
		return;
	}
	curr = curr->next;
}

void mbus_heartbeat(struct mbus *m, int interval)
{
	struct timeval	curr_time;

	gettimeofday(&curr_time, NULL);

	if (curr_time.tv_sec - m->last_heartbeat.tv_sec > interval) {
		mbus_qmsg(m, "()", "mbus.hello", "", FALSE);
		m->last_heartbeat = curr_time;
	}
}

int mbus_waiting_ack(struct mbus *m)
{
	return m->waiting_ack != NULL;
}

struct mbus *mbus_init(void  (*cmd_handler)(char *src, char *cmd, char *arg, void *dat), 
		       void  (*err_handler)(int seqnum, int reason))
{
	struct mbus	*m;
	struct mbus_key	 k;
	int		 i;

	m = (struct mbus *) xmalloc(sizeof(struct mbus));
	if (m == NULL) {
		debug_msg("Unable to allocate memory for mbus\n");
		return NULL;
	}

	m->cfg = (struct mbus_config *) xmalloc(sizeof(struct mbus_config));
	mbus_lock_config_file(m->cfg);
	m->s		  = udp_init("224.255.222.239", (u_int16) 47000, (u_int16) 47000, 0);
	m->seqnum         = 0;
	m->cmd_handler    = cmd_handler;
	m->err_handler	  = err_handler;
	m->num_addr       = 0;
	m->num_other_addr = 0;
	m->max_other_addr = 10;
	m->other_addr     = (char **) xmalloc(sizeof(char *) * 10);
	m->parse_depth    = 0;
	m->cmd_queue	  = NULL;
	m->waiting_ack	  = NULL;

	gettimeofday(&(m->last_heartbeat), NULL);

	mbus_get_encrkey(m->cfg, &k);
	m->encrkey    = k.key;
	m->encrkeylen = k.key_len;

	mbus_get_hashkey(m->cfg, &k);
	m->hashkey    = k.key;
	m->hashkeylen = k.key_len;

	for (i = 0; i < MBUS_MAX_ADDR; i++) m->addr[i]         = NULL;
	for (i = 0; i < MBUS_MAX_PD;   i++) m->parse_buffer[i] = NULL;
	for (i = 0; i < MBUS_MAX_PD;   i++) m->parse_bufend[i] = NULL;
	mbus_unlock_config_file(m->cfg);

	return m;
}

void mbus_cmd_handler(struct mbus *m, void  (*cmd_handler)(char *src, char *cmd, char *arg, void *dat))
{
	m->cmd_handler = cmd_handler;
}

static void mbus_flush_msgs(struct mbus_msg *queue)
{
        struct mbus_msg *curr, *next;
        int i;

        curr = queue;
        while(curr) {
                next = curr->next;
                xfree(curr->dest);
                for(i = 0; i < curr->num_cmds; i++) {
                        xfree(curr->cmd_list[i]);
                        xfree(curr->arg_list[i]);
                }
                curr = next;
        }
}

void mbus_exit(struct mbus *m) 
{
        assert(m != NULL);

        while(m->parse_depth--) {
                xfree(m->parse_buffer[m->parse_depth]);
		m->parse_buffer[m->parse_depth] = NULL;
		m->parse_bufend[m->parse_depth] = NULL;
        }

        mbus_flush_msgs(m->cmd_queue);
        mbus_flush_msgs(m->waiting_ack);

        if (m->encrkey != NULL) {
                xfree(m->encrkey);
        }

        udp_exit(m->s);

        xfree(m->hashkey);
	xfree(m->cfg);
        xfree(m);
}

void mbus_addr(struct mbus *m, char *addr)
{
	assert(m->num_addr < MBUS_MAX_ADDR);
	mbus_parse_init(m, xstrdup(addr));
	if (mbus_parse_lst(m, &(m->addr[m->num_addr]))) {
		m->num_addr++;
	}
	mbus_parse_done(m);
}

void mbus_send(struct mbus *m)
{
	/* Send one, or more, messages previosly queued with mbus_qmsg(). */
	/* Messages for the same destination are batched together. Stops  */
	/* when a reliable message is sent, until the ACK is received.    */
	struct mbus_msg	*curr = m->cmd_queue;
	int		 i;

	if (m->waiting_ack != NULL) {
		return;
	}

	while (curr != NULL) {
		/* Create the message... */
		tx_header(curr->seqnum, curr->ts.tv_sec, (char)(curr->reliable?'R':'U'), m->addr[0], curr->dest, -1);
		for (i = 0; i < curr->num_cmds; i++) {
			tx_add_command(curr->cmd_list[i], curr->arg_list[i]);
		}
		tx_send(m);
		
		m->cmd_queue = curr->next;
		if (curr->reliable) {
			/* Reliable message, wait for the ack... */
			gettimeofday(&(curr->time), NULL);
			m->waiting_ack = curr;
			return;
		} else {
			while (curr->num_cmds > 0) {
				curr->num_cmds--;
				xfree(curr->cmd_list[curr->num_cmds]);
				xfree(curr->arg_list[curr->num_cmds]);
			}
			xfree(curr->dest);
			xfree(curr);
		}
		curr = m->cmd_queue;
	}
}

void mbus_qmsg(struct mbus *m, char *dest, const char *cmnd, const char *args, int reliable)
{
	/* Queue up a message for sending. The message is not */
	/* actually sent until mbus_send() is called.         */
	struct mbus_msg	*curr = m->cmd_queue;
	struct mbus_msg	*prev = NULL;
	int		 alen = strlen(cmnd) + strlen(args) + 4;

	if (reliable && !mbus_addr_valid(m, dest)) {
		debug_msg("Trying to send reliably to an unknown address...\n");
#ifdef NDEF
		if (m->err_handler == NULL) {
			abort();
		}
		m->err_handler(curr->seqnum, MBUS_DESTINATION_UNKNOWN);
#endif
	}

	while (curr != NULL) {
		if ((!curr->complete)
		&& mbus_addr_match(curr->dest, dest) 
		&& (curr->num_cmds < MBUS_MAX_QLEN) 
		&& ((curr->message_size + alen) < (MBUS_BUF_SIZE - 8))) {
			/* Slots message in if it fits, but this breaks ordering.  Msg
		         * X+1 maybe shorter than X that is in next packet, so X+1 jumps
		         * ahead.
		         */
			curr->num_cmds++;
			curr->reliable |= reliable;
			curr->cmd_list[curr->num_cmds-1] = xstrdup(cmnd);
			curr->arg_list[curr->num_cmds-1] = xstrdup(args);
			curr->message_size += alen;
			return;
		} else {
			curr->complete = TRUE;
		}
		prev = curr;
		curr = curr->next;
	}
	curr = (struct mbus_msg *) xmalloc(sizeof(struct mbus_msg));
	curr->next             = NULL;
	curr->dest             = xstrdup(dest);
	curr->retransmit_count = 0;
	curr->message_size     = alen + 60 + strlen(dest) + strlen(m->addr[0]);
	curr->seqnum           = m->seqnum++;
	curr->reliable         = reliable;
	curr->complete         = FALSE;
	curr->num_cmds         = 1;
	curr->cmd_list[0]      = xstrdup(cmnd);
	curr->arg_list[0]      = xstrdup(args);
	if (prev == NULL) {
		m->cmd_queue = curr;
	} else {
		prev->next = curr;
	}
	gettimeofday(&(curr->time), NULL);
	gettimeofday(&(curr->ts),   NULL);
}

void mbus_qmsgf(struct mbus *m, char *dest, int reliable, const char *cmnd, const char *format, ...)
{
	/* This is a wrapper around mbus_qmsg() which does a printf() style format into  */
	/* a buffer. Saves the caller from having to a a malloc(), write the args string */
	/* and then do a free(), and also saves worring about overflowing the buffer, so */
	/* removing a common source of bugs!                                             */
	char	buffer[MBUS_BUF_SIZE];
	va_list	ap;

	va_start(ap, format);
#ifdef WIN32
        _vsnprintf(buffer, MBUS_BUF_SIZE, format, ap);
#else
        vsnprintf(buffer, MBUS_BUF_SIZE, format, ap);
#endif
	va_end(ap);
	mbus_qmsg(m, dest, cmnd, buffer, reliable);
}

void mbus_parse_init(struct mbus *m, char *str)
{
	assert(m->parse_depth < (MBUS_MAX_PD - 1));
	m->parse_depth++;
	m->parse_buffer[m->parse_depth] = str;
	m->parse_bufend[m->parse_depth] = str + strlen(str);
}

void mbus_parse_done(struct mbus *m)
{
	m->parse_buffer[m->parse_depth] = NULL;
	m->parse_bufend[m->parse_depth] = NULL;
	m->parse_depth--;
	assert(m->parse_depth >= 0);
}

#define CHECK_OVERRUN if (m->parse_buffer[m->parse_depth] > m->parse_bufend[m->parse_depth]) {\
	debug_msg("parse buffer overflow\n");\
	return FALSE;\
}

int mbus_parse_lst(struct mbus *m, char **l)
{
	int instr = FALSE;
	int inlst = FALSE;

	*l = m->parse_buffer[m->parse_depth];
        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
		CHECK_OVERRUN;
        }
	if (*m->parse_buffer[m->parse_depth] != '(') {
		return FALSE;
	}
	*(m->parse_buffer[m->parse_depth]) = ' ';
	while (*m->parse_buffer[m->parse_depth] != '\0') {
		if ((*m->parse_buffer[m->parse_depth] == '"') && (*(m->parse_buffer[m->parse_depth]-1) != '\\')) {
			instr = !instr;
		}
		if ((*m->parse_buffer[m->parse_depth] == '(') && (*(m->parse_buffer[m->parse_depth]-1) != '\\') && !instr) {
			inlst = !inlst;
		}
		if ((*m->parse_buffer[m->parse_depth] == ')') && (*(m->parse_buffer[m->parse_depth]-1) != '\\') && !instr) {
			if (inlst) {
				inlst = !inlst;
			} else {
				*m->parse_buffer[m->parse_depth] = '\0';
				m->parse_buffer[m->parse_depth]++;
				CHECK_OVERRUN;
				return TRUE;
			}
		}
		m->parse_buffer[m->parse_depth]++;
		CHECK_OVERRUN;
	}
	return FALSE;
}

int mbus_parse_str(struct mbus *m, char **s)
{
        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
		CHECK_OVERRUN;
        }
	if (*m->parse_buffer[m->parse_depth] != '"') {
		return FALSE;
	}
	*s = m->parse_buffer[m->parse_depth]++;
	while (*m->parse_buffer[m->parse_depth] != '\0') {
		if ((*m->parse_buffer[m->parse_depth] == '"') && (*(m->parse_buffer[m->parse_depth]-1) != '\\')) {
			m->parse_buffer[m->parse_depth]++;
			*m->parse_buffer[m->parse_depth] = '\0';
			m->parse_buffer[m->parse_depth]++;
			return TRUE;
		}
		m->parse_buffer[m->parse_depth]++;
		CHECK_OVERRUN;
	}
	return FALSE;
}

static int mbus_parse_sym(struct mbus *m, char **s)
{
        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
		CHECK_OVERRUN;
        }
	if (!isgraph((unsigned char)*m->parse_buffer[m->parse_depth])) {
		return FALSE;
	}
	*s = m->parse_buffer[m->parse_depth]++;
	while (!isspace((unsigned char)*m->parse_buffer[m->parse_depth]) && (*m->parse_buffer[m->parse_depth] != '\0')) {
		m->parse_buffer[m->parse_depth]++;
		CHECK_OVERRUN;
	}
	*m->parse_buffer[m->parse_depth] = '\0';
	m->parse_buffer[m->parse_depth]++;
	CHECK_OVERRUN;
	return TRUE;
}

int mbus_parse_int(struct mbus *m, int *i)
{
	char	*p;

        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
		CHECK_OVERRUN;
        }

	*i = strtol(m->parse_buffer[m->parse_depth], &p, 10);
	if (((*i == LONG_MAX) || (*i == LONG_MIN)) && (errno == ERANGE)) {
		debug_msg("integer out of range\n");
		return FALSE;
	}

	if (p == m->parse_buffer[m->parse_depth]) {
		return FALSE;
	}
	if (!isspace((unsigned char)*p) && (*p != '\0')) {
		return FALSE;
	}
	m->parse_buffer[m->parse_depth] = p;
	CHECK_OVERRUN;
	return TRUE;
}

int mbus_parse_flt(struct mbus *m, double *d)
{
	char	*p;
        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
		CHECK_OVERRUN;
        }

	*d = strtod(m->parse_buffer[m->parse_depth], &p);
	if (errno == ERANGE) {
		debug_msg("float out of range\n");
		return FALSE;
	}

	if (p == m->parse_buffer[m->parse_depth]) {
		return FALSE;
	}
	if (!isspace((unsigned char)*p) && (*p != '\0')) {
		return FALSE;
	}
	m->parse_buffer[m->parse_depth] = p;
	CHECK_OVERRUN;
	return TRUE;
}

char *mbus_decode_str(char *s)
{
	int	l = strlen(s);
	int	i, j;

	/* Check that this an encoded string... */
	assert(s[0]   == '\"');
	assert(s[l-1] == '\"');

	for (i=1,j=0; i < l - 1; i++,j++) {
		if (s[i] == '\\') {
			i++;
		}
		s[j] = s[i];
	}
	s[j] = '\0';
	return s;
}

char *mbus_encode_str(const char *s)
{
	int 	 i, j;
	int	 len = strlen(s);
	char	*buf = (char *) xmalloc((len * 2) + 3);

	for (i = 0, j = 1; i < len; i++,j++) {
		if (s[i] == ' ') {
			buf[j] = '\\';
			buf[j+1] = ' ';
			j++;
		} else if (s[i] == '\"') {
			buf[j] = '\\';
			buf[j+1] = '\"';
			j++;
		} else {
			buf[j] = s[i];
		}
	}
	buf[0]   = '\"';
	buf[j]   = '\"';
	buf[j+1] = '\0';
	return buf;
}

int mbus_recv(struct mbus *m, void *data, struct timeval *timeout)
{
	char		*auth, *ver, *src, *dst, *ack, *r, *cmd, *param, *npos;
	char	 	buffer[MBUS_BUF_SIZE];
	int	 	buffer_len, seq, i, a, rx, ts, authlen;
	char	 	ackbuf[MBUS_ACK_BUF_SIZE];
	char	 	digest[16];
	unsigned char	initVec[8] = {0,0,0,0,0,0,0,0};

	rx = FALSE;
	while (1) {
		memset(buffer, 0, MBUS_BUF_SIZE);
                assert(m->s != NULL);
		udp_fd_zero();
		udp_fd_set(m->s);
                if ((udp_select(timeout) > 0) && udp_fd_isset(m->s)) {
			buffer_len = udp_recv(m->s, buffer, MBUS_BUF_SIZE);
			if (buffer_len > 0) {
				rx = TRUE;
			} else {
				return rx;
			}
		} else {
			return FALSE;
		}

		if (m->encrkey != NULL) {
			/* Decrypt the message... */
			if ((buffer_len % 8) != 0) {
				debug_msg("Encrypted message not a multiple of 8 bytes in length\n");
				continue;
			}
			memcpy(tx_cryptbuf, buffer, buffer_len);
			memset(initVec, 0, 8);
			qfDES_CBC_d(m->encrkey, tx_cryptbuf, buffer_len, initVec);
			memcpy(buffer, tx_cryptbuf, buffer_len);
		}

		/* Sanity check that this is a vaguely sensible format message... Should prevent */
		/* problems if we're fed complete garbage, but won't prevent determined hackers. */
		if (strncmp(buffer + MBUS_AUTH_LEN + 1, "mbus/1.0", 8) != 0) {
			debug_msg("Message did not correctly decrypt...\n");
			continue;
		}

		mbus_parse_init(m, buffer);
		/* remove trailing 0 bytes */
		npos=strchr(buffer,'\0');
		if(npos!=NULL) {
			buffer_len=npos-buffer;
		}
		/* Parse the authentication header */
		if (!mbus_parse_sym(m, &auth)) {
			debug_msg("Failed to parse authentication header\n");
			mbus_parse_done(m);
			continue;
		}

		/* Check that the packet authenticates correctly... */
		authlen = strlen(auth);
		hmac_md5(buffer + authlen + 1, buffer_len - authlen - 1, m->hashkey, m->hashkeylen, digest);
		base64encode(digest, 12, ackbuf, 16);
		if ((strlen(auth) != 16) || (strncmp(auth, ackbuf, 16) != 0)) {
			debug_msg("Failed to authenticate message...\n");
			mbus_parse_done(m);
			continue;
		}

		/* Parse the header */
		if (!mbus_parse_sym(m, &ver)) {
			mbus_parse_done(m);
			debug_msg("Parser failed version (1): %s\n",ver);
			continue;
		}
		if (strcmp(ver, "mbus/1.0") != 0) {
			mbus_parse_done(m);
			debug_msg("Parser failed version (2): %s\n",ver);
			continue;
		}
		if (!mbus_parse_int(m, &seq)) {
			mbus_parse_done(m);
			debug_msg("Parser failed seq\n");
			continue;
		}
		if (!mbus_parse_int(m, &ts)) {
			mbus_parse_done(m);
			debug_msg("Parser failed ts\n");
			continue;
		}
		if (!mbus_parse_sym(m, &r)) {
			mbus_parse_done(m);
			debug_msg("Parser failed reliable\n");
			continue;
		}
		if (!mbus_parse_lst(m, &src)) {
			mbus_parse_done(m);
			debug_msg("Parser failed src\n");
			continue;
		}
		if (!mbus_parse_lst(m, &dst)) {
			mbus_parse_done(m);
			debug_msg("Parser failed dst\n");
			continue;
		}
		if (!mbus_parse_lst(m, &ack)) {
			mbus_parse_done(m);
			debug_msg("Parser failed ack\n");
			continue;
		}

		store_other_addr(m, src);

		/* Check if the message was addressed to us... */
		for (i = 0; i < m->num_addr; i++) {
			if (mbus_addr_match(m->addr[i], dst)) {
				/* ...if so, process any ACKs received... */
				mbus_parse_init(m, ack);
				while (mbus_parse_int(m, &a)) {
					if (mbus_waiting_ack(m) && (m->waiting_ack->seqnum == a)) {
						while (m->waiting_ack->num_cmds > 0) {
							m->waiting_ack->num_cmds--;
							xfree(m->waiting_ack->cmd_list[m->waiting_ack->num_cmds]);
							xfree(m->waiting_ack->arg_list[m->waiting_ack->num_cmds]);
						}
						xfree(m->waiting_ack->dest);
						xfree(m->waiting_ack);
						m->waiting_ack = NULL;
					}
				}
				mbus_parse_done(m);
				/* ...if an ACK was requested, send one... */
				if (strcmp(r, "R") == 0) {
					char 		*newsrc = (char *) xmalloc(strlen(src) + 3);
					struct timeval	 t;

					sprintf(newsrc, "(%s)", src);	/* Yes, this is a kludge. */
					gettimeofday(&t, NULL);
					tx_header(++m->seqnum, (int) t.tv_sec, 'U', m->addr[0], newsrc, seq);
					tx_send(m);
					xfree(newsrc);
				}
				/* ...and process the commands contained in the message */
				while (mbus_parse_sym(m, &cmd)) {
					if (mbus_parse_lst(m, &param)) {
						char 		*newsrc = (char *) xmalloc(strlen(src) + 3);
						sprintf(newsrc, "(%s)", src);	/* Yes, this is a kludge. */
						m->cmd_handler(newsrc, cmd, param, data);
						xfree(newsrc);
					} else {
						debug_msg("Unable to parse mbus command:\n");
						debug_msg("cmd = %s\n", cmd);
						debug_msg("arg = %s\n", param);
						break;
					}
				}
			}
		}
		mbus_parse_done(m);
	}
}

