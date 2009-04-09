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

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <invent.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <dmedia/vl.h>
#include <dmedia/cl.h>
#include <dmedia/cl_cosmo.h>

#include "grabber.h"
#include "crdef.h"
#include "module.h"
#include "device-input.h"
#include "vic_tcl.h"
#include "transmitter.h"

class CosmoGrabber : public Grabber {
 public:
	CosmoGrabber();
	virtual ~CosmoGrabber();
	virtual int command(int argc, const char*const* argv);
	virtual void fps(int);
	virtual void start();
	virtual void stop();
	virtual void timeout();
 protected:
	virtual void startup();
	virtual void shutdown();
	virtual int grab();
	int send(u_char* p, int cc);
	VLDev lookup_device(const char*) const;

	VLServer server_;
	VLPath path_;
	CL_BufferHdl cb_;
	VLNode src_;
	VLNode drn_;
	VLDev device_;
	CLcompressorHdl handle_;

	int q_;
	int inw_;
	int inh_;
	int decimate_;
	int current_decimate_;
};

class CosmoDevice : public InputDevice {
 public:
	CosmoDevice(const char* name);
	virtual int command(int argc, const char*const* argv);
	static int havedev();
};

static CosmoDevice cosmo_device("cosmo");

int CosmoDevice::havedev()
{
	setinvent();
	inventory_t* p;
	while ((p = getinvent()) != 0)
		if (p->inv_class == INV_COMPRESSION &&
		    p->inv_type == INV_COSMO)
			return (1);
	return (0);
}

CosmoDevice::CosmoDevice(const char* name) : InputDevice(name)
{
	if (havedev())
		attributes_ = "format { jpeg }";
	else
		attributes_ = "disabled";
}

int CosmoDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3 && strcmp(argv[1], "open") == 0) {
		if (strcmp(argv[2], "jpeg") == 0) {
			TclObject* o = new CosmoGrabber;
			tcl.result(o->name());
		}
		return (TCL_OK);
	}
	return (InputDevice::command(argc, argv));
}

int CosmoGrabber::send(u_char* p, int cc)
{
	/* strip off jfif header */
	int sawff = 0;
	u_char* ep;
	for (ep = p + cc; p < ep; ) {
		int c = *p++;
		if (c == 0xff) {
			sawff = 1;
			continue;
		}
		if (!sawff)
			continue;
		if  (c == 0xda) {
			/* found start-of-scan marker */
			if (p + 2 <= ep) {
				/* skip over SOS */
				u_int t = (p[0] << 8) | p[1];
				p += t;
				break;
			}
		}
		sawff = 0;
	}
	if (p < ep) {
		/*XXX get ts from CL */
		JpegFrame f(media_ts(), p, cc, q_, 0, inw_, inh_);
		target_->consume(&f);
		tx_->flush();
	}
	return (0);
}

void CosmoGrabber::timeout()
{
#ifdef notdef
	for (;;) {
		double delta = tick(grab());
		if (delta != 0.) {
			usched(delta / 2);
			return;
		}
	}
#endif
	grab();
	msched(10);
}

int CosmoGrabber::grab()
{
	int cc;

	CLimageInfo ii;
	int s;
	s = clGetNextImageInfo(handle_, &ii, sizeof(ii));
	if (s == CL_NEXT_NOT_AVAILABLE) {
		sginap(1);
		s = clGetNextImageInfo(handle_, &ii, sizeof(ii));
		if (s == CL_NEXT_NOT_AVAILABLE)
			return (s);
	}
#ifdef notdef
{
		sginap(1);
		s = clGetNextImageInfo(handle_, &ii, sizeof(ii));
	}
#endif
	if (s < 0) {
		/*XXX*/
		printf("clGetNextImageInfo failed\n");
		exit(1);
	}
#ifdef notdef
	CLimageInfo nii;
	while (clGetNextImageInfo(handle_, &nii, sizeof(nii)) == SUCCESS) {
		clUpdateTail(cb_, ii.size);
		ii = nii;
	}
#endif

	cc = ii.size;
	/*XXX*/

	u_char* bp = 0;
	int wrap;
	int n = clQueryValid(cb_, cc, (void**)&bp, &wrap);
	if (n <= 0) {
		/*XXX*/
		printf("clQueryValid failed %d\n", n);
		exit(1);
	}
#ifdef notdef
printf("n %d cc %d w %d cc-n %d ic %d\n", n, cc, wrap, cc - n, ii.imagecount);
#endif

	/*XXX throw away odd fields */
	if (ii.imagecount & 1) {
		clUpdateTail(cb_, cc);
		return (0);
	}

	/*XXX get ts from ii*/
	if (n < cc) {
		/* frame wrapped */
		/*XXX keep sp around*/
		u_char* sp = new u_char[cc];
		memcpy(sp, bp, n);
		clUpdateTail(cb_, n);
		(void)clQueryValid(cb_, cc - n, (void**)&bp, &wrap);
		memcpy(sp + n, bp, cc - n);
		s = send(sp, cc);
		delete sp;
		cc -= n;
	} else
		s = send(bp, cc);

	clUpdateTail(cb_, cc);

	return (s);
}

