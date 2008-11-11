/*
 * Copyright (c) 1993-1996 Regents of the University of California.
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

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
//#   include <X11/Xlib.h>
//#   include <X11/Xutil.h>
#endif
#include "bsd-endian.h"
#include "color.h"
#include "renderer-window.h"
#include "inet.h"
#include "tcl.h"
#include "vw.h"

#if defined(WIN32) && 0
typedef RGBTRIPLE* RGBPointer;
#else
typedef u_int* RGBPointer;
#endif
#define swapbyte32(x) \
        ((u_int)( \
                (((u_int)(x) & (u_int)0x000000ffUL) << 24) | \
                (((u_int)(x) & (u_int)0x0000ff00UL) <<  8) | \
                (((u_int)(x) & (u_int)0x00ff0000UL) >>  8) | \
                (((u_int)(x) & (u_int)0xff000000UL) >> 24) ))


class TrueColorModel : public ColorModel {
public:
	~TrueColorModel();
	WindowRenderer* allocate(VideoWindow* vw, int decimation);
	virtual int command(int argc, const char*const* argv);
	virtual int alloc_colors();
	virtual int alloc_grays();
	inline u_int omask() const { return (omask_); }
	inline u_int pmask() const { return (pmask_); }
	inline const u_int* uvtab() const { return (&uvtab_[0]); }
protected:
	u_int omask_;
	u_int pmask_;
	u_int uvtab_[65536];
};

class TrueColorMatcher : public Matcher {
public:
	TrueColorMatcher() : Matcher("colormodel") {}
	virtual TclObject* match(const char* id) {
		if (strcasecmp(id, "truecolor/24") == 0 ||
		    strcasecmp(id, "truecolor/32") == 0)
			return (new TrueColorModel());
		return (0);
	}
} matcher_true;


TrueColorModel::~TrueColorModel()
{
	/*XXX*/
}

static int
mtos(int mask)
{
	int shift = 0;
	if (mask) {
		while ((mask & 1) == 0) {
			mask >>= 1;
			++shift;
		}
	}
	return (shift);
}

int TrueColorModel::alloc_grays()
{
	return (0);
}

int TrueColorModel::alloc_colors()
{
#if WIN32
	u_int bmask = visual_->red_mask;
	u_int gmask = visual_->green_mask;
	u_int rmask = visual_->blue_mask;
#else
	u_int rmask = visual_->red_mask;
	u_int gmask = visual_->green_mask;
	u_int bmask = visual_->blue_mask;
#endif
	/* XXX
	 * we would expect the masks we get back from the server to
	 * reflect the byte position of the color *from the server's
	 * point of view* but, for a dec mips or alpha talking to
	 * an sgi, the masks appear to have been byte-swapped to host
	 * int form.  This is either an sgi bug, a dec bug or an
	 * X protocol bug.  For right now, we put the masks in correct
	 * order when a little-endian machine talks to a 24-bit
	 * big-endian display.
	 * Note that if this is an X protocol bug, there should be
	 * an equivalent operation for big-to-little that byte swaps
	 * the masks but, unfortunately, htonl() on a big-endian won't
	 * do the job & there's no standard library that does.  For
	 * now we ignore this problem.
	 */
//printf("rmask: %d(%d), gmask: %d(%d), bmask: %d(%d);\n",rmask,htonl(rmask),gmask,htonl(gmask),bmask,htonl(bmask));
//|| visual_->bits_per_rgb==32
#if BYTE_ORDER == LITTLE_ENDIAN
// OSX TK doesn't set the dpy_ byteorder properly
#ifndef MAC_OSX_TK
	if (ImageByteOrder(dpy_) == MSBFirst ) {
		rmask = htonl(rmask);
		gmask = htonl(gmask);
		bmask = htonl(bmask);
	}
#endif

#else
#ifdef MAC_OSX_TK
		rmask = htonl(rmask);
		gmask = htonl(gmask);
		bmask = htonl(bmask);
#else
	if (ImageByteOrder(dpy_) != MSBFirst) {
		rmask = swapbyte32(rmask);
		gmask = swapbyte32(gmask);
		bmask = swapbyte32(bmask);
	}
#endif
#endif
	u_int rshft = mtos(rmask);
	u_int rlose = 8 - mtos(~(rmask >> rshft));
	u_int gshft = mtos(gmask);
	u_int glose = 8 - mtos(~(gmask >> gshft));
	u_int bshft = mtos(bmask);
	u_int blose = 8 - mtos(~(bmask >> bshft));

	omask_ = 0x80 >> rlose << rshft;
	omask_ |= 0x80 >> glose << gshft;
	omask_ |= 0x80 >> blose << bshft;

	pmask_ = 0xff >> rlose << rshft;
	pmask_ |= 0xff >> glose << gshft;
	pmask_ |= 0xff >> blose << bshft;

	for (int u = 0; u < 256; ++u) {
		register double uf = double(u - 128);
		for (int v = 0; v < 256; ++v) {
			register double vf = double(v - 128);
			int r = int(vf * 1.402);
			r = (r < -128)? -128 : (r > 127)? 127 : r;
			int b = int(uf * 1.772);
			b = (b < -128)? -128 : (b > 127)? 127 : b;
			int g = int(uf * -0.34414 - vf * 0.71414);
			g = (g < -128)? -128 : (g > 127)? 127 : g;
			uvtab_[(u << 8)|v] =
				(r & 0xff) >> rlose << rshft |
				(g & 0xff) >> glose << gshft |
				(b & 0xff) >> blose << bshft;
		}
	}
	return (0);
}

class TrueWindowRenderer : public WindowDitherer {
public:
	TrueWindowRenderer(VideoWindow* vw, int decimation, TrueColorModel& cm)
		: WindowDitherer(vw, decimation), cm_(cm) { }
	virtual void render(const u_char* frm, int off, int x, int w, int h) = 0;
protected:
	TrueColorModel& cm_;
	virtual void update() = 0;
	virtual void disable() = 0;
};

class TrueWindowRenderer24;
typedef void (TrueWindowRenderer24::*True24Method)(const u_char*, u_int,
					   u_int, u_int, u_int) const;

class TrueWindowRenderer24 : public TrueWindowRenderer {
public:
    TrueWindowRenderer24(VideoWindow* vw, int decimation, TrueColorModel& cm)
	: TrueWindowRenderer(vw, decimation, cm) {}
    virtual void render(const u_char* frm, int off, int x, int w, int h) {
	(this->*method_)(frm, off, x, w, h);
    }
protected:
	virtual void update();
	virtual void disable() { method_ = True24Method(&WindowRenderer::dither_null); }
	True24Method method_;
    
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
	void map_gray(const u_char* frm, u_int off, u_int x,
			      u_int width, u_int height) const;
	void map_gray_down2(const u_char* frm, u_int off, u_int x,
				    u_int width, u_int height) const;
	void map_gray_down4(const u_char* frm, u_int off, u_int x,
				    u_int width, u_int height) const;
	void map_gray_down(const u_char* frm, u_int off, u_int x,
				   u_int width, u_int height) const;
	void map_gray_up2(const u_char* frm, u_int off, u_int x,
				  u_int width, u_int height) const;
};

