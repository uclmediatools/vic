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
 *      This product includes software developed by the University of
 *      California, Berkeley and the Network Research Group at
 *      Lawrence Berkeley Laboratory.
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

/*
 * (c) Copyright 1994 HEWLETT-PACKARD COMPANY
 * 
 * To anyone who acknowledges that this file is provided 
 * "AS IS" without any express or implied warranty:
 * permission to use, copy, modify, and distribute this 
 * file for any purpose is hereby granted without fee, 
 * provided that the above copyright notice and this 
 * notice appears in all copies, and that the name of 
 * Hewlett-Packard Company not be used in advertising or 
 * publicity pertaining to distribution of the software 
 * without specific, written prior permission.  Hewlett-
 * Packard Company makes no representations about the 
 * suitability of this software for any purpose.
 *
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#ifdef HP_YUVLIB

#include <stdio.h>
#include <osfcn.h>
//#include <X11/Xlib.h>
//#include <X11/Xutil.h>
#include "endian.h"
#include "color.h"
#include "color-lut.h"
#include "inet.h"
#include "vic_tcl.h"

extern "C" {
#include <vtc/yuvlib.h>
}

class YuvColorModel : public ColorModel {
public:
	YuvColorModel(Visual*);
	~YuvColorModel();
	WindowRenderer* allocate(VideoWindow* vw, int decimation);
	virtual int alloc_colors();
	virtual int alloc_grays();
	inline yuvHandle yuv_handle() const { return (yuvHandle_); }
protected:
	yuvHandle yuvHandle_;
	double *yuvScalefactors_;
	int yuvNumScalefactors_;
};

class YuvVideoWindow : public VideoWindow {
public:
	YuvVideoWindow(const char* name, int width, int height);
protected:
	yuvWindowHandle yuvWindow_;
};

class YuvColorMatcher : public ColorMatcher {
	virtual ColorModel* match(const char* colormodel);
} matcher_yuv;

ColorModel* YuvColorMatcher::match(const char* colormodel)
{
	if (strcasecmp(colormodel, "yuv") == 0) {
		Visual* visual = ColorModel::pseudocolor_visual();
		if (visual != 0)
			return (new YuvColorModel(visual));
	}
	return (0);
}

YuvColorModel::YuvColorModel(Visual* visaul)
{
	visual_ = visual;
	Tk_Window tk = Tcl::instance().tkmain();
	int screen = DefaultScreen(dpy_);
	int method;
	yuvError status;

#ifdef DEBUG
	printf("Init YUVlib\n");
#endif
	if ((status = yuvInit(DisplayString(dpy_), screen, 0, NULL, &visual_, &colormap_, &method, &yuvHandle_)) != YUV_OK)
	  fprintf(stderr, "yuvInit: failed: %d\n", status);

#ifdef DEBUG
	printf("YUVlib method: %d\n", method);
#endif

	if ((status = yuvGetScaleFactors(yuvHandle_, &yuvNumScalefactors_, &yuvScalefactors_)) != YUV_OK)
	  fprintf(stderr, "yuvGetScaleFactors: failed: %d\n", status);

#ifdef DEBUG
	printf("yuv Scale Factors: ");
	for (int i=0; i < yuvNumScalefactors_; i++)
	  printf("%3f ", yuvScalefactors_[i]);
	printf("\n");
#endif
}

YuvColorModel::~YuvColorModel()
{
	/*XXX*/
}

VideoWindow* YuvColorModel::allocate_window(const char* name, int w, int h)
{
	return (new YuvVideoWindow(name, w, h));
}

int YuvColorModel::alloc_grays()
{
#ifdef DEBUG
	printf("Install YUV grey lut\n");
#endif
#define DELTA (256 / 32)
	int pixel[32];
	for (int i = 0; i < 32; ++i) {
		int luma = (i + 1) * DELTA - 1;
		color c;
		c.r = c.g = c.b = luma;
		pixel[i] = alloc_color(c);
		if (pixel[i] < 0)
			return (-1);
	}

	return (0);
}

