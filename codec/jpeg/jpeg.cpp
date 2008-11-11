/*
 * Copyright (c) 1993-1995 Regents of the University of California.
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
 *
 */
/*
 * This code is derived from the Independent JPEG Group's JPEG software:
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying
 * README.IJPG file.
 */

#include "jpeg.h"
#include "bsd-endian.h"
#include "dct.h"

#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
//#include <winsock.h>
#else
#include <sys/param.h>
#include <netinet/in.h>
#endif
#include <string.h>

#define NCC 6	/* # of components to check for CR */
#define	BMB 6	/* # of (8x8) blocks in a "macroblock" [2xY+U+V] */
#define	DCT_GRAY	(0x80 * 8)	// DCT gray value

#define	EVEN(x)	((x & 1) == 0)
#define	ODD(x)	((x & 1) == 1)

//
// derived decoders (420-pixel, 422-pixel, 420-DCT, and 422-DCT)
//


class JpegDecoder_420 : public JpegPixelDecoder {
public:
	JpegDecoder_420(const config&, int, int);
	virtual int decode(const u_char* in, int len, u_char *marks, int mark);
};

class JpegDecoder_422 : public JpegPixelDecoder {
public:
	JpegDecoder_422(const config&, int, int);
	virtual int decode(const u_char* in, int len, u_char *marks, int mark);
};

class JpegDCTDecoder_420 : public JpegDCTDecoder {
public:
	JpegDCTDecoder_420(const config&, int, int);
	virtual int decode(const u_char* in, int len, u_char *marks, int mark);
};

class JpegDCTDecoder_422 : public JpegDCTDecoder {
 public:
	JpegDCTDecoder_422(const config&, int, int);
	virtual int decode(const u_char* in, int len, u_char *marks, int mark);
};

JpegPixelDecoder::JpegPixelDecoder(const config &c, int dec, int ow, int oh)
		 :JpegDecoder(c, dec, ow, oh)
{
	int ns = NCC * owidth_ * oheight_ / 64;	// # blocks
	cache_ = new short[ns];
	/*
	 * Initialize to some "large value" so threshold
	 * is exceeded on first pass.
	 */
	memset(cache_, 0x7f, ns * sizeof(*cache_));
	setlrskips();
}

JpegPixelDecoder::~JpegPixelDecoder() 
{
	delete[] cache_; //SV-XXX: Debian
	delete[] frm_; //SV-XXX: Debian
}

JpegDCTDecoder::JpegDCTDecoder(const config& c, int dec, int ow, int oh)
	       :JpegDecoder(c, dec, ow, oh)
{
	setlrskips();
}

JpegDCTDecoder::~JpegDCTDecoder() 
{
	delete[] frm_; //SV-XXX: Debian
}

JpegDecoder_420::JpegDecoder_420(const config& c, int ow, int oh) 
	        :JpegPixelDecoder(c, 420, ow, oh)
{
	int n = osize_ + osize_ / 2;
	frm_ = new u_char[n];		// pixel store
	memset(frm_, 0x80, n);
	dct_unbias_ = 0;
}

JpegDCTDecoder_420::JpegDCTDecoder_420(const config& c, int ow, int oh) 
		   :JpegDCTDecoder(c, 420, ow, oh)
{
	int n = osize_ + osize_ / 2;
	frm_ = new short[n];		// DCT store
	memset(frm_, 0x0, n * sizeof(short));
	dct_unbias_ = 1;

	short *sp = (short *)frm_;
	for (register int i = 0; i < n / (BMB * 64) ; i++) {
		*sp = DCT_GRAY;
		sp += 64;
		*sp = DCT_GRAY;
		sp += 64;
		*sp = DCT_GRAY;
		sp += 64;
		*sp = DCT_GRAY;
		sp += 64;
		*sp = DCT_GRAY;
		sp += 64;
		*sp = DCT_GRAY;
		sp += 64;
	}
}

JpegDecoder_422::JpegDecoder_422(const config& c, int ow, int oh) 
	        :JpegPixelDecoder(c, 422, ow, oh)
{
	int n = osize_ * 2;
	frm_ = new u_char[n];		// pixel store
	memset(frm_, 0x80, n);
	dct_unbias_ = 0;
}

JpegDCTDecoder_422::JpegDCTDecoder_422(const config& c, int ow, int oh) 
		   :JpegDCTDecoder(c, 422, ow, oh)
{
	int n = osize_ * 2;
	frm_ = new short[n];		// DCT store
	memset(frm_, 0x0, n * sizeof(short));
	dct_unbias_ = 1;

	short *sp = (short*)frm_;
	for (register int i = 0; i < n / (BMB * 64) ; i++) {
		*sp = DCT_GRAY;
		sp += 64;
		*sp = DCT_GRAY;
		sp += 64;
		*sp = DCT_GRAY;
		sp += 64;
		*sp = DCT_GRAY;
		sp += 64;
		*sp = DCT_GRAY;
		sp += 64;
		*sp = DCT_GRAY;
		sp += 64;
	}
}

JpegPixelDecoder* JpegPixelDecoder::create(const config& c, int ow, int oh)
{
	if (c.ncomp == 3 && c.comp[0].hsf == 2 &&
	    c.comp[1].hsf == 1 && c.comp[1].vsf == 1 &&
	    c.comp[2].hsf == 1 && c.comp[2].vsf == 1) {	

		// 420 decoder, RTP type 1, vsf = 2
		if (c.comp[0].vsf == 2)
				return (new JpegDecoder_420(c, ow, oh));

		// 422 decoder, RTP type 0, vsf = 1
		if (c.comp[0].vsf == 1)
				return (new JpegDecoder_422(c, ow, oh));
	}
	return (0);
}

JpegDCTDecoder* JpegDCTDecoder::create(const config& c, int ow, int oh)
{
	if (c.ncomp == 3 && c.comp[0].hsf == 2 &&
	    c.comp[1].hsf == 1 && c.comp[1].vsf == 1 &&
	    c.comp[2].hsf == 1 && c.comp[2].vsf == 1) {	

		// 420 decoder, RTP type 1, vsf = 2
		if (c.comp[0].vsf == 2) 
			return (new JpegDCTDecoder_420(c, ow, oh));

		// 422 decoder, RTP type 0, vsf = 1
		if (c.comp[0].vsf == 1)
			return (new JpegDCTDecoder_422(c, ow, oh));
	}
	return (0);
}
	
int quality_to_qfactor(int v)
{
	if (v < 1)
		v = 5000;
	else if (v < 50)
		v = 5000 / v;
	else if (v < 100)
		v = 200 - v * 2;
	else 
		v = 1;

	return (v);
}


/*
 * Table K.1 from JPEG spec.
 */
