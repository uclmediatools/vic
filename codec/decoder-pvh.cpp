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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the MASH Research
 *	Group at the University of California, Berkeley and the Network
 *	Research Group at Lawrence Berkeley National Laboratory.
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
static const char rcsid[] =
    "@(#) $Header$";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "config.h"
#include "decoder.h"
#include "dct.h"
#include "huffman.h"
#include "pktbuf.h"
extern "C" {
#include "pvh-huff.h"
           }

#include "pvh.h"

#define LINEPAD 8

#ifdef INT_64
#define SET_BIT_64(k, m0, m1) \
		(m0) |= (INT_64)1 << (k)
#else
#define SET_BIT_64(k, m0, m1) \
		if ((k) < 32) \
			(m0) |= 1 << (k); \
		else \
			(m1) |= 1 << ((k) - 32);
#endif

/*
 * True iff there are bits left in the bit-buffer
 * in the indicated level data structure.
 */
#define BB_EOB(lp) ((((lp)->es - (lp)->bs) << 3) + (lp)->nbb <= (lp)->ebit)
#define BB_NBIT(lp) ((((lp)->es - (lp)->bs) << 3) + (lp)->nbb - (lp)->ebit)

#define NRB 128

struct pvh_resump {
	int off;
	int channel;
};

struct pkt_window;

/*
 * Receive-buffer entry that we wrap around each pktbuf.
 */
struct rbent {
	rbent* next;
	int sblk;
	int eblk;
	int base;
	u_int seqno;
	pktbuf* pb;
	int nrsmp;
	pvh_resump rsmp[NLAYER]; /* resumption pointers if base layer pkt */

	/* dynamic bit decoding state */
	int nbb;
	u_int bb;
	const u_char* bs;
	const u_char* es;
	int ebit;
};

/*
 * Frame entry.  All the packets of the same video frame get
 * grouped together using this data structure so we can
 * easily carry out garbage collection.
 */
struct pkt_window {
	int first;		/* RTP seqno of first packet in window */
	int last;		/* RTP seqno of last packet in window */
	rbent* pkts[NRB];	/* place to store packets */
	pkt_window* next;
};

#define NFE 2
struct frment {
	u_int32_t ts;
	rbent* blpkts;	/* linked list of base-layer pkts */
	pkt_window* win[NLAYER];
};

#define PVH_GET_BITS(n, lp, v) \
	GET_BITS(n, lp->nbb, lp->bb, lp->bs, v)

class PvhDecoder : public Decoder {
 public:
	PvhDecoder();
	~PvhDecoder();
	virtual void recv(pktbuf*);
	void resize(int width, int height);
	int colorhist(u_int* hist) const;
	int command(int argc, const char*const* argv);
 protected:
	void decode_slice(frment*, rbent*);
	virtual void redraw();
	void sync();

#ifdef INT_64
	void parse_block(rbent*, short* blk, INT_64* mask);
#else
	void parse_block(rbent*, short* blk, u_int* mask);
#endif
	void refine_old(rbent*, short* blk, const u_int* mask, int off);
	void refine_new(rbent*, short* blk, u_int* mask, int off);
	void bias_to_midstep(short* blk, int bias, u_int* mask);
	void decode_block(int x, int y);
	void decode_lum(int8_t*);
	void decode_chm(u_int8_t* out, int stride);
	void decode_dct(spatial_hierarchy*, u_int8_t* out, int stride);
	void decode_dct_base(rbent*, int bit);
	void decode_dct_refinement(rbent* lp, int bit, int nr);
	void reconstruct_block(int x, int y);
	int decode_resumption_ptrs(rbent*, pvh_resump*);
	void decode_comp_layout(spatial_hierarchy* sh, rbent* base);
	void setup_layers(frment* fe, rbent* rb);
	void adjust_sh(spatial_hierarchy* sh);

	int dc_;/*XXX*/
	rbent* channels_[NLAYER];
	spatial_hierarchy sh_[NCOMP];

	/*XXX can make 8-bits with DCT smarts */
    	int16_t coef_[64];
	u_int mask_[2];
	int dct_bit_;
	int sbc_bit_;

	/*XXX*/
	int8_t* level0_;
	int8_t* ll_;
	int8_t* lh_;
	int8_t* hl_;
	int8_t* hh_;
	/*XXX*/
	u_int8_t* blk_;
	u_int8_t* frm_;

	frment fe_[NFE];

	u_int decode_tree(rbent*);
	void decode_sbc(const spatial_hierarchy*, int8_t* out);
	void decode_sbc_base(rbent*, int8_t* out, u_int mask);
	void decode_sbc_refinement(rbent*, int8_t* p, u_int m, int bit);
	void bias_sbc_to_midstep(int8_t* p, u_int m, int bit);

	void free_window(pkt_window*);
	pkt_window* alloc_window();
	rbent* alloc_layer();
	void free_layer(rbent*);
	void free_rb(rbent* rb);
	rbent* alloc_rb(pktbuf*);
	static pkt_window* free_window_;
	static rbent* free_rb_;

	rbent* add_packet(frment*, rbent*);
	void add_base_packet(frment* fe, rbent* rb);
	void free_slice(frment*, rbent*);
	frment* lookup_frame(u_int32_t ts);
	rbent* lookup_base_packet(frment* fe, rbent* rb) const;
	void extend_window(pkt_window* win, rbent* rb);
	void free_frment(frment* fe);
	int layer_ready(frment* fe, rbent* rb, int layer) const;
	int slice_ready(frment* fe, rbent* rb) const;
	void salvage_base_packets(frment*);
	rbent* find_first_packet(frment* fe, rbent* bae, int layer) const;

	void init_bbs(rbent*);
	void check_bbs(frment* fe, rbent* base, int blkno);
	int maxChannel_;
};

static class PvhDecoderClass : public Matcher {
public:
	PvhDecoderClass() : Matcher("decoder") {}
	//TclClass("Module/VideoDecoder/PVH") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "pvh") == 0)
			return (new PvhDecoder());
		else
			return (0);
	}
} decoder_matcher_pvh;

#define STAT_BAD_SYNTAX	0
#define STAT_BAD_GEOM	1
#define STAT_BAD_REFBIT	2

