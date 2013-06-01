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

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>

#ifdef USE_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#if !defined(__svr4__) && !defined(__hpux)
extern "C" {
#include <sys/shm.h>
int shmget(key_t, int, int);
char *shmat(int, char*, int);
int shmdt(char*);
int shmctl(int, int, struct shm_ds *);
}
#endif
#endif

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef USE_SHM
extern "C" {
#include <X11/extensions/XShm.h>
}
#endif

/*XXX*/
#ifdef __svr4__
extern "C" {
#include <XPlxExt.h>
}
#else
#include "plxfuncs.h"
#endif

#include "grabber.h"
#include "module.h"
#include "vw.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "transmitter.h"
#include "rgb-converter.h"

#define NTSC_WIDTH 640
#define NTSC_HEIGHT 480
#define PAL_WIDTH 768
#define PAL_HEIGHT 576

class PlxWindow : public BareWindow {
public:
	PlxWindow(const char* name, XVisualInfo*, int port);
	virtual ~PlxWindow();
	void capture(u_int8_t* frm);
	void setsize(int w, int h);

	void port(const char *s);
	inline void hue(int v) { hue_ = v; setctrl(); }
	inline void saturation(int v) { saturation_ = v; setctrl(); }
	inline void brightness(int v) { brightness_ = v; setctrl(); }
	inline void contrast(int v) { contrast_ = v; setctrl(); }

	void freeze_frame(sigset_t* ss);
	void release_frame(sigset_t* ss);
	virtual void redraw() { need_refresh_ = 1; }

	inline int basewidth() { return (format_width_); }
	inline int baseheight() { return (format_height_); }

	void setq(int q);
	inline GC gc() const { return (gc_); }
protected:
	GC gc_;
	int need_refresh_;
	u_int format_width_, format_height_, format_offset_;
	int hue_, saturation_, brightness_, contrast_;
	int port_;
	int format_;
	void setctrl();
};

class PlxGrabber : public Grabber {
protected:
	PlxGrabber();
	virtual ~PlxGrabber();
public:
	virtual int command(int argc, const char *const *argv);
	void init();
protected:
	int lookup_visual();
	virtual void setsize() = 0;
	int decimate_;
	PlxWindow* capwin_;
	XVisualInfo vinfo_;
};

class Plx422Grabber : public PlxGrabber {
public:
	Plx422Grabber();
	~Plx422Grabber();
	virtual int grab();
	virtual void setsize();
protected:
	void capture();
	StandardVideoImage* image_;
	RGB_Converter* converter_;
};

class Plx420Grabber : public Plx422Grabber {
public:
	virtual void setsize();
};

class PlxCIFGrabber : public Plx422Grabber {
public:
	virtual void setsize();
};

class PlxJpegGrabber : public PlxGrabber {
public:
	PlxJpegGrabber();
	~PlxJpegGrabber();
	virtual int grab();
	virtual void setsize();
	virtual int command(int argc, const char *const *argv);
protected:
	int capture(u_int8_t* bp);
	int q_;
	XPlxCImage *cimage_;
};

static class PlxDevice : public InputDevice {
 public:
	PlxDevice(const char*);
	virtual int command(int argc, const char*const* argv);
} plx_device("parallax");

PlxDevice::PlxDevice(const char *s) : InputDevice(s) 
{
	attributes_ = "\
format { 422 420 jpeg } \
size { small large cif } \
port { Input-1 Input-2 }";
}

int PlxDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3 && strcmp(argv[1], "open") == 0) {
		const char* fmt = argv[2];
		TclObject* o = 0;
		if (strcmp(fmt, "jpeg") == 0)
			o = new PlxJpegGrabber;
		else if (strcmp(fmt, "422") == 0)
			o = new Plx422Grabber;
		else if (strcmp(fmt, "420") == 0)
			o = new Plx420Grabber;
		else if (strcmp(fmt, "cif") == 0)
			o = new PlxCIFGrabber;
		if (o != 0)
			Tcl::instance().result(o->name());
		return (TCL_OK);
	}
	return (InputDevice::command(argc, argv));
}

