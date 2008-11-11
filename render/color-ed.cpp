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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

/*
 * Implementation of Floyd-Steinburg style error diffusion dithering.
 * The diffusion is a simple one-dimensional pattern.  We restrict it
 * to one dimension so that we can have an efficient loop with the
 * current error always contained in a register (i.e., no need to
 * write and read back pixel errors from memory).
 *
 * This serpentine pattern yields reasonable results (considering we're
 * restricted to 1D):
 *
 *		. .-. .-. .-. .-. 
 *		| | | | | | | | | 
 *		.-. .-. .-. .-. .
 *				|
 *		.-. .-. .-. .-. .
 *		| | | | | | | | | 
 *		. .-. .-. .-. .-
 *              |
 *              ...
 *
 * Since we dither on a block basis, we begin each dither run with
 * random error to avoid artifacts induced by boundaries.
 *
 * The dither is carried out in the YUV, rather than the RGB, color space.
 * Color dithering relies on the fact that the three color planes
 * can be dithered independently.  This works because of additivity
 * of the primaries.  However, the YUV space is not a cube and
 * the three planes cannot be dithered independently (in theory).
 * This is because a valid dithering "jump" along one axis can cause
 * the current point to fall far outside the color space.  This
 * happens frequently, especially at low and high luminances where
 * the U-V plane is small.  The dithering jumps are large because
 * the available number of colors is small and the distance between
 * lattice points in the allocated color grid is large.
 * Fortunately, this happens only at low or high luminance levels,
 * where color fidelity is less perceptible, so in practice
 * we're okay.  One problem is at edges.  A large color error
 * can accumulate in a dark region, so at the boundary where
 * the luminance jumps, there can be a large color error which
 * appears as a pastel shadow.  This effect is reduced by
 * scaling down the color errors at low and high luminosities.
 *
 * Another problem is that error-diffusion relies on the placement
 * of color cells on a convex hull of the color space (i.e., so there
 * is always a color on the other side of the input color, wrt to the
 * quantized pixel, to absorb the error).  In RGB, such placement is
 * trivial -- e.g., use the corners of the RGB cube.  But in the YUV
 * space, these points are not obvious.  Instead, we let the dither
 * run outside the color space under the assumption that retaining
 * the error terms is more important than making sure the dither
 * always stays inside the convex hull of available colors.  Of course,
 * the dither is limited to range of the three planes (i.e., the
 * dither never leaves the "YUV-cube" but it may leave YUV-space,
 * which doesn't fill the whole cube).
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//#include <X11/Xlib.h>
//#include <X11/Xutil.h>
#include "color.h"
#include "color-pseudo.h"
#include "bsd-endian.h"
#include "vw.h"

class EDColorModel : public PseudoColorModel {
public:
	EDColorModel();
	virtual int alloc_colors();
	virtual int command(int argc, const char*const* argv);
	inline const u_int* lut() const { return (lut_); }
private:
	u_int* lut_;
};

class EDColorMatcher : public Matcher {
public:
	EDColorMatcher() : Matcher("colormodel") {}
	virtual TclObject* match(const char* id) {
		if (strcasecmp(id, "pseudocolor/8/ed") == 0)
			return (new EDColorModel());
		return (0);
	}
} matcher_ed;

EDColorModel::EDColorModel() : lut_(0)
{
}

extern "C" int ed_ncolor;
extern "C" u_char ed_cmap[];
extern "C" u_char ed_lut[];

inline int uverr(int uin, int vin, const ColorModel::color& c)
{
	int u = (int)(uin - c.u) & 0x1ff ^ 0x100;
	int v = (int)(vin - c.v) & 0x1ff ^ 0x100;
	return (0x100 | u << 10 | v << 21);
}

int EDColorModel::alloc_colors()
{
	if (installLUT(ed_ncolor, ed_cmap, ed_lut) < 0)
		return (-1);

	/*
	 * Map from colormap indices to allocated pixels.
	 */
	u_char* lut = PseudoColorModel::lut_;
#define NLUT (1 << (7+7+ED_YBITS))
	delete[] lut_; //SV-XXX: Debian
	lut_ = new u_int[NLUT];

	for (int v = 0; v < 1 << 7; ++v) {
		for (int u = 0; u < 1 << 7; ++u) {
			for (int y = 0; y < 1 << ED_YBITS; ++y) {
				u_int pix = lut[(v >> 1) << (ED_YBITS+6) |
						(u >> 1) << ED_YBITS | y];
				pix |= uverr(u << 1, v << 1, *lookup(pix));
				lut_[v << 11 | y << 7 | u] = pix;
			}
		}
	}
	return (0);
}

