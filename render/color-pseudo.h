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

#ifndef vic_color_pseudo_h
#define vic_color_pseudo_h

#include "color.h"
#include "renderer-window.h"

class PseudoColorModel : public ColorModel {
public:
	PseudoColorModel();
	~PseudoColorModel();
	inline const u_char* lut() const { return (lut_); }
	virtual int command(int argc, const char*const* argv);
protected:
	int installLUT(int def_ncolor, const u_char* def_cmap,
		       const u_char* def_lut);
	int allocate(int ncolor, const u_char* cmap, u_char* lut);
	int readLUT(const char* file, u_char* cmap);
	const char* file_;
	u_char* lut_;
};

class PseudoWindowRenderer : public WindowDitherer {
 protected:
	PseudoWindowRenderer(VideoWindow* vw, int decimation,
			     const u_short* glut) :
		WindowDitherer(vw, decimation), graylut_(glut) { }
	void dither_gray_down(const u_char* yp,
			      u_int off, u_int x,
			      u_int width, u_int height) const;
	void dither_gray(const u_char* yp,
			 u_int off, u_int x,
			 u_int width, u_int height) const;
	void dither_gray_up(const u_char* yp,
			    u_int off, u_int x,
			    u_int width, u_int height) const;
	const u_short* graylut_;
};


#endif

