/*
 * FILE:    mbus.c
 * AUTHORS: Colin Perkins
 * Modified by Dimitrios Miras
 * 
 * Copyright (c) 1997,1998 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted, for non-commercial use only, provided
 * that the following conditions are met:
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
 * Use of this software for commercial purposes is explicitly forbidden
 * unless prior written permission is obtained from the authors.
 *
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

#include <strings.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <assert.h>
#include "mbus.h"


#define MBUS_ADDR 	0xe0ffdeef	/* 224.255.222.239 */
#define MBUS_PORT 	47000
#define MBUS_BUF_SIZE	1024
#define MBUS_MAX_ADDR	10
#define MBUS_MAX_PD	10


int MBusHandler::mbus_addr_match(char *a, char *b)
{
	while ((*a != '\0') && (*b != '\0')) {
		while (isspace(*a)) a++;
		while (isspace(*b)) b++;
		if (*a == '*') {
			a++;
			if ((*a != '\0') && !isspace(*a)) {
				return 0;
			}
			while(!isspace(*b) && (*b != '\0')) b++;
		}
		if (*b == '*') {
			b++;
			if ((*b != '\0') && !isspace(*b)) {
				return 0;
			}
			while(!isspace(*a) && (*a != '\0')) a++;
		}
		if (*a != *b) {
			return 0;
		}
		a++;
		b++;
	}
	return 1;
}



void MBusHandler::mbus_ack_list_insert(char *srce, char *dest, char *cmnd, char *args, int seqnum)
{
	struct mbus_ack	*curr = (struct mbus_ack *) malloc(sizeof(struct mbus_ack));

	assert(srce != NULL);
	assert(dest != NULL);
	assert(cmnd != NULL);
	assert(args != NULL);

	mbus_parse_init(strdup(dest));
	mbus_parse_lst(&(curr->dest));
	mbus_parse_done();

	curr->next = m_->ack_list;
	curr->prev = NULL;
	curr->cmnd = strdup(cmnd);
	curr->args = strdup(args);
	curr->seqn = seqnum;
	gettimeofday(&(curr->time), NULL);

	if (m_->ack_list != NULL) {
		m_->ack_list->prev = curr;
	}
	m_->ack_list = curr;
}

void MBusHandler::mbus_ack_list_remove(char *srce, char *dest, int seqnum)
{
	/* This would be much more efficient if it scanned from last to first, since     */
	/* we're most likely to receive ACKs in LIFO order, and this assumes FIFO...     */
	/* We hope that the number of outstanding ACKs is small, so this doesn't matter. */
	struct mbus_ack	*curr = m_->ack_list;

	while (curr != NULL) {
		if (mbus_addr_match(curr->srce, dest) && mbus_addr_match(curr->dest, srce) && (curr->seqn == seqnum)) {
			free(curr->srce);
			free(curr->dest);
			free(curr->cmnd);
			free(curr->args);
			if (curr->next != NULL) curr->next->prev = curr->prev;
			if (curr->prev != NULL) curr->prev->next = curr->next;
			if (m_->ack_list == curr) m_->ack_list = curr->next;
			free(curr);
			return;
		}
		curr = curr->next;
	}
	/* If we get here, it's an ACK for something that's not in the ACK
	 * list. That's not necessarily a problem, could just be a duplicate
	 * ACK for a retransmission... We ignore it for now...
	 */
}


void MBusHandler::mbus_send_ack(char *dest, int seqnum)
{
	char			buffer[80];
	struct sockaddr_in	saddr;
	u_long			addr = MBUS_ADDR;

	memcpy((char *) &saddr.sin_addr.s_addr, (char *) &addr, sizeof(addr));
	saddr.sin_family = AF_INET;
	saddr.sin_port   = htons(MBUS_PORT+m_->channel);
	sprintf(buffer, "mbus/1.0 %d U (%s) (%s) (%d)\n", ++m_->seqnum, m_->addr[0], dest, seqnum);
	if ((sendto(m_->fd, buffer, strlen(buffer), 0, (struct sockaddr *) &saddr, sizeof(saddr))) < 0) {
		perror("mbus_send: sendto");
	}
}