class EDWindowRenderer;

typedef void (EDWindowRenderer::*EDMethod)(const u_char*, u_int,
					   u_int, u_int, u_int) const;

class EDWindowRenderer : public PseudoWindowRenderer {
public:
	EDWindowRenderer(VideoWindow* vw, int decimation, EDColorModel& cm) :
		PseudoWindowRenderer(vw, decimation, cm.graylut()),
		cm_(cm) { }
	void render(const u_char* frm, int off, int x, int w, int h) {
		(this->*method_)(frm, off, x, w, h);
	}
protected:
	EDColorModel& cm_;
	virtual void update();
	virtual void disable() { method_ = &PseudoWindowRenderer::dither_null; }
	EDMethod method_;
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

int EDColorModel::command(int argc, const char*const* argv)
{
	if (argc == 4 && strcmp(argv[1], "renderer") == 0) {
		Tcl& tcl = Tcl::instance();
		VideoWindow* vw = VideoWindow::lookup(argv[2]);
		int decimation = atoi(argv[3]);
		Renderer* r = new EDWindowRenderer(vw, decimation, *this);
		tcl.result(r->name());
		return (TCL_OK);
	}
	return (PseudoColorModel::command(argc, argv));
}

void EDWindowRenderer::update()
{
	static EDMethod	methods[] = {
		&EDWindowRenderer::dither_up2_420,
		&EDWindowRenderer::dither_up2_422,
		&EDWindowRenderer::dither_gray_up,
		&EDWindowRenderer::dither_gray_up,
		&EDWindowRenderer::dither_420,
		&EDWindowRenderer::dither_422,
		&EDWindowRenderer::dither_gray,
		&EDWindowRenderer::dither_gray,
		&EDWindowRenderer::dither_down2_420,
		&EDWindowRenderer::dither_down2_422,
		&EDWindowRenderer::dither_gray_down,
		&EDWindowRenderer::dither_gray_down,
		&EDWindowRenderer::dither_down4_420,
		&EDWindowRenderer::dither_down4_422,
		&EDWindowRenderer::dither_gray_down,
		&EDWindowRenderer::dither_gray_down,
		&EDWindowRenderer::dither_down_420,
		&EDWindowRenderer::dither_down_422,
		&EDWindowRenderer::dither_gray_down,
		&EDWindowRenderer::dither_gray_down,
	};
	method_ = methods[index()];
}

/*
 * Declare and initialize variables for ONEPIX macro.
 * 'e' is the error term of the current pixel.  All three
 * errors are stored as parallel 8-bit quantities.
 * Two extra bits are used to detect overflow and underflow,
 * yielding 10 bits per component.  We only ever add
 * an 8-bit unsigned value (the input pixel) to a signed
 * delta.  The delta, d, is coded as follows:
 *
 *		01 xxxx xxxx  d >= 0
 *		00 xxxx xxxx  d < 0
 *
 * (i.e. "xxxx xxxx" is the 2's complement binary representation of d)
 *
 * Coding the deltas this way makes the under/overflow check cheap.
 * The two high bits will be 01 if there was no under/overflow,
 * i.e., no carry out from a positive delta, or a carry out from
 * a negative delta.  The high bits will be 10 on overflow, and
 * 00 on underflow.  So you can check the over/underflow condition
 * with bit 8, and you can tell which of over/under happened with bit 9.
 *
 * 'omask' is the parrallel set of under/overflow check bits.
 */
#define YERRMASK ((1 << (8 - ED_YBITS)) - 1) 
#define DIFFUSION_SETUP \
	register const u_int* lut = cm_.lut(); \
	u_int omask = (0x100 << 10) | (0x100 << 21) | 0x100; \
	/* start with random error */ \
	u_int e = random() & (YERRMASK | 0x1f << 10 | 0x1f << 21); \
	{ \
		/* sign extend uv errors */ \
		u_int s = e & (0x10 << 10 | 0x10 << 21); \
	  	e |= s << 1 | s << 2 | s << 3; \
		/* assume positive - set bit 8 */ \
		e |= omask; \
		/* clear bit 8 if negative */ \
	  	e &=~ s << 4; \
	}

/*
 * Process one pixel.  Add in the current pixel to the previous error,
 * checking for overflow.  Look up the closest color in the lookup
 * table.  Splice it into the output word, and compute the next error.
 *
 * We avoid most of the overflow fix-ups by doing a parallel check
 * of all the components.  If all of the overflow bits are one, we
 * pay only a couple instructions to check.  Less often, some of the
 * overflow bits are zero, so we do the more costly processing
 * limit operations.  Again they are done in parallel and only
 * when needed.  Bit 9, is replicated (8 times) and or'd in to the
 * error to set each component that overflowed to 255.  Likewise
 * bit 9 is complemented, replicated, and used to mask off each
 * component that underflowed, setting them to 0.
 *
 * The lookup table is indexed by a <V:7 U:7 Y:3> tuple.  We throw
 * away some bits on the Y axis to make the table smaller, but
 * recover them by allocating colors along uv-planes uniformly
 * spaced along the Y axis, and computing the Y diffusion error
 * directly (i.e., with a simple mask).  Effectively, a two dimensional
 * u-v dither is carried out independently of a one-dimensional y dither.
 * This allows us to allocate colors non-uniformly in the uv-planes
 * (i.e., to concentrate colors in the flesh tone region for better
 * rendition of faces).  Note that the Y error is always positive
 * (because we truncate down to the closest level), so bit 8 should
 * always be set.  This bit is actually stored in the lookup-table
 * and gets or'd in with the precomputed uv error.
 * So the lookup-table contains a pixel in the 8 lowest bits,
 * bit 8 to fix up the Y error term, and precomputed U and V
 * errors properly aligned.
 */
#define ONEPIX0(in, out) \
{ \
	e += in; \
	u_int m = e & omask; \
	m ^= omask; \
	if (m != 0) { \
		/* happens rarely */ \
		u_int oflo = e >> 1 & m; \
		if (oflo) { \
			oflo >>= 1; \
			oflo |= oflo >> 1; \
			oflo |= oflo >> 2; \
			oflo |= oflo >> 4; \
			e |= oflo; \
		} \
		u_int uflo = (e >> 1 ^ omask) & m; \
		if (uflo) { \
			uflo >>= 1; \
			uflo |= uflo >> 1; \
			uflo |= uflo >> 2; \
			uflo |= uflo >> 4; \
			e &=~ uflo; \
		} \
	} \
	/* XXX assume ED_YBITS=4*/ \
	t = e &~ (0xf << 18); \
	t |= (e << 14) & (0xf << 18); \
	t >>= 11; \
	t = lut[t & (1 << (7+7+ED_YBITS)) - 1]; \
	/* y error */ \
	e &= YERRMASK;  \
	/* uv error */ \
	e |= t & ~0xff; \
}

#define ONEPIX(in, out, off) \
{ \
	u_int t; \
	ONEPIX0(in, out); \
	SPLICE((out), t & 0xff, (off)); \
}
	
void EDWindowRenderer::dither_422(const u_char* frm, u_int off, u_int x,
				  u_int width, u_int height) const
{
        UNUSED(x); //SV-XXX: unused

	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_char* xip = pixbuf_ + off;

	DIFFUSION_SETUP;

	register u_int hiw = iw >> 1;
	for (; height > 0; height -= 4) {
		int w;
		for (w = width; w > 0; w -= 4) {
			register u_int uv0 = up[0] << 10 | vp[0] << 21;
			register u_int uv1 = up[hiw] << 10 | vp[hiw] << 21;
			register u_int out0 = 0;
			ONEPIX(yp[0] | uv0, out0, 24);
			register u_int out1 = 0;
			ONEPIX(yp[iw] | uv1, out1, 24);
			ONEPIX(yp[iw+1] | uv1, out1, 16);
			ONEPIX(yp[1] | uv0, out0, 16);
			
			uv0 = up[1] << 10 | vp[1] << 21;
			uv1 = up[hiw+1] << 10 | vp[hiw+1] << 21;
			ONEPIX(yp[2] | uv0, out0, 8);
			ONEPIX(yp[iw+2] | uv1, out1, 8);
			ONEPIX(yp[iw+3] | uv1, out1, 0);
			ONEPIX(yp[3] | uv0, out0, 0);

			*(u_int*)xip = out0;
			*(u_int*)(xip + iw) = out1;

			xip += 4;
			yp += 4;
			up += 2;
			vp += 2;
		}
		xip += iw << 1;
		yp += iw << 1;
		up += iw;
		vp += iw;
		for (w = width; w > 0; w -= 4) {
			xip -= 4;
			yp -= 4;
			up -= 2;
			vp -= 2;

			register u_int uv0 = up[1] << 10 | vp[1] << 21;
			register u_int uv1 = up[hiw+1] << 10 | 
				vp[hiw+1] << 21;
			register u_int out0 = 0;
			ONEPIX(yp[3] | uv0, out0, 0);
			register u_int out1 = 0;
			ONEPIX(yp[iw+3] | uv1, out1, 0);
			ONEPIX(yp[iw+2] | uv1, out1, 8);
			ONEPIX(yp[2] | uv0, out0, 8);
			
			uv0 = up[0] << 10 | vp[0] << 21;
			uv1 = up[hiw] << 10 | vp[hiw] << 21;
			ONEPIX(yp[1] | uv0, out0, 16);
			ONEPIX(yp[iw+1] | uv1, out1, 16);
			ONEPIX(yp[iw] | uv1, out1, 24);
			ONEPIX(yp[0] | uv0, out0, 24);

			*(u_int*)xip = out0;
			*(u_int*)(xip + iw) = out1;
		}
		xip += iw << 1;
		yp += iw << 1;
		up += iw;
		vp += iw;
	}
}

void EDWindowRenderer::dither_down2_422(const u_char* frm,
					u_int off, u_int x,
					u_int width, u_int height) const
{
#ifndef TESTING
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_char* xip = pixbuf_ + ((off - x) >> 2) + (x >> 1);

	DIFFUSION_SETUP;

	register u_int diw = iw << 1;
	for (; height > 0; height -= 8) {
		int w;
		for (w = width; w > 0; w -= 8) {
			register u_int uv = up[0] << 10 | vp[0] << 21;
			register u_int out0 = 0;
			ONEPIX(yp[0] | uv, out0, 24);
			uv = up[iw] << 10 | vp[iw] << 21;
			register u_int out1 = 0;
			ONEPIX(yp[diw] | uv, out1, 24);
			
			uv = up[1] << 10 | vp[1] << 21;
			ONEPIX(yp[2] | uv, out0, 16);
			uv = up[iw+1] << 10 | vp[iw+1] << 21;
			ONEPIX(yp[diw+2] | uv, out1, 16);

			uv = up[2] << 10 | vp[2] << 21;
			ONEPIX(yp[4] | uv, out0, 8);
			uv = up[iw+2] << 10 | vp[iw+2] << 21;
			ONEPIX(yp[diw+4] | uv, out1, 8);

			uv = up[3] << 10 | vp[3] << 21;
			ONEPIX(yp[6] | uv, out0, 0);
			uv = up[iw+3] << 10 | vp[iw+3] << 21;
			ONEPIX(yp[diw+6] | uv, out1, 0);

			*(u_int*)xip = out0;
			*(u_int*)(xip + (iw >> 1)) = out1;

			xip += 4;
			yp += 8;
			up += 4;
			vp += 4;
		}
		yp += diw << 1;
		up += diw;
		vp += diw;
		xip += iw;
		for (w = width; w > 0; w -= 8) {
			xip -= 4;
			yp -= 8;
			up -= 4;
			vp -= 4;
			
			register u_int uv = up[3] << 10 | vp[3] << 21;
			register u_int out0 = 0;
			ONEPIX(yp[6] | uv, out0, 0);
			uv = up[iw+3] << 10 | vp[iw+3] << 21;
			register u_int out1 = 0;
			ONEPIX(yp[diw+3] | uv, out1, 0);
			
			uv = up[2] << 10 | vp[2] << 21;
			ONEPIX(yp[4] | uv, out0, 8);
			uv = up[iw+2] << 10 | vp[iw+2] << 21;
			ONEPIX(yp[diw+4] | uv, out1, 8);

			uv = up[1] << 10 | vp[1] << 21;
			ONEPIX(yp[2] | uv, out0, 16);
			uv = up[iw+1] << 10 | vp[iw+1] << 21;
			ONEPIX(yp[diw+2] | uv, out1, 16);

			uv = up[0] << 10 | vp[0] << 21;
			ONEPIX(yp[0] | uv, out0, 24);
			uv = up[iw] << 10 | vp[iw] << 21;
			ONEPIX(yp[diw] | uv, out1, 24);

			*(u_int*)xip = out0;
			*(u_int*)(xip + (iw >> 1)) = out1;
		}
		yp += diw << 1;
		up += diw;
		vp += diw;
		xip += iw;
	}
#endif
}

void EDWindowRenderer::dither_down4_422(const u_char* frm,
					u_int off, u_int x,
					u_int width, u_int height) const
{
#ifndef TESTING
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_char* xip = pixbuf_ + ((off - x) >> 4) + (x >> 2);

	DIFFUSION_SETUP;

	for (; height > 0; height -= 8) {
		int w;
		for (w = width; w > 0; w -= 16) {
			register u_int t;
			register u_int uv = up[0] << 10 | vp[0] << 21;
			ONEPIX0(yp[0] | uv, out0);
			xip[0] = t;
			uv = up[2] << 10 | vp[2] << 21;
			ONEPIX0(yp[4] | uv, out0);
			xip[1] = t;
			uv = up[4] << 10 | vp[4] << 21;
			ONEPIX0(yp[8] | uv, out0);
			xip[2] = t;
			uv = up[6] << 10 | vp[6] << 21;
			ONEPIX0(yp[12] | uv, out0);
			xip[3] = t;

			xip += 4;
			yp += 16;
			up += 8;
			vp += 8;
		}
		yp += iw << 2;
		up += iw << 1;
		vp += iw << 1;
		xip += iw >> 2;
		for (w = width; w > 0; w -= 16) {
			xip -= 4;
			yp -= 16;
			up -= 8;
			vp -= 8;
			
			register u_int t;
			register u_int uv = up[6] << 10 | vp[6] << 21;
			ONEPIX0(yp[12] | uv, out0);
			xip[3] = t;
			uv = up[4] << 10 | vp[4] << 21;
			ONEPIX0(yp[8] | uv, out0);
			xip[2] = t;
			uv = up[2] << 10 | vp[2] << 21;
			ONEPIX0(yp[4] | uv, out0);
			xip[1] = t;
			uv = up[0] << 10 | vp[0] << 21;
			ONEPIX0(yp[0] | uv, out0);
			xip[0] = t;
		}
		yp += iw << 2;
		up += iw << 1;
		vp += iw << 1;
		xip += iw >> 2;
	}
#endif
}

void EDWindowRenderer::dither_down_422(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
#ifndef TESTING
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register int s = scale_;
	register int is = 1 << s;
	register int is3 = (is << 1) + is;
	register u_char* xip = pixbuf_ + ((off - x) >> (s + s)) + (x >> s);

	DIFFUSION_SETUP;

	for (; height > 0; height -= is << 1) {
		int w;
		for (w = width; w > 0; w -= is << 2) {
			register u_int uv = up[0] << 10 | vp[0] << 21;
			register u_int t;
			ONEPIX0(yp[0] | uv, out0);
			xip[0] = t;
			uv = up[is >> 1] << 10 | vp[is >> 1] << 21;
			ONEPIX0(yp[is] | uv, out0);
			xip[1] = t;
			uv = up[s] << 10 | vp[is] << 21;
			ONEPIX0(yp[is << 1] | uv, out0);
			xip[2] = t;
			uv = up[is3 >> 1] << 10 | vp[is3 >> 1] << 21;
			ONEPIX0(yp[is3] | uv, out0);
			xip[3] = t;

			xip += 4;
			yp += is << 2;
			up += is << 1;
			vp += is << 1;
		}
		yp += iw << s;
		up += iw << (s - 1);
		vp += iw << (s - 1);
		xip += iw >> s;
		for (w = width; w > 0; w -= is << 2) {
			xip -= 4;
			yp -= is << 2;
			up -= is << 1;
			vp -= is << 1;
			
			register u_int uv = up[is3 >> 1] << 10 | 
				vp[is3 >> 1] << 21;
			register int t;
			ONEPIX0(yp[is3] | uv, out0);
			xip[3] = t;
			uv = up[is] << 10 | vp[is] << 21;
			ONEPIX0(yp[is << 1] | uv, out0);
			xip[2] = t;
			uv = up[is >> 1] << 10 | vp[is >> 1] << 21;
			ONEPIX0(yp[is] | uv, out0);
			xip[1] = t;
			uv = up[0] << 10 | vp[0] << 21;
			ONEPIX0(yp[0] | uv, out0);
			xip[0] = t;
		}
		yp += iw << s;
		up += iw << (s - 1);
		vp += iw << (s - 1);
		xip += iw >> s;
	}
#endif
}

void EDWindowRenderer::dither_up2_422(const u_char* frm,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
#ifndef TESTING
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);
	register u_char* xip = pixbuf_ + ((off - x) << 2) + (x << 1);

