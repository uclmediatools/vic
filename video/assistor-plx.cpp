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
 * Copyright 1995 Apple Computer, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * APPLE COMPUTER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Apple Computer shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Apple Computer.
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
extern "C" {
#include <sys/shm.h>
}

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
extern "C" {
#include <X11/extensions/XShm.h>
}

/*XXX*/
#ifdef __svr4__
extern "C" {
#include <XPlxExt.h>
}
#else
#include "plxfuncs.h"
#endif

#include "vic_tcl.h"
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "vw.h"
#include "renderer.h"

#define DEBUG 0

class PlxWindowAssistor : public Renderer {
public:
	PlxWindowAssistor();
	~PlxWindowAssistor();
	virtual int command(int argc, const char*const* argv);
protected:
	virtual int consume(const VideoFrame*);
	void reset(int q, int w, int h);
	int inq_;
	XPlxCImage cimage_;
	GC gc_;
	int need_refresh_;
	static void handle_exposure_callback(ClientData, XEvent *);
	static void handle_exposure_idle(ClientData);
	void handle_exposure() { need_refresh_ = 1; }
	VideoWindow* window_;
};

static class PlxWindowAssistorMatcher : public Matcher {
public:
	PlxWindowAssistorMatcher() : Matcher("assistor") {}
	int have_extension() {
		Tk_Window tk = Tcl::instance().tkmain();
		Display* dpy = Tk_Display(tk);
		int majop, eventbase, errbase;
		return (XQueryExtension(dpy, "ParallaxVideo", &majop,
					&eventbase, &errbase));
	}
	TclObject* match(const char* id) {
		if (strcmp(id, "jpeg/422") == 0 && have_extension())
			return (new PlxWindowAssistor());
		return (0);
	}
} assistor_plx;

int PlxWindowAssistor::consume(const VideoFrame* vf)
{
	JpegFrame* p = (JpegFrame*)vf;
	// Can't handle 420, only 422 XXX
	if (p->type_ != 0)
		return (0);

	if (p->q_ != inq_ || p->width_ != width_ || p->height_ != height_)
		reset(p->q_, p->width_, p->height_);

	if (need_refresh_) {
		XPlxVideoTag(Tk_Display(window_->tkwin()),
			Tk_WindowId(window_->tkwin()), gc_, PLX_VIDEO);
	}

	cimage_.size = p->len_;
	cimage_.data = (char*)p->bp_;
	XPlxPutCImage(Tk_Display(window_->tkwin()), 
		Tk_WindowId(window_->tkwin()),
		gc_, &cimage_, 0, 0, width_, height_, 
		0, 0, window_->width(), window_->height(),
		1 /* scale, instead of crop */);

	if (need_refresh_) {
		XPlxVideoTag(Tk_Display(window_->tkwin()),
			Tk_WindowId(window_->tkwin()), gc_, PLX_VIDEO_OVR);
		need_refresh_ = 0;
	}
	return (0);
}

void PlxWindowAssistor::handle_exposure_idle(ClientData cd) {
	((PlxWindowAssistor *)cd)->handle_exposure();
}

void PlxWindowAssistor::handle_exposure_callback(ClientData cd, XEvent *) {
	Tk_DoWhenIdle(handle_exposure_idle, cd);
}

PlxWindowAssistor::PlxWindowAssistor()
	:  Renderer(FT_JPEG), inq_(20)
{
	need_refresh_ = 1;
}

PlxWindowAssistor::~PlxWindowAssistor()
{
	/*
	 * Clear out the image so that the Video class does
	 * not try to use it before another window gets attached
	 * and replaces the image.
	 */
	window_->setimage(0);
	// Tk_FreeGC(Tk_Display(window_->tkwin()), gc_);
	XFreeGC(Tk_Display(window_->tkwin()), gc_);
}

int PlxWindowAssistor::command(int argc, const char*const* argv)
{
	if (argc == 3 && strcmp(argv[1], "window") == 0) {
		window_ = VideoWindow::lookup(argv[2]);
		Tk_MakeWindowExist(window_->tkwin());
		// gc_ = Tk_GetGC(window_->tkwin(), 0, 0);
		gc_ = XCreateGC(Tk_Display(window_->tkwin()),
				Tk_WindowId(window_->tkwin()), 0, 0);
		Tk_CreateEventHandler(window_->tkwin(), ExposureMask,
				      handle_exposure_callback,
				      (ClientData)this);
		return (TCL_OK);
	}
	return (Renderer::command(argc, argv));
}

void PlxWindowAssistor::reset(int inq, int w, int h)
{
	Module::size(w, h);
	/*
	 * Make sure all outstanding putimage's finish.
	 * Syncing with the first window, syncs the server,
	 * and hence all of them.
	 */
	window_->sync();
	
	if (inq != inq_) {
		inq_ = inq;
		// Prepare new Q table
		extern int quality_to_qfactor(int);
		char *qtable;
		int size = MakeQTables(quality_to_qfactor(inq),
				       (unsigned char**)&qtable);
		XPlxPutTable(Tk_Display(window_->tkwin()), 
			Tk_WindowId(window_->tkwin()), gc_,
#ifdef __hpux
		     (short *)
#endif
			qtable, size, 1 /* decompression, not compression */);
		free(qtable);
	}
	cimage_.width = width_;
	cimage_.height = height_;
}