pkt_window* PvhDecoder::free_window_ = 0;
rbent* PvhDecoder::free_rb_ = 0;

PvhDecoder::PvhDecoder() : Decoder(sizeof(pvhhdr)),
	level0_(0), blk_(0), frm_(0), maxChannel_(1)
{
memset(channels_, 0, sizeof(channels_));
memset(fe_, 0, sizeof(fe_));
//printf("PvhDecoder: fix fe init\n");

	stat_[STAT_BAD_SYNTAX].name = "Pvh-Bad-Syntax";
	stat_[STAT_BAD_GEOM].name = "Pvh-Bad-Geom";
	stat_[STAT_BAD_REFBIT].name = "Pvh-Bad-Refbits";
	nstat_ = 3;
	decimation_ = 420;

	/*XXX*/
	inw_ = 320;
	inh_ = 240;

	resize(inw_, inh_);

	//bind("maxChannel_", &maxChannel_);
}

PvhDecoder::~PvhDecoder()
{
	delete[] level0_;
	delete[] frm_;
}

int
PvhDecoder::command(int argc, const char*const* argv)
{
	if (argc == 3 && strcmp(argv[1], "maxChannel") == 0) {
		maxChannel_=atoi(argv[2]);
		return (TCL_OK);
	}
	return (Decoder::command(argc, argv));
}

void PvhDecoder::resize(int width, int height)
{
	delete[] frm_;
	int size = width * height;
	/*XXX 4:1:1*/
	frm_ = new u_char[2 * size];
	/* 
	 * Initialize image to gray.
	 */
	memset(frm_, 0x80, 2 * size);

	/*XXX*/
	int w = width >> 1;
	int h = height >> 1;
	int s = (w + LINEPAD) * (h + 2);
	delete[] level0_;
#ifdef HAVE_HH
	int ss = 4 * s;
#else
	int ss = 3 * s;
#endif
	level0_ = new int8_t[ss];
	memset(level0_, 0, ss);
	int off = w + LINEPAD / 2;
	ll_ = level0_ + 0 * s + off;
	lh_ = level0_ + 1 * s + off;
	hl_ = level0_ + 2 * s + off;
#ifdef HAVE_HH
	hh_ = level0_ + 3 * s + off;
#endif
	memset(ll_, 0x80, w * h);

	delete[] blk_;
	blk_ = new u_char[(width >> 4) * (height >> 4)];
	memset(blk_, 0, (width >> 4) * (height >> 4));

	Decoder::resize(width, height);
}

/*XXX make sure there is enough slack bit buffer to walk
 * off end here (so that we don't need to add explicit checks)
 */
/*
 * Refinement pass: update each coefficient that has already been
 * received with the next bit of precision.  Each bits is sent
 * as is (this is little opportunity to compress these bits as
 * they are more or less random).
 */
void PvhDecoder::refine_old(rbent* rb, short* blk, const u_int* mask, int bit)
{
	u_int m0 = mask[0];
	u_int m1 = mask[1];
	m0 &=~ 1;
	bit = 1 << bit;
	const u_char* zp = &COLZAG[0];
	for (int i = 8; --i >= 0; ) {
		if ((m0 & 0xff) != 0) {
			for (int k = 8; --k >= 0; ++zp, m0 >>= 1) {
				if ((m0 & 1) == 0)
					continue;
				int v;
				PVH_GET_BITS(1, rb, v);
				if (v != 0)
					blk[*zp] |= bit;
			}
		} else {
			m0 >>= 8;
			zp += 8;
		}
#ifndef INT_64
		m0 |= m1 << 24;
		m1 >>= 8;
#endif
	}
}

static u_int8_t bitcnt[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};


/*
 * Parse the new DCT coefficients that become sigficant on this
 * pass (at the digit position indicated by "bit").  We are sending
 * exactly one bit of precision of each new coefficient (and therefore
 * we know that bit must be 1, otherwise, the coefficient would not
 * be significant).  But we do not know the sign, so it is explicit
 * in the stream and we read it off.  The coefficient positions are
 * simply run length encoded (using Huffman codes for the runs).
 * Note that the run lengths do not include the slots taken up by
 * coefficients that have already been found to be significant.
 */
void PvhDecoder::refine_new(rbent* rb, short* blk, u_int* mask, int bit)
{
	u_int m0 = mask[0];
	u_int m1 = mask[1];
	u_int o0 = m0;
	u_int o1 = m1;
	int pos = 0;
	m0 &=~ 1;
	bit = 1 << bit;
	for (;;) {
		int runlen, sign;
		/*XXX*/
		HUFF_DECODE_PVH_RC(rb->nbb, rb->bb, rb->bs, runlen);
		if (runlen == 0) {
			mask[0] = o0;
			mask[1] = o1;
			return;
		}
		if (runlen >= 9) {
			if (runlen == 9) {
				PVH_GET_BITS(6, rb, runlen);
			} else {
				/* illegal codeword */
				count(STAT_BAD_REFBIT);
				return;
			}
		}
		PVH_GET_BITS(1, rb, sign);
		do {
			int n = runlen;
			if (n > 8)
				n = 8;
			int m = (1 << n) - 1;
			/*
			 * ignore bit positions that correspond
			 * to coefficients that have already been
			 * received.
			 */
			runlen += bitcnt[m0 & m];

			runlen -= n;
			pos += n;
			m0 >>= n;
			m0 |= m1 << (32 - n);
			m1 >>= n;
		} while (runlen > 0);

		/*
		 * skip over any contiguous bits
		 */
		while ((m0 & 1) != 0) {
			++pos;
			m0 >>= 1;
			m0 |= m1 << 31;
			m1 >>= 1;
		}
		if (pos >= 64) {
			count(STAT_BAD_REFBIT);
			return;
		}
		blk[COLZAG[pos]] = bit | sign << 15;
		SET_BIT_64(pos, o0, o1);
	}
}

/*
 * Decode the next block of transform coefficients.
 */
