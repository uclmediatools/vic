/*
 * Copyright (c) 1994 The Regents of the University of California.
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
#include <stdio.h>
//#include <X11/Xlib.h>
//#include <X11/Xutil.h>
#include "renderer-window.h"
#include "color-pseudo.h"
#include "bsd-endian.h"
#include "vw.h"

class GrayColorModel : public ColorModel {
public:
	~GrayColorModel();
	virtual int command(int argc, const char*const* argv);
};

class GrayColorMatcher : public Matcher {
public:
	GrayColorMatcher() : Matcher("colormodel") {}
	virtual TclObject* match(const char* id) {
		if (strcasecmp(id, "pseudocolor/8/gray") == 0)
			return (new GrayColorModel());
		return (0);
	}
} matcher_gray;

GrayColorModel::~GrayColorModel()
{
	/*XXX*/
}

class GrayWindowRenderer;

typedef void (GrayWindowRenderer::*GrayMethod)(const u_char*, u_int,
					       u_int, u_int, u_int) const;

class GrayWindowRenderer : public PseudoWindowRenderer {
public:
	GrayWindowRenderer(VideoWindow* vw, int decimation,
			   const u_short* graylut) :
		PseudoWindowRenderer(vw, decimation, graylut) { }
	void render(const u_char* frm, int off, int x, int w, int h) {
		(this->*method_)(frm, off, x, w, h);
	}
protected:
	virtual void update();
	virtual void disable() { method_ = &PseudoWindowRenderer::dither_null; }
	GrayMethod method_;
};

int GrayColorModel::command(int argc, const char*const* argv)
{
	if (argc == 4 && strcmp(argv[1], "renderer") == 0) {
		Tcl& tcl = Tcl::instance();
		VideoWindow* vw = VideoWindow::lookup(argv[2]);
		int decimation = atoi(argv[3]);
		Renderer* r = new GrayWindowRenderer(vw, decimation,
						     graylut());
		tcl.result(r->name());
		return (TCL_OK);
	}
	return (ColorModel::command(argc, argv));
}

void GrayWindowRenderer::update()
{
	if (scale_ > 0)
		method_ = &GrayWindowRenderer::dither_gray_down;
	else if (scale_ < 0)
		method_ = &GrayWindowRenderer::dither_gray_up;
	else
		method_ = &GrayWindowRenderer::dither_gray;
}