	DIFFUSION_SETUP;

	for (; height > 0; height -= 2) {
		int w;
		for (w = width; w > 0; w -= 2) {

			register u_int uv = up[0] << 10 | vp[0] << 21;
			register u_int out0 = 0;
			ONEPIX(yp[0] | uv, out0, 24);
			register u_int out1 = 0;
			ONEPIX(yp[0] | uv, out1, 24);
			ONEPIX(yp[0] | uv, out1, 16);
			ONEPIX(yp[0] | uv, out0, 16);

			ONEPIX(yp[1] | uv, out0, 8);
			ONEPIX(yp[1] | uv, out1, 8);
			ONEPIX(yp[1] | uv, out1, 0);
			ONEPIX(yp[1] | uv, out0, 0);

			*(u_int*)xip = out0;
			*(u_int*)(xip + (iw << 1)) = out1;

			xip += 4;
			yp += 2;
			up += 1;
			vp += 1;

		}
		yp += iw;
		up += iw >> 1;
		vp += iw >> 1;
		xip += iw << 2;

		for (w = width; w > 0; w -= 2) {
			xip -= 4;
			yp -= 2;
			up -= 1;
			vp -= 1;
			
			register u_int uv = up[0] << 10 | vp[0] << 21;
			register u_int out0 = 0;
			ONEPIX(yp[1] | uv, out0, 0);
			register u_int out1 = 0;
			ONEPIX(yp[1] | uv, out1, 0);
			ONEPIX(yp[1] | uv, out1, 8);
			ONEPIX(yp[1] | uv, out0, 8);

			ONEPIX(yp[0] | uv, out0, 16);
			ONEPIX(yp[0] | uv, out1, 16);
			ONEPIX(yp[0] | uv, out1, 24);
			ONEPIX(yp[0] | uv, out0, 24);

			*(u_int*)xip = out0;
			*(u_int*)(xip + (iw << 1)) = out1;
		}
		yp += iw;
		up += iw >> 1;
		vp += iw >> 1;
		xip += iw << 2;
	}
#endif
}

