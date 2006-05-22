/*
 * Copyright (c) 1993-1994 Regents of the University of California.
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
static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vw.h"
#include "color.h"
#include "rgb-converter.h"

extern "C" {
#include <tk.h>

#ifdef USE_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#if defined(sun) && !defined(__svr4__)
int shmget(key_t, int, int);
char *shmat(int, char*, int);
int shmdt(char*);
int shmctl(int, int, struct shmid_ds*);
#endif
#ifdef __osf__
int XShmGetEventBase(struct _XDisplay *);
#else
int XShmGetEventBase(Display *);
#endif
#ifdef sgi_old
#define XShmAttach __XShmAttach__
#define XShmDetach __XShmDetach__
#define XShmPutImage __XShmPutImage__
#endif
#include <X11/extensions/XShm.h>
#ifdef sgi_old
#undef XShmAttach
#undef XShmDetach
#undef XShmPutImage
int XShmAttach(Display*, XShmSegmentInfo*);
int XShmDetach(Display*, XShmSegmentInfo*);
int XShmPutImage(Display*, Drawable, GC, XImage*, int, int, int, int,
		 int, int, int);
#endif
#endif
}

static class VideoCommand : public TclObject {
public:
	VideoCommand(const char* name) : TclObject(name) { }
protected:
	int command(int argc, const char*const* argv);
} video_cmd("video");

/*
 * video $path $width $height
 */
int VideoCommand::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc != 4) {
		tcl.result("video arg mismatch");
		return (TCL_ERROR);
	}
	const char* name = argv[1];
	int width = atoi(argv[2]);
	int height = atoi(argv[3]);
	VideoWindow* p = new VideoWindow(name);
	p->setsize(width, height);
	tcl.result(p->name());

	return (TCL_OK);
}

class SlowVideoImage : public StandardVideoImage {
public:
	SlowVideoImage(Tk_Window, int width, int height);
	~SlowVideoImage();
	void putimage(Display* dpy, Window window, GC gc,
		      int sx, int sy, int x, int y,
		      int w, int h) const;
};

VideoImage::VideoImage(Tk_Window tk, int w, int h)
	: width_(w), height_(h)
{
	dpy_ = Tk_Display(tk);
	int depth = Tk_Depth(tk);
	/*XXX*/
	bpp_ =  (depth == 24) ? 32 : depth;
}

VideoImage::~VideoImage()
{
}

StandardVideoImage* StandardVideoImage::allocate(Tk_Window tk, int w, int h)
{
#ifdef USE_SHM
	extern int use_shm;
	if (use_shm) {
		SharedVideoImage* p = new SharedVideoImage(tk, w, h);
		if (p->valid())
			return (p);
		delete p;
	}
#endif
	return (new SlowVideoImage(tk, w, h));
}

StandardVideoImage::StandardVideoImage(Tk_Window tk, int w, int h)
	: VideoImage(tk, w, h)
{
	image_ = XCreateImage(dpy_, Tk_Visual(tk), Tk_Depth(tk),
			      ZPixmap, 0, (char*)0, w, h, 8, 0);
}

StandardVideoImage::~StandardVideoImage()
{
	/*XXX*/
#ifndef WIN32
	XSync(dpy_, 0);
#endif
	image_->data = 0;
	image_->obdata = 0;
	XDestroyImage(image_);
}

SlowVideoImage::SlowVideoImage(Tk_Window tk, int w, int h)
	: StandardVideoImage(tk, w, h)
{
	int size = w * h;
	if (bpp_ > 8)
		size *= bpp_ / 8;
	image_->data = new char[size];
}

SlowVideoImage::~SlowVideoImage()
{
	delete[] image_->data; //SV-XXX: Debian
}

void SlowVideoImage::putimage(Display* dpy, Window window, GC gc,
			      int sx, int sy, int x, int y,
			      int w, int h) const
{
	XPutImage(dpy, window, gc, image_, sx, sy, x, y, w, h);
}

