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
static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include <stdio.h>
#include <stdlib.h>
//#include <X11/Xlib.h>
//#include <X11/Xutil.h>
#include "bsd-endian.h"
#include "color.h"
#include "renderer-window.h"
#include "inet.h"
#include "vic_tcl.h"
#include "vw.h"
#define swapbyte32(x) \
        ((u_int)( \
                (((u_int)(x) & (u_int)0x000000ffUL) << 24) | \
                (((u_int)(x) & (u_int)0x0000ff00UL) <<  8) | \
                (((u_int)(x) & (u_int)0x00ff0000UL) >>  8) | \
                (((u_int)(x) & (u_int)0xff000000UL) >> 24) ))
#define swapbyte32l(x) \
        ((u_int)( \
                (((u_int)(x) & (u_int)0x000000ffUL) <<  8) | \
                (((u_int)(x) & (u_int)0x0000ff00UL) >>  8) ))


class HiColorModel : public ColorModel {
    public:
	~HiColorModel();
	WindowRenderer* allocate(VideoWindow* vw, int decimation);
	virtual int command(int argc, const char*const* argv);
	virtual int alloc_colors();
	virtual int alloc_grays();
	inline const u_short* uvtab() const { return (&uvtab_[0]); }
    protected:
	u_short uvtab_[65536];
};

class HiColorMatcher : public Matcher {
public:
	HiColorMatcher() : Matcher("colormodel") {}
	virtual TclObject* match(const char* id) {
		if (strcasecmp(id, "truecolor/16") == 0 ||
		    strcasecmp(id, "hicolor/16") == 0)
			return (new HiColorModel());
		return (0);
	}
} matcher_hicolor;

HiColorModel::~HiColorModel()
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

#if defined(_WIN32) && defined(_MSC_VER)
#pragma optimize("g", off)
#endif

int HiColorModel::alloc_grays()
{
	return (0);
}

#define UVINDX(u, v) ((((u) >> 3) << 11) | (((v) >> 3) << 6))

int HiColorModel::alloc_colors()
{
#ifndef WIN32
	u_int rmask = visual_->red_mask;
	u_int gmask = visual_->green_mask;
	u_int bmask = visual_->blue_mask;
#else /* WIN32: tcl/tk uses a different bit mask in images and visuals */
#ifndef USE_DDRAW
	u_int rmask   = 0x7c00;
	u_int gmask   = 0x03e0;
	u_int bmask   = 0x001f; 
#else
	u_int rmask   = 0xf800;
	u_int gmask   = 0x07e0;
	u_int bmask   = 0x001f; 
//#else
//	u_int rmask = visual_->red_mask;
//	u_int gmask = visual_->green_mask;
//	u_int bmask = visual_->blue_mask;


#endif
#endif /* WIN32 */
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
//printf("HI:rmask: %d(%d), gmask: %d(%d), bmask: %d(%d)\n",rmask,htonl(rmask),gmask,htonl(gmask),bmask,htonl(bmask));

#if BYTE_ORDER == LITTLE_ENDIAN
	if (ImageByteOrder(dpy_) == MSBFirst) {
		rmask = htonl(rmask);
		gmask = htonl(gmask);
		bmask = htonl(bmask);
	}
#else
        if (ImageByteOrder(dpy_) != MSBFirst) {

                rmask = swapbyte32l(rmask);
                gmask = swapbyte32l(gmask);
                bmask = swapbyte32l(bmask);
        }

#endif

	u_int rshft = mtos(rmask);
	u_int rlose = 8 - mtos(~(rmask >> rshft));
	u_int gshft = mtos(gmask);
	u_int glose = 8 - mtos(~(gmask >> gshft));
	u_int bshft = mtos(bmask);
	u_int blose = 8 - mtos(~(bmask >> bshft));

	/*
	 * build a table that maps 5:5:6 u:v:y to rgb.
	 */
	color c;
	for (int u = 0; u < 256; u += 8) {
		c.u = u<128? u+4 : u>128? u-4 : u;
		for (int v = 0; v < 256; v += 8) {
			c.v = v<128? v+4 : v>128? v-4 : v;
			for (int y = 3; y < 256; y += 4) {
				c.y = y;
				yuv_to_rgb(c);
				u_int i = UVINDX(u, v) | (y >> 2);
				uvtab_[i] =
					(c.r & 0xff) >> rlose << rshft |
					(c.g & 0xff) >> glose << gshft |
					(c.b & 0xff) >> blose << bshft;
			}			
		}
	}
	return (0);
}