void  MBusHandler::mbus_retransmit()
{
	struct mbus_ack	 	*curr = m_->ack_list;
	struct timeval	 	 time;
	long		 	 diff;
	char			*b;
	struct sockaddr_in	 saddr;
	u_long			 addr = MBUS_ADDR;

	gettimeofday(&time, NULL);

	while (curr != NULL) {
		/* diff is time in milliseconds that the message has been awaiting an ACK */
		diff = ((time.tv_sec * 1000) + (time.tv_usec / 1000)) - ((curr->time.tv_sec * 1000) + (curr->time.tv_usec / 1000));
		if (diff > 10000) {
			printf("Reliable mbus message failed! (wait=%ld)\n", diff);
			printf(">>>\n");
			printf("   mbus/1.0 %d R (%s) %s ()\n   %s (%s)\n", curr->seqn, curr->srce, curr->dest, curr->cmnd, curr->args);
			printf("<<<\n");
			if (m_->err_handler != NULL) {
				m_->err_handler(curr->seqn);
			} else {
				abort();
			}
			abort();
		}
		if (diff > 2000) {
			memcpy((char *) &saddr.sin_addr.s_addr, (char *) &addr, sizeof(addr));
			saddr.sin_family = AF_INET;
			saddr.sin_port   = htons(MBUS_PORT+m_->channel);
			b                = malloc(strlen(curr->dest)+strlen(curr->cmnd)+strlen(curr->args)+strlen(curr->srce)+80);
			sprintf(b, "mbus/1.0 %d R (%s) %s ()\n%s (%s)\n", curr->seqn, curr->srce, curr->dest, curr->cmnd, curr->args);
			if ((sendto(m_->fd, b, strlen(b), 0, (struct sockaddr *) &saddr, sizeof(saddr))) < 0) {
				perror("mbus_send: sendto");
			}
			free(b);
		}
		curr = curr->next;
	}
}

int MBusHandler::mbus_socket_init(int channel)
{
	struct sockaddr_in sinme;
	struct ip_mreq     imr;
	char               ttl   =  0;
	int                reuse =  1;
	char               loop  =  1;
	int                fd    = -1;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("mbus: socket");
		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) < 0) {
		perror("mbus: setsockopt SO_REUSEADDR");
		return -1;
	}
#ifdef SO_REUSEPORT
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *) &reuse, sizeof(reuse)) < 0) {
		perror("mbus: setsockopt SO_REUSEPORT");
		return -1;
	}
#endif

	sinme.sin_family      = AF_INET;
	sinme.sin_addr.s_addr = htonl(INADDR_ANY);
	sinme.sin_port        = htons(MBUS_PORT+channel);
	if (bind(fd, (struct sockaddr *) & sinme, sizeof(sinme)) < 0) {
		perror("mbus: bind");
		return -1;
	}

	imr.imr_multiaddr.s_addr = MBUS_ADDR;
	imr.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &imr, sizeof(struct ip_mreq)) < 0) {
		perror("mbus: setsockopt IP_ADD_MEMBERSHIP");
		return -1;
	}

	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
		perror("mbus: setsockopt IP_MULTICAST_LOOP");
		return -1;
	}

	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
		perror("mbus: setsockopt IP_MULTICAST_TTL");
		return -1;
	}
	assert(fd != -1);
	return fd;
}


MBusHandler::MBusHandler(int  channel, 
                       void (*cmd_handler)(char *src, char *cmd, char *arg, void *dat), 
		       void (*err_handler)(int seqnum))
	: msgs(0)
{
	int i;

	m_ = (struct mbus *) malloc(sizeof(struct mbus));
	m_->fd = mbus_socket_init(channel);
	unlink();
	link(m_->fd, TK_READABLE);
	m_->channel = channel;
	m_->seqnum = 0;
	m_->ack_list = NULL;
	m_->cmd_handler = cmd_handler;
	m_->err_handler	= err_handler;
	m_->num_addr = 0;
	m_->parse_depth = 0;
	for (i=0; i<MBUS_MAX_ADDR; i++) 
		m_->addr[i] = NULL;
	for (i=0; i<MBUS_MAX_PD; i++) 
		m_->parse_buffer[i] = NULL;
	/* XXX 0 should eventually be replaced by pid */
	mbus_addr("(video engine vic 0)"); 
	mbus_audio_addr = strdup("(audio engine * *)");
}


MBusHandler::~MBusHandler()
{
	int i;

	struct mbus_ack *tmp;	
	if (m_) {
		close(m_->fd);
		if (mbus_audio_addr)
			free(mbus_audio_addr);
		for (i=0; i<MBUS_MAX_ADDR; i++)
			if (m_->addr[i])
				free(m_->addr[i]);
		for (i=0; i<MBUS_MAX_PD; i++)
			if (m_->parse_buffer[i])
				free(m_->parse_buffer[i]);
		while (m_->ack_list) {
			tmp = m_->ack_list;
			m_->ack_list = m_->ack_list->next;
			free(tmp);
		}
		free(m_);
	}
}


