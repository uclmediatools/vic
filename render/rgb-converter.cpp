/*
 * Copyright (c) 1996 Isidor Kouvelas (University College London)
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

#ifndef WIN32
#define min(a,b) (a<b?a:b)
#endif

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include "stdio.h"
#include "rgb-converter.h"
#include "bsd-endian.h"

u_int32_t RGB_Converter::r2yuv_[256];
u_int32_t RGB_Converter::g2yuv_[256];
u_int32_t RGB_Converter::b2yuv_[256];

RGB_Converter_422 RGB_Converter_422::instance_;
RGB_Converter_420 RGB_Converter_420::instance_;

RGB_Converter::RGB_Converter(int bpp, u_int8_t *map, int ncol) :
	bpp_(bpp), ncol_(0), ymap_(0), umap_(0), vmap_(0), method_(0)
{
	static int init = 0;
	if (init == 0) {
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
		init = 1;
	}

	switch (bpp) {
	case 32 :
		method_ = &RGB_Converter::convert32;
		break;
	case 24:
		method_ = &RGB_Converter::convert24;
		break;
	case 16:
		method_ = &RGB_Converter::convert16;
		ncol_ = 65536;
		break;
	case 8:
		method_ = &RGB_Converter::convert8;
		ncol_ = 256;
		break;
	case 4:
		method_ = &RGB_Converter::convert4;
		ncol_ = 16;
		break;
	case 1:
		method_ = &RGB_Converter::convert1;
		ncol_ = 2;
		break;
	default:
		fprintf(stderr, "RGB_Converter: Unsuported bpp!\n");
		abort();
	}

	if (ncol_ > 0) {
		if (ncol > ncol_) {
			fprintf(stderr, "RGB_Converter too many colors %d > %d\n", ncol, ncol_);
			ncol = ncol_;
		}

		u_int32_t	yuv;
		u_int8_t	*yp = ymap_ = new u_int8_t[ncol_];
		u_int8_t	*up = umap_ = new u_int8_t[ncol_];
		u_int8_t	*vp = vmap_ = new u_int8_t[ncol_];
	
		for (int i = 0; i < ncol; i++) {
			yuv = b2yuv_[*map++];
			yuv += g2yuv_[*map++];
			yuv += r2yuv_[*map++];
			map++;

			*yp++ = yuv;
			*up++ = (yuv >> 10) ^ 0x80;
			*vp++ = (yuv >> 20) ^ 0x80;
		}
	}
}

RGB_Converter::~RGB_Converter()
{
	if (ncol_ > 0) {
		delete [] ymap_;
		delete [] umap_;
		delete [] vmap_;
	}
}

void
RGB_Converter::convert(u_int8_t* p, int w, int h, u_int8_t* frm, int outw, int outh, int invert)
{
	if (outw == 0)
		outw = w;
	if (outh == 0)
		outh = h;
	(this->*method_)(p, w, h, frm, outw, outh, invert);
}


#define INIT422 \
	if (outw == 0) \
		outw = inw; \
	if (outh == 0) \
		outh = inh; \
\
	u_int8_t* yp = (u_int8_t*)frm; \
	int off = outw * outh; \
	u_int8_t* up = (u_int8_t*)(frm + off); \
	off += off >> 1; \
	u_int8_t* vp = (u_int8_t*)(frm + off); \
\
	u_int32_t* R = r2yuv_; \
	u_int32_t* G = g2yuv_; \
	u_int32_t* B = b2yuv_; \
\
	int h = min(inh, outh); \
	int w = min(inw, outw); \
	int inpad = (inw - w) * bpp_ / 8; \
	int outpad = outw - w; \
\
	int stride = inw * bpp_ / 8; \
	if (invert) \
		p += stride * (inh - 1 - (inh - h) / 2); \
	else \
		p += stride * ((inh - h) / 2); \
\
	int outvpad = (outh - h) / 2; \
	yp += outw * outvpad; \
	up += outw / 2 * outvpad; \
	vp += outw / 2 * outvpad; \
\
	p += inpad / 2; \
\
	yp += outpad >> 1; \
	up += outpad >> 2; \
	vp += outpad >> 2;

#define ADVANCE422 \
	p += inpad; \
	yp += outpad; \
	up += outpad >> 1; \
	vp += outpad >> 1; \
	if (invert) \
		p -= stride << 1;

#define INIT420 \
	if (outw == 0) \
		outw = inw; \
	if (outh == 0) \
		outh = inh; \
\
	u_int8_t* yp = (u_int8_t*)frm; \
	int off = outw * outh; \
	u_int8_t* up = (u_int8_t*)(frm + off); \
	off += off >> 2; \
	u_int8_t* vp = (u_int8_t*)(frm + off); \
\
	u_int32_t* R = r2yuv_; \
	u_int32_t* G = g2yuv_; \
	u_int32_t* B = b2yuv_; \
\
	int h = min(inh, outh); \
	int w = min(inw, outw); \
	int inpad = (inw - w) * bpp_ / 8; \
	int outpad = outw - w; \
\
	int stride = inw * bpp_ / 8; \
	int next_line = stride; \
	if (invert) { \
		p += stride * (inh - 1 - (inh - h) / 2); \
		next_line = -next_line; \
	} else \
		p += stride * ((inh - h) / 2); \
\
	/* Hack to get NTSC image to sit at right position within CIF */ \
	int outvpad = ((outh - h) / 2) & (outw > 176? ~0xf: ~0x3); \
	yp += outw * outvpad; \
	up += outw / 4 * outvpad; \
	vp += outw / 4 * outvpad; \
