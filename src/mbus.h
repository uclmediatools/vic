/*
 * FILE:    mbus.h
 * AUTHORS: Colin Perkins
 * 
 * Copyright (c) 1997,1998 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted, provided that the following conditions 
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

#ifndef _MBUS_H
#define _MBUS_H

struct mbus;

struct mbus *mbus_init(void  (*cmd_handler)(char *src, char *cmd, char *arg, void *dat), 
		       void  (*err_handler)(int seqnum));
void         mbus_exit(struct mbus *m);
void         mbus_addr(struct mbus *m, char *addr);
void         mbus_qmsg(struct mbus *m, char *dest, const char *cmnd, const char *args, int reliable);
void         mbus_send(struct mbus *m);
int          mbus_recv(struct mbus *m, void *data);
void         mbus_parse_init(struct mbus *m, char *str);
void         mbus_parse_done(struct mbus *m);
int          mbus_parse_lst(struct mbus *m, char **l);
int          mbus_parse_str(struct mbus *m, char **s);
int          mbus_parse_int(struct mbus *m, int *i);
int          mbus_parse_flt(struct mbus *m, double *d);
char        *mbus_decode_str(char *s);
char        *mbus_encode_str(const char *s);
void         mbus_retransmit(struct mbus *m);
void         mbus_heartbeat(struct mbus *m, int interval);
int          mbus_waiting_ack(struct mbus *m);

#endif
