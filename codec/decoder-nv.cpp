/*
 * This module was originally derived from:
 *
 * Netvideo version 3.2
 * Written by Ron Frederick <frederick@parc.xerox.com>
 *
 * Video decode routines
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
#include <string.h>
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "pktbuf.h"

class NvDecoder : public PlaneDecoder {
public:
	NvDecoder();
protected:
	void info(char* wrk) const;
	virtual void recv(pktbuf*);
	const u_char* decode_run(const u_char* data, const u_char* end,
				 int color);
	void VidTransform_Rev(u_int* inp, u_char* yp, char* up, char* vp,
			      int width);
	void NVDCT_RevTransform(u_int* inp, u_char *yp,
				char* up, char* vp, int width);
	int use_dct_;
};

static class NvDecoderMatcher : public Matcher {
public:
	NvDecoderMatcher() : Matcher("decoder") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "nv") == 0)
			return (new NvDecoder());
		return (0);
	}
} dm_nv;

#define STAT_BAD_GEOM	0
#define STAT_BAD_COORD	1
#define STAT_BAD_BLKRUN	2
#define STAT_BAD_RUNLEN	3
#define STAT_BAD_ENC	4

NvDecoder::NvDecoder() : PlaneDecoder(sizeof(nvhdr)), use_dct_(0)
{
	stat_[STAT_BAD_GEOM].name = "Nv-Bad-Geom";
	stat_[STAT_BAD_COORD].name = "Nv-Bad-Coord";
	stat_[STAT_BAD_BLKRUN].name = "Nv-Bad-Blkrun";
	stat_[STAT_BAD_RUNLEN].name = "Nv-Bad-Runlen";
	stat_[STAT_BAD_ENC].name = "Nv-Bad-Enc";
	nstat_ = 5;

	inw_ = 0;
	inh_ = 0;

}
void NvDecoder::info(char* wrk) const
{
	strcpy(wrk, use_dct_ ? "[dct]" : "[haar]");
}

/* Sick little macro which will limit x to [0..255] with logical ops */
#define UCLIMIT(x) ((t = (x)), (t &= ~(t>>31)), (t | ~((t-256) >> 31)))
/* A variant of above which will limit x to [-128..127] */
#define SCLIMIT(x) (UCLIMIT((x)+128)-128)

