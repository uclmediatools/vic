/* This code was originally derived from a cellb reference encoder from
 * Sun Microsystems with the following copyright:
 *
 * Copyright (c) Sun Microsystems, Inc.  1992, 1993. All rights reserved. 
 *
 * License is granted to copy, to use, and to make and to use derivative 
 * works for research and evaluation purposes, provided that Sun Microsystems is
 * acknowledged in all documentation pertaining to any such copy or derivative
 * work. Sun Microsystems grants no other licenses expressed or implied. The
 * Sun Microsystems  trade name should not be used in any advertising without
 * its written permission.
 *
 * SUN MICROSYSTEMS MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF
 * THIS SOFTWARE FOR ANY PARTICULAR PURPOSE.  The software is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this software.
 */

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "inet.h"
#include "net.h"
#include "rtp.h"
#include "bsd-endian.h"
#include "crdef.h"
#include "transmitter.h"
#include "pktbuf.h"
#include "module.h"

#define MAXSKIP		32

extern "C" u_char cellb_yyremap[];
extern "C" u_char cellb_uvremap[];

#define HLEN (sizeof(rtphdr) + sizeof(cellbhdr))

class CellbEncoder : public TransmitterModule {
 public:
	CellbEncoder();
	~CellbEncoder();
	virtual int consume(const VideoFrame*);
 private:
	void size(int w, int h);
	int diff(const u_char blk1[4], const u_char blk2[4]) const;
	void send(pktbuf* pb, int sync, int x, int y, int cc);
	u_int encode_cell(const u_char* lum, const u_char* chm, int stride);

	int nw_;
	int nh_;

	/*
	 * Tables for CellB.
	 */
	u_char* divtable_[17];/*XXX*/
	u_char dtbl_[34816];
};

static class CellbEncoderMatcher : public Matcher {
public:
	CellbEncoderMatcher() : Matcher("module") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "cellb") == 0)
			return (new CellbEncoder);
		return (0);
	}
} encoder_matcher_cellb;

void CellbEncoder::send(pktbuf* pb, int sync, int x, int y, int cc)
{
	pb->len = cc + HLEN;
	rtphdr* rh = (rtphdr*)pb->data;
	if (sync)
		rh->rh_flags |= htons(RTP_M);

	cellbhdr* ch = (cellbhdr*)(rh + 1);
	ch->width = nw_;
	ch->height = nh_;
	ch->x = htons(x);
	ch->y = htons(y);
	tx_->send(pb);
}

/*
 * Returns 32-bit encoded cell in host order.
 */
u_int CellbEncoder::encode_cell(const u_char* lum,
				const u_char* chm,
				int stride)
{
	u_int mask = 0xff00ff;
	u_int q0 = *(u_int *)lum;
	u_int q1 = *(u_int *)&lum[stride];
	u_int q2 = *(u_int *)&lum[stride << 1];
	u_int q3 = *(u_int *)&lum[(stride << 1) + stride];

	/*
	 * Compute the mean of the luminance component.
	 * Do some of the adds in parallel.
	 */
	u_int ymean = q0 & mask;
	ymean += (q0 >> 8) & mask;
	ymean += q1 & mask;
	ymean += (q1 >> 8) & mask;
	ymean += q2 & mask;
	ymean += (q2 >> 8) & mask;
	ymean += q3 & mask;
	ymean += (q3 >> 8) & mask;
	ymean = (ymean & 0xffff) + (ymean >> 16);
	u_int sum = ymean;
	ymean >>= 4;

	/*
	  .... next the bit mask for the tile is generated. for
	  the first fifteen bits this is done by comparing each
	  luminance value to the mean. if that value is greater
	  than the mean the correnponding bit in the mask is set,
	  otherwise the counter for the number of zeros in the mask,
	  tmp, is incremented. also, the values of those pixels less
	  than the mean are accumulted in ylo ....
	  */

	mask = 0;
	int ylo = 0;
	int nb = 0;

#define ACCUMULATE(pix, mean, n, slow, mask, pos) \
	{ \
		int t = (pix) & 0xff; \
		t -= (mean); \
		int neg = t >> 31; \
		(slow) += neg & t; \
		(slow) += neg & (mean); \
		neg &= 1; \
		(n) += neg; \
		(mask) |= neg << (pos); \
	}
#if BYTE_ORDER == LITTLE_ENDIAN
#define BYTE(b, n) ((b) >> (8 * (3 - (n))))
#else
#define BYTE(b, n) ((b) >> (8 * (n)))
#endif
	ACCUMULATE(BYTE(q3, 0), ymean, nb, ylo, mask, 0);
	ACCUMULATE(BYTE(q3, 1), ymean, nb, ylo, mask, 1);
	ACCUMULATE(BYTE(q3, 2), ymean, nb, ylo, mask, 2);
	ACCUMULATE(BYTE(q3, 3), ymean, nb, ylo, mask, 3);

	ACCUMULATE(BYTE(q2, 0), ymean, nb, ylo, mask, 4);
	ACCUMULATE(BYTE(q2, 1), ymean, nb, ylo, mask, 5);
	ACCUMULATE(BYTE(q2, 2), ymean, nb, ylo, mask, 6);
	ACCUMULATE(BYTE(q2, 3), ymean, nb, ylo, mask, 7);

	ACCUMULATE(BYTE(q1, 0), ymean, nb, ylo, mask, 8);
	ACCUMULATE(BYTE(q1, 1), ymean, nb, ylo, mask, 9);
	ACCUMULATE(BYTE(q1, 2), ymean, nb, ylo, mask, 10);
	ACCUMULATE(BYTE(q1, 3), ymean, nb, ylo, mask, 11);

	ACCUMULATE(BYTE(q0, 0), ymean, nb, ylo, mask, 12);
	ACCUMULATE(BYTE(q0, 1), ymean, nb, ylo, mask, 13);
	ACCUMULATE(BYTE(q0, 2), ymean, nb, ylo, mask, 14);
	ACCUMULATE(BYTE(q0, 3), ymean, nb, ylo, mask, 15);
#undef BYTE
#undef ACCUMULATE

	int yhi = sum - ylo;
	yhi = divtable_[16 - nb][yhi];
	ylo = divtable_[nb][ylo];
	/*
	 * Normalize bitmask.  Note mask is
	 * computed in negative logic.
	 */
	if (mask & 0x8000) {
		mask ^= 0xffff;
		ylo <<= 8;
		ylo |= yhi;
	} else
		ylo |= yhi << 8;

	mask <<= 16;

	/*
	  .... the high mean and low mean values are finally quantized
	  by the cellb_yyremap[] table, the Y/Y vector codeword byte, the
	  U/V codeword byte and the bitmask are then assembled into
	  a Normal Cell bytecode and returned form the routine ....
	  */
	mask |= cellb_yyremap[ylo];

	/*
	 * Simply downsample the chroma components, then
	 * vector quantize according to the CellB spec.
	 * (The encoder in nv computes a mean of the 16
	 * chroma components.  This is probably not a good
	 * idea first because it's computationally more
	 * expensive and second because it doesn't buy
	 * much.  This is equivalent to placing a low-pass
	 * (rectangular) filter in front of the downsampling
	 * stage.  But the chroma signal in most cases
	 * doesn't have much of a high frequency component.
	 * Try visually comparing this coder with nv's.
	 * You will probably see little difference.)
	 */
	int u = chm[0];
	int v = chm[framesize_ >> 1];
	mask |= cellb_uvremap[u >> 2 << 6 | v >> 2] << 8;

	return (mask);
}