int YuvColorModel::alloc_colors()
{
    /*
     * Don't use alloc_color() here because it plays with the
     * rgb values too much. This needs to match the YUV standard
     * colormap.
     */
#ifdef DEBUG
  	printf("Install YUV color lut\n");
#endif
	XColor color;
	for (int b=0; b<4; b++) {
 	    for (int g=0; g<8; g++) {
 		for (int r=0; r<4; r++) {
 		    color.red = r * 65535/3;
 		    color.green = g * 65535/7;
 		    color.blue = b * 65535/3;
 		    color.flags = DoRed | DoGreen | DoBlue;
 		    if (XAllocColor(dpy_, colormap_, &color) == 0) {
 			free_colors();
 			return (-1);
 		    }
 		    u_long pixel = color.pixel;
 		    pixel_[ncolor_] = pixel;
 		    if (++ncolor_ >= 256) {
 			fprintf(stderr, "vic: colormap overflow (internal error)\n");
 			exit(1);
 		    }
 		}
 	    }
	}
#ifdef DEBUG
	printf("installLut done.\n");
#endif
	return (0);
}

inline trunc16(int v)
{
    return( (v) / 16 * 16 );
}

inline round16(int v)
{
    return ( ((v) + 15) / 16 * 16 );
}

inline trunc2(int v)
{
    return ( (v) / 2 * 2 );
}

inline round2(int v)
{
    return ( ((v) + 1) / 2 * 2 );
}

class YuvWindowRenderer : public WindowRenderer {
public:
	YuvWindowRenderer(YuvVideoWindow*, int decimation, YuvColorModel& cm)
		: WindowRenderer(vw, decimation), cm_(cm) { } 
	void render(const u_char* frm, int off, int x, int w, int h) {
		(this->*method_)((u_char*)image_->ximage()->data,
				 frm, off, x, w, h);
	}
protected:
	const YuvColorModel& cm_;
	void map_gray(u_char* xip,
		      const u_char* frm, u_int off, u_int x,
		      u_int width, u_int height) const;
	void map_422(u_char* xip,
		     const u_char* frm, u_int off, u_int x,
		     u_int width, u_int height) const;
	void map_420(u_char* xip,
		     const u_char* frm, u_int off, u_int x,
		     u_int width, u_int height) const;
};

WindowRenderer* YuvColorModel::allocate(VideoWindow* w, int decimation)
{
	return (new YuvWindowRenderer((YuvVideoWindow*)vw, decimation, *this));
}

void YuvWindowRenderer::update()
{
	if (!color_)
		method_ = &map_gray;
	else if (decimation_ == 420)
		method_ = &map_420;
	else
		method_ = &map_422;
}

