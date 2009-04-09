/*
 * Copyright (c) 1993-1994 The Regents of the University of California.
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

#ifndef vic_color_h
#define vic_color_h

#include "vic_tcl.h"
#include "iohandler.h"

extern "C" {
#include <tk.h>
}

#define GREY_LEVELS 32
#define YUVLIM(x) (((x) < 0) ? 0 : ((x) > 127) ? 127 : (x))


/*
 * Classes for mapping YUV image data into a representation that
 * can be rendered on the display.  The ColorModel class represents
 * a finite-element color cube, which might be uniformaly sampled,
 * or biased; or, a map from YUV space to 24-bit true color.
 * The Colorizer class represents the agents that do the work of
 * mapping from YUV space to the ColorModel space.  For example,
 * they might do dithering or simple (non-uniform) quantization.
 */

class ColorModel : public TclObject {
 public:
	struct color {
		short r;
		short g;
		short b;
		short y;
		short u;
		short v;
	};
	virtual ~ColorModel();
	inline const u_short* graylut() const { return (grey_lut_); }
	int allocate();
	virtual int alloc_colors();
	virtual int alloc_grays();
	const color* lookup(int pixel) const { return (&colors_[pixel]); }
	virtual int command(int argc, const char*const* argv);
 protected:
	ColorModel();

	void free_colors();
	void rgb_to_yuv(color& c) const;
	int yuv_to_rgb(color& c) const;

	/*
	 * Return the n'th allocated color.
	 */
	inline const color& colors(int n) const { return (colors_[pixel_[n]]); }

	int alloc_color(color&);
	int alloc_color(int r, int g, int b);
	
 public:
	inline void setvisual(Visual* v, Colormap c, int d) {
		visual_ = v;
		colormap_ = c;
		depth_ = d;
	}
 protected:
	Display* dpy_;
	Colormap colormap_;
	Visual* visual_;
	int depth_;
	double gamma_;

	int ncolor_;
	color colors_[256];
	u_long pixel_[256];
	u_short grey_lut_[512];
};

inline int ColorModel::alloc_color(int r, int g, int b)
{
	color c;
	c.r = r;
	c.g = g;
	c.b = b;
	return (alloc_color(c));
}
#endif