void NvDecoder::VidTransform_Rev(u_int* inp, u_char *yp,
				 char* up, char* vp, int width)
{
    register int i, t, t0, t1, t2, t3, t4, t5, *dataptr;
    register int width2=2*width, width6=6*width;
    register signed char *inpcptr=(signed char *)inp;
    static int block[64];
#define SIGN_EXTEND(c) c

    dataptr = block;
    for (i=0; i<8; i++) {
	if ((inp[0] << 8) == 0) {
	    t2 = t3 = t4 = t5 = SIGN_EXTEND(inpcptr[0]);
	} else {
	    t4 = SIGN_EXTEND(inpcptr[0]);
	    t5 = SIGN_EXTEND(inpcptr[1]);
	    t0 = t4 - t5;
	    t1 = t4 + t5;

	    t4 = SIGN_EXTEND(inpcptr[2]);
	    t5 = SIGN_EXTEND(inpcptr[3]);
	    t2 = t0 - t4;
	    t3 = t0 + t4;
	    t4 = t1 - t5;
	    t5 = t1 + t5;
	}

	if (inp[1] == 0) {
	    dataptr[0] = dataptr[1] = t2;
	    dataptr[2] = dataptr[3] = t3;
	    dataptr[4] = dataptr[5] = t4;
	    dataptr[6] = dataptr[7] = t5;
	} else {
	    t0 = SIGN_EXTEND(inpcptr[4]);
	    t1 = SIGN_EXTEND(inpcptr[5]);
	    dataptr[0] = t2 - t0;
	    dataptr[1] = t2 + t0;
	    dataptr[2] = t3 - t1;
	    dataptr[3] = t3 + t1;

	    t0 = SIGN_EXTEND(inpcptr[6]);
	    t1 = SIGN_EXTEND(inpcptr[7]);
	    dataptr[4] = t4 - t0;
	    dataptr[5] = t4 + t0;
	    dataptr[6] = t5 - t1;
	    dataptr[7] = t5 + t1;
	}

	inp += 2;
	inpcptr += 8;
	dataptr += 8;
    }

    dataptr = block;
    for (i=0; i<8; i++) {
	t4 = dataptr[0] + 128; /* Add back DC offset */
	t5 = dataptr[8];
	t0 = t4 - t5;
	t1 = t4 + t5;

	t4 = dataptr[16];
	t5 = dataptr[24];
	t2 = t0 - t4;
	t3 = t0 + t4;
	t4 = t1 - t5;
	t5 = t1 + t5;

	t0 = dataptr[32];
	t1 = dataptr[40];
	yp[0] = UCLIMIT(t2 - t0);
	yp[width] = UCLIMIT(t2 + t0);
	yp += width2;
	yp[0] = UCLIMIT(t3 - t1);
	yp[width] = UCLIMIT(t3 + t1);
	yp += width2;

	t0 = dataptr[48];
	t1 = dataptr[56];
	yp[0] = UCLIMIT(t4 - t0);
	yp[width] = UCLIMIT(t4 + t0);
	yp += width2;
	yp[0] = UCLIMIT(t5 - t1);
	yp[width] = UCLIMIT(t5 + t1);
	yp -= width6;

	yp++;
	dataptr++;
    }

    if (up) {
	int uswitch = 1;
	signed char* p = (signed char*)up++;
	dataptr = block;
	for (i=0; i<8; i++) {
	    if ((inp[0] << 16) == 0) {
		t2 = t3 = SIGN_EXTEND(inpcptr[0]);
		t4 = t5 = SIGN_EXTEND(inpcptr[1]);
	    } else {
		t0 = SIGN_EXTEND(inpcptr[0]);
		t1 = SIGN_EXTEND(inpcptr[1]);

		t4 = SIGN_EXTEND(inpcptr[2]);
		t5 = SIGN_EXTEND(inpcptr[3]);
		t2 = t0 - t4;
		t3 = t0 + t4;
		t4 = t1 - t5;
		t5 = t1 + t5;
	    }

	    if (inp[1] == 0) {
		dataptr[0] = dataptr[2] = t2;
		dataptr[4] = dataptr[6] = t3;
		dataptr[1] = dataptr[3] = t4;
		dataptr[5] = dataptr[7] = t5;
	    } else {
		t0 = SIGN_EXTEND(inpcptr[4]);
		t1 = SIGN_EXTEND(inpcptr[5]);
		dataptr[0] = t2 - t0;
		dataptr[2] = t2 + t0;
		dataptr[4] = t3 - t1;
		dataptr[6] = t3 + t1;

		t0 = SIGN_EXTEND(inpcptr[6]);
		t1 = SIGN_EXTEND(inpcptr[7]);
		dataptr[1] = t4 - t0;
		dataptr[3] = t4 + t0;
		dataptr[5] = t5 - t1;
		dataptr[7] = t5 + t1;
	    }

	    inp += 2;
	    inpcptr += 8;
	    dataptr += 8;
	}

	dataptr = block;
	for (i=0; i<8; i++) {
	    t4 = dataptr[0];
	    t5 = dataptr[8];
	    t0 = t4 - t5;
	    t1 = t4 + t5;

	    t4 = dataptr[16];
	    t5 = dataptr[24];
	    t2 = t0 - t4;
	    t3 = t0 + t4;
	    t4 = t1 - t5;
	    t5 = t1 + t5;

	    t0 = dataptr[32];
	    t1 = dataptr[40];

	    *p = SCLIMIT(t2 - t0) + 0x80;
	    p += width / 2;
	    *p = SCLIMIT(t2 + t0) + 0x80;
	    p += width / 2;
	    *p = SCLIMIT(t3 - t1) + 0x80;
	    p += width / 2;
	    *p = SCLIMIT(t3 + t1) + 0x80;
	    p += width / 2;
	    
	    t0 = dataptr[48];
	    t1 = dataptr[56];
	    *p = SCLIMIT(t4 - t0) + 0x80;
	    p += width / 2;
	    *p = SCLIMIT(t4 + t0) + 0x80;
	    p += width / 2;
	    *p = SCLIMIT(t5 - t1) + 0x80;
	    p += width / 2;
	    *p = SCLIMIT(t5 + t1) + 0x80;
	    p += width / 2;

	    if (uswitch) {
		    uswitch = 0;
		    p = (signed char*)vp++;
	    } else {
		    uswitch = 1;
		    p = (signed char*)up++;
	    }
	    dataptr++;
	}
    }
}

