/*
 * FILE:     mbus.c
 * AUTHOR:   Colin Perkins
 * MODIFIED: Orion Hodson
 *           Markus Germeier
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
        struct timeval          **other_hello;                  /* Time of last mbus.hello we received from other entities      */
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
	char	*y = NULL, c='\0';

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
		m->other_hello = (struct timeval **) xrealloc(m->other_hello, m->max_other_addr * sizeof(struct timeval *));
	}
	m->other_hello[m->num_other_addr]=(struct timeval *)xmalloc(sizeof(struct timeval));
	gettimeofday(m->other_hello[m->num_other_addr],NULL);
	m->other_addr[m->num_other_addr++] = xstrdup(a);
}

static void remove_other_addr(struct mbus *m, char *a)
{
	/* Removes the address a from the m->other_addr field of the */
	/* mbus structure.                                           */
	int	i, j;

	for (i = 0; i < m->num_other_addr; i++) {
		if (mbus_addr_match(m->other_addr[i], a)) {
			xfree(m->other_addr[i]);
			xfree(m->other_hello[i]);
			for (j = i+1; j < m->num_other_addr; j++) {
				m->other_addr[j-1] = m->other_addr[j];
				m->other_hello[j-1] = m->other_hello[j];
			}
			m->num_other_addr--;
		}
	}
}

static void mark_activ_other_addr(struct mbus *m, char *a){
    int i;
    struct timeval	 t;
    
    gettimeofday(&t, NULL);
    for (i = 0; i < m->num_other_addr; i++) {
	if (mbus_addr_match(m->other_addr[i], a)) {
	    m->other_hello[i]->tv_sec=t.tv_sec;
	}
    }
}

