/*
 * FILE:    mbus.h
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

#ifndef _MBUS_H
#define _MBUS_H

#include "iohandler.h"
#include "mbus_engine.h"
#ifdef WIN32
#include <winsock.h>
#endif

#define MBUS_ADDR       0xe0ffdeef      /* 224.255.222.239 */
#define MBUS_PORT       47000
#define MBUS_BUF_SIZE   1024
#define MBUS_MAX_ADDR   10
#define MBUS_MAX_PD     10

#ifdef WIN32
extern "C" int gettimeofday(struct timeval *, struct timezone *);
#endif

struct mbus_ack {
        struct mbus_ack *next;
        struct mbus_ack *prev;
        char *srce;
        char *dest;
        char *cmnd;
        char *args;
        int seqn;
        struct timeval time;  /* Used to determine when to request retransmissions, etc... */
};

struct mbus {
        int fd;
        int channel;
        int num_addr;
        char *addr[MBUS_MAX_ADDR];
        char *parse_buffer[MBUS_MAX_PD];
        int parse_depth;
        int seqnum;
        struct mbus_ack *ack_list;
        void (*cmd_handler)(char *src, char *cmd, char *arg, void *dat);
        void (*err_handler)(int seqnum);
};
/*
class MBusHandler;

class MBusEngine {
    public: 
        inline MBusEngine(MBusHandler& mb);
        void mbus_handler_engine(char *srce, char *cmnd, char *args, void *data);
        
    private: 
        MBusHandler& mb_;
        char **mbus_cmnd;
        void (**mbus_func)(char *srce, char *args);
        u_int32_t cname2addr(char *cname);
        void func_source_playout(char *srce, char *args);
};  
*/
class MBusHandler : public IOHandler {
    private:
	struct mbus *m_;
    public:
	MBusHandler(int  channel,
		void (*cmd_handler)(char *src, char *cmd, char *arg, void *dat),
		void (*err_handler)(int seqnum) );
	~MBusHandler();
	inline struct mbus* m() const { return (m_); }
	virtual void dispatch(int mask); 
	void mbus_addr(char *addr);
	inline int mbus_fd() { return m_->fd; }
	int mbus_send(char *dest, char *cmnd, char *args, int reliable);
	void mbus_recv(void *data);
	void mbus_parse_init(char *str);
	void mbus_parse_done();
	int mbus_parse_lst(char **l);
	int mbus_parse_str(char **s);
	int mbus_parse_int(int *i);
	int mbus_parse_flt(double *d);
	char *mbus_decode_str(char *s);
	char *mbus_encode_str(char *s);
	void mbus_retransmit();

	char *mbus_audio_addr;  /* the mbus addr of the audio tool */
	int msgs;

    private:
	int mbus_addr_match(char *a, char *b);
	void mbus_ack_list_insert(char *srce, char *dest, 
			char *cmnd, char *args, int seqnum);
	void mbus_ack_list_remove(char *srce, char *dest, int seqnum);
	void mbus_send_ack(char *dest, int seqnum);
	int mbus_socket_init(int channel);
	int mbus_parse_sym(char **s);

	//MBusEngine mb_engine_;
};

#endif
