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
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
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
#include "renderer-window.h"
#include "bsd-endian.h"
#include "vic_tcl.h"
#include "vw.h"

/* From nv */
static u_char halftone[1024] =
    { 108, 217, 192,  15, 103,  65, 129,  17, 
      191, 151, 206, 136, 182,  14, 197, 135, 
       54, 216,  96, 139, 157,  54, 190, 214, 
       45,  90, 199,   4, 253,  30,  82, 244, 
        1, 147,  87, 227, 179,  34, 240, 214, 
       49, 231,   2,  61, 235, 113, 156,  28, 
      170, 118, 185,  38, 236, 117,  76, 167, 
       12, 224,  53, 147, 102, 166, 207,  62, 
      116, 168,  29,  56, 114, 155,  79, 108, 
      167,  95, 126, 171,  86,  41, 252,  92, 
      225,   2, 247,  84,  17, 204,  29, 250, 
      109, 128, 186,  75, 215,  47, 135, 188, 
       43, 245, 203, 139, 233, 193,   8, 140, 
       33,  71, 248,  26, 216, 137,  58, 203, 
       74, 144,  49, 163, 132, 178,  93, 141, 
       62, 162,  22, 230, 123,   9,  92, 228, 
       78, 129,  93,  22,  74,  47, 254, 178, 
      222, 195, 149, 103, 177,   7, 159, 121, 
       21, 180, 209,  99, 221,  57, 231,  40, 
      208, 240,  98,  39, 174, 246, 161,  21, 
      221,  58, 185, 157, 218, 127,  99,  60, 
      115,  11,  53, 209,  68, 233, 194,  98, 
      240, 127,  68,  23, 115,  10, 150, 191, 
        0,  80, 145, 198, 110,  61, 144, 200, 
      173,  14, 249, 109,   3, 172, 205,  26, 
      153, 243, 131,  36, 122,  85,  18,  51, 
      168,  34, 228, 185, 253, 172,  67, 105, 
      125, 183,  57,  16, 212,  83,  29, 104, 
      120, 141,  40, 197,  84,  43, 239,  77, 
      191,  89, 166, 189, 220, 152, 248, 114, 
      217,  86, 148,  48,  80, 130, 212, 242, 
       44, 225, 155, 254, 132, 180, 241,  48, 
       87, 235,  69, 148, 228, 131, 162, 112, 
       17, 229,  64,   1, 100,  31, 177,  63, 
      138,   3, 199, 110, 160,  37,  16,  91, 
      167,  25, 114,  90,  46,   3, 157, 218, 
       10, 202, 166,  19,  97,  58,  30, 210, 
      146,  47, 111, 253, 145,  76, 208,  38, 
      189,  94, 246,  19, 206, 234, 187,  62, 
      140, 208,  71, 192, 233, 109,  73, 188, 
      125,  55, 111, 219, 185, 252, 174,  72, 
      223, 183, 128, 200,  20, 234, 159, 121, 
      230, 171,  57, 131,  71,  99, 149, 118, 
      246,  13, 176,  33, 151, 213, 140,  37, 
      153, 248,  78,  34, 123,  11, 137, 103, 
        6,  84,  35, 169,  92,  50, 105,   9, 
       74,  32, 154, 227, 179,  42,   6, 196, 
       48, 100, 224, 122,  59,  20,  95, 232, 
      180,  17, 195, 158,  88, 207,  44, 197, 
      244, 160, 230,  67, 218, 142, 196, 245, 
      136, 215, 104,  23, 122, 252, 162, 215, 
       75, 137, 163,  81, 251, 170, 201,  68, 
       45, 100, 138, 241,  52, 228, 150,  64, 
      121,  23, 136, 106,   5, 176,  36,  86, 
      167,  49, 190,  83, 207,  55,  87, 115, 
       31, 234,   7, 205,  40, 134,   5, 115, 
      225, 204,  66,   0, 169, 106,  15, 177, 
       90, 209,  52, 189, 243, 116,  63, 202, 
       19, 120, 237,   3, 142, 173,  23, 241, 
      156, 187,  53, 111, 182,  91, 242, 164, 
      143,  26, 126, 219,  77, 133, 193, 251, 
       36, 144, 233,  82,  29, 156, 226, 143, 
      249,  97, 156,  69, 225, 107, 194, 135, 
       63,  96, 218, 145,  65, 214,  32,  79, 
      176, 254,  89, 184,  37, 231,  25,  69, 
      108, 164,  11, 182, 130,  47,  91,  14, 
       73, 179,  30, 188,  44,  78,  10, 211, 
       42, 128,  27, 237,  12, 158, 123,  56, 
        8, 110,  50, 148, 203,  96, 161, 129, 
      205, 224,  59, 102, 254, 169, 213, 190, 
      128,  56, 215, 125, 250, 147, 231,  93, 
      180, 254, 165,  83, 187, 101, 232, 191, 
      222, 160, 212,  12, 119,  61, 239,   4, 
       81,  41, 124, 199,  72,   0, 109,  38, 
      154, 235, 103,  20, 170, 114,  33, 158, 
       71,   9, 107, 207,  40, 143,  22,  86, 
      130,  36,  73, 247, 174,  32, 195, 145, 
      181, 237, 153,  28, 217, 138, 238,  80, 
      200,   7, 147,  85,  64, 208,  55, 198, 
      125, 221,  52, 134,  72, 247, 202,  59, 
      242, 104, 198, 136,  88, 220, 112,  46, 
       98,  15, 175,  94,  54, 165,  22, 120, 
      172,  49, 210, 243, 181,   1, 236, 104, 
       21, 183, 155, 227,   4, 171, 113, 153, 
      183,   2, 166,  51,  20, 157,  75, 250, 
      201, 121,  74, 246, 192, 101, 223,  65, 
      252,  93, 116,  31, 127, 155,  87, 144, 
      244,  79,  33, 117, 190,  90,  46,  24, 
       69,  94, 226, 118, 238, 188,   8, 133, 
       60, 229,  39, 133,  16, 152,  44, 181, 
      141,  12, 163, 196,  72, 226,  42, 175, 
       58, 211,  98, 251,  62, 139, 236, 211, 
      134, 201,  35, 154,  67, 102, 213, 168, 
       25, 150, 181, 216,  85, 205, 124,  24, 
      226,  81, 214,  50, 106,  18, 202, 118, 
        6, 135, 162,  16, 203,  31, 165, 108, 
       51, 243,  82, 192,  27, 141,  43,  84, 
      222, 101,  10, 113,  55, 241,  70, 194, 
      112,  38, 146, 239, 184, 138, 253,  76, 
      193, 234,  51, 179, 122,  73, 220,  13, 
      142, 168,   9, 126, 249, 175, 229, 119, 
      196,  67, 254, 143, 174,   2, 158,  92, 
      248, 169,  66,   4,  89, 165,  26, 101, 
      152,  35,  81, 105, 240, 154,  95, 186, 
       66, 219, 111, 210,  60,  95,   1,  52, 
      159,  28, 189,  41,  89, 211, 129,  48, 
       13, 200, 131, 223, 119,  45, 219,  61, 
      209, 124, 186, 212,   0,  45, 229,  28, 
      254,  91,  43, 164,  24, 151, 204, 245, 
      133,  99, 232, 120, 224,  30, 186, 235, 
      150, 102,  25, 187,  75, 242, 132, 175, 
        8, 249,  27, 146,  64, 132, 197, 116, 
        5, 146, 182,  76, 237, 124,  70, 177, 
       13,  77, 171,  59, 149,  80, 110,  63, 
      216,  82, 232,  53, 152,  15, 199,  85, 
      113,  54, 164,  96, 244, 176,  78, 161, 
       60, 198,  21, 220,  97, 193,  37, 106, 
      217, 142,  24, 201,   7, 250, 178,  18, 
      139,  42, 172, 126, 210, 107,  39, 160, 
      227, 195,  79, 221,  14, 107,  35, 213, 
       94, 238, 117, 137,  50,   6, 148, 236, 
       57, 184, 245, 105, 161, 127,  39, 206, 
      163, 112, 254,  27,  88, 178, 247,  65, 
       18, 123,  34, 140, 204,  56, 230, 130, 
      173,  41,  70, 159, 251, 206, 170,  83, 
      119,  32,  88,  46, 222,  68,  97, 238, 
       77,  11, 194,  66, 223,   5, 134, 100, 
      151, 239, 173,  70, 117, 184, 149,  19 };