class HiWindowRenderer;

typedef void (HiWindowRenderer::*HiMethod)(const u_char*, u_int,
					   u_int, u_int, u_int) const;

class HiWindowRenderer : public WindowDitherer {
    public:
	HiWindowRenderer(VideoWindow* vw, int decimation, HiColorModel& cm)
		: WindowDitherer(vw, decimation), cm_(cm), method_(0){ }
	void render(const u_char* frm, int off, int x, int w, int h) {
		(this->*method_)(frm, off, x, w, h);
	}
    protected:
	HiColorModel& cm_;
	virtual void update();
	virtual void disable() { method_ = HiMethod(&WindowRenderer::dither_null); }
	HiMethod method_;
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

int HiColorModel::command(int argc, const char*const* argv)
{
	if (argc == 4 && strcmp(argv[1], "renderer") == 0) {
		Tcl& tcl = Tcl::instance();
		VideoWindow* vw = VideoWindow::lookup(argv[2]);
		int decimation = atoi(argv[3]);
		Renderer* r = new HiWindowRenderer(vw, decimation, *this);
		tcl.result(r->name());
		return (TCL_OK);
	}
	return (ColorModel::command(argc, argv));
}

void HiWindowRenderer::update()
{
	static HiMethod methods[] = {
	    &HiWindowRenderer::map_up2_420,
	    &HiWindowRenderer::map_up2_422,
	    &HiWindowRenderer::map_gray_up2,
	    &HiWindowRenderer::map_gray_up2,
	    &HiWindowRenderer::map_420,
	    &HiWindowRenderer::map_422,
	    &HiWindowRenderer::map_gray,
	    &HiWindowRenderer::map_gray,
	    &HiWindowRenderer::map_down2_420,
	    &HiWindowRenderer::map_down2_422,
	    &HiWindowRenderer::map_gray_down2,
	    &HiWindowRenderer::map_gray_down2,
	    &HiWindowRenderer::map_down4_420,
	    &HiWindowRenderer::map_down4_422,
	    &HiWindowRenderer::map_gray_down4,
	    &HiWindowRenderer::map_gray_down4,
	    &HiWindowRenderer::map_down_420,
	    &HiWindowRenderer::map_down_422,
	    &HiWindowRenderer::map_gray_down,
	    &HiWindowRenderer::map_gray_down,
	};
	method_ = methods[index()];
}

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

#define PIXSETUP \
	register const u_short* uv;

#define ONEPIX(src, dst) { \
	dst = uv[src >> 2]; \
}