void YuvWindowRenderer::map_422(register u_char* xip,
				const u_char* frm, u_int off, u_int x,
				u_int width, u_int height) const
{
    u_int y = (off - x) / inw_;/*XXX*/
    int scaler = 1<<scale_, scalebits = scale_;
    u_int iw = inw_;
    register const u_char* yp = frm + off;
    u_int Y;
    yuvFramePtr frame = image->yuv_frame();
    int ywidth    = frame->nYBytesPerRow/4;
    int cbcrwidth = frame->nCbCrBytesPerRow;

#ifdef DEBUG_XX
    printf("YuvColorizer_422::colorize: scale=%d Y_bpr=%d UV_bpr=%d frame %dx%d x=%d y=%d %dx%d\n",
	   scale_, frame->nYBytesPerRow, frame->nCbCrBytesPerRow, frame->width, frame->height, x, y, width, height);
#endif

    u_int *pY = &((u_int *)frame->pY)
    		[(y >> scalebits) * ywidth + (x >> (scalebits+2))];
    for (int h = y; h < y+height; h += scaler) {
	for (int w = x; w < x+width; w += (scaler<<3)) {
	    Y =                yp[0];
	    Y =     (Y << 8) + yp[scaler];
	    yp += scaler<<1;

	    Y =     (Y << 8) + yp[0];
	    pY[0] = (Y << 8) + yp[scaler];
	    yp += scaler<<1;

	    Y =                yp[0];
	    Y =     (Y << 8) + yp[scaler];
	    yp += scaler<<1;

	    Y =     (Y << 8) + yp[0];
	    pY[1] = (Y << 8) + yp[scaler];
	    yp += scaler<<1;

	    pY += 2;
	}
	yp += (iw << scalebits)-width;
	pY += ywidth-(width >> (scalebits+2));
    }

    register const u_char* u_data = frm + size_;
    register const u_char* v_data = u_data + (size_ >> 1);
    register u_int Cb, Cr;
    register u_char *up, *vp;
    u_int *pCb, *pCr;

    for (int h = trunc2(y); h < round2(y+height); h += (scaler<<1)) {
	int cindex = h/(scaler<<1) * cbcrwidth + x/(scaler<<1);
	cindex &= ~3;
	pCb = (u_int *)&frame->pCb[cindex];
	pCr = (u_int *)&frame->pCr[cindex];
	cstart = (h * iw + x)>>1;
	up = (u_char *)&u_data[cstart];
	vp = (u_char *)&v_data[cstart];
	for (int w = x; w < x+width; w += (scaler<<3)) {
	    Cb =                 up[0];
	    Cr =                 vp[0];
	    Cb =     (Cb << 8) + up[scaler];
	    Cr =     (Cr << 8) + vp[scaler];

	    up += scaler<<1;
	    vp += scaler<<1;
		
	    Cb =     (Cb << 8) + up[0];
	    Cr =     (Cr << 8) + vp[0];
	    pCb[0] = (Cb << 8) + up[scaler];
	    pCr[0] = (Cr << 8) + vp[scaler];

	    up += scaler<<1;
	    vp += scaler<<1;

	    pCb++;
	    pCr++;
        }
    }

    if (frame->pHints) {
	yuvBytePtr pHints;
	int yimagewidth=frame->nYBytesPerRow/16;
	for (int h = trunc16(y); h < round16(y+height); h += (scaler<<1)) {
	    pHints = &frame->pHints[h/(scaler<<4)*yimagewidth+x/(scaler<<4)];
	    for (int w = x; w < (x+width); w += (scaler<<4))
		*pHints++ = 0xff;
	}
    }
}

void YuvColorizer_422::colorize(SharedVideoImage* image,
			       const u_char *y_data,
			       const u_char* u_data,
			       const u_char *v_data,
			       int x, int y, int width, int height) const
{
    if (image->yuvScale != 0) {
	yuvError status;
	if ((status = yuvSetScaleFactor(image->yuv_window(), 1.0)) != YUV_OK)
	    fprintf(stderr, "yuvSetScaleFactor: error = %d\n", status);
	image->yuvScale = 0;
    }

    colorize_422(image,
		  y_data, u_data, v_data,
		  x, y, width, height);
}

void Int2YuvColorizer_422::colorize(SharedVideoImage* image,
				   const u_char *y_data,
				   const u_char* u_data,
				   const u_char *v_data,
				   int x, int y, int width, int height) const
{
    if (image->yuvScale != -1) {
	yuvError status;
	if ((status = yuvSetScaleFactor(image->yuv_window(), 2.0)) != YUV_OK)
	    fprintf(stderr, "yuvSetScaleFactor: error = %d\n", status);
	image->yuvScale = -1;
    }

    colorize_422(image,
		  y_data, u_data, v_data,
		  x, y, width, height);
}

#if 0
YuvColorizer_420::YuvColorizer_420(const YuvColorModel& cm, int w, int h, int s)
{
    printf("new YuvColorizer_420\n");
}
#endif

YuvColorizer_420::~YuvColorizer_420()
{
    printf("delete YuvColorizer_420\n");
}

