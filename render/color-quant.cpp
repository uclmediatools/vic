/*
 * Copyright (c) 1994-1995 Regents of the University of California.
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdlib.h>
#include <stdio.h>
#ifndef WIN32
#include <sys/file.h>
#endif
//#include <X11/Xlib.h>
//#include <X11/Xutil.h>
#include "color.h"
#include "color-pseudo.h"
#include "bsd-endian.h"
#include "vic_tcl.h"
#include "vw.h"

extern "C" int quant_ncolor;
extern "C" u_char quant_cmap[];
extern "C" u_char quant_lut[];

class QuantColorModel : public PseudoColorModel {
public:
	virtual int alloc_colors();
	virtual int command(int argc, const char*const* argv);
};

class QuantColorMatcher : public Matcher {
public:
	QuantColorMatcher() : Matcher("colormodel") {}
	virtual TclObject* match(const char* id) {
		if (strcasecmp(id, "pseudocolor/8/quant") == 0 ||
		    strcasecmp(id, "pseudocolor/8/quantize") == 0)
			return (new QuantColorModel());
		return (0);
	}
} matcher_quant;

int QuantColorModel::alloc_colors()
{
	return (installLUT(quant_ncolor, quant_cmap, quant_lut));
}

class QuantWindowRenderer;

typedef void (QuantWindowRenderer::*QuantMethod)(const u_char*, u_int,
						 u_int, u_int, u_int) const;

class QuantWindowRenderer : public PseudoWindowRenderer {
public:
	QuantWindowRenderer(VideoWindow* vw, int decimation, QuantColorModel& cm) :
		PseudoWindowRenderer(vw, decimation, cm.graylut()),
		cm_(cm) { }
	void render(const u_char* frm, int off, int x, int w, int h) {
		(this->*method_)(frm, off, x, w, h);
	}
protected:
	QuantColorModel& cm_;
	virtual void update();
	virtual void disable() { method_ = &PseudoWindowRenderer::dither_null; }
	QuantMethod method_;
	void map_422(const u_char* frm, u_int off, u_int x,
		     u_int width, u_int height) const;
	void map_down2_422(const u_char* frm, u_int off, u_int x,
			   u_int width, u_int height) const;
	void map_down4_422(const u_char* frm, u_int off, u_int x,
			   u_int width, u_int height) const;
	void map_down_422(const u_char* frm, u_int off, u_int x,
			  u_int width, u_int height) const;
	void map_up2_422(const u_char* frm, u_int off, u_int x,
			 u_int width, u_int height) const;
	void map_420(const u_char* frm, u_int off, u_int x,
		     u_int width, u_int height) const;
	void map_down2_420(const u_char* frm, u_int off, u_int x,
			   u_int width, u_int height) const;
	void map_down4_420(const u_char* frm, u_int off, u_int x,
			   u_int width, u_int height) const;
	void map_down_420(const u_char* frm, u_int off, u_int x,
			  u_int width, u_int height) const;
	void map_up2_420(const u_char* frm, u_int off, u_int x,
			 u_int width, u_int height) const;
};

int QuantColorModel::command(int argc, const char*const* argv)
{
	if (argc == 4 && strcmp(argv[1], "renderer") == 0) {
		Tcl& tcl = Tcl::instance();
		VideoWindow* vw = VideoWindow::lookup(argv[2]);
		int decimation = atoi(argv[3]);
		Renderer* r = new QuantWindowRenderer(vw, decimation, *this);
		tcl.result(r->name());
		return (TCL_OK);
	}
	return (PseudoColorModel::command(argc, argv));
}

void QuantWindowRenderer::update()
{
	static QuantMethod methods[] = {
	  &QuantWindowRenderer::map_up2_420,
	  &QuantWindowRenderer::map_up2_422,
	  &QuantWindowRenderer::dither_gray_up,
	  &QuantWindowRenderer::dither_gray_up,
	  &QuantWindowRenderer::map_420,
	  &QuantWindowRenderer::map_422,
	  &QuantWindowRenderer::dither_gray,
	  &QuantWindowRenderer::dither_gray,
	  &QuantWindowRenderer::map_down2_420,
	  &QuantWindowRenderer::map_down2_422,
	  &QuantWindowRenderer::dither_gray_down,
	  &QuantWindowRenderer::dither_gray_down,
	  &QuantWindowRenderer::map_down4_420,
	  &QuantWindowRenderer::map_down4_422,
	  &QuantWindowRenderer::dither_gray_down,
	  &QuantWindowRenderer::dither_gray_down,
	  &QuantWindowRenderer::map_down_420,
	  &QuantWindowRenderer::map_down_422,
	  &QuantWindowRenderer::dither_gray_down,
	  &QuantWindowRenderer::dither_gray_down,
	};
	method_ = methods[index()];
}

#if BYTE_ORDER == LITTLE_ENDIAN
#define PIX0(i) ((i) >> 2)
#define PIX1(i) ((i) >> 10)
#define PIX2(i) ((i) >> 18)
#define PIX3(i) ((i) >> 26)
#define UV0 ((v << 3) & 0xffc0)
#define UV1 (u & 0xffc0)
#define UV2 ((v >> 13) & 0xffc0)
#define UV3 ((u >> 16) & 0xffc0)
#else
#define PIX0(i) ((i) >> 26)
#define PIX1(i) ((i) >> 18)
#define PIX2(i) ((i) >> 10)
#define PIX3(i) ((i) >> 2)
#define UV0 ((u >> 16) & 0xffc0)
#define UV1 ((v >> 13) & 0xffc0)
#define UV2 (u & 0xffc0)
#define UV3 ((v << 3) & 0xffc0)
#endif

void QuantWindowRenderer::map_422(const u_char* frm, u_int off,
				  u_int x, u_int width, u_int height) const
{
        UNUSED(x); //SV-XXX: unused

	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_int* xip = (u_int*)(pixbuf_ + off);
	register int w = width;
	register const u_char* lut = cm_.lut();
	for (register int len = w * height; len > 0; len -= 8) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1;
		register u_int out = 0;
		register const u_char* y2r;

		/*
		 * we do 8 pixels (4 u & v values) per iteration.
		 * the yuv to rgb conversion table is 16 bits (6
		 * y, 5 u, 5 v).  grab 4 u & v values and
		 * construct the appropriate 16 bit representation
		 * (minus the y values).
		 */
		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		/*
		 * grab first 4 y values & map them to rgb.
		 */
		y1 = *(const u_int*)yp;
		y2r = lut + UV0;
		SPLICE(out, y2r[PIX0(y1) & 0x3f], 24);
		SPLICE(out, y2r[PIX1(y1) & 0x3f], 16);

		y2r = lut + UV1;
		SPLICE(out, y2r[PIX2(y1) & 0x3f], 8);
		SPLICE(out, y2r[PIX3(y1) & 0x3f], 0);
		xip[0] =  out;

		/*
		 * grab last 4 y values & map them to rgb.
		 */
		y1 = *(const u_int*)(yp + 4);
		y2r = lut + UV2;
		out = 0;
		SPLICE(out, y2r[PIX0(y1) & 0x3f], 24);
		SPLICE(out, y2r[PIX1(y1) & 0x3f], 16);

		y2r = lut + UV3;
		SPLICE(out, y2r[PIX2(y1) & 0x3f], 8);
		SPLICE(out, y2r[PIX3(y1) & 0x3f], 0);
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
		}
	}
}