void MBusHandler::dispatch(int mask)
{
	mbus_recv(this);
}



void MBusHandler::mbus_addr(char *addr)
{
	assert(m_->num_addr < MBUS_MAX_ADDR);
	mbus_parse_init(strdup(addr));
	if (mbus_parse_lst(&(m_->addr[m_->num_addr]))) {
		m_->num_addr++;
	}
	mbus_parse_done();
}



int MBusHandler::mbus_send(char *dest, char *cmnd, char *args, int reliable)
{
	char			*buffer;
	struct sockaddr_in	 saddr;
	u_long			 addr = MBUS_ADDR;

	assert(dest != NULL);
	assert(cmnd != NULL);
	assert(args != NULL);
	assert(strlen(cmnd) != 0);

	m_->seqnum++;

	if (reliable) {
		mbus_ack_list_insert(m_->addr[0], dest, cmnd, args, m_->seqnum);
	}

	memcpy((char *) &saddr.sin_addr.s_addr, (char *) &addr, sizeof(addr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(MBUS_PORT+m_->channel);
	buffer = (char *) malloc(strlen(dest) + strlen(cmnd) + strlen(args) + strlen(m_->addr[0]) + 80);
	sprintf(buffer, "mbus/1.0 %d %c (%s) %s ()\n%s (%s)\n", m_->seqnum, reliable?'R':'U', m_->addr[0], dest, cmnd, args);
	if ((sendto(m_->fd, buffer, strlen(buffer), 0, (struct sockaddr *) &saddr, sizeof(saddr))) < 0) {
		perror("mbus_send: sendto");
	}
	free(buffer);
	return m_->seqnum;
}

void MBusHandler::mbus_parse_init(char *str)
{
	assert(m_->parse_depth < (MBUS_MAX_PD - 1));
	m_->parse_buffer[++m_->parse_depth] = str;
}

void MBusHandler::mbus_parse_done()
{
	m_->parse_depth--;
	assert(m_->parse_depth >= 0);
}

int MBusHandler::mbus_parse_lst(char **l)
{
	int instr = 0;
	int inlst = 0;

	*l = m_->parse_buffer[m_->parse_depth];
        while (isspace(*m_->parse_buffer[m_->parse_depth])) {
                m_->parse_buffer[m_->parse_depth]++;
        }
	if (*m_->parse_buffer[m_->parse_depth] != '(') {
		return 0;
	}
	*(m_->parse_buffer[m_->parse_depth]) = ' ';
	while (*m_->parse_buffer[m_->parse_depth] != '\0') {
		if ((*m_->parse_buffer[m_->parse_depth] == '"') && (*(m_->parse_buffer[m_->parse_depth]-1) != '\\')) {
			instr = !instr;
		}
		if ((*m_->parse_buffer[m_->parse_depth] == '(') && (*(m_->parse_buffer[m_->parse_depth]-1) != '\\') && !instr) {
			inlst = !inlst;
		}
		if ((*m_->parse_buffer[m_->parse_depth] == ')') && (*(m_->parse_buffer[m_->parse_depth]-1) != '\\') && !instr) {
			if (inlst) {
				inlst = !inlst;
			} else {
				*m_->parse_buffer[m_->parse_depth] = '\0';
				m_->parse_buffer[m_->parse_depth]++;
				return 1;
			}
		}
		m_->parse_buffer[m_->parse_depth]++;
	}
	return 0;
}

int MBusHandler::mbus_parse_str(char **s)
{
        while (isspace(*m_->parse_buffer[m_->parse_depth])) {
                m_->parse_buffer[m_->parse_depth]++;
        }
	if (*m_->parse_buffer[m_->parse_depth] != '"') {
		return 0;
	}
	*s = m_->parse_buffer[m_->parse_depth]++;
	while (*m_->parse_buffer[m_->parse_depth] != '\0') {
		if ((*m_->parse_buffer[m_->parse_depth] == '"') && (*(m_->parse_buffer[m_->parse_depth]-1) != '\\')) {
			m_->parse_buffer[m_->parse_depth]++;
			*m_->parse_buffer[m_->parse_depth] = '\0';
			m_->parse_buffer[m_->parse_depth]++;
			return 1;
		}
		m_->parse_buffer[m_->parse_depth]++;
	}
	return 0;
}

int MBusHandler::mbus_parse_sym(char **s)
{
        while (isspace(*m_->parse_buffer[m_->parse_depth])) {
                m_->parse_buffer[m_->parse_depth]++;
        }
	if (!isalpha(*m_->parse_buffer[m_->parse_depth])) {
		return 0;
	}
	*s = m_->parse_buffer[m_->parse_depth]++;
	while (!isspace(*m_->parse_buffer[m_->parse_depth]) && (*m_->parse_buffer[m_->parse_depth] != '\0')) {
		m_->parse_buffer[m_->parse_depth]++;
	}
	*m_->parse_buffer[m_->parse_depth] = '\0';
	m_->parse_buffer[m_->parse_depth]++;
	return 1;
}

int MBusHandler::mbus_parse_int(int *i)
{
	char	*p;
	*i = strtol(m_->parse_buffer[m_->parse_depth], &p, 10);

	if (p == m_->parse_buffer[m_->parse_depth]) {
		return 0;
	}
	if (!isspace(*p) && (*p != '\0')) {
		return 0;
	}
	m_->parse_buffer[m_->parse_depth] = p;
	return 1;
}

int MBusHandler::mbus_parse_flt(double *d)
{
	char	*p;
	*d = strtod(m_->parse_buffer[m_->parse_depth], &p);

	if (p == m_->parse_buffer[m_->parse_depth]) {
		return 0;
	}
	if (!isspace(*p) && (*p != '\0')) {
		return 0;
	}
	m_->parse_buffer[m_->parse_depth] = p;
	return 1;
}

char *MBusHandler::mbus_decode_str(char *s)
{
	int   l = strlen(s);

	/* Check that this an encoded string... */
	assert(s[0]   == '\"');
	assert(s[l-1] == '\"');

	bcopy(s+1, s, l-2);
	s[l-2] = '\0';
	return s;
}

char *MBusHandler::mbus_encode_str(char *s)
{
	static char	*encode_buffer = NULL;
	static int	 encode_buflen = 0;

	int l = strlen(s);
	if (encode_buflen < l) {
		if (encode_buffer != NULL) {
			free(encode_buffer);
		}
		encode_buflen = l+3;
		encode_buffer = (char *) malloc(encode_buflen);
	}
	strcpy(encode_buffer+1, s);
	encode_buffer[0]   = '\"';
	encode_buffer[l+1] = '\"';
	encode_buffer[l+2] = '\0';
	return encode_buffer;
}

void MBusHandler::mbus_recv(void *data)
{
	char	*ver, *src, *dst, *ack, *r, *cmd, *param;
	char	 buffer[MBUS_BUF_SIZE];
	int	 buffer_len, seq, i, a;

	memset(buffer, 0, MBUS_BUF_SIZE);
	if ((buffer_len = recvfrom(m_->fd, buffer, MBUS_BUF_SIZE, 0, NULL, NULL)) <= 0) {
		return;
	}
	msgs++;
	mbus_parse_init(buffer);
	/* Parse the header */
	if (!mbus_parse_sym(&ver) || (strcmp(ver, "mbus/1.0") != 0)) {
		mbus_parse_done();
		return;
	}
	if (!mbus_parse_int(&seq)) {
		mbus_parse_done();
		return;
	}
	if (!mbus_parse_sym(&r)) {
		mbus_parse_done();
		return;
	}
	if (!mbus_parse_lst(&src)) {
		mbus_parse_done();
		return;
	}
	if (!mbus_parse_lst(&dst)) {
		mbus_parse_done();
		return;
	}
	if (!mbus_parse_lst(&ack)) {
		mbus_parse_done();
		return;
	}
	/* Check if the message was addressed to us... */
	for (i = 0; i < m_->num_addr; i++) {
		if (mbus_addr_match(m_->addr[i], dst)) {
			/* ...if so, process any ACKs received... */
			mbus_parse_init(ack);
			while (mbus_parse_int(&i)) {
				mbus_ack_list_remove(src, dst, i);
			}
			mbus_parse_done();
			/* ...if an ACK was requested, send one... */
			if (strcmp(r, "R") == 0) {
				mbus_send_ack(src, seq);
			}
			/* ...and process the commands contained in the message */
			while (mbus_parse_sym(&cmd)) {
				if (mbus_parse_lst(&param) == 0) {
					break;
				}
				m_->cmd_handler(src, cmd, param, data);
			}
		}
	}
	mbus_parse_done();
}

