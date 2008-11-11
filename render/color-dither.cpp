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
 */

/*
 * This module derived from:
 *
 * Netvideo version 3.2
 * Written by Ron Frederick <frederick@parc.xerox.com>
 * 
 * Video TK widget color rectangle update routines
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdlib.h>
#include <stdio.h>
//#include <X11/Xlib.h>
//#include <X11/Xutil.h>
#include "color.h"
#include "color-pseudo.h"
#include "bsd-endian.h"
#include "vic_tcl.h"
#include "vw.h"

class DitherColorModel : public PseudoColorModel {
public:
	virtual int alloc_colors();
	virtual int command(int argc, const char*const* argv);
};

class DitherColorMatcher : public Matcher {
public:
	DitherColorMatcher() : Matcher("colormodel") {}
	virtual TclObject* match(const char* id) {
		if (strcasecmp(id, "pseudocolor/8/dither") == 0 ||
		    strcasecmp(id, "pseudocolor/8/od") == 0)
			return (new DitherColorModel());
		return (0);
	}
} matcher_dither;

ColorModel::color tessellate[16] = {
	{ 0, 10, 24, 8, 8, -6 }, 
	{ 16, 0, 8, 5, 1, 7 }, 
	{ 4, 8, 28, 9, 10, -3 }, 
	{ 20, 2, 12, 8, 1, 8 }, 
	{ 24, 6, 6, 11, -3, 9 }, 
	{ 8, 12, 22, 11, 5, -2 }, 
	{ 28, 4, 2, 10, -5, 12 }, 
	{ 12, 14, 18, 13, 2, -1 }, 
	{ 6, 9, 30, 10, 11, -3 }, 
	{ 22, 3, 14, 9, 2, 8 }, 
	{ 2, 11, 26, 10, 9, -5 }, 
	{ 18, 1, 10, 7, 1, 7 }, 
	{ 30, 5, 0, 11, -6, 12 }, 
	{ 14, 15, 16, 14, 0, 0 }, 
	{ 26, 7, 4, 12, -4, 9 }, 
	{ 10, 13, 20, 12, 4, -2 }, 
};

#define LIM(x) ((x) >= 255 ? 255 : ((x) < 0 ? 0 : (x)))

extern "C" int cube_ncolor;
extern "C" u_char cube_cmap[];
extern "C" u_char cube_lut[];

extern "C" int jv_cube_ncolor;
extern "C" u_char jv_cube_cmap[];
extern "C" u_char jv_cube_lut[];

/*
 * Allocate a standard color cube.
 */
int DitherColorModel::alloc_colors()
{
	if (installLUT(cube_ncolor, cube_cmap, cube_lut) < 0)
		return (-1);

	/*
	 * Fold ordered dither matrix into lookup table.
	 */
	u_char* lut = lut_;
	lut_ = new u_char[16 * 65536];
	for (int i = 0; i < 65536; ++i) {
		int u0 = (i >> 11) << 3;
		int v0 = ((i >> 6) & 0x1f) << 3;
		int y0 = (i & 0x3f) << 2;
			
		for (int k = 0; k < 16 ; ++k) {
			int y = y0 + 2 * tessellate[k].y;
			int u = u0 + 2 * tessellate[k].u;
			int v = v0 + 2 * tessellate[k].v;

			u = LIM(u);
			v = LIM(v);
			y = LIM(y);

			/* convert to right precision (5U:5V:6Y) */
			y >>= 2;
			u >>= 3;
			v >>= 3;

			/* lookup best match */
			int pixel = lut[(u << 11) | (v << 6) | y];
			lut_[(i << 4) | k] = pixel;
		}
	}
	delete lut;

	return (0);
}

class DitherWindowRenderer;

typedef void (DitherWindowRenderer::*DitherMethod)(const u_char*,
						   u_int, u_int,
						   u_int, u_int) const;