#ifdef USE_SHM
SharedVideoImage::SharedVideoImage(Tk_Window tk, int w, int h)
	: StandardVideoImage(tk, w, h)
{
	int size = w * h;
	if (bpp_ > 8)
		size *= bpp_ / 8;

	shminfo_.shmid = shmget(IPC_PRIVATE, size, IPC_CREAT|0777);
	if (shminfo_.shmid < 0) {
		perror("vic: shmget");
		fprintf(stderr, "\
vic: reverting to non-shared memory; you should reconfigure your system\n\
vic: with more a higher limit on shared memory segments.\n\
vic: refer to the README that accompanies the vic distribution\n");
		extern int use_shm;
		use_shm = 0;
		return;
	}
	shminfo_.shmaddr = (char*)shmat(shminfo_.shmid, 0, 0);
	if (shminfo_.shmaddr == (char*)-1) {
		perror("shmat");
		exit(1);
	}
	init(tk);
}

/*
 * side affect - shmid is detached from local addr space
 */
SharedVideoImage::SharedVideoImage(Tk_Window tk, int w, int h,
				   u_char* shmaddr, int shmid)
	: StandardVideoImage(tk, w, h)
{
	shminfo_.shmid = shmid;
	shminfo_.shmaddr = (char*)shmaddr;
	init(tk);
}

SharedVideoImage::~SharedVideoImage()
{
	if (valid()) {
		XShmDetach(dpy_, &shminfo_);
		if (shmdt(shminfo_.shmaddr) < 0)
			perror("vic: shmdt");
	}
}

void SharedVideoImage::init(Tk_Window tk)
{
        UNUSED(tk); //SV-XXX: unused

/*XXX capture-windows need to be writeable */
#ifdef notdef
	shminfo_.readOnly = 1;
#else
	shminfo_.readOnly = 0;
#endif
	XShmAttach(dpy_, &shminfo_);
	/*
	 * Once the X server has attached the shm segments,
	 * we rmid them so they will go away when we exit.
	 * The sync is to make the X server do the attach
	 * before we do the rmid.
	 */
	XSync(dpy_, 0);
	(void)shmctl(shminfo_.shmid, IPC_RMID, 0);

	/*
	 * Wrap segment in an ximage
	 */
	image_->obdata = (char*)&shminfo_;
	image_->data = shminfo_.shmaddr;
}

void SharedVideoImage::putimage(Display* dpy, Window window, GC gc,
				int sx, int sy, int x, int y,
				int w, int h) const
{
	XShmPutImage(dpy, window, gc, image_, sx, sy, x, y, w, h, 0);
}
#endif

BareWindow::BareWindow(const char* name, XVisualInfo* vinfo)
	: TclObject(name), width_(0), height_(0)
{
	Tcl& tcl = Tcl::instance();
	tk_ = Tk_CreateWindowFromPath(tcl.interp(), tcl.tkmain(),
				      (char*)name, 0);
	if (tk_ == 0)
		abort();
	Tk_SetClass(tk_, "Vic");
	Tk_CreateEventHandler(tk_, ExposureMask|StructureNotifyMask,
			      handle, (ClientData)this);
	dpy_ = Tk_Display(tk_);
	if (vinfo != 0) {
		/*XXX*/
		Colormap cm = XCreateColormap(dpy_, Tk_WindowId(tcl.tkmain()),
					      vinfo->visual, AllocNone);
		Tk_SetWindowVisual(tk_, vinfo->visual, vinfo->depth, cm);
	}
}

BareWindow::~BareWindow()
{
	Tk_DeleteEventHandler(tk_, ExposureMask|StructureNotifyMask,
			      handle, (ClientData)this);
}