class TrueWindowRenderer32;
typedef void (TrueWindowRenderer32::*True32Method)(const u_char*, u_int,
					   u_int, u_int, u_int) const;

class TrueWindowRenderer32 : public TrueWindowRenderer {
public:
    TrueWindowRenderer32(VideoWindow* vw, int decimation, TrueColorModel& cm)
	: TrueWindowRenderer(vw, decimation, cm) {}
    virtual void render(const u_char* frm, int off, int x, int w, int h) {
	(this->*method_)(frm, off, x, w, h);
    }
protected:
	virtual void update();
	virtual void disable() { method_ = True32Method(&WindowRenderer::dither_null); }
	True32Method method_;

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
	void map_gray(const u_char* frm, u_int off, u_int x,
			      u_int width, u_int height) const;
	void map_gray_down2(const u_char* frm, u_int off, u_int x,
				    u_int width, u_int height) const;
	void map_gray_down4(const u_char* frm, u_int off, u_int x,
				    u_int width, u_int height) const;
	void map_gray_down(const u_char* frm, u_int off, u_int x,
				   u_int width, u_int height) const;
	void map_gray_up2(const u_char* frm, u_int off, u_int x,
				  u_int width, u_int height) const;
};

int TrueColorModel::command(int argc, const char*const* argv)
{
	if (argc == 4 && strcmp(argv[1], "renderer") == 0) {
		Tcl& tcl = Tcl::instance();
		VideoWindow* vw = VideoWindow::lookup(argv[2]);
		int decimation = atoi(argv[3]);
		if (vw->bpp() == 24) {
			Renderer* r = new TrueWindowRenderer24(vw, decimation, *this);
			tcl.result(r->name());
		}
		if (vw->bpp() == 32) {
			Renderer* r = new TrueWindowRenderer32(vw, decimation, *this);
			tcl.result(r->name());
		}
		return (TCL_OK);
	}
	return (ColorModel::command(argc, argv));
}

void TrueWindowRenderer24::update()
{
	static True24Method methods[] = {
	    &TrueWindowRenderer24::map_up2_420,
	    &TrueWindowRenderer24::map_up2_422,
	    &TrueWindowRenderer24::map_gray_up2,
	    &TrueWindowRenderer24::map_gray_up2,
	    &TrueWindowRenderer24::map_420,
	    &TrueWindowRenderer24::map_422,
	    &TrueWindowRenderer24::map_gray,
	    &TrueWindowRenderer24::map_gray,
	    &TrueWindowRenderer24::map_down2_420,
	    &TrueWindowRenderer24::map_down2_422,
	    &TrueWindowRenderer24::map_gray_down2,
	    &TrueWindowRenderer24::map_gray_down2,
	    &TrueWindowRenderer24::map_down4_420,
	    &TrueWindowRenderer24::map_down4_422,
	    &TrueWindowRenderer24::map_gray_down4,
	    &TrueWindowRenderer24::map_gray_down4,
	    &TrueWindowRenderer24::map_down_420,
	    &TrueWindowRenderer24::map_down_422,
	    &TrueWindowRenderer24::map_gray_down,
	    &TrueWindowRenderer24::map_gray_down,
	};
	method_ = methods[index()];
}

void TrueWindowRenderer32::update()
{
	static True32Method methods[] = {
	    &TrueWindowRenderer32::map_up2_420,
	    &TrueWindowRenderer32::map_up2_422,
	    &TrueWindowRenderer32::map_gray_up2,
	    &TrueWindowRenderer32::map_gray_up2,
	    &TrueWindowRenderer32::map_420,
	    &TrueWindowRenderer32::map_422,
	    &TrueWindowRenderer32::map_gray,
	    &TrueWindowRenderer32::map_gray,
	    &TrueWindowRenderer32::map_down2_420,
	    &TrueWindowRenderer32::map_down2_422,
	    &TrueWindowRenderer32::map_gray_down2,
	    &TrueWindowRenderer32::map_gray_down2,
	    &TrueWindowRenderer32::map_down4_420,
	    &TrueWindowRenderer32::map_down4_422,
	    &TrueWindowRenderer32::map_gray_down4,
	    &TrueWindowRenderer32::map_gray_down4,
	    &TrueWindowRenderer32::map_down_420,
	    &TrueWindowRenderer32::map_down_422,
	    &TrueWindowRenderer32::map_gray_down,
	    &TrueWindowRenderer32::map_gray_down,
	};
	method_ = methods[index()];
}

#if BYTE_ORDER == LITTLE_ENDIAN
#define SHIFT_0		24
#define SHIFT_8		16
#define SHIFT_16	8
#define SHIFT_24	0
#define UV0 ((v << 5) & 0x3ff00)
#define UV1 ((u << 2) & 0x3ff00)
#define UV2 ((v >> 11) & 0x3ff00)
#define UV3 ((u >> 14) & 0x3ff00)
#else
#define SHIFT_0		0
#define SHIFT_8		8
#define SHIFT_16	16
#define SHIFT_24	24
#define UV0 ((u >> 12) & 0xfff00)
#define UV1 ((v >> 10) & 0xfff00)
#define UV2 ((u << 4) & 0xfff00)
#define UV3 ((v << 6) & 0xfff00)
#endif

/*
 * This routine sums the luma & chroma components of one pixel &
 * constructs an rgb output.  It does all three r g b components
 * in parallel.  The one complication is that it has to
 * deal with overflow (sum > 255) and underflow (sum < 0).  Underflow
 * & overflow are only possible if both terms have the same sign and
 * are indicated by the result having a different sign than the terms.
 * Note that we ignore the carry into the next byte's lsb that happens
 * on an overflow/underflow on the grounds that it's probably invisible.
 * The luma term and sum are biased by 128 so a negative number has the
 * 2^7 bit = 0.  The chroma term is not biased so a negative number has
 * the 2^7 bit = 1.  So underflow is indicated by (L & C & sum) != 0;
 */
#if defined(WIN32) && 0

#define ONERGB(dst, rgb) \
	(dst).rgbtBlue = GetBValue(rgb); \
	(dst).rgbtGreen = GetGValue(rgb); \
 	(dst).rgbtRed = GetRValue(rgb);


#define ONEGRAY(dst, pix) \
	(dst).rgbtBlue = (dst).rgbtGreen = (dst).rgbtRed = (pix & 0xff);


#else /* !WIN32 */

#define ONERGB(dst, rgb) \
	dst = rgb;

#define ONEGRAY(dst, pix) \
	(dst) =  (pix << 16) | (pix << 8) | pix;

#endif /* !WIN32 */

