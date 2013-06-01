/*
 * Copyright (c) 1993-1994 The Regents of the University of California.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef MAC_OSX_TK
/* stuff needed to include declaration of TkPutImage */
#include <math.h>
extern "C" {
#define HAVE_LIMITS_H 1
#ifndef WIN32
#define HAVE_UNISTD_H 1
#endif
#include <tkPort.h>
#undef strcasecmp
#define TIMEZONE_DEFINED_
}
#endif

#include "vw.h"
#include "color.h"
#include "rgb-converter.h"
#ifdef HAVE_XVIDEO
#include "xvideo.h"
#endif
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
#ifdef USE_DDRAW

//  #define DDRAW_USE_BLTFAST

#include <windows.h>
#include <ddraw.h>

#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION == 0)
#include <tkWin.h>
#else
#include <tkPlatDecls.h>
#endif

#include <dvp.h>

const char *ddrawErrorString(HRESULT rc);

static double totalPixelsDrawn = 0.0;

DDrawVideoImage::MonitorList DDrawVideoImage::monitors_;
HWND DDrawVideoImage::appMainWindow_;


#endif

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
#ifdef USE_DDRAW
	extern int use_ddraw;
	Tcl tcl = Tcl::instance();
	if (use_ddraw)
	{
		const char *minWS, *minHS;
		int minW, minH;

		if ((minWS = tcl.attr("ddraw_min_width")) != NULL)
			minW = atoi(minWS);
		else
			minW = 10;

		if ((minHS = tcl.attr("ddraw_min_height")) != NULL)
			minH = atoi(minHS);
		else
			minH = 10;


		if (w > minW && h > minH)
		{
			DDrawVideoImage *p = new DDrawVideoImage(tk, w, h);
			return p;
		}
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


#ifdef HAVE_XVIDEO
// FIXME: need to detect xvdieo capability
bool XVideoImage::enable_xv = true;

XVideoImage::XVideoImage(Tk_Window tk, int width, int height)
  : VideoImage(tk, width, height), image_(NULL)
{			     
	if(render.init(dpy_, FOURCC_I420, Tk_Visual(tk), Tk_Depth(tk), 3) >= 0){
	  image_ = render.createImage(width, height);
	  debug_msg("xvideo: initialie video with %dx%d\n", width, height); 
	}else{
	  debug_msg("cannot initialize xvideo extension");
	  enable_xv = false;
    }
}

XVideoImage* XVideoImage::allocate(Tk_Window tk, int width, int height){
	
	if(enable_xv){
	  XVideoImage* p = new XVideoImage(tk, width, height);
	  if(enable_xv){
	    debug_msg("using xvideo extension\n");
	    return (p);	
	  }else{
	    delete p;
	    return NULL;
	  }
	}
	return NULL;
}

void XVideoImage::putimage(Display* dpy, Window window, GC gc,
		      int sx, int sy, int x, int y,int w, int h) const{
	render.displayImage(window, gc, w, h);
}
#endif /*HAVE_XVIDEO*/

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

#if defined(MAC_OSX_TK) || ( defined(WIN32) && (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION != 0))
        TkPutImage(NULL, 0, dpy, window, gc, image_, sx, sy, x, y, w, h);
#else
	XPutImage(dpy, window, gc, image_, sx, sy, x, y, w, h);
#endif
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

#ifdef USE_DDRAW
DDrawVideoImage::DDrawVideoImage(Tk_Window tk, int w, int h)
	: StandardVideoImage(tk, w, h)
{
	init(tk, w, h);
}

DDrawVideoImage::DDrawVideoImage(Tk_Window tk, int w, int h, int bpp)
	: StandardVideoImage(tk, w, h)
{
	bpp_ = bpp;
	init(tk, w, h);
}



DDrawVideoImage::~DDrawVideoImage()
{
    Tk_DeleteEventHandler(toplevelTkWindow_, StructureNotifyMask, windowConfigureEvent_s, this);
    Tk_DeleteEventHandler(tk_, StructureNotifyMask, videoWindowConfigureEvent_s, this);

    ImageMonitorList::iterator it;
    for (it = imageMonitors_.begin(); it != imageMonitors_.end(); it++)
    {
	DDrawImageMonitor *im = *it;

	delete im;
    }

    delete image_->data;
    image_->data = 0;
}

void DDrawVideoImage::init(Tk_Window tk, int w, int h)
{
    int size = w * h;

    if (bpp_ > 8)
	size *= bpp_ / 8;
    image_->data = new char[size];

    window_ = 0;
    needRecalculate_ = true;
    
    /*
    * Find the toplevel window for this video window
    */
    
    for (toplevelTkWindow_ = tk; !Tk_IsTopLevel(toplevelTkWindow_); )
	toplevelTkWindow_ = Tk_Parent(toplevelTkWindow_);
//    debug_msg("got toplevel=%d\n", toplevelTkWindow_);
     
    /*
    * Register for configure events, so that we can recalculate
    * the monitor/surface drawing stuff.
    */
    tk_ = tk;
    Tk_CreateEventHandler(toplevelTkWindow_, StructureNotifyMask, windowConfigureEvent_s, this);
    Tk_CreateEventHandler(tk, StructureNotifyMask, videoWindowConfigureEvent_s, this);

    /*
     * Set up the ImageMonitor objects
     */

    MonitorList::iterator it;
    for (it = monitors_.begin(); it != monitors_.end(); it++)
    {
	DDrawMonitor *m = (*it);

	DDrawImageMonitor *im = new DDrawImageMonitor(image_, m);

	imageMonitors_.push_back(im);
    }
    
}

/*
 * Determine which monitors should display this image by 
 * passing the video window to each ImageMonitor object. If
 * the ImageMonitor should draw the window, it enables itself.
 */
void DDrawVideoImage::calculateCoverage(HWND hwnd)
{
    window_ = hwnd;
//    debug_msg("calculate coverage this=%x window_=%x\n", this, window_);
    ImageMonitorList::iterator it;

    for (it = imageMonitors_.begin(); it != imageMonitors_.end(); it++)
    {
	(*it)->setEnabled(false);
    }

    RECT win;
    POINT points[4];
    int i;

    GetClientRect(window_, &win);

    points[0].x = win.left;
    points[0].y = win.top;

    points[1].x = win.right;
    points[1].y = win.top;

    points[2].x = win.left;
    points[2].y = win.bottom;

    points[3].x = win.right;
    points[3].y = win.bottom;

    for (i = 0; i < 4; i++)
	ClientToScreen(window_, &(points[i]));

    for (it = imageMonitors_.begin(); it != imageMonitors_.end(); it++)
    {
	DDrawImageMonitor *m = *it;
	//debug_msg("About to calculate coverage for monitor %x\n", m);
	CHECK_SENTINELS(m)
	//debug_msg("Monitor name is %s\n", m->monitor()->driverDescription_);
	for (i = 0; i < 4; i++)
	{
	    CHECK_SENTINELS(m)
	    //debug_msg("calculate coverage for %d\n", i);
	    m->calculateCoverage(points[i]);
	    //debug_msg("done with %d\n", i);
	    CHECK_SENTINELS(m)
	}

	m->configureSurface();
    }

#if 0
    debug_msg("After calculate, enabled status is:\n");
    for (it = imageMonitors_.begin(); it != imageMonitors_.end(); it++)
    {
	DDrawImageMonitor *m = (*it);
	debug_msg("%s: %s\n", m->enabled() ? "ENABLED " : "DISABLED",
	    m->monitor()->driverDescription_);
   }
#endif
}

void DDrawVideoImage::windowConfigureEvent_s(ClientData data, XEvent *eventPtr)
{
    DDrawVideoImage *img = (DDrawVideoImage *) data;
    // debug_msg("got event type=%d\n", eventPtr->type);
    if (eventPtr->type == ConfigureNotify)
	img->windowConfigureEvent(&(eventPtr->xconfigure));
}

void DDrawVideoImage::windowConfigureEvent(XConfigureEvent *event)
{
//    debug_msg("Got window configure event for img pos=%d %d\n",
//	event->x, event->y);

    forceRecalculate();

    std::list<WindowMotionCallback *>::iterator it;
    for (it = windowMotionCallbacks_.begin(); it != windowMotionCallbacks_.end(); it++)
    {
	(*it)->windowMoved();
    }
}
void DDrawVideoImage::videoWindowConfigureEvent_s(ClientData data, XEvent *eventPtr)
{
    DDrawVideoImage *img = (DDrawVideoImage *) data;
    // debug_msg("got video window event type=%d\n", eventPtr->type);
    if (eventPtr->type == MapNotify)
	img->videoWindowConfigureEvent(&(eventPtr->xmap));
}

void DDrawVideoImage::videoWindowConfigureEvent(XMapEvent *event)
{
//    debug_msg("Got window map event window=%x \n", event->window);

    forceRecalculate();

    std::list<WindowMotionCallback *>::iterator it;
    for (it = windowMotionCallbacks_.begin(); it != windowMotionCallbacks_.end(); it++)
    {
	(*it)->windowMoved();
    }
	
}

//
// Find the first enabled monitor.
//
DDrawImageMonitor * DDrawVideoImage::currentMonitor()
{

    DDrawImageMonitor *mon = 0;
    ImageMonitorList::iterator it;
    for (it = imageMonitors_.begin(); it != imageMonitors_.end(); it++)
    {
	DDrawImageMonitor *m = (*it);
	if (m->enabled())
	{
	    mon = m;
	    break;
	}
    }

    return mon;
    
}

void DDrawVideoImage::lockSurface(int sx, int sy, int x, int y, int w, int h,
				    LockedRegion &lock)
{
    DDrawImageMonitor *mon = currentMonitor();

    if (mon == 0)
	return;

    mon->lockSurface(window_, sx, sy, x, y, w, h, lock);
}

void DDrawMonitor::init(GUID *g, char *desc, char *name, HMONITOR mon)
{
    static int nextIndex = 0;

    myIndex_ = nextIndex++;
    nPixelsDrawn_ = 0.0;

    if (g != 0)
    {
	guid_ = new GUID;
	memcpy(guid_, g, sizeof(GUID));
    }
    else
	guid_ = 0;
    if (desc != 0)
    {
	driverDescription_ = new char[strlen(desc) + 1];
	strcpy(driverDescription_, desc);
    }
    else
	driverDescription_ = "none";
    
    if (name != 0)
    {
	driverName_ = new char[strlen(name) + 1];
	strcpy(driverName_, name);
    }
    else
	driverName_ = "none";
    
    if (guid_ != 0)
    {
	WCHAR s[1024];
	int n = StringFromGUID2(*guid_, s, sizeof(s));
	if (n > 0)
	{
	    WideCharToMultiByte(CP_UTF8, 0, s, n, guidString_, sizeof(guidString_), 0, 0);
	}
	else
	    guidString_[0] = 0;
    }
    else
	guidString_[0] = 0;
    
    monitorInfo_.cbSize = sizeof(monitorInfo_);
    HRESULT rc = GetMonitorInfo(mon, &monitorInfo_);
    if (rc == 0)
	debug_msg("getmonitorinfo failed\n");
    
    handle_ = mon;
};

void DDrawMonitor::updatePixelCount(int pixels, int depth)
{
    nPixelsDrawn_ += (double) pixels;
}

void DDrawMonitor::updateRate()
{
    Tcl& tcl = Tcl::instance();
    tcl.evalf("set pixrate(%d) %f", myIndex_, nPixelsDrawn_ / 1000.0);
}

void DDrawMonitor::allocateDirectDraw(HWND hwnd)
{
	HRESULT ddrval;

//	debug_msg("ddraw create for monitor %s\n", driverName_);

	// ddrval = DirectDrawCreate(NULL, &lpDD, NULL);

	ddrval = DirectDrawCreateEx(guid_, (VOID**)&directDraw_, IID_IDirectDraw7, NULL);

//	debug_msg("dd create returns %d\n", ddrval);

	if (ddrval != DD_OK)
	{
		debug_msg("ddcreate failed\n");
		return;
	}

	ddrval = directDraw_->SetCooperativeLevel(hwnd, DDSCL_NORMAL);

	if (ddrval != DD_OK)
	{
		debug_msg("setcoop failed\n");
		return;
	}

	DDCAPS caps;
	ZeroMemory(&caps, sizeof(caps));
	caps.dwSize = sizeof(caps);
	ddrval = directDraw_->GetCaps(&caps, NULL);

	if (ddrval != DD_OK)
	{
		debug_msg("getcaps failed with %s\n", ddrawErrorString(ddrval));
		return;
	}

	debug_msg("got caps: max overlays=%d currently visible overlays=%d\n",
		caps.dwMaxVisibleOverlays, caps.dwCurrVisibleOverlays);
}

static HRESULT WINAPI enumPortsCallback(
  LPDDVIDEOPORTCAPS lpDDVideoPortCaps,  
  LPVOID lpContext                      
)
{
    debug_msg("Enum ports: \n");
    return DDENUMRET_OK;
}
 

void DDrawMonitor::createSurface(HWND hwnd)
{
    /* Now create the primary surface */
    
    HRESULT		        hRet;
    DDSURFACEDESC2      ddsd;
    
    // Create the primary surface
    ZeroMemory(&ddsd,sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS; // | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    ddsd.ddpfPixelFormat.dwFourCC = 0 ;
    hRet = directDraw_->CreateSurface(&ddsd, &primarySurface_, NULL);
    if (hRet != DD_OK)
    {
	debug_msg("primary surface creation failed\n");
	return;
    }

    // What's the pixel format here?

    DDPIXELFORMAT pfmt;

    ZeroMemory(&pfmt, sizeof(DDPIXELFORMAT));
    pfmt.dwSize = sizeof(pfmt);
    hRet = primarySurface_->GetPixelFormat(&pfmt);
    if (hRet == DD_OK)
    {
	debug_msg("Pixel format:\n");
    }
    else
	debug_msg("GetPixelFormat failed: %s\n", ddrawErrorString(hRet));

    //
    // Check out video ports
    //

    LPDDVIDEOPORTCONTAINER portContainer;
    hRet = directDraw_->QueryInterface(IID_IDDVideoPortContainer, (void **) &portContainer);
    if (hRet == S_OK)
    {
	debug_msg("Got port container!\n");
	portContainer->EnumVideoPorts(0, 0, 0, enumPortsCallback);
    }
    //
    // What color codes does this surface support?
    //
    DWORD codes[32];
    DWORD nCodes = 32;
    hRet = directDraw_->GetFourCCCodes(&nCodes, codes);
    if (hRet == DD_OK)
    {
	unsigned char *c;
	for (int i= 0; i < nCodes; i++)
	{
	    c = (unsigned char *) (&codes[i]);
	    debug_msg("Code %d: %x %c%c%c%c\n", i, codes[i], c[0], c[1], c[2], c[3]);
	}
    }

    
//    debug_msg("created surface\n");
    
#ifndef DDRAW_USE_BLTFAST
    // Create a clipper object since this is for a Windowed render
    
    hRet = directDraw_->CreateClipper(0, &clipper_, NULL);
    if (hRet != DD_OK)
    {
	debug_msg("CreateClipper FAILED");
	return;
	
    }
    
//   debug_msg("created clipper\n");
    
    primarySurface_->SetClipper(clipper_);
#endif  
}

BOOL CALLBACK DDrawVideoImage::enumProc(HMONITOR hMonitor, HDC hdcMon, 
					LPRECT lprcMon, LPARAM dwData)
{
    debug_msg("other enum proc\n");
    DDrawMonitor *m;
    
    m = new DDrawMonitor(0, "primary", "primary", hMonitor);
    m->print();
    m->allocateDirectDraw(appMainWindow_);
    m->createSurface(appMainWindow_);
    
    monitors_.push_back(m);
    return TRUE;
}

void DDrawVideoImage::initMonitors()
{
    HRESULT rc;
    
    rc = DirectDrawEnumerateEx(findMonitorsCallback, 0,
	DDENUM_ATTACHEDSECONDARYDEVICES);
    
    if (rc != DD_OK)
    {
	debug_msg("device enumeration failed");
    }

    if (monitors_.size() == 0)
    {
	debug_msg("single monitor");

	HDC hdc;
	RECT rclip;

	hdc = GetDC(NULL);
	rclip.bottom = 10;
	rclip.top = 0;
	rclip.left = 0;
	rclip.right = 10;
	EnumDisplayMonitors(hdc, &rclip, enumProc, 0);
	ReleaseDC(NULL, hdc);

#if 0
	DDrawMonitor *m;
	m = new DDrawMonitor();
	m->print();
	m->allocateDirectDraw(appMainWindow_);
	m->createSurface(appMainWindow_);
	monitors_.push_back(m);
#endif
    }
}


BOOL WINAPI DDrawVideoImage::findMonitorsCallback(GUID FAR *lpGUID,    
						  LPSTR     lpDriverDescription, 
						  LPSTR     lpDriverName,        
						  LPVOID    lpContext,           
						  HMONITOR  hm)
{
    debug_msg("monitor enumeration: %x %s %s %x\n",
	lpGUID, lpDriverDescription, lpDriverName, hm);
    if (lpGUID != 0)
    {
	DDrawMonitor *m;
	
	m = new DDrawMonitor(lpGUID, lpDriverDescription, lpDriverName, hm);
	m->print();
	m->allocateDirectDraw(appMainWindow_);
	m->createSurface(appMainWindow_);
	
	monitors_.push_back(m);
    }		
    return TRUE;
}

void DDrawVideoImage::initDirectDraw()
{
    Tcl& t = Tcl::instance();
    Tk_Window tkMain = t.tkmain();
    appMainWindow_ = Tk_GetHWND(Tk_WindowId(tkMain));
    
    initMonitors();
}

void DDrawVideoImage::getTclMonitorList(char *buf)
{
    MonitorList::iterator it;

    strcpy(buf, "");
    for (it = monitors_.begin(); it != monitors_.end(); it++)
    {
	char moninfo[60];
	DDrawMonitor *m = (*it);

	RECT *r = &m->monitorInfo_.rcWork;
	sprintf(moninfo, "{ %d %d %d %d %d %d } ", 
	    r->left, r->top, r->right, r->bottom, 
	   m->myIndex_, m->myIndex_);
	strcat(buf, moninfo);
    }
}

static HRESULT bindImageToSurface(XImage *image, LPDIRECTDRAWSURFACE7 *lpDDS,
								  LPDIRECTDRAW7 lpDD)
{ 
    HRESULT hr;
    LPVOID  lpSurface  = NULL;
    HLOCAL  hMemHandle = NULL;
    DDSURFACEDESC2 ddsd2;
 
 
    // Initialize the surface description.
    ZeroMemory(&ddsd2, sizeof(DDSURFACEDESC2));
    ZeroMemory(&ddsd2.ddpfPixelFormat, sizeof(DDPIXELFORMAT));
    ddsd2.dwSize = sizeof(ddsd2);
    ddsd2.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_LPSURFACE |
                    DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd2.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN |
                           DDSCAPS_SYSTEMMEMORY;
    ddsd2.dwWidth = image->width;
    ddsd2.dwHeight= image->height;
    ddsd2.lPitch  = image->bytes_per_line;
    ddsd2.lpSurface = image->data;
 
    // Set up the pixel format for 24-bit RGB (8-8-8).
    ddsd2.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    ddsd2.ddpfPixelFormat.dwFlags= DDPF_RGB;
    ddsd2.ddpfPixelFormat.dwRGBBitCount = (DWORD) image->depth;

    if (image->depth == 16)
    {
#if 0
    	ddsd2.ddpfPixelFormat.dwRBitMask = 0x7c00;
	ddsd2.ddpfPixelFormat.dwGBitMask = 0x03e0;
	ddsd2.ddpfPixelFormat.dwBBitMask = 0x001f; 
#else
    	ddsd2.ddpfPixelFormat.dwRBitMask = 0xf800;
	ddsd2.ddpfPixelFormat.dwGBitMask = 0x07e0;
	ddsd2.ddpfPixelFormat.dwBBitMask = 0x001f; 
#endif
    }
    else
    {
	ddsd2.ddpfPixelFormat.dwRBitMask    = 0x00FF0000;
	ddsd2.ddpfPixelFormat.dwGBitMask    = 0x0000FF00;
	ddsd2.ddpfPixelFormat.dwBBitMask    = 0x000000FF;
    }
 
    // Create the surface
    hr = lpDD->CreateSurface(&ddsd2, lpDDS, NULL);
    return hr;
}



DDrawImageMonitor::DDrawImageMonitor(XImage *image, DDrawMonitor *mon) :
image_(image),
monitor_(mon),
enabled_(1)
{
    SETUP_SENTINELS()

    imageSurface_ = 0;
    /*
     * Delay this until configureSurface() is called.
     *
    rc = bindImageToSurface(image_, &imageSurface_, monitor_->directDraw_);
    if (rc != DD_OK)
    {
	debug_msg("monitor image bind failed with %s\n", ddrawErrorString(rc));
	return;
    }
    */
//    debug_msg("bound monitor ximage to surface\n");
}

DDrawImageMonitor::~DDrawImageMonitor()
{
//    debug_msg("delete DDRawImageMonitor %x\n", this);
    CHECK_SENTINELS(this);

    
    if (imageSurface_ != 0)
    {
	releaseSurface();
    }
}

static DWORD exceptionFilter(LPEXCEPTION_POINTERS p)
{
//    LPEXCEPTION_POINTERS p;
    
    debug_msg("Exception on surface release!\n");
//    p = GetExceptionInformation();
    
    LPEXCEPTION_RECORD e = p->ExceptionRecord;
    debug_msg("Code=%d\n", e->ExceptionCode);
    if (e->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
	debug_msg("access type = %s, address=%x\n",
	    e->ExceptionInformation[0] == 0 ? "read" : "write",
	    e->ExceptionInformation[1]);
    }
    
    return EXCEPTION_EXECUTE_HANDLER;
}

void DDrawImageMonitor::releaseSurface()
{
#ifdef _MSC_VER
    __try {
#endif
	ULONG oldRefCount = imageSurface_->Release();
//	debug_msg("oldRefCount=%d\n", oldRefCount);
#ifdef _MSC_VER
    } 
    __except(exceptionFilter(GetExceptionInformation())) {
    }
#endif
    imageSurface_ = 0;
}


/*
 * Determine if this monitor should be enabled by testing
 * if the given point is in the monitor's coverage.
 */
void DDrawImageMonitor::calculateCoverage(POINT pt)
{
    if (PtInRect(&monitor_->monitorInfo_.rcWork, pt))
	enabled_ = true;
}

/*
 * If this monitor is enabled, create the surface if one doesn't
 * yet exist.
 *
 * If this monitor is not enabled, release the surface if one
 * already exists.
 */

void DDrawImageMonitor::configureSurface()
{
    if (enabled_)
    {
	if (imageSurface_ == 0)
	{
	    int rc;
	    rc = bindImageToSurface(image_, &imageSurface_, monitor_->directDraw_);
	    if (rc != DD_OK)
	    {
		debug_msg("monitor image bind failed with %s\n", ddrawErrorString(rc));
		return;
	    }
	}
    }
    else
    {
	if (imageSurface_ != 0)
	{
    
	    releaseSurface();
	}
    }
 }

void DDrawImageMonitor::putimage(HWND hWnd, int sx, int sy, int x, int y, int w, int h)
{
    if (!enabled_)
	return;
    
    int dx, dy;
    
    // Adjust for display in non-primary monitor
    
    dx = x - monitor_->monitorInfo_.rcMonitor.left;
    dy = y - monitor_->monitorInfo_.rcMonitor.top;
    
//    debug_msg("putimage mon %s image %x\n", 
//	monitor_->driverDescription_, image_);
    
#ifdef USE_PROFILING
    startCounter(STATE_PUTIMAGE_DDRAW);
#endif
    RECT srcRect, dstRect;
    
#ifndef DDRAW_USE_BLTFAST
    // Associate the clipper with the window
    
    monitor_->clipper_->SetHWnd(0, hWnd);
#endif		

    GetClientRect(hWnd, &dstRect);
        
    ClientToScreen(hWnd, (POINT*)&dstRect.left);
    ClientToScreen(hWnd, (POINT*)&dstRect.right);
    
    dstRect.left += dx;
    dstRect.right = dstRect.left + w;
    dstRect.top += dy;
    dstRect.bottom = dstRect.top + h;
    
    SetRect(&srcRect, sx, sy, sx + w, sy + h);
#ifdef USE_PROFILING
    startCounter(STATE_BITBLT);
#endif
#ifdef DDRAW_USE_BLTFAST
    HRESULT res = monitor_->primarySurface_->BltFast(dstRect.left, dstRect.top,
	imageSurface_, 
	&srcRect, DDBLTFAST_NOCOLORKEY );
#else
    HRESULT res = monitor_->primarySurface_->Blt(&dstRect, imageSurface_, 
	&srcRect, DDBLT_WAIT, 0);
#endif
#ifdef USE_PROFILING
    LARGE_INTEGER elap;
    stopCounter(STATE_BITBLT, &elap);
    addBlitEntry(w,h,&elap);
#endif
    if (res != DD_OK)
    {
	debug_msg("blit failed with %s\n", ddrawErrorString(res));
	return;
    }
    monitor_->updatePixelCount(w * h, image_->depth);

#ifdef USE_PROFILING
    stopCounter(STATE_PUTIMAGE_DDRAW);
#endif
}

bool DDrawImageMonitor::supportsOverlay(int &maxOverlays, int &visibleOverlays)
{
    DDCAPS caps;
    HRESULT rc;

    maxOverlays = visibleOverlays = -1;

    ZeroMemory(&caps, sizeof(caps));
    caps.dwSize = sizeof(caps);

    rc = monitor()->directDraw_->GetCaps(&caps, 0);

    if (FAILED(rc))
	return false;

    if (caps.dwCaps & DDCAPS_OVERLAY)
    {
	maxOverlays = caps.dwMaxVisibleOverlays;
	visibleOverlays = caps.dwCurrVisibleOverlays;
	return true;
    }
    else
	return false;
}

void DDrawImageMonitor::getDisplayRect(HWND hWnd, int x, int y, int w, int h,
				    RECT &dstRect)
{
    if (!enabled_)
	return;
    
    int dx, dy;
    
    // Adjust for display in non-primary monitor
    
    dx = x - monitor_->monitorInfo_.rcMonitor.left;
    dy = y - monitor_->monitorInfo_.rcMonitor.top;
    
    GetClientRect(hWnd, &dstRect);
        
    ClientToScreen(hWnd, (POINT*)&dstRect.left);
    ClientToScreen(hWnd, (POINT*)&dstRect.right);
    
    dstRect.left += dx;
    dstRect.right = dstRect.left + w;
    dstRect.top += dy;
    dstRect.bottom = dstRect.top + h;
    
}
void DDrawImageMonitor::lockSurface(HWND hWnd, int sx, int sy, int x, int y, int w, int h,
				    DDrawVideoImage::LockedRegion &lock)
{
    if (!enabled_)
	return;
    
    int dx, dy;
    
    // Adjust for display in non-primary monitor
    
    dx = x - monitor_->monitorInfo_.rcMonitor.left;
    dy = y - monitor_->monitorInfo_.rcMonitor.top;
    
//    debug_msg("putimage mon %s image %x\n", 
//	monitor_->driverDescription_, image_);
    
#ifdef USE_PROFILING
    startCounter(STATE_PUTIMAGE_DDRAW);
#endif
    RECT srcRect, dstRect;
    
    GetClientRect(hWnd, &dstRect);
        
    ClientToScreen(hWnd, (POINT*)&dstRect.left);
    ClientToScreen(hWnd, (POINT*)&dstRect.right);
    
    dstRect.left += dx;
    dstRect.right = dstRect.left + w;
    dstRect.top += dy;
    dstRect.bottom = dstRect.top + h;
    
    SetRect(&srcRect, sx, sy, sx + w, sy + h);

    HRESULT res = monitor_->primarySurface_->Lock(&dstRect, lock.surfaceDesc(), 
	DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, 0);
    
    if (res != DD_OK)
    {
	debug_msg("lock failed with %s\n", ddrawErrorString(res));
	return;
    }

    lock.setLockParams(monitor_->primarySurface_, dstRect);
//    debug_msg("Lock succeeded! size=%d %d pitch=%d ptr=%x\n",
//	surfp->dwWidth, surfp->dwHeight, surfp->lPitch, surfp->lpSurface);
//    ZeroMemory(surfp->lpSurface, 8192);

//    monitor_->primarySurface_->Unlock(&dstRect);
}

DDrawVideoImage::LockedRegion::LockedRegion() :
    locked_(0), surface_(0)
{
    ZeroMemory(&surfaceDesc_, sizeof(surfaceDesc_));
    surfaceDesc_.dwSize = sizeof(surfaceDesc_);
}

DDrawVideoImage::LockedRegion::~LockedRegion()
{
    release();
}

void  DDrawVideoImage::LockedRegion::release()
{
    if (!locked_)
	return;
    surface_->Unlock(&lockedRect_);
    locked_ = 0;
}

void DDrawVideoImage::LockedRegion::setLockParams(LPDIRECTDRAWSURFACE7 surface, RECT &rect)
{
    surface_ = surface;
    lockedRect_ = rect;
    locked_ = 1;
}

int DDrawVideoImage::LockedRegion::width()
{
    return surfaceDesc_.dwWidth;
}

int DDrawVideoImage::LockedRegion::height()
{
    return surfaceDesc_.dwWidth;
}

int DDrawVideoImage::LockedRegion::locked()
{
    return locked_;
}

long DDrawVideoImage::LockedRegion::pitch()
{
    return surfaceDesc_.lPitch;
}

void * DDrawVideoImage::LockedRegion::address()
{
    return surfaceDesc_.lpSurface;
}

/*
     XPutImage(display, d, gc, image, src_x, src_y, dest_x,
     dest_y, width, height)
             Display *display;
             Drawable d;
             GC gc;
             XImage *image;
             int src_x, src_y;
             int dest_x, dest_y;
             unsigned int width, height;
*/

void DDrawVideoImage::doPutImage(LPDIRECTDRAWSURFACE7 surface,
				 LPDIRECTDRAWSURFACE7 imageSurface,
				 LPDIRECTDRAWCLIPPER clipper,
				 HWND hWnd,
				 int sx, int sy, int x, int y,
				 int w, int h) const
{
    
}		

void DDrawVideoImage::checkRecalculate(Window window) 
{
    HWND hWnd = Tk_GetHWND(window);

    if (needRecalculate_)
    {
	calculateCoverage(hWnd);
	needRecalculate_ = false;
    }
}

void DDrawVideoImage::putimage(Display* dpy, Window window, GC gc,
			       int sx, int sy, int x, int y,
			       int w, int h) const
{
    HWND hWnd = Tk_GetHWND(window);


    ((DDrawVideoImage *) this)->checkRecalculate(window);
    totalPixelsDrawn += (double) w * h;

    ImageMonitorList::const_iterator it;
    for (it = imageMonitors_.begin(); it != imageMonitors_.end(); it++)
    {
	DDrawImageMonitor *m = *it;

	m->putimage(hWnd, sx, sy, x, y, w, h);
    }
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
#ifdef HAVE_SWSCALE
		return (TCL_OK);
#else
		if (strcmp(argv[1], "voff") == 0) {
			voff_ = atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "hoff") == 0) {
			hoff_ = atoi(argv[2]);
			return (TCL_OK);
		}
#endif
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
// for fullscreen rendering, video frame is larger than render window size
//	else if (h > vi_->height())
//		h = vi_->height();
	else if (h < 0)
		return;
	int w = x1 - x0;
	if (w == 0)
		w = vi_->width();
//	else if (w > vi_->width())
//		w = vi_->width();
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
#if !defined(MAC_OSX_TK) && !defined(WIN32)
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
#endif
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

#ifdef USE_DDRAW

const char *ddrawErrorString(HRESULT rc)
{
	const char *mesg;
	switch (rc)
	{
    case DD_OK:
        mesg = "DD_OK";
        break;
    case DDERR_ALREADYINITIALIZED:
        return( "DDERR_ALREADYINITIALIZED");
        break;
    case DDERR_BLTFASTCANTCLIP:
        mesg = "DDERR_BLTFASTCANTCLIP";
        break;
    case DDERR_CANNOTATTACHSURFACE:
        mesg = "DDERR_CANNOTATTACHSURFACE";
        break;
    case DDERR_CANNOTDETACHSURFACE:
        mesg = "DDERR_CANNOTDETACHSURFACE";
        break;
    case DDERR_CANTCREATEDC:
        mesg = "DDERR_CANTCREATEDC";
        break;
    case DDERR_CANTDUPLICATE:
        mesg = "DDERR_CANTDUPLICATE";
        break;
    case DDERR_CANTLOCKSURFACE:
        mesg = "DDERR_CANTLOCKSURFACE";
        break;
    case DDERR_CANTPAGELOCK:
        mesg = "DDERR_CANTPAGELOCK";
        break;
    case DDERR_CANTPAGEUNLOCK:
        mesg = "DDERR_CANTPAGEUNLOCK";
        break;
    case DDERR_CLIPPERISUSINGHWND:
        mesg = "DDERR_CLIPPERISUSINGHWND";
        break;
    case DDERR_COLORKEYNOTSET:
        mesg = "DDERR_COLORKEYNOTSET";
        break;
    case DDERR_CURRENTLYNOTAVAIL:
        mesg = "DDERR_CURRENTLYNOTAVAIL";
        break;
    case DDERR_DCALREADYCREATED:
        mesg = "DDERR_DCALREADYCREATED";
        break;
    case DDERR_DEVICEDOESNTOWNSURFACE:
        mesg = "DDERR_DEVICEDOESNTOWNSURFACE";
        break;
    case DDERR_DIRECTDRAWALREADYCREATED:
        mesg = "DDERR_DIRECTDRAWALREADYCREATED";
        break;
    case DDERR_EXCEPTION:
        mesg = "DDERR_EXCEPTION";
        break;
    case DDERR_EXCLUSIVEMODEALREADYSET:
        mesg = "DDERR_EXCLUSIVEMODEALREADYSET";
        break;
    case DDERR_EXPIRED:
        mesg = "DDERR_EXPIRED";
        break;
    case DDERR_GENERIC:
        mesg = "DDERR_GENERIC";
        break;
    case DDERR_HEIGHTALIGN:
        mesg = "DDERR_HEIGHTALIGN";
        break;
    case DDERR_HWNDALREADYSET:
        mesg = "DDERR_HWNDALREADYSET";
        break;
    case DDERR_HWNDSUBCLASSED:
        mesg = "DDERR_HWNDSUBCLASSED";
        break;
    case DDERR_IMPLICITLYCREATED:
        mesg = "DDERR_IMPLICITLYCREATED";
        break;
    case DDERR_INCOMPATIBLEPRIMARY:
        mesg = "DDERR_INCOMPATIBLEPRIMARY";
        break;
    case DDERR_INVALIDCAPS:
        mesg = "DDERR_INVALIDCAPS";
        break;
    case DDERR_INVALIDCLIPLIST:
        mesg = "DDERR_INVALIDCLIPLIST";
        break;
    case DDERR_INVALIDDIRECTDRAWGUID:
        mesg = "DDERR_INVALIDDIRECTDRAWGUID";
        break;
    case DDERR_INVALIDMODE:
        mesg = "DDERR_INVALIDMODE";
        break;
    case DDERR_INVALIDOBJECT:
        mesg = "DDERR_INVALIDOBJECT";
        break;
    case DDERR_INVALIDPARAMS:
        mesg = "DDERR_INVALIDPARAMS";
        break;
    case DDERR_INVALIDPIXELFORMAT:
        mesg = "DDERR_INVALIDPIXELFORMAT";
        break;
    case DDERR_INVALIDPOSITION:
        mesg = "DDERR_INVALIDPOSITION";
        break;
    case DDERR_INVALIDRECT:
        mesg = "DDERR_INVALIDRECT";
        break;
    case DDERR_INVALIDSTREAM:
        mesg = "DDERR_INVALIDSTREAM";
        break;
    case DDERR_INVALIDSURFACETYPE:
        mesg = "DDERR_INVALIDSURFACETYPE";
        break;
    case DDERR_LOCKEDSURFACES:
        mesg = "DDERR_LOCKEDSURFACES";
        break;
    case DDERR_MOREDATA:
        mesg = "DDERR_MOREDATA";
        break;
    case DDERR_NO3D:
        mesg = "DDERR_NO3D";
        break;
    case DDERR_NOALPHAHW:
        mesg = "DDERR_NOALPHAHW";
        break;
    case DDERR_NOBLTHW:
        mesg = "DDERR_NOBLTHW";
        break;
    case DDERR_NOCLIPLIST:
        mesg = "DDERR_NOCLIPLIST";
        break;
    case DDERR_NOCLIPPERATTACHED:
        mesg = "DDERR_NOCLIPPERATTACHED";
        break;
    case DDERR_NOCOLORCONVHW:
        mesg = "DDERR_NOCOLORCONVHW";
        break;
    case DDERR_NOCOLORKEY:
        mesg = "DDERR_NOCOLORKEY";
        break;
    case DDERR_NOCOLORKEYHW:
        mesg = "DDERR_NOCOLORKEYHW";
        break;
    case DDERR_NOCOOPERATIVELEVELSET:
        mesg = "DDERR_NOCOOPERATIVELEVELSET";
        break;
    case DDERR_NODC:
        mesg = "DDERR_NODC";
        break;
    case DDERR_NODDROPSHW:
        mesg = "DDERR_NODDROPSHW";
        break;
    case DDERR_NODIRECTDRAWHW:
        mesg = "DDERR_NODIRECTDRAWHW";
        break;
    case DDERR_NODIRECTDRAWSUPPORT:
        mesg = "DDERR_NODIRECTDRAWSUPPORT";
        break;
    case DDERR_NOEMULATION:
        mesg = "DDERR_NOEMULATION";
        break;
    case DDERR_NOEXCLUSIVEMODE:
        mesg = "DDERR_NOEXCLUSIVEMODE";
        break;
    case DDERR_NOFLIPHW:
        mesg = "DDERR_NOFLIPHW";
        break;
    case DDERR_NOFOCUSWINDOW:
        mesg = "DDERR_NOFOCUSWINDOW";
        break;
    case DDERR_NOGDI:
        mesg = "DDERR_NOGDI";
        break;
    case DDERR_NOHWND:
        mesg = "DDERR_NOHWND";
        break;
    case DDERR_NOMIPMAPHW:
        mesg = "DDERR_NOMIPMAPHW";
        break;
    case DDERR_NOMIRRORHW:
        mesg = "DDERR_NOMIRRORHW";
        break;
    case DDERR_NONONLOCALVIDMEM:
        mesg = "DDERR_NONONLOCALVIDMEM";
        break;
    case DDERR_NOOPTIMIZEHW:
        mesg = "DDERR_NOOPTIMIZEHW";
        break;
    case DDERR_NOOVERLAYHW:
        mesg = "DDERR_NOOVERLAYHW";
        break;
    case DDERR_NOPALETTEATTACHED:
        mesg = "DDERR_NOPALETTEATTACHED";
        break;
    case DDERR_NOPALETTEHW:
        mesg = "DDERR_NOPALETTEHW";
        break;
    case DDERR_NORASTEROPHW:
        mesg = "DDERR_NORASTEROPHW";
        break;
    case DDERR_NOROTATIONHW:
        mesg = "DDERR_NOROTATIONHW";
        break;
    case DDERR_NOSTEREOHARDWARE:
        mesg = "DDERR_NOSTEREOHARDWARE";
        break;
    case DDERR_NOSTRETCHHW:
        mesg = "DDERR_NOSTRETCHHW";
        break;
    case DDERR_NOSURFACELEFT:
        mesg = "DDERR_NOSURFACELEFT";
        break;
    case DDERR_NOT4BITCOLOR:
        mesg = "DDERR_NOT4BITCOLOR";
        break;
    case DDERR_NOT4BITCOLORINDEX:
        mesg = "DDERR_NOT4BITCOLORINDEX";
        break;
    case DDERR_NOT8BITCOLOR:
        mesg = "DDERR_NOT8BITCOLOR";
        break;
    case DDERR_NOTAOVERLAYSURFACE:
        mesg = "DDERR_NOTAOVERLAYSURFACE";
        break;
    case DDERR_NOTEXTUREHW:
        mesg = "DDERR_NOTEXTUREHW";
        break;
    case DDERR_NOTFLIPPABLE:
        mesg = "DDERR_NOTFLIPPABLE";
        break;
    case DDERR_NOTFOUND:
        mesg = "DDERR_NOTFOUND";
        break;
    case DDERR_NOTINITIALIZED:
        mesg = "DDERR_NOTINITIALIZED";
        break;
	default:
		mesg = "??";
	}
	return mesg;
}
#endif

static class MonitorCommand : public TclObject {
public:
	MonitorCommand(const char* name) : TclObject(name) { }
protected:
	int command(int argc, const char*const* argv);
} get_monitors_command("get_monitors");

/*
 * get_monitors
 */
int MonitorCommand::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc != 1) {
		tcl.result("get_monitors arg mismatch");
		return (TCL_ERROR);
	}

#ifdef USE_DDRAW

	char * buf = new char[4096];
	// char buf[1024];
	DDrawVideoImage::getTclMonitorList(buf);
	tcl.result(buf);
	
#else

	tcl.result("");
#endif

	return (TCL_OK);
}

static class UpdateRatesCommand : public TclObject {
public:
	UpdateRatesCommand(const char* name) : TclObject(name) { }
protected:
	int command(int argc, const char*const* argv);
} update_rates_command("update_pixrates_vars");

/*
 * get_monitors
 */
int UpdateRatesCommand::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc != 1) {
		tcl.result("update_pixrates arg mismatch");
		return (TCL_ERROR);
	}

	debug_msg("Here in update rates...\n");
#ifdef USE_DDRAW
    DDrawVideoImage::MonitorList::iterator it;

    for (it = DDrawVideoImage::monitors_.begin(); it != DDrawVideoImage::monitors_.end(); it++)
    {
	DDrawMonitor *im = *it;

	im->updateRate();
    }

    debug_msg("total %d\n", totalPixelsDrawn);
    tcl.evalf("set pixrate(total) %f", totalPixelsDrawn / 1000.0);


#else

#endif

	return (TCL_OK);
}
