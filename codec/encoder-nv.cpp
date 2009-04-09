/*
 * This module was originally derived from:
 *
 * Netvideo version 3.2
 * Written by Ron Frederick <frederick@parc.xerox.com>
 * 
 * Video encode routines
 *
 * Copyright (c) Xerox Corporation 1992. All rights reserved.
 *  
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or derivative
 * work. Xerox grants no other licenses expressed or implied. The Xerox trade
 * name should not be used in any advertising without its written permission.
 *  
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
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
#include "vic_tcl.h"
#include "crdef.h"

/*XXX*/
#define MAXBLOCK (128+32)

#include <sys/types.h>
#ifndef WIN32
#include <netinet/in.h>
#endif
#include "inet.h"
#include "transmitter.h"
#include "module.h"

class NvEncoder : public TransmitterModule {
 public:
	NvEncoder();
	~NvEncoder();
	virtual int command(int argc, const char*const* argv);
	virtual int consume(const VideoFrame*);
 protected:
	void size(int w, int h);
	int flush(pktbuf*, int sync);
	void putblock(const u_char* yp, const u_char* up, const u_char* vp,
		      int x, int y, int loss, u_char*& runlen);
	int endblock();
	void VidTransform_Fwd(const u_char* yp,
			      const u_char* up,
			      const u_char* vp,
			      int width, u_int32_t* out) const;
	void NVDCT_FwdTransform(const u_int8_t *yp,
				const u_int8_t *up,
				const u_int8_t* vp,
				int width, u_int32_t *out) const;

	int nw_;
	int nh_;
	int loss_;
	int use_dct_;

	u_char* pt_;
	u_char* ep_;
};


static class NvEncoderMatcher : public Matcher {
public:
	NvEncoderMatcher() : Matcher("module") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "nv") == 0)
			return (new NvEncoder);
		return (0);
	}
} encoder_matcher_nv;

//SV-XXX: rearranged initialisation order to shut up gcc4
NvEncoder::NvEncoder() : TransmitterModule(FT_YUV_422),
	nw_(0), nh_(0), loss_(2), use_dct_(0), pt_(0), ep_(0)
{
	width_ = 0;
	height_ = 0;
}

NvEncoder::~NvEncoder()
{
}

#define HLEN (sizeof(rtphdr) + sizeof(nvhdr))

int NvEncoder::flush(pktbuf* pb, int sync)
{
	int cc = pt_ - (u_char*)&pb->data[HLEN];
	/*
	 * If there's no packet data to send and we don't need
	 * to transmit a sync bit (to indicate a new frame),
	 * then release the packet buffer and return.
	 */
	if (cc == 0 && !sync) {
		pool_->release(pb);
		return (0);
	}
	pb->len = cc + HLEN;
	rtphdr* rh = (rtphdr*)pb->data;
	if (sync)
		rh->rh_flags |= htons(RTP_M);

	nvhdr* nh = (nvhdr*)(rh + 1);
	nh->width = nw_;
	nh->height = nh_ | use_dct_;

	tx_->send(pb);

	return (cc);
}

void NvEncoder::size(int w, int h)
{
	Module::size(w, h);
	nw_ = htons(w | 0x8000);
	nh_ = htons(h);
}

int NvEncoder::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "q") == 0) {
			loss_ = atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "use-dct") == 0) {
			use_dct_ = atoi(argv[2]) ? htons(1 << 12) : 0;
			return (TCL_OK);
		}
	}
	return (TransmitterModule::command(argc, argv));
}