#define ONEPIX(src, dst) { \
	l = src; \
	l |= l << 8; l |= l << 16; \
	sum = l + uv; \
	uflo = (l ^ uv) & (l ^ sum) & omask; \
	if (uflo) { \
		if ((l = uflo & l) != 0) { \
			/* saturate overflow(s) */ \
			l |= l >> 1; \
			l |= l >> 2; \
			l |= l >> 4; \
			sum |= l; \
			uflo &=~ l; \
		} \
		if (uflo != 0) { \
			/* zero underflow(s) */ \
			uflo |= uflo >> 1; \
			uflo |= uflo >> 2; \
			uflo |= uflo >> 4; \
			sum &=~ uflo; \
		} \
	} \
	ONERGB(dst, sum & pmask); \
}

void TrueWindowRenderer32::map_422(const u_char* frm, u_int off,
				 u_int x, u_int width, u_int height) const
{
	UNUSED(x); //SV-XXX: unused

	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register RGBPointer xip = (RGBPointer)pixbuf_ + off;
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height; len > 0; len -= 8) {
		register u_int l, uv;
		register u_int uflo, sum;

#define TWO422(n) \
		uv = yuv2rgb[(up[(n)/2] << 8) | vp[(n)/2]]; \
		ONEPIX(yp[(n)], xip[(n)]) \
		ONEPIX(yp[(n)+1], xip[(n)+1])

		TWO422(0)
		TWO422(2)
		TWO422(4)
		TWO422(6)

		xip += 8;
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
			xip += pstride;
		}
	}
}

void TrueWindowRenderer32::map_down2_422(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register RGBPointer xip = (RGBPointer)pixbuf_ + ((off - x) >> 2) + (x >> 1);
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height >> 1; len > 0; len -= 8) {
		register u_int l, uv;
		register u_int uflo, sum;

#define ONE422(n) \
		uv = yuv2rgb[(up[(n)/2] << 8) | vp[(n)/2]]; \
		ONEPIX(yp[(n)], xip[(n)/2])

		ONE422(0)
		ONE422(2)
		ONE422(4)
		ONE422(6)

		xip += 4;
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
			xip += (iw - w) >> 1;
		}
	}
}


void TrueWindowRenderer32::map_down4_422(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register RGBPointer xip = (RGBPointer)pixbuf_ + ((off - x) >> 4) + (x >> 2);
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height >> 2; len > 0; len -= 8) {
		register u_int l, uv;
		register u_int uflo, sum;

		uv = yuv2rgb[(up[0] << 8) | vp[0]];
		ONEPIX(yp[0], xip[0])
		uv = yuv2rgb[(up[2] << 8) | vp[2]];
		ONEPIX(yp[4], xip[1])

		xip += 2;
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
			xip += (iw - w) >> 2;
		}
	}
}

/*
 * decimate by some power of 2 >= 2^3.
 */
void TrueWindowRenderer32::map_down_422(const u_char* frm,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register int s = scale_;
	register int istride = 1 << s;
	register RGBPointer xip = (RGBPointer)pixbuf_ +
		((off - x) >> (s + s)) + (x >> s);
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height >> s; len > 0; len -= istride) {
		register u_int l, uv;
		register u_int uflo, sum;

		uv = yuv2rgb[(up[0] << 8) | vp[0]];
		ONEPIX(yp[0], xip[0])

		xip += 1;
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
		}
	}
}

void TrueWindowRenderer32::map_up2_422(const u_char* frm,
				     u_int off, u_int x,
				     u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register RGBPointer xip = (RGBPointer)pixbuf_ + ((off - x) << 2) + (x << 1);
	register int w = width;
	register u_int e1 = yp[0];
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height; len > 0; len -= 2) {
		register u_int l, uv;
		register u_int uflo, sum;
		register u_int e2;
		register RGBPointer xip2 = xip + (iw << 1);

		uv = yuv2rgb[(up[0] << 8) | vp[0]];
		e2 = yp[0];
		ONEPIX((e1 + e2) >> 1, xip[0])
		ONERGB(xip2[0], sum);
		ONEPIX(e2, xip[1])
		ONERGB(xip2[1], sum);
		e1 = yp[1];
		ONEPIX((e1 + e2) >> 1, xip[2])
		ONERGB(xip2[2], sum);
		ONEPIX(e1, xip[3])
		ONERGB(xip2[3], sum);

		xip += 4;
		yp += 2;
		up += 1;
		vp += 1;

		w -= 2;
		if (w <= 0) {
			w = width;
			register u_int pstride = iw - w;
			register u_int cstride = pstride >> 1;
			yp += pstride;
			e1 = yp[0];
			up += cstride;
			vp += cstride;
			xip += (iw + pstride) << 1;
		}
	}
}

void TrueWindowRenderer32::map_420(const u_char* frm, u_int off,
				 u_int x, u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register RGBPointer xip = (RGBPointer)pixbuf_ + off;
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height; len > 0; len -= 8) {
		register u_int l, uv;
		register u_int uflo, sum;
		register RGBPointer xip2 = xip + iw;
		register const u_char* yp2 = yp + iw;

#define FOUR420(n) \
		uv = yuv2rgb[(up[(n)/2] << 8) | vp[(n)/2]]; \
		ONEPIX(yp[(n)], xip[(n)]) \
		ONEPIX(yp[(n)+1], xip[(n)+1]) \
		ONEPIX(yp2[(n)], xip2[(n)]) \
		ONEPIX(yp2[(n)+1], xip2[(n)+1])

		FOUR420(0)
		FOUR420(2)

		xip += 4;
		yp += 4;
		up += 2;
		vp += 2;

		w -= 4;
		if (w <= 0) {
			w = width;
			register int pstride = 2 * iw - w;
			register int cstride = (iw - w) >> 1;
			yp += pstride;
			up += cstride;
			vp += cstride;
			xip += pstride;
		}
	}
}