static void remove_inactiv_other_addr(struct mbus *m, struct timeval t, int interval){
    /* Remove addresses we haven't heard from for about 5 * interval */
    /* Count backwards so it is save to remove entries               */
    int i;
    
    for (i=m->num_other_addr-1; i>=0; i--){
	if ((t.tv_sec-(m->other_hello[i]->tv_sec)) > 5 * interval) {
	    debug_msg("remove dead entity (%s)\n", m->other_addr[i]);
	    remove_other_addr(m, m->other_addr[i]);
	    
	}
    }
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

static int mbus_addr_unique(struct mbus *m, char *addr)
{
      int     i, n;

      n = 0;

      for (i = 0; i < m->num_other_addr; i++) {
              if (mbus_addr_match(m->other_addr[i], addr)) {
                  n++;
              }
      }
      return (n==1);
}

/* The mb_* functions are used to build an mbus message up in the */
/* mb_buffer, and to add authentication and encryption before the */
/* message is sent.                                               */
static char	 mb_cryptbuf[MBUS_BUF_SIZE];
static char	 mb_buffer[MBUS_BUF_SIZE];
static char	*mb_bufpos;

#define MBUS_AUTH_LEN 16

static void mb_header(int seqnum, int ts, char reliable, char *src, char *dst, int ackseq)
{
	memset(mb_buffer,   0, MBUS_BUF_SIZE);
	memset(mb_buffer, ' ', MBUS_AUTH_LEN);
	mb_bufpos = mb_buffer + MBUS_AUTH_LEN;
	sprintf(mb_bufpos, "\nmbus/1.0 %6d %9d %c (%s) %s ", seqnum, ts, reliable, src, dst);
	mb_bufpos += 33 + strlen(src) + strlen(dst);
	if (ackseq == -1) {
		sprintf(mb_bufpos, "()\n");
		mb_bufpos += 3;
	} else {
		sprintf(mb_bufpos, "(%6d)\n", ackseq);
		mb_bufpos += 9;
	}
}

static void mb_add_command(char *cmnd, char *args)
{
	sprintf(mb_bufpos, "%s (%s)\n", cmnd, args);
	mb_bufpos += strlen(cmnd) + strlen(args) + 4;
}

static void mb_send(struct mbus *m)
{
	char		digest[16];
	int		len;
	unsigned char	initVec[8] = {0,0,0,0,0,0,0,0};

	while (((mb_bufpos - mb_buffer) % 8) != 0) {
		/* Pad to a multiple of 8 bytes, so the encryption can work... */
		*(mb_bufpos++) = '\0';
	}
	*mb_bufpos = '\0';
	len = mb_bufpos - mb_buffer;

	if (m->hashkey != NULL) {
		/* Authenticate... */
		hmac_md5(mb_buffer + MBUS_AUTH_LEN+1, strlen(mb_buffer) - (MBUS_AUTH_LEN+1), m->hashkey, m->hashkeylen, digest);
		base64encode(digest, 12, mb_buffer, MBUS_AUTH_LEN);
	}
	if (m->encrkey != NULL) {
		/* Encrypt... */
		memset(mb_cryptbuf, 0, MBUS_BUF_SIZE);
		memcpy(mb_cryptbuf, mb_buffer, len);
		assert((len % 8) == 0);
		assert(len < MBUS_BUF_SIZE);
		assert(m->encrkeylen == 8);
		qfDES_CBC_e(m->encrkey, mb_cryptbuf, len, initVec);
		memcpy(mb_buffer, mb_cryptbuf, len);
	}
	udp_send(m->s, mb_buffer, len);
}

static void resend(struct mbus *m, struct mbus_msg *curr) 
{
	/* Don't need to check for buffer overflows: this was done in mbus_send() when */
	/* this message was first transmitted. If it was okay then, it's okay now.     */
	int	 i;

	mb_header(curr->seqnum, curr->ts.tv_sec, (char)(curr->reliable?'R':'U'), m->addr[0], curr->dest, -1);
	for (i = 0; i < curr->num_cmds; i++) {
		mb_add_command(curr->cmd_list[i], curr->arg_list[i]);
	}
	mb_send(m);
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
		/* if we don't delete this failed message, the error handler
                   gets triggered every time we call mbus_retransmit */
		while (m->waiting_ack->num_cmds > 0) {
		    m->waiting_ack->num_cmds--;
		    xfree(m->waiting_ack->cmd_list[m->waiting_ack->num_cmds]);
		    xfree(m->waiting_ack->arg_list[m->waiting_ack->num_cmds]);
		}
		xfree(m->waiting_ack->dest);
		xfree(m->waiting_ack);
		m->waiting_ack = NULL;
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

	if (curr_time.tv_sec - m->last_heartbeat.tv_sec >= interval) {
		mbus_qmsg(m, "()", "mbus.hello", "", FALSE);
		m->last_heartbeat = curr_time;
		/* Remove dead sources */
		remove_inactiv_other_addr(m, curr_time, interval);
	}
}

int mbus_waiting_ack(struct mbus *m)
{
	return m->waiting_ack != NULL;
}

int mbus_sent_all(struct mbus *m)
{
	return (m->cmd_queue == NULL) && (m->waiting_ack == NULL);
}

struct mbus *mbus_init(void  (*cmd_handler)(char *src, char *cmd, char *arg, void *dat), 
		       void  (*err_handler)(int seqnum, int reason))
{
	struct mbus	*m;
	struct mbus_key	 k;
	int		 i;
	char            *net_addr;
	uint16_t        net_port;
	int              net_scope;

	m = (struct mbus *) xmalloc(sizeof(struct mbus));
	if (m == NULL) {
		debug_msg("Unable to allocate memory for mbus\n");
		return NULL;
	}

	m->cfg = (struct mbus_config *) xmalloc(sizeof(struct mbus_config));
	mbus_lock_config_file(m->cfg);
	net_addr = (char *) xmalloc(20);
	mbus_get_net_addr(m->cfg, net_addr, &net_port, &net_scope);
	m->s		  = udp_init(net_addr, net_port, net_port, net_scope);	
	m->seqnum         = 0;
	m->cmd_handler    = cmd_handler;
	m->err_handler	  = err_handler;
	m->num_addr       = 0;
	m->num_other_addr = 0;
	m->max_other_addr = 10;
	m->other_addr     = (char **) xmalloc(sizeof(char *) * 10);
	m->other_hello    = (struct timeval **) xmalloc(sizeof(struct timeval *) * 10);
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

	xfree(net_addr);

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
        int i;

        assert(m != NULL);

	mbus_qmsg(m, "()", "mbus.bye", "", FALSE);
	mbus_send(m);

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

	/* Clean up other_* */
	for (i=m->num_other_addr-1; i>=0; i--){
	    remove_other_addr(m, m->other_addr[i]);
	}

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
		if (curr->reliable) {
		        if (!mbus_addr_valid(m, curr->dest)) {
			    debug_msg("Trying to send reliably to an unknown address...\n");
			    if (m->err_handler == NULL) {
				abort();
			    }
			    m->err_handler(curr->seqnum, MBUS_DESTINATION_UNKNOWN);
			}
		        if (!mbus_addr_unique(m, curr->dest)) {
			    debug_msg("Trying to send reliably but address is not unique...\n");
			    if (m->err_handler == NULL) {
				abort();
			    }
			    m->err_handler(curr->seqnum, MBUS_DESTINATION_NOT_UNIQUE);
			}
		}
		/* Create the message... */
		mb_header(curr->seqnum, curr->ts.tv_sec, (char)(curr->reliable?'R':'U'), m->addr[0], curr->dest, -1);
		for (i = 0; i < curr->num_cmds; i++) {
			mb_add_command(curr->cmd_list[i], curr->arg_list[i]);
		}
		mb_send(m);
		
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

	while (curr != NULL) {
		if ((!curr->complete)
		&& mbus_addr_match(curr->dest, dest) 
		&& (curr->num_cmds < MBUS_MAX_QLEN) 
		&& ((curr->message_size + alen) < (MBUS_BUF_SIZE - 8))) {
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
	int	 	buffer_len, seq, i, a, rx, ts, authlen, loop_count;
	char	 	ackbuf[MBUS_ACK_BUF_SIZE];
	char	 	digest[16];
	unsigned char	initVec[8] = {0,0,0,0,0,0,0,0};

	rx = FALSE;
	loop_count = 0;
	while (loop_count++ < 10) {
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
			memcpy(mb_cryptbuf, buffer, buffer_len);
			memset(initVec, 0, 8);
			qfDES_CBC_d(m->encrkey, mb_cryptbuf, buffer_len, initVec);
			memcpy(buffer, mb_cryptbuf, buffer_len);
		}

		/* Sanity check that this is a vaguely sensible format message... Should prevent */
		/* problems if we're fed complete garbage, but won't prevent determined hackers. */
		if (strncmp(buffer + MBUS_AUTH_LEN + 1, "mbus/1.0", 8) != 0) {
			continue;
		}

		mbus_parse_init(m, buffer);
		/* remove trailing 0 bytes */
		npos = (char *) strchr(buffer,'\0');
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
					mb_header(++m->seqnum, (int) t.tv_sec, 'U', m->addr[0], newsrc, seq);
					mb_send(m);
					xfree(newsrc);
				}
				/* ...and process the commands contained in the message */
				while (mbus_parse_sym(m, &cmd)) {
					if (mbus_parse_lst(m, &param)) {
						char 		*newsrc = (char *) xmalloc(strlen(src) + 3);
						sprintf(newsrc, "(%s)", src);	/* Yes, this is a kludge. */
						m->cmd_handler(newsrc, cmd, param, data);
						/* Finally, we snoop on the message we just passed to the application, */
						/* to do housekeeping of our list of known mbus sources...             */
						if (strcmp(cmd, "mbus.bye") == 0) {
							remove_other_addr(m, newsrc);
						} 
						if (strcmp(cmd, "mbus.hello") == 0) {
						/* Mark this source as activ. We remove dead sources in mbus_heartbeat */
						    mark_activ_other_addr(m, newsrc);
						}
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
	return rx;
}

#define RZ_HANDLE_WAITING 1
#define RZ_HANDLE_GO      2

struct mbus_rz {
	char		*peer;
	char		*token;
	struct mbus	*m;
	void		*data;
	int		 mode;
	void (*cmd_handler)(char *src, char *cmd, char *args, void *data);
};

static void rz_handler(char *src, char *cmd, char *args, void *data)
{
	struct mbus_rz	*r = (struct mbus_rz *) data;;

	if ((r->mode == RZ_HANDLE_WAITING) && (strcmp(cmd, "mbus.waiting") == 0)) {
		char	*t;

		mbus_parse_init(r->m, args);
		mbus_parse_str(r->m, &t);
		if (strcmp(mbus_decode_str(t), r->token) == 0) {
			r->peer = xstrdup(src);
		}
		mbus_parse_done(r->m);
	} else if ((r->mode == RZ_HANDLE_GO) && (strcmp(cmd, "mbus.go") == 0)) {
		char	*t;

		mbus_parse_init(r->m, args);
		mbus_parse_str(r->m, &t);
		if (strcmp(mbus_decode_str(t), r->token) == 0) {
			r->peer = xstrdup(src);
		}
		mbus_parse_done(r->m);
	} else if (strcmp(cmd, "mbus.hello") == 0) {
		/* Silently ignore this... */
	} else {
		r->cmd_handler(src, cmd, args, r->data);
	}
}

char *mbus_rendezvous_waiting(struct mbus *m, char *addr, char *token, void *data)
{
	/* Loop, sending mbus.waiting(token) to "addr", until we get mbus.go(token) */
	/* back from our peer. Any other mbus commands received whilst waiting are  */
	/* processed in the normal manner, as if mbus_recv() had been called.       */
	char		*token_e, *peer;
	struct timeval	 timeout;
	struct mbus_rz	*r;

	r = (struct mbus_rz *) xmalloc(sizeof(struct mbus_rz));
	r->peer        = NULL;
	r->token       = token;
	r->m           = m;
	r->data        = data;
	r->mode        = RZ_HANDLE_GO;
	r->cmd_handler = m->cmd_handler;
	m->cmd_handler = rz_handler;
	token_e        = mbus_encode_str(token);
	while (r->peer == NULL) {
		timeout.tv_sec  = 0;
		timeout.tv_usec = 250000;
		mbus_heartbeat(m, 1);
		mbus_qmsgf(m, addr, FALSE, "mbus.waiting", "%s", token_e);
		mbus_send(m);
		mbus_recv(m, r, &timeout);
	}
	m->cmd_handler = r->cmd_handler;
	peer = xstrdup(r->peer);
	xfree(r);
	xfree(token_e);
	return peer;
}

char *mbus_rendezvous_go(struct mbus *m, char *token, void *data)
{
	/* Wait until we receive mbus.waiting(token), then send mbus.go(token) back to   */
	/* the sender of that message. Whilst waiting, other mbus commands are processed */
	/* in the normal manner as if mbus_recv() had been called.                       */
	char		*token_e, *peer;
	struct timeval	 timeout;
	struct mbus_rz	*r;

	r = (struct mbus_rz *) xmalloc(sizeof(struct mbus_rz));
	r->peer        = NULL;
	r->token       = token;
	r->m           = m;
	r->data        = data;
	r->mode        = RZ_HANDLE_WAITING;
	r->cmd_handler = m->cmd_handler;
	m->cmd_handler = rz_handler;
	token_e        = mbus_encode_str(token);
	while (r->peer == NULL) {
		timeout.tv_sec  = 0;
		timeout.tv_usec = 250000;
		mbus_heartbeat(m, 1);
		mbus_send(m);
		mbus_recv(m, r, &timeout);
	}
	mbus_qmsgf(m, r->peer, FALSE, "mbus.go", "%s", token_e);
	mbus_send(m);
	m->cmd_handler = r->cmd_handler;
	peer = xstrdup(r->peer);
	xfree(r);
	xfree(token_e);
	return peer;
}

