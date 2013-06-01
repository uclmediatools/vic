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

#include <stdlib.h>
#include "renderer.h"
#include "renderer-window.h"
#include "vw.h"
//#include "rv.h"

#include "color.h"
#include "vic_tcl.h"
/*
 * Rendering vector.  We keep a vector of timestamps of when each individual
 * block of frame has been updated by a decoder.  Then, different renderers
 * can keep track of the last time they've updated their view of the frame
 * (and dither only those blocks that need to be updated).
 */

#define RV_PAST(now, ts) ((((now) - (ts)) & 0x80) == 0)

Renderer::Renderer(int ft) :
	Module(ft),
	next_(0),
	now_(0),
	update_interval_(0),
	need_update_(1),
	enable_xv(false)
{
}

int Renderer::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "update-interval") == 0) {
			setinterval(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "color") == 0) {
			setcolor(atoi(argv[2]));
			return (TCL_OK);
		}else if (strcmp(argv[1], "enable_xv") == 0) {
#ifdef HAVE_XVIDEO
			enable_xv = !strcmp(argv[2], "true");
#endif
			return (TCL_OK);
		}	
	}
	return (Module::command(argc, argv));
}

void Renderer::setinterval(u_int v)
{
	if (update_interval_ != v) {
		update_interval_ = v;
		if (update_interval_) {
			msched(v);
			need_update_ = 0;
		} else {
			cancel();
			need_update_ = 1;
		}
	}
}

void Renderer::timeout()
{
	need_update_ = 1;
	msched(update_interval_);
}

int BlockRenderer::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "irthresh") == 0) {
			irthresh_ = atoi(argv[2]);
			return (TCL_OK);
		}
	}
	return (Renderer::command(argc, argv));
}

int BlockRenderer::consume(const VideoFrame* vf)
{
	if (!samesize(vf))
		resize(vf->width_, vf->height_);
	YuvFrame* p = (YuvFrame*)vf;

	if (update_interval_ != 0) {
		if (need_update_ == 0)
			return (0);
		need_update_ = 0;
	}

#ifdef HAVE_SWSCALE
	render(vf->bp_, 0, 0, 0, 0);
	
	// put the image to display
	// in WindowRenderer::push
	push(NULL, 0, 0, 0, 0);
	sync();
#else
	
	
	/*
	 * check how many blocks we need to update.  If more than
	 * 12%, do a single image push call for them.  Otherwise,
	 * only push the ones that change.
	 */	
	u_int now = now_;
	const u_int8_t* ts = p->crvec_;
	u_int bcnt = 0;
	int w = width_ >> 3;
	int y;
	for (y = height_ >> 3; --y >= 0; ) {
		for (int x = 0; x < w; ++x)
			if (!RV_PAST(now, ts[x]))
				++bcnt;
		ts += w;
	}
	int immed = bcnt < irthresh_;

#ifdef WIN32
	immed = 0;
#endif

	int ymin = height_;
	int ymax = 0;
	ts = p->crvec_;
	for (y = 0; y < height_; y += 8) {
		for (int x = 0; x < width_; ) {
			if (RV_PAST(now, ts[x >> 3])) {
				x += 8;
				continue;
			}
			int sx = x;
			do {
				x += 8;
			} while (x < width_ && !RV_PAST(now, ts[x >> 3]));

			if (y < ymin)
				ymin = y;
			if (y > ymax)
				ymax = y;
			// XXX
			int off = y * width_ + sx;
			render(p->bp_, off, sx, x - sx, 8);
			if (immed)
				push(p->bp_, y, y + 8, sx, x);
		}
		ts += width_ >> 3;
	}
	// XXX
	now_ = p->ts_;
	if (ymin <= ymax) {
		if (!immed)
			push(p->bp_, ymin, ymax + 8, 0, 0);
		sync();
	}
#endif

	return (0);
}