#ifdef INT_64
void PvhDecoder::parse_block(rbent* lp, short* blk, INT_64* mask)
#else
void PvhDecoder::parse_block(rbent* lp, short* blk, u_int* mask)
#endif
{
#ifdef INT_64
	INT_64 m0 = 1;
#else
	u_int m1 = 0, m0 = 1;
#endif
	/*
	 * Cache bit buffer in registers.
	 */
	register int nbb = lp->nbb;
	register u_int bb = lp->bb;

	int k, v;
	GET_BITS(8, nbb, bb, lp->bs, v);
	v <<= 3;
	blk[0] = v;
	k = 1;
	for (;;) {
		int r, v;
		HUFF_DECODE_PVH_TC(nbb, bb, lp->bs, r);
		if (r & 1) {
			/* EOB, ESCAPE, or an illegal symbol */
			if (r == 5) {
				/* ESCAPE */
				GET_BITS(14, nbb, bb, lp->bs, r);
				v = (r & 0x7f) << 5 | (r & 0x80) << 8;
				r >>= 8;
			} else {
				if (r == 1)
					/*
					 * illegal symbol:
					 * bitstream is corrupt
					 */
					count(STAT_BAD_SYNTAX);

				/* otherwise EOB */
				break;
			}
		} else {
			r >>= 1;
			v = (r & 0xf) << 5; /* XXX assume q=16 */
			v |= (r & 0x10) << 11; /* sign bit */
			r >>= 5;
		}
		k += r;
		if (k >= 64) {
			count(STAT_BAD_SYNTAX);
			break;
		}
		r = COLZAG[k];
		blk[r] = v;
		SET_BIT_64(k, m0, m1);
		++k;
	}
	*mask = m0;
#ifndef INT_64
	mask[1] = m1;
#endif
	/*
	 * Done reading input.  Update bit buffer.
	 */
	lp->bb = bb;
	lp->nbb = nbb;
}

void PvhDecoder::bias_to_midstep(short* blk, int bias, u_int* mask)
{
	u_int m0 = mask[0];
	u_int m1 = mask[1];
	u_int o0 = 1;
	u_int o1 = 0;

	m0 &=~ 1;
	const u_char* zp = &COLZAG[0];
	for (int i = 8; --i >= 0; ) {
		if ((m0 & 0xff) != 0) {
			for (int k = 8; --k >= 0; ++zp, m0 >>= 1) {
				if ((m0 & 1) == 0)
					continue;
				int j = *zp;
				int v = blk[j];
				v += bias;
				if ((v & 0x8000) != 0)
					v = -(v & 0x7fff);
				blk[j] = v;
				SET_BIT_64(j, o0, o1);
			}
		} else {
			m0 >>= 8;
			zp += 8;
		}
#ifndef INT_64
		m0 |= m1 << 24;
		m1 >>= 8;
#endif
	}
	/* store back block-order version */
	mask[0] = o0;
	mask[1] = o1;
}

void PvhDecoder::decode_dct_base(rbent* lp, int bit)
{
	/*XXX pull out base-layer quantizer (we assume 5) */
	if (bit != 5)
		/*XXX should bump an error counter*/
		return;

	mask_[0] = 0;
	mask_[1] = 0;
#ifdef INT_64
	parse_block(lp, coef_, (INT_64 *)mask_); //SV-XXX: Debian
#else
	parse_block(lp, coef_, mask_);
#endif
}

void PvhDecoder::decode_dct_refinement(rbent* lp, int bit, int nr)
{
	while (--nr >= 0) {
		refine_old(lp, coef_, mask_, bit);
		refine_new(lp, coef_, mask_, bit);
		--bit;
	}
}

/*16x16*/
void PvhDecoder::reconstruct_block(int x, int y)
{
	int stride = (inw_ + LINEPAD) >> 1;
	int off = (y >> 1) * stride + (x >> 1);
	int16_t blk[2*10*16];

	int16_t* o = blk;
	u_int8_t* lu = (u_int8_t*)ll_ + off - stride;
	int8_t* h = lh_ + off - stride;
	o = blk;
	int th;
	for (th = 10; --th >= 0; ) {
		int l0 = lu[-1];
		int h0 = h[-1];
		int l1 = *lu++;
		int h1 = *h++;
		for (int w = 8; --w >= 0; ) {
			int l3 = l1 + (l1 << 1);
			int h3 = h1 + (h1 << 1);
			int v = l0 + l3 + h0 - h3;
			*o++ = v;

			l0 = l1;
			h0 = h1;
			l1 = *lu++;
			h1 = *h++;

			v = l3 + l1 + h3 - h1;
			*o++ = v;
		}
		lu += stride - 9;
		h += stride - 9;
	}
	int8_t* l = hl_ + off - stride;
#ifdef HAVE_HH
	h = hh_ + off - stride;
#endif
	for (th = 10; --th >= 0; ) {
		int l0 = l[-1];
		int h0 = h[-1];
		int l1 = *l++;
#ifdef HAVE_HH
		int h1 = *h++;
#else
		int h1 = 0;
#endif
		for (int w = 8; --w >= 0; ) {
			int l3 = l1 + (l1 << 1);
			int h3 = h1 + (h1 << 1);
			int v = l0 + l3 + h0 - h3;
			*o++ = v;

			l0 = l1;
			h0 = h1;
			l1 = *l++;
#ifdef HAVE_HH
			h1 = *h++;
#endif
			v = l3 + l1 + h3 - h1;
			*o++ = v;
		}
		l += stride - 9;
#ifdef HAVE_HH
		h += stride - 9;
#endif
	}

	o = blk;
	off = y * inw_ + x;
	u_int8_t* out = frm_ + off;
	for (int w = 16; --w >= 0; ) {
		const int16_t* cl = o++;
		const int16_t* ch = cl + 10*16;
		u_int8_t* p = out++;

		int l0 = *cl;
		cl += 16;
		int h0 = *ch;
		ch += 16;
		int l1 = *cl;
		cl += 16;
		int h1 = *ch;
		ch += 16;
		
		for (int tk = 8; --tk >= 0; ) {
			int l3 = l1 + (l1 << 1);
			int h3 = h1 + (h1 << 1);
			int v = l3 + l0 + h0 - h3;
			v = (v + 7) >> 4;
			/*XXX*/
			if (v & ~0xff)
				v = (v < 0) ? 0 : 255;
			*p = v;
			p += inw_;

			l0 = l1;
			h0 = h1;
			l1 = *cl;
			cl += 16;
			h1 = *ch;
			ch += 16;

			v = l1 + l3 + h3 - h1;
			v = (v + 7) >> 4;
			/*XXX*/
			if (v & ~0xff)
				v = (v < 0) ? 0 : 255;
			*p = v;
			p += inw_;
		}
	}
}