PlxWindow::PlxWindow(const char* name, XVisualInfo* vinfo, int port)
	: BareWindow(name, vinfo)
{
	gc_ = Tk_GetGC(tk_, 0, 0);
	
	/*
	 * query the server to get hardware/signal information.
	 */
	Tk_MakeWindowExist(tk_);
	Window window = Tk_WindowId(tk_);
	plx_IO* plxio = XPlxQueryConfig(dpy_, window, gc_);
	if (plxio->hardware != PLX_XVIDEO_24)
		/* shouldn't happen. */
		abort();

	/* go through the list of signals and find an active signal */

	plx_IO_list *il;
	/* find a pointer to channel one input */
	for (il = plxio->inputs; il->channel != port; il = il->next_IO)
		;

	plx_signal* sig = 0;
	plx_signal_list *sl;
	for (sl = il->signal_list; sl != 0; sl = sl->next_signal) {
		/*
		 * do an input select
		 * we are only using input 0 so it, and PLX_COMP are hard coded
		 * we also know that all the XVideo hardware can do is PLX_RGB24
		 */
		format_ = sl->signal->standard;
		XPlxVideoInputSelect(dpy_, window, gc_, port,
				     format_, PLX_COMP, PLX_RGB24);

		/* now do a query video to find out if there is sync */
		sig = XPlxQueryVideo(dpy_, window, gc_);
		if (sig->sync_ok)
			break;
	}
	if (sig == 0) {
		/*XXX don't exit!*/
		fprintf(stderr, "no video connected to input 0\n");
		exit(1);
	}
	port_ = port;
	/* maintain 4:3 aspect ratio */
	format_width_ = sig->w;
	format_height_ = (format_width_ * 3) / 4;
	format_offset_ = sig->h - format_height_ + 1;

	XPlxVideoTag(dpy_, window, gc_, PLX_VIDEO);
	XPlxVideoValueQuery(dpy_, window, gc_,
			    &saturation_, &contrast_, &hue_, &brightness_);
}

PlxWindow::~PlxWindow()
{
	Tk_FreeGC(dpy_, gc_);
}

void PlxWindow::setsize(int w, int h)
{
	BareWindow::setsize(w, h);
	/* Start the video display */
	Tcl::instance().evalc("update idletasks");/*XXX*/
	XPlxVideoSqueezeLive(dpy_, Tk_WindowId(tk_), gc_,
			     0, format_offset_, format_width_, format_height_,
			     0, 0, width_, height_);
}

void PlxWindow::setctrl()
{
	if (tk_ != 0)
		XPlxVideoValueLoad(dpy_, Tk_WindowId(tk_), gc_,
				   saturation_, contrast_, hue_, brightness_);
}

void PlxWindow::port(const char* s)
{
	port_ = (strcasecmp(s, "input-1") == 0) ? PLX_INPUT_0 : PLX_INPUT_1;
	/*
	 * XXX This won't work if we have different signal types
	 * on the different ports.
	 */
	XPlxVideoInputSelect(dpy_, Tk_WindowId(tk_), gc_,
			     port_, format_, PLX_COMP, PLX_RGB24);
}

void PlxWindow::freeze_frame(sigset_t* ss)
{
	sigset_t fullset;
	sigfillset(&fullset);
	sigprocmask(SIG_SETMASK, &fullset, ss);
#if GRAB_SERVER
	XGrabServer(dpy_);
#endif
#ifdef notdef
	/*XXX need toplevel window */
	XRaiseWindow(dpy_, Tk_WindowId(tk_));
#endif
	if (need_refresh_) {
		XPlxVideoTag(dpy_, Tk_WindowId(tk_), gc_, PLX_VIDEO);
		XPlxVideoSqueezeLive(dpy_, Tk_WindowId(tk_), gc_,
				     0, format_offset_,
				     format_width_, format_height_,
				     0, 0, width_, height_);
	}
#ifdef notdef
	XSync(dpy_, 0);
	XPlxVideoSqueezeStill(dpy_, Tk_WindowId(tk_), gc_,
		0, format_offset_, format_width_, format_height_,
		0, 0, width_, height_);
#endif
}