class DitherWindowRenderer : public PseudoWindowRenderer {
public:
	DitherWindowRenderer(VideoWindow* vw, int decimation,
			     DitherColorModel& cm) :
		PseudoWindowRenderer(vw, decimation, cm.graylut()),
		cm_(cm) { }
	void render(const u_char* frm, int off, int x, int w, int h) {
		(this->*method_)(frm, off, x, w, h);
	}
protected:
	DitherColorModel& cm_;
	virtual void update();
	virtual void disable() { method_ = &PseudoWindowRenderer::dither_null; }
	DitherMethod method_;
	void dither_422(const u_char* frm, u_int off, u_int x,
			u_int width, u_int height) const;
	void dither_down2_422(const u_char* frm, u_int off, u_int x,
			      u_int width, u_int height) const;
	void dither_down4_422(const u_char* frm, u_int off, u_int x,
			      u_int width, u_int height) const;
	void dither_down_422(const u_char* frm, u_int off, u_int x,
			     u_int width, u_int height) const;
	void dither_up2_422(const u_char* frm, u_int off, u_int x,
			    u_int width, u_int height) const;
	void dither_420(const u_char* frm, u_int off, u_int x,
			u_int width, u_int height) const;
	void dither_down2_420(const u_char* frm, u_int off, u_int x,
			      u_int width, u_int height) const;
	void dither_down4_420(const u_char* frm, u_int off, u_int x,
			      u_int width, u_int height) const;
	void dither_down_420(const u_char* frm, u_int off, u_int x,
			     u_int width, u_int height) const;
	void dither_up2_420(const u_char* frm, u_int off, u_int x,
			    u_int width, u_int height) const;
};

int DitherColorModel::command(int argc, const char*const* argv)
{
	if (argc == 4 && strcmp(argv[1], "renderer") == 0) {
		Tcl& tcl = Tcl::instance();
		VideoWindow* vw = VideoWindow::lookup(argv[2]);
		int decimation = atoi(argv[3]);
		Renderer* r = new DitherWindowRenderer(vw, decimation, *this);
		tcl.result(r->name());
		return (TCL_OK);
	}
	return (PseudoColorModel::command(argc, argv));
}

void DitherWindowRenderer::update()
{
	static DitherMethod methods[] = {
		&DitherWindowRenderer::dither_up2_420,
		&DitherWindowRenderer::dither_up2_422,
		&DitherWindowRenderer::dither_gray_up,
		&DitherWindowRenderer::dither_gray_up,
		&DitherWindowRenderer::dither_420,
		&DitherWindowRenderer::dither_422,
		&DitherWindowRenderer::dither_gray,
		&DitherWindowRenderer::dither_gray,
		&DitherWindowRenderer::dither_down2_420,
		&DitherWindowRenderer::dither_down2_422,
		&DitherWindowRenderer::dither_gray_down,
		&DitherWindowRenderer::dither_gray_down,
		&DitherWindowRenderer::dither_down4_420,
		&DitherWindowRenderer::dither_down4_422,
		&DitherWindowRenderer::dither_gray_down,
		&DitherWindowRenderer::dither_gray_down,
		&DitherWindowRenderer::dither_down_420,
		&DitherWindowRenderer::dither_down_422,
		&DitherWindowRenderer::dither_gray_down,
		&DitherWindowRenderer::dither_gray_down,
	};
	method_ = methods[index()];
}

#if BYTE_ORDER == LITTLE_ENDIAN
#define PIX0(i) ((i) << 2)
#define PIX1(i) ((i) >> 6)
#define PIX2(i) ((i) >> 14)
#define PIX3(i) ((i) >> 22)
#define UV0 ((v << 7) & 0xffc00)
#define UV1 ((u << 4) & 0xffc00)
#define UV2 ((v >> 9) & 0xffc00)
#define UV3 ((u >> 12) & 0xffc00)
#else
#define PIX0(i) ((i) >> 22)
#define PIX1(i) ((i) >> 14)
#define PIX2(i) ((i) >> 6)
#define PIX3(i) ((i) << 2)
#define UV0 ((u >> 12) & 0xffc00)
#define UV1 ((v >> 9) & 0xffc00)
#define UV2 ((u << 4) & 0xffc00)
#define UV3 ((v << 7) & 0xffc00)
#endif

