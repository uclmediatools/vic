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

#ifndef vic_renderer_window_h
#define vic_renderer_window_h

#include "renderer.h"

class VideoWindow;
class VideoImage;

/*
 * A subclass of Renderer where video is sent to a display window.
 */
class WindowRenderer : public BlockRenderer {
    protected:
	WindowRenderer(VideoWindow* w, int decimation);
    public:
	~WindowRenderer();
	void push(const u_char* frm, int miny, int maxy,
		  int minx, int maxx) const;
	void sync() const;
	void resize(int w, int h);
	void dither_null(const u_char* frm, u_int off, u_int x,
			 u_int width, u_int height) const;
    protected:
	/*XXX*/
	virtual void setcolor(int c);
	void compute_scale(int w, int h);
	virtual void alloc_image() = 0;
	void doupdate();
	virtual void update() = 0;
	virtual void disable() = 0;
	int index() const;

	VideoWindow* window_;
	VideoImage* image_;
	int ww_;		/* width of target window */
	int wh_;		/* height of target window */
	int scale_;		/* log base two of inverse of scale factor */
	int outw_;              /* width of target image (maybe != width_) */
	int outh_;              /* height of target image (maybe != height_) */
	int color_;		/* false to render grayscale */
	int decimation_;	/* 422 or 420 */
};

inline int WindowRenderer::index() const
{
	int s = scale_ + 1;
	if (s > 4)
		s = 4;
	else if (s <= 0)
		/*XXX never upsample by more than 2*/
		s = 0;
	s <<= 2;
	if (decimation_ == 422)
		s |= 1;
	if (!color_)
		s |= 2;
	return (s);
}

/*
 * A subclass of WindowRenderer where video is dithered (or colorspace
 * converted) locally.
 */
class WindowDitherer : public WindowRenderer {
    protected:
	WindowDitherer(VideoWindow*, int decimation);
	void alloc_image();

	u_char* pixbuf_;
};

#endif
