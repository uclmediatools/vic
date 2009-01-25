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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 * Derived from code written by Eric Anderson, UCSD Computer Systems Lab,
 * 4/92 and hacked on by Jon Kay, Vach Kompella, Kevin Fall.
 * Ported to vic 1/94, Steven McCanne (mccanne@ee.lbl.gov) March 1993.
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

/*
 * Apply this patch to the extensions/xv/lib/Xv.c file:
*** Xv.c~	Thu Mar 31 11:13:47 1994
--- Xv.c	Fri May 27 11:38:33 1994
***************
*** 192,197 ****
--- 192,198 ----
  	  return(XvBadAlloc);
  	}
        (void)strncpy(name, u.string, size);
+       name[size] = '\0';
        pa->name = name;
  
        u.buffer += (size + 3) & ~3;
***************
*** 351,356 ****
--- 352,358 ----
  	  return(XvBadAlloc);
  	}
        strncpy(name, u.string, size);
+       name[size] = '\0';
        pe->name = name;
        pe++;

 * It fixes a problem with strings NOT being null terminated.
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/ipc.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

extern "C" {
#ifdef _AIX
#include <Xvlib.h>
#else
#include <X11/extensions/Xvlib.h>
#endif
}

#include "grabber.h"
#include "crdef.h"
#include "module.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "rgb-converter.h"
#include "vw.h"

#define NTSC_WIDTH 640
#define NTSC_HEIGHT 480
#define PAL_WIDTH 768
#define PAL_HEIGHT 576
#define CIF_WIDTH 352
#define CIF_HEIGHT 288

class XvWindow : public CaptureWindow {
public:
	XvWindow(const char* name, XVisualInfo*, XvPortID);
	virtual ~XvWindow();
	virtual void setsize(int w, int h);
protected:
	XvPortID grabID_;
};

class XvGrabber : public Grabber {
 public:
	XvGrabber();
	virtual ~XvGrabber();
	virtual int command(int argc, const char*const* argv);
 protected:
	virtual void setsize();
	void setattr(Atom attr, int value);
	void setport(const char *port);
	int getattr(Atom attr);
	int getgrabber();
	virtual int grab();
	XvEncodingInfo *lookup_encoding_by_id(XvEncodingID id);    
	XvEncodingInfo *lookup_encoding_by_name(char *name);

	Visual* visual_;
	XvPortID grabID_;
	XvWindow* capwin_;
	
	char grabber_name[80];
	Atom XAbrightness, XAcontrast, XAhue, XAsaturation, XAencoding;
	u_int nencodings;
	XvEncodingInfo *encoding;

	int decimate_;
	XvEncodingID  encodingid_;
	XVisualInfo vinfo_;
};

class XvCIFGrabber : public XvGrabber {
protected:
	virtual void setsize();
};

class Xv420Grabber : public XvGrabber {
protected:
	virtual void setsize();
};

class XvDevice : public InputDevice {
 public:
	XvDevice(const char* s);
	int command(int argc, const char*const* argv);
};

static XvDevice xv_device("xv");

XvDevice::XvDevice(const char* s) : InputDevice(s)
{
	/*XXX port & 420*/
	attributes_ = "format { 422 420 } size { small large cif }";
}

int XvDevice::command(int argc, const char*const* argv)
{
#if 0
	printf("XvDevice::command: %d: ", argc);
	for (int i=0; i<argc; i++)
		printf("%s ", argv[i]);
	printf("\n");
#endif
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			if (strcmp(argv[2], "422") == 0) {
				TclObject* o = new XvGrabber;
				tcl.result(o->name());
			} else if (strcmp(argv[2], "cif") == 0) {
				TclObject* o = new XvCIFGrabber;
				tcl.result(o->name());
			}
			return (TCL_OK);
		}
	}
	return (InputDevice::command(argc, argv));
}

XvWindow::XvWindow(const char* name, XVisualInfo* vinfo, XvPortID grabID)
	: CaptureWindow(name, vinfo), grabID_(grabID)
{
	/*
	 * Xv doesn't have a way to query the signal type, and
	 * hence the input video geometry.
	 * XXX should make this configurable.
	 */
#define NTSC_WIDTH 640	
#define NTSC_HEIGHT 480
	base_width_ = NTSC_WIDTH;
	base_height_ = NTSC_HEIGHT;
}

XvWindow::~XvWindow()
{
	if (image_ != 0)
		XvStopVideo(dpy_, grabID_, Tk_WindowId(tk_));
}

void XvWindow::setsize(int w, int h)
{
	CaptureWindow::setsize(w, h);
	/* Start the video display */
	Tk_MakeWindowExist(tk_);
	XvPutVideo(dpy_, grabID_, Tk_WindowId(tk_), gc_,
		   0, 0, base_width_, base_height_,
		   0, 0, width_, height_);
}

XvGrabber::XvGrabber() : capwin_(0), decimate_(0), grabID_(0)
{
	Tcl& tcl = Tcl::instance();

	if (getgrabber() < 0) {
		status_ = -1;
		return;
	}
		fprintf(stderr, "set to grab port\n");
	Display* dpy = Tk_Display(Tcl::instance().tkmain());
	if (XvGrabPort(dpy, grabID_, CurrentTime) != Success) {
		fprintf(stderr, "Unable to grab port\n");
		tcl.result("Unable to reserve grabber.");
		status_ = -1;
		return;
	}
}

XvGrabber::~XvGrabber()
{
	if (status_ >= 0) {
		Display* dpy = Tk_Display(Tcl::instance().tkmain());
		XvUngrabPort(dpy, grabID_, CurrentTime);
	}
}