void DitherWindowRenderer::dither_422(const u_char* frm,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
        UNUSED(x); //SV-XXX: unused

	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_int* xip = (u_int*)(pixbuf_ + off);
	register int w = width;
	register int z = 0;
	register const u_char* dither = cm_.lut();
	for (register int len = w * height; len > 0; len -= 8) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1;
		register u_int out = 0;
		register const u_char* y2r;

		/*
		 * we do 8 pixels (4 u & v values) per iteration.
		 * the yuv to rgb conversion table is 20 bits (6
		 * y, 5 u, 5 v, 4 dither).  grab 4 u & v values and
		 * construct the appropriate 16 bit representation
		 * (minus the y values).
		 */
		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		/*
		 * grab 4 y values & map them to rgb.
		 */
		y1 = *(const u_int*)yp;
		y2r = dither + z + UV0;
		SPLICE(out, y2r[(PIX0(y1) & 0x3f0) + 0], 24);
		SPLICE(out, y2r[(PIX1(y1) & 0x3f0) + 1], 16);

		y2r = dither + z + UV1;
		SPLICE(out, y2r[(PIX2(y1) & 0x3f0) + 2], 8);
		SPLICE(out, y2r[(PIX3(y1) & 0x3f0) + 3], 0);
		xip[0] =  out;

		y1 = *(const u_int*)(yp + 4);
		y2r = dither + z + UV2;
		out = 0;
		SPLICE(out, y2r[(PIX0(y1) & 0x3f0) + 0], 24);
		SPLICE(out, y2r[(PIX1(y1) & 0x3f0) + 1], 16);

		y2r = dither + z + UV3;
		SPLICE(out, y2r[(PIX2(y1) & 0x3f0) + 2], 8);
		SPLICE(out, y2r[(PIX3(y1) & 0x3f0) + 3], 0);
		xip[1] =  out;

		xip += 2;
		yp += 8;
		up += 4;
		vp += 4;

		w -= 8;
		if (w <= 0) {
			w = width;
			register int pstride = iw - w;
			register int cstride = pstride >> 1;
			yp += pstride;
			up += cstride;
			vp += cstride;
			xip += (pstride >> 2);
			z = (z + 4) & 0xf;
		}
	}
}

void DitherWindowRenderer::dither_down2_422(const u_char* frm,
					    u_int off, u_int x,
					    u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_int* xip = (u_int*)(pixbuf_ + ((off - x) >> 2) + (x >> 1));
	register int w = width;
	register int z = 0;
	register const u_char* dither = cm_.lut();
	for (register int len = w * height >> 1; len > 0; len -= 8) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1, out = 0;
		register const u_char* dz = dither + z;
		register const u_char* y2r;

		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		y1 = *(const u_int*)yp;
		y2r = dz + UV0;
		SPLICE(out, y2r[(PIX0(y1) & 0x3f0)], 24);

		y2r = ++dz + UV1;
		SPLICE(out, y2r[PIX2(y1) & 0x3f0], 16);

		y1 = *(const u_int*)(yp + 4);
		y2r = ++dz + UV2;
		SPLICE(out, y2r[PIX0(y1) & 0x3f0], 8);

		y2r = ++dz + UV3;
		SPLICE(out, y2r[PIX2(y1) & 0x3f0], 0);

		*xip++ = out;
		yp += 8;
		up += 4;
		vp += 4;

		w -= 8;
		if (w <= 0) {
			w = width;
			register int pstride = 2 * iw - w;
			register int cstride = pstride >> 1;
			yp += pstride;
			up += cstride;
			vp += cstride;
			xip += (iw - w) >> 3;
			z = (z + 4) & 0xf;
		}
	}
}

void DitherWindowRenderer::dither_down4_422(const u_char* frm,
					    u_int off, u_int x,
					    u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register int w = width;
	register u_int doff = ((off - x) >> 4) + (x >> 2);
	register int zl = 0;
	register const u_char* dither = cm_.lut() + zl;
	register int len = w * height >> 2;
	if ((w & 0xf) != 0 || (len & 0xf) != 0 || (doff & 3) != 0) {
		register u_short* xip = (u_short*)(pixbuf_ + doff);
		register int z = 0;
		for (; len > 0; len -= 8) {
			register u_int u = *(const u_int*)up;
			register u_int v = *(const u_int*)vp;
			register u_int y1;
			register u_short out = 0;
#if BYTE_ORDER == LITTLE_ENDIAN
			v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
#else
			u = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
#endif

			y1 = (u_int)(yp[0] & 0xfc) << 2;
			SPLICE(out, dither[y1 + UV0 + z], 8);
			++z;
			y1 = (u_int)(yp[4] & 0xfc) << 2;
			SPLICE(out, dither[y1 + UV2 + z], 0);
			*xip++ = out;
			z = (z + 1) & 0x3;
			yp += 8;
			up += 4;
			vp += 4;

			w -= 8;
			if (w <= 0) {
				w = width;
				register int pstride = 4 * iw - w;
				register int cstride = pstride >> 1;
				yp += pstride;
				up += cstride;
				vp += cstride;
				xip += (iw - w) >> 3;
				zl = (zl + 4) & 0xf;
				dither = cm_.lut() + zl;
				z = 0;
			}
		}
	} else {
		register u_int* xip = (u_int*)(pixbuf_ + doff);
		for (; len > 0; len -= 16) {
			register u_int u = *(const u_int*)up;
			register u_int v = *(const u_int*)vp;
			register u_int y1, out = 0;
#if BYTE_ORDER == LITTLE_ENDIAN
			v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
#else
			u = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
#endif
			y1 = (u_int)(yp[0] & 0xfc) << 2;
			SPLICE(out, dither[y1 + UV0 + 0], 24);
			y1 = (u_int)(yp[4] & 0xfc) << 2;
			SPLICE(out, dither[y1 + UV2 + 1], 16);
			up += 4;
			vp += 4;
			u = *(const u_int*)up;
			v = *(const u_int*)vp;
#if BYTE_ORDER == LITTLE_ENDIAN
			v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
#else
			u = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
#endif
			y1 = (u_int)(yp[8] & 0xfc) << 2;
			SPLICE(out, dither[y1 + UV0 + 2], 8);
			y1 = (u_int)(yp[12] & 0xfc) << 2;
			SPLICE(out, dither[y1 + UV2 + 3], 0);
			*xip++ = out;

			yp += 16;
			up += 4;
			vp += 4;

			w -= 16;
			if (w <= 0) {
				w = width;
				register int pstride = 4 * iw - w;
				register int cstride = pstride >> 1;
				yp += pstride;
				up += cstride;
				vp += cstride;
				xip += (iw - w) >> 4;
				zl = (zl + 4) & 0xf;
				dither = cm_.lut() + zl;
			}
		}
	}
}