static const int jpeg_luma_quantizer[64] = {
	16, 11, 10, 16, 24, 40, 51, 61,
	12, 12, 14, 19, 26, 58, 60, 55,
	14, 13, 16, 24, 40, 57, 69, 56,
	14, 17, 22, 29, 51, 87, 80, 62,
	18, 22, 37, 56, 68, 109, 103, 77,
	24, 35, 55, 64, 81, 104, 113, 92,
	49, 64, 78, 87, 103, 121, 120, 101,
	72, 92, 95, 98, 112, 100, 103, 99
};

/*
 * Table K.2 from JPEG spec.
 */
static const int jpeg_chroma_quantizer[64] = {
	17, 18, 24, 47, 99, 99, 99, 99,
	18, 21, 26, 66, 99, 99, 99, 99,
	24, 26, 56, 99, 99, 99, 99, 99,
	47, 66, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
};

void jpeg_qt(int q, int* out, const int* in)
{
	q = quality_to_qfactor(q);
	for (int i = 0; i < 64; i++) {
		int val = (q * in[i] + 50) / 100;
		/*
		 * Baseline JPEG restricts range to [1,255].
		 */
		if (val < 1)
			val = 1;
		else if (val > 255)
			val = 255;
		out[i] = val;
	}
}

void jpeg_luma_qt(int q, int* qt)
{
	jpeg_qt(q, qt, jpeg_luma_quantizer);
}

void jpeg_chroma_qt(int q, int* qt)
{
	jpeg_qt(q, qt, jpeg_chroma_quantizer);
}