#ifdef notdef
void TrueWindowRenderer32::map_420_16(const u_char* frm, u_int off,
				    u_int x, u_int width, u_int height) const
{
#ifdef FIX_THIS
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	/*XXX use u_int here*/
	register u_short* xip = (u_short*)pixbuf_ + off;
	register int w = width;
	register const u_char* yuv2rgb = (u_char*)cm_.yuv2rgb();
	for (register int len = width * height; len > 0; len -= 16) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1, y2;
		register const u_char* y2r;

		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		y1 = *(const u_int*)yp;
		y2r = yuv2rgb + UV0;
		xip[0] =  *(u_int*)(y2r + ((y1 >> SHIFT_24) & 0xfc));
		xip[1] =  *(u_int*)(y2r + ((y1 >> SHIFT_16) & 0xfc));
		y2 = *(const u_int*)(yp + iw);
		register u_short* xip2 = xip + iw;
		xip2[0] = *(u_int*)(y2r + ((y2 >> SHIFT_24) & 0xfc));
		xip2[1] = *(u_int*)(y2r + ((y2 >> SHIFT_16) & 0xfc));

		y2r = yuv2rgb + UV1;
		xip[2] =  *(u_int*)(y2r + ((y1 >> SHIFT_8) & 0xfc));
		xip[3] =  *(u_int*)(y2r + ((y1 >> SHIFT_0) & 0xfc));
		xip2[2] = *(u_int*)(y2r + ((y2 >> SHIFT_8) & 0xfc));
		xip2[3] = *(u_int*)(y2r + ((y2 >> SHIFT_0) & 0xfc));

		y1 = *(const u_int*)(yp + 4);
		y2r = yuv2rgb + UV2;
		xip[4] =  *(u_int*)(y2r + ((y1 >> SHIFT_24) & 0xfc));
		xip[5] =  *(u_int*)(y2r + ((y1 >> SHIFT_16) & 0xfc));
		y2 = *(const u_int*)(yp + iw + 4);
		xip2[4] = *(u_int*)(y2r + ((y2 >> SHIFT_24) & 0xfc));
		xip2[5] = *(u_int*)(y2r + ((y2 >> SHIFT_16) & 0xfc));

		y2r = yuv2rgb + UV3;
		xip[6] =  *(u_int*)(y2r + ((y1 >> SHIFT_8) & 0xfc));
		xip[7] =  *(u_int*)(y2r + ((y1 >> SHIFT_0) & 0xfc));
		xip2[6] = *(u_int*)(y2r + ((y2 >> SHIFT_8) & 0xfc));
		xip2[7] = *(u_int*)(y2r + ((y2 >> SHIFT_0) & 0xfc));

		xip += 8;
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
			xip += pstride;
		}
	}
#endif
}
#endif

void TrueWindowRenderer32::map_down2_420(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	off = ((off - x) >> 2) + (x >> 1);
	register const u_char* up = frm + framesize_ + off;
	register const u_char* vp = up + (framesize_ >> 2);
	register RGBPointer xip = (RGBPointer)pixbuf_ + off;
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height >> 1; len > 0; len -= 8) {
		register u_int l, uv;
		register u_int uflo, sum;

#define ONE420(n) \
		uv = yuv2rgb[(up[(n)/2] << 8) | vp[(n)/2]]; \
		ONEPIX(yp[(n)], xip[(n)/2])

		ONE420(0)
		ONE420(2)
		ONE420(4)
		ONE420(6)

		xip += 4;
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
			xip += cstride;
		}
	}
}

void TrueWindowRenderer32::map_down4_420(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register RGBPointer xip = (RGBPointer)pixbuf_ + ((off - x) >> 4) + (x >> 2);
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height >> 2; len > 0; len -= 8) {
		register u_int l, uv;
		register u_int uflo, sum;

		uv = yuv2rgb[(up[0] << 8) | vp[0]];
		ONEPIX(yp[0], xip[0])
		uv = yuv2rgb[(up[2] << 8) | vp[2]];
		ONEPIX(yp[4], xip[1])

		xip += 2;
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
			xip += (iw - w) >> 2;
		}
	}
}

/*
 * decimate by some power of 2 >= 2^3.
 */
void TrueWindowRenderer32::map_down_420(const u_char* frm,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register int s = scale_;
	register int istride = 1 << s;
	register RGBPointer xip = (RGBPointer)pixbuf_
		+ ((off - x) >> (s + s)) + (x >> s);
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height >> s; len > 0; len -= istride) {
		register u_int l, uv;
		register u_int uflo, sum;

		uv = yuv2rgb[(up[0] << 8) | vp[0]];
		ONEPIX(yp[0], xip[0])

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
		}
	}
}

void TrueWindowRenderer32::map_up2_420(const u_char* frm,
				     u_int off, u_int x,
				     u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register RGBPointer xip = (RGBPointer)pixbuf_ + ((off - x) << 2) + (x << 1);
	register int w = width;
	register u_int e1 = yp[0], o1 = yp[iw];
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height; len > 0; len -= 4) {
		register u_int l, uv;
		register u_int uflo, sum;
		register u_int e2, o2;
		register const u_char* yp2 = yp + iw;
		register RGBPointer xip2 = xip + (iw << 1);
		register RGBPointer xip3 = xip2 + (iw << 1);
		register RGBPointer xip4 = xip3 + (iw << 1);

		uv = yuv2rgb[(up[0] << 8) | vp[0]];
		e2 = yp[0];
		ONEPIX((e1 + e2) >> 1, xip[0])
		ONERGB(xip2[0], sum);
		ONEPIX(e2, xip[1])
		ONERGB(xip2[1], sum);
		e1 = yp[1];
		ONEPIX((e1 + e2) >> 1, xip[2])
		ONERGB(xip2[2], sum);
		ONEPIX(e1, xip[3])
		ONERGB(xip2[3], sum);

		o2 = yp2[0];
		ONEPIX((o1 + o2) >> 1, xip3[0])
		ONERGB(xip4[0], sum);
		ONEPIX(o2, xip3[1])
		ONERGB(xip4[1], sum);
		o1 = yp2[1];
		ONEPIX((o1 + o2) >> 1, xip3[2])
		ONERGB(xip4[2], sum);
		ONEPIX(o1, xip3[3])
		ONERGB(xip4[3], sum);


		xip += 4;
		yp += 2;
		up += 1;
		vp += 1;

		w -= 2;
		if (w <= 0) {
			w = width;
			register u_int pstride = 2 * iw - w;
			register u_int cstride = (iw - w) >> 1;
			yp += pstride;
			e1 = yp[0];
			o1 = yp[iw];
			up += cstride;
			vp += cstride;
			xip += 8 * iw - 2 * w;
		}
	}
}

void TrueWindowRenderer32::map_gray(register const u_char *yp,
				  u_int off, u_int x,
				  u_int width, u_int height) const
{
        UNUSED(x); //SV-XXX: unused

	register u_int iw = width_;
	yp += off;
	register RGBPointer xip = (RGBPointer)pixbuf_ + off;
	register int w = width;
	for (register int len = w * height; len > 0; len -= 8) {
		register u_int y1;
		register u_int pix;

		y1 = *(const u_int*)yp;
		pix = (y1 >> SHIFT_24) & 0xff;
		ONEGRAY(xip[0], pix);
		pix = (y1 >> SHIFT_16) & 0xff;
		ONEGRAY(xip[1], pix);
		pix = (y1 >> SHIFT_8) & 0xff;
		ONEGRAY(xip[2], pix);
		pix = (y1 >> SHIFT_0) & 0xff;
		ONEGRAY(xip[3], pix);

		y1 = *(const u_int*)(yp + 4);
		pix = (y1 >> SHIFT_24) & 0xff;
		ONEGRAY(xip[4], pix);
		pix = (y1 >> SHIFT_16) & 0xff;
		ONEGRAY(xip[5], pix);
		pix = (y1 >> SHIFT_8) & 0xff;
		ONEGRAY(xip[6], pix);
		pix = (y1 >> SHIFT_0) & 0xff;
		ONEGRAY(xip[7], pix);

		xip += 8;
		yp += 8;

		w -= 8;
		if (w <= 0) {
			w = width;
			register u_int pstride = iw - w;
			yp += pstride;
			xip += pstride;
		}
	}
}

