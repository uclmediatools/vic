/*
 * Copyright (c) 1995 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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

#include "rgb-converter.h"
#include "bsd-endian.h"

RGB_Converter_411 RGB_Converter_411::instance_;
RGB_Converter_422 RGB_Converter_422::instance_;

u_int32_t RGB_Converter::r2yuv_[256];
u_int32_t RGB_Converter::g2yuv_[256];
u_int32_t RGB_Converter::b2yuv_[256];

RGB_Converter::RGB_Converter()
{
	for (int rgb = 0; rgb < 256; ++rgb) {
		/* can't have overflow in this direction */
		int y = int(0.299 * rgb);
		int u = int(-0.1687 * rgb) & 0xff;
		int v = int(0.5 * rgb);
		r2yuv_[rgb] = y | u << 10 | v << 20;

		y = int(0.587 * rgb);
		u = int(-0.3313 * rgb) & 0xff;
		v = int(-0.4187 * rgb) & 0xff;
		g2yuv_[rgb] = y | u << 10 | v << 20;

		y = int(0.114 * rgb);
		u = int(0.5 * rgb);
		v = int(- 0.0813 * rgb) & 0xff;
		b2yuv_[rgb] = y | u << 10 | v << 20;
	}
}

void RGB_Converter_422::convert(u_int8_t* p, int w, int h, u_int8_t* frm)
{
	u_int8_t* yp = (u_int8_t*)frm;
	int off = w * h;
	u_int8_t* up = (u_int8_t*)(frm + off);
	off += off >> 1;
	u_int8_t* vp = (u_int8_t*)(frm + off);
 
	u_int32_t* R = r2yuv_;
	u_int32_t* G = g2yuv_;
	u_int32_t* B = b2yuv_;

/*XXX*/
#if BYTE_ORDER == BIG_ENDIAN
#define ROFF 3
#define GOFF 2
#define BOFF 1
#else
#define ROFF 0
#define GOFF 1
#define BOFF 2
#endif
	while (--h >= 0) {
		for (int x = 0; x < w; x += 2) {
			/*
			 * We use the linearity of the colorspace conversion
			 * to use a compact table for each of the R, G, and
			 * B color components.  Note that we do not have
			 * to fix up overflow because the transform does
			 * not escape the YUV cube in the RGB->YUV direction.
			 * Moreover, we do not need to worry about overflow
			 * between the y,u, and v components affecting
			 * eachother (in the parallel add) because there
			 * are two empty bits between each component
			 * so we can survice two overflows.
			 *
			 * XXX assume pixels are formatted as follows:
			 * 	. B0 G0 R0 . B1 G1 R1 ...
			 * We should look at the RGB masks in the X image
			 * and choose an appropriate routine.
			 */
			u_int32_t yuv = R[p[ROFF]];
			yuv += G[p[GOFF]];
			yuv += B[p[BOFF]];
			p += 4;

			/*
			 * Flip the high bit on the chrominance because
			 * the encoder expects them in the range 0-255.
			 */
			*yp++ = yuv;
			*up++ = (yuv >> 10) ^ 0x80;
			*vp++ = (yuv >> 20) ^ 0x80;

			yuv = R[p[ROFF]];
			yuv += G[p[GOFF]];
			yuv += B[p[BOFF]];
			p += 4;

			*yp++ = yuv;
		}
	}
}

void RGB_Converter_411::convert(u_int8_t* p, int w, int h, u_int8_t* frm)
{
	u_int8_t* yp = (u_int8_t*)frm;
	int off = w * h;
	u_int8_t* up = (u_int8_t*)(frm + off);
	off += off >> 2;
	u_int8_t* vp = (u_int8_t*)(frm + off);
 
	u_int32_t* R = r2yuv_;
	u_int32_t* G = g2yuv_;
	u_int32_t* B = b2yuv_;

	int stride = w << 2;
	for (h >>= 1; --h >= 0; ) {
		for (int x = 0; x < w; x += 2) {
			/*
			 * We use the linearity of the colorspace conversion
			 * to use a compact table for each of the R, G, and
			 * B color components.  Note that we do not have
			 * to fix up overflow because the transform does
			 * not escape the YUV cube in the RGB->YUV direction.
			 * Moreover, we do not need to worry about overflow
			 * between the y,u, and v components affecting
			 * eachother (in the parallel add) because there
			 * are two empty bits between each component
			 * so we can survice two overflows.
			 *
			 * XXX assume pixels are formatted as follows:
			 * 	. B0 G0 R0 . B1 G1 R1 ...
			 * We should look at the RGB masks in the X image
			 * and choose an appropriate routine.
			 */
			u_int32_t yuv = R[p[ROFF]];
			yuv += G[p[GOFF]];
			yuv += B[p[BOFF]];

			/*
			 * Flip the high bit on the chrominance because
			 * the encoder expects them in the range 0-255.
			 */
			yp[0] = yuv;
			*up++ = (yuv >> 10) ^ 0x80;
			*vp++ = (yuv >> 20) ^ 0x80;

			yuv = R[p[stride + ROFF]];
			yuv += G[p[stride + GOFF]];
			yuv += B[p[stride + BOFF]];
			yp[w] = yuv;
			p += 4;
			++yp;

			yuv = R[p[ROFF]];
			yuv += G[p[GOFF]];
			yuv += B[p[BOFF]];
			yp[0] = yuv;

			yuv = R[p[stride + ROFF]];
			yuv += G[p[stride + GOFF]];
			yuv += B[p[stride + BOFF]];
			yp[w] = yuv;
			p += 4;
			++yp;
		}
		yp += w;
		p += stride;
	}
}
