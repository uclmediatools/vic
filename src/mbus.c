/*
 * FILE:    mbus.c
 * AUTHORS: Colin Perkins
 * 
 * Copyright (c) 1997,1998 University College London
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
#include "mbus.h"

#define MBUS_BUF_SIZE	1500
#define MBUS_MAX_ADDR	10
#define MBUS_MAX_PD	10
#define MBUS_MAX_QLEN	50		/* Number of messages we can queue with mbus_qmsg() */

struct mbus_msg {
	struct mbus_msg	*next;
	struct timeval	time;
	char		*dest;
	int		 reliable;
	int		 seqnum;
	int		 retransmit_count;
	int		 message_size;
	int		 num_cmds;
	char		*cmd_list[MBUS_MAX_QLEN];
	char		*arg_list[MBUS_MAX_QLEN];
};

struct mbus {
	socket_udp	*s;
	unsigned short	 channel;
	int		 num_addr;
	char		*addr[MBUS_MAX_ADDR];
	char		*parse_buffer[MBUS_MAX_PD];
	int		 parse_depth;
	int		 seqnum;
	void (*cmd_handler)(char *src, char *cmd, char *arg, void *dat);
	void (*err_handler)(int seqnum);
	struct mbus_msg	*cmd_queue;
	struct mbus_msg	*waiting_ack;
};

static int mbus_addr_match(char *a, char *b)
{
	assert(a != NULL);
	assert(b != NULL);

	while ((*a != '\0') && (*b != '\0')) {
		while (isspace((unsigned char)*a)) a++;
		while (isspace((unsigned char)*b)) b++;
		if (*a == '*') {
			a++;
			if ((*a != '\0') && !isspace((unsigned char)*a)) {
				return FALSE;
			}
			while(!isspace((unsigned char)*b) && (*b != '\0')) b++;
		}
		if (*b == '*') {
			b++;
			if ((*b != '\0') && !isspace((unsigned char)*b)) {
				return FALSE;
			}
			while(!isspace((unsigned char)*a) && (*a != '\0')) a++;
		}
		if (*a != *b) {
			return FALSE;
		}
		a++;
		b++;
	}
	return TRUE;
}

static void resend(struct mbus *m, struct mbus_msg *curr) 
{
	char	 buffer[MBUS_BUF_SIZE];
	char	*bufp = buffer;
	int	 i;

	/* Don't need to check for buffer overflows: this was done in mbus_send() when */
	/* this message was first transmitted. If it was okay then, it's okay now.     */
	memset(buffer, 0, MBUS_BUF_SIZE);
	sprintf(bufp, "mbus/1.0 %6d %c (%s) %s ()\n", curr->seqnum, curr->reliable?'R':'U', m->addr[0], curr->dest);
	bufp += strlen(m->addr[0]) + strlen(curr->dest) + 25;
	for (i = 0; i < curr->num_cmds; i++) {
		sprintf(bufp, "%s (%s)\n", curr->cmd_list[i], curr->arg_list[i]);
		bufp += strlen(curr->cmd_list[i]) + strlen(curr->arg_list[i]) + 4;
	}
	debug_msg("### resending %d\n", curr->seqnum);
	udp_send(m->s, buffer, bufp - buffer);
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
		m->err_handler(curr->seqnum);
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

int mbus_waiting_ack(struct mbus *m)
{
	return m->waiting_ack != NULL;
}

struct mbus *mbus_init(unsigned short channel, 
                       void  (*cmd_handler)(char *src, char *cmd, char *arg, void *dat), 
		       void  (*err_handler)(int seqnum))
{
	struct mbus	*m;
	int		 i;

	m = (struct mbus *) xmalloc(sizeof(struct mbus));
	m->s		= udp_init("224.255.222.239", (u_int16) (47000 + channel), 0);
	m->channel	= channel;
	m->seqnum       = 0;
	m->cmd_handler  = cmd_handler;
	m->err_handler	= err_handler;
	m->num_addr     = 0;
	m->parse_depth  = 0;
	m->cmd_queue	= NULL;
	m->waiting_ack	= NULL;
	for (i = 0; i < MBUS_MAX_ADDR; i++) m->addr[i]         = NULL;
	for (i = 0; i < MBUS_MAX_PD;   i++) m->parse_buffer[i] = NULL;
	return m;
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
	char		 buffer[MBUS_BUF_SIZE];
	char		*bufp;
	struct mbus_msg	*curr = m->cmd_queue;
	int		 i;

	if (m->waiting_ack != NULL) {
		return;
	}

	while (curr != NULL) {
		bufp = buffer;
		memset(buffer, 0, MBUS_BUF_SIZE);
		sprintf(bufp, "mbus/1.0 %6d %c (%s) %s ()\n", curr->seqnum, curr->reliable?'R':'U', m->addr[0], curr->dest);
		bufp += strlen(m->addr[0]) + strlen(curr->dest) + 25;
		for (i = 0; i < curr->num_cmds; i++) {
			sprintf(bufp, "%s (%s)\n", curr->cmd_list[i], curr->arg_list[i]);
			bufp += strlen(curr->cmd_list[i]) + strlen(curr->arg_list[i]) + 4;
		}
		assert(strlen(buffer) > 0);
		udp_send(m->s, buffer, bufp - buffer);
		m->cmd_queue = curr->next;
		if (curr->reliable) {
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
		if (mbus_addr_match(curr->dest, dest) && (curr->num_cmds < MBUS_MAX_QLEN) && ((curr->message_size + alen) < MBUS_BUF_SIZE)) {
			curr->num_cmds++;
			curr->reliable |= reliable;
			curr->cmd_list[curr->num_cmds-1] = xstrdup(cmnd);
			curr->arg_list[curr->num_cmds-1] = xstrdup(args);
			curr->message_size += alen;
			return;
		}
		prev = curr;
		curr = curr->next;
	}
	curr = (struct mbus_msg *) xmalloc(sizeof(struct mbus_msg));
	curr->next             = NULL;
	curr->dest             = xstrdup(dest);
	curr->retransmit_count = 0;
	curr->message_size     = alen + 25 + strlen(dest) + strlen(m->addr[0]);
	curr->seqnum           = m->seqnum++;
	curr->reliable         = reliable;
	curr->num_cmds         = 1;
	curr->cmd_list[0]      = xstrdup(cmnd);
	curr->arg_list[0]      = xstrdup(args);
	if (prev == NULL) {
		m->cmd_queue = curr;
	} else {
		prev->next = curr;
	}
}

void mbus_parse_init(struct mbus *m, char *str)
{
	assert(m->parse_depth < (MBUS_MAX_PD - 1));
	m->parse_buffer[++m->parse_depth] = str;
}

void mbus_parse_done(struct mbus *m)
{
	m->parse_depth--;
	assert(m->parse_depth >= 0);
}

int mbus_parse_lst(struct mbus *m, char **l)
{
	int instr = FALSE;
	int inlst = FALSE;

	*l = m->parse_buffer[m->parse_depth];
        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
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
				return TRUE;
			}
		}
		m->parse_buffer[m->parse_depth]++;
	}
	return FALSE;
}