void TrueWindowRenderer32::map_gray_down2(register const u_char *yp,
					u_int off, u_int x,
					u_int width, u_int height) const
{
	register u_int iw = width_;
	yp += off;
	off = ((off - x) >> 2) + (x >> 1);
	register RGBPointer xip = (RGBPointer)pixbuf_ + off;
	register int w = width;
	for (register int len = w * height >> 1; len > 0; len -= 8) {
		register u_int y1;
		register u_int pix;

		y1 = *(const u_int*)yp;
		pix = (y1 >> SHIFT_24) & 0xff;
		ONEGRAY(xip[0], pix);
		pix = (y1 >> SHIFT_8) & 0xff;
		ONEGRAY(xip[1], pix);

		y1 = *(const u_int*)(yp + 4);
		pix = (y1 >> SHIFT_24) & 0xff;
		ONEGRAY(xip[2], pix);
		pix = (y1 >> SHIFT_8) & 0xff;
		ONEGRAY(xip[3], pix);

		xip += 4;
		yp += 8;

		w -= 8;
		if (w <= 0) {
			w = width;
			register int pstride = 2 * iw - w;
			yp += pstride;
			xip += (iw - w) >> 1;
		}
	}
}

void TrueWindowRenderer32::map_gray_down4(register const u_char *yp,
					u_int off, u_int x,
					u_int width, u_int height) const
{
	register u_int iw = width_;
	yp += off;
	register RGBPointer xip = (RGBPointer)pixbuf_ + ((off - x) >> 4) + (x >> 2);
	register int w = width;
	for (register int len = w * height >> 2; len > 0; len -= 8) {
		register u_int pix;

		pix = yp[0];
		ONEGRAY(xip[0], pix);
		pix = yp[4];
		ONEGRAY(xip[1], pix);

		xip += 2;
		yp += 8;

		w -= 8;
		if (w <= 0) {
			w = width;
			register int pstride = 4 * iw - w;
			yp += pstride;
			xip += (iw - w) >> 2;
		}
	}
}

void TrueWindowRenderer32::map_gray_down(register const u_char *yp,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	yp += off;
	register int s = scale_;
	register int istride = 1 << s;
	register RGBPointer xip = (RGBPointer)pixbuf_ +
		((off - x) >> (s + s)) + (x >> s);
	register int w = width;
	for (register int len = w * height >> s; len > 0; len -= istride) {
		register u_int pix = *yp;
		ONEGRAY(xip[0], pix);
		xip++;
		yp += istride;
		w -= istride;
		if (w <= 0) {
			w = width;
			register int pstride = (iw << s) - w;
			yp += pstride;
			xip += (iw - w) >> s;
		}
	}
}

void TrueWindowRenderer32::map_gray_up2(register const u_char *yp,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
	register u_int iw = width_;
	yp += off;
	register RGBPointer xip = (RGBPointer)pixbuf_ + ((off - x) << 2) + (x << 1);
	register int w = width;
	register u_int e1 = yp[0];

	for (register int len = width * height; len > 0; len -= 8) {
		register u_int y1, e2, pix;
		register RGBPointer xip2 = xip + iw * 2;

		y1 = *(const u_int*)yp;
		e2 = (y1 >> SHIFT_24) & 0xff;
		pix = (e1 + e2) >> 1;
		ONEGRAY(xip[0], pix);
		ONEGRAY(xip2[0], pix);
		ONEGRAY(xip[1], e2);
		ONEGRAY(xip2[1], e2);
		e1 = (y1 >> SHIFT_16) & 0xff;
		pix = (e1 + e2) >> 1;
		ONEGRAY(xip[2], pix);
		ONEGRAY(xip2[2], pix);
		ONEGRAY(xip[3], e1);
		ONEGRAY(xip2[3], e1);

		e2 = (y1 >> SHIFT_8) & 0xff;
		pix = (e1 + e2) >> 1;
		ONEGRAY(xip[4], pix);
		ONEGRAY(xip2[4], pix);
		ONEGRAY(xip[5], e2);
		ONEGRAY(xip2[5], e2);
		e1 = (y1 >> SHIFT_0) & 0xff;
		pix = (e1 + e2) >> 1;
		ONEGRAY(xip[6], pix);
		ONEGRAY(xip2[6], pix);
		ONEGRAY(xip[7], e1);
		ONEGRAY(xip2[7], e1);

		y1 = *(const u_int*)(yp + 4);
		e2 = (y1 >> SHIFT_24) & 0xff;
		pix = (e1 + e2) >> 1;
		ONEGRAY(xip[8], pix);
		ONEGRAY(xip2[8], pix);
		ONEGRAY(xip[9], e2);
		ONEGRAY(xip2[9], e2);
		e1 = (y1 >> SHIFT_16) & 0xff;
		pix = (e1 + e2) >> 1;
		ONEGRAY(xip[10], pix);
		ONEGRAY(xip2[10], pix);
		ONEGRAY(xip[11], e1);
		ONEGRAY(xip2[11], e1);

		e2 = (y1 >> SHIFT_8) & 0xff;
		pix = (e1 + e2) >> 1;
		ONEGRAY(xip[12], pix);
		ONEGRAY(xip2[12], pix);
		ONEGRAY(xip[13], e2);
		ONEGRAY(xip2[13], e2);
		e1 = (y1 >> SHIFT_0) & 0xff;
		pix = (e1 + e2) >> 1;
		ONEGRAY(xip[14], pix);
		ONEGRAY(xip2[14], pix);
		ONEGRAY(xip[15], e1);
		ONEGRAY(xip2[15], e1);

		xip += 16;
		yp += 8;

		w -= 8;
		if (w <= 0) {
			w = width;
			register u_int pstride = iw - w;
			yp += pstride;
			e1 = yp[0];
			xip += (iw + pstride) << 1;
		}
	}
}

// Dithers for 24 bpp displays
// XXX might be possible to replace char* by u_int* somehow
//     or to do other optimizations

#if  (BYTE_ORDER == LITTLE_ENDIAN)
#define PONERGB(dst, rgb)						\
    (&(dst))[0] = (rgb);						\
    (&(dst))[1] = (rgb)>>8;						\
    (&(dst))[2] = (rgb)>>16;

#else
#define PONERGB(dst, rgb)						\
    (&(dst))[0] = (rgb)>>16;						\
    (&(dst))[1] = (rgb)>>8;						\
    (&(dst))[2] = (rgb);
#endif

#define PONEGRAY(dst, pix)						\
    (&(dst))[0] = pix;							\
    (&(dst))[1] = pix;							\
    (&(dst))[2] = pix;

