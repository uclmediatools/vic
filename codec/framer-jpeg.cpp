/*
 * Copyright (c) 1993-1994 Regents of the University of California.
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
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include "transmitter.h"
#include "module.h"

class JpegFramer : public TransmitterModule {
 public:
	JpegFramer();
	~JpegFramer();
	virtual int consume(const VideoFrame*);
};

static class JpegFramerMatcher : public Matcher {
public:
	JpegFramerMatcher() : Matcher("module") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "jpeg/hw") == 0)
			return (new JpegFramer);
		return (0);
	}
} framer_matcher_jpeg;

JpegFramer::JpegFramer() : TransmitterModule(FT_JPEG)
{
}

JpegFramer::~JpegFramer()
{
	if (tx_ != 0)
		tx_->flush();
}

#define HLEN (sizeof(jpeghdr) + sizeof(rtphdr))

int JpegFramer::consume(const VideoFrame* vf)
{
	JpegFrame* p = (JpegFrame*)vf;

	/* make sure the last frame is completely transmitted */
	tx_->flush();

	int tot = p->len_;
	int off = 0;
	int payload = tx_->mtu() - HLEN;
	while (off < p->len_) {
		Transmitter::pktbuf* pb = tx_->alloch(p->ts_, RTP_PT_JPEG);
		rtphdr* rh = (rtphdr*)pb->hdr;
		jpeghdr* h = (jpeghdr*)(rh + 1);

		/*XXX could maintain this as one 32-bit value */
		h->q = p->q_;
		h->width = p->width_ >> 3;
		h->height = p->height_ >> 3;
		h->type = p->type_;

		int cc = p->len_ - off;
		if (cc > payload)
			cc = payload;
		else
			rh->rh_flags |= htons(RTP_M);

		h->off = htonl(off);
		pb->iov[0].iov_len = HLEN;
		pb->iov[1].iov_base = (caddr_t)p->bp_ + off;
		pb->iov[1].iov_len = cc;
		/*XXX DEC C++ needs scope qualifier here */
		tx_->send(pb);
		tot += HLEN;
		off += cc;
	}
	return (tot);
}