int BareWindow::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "width") == 0) {
			sprintf(tcl.buffer(), "%d", width_);
			tcl.result(tcl.buffer());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "height") == 0) {
			sprintf(tcl.buffer(), "%d", height_);
			tcl.result(tcl.buffer());
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "resize") == 0) {
			setsize(atoi(argv[2]), atoi(argv[3]));
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

void BareWindow::handle(ClientData cd, XEvent* ep)
{
	BareWindow* w = (BareWindow*)cd;
	
	switch (ep->type) {
	case Expose:
		if (ep->xexpose.count == 0)
			w->redraw();
		break;

	case DestroyNotify:
		w->destroy();
		break;

#ifdef notyet
	case ConfigureNotify:
		if (w->width_ != ep->xconfigure.width ||
		    w->height_ != ep->xconfigure.height)
			;
		break;
#endif
	}
}

void BareWindow::destroy()
{
}

void BareWindow::setsize(int w, int h)
{
	width_ = w;
	height_ = h;
	Tk_GeometryRequest(tk_, w, h);
}

GC VideoWindow::gc_;

VideoWindow::VideoWindow(const char* name, XVisualInfo* vinfo)
	: BareWindow(name, vinfo),
	  vi_(0),
	  callback_pending_(0),
	  damage_(0),
	  voff_(0),
	  hoff_(0)
{
	if (gc_ == 0) {
		/*XXX should use Vic.background */
		XColor* c = Tk_GetColor(Tcl::instance().interp(), tk_, 
					Tk_GetUid("gray50"));
		if (c == 0)
			abort();
		XGCValues v;
		v.background = c->pixel;
		v.foreground = c->pixel;
		const u_long mask = GCForeground|GCBackground;
		gc_ = Tk_GetGC(tk_, mask, &v);
	}
}

VideoWindow::~VideoWindow()
{
	if (callback_pending_)
		Tk_CancelIdleCall(display, (ClientData)this);
}

int VideoWindow::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "redraw") == 0) {
			redraw();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "clear") == 0) {
			clear();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "dim") == 0) {
			dim();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "voff") == 0) {
			voff_ = atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "hoff") == 0) {
			hoff_ = atoi(argv[2]);
			return (TCL_OK);
		}
	}
	return (BareWindow::command(argc, argv));
}

void VideoWindow::display(ClientData cd)
{
	VideoWindow* vw = (VideoWindow*)cd;
	vw->callback_pending_ = 0;
	int h = (vw->vi_ != 0) ? vw->vi_->height() : vw->height_;
	vw->draw(0, h, 0, 0);
}

void VideoWindow::draw(int y0, int y1, int x0, int x1)
{
	if (callback_pending_) {
		callback_pending_ = 0;
		Tk_CancelIdleCall(display, (ClientData)this);
	}
	if (!Tk_IsMapped(tk_))
		return;

	Window window = Tk_WindowId(tk_);

	if (vi_ == 0) {
		XFillRectangle(dpy_, window, gc_, 0, 0, width_, height_);
		return;
	}
	int hoff = (width_ - vi_->width()) >> 1;
	int voff = (height_ - vi_->height()) >> 1;
	hoff += hoff_;
	voff += voff_;
	/*XXX*/
	if (damage_) {
		damage_ = 0;
		if (hoff > 0) {
			XFillRectangle(dpy_, window, gc_,
				       0, 0, hoff, height_ + 1);
			XFillRectangle(dpy_, window, gc_,
				       width_ - hoff, 0, hoff, height_ + 1);
		}
		if (voff > 0) {
			XFillRectangle(dpy_, window, gc_,
				       0, 0, width_, voff + 1);
			XFillRectangle(dpy_, window, gc_,
				       0, height_ - voff, width_, voff + 1);
		}
	}
	int h = y1 - y0;
	if (h == 0)
		h = vi_->height();
	else if (h > vi_->height())
		h = vi_->height();
	else if (h < 0)
		return;
	int w = x1 - x0;
	if (w == 0)
		w = vi_->width();
	else if (w > vi_->width())
		w = vi_->width();
	else if (w < 0)
		return;

	vi_->putimage(dpy_, window, gc_, x0, y0, x0 + hoff, y0 + voff, w, h);
}

/*XXX*/
void VideoWindow::redraw()
{
	damage_ = 1;
	if (!callback_pending_) {
		callback_pending_ = 1;
		Tk_DoWhenIdle(display, (ClientData)this);
	}
}