int XvGrabber::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "decimate") == 0) {
			int dec = atoi(argv[2]);
			Tcl& tcl = Tcl::instance();
			if (dec <= 0) {
				tcl.resultf("%s: divide by zero", argv[0]);
				return (TCL_ERROR);
			}
			if (dec != decimate_) {
				decimate_ = dec;
				setsize();
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "port") == 0) {
			setport(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "contrast") == 0) {
			setattr(XAcontrast, atoi(argv[2]));
			return (TCL_OK);
		} else if (strcmp(argv[1], "brightness") == 0) {
			setattr(XAbrightness, atoi(argv[2]));
			return (TCL_OK);
		} else if (strcmp(argv[1], "hue") == 0) {
			setattr(XAhue, atoi(argv[2]));
			return (TCL_OK);
		} else if (strcmp(argv[1], "saturation") == 0) {
			setattr(XAsaturation, atoi(argv[2]));
			return (TCL_OK);
		} else if (strcmp(argv[1], "create-capwin") == 0) {
			capwin_ = new XvWindow(argv[2], &vinfo_, grabID_);
			tcl.result(capwin_->name());
			return (TCL_OK);
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "need-capwin") == 0) {
			tcl.result("1");
			return (TCL_OK);
		}
	}
	return (Grabber::command(argc, argv));
}

XvEncodingInfo *XvGrabber::lookup_encoding_by_id(XvEncodingID id)
{
	int i;
	for (i=0; i<nencodings; i++) {
		if (id == encoding[i].encoding_id)
			return &encoding[i];
	}
	return NULL;
}

XvEncodingInfo *XvGrabber::lookup_encoding_by_name(char *name)
{
	for (int i=0; i<nencodings; i++) {
		if (strncmp(name, encoding[i].name, strlen(name)) == 0)
			return &encoding[i];
	}
	return NULL;
}

void XvGrabber::setport(const char *port)
{
	printf("setport: %s\n", port);
}

void XvGrabber::setattr(Atom attr, int value)
{
	Display* dpy = Tk_Display(Tcl::instance().tkmain());
	XvSetPortAttribute(dpy, grabID_, attr, value);
}

int XvGrabber::getattr(Atom attr)
{
	int v;
	Display* dpy = Tk_Display(Tcl::instance().tkmain());
	XvGetPortAttribute(dpy, grabID_, attr, &v);
	return(v);
}

void XvGrabber::setsize()
{
	int w = capwin_->basewidth() / decimate_;
	int h = capwin_->baseheight() / decimate_;
	capwin_->setsize(w, h);
	capwin_->converter(RGB_Converter_422::instance());
	set_size_422(capwin_->width(), capwin_->height());
	allocref();
}

void XvCIFGrabber::setsize()
{
	int w = 2 * 352 / decimate_;
	int h = 2 * 288 / decimate_;
	capwin_->setsize(w, h);
	capwin_->converter(RGB_Converter_420::instance());
	set_size_cif(w, h);
	allocref();
}

void Xv420Grabber::setsize()
{
	int w = capwin_->basewidth() / decimate_;
	int h = capwin_->baseheight() / decimate_;
	capwin_->setsize(w, h);
	capwin_->converter(RGB_Converter_420::instance());
	set_size_420(w, h);
	allocref();
}

int XvGrabber::grab()
{
	capwin_->capture(frame_);
	suppress(frame_);
	saveblks(frame_);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

int XvGrabber::getgrabber()
{
	Tk_Window tk = Tcl::instance().tkmain();
	Display* dpy = Tk_Display(tk);

	int majop, eventbase, errbase;
	if (XQueryExtension(dpy, "XVideo", &majop,
			    &eventbase, &errbase) == False)
		return (-1);

	Window root = DefaultRootWindow(dpy);
	u_int ngrabbers=0;
	XvAdaptorInfo* grabbers;
	if (XvQueryAdaptors(dpy, root, &ngrabbers, &grabbers) != Success)
	  return (-1);
	if (ngrabbers > 1)
	  fprintf(stderr, "XVgrabber: warning: more than one frame grabber\n");
		
        for (int i=2; i<ngrabbers; i++) {
          //if ((grabbers[i].type)&XvOutputMask) {
          if ((grabbers[i].type)&XvInputMask) {
	    fprintf(stderr, "Xv %d grabber: %s\n",i,grabbers[i].name);
	    strncpy(grabber_name, grabbers[i].name, sizeof(grabber_name));
	    grabID_ = grabbers[i].base_id;
          }
        }
        if (!grabID_) return (-1);

	XvFreeAdaptorInfo(grabbers);

	XvQueryEncodings(dpy, grabID_, &nencodings, &encoding);
#if 1
	printf("Encodings(%d): ", nencodings);
	for (int i=0; i<nencodings; i++)
	  printf("%s %d %d\n", encoding[i].name,
		 encoding[i].width, encoding[i].height);
	printf("\n");
#endif

	XAbrightness = XInternAtom(dpy, "XV_BRIGHTNESS", False);
	XAcontrast = XInternAtom(dpy, "XV_CONTRAST", False);
	XAhue = XInternAtom(dpy, "XV_HUE", False);
	XAsaturation = XInternAtom(dpy, "XV_SATURATION", False);
	XAencoding = XInternAtom(dpy, "XV_ENCODING", False);

	XvGetPortAttribute(dpy, grabID_, XAencoding, (int *)&encodingid_);

	if (!XMatchVisualInfo(dpy, Tk_ScreenNumber(tk), 24, TrueColor, &vinfo_))
		return (-1);

	return (0);
}

