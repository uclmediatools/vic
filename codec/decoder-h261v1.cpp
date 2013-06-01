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
#include "p64.h"
#include "pktbuf.h"

class RTPv1H261Decoder : public Decoder {
public:
	RTPv1H261Decoder();
	~RTPv1H261Decoder();
	int colorhist(u_int* hist) const;
	virtual void recv(pktbuf*);
	void stats(char* wrk);
protected:
	const u_char* reassemble(const u_char* bp, int& cc, u_int& seqno,
				 u_int& flags);
	virtual void redraw();

	u_short seqno_;		/* last rtp seqno */
	/*
	 * Reassembly buffer 'slots' for resequencing & reassembling
	 * gobs split across packet boundaries.
	 */
#define H261_SLOTS 64
#define H261_SLOTMASK (H261_SLOTS - 1)
	static u_char* wrkbuf_;
	static int wrkbuflen_;
	int packetsize_;	/* max size of gob fragments */
	struct slot {
		int len;
		u_short flags;
		u_short seqno;
		u_char* bp;
		int pad;
	} slot_[H261_SLOTS];
	u_char* slotbuf_;

	P64Decoder* codec_;
};

static class RTPv1H261DecoderMatcher : public Matcher {
public:
	RTPv1H261DecoderMatcher() : Matcher("decoder") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "h261v1") == 0)
			return (new RTPv1H261Decoder());
		else
			return (0);
	}
} dm_h261v1;

#define STAT_BAD_PSC	0
#define STAT_BAD_GOB	1
#define STAT_BAD_SYNTAX	2
#define STAT_BAD_FMT	3
#define STAT_BAD_SIZE	4	/* packet dropped for unsupported fmt */
#define STAT_BAD_BITOFF	5	/* unexpected non-zero sbits or ebits */
#define STAT_SIZE_CHANGE 6	/* packet dropped because frag size changed */
#define STAT_FMT_CHANGE	7	/* # times fmt changed */

u_char* RTPv1H261Decoder::wrkbuf_;
int RTPv1H261Decoder::wrkbuflen_;

RTPv1H261Decoder::RTPv1H261Decoder() : Decoder(2)
{
	stat_[STAT_BAD_PSC].name = "Bad-PSC";
	stat_[STAT_BAD_GOB].name = "Bad-GOB";
	stat_[STAT_BAD_SYNTAX].name = "Bad-Syntax";
	stat_[STAT_BAD_FMT].name = "Bad-fmt";
	stat_[STAT_BAD_SIZE].name = "Bad-size";
	stat_[STAT_BAD_BITOFF].name = "Bad-bitoff";
	stat_[STAT_SIZE_CHANGE].name = "Size-change";
	stat_[STAT_FMT_CHANGE].name = "Fmt-change";
	nstat_ = 8;

	/* cause a resize trigger later */
	inw_ = 0;
	inh_ = 0;
	decimation_ = 420;
	seqno_ = 0;
	packetsize_ = 0;
	for (int i = 0; i < H261_SLOTS; ++i) {
		slot_[i].len = 0;
		slot_[i].flags = 0;
		slot_[i].seqno = 0;
		slot_[i].bp = 0;
	}
	slotbuf_ = 0;
	codec_ = new FullP64Decoder();
	resize(codec_->width(), codec_->height());
	codec_->marks(rvts_);
}

RTPv1H261Decoder::~RTPv1H261Decoder()
{
	delete[] slotbuf_; //SV-XXX: Debian
	delete codec_;
}

void RTPv1H261Decoder::stats(char* wrk)
{
	/* pull stats out of vic indepdendent P64Decoder */
	setstat(STAT_BAD_PSC, codec_->bad_psc());
	setstat(STAT_BAD_GOB, codec_->bad_GOBno());
	setstat(STAT_BAD_SYNTAX, codec_->bad_bits());
	setstat(STAT_BAD_FMT, codec_->bad_fmt());
	Decoder::stats(wrk);
}

/*
 * Packets waiting for reassembly are kept in 'slots'
 * indexed by the low bits of the sequence number.
 * To try to avoid copying packets twice, we assume
 * that the packet source is breaking packets at some
 * fixed, max size (which we can determine from the size
 * of a packet with the SOG marker) and we allocate the buffer 
 * for slots to be this size.  As long as a packet doesn't
 * wrap, this should make the final packets contiguous.
 */