void QuantWindowRenderer::map_down2_422(const u_char* frm,
					u_int off, u_int x,
					u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_int* xip = (u_int*)(pixbuf_ + ((off - x) >> 2) + (x >> 1));
	register int w = width;
	register const u_char* lut = cm_.lut();
	for (register int len = w * height >> 1; len > 0; len -= 8) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1, out = 0;
		register const u_char* y2r;

		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		y1 = *(const u_int*)yp;
		y2r = lut + UV0;
		SPLICE(out, y2r[PIX0(y1) & 0x3f], 24);

		y2r = lut + UV1;
		SPLICE(out, y2r[PIX2(y1) & 0x3f], 16);

		y1 = *(const u_int*)(yp + 4);
		y2r = lut + UV2;
		SPLICE(out, y2r[PIX0(y1) & 0x3f], 8);

		y2r = lut + UV3;
		SPLICE(out, y2r[PIX2(y1) & 0x3f], 0);

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
		}
	}
}

void QuantWindowRenderer::map_down4_422(const u_char* frm,
					u_int off, u_int x,
					u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register int w = width;
	register const u_char* lut = cm_.lut();
	register u_int doff = ((off - x) >> 4) + (x >> 2);
	register int len = w * height >> 2;
	if ((w & 0xf) != 0 || (len & 0xf) != 0 || (doff & 3) != 0) {
		register u_short* xip = (u_short*)(pixbuf_ + doff);
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
			y1 = (u_int)(yp[0] & 0xfc) >> 2;
			SPLICE(out, lut[y1 + UV0], 8);
			y1 = (u_int)(yp[4] & 0xfc) >> 2;
			SPLICE(out, lut[y1 + UV2], 0);
			*xip++ = out;
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
			y1 = (u_int)(yp[0] & 0xfc) >> 2;
			SPLICE(out, lut[y1 + UV0], 24);
			y1 = (u_int)(yp[4] & 0xfc) >> 2;
			SPLICE(out, lut[y1 + UV2], 16);
			u = *(const u_int*)(up + 4);
			v = *(const u_int*)(vp + 4);
#if BYTE_ORDER == LITTLE_ENDIAN
			v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
#else
			u = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
#endif
			y1 = (u_int)(yp[8] & 0xfc) >> 2;
			SPLICE(out, lut[y1 + UV0], 8);
			y1 = (u_int)(yp[12] & 0xfc) >> 2;
			SPLICE(out, lut[y1 + UV2], 0);
			*xip++ = out;
			yp += 16;
			up += 8;
			vp += 8;
			w -= 16;
			if (w <= 0) {
				w = width;
				register int pstride = 4 * iw - w;
				register int cstride = pstride >> 1;
				yp += pstride;
				up += cstride;
				vp += cstride;
				xip += (iw - w) >> 4;
			}
		}
	}
}

