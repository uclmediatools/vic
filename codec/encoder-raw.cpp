/*
 * Copyright (c) 1993-1994 The Regents of the University of California.
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
    "@(#) $Header$ (LBL)";
#endif

#include "transmitter.h"
#include "pktbuf-rtp.h"
#include "module.h"

class RawEncoder : public TransmitterModule {
 public:
	RawEncoder();
	~RawEncoder();
	void setq(int q);
	virtual int consume(const VideoFrame*);
	int command(int argc, const char*const* argv);
};

static class RawEncoderMatcher : public Matcher {
public:
	RawEncoderMatcher() : Matcher("module") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "raw") == 0)
			return (new RawEncoder);
		return (0);
	}
} framer_matcher_raw;

RawEncoder::RawEncoder() : TransmitterModule(FT_RAW)
{
}

RawEncoder::~RawEncoder()
{
	//if (tx_ != 0)
	//	tx_->flush();
}

#define HLEN (sizeof(jpeghdr) + sizeof(rtphdr))

int RawEncoder::consume(const VideoFrame* vf)
{
	YuvFrame* p = (YuvFrame*)vf;

	/* make sure the last frame is completely transmitted */
	tx_->flush();

	int tot = strlen((const char*)vf->bp_);
	int size = tot;
	int off = 0;
	int payload = tx_->mtu() - HLEN;
	while (off < size) {
		//Transmitter::pktbuf* pb = tx_->alloch(p->ts_, RTP_PT_RAW);
		pktbuf* pb = pool_->alloc(p->ts_, RTP_PT_RAW);
		/* use a rtp/jpeg header */
		//rtphdr* rh = (rtphdr*)pb->hdr;
		rtphdr* rh = (rtphdr*)pb->data;
		jpeghdr* h = (jpeghdr*)(rh + 1);

		h->width = p->width_ >> 3;
		h->height = p->height_ >> 3;

		int cc = size - off;
		if (cc > payload)
			cc = payload;
		else
			/* RTP_M = last packet for this frame */
			rh->rh_flags |= htons(RTP_M);

		h->off = htonl(off);
		//pb->iov[0].iov_len = HLEN;
		//pb->iov[1].iov_base = (caddr_t)p->bp_ + off;
		memcpy(&pb->data[HLEN],(caddr_t)p->bp_ + off, cc);
		//pb->iov[1].iov_len = cc;
		pb->len = cc + HLEN;
		/*XXX DEC C++ needs scope qualifier here */
		tx_->send(pb);
		tot += HLEN;
		off += cc;
	}
	return (tot);
}

void
RawEncoder::setq(int q)
{
	UNUSED(q); //SV-XXX: unused
}

int
RawEncoder::command(int argc, const char*const* argv)
{
    if (argc == 2 && strcmp(argv[1], "frame-format") == 0) {
		Tcl& tcl = Tcl::instance();
		tcl.result("422");
		return (TCL_OK);
    }
	if (argc == 3 && strcmp(argv[1], "q") == 0) {
		setq(atoi(argv[2]));
		return (TCL_OK);
	}
	return (TransmitterModule::command(argc, argv));
}