void EDWindowRenderer::dither_420(const u_char* frm,
				  u_int off, u_int x,
				  u_int width, u_int height) const
{
#ifndef TESTING
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register u_char* xip = pixbuf_ + off;

	DIFFUSION_SETUP;

	for (; height > 0; height -= 4) {
		int w;
		for (w = width; w > 0; w -= 4) {
			register u_int out0 = 0;
			register u_int out1 = 0;

			register u_int uv = up[0] << 10 | vp[0] << 21;
			ONEPIX(yp[0] | uv, out0, 24);
			ONEPIX(yp[iw] | uv, out1, 24);
			ONEPIX(yp[iw+1] | uv, out1, 16);
			ONEPIX(yp[1] | uv, out0, 16);
			
			uv = up[1] << 10 | vp[1] << 21;
			ONEPIX(yp[2] | uv, out0, 8);
			ONEPIX(yp[iw+2] | uv, out1, 8);
			ONEPIX(yp[iw+3] | uv, out1, 0);
			ONEPIX(yp[3] | uv, out0, 0);

			*(u_int*)xip = out0;
			*(u_int*)(xip + iw) = out1;

			xip += 4;
			yp += 4;
			up += 2;
			vp += 2;
		}
		yp += iw << 1;
		up += iw >> 1;
		vp += iw >> 1;
		xip += iw << 1;
		for (w = width; w > 0; w -= 4) {
			xip -= 4;
			yp -= 4;
			up -= 2;
			vp -= 2;
			register u_int out0 = 0;
			register u_int out1 = 0;

			register u_int uv = up[1] << 10 | vp[1] << 21;
			ONEPIX(yp[3] | uv, out0, 0);
			ONEPIX(yp[iw+3] | uv, out1, 0);
			ONEPIX(yp[iw+2] | uv, out1, 8);
			ONEPIX(yp[2] | uv, out0, 8);
			
			uv = up[0] << 10 | vp[0] << 21;
			ONEPIX(yp[1] | uv, out0, 16);
			ONEPIX(yp[iw+1] | uv, out1, 16);
			ONEPIX(yp[iw] | uv, out1, 24);
			ONEPIX(yp[0] | uv, out0, 24);

			*(u_int*)xip = out0;
			*(u_int*)(xip + iw) = out1;
		}
		yp += iw << 1;
		up += iw >> 1;
		vp += iw >> 1;
		xip += iw << 1;
	}
#endif
}