class MonoColorModel : public ColorModel {
public:
	MonoColorModel();
	~MonoColorModel();
	virtual int alloc_grays();
	virtual int command(int argc, const char*const* argv);
	inline int white(void) { return (white_); }
private:
	int white_;
};

class MonoColorMatcher : public Matcher {
public:
	MonoColorMatcher() : Matcher("colormodel") {}
	virtual TclObject* match(const char* id) {
		if (strcasecmp(id, "staticgray/1") == 0) {
			return (new MonoColorModel);
		}
		return (0);
	}
} matcher_mono;

MonoColorModel::MonoColorModel()
{
	Tk_Window tk = Tcl::instance().tkmain();
	white_ = WhitePixel(Tk_Display(tk), Tk_ScreenNumber(tk)) ^ 1;
	white_ |= white_ << 1;
	white_ |= white_ << 2;
	white_ |= white_ << 4;
}

MonoColorModel::~MonoColorModel()
{
	/*XXX*/
}

int MonoColorModel::alloc_grays()
{
	return (0);
}

class MonoWindowRenderer;

typedef void (MonoWindowRenderer::*MonoMethod)(const u_char*, u_int,
					       u_int, u_int, u_int) const;

class MonoWindowRenderer : public WindowDitherer {
public:
	MonoWindowRenderer(VideoWindow* vw, int decimation, MonoColorModel& cm) :
		WindowDitherer(vw, decimation), cm_(cm) { }
	void render(const u_char* frm, int off, int x, int w, int h) {
		(this->*method_)(frm, off, x, w, h);
	}
protected:
	MonoColorModel& cm_;
	virtual void update();
	virtual void disable() { method_ = &WindowDitherer::dither_null; }
	MonoMethod method_;
	void dither(const u_char* frm, u_int off, u_int x,
		    u_int width, u_int height) const;
	void dither_up(const u_char* frm, u_int off, u_int x,
		       u_int width, u_int height) const;
	void dither_down(const u_char* frm, u_int off, u_int x,
			 u_int width, u_int height) const;
};