void YuvColorizer::colorize_420(SharedVideoImage* image,
			       register const u_char *y_data,
			       register const u_char* u_data,
			       register const u_char *v_data,
			       int x, int y, int width, int height) const
{
    int scaler = 1<<scale_, scalebits = scale_;
    u_int iw = inw_;
    u_int pstart = y * iw + x, cstart;
    register u_int Y;
    yuvFramePtr frame = image->yuv_frame();
    int ywidth    = frame->nYBytesPerRow/4;
    int cbcrwidth = frame->nCbCrBytesPerRow;

#ifdef DEBUG_XX
    printf("YuvColorizer_420::colorize: scale=%d Y_bpr=%d UV_bpr=%d frame %dx%d x=%d y=%d %dx%d\n",
	   scale_, frame->nYBytesPerRow, frame->nCbCrBytesPerRow, frame->width, frame->height, x, y, width, height);
#endif

    const register u_char *yp = &y_data[pstart];
    register u_int *pY = &((u_int *)frame->pY)
    		[(y >> scalebits) * ywidth + (x >> (scalebits+2))];
    for (int h = y; h < y+height; h += scaler) {
	for (int w = x; w < x+width; w += (scaler<<3)) {
	    Y =                yp[0];
	    Y =     (Y << 8) + yp[scaler];
	    yp += scaler<<1;

	    Y =     (Y << 8) + yp[0];
	    pY[0] = (Y << 8) + yp[scaler];
	    yp += scaler<<1;

	    Y =                yp[0];
	    Y =     (Y << 8) + yp[scaler];
	    yp += scaler<<1;

	    Y =     (Y << 8) + yp[0];
	    pY[1] = (Y << 8) + yp[scaler];
	    yp += scaler<<1;

	    pY += 2;
	}
	yp += (iw << scalebits)-width;
	pY += ywidth-(width >> (scalebits+2));
    }

    if (u_data && v_data) {
	register u_int Cb, Cr;
	const register u_char *up, *vp;
	register u_int *pCb, *pCr;

#ifdef DEBUG_XX
	if (scaler==2)
	  fprintf(stderr, "u_data=%x v_data=%x pCb=%x pCr=%x\n",
		  u_data, v_data, frame->pCb, frame->pCr);
#endif

	for (int h = trunc2(y); h < round2(y+height); h += (scaler<<1)) {
	    int cindex = h/(scaler<<1) * cbcrwidth + x/(scaler<<1);
	    cindex &= ~3;
	    pCb = (u_int *)&frame->pCb[cindex];
	    pCr = (u_int *)&frame->pCr[cindex];
	    cstart = ((h * iw >> 1) + x) >> 1;
	    up = (u_char *)&u_data[cstart];
	    vp = (u_char *)&v_data[cstart];
#ifdef DEBUG_XX
	    if (scaler==2)
	      fprintf(stderr, "%dx%d-%dx%d up=%x vp=%x pCb=%x pCr=%x\n",
		      x, h, x+width, h, up, vp, pCb, pCr);
#endif
	    for (int w = x; w < (x+width); w += (scaler<<3)) {
		Cb =                 up[0];
		Cr =                 vp[0];
		Cb =     (Cb << 8) + up[scaler];
		Cr =     (Cr << 8) + vp[scaler];

		up += scaler<<1;
		vp += scaler<<1;
		
		Cb =     (Cb << 8) + up[0];
		Cr =     (Cr << 8) + vp[0];
		pCb[0] = (Cb << 8) + up[scaler];
		pCr[0] = (Cr << 8) + vp[scaler];

		up += scaler<<1;
		vp += scaler<<1;

		pCb++;
		pCr++;
	    }
	}
    }

    if (frame->pHints) {
	yuvBytePtr pHints;
	int yimagewidth = frame->nYBytesPerRow/16;
	for (int h = trunc16(y); h < round16(y+height); h += (scaler<<1)) {
	    pHints = &frame->pHints[h/(scaler<<4)*yimagewidth+x/(scaler<<4)];
	    for (int w = x; w < (x+width); w += (scaler<<4))
		*pHints++ = 0xff;
	}
    }
}

void YuvColorizer_420::colorize(SharedVideoImage* image,
			       const u_char *y_data,
			       const u_char* u_data,
			       const u_char *v_data,
			       int x, int y, int width, int height) const
{
    if (image->yuvScale != 0) {
	yuvError status;
	if ((status = yuvSetScaleFactor(image->yuv_window(), 1.0)) != YUV_OK)
	    fprintf(stderr, "yuvSetScaleFactor: error = %d\n", status);
	image->yuvScale = 0;
    }

    colorize_420(image,
		 y_data, u_data, v_data,
		 x, y, width, height);
}

