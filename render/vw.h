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
 *
 * @(#) $Header$ (LBL)
 */

#ifndef vic_video_h
#define vic_video_h

#include "vic_tcl.h"


extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef USE_SHM
#include <X11/extensions/XShm.h>
#endif
#include <tk.h>
#include "xvideo.h"
}

class VideoWindow;
class RGB_Converter;

/*
 * An image to be displayed in a VideoWindow.
 */
class VideoImage {
    protected:
	VideoImage(Tk_Window, int width, int height);
    public:
	virtual ~VideoImage();
	inline int width() const { return (width_); }
	inline int height() const { return (height_); }
	virtual void putimage(Display* dpy, Window window, GC gc,
			      int sx, int sy, int x, int y,
			      int w, int h) const = 0;
    protected:
	int bpp_;		/* bits per pixel (XXX must be 1,8,16,32) */
	int width_;
	int height_;
	Display* dpy_;/*XXX*/
};

class StandardVideoImage : public VideoImage {
    protected:
	StandardVideoImage(Tk_Window, int width, int height);
    public:
	virtual ~StandardVideoImage();
	static StandardVideoImage* allocate(Tk_Window, int width, int height);
	inline u_char* pixbuf() { return ((u_char*)image_->data); }
	inline XImage* ximage() { return (image_); }
    protected:
	XImage* image_;
};

/* 
 * XVideoImage supports shared memory, 
 * Xvideo extension, and basic X11 rendering
 */

class XVideoImage : public VideoImage {
    protected:
	XVideoImage(Tk_Window, int width, int height);
    public:
	static XVideoImage* allocate(Tk_Window, int width, int height);
	inline u_char* pixbuf() { return ((u_char*)image_->data); }
	inline IMAGE_TYPE* ximage() { return (image_); }
	void putimage(Display* dpy, Window window, GC gc,
		      int sx, int sy, int x, int y,int w, int h) const;
        static inline bool is_supported() { return enable_xv; }        		      
        
    protected:
	IMAGE_TYPE* image_;
#ifdef HAVE_XVIDEO
	XRender render;
#endif
	static bool enable_xv;
};
#ifdef USE_SHM
/*
 * A class for ximages, which will be allocate in shared memory
 * if available.  The constructor takes the width and height
 * of the bitmap, plus a size which must be >= width*height.
 * The size might want to be bigger than width*height for decoders
 * that might walk off the end of an image because of bit errors,
 * but the distance they walk off the end is bounded.
 */
class SharedVideoImage : public StandardVideoImage {
    public:
	SharedVideoImage(Tk_Window, int width, int height);
	SharedVideoImage(Tk_Window, int width, int height,
			 u_char* shmaddr, int shmid);
	virtual ~SharedVideoImage();
	inline int shmid() const { return (shminfo_.shmid); }
	inline char* buffer() const { return (shminfo_.shmaddr); }
	void putimage(Display* dpy, Window window, GC gc,
		      int sx, int sy, int x, int y,
		      int w, int h) const;
	inline int valid() const { return (shminfo_.shmid >= 0); }
    protected:
	void init(Tk_Window tk);
	XShmSegmentInfo	shminfo_;
};
#endif

class BareWindow : public TclObject {
    public:
	BareWindow(const char* name, XVisualInfo* vinfo = 0);
	~BareWindow();
	virtual int command(int argc, const char*const* argv);
	inline Tk_Window tkwin() { return (tk_); }
	inline int width() { return (width_); }
	inline int height() { return (height_); }
	virtual void setsize(int w, int h);

	inline void map() { Tk_MapWindow(tk_); }
	inline void unmap() { Tk_UnmapWindow(tk_); }
	void sync();
	inline void raise() { XRaiseWindow(dpy_, Tk_WindowId(tk_)); }

	virtual void redraw() {}
	virtual void destroy();
    protected:
	void draw(int miny, int maxy);

	Display* dpy_;
	Tk_Window tk_;
	int width_;
	int height_;
    private:
	static void handle(ClientData, XEvent*);
};

inline void BareWindow::sync()
{
#if !defined(WIN32) && !defined(MAC_OSX_TK)
	XSync(Tk_Display(tk_), 0);
#endif
}

class VideoWindow : public BareWindow {
    public:
	VideoWindow(const char* name, XVisualInfo* vinfo = 0);
	~VideoWindow();
	virtual int command(int argc, const char*const* argv);
	void setimage(VideoImage* v) { vi_ = v; }
	void render(const VideoImage* vi, int miny = 0, int maxy = 0,
		    int minx = 0, int maxx = 0);
	inline void complete() { sync(); }	/* complete last render call */
	void redraw();
	inline void damage(int v) { damage_ = v; }
	inline int damage() const { return (damage_); }

	inline void voff(int v) { voff_ = v; }
	inline void hoff(int v) { hoff_ = v; }
	int bpp();
	/*
	 * Return the VideoWindow object associated with
	 * the tk window identified by name.  Name has the
	 * usual tk form, e.g., ".top.video.win0".
	 */
	static inline VideoWindow* lookup(const char* name) {
		return ((VideoWindow*)TclObject::lookup(name));
	}
    protected:
	void draw(int miny, int maxy, int minx, int maxx);
	void dim();
	void clear();

	static GC gc_;
	const VideoImage* vi_;
	int callback_pending_;
	int damage_;
	int voff_;
	int hoff_;
    private:
	static void display(ClientData);
	static void doclear(ClientData);
	static void dodim(ClientData);
};

class CaptureWindow : public BareWindow {
    public:
	CaptureWindow(const char* name, XVisualInfo*);
	virtual ~CaptureWindow();
	void capture(u_int8_t* frm);
	inline int basewidth() { return (base_width_); }
	inline int baseheight() { return (base_height_); }
	void setsize(int w, int h);
	inline void converter(RGB_Converter* v) { converter_ = v; }
    protected:
	void grab_image();
	GC gc_;
	int base_width_;
	int base_height_;
	StandardVideoImage* image_;
	RGB_Converter* converter_;
};

#endif