int MonoColorModel::command(int argc, const char*const* argv)
{
	if (argc == 4 && strcmp(argv[1], "renderer") == 0) {
		Tcl& tcl = Tcl::instance();
		VideoWindow* vw = VideoWindow::lookup(argv[2]);
		int decimation = atoi(argv[3]);
		Renderer* r = new MonoWindowRenderer(vw, decimation, *this);
		tcl.result(r->name());
		return (TCL_OK);
	}
	return (ColorModel::command(argc, argv));
}

void MonoWindowRenderer::update()
{
	if (scale_ > 0)
		method_ = &MonoWindowRenderer::dither_down;
	else if (scale_ < 0)
		method_ = &MonoWindowRenderer::dither_up;
	else
		method_ = &MonoWindowRenderer::dither;
}

#define ONEPIX(out, in, n, thresh)  \
	if ((in) > (thresh)) { \
		(out) |= (0x100 >> (n)); \
	}

void MonoWindowRenderer::dither_up(const u_char* p, u_int off,
				   u_int x, u_int width, u_int height) const
{
	p += off;
	u_char* xip = &pixbuf_[(((off - x) << 2) + (x << 1)) >> 3];
	int y = ((off - x) / width_) << 1;
	x <<= 1;
	const int stride = (width_ << 1) >> 3;
	u_char* xstart = xip;
	int ystop = y + (height << 1);
	int xstop = x + (width << 1);
	int white = cm_.white();
	for (; y < ystop; y += 2) {
		for (int x0 = x; x0 < xstop; x0 += 8) {
			int r = (y & 0x1f) << 5 | (x0 & 0x1f);
			u_char* h0 = &halftone[r];
			u_char* h1 = &halftone[(r + (1 << 5)) & 0x3ff];
			u_char out0 = 0;
			u_char out1 = 0;
			ONEPIX(out0, p[0], 1, h0[0]);
			ONEPIX(out0, p[0], 2, h0[1]);
			ONEPIX(out1, p[0], 1, h1[0]);
			ONEPIX(out1, p[0], 2, h1[1]);
			ONEPIX(out0, p[1], 3, h0[2]);
			ONEPIX(out0, p[1], 4, h0[3]);
			ONEPIX(out1, p[1], 3, h1[2]);
			ONEPIX(out1, p[1], 4, h1[3]);
			ONEPIX(out0, p[2], 5, h0[4]);
			ONEPIX(out0, p[2], 6, h0[5]);
			ONEPIX(out1, p[2], 5, h1[4]);
			ONEPIX(out1, p[2], 6, h1[5]);
			ONEPIX(out0, p[3], 7, h0[6]);
			ONEPIX(out0, p[3], 8, h0[7]);
			ONEPIX(out1, p[3], 7, h1[6]);
			ONEPIX(out1, p[3], 8, h1[7]);

			xip[0] = out0 ^ white;
			xip[stride] = out1 ^ white;
			
			p += 4;
			xip += 1;
		}
		p += width_ - width;
		xip = xstart + (stride << 1);
		xstart = xip;
	}
}

