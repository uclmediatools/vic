/*
 * Copyright (c) 1991-1994 The Regents of the University of California.
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
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef vic_net_h
#define vic_net_h

#include <sys/time.h>
#include "inet.h"
#include "vic_tcl.h"

class pktbuf;

struct msghdr;

class Crypt;

/* cretinous win95 #define's this...*/
#ifdef interface
#undef interface
#endif

#define NOT_ECT 0x00
#define ECT0    0x02
#define ECT1    0x01
#define ECN_CE  0x03


/* Generic Address class */
class Address {
public:
	Address() : text_(0) {;}
	virtual ~Address() { if (text_) delete[] text_; } //SV-XXX: Debian

	virtual int operator=(const char*) {return (0);}
	virtual Address* copy() const { return (0); }
	virtual size_t length() const { return (0); }
	virtual operator const void*() const { return (0); }

	operator const char*() const { return (const char *)text_; };
	int operator==(const Address & addr) const;

	static Address * alloc(const char * name);
	static Address * default_alloc();
	int is_set();
protected:
	char *text_;
};

class Network : public TclObject {
public:
	Network();
	Network(Address & g_addr, Address & s_addr_ssm, Address & local);
	virtual ~Network();
	virtual int command(int argc, const char*const* argv);
	virtual void send(u_char* buf, int len);
	//virtual void send(const msghdr& mh);
	virtual void send(const pktbuf* );
	virtual int recv(u_char* buf, int len, u_int32_t& from);
	virtual int recv(u_char* buf, int len, Address &from);
	inline int rchannel() const { return (rsock_); }
	inline int schannel() const { return (ssock_); }
	inline const Address & addr() const { return (g_addr_); }
	inline const Address & interface() const { return (local_); }
	inline int port() const { return (port_); }
	inline int ttl() const { return (ttl_); }
	inline int noloopback_broken() const { return (noloopback_broken_); }
	virtual void reset();
	static void nonblock(int fd);
	inline Crypt* crypt() const { return (crypt_); }
	virtual Address* alloc(const char* name) { UNUSED(name); return (0);}

	// IP_TOS
	inline u_int8_t recvd_tos() const { return (recv_tos_); }
	// SO_TIMESTAMP
	inline double recvd_so_time() const {
	return ((double) tvrecv.tv_sec + 1e-6 * (double) tvrecv.tv_usec);
	}

protected:
	virtual void dosend(u_char* buf, int len, int fd);
    virtual int dorecv(u_char* buf, int len, u_int32_t& from, int fd);
	virtual int dorecv(u_char* buf, int len, Address &from, int fd) {UNUSED(buf); UNUSED(len); UNUSED(from); UNUSED(fd); return (0);}

	Address & g_addr_; // Group or host address
	Address & s_addr_ssm_; // Src address (as in S,G) for SSM groups
	Address & local_;
	u_short lport_;
	u_short port_;
	int ttl_;
	int rsock_;
	int ssock_;

	// IP_TOS--------------
	u_int8_t recv_tos_;
	int noloopback_broken_;
	//---------------------

	// SO_TIMESTAMP--------
	struct timeval tvrecv;
	int tvlen;
	//---------------------

	Crypt* crypt_;

	static u_char* wrkbuf_;
	static int wrkbuflen_;
	static void expand_wrkbuf(int len);
	static int cpmsg(const msghdr& mh);
};
#endif