void EDWindowRenderer::dither_down2_420(const u_char* frm, u_int off, u_int x,
					u_int width, u_int height) const
{
#ifndef TESTING
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	off = ((off - x) >> 2) + (x >> 1);
	register const u_char* up = frm + framesize_ + off;
	register const u_char* vp = up + (framesize_ >> 2);
	register u_char* xip = pixbuf_ + off;

	DIFFUSION_SETUP;

	register u_int diw = iw << 1;
	register u_int hiw = iw >> 1;
	for (; height > 0; height -= 8) {
		int w;
		for (w = width; w > 0; w -= 8) {
			register u_int out0 = 0;
			register u_int out1 = 0;

			register u_int uv = up[0] << 10 | vp[0] << 21;
			ONEPIX(yp[0] | uv, out0, 24);
			uv = up[iw] << 10 | vp[iw] << 21;
			ONEPIX(yp[diw] | uv, out1, 24);
			
			uv = up[1] << 10 | vp[1] << 21;
			ONEPIX(yp[2] | uv, out0, 16);
			uv = up[iw+1] << 10 | vp[iw+1] << 21;
			ONEPIX(yp[diw+2] | uv, out1, 16);

			uv = up[2] << 10 | vp[2] << 21;
			ONEPIX(yp[4] | uv, out0, 8);
			uv = up[iw+2] << 10 | vp[iw+2] << 21;
			ONEPIX(yp[diw+4] | uv, out1, 8);

			uv = up[3] << 10 | vp[3] << 21;
			ONEPIX(yp[6] | uv, out0, 0);
			uv = up[iw+3] << 10 | vp[iw+3] << 21;
			ONEPIX(yp[diw+6] | uv, out1, 0);

			*(u_int*)xip = out0;
			*(u_int*)(xip + hiw) = out1;

			xip += 4;
			yp += 8;
			up += 4;
			vp += 4;
		}
		yp += diw << 1;
		up += iw;
		vp += iw;
		xip += iw;
		for (w = width; w > 0; w -= 8) {
			xip -= 4;
			yp -= 8;
			up -= 4;
			vp -= 4;
			
			register u_int out0 = 0;
			register u_int out1 = 0;

			register u_int uv = up[3] << 10 | vp[3] << 21;
			ONEPIX(yp[6] | uv, out0, 0);
			register u_int uv1 = up[iw+3] << 10 | vp[iw+3] << 21;
			ONEPIX(yp[diw+3] | uv1, out1, 0);
			
			uv = up[2] << 10 | vp[2] << 21;
			ONEPIX(yp[4] | uv, out0, 8);
			uv1 = up[iw+2] << 10 | vp[iw+2] << 21;
			ONEPIX(yp[diw+4] | uv1, out1, 8);

			uv = up[1] << 10 | vp[1] << 21;
			ONEPIX(yp[2] | uv, out0, 16);
			uv1 = up[iw+1] << 10 | vp[iw+1] << 21;
			ONEPIX(yp[diw+2] | uv1, out1, 16);

			uv = up[0] << 10 | vp[0] << 21;
			ONEPIX(yp[0] | uv, out0, 24);
			uv1 = up[iw] << 10 | vp[iw] << 21;
			ONEPIX(yp[diw] | uv1, out1, 24);

			*(u_int*)xip = out0;
			*(u_int*)(xip + hiw) = out1;
		}
		yp += diw << 1;
		up += iw;
		vp += iw;
		xip += iw;
	}
#endif
}