void HiWindowRenderer::map_422(const u_char* frm, u_int off,
				 u_int x, u_int width, u_int height) const
{
        UNUSED(x); //SV-XXX: unused

	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_short* xip = (u_short*)pixbuf_ + off;
	register int w = width;
	register const u_short* yuv2rgb = cm_.uvtab();

	for (register int len = w * height; len > 0; len -= 8) {
		PIXSETUP

#define TWO422(n) \
		uv = yuv2rgb + UVINDX(up[(n)/2], vp[(n)/2]); \
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

void HiWindowRenderer::map_down2_422(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_short* xip = (u_short*)pixbuf_ + ((off - x) >> 2) + (x >> 1);
	register int w = width;
	register const u_short* yuv2rgb = cm_.uvtab();

	for (register int len = w * height >> 1; len > 0; len -= 8) {
		PIXSETUP

#define ONE422(n) \
		uv = yuv2rgb + UVINDX(up[(n)/2], vp[(n)/2]); \
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


void HiWindowRenderer::map_down4_422(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_short* xip = (u_short*)pixbuf_ + ((off - x) >> 4) + (x >> 2);
	register int w = width;
	register const u_short* yuv2rgb = cm_.uvtab();

	for (register int len = w * height >> 2; len > 0; len -= 8) {
		PIXSETUP

		uv = yuv2rgb + UVINDX(up[0], vp[0]);
		ONEPIX(yp[0], xip[0])
		uv = yuv2rgb + UVINDX(up[2], vp[2]);
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
void HiWindowRenderer::map_down_422(const u_char* frm,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register int s = scale_;
	register int istride = 1 << s;
	register u_short* xip = (u_short*)pixbuf_ +
		((off - x) >> (s + s)) + (x >> s);
	register int w = width;
	register const u_short* yuv2rgb = cm_.uvtab();

	for (register int len = w * height >> s; len > 0; len -= istride) {
		PIXSETUP

		uv = yuv2rgb + UVINDX(up[0], vp[0]);
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

void HiWindowRenderer::map_up2_422(const u_char* frm,
				     u_int off, u_int x,
				     u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_short* xip = (u_short*)pixbuf_ + ((off - x) << 2) + (x << 1);
	register int w = width;
	register u_short e1 = yp[0];
	register const u_short* yuv2rgb = cm_.uvtab();

	for (register int len = w * height; len > 0; len -= 2) {
		PIXSETUP
		register u_short e2, t;
		register u_short* xip2 = xip + (iw << 1);

		uv = yuv2rgb + UVINDX(up[0], vp[0]);
		e2 = yp[0];
		ONEPIX((e1 + e2) >> 1, t)
		xip[0] = t;
		xip2[0] = t;
		ONEPIX(e2, t)
		xip[1] = t;
		xip2[1] = t;
		e1 = yp[1];
		ONEPIX((e1 + e2) >> 1, t)
		xip[2] = t;
		xip2[2] = t;
		ONEPIX(e1, t)
		xip[3] = t;
		xip2[3] = t;

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

void HiWindowRenderer::map_420(const u_char* frm, u_int off,
				 u_int x, u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register u_short* xip = (u_short*)pixbuf_ + off;
	register int w = width;
	register const u_short* yuv2rgb = cm_.uvtab();

	for (register int len = w * height; len > 0; len -= 8) {
		PIXSETUP
		register u_short* xip2 = xip + iw;
		register const u_char* yp2 = yp + iw;

#define FOUR420(n) \
		uv = yuv2rgb + UVINDX(up[(n)/2], vp[(n)/2]); \
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

void HiWindowRenderer::map_down2_420(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	off = ((off - x) >> 2) + (x >> 1);
	register const u_char* up = frm + framesize_ + off;
	register const u_char* vp = up + (framesize_ >> 2);
	register u_short* xip = (u_short*)pixbuf_ + off;
	register int w = width;
	register const u_short* yuv2rgb = cm_.uvtab();

	for (register int len = w * height >> 1; len > 0; len -= 8) {
		PIXSETUP

#define ONE420(n) \
		uv = yuv2rgb + UVINDX(up[(n)/2], vp[(n)/2]); \
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

void HiWindowRenderer::map_down4_420(const u_char* frm,
				       u_int off, u_int x, 
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register u_short* xip = (u_short*)pixbuf_ + ((off - x) >> 4) + (x >> 2);
	register int w = width;
	register const u_short* yuv2rgb = cm_.uvtab();

	for (register int len = w * height >> 2; len > 0; len -= 8) {
		PIXSETUP

		uv = yuv2rgb + UVINDX(up[0], vp[0]);
		ONEPIX(yp[0], xip[0])
		uv = yuv2rgb + UVINDX(up[2], vp[2]);
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
void HiWindowRenderer::map_down_420(const u_char* frm,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register int s = scale_;
	register int istride = 1 << s;
	register u_short* xip = (u_short*)pixbuf_
		+ ((off - x) >> (s + s)) + (x >> s);
	register int w = width;
	register const u_short* yuv2rgb = cm_.uvtab();

	for (register int len = w * height >> s; len > 0; len -= istride) {
		PIXSETUP

		uv = yuv2rgb + UVINDX(up[0], vp[0]);
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

void HiWindowRenderer::map_up2_420(const u_char* frm,
				     u_int off, u_int x,
				     u_int width, u_int height) const
{
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register u_short* xip = (u_short*)pixbuf_ + ((off - x) << 2) + (x << 1);
	register int w = width;
	register u_short e1 = yp[0], o1 = yp[iw];
	register const u_short* yuv2rgb = cm_.uvtab();

	for (register int len = w * height; len > 0; len -= 4) {
		PIXSETUP
		register u_short e2, o2, t;
		register const u_char* yp2 = yp + iw;
		register u_short* xip2 = xip + (iw << 1);
		register u_short* xip3 = xip2 + (iw << 1);
		register u_short* xip4 = xip3 + (iw << 1);

		uv = yuv2rgb + UVINDX(up[0], vp[0]);
		e2 = yp[0];
#define ONEx2e(v, n) \
	ONEPIX(v, t) \
	xip[n] = t; \
	xip2[n] = t;
#define ONEx2o(v, n) \
	ONEPIX(v, t) \
	xip3[n] = t; \
	xip4[n] = t;
		ONEx2e((e1 + e2) >> 1, 0)
		ONEx2e(e2, 1)
		e1 = yp[1];
		ONEx2e((e1 + e2) >> 1, 2)
		ONEx2e(e1, 3)

		o2 = yp2[0];
		ONEx2o((o1 + o2) >> 1, 0)
		ONEx2o(o2, 1)
		o1 = yp2[1];
		ONEx2o((o1 + o2) >> 1, 2)
		ONEx2o(o1, 3)

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

void HiWindowRenderer::map_gray(register const u_char *yp,
				  u_int off, u_int x,
				  u_int width, u_int height) const
{
        UNUSED(x); //SV-XXX: unused

	register u_int iw = width_;
	yp += off;
	register u_short* xip = (u_short*)pixbuf_ + off;
	register const u_short* uv = cm_.uvtab() + UVINDX(0x80, 0x80);
	register int w = width;
	for (register int len = w * height; len > 0; len -= 8) {
		ONEPIX(yp[0], xip[0])
		ONEPIX(yp[1], xip[1])
		ONEPIX(yp[2], xip[2])
		ONEPIX(yp[3], xip[3])
		ONEPIX(yp[4], xip[4])
		ONEPIX(yp[5], xip[5])
		ONEPIX(yp[6], xip[6])
		ONEPIX(yp[7], xip[7])

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

void HiWindowRenderer::map_gray_down2(register const u_char *yp,
					u_int off, u_int x,
					u_int width, u_int height) const
{
	register u_int iw = width_;
	yp += off;
	off = ((off - x) >> 2) + (x >> 1);
	register u_short* xip = (u_short*)pixbuf_ + off;
	register const u_short* uv = cm_.uvtab() + UVINDX(0x80, 0x80);
	register int w = width;
	for (register int len = w * height >> 1; len > 0; len -= 8) {
		ONEPIX(yp[0], xip[0])
		ONEPIX(yp[2], xip[1])
		ONEPIX(yp[4], xip[2])
		ONEPIX(yp[6], xip[3])

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

void HiWindowRenderer::map_gray_down4(register const u_char *yp,
					u_int off, u_int x,
					u_int width, u_int height) const
{
	register u_int iw = width_;
	yp += off;
	register u_short* xip = (u_short*)pixbuf_ + ((off - x) >> 4) + (x >> 2);
	register const u_short* uv = cm_.uvtab() + UVINDX(0x80, 0x80);
	register int w = width;
	for (register int len = w * height >> 2; len > 0; len -= 8) {
		ONEPIX(yp[0], xip[0])
		ONEPIX(yp[4], xip[1])

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

void HiWindowRenderer::map_gray_down(register const u_char *yp,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	register u_int iw = width_;
	yp += off;
	register int s = scale_;
	register int istride = 1 << s;
	register u_short* xip = (u_short*)pixbuf_ +
		((off - x) >> (s + s)) + (x >> s);
	register const u_short* uv = cm_.uvtab() + UVINDX(0x80, 0x80);
	register int w = width;
	for (register int len = w * height >> s; len > 0; len -= istride) {
		ONEPIX(yp[0], xip[0])

		++xip;
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

void HiWindowRenderer::map_gray_up2(register const u_char *yp,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
	register u_int iw = width_;
	yp += off;
	register u_short* xip = (u_short*)pixbuf_ + ((off - x) << 2) + (x << 1);
	register const u_short* uv = cm_.uvtab() + UVINDX(0x80, 0x80);
	register int w = width;
	register u_short e1 = yp[0];

	for (register int len = width * height; len > 0; len -= 8) {
		register u_short t, e2;
		register u_short* xip2 = xip + iw * 2;

#define ONEx2(v, n) \
	ONEPIX(v, t) \
	xip[n] = t; \
	xip2[n] = t;

#define TWOx2(n) \
	e2 = yp[n]; \
	ONEx2((e1 + e2) >> 1, (n)*2) \
	ONEx2(e2, (n)*2 + 1) \
	e1 = yp[(n) + 1]; \
	ONEx2((e1 + e2) >> 1, (n)*2 + 2) \
	ONEx2(e1, (n)*2 + 3)

		TWOx2(0)
		TWOx2(2)
		TWOx2(4)
		TWOx2(6)

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
