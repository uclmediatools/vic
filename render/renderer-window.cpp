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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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

#include <stdlib.h>
#include "vw.h"
#include "renderer-window.h"
#include "color.h"

WindowRenderer::WindowRenderer(VideoWindow* w, int decimation) :
	BlockRenderer(decimation == 422 ? FT_YUV_422 : FT_YUV_420),
	window_(w),
	image_(0),
	ww_(w->width()),
	wh_(w->height()),
	scale_(0),
	outw_(0),
	outh_(0),
	color_(0),
	decimation_(decimation)
{
}

WindowRenderer::~WindowRenderer()
{
	/*
	 * Clear out the image so that the VideoWindow class does
	 * not try to use it before another window gets attached
	 * and replaces the image.
	 */
	window_->setimage(0);
	delete image_;
}

static inline int
distance(int a, int b)
{
	a -= b;
	return (a >= 0 ? a : -a);
}

void WindowRenderer::compute_scale(int w, int h)
{
	width_ = w;
	height_ = h;
	framesize_ = w * h;

	/*
	 * Choose a good scale factor.  We scale up or down
	 * by factors of two.  Find the closest scale so that
	 * the input geometry matches to the desired window size.
	 * On a mismatch, the image will be centered in the window.
	 */
	scale_ = 0;
	outw_ = (ww_ >> 2) << 2;
	outh_ = (wh_ >> 2) << 2;

#ifndef HAVE_SWSCALE
	int d = distance(ww_, width_);
	int t = distance(ww_, width_ << 1);
	if (t < d) {
		outw_ = width_ << 1;
		outh_ = height_ << 1;
		scale_ = -1;
	} else {
		// XXX this should be a loop 
		t = distance(ww_, width_ >> 1);
		if (t < d) {
			d = t;
			scale_ = 1;
		}
		t = distance(ww_, width_ >> 2);
		if (t < d) {
			d = t;
			scale_ = 2;
		}
		t = distance(ww_, width_ >> 3);
		if (t < d)
			scale_ = 3;

		outw_ = width_ >> scale_;
		outh_ = height_ >> scale_;
	}
#endif
}

void WindowRenderer::sync() const
{
	window_->complete();
}

void WindowRenderer::push(const u_char*, int miny, int maxy, int minx, int maxx) const
{
        if(enable_xv){
            window_->render(image_, 0, outh_, 0, outw_ );
			return;
        }            
	
#ifdef HAVE_SWSCALE
		// swscaler render called - alothough args are zero it uses the global variables
		// Should probably alter this so that global variables are passed in the args for clarity
	window_->render(image_, 0, 0 , 0, 0);
#else
	if (scale_ >= 0) {
		miny >>= scale_;
		maxy >>= scale_;
		minx >>= scale_;
		maxx >>= scale_;
	} else {
		miny <<= -scale_;
		maxy <<= -scale_;
		minx <<= -scale_;
		maxx <<= -scale_;
	}
	window_->render(image_, miny, maxy, minx, maxx);
#endif	

}

/*
 * Tell subclass that we've change decimation/color state
 * so it can reconfigure with different rendering parameters.
 */
void WindowRenderer::doupdate()
{
	/*
	 * Subclasses assume that output width is a multiple of 4.
	 * If not, disable the renderer.  (This is a pathological
	 * case that should only happen with garbage streams.
	 * In theory, it could happen with non-standard video sources
	 * like X screen captures; XXX deal with this when things break.)
	 */
	if (outw_ & 3)
		disable();
	else
		update();
}

void WindowRenderer::resize(int w, int h)
{
	int outw = outw_;
	int outh = outh_;
	compute_scale(w, h);
	/*
	 * Doing a resize can change the output window size
	 * (but it's not likely because this size is mainly
	 * dependent on the size of the window).
	 */
//	if (outw != outw_ || outh != outh_) {
		delete image_;
		window_->setimage(0);
		alloc_image();
		/*XXX*/
		window_->damage();
		window_->redraw();
//	}
	doupdate();
}

void WindowRenderer::setcolor(int c)
{
	color_ = c;
	doupdate();
}

void WindowRenderer::dither_null(const u_char*, u_int, u_int,
				 u_int, u_int) const
{
}

WindowDitherer::WindowDitherer(VideoWindow* vw, int decimation)
	: WindowRenderer(vw, decimation)
{
}

void WindowDitherer::alloc_image()
{
	StandardVideoImage* p;

#ifdef HAVE_XVIDEO
	
	if (enable_xv){
	  XVideoImage *p;		
	  p = XVideoImage::allocate(window_->tkwin(), width_, height_);	
	  if(p){
	    pixbuf_ = p->pixbuf();
	    image_ = p;
	    return;
	  }	  
	  enable_xv = false;
	  debug_msg("disable xvideo\n");
	}
#endif

	p = StandardVideoImage::allocate(window_->tkwin(), outw_, outh_);
	pixbuf_ = p->pixbuf();
	image_ = p;
}