void QuantWindowRenderer::map_down_422(const u_char* frm,
					u_int off, u_int x,
					u_int width, u_int height) const
{
	/*XXX*/
	map_down4_422(frm, off, x, width, height);
}

void QuantWindowRenderer::map_up2_422(const u_char* frm,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_int* xip = (u_int*)(pixbuf_ + ((off - x) << 2) + (x << 1));
	register int w = width;
	register u_int e1 = yp[0] >> 2;
	register const u_char* lut = cm_.lut();

	for (register int len = width * height; len > 0; len -= 8) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1, e2, out, p;
		register const u_char* y2r;
		register u_int* xip2 = xip + (iw >> 1);

		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		y1 = *(const u_int*)yp;
		out = 0;
		y2r = lut + UV0;
		e2 = PIX0(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 24);
		SPLICE(out, y2r[e2], 16);
		e1 = PIX1(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 8);
		SPLICE(out, y2r[e1], 0);
		*xip++ = out;
		*xip2++ = out;

		out = 0;
		y2r = lut + UV1;
		e2 = PIX2(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 24);
		SPLICE(out, y2r[e2], 16);
		e1 = PIX3(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 8);
		SPLICE(out, y2r[e1], 0);
		*xip++ = out;
		*xip2++ = out;

		yp += 4;
		y1 = *(const u_int*)yp;
		out = 0;
		y2r = lut + UV2;
		e2 = PIX0(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 24);
		SPLICE(out, y2r[e2], 16);
		e1 = PIX1(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 8);
		SPLICE(out, y2r[e1], 0);
		*xip++ = out;
		*xip2++ = out;

		out = 0;
		y2r = lut + UV3;
		e2 = PIX2(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 24);
		SPLICE(out, y2r[e2], 16);
		e1 = PIX3(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 8);
		SPLICE(out, y2r[e1], 0);
		*xip++ = out;
		*xip2++ = out;

		yp += 4;
		up += 4;
		vp += 4;

		w -= 8;
		if (w <= 0) {
			w = width;
			register u_int pstride = iw - w;
			register u_int cstride = pstride >> 1;
			yp += pstride;
			e1 = yp[0] >> 2;
			up += cstride;
			vp += cstride;
			xip += (iw + pstride) >> 1;
		}
	}
}