void NvEncoder::putblock(const u_char* yp, const u_char* up, const u_char* vp,
			int x, int y, int loss, u_char*& runlen)
{
	if (runlen == 0) {
		runlen = pt_;
		*pt_++ = 1;
		*pt_++ = x;
		*pt_++ = y;
	} else
		++*runlen;

	static u_int32_t block[32];
	if (use_dct_)
		NVDCT_FwdTransform(yp, up, vp, width_, block);
	else
		VidTransform_Fwd(yp, up, vp, width_, block);

	if (loss > 0) {
		signed char *blkp = (signed char*)block;
		if ((blkp[2]>=-loss) && (blkp[2]<=loss)) blkp[2] = 0;
		if ((blkp[3]>=-loss) && (blkp[3]<=loss)) blkp[3] = 0;
		if ((blkp[4]>=-loss) && (blkp[5]<=loss)) blkp[4] = 0;
		if ((blkp[5]>=-loss) && (blkp[5]<=loss)) blkp[5] = 0;
		if ((blkp[6]>=-loss) && (blkp[6]<=loss)) blkp[6] = 0;
		if ((blkp[7]>=-loss) && (blkp[7]<=loss)) blkp[7] = 0;
		blkp = ((signed char *)block)+10;
		int i;
		for (i=10; i<64; i+=6, blkp+=6) {
			if ((blkp[0]>=-loss) && (blkp[0]<=loss)) blkp[0] = 0;
			if ((blkp[1]>=-loss) && (blkp[1]<=loss)) blkp[1] = 0;
			if ((blkp[2]>=-loss) && (blkp[2]<=loss)) blkp[2] = 0;
			if ((blkp[3]>=-loss) && (blkp[3]<=loss)) blkp[3] = 0;
			if ((blkp[4]>=-loss) && (blkp[4]<=loss)) blkp[4] = 0;
			if ((blkp[5]>=-loss) && (blkp[5]<=loss)) blkp[5] = 0;
		}

		if (up != 0) {
			blkp = ((signed char *)block)+64;
			if ((blkp[2]>=-2*loss) && (blkp[2]<=2*loss))
				blkp[2] = 0;
			if ((blkp[3]>=-2*loss) && (blkp[3]<=2*loss))
				blkp[3] = 0;
			if ((blkp[4]>=-2*loss) && (blkp[5]<=2*loss))
				blkp[4] = 0;
			if ((blkp[5]>=-2*loss) && (blkp[5]<=2*loss))
				blkp[5] = 0;
			if ((blkp[6]>=-2*loss) && (blkp[6]<=2*loss))
				blkp[6] = 0;
			if ((blkp[7]>=-2*loss) && (blkp[7]<=2*loss))
				blkp[7] = 0;
			blkp = ((signed char *)block)+74;
			for (i=10; i<64; i+=6, blkp+=6) {
				if ((blkp[0]>=-2*loss) && (blkp[0]<=2*loss))
					blkp[0] = 0;
				if ((blkp[1]>=-2*loss) && (blkp[1]<=2*loss))
					blkp[1] = 0;
				if ((blkp[2]>=-2*loss) && (blkp[2]<=2*loss))
					blkp[2] = 0;
				if ((blkp[3]>=-2*loss) && (blkp[3]<=2*loss))
					blkp[3] = 0;
				if ((blkp[4]>=-2*loss) && (blkp[4]<=2*loss))
					blkp[4] = 0;
				if ((blkp[5]>=-2*loss) && (blkp[5]<=2*loss))
					blkp[5] = 0;
			}
		}
	}

	/* XXX: The ntohl() calls here need to be redone as something more
		efficient for little-endian machines! */
	u_int32_t* blkwp = block;
	u_int32_t* blkwLim = up? block+32 : block+16;
	int i = 2;
	int zcount = 0;
	u_int32_t blkw = ntohl(*blkwp++);
	pt_[1] = blkw >> 24;
	blkw <<= 8;
	int rem = 3;
	do {
		int b;
		while ((b = blkw >> 24) != 0) {
			pt_[i++] = b;
			blkw <<= 8;
			rem--;
			if (i == 4) break;
			if (rem == 0) {
				if (blkwp == blkwLim) break;
				blkw = ntohl(*blkwp++);
				rem = 4;
			}
		}

		while (1) {
			if (zcount+rem >= 63) {
				break;
			} else if (blkw == 0) {
				zcount += rem;
				rem = 0;
				if (blkwp == blkwLim) break;
				blkw = ntohl(*blkwp++);
				rem = 4;
			} else if ((blkw >> 24) == 0) {
				zcount++;
				blkw <<= 8;
				rem--;
			} else break;
		}

		*pt_ = ((i-1) << 6) + zcount;
		pt_ += i;
		if (pt_ > ep_)
			abort();
		i = 1;
		zcount = 0;
	} while ((rem != 0) || (blkwp != blkwLim));
}