\
	p += inpad / 2; \
\
	yp += outpad >> 1; \
	up += outpad >> 2; \
	vp += outpad >> 2;

#define ADVANCE420 \
		p += inpad; \
		yp += outw + outpad; \
		up += outpad >> 1; \
		vp += outpad >> 1; \
		if (invert) \
			p -= 3 * stride; \
		else \
			p += stride;

/*XXX*/
#if BYTE_ORDER == BIG_ENDIAN
#define ROFF 3
#define GOFF 2
#define BOFF 1
#else
#define ROFF 2
#define GOFF 1
#define BOFF 0
#endif

void
RGB_Converter_422::convert32(u_int8_t* p, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)
{
	INIT422
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
		ADVANCE422
	}
}

void RGB_Converter_420::convert32(u_int8_t* p, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)
{
	INIT420
	for (h >>= 1; --h >= 0; ) {
		for (int x = 0; x < w; x += 2) {
			u_int32_t yuv = R[p[ROFF]];
			yuv += G[p[GOFF]];
			yuv += B[p[BOFF]];

			yp[0] = yuv;
			*up++ = (yuv >> 10) ^ 0x80;
			*vp++ = (yuv >> 20) ^ 0x80;

			yuv = R[p[next_line + ROFF]];
			yuv += G[p[next_line + GOFF]];
			yuv += B[p[next_line + BOFF]];
			yp[outw] = yuv;
			p += 4;
			++yp;

			yuv = R[p[ROFF]];
			yuv += G[p[GOFF]];
			yuv += B[p[BOFF]];
			yp[0] = yuv;

			yuv = R[p[next_line + ROFF]];
			yuv += G[p[next_line + GOFF]];
			yuv += B[p[next_line + BOFF]];
			yp[outw] = yuv;
			p += 4;
			++yp;
		}
		ADVANCE420
	}
}

#undef ROFF
#undef GOFF
#undef BOFF

#define ROFF 2
#define GOFF 1
#define BOFF 0

void RGB_Converter_422::convert24(u_int8_t* p, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)
{
	INIT422
	while (--h >= 0) {
		for (int x = 0; x < w; x += 2) {
			u_int32_t yuv = R[p[ROFF]];
			yuv += G[p[GOFF]];
			yuv += B[p[BOFF]];
			p += 3;

			*yp++ = yuv;
			*up++ = (yuv >> 10) ^ 0x80;
			*vp++ = (yuv >> 20) ^ 0x80;

			yuv = R[p[ROFF]];
			yuv += G[p[GOFF]];
			yuv += B[p[BOFF]];
			p += 3;

			*yp++ = yuv;
		}
		ADVANCE422
	}
}