void EDWindowRenderer::dither_down4_420(const u_char* frm,
					u_int off, u_int x,
					u_int width, u_int height) const
{
#ifndef TESTING
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register u_char* xip = pixbuf_ + ((off - x) >> 4) + (x >> 2);

	DIFFUSION_SETUP;

	for (; height > 0; height -= 8) {
		int w;
		for (w = width; w > 0; w -= 16) {
			register u_int uv = up[0] << 10 | vp[0] << 21;
			register u_int t;
			ONEPIX0(yp[0] | uv, out0);
			xip[0] = t;
			uv = up[2] << 10 | vp[2] << 21;
			ONEPIX0(yp[4] | uv, out0);
			xip[1] = t;
			uv = up[4] << 10 | vp[4] << 21;
			ONEPIX0(yp[8] | uv, out0);
			xip[2] = t;
			uv = up[6] << 10 | vp[6] << 21;
			ONEPIX0(yp[12] | uv, out0);
			xip[3] = t;

			xip += 4;
			yp += 16;
			up += 8;
			vp += 8;
		}
		yp += iw << 2;
		up += iw;
		vp += iw;
		xip += iw >> 2;
		for (w = width; w > 0; w -= 16) {
			xip -= 4;
			yp -= 16;
			up -= 8;
			vp -= 8;
			
			register u_int uv0 = up[6] << 10 | vp[6] << 21;
			register u_int t;
			ONEPIX0(yp[12] | uv0, out0);
			xip[3] = t;
			uv0 = up[4] << 10 | vp[4] << 21;
			ONEPIX0(yp[8] | uv0, out0);
			xip[2] = t;
			uv0 = up[2] << 10 | vp[2] << 21;
			ONEPIX0(yp[4] | uv0, out0);
			xip[1] = t;
			uv0 = up[0] << 10 | vp[0] << 21;
			ONEPIX0(yp[0] | uv0, out0);
			xip[0] = t;
		}
		yp += iw << 2;
		up += iw;
		vp += iw;
		xip += iw >> 2;
	}
#endif
}

