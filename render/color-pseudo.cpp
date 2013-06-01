/*
 * Copyright (c) 1993-1995 The Regents of the University of California.
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

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#else
#include <sys/file.h>
#endif
//#include <X11/Xlib.h>
//#include <X11/Xutil.h>

#undef Status
#include "color-pseudo.h"
#include "bsd-endian.h"
#include "color-hist.h"

//SV-XXX: rearranged intialistaion order to shut upp gcc4
PseudoColorModel::PseudoColorModel() : file_(0), lut_(0)
{
}

PseudoColorModel::~PseudoColorModel()
{
	delete[] lut_; //SV-XXX: Debian
}

/*
 * $cm lut $filename
 */
int PseudoColorModel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "create-hist") == 0) {
			ColorHist* p = new ColorHist;
			tcl.result(p->name());
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "lut") == 0) {
			file_ = Tk_GetUid((char*)argv[2]);
			return (TCL_OK);
		}
	}
	return (ColorModel::command(argc, argv));
}

int PseudoColorModel::allocate(int ncolor, const u_char* cmap, u_char* lut)
{
	int LUT[256];
	int i;
	for (i = 0; i < ncolor; ++i) {
		int r = cmap[0];
		int g = cmap[1];
		int b = cmap[2];
		/*XXX boost luminance to make up for CCIR 601 YCrCb*/
#ifdef notdef
		if (r < 240 && g < 240 && b < 240) {
			r += 16;
			g += 16;
			b += 16;
		}
#endif
		int pixel = alloc_color(r, g, b);
		if (pixel < 0)
			return (-1);
		cmap += 3;
		LUT[i] = pixel;
	}
	/*
	 * Map from colormap indices to allocated pixels.
	 */
	for (i = 0; i < 65536; ++i)
		lut[i] = LUT[lut[i]];

	return (0);
}

int PseudoColorModel::installLUT(int def_ncolor, const u_char* def_cmap,
			      const u_char* def_lut)
{
	delete[] lut_; //SV-XXX: Debian
	lut_ = new u_char[65536];
	u_char cmap[3*256];
	int ncolor = -1;
	if (file_ != 0) {
		ncolor = readLUT(file_, cmap);
		if (ncolor < 0) {
			fprintf(stderr, "vic: couldn't parse %s\n", file_);
			fprintf(stderr, "vic: using builtin color table\n");
		}
	}
	if (ncolor < 0) {
		ncolor = def_ncolor;
		memcpy(cmap, def_cmap, 3 * ncolor);
		memcpy(lut_, def_lut, 65536);
	}
	if (allocate(ncolor, cmap, lut_) < 0) {
		delete[] lut_; //SV-XXX: Debian
		lut_ = 0;
		return (-1);
	}
	return (0);
}

int PseudoColorModel::readLUT(const char* file, u_char* cmap)
{
	int fd = open(file, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "vic: %s: ", file);
		perror("");
		return (-1);
	}
	u_char c;
	if (read(fd, (char*)&c, 1) != 1)
		return (-1);

	int ncolor = c;
	int cnt = 3 * ncolor;
	if (read(fd, (char*)cmap, cnt) != cnt ||
	    read(fd, (char*)lut_, 65536) != 65536)
		return (-1);

	return (ncolor);
}

void PseudoWindowRenderer::dither_gray_down(const u_char* yp,
					    u_int off, u_int x,
					    u_int width, u_int height) const
{
	int s = scale_;
	int h = height >> s;
	width >>= s;
	width = (width + 3) & ~3;
	register int scaler = 1 << s;
	yp += off;
	register u_char* xip = pixbuf_ + ((off - x) >> (s + s)) + (x >> s);
	u_char* xstart = xip;
	const int stride = width_ >> s;
	const u_short* const map = graylut_;
	int e = 0;
	while (--h >= 0) {
		for (int w = width; w > 0; w -= 4) {
			//SV-XXX: unused u_int out = 0;
			e += *yp;
			yp += scaler;
			e = map[e];
			*xip++ = e;
			e >>= 8;
			e += *yp;
			yp += scaler;
			e = map[e];
			*xip++ = e;
			e >>= 8;
			e += *yp;
			yp += scaler;
			e = map[e];
			*xip++ = e;
			e >>= 8;
			e += *yp;
			yp += scaler;
			e = map[e];
			*xip++ = e;
			e >>= 8;
		}
		yp += (width_ - width) << s;
		xip = xstart + stride;
		xstart = xip;
	}
}

void PseudoWindowRenderer::dither_gray(const u_char* yp,
				       u_int off, u_int x,
				       u_int width, u_int height) const
{
	UNUSED(x); //SV-XXX: unused
	yp += off;
	u_int* xip = (u_int*)(pixbuf_ + off);
	const u_short* const map = graylut_;
	int len = width * height;
	int w = width;
	int e = 0;
	for (; len > 0; len -= 4) {
		u_int out = 0;
		e += yp[0];
		e = map[e];
		SPLICE(out, e & 0xff, 24);
		e >>= 8;
		e += yp[1];
		e = map[e];
		SPLICE(out, e & 0xff, 16);
		e >>= 8;
		e += yp[2];
		e = map[e];
		SPLICE(out, e & 0xff, 8);
		e >>= 8;
		e += yp[3];
		e = map[e];
		SPLICE(out, e & 0xff, 0);
		e >>= 8;

		*xip++ = out;
		
		yp += 4;
		w -= 4;
		if (w <= 0) {
			yp += width_ - width;
			xip += (width_ - width) / 4;
			w = width;
		}
	}
}

void PseudoWindowRenderer::dither_gray_up(const u_char* yp,
					  u_int off, u_int x,
					  u_int width, u_int height) const
{
	yp += off;
	u_int* xip = (u_int*)(pixbuf_ + ((off - x) << 2) + (x << 1));
	const int stride = (width_ * 2) / 4;
	const u_short* const map = graylut_;
	u_int* xstart = xip;
	int w = width;
	int e = 0;
	for (int len = width * height; len > 0; len -= 2) {
		int v = yp[0];
		e += v;
		e = map[e];
		u_int out0 = 0;
		SPLICE(out0, e & 0xff, 24);
		e >>= 8;
		e += v;
		e = map[e];
		u_int out1 = 0;
		SPLICE(out1, e & 0xff, 24);
		e >>= 8;
		e += v;
		e = map[e];
		SPLICE(out1, e & 0xff, 16);
		e >>= 8;
		e += v;
		e = map[e];
		SPLICE(out0, e & 0xff, 16);
		e >>= 8;
		
		v = yp[1];
		e += v;
		e = map[e];
		SPLICE(out0, e & 0xff, 8);
		e >>= 8;
		e += v;
		e = map[e];
		SPLICE(out1, e & 0xff, 8);
		e >>= 8;
		e += v;
		e = map[e];
		SPLICE(out1, e & 0xff, 0);
		e >>= 8;
		e += v;
		e = map[e];
		SPLICE(out0, e & 0xff, 0);
		e >>= 8;

		xip[0] = out0;
		xip[stride] = out1;
		
		yp += 2;
		xip += 1;

		w -= 2;
		if (w <= 0) {
			yp += width_ - width;
			xip = xstart + 2 * stride;
			xstart = xip;
			w = width;
		}
	}
}