/*
 * XXX tune this.  can use small table lookups, or larger
 * table for multiple coefficients in parallel.
 */
#define DECODE_COEF(lp, cf, bb, nbb) \
{ \
	int t; \
	GET_BITS(1, nbb, bb, lp->bs, t); \
	if (t != 0) { \
		int sign; \
		GET_BITS(2, nbb, bb, lp->bs, sign); \
		if (sign & 2) { \
			sign &= 1; \
			GET_BITS(3, nbb, bb, lp->bs, t); \
			t <<= 4; \
		} else { \
			t = 1 << 4; \
		} \
		*(cf) = t | sign << 7; \
	} \
}

void PvhDecoder::decode_sbc_base(rbent* lp, int8_t* p, u_int m)
{
	u_int bb = lp->bb;
	int nbb = lp->nbb;
	for (int k = 0; m != 0; ++k, m >>= 1) {
		if ((m & 1) != 0) {
			int8_t* q = &p[(k & 3) << 1 | (k & 0xc) << 2];
			DECODE_COEF(lp, &q[0], bb, nbb);
			DECODE_COEF(lp, &q[1], bb, nbb);
			DECODE_COEF(lp, &q[8], bb, nbb);
			DECODE_COEF(lp, &q[9], bb, nbb);
		}
	}
	lp->bb = bb;
	lp->nbb = nbb;
}

#define SBC_BIAS(cf, bias) \
{ \
	int v_ = (cf); \
	if (v_ != 0) { \
		v_ += bias; \
		if ((v_ & 0x80) != 0) \
			v_ = -(v_ & 0x7f); \
		(cf) = v_; \
	} \
}

/*
 * XXX could do sign/mag conversion as first step in reconstruction
 * and could 'or' in bias in decode_sbc (since it won't interface
 * with the mask check in the refinement pass).  thus, this
 * last pass can probably be eliminated.
 */
void PvhDecoder::bias_sbc_to_midstep(int8_t* p, u_int m, int bit)
{
	bit = 1 << (bit - 1);
	for (int k = 0; m != 0; ++k, m >>= 1) {
		if ((m & 1) != 0) {
			int8_t* q = &p[(k & 3) << 1 | (k & 0xc) << 2];
			SBC_BIAS(q[0], bit);
			SBC_BIAS(q[1], bit);
			SBC_BIAS(q[8], bit);
			SBC_BIAS(q[9], bit);
		}
	}
}

u_int PvhDecoder::decode_tree(rbent* lp)
{
	int sm;
	PVH_GET_BITS(4, lp, sm);
	u_int m = 0;
	for (int i = 4; --i >= 0; ) {
		m <<= 4;
		if ((sm & (1 << i)) != 0) {
			int s;
			PVH_GET_BITS(4, lp, s);
			m |= s;
		}
	}
	return (m);
}

#define DECODE_REFINEMENT(lp, cf, mask, bit) \
{ \
	int _t; \
        GET_BITS(1, nbb, bb, lp->bs, _t); \
	int _s = (cf) | (_t << (bit)); \
	if (((_s & (mask)) >> (bit)) == 1) { \
		GET_BITS(1, nbb, bb, lp->bs, _t); \
		_s |= _t << 7; \
	} \
	(cf) = _s; \
}

void PvhDecoder::decode_sbc_refinement(rbent* lp, int8_t* p,
				       u_int m, int bit)
{
	u_int bb = lp->bb;
	int nbb = lp->nbb;
	int mask = 0x80 - (1 << bit);
	for (int k = 0; m != 0; ++k, m >>= 1) {
		if ((m & 1) != 0) {
			u_int8_t* q = (u_int8_t*)&p[(k & 3) << 1 | (k & 0xc) << 2];
			DECODE_REFINEMENT(lp, q[0], mask, bit);
			DECODE_REFINEMENT(lp, q[1], mask, bit);
			DECODE_REFINEMENT(lp, q[8], mask, bit);
			DECODE_REFINEMENT(lp, q[9], mask, bit);
		}
	}	
	lp->bb = bb;
	lp->nbb = nbb;
}

void PvhDecoder::decode_sbc(const spatial_hierarchy* sh, int8_t* out)
{
	int bit = sh->bq;
	if (bit <= 0) {
		memset(out, 0, 64);
		return;
	}

	if (bit != 4)
		/*XXX*/
		abort();

	/* no easy way around this */
	memset(out, 0, 64);

	rbent* lp = channels_[sh->base_channel];
	u_int m;
	PVH_GET_BITS(1, lp, m);
	if (m == 0)
		return;

	m = decode_tree(lp);
	decode_sbc_base(lp, out, m);
	bit -= 1;

	u_int all = m;
	for (int i = 0; i < sh->n; ++i) {
		for (int nr = sh->ref[i].nr; --nr >= 0; ) {
			lp = channels_[sh->ref[i].channel];
			m = decode_tree(lp);
			all |= m;
			decode_sbc_refinement(lp, out, m, bit);
			--bit;
		}
	}
	bias_sbc_to_midstep(out, all, bit);
}

void PvhDecoder::decode_chm(u_int8_t* out, int stride)
{
	spatial_hierarchy* sh = &sh_[COMP_CHM];
	if (sh->bq == 0 && sh->n == 0) {
		/* no bits present in this hierarchy -- fill with gray*/
		for (int i = 8; --i >= 0; ) {
			u_int32_t* p = (u_int32_t*)out;
			p[0] = 0x80808080;
			p[1] = 0x80808080;
			out += stride;
		}
	} else
		decode_dct(sh, out, stride);
}