void NvDecoder::NVDCT_RevTransform(u_int* inp, u_char *yp,
				   char* up, char* vp, int width)
{
    int i, t, *dataptr;
    int a0, a1, a2, a3, b0, b1, b2, b3, c0, c1, c2, c3;
    int8_t *inpcptr=(int8_t *)inp;
    static int block[64];

    dataptr = block;
    for (i=0; i<8; i++) {
	if ((inp[0]|inp[1]) == 0) {
	    dataptr[0] = dataptr[1] = dataptr[2] = dataptr[3] =
		dataptr[4] = dataptr[5] = dataptr[6] = dataptr[7] = 0;
	} else {
	    b0 = inpcptr[0] << 4;
	    b1 = inpcptr[4] << 4;
	    b2 = inpcptr[2] << 4;
	    b3 = inpcptr[6] << 4;

	    a0 = (362 * (b0+b1)) >> 9;
	    a1 = (362 * (b0-b1)) >> 9;
	    a2 = (196*b2 - 473*b3) >> 9;
	    a3 = (473*b2 + 196*b3) >> 9;

	    b0 = a0+a3;
	    b1 = a1+a2;
	    b2 = a1-a2;
	    b3 = a0-a3;

	    a0 = inpcptr[1] << 4;
	    a1 = inpcptr[3] << 4;
	    a2 = inpcptr[5] << 4;
	    a3 = inpcptr[7] << 4;

	    c0 = (100*a0 - 502*a3) >> 9;
	    c1 = (426*a2 - 284*a1) >> 9;
	    c2 = (426*a1 + 284*a2) >> 9;
	    c3 = (502*a0 + 100*a3) >> 9;

	    a0 = c0+c1;
	    a1 = c0-c1;
	    a2 = c3-c2;
	    a3 = c3+c2;

	    c0 = a0;
	    c1 = (362 * (a2-a1)) >> 9;
	    c2 = (362 * (a1+a2)) >> 9;
	    c3 = a3;

	    dataptr[0] = b0+c3;
	    dataptr[1] = b1+c2;
	    dataptr[2] = b2+c1;
	    dataptr[3] = b3+c0;
	    dataptr[4] = b3-c0;
	    dataptr[5] = b2-c1;
	    dataptr[6] = b1-c2;
	    dataptr[7] = b0-c3;
	}

	inp += 2;
	inpcptr += 8;
	dataptr += 8;
    }

    dataptr = block;
    for (i=0; i<8; i++) {
	b0 = dataptr[0]+1448; /* Add back DC offset */
	b1 = dataptr[32];
	b2 = dataptr[16];
	b3 = dataptr[48];

	a0 = (362 * (b0+b1)) >> 9;
	a1 = (362 * (b0-b1)) >> 9;
	a2 = (196*b2 - 473*b3) >> 9;
	a3 = (473*b2 + 196*b3) >> 9;

	b0 = a0+a3;
	b1 = a1+a2;
	b2 = a1-a2;
	b3 = a0-a3;

	a0 = dataptr[8];
	a1 = dataptr[24];
	a2 = dataptr[40];
	a3 = dataptr[56];

	c0 = (100*a0 - 502*a3) >> 9;
	c1 = (426*a2 - 284*a1) >> 9;
	c2 = (426*a1 + 284*a2) >> 9;
	c3 = (502*a0 + 100*a3) >> 9;

	a0 = c0+c1;
	a1 = c0-c1;
	a2 = c3-c2;
	a3 = c3+c2;

	c0 = a0;
	c1 = (362 * (a2-a1)) >> 9;
	c2 = (362 * (a2+a1)) >> 9;
	c3 = a3;

	yp[0]     = UCLIMIT((b0+c3+4) >> 3);
	yp[width] = UCLIMIT((b1+c2+4) >> 3);
	yp += 2*width;

	yp[0]     = UCLIMIT((b2+c1+4) >> 3);
	yp[width] = UCLIMIT((b3+c0+4) >> 3);
	yp += 2*width;

	yp[0]     = UCLIMIT((b3-c0+4) >> 3);
	yp[width] = UCLIMIT((b2-c1+4) >> 3);
	yp += 2*width;

	yp[0]     = UCLIMIT((b1-c2+4) >> 3);
	yp[width] = UCLIMIT((b0-c3+4) >> 3);
	yp -= 6*width;

	yp++;
	dataptr++;
    }

    if (up) {
	u_int8_t uvblk[64];
	u_int8_t* uvp = uvblk;
#define width 8
	dataptr = block;
	for (i=0; i<8; i++) {
	    if ((inp[0]|inp[1]) == 0) {
		dataptr[0] = dataptr[1] = dataptr[2] = dataptr[3] =
		    dataptr[4] = dataptr[5] = dataptr[6] = dataptr[7] = 0;
	    } else {
		b0 = inpcptr[0] << 4;
		b2 = inpcptr[2] << 4;
		b1 = inpcptr[4] << 4;
		b3 = inpcptr[6] << 4;

		a0 = (362 * (b0+b1)) >> 9;
		a1 = (362 * (b0-b1)) >> 9;
		a2 = (196*b2 - 473*b3) >> 9;
		a3 = (473*b2 + 196*b3) >> 9;

		dataptr[0] = a0+a3;
		dataptr[2] = a1+a2;
		dataptr[4] = a1-a2;
		dataptr[6] = a0-a3;

		b0 = inpcptr[1] << 4;
		b2 = inpcptr[3] << 4;
		b1 = inpcptr[5] << 4;
		b3 = inpcptr[7] << 4;

		a0 = (362 * (b0+b1)) >> 9;
		a1 = (362 * (b0-b1)) >> 9;
		a2 = (196*b2 - 473*b3) >> 9;
		a3 = (473*b2 + 196*b3) >> 9;

		dataptr[1] = a0+a3;
		dataptr[3] = a1+a2;
		dataptr[5] = a1-a2;
		dataptr[7] = a0-a3;
	    }

	    inp += 2;
	    inpcptr += 8;
	    dataptr += 8;
	}

	dataptr = block;
	for (i=0; i<8; i++) {
	    b0 = dataptr[0];
	    b1 = dataptr[32];
	    b2 = dataptr[16];
	    b3 = dataptr[48];

	    a0 = (362 * (b0+b1)) >> 9;
	    a1 = (362 * (b0-b1)) >> 9;
	    a2 = (196*b2 - 473*b3) >> 9;
	    a3 = (473*b2 + 196*b3) >> 9;

	    b0 = a0+a3;
	    b1 = a1+a2;
	    b2 = a1-a2;
	    b3 = a0-a3;

	    a0 = dataptr[8];
	    a1 = dataptr[24];
	    a2 = dataptr[40];
	    a3 = dataptr[56];

	    c0 = (100*a0 - 502*a3) >> 9;
	    c1 = (426*a2 - 284*a1) >> 9;
	    c2 = (426*a1 + 284*a2) >> 9;
	    c3 = (502*a0 + 100*a3) >> 9;

	    a0 = c0+c1;
	    a1 = c0-c1;
	    a2 = c3-c2;
	    a3 = c3+c2;

	    c0 = a0;
	    c1 = (362 * (a2-a1)) >> 9;
	    c2 = (362 * (a2+a1)) >> 9;
	    c3 = a3;

	    uvp[0]     = SCLIMIT((b0+c3+4) >> 3);
	    uvp[width] = SCLIMIT((b1+c2+4) >> 3);
	    uvp += 2*width;

	    uvp[0]     = SCLIMIT((b2+c1+4) >> 3);
	    uvp[width] = SCLIMIT((b3+c0+4) >> 3);
	    uvp += 2*width;

	    uvp[0]     = SCLIMIT((b3-c0+4) >> 3);
	    uvp[width] = SCLIMIT((b2-c1+4) >> 3);
	    uvp += 2*width;

	    uvp[0]     = SCLIMIT((b1-c2+4) >> 3);
	    uvp[width] = SCLIMIT((b0-c3+4) >> 3);
	    uvp -= 6*width;

	    uvp++;
	    dataptr++;
	}
#undef width
	uvp = uvblk;
	for (i = 0; i < 8; ++i) {
		up[0] = uvp[0] + 0x80;
		vp[0] = uvp[1] + 0x80;
		up[1] = uvp[2] + 0x80;
		vp[1] = uvp[3] + 0x80;
		up[2] = uvp[4] + 0x80;
		vp[2] = uvp[5] + 0x80;
		up[3] = uvp[6] + 0x80;
		vp[3] = uvp[7] + 0x80;
		up += width >> 1;
		vp += width >> 1;
		uvp += 8;
	}
    }
}