#define PONEPIX(src,dst) {						\
    l = src;								\
    l |= l << 8; l |= l << 16;						\
    sum = l + uv;							\
    uflo = (l ^ uv) & (l ^ sum) & omask;				\
    if (uflo) {								\
	if ((l = uflo & l) != 0) {					\
	    /* saturate overflow(s) */					\
	    l |= l >> 1;						\
	    l |= l >> 2;						\
	    l |= l >> 4;						\
	    sum |= l;							\
	    uflo &=~ l;							\
	}								\
	if (uflo != 0) {						\
	    /* zero underflow(s) */					\
	    uflo |= uflo >> 1;						\
	    uflo |= uflo >> 2;						\
	    uflo |= uflo >> 4;						\
	    sum &=~ uflo;						\
	}								\
    }									\
    PONERGB(dst, sum & pmask);						\
}

void TrueWindowRenderer24::map_422(const u_char* frm, u_int off,
				 u_int x, u_int width, u_int height) const
{
        UNUSED(x); //SV-XXX: unused

	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register char* xip = (char*)pixbuf_ + 3*off;
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height; len > 0; len -= 8) {
		register u_int l, uv;
		register u_int uflo, sum;

#define PTWO422(n) \
		uv = yuv2rgb[(up[(n)/2] << 8) | vp[(n)/2]]; \
		PONEPIX(yp[(n)], xip[3*(n)]) \
		PONEPIX(yp[(n)+1], xip[3*(n)+3])

		PTWO422(0)
		PTWO422(2)
		PTWO422(4)
		PTWO422(6)

		xip += 24;
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
			xip += 3*pstride;
		}
	}
}

void TrueWindowRenderer24::map_down2_422(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register char* xip = (char*)pixbuf_ + 3*(((off - x) >> 2) + (x >> 1));
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height >> 1; len > 0; len -= 8) {
		register u_int l, uv;
		register u_int uflo, sum;

#define PONE422(n) \
		uv = yuv2rgb[(up[(n)/2] << 8) | vp[(n)/2]]; \
		PONEPIX(yp[(n)], xip[3*(n)/2])

		PONE422(0)
		PONE422(2)
		PONE422(4)
		PONE422(6)

		xip += 12;
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
			xip += 3*((iw - w) >> 1);
		}
	}
}


void TrueWindowRenderer24::map_down4_422(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register char* xip = (char*)pixbuf_ + 3*(((off - x) >> 4) + (x >> 2));
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height >> 2; len > 0; len -= 8) {
		register u_int l, uv;
		register u_int uflo, sum;

		uv = yuv2rgb[(up[0] << 8) | vp[0]];
		PONEPIX(yp[0], xip[0])
		uv = yuv2rgb[(up[2] << 8) | vp[2]];
		PONEPIX(yp[4], xip[3])

		xip += 6;
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
			xip += 3*((iw - w) >> 2);
		}
	}
}

/*
 * decimate by some power of 2 >= 2^3.
 */
void TrueWindowRenderer24::map_down_422(const u_char* frm,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register int s = scale_;
	register int istride = 1 << s;
	register char* xip = (char*)pixbuf_ +
	    3*(((off - x) >> (s + s)) + (x >> s));
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height >> s; len > 0; len -= istride) {
		register u_int l, uv;
		register u_int uflo, sum;

		uv = yuv2rgb[(up[0] << 8) | vp[0]];
		PONEPIX(yp[0], xip[0])

		xip += 3;
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
			xip += 3*((iw - w) >> s);
		}
	}
}

void TrueWindowRenderer24::map_up2_422(const u_char* frm,
				     u_int off, u_int x,
				     u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register char* xip = (char*)pixbuf_ + 3*(((off - x) << 2) + (x << 1));
	register int w = width;
	register u_int e1 = yp[0];
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height; len > 0; len -= 2) {
		register u_int l, uv;
		register u_int uflo, sum;
		register u_int e2;
		register char* xip2 = xip + 3*((iw << 1));

		uv = yuv2rgb[(up[0] << 8) | vp[0]];
		e2 = yp[0];
		PONEPIX((e1 + e2) >> 1, xip[0])
		PONERGB(xip2[0], sum);
		PONEPIX(e2, xip[3])
		PONERGB(xip2[3], sum);
		e1 = yp[1];
		PONEPIX((e1 + e2) >> 1, xip[6])
		PONERGB(xip2[6], sum);
		PONEPIX(e1, xip[9])
		PONERGB(xip2[9], sum);

		xip += 12;
		yp += 2;
		up += 1;
		vp += 1;

		w -= 2;
		if (w <= 0) {
			w = width;
			register u_int pstride = iw - w;
			register u_int cstride = pstride >> 1;
			yp += pstride;
			e1 = yp[0];
			up += cstride;
			vp += cstride;
			xip += 3*((iw + pstride) << 1);
		}
	}
}

void TrueWindowRenderer24::map_420(const u_char* frm, u_int off,
				 u_int x, u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register char* xip = (char*)pixbuf_ + 3*off;
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height; len > 0; len -= 8) {
		register u_int l, uv;
		register u_int uflo, sum;
		register char* xip2 = xip + 3*iw;
		register const u_char* yp2 = yp + iw;

#define PFOUR420(n) \
		uv = yuv2rgb[(up[(n)/2] << 8) | vp[(n)/2]]; \
		PONEPIX(yp[(n)], xip[3*(n)]) \
		PONEPIX(yp[(n)+1], xip[3*(n)+3]) \
		PONEPIX(yp2[(n)], xip2[3*(n)]) \
		PONEPIX(yp2[(n)+1], xip2[3*(n)+3])

		PFOUR420(0)
		PFOUR420(2)

		xip += 12;
		yp += 4;
		up += 2;
		vp += 2;

		w -= 4;
		if (w <= 0) {
			w = width;
			register int pstride = 2 * iw - w;
			register int cstride = (iw - w) >> 1;
			yp += pstride;
			up += cstride;
			vp += cstride;
			xip += 3*pstride;
		}
	}
}