void PvhDecoder::decode_dct(spatial_hierarchy* sh, u_int8_t* p, int stride)
{
	int bit = sh->bq;
	if (bit == 0)
		return;
	rbent* lp = channels_[sh->base_channel];
	decode_dct_base(lp, bit);
	bit -= 1;
	for (int i = 0; i < sh->n; ++i) {
		int nr = sh->ref[i].nr;
		lp = channels_[sh->ref[i].channel];
		decode_dct_refinement(lp, bit, nr);
		bit -= nr;
	}
	/*XXX check dct_bias_ computation*/
	bias_to_midstep(coef_, 1 << bit, mask_);
#ifdef INT_64
	rdct(coef_, *(INT_64 *)mask_, (u_char*)p, stride, (u_char*)0); //SV-XXX: Debian
#else
	rdct(coef_, mask_[0], mask_[1], (u_char*)p, stride, (u_char*)0);
#endif
}

void PvhDecoder::decode_lum(int8_t* p)
{
	decode_dct(&sh_[COMP_LUM_DCT], (u_int8_t*)p, 8);
	decode_sbc(&sh_[COMP_LUM_SBC], p + 64);
	decode_sbc(&sh_[COMP_LUM_SBC], p + 128);
}

inline void cpblk(int8_t* d, int8_t* s, int stride)
{
	for (int h = 8; --h >= 0; ) {
		*(u_int32_t*)d = *(u_int32_t*)s;
		*(u_int32_t*)(d + 4) = *(u_int32_t*)(s + 4);
		s += 8;
		d += stride;
	}
}

void PvhDecoder::decode_block(int x, int y)
{
	int stride = (inw_ + LINEPAD) >> 1;
	int off = (y >> 1) * stride + (x >> 1);

	int8_t* ll = ll_ + off;
	int8_t* hl = hl_ + off;
	int8_t* lh = lh_ + off;
#ifdef HAVE_HH
	int8_t* hh = hh_ + off;
	int8_t blk[4*64];
#else
	int8_t blk[3*64];
#endif
	decode_lum(blk);
	/*XXX eliminate these copies: need to fix sbc code to deal
	 with stride etc (or simply fold into last-stage bias routines). */
	cpblk(ll, blk, stride);
	cpblk(hl_ + off, blk + 64, stride);
	cpblk(lh_ + off, blk + 128, stride);
#ifdef HAVE_HH
	cpblk(hh_ + off, blk + 192, stride);
#endif
	/*
	 * XXX this needs a comment; esp. subtlety about sign change
	 * on lh extension.
	 */

	if (y == 0) {
		memcpy(ll - stride, ll, 8 * sizeof(*ll));
		memcpy(hl - stride, hl, 8 * sizeof(*hl));
		for (int k = 0; k < 8; ++k) {
			lh[-stride + k] = -lh[k];
#ifdef HAVE_HH
			hh[-stride + k] = -hh[k];
#endif
		}
	} else if (y == inh_ - 16 /*XXX*/) {
		ll += stride << 3;
		lh += stride << 3;
		hl += stride << 3;
#ifdef HAVE_HH
		hh += stride << 3;
#endif

		memcpy(ll, ll - stride, 8 * sizeof(*ll));
		memcpy(hl, hl - stride, 8 * sizeof(*hl));
		for (int k = 0; k < 8; ++k) {
			lh[k] = -lh[-stride + k];
#ifdef HAVE_HH
			lh[k] = -hh[-stride + k];
#endif
		}

		ll -= stride << 3;
		lh -= stride << 3;
		hl -= stride << 3;
#ifdef HAVE_HH
		hh -= stride << 3;
#endif
	}
	if (x == 0) {
		for (int k = 0; k < 8; ++k) {
			ll[-1] = ll[0];
			ll += stride;
			hl[-1] = hl[0];
			hl += stride;
			lh[-1] = -lh[0];
			lh += stride;
#ifdef HAVE_HH
			hh[-1] = -hh[0];
			hh += stride;
#endif
		}
	} else if (x == inw_ - 16 /*XXX*/) {
		ll += 8;
		lh += 8;
		hl += 8;
#ifdef HAVE_HH
		hh += 8;
#endif
		for (int k = 0; k < 8; ++k) {
			ll[0] = ll[-1];
			ll += stride;
			hl[0] = hl[-1];
			hl += stride;
			lh[0] = -lh[-1];
			lh += stride;
#ifdef HAVE_HH
			hh[0] = -hh[-1];
			hh += stride;
#endif
		}
	}
	int s = inw_ * inh_;
	u_int8_t* chm = frm_ + s;
	chm += (y >> 1) * (inw_ >> 1) + (x >> 1);
	decode_chm(chm, inw_ >> 1);
	decode_chm(chm + (s >> 2), inw_ >> 1);
}

void PvhDecoder::init_bbs(rbent* rb)
{
	rb->nbb = 0;
	rb->bb = 0;

	pktbuf* pb = rb->pb;
	pvhhdr* ph = (pvhhdr*)((rtphdr*)pb->dp + 1);
	rb->bs = (u_int8_t*)(ph + 1);
	rb->es = rb->bs + pb->len - (sizeof(rtphdr) + sizeof(pvhhdr));
	rb->ebit = ph->ebit;
}

/*
 * go through each resumption layer and see if bit buffer has
 * ran out of bits.  this can only happen on block boundaries
 * (assuming no corruption).
 */
void PvhDecoder::check_bbs(frment* fe, rbent* base, int blkno)
{
	for (int i = 0; i < base->nrsmp; ++i) {
		int chan = base->rsmp[i].channel;
		if (chan > maxChannel_)
			break;
		rbent* rb = channels_[chan];
		if (BB_EOB(rb)) {
			pkt_window* win = fe->win[chan];
			rb = win->pkts[(rb->seqno + 1) & (NRB - 1)];
			if (rb == 0) {
				/*XXX can happen if bit stream corrupt*/
				printf("channel-%d: stop base (%d %d) chan (%d %d) @%d\n", chan,
				       base->sblk, base->eblk,
				       channels_[chan]->sblk,
				       channels_[chan]->eblk, blkno);

				base->nrsmp = i;
				return;
			}
			init_bbs(rb);
			channels_[chan] = rb;
		}
	}
}

