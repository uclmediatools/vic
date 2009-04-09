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

#ifdef USE_DDRAW

#define FRONT_SENTINEL 0xdabbfeed
#define BACK_SENTINEL 0xbaddbeef

#define SETUP_SENTINELS() { \
    frontSentinel_ = FRONT_SENTINEL; \
backSentinel_ = BACK_SENTINEL; }

#define CHECK_SENTINELS(obj) {			    \
if (obj == 0) {					    \
    debug_msg("Null object in sentinel check\n");   \
}	\
    else {					    \
    if (obj->frontSentinel_ != FRONT_SENTINEL)			    \
	debug_msg("Front sentinel on object %x bad: %x != %x\n",    \
	    obj->frontSentinel_, FRONT_SENTINEL);		    \
    if (obj->backSentinel_ != BACK_SENTINEL)			   \
	debug_msg("Back sentinel on object %x bad: %x != %x\n",	\
	    obj->backSentinel_, BACK_SENTINEL)	;		\
} \
}

#pragma warning( disable : 4786)

#include <windows.h>
#include <winuser.h>
#include <ddraw.h>
#include <list>
#include <map>

class DDrawMonitor
{
public:
    int frontSentinel_;
    GUID *guid_;
    char guidString_[1024];
    char *driverDescription_;
    char *driverName_;
    int myIndex_;
    HMONITOR handle_;
    LPDIRECTDRAW7 directDraw_;
    LPDIRECTDRAWSURFACE7 primarySurface_;
    LPDIRECTDRAWCLIPPER clipper_;
    
    MONITORINFO monitorInfo_;

    double nPixelsDrawn_;

    int backSentinel_;
    
    DDrawMonitor(GUID *g, char *desc, char *name, HMONITOR mon)
    {
	SETUP_SENTINELS();
	init(g, desc, name, mon);
    }
    
    DDrawMonitor()
    {
	SETUP_SENTINELS();
	init(0, 0, 0, 0);
    }
    
    ~DDrawMonitor()
    {
	delete driverDescription_;
	delete driverName_;
    }
    
    void allocateDirectDraw(HWND);
    void createSurface(HWND);

    LPDIRECTDRAW7 getDirectDraw() { return directDraw_; }
    
    void print()
    {
	debug_msg("Monitor: index=%d guid=%s desc=%s name=%s handle=%x\n",
	    myIndex_, guidString_, driverDescription_, driverName_, handle_);
#if 1
	debug_msg("         disp origin=%d %d work origin=%d %d \n",
	    monitorInfo_.rcMonitor.left, monitorInfo_.rcMonitor.top,
	    monitorInfo_.rcWork.left, monitorInfo_.rcWork.top);
#endif
    }

    void updatePixelCount(int pixels, int depth);
    void updateRate();
    
private:
    void init(GUID *g, char *desc, char *name, HMONITOR mon);
       
};

class DDrawImageMonitor;

/*
 * A class for DirectDraw based video images. 
 */
class DDrawVideoImage : public StandardVideoImage {
    public:
	DDrawVideoImage(Tk_Window, int width, int height);
	DDrawVideoImage(Tk_Window, int width, int height, int bpp);
	virtual ~DDrawVideoImage();
	void checkRecalculate(Window window);
	void putimage(Display* dpy, Window window, GC gc,
		      int sx, int sy, int x, int y,
		      int w, int h) const;

	static void initDirectDraw();
	static void getTclMonitorList(char *);

	class LockedRegion;

	void lockSurface(int sx, int sy, int x, int y, int w, int h,
				    DDrawVideoImage::LockedRegion &lock);

	void forceRecalculate() { needRecalculate_ = true; }

	DDrawImageMonitor *currentMonitor();

	class WindowMotionCallback
	{
	public:
	    virtual void windowMoved() = 0;
	};

	void addWindowMotionCallback(WindowMotionCallback *c)
	{
	    windowMotionCallbacks_.push_back(c);
	}