void Int2YuvColorizer_420::colorize(SharedVideoImage* image,
				   const u_char *y_data,
				   const u_char* u_data,
				   const u_char *v_data,
				   int x, int y, int width, int height) const
{
    if (image->yuvScale != -1) {
	yuvError status;
	if ((status = yuvSetScaleFactor(image->yuv_window(), 2.0)) != YUV_OK)
	    fprintf(stderr, "yuvSetScaleFactor: error = %d\n", status);
	image->yuvScale = -1;
    }

    colorize_420(image,
		 y_data, u_data, v_data,
		 x, y, width, height);
}

#if 0
YuvColorizer16_420::YuvColorizer16_420(const YuvColorModel& cm, int w, int h, int s)
{
    printf("new YuvColorizer16_420\n");
}
#endif

YuvColorizer16_420::~YuvColorizer16_420()
{
    printf("delete YuvColorizer16_420\n");
}

void YuvColorizer16_420::colorize(SharedVideoImage* image,
				 register const u_char *y_data,
				 register const u_char* u_data,
				 register const u_char *v_data,
				 int x, int y, int width, int height) const
{
    printf("YuvColorizer16_420::colorize: \n");
}


#if 0
YuvGrayColorizer::YuvGrayColorizer(const YuvColorModel& cm, int w, int h, int s)
{
    printf("new YuvGrayColorizer\n");
}
#endif

YuvGrayColorizer::~YuvGrayColorizer()
{
    printf("delete YuvGrayColorizer\n");
}

void YuvColorizer::colorize_gray(SharedVideoImage* image,
				register const u_char *y_data,
				register const u_char* u_data,
				register const u_char *v_data,
				int x, int y, int width, int height) const
{
    int scaler = 1<<scale_, scalebits = scale_;
    u_int iw = inw_;
    u_int pstart = y * iw + x;
    register u_int Y;
    yuvFramePtr frame = image->yuv_frame();
    int ywidth    = frame->nYBytesPerRow/4;
    int cbcrwidth = frame->nCbCrBytesPerRow;

    const u_char *yp = &y_data[pstart];
    u_int *pY = &((u_int *)frame->pY)
    		[(y >> scalebits) * ywidth + (x >> (scalebits+2))];
    for (int h = y; h < y+height; h += scaler) {
	for (int w = x; w < x+width; w += (scaler<<3)) {
	    Y = yp[0];
	    Y = (Y << 8) + yp[scaler];
	    yp += scaler<<1;

	    Y = (Y << 8) + yp[0];
	    pY[0] = (Y << 8) + yp[scaler];
	    yp += scaler<<1;

	    Y = yp[0];
	    Y = (Y << 8) + yp[scaler];
	    yp += scaler<<1;

	    Y = (Y << 8) + yp[0];
	    pY[1] = (Y << 8) + yp[scaler];
	    yp += scaler<<1;
	    pY += 2;
	}
	yp += (iw << scalebits)-width;
	pY += ywidth-(width >> (scalebits+2));
    }

    /* XXX This should only be done in constructor */
    int  CbCr = 128;
    yuvFillFrame (frame, (int *)0, &CbCr, &CbCr);

    if (frame->pHints) {
	yuvBytePtr pHints;
	int yimagewidth=frame->nYBytesPerRow/16;
	for (int h = trunc16(y); h < round16(y+height); h += (scaler<<1)) {
	    pHints = &frame->pHints[h/(scaler<<4)*yimagewidth+x/(scaler<<4)];
	    for (int w = x; w < (x+width); w += (scaler<<4))
		*pHints++ = 0xff;
	}
    }
}

void YuvGrayColorizer::colorize(SharedVideoImage* image,
			       const u_char *y_data,
			       const u_char* u_data,
			       const u_char *v_data,
			       int x, int y, int width, int height) const
{
    if (image->yuvScale == 0) {
	yuvError status;
	if ((status = yuvSetScaleFactor(image->yuv_window(), 1.0)) != YUV_OK)
	    fprintf(stderr, "yuvSetScaleFactor: error = %d\n", status);
	image->yuvScale = 0;
    }

    colorize_gray(image,
		  y_data, u_data, v_data,
		  x, y, width, height);
}