void DitherWindowRenderer::dither_down_422(const u_char* frm,
					   u_int off, u_int x,
					   u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register int s = scale_;
	register int istride = 1 << s;
	register int w = width;
	register int zl = 0;
	register const u_char* dither = cm_.lut() + zl;
	register int z = (x >> s) & 3;
	register u_char* xip = pixbuf_ + ((off - x) >> (s + s)) + (x >> s);
	for (register int len = w * height >> s; len > 0; len -= istride) {
		register u_int o;
		o = ((*up & 0xf8) << 12) + ((*vp & 0xf8) << 7) +
		    ((*yp & 0xfc) << 2);
		*xip++ = dither[o + z];
		z = (z + 1) & 3;

		yp += istride;
		up += istride >> 1;
		vp += istride >> 1;

		w -= istride;
		if (w <= 0) {
			w = width;
			register int pstride = (iw << s) - w;
			register int cstride = pstride >> 1;
			yp += pstride;
			up += cstride;
			vp += cstride;
			xip += (iw - w) >> s;
			zl = (zl + 4) & 0xc;
			dither = cm_.lut() + zl;
		}
	}
}

void DitherWindowRenderer::dither_up2_422(const u_char* frm,
					  u_int off, u_int x,
					  u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_int* xip = (u_int*)(pixbuf_ + ((off - x) << 2) + (x << 1));
	register int w = width;
	register u_int e1 = yp[0] << 2;
	register int z = 0;
	register const u_char* dither = cm_.lut() + z;

	for (register int len = width * height; len > 0; len -= 8) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1, e2, out1, out2, p;
		register const u_char* y2r;
		register u_int* xip2 = xip + (iw >> 1);

		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		y1 = *(const u_int*)yp;
		out1 = 0;
		out2 = 0;
		y2r = dither + UV0;
		e2 = PIX0(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2r[p + 0], 24);
		SPLICE(out2, y2r[p + 4], 24);
		SPLICE(out1, y2r[e2 + 1], 16);
		SPLICE(out2, y2r[e2 + 5], 16);
		e1 = PIX1(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2r[p + 2], 8);
		SPLICE(out2, y2r[p + 6], 8);
		SPLICE(out1, y2r[e1 + 3], 0);
		SPLICE(out2, y2r[e1 + 7], 0);
		*xip++ = out1;
		*xip2++ = out2;

		out1 = 0;
		out2 = 0;
		y2r = dither + UV1;
		e2 = PIX2(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2r[p + 0], 24);
		SPLICE(out2, y2r[p + 4], 24);
		SPLICE(out1, y2r[e2 + 1], 16);
		SPLICE(out2, y2r[e2 + 5], 16);
		e1 = PIX3(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2r[p + 2], 8);
		SPLICE(out2, y2r[p + 6], 8);
		SPLICE(out1, y2r[e1 + 3], 0);
		SPLICE(out2, y2r[e1 + 7], 0);
		*xip++ = out1;
		*xip2++ = out2;

		yp += 4;
		y1 = *(const u_int*)yp;
		out1 = 0;
		out2 = 0;
		y2r = dither + UV2;
		e2 = PIX0(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2r[p + 0], 24);
		SPLICE(out2, y2r[p + 4], 24);
		SPLICE(out1, y2r[e2 + 1], 16);
		SPLICE(out2, y2r[e2 + 5], 16);
		e1 = PIX1(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2r[p + 2], 8);
		SPLICE(out2, y2r[p + 6], 8);
		SPLICE(out1, y2r[e1 + 3], 0);
		SPLICE(out2, y2r[e1 + 7], 0);
		*xip++ = out1;
		*xip2++ = out2;

		out1 = 0;
		out2 = 0;
		y2r = dither + UV3;
		e2 = PIX2(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2r[p + 0], 24);
		SPLICE(out2, y2r[p + 4], 24);
		SPLICE(out1, y2r[e2 + 1], 16);
		SPLICE(out2, y2r[e2 + 5], 16);
		e1 = PIX3(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2r[p + 2], 8);
		SPLICE(out2, y2r[p + 6], 8);
		SPLICE(out1, y2r[e1 + 3], 0);
		SPLICE(out2, y2r[e1 + 7], 0);
		*xip++ = out1;
		*xip2++ = out2;

		yp += 4;
		up += 4;
		vp += 4;

		w -= 8;
		if (w <= 0) {
			w = width;
			register u_int pstride = iw - w;
			register u_int cstride = pstride >> 1;
			yp += pstride;
			e1 = yp[0] << 2;
			up += cstride;
			vp += cstride;
			xip += (iw + pstride) >> 1;
			z = (z + 8) & 0xc;
			dither = cm_.lut() + z;
		}
	}
}