int PvhDecoder::decode_resumption_ptrs(rbent* lp, pvh_resump* pr)
{
	int n = 0;
	while (!BB_EOB(lp)) {
		int v;
		PVH_GET_BITS(1, lp, v);
		if (v == 0)
			return (n);
		int chan;
		/*XXX this macro can't hack more than 16 bits at a time */
		PVH_GET_BITS(4, lp, chan);
		PVH_GET_BITS(16, lp, v);
		pr->channel = chan;
		pr->off = v;
#ifdef notdef
printf("D RP +%d @%d\n", pr->layer, pr->off);
#endif
		++pr;
		++n;
	}
	return (n);
}

void PvhDecoder::free_slice(frment* fe, rbent* rb)
{
	/*
	 * Remove this base-layer packet from the list.
	 */
	rbent** p;
	for (p = &fe->blpkts; *p != 0; p = &(*p)->next) 
		if (*p == rb)
			break;
	//Added by Piers PPP
	if (*p != 0)
		*p = (*p)->next;
	/*
	 * XXX should probably free all pkts that are wholly contained
	 * in this slice...
	 * for now, we rely on frment garbage collector.
	 */
}

void PvhDecoder::salvage_base_packets(frment* fe)
{
	for (rbent* base = fe->blpkts; base != 0; base = base->next) {
		if (slice_ready(fe, base)) {
			decode_slice(fe, base);
			free_slice(fe, base);
		}
	}
}

void dumpSH(const char* c, spatial_hierarchy* sh)
{
	printf("%s SH L=%d Q=%d ", c, sh->base_channel, sh->bq);
	for (int i = 0; i < sh->n; ++i)
		printf("%d+%d ", sh->ref[i].channel, sh->ref[i].nr);
	printf("\n");
}

void PvhDecoder::decode_comp_layout(spatial_hierarchy* sh, rbent* base)
{
	/*
	 * First find the base layer and base-layer quantizer.
	 */
	int i = 0;
	int n = 0;
	int v;
	PVH_GET_BITS(1, base, v);
	if (v) {
		sh->base_channel = base->pb->layer;
		PVH_GET_BITS(3, base, v);
		sh->bq = v;
		/*
		 * Look for refinement bits on base layer.
		 * Since this stream does not appear in the
		 * resumption pointers in the loop below.
		 */
		PVH_GET_BITS(3, base, v);
		if (v != 0) {
			sh->ref[0].nr = v;
			sh->ref[0].channel = sh->base_channel;
			n = 1;
		}
	} else {
		/*
		 * Keep looking for base layer
		 * (which need not be on the bottom of the hierarchy
		 * for the chrominance and luma-subband bits)
		 */
		for (; i < base->nrsmp; ++i) {
			int chan = base->rsmp[i].channel;
			PVH_GET_BITS(1, base, v);
			if (v) {
				sh->base_channel = chan;
				PVH_GET_BITS(3, base, v);
				sh->bq = v;
				break;
			}
		}
		if (i >= base->nrsmp) {
			/*
			 * Didn't find a base layer quantizer.
			 * This can happen on the subband and
			 * chroma hierarchies when we're not
			 * receiving all the layers.  Set the
			 * quantizer to 0 so we know these bits
			 * aren't present when decoding.
			 */
			sh->n = 0;
			sh->bq = 0;
			return;
		}
	}
	/*
	 * Now determine where the refinement layers fall.
	 * (we check on the base layer for refinement as well)
	 */
	for (; i < base->nrsmp; ++i) {
		PVH_GET_BITS(3, base, v);
		if (v != 0) {
			sh->ref[n].nr = v;
			sh->ref[n].channel = base->rsmp[i].channel;
			++n;
		}
	}
	sh->n = n;
#ifdef notdef
dumpSH("D", sh);
#endif
}

#define SEQ_LEQ(a, b) (((a) - (b)) << 16 >> 16 <= 0)
#define SEQ_GEQ(a, b) ((((a) - (b)) >> 16) == 0)
#define SEQ_LT(a, b)  ((((a) - (b)) >> 16) != 0)

/*
 * Return the first packet of layer "layer" in the frame-entry that
 * overlaps with the base layer packet "base".
 */
rbent* PvhDecoder::find_first_packet(frment* fe, rbent* base, int chan) const
{
	pkt_window* win = fe->win[chan];
	if (win == 0)
		abort();

	int s = win->first;
	if (s < 0)
		abort();

	int sblk = base->sblk;
	//SV-XXX: unused: int eblk = base->eblk;

	/*
	 * Find first packet that overlaps left side of base layer packet.
	 */
	int last = win->last;
	for (;;) {
		if (!SEQ_LEQ(s, last)) {
			abort();
		}
		int k = s & (NRB-1);
		rbent* p = win->pkts[k];
		if (p != 0 && p->eblk >= sblk) {
			if (p->sblk > sblk)
				/* missing front edge */
				abort();
			return (p);
		}
		s = (s + 1) & 0xffff;
	}
}

/*
 * Peg the fields in "sh" to account for the
 * limited number of channels that we might be
 * receiving.
 */
void PvhDecoder::adjust_sh(spatial_hierarchy* sh)
{
	if (sh->base_channel > maxChannel_) {
		sh->bq = 0;
		sh->n = 0;
		return;
	}
	for (int i = 0; i < sh->n; ++i) {
		if (sh->ref[i].channel > maxChannel_) {
			sh->n = i;
			return;
		}
	}
}

void PvhDecoder::setup_layers(frment* fe, rbent* base)
{
	/*
	 * First initialize the bit stream of each layer
	 * (the layering stucture is determined by the
	 * resumption pointers in the base layer packet).
	 */
	channels_[base->pb->layer] = base;
	int i;
	for (i = 0; i < base->nrsmp; ++i) {
		int chan = base->rsmp[i].channel;
		if (chan > maxChannel_)
			break;
		/*XXX BUG: need to check that offset is in bounds! */
		int off = base->rsmp[i].off;
		rbent* p = find_first_packet(fe, base, chan);
		channels_[chan] = p;
		init_bbs(p);
		p->bs += off >> 3;
		off &= 7;
		HUFF_READ(p->bb, p->bs);
		p->nbb += (16 - off);
	}
	/*
	 * Now read the bit allocation code words from the front
	 * of each packet to determine the layout of bits over
	 * the spatial hierarchy.
	 */
	spatial_hierarchy* sh = sh_;
	for (i = 0; i < 3; ++i, ++sh) {
		decode_comp_layout(sh, base);
		adjust_sh(sh);
	}
}