void RGB_Converter_420::convert24(u_int8_t* p, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)
{
	INIT420
	for (h >>= 1; --h >= 0; ) {
		for (int x = 0; x < w; x += 2) {
			u_int32_t yuv = R[p[ROFF]];
			yuv += G[p[GOFF]];
			yuv += B[p[BOFF]];

			yp[0] = yuv;
			*up++ = (yuv >> 10) ^ 0x80;
			*vp++ = (yuv >> 20) ^ 0x80;

			yuv = R[p[next_line + ROFF]];
			yuv += G[p[next_line + GOFF]];
			yuv += B[p[next_line + BOFF]];
			yp[outw] = yuv;
			p += 3;
			++yp;

			yuv = R[p[ROFF]];
			yuv += G[p[GOFF]];
			yuv += B[p[BOFF]];
			yp[0] = yuv;

			yuv = R[p[next_line + ROFF]];
			yuv += G[p[next_line + GOFF]];
			yuv += B[p[next_line + BOFF]];
			yp[outw] = yuv;
			p += 3;
			++yp;
		}
		ADVANCE420
	}
}

#define RED(x)		(((x) >> 7) & 0xf8)
#define GREEN(x)	(((x) >> 2) & 0xf8)
#define BLUE(x)		(((x) << 3) & 0xf8)

void RGB_Converter_422::convert16(u_int8_t* p, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)
{
	INIT422
	u_int16_t* sp;
	while (--h >= 0) {
		sp = (u_int16_t *)p;
		for (int x = 0; x < w; x += 2) {
			u_int32_t yuv = R[RED(*sp)];
			yuv += G[GREEN(*sp)];
			yuv += B[BLUE(*sp)];
			sp++;

			*yp++ = yuv;
			*up++ = (yuv >> 10) ^ 0x80;
			*vp++ = (yuv >> 20) ^ 0x80;

			yuv = R[RED(*sp)];
			yuv += G[GREEN(*sp)];
			yuv += B[BLUE(*sp)];
			sp++;

			*yp++ = yuv;
		}
		p = (u_int8_t *)sp;
		ADVANCE422
	}
}

void RGB_Converter_420::convert16(u_int8_t* p, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)
{
	INIT420
	next_line /= 2;
	u_int16_t* sp;
	for (h >>= 1; --h >= 0; ) {
		sp = (u_int16_t *)p;
		for (int x = 0; x < w; x += 2) {
			u_int32_t yuv = R[RED(*sp)];
			yuv += G[GREEN(*sp)];
			yuv += B[BLUE(*sp)];

			yp[0] = yuv;
			*up++ = (yuv >> 10) ^ 0x80;
			*vp++ = (yuv >> 20) ^ 0x80;

			yuv = R[RED(sp[next_line])];
			yuv += G[GREEN(sp[next_line])];
			yuv += B[BLUE(sp[next_line])];
			yp[outw] = yuv;
			sp++;
			++yp;

			yuv = R[RED(*sp)];
			yuv += G[GREEN(*sp)];
			yuv += B[BLUE(*sp)];
			yp[0] = yuv;

			yuv = R[RED(sp[next_line])];
			yuv += G[GREEN(sp[next_line])];
			yuv += B[BLUE(sp[next_line])];
			yp[outw] = yuv;
			sp++;
			++yp;
		}
		p = (u_int8_t *)sp;
		ADVANCE420
	}
}

void RGB_Converter_422::convert8(u_int8_t* p, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)
{
	INIT422

        UNUSED(R); //SV-XXX
        UNUSED(G); //SV-XXX
        UNUSED(B); //SV-XXX

	while (--h >= 0) {
		for (int x = 0; x < w; x += 2) {
			*yp++ = ymap_[*p];
			*up++ = umap_[*p];
			*vp++ = vmap_[*p];
			p++;

			*yp++ = ymap_[*p];
			p++;
		}
		ADVANCE422
	}
}