#ifdef notdef
void TrueWindowRenderer24::map_420_16(const u_char* frm, u_int off,
				    u_int x, u_int width, u_int height) const
{
#ifdef FIX_THIS
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	/*XXX use u_int here*/
	register u_short* xip = (u_short*)pixbuf_ + off;
	register int w = width;
	register const u_char* yuv2rgb = (u_char*)cm_.yuv2rgb();
	for (register int len = width * height; len > 0; len -= 16) {
		register u_int u = *(const u_int*)up;
		register u_int v = *(const u_int*)vp;
		register u_int y1, y2;
		register const u_char* y2r;

		y1 = (u & 0xf800f800) | ((v & 0xf800f800) >> 5);
		v = ((u & 0x00f800f8) << 5) | (v & 0x00f800f8);
		u = y1;

		y1 = *(const u_int*)yp;
		y2r = yuv2rgb + UV0;
		xip[0] =  *(u_int*)(y2r + ((y1 >> SHIFT_24) & 0xfc));
		xip[1] =  *(u_int*)(y2r + ((y1 >> SHIFT_16) & 0xfc));
		y2 = *(const u_int*)(yp + iw);
		register u_short* xip2 = xip + iw;
		xip2[0] = *(u_int*)(y2r + ((y2 >> SHIFT_24) & 0xfc));
		xip2[1] = *(u_int*)(y2r + ((y2 >> SHIFT_16) & 0xfc));

		y2r = yuv2rgb + UV1;
		xip[2] =  *(u_int*)(y2r + ((y1 >> SHIFT_8) & 0xfc));
		xip[3] =  *(u_int*)(y2r + ((y1 >> SHIFT_0) & 0xfc));
		xip2[2] = *(u_int*)(y2r + ((y2 >> SHIFT_8) & 0xfc));
		xip2[3] = *(u_int*)(y2r + ((y2 >> SHIFT_0) & 0xfc));

		y1 = *(const u_int*)(yp + 4);
		y2r = yuv2rgb + UV2;
		xip[4] =  *(u_int*)(y2r + ((y1 >> SHIFT_24) & 0xfc));
		xip[5] =  *(u_int*)(y2r + ((y1 >> SHIFT_16) & 0xfc));
		y2 = *(const u_int*)(yp + iw + 4);
		xip2[4] = *(u_int*)(y2r + ((y2 >> SHIFT_24) & 0xfc));
		xip2[5] = *(u_int*)(y2r + ((y2 >> SHIFT_16) & 0xfc));

		y2r = yuv2rgb + UV3;
		xip[6] =  *(u_int*)(y2r + ((y1 >> SHIFT_8) & 0xfc));
		xip[7] =  *(u_int*)(y2r + ((y1 >> SHIFT_0) & 0xfc));
		xip2[6] = *(u_int*)(y2r + ((y2 >> SHIFT_8) & 0xfc));
		xip2[7] = *(u_int*)(y2r + ((y2 >> SHIFT_0) & 0xfc));

		xip += 8;
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
			xip += pstride;
		}
	}
#endif
}
#endif

void TrueWindowRenderer24::map_down2_420(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	off = ((off - x) >> 2) + (x >> 1);
	register const u_char* up = frm + framesize_ + off;
	register const u_char* vp = up + (framesize_ >> 2);
	register char* xip = (char*)pixbuf_ + 3*off;
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height >> 1; len > 0; len -= 8) {
		register u_int l, uv;
		register u_int uflo, sum;

#define PONE420(n) \
		uv = yuv2rgb[(up[(n)/2] << 8) | vp[(n)/2]]; \
		PONEPIX(yp[(n)], xip[3*(n)/2])

		PONE420(0)
		PONE420(2)
		PONE420(4)
		PONE420(6)

		xip += 12;
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
			xip += 3*cstride;
		}
	}
}

void TrueWindowRenderer24::map_down4_420(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register char* xip = (char*)pixbuf_ + 3*(((off - x) >> 4) + (x >> 2));
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height >> 2; len > 0; len -= 8) {
		register u_int l, uv;
		register u_int uflo, sum;

		uv = yuv2rgb[(up[0] << 8) | vp[0]];
		PONEPIX(yp[0], xip[0])
		uv = yuv2rgb[(up[2] << 8) | vp[2]];
		PONEPIX(yp[4], xip[3])

		xip += 6;
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
			xip += 3*((iw - w) >> 2);
		}
	}
}

/*
 * decimate by some power of 2 >= 2^3.
 */
void TrueWindowRenderer24::map_down_420(const u_char* frm,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register int s = scale_;
	register int istride = 1 << s;
	register char* xip = (char*)pixbuf_
		+ 3*(((off - x) >> (s + s)) + (x >> s));
	register int w = width;
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height >> s; len > 0; len -= istride) {
		register u_int l, uv;
		register u_int uflo, sum;

		uv = yuv2rgb[(up[0] << 8) | vp[0]];
		PONEPIX(yp[0], xip[0])

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
			xip += 3*((iw - w) >> s);
		}
	}
}

void TrueWindowRenderer24::map_up2_420(const u_char* frm,
				     u_int off, u_int x,
				     u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register char* xip = (char*)pixbuf_ + 3*(((off - x) << 2) + (x << 1));
	register int w = width;
	register u_int e1 = yp[0], o1 = yp[iw];
	register const u_int* yuv2rgb = cm_.uvtab();
	register u_int omask = cm_.omask();
	register u_int pmask = cm_.pmask();

	for (register int len = w * height; len > 0; len -= 4) {
		register u_int l, uv;
		register u_int uflo, sum;
		register u_int e2, o2;
		register const u_char* yp2 = yp + iw;
		register char* xip2 = xip + 3*((iw << 1));
		register char* xip3 = xip2 + 3*((iw << 1));
		register char* xip4 = xip3 + 3*((iw << 1));

		uv = yuv2rgb[(up[0] << 8) | vp[0]];
		e2 = yp[0];
		PONEPIX((e1 + e2) >> 1, xip[0])
		PONERGB(xip2[0], sum);
		PONEPIX(e2, xip[3])
		PONERGB(xip2[3], sum);
		e1 = yp[1];
		PONEPIX((e1 + e2) >> 1, xip[6])
		PONERGB(xip2[6], sum);
		PONEPIX(e1, xip[9])
		PONERGB(xip2[9], sum);

		o2 = yp2[0];
		PONEPIX((o1 + o2) >> 1, xip3[0])
		PONERGB(xip4[0], sum);
		PONEPIX(o2, xip3[3])
		PONERGB(xip4[3], sum);
		o1 = yp2[1];
		PONEPIX((o1 + o2) >> 1, xip3[6])
		PONERGB(xip4[6], sum);
		PONEPIX(o1, xip3[9])
		PONERGB(xip4[9], sum);


		xip += 12;
		yp += 2;
		up += 1;
		vp += 1;

		w -= 2;
		if (w <= 0) {
			w = width;
			register u_int pstride = 2 * iw - w;
			register u_int cstride = (iw - w) >> 1;
			yp += pstride;
			e1 = yp[0];
			o1 = yp[iw];
			up += cstride;
			vp += cstride;
			xip += 3 * (8 * iw - 2 * w);
		}
	}
}