void QuantWindowRenderer::map_420(const u_char* frm, u_int off,
				  u_int x, u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register u_int* xip = (u_int*)(pixbuf_ + off);
	register int w = width;
	register const u_char* lut = cm_.lut();
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
		y2r = lut + UV0;
		SPLICE(out1, y2r[PIX0(y1) & 0x3f], 24);
		SPLICE(out1, y2r[PIX1(y1) & 0x3f], 16);
		SPLICE(out2, y2r[PIX0(y2) & 0x3f], 24);
		SPLICE(out2, y2r[PIX1(y2) & 0x3f], 16);

		y2r = lut + UV1;
		SPLICE(out1, y2r[PIX2(y1) & 0x3f], 8);
		SPLICE(out1, y2r[PIX3(y1) & 0x3f], 0);
		SPLICE(out2, y2r[PIX2(y2) & 0x3f], 8);
		SPLICE(out2, y2r[PIX3(y2) & 0x3f], 0);

		*xip++ = out1;
		*xip2++ = out2;

		yp += 4;
		y1 = *(const u_int*)yp;
		y2 = *(const u_int*)(yp + iw);
		out1 = 0;
		out2 = 0;
		y2r = lut + UV2;
		SPLICE(out1, y2r[PIX0(y1) & 0x3f], 24);
		SPLICE(out1, y2r[PIX1(y1) & 0x3f], 16);
		SPLICE(out2, y2r[PIX0(y2) & 0x3f], 24);
		SPLICE(out2, y2r[PIX1(y2) & 0x3f], 16);

		y2r = lut + UV3;
		SPLICE(out1, y2r[PIX2(y1) & 0x3f], 8);
		SPLICE(out1, y2r[PIX3(y1) & 0x3f], 0);
		SPLICE(out2, y2r[PIX2(y2) & 0x3f], 8);
		SPLICE(out2, y2r[PIX3(y2) & 0x3f], 0);

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
		}
	}
}

void QuantWindowRenderer::map_down2_420(const u_char* frm,
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
	register const u_char* lut = cm_.lut();
	for (register int len = w * height >> 1; len > 0; len -= 8) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1, out = 0;
		register const u_char* y2r;

		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		y1 = *(const u_int*)yp;
		y2r = lut + UV0;
		SPLICE(out, y2r[PIX0(y1) & 0x3f], 24);

		y2r = lut + UV1; //SV-XXX: was: y2r = y2r = lut + UV1; ???????
		SPLICE(out, y2r[PIX2(y1) & 0x3f], 16);

		y1 = *(const u_int*)(yp + 4);
		y2r = lut + UV2;
		SPLICE(out, y2r[PIX0(y1) & 0x3f], 8);

		y2r = lut + UV3;
		SPLICE(out, y2r[PIX2(y1) & 0x3f], 0);

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
		}
	}
}

void QuantWindowRenderer::map_down4_420(const u_char* frm,
					u_int off, u_int x,
					u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register int w = width;
	register const u_char* lut = cm_.lut();
	register u_int doff = ((off - x) >> 4) + (x >> 2);
	register int len = w * height >> 2;
	if ((w & 0xf) != 0 || (len & 0xf) != 0 || (doff & 3) != 0) {
		register u_short* xip = (u_short*)(pixbuf_ + doff);
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
			y1 = (u_int)(yp[0] & 0xfc) >> 2;
			SPLICE(out, lut[y1 + UV0], 8);
			y1 = (u_int)(yp[4] & 0xfc) >> 2;
			SPLICE(out, lut[y1 + UV2], 0);
			*xip++ = out;
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
			y1 = (u_int)(yp[0] & 0xfc) >> 2;
			SPLICE(out, lut[y1 + UV0], 24);
			y1 = (u_int)(yp[4] & 0xfc) >> 2;
			SPLICE(out, lut[y1 + UV2], 16);
			u = *(const u_int*)(up + 4);
			v = *(const u_int*)(vp + 4);
#if BYTE_ORDER == LITTLE_ENDIAN
			v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
#else
			u = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
#endif
			y1 = (u_int)(yp[8] & 0xfc) >> 2;
			SPLICE(out, lut[y1 + UV0], 8);
			y1 = (u_int)(yp[12] & 0xfc) >> 2;
			SPLICE(out, lut[y1 + UV2], 0);
			*xip++ = out;
			yp += 16;
			up += 8;
			vp += 8;
			w -= 16;
			if (w <= 0) {
				w = width;
				register int pstride = 4 * iw - w;
				register int cstride = iw - (w >> 1);
				yp += pstride;
				up += cstride;
				vp += cstride;
				xip += (iw - w) >> 4;
			}
		}
	}
}