void EDWindowRenderer::dither_down_420(const u_char* frm,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
#ifndef TESTING
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register int s = scale_;
	register int istride = 1 << s;
	register int istride3 = (istride << 1) + istride;
	register u_char* xip = pixbuf_ + ((off - x) >> (s + s)) + (x >> s);

	DIFFUSION_SETUP;

	for (; height > 0; height -= istride << 1) {
		int w;
		for (w = width; w > 0; w -= istride << 2) {
			register u_int uv = up[0] << 10 | vp[0] << 21;
			register u_int t;
			ONEPIX0(yp[0] | uv, out0);
			xip[0] = t;
			uv = up[istride >> 1] << 10 | vp[istride >> 1] << 21;
			ONEPIX0(yp[istride] | uv, out0)
			xip[1] = t;
			uv = up[istride] << 10 | vp[istride] << 21;
			ONEPIX0(yp[istride << 1] | uv, out0);
			xip[2] = t;
			uv = up[istride3 << 1] << 10 | vp[istride3 << 1] << 21;
			ONEPIX0(yp[istride3] | uv, out0);
			xip[3] = t;

			xip += 4;
			yp += istride << 2;
			up += istride << 1;
			vp += istride << 1;
		}
		yp += iw << s;
		up += iw << (s - 2);
		vp += iw << (s - 2);
		xip += iw >> 2;
		for (w = width; w > 0; w -= istride << 2) {
			xip -= 4;
			yp -= istride << 2;
			up -= istride << 1;
			vp -= istride << 1;
			
			register u_int uv = up[istride3 << 1] << 10 | 
				vp[istride3 << 1] << 21;
			register u_int t;
			ONEPIX0(yp[istride3] | uv, out0);
			xip[3] = t;
			uv = up[istride] << 10 | vp[istride] << 21;
			ONEPIX0(yp[istride << 1] | uv, out0);
			xip[2] = t;
			uv = up[istride >> 1] << 10 | vp[istride >> 1] << 21;
			ONEPIX0(yp[istride] | uv, out0);
			xip[1] = t;
			uv = up[0] << 10 | vp[0] << 21;
			ONEPIX0(yp[0] | uv, out0);
			xip[0] = t;
		}
		yp += iw << s;
		up += iw << (s - 2);
		vp += iw << (s - 2);
		xip += iw >> 2;
	}
#endif
}

