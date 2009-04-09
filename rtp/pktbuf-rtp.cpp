/*
 * Copyright (c) 1996 The Regents of the University of California.
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
static const char rcsid[] =
    "@(#) $Header$";
#endif

//#include <tclcl.h>

#include "config.h"
#include "pktbuf-rtp.h"
#include "rtp.h"
#include "source.h"
#include "ntp-time.h"
#include "pktbuf.h"

/*static class RTP_BufferPoolClass : public TclClass {
public:
	RTP_BufferPoolClass() : TclClass("BufferPool/RTP") {}
	virtual TclObject* create(int argc, const char*const* argv) {
		return (new RTP_BufferPool);
	}
} rtp_bp_matcher;
	

 // *XXX
 // * Sequence numbers are static so when we change the encoding (which causes
 // * new encoder to be allocated) we don't reset the sequence counter.
 // * Otherwise, receivers will get confused, reset their stats, and generate
 // * odd looking streams of reception reports (i.e., the packet counts will
 // * drop back to 0).
 */

RTP_BufferPool::RTP_BufferPool() : srcid_((u_int32_t)-1) //SV-XXX: Debian
{
	for (int i = 0; i < NLAYER; ++i)
		seqno_[i] = 1;
}		


int RTP_BufferPool::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "srcid") == 0) {
			srcid_ = htonl(strtoul(argv[2], 0, 10));
			return (TCL_OK);
		}
	}
	
	return (BufferPool::command(argc, argv));
}

void RTP_BufferPool::seqno(u_int16_t s)
{
	/*XXX*/
	for (int i = 0; i < NLAYER; ++i)
		seqno_[i] = s;
}

void RTP_BufferPool::initpkt(pktbuf *pb, u_int32_t ts, int fmt, int layer)
{
	pb->layer = layer;
	rtphdr* rh = (rtphdr*)pb->data;
	int flags = RTP_VERSION << 14 | fmt;
	rh->rh_flags = ntohs(flags);
	rh->rh_seqno = htons(seqno_[layer]);
	++seqno_[layer];
	rh->rh_ts = htonl(ts);
	//rh->rh_ssrc = srcid_;
	rh->rh_ssrc = SourceManager::instance().localsrc()->srcid();
}