void DitherWindowRenderer::dither_420(const u_char* frm,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register u_int* xip = (u_int*)(pixbuf_ + off);
	register int w = width;
	register int z = 0;
	register const u_char* dither = cm_.lut() + z;
	for (register int len = width * height; len > 0; len -= 16) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1, y2, out1, out2;
		register const u_char* y2r;
		register u_int* xip2 = xip + (iw >> 2);

		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		y1 = *(const u_int*)yp;
		y2 = *(const u_int*)(yp + iw);
		out1 = 0;
		out2 = 0;
		y2r = dither + UV0;
		SPLICE(out1, y2r[(PIX0(y1) & 0x3f0)+ 0], 24);
		SPLICE(out1, y2r[(PIX1(y1) & 0x3f0)+ 1], 16);
		SPLICE(out2, y2r[(PIX0(y2) & 0x3f0)+ 4], 24);
		SPLICE(out2, y2r[(PIX1(y2) & 0x3f0)+ 5], 16);

		y2r = dither + UV1;
		SPLICE(out1, y2r[(PIX2(y1) & 0x3f0)+ 2], 8);
		SPLICE(out1, y2r[(PIX3(y1) & 0x3f0)+ 3], 0);
		SPLICE(out2, y2r[(PIX2(y2) & 0x3f0)+ 6], 8);
		SPLICE(out2, y2r[(PIX3(y2) & 0x3f0)+ 7], 0);

		*xip++ = out1;
		*xip2++ = out2;

		yp += 4;
		y1 = *(const u_int*)yp;
		y2 = *(const u_int*)(yp + iw);
		out1 = 0;
		out2 = 0;
		y2r = dither + UV2;
		SPLICE(out1, y2r[(PIX0(y1) & 0x3f0)+ 0], 24);
		SPLICE(out1, y2r[(PIX1(y1) & 0x3f0)+ 1], 16);
		SPLICE(out2, y2r[(PIX0(y2) & 0x3f0)+ 4], 24);
		SPLICE(out2, y2r[(PIX1(y2) & 0x3f0)+ 5], 16);

		y2r = dither + UV3;
		SPLICE(out1, y2r[(PIX2(y1) & 0x3f0)+ 2], 8);
		SPLICE(out1, y2r[(PIX3(y1) & 0x3f0)+ 3], 0);
		SPLICE(out2, y2r[(PIX2(y2) & 0x3f0)+ 6], 8);
		SPLICE(out2, y2r[(PIX3(y2) & 0x3f0)+ 7], 0);

		*xip++ = out1;
		*xip2++ = out2;

		yp += 4;
		up += 4;
		vp += 4;

		w -= 8;
		if (w <= 0) {
			w = width;
			register int pstride = 2 * iw - w;
			register int cstride = (iw - w) >> 1;
			yp += pstride;
			up += cstride;
			vp += cstride;
			xip += (pstride >> 2);
			z = (z + 8) & 0xf;
			dither = cm_.lut() + z;
		}
	}
}