void PlxWindow::release_frame(sigset_t* ss)
{
	XUngrabServer(dpy_);
	sync();
	(void)sigprocmask(SIG_SETMASK, ss, 0);
	if (need_refresh_) {
		XPlxVideoTag(dpy_, Tk_WindowId(tk_), gc_, PLX_VIDEO_OVR);
		need_refresh_ = 0;
	}
}

extern int quality_to_qfactor(int);

void PlxWindow::setq(int q)
{
	char *table;
	int size = MakeQTables(quality_to_qfactor(q), (unsigned char**)&table);
	XPlxPutTable(dpy_, Tk_WindowId(tk_), gc_,
#ifdef __hpux
		     (short *)
#endif
		     table, size, 0 /* compress */);
	free(table);
}

PlxGrabber::PlxGrabber() : capwin_(0), decimate_(0)
{
	if (lookup_visual() < 0)
		status_ = -1;
}

PlxGrabber::~PlxGrabber()
{
}

int PlxGrabber::lookup_visual()
{
	Tk_Window tk = Tcl::instance().tkmain();
	Display* dpy = Tk_Display(tk);
	int majop, eventbase, errbase;
	if (XQueryExtension(dpy, "ParallaxVideo", &majop,
			    &eventbase, &errbase) &&
	    (XMatchVisualInfo(Tk_Display(tk), Tk_ScreenNumber(tk),
			      24, TrueColor, &vinfo_) ||
	     XMatchVisualInfo(Tk_Display(tk), Tk_ScreenNumber(tk),
			      32, TrueColor, &vinfo_))) {
		return (0);
	}
	return (-1);
}

int PlxGrabber::command(int argc, const char * const*argv)
{
	Tcl &tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "need-capwin") == 0) {
			tcl.result("1");
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "decimate") == 0) {
			int d = atoi(argv[2]);
			if (d <= 0) {
				Tcl_AppendResult(tcl.interp(),
					"%s: divide by zero",
					argv[0], 0);
				return TCL_ERROR;
			}
			if (d != decimate_) {
				decimate_ = d;
				setsize();
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "port") == 0) {
			capwin_->port(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "contrast") == 0) {
			capwin_->contrast(atoi(argv[2]));
			return (TCL_OK);
		} else if (strcmp(argv[1], "brightness") == 0) {
			capwin_->brightness(atoi(argv[2]));
			return (TCL_OK);
		} else if (strcmp(argv[1], "hue") == 0) {
			capwin_->hue(atoi(argv[2]));
			return (TCL_OK);
		} else if (strcmp(argv[1], "saturation") == 0) {
			capwin_->saturation(atoi(argv[2]));
			return (TCL_OK);
		} else if (strcmp(argv[1], "create-capwin") == 0) {
			/*XXX port*/
			capwin_ = new PlxWindow(argv[2], &vinfo_, PLX_INPUT_0);
			tcl.result(capwin_->name());
			return (TCL_OK);
		}
	}
	return (Grabber::command(argc, argv));
}

Plx422Grabber::Plx422Grabber() : image_(0), converter_(0)
{
}

Plx422Grabber::~Plx422Grabber()
{
	delete[] image_; //SV-XXX: Debian
}

void Plx422Grabber::capture()
{
	/*
	 * Grab the frame.
	 */
	sigset_t ss;
	capwin_->freeze_frame(&ss);
	Tk_Window tk = capwin_->tkwin();
	/*
	 * The parallax X server doesn't seem to support xshmgetimage.
	 */
	XImage* image = image_->ximage();
#ifdef USE_SHM
	if (image->obdata != 0)
		XShmGetImage(Tk_Display(tk), Tk_WindowId(tk), image,
			     0, 0, AllPlanes);
	else
#endif
		XGetSubImage(Tk_Display(tk), Tk_WindowId(tk),
			     0, 0, image->width, image->height,
			     AllPlanes, ZPixmap, image, 0, 0);
	capwin_->release_frame(&ss);
	converter_->convert((u_int8_t*)image->data,
			    image->width, image->height, frame_);
}