	class LockedRegion
	{
	public:
	    LockedRegion();
	    ~LockedRegion();

	    void setLockParams(LPDIRECTDRAWSURFACE7 surface, RECT &rect);

	    LPDDSURFACEDESC2 surfaceDesc() {
		return &surfaceDesc_;
	    }
	    void *address();
	    int width();
	    int height();
	    long pitch();
	    int locked();

	    void release();
	private:
	    int locked_;
	    DDSURFACEDESC2 surfaceDesc_;
	    RECT lockedRect_;
	    LPDIRECTDRAWSURFACE7 surface_;
	};
    protected:
	void init(Tk_Window tk, int w, int h);

	static void initMonitors();
	static BOOL WINAPI findMonitorsCallback(									  
		GUID FAR *lpGUID,    
		LPSTR     lpDriverDescription, 
		  LPSTR     lpDriverName,        
		  LPVOID    lpContext,           
		  HMONITOR  hm);
	static BOOL CALLBACK enumProc(HMONITOR hMonitor, HDC hdcMon, LPRECT lprcMon,
			      LPARAM dwData);

	void doPutImage(LPDIRECTDRAWSURFACE7 surface,
		LPDIRECTDRAWSURFACE7 imageSurface,
		 LPDIRECTDRAWCLIPPER clipper,
		 HWND hWnd,
		 int sx, int sy, int x, int y,
		 int w, int h) const;

	std::list<WindowMotionCallback *> windowMotionCallbacks_;

	bool needRecalculate_;
	void calculateCoverage(HWND);
	/*
	 * Windows handle for the video window
	 */
	HWND window_;

	/* 
	 * Toplevel Tk window
	 */
	Tk_Window toplevelTkWindow_;
	Tk_Window tk_;
	 
	/*
	 * Application main window
	 */
	static HWND appMainWindow_;

	/*
	 * Static list of monitors that are available
	 */
	typedef std::list<DDrawMonitor *> MonitorList;
	static MonitorList monitors_;

	/* 
	 * List of ImageMonitor mapping objects for this window
	 */
	typedef std::list<DDrawImageMonitor *> ImageMonitorList;
	ImageMonitorList imageMonitors_;


	/*
	 * Tk event stuff to detect window moves
	 */
	static void windowConfigureEvent_s(ClientData data, XEvent *eventPtr);
	void windowConfigureEvent(XConfigureEvent *eventPtr);
	static void videoWindowConfigureEvent_s(ClientData data, XEvent *eventPtr);
	void videoWindowConfigureEvent(XMapEvent *eventPtr);

	friend class UpdateRatesCommand;
};
	
/*
 * A DDrawImageMonitor maps an image surface to a monitor.
 * The DDrawVideoImage keeps a list of these for displaying
 * its XImage onto each of the attached monitors.
 * An imagemonitor can be disabled when the video image is not
 * visible on that monitor.
 */

class DDrawImageMonitor
{
public:
    int frontSentinel_;
    DDrawImageMonitor(XImage *image, DDrawMonitor *mon);
    ~DDrawImageMonitor();
    void setEnabled(bool enabled) { enabled_ = enabled; }
    void putimage(HWND hwnd, int sx, int sy, int x, int y, int w, int h);

    void calculateCoverage(POINT);

    void configureSurface();
    void releaseSurface();

    void lockSurface(HWND hWnd, int sx, int sy, int x, int y, int w, int h,
				    DDrawVideoImage::LockedRegion &lock);

    bool enabled() { return enabled_; }
    DDrawMonitor *monitor() { return monitor_; }

    bool supportsOverlay(int &maxOverlays, int &visibleOverlays);
    LPDIRECTDRAWSURFACE7 getSurface() { return imageSurface_; }

    void getDisplayRect(HWND hWnd, int x, int y, int w, int h,
				    RECT &dstRect);
private:

    XImage *image_;
    LPDIRECTDRAWSURFACE7 imageSurface_;
    DDrawMonitor *monitor_;

    bool enabled_;
public:
    int backSentinel_;
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