int mbus_parse_str(struct mbus *m, char **s)
{
        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
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
	}
	return FALSE;
}

static int mbus_parse_sym(struct mbus *m, char **s)
{
        while (isspace((unsigned char)*m->parse_buffer[m->parse_depth])) {
                m->parse_buffer[m->parse_depth]++;
        }
	if (!isalpha((unsigned char)*m->parse_buffer[m->parse_depth])) {
		return FALSE;
	}
	*s = m->parse_buffer[m->parse_depth]++;
	while (!isspace((unsigned char)*m->parse_buffer[m->parse_depth]) && (*m->parse_buffer[m->parse_depth] != '\0')) {
		m->parse_buffer[m->parse_depth]++;
	}
	*m->parse_buffer[m->parse_depth] = '\0';
	m->parse_buffer[m->parse_depth]++;
	return TRUE;
}

int mbus_parse_int(struct mbus *m, int *i)
{
	char	*p;
	*i = strtol(m->parse_buffer[m->parse_depth], &p, 10);

	if (p == m->parse_buffer[m->parse_depth]) {
		return FALSE;
	}
	if (!isspace((unsigned char)*p) && (*p != '\0')) {
		return FALSE;
	}
	m->parse_buffer[m->parse_depth] = p;
	return TRUE;
}

int mbus_parse_flt(struct mbus *m, double *d)
{
	char	*p;
	*d = strtod(m->parse_buffer[m->parse_depth], &p);

	if (p == m->parse_buffer[m->parse_depth]) {
		return FALSE;
	}
	if (!isspace((unsigned char)*p) && (*p != '\0')) {
		return FALSE;
	}
	m->parse_buffer[m->parse_depth] = p;
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

int mbus_recv(struct mbus *m, void *data)
{
	char	*ver, *src, *dst, *ack, *r, *cmd, *param;
	char	 buffer[MBUS_BUF_SIZE];
	int	 buffer_len, seq, i, a, rx;
	char	 ackbuf[96];

	rx = FALSE;
	while (1) {
		memset(buffer, 0, MBUS_BUF_SIZE);
                assert(m->s != NULL);
		buffer_len = udp_recv(m->s, buffer, MBUS_BUF_SIZE);
		if (buffer_len > 0) {
			rx = TRUE;
		} else {
			return rx;
		}

		mbus_parse_init(m, buffer);
		/* Parse the header */
		if (!mbus_parse_sym(m, &ver)) {
			mbus_parse_done(m);
			debug_msg("Parser failed version (1): %s\n",ver);
			return FALSE;
		}
		if (strcmp(ver, "mbus/1.0") != 0) {
			mbus_parse_done(m);
			debug_msg("Parser failed version (2): %s\n",ver);
			return FALSE;
		}
		if (!mbus_parse_int(m, &seq)) {
			mbus_parse_done(m);
			debug_msg("Parser failed seq: %s\n", seq);
			return FALSE;
		}
		if (!mbus_parse_sym(m, &r)) {
			mbus_parse_done(m);
			debug_msg("Parser failed reliable: %s\n", seq);
			return FALSE;
		}
		if (!mbus_parse_lst(m, &src)) {
			mbus_parse_done(m);
			debug_msg("Parser failed seq: %s\n", src);
			return FALSE;
		}
		if (!mbus_parse_lst(m, &dst)) {
			mbus_parse_done(m);
			debug_msg("Parser failed dst: %s\n", dst);
			return FALSE;
		}
		if (!mbus_parse_lst(m, &ack)) {
			mbus_parse_done(m);
			debug_msg("Parser failed ack: %s\n", ack);
			return FALSE;
		}
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
					sprintf(ackbuf, "mbus/1.0 %d U (%s) (%s) (%d)\n", ++m->seqnum, m->addr[0], src, seq);
					udp_send(m->s, ackbuf, strlen(ackbuf));
				}
				/* ...and process the commands contained in the message */
				while (mbus_parse_sym(m, &cmd)) {
					if (mbus_parse_lst(m, &param) == FALSE) {
						debug_msg("Unable to parse mbus command paramaters...\n");
						debug_msg("cmd = %s\n", cmd);
						debug_msg("arg = %s\n", param);
						break;
					}
					m->cmd_handler(src, cmd, param, data);
				}
			}
		}
		mbus_parse_done(m);
	}
}