#define VIDCODE_COLORFLAG 0x8000
#define VIDCODE_WIDTHMASK 0x0fff

const u_char* NvDecoder::decode_run(const u_char* data, const u_char* end,
				    int color)
{
	if (end - data < 3)
		return (end);

	int w0, w;
	w0 = w = *data++;
	int x0 = *data++;
	int y0 = *data++;

	int inw = inw_;
	int inh = inh_;
	if ((x0+w > inw/8) || (y0 >= inh/8)) {
		count(STAT_BAD_COORD);
#ifdef notdef
		fprintf(stderr, "vic: bogus offset in nv stream (%d,%d)\n",
			x0 + w, y0);
#endif
		return (end);
	}

	int offset = y0 * inw;
	u_char* ts = &rvts_[(offset >> 3) + x0];
	offset = (offset << 3) + (x0 << 3);
	u_char* yp = &frm_[offset];
	int s = inw * inh;/*XXX*/
	u_char* up = frm_ + s + (offset >> 1);
	u_char* vp = up + (s >> 1);
	while (w-- > 0) {
		if (data >= end) {
			count(STAT_BAD_BLKRUN);
			return (end);
		}
		static u_int block[32];
		int lim=color? 128 : 64;
		for (int i=0; i<lim; ) {
			int run = *data++;
			int j = run >> 6;
			int k = run & 0x3f;
			if (i+j+k > lim) {
#ifdef notdef
				fprintf(stderr,
			"vic: bogus run length in nv stream (%d %d %d %d %d)\n",
					i, j, k, lim, end - data);
#endif
				count(STAT_BAD_RUNLEN);
				return (end);
			}
			char* blkp = (char*)block;
			while (j--) blkp[i++] = (char) *data++;
			while (k--) blkp[i++] = 0;
			ndblk_++;
		}
		if (use_dct_)
			NVDCT_RevTransform(block, yp, (char*)up, (char*)vp,
					   inw);
		else
			VidTransform_Rev(block, yp, (char*)up, (char*)vp, inw);
		
		yp += 8;
		if (up) {
			up += 4;
			vp += 4;
		}
		*ts++ = now_;
	}
	return (data);
}

void NvDecoder::recv(pktbuf* pb)
{
	rtphdr* rh = (rtphdr*)pb->dp;
	const nvhdr* ph = (nvhdr*)(rh + 1);
	int h = ntohs(ph->height);
	int w = ntohs(ph->width);
	int color = w >> 15;
	w &= 0xfff;
	int fmt = h >> 12;
	h &= 0xfff;
	if (fmt == 0)
		use_dct_ = 0;
	else if (fmt == 1)
		use_dct_ = 1;
	else
		count(STAT_BAD_ENC);
#ifdef notyet
	if (color != color_)
		/*XXX should do this by invoking tcl command */
		setcolor(color);
#endif
	if (w != inw_ || h != inh_) {
		if ((unsigned)w > 1000 || (unsigned)h > 1000) {
			/*XXX*/
			w = 320;
			h = 240;
			count(STAT_BAD_GEOM);
		}
		resize(w, h);
	}
	const u_int8_t* end = &pb->dp[pb->len];
	const u_int8_t* bp = (u_int8_t*)(ph + 1);
	while (bp < end)
		bp = decode_run(bp, end, color);

	if (ntohs(rh->rh_flags) & RTP_M) {
		render_frame(frm_);
		resetndblk();
	}
	pb->release();
}