static const unsigned char dc_luminance_bits[17] =
    { /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
static const unsigned char dc_luminance_val[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
static const unsigned char dc_chrominance_bits[17] =
    { /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
static const unsigned char dc_chrominance_val[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
static const unsigned char ac_luminance_bits[17] =
    { /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
static const unsigned char ac_luminance_val[] =
    { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
      0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
      0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
      0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
      0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
      0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
      0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
      0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
      0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
      0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
      0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
      0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
      0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
      0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
      0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
      0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
      0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
      0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
      0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
      0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  
static const unsigned char ac_chrominance_bits[17] =
    { /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
static const unsigned char ac_chrominance_val[] =
    { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
      0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
      0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
      0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
      0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
      0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
      0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
      0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
      0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
      0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
      0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
      0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
      0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
      0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
      0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
      0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
      0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
      0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
      0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
      0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };

void JpegDecoder::defaults(JpegDecoder::config& c)
{
	c.width = 0;
	c.height = 0;

	c.precision = 8;
	c.ncomp = 3;
	/* Y */
	c.comp[0].id = 0;
	c.comp[0].hsf = 2;
	c.comp[0].vsf = 1;	// RTP type 0, 4:2:2 JPEG
	c.comp[0].qno = 0;
	/* U */
	c.comp[1].id = 1;
	c.comp[1].hsf = 1;
	c.comp[1].vsf = 1;
	c.comp[1].qno = 1;
	/* V */
	c.comp[2].id = 2;
	c.comp[2].hsf = 1;
	c.comp[2].vsf = 1;
	c.comp[2].qno = 1;
	
	c.dc_huffbits[0] = dc_luminance_bits;
	c.dc_huffval[0] = dc_luminance_val;
	c.dc_huffbits[1] = dc_chrominance_bits;
	c.dc_huffval[1] = dc_chrominance_val;
	c.ac_huffbits[0] = ac_luminance_bits;
	c.ac_huffval[0] = ac_luminance_val;
	c.ac_huffbits[1] = ac_chrominance_bits;
	c.ac_huffval[1] = ac_chrominance_val;
	for (int i = 2; i < 4; ++i) {
		c.ac_huffbits[i] = 0;
		c.ac_huffval[i] = 0;
		c.dc_huffbits[i] = 0;
		c.dc_huffval[i] = 0;
	}
	c.comp[0].dc_tbl_no = 0;
	c.comp[0].ac_tbl_no = 0;
	c.comp[1].dc_tbl_no = 1;
	c.comp[1].ac_tbl_no = 1;
	c.comp[2].dc_tbl_no = 1;
	c.comp[2].ac_tbl_no = 1;
}

/*
 * Set the quantizer for this configuration.
 * Q is the IJPG quality factor, which has a value in [0,100].
 */
void JpegDecoder::quantizer(JpegDecoder::config& c, int q)
{
	jpeg_luma_qt(q, c.qtab[0]);
	jpeg_chroma_qt(q, c.qtab[1]);
	memset(c.qtab[2], 0, sizeof(c.qtab[2]));
	memset(c.qtab[3], 0, sizeof(c.qtab[3]));
}

int JpegDecoder::q_to_thresh(int q)
{
	int s = q / 10;
	if (q <= 30 || q >= 80)
		s += 1;
	return (s);
}

//SV-XXX: rearranged initialisation order to sgut up gcc4
JpegDecoder::JpegDecoder(const config& c, int decimation, int ow, int oh)
	: ndblk_(0), decimation_(decimation), width_(-1), height_(-1), 
	  owidth_(ow), oheight_(oh), color_(1), thresh_(0), cthresh_(6)
{
	// initialize huffman tables
	for (int i = NUM_HUFF_TBLS; --i >= 0; ) {
		dcht_[i] = 0;
		acht_[i] = 0;
	}
	memset((char*)comp_, 0, sizeof(comp_));
	init(c);
}

JpegDecoder::~JpegDecoder()
{
	freehufftab();
}

/*
 * init() is called only by the constructor above
 */
void JpegDecoder::init(const config& c)
{
	rlen_ = 0;

	osize_ = owidth_ * oheight_;	// output size
	memcpy((char*)qt_, (char*)c.qtab, sizeof(qt_));
	int i;
	for (i = 0; i < 4; ++i)
		rdct_fold_q(qt_[i], fqt_[i]);

	width_ = c.width;		// input w/h
	height_ = c.height;
	size_ = width_ * height_;	// input size

	ncomp_ = c.ncomp;/*XXX*/
	for (i = ncomp_; --i >= 0; ) {
		int id = c.comp[i].id;
		comp_[id].hsf = c.comp[i].hsf;
		comp_[id].vsf = c.comp[i].vsf;
		comp_[id].qno = c.comp[i].qno;
		comp_[id].dc_tbl_no = c.comp[i].dc_tbl_no;
		comp_[id].ac_tbl_no = c.comp[i].ac_tbl_no;
	}
	/*
	 * XXX should check if huffman table won't change
	 * before reallocating.
	 */
	freehufftab();
	for (i = 0; i < 4; ++i) {
		if (c.dc_huffval[i] != 0) {
			int id = c.comp[i].id;
			dcht_[id] = huffbuild(c.dc_huffbits[i],
					      c.dc_huffval[i]);
		}
		if (c.ac_huffval[i] != 0) {
			int id = c.comp[i].id;
			acht_[id] = huffbuild(c.ac_huffbits[i],
					      c.ac_huffval[i]);
		}
	}

	int maxh = 1;
	int maxv = 1;
	for (i = ncomp_; --i >= 0; ) {
		if (maxh < comp_[i].hsf)
			maxh = comp_[i].hsf;
		if (maxv < comp_[i].vsf)
			maxv = comp_[i].vsf;
	}

	// # rows and cols in input JPEG image
	ncol_ = (width_ + 8 * maxh - 1) / (8 * maxh);
	nrow_ = (height_ + 8 * maxv - 1) / (8 * maxv);
 
	if (compute_margins(maxh, maxv) != 0)  {
		fprintf(stderr, 
			"JpegDecoder::init: couldn't compute margins\n");
	}
		
}


int JpegDecoder::compute_margins(int maxh, int maxv)
{
	/*
	 * Embed one image size in a bigger one.
	 * (extra areas will already be gray from init() )
	 */
	margin& m = margin_;
	m.top = m.left = m.right = 0;
	lcrop_ = 0;
	rcrop_ = ncol_;
	topcrop_ = 0;
	botcrop_ = nrow_;

	int dx = owidth_ - width_;
	int dy = oheight_ - height_;

	/*
	 * If output size is smaller that intput, crop input image to fit 
	 * output size, otherwise try and center the image in the larger 
	 * frame, making sure to align on macroblock (16x16) boundaries.
	 */
	if (dx < 0) {
		dx = -dx;
		/* Bail if can't align on block boundary. */
		if (dx % 8 != 0)
			return (-1);

		int q = dx / 16;
		if (q % 2 != 0) {
			/* Can't center crop horizontally */
			lcrop_ = (((dx - 16) / 2) + 8 * maxh - 1) / (8 * maxh);
			rcrop_ = ncol_ - lcrop_ + 1;
		} else {
			/* Center crop horizontally */
			lcrop_ = ((dx / 2) + 8 * maxh - 1) / (8 * maxh);
			rcrop_ = ncol_ - lcrop_;
		}
	} else {
		if (dx % 8 != 0)
			return (-1);

		/* Know dx is multiple of 8 at least */
		if (dx % 16 == 8) {
			/* crop one column and bias dx */
			ncol_--;
			dx -= 8;
		}

		int q = dx / 16;
		if (q % 2 != 0) {
			/* Can't center horizontally */
			m.right = (dx - 16) / 2;
			m.left = (dx + 16) / 2;
		} else {
			/* Center horizontally */
			m.right = m.left = dx / 2;
		}
	}

	if (dy < 0) {
		dy = -dy;
		/* Bail if can't align on block boundary. */
		if (dy % 8 != 0)
			return (-1);

		int q = dy / 16;
		if (q % 2 != 0) {
			/* Can't center crop vertically */
			topcrop_ = (((dy - 16) / 2) + 8 * maxv - 1) / 
				(8 * maxv);
			botcrop_ = nrow_ - topcrop_ + 1;
		} else {
			/* Center crop vertically */
			topcrop_ = ((dy / 2) + 8 * maxv - 1) / (8 * maxv);
			botcrop_ = nrow_ - topcrop_;
		}
	} else {
		/* Bail if can't align on block boundary. */
		if (dy % 8 != 0)
			return (-1);
		/* Know dy is multiple of 8 at least */
		if (dy % 16 == 8) {
			/* crop one row and bias dy */
			nrow_--;
			dy -= 8;
		}
		int q = dy / 16;
		if (q % 2 != 0) {
			/* Can't center vertically */
			m.top = (dy - 16) / 2;
		} else {
			/* Center vertically */
			m.top = dy / 2;
		}
	}

	
	return (0);
}

void JpegDCTDecoder::setlrskips()
{
	margin& m = margin_;

	// marks in CRvec relate to 16x16 DCT blocks

	m.flskip = m.left / 16 * (BMB * 64);
	m.frskip = m.right / 16 * (BMB * 64);
	m.ftopskip = m.top / 16 * owidth_ / 16 * (BMB * 64);
	m.marktopskip = m.top / 16 * owidth_ / 16;
	m.marklskip = m.left / 16;
	m.markrskip = m.right / 16;
}
	   
void JpegPixelDecoder::setlrskips()
{
	margin& m = margin_;

	m.ylskip = m.left;
	m.uvlskip = m.left / 2;

	m.yrskip = m.right;
	m.uvrskip = m.right / 2;

	m.ytopskip = m.top * owidth_;
	if (decimation_ == 422)
		m.uvtopskip = m.ytopskip / 128 * 64;
	else
		m.uvtopskip = m.ytopskip / 256 * 64;


	m.marktopskip = m.top / 8 * owidth_ / 8;
	m.marklskip = m.left / 8;
	m.markrskip = m.right / 8;
}
	
void JpegDecoder::freehufftab()
{
	for (int i = 0; i < 4; ++i) {
		if (dcht_[i] != 0)
			free(dcht_[i]);
		if (acht_[i] != 0)
			free(acht_[i]);
	}
}

#if NCC != 6

@BUG in blkdiff@
#endif
#define ABS(t) ((t) - (((t) >> 31 & (t)) << 1))
inline int zag_blkdiff(short* blk, short* ref)
{
	int t = blk[0] - ref[0];
	int d = ABS(t);
	t = blk[1] - ref[8];
	d += ABS(t);
	t = blk[2] - ref[1];
	d += ABS(t);
	t = blk[3] - ref[2];
	d += ABS(t);
	t = blk[4] - ref[9];
	d += ABS(t);
	t = blk[5] - ref[16];
	d += ABS(t);
	return (d);
}

inline int inorder_blkdiff(short* blk, short* ref)
{
	int t = blk[0] - ref[0];
	int d = ABS(t);
	t = blk[1] - ref[1];
	d += ABS(t);
	t = blk[2] - ref[2];
	d += ABS(t);
	t = blk[3] - ref[3];
	d += ABS(t);
	t = blk[4] - ref[4];
	d += ABS(t);
	t = blk[5] - ref[5];
	d += ABS(t);
	return (d);
}

#ifdef notdef
/* XXX this is negligibly faster than above, and has a bug */
inline int blkdiff(short* blk, short* cache)
{
	u_int* p0 = (u_int*)blk;
	u_int* p1 = (u_int*)cache;
	
	int m = ~0x80008000;
	int t = (p0[0] >> 1) & m;
	t += (~p1[0] >> 1) & m;
	int v = t << 17 >> 17;
	int d = ABS(v);
	t = (t << 1) >> 17;
	d += ABS(v);

	t = (p0[1] >> 1) & m;
	t += (~p1[1] >> 1) & m;
	v = t << 17 >> 17;
	d += ABS(v);
	t = (t << 1) >> 17;
	d += ABS(v);

	t = (p0[2] >> 1) & m;
	t += (~p1[2] >> 1) & m;
	v = t << 17 >> 17;
	d += ABS(v);
	t = (t << 1) >> 17;
	d += ABS(v);

	return (d);
}
#endif

int JpegDecoder::rdqt(const u_char* p)
{
	int len = *p++ << 8;
	len |= *p++;
  
	const u_char* ep = p + len - 2;
	while (p < ep) {
		int n = *p++;
		int prec = n >> 4;
		n &= 0x0f;
		if (n >= 4) {
			/*XXX illegal number*/
			return (-1);
		}
		int qt[64];
		int i;
		for (i = 0; i < 64; i++) {
			int v = *p++;
			if (prec)
				v = (v << 8) + *p++;
			qt[i] = v;
		}
		for (i = 0; i < 64; i++) {
			/* Compute new table only if it changed */
			int j = ROWZAG[i];
			if (qt[i] != qt_[n][j]) {
				for (; i < 64; ++i) {
					j = ROWZAG[i];
					qt_[n][j] = qt[i];
				}
				rdct_fold_q(qt_[n], fqt_[n]);
				break;
			}
		}
	}
	return (len);
}

void JpegDecoder::restart()
{
	int c;
	nbb_ = 0;
	/*XXXwhat if ff is sitting in bit buffer?*/
	/* Scan for next JPEG marker */
	do {
		do {			/* skip any non-FF bytes */
			c = *inb_++;
		} while (c != 0xFF);
		do {
			/* skip any duplicate FFs */
			/* we don't increment nbytes here since extra FFs are legal */
			c = *inb_++;
		} while (c == 0xFF);
	} while (c == 0);		/* repeat if it was a stuffed FF/00 */
#ifdef notdef
	if (nbytes != 1)
		WARNMS2(cinfo->emethods,
			"Corrupt JPEG data: %d extraneous bytes before marker 0x%02x",
			nbytes-1, c);
	
#endif
#ifdef notdef
	if (c != (RST0 + cinfo->next_restart_num)) {
		/* Uh-oh, the restart markers have been messed up too. */
		/* Let the file-format module try to figure out how to resync. */
		(*cinfo->methods->resync_to_restart) (cinfo, c);
	} else
		TRACEMS1(cinfo->emethods, 2, "RST%d", cinfo->next_restart_num);
#endif
	/* Re-initialize DC predictions to 0 */
	comp_[0].dc = 0;
	comp_[1].dc = 0;
	comp_[2].dc = 0;
#ifdef notdef
	cinfo->next_restart_num = (cinfo->next_restart_num + 1) & 7;
#endif
}

const u_char* JpegDecoder::parseJFIF(const u_char* in)
{
	int t;
	while (in < end_) {
		if (*in++ != 0xff)
			continue;
		/*XXX need more checks for buffer overflow*/
		switch (*in++) {

		default:
			/* Don't know.  Keep looking for SOS. */
			continue;
			
		case 0xdb:
			/* quantization table */
			t = rdqt(in);
			if (t < 0)
				/*XXX*/
				return (end_);
			in += t;
			continue;

		case 0xdd:
			/* restart interval definition */
			t = *in++ << 8;
			t |= *in++;
			if (t != 4) {
				/* XXX bad length */
				;
			}
			rlen_ = *in++ << 8;
			rlen_ |= *in++;
			rcnt_ = 0;
			continue;

		case 0xda:
			/* start-of-scan marker */
			if (in + 2 <= end_) {
				/* skip over SOS */
				int t = *in++ << 8;
				t |= *in++;
				in += (t - 2);
			}
			return (in);
		}
	}
	/*XXX*/
	return (end_);
}

/*XXX*/
#ifdef INT_64
#define MASK_DECL	INT_64 m0
#define MASK_VAL	m0
#define MASK_REF	&m0
inline void JpegDecoder::rdct(int nc, register short *bp, INT_64 mask,
			      u_char* pixels, int stride, int qno)
{
#ifdef notyet
	if (nc == 2) {
		for (int k = 1; k < 64; ++k) 
			if (mask & ((INT_64)1 << k)) {
				int s = bp[k] * qt_[qno][k];
				int dc = (bp[0] * qt_[qno][0] >> 3) + 128;
				/*XXX limit dc?*/
				bv_rdct1(dc, bp, k, pixels, stride);
				return;
			}
	}
#endif
	if (nc != 0)
		::rdct(bp, mask, pixels, stride, fqt_[qno]);
#ifdef notdef
	if (nc > cthresh_)
		::rdct(bp, mask, pixels, stride, fqt_[qno]);
	else if (nc > 0)
		bv_rdct(bp, mask, pixels, stride, qt_[qno]);
#endif
}
#else
#define MASK_DECL	u_int mask[2]
#define MASK_VAL	mask[0], mask[1]
#define MASK_REF	mask
inline void JpegDecoder::rdct(int nc, register short *bp, u_int m0, u_int m1,
			      u_char* pixels, int stride, int qno)
{
#ifdef notyet
	if (nc > cthresh_)
		::rdct(bp, m0, m1, pixels, stride, fqt_[qno]);
	else if (nc > 0)
		bv_rdct(bp, m0, m1, pixels, stride, qt_[qno]);
#else
	if (nc != 0)
		::rdct(bp, m0, m1, pixels, stride, fqt_[qno]);
#endif
}
#endif

/*
 * 422 Decoders
 */

int JpegDecoder_422::decode(const u_char* in, int len, u_char *marks, int mark)
{
	inb_ = in;
	end_ = in + len;
	nbb_ = 0;

	/*
	 * If first symbol is a marker (a not a stuffed ff),
	 * assume a jfif header is present and parse it.
	 * XXX this could change state that needs to be
	 * communicated back to caller.
	 */
	if (in[0] == 0xff && in[1] != 0)
		inb_ = parseJFIF(inb_);

	huffreset();

	int q0 = comp_[0].qno;
	int q1 = comp_[1].qno;
	u_char* yp = frm_;
	u_char* up = yp + osize_;
	u_char* vp = up + osize_ / 2;
	short* cache = cache_;
	short blk[64];
	margin& m = margin_;
	/* Skip top */
	yp += m.ytopskip;
	up += m.uvtopskip;
	vp += m.uvtopskip;
	marks += m.marktopskip;
	cache += m.marktopskip * NCC;
	for (int y = 0; y < nrow_; ++y) {
		int ycrop = (y < topcrop_ || y >= botcrop_);
		if (!ycrop) {
			marks += m.marklskip;
			yp += m.ylskip;
			up += m.uvlskip;
			vp += m.uvlskip;
			cache += m.marklskip * NCC;
		}
		for (int x = 0; x < ncol_; ++x) {
			if (ycrop || x < lcrop_ || x >= rcrop_) {
				(void)huffskip(comp_[0]);
				(void)huffskip(comp_[0]);	
				(void)huffskip(comp_[1]);
				(void)huffskip(comp_[2]);
				continue;
			}

			MASK_DECL;
			/*
			 * If we're handling restart markers,
			 * check if we need to resync.
			 */
			if (rlen_ != 0 && --rcnt_ <= 0) {
				rcnt_ = rlen_;
				restart();
			}

			int nc = huffparse(comp_[0], blk, cache, MASK_REF);
			int dontskip = nc;
			cache += NCC;
			rdct(nc, blk, MASK_VAL, yp, owidth_, q0);
			nc = huffparse(comp_[0], blk, cache, MASK_REF, 
				       dontskip);
			dontskip |= nc;
			cache += NCC;
			rdct(nc, blk, MASK_VAL, yp + 8, owidth_, q0);
			if (color_ && dontskip) {
				/*
				 * If we found above that the luminance
				 * planes exceeded the threhold, decode
				 * the choma planes unconditionally.
				 * Otherwise, see if they can be
				 * suppressed too.
				 */
				short dummy[6];
				nc = huffparse(comp_[1], blk, dummy, MASK_REF,
					       1);
				rdct(nc, blk, MASK_VAL, up, owidth_ / 2, q1);
				nc = huffparse(comp_[2], blk, dummy, MASK_REF,
					       1);
				rdct(nc, blk, MASK_VAL, vp, owidth_ / 2, q1);
			} else {
				(void)huffskip(comp_[1]);
				(void)huffskip(comp_[2]);
			}

			if (dontskip) {
				marks[0] = mark;
				marks[1] = mark;
				ndblk_ += 2;
			}
			marks += 2;
			yp += 16;
			up += 8;
			vp += 8;
			
		}
		if (!ycrop) {
			marks += m.markrskip;
			yp += m.yrskip;
			up += m.uvrskip;
			vp += m.uvrskip;
			cache += m.markrskip * NCC;
			/*
			 * We're at the end of the current line.
			 * Back up to the beggining, then skip down
			 * one row to the start of the next mcu.
			 */
			yp -= owidth_;
			up -= owidth_ / 2;
			vp -= owidth_ / 2;
			
			yp += 8 * owidth_;
			up += 8 * owidth_ / 2;
			vp += 8 * owidth_ / 2;
		}
	}
	return (0);
}


int JpegDCTDecoder_422::decode(const u_char* in, int len, u_char *marks, 
			       int mark)
{

	inb_ = in;
	end_ = in + len;
	nbb_ = 0;

	/*
	 * If first symbol is a marker (a not a stuffed ff),
	 * assume a jfif header is present and parse it.
	 * XXX this could change state that needs to be
	 * communicated back to caller.
	 */
	if (in[0] == 0xff && in[1] != 0)
		inb_ = parseJFIF(inb_);

	huffreset();

	short *fp = frm_;
	short *lastoff = fp;
	u_char *lastmark = marks;
	int dontskip;

	margin& m = margin_;

	/* Skip top */
	fp += m.ftopskip;
	marks += m.marktopskip;

	for (int y = 0; y < nrow_; ++y) {

		int ycrop = (y < topcrop_ || y >= botcrop_);

		if (EVEN(y)) {
			/* process an even-numbered line */
			/* offset accounting takes place here */
			if (!ycrop) {
				marks += m.marklskip;
				fp += m.flskip;
			}
			lastmark = marks;
			lastoff = fp;
			for (int x = 0; x < ncol_; ++x) {
				/* processing for one 4.2.2 mcu */
				if (ycrop || x < lcrop_ || x >= rcrop_) {
					/* Y Y U V */
					(void)huffskip(comp_[0]);
					(void)huffskip(comp_[0]);	
					(void)huffskip(comp_[1]);
					(void)huffskip(comp_[2]);
					continue;
				}

				MASK_DECL;
				/*
				 * If we're handling restart markers,
				 * check if we need to resync.
				 */
				if (rlen_ != 0 && --rcnt_ <= 0) {
					rcnt_ = rlen_;
					restart();
				}

				/* Y's for even lines */
				int nc = huffparse(comp_[0], fp,
					MASK_REF, qt_[0]);
				dontskip = nc;
				nc = huffparse(comp_[0], fp + 64,
					MASK_REF, qt_[0], dontskip);
				dontskip |= nc;

				/*
				 * always decode these, as they might
				 * be needed later for dct_decimate
				 */
				if (color_) {
					huffparse(comp_[1], fp + 256,
						MASK_REF, qt_[1], 1);
					huffparse(comp_[2], fp + 320,
						MASK_REF, qt_[1], 1);
				} else {
					(void)huffskip(comp_[1]);
					(void)huffskip(comp_[2]);
				}

				fp += BMB * 64;
				if (dontskip) {
					marks[0] = mark;
					ndblk_++;
				}
				marks++;
			}
			if (!ycrop) {
				marks += m.markrskip;
				fp += m.frskip;
			}
		} else {
			for (int x = 0; x < ncol_; ++x) {
				/* processing for one column */
				if (ycrop || x < lcrop_ || x >= rcrop_) {
					/* Y Y U V */
					(void)huffskip(comp_[0]);
					(void)huffskip(comp_[0]);	
					(void)huffskip(comp_[1]);
					(void)huffskip(comp_[2]);
					continue;
				}

				MASK_DECL;
				/*
				 * If we're handling restart markers,
				 * check if we need to resync.
				 */
				if (rlen_ != 0 && --rcnt_ <= 0) {
					rcnt_ = rlen_;
					restart();
				}

				/* Y's for odd lines */
				int nc = huffparse(comp_[0], lastoff + 128,
					MASK_REF, qt_[0]);
				dontskip = nc;
				nc = huffparse(comp_[0], lastoff + 192,
					MASK_REF, qt_[0], dontskip);
				dontskip |= nc;

				if (color_ && (dontskip || lastmark[0] == mark)) {
					/* UV's */
					short uvbuf[2 * 64];
					huffparse(comp_[1], uvbuf,
						MASK_REF, qt_[1], 1);
					huffparse(comp_[2], uvbuf + 64,
						MASK_REF, qt_[1], 1);
					dct_decimate(lastoff + 256,
						uvbuf, lastoff + 256);
					dct_decimate(lastoff + 320,
						uvbuf + 64, lastoff + 320);
				} else {
					(void)huffskip(comp_[1]);
					(void)huffskip(comp_[2]);
				}

				lastoff += BMB * 64;
				if (dontskip) {
					lastmark[0] = mark;
					ndblk_++;
				}
				lastmark++;
			}

		}
	}
	return (0);
}

/*
 * 420 Decoders
 */

int JpegDecoder_420::decode(const u_char* in, int len, u_char *marks, int mark)
{
	inb_ = in;
	end_ = in + len;
	nbb_ = 0;

	/*
	 * If first symbol is a marker (a not a stuffed ff),
	 * assume a jfif header is present and parse it.
	 * XXX this could change state that needs to be
	 * communicated back to caller.
	 */
	if (in[0] == 0xff && in[1] != 0)
		inb_ = parseJFIF(inb_);

	huffreset();
	int q0 = comp_[0].qno;
	int q1 = comp_[1].qno;
	u_char* yp = frm_;
	u_char* up = yp + osize_;
	u_char* vp = up + osize_ / 4;
	short* cache = cache_;
	short blk[64];

	/* Skip top */
	margin& m = margin_;
	yp += m.ytopskip;
	up += m.uvtopskip;
	vp += m.uvtopskip;
	marks += m.marktopskip;
	cache += m.marktopskip * NCC;

	for (int y = 0; y < nrow_; ++y) {
		int ycrop = (y < topcrop_ || y >= botcrop_);
		if (!ycrop) {
			marks += m.marklskip;
			yp += m.ylskip;
			up += m.uvlskip;
			vp += m.uvlskip;
			cache += m.marklskip * 2 * NCC;
		}
		for (int x = 0; x < ncol_; ++x) {
			if (ycrop || x < lcrop_ || x >= rcrop_) {	
				(void)huffskip(comp_[0]);
				(void)huffskip(comp_[0]);	
				(void)huffskip(comp_[0]);
				(void)huffskip(comp_[0]);	
				(void)huffskip(comp_[1]);
				(void)huffskip(comp_[2]);
				continue;
			}
			MASK_DECL;
			/*
			 * If we're handling restart markers,
			 * check if we need to resync.
			 */
			if (rlen_ != 0 && --rcnt_ <= 0) {
				rcnt_ = rlen_;
				restart();
			}
			int nc = huffparse(comp_[0], blk, cache, MASK_REF);
			cache += NCC;
			int dontskip = nc;
			rdct(nc, blk, MASK_VAL, yp, owidth_, q0);
			nc = huffparse(comp_[0], blk, cache, MASK_REF,
				       dontskip);
			cache += NCC;
			dontskip |= nc;
			rdct(nc, blk, MASK_VAL, yp + 8, owidth_, q0);
			nc = huffparse(comp_[0], blk, cache, MASK_REF,
				       dontskip);
			cache += NCC;
			dontskip |= nc;
			rdct(nc, blk, MASK_VAL, yp + 8 * owidth_, owidth_, q0);
			nc = huffparse(comp_[0], blk, cache, MASK_REF,
				       dontskip);
			cache += NCC;
			dontskip |= nc;
			rdct(nc, blk, MASK_VAL, yp + 8 * owidth_ + 8, owidth_, 
			     q0);
			if (color_ && dontskip) {
				/*
				 * If we found above that the luminance
				 * planes exceeded the threhold, decode
				 * the choma planes unconditionally.
				 * Otherwise, see if they can be
				 * suppressed too.
				 */
				short dummy[6];
				nc = huffparse(comp_[1], blk, dummy, MASK_REF,
					       1);
				rdct(nc, blk, MASK_VAL, up, owidth_ / 2, q1);
				nc = huffparse(comp_[2], blk, dummy, MASK_REF,
					       1);
				rdct(nc, blk, MASK_VAL, vp, owidth_ / 2, q1);
			} else {
				(void)huffskip(comp_[1]);
				(void)huffskip(comp_[2]);
			}
			if (dontskip) {
				marks[0] = mark;
				marks[1] = mark;
				int off = owidth_ >> 3;
				marks[off] = mark;
				marks[off + 1] = mark;
				ndblk_ += 4;
			}
			marks += 2;
			yp += 16;
			up += 8;
			vp += 8;
		}
		if (!ycrop) {
			/* Skip over entire macroblock */
			marks += m.markrskip;
			yp += m.yrskip;
			up += m.uvrskip;
			vp += m.uvrskip;
			cache += m.markrskip * 2 * NCC;

			marks += owidth_ >> 3;
			yp -= owidth_;
			up -= owidth_ / 2;
			vp -= owidth_ / 2;

			yp += 2 * 8 * owidth_;
			up += 8 * owidth_ / 2;
			vp += 8 * owidth_ / 2;
		}
	}
	return (0);
}


//
// decode only to DCT not pixels (i.e. don't to rdct step)
// output is stored interleaved 4 Y's followed by 1 U and 1 V block
//
int JpegDCTDecoder_420::decode(const u_char* in, int len, u_char *marks, int mark)
{
	inb_ = in;
	end_ = in + len;
	nbb_ = 0;

	/*
	 * If first symbol is a marker (a not a stuffed ff),
	 * assume a jfif header is present and parse it.
	 * XXX this could change state that needs to be
	 * communicated back to caller.
	 */
	if (in[0] == 0xff && in[1] != 0)
		inb_ = parseJFIF(inb_);

	huffreset();

	short* fp = frm_;
	
	margin& m = margin_;

	/* Skip top */
	fp += m.ftopskip;
	marks += m.marktopskip;
	
	for (int y = 0; y < nrow_; ++y) {
		int ycrop = (y < topcrop_ || y >= botcrop_);
		if (!ycrop) {
			marks += m.marklskip;
			fp += m.flskip;
		}
		for (int x = 0; x < ncol_; ++x) {
			if (ycrop || x < lcrop_ || x >= rcrop_) {	
				(void)huffskip(comp_[0]);
				(void)huffskip(comp_[0]);	
				(void)huffskip(comp_[0]);
				(void)huffskip(comp_[0]);	
				(void)huffskip(comp_[1]);
				(void)huffskip(comp_[2]);
				continue;
			}
			MASK_DECL;
			/*
			 * If we're handling restart markers,
			 * check if we need to resync.
			 */
			if (rlen_ != 0 && --rcnt_ <= 0) {
				rcnt_ = rlen_;
				restart();
			}

			/*
			 * decode the 4 Y blocks
			 */
			int nc = huffparse(comp_[0], fp, MASK_REF, qt_[0]);
			int dontskip = nc;
			short* t = fp + 64;
			nc = huffparse(comp_[0], t, MASK_REF, qt_[0], dontskip);
			dontskip |= nc;
			t = fp + 128;
			nc = huffparse(comp_[0], t, MASK_REF, qt_[0], dontskip);
			dontskip |= nc;
			t = fp + 192;
			nc = huffparse(comp_[0], t, MASK_REF, qt_[0], 
				       dontskip);
			dontskip |= nc;
			if (color_ && dontskip) {
				/* decode U and V blocks */
				t = fp + 256;
				huffparse(comp_[1], t, MASK_REF, qt_[1], 1);
				t = fp + 320;
				huffparse(comp_[2], t, MASK_REF, qt_[1], 1);
			} else {
				/* skip U and V blocks */
				(void)huffskip(comp_[1]);
				(void)huffskip(comp_[2]);
			}
			if (dontskip) {
				marks[0] = mark;	// one MB decoded
				ndblk_++;
			}

			fp += BMB * 64;
			marks++;
		}
		if (!ycrop) {
			/* Skip over entire macroblock */
			marks += m.markrskip;
			fp += m.frskip;
		}
	}
	return (0);
}


/* Figure F.12: extend sign bit */

#ifdef notdef
#define huff_EXTEND(x,s)  ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

static const int extend_test[16] =   /* entry n is 2**(n-1) */
  { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };

static const int extend_offset[16] = /* entry n is (-1 << n) + 1 */
  { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };
#else
/* is this really faster? */
inline int
huff_EXTEND(int x, int s)
{
	register int b = x >> (s - 1);
	register int m = ((b & 1) - 1) << s;
	return ((x | m) + (~b & 1));
}
#endif

/*
 * Read the next 16 bits off the bit string into the bit buffer.
 * Skip over zero-stuffed ff's but make no attempt to verify
 * that they aren't some other marker (which shouldn't be in the
 * middle of a block anyway).
 */
#define HUFFRQ(bb) \
 { \
	register int v; \
	register const u_char *cp = inb_; \
 \
	bb <<= 16; \
	v = *cp++; \
	if (v == 0xff) ++cp; \
	bb |= v << 8; \
	v = *cp++; \
	if (v == 0xff) ++cp; \
	bb |= v; \
	inb_ = cp; \
 \
}

#define MASK(s) ((1 << (s)) - 1)

#define HUFF_DECODE(ht, nbb, bb, result) { \
	register int s_, v_; \
 \
	if (nbb < 16) { \
		HUFFRQ(bb); \
		nbb += 16; \
	} \
	v_ = (bb >> (nbb - 16)) & 0xffff; \
	s_ = (ht)[v_]; \
	nbb -= (s_ >> 8); \
	result = s_ & 0xff; \
 }

#define GET_BITS(n, nbb, bb, result) \
{ \
	nbb -= n; \
	if (nbb < 0)  { \
		HUFFRQ(bb); \
		nbb += 16; \
	} \
	(result) = ((bb >> nbb) & MASK(n)); \
}

#define SKIP_BITS(n, nbb, bb) \
{ \
	nbb -= n; \
	if (nbb < 0)  { \
		HUFFRQ(bb); \
		nbb += 16; \
	} \
}

int JpegDecoder::huffdc(component& p)
{
	/* Decode a single block's worth of coefficients */
		
	/* Section F.2.2.1: decode the DC coefficient difference */
	register int bb = bb_;
	register int nbb = nbb_;
	u_short* ht = dcht_[p.dc_tbl_no];
	register int s, r;
	HUFF_DECODE(ht, nbb, bb, s);
	if (s != 0) {
		GET_BITS(s, nbb, bb, r);
		s = huff_EXTEND(r, s);
	}
	/* Convert DC difference to actual value, update predictor */
	s += p.dc;
	p.dc = s;
	
	/* Section F.2.2.2: decode the AC coefficients */
	ht = acht_[p.ac_tbl_no];
	for (register int k = 1; k < 64; ) {
		/* Symbol-1 */
		register int v;
		HUFF_DECODE(ht, nbb, bb, v);
		s = v & 15;
		r = v >> 4;
		if (s != 0) {
			k += r;
			/* Symbol-2 */
			SKIP_BITS(s, nbb, bb);
			++k;
		} else {
			if (r != 15)
				/* end of block */
				break;
			k += 16;
		}
	}
	nbb_ = nbb;
	bb_ = bb;

	return (0);
}

/*
 * Parse a huffman-encoded 8x8 block.  Blocks are independent
 * of eachother, except for the dc predictor, and blocks can
 * start and end on arbitrary bit boundaries.  No markers should
 * appear in the bit stream, and ff bytes should be zero stuffed
 * (i.e., replaced with ff 00).
 *
 * The block is coded as a sequence of pairs of symbols.  Where the
 * first symbol is a huffman-encoded value <r,n> where r is a four-bit
 * runlength and n is the length of the second symbol (of the pair),
 * which follows verbatim in the bit string.
 */
#ifdef INT_64
int JpegDCTDecoder::huffparse(component& p, short* out,
			   INT_64* mask, const int *quant_table, int dontskip)
#else
int JpegDCTDecoder::huffparse(component& p, short* out,
			   u_int* mask, const int *quant_table, int dontskip)
#endif
{
	register int bb = bb_;
	register int nbb = nbb_;
	u_short* ht = dcht_[p.dc_tbl_no];
	register int s, r;
	HUFF_DECODE(ht, nbb, bb, s);
	if (s != 0) {
		GET_BITS(s, nbb, bb, r);
		s = huff_EXTEND(r, s);
		/* update predictor */
		s += p.dc;
		p.dc = s;
	} else
		s = p.dc;

	short in[6];
	in[0] = s * quant_table[0];	// get  & de-quantize DC coef
	if (dct_unbias_)
		in[0] += 128 * 8;
	in[1] = 0;
	in[2] = 0;
	in[3] = 0;
	in[4] = 0;
	in[5] = 0;

	/*
	 * First, grab only a few low frequency coefficients.
	 * If they aren't sufficiently different from the current
	 * block, skip over this block quickly.
	 */
	ht = acht_[p.ac_tbl_no];
	register int k = 1;

#ifdef INT_64
	INT_64 m0 = 1;
#else
	u_int m1 = 0;
	u_int m0 = 1;
#endif
	for (;;) {
		register int v;
		/* Symbol-1 */
		HUFF_DECODE(ht, nbb, bb, v);
		s = v & 15;
		r = v >> 4;
		if (s != 0) {
			k += r;
			if (k >= 6) {
				k -= r;
				break;
			}
			/* Symbol-2 */
			GET_BITS(s, nbb, bb, v);
			s = huff_EXTEND(v, s);
			in[k] = s * quant_table[ROWZAG[k]]; // store AC coef
			v = COLZAG[k];
			m0 |= 1 << v;

			++k;
		} else
			break;
	}

	/*
	 * skip past a block [in] if it sufficiently different
	 * from a reference block [out]
	 */

	if (!dontskip && zag_blkdiff(in, out) < thresh_) {
		/* skip this block */
		for (;;) {
			if (s != 0) {
				k += r;
				/* Symbol-2 */
				SKIP_BITS(s, nbb, bb);
				++k;
			} else {
				if (r != 15)
				/* end of block */
					break;
				k += 16;
			}
			if (k >= 64)
				break;

			/* Symbol-1 */
			register int v;
			HUFF_DECODE(ht, nbb, bb, v);
			s = v & 15;
			r = v >> 4;
		}
		nbb_ = nbb;
		bb_ = bb;
		return (0);
	}

	/* 
	 * Need to do this since there may be holes in the loop below and
	 * coefficients won't get updated.
	 */
	memset((char*)out, 0, sizeof(*out) * 64);

	/* store first NCC coefs */
	out[0] = in[0];
	out[8] = in[1];
	out[1] = in[2];
	out[2] = in[3];
	out[9] = in[4];
	out[16] = in[5];

	for (;;) {
		register int v;
		if (s != 0) {
			k += r;
			/* Symbol-2 */
			GET_BITS(s, nbb, bb, v);
			s = huff_EXTEND(v, s);
			v = COLZAG[k];
			
			out[v] = s * quant_table[ROWZAG[k]];
#ifdef INT_64
			m0 |= (INT_64)1 << v;
#else
			if (v < 32)
				m0 |= 1 << v;
			else
				m1 |= 1 << (v - 32);
#endif
			++k;
		} else {
			if (r != 15)
				/* end of block */
				break;

			k += 16;
		}
		if (k >= 64)
			break;

		/* Symbol-1 */
		HUFF_DECODE(ht, nbb, bb, v);
		s = v & 15;
		r = v >> 4;
	}

#ifdef INT_64
	*mask = m0;
#else
	mask[0] = m0;
	mask[1] = m1;
#endif
	nbb_ = nbb;
	bb_ = bb;

	return (1);
}

#ifdef INT_64
int JpegPixelDecoder::huffparse(component& p, short* out, short* ref,
			   INT_64* mask, int dontskip)
#else
int JpegPixelDecoder::huffparse(component& p, short* out, short* ref,
			   u_int* mask, int dontskip)
#endif
{
	register int bb = bb_;
	register int nbb = nbb_;
	u_short* ht = dcht_[p.dc_tbl_no];
	register int s, r;
	HUFF_DECODE(ht, nbb, bb, s);
	if (s != 0) {
		GET_BITS(s, nbb, bb, r);
		s = huff_EXTEND(r, s);
		/* update predictor */
		s += p.dc;
		p.dc = s;
	} else
		s = p.dc;

	short in[6];
	in[0] = s;
	in[1] = 0;
	in[2] = 0;
	in[3] = 0;
	in[4] = 0;
	in[5] = 0;

	/*
	 * First, grab only a few low frequency coefficients.
	 * If they aren't sufficiently different from the current
	 * block, skip over this block quickly.
	 */
	ht = acht_[p.ac_tbl_no];
	register int k = 1;

#ifdef INT_64
	INT_64 m0 = 1;
#else
	u_int m1 = 0;
	u_int m0 = 1;
#endif
	for (;;) {
		register int v;
		/* Symbol-1 */
		HUFF_DECODE(ht, nbb, bb, v);
		s = v & 15;
		r = v >> 4;
		if (s != 0) {
			k += r;
			if (k >= 6) {
				k -= r;
				break;
			}
			/* Symbol-2 */
			GET_BITS(s, nbb, bb, v);
			s = huff_EXTEND(v, s);
			in[k] = s;
			v = COLZAG[k];
			m0 |= 1 << v;

			++k;
		} else
			break;
	}

	if (!dontskip && inorder_blkdiff(in, ref) < thresh_) {
		/* skip this block */
		for (;;) {
			if (s != 0) {
				k += r;
				/* Symbol-2 */
				SKIP_BITS(s, nbb, bb);
				++k;
			} else {
				if (r != 15)
				/* end of block */
					break;
				k += 16;
			}
			if (k >= 64)
				break;

			/* Symbol-1 */
			register int v;
			HUFF_DECODE(ht, nbb, bb, v);
			s = v & 15;
			r = v >> 4;
		}
		nbb_ = nbb;
		bb_ = bb;
		return (0);
	}

	/* 
	 * Need to do this since there may be holes in the loop below and
	 * coefficients won't get updated.
	 */
	memset((char*)out, 0, 2 * 64);

	ref[0] = out[0] = in[0];
	ref[1] = out[8] = in[1];
	ref[2] = out[1] = in[2];
	ref[3] = out[2] = in[3];
	ref[4] = out[9] = in[4];
	ref[5] = out[16] = in[5];

	for (;;) {
		register int v;
		if (s != 0) {
			k += r;
			/* Symbol-2 */
			GET_BITS(s, nbb, bb, v);
			s = huff_EXTEND(v, s);
			v = COLZAG[k];
			
			out[v] = s;
#ifdef INT_64
			m0 |= (INT_64)1 << v;
#else
			if (v < 32)
				m0 |= 1 << v;
			else
				m1 |= 1 << (v - 32);
#endif
			++k;
		} else {
			if (r != 15)
				/* end of block */
				break;

			k += 16;
		}
		if (k >= 64)
			break;

		/* Symbol-1 */
		HUFF_DECODE(ht, nbb, bb, v);
		s = v & 15;
		r = v >> 4;
	}

#ifdef INT_64
	*mask = m0;
#else
	mask[0] = m0;
	mask[1] = m1;
#endif
	nbb_ = nbb;
	bb_ = bb;

	return (1);
}

/*
 * Skip over a block.
 */
int JpegDecoder::huffskip(component& p)
{
	register int bb = bb_;
	register int nbb = nbb_;
	u_short* ht = dcht_[p.dc_tbl_no];
	register int s;
	HUFF_DECODE(ht, nbb, bb, s);
	if (s != 0) {
		int r;
		GET_BITS(s, nbb, bb, r);
		s = huff_EXTEND(r, s);
		/* update predictor */
		s += p.dc;
		p.dc = s;
	}
	ht = acht_[p.ac_tbl_no];
	for (register int k = 1; k < 64; ) {
		/* Symbol-1 */
		register int v;
		HUFF_DECODE(ht, nbb, bb, v);
		s = v & 15;
		register int r = v >> 4;
		if (s != 0) {
			k += r;
			/* Symbol-2 */
			SKIP_BITS(s, nbb, bb);
			++k;
		} else {
			if (r != 15)
				/* end of block */
				break;
			k += 16;
		}
	}
	nbb_ = nbb;
	bb_ = bb;

	return (0);
}

void JpegDecoder::huffreset()
{
	nbb_ = 0;
	comp_[0].dc = 0;
	comp_[1].dc = 0;
	comp_[2].dc = 0;
}

/*
 * Build a 64k lookup table from the jpeg huffman table described
 * by the arguments.  The table is indexed by the next 16-bits
 * of the input stream.  The entry in the table tells us the
 * length of the next code and its decoded value.
 */
u_short* JpegDecoder::huffbuild(const u_char* bits, const u_char* vals) const
{
	/* Figure C.1: make table of Huffman code length for each symbol */
	/* Note that this is in code-length order. */

	int nsym = 0;
	int huffsize[257];
	for (int codelen = 1; codelen <= 16; ++codelen) {
		for (int i = 1; i <= bits[codelen]; ++i)
			/*
			 * XXX should sanity check that nsym stays
			 * below 256.
			 */
			huffsize[nsym++] = codelen;
	}
	huffsize[nsym] = 0;

	/* Figure C.2: generate the codes themselves */
	/* Note that this is in code-length order. */
  
	int code = 0;
	int si = huffsize[0];
	u_short huffcode[256];
	int p = 0;
	while (p < nsym) {
		while (huffsize[p] == si)
			huffcode[p++] = code++;

		code <<= 1;
		++si;
	}
	/*
	 * Build the direct-map lookup table.
	 */
	u_short *ht = new u_short[65536];
	memset((char*)ht, 0, 65536 * sizeof(u_short));
	for (int sym = 0; sym < nsym; ++sym) {
		int codelen = huffsize[sym];
		int nbit = 16 - codelen;
		int code = huffcode[sym] << nbit;
		int map = (codelen << 8) | vals[sym];
		/*
		 * The low nbit bits are don't cares.
		 * Spin through all possible combos.
		 */
		for (int n = 1 << nbit; --n >= 0; )
			ht[code | n] = map;
	}
	return (ht);
}
