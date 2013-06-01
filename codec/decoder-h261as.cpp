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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "renderer.h"
#include "p64/p64as.h"

class H261ASDecoder : public Decoder {
    public:
	H261ASDecoder();
	virtual ~H261ASDecoder();
	virtual void info(char* wrk) const;
	virtual void recv(pktbuf*);
	int colorhist(u_int* hist) const;
	virtual void stats(char* wrk);
    protected:
	void decode(const u_char* vh, const u_char* bp, int cc);
	virtual void redraw();
	P64ASDecoder* codec_;
};

static class H261ASDecoderMatcher : public Matcher {
    public:
	H261ASDecoderMatcher() : Matcher("decoder") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "h.261as") == 0 ||
		    strcasecmp(id, "h261as") == 0)
			return (new H261ASDecoder());
		return (0);
	}
} dm_h261as;

#define STAT_BAD_PSC	0
#define STAT_BAD_GOB	1
#define STAT_BAD_SYNTAX	2
#define STAT_BAD_FMT	3
#define STAT_FMT_CHANGE 4	/* # times fmt changed */
#define STAT_BAD_HEADER 5


H261ASDecoder::H261ASDecoder() : Decoder(4), codec_(0)
{
	stat_[STAT_BAD_PSC].name = "H261AS-Bad-PSC";
	stat_[STAT_BAD_GOB].name = "H261AS-Bad-GOB";
	stat_[STAT_BAD_SYNTAX].name = "H261AS-Bad-Syntax";
	stat_[STAT_BAD_FMT].name = "H261AS-Bad-fmt";
	stat_[STAT_FMT_CHANGE].name = "H261AS-Fmt-change";
	stat_[STAT_BAD_HEADER].name = "H261AS-Bad-Header";
	nstat_ = 6;

	decimation_ = 420;
	/*
	 * Assume CIF.  Picture header will trigger a resize if
	 * we encounter QCIF instead.
	 */
	inw_ = 352;
	inh_ = 288;

	/*XXX*/
	resize(inw_, inh_);
}

H261ASDecoder::~H261ASDecoder()
{
	delete codec_;
}

void H261ASDecoder::info(char* wrk) const
{
	if (codec_ == 0)
		*wrk = 0;
	else
		sprintf(wrk, "[q=%d]", codec_->gobquant());
}

void H261ASDecoder::stats(char* wrk)
{
	/* pull stats out of vic indepdendent P64Decoder */
	setstat(STAT_BAD_PSC, codec_->bad_psc());
	setstat(STAT_BAD_GOB, codec_->bad_GOBno());
	setstat(STAT_BAD_SYNTAX, codec_->bad_bits());
	setstat(STAT_BAD_FMT, codec_->bad_fmt());
	Decoder::stats(wrk);
}

int H261ASDecoder::colorhist(u_int* hist) const
{
	const u_char* frm = codec_->frame();
	int w = inw_;
	int h = inh_;
	int s = w * h;
	colorhist_420_556(hist, frm, frm + s, frm + s + (s >> 2), w, h);
	return (1);
}


#ifdef CR_STATS
u_char shadow[640*480/64];

void dumpShadow(u_int32_t ts, u_char* p, int nblk)
{
	ts &= 0x7fffffff;
	static u_int32_t lastTS = 0;
	if (ts != lastTS) {
		printf("ts %u\n", ts);
		lastTS = ts;
	}
	int left = -1;
	for (int i = 0; i < nblk; ++i) {
		if (shadow[i] != p[i]) {
			if (left < 0) {
				left = i;
			}
		} else {
			if (left > 0)
				printf("b %d %d\n", left, i - 1);
			left = -1;
		}
	}
	if (left > 0)
		printf("b %d %d\n", left, i - 1);
}
#endif

void H261ASDecoder::recv(pktbuf* pb)
{	
	rtphdr* rh = (rtphdr*)pb->dp;
	u_int8_t* vh = (u_int8_t*)(rh + 1);
	if (codec_ == 0) {
		codec_ = new P64ASDecoder();
		codec_->marks(rvts_);
	}
#ifdef CR_STATS
	memcpy(shadow, rvts_, nblk_);
#endif
	/*
	 * h261as header is as follows:
         * ebit    3
         * quant   5
         * width  12
         * height 12 
         */
	u_int v = ntohl(*(u_int32_t*)vh);
	int ebit = v >> 29;
	int quant = (v >> 24) & 0x1f;
	int pwidth = (v >> 12) & 0x7ff;
	int pheight = v & 0x7ff;

	int cc = pb->len - (sizeof(*rh) + 4);

	pwidth = (pwidth + 1) * 16;
	pheight = (pheight + 1) * 16;

	/*
	 * If the stream changes format, issue a resize.
	 */
	if (pwidth != codec_->width() ||
	    pheight != codec_->height()) {
	        codec_->set_size(pwidth, pheight);
		resize(codec_->width(), codec_->height());
		codec_->marks(rvts_);
		count(STAT_FMT_CHANGE);
	}

	codec_->mark(now_);
	(void)codec_->decode(vh + 4, cc, ebit, quant);

#ifdef CR_STATS
	dumpShadow(ntohl(rh->rh_ts), rvts_, nblk_);
#endif
	/*XXX*/
	if (ntohs(rh->rh_flags) & RTP_M) {
		codec_->sync();
		ndblk_ = codec_->ndblk();
		render_frame(codec_->frame());
		codec_->resetndblk();
	}
	pb->release();
}

void H261ASDecoder::redraw()
{
	if (codec_ != 0)
		Decoder::redraw(codec_->frame());
}