void DitherWindowRenderer::dither_down2_420(const u_char* frm,
					    u_int off, u_int x,
					    u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	off = ((off - x) >> 2) + (x >> 1);
	register const u_char* up = frm + framesize_ + off;
	register const u_char* vp = up + (framesize_ >> 2);
	register u_int* xip = (u_int*)(pixbuf_ + off);
	register int w = width;
	register int z = 0;
	register const u_char* dither = cm_.lut();
	for (register int len = w * height >> 1; len > 0; len -= 8) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1, out = 0;
		register const u_char* dz = dither + z;
		register const u_char* y2r;

		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		y1 = *(const u_int*)yp;
		y2r = dz + UV0;
		SPLICE(out, y2r[(PIX0(y1) & 0x3f0)], 24);

		y2r = ++dz + UV1;
		SPLICE(out, y2r[PIX2(y1) & 0x3f0], 16);

		y1 = *(const u_int*)(yp + 4);
		y2r = ++dz + UV2;
		SPLICE(out, y2r[PIX0(y1) & 0x3f0], 8);

		y2r = ++dz + UV3;
		SPLICE(out, y2r[PIX2(y1) & 0x3f0], 0);

		*xip++ = out;
		yp += 8;
		up += 4;
		vp += 4;

		w -= 8;
		if (w <= 0) {
			w = width;
			register int pstride = 2 * iw - w;
			register int cstride = (iw - w) >> 1;
			yp += pstride;
			up += cstride;
			vp += cstride;
			xip += cstride >> 2;
			z = (z + 4) & 0xf;
		}
	}
}

void DitherWindowRenderer::dither_down4_420(const u_char* frm,
					    u_int off, u_int x,
					    u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register u_int doff = ((off - x) >> 4) + (x >> 2);
	register int w = width;
	register int zl = 0;
	register const u_char* dither = cm_.lut() + zl;
	register int len = w * height >> 2;
	if ((w & 0xf) != 0 || (len & 0xf) != 0 || (doff & 3) != 0) {
		register u_short* xip = (u_short*)(pixbuf_ + doff);
		register int z = 0;
		for (; len > 0; len -= 8) {
			register u_int u = *(const u_int*)up;
			register u_int v = *(const u_int*)vp;
			register u_int y1;
			register u_short out = 0;
#if BYTE_ORDER == LITTLE_ENDIAN
			v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
#else
			u = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
#endif
			y1 = (u_int)(yp[0] & 0xfc) << 2;
			SPLICE(out, dither[y1 + UV0 + z], 8);
			++z;
			y1 = (u_int)(yp[4] & 0xfc) << 2;
			SPLICE(out, dither[y1 + UV2 + z], 0);
			*xip++ = out;
			z = (z + 1) & 0x3;
			yp += 8;
			up += 4;
			vp += 4;

			w -= 8;
			if (w <= 0) {
				w = width;
				register int pstride = 4 * iw - w;
				register int cstride = iw - (w >> 1);
				yp += pstride;
				up += cstride;
				vp += cstride;
				xip += (iw - w) >> 3;
				zl = (zl + 4) & 0xf;
				dither = cm_.lut() + zl;
				z = 0;
			}
		}
	} else {
		register u_int* xip = (u_int*)(pixbuf_ + doff);
		for (; len > 0; len -= 16) {
			register u_int u = *(const u_int*)up;
			register u_int v = *(const u_int*)vp;
			register u_int y1, out = 0;
#if BYTE_ORDER == LITTLE_ENDIAN
			v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
#else
			u = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
#endif
			y1 = (u_int)(yp[0] & 0xfc) << 2;
			SPLICE(out, dither[y1 + UV0 + 0], 24);
			y1 = (u_int)(yp[4] & 0xfc) << 2;
			SPLICE(out, dither[y1 + UV2 + 1], 16);
			up += 4;
			vp += 4;
			u = *(const u_int*)up;
			v = *(const u_int*)vp;
#if BYTE_ORDER == LITTLE_ENDIAN
			v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
#else
			u = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
#endif
			y1 = (u_int)(yp[8] & 0xfc) << 2;
			SPLICE(out, dither[y1 + UV0 + 2], 8);
			y1 = (u_int)(yp[12] & 0xfc) << 2;
			SPLICE(out, dither[y1 + UV2 + 3], 0);
			*xip++ = out;

			yp += 16;
			up += 4;
			vp += 4;

			w -= 16;
			if (w <= 0) {
				w = width;
				register int pstride = 4 * iw - w;
				register int cstride = iw - (w >> 1);
				yp += pstride;
				up += cstride;
				vp += cstride;
				xip += (iw - w) >> 4;
				zl = (zl + 4) & 0xf;
				dither = cm_.lut() + zl;
			}
		}
	}
}