void EDWindowRenderer::dither_up2_420(const u_char* frm,
				      u_int off, u_int x,
				      u_int width, u_int height) const
{
#ifndef TESTING
	register u_int iw = width_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + ((off - x) >> 2) + (x >> 1);
	register const u_char* vp = up + (framesize_ >> 2);
	register u_char* xip = pixbuf_ + ((off - x) << 2) + (x << 1);

	DIFFUSION_SETUP;

	for (; height > 0; height -= 2) {
		int w;
		for (w = width; w > 0; w -= 2) {
			register u_int out0 = 0;
			register u_int out1 = 0;

			register u_int uv = up[0] << 10 | vp[0] << 21;
			ONEPIX(yp[0] | uv, out0, 24);
			ONEPIX(yp[0] | uv, out1, 24);
			ONEPIX(yp[0] | uv, out1, 16);
			ONEPIX(yp[0] | uv, out0, 16);

			ONEPIX(yp[1] | uv, out0, 8);
			ONEPIX(yp[1] | uv, out1, 8);
			ONEPIX(yp[1] | uv, out1, 0);
			ONEPIX(yp[1] | uv, out0, 0);

			*(u_int*)xip = out0;
			*(u_int*)(xip + (iw << 1)) = out1;

			xip += 4;
			yp += 2;
			up += 1;
			vp += 1;
		}
		yp += iw;
		xip += iw << 2;

		for (w = width; w > 0; w -= 2) {
			xip -= 4;
			yp -= 2;
			up -= 1;
			vp -= 1;
			
			register u_int out0 = 0;
			register u_int out1 = 0;

			register u_int uv = up[0] << 10 | vp[0] << 21;
			ONEPIX(yp[1] | uv, out0, 0);
			ONEPIX(yp[1] | uv, out1, 0);
			ONEPIX(yp[1] | uv, out1, 8);
			ONEPIX(yp[1] | uv, out0, 8);

			ONEPIX(yp[0] | uv, out0, 16);
			ONEPIX(yp[0] | uv, out1, 16);
			ONEPIX(yp[0] | uv, out1, 24);
			ONEPIX(yp[0] | uv, out0, 24);

			*(u_int*)xip = out0;
			*(u_int*)(xip + (iw << 1)) = out1;
		}
		yp += iw;
		up += iw >> 1;
		vp += iw >> 1;
		xip += iw << 2;
	}
#endif
}