inline const u_char*
RTPv1H261Decoder::reassemble(const u_char* bp, int& cc,
			 u_int& rseqno, u_int& rflags)
{
	register int flags = rflags;
	/* XXX if the packet is complete just render it */
	if ((flags & 0x88) == 0x88)
		return (bp);

	register u_int seqno = rseqno;
	if (packetsize_ != cc) {
		if (packetsize_ == 0) {
			if ((flags & 0x80) == 0)
				return (0);
			packetsize_ = cc;
			u_int len = cc * H261_SLOTS;
			slotbuf_ = new u_char[len+4];
			u_char* bp = slotbuf_ + 4;
			for (int i = 0; i < H261_SLOTS; ++i) {
				slot_[i].bp = bp;
				bp += cc;
			}
			if (len > wrkbuflen_) {
				delete[] wrkbuf_; //SV-XXX: Debian
				wrkbuf_ = new u_char[len+4];
				wrkbuflen_ = len;
			}
		} else if ((flags & 0x08) == 0 || cc > packetsize_) {
			count(STAT_SIZE_CHANGE);
			delete[] slotbuf_; //SV-XXX: Debian
			slotbuf_ = 0;
			packetsize_ = 0;
			for (int i = 0; i < H261_SLOTS; ++i) {
				slot_[i].len = 0;
				slot_[i].flags = 0;
				slot_[i].seqno = 0;
				slot_[i].bp = 0;
			}
			return (0);
		}
	}
	register u_int l = seqno & H261_SLOTMASK;

	slot_[l].len = cc;
	slot_[l].seqno = seqno;
	slot_[l].flags = flags;
	memcpy((char*)slot_[l].bp, (char*)bp, cc);

	/*
	 * Don't bother trying to reassemble packet unless it
	 * has an EOG marker or we have the next packet.
	 */
	if ((flags & 0x08) == 0 &&
	    ((seqno + 1) & 0xffff) !=
	    slot_[(l + 1) & H261_SLOTMASK].seqno)
		return (0);

	/* scan backward for BOG */
	register u_int pseq = seqno;
	register u_int f = l;
	while ((slot_[f].flags & 0x80) == 0) {
		pseq = (pseq - 1) & 0xffff;
		f = pseq & H261_SLOTMASK;
		if (slot_[f].seqno != pseq)
			/* packet missing */
			return (0);
	}

	/* scan forward for EOG */
	while ((slot_[l].flags & 0x08) == 0) {
		seqno = (seqno + 1) & 0xffff;
		l = seqno & H261_SLOTMASK;
		if (slot_[l].seqno != seqno)
			/* packet missing */
			return (0);
	}

	if (l < f) {
		/* packet wraps around buffer, have to copy it */
		u_char* wp = wrkbuf_ + 4;
		register u_int i = f - 1;
		do {
			i = (i + 1) & H261_SLOTMASK;
			register int slen = slot_[i].len;
			memcpy((char*)wp, (char*)slot_[i].bp, slen);
			wp += slen;
		} while (i != l);
		bp = wrkbuf_ + 4;
		cc = wp - bp;
	} else {
		bp = slot_[f].bp;
		cc = slot_[l].bp - bp + slot_[l].len;
	}

	/*
	 * At this point, all we care about is ebit and sbit.
	 */
	flags = slot_[f].flags & 0x70;
	flags |= slot_[l].flags & 0x07;
	rflags = flags;
	rseqno = seqno;

	return (bp);
}

void RTPv1H261Decoder::recv(pktbuf* pb)
{	
	rtphdr* rh = (rtphdr*)pb->dp;
	u_int8_t* vh = (u_int8_t*)(rh + 1);
	/*
	 * if the packet has a non-zero sbit but no SOG or
	 * a non-zero ebit but no EOG, toss it.
	 * if the packet contains both start & end gobs we can
	 * just render it.  Otherwise we wait until we get the
	 * start and end markers and all the packets in between.
	 */
	u_int v = ntohs(*(u_int16_t*)vh);
	u_int flags = v >> 8;
	if (((flags & 0x70) != 0 && (flags & 0x80) == 0) ||
	    ((flags & 0x07) != 0 && (flags & 0x08) == 0)) {
		count(STAT_BAD_BITOFF);
		return;
	}

	int cc = pb->len - (sizeof(*rh) + 2);
	const u_int8_t* bp = vh + 2;
	u_int seqno = ntohs(rh->rh_seqno);
	bp = reassemble(bp, cc, seqno, flags);
	if (bp == 0)
		return;

	int sbit = (flags >> 4) & 7;
	int ebit = flags & 7;

	/* XXX tack on the start code */
	/*XXX*/
	cc += 2;
	bp -= 2;
	((u_char*)bp)[0] = 0;
	if (sbit == 0)
		((u_char*)bp)[1] = 1;
	else {
		((u_char*)bp)[1] = 0;
		((u_char*)bp)[2] |= 0x100 >> sbit;
	}

	codec_->mark(now_);
	(void)codec_->decode(bp, cc, sbit, ebit, 0, 0, 0, 0, 0);
	/*
	 * If the stream changes format, issue a resize.
	 */
	if (codec_->width() != inw_) {
		count(STAT_FMT_CHANGE);
		resize(codec_->width(), codec_->height());
		codec_->marks(rvts_);
	}

	/*XXX*/
	if (ntohs(rh->rh_flags) & RTP_M) {
		codec_->sync();
		ndblk_ = codec_->ndblk();
		render_frame(codec_->frame());
		codec_->resetndblk();
	}
	pb->release();
}

int RTPv1H261Decoder::colorhist(u_int* hist) const
{
	const u_char* frm = codec_->frame();
	int w = inw_;
	int h = inh_;
	int s = w * h;
	colorhist_420_556(hist, frm, frm + s, frm + s + (s >> 2), w, h);
	return (1);
}

void RTPv1H261Decoder::redraw()
{
	if (codec_ != 0)
		Decoder::redraw(codec_->frame());
}