void Int2YuvGrayColorizer::colorize(SharedVideoImage* image,
				   const u_char *y_data,
				   const u_char* u_data,
				   const u_char *v_data,
				   int x, int y, int width, int height) const
{
    if (image->yuvScale != -1) {
	yuvError status;
	if ((status = yuvSetScaleFactor(image->yuv_window(), 2.0)) != YUV_OK)
	    fprintf(stderr, "yuvSetScaleFactor: error = %d\n", status);
	image->yuvScale = -1;
    }

    colorize_gray(image,
		  y_data, u_data, v_data,
		  x, y, width, height);
}


YuvWindowRenderer::YuvWindowRenderer(VideoWindow* w, int decimation)
	: WindowRenderer(w, decimate), image_(0), colorizer_(0)
{
}

YuvWindowRenderer::~YuvWindowRenderer()
{
	delete colorizer_;
}

void YuvWindowRenderer::render(const u_char* frm, int off, int x, int w, int h)
{
	colorizer_->colorize(image(), off, x, w, h);
}

void YuvWindowRenderer::alloc_image()
{
	image_ = new YuvSharedVideoImage(tk, outw_, outh_);
}

class YuvVideoImage : public SharedVideoImage {
public:
	YuvVideoImage(Tk_Window, int width, int height);
	yuvFramePtr yuvFrame_;
};

YuvVideoImage::YuvVideoImage(YuvVideoWindow* vw, int w, int h)
	: VideoImage(vw->tkwin(), w, h)
{
	w = YUV_NEXT_MULT(w, 4);
	h = YUV_NEXT_MULT(h, 4);

	yuvError status;
	if ((status = yuvCreateFrame(YUV_FORMAT_420,
				     w, h,
				     w, h,
				     ((w % 16) || (h % 16))?0:YUV_CREATE_FRAME_HINTS,
				     &yuvFrame_)) != YUV_OK) {
		fprintf(stderr, "yuvCreateFrame: failed: %d\n", status);
		exit(1);
	}
	/* Clear the frame */
	int Y = 128;
	int CbCr = 128;
	(void) yuvFillFrame(yuvFrame_, &Y, &CbCr, &CbCr);
}

YuvVideoImage::~YuvVideoImage()
{
	if (yuvFrame_)
		(void)yuvDestroyFrame(yuvFrame_);
}

YuvVideoWindow::YuvVideoWindow(const char* name, int width, int height,
			       YuvColorModel& cm)
	: VideoWindow(name, width, height)
{
        yuvError status;
	Tcl& tcl = Tcl::instance();
	int screen = Tk_ScreenNumber(tcl.tkmain());

	Tk_SetWindowVisual(tk_, cm.visual(), cm.depth(), cm.colormap());
	Tk_MakeWindowExist(tk_);
	XSync(dpy_, False);

#ifdef DEBUG
	printf("Register wid=%x with YUVlib\n", Tk_WindowId(tk));
#endif

	if ((status = yuvRegisterWindow(cm.yuv_handle(), Tk_WindowId(tk), &yuvWindow_)) != YUV_OK) {
		fprintf(stderr, "yuvRegister: failed: %d\n", status);
		return -1;
	}

	/* Give a slight boost to the brightness */
	if ((status = yuvAdjustColor(yuvWindow_, 50.0, 1.0, 1.0)) != YUV_OK) {
		fprintf(stderr, "yuvAdjustColor: failed: %d\n", status);
	}
}

YuvVideoWindow::~YuvVideoWindow()
{
	if (yuvWindow_)
		(void)yuvUnregisterWindow(yuvWindow_);
}

void YuvVideoImage::putimage(Display* dpy, Window window, GC gc,
			     int sx, int sy, int x, int y,
			     int w, int h) const
{
	yuvError status;
#ifdef DEBUG_XX
	printf("displayFrame %x for window %x\n", vi_->yuv_frame(), vi_->yuv_window());
#endif
	/*XXX no bounding box? */
	if ((status = yuvDisplayFrame(vi_->yuv_window(), (yuvFrameRec*)vi_->yuv_frame(), 0)) != YUV_OK)
			fprintf(stderr, "yuvDisplayFrame: failed: %d\n", status);
	}
}
#endif	/* HP_YUVLIB */