void QuantWindowRenderer::map_down_420(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	/*XXX*/
	map_down4_420(frm, off, x, width, height);
}

void QuantWindowRenderer::map_up2_420(const u_char* frm,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register u_int* xip = (u_int*)(pixbuf_ + ((off - x) << 2) + (x << 1));
	register int w = width;
	register const u_char* lut = cm_.lut();
	register u_int e1 = (yp[0] & 0xfc) >> 2, o1 = (yp[iw] & 0xfc) >> 2;
	for (register int len = width * height; len > 0; len -= 16) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1, y2, e2, o2, out, p;
		register const u_char* y2r;
		register u_int* xip2 = xip + (iw >> 1);

		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		/* double pixels 0 & 1 of the `even' row */
		y1 = *(const u_int*)yp;
		out = 0;
		y2r = lut + UV0;
		e2 = PIX0(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 24);
		SPLICE(out, y2r[e2], 16);
		e1 = PIX1(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 8);
		SPLICE(out, y2r[e1], 0);
		*xip = out;
		*xip2 = out;

		/* double pixels 0 & 1 of the `odd' row */
		y2 = *(const u_int*)(yp + iw);
		out = 0;
		o2 = PIX0(y1) & 0x3f;
		p = ((o1 + o2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 24);
		SPLICE(out, y2r[o2], 16);
		o1 = PIX1(y1) & 0x3f;
		p = ((o1 + o2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 8);
		SPLICE(out, y2r[o1], 0);
		xip[iw] = out;
		xip2[iw] = out;
		++xip;
		++xip2;

		/* double pixels 2 & 3 of the `even' row */
		out = 0;
		y2r = lut + UV1;
		e2 = PIX2(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 24);
		SPLICE(out, y2r[e2], 16);
		e1 = PIX3(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 8);
		SPLICE(out, y2r[e1], 0);
		*xip = out;
		*xip2 = out;

		/* double pixels 2 & 3 of the `odd' row */
		out = 0;
		o2 = PIX2(y1) & 0x3f;
		p = ((o1 + o2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 24);
		SPLICE(out, y2r[o2], 16);
		o1 = PIX3(y1) & 0x3f;
		p = ((o1 + o2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 8);
		SPLICE(out, y2r[o1], 0);
		xip[iw] = out;
		xip2[iw] = out;
		++xip;
		++xip2;

		/* double pixels 4 & 5 of the `even' row */
		yp += 4;
		y1 = *(const u_int*)yp;
		out = 0;
		y2r = lut + UV2;
		e2 = PIX0(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 24);
		SPLICE(out, y2r[e2], 16);
		e1 = PIX1(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 8);
		SPLICE(out, y2r[e1], 0);
		*xip = out;
		*xip2 = out;

		/* double pixels 4 & 5 of the `odd' row */
		y2 = *(const u_int*)(yp + iw);
		out = 0;
		o2 = PIX0(y1) & 0x3f;
		p = ((o1 + o2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 24);
		SPLICE(out, y2r[o2], 16);
		o1 = PIX1(y1) & 0x3f;
		p = ((o1 + o2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 8);
		SPLICE(out, y2r[o1], 0);
		xip[iw] = out;
		xip2[iw] = out;
		++xip;
		++xip2;

		/* double pixels 6 & 7 of the `even' row */
		out = 0;
		y2r = lut + UV3;
		e2 = PIX2(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 24);
		SPLICE(out, y2r[e2], 16);
		e1 = PIX3(y1) & 0x3f;
		p = ((e1 + e2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 8);
		SPLICE(out, y2r[e1], 0);
		*xip = out;
		*xip2 = out;

		/* double pixels 6 & 7 of the `odd' row */
		out = 0;
		o2 = PIX2(y1) & 0x3f;
		p = ((o1 + o2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 24);
		SPLICE(out, y2r[o2], 16);
		o1 = PIX3(y1) & 0x3f;
		p = ((o1 + o2) >> 1) & 0x3f;
		SPLICE(out, y2r[p], 8);
		SPLICE(out, y2r[o1], 0);
		xip[iw] = out;
		xip2[iw] = out;
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
			e1 = (yp[0] & 0xfc) >> 2;
			o1 = (yp[iw] & 0xfc) >> 2;
			up += cstride;
			vp += cstride;
			xip += (8 * iw - 2 * w) >> 2;
		}
	}
}
