/*
 * Copyright (c) 1991-1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and the Network Research Group at
 *      Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header$ (LBL)
 */

#ifndef vic_net_h
#define vic_net_h

#include "inet.h"
#include "Tcl.h"

struct msghdr;

class Crypt;

/* cretinous win95 #define's this...*/
#ifdef interface
#undef interface
#endif

class Network : public TclObject {
    public:
	Network();
	virtual ~Network();
	virtual int command(int argc, const char*const* argv);
	virtual void send(u_char* buf, int len);
	virtual void send(const msghdr& mh);
	virtual int recv(u_char* buf, int len, u_int32_t& from);
	inline int rchannel() const { return (rsock_); }
	inline int schannel() const { return (ssock_); }
	inline u_int32_t addr() const { return (addr_); }
	inline u_int32_t interface() const { return (local_); }
	inline int port() const { return (port_); }
	inline int ttl() const { return (ttl_); }
	inline int noloopback_broken() const { return (noloopback_broken_); }
	virtual void reset();
	static void nonblock(int fd);
	inline Crypt* crypt() const { return (crypt_); }
    protected:
	virtual void dosend(u_char* buf, int len, int fd);
	virtual int dorecv(u_char* buf, int len, u_int32_t& from, int fd);

	u_int32_t addr_;
	u_int32_t local_;
	u_short lport_;
	u_short port_;
	int ttl_;
	int rsock_;
	int ssock_;

	int noloopback_broken_;
	
	Crypt* crypt_;

	static u_char* wrkbuf_;
	static int wrkbuflen_;
	static void expand_wrkbuf(int len);
	static int cpmsg(const msghdr& mh);
};

#endif