void PvhDecoder::decode_slice(frment* fe, rbent* rb)
{
	setup_layers(fe, rb);

	/*XXX*/
	int blkw = inw_ >> 4;
	int blkno = -1;

	int base_channel = rb->pb->layer;
	rbent* lp = channels_[base_channel];
/*XXX*/
if (lp != rb)
	abort();
	while (!BB_EOB(lp)) {
		if (blkno < 0)
			blkno = rb->sblk;
		else {
			int v;
			PVH_GET_BITS(1, lp, v);
			if (v == 1)
				blkno += 1;
			else {
				PVH_GET_BITS(1, lp, v);
				if (v == 1) {
					PVH_GET_BITS(4, lp, v);
					v += 2;
				} else {
					PVH_GET_BITS(11, lp, v);
				}
				blkno += v;
			}
		}
		int y = blkno / blkw;
		int x = blkno - y * blkw;

		x <<= 4;
		y <<= 4;
		if ((unsigned)(x + 16) > (unsigned)inw_ ||
		    (unsigned)(y + 16) > (unsigned)inh_) {
#ifdef notdef
printf("bad-size\n");
#endif
			count(STAT_BAD_SYNTAX);
			return;
		}
		check_bbs(fe, rb, blkno);
		decode_block(x, y);

		/*XXX*/
		/* remember that we need to reconstruct this block */
		blk_[(x >> 4) + blkw * (y >> 4)] = 1;

		/*XXX*/
		int k = (x >> 3) + (inw_ >> 3) * (y >> 3);
		rvts_[k] = now_;
		rvts_[k + 1] = now_;
		k += inw_ >> 3;
		rvts_[k] = now_;
		rvts_[k + 1] = now_;
	}
}

void PvhDecoder::extend_window(pkt_window* win, rbent* rb)
{
	int seqno = rb->seqno;
	int off = seqno & (NRB - 1);
	rbent* p = win->pkts[off];
	if (p != 0)
		/* duplicate? should've been filtered by RTP layer */
		free_rb(p);
	win->pkts[off] = rb;
	if (win->first < 0)
		win->first = win->last = seqno;
	else if (SEQ_LT(seqno, win->first))
		win->first = seqno;
	else if (SEQ_GEQ(seqno, win->last))
		win->last = seqno;
}

#define TS_LT(a, b)  (((int32_t)((a) - (b))) < 0)

frment* PvhDecoder::lookup_frame(u_int32_t ts)
{
	int oldest = 0;
	for (int i = 0; i < NFE; ++i) {
		if (fe_[i].ts == ts)
			return (&fe_[i]);
		if (TS_LT(fe_[i].ts, fe_[oldest].ts))
			oldest = i;
	}
	frment* fe = &fe_[oldest];
	free_frment(fe);
	fe->ts = ts;
	return (fe);
}

/*
 * Return true iff the indicated layer fully "covers"
 * the base layer packet "rb".
 * XXX this can be made more efficient by collapsing ranges
 * on the fly (though we should look at a run-time profile first).
 */
int PvhDecoder::layer_ready(frment* fe, rbent* rb, int chan) const
{
/*XXX*/
#ifdef notdef
if (chan > maxChannel_)
	return 1;
#endif

	pkt_window* win = fe->win[chan];
	if (win == 0)
		/* haven't seen ANY packets from this layer */
		return (0);

	int s = win->first;
	if (s < 0)
		return (0);

	int sblk = rb->sblk;
	int eblk = rb->eblk;

	/*
	 * Find first packet that overlaps left side of base layer packet.
	 */
	int last = win->last;
	for (;;) {
		if (!SEQ_LEQ(s, last)) {
			return (0);
		}
		int k = s & (NRB-1);
		rbent* p = win->pkts[k];
		if (p != 0 && p->eblk >= sblk) {
			if (p->sblk > sblk)
				/* missing front edge */
				return (0);
			break;
		}
		s = (s + 1) & 0xffff;
	}
	/*
	 * Return true if we find a contiguos range of packets
	 * that covers the base layer.
	 */
	for (;;) {
		if (!SEQ_LEQ(s, last)) {
			return (0);
		}
		int k = s & (NRB-1);
		rbent* p = win->pkts[k];
		if (p == 0)
			return (0);
		if (p->eblk >= eblk)
			return (1);
		s = (s + 1) & 0xffff;
	}
}

/*
 * Return true if we have all the packets that to decode
 * the slice rooted at base layer packet "rb".
 */
int PvhDecoder::slice_ready(frment* fe, rbent* rb) const
{
	for (int i = 0; i < rb->nrsmp; ++i) {
		int ch = rb->rsmp[i].channel;
		if (ch > maxChannel_)
			return (1);
		if (!layer_ready(fe, rb, ch))
			return (0);
	}
	return (1);
}

/*
 * Add the base-layer packet "rb" to the list maintained
 * in thie frame-entry.  Initialize the base-layers bit buffer
 * and read the resumption pointers into the rbent.
 */
void PvhDecoder::add_base_packet(frment* fe, rbent* rb)
{
	rb->next = fe->blpkts;
	fe->blpkts = rb;
	init_bbs(rb);
	rb->nrsmp = decode_resumption_ptrs(rb, rb->rsmp);
}

rbent* PvhDecoder::lookup_base_packet(frment* fe, rbent* rb) const
{
	for (rbent* base = fe->blpkts; base != 0; base = base->next) {
		/*
		 * If there is ANY overlap in the block ranges,
		 * return the base-layer packet.
		 */
		if (SEQ_GEQ(rb->eblk, base->sblk) &&
		    SEQ_LEQ(rb->sblk, base->eblk)) {
			return (base);
		}
	}
	return (0);
}