void Plx422Grabber::setsize()
{
	int w = capwin_->basewidth() / decimate_;
	int h = capwin_->baseheight() / decimate_;
	capwin_->setsize(w, h);
	converter_ = RGB_Converter_422::instance();
	image_ = StandardVideoImage::allocate(capwin_->tkwin(), w, h);
	set_size_422(w, h);
	allocref();
}

int Plx422Grabber::grab()
{
	capture();
	suppress(frame_);
	saveblks(frame_);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

void Plx420Grabber::setsize()
{
	int w = capwin_->basewidth() / decimate_;
	int h = capwin_->baseheight() / decimate_;
	capwin_->setsize(w, h);
	converter_ = RGB_Converter_420::instance();
	image_ = StandardVideoImage::allocate(capwin_->tkwin(), w, h);
	set_size_420(w, h);
	allocref();
}

void PlxCIFGrabber::setsize()
{
	int w = 2 * 352 / decimate_;
	int h = 2 * 288 / decimate_;
	capwin_->setsize(w, h);
	converter_ = RGB_Converter_420::instance();
	image_ = StandardVideoImage::allocate(capwin_->tkwin(), w, h);
	set_size_cif(w, h);
	allocref();
}

PlxJpegGrabber::PlxJpegGrabber() : q_(50), cimage_(0)
{
}

PlxJpegGrabber::~PlxJpegGrabber()
{
	/*XXX free cimage_ */
}

void PlxJpegGrabber::setsize()
{
	int w = capwin_->basewidth() / decimate_;
	int h = capwin_->baseheight() / decimate_;
	capwin_->setsize(w, h);
	inw_ = outw_ = w;
	inh_ = outh_ = h;
#ifdef notdef
	Tk_Window tk = capwin_->tkwin();
	if (cimage_ != 0)
		(void)XPlxDestroyCImage(cimage_);
	cimage_ = XPlxCreateCImage(Tk_Display(tk), DATA, SIZE, w, h);
#endif
}

int PlxJpegGrabber::capture(u_int8_t* bp)
{
	sigset_t ss;
	capwin_->freeze_frame(&ss);
	XPlxCImage* p;
	Tk_Window tk = capwin_->tkwin();
	int w = capwin_->width();
	int h = capwin_->height();
#ifdef USE_SHM_notyet
	p = cimage_;
	XPlxShmGetCImage(Tk_Display(tk), Tk_WindowId(tk),
			 capwin_->gc(), cimage_, 0, 0, w, h);
#else
	p = XPlxGetCImage(Tk_Display(tk), Tk_WindowId(tk),
			  capwin_->gc(), 0, 0, w, h, w, h);
#endif
	capwin_->release_frame(&ss);
	/*XXX*/
	memcpy(bp, p->data, p->size);
	int size = p->size;

#ifndef USE_SHM_notyet
	XPlxDestroyCImage(p);
#endif

	return (size);
}

int PlxJpegGrabber::grab()
{
	/*
	 * Make sure the previous frame has been completely transmitted
	 * before we overwrite the buffer.
	 */
	tx_->flush();
	/*XXX*/
	static u_int32_t wrk[80*1024 / 4];
	int len = capture((u_int8_t*)wrk);

	JpegFrame f(media_ts(), (u_int8_t*)wrk, len, q_, 0, outw_, outh_);
	return (target_->consume(&f));
}

int PlxJpegGrabber::command(int argc, const char* const *argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "q") == 0) {
			/* assume value is in range */
			q_ = atoi(argv[2]);
			capwin_->setq(q_);
			return (TCL_OK);
		}
	}
	return (PlxGrabber::command(argc, argv));
}