void MonoWindowRenderer::dither_down(const u_char* p, u_int off,
				     u_int x, u_int width, u_int height) const
{
	int s = scale_;
	u_int m = ((1 << (s + 3)) - 1);
	off &=~ m;
	x &=~ m;
	width = (width + m) & ~m;
	p += off;
	register int scaler = 1 << s;
	const int stride = width_ >> s;
	register u_char* xip = &pixbuf_[(((off - x) >> (s + s)) +
					 (x >> s)) >> 3];
	u_char* xstart = xip;
	int white = cm_.white();
	int y = ((off - x) / width_) >> s;
	x >>= s;
	int ystop = y + (((height >> s) + 7) & ~7);
	int xstop = x + (((width >> s) + 7) & ~7);
	for (; y < ystop; ++y) {
		const u_char* pstart = p;
		for (int x0 = x; x0 < xstop; x0 += 8) {
			u_char out = 0;
                        int r = (y & 0x1f) << 5 | (x0 & 0x1f);
			u_char* ep = &halftone[r];
			ONEPIX(out, *p, 1, ep[0]);
			p += scaler;
			ONEPIX(out, *p, 2, ep[1]);
			p += scaler;
			ONEPIX(out, *p, 3, ep[2]);
			p += scaler;
			ONEPIX(out, *p, 4, ep[3]);
			p += scaler;
			ONEPIX(out, *p, 5, ep[4]);
			p += scaler;
			ONEPIX(out, *p, 6, ep[5]);
			p += scaler;
			ONEPIX(out, *p, 7, ep[6]);
			p += scaler;
			ONEPIX(out, *p, 8, ep[7]);
			p += scaler;

			*xip++ = out ^ white;
		}
		p = pstart + (width_ << s);
		xip = xstart + (stride >> 3);
		xstart = xip;
	}
}

void MonoWindowRenderer::dither(const u_char* p, u_int off,
				u_int x, u_int width, u_int height) const
{
	p += off;
	register u_char* xip = pixbuf_ + (off >> 3);
	
	int white = cm_.white();
	int h;
	int y = (off - x) / width_;
	for (h = 0; h < height; h++) {
		int w;
		for (w = 0; w < width; w += 8) {
			u_char out = 0;
			int r = (((y + h) & 0x1f) << 5) | ((x + w) & 0x1f);
			u_char* ep = &halftone[r];
			ONEPIX(out, *p++, 1, ep[0]);
			ONEPIX(out, *p++, 2, ep[1]);
			ONEPIX(out, *p++, 3, ep[2]);
			ONEPIX(out, *p++, 4, ep[3]);
			ONEPIX(out, *p++, 5, ep[4]);
			ONEPIX(out, *p++, 6, ep[5]);
			ONEPIX(out, *p++, 7, ep[6]);
			ONEPIX(out, *p++, 8, ep[7]);

			*xip++ = out ^ white;
		}
		p += width_ - width;
		xip += (width_ - width) >> 3;
	}
}