void VideoWindow::clear()
{
	if (!callback_pending_) {
		callback_pending_ = 1;
		Tk_DoWhenIdle(doclear, (ClientData)this);
	}
}

int VideoWindow::bpp()
{
    // XXX
    StandardVideoImage* vi = StandardVideoImage::allocate(tk_, 1, 1);
    int bpp = vi->ximage()->bits_per_pixel;
    delete vi;
    return bpp;
}

void VideoWindow::dim()
{
	if (!callback_pending_) {
		callback_pending_ = 1;
		Tk_DoWhenIdle(dodim, (ClientData)this);
	}
}

void VideoWindow::doclear(ClientData cd)
{
	VideoWindow* vw = (VideoWindow*)cd;
	vw->callback_pending_ = 0;
	if (Tk_IsMapped(vw->tk_)) {
		Window window = Tk_WindowId(vw->tk_);
		XFillRectangle(vw->dpy_, window, vw->gc_,
			       0, 0, vw->width_, vw->height_);
	}
}

void VideoWindow::dodim(ClientData cd)
{
	static Pixmap graypm; /*XXX*/
	static GC graygc; /*XXX*/

	VideoWindow* vw = (VideoWindow*)cd;
	vw->callback_pending_ = 0;
	if (Tk_IsMapped(vw->tk_)) {
		Window window = Tk_WindowId(vw->tk_);
		if (graypm == 0) {
			u_int32_t bm[32];
			for (int i = 0; i < 32; i += 2) {
				bm[i]   = 0x55555555;
				bm[i+1] = 0xaaaaaaaa;
			}
			graypm = XCreateBitmapFromData(vw->dpy_, window,
						       (const char*)bm,
						       32, 32);
			XColor* c = Tk_GetColor(Tcl::instance().interp(),
						vw->tk_, Tk_GetUid("gray50"));
			if (c == 0)
				abort();
			XGCValues v;
			v.background = c->pixel;
			v.foreground = c->pixel;
			v.fill_style = FillStippled;
			v.stipple = graypm;
			graygc = Tk_GetGC(vw->tk_,
			       GCForeground|GCBackground|GCFillStyle|GCStipple,
			       &v);
		}
		XFillRectangle(vw->dpy_, window, graygc,
			       0, 0, vw->width_, vw->height_);
	}
}

void VideoWindow::render(const VideoImage* v, int miny, int maxy,
			 int minx, int maxx)
{
	vi_ = v;
	draw(miny, maxy, minx, maxx);
}

CaptureWindow::CaptureWindow(const char* name, XVisualInfo* vinfo)
	: BareWindow(name, vinfo),
	  base_width_(0),
	  base_height_(0),
	  image_(0)
{
	gc_ = Tk_GetGC(tk_, 0, 0);
}

CaptureWindow::~CaptureWindow()
{
	Tk_FreeGC(dpy_, gc_);
	delete image_;
}

void CaptureWindow::setsize(int w, int h)
{
	BareWindow::setsize(w, h);
	delete image_;
	image_ = StandardVideoImage::allocate(tk_, width_, height_);
}

void CaptureWindow::grab_image()
{
	XImage* image = image_->ximage();
#ifdef USE_SHM
	if (image->obdata != 0)
		XShmGetImage(dpy_, Tk_WindowId(tk_), image,
			     0, 0, AllPlanes);
	else
#endif
		XGetSubImage(Tk_Display(tk_), Tk_WindowId(tk_),
			     0, 0, image->width, image->height,
			     AllPlanes, ZPixmap, image, 0, 0);
}
			
void CaptureWindow::capture(u_int8_t* frm)
{
	/*
	 * Xv requires that the window be unobscured in order
	 * to capture the video.  So we grab the server and
	 * raise the window.  This won't work if the window
	 * isn't mapped.  Also, we block signals while
	 * the server is grabbed.  Otherwise, the process
	 * could be stopped while the display is locked out.
	 */
	if (Tk_IsMapped(tk_)) {
		raise();
		grab_image();
		converter_->convert((u_int8_t*)image_->pixbuf(),
				    width_, height_, frm);
	}
}
