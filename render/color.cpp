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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "color.h"
#ifndef WIN32
#include <sys/wait.h>
#include <sys/file.h>
#endif
#if defined(sun) && defined(__svr4__)
#include <fcntl.h>
#endif
#include <string.h>
#ifdef WIN32
#include <tk.h>

struct TkWinColormap {
	HPALETTE palette;	/* Palette handle used when drawing. */
	UINT size;		/* Number of entries in the palette. */
	int stale;		/* 1 if palette needs to be realized,
				 * otherwise 0.  If the palette is stale,
				 * then an idle handler is scheduled to
				 * realize the palette. */
	Tcl_HashTable refCounts; /* Hash table of palette entry reference
				  * counts indexed by pixel value. */
};

#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION == 0)
extern "C" unsigned long *win32Colors;
extern "C" int win32NColors;
#endif

#endif

//SV-XXX: rearranged intialistaion order to shut upp gcc4
ColorModel::ColorModel() :
	colormap_(0), visual_(0), depth_(0), gamma_(1.), ncolor_(0)
{
	Tcl& tcl = Tcl::instance();
	dpy_ = Tk_Display(tcl.tkmain());
	memset((char*)colors_, 0, sizeof(colors_));
}

ColorModel::~ColorModel()
{
	free_colors();
}

int ColorModel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "alloc-colors") == 0) {
			tcl.result(allocate() < 0 ? "0" : "1");
			return (TCL_OK);
		}
		if (strcmp(argv[1], "free-colors") == 0) {
			free_colors();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "visual") == 0) {
			Tk_Window tk = Tk_NameToWindow(tcl.interp(),
						       (char*)argv[2],
						       tcl.tkmain());
			setvisual(Tk_Visual(tk), Tk_Colormap(tk),
				  Tk_Depth(tk));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "gamma") == 0) {
			double v = atof(argv[2]);
			if (v < 0)
				tcl.result("0");
			gamma_ = v;
			tcl.result("1");
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}


int ColorModel::allocate()
{
	if (alloc_grays() < 0 || alloc_colors() < 0) {
		free_colors();
		return (-1);
	}
	return (0);
}

void ColorModel::rgb_to_yuv(color& c) const
{
	double r = c.r;
	double g = c.g;
	double b = c.b;

	c.y = short(0.299 * r + 0.587 * g + 0.114 * b + 0.5);
	c.u = short(-0.1687 * r - 0.3313 * g + 0.5 * b + 0.5) + 128;
	c.v = short(0.5 * r - 0.4187 * g - 0.0813 * b + 0.5) + 128;
}

int ColorModel::yuv_to_rgb(color& c) const
{
	double y = c.y;
	double u = c.u - 128.;
	double v = c.v - 128.;

	int r = int(y + 1.402 * v + 0.5);
	int g = int(y - 0.34414 * u - 0.71414 * v + 0.5);
	int b = int(y + 1.772 * u + 0.5);

	int valid = 1;
#define CHECK(v) \
	if (v > 255) { \
		v = 255; \
		valid = 0; \
	} else if (v < 0) { \
		v = 0; \
		valid = 0; \
	}
	CHECK(r)
	c.r = r;
	CHECK(g)
	c.g = g;
	CHECK(b)
	c.b = b;

	return (valid);
}

void ColorModel::free_colors()
{
#ifdef MAC_OSX_TK
       /*
        * XXX should either figure out the appropriate thing
        * for macosx or just ditch support for <16 bit depth
        */
        abort();
#else
	if (ncolor_ > 0) {
		XFreeColors(dpy_, colormap_, pixel_, ncolor_, 0);
		ncolor_ = 0;
	}
#endif
}

int ColorModel::alloc_color(color& c)
{
#ifdef MAC_OSX_TK
     abort();
#else
	int r = c.r;
	int g = c.g;
	int b = c.b;

	if (gamma_ != 1.) {
		r = int(256. * pow(r / 256., gamma_));
		g = int(256. * pow(g / 256., gamma_));
		b = int(256. * pow(b / 256., gamma_));
	}
#ifndef WIN32
	if (r == g && r == b && (r & 7) == 0 && r > 0) {
		/*
		 * This color is one level away from the gray ramp
		 * used in nv, ghostscript, and the LBL-modified
		 * tk library.  Change it so we'll end up sharing
		 * the colormap entry.  The error won't be perceptible.
		 */
		 r -= 1;
		 g -= 1;
		 b -= 1;
	 }
#endif
	XColor xc;
	xc.red = r << 8;
	xc.green = g << 8;
	xc.blue = b << 8;
	if (XAllocColor(dpy_, colormap_, &xc) == 0) {
		free_colors();
		return (-1);
	}
#ifdef WIN32
	/*
	 * The current WinTk returns RGB values in the pixel. X returns
	 * a colormap index. So get the palette index and use that.
	 */
	TkWinColormap *cmap = (TkWinColormap *) colormap_;
	UINT index = GetNearestPaletteIndex(cmap->palette, PALETTERGB(r, g, b));
	int pixel = index;
	pixel_[index] = xc.pixel;
	color& p = colors_[ncolor_];
	if (++ncolor_ >= 256) {
		fprintf(stderr, "vic: colormap overflow (internal error)\n");
		exit(1);
	}

#if defined(WIN32) && (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION == 0)
	if (!win32Colors)
		win32Colors = pixel_;
	win32NColors = ncolor_;
#endif

#else
	int pixel = xc.pixel;
	pixel_[ncolor_] = pixel;
	if (++ncolor_ >= 256) {
		fprintf(stderr, "vic: colormap overflow (internal error)\n");
		exit(1);
	}
	color& p = colors_[pixel];
#endif
	p.r = xc.red >> 8;
	p.g = xc.green >> 8;
	p.b = xc.blue >> 8;

	/* XXX ed ditherer needs this */
	rgb_to_yuv(p);

	return (pixel);
#endif // MAC_OSX_TK
}

int ColorModel::alloc_colors()
{
	return (0);
}

int ColorModel::alloc_grays()
{
#define DELTA (256 / 32)
	int pixel[32];
	int i;
	for (i = 0; i < 32; ++i) {
		int luma = (i + 1) * DELTA - 1;
		color c;
		c.r = c.g = c.b = luma;
		pixel[i] = alloc_color(c);
		if (pixel[i] < 0)
			return (-1);
	}
	for (i = 0; i < 256; ++i) {
		int level = (i + 1) / DELTA;
		int err;
		if (level == 0)
			err = 0;
		else {
			level -= 1;
			err = (i + 1) & (DELTA - 1);
		}
		grey_lut_[i] = pixel[level] | err << 8;
	}
	for (; i < 512; ++i)
		grey_lut_[i] = pixel[31];

	return (0);
}