void DitherWindowRenderer::dither_down_420(const u_char* frm,
					   u_int off, u_int x,
					   u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register int s = scale_;
	register int istride = 1 << s;
	register int w = width;
#ifdef notdef
	register int zl = (((off - x) >> s) & 3) << 2;
#endif
	register int zl = 0;
	register const u_char* dither = cm_.lut() + zl;
	register int z = (x >> s) & 3;
	register u_char* xip = pixbuf_ + ((off - x) >> (s + s)) + (x >> s);
	for (register int len = w * height >> s; len > 0; len -= istride) {
		register u_int o;
		o = ((*up & 0xf8) << 12) + ((*vp & 0xf8) << 7) +
		    ((*yp & 0xfc) << 2);
		*xip++ = dither[o + z];
		z = (z + 1) & 3;

		yp += istride;
		up += istride >> 1;
		vp += istride >> 1;

		w -= istride;
		if (w <= 0) {
			w = width;
			register int pstride = (iw << s) - w;
			register int cstride = (iw << (s - 1)) - (w >> 1);
			yp += pstride;
			up += cstride;
			vp += cstride;
			xip += (iw - w) >> s;
			zl = (zl + 4) & 0xc;
			dither = cm_.lut() + zl;
		}
	}
}

void DitherWindowRenderer::dither_up2_420(const u_char* frm,
					  u_int off, u_int x,
					  u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register u_int* xip = (u_int*)(pixbuf_ + ((off - x) << 2) + (x << 1));
	register int w = width;
	register int z = 0;
	register const u_char* dither = cm_.lut() + z;
	register u_int e1 = (yp[0] & 0xfc) << 2, o1 = (yp[iw] & 0xfc) << 2;
	for (register int len = width * height; len > 0; len -= 16) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1, y2, e2, o2, out1, out2, p;
		register const u_char* y2re;
		register const u_char* y2ro;
		register u_int* xip2 = xip + (iw >> 1);

		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		/* double pixels 0 & 1 of the `even' row */
		y1 = *(const u_int*)yp;
		out1 = 0;
		out2 = 0;
		y2re = dither + UV0;
		y2ro = y2re + 8;
		e2 = PIX0(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2re[p + 0], 24);
		SPLICE(out2, y2re[p + 4], 24);
		SPLICE(out1, y2re[e2 + 1], 16);
		SPLICE(out2, y2re[e2 + 5], 16);
		e1 = PIX1(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2re[p + 2], 8);
		SPLICE(out2, y2re[p + 6], 8);
		SPLICE(out1, y2re[e1 + 3], 0);
		SPLICE(out2, y2re[e1 + 7], 0);
		*xip = out1;
		*xip2 = out2;

		/* double pixels 0 & 1 of the `odd' row */
		y2 = *(const u_int*)(yp + iw);
		out1 = 0;
		out2 = 0;
		o2 = PIX0(y1) & 0x3f0;
		p = ((o1 + o2) >> 1) & 0x3f0;
		SPLICE(out1, y2ro[p + 0], 24);
		SPLICE(out2, y2ro[p + 4], 24);
		SPLICE(out1, y2ro[o2 + 1], 16);
		SPLICE(out2, y2ro[o2 + 5], 16);
		o1 = PIX1(y1) & 0x3f0;
		p = ((o1 + o2) >> 1) & 0x3f0;
		SPLICE(out1, y2ro[p + 2], 8);
		SPLICE(out2, y2ro[p + 6], 8);
		SPLICE(out1, y2ro[o1 + 3], 0);
		SPLICE(out2, y2ro[o1 + 7], 0);
		xip[iw] = out1;
		xip2[iw] = out2;
		++xip;
		++xip2;

		/* double pixels 2 & 3 of the `even' row */
		out1 = 0;
		out2 = 0;
		y2re = dither + UV1;
		y2ro = y2re + 8;
		e2 = PIX2(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2re[p + 0], 24);
		SPLICE(out2, y2re[p + 4], 24);
		SPLICE(out1, y2re[e2 + 1], 16);
		SPLICE(out2, y2re[e2 + 5], 16);
		e1 = PIX3(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2re[p + 2], 8);
		SPLICE(out2, y2re[p + 6], 8);
		SPLICE(out1, y2re[e1 + 3], 0);
		SPLICE(out2, y2re[e1 + 7], 0);
		*xip = out1;
		*xip2 = out2;

		/* double pixels 2 & 3 of the `odd' row */
		out1 = 0;
		out2 = 0;
		o2 = PIX2(y1) & 0x3f0;
		p = ((o1 + o2) >> 1) & 0x3f0;
		SPLICE(out1, y2ro[p + 0], 24);
		SPLICE(out2, y2ro[p + 4], 24);
		SPLICE(out1, y2ro[o2 + 1], 16);
		SPLICE(out2, y2ro[o2 + 5], 16);
		o1 = PIX3(y1) & 0x3f0;
		p = ((o1 + o2) >> 1) & 0x3f0;
		SPLICE(out1, y2ro[p + 2], 8);
		SPLICE(out2, y2ro[p + 6], 8);
		SPLICE(out1, y2ro[o1 + 3], 0);
		SPLICE(out2, y2ro[o1 + 7], 0);
		xip[iw] = out1;
		xip2[iw] = out2;
		++xip;
		++xip2;

		/* double pixels 4 & 5 of the `even' row */
		yp += 4;
		y1 = *(const u_int*)yp;
		out1 = 0;
		out2 = 0;
		y2re = dither + UV2;
		y2ro = y2re + 8;
		e2 = PIX0(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2re[p + 0], 24);
		SPLICE(out2, y2re[p + 4], 24);
		SPLICE(out1, y2re[e2 + 1], 16);
		SPLICE(out2, y2re[e2 + 5], 16);
		e1 = PIX1(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2re[p + 2], 8);
		SPLICE(out2, y2re[p + 6], 8);
		SPLICE(out1, y2re[e1 + 3], 0);
		SPLICE(out2, y2re[e1 + 7], 0);
		*xip = out1;
		*xip2 = out2;

		/* double pixels 4 & 5 of the `odd' row */
		y2 = *(const u_int*)(yp + iw);
		out1 = 0;
		out2 = 0;
		o2 = PIX0(y1) & 0x3f0;
		p = ((o1 + o2) >> 1) & 0x3f0;
		SPLICE(out1, y2ro[p + 0], 24);
		SPLICE(out2, y2ro[p + 4], 24);
		SPLICE(out1, y2ro[o2 + 1], 16);
		SPLICE(out2, y2ro[o2 + 5], 16);
		o1 = PIX1(y1) & 0x3f0;
		p = ((o1 + o2) >> 1) & 0x3f0;
		SPLICE(out1, y2ro[p + 2], 8);
		SPLICE(out2, y2ro[p + 6], 8);
		SPLICE(out1, y2ro[o1 + 3], 0);
		SPLICE(out2, y2ro[o1 + 7], 0);
		xip[iw] = out1;
		xip2[iw] = out2;
		++xip;
		++xip2;

		/* double pixels 6 & 7 of the `even' row */
		out1 = 0;
		out2 = 0;
		y2re = dither + UV3;
		y2ro = y2re + 8;
		e2 = PIX2(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2re[p + 0], 24);
		SPLICE(out2, y2re[p + 4], 24);
		SPLICE(out1, y2re[e2 + 1], 16);
		SPLICE(out2, y2re[e2 + 5], 16);
		e1 = PIX3(y1) & 0x3f0;
		p = ((e1 + e2) >> 1) & 0x3f0;
		SPLICE(out1, y2re[p + 2], 8);
		SPLICE(out2, y2re[p + 6], 8);
		SPLICE(out1, y2re[e1 + 3], 0);
		SPLICE(out2, y2re[e1 + 7], 0);
		*xip = out1;
		*xip2 = out2;

		/* double pixels 6 & 7 of the `odd' row */
		out1 = 0;
		out2 = 0;
		o2 = PIX2(y1) & 0x3f0;
		p = ((o1 + o2) >> 1) & 0x3f0;
		SPLICE(out1, y2ro[p + 0], 24);
		SPLICE(out2, y2ro[p + 4], 24);
		SPLICE(out1, y2ro[o2 + 1], 16);
		SPLICE(out2, y2ro[o2 + 5], 16);
		o1 = PIX3(y1) & 0x3f0;
		p = ((o1 + o2) >> 1) & 0x3f0;
		SPLICE(out1, y2ro[p + 2], 8);
		SPLICE(out2, y2ro[p + 6], 8);
		SPLICE(out1, y2ro[o1 + 3], 0);
		SPLICE(out2, y2ro[o1 + 7], 0);
		xip[iw] = out1;
		xip2[iw] = out2;
		++xip;
		++xip2;

		up += 4;
		vp += 4;
		yp += 4;

		w -= 8;
		if (w <= 0) {
			w = width;
			register u_int pstride = 2 * iw - w;
			register u_int cstride = (iw - w) >> 1;
			yp += pstride;
			e1 = (yp[0] & 0xfc) << 2;
			o1 = (yp[iw] & 0xfc) << 2;
			up += cstride;
			vp += cstride;
			xip += (8 * iw - 2 * w) >> 2;
		}
	}
}
