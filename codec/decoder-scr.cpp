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

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

//#include <osfcn.h>
#include <string.h>
#include <sys/param.h>
#include "inet.h"
#include "packet.h"
#include "decoder.h"
#include "rcvr.h"
#include "bsd-endian.h"

#ifdef SCR_CODEC

class ScrReceiver : public Receiver {
public:
	ScrReceiver(const nvhdr* ph);
	virtual int recv(const rtphdr*, const u_char* data,
			 int len, int jitter);
};

class ScrDecoder : public PlaneDecoder {
public:
	ScrDecoder(Receiver&);
protected:
	void decode(const u_char* vh, const u_char* bp, int cc);
	const u_char* decode_block(const u_char* data, const u_char* end);
	int recv(rtphdr*, u_char* vh, u_char* pkt, int len, int jitter);

};

static class ScrReceiverMatcher : public ReceiverMatcher {
 protected:
	Receiver* match(int fmt, const u_char* vh) {
		if (fmt == RTP_PT_SCR)
			return (new ScrReceiver((const nvhdr*)vh));
		else
			return (0);
	}
} rcvr_scr_;

static class ScrSoftwareDecoderMatcher : public SoftwareDecoderMatcher {
 public:
	ScrSoftwareDecoderMatcher() : SoftwareDecoderMatcher(RTP_PT_SCR) { }
 protected:
	SoftwareDecoder* create(Receiver& r) {
		return (new ScrDecoder(r));
	}
} swd_scr_;

#define STAT_BAD_GEOM 0
#define STAT_BAD_COORD 1

ScrReceiver::ScrReceiver(const nvhdr* ph) : Receiver(sizeof(*ph))
{
	stat_[STAT_BAD_GEOM].name = "Scr-Bad-Geom";
	stat_[STAT_BAD_COORD].name = "Scr-Bad-Coord";
	nstat_ = 2;

	inw_ = ntohs(ph->width) & 0x7fff;
	inh_ = ntohs(ph->height);
	if ((unsigned)inw_ > 1000 || (unsigned)inh_ > 1000) {
		/*XXX*/
		inw_ = 320;
		inh_ = 240;
		count(STAT_BAD_GEOM);
	}
}

int ScrReceiver::recv(const rtphdr* rh, const u_char* data, int len, int jitter)
{
	const nvhdr* ph = (nvhdr*)(rh + 1);
	int w = ntohs(ph->width) & 0x7fff;
	int h = ntohs(ph->height);
	if (w != inw_ || h != inh_) {
		if ((unsigned)w > 1000 || (unsigned)h > 1000) {
			/*XXX*/
			w = 320;
			h = 240;
			count(STAT_BAD_GEOM);
		}
		resize(w, h);
	}
	return (Receiver::recv(rh, data, len, jitter));
}

ScrDecoder::ScrDecoder(Receiver& r) : PlaneDecoder(r)
{
}

const u_char* ScrDecoder::decode_block(const u_char* bp, const u_char* end)
{
	/*XXX check for whole block here*/
	if (end - bp < 2)
		return (end);

	int x0 = bp[0];
	int y0 = bp[1];
	bp += 4;

	int inw = rcvr_.width();
	int inh = rcvr_.height();
	if (x0 > inw / 8 || y0 >= inh / 8) {
		/*XXX*/
		return 0;
	}

	int width = rcvr_.width();
	int offset = y0 * 8 * width + x0 * 8;
	n_long* yp = (n_long*)&frm_[offset];
	int s = inw * inh;/*XXX*/
	u_char* up = frm_ + s + (offset >> 1);
	u_char* vp = up + (s >> 1);
	/*XXX*/
	if (bp + 128 > end) {
		/*XXX*/
		return (end);
	}
	for (int i = 8; --i >= 0; ) {
		yp[0] = *(u_int*)bp;
		bp += 4;
		yp[1] = *(u_int*)bp;
		bp += 4;

		if (up != 0) {
			*(u_int*)up = *(u_int*)bp;
			bp += 4;
			*(u_int*)vp = *(u_int*)bp;
			bp += 4;
			up += width / 2 / sizeof(*up);
			vp += width / 2 / sizeof(*vp);
		}
		yp += width / sizeof(*yp);
	}
	rvts_[RV_INDEX(x0, y0)] = now_;
	return (bp);
}

void ScrDecoder::decode(const u_char* vh, const u_char* p, int len)
{	
	const u_char* end = p + len;
	while (p < end && p != 0)
		p = decode_block(p, end);
}
#endif
