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

/*
 * This module derived from:
 *
 *	Netvideo version 3.2
 *	Written by Ron Frederick <frederick@parc.xerox.com>
 *
 *	IndigoVideo frame grab routines for the Silicon Graphics IRIS Indigo
 *	with Entry graphics.
 *	Written by Andrew Cherenson <arc@sgi.com>
 *
 * Copyright (c) Xerox Corporation 1992. All rights reserved.
 *  
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or derivative
 * work. Xerox grants no other licenses expressed or implied. The Xerox trade
 * name should not be used in any advertising without its written permission.
 *  
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *  
 * These notices must be retained in any copies of any part of this software.
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "grabber.h"
#include "module.h"
#include "device-input.h"

#include <stdlib.h>
#include <string.h>
#include <svideo.h>
#include <math.h>

class SvideoGrabber : public Grabber {
 public:
	SvideoGrabber();
	virtual ~SvideoGrabber();
 protected:
	virtual int grab();
	SVhandle handle_;
};

class SvideoDevice : public InputDevice {
 public:
	SvideoDevice(const char*);
	virtual int command(int argc, const char*const* argv);
};

static SvideoDevice svideo_device("indigo");

SvideoDevice::SvideoDevice(const char* name) : InputDevice(name)
{
	SVhandle handle = svOpenVideo();
	if (handle == 0)
		attributes_ = "disabled";
	else {
		(void)svCloseVideo(handle);
#ifdef notyet
		attributes_ = "format { 422 420 cif } \
			size { small large cif } port { xxx }";
#else
		attributes_ = "format { 422 }";
#endif
	}
}

int SvideoDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			TclObject* o = 0;
			if (strcmp(argv[2], "422") == 0)
				o = new SvideoGrabber();
			if (o != 0)
				tcl.result(o->name());
			return (TCL_OK);
		}
	}
	return (InputDevice::command(argc, argv));
}

/*
 * The sampling rate specifies the number of frames seen for each
 * frame captured. The minimum value is 2.
 *
 * For the Indigo R3000, a value of 2 consumes too much CPU time, so use 3.
 * For the Indigo R4000, a value of 2 is fine.
 *
 * Sampling			Approx. CPU Usage
 *  rate	Frames/s	R3000	R4000
 *   2		 15		 >90%	 10%
 *   3		 10		  31	  8
 *   4		  7.5		  23	 <8
 *   5		  6		  18	 <8
 *  30		  1		   2	  1
 *				(quiescent scene at 128 kbps)
 */

#define DEFAULT_SAMPLING_RATE  3		/* 10 fps */

int SvideoGrabber::grab()
{
	void* captureData;
	while (1) {
		(void)svGetCaptureData(handle_, &captureData, NULL);
		if (captureData != NULL) {
			break;
		}
		sginap(1);	/* wait a tick */
	}
	/*
	 * Decimate the full-size image by 2 in each dimension.
	 */
	u_int* sp = (u_int*)captureData;
	u_char* yp = frame_;
	int size  = framesize_;
	u_char* up = yp + size;
	u_char* vp = up + (size >> 1);
	
	/* loop processes two pixels at a time */
	int n = size >> 1;
	while (--n >= 0) {
		u_int d1 = sp[0];
		u_int d2 = sp[1];
		u_int d3 = sp[2];
		u_int d4 = sp[3];
		yp[0] = d1 >> 4;
		yp[1] = d3 >> 4;
		u_char c = ((d1 << 4) & 0xc0)		/* u */
			| ((d2 << 2) & 0x30)
			| ((d3) & 0x0c)
			| ((d4 >> 2) & 0x03);
		*up++ = c;
		c = ((d1 << 6) & 0xc0)		/* v */
			| ((d2 << 4) & 0x30)
			| ((d3 << 2) & 0x0c)
			| ((d4) & 0x03);
		*vp++ = c;
		sp += 4;
		yp += 2;
	}
	(void)svUnlockCaptureData(handle_, captureData);

	/*XXX this is the slow way*/
	suppress(frame_);
	saveblks(frame_);

	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

SvideoGrabber::~SvideoGrabber()
{
	if (handle_ != 0)
		(void)svCloseVideo(handle_);
}

SvideoGrabber::SvideoGrabber()
{
	/* Open the video device */
	handle_ = svOpenVideo();
	if (handle_ == 0) {
		status_ = -1;
		return;
	}

	/*
	 * Set sizes based on broadcast standard.
	 */
	long params[2];
	params[0] = SV_BROADCAST;
	if (svGetParam(handle_, params, 2) < 0) {
		svPerror("vic: getparam");
		status_ = -1;
		return;
	}
	svCaptureInfo svci;
	if (params[1] == SV_PAL) {
		svci.width = SV_PAL_XMAX;
		svci.height = SV_PAL_YMAX;
	} else {
		svci.width = SV_NTSC_XMAX ;
		svci.height = SV_NTSC_YMAX;
	}
	/*XXX*/
	int framerate = 0;
	if (framerate <= 0 || framerate > 15)
		svci.samplingrate = DEFAULT_SAMPLING_RATE;
	else
		svci.samplingrate = (int) ceilf(30.0 / (float)framerate);

	svci.format = SV_YUV411_FRAMES;
	svci.size = 1;
	if (svInitContinuousCapture(handle_, &svci) < 0) {
		svPerror("vic: init capt");
		return;
	}
	set_size_422(svci.width / 2, svci.height / 2);
	allocref();
}
