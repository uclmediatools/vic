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
#include "p64/p64.h"

class H261Decoder : public Decoder {
    public:
	H261Decoder();
	virtual ~H261Decoder();
	virtual void info(char* wrk) const;
	virtual void recv(pktbuf*);
	int colorhist(u_int* hist) const;
	virtual void stats(char* wrk);
    protected:
	void decode(const u_char* vh, const u_char* bp, int cc);
	virtual void redraw();
	P64Decoder* codec_;
	int h261_rtp_bug_;
};

static class H261DecoderMatcher : public Matcher {
    public:
	H261DecoderMatcher() : Matcher("decoder") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "h.261") == 0 ||
		    strcasecmp(id, "h261") == 0)
			return (new H261Decoder());
		return (0);
	}
} dm_h261;

#define STAT_BAD_PSC	0
#define STAT_BAD_GOB	1
#define STAT_BAD_SYNTAX	2
#define STAT_BAD_FMT	3
#define STAT_FMT_CHANGE 4	/* # times fmt changed */
#define STAT_BAD_HEADER 5


H261Decoder::H261Decoder() : Decoder(4), codec_(0), h261_rtp_bug_(0)
{
	stat_[STAT_BAD_PSC].name = "H261-Bad-PSC";
	stat_[STAT_BAD_GOB].name = "H261-Bad-GOB";
	stat_[STAT_BAD_SYNTAX].name = "H261-Bad-Syntax";
	stat_[STAT_BAD_FMT].name = "H261-Bad-fmt";
	stat_[STAT_FMT_CHANGE].name = "H261-Fmt-change";
	stat_[STAT_BAD_HEADER].name = "H261-Bad-Header";
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

H261Decoder::~H261Decoder()
{
	delete codec_;
}

void H261Decoder::info(char* wrk) const
{
	if (codec_ == 0)
		*wrk = 0;
	else
		sprintf(wrk, "[q=%d]", codec_->gobquant());
}

void H261Decoder::stats(char* wrk)
{
	/* pull stats out of vic indepdendent P64Decoder */
	setstat(STAT_BAD_PSC, codec_->bad_psc());
	setstat(STAT_BAD_GOB, codec_->bad_GOBno());
	setstat(STAT_BAD_SYNTAX, codec_->bad_bits());
	setstat(STAT_BAD_FMT, codec_->bad_fmt());
	Decoder::stats(wrk);
}

int H261Decoder::colorhist(u_int* hist) const
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

void H261Decoder::recv(pktbuf* pb)
{	
	rtphdr* rh = (rtphdr*)pb->dp;
	u_int8_t* vh = (u_int8_t*)(rh + 1);
	if (codec_ == 0) {
		if ((vh[0] & 2) != 0)
			codec_ = new IntraP64Decoder();
		else
			codec_ = new FullP64Decoder();
		codec_->marks(rvts_);
	}
#ifdef CR_STATS
	memcpy(shadow, rvts_, nblk_);
#endif
	/*XXX*/
	u_int v = ntohl(*(u_int32_t*)vh);
	int sbit = v >> 29;
	int ebit = (v >> 26) & 7;
	int quant = (v >> 10) & 0x1f;
	int mvdh = (v >> 5) & 0x1f;
	int mvdv = v & 0x1f;
	int mba, gob;
	/*
	 * vic-2.7 swapped the GOB and MBA fields in the RTP packet header
	 * with respect to the spec.  To maintain backward compat, whenever
	 * we see an out of range gob, we change our assumption about the
	 * stream and continue.
	 */
	if (!h261_rtp_bug_) {
		mba = (v >> 15) & 0x1f;
		gob = (v >> 20) & 0xf;
		if (gob > 12) {
			h261_rtp_bug_ = 1;
			mba = (v >> 19) & 0x1f;
			gob = (v >> 15) & 0xf;
		}
	} else {
		mba = (v >> 19) & 0x1f;
		gob = (v >> 15) & 0xf;
		if (gob > 12) {
			h261_rtp_bug_ = 0;
			mba = (v >> 15) & 0x1f;
			gob = (v >> 20) & 0xf;
		}
	}

	if (gob > 12) {
		count(STAT_BAD_HEADER);
		pb->release();
		return;
	}
	int cc = pb->len - (sizeof(*rh) + 4);
	codec_->mark(now_);
	(void)codec_->decode(vh + 4, cc, sbit, ebit,
			     mba, gob, quant, mvdh, mvdv);
#ifdef CR_STATS
	dumpShadow(ntohl(rh->rh_ts), rvts_, nblk_);
#endif
	/*
	 * If the stream changes format, issue a resize.
	 */
	if (codec_->width() != inw_) {
		resize(codec_->width(), codec_->height());
		codec_->marks(rvts_);
		count(STAT_FMT_CHANGE);
	}

	/*XXX*/
	if (ntohs(rh->rh_flags) & RTP_M) {
		codec_->sync();
		ndblk_ = codec_->ndblk();
		render_frame(codec_->frame(),rh->frame_no);
		codec_->resetndblk();
	}
	pb->release();
}

void H261Decoder::redraw()
{
	if (codec_ != 0)
		Decoder::redraw(codec_->frame());
}
