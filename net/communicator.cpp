/*
 * Copyright (c) 1995 The Regents of the University of California.
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

#ifndef lint
static const char rcsid[] = "@(#) $Header$ (LBL)";
#endif

#include <ctype.h>
#include "config.h"
#include "iohandler.h"
#include "vic_tcl.h"
#include "net.h"
#include "rtp.h"

class Communicator : public TclObject, public IOHandler {
public:
	Communicator();
	~Communicator();
	virtual int command(int argc, const char*const* argv);
	virtual void dispatch(int mask);
protected:
	void send(const char* s);
	char* callback_;
	Network* net_;
	u_int16_t seqno_;
	u_char* pktbuf_;
};

static class CommunicatorMatcher : public Matcher {
public:
	CommunicatorMatcher() : Matcher("communicator") {}
	TclObject* match(const char*) {
		return (new Communicator);
	}
} comm_matcher;


#define MTU 2048

Communicator::Communicator() : callback_(0), net_(0), seqno_(0)
{
	pktbuf_ = new u_char[MTU];
}

Communicator::~Communicator()
{
	delete[] callback_; //SV-XXX: Debian
	delete[] pktbuf_; //SV-XXX: Debian
}

void Communicator::dispatch(int)
{
	u_int32_t src;
	int cc = net_->recv(pktbuf_, MTU, src);
	if (cc <= 0 || callback_ == 0)
		return;

	char* p = (char*)pktbuf_;

	cc -= sizeof(struct rtphdr);
	p += sizeof(struct rtphdr);

	int n = cc - 1;
	if (p[n] != 0)
		return;
	for (int i = 0; i < n; ++i)
		if (!isprint(p[i]))
			return;

	Tcl::instance().evalf("%s %s {%s}", callback_, name(), p);
}

int Communicator::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "network") == 0) {
			if (net_ != 0)
				unlink();
			net_ = (Network*)TclObject::lookup(argv[2]);
			if (net_ != 0)
				link(net_->rchannel(), TK_READABLE);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "handler") == 0) {
			const char* proc = argv[2];
			delete[] callback_; //SV-XXX: Debian
			callback_ = new char[strlen(proc) + 1];
			strcpy(callback_, proc);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "send") == 0) {
			send(argv[2]);
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

inline u_int32_t tod90()
{
	timeval t;
	::gettimeofday(&t, 0);
	u_int32_t u = t.tv_usec;
	u = (u << 3) + u; /* x 9 */
	/* sec * 90Khz + (usec * 90Khz) / 1e6 */
	return (t.tv_sec * 90000 + (u / 100));
}

void Communicator::send(const char* s)
{
	/* XXX use rtp */
	rtphdr* rh = (rtphdr*)pktbuf_;
	/*XXX fmt=133*/
	int flags = RTP_VERSION << 14 | 133;
	rh->rh_flags = ntohs(flags);
	rh->rh_seqno = htons(seqno_);
	++seqno_;
	rh->rh_ssrc = 0;/*XXX*/

	u_int32_t ts = tod90();
	rh->rh_ts = htonl(ts);
	strcpy((char*)(rh + 1), s);
	net_->send(pktbuf_, sizeof(*rh) + strlen(s) + 1);
}