int NvEncoder::consume(const VideoFrame* vf)
{
	if (!samesize(vf))
		size(vf->width_, vf->height_);
	YuvFrame* p = (YuvFrame*)vf;

	tx_->flush();

	pktbuf* pb = pool_->alloc(p->ts_, RTP_PT_NV);
	pt_ = &pb->data[HLEN];
	ep_ = pt_ + tx_->mtu() - HLEN;
	
	int cc = 0;
	u_char* runlen;
	int stride = 7 * width_;
	int off = framesize_;
	const u_char* lum = p->bp_;
	const u_char* chm = lum + off;
	off >>= 1;
	int w = width_ >> 3;
	int h = height_ >> 3;
	const u_int8_t* crvec = p->crvec_;
	for (int y = 0; y < h; ++y) {
		runlen = 0;
		for (int x = 0; x < w; ++x) {
			if (pt_ + MAXBLOCK >= ep_) {
				cc += flush(pb, 0);
				runlen = 0;
				pb = pool_->alloc(p->ts_, RTP_PT_NV);
				pt_ = &pb->data[HLEN];
				ep_ = pt_ + tx_->mtu() - HLEN;
			}
			int cr = *crvec;
			if (cr & CR_SEND) {
				cr = CR_STATE(cr);
				int loss;
				if (cr == CR_BG)
					loss = 0;
				else
					loss = loss_;

				putblock(lum, chm, chm + off,
					 x, y, loss, runlen);
			} else
				/*
				 * We're going to skip a block.
				 * Invalidate the run.
				 */
				runlen = 0;

			if (x & 1)
				++crvec;

			lum += 8;
			chm += 4;
		}
		if ((y & 1) == 0)
			crvec -= width_ / 16;

		lum += stride;
		chm += stride >> 1;
	}
	cc += flush(pb, 1);
	return (cc);
}

/*
 * Haar transform the block, the way nv expects it
 * (i.e., interleaved u,v)
 */
void NvEncoder::VidTransform_Fwd(const u_char *yp,
				 const u_char *up,
				 const u_char* vp,
				 int stride, u_int32_t *out) const
{
    register int i, t0, t1, t2, t3, t4, t5, *dataptr;
    register int stride2=2*stride, stride6=6*stride;
    register char *outcptr = (char *)out;
    static int block[64];

    dataptr = block;
    for (i=0; i<8; i++) {
	t0 = yp[0];
	t1 = yp[stride];
	t2 = t0 + t1;
	dataptr[32] = (t1 - t0) << 2;
	yp += stride2;

	t0 = yp[0];
	t1 = yp[stride];
	t3 = t0 + t1;
	dataptr[40] = (t1 - t0) << 2;
	yp += stride2;

	t0 = yp[0];
	t1 = yp[stride];
	t4 = t0 + t1;
	dataptr[48] = (t1 - t0) << 2;
	yp += stride2;

	t0 = yp[0];
	t1 = yp[stride];
	t5 = t0 + t1;
	dataptr[56] = (t1 - t0) << 2;
	yp -= stride6;

	t0 = t2 + t3;
	t1 = t4 + t5;
	dataptr[0]  = t0 + t1 - 1024; /* Correct for DC offset */
	dataptr[8]  = t1 - t0;
	dataptr[16] = (t3 - t2) << 1;
	dataptr[24] = (t5 - t4) << 1;

	yp++;
	dataptr++;
    }

    dataptr = block;
    for (i=0; i<8; i++) {
	t0 = dataptr[0];
	t1 = dataptr[1];
	t2 = t0 + t1;
	outcptr[4] = (t1 - t0 + 7) >> 4;

	t0 = dataptr[2];
	t1 = dataptr[3];
	t3 = t0 + t1;
	outcptr[5] = (t1 - t0 + 7) >> 4;

	t0 = dataptr[4];
	t1 = dataptr[5];
	t4 = t0 + t1;
	outcptr[6] = (t1 - t0 + 7) >> 4;

	t0 = dataptr[6];
	t1 = dataptr[7];
	t5 = t0 + t1;
	outcptr[7] = (t1 - t0 + 7) >> 4;

	t0 = t2 + t3;
	t1 = t4 + t5;
	outcptr[0] = (t0 + t1 + 31) >> 6;
	outcptr[1] = (t1 - t0 + 31) >> 6;
	outcptr[2] = (t3 - t2 + 15) >> 5;
	outcptr[3] = (t5 - t4 + 15) >> 5;

	dataptr += 8;
	outcptr += 8;
    }

    if (up != 0) {
	stride >>= 1;
	stride2 >>= 1;
	stride6 >>= 1;

        int uswitch = 1;
	const u_char* p = up++;
	dataptr = block;
	for (i=0; i<8; i++) {
	    t0 = p[0] - 0x80;
	    t1 = p[stride] - 0x80;
	    t2 = t0 + t1;
	    dataptr[32] = (t1 - t0) << 2;
	    p += stride2;

	    t0 = p[0] - 0x80;
	    t1 = p[stride] - 0x80;
	    t3 = t0 + t1;
	    dataptr[40] = (t1 - t0) << 2;
	    p += stride2;

	    t0 = p[0] - 0x80;
	    t1 = p[stride] - 0x80;
	    t4 = t0 + t1;
	    dataptr[48] = (t1 - t0) << 2;

	    t0 = p[0] - 0x80;
	    t1 = p[stride] - 0x80;
	    t5 = t0 + t1;
	    dataptr[56] = (t1 - t0) << 2;
	    p -= stride6;

	    t0 = t2 + t3;
	    t1 = t4 + t5;
	    dataptr[0]  = t0 + t1;
	    dataptr[8]  = t1 - t0;
	    dataptr[16] = (t3 - t2) << 1;
	    dataptr[24] = (t5 - t4) << 1;

	    if (uswitch) {
		    uswitch = 0;
		    p = vp++;
	    } else {
		    uswitch = 1;
		    p = up++;
	    }
	    dataptr++;
        }

	dataptr = block;
	for (i=0; i<8; i++) {
	    t0 = dataptr[0];
	    t1 = dataptr[2];
	    t2 = t0 + t1;
	    outcptr[4] = (t1 - t0 + 7) >> 4;

	    t0 = dataptr[4];
	    t1 = dataptr[6];
	    t3 = t0 + t1;
	    outcptr[5] = (t1 - t0 + 7) >> 4;

	    t0 = dataptr[1];
	    t1 = dataptr[3];
	    t4 = t0 + t1;
	    outcptr[6] = (t1 - t0 + 7) >> 4;

	    t0 = dataptr[5];
	    t1 = dataptr[7];
	    t5 = t0 + t1;
	    outcptr[7] = (t1 - t0 + 7) >> 4;

	    outcptr[0] = (t2 + t3 + 15) >> 5;
	    outcptr[1] = (t4 + t5 + 15) >> 5;
	    outcptr[2] = (t3 - t2 + 15) >> 5;
	    outcptr[3] = (t5 - t4 + 15) >> 5;

	    dataptr += 8;
	    outcptr += 8;
	}
    }
}