void RGB_Converter_420::convert8(u_int8_t* p, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)
{
	INIT420

	UNUSED(R); //SV-XXX
        UNUSED(G); //SV-XXX
        UNUSED(B); //SV-XXX

	for (h >>= 1; --h >= 0; ) {
		for (int x = 0; x < w; x += 2) {
			yp[0] = ymap_[*p];
			*up++ = umap_[*p];
			*vp++ = vmap_[*p];
			yp[outw] = ymap_[p[next_line]];
			p++;
			++yp;
			yp[0] = ymap_[*p];
			yp[outw] = ymap_[p[next_line]];
			p++;
			++yp;
		}
		ADVANCE420
	}
}

void RGB_Converter_422::convert4(u_int8_t* p, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)
{
	INIT422
        
        UNUSED(R); //SV-XXX
        UNUSED(G); //SV-XXX
        UNUSED(B); //SV-XXX

	while (--h >= 0) {
		for (int x = 0; x < w; x += 2) {
			int v = *p >> 4;
			*yp++ = ymap_[v];
			*up++ = umap_[v];
			*vp++ = vmap_[v];

			*yp++ = ymap_[*p & 0xf];
			p++;
		}
		ADVANCE422
	}
}

void RGB_Converter_420::convert4(u_int8_t* p, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)
{
	INIT420

        UNUSED(R); //SV-XXX
        UNUSED(G); //SV-XXX
        UNUSED(B); //SV-XXX

	for (h >>= 1; --h >= 0; ) {
		for (int x = 0; x < w; x += 2) {
			int v = *p >> 4;
			yp[0] = ymap_[v];
			*up++ = umap_[v];
			*vp++ = vmap_[v];
			yp[outw] = ymap_[*(p + next_line) >> 4];

			++yp;
			yp[0] = ymap_[*p & 0xf];
			yp[outw] = ymap_[p[next_line] & 0xf];
			++yp;

			p++;
		}
		ADVANCE420
	}
}

void RGB_Converter_422::convert1(u_int8_t* p, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)
{
	INIT422
        
        UNUSED(R); //SV-XXX
        UNUSED(G); //SV-XXX
        UNUSED(B); //SV-XXX

	while (--h >= 0) {
		for (int x = 0; x < w; x += 8) {
			int v = *p;
			for (int i = 8; i > 0 ; i -= 2) {
				if (v & 0x80) {
					*yp++ = *ymap_;
					*up++ = *umap_;
					*vp++ = *vmap_;
				} else {
					*yp++ = ymap_[1];
					*up++ = umap_[1];
					*vp++ = vmap_[1];
				}

				if (v & 0x40)
					*yp++ = *ymap_;
				else
					*yp++ = ymap_[1];

				v <<= 2;
			}
			p++;
		}
		ADVANCE422
	}
}

void RGB_Converter_420::convert1(u_int8_t* p, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)
{
	INIT420

        UNUSED(R); //SV-XXX
        UNUSED(G); //SV-XXX
        UNUSED(B); //SV-XXX

	for (h >>= 1; --h >= 0; ) {
		for (int x = 0; x < w; x += 2) {
			int v = *p;
			int vn = p[next_line];
			for (int i = 8; i > 0 ; i -= 2) {
				if (v & 0x80) {
					*yp = *ymap_;
					*up++ = *umap_;
					*vp++ = *vmap_;
				} else {
					*yp = ymap_[1];
					*up++ = umap_[1];
					*vp++ = vmap_[1];
				}

				if (vn & 0x80)
					yp[outw] = *ymap_;
				else
					yp[outw] = ymap_[1];
				++yp;

				if (v & 0x40)
					*yp = *ymap_;
				else
					*yp = ymap_[1];

				if (vn & 0x40)
					yp[outw] = *ymap_;
				else
					yp[outw] = ymap_[1];
				++yp;

				v <<= 2;
				vn <<= 2;
			}
			p++;
		}
		ADVANCE420
	}
}