void CellbEncoder::size(int w, int h)
{
	Module::size(w, h);
	nw_ = htons(w);
	nh_ = htons(h);
}

CellbEncoder::CellbEncoder() : TransmitterModule(FT_YUV_422)
{
	/* .... initialize division table ....  */
	u_char* p = dtbl_;
	divtable_[0] = p;
	for (int i = 1; i <= 16; i++) {
		divtable_[i] = p;
		for (int j = 0; j < 256*i; j++)
			*p++ = j / i;
	}
	width_ = 0;
	height_ = 0;
	nw_ = 0;
	nh_ = 0;
}

CellbEncoder::~CellbEncoder()
{
}

/*
 * Encode a cellb frame.  this code can be improved by pruning
 * skip-code runs off the front and back of the frame.
 */
int CellbEncoder::consume(const VideoFrame* vf)
{
	if (!samesize(vf))
		size(vf->width_, vf->height_);
	YuvFrame* p = (YuvFrame*)vf;
	tx_->flush();

	pktbuf* pb = pool_->alloc(p->ts_, RTP_PT_CELLB);
	u_char* op = &pb->data[HLEN];
	u_char* ep = op + tx_->mtu() - HLEN;
	int x0 = 0;
	int y0 = 0;

	int cc = 0;
	int nb = 0;
	int nskip = 0;
	int stride = 3 * width_;

	int blkbase = 0;
	/*
	 * Compute width and height in cells.
	 */
	int w = width_ >> 2;
	int h = height_ >> 2;

	/*
	 * Loop over 4x4 cells, encoding those indicated by the 16x16
	 * CR vector.  Because there are 4 cells per 16x16 replenishment
	 * block, the blocks are incremented only every other interation
	 * of each loop.
	 */

	const u_int8_t* lum = p->bp_;
	const u_int8_t* chm = lum + framesize_;
	const u_int8_t* crvec = p->crvec_;
	for (int y = 0; y < h; ) {
		int blkno = blkbase;
		for (int x = 0; x < w; ) {
			if (op + 2*5 >= ep) {
				/*XXX sanity*/
//				if (cc != op - (u_char*)pb->iov[1].iov_base)
//					abort();

				nb += cc + HLEN;
				send(pb, 0, x0, y0, cc);
				cc = 0;
				pb = pool_->alloc(p->ts_, RTP_PT_CELLB);
				op = &pb->data[HLEN];
				ep = op + tx_->mtu() - HLEN;
				x0 = x;
				y0 = y;
			}
			if (crvec[blkno] & CR_SEND) {
				/*
				 * cell has seen recent change. send it.
				 */
				if (nskip > 0) {
					++cc;
					*op++ = 0x80 | (nskip - 1);
					nskip = 0;
				}
				int code = encode_cell(lum, chm, width_);
				op[0] = code >> 24;
				op[1] = code >> 16;
				op[2] = code >> 8;
				op[3] = code;
				op += 4;
				cc += 4;
			} else {
				if (++nskip >= MAXSKIP) {
					++cc;
					*op++ = 0x80 | (MAXSKIP - 1);
					nskip = 0;
				}
			}
			if ((++x & 3) == 0)
				++blkno;
			lum += 4;
			chm += 2;
		}
		if ((++y & 3) == 0)
			blkbase += w >> 2;

		lum += stride;
		chm += stride >> 1;
	}
	nb += cc;
	send(pb, 1, x0, y0, cc);
	return (nb);
}