void TrueWindowRenderer24::map_gray(register const u_char *yp,
				  u_int off, u_int x,
				  u_int width, u_int height) const
{
        UNUSED(x); //SV-XXX: unused

	register u_int iw = width_;
	yp += off;
	register char* xip = (char*)pixbuf_ + 3*off;
	register int w = width;
	for (register int len = w * height; len > 0; len -= 8) {
		register u_int y1;
		register u_int pix;

		y1 = *(const u_int*)yp;
		pix = (y1 >> SHIFT_24) & 0xff;
		PONEGRAY(xip[0], pix);
		pix = (y1 >> SHIFT_16) & 0xff;
		PONEGRAY(xip[3], pix);
		pix = (y1 >> SHIFT_8) & 0xff;
		PONEGRAY(xip[6], pix);
		pix = (y1 >> SHIFT_0) & 0xff;
		PONEGRAY(xip[9], pix);

		y1 = *(const u_int*)(yp + 4);
		pix = (y1 >> SHIFT_24) & 0xff;
		PONEGRAY(xip[12], pix);
		pix = (y1 >> SHIFT_16) & 0xff;
		PONEGRAY(xip[15], pix);
	        pix = (y1 >> SHIFT_8) & 0xff;
		PONEGRAY(xip[18], pix);
		pix = (y1 >> SHIFT_0) & 0xff;
		PONEGRAY(xip[21], pix);

		xip += 24;
		yp += 8;

		w -= 8;
		if (w <= 0) {
			w = width;
			register u_int pstride = iw - w;
			yp += pstride;
			xip += 3*pstride;
		}
	}
}

void TrueWindowRenderer24::map_gray_down2(register const u_char *yp,
					u_int off, u_int x,
					u_int width, u_int height) const
{
	register u_int iw = width_;
	yp += off;
	off = ((off - x) >> 2) + (x >> 1);
	register char* xip = (char*)pixbuf_ + 3*off;
	register int w = width;
	for (register int len = w * height >> 1; len > 0; len -= 8) {
		register u_int y1;
		register u_int pix;

		y1 = *(const u_int*)yp;
		pix = (y1 >> SHIFT_24) & 0xff;
		PONEGRAY(xip[0], pix);
		pix = (y1 >> SHIFT_8) & 0xff;
		PONEGRAY(xip[3], pix);

		y1 = *(const u_int*)(yp + 4);
		pix = (y1 >> SHIFT_24) & 0xff;
		PONEGRAY(xip[6], pix);
		pix = (y1 >> SHIFT_8) & 0xff;
		PONEGRAY(xip[9], pix);

		xip += 12;
		yp += 8;

		w -= 8;
		if (w <= 0) {
			w = width;
			register int pstride = 2 * iw - w;
			yp += pstride;
			xip += 3*((iw - w) >> 1);
		}
	}
}

void TrueWindowRenderer24::map_gray_down4(register const u_char *yp,
					u_int off, u_int x,
					u_int width, u_int height) const
{
	register u_int iw = width_;
	yp += off;
	register char* xip = (char*)pixbuf_ + 3*(((off - x) >> 4) + (x >> 2));
	register int w = width;
	for (register int len = w * height >> 2; len > 0; len -= 8) {
		register u_int pix;

		pix = yp[0];
		PONEGRAY(xip[0], pix);
		pix = yp[4];
		PONEGRAY(xip[3], pix);

		xip += 6;
		yp += 8;

		w -= 8;
		if (w <= 0) {
			w = width;
			register int pstride = 4 * iw - w;
			yp += pstride;
			xip += 3*((iw - w) >> 2);
		}
	}
}

void TrueWindowRenderer24::map_gray_down(register const u_char *yp,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	yp += off;
	register int s = scale_;
	register int istride = 1 << s;
	register char* xip = (char*)pixbuf_ +
		3*(((off - x) >> (s + s)) + (x >> s));
	register int w = width;
	for (register int len = w * height >> s; len > 0; len -= istride) {
		register u_int pix = *yp;
		PONEGRAY(xip[0], pix);
		xip += 3;
		yp += istride;
		w -= istride;
		if (w <= 0) {
			w = width;
			register int pstride = (iw << s) - w;
			yp += pstride;
			xip += 3*((iw - w) >> s);
		}
	}
}

void TrueWindowRenderer24::map_gray_up2(register const u_char *yp,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
	register u_int iw = width_;
	yp += off;
	register char* xip = (char*)pixbuf_ + 3*(((off - x) << 2) + (x << 1));
	register int w = width;
	register u_int e1 = yp[0];

	for (register int len = width * height; len > 0; len -= 8) {
		register u_int y1, e2, pix;
		register char* xip2 = xip + 3*iw * 2;

		y1 = *(const u_int*)yp;
		e2 = (y1 >> SHIFT_24) & 0xff;
		pix = (e1 + e2) >> 1;
		PONEGRAY(xip[0], pix);
		PONEGRAY(xip2[0], pix);
		PONEGRAY(xip[3], e2);
		PONEGRAY(xip2[3], e2);
		e1 = (y1 >> SHIFT_16) & 0xff;
		pix = (e1 + e2) >> 1;
		PONEGRAY(xip[6], pix);
		PONEGRAY(xip2[6], pix);
		PONEGRAY(xip[9], e1);
		PONEGRAY(xip2[9], e1);

		e2 = (y1 >> SHIFT_8) & 0xff;
		pix = (e1 + e2) >> 1;
		PONEGRAY(xip[12], pix);
		PONEGRAY(xip2[12], pix);
		PONEGRAY(xip[15], e2);
		PONEGRAY(xip2[15], e2);
		e1 = (y1 >> SHIFT_0) & 0xff;
		pix = (e1 + e2) >> 1;
		PONEGRAY(xip[18], pix);
		PONEGRAY(xip2[18], pix);
		PONEGRAY(xip[21], e1);
		PONEGRAY(xip2[21], e1);

		y1 = *(const u_int*)(yp + 4);
		e2 = (y1 >> SHIFT_24) & 0xff;
		pix = (e1 + e2) >> 1;
		PONEGRAY(xip[24], pix);
		PONEGRAY(xip2[24], pix);
		PONEGRAY(xip[27], e2);
		PONEGRAY(xip2[27], e2);
		e1 = (y1 >> SHIFT_16) & 0xff;
		pix = (e1 + e2) >> 1;
		PONEGRAY(xip[30], pix);
		PONEGRAY(xip2[30], pix);
		PONEGRAY(xip[33], e1);
		PONEGRAY(xip2[33], e1);

		e2 = (y1 >> SHIFT_8) & 0xff;
		pix = (e1 + e2) >> 1;
		PONEGRAY(xip[36], pix);
		PONEGRAY(xip2[36], pix);
		PONEGRAY(xip[39], e2);
		PONEGRAY(xip2[39], e2);
		e1 = (y1 >> SHIFT_0) & 0xff;
		pix = (e1 + e2) >> 1;
		PONEGRAY(xip[42], pix);
		PONEGRAY(xip2[42], pix);
		PONEGRAY(xip[45], e1);
		PONEGRAY(xip2[45], e1);

		xip += 48;
		yp += 8;

		w -= 8;
		if (w <= 0) {
			w = width;
			register u_int pstride = iw - w;
			yp += pstride;
			e1 = yp[0];
			xip += 3*((iw + pstride) << 1);
		}
	}
}