void NvEncoder::NVDCT_FwdTransform(const u_int8_t *yp,
				   const u_int8_t *up,
				   const u_int8_t* vp,
				   int width, u_int32_t *out) const
{
    int i, t0, t1, *dataptr;
    int a0, a1, a2, a3, b0, b1, b2, b3, c0, c1, c2, c3;
    int8_t *outcptr=(int8_t *)out;
    static int block[64];

    dataptr = block;
    for (i=0; i<8; i++) {
	t0 = yp[0];
	t1 = yp[7*width];
	a0 = t0+t1;
	c3 = t0-t1;
	yp += width;

	t0 = yp[0];
	t1 = yp[5*width];
	a1 = t0+t1;
	c2 = t0-t1;
	yp += width;

	t0 = yp[0];
	t1 = yp[3*width];
	a2 = t0+t1;
	c1 = t0-t1;
	yp += width;

	t0 = yp[0];
	t1 = yp[width];
	a3 = t0+t1;
	c0 = t0-t1;
	yp -= 3*width;

	b0 = a0+a3;
	b1 = a1+a2;
	b2 = a1-a2;
	b3 = a0-a3;

	dataptr[0]  = (362 * (b0+b1-1024))   >> 7; /* Correct for DC offset */
	dataptr[32] = (362 * (b0-b1))        >> 7;
	dataptr[16] = (196*b2 + 473*b3)      >> 7;
	dataptr[48] = (196*b3 - 473*b2)      >> 7;

	b0 = (362 * (c2-c1)) >> 7;
	b1 = (362 * (c2+c1)) >> 7;
	c0 = c0 << 2;
	c3 = c3 << 2;

	a0 = c0+b0;
	a1 = c0-b0;
	a2 = c3-b1;
	a3 = c3+b1;

	dataptr[8]  = (100*a0 + 502*a3) >> 9;
	dataptr[24] = (426*a2 - 284*a1) >> 9;
	dataptr[40] = (426*a1 + 284*a2) >> 9;
	dataptr[56] = (100*a3 - 502*a0) >> 9;

	yp++;
	dataptr++;
    }

    dataptr = block;
    for (i=0; i<8; i++) {
	t0 = dataptr[0];
	t1 = dataptr[7];
	a0 = t0+t1;
	c3 = t0-t1;

	t0 = dataptr[1];
	t1 = dataptr[6];
	a1 = t0+t1;
	c2 = t0-t1;

	t0 = dataptr[2];
	t1 = dataptr[5];
	a2 = t0+t1;
	c1 = t0-t1;

	t0 = dataptr[3];
	t1 = dataptr[4];
	a3 = t0+t1;
	c0 = t0-t1;

	b0 = a0+a3;
	b1 = a1+a2;
	b2 = a1-a2;
	b3 = a0-a3;

	outcptr[0] = (362 * (b0+b1)   + 32768) >> 16;
	outcptr[4] = (362 * (b0-b1)   + 32768) >> 16;
	outcptr[2] = (196*b2 + 473*b3 + 32768) >> 16;
	outcptr[6] = (196*b3 - 473*b2 + 32768) >> 16;

	b0 = (362 * (c2-c1)) >> 9;
	b1 = (362 * (c2+c1)) >> 9;

	a0 = c0+b0;
	a1 = c0-b0;
	a2 = c3-b1;
	a3 = c3+b1;

	outcptr[1] = (100*a0 + 502*a3 + 32768) >> 16;
	outcptr[3] = (426*a2 - 284*a1 + 32768) >> 16;
	outcptr[5] = (426*a1 + 284*a2 + 32768) >> 16;
	outcptr[7] = (100*a3 - 502*a0 + 32768) >> 16;

	dataptr += 8;
	outcptr += 8;
    }

    if (up) {
	int8_t uvblk[64];

	/* XXX interleave how nv wants it */
	int8_t* uvp = uvblk;
	for (i = 0; i < 8; ++i) {
		uvp[0] = up[0] + 0x80;
		uvp[1] = vp[0] + 0x80;
		uvp[2] = up[1] + 0x80;
		uvp[3] = vp[1] + 0x80;
		uvp[4] = up[2] + 0x80;
		uvp[5] = vp[2] + 0x80;
		uvp[6] = up[3] + 0x80;
		uvp[7] = vp[3] + 0x80;
		uvp += 8;
		up += width >> 1;
		vp += width >> 1;
	}
	uvp = uvblk;
#define width 8

	dataptr = block;
	for (i=0; i<8; i++) {
	    t0 = uvp[0];
	    t1 = uvp[7*width];
	    a0 = t0+t1;
	    c3 = t0-t1;
	    uvp += width;

	    t0 = uvp[0];
	    t1 = uvp[5*width];
	    a1 = t0+t1;
	    c2 = t0-t1;
	    uvp += width;

	    t0 = uvp[0];
	    t1 = uvp[3*width];
	    a2 = t0+t1;
	    c1 = t0-t1;
	    uvp += width;

	    t0 = uvp[0];
	    t1 = uvp[width];
	    a3 = t0+t1;
	    c0 = t0-t1;
	    uvp -= 3*width;

	    b0 = a0+a3;
	    b1 = a1+a2;
	    b2 = a1-a2;
	    b3 = a0-a3;

	    dataptr[0]  = (362 * (b0+b1))   >> 7;
	    dataptr[32] = (362 * (b0-b1))   >> 7;
	    dataptr[16] = (196*b2 + 473*b3) >> 7;
	    dataptr[48] = (196*b3 - 473*b2) >> 7;

	    b0 = (362 * (c2-c1)) >> 7;
	    b1 = (362 * (c2+c1)) >> 7;
	    c0 = c0 << 2;
	    c3 = c3 << 2;

	    a0 = c0+b0;
	    a1 = c0-b0;
	    a2 = c3-b1;
	    a3 = c3+b1;

	    dataptr[8]  = (100*a0 + 502*a3) >> 9;
	    dataptr[24] = (426*a2 - 284*a1) >> 9;
	    dataptr[40] = (426*a1 + 284*a2) >> 9;
	    dataptr[56] = (100*a3 - 502*a0) >> 9;


	    uvp++;
	    dataptr++;
	}

	dataptr = block;
	for (i=0; i<8; i++) {
	    a0 = dataptr[0];
	    a3 = dataptr[6];

	    a1 = dataptr[2];
	    a2 = dataptr[4];

	    b0 = a0+a3;
	    b1 = a1+a2;
	    b2 = a1-a2;
	    b3 = a0-a3;

	    outcptr[0] = (362 * (b0+b1)   + 16384) >> 15;
	    outcptr[4] = (362 * (b0-b1)   + 16384) >> 15;
	    outcptr[2] = (196*b2 + 473*b3 + 16384) >> 15;
	    outcptr[6] = (196*b3 - 473*b2 + 16384) >> 15;

	    a0 = dataptr[1];
	    a3 = dataptr[7];

	    a1 = dataptr[3];
	    a2 = dataptr[5];

	    b0 = a0+a3;
	    b1 = a1+a2;
	    b2 = a1-a2;
	    b3 = a0-a3;

	    outcptr[1] = (362 * (b0+b1)   + 16384) >> 15;
	    outcptr[5] = (362 * (b0-b1)   + 16384) >> 15;
	    outcptr[3] = (196*b2 + 473*b3 + 16384) >> 15;
	    outcptr[7] = (196*b3 - 473*b2 + 16384) >> 15;

	    dataptr += 8;
	    outcptr += 8;
	}
#undef width
    }
}