int CosmoGrabber::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "q") == 0) {
			/*
			 * XXX seems to not respond to 
			 * quality changes while running.
			 */
			shutdown();

			/* assume value is in range */
			q_ = atoi(argv[2]);
			int options[2];
			options[0] = CL_JPEG_QUALITY_FACTOR;
			options[1] = q_;
			(void)clSetParams(handle_, options, 2);

			if (running_)
				startup();

			return (TCL_OK);
		}
	}
	return (Grabber::command(argc, argv));
}

void CosmoGrabber::start()
{
	startup();
	Grabber::start();
}

void CosmoGrabber::stop()
{
	shutdown();
	Grabber::stop();
}

void CosmoGrabber::fps(int f)
{
	Grabber::fps(f);
	startup();
}

void CosmoGrabber::shutdown()
{
	if (server_) {
		if (path_) {
			/*XXX nuke cb_ and else?*/
#ifdef notdef
			if (transferBuf_) {
				vlEndTransfer(server_, path_);
				vlDeregisterBuffer(server_, path_, drn_,
						   transferBuf_);
				vlDestroyBuffer(server_, transferBuf_);
				transferBuf_ = 0;
			}
#endif
			vlDestroyPath(server_, path_);
			path_ = 0;
		}
		vlCloseVideo(server_);
		server_ = 0;
		if (handle_ != 0) {
			/*XXX*/
			clDestroyBuf(cb_);
			clCloseCompressor(handle_);
			handle_ = 0;
		}
	}
}

VLDev CosmoGrabber::lookup_device(const char* device) const
{
	VLDevList list;
	vlGetDeviceList(server_, &list);
	VLDevice* p = list.devices;
	for (int n = list.numDevices; --n >= 0; ++p) {
		if (strcmp(p->name, device) == 0)
			return (p->dev);
	}
	return ((VLDev)-1);
}
    
void CosmoGrabber::startup()
{
	if (server_ != 0 &&
	    decimate_ == current_decimate_)
		return;

	current_decimate_ = decimate_;
	shutdown();

	server_ = vlOpenVideo("");
	if (server_ == 0)
		goto failed;

	/*
	 * locate an express video device
	 */
	device_ = lookup_device("ev1");
	if (device_ < 0)
		goto failed;

	int port;
	port = VL_ANY;/*XXX*/
	src_ = vlGetNode(server_, VL_SRC, VL_VIDEO, port);
#define COSMO_PORT 2
	drn_ = vlGetNode(server_, VL_DRN, VL_VIDEO, COSMO_PORT);
	path_ = vlCreatePath(server_, device_, src_, drn_);
	if (path_ < 0) {
		vlPerror("vic: create path");
		goto failed;
	}
	if (vlSetupPaths(server_, &path_, 1, VL_SHARE, VL_SHARE) < 0) {
		vlPerror("vic: set up paths");
		goto failed;
	}
	
	VLControlValue cv;
	if (vlGetControl(server_, path_, src_, VL_SIZE, &cv) < 0) {
		vlPerror("vic: get image size");
		goto failed;
	}
	inw_ = cv.xyVal.x & ~7;
	inw_ >>= 1;
	/*XXX*/
	inh_ = ((cv.xyVal.y + 15) >> 1) & ~7;

	if (clOpenCompressor(CL_JPEG_COSMO, &handle_) < 0)
		/*XXX*/
		goto failed;

	int options[10];
	int* p;
	p = options;
	*p++ = CL_IMAGE_WIDTH;
	*p++ = inw_ << 1;
	/*
	 * decimate by two horizontally to give us a natural aspect ratio
	 * since we are capturing fields instead of frames.
	 */
	*p++ = CL_INTERNAL_IMAGE_WIDTH;
	*p++ = inw_;
	*p++ = CL_IMAGE_HEIGHT;
	*p++ = inh_;
	*p++ = CL_JPEG_QUALITY_FACTOR;
	q_ = 80;/*XXX*/
	*p++ = q_;
	*p++ = CL_ENABLE_IMAGEINFO;
	*p++ = 1;
	if (clSetParams(handle_, options, 10) < 0)
		goto failed;
	/*XXX*/
	cb_ = clCreateBuf(handle_, CL_DATA, 1, 100*1024, 0);

	if (vlBeginTransfer(server_, path_, 0, NULL) < 0) {
		vlPerror("vic: begin transfer");
		goto failed;
	}
	/*
	 * Start it up.
	 */
	clCompress(handle_, CL_CONTINUOUS_NONBLOCK, CL_EXTERNAL_DEVICE,
		   0, NULL);
	return;
 failed:
	shutdown();
	status_ = -1;
}

CosmoGrabber::CosmoGrabber()
{
	if (clOpenCompressor(CL_JPEG_COSMO, &handle_) < 0) {
		status_ = -1;
		return;
	}
	(void)clCloseCompressor(handle_);
	handle_ = 0;
	inw_ = 0;
	inh_ = 0;
	current_decimate_ = -1;
	Grabber::fps(2);
	decimate_ = 2;
	server_ = 0;
	path_ = 0;
}

CosmoGrabber::~CosmoGrabber()
{
	shutdown();
}