rbent* PvhDecoder::add_packet(frment* fe, rbent* rb)
{
	int chan = rb->pb->layer;
	if (chan >= NLAYER)
		abort();

	/*
	 * Add the packet to the window and extend if necessary.
	 * If it's a base-layer packet, initialize its parsing
	 * state and see if we're ready to decode.  Otherwise,
	 * lookup the base-layer packet corresponding to this
	 * new packet, and see if we can decode it.  If the
	 * base layer packet isn't hear yet, just return 0
	 * (and its later arrival will trigger the decoding action).
	 */
	extend_window(fe->win[chan], rb);
	if (rb->base)
		add_base_packet(fe, rb);
	else {
		rb = lookup_base_packet(fe, rb);
		if (rb == 0)
			return (0);
	}
	return (slice_ready(fe, rb) ? rb : 0);
}

pkt_window* PvhDecoder::alloc_window()
{
	pkt_window* p = free_window_;
	if (p == 0) {
		p = new pkt_window;
		memset(p, 0, sizeof(*p));
	} else
		/* packet array guaranteed to be null */
		free_window_ = p->next;

	p->first = p->last = -1;
	return (p);
}

void PvhDecoder::free_window(pkt_window* p)
{
	/* free packets*/
	int s = p->first;
	if (s >= 0) {
		int last = p->last;
		while (SEQ_LEQ(s, last)) {
			int k = s & (NRB-1);
			rbent* rb = p->pkts[k];
			if (rb != 0) {
				free_rb(rb);
				p->pkts[k] = 0;
			}
			s = (s + 1) & 0xffff;
		}
	}
	/* add to free list */
	p->next = free_window_;
	free_window_ = p;
}

void PvhDecoder::free_rb(rbent* rb)
{
	rb->pb->release();
	rb->pb = 0;
	rb->next = free_rb_;
	free_rb_ = rb;
}

rbent* PvhDecoder::alloc_rb(pktbuf* pb)
{
	rbent* rb = free_rb_;
	if (rb != 0)
		free_rb_ = rb->next;
	else
		rb = new rbent;

	rb->pb = pb;
	rb->nrsmp = 0;
	return (rb);
}

void PvhDecoder::free_frment(frment* fe)
{
	fe->blpkts = 0;
	for (int i = 0; i < NLAYER; ++i) {
		pkt_window* p = fe->win[i];
		if (p != 0) {
			free_window(p);
			fe->win[i] = 0;
		}
	}
}

void dump(int chan, const rtphdr* rh, const pvhhdr* sh, int size)
{
	int seqno = ntohs(rh->rh_seqno);
	int flags = ntohs(rh->rh_flags);
	printf("CH%d size %d seq %d %x %x %c pvh %dx%d sblk=%d eblk=%d base=%d ebit=%d\n",
	       chan, size, seqno,
	       ntohl(rh->rh_ts),
	       ntohs(rh->rh_seqno),
	       (flags & RTP_M) ? 'M' : '-',
	       sh->width, sh->height,
	       ntohs(sh->sblk),
	       ntohs(sh->eblk),
	       sh->base, sh->ebit);
}

void PvhDecoder::recv(pktbuf* pb)
{
	int chan = pb->layer;
	const rtphdr* rh = (rtphdr*)pb->dp;
	const pvhhdr* ph = (pvhhdr*)(rh + 1);

	if (pb->len <= sizeof(rtphdr) + sizeof(pvhhdr))
		/*XXX should allow marker bit through!*/
		return;

	int w = ph->width << 3;
	int h = ph->height << 3;
	if (w != inw_ || h != inh_) {
		if ((unsigned)w > 1000 || (unsigned)h > 1000) {
			/*XXX*/
			w = 320;
			h = 240;
			count(STAT_BAD_GEOM);
		}
		resize(w, h);
	}
#ifdef notdef
dump(chan, rh, ph, pb->len);
#endif
	/*
	 * Wrap packet buffer in an rbent,
	 * and pull up frequently accessed fields.
	 */
	rbent* rb = alloc_rb(pb);
	rb->seqno = ntohs(rh->rh_seqno);
	rb->sblk = ntohs(ph->sblk);
	rb->eblk = ntohs(ph->eblk);
	rb->base = ph->base;

	/*
	 * Lookup the frame-entry object for this packet.
	 * If we haven't yet seen this layer within the frame,
	 * allocate a "packet window" to house the packets
	 * of this layer.
	 */
	frment* fe = lookup_frame(rh->rh_ts);
	if (fe->win[chan] == 0)
		fe->win[chan] = alloc_window();

	/*
	 * Process the packet.  If we're ready to decode
	 * a slice, then add_packet returns the base layer
	 * packet corresponding to the slice to decode.
	 * Conceivably we could get a packet that
	 * straddles two slices, in which case we would want
	 * to decode both slices (and can't because add_packet
	 * only returns one base-layer packet).  But this
	 * should be rare because higher layers generally
	 * have much higher rate (so the overlap is not likely).
	 * In case it does happen, we will salvage_base_packets
	 * will pick up all the missed base-layer packets before
	 * rendering the frame.
	 */
	rb = add_packet(fe, rb);
	if (rb != 0) {
		/*XXX*/
		rtphdr* rh = (rtphdr*)rb->pb->dp;
		int mark = ntohs(rh->rh_flags) & RTP_M;
		decode_slice(fe, rb);
		free_slice(fe, rb);
		/*
		 * Decode all the remaining base layer packets
		 * that we missed, e.g., because multiple base
		 */
		salvage_base_packets(fe);

		if (mark) {
			sync();
			render_frame(frm_);
		}
	}
}

void PvhDecoder::sync()
{
	u_char* p = blk_;
	int blkw = inw_ >> 4;
	int blkh = inh_ >> 4;
	for (int y = 0; y < blkh; ++y) {
		for (int x = 0; x < blkw; ++x) {
			if (*p != 0) {
				*p = 0;
				reconstruct_block(x << 4, y << 4);
			}
			++p;
		}
	}
}

/*
 * XXX should use PlaneDecoder class for 420 also
 */

int PvhDecoder::colorhist(u_int* hist) const
{
	int s = inw_ * inh_;
	colorhist_420_556(hist, frm_, frm_ + s, frm_ + s + (s >> 2),
			  inw_, inh_);
	return (1);
}

void PvhDecoder::redraw()
{
	Decoder::redraw(frm_);
}
