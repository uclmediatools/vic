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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>

extern "C" {
#include <vigrapix.h>
}

#include "grabber.h"
#include "module.h"
#include "vic_tcl.h"
#include "device-input.h"

class VigraPixGrabber : public Grabber {
 public:
	VigraPixGrabber();
	virtual ~VigraPixGrabber();
	virtual void start();
 protected:
	virtual int command(int argc, const char*const* argv);
	virtual int capture();
	virtual int grab();
	virtual void setsize();

	vigrapix_t handle_;
	int format_;		/* video format: NTSC or PAL */
	int port_;		/* VigraPix input port */
	u_int decimate_;
};

class CIFVigraPixGrabber : public VigraPixGrabber {
 protected:
	virtual void setsize();
	virtual int capture();
};

class VigraPixDevice : public InputDevice {
 public:
	VigraPixDevice(const char*);
	virtual int command(int argc, const char*const* argv);
};

static VigraPixDevice vpix_device("vigrapix");

VigraPixDevice::VigraPixDevice(const char* name) : InputDevice(name)
{
	if (access("/dev/vigrapix0", R_OK) == 0)
		attributes_ = "\
format { 420 422 } \
size { small large cif } \
port { Composite S-Video-1 S-Video-2}";
	else
		attributes_ = "disabled";
}

int VigraPixDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			TclObject* o = 0;
			if (strcmp(argv[2], "422") == 0)
				o = new VigraPixGrabber;
			else if (strcmp(argv[2], "cif") == 0)
				o = new CIFVigraPixGrabber;
			if (o != 0)
				tcl.result(o->name());
			return (TCL_OK);
		}
	}
	return (InputDevice::command(argc, argv));
}

VigraPixGrabber::VigraPixGrabber()
{
	/*XXX device file*/
	handle_ = vigrapix_open("/dev/vigrapix0");
	if (handle_ == 0) {
		status_ = -1;
		return;
	}
	port_ = VIGRAPIX_COMP;
	format_ = VIGRAPIX_NOLOCK;
	decimate_ = 2;
}

VigraPixGrabber::~VigraPixGrabber()
{
	if (handle_ != 0) {
		vigrapix_close(handle_);
		handle_ = 0;
	}
}

void VigraPixGrabber::setsize()
{
	if (format_ == VIGRAPIX_NOLOCK)
		format_ = vigrapix_get_format(handle_);

	int inw, inh, off;
	switch (format_) {

	default:
	case VIGRAPIX_NTSC:
		inw = 640;
		inh = 480;
		off = 15;
		break;

	case VIGRAPIX_PAL:
	case VIGRAPIX_SECAM:
		inw = 768;
		inh = 576;
		off = 19;
		break;
	}
	set_size_422(inw / decimate_, inh / decimate_);
	allocref();
	if (format_ != VIGRAPIX_NOLOCK)
		vigrapix_set_scaler(handle_, 0, off, inw, inh / 2,
				    outw_, outh_);
}

void VigraPixGrabber::start()
{
	vigrapix_set_port(handle_, port_);
	setsize();
	Grabber::start();
}

int VigraPixGrabber::command(int argc, const char*const* argv)
{
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
			const char* name = argv[2];
			int p = VIGRAPIX_COMP;
			if (strcasecmp(name, "s-video-1") == 0)
				p = VIGRAPIX_SVIDEO1;
			else if (strcasecmp(name, "s-video-2") == 0)
				p = VIGRAPIX_SVIDEO2;
			if (p != port_) {
				port_ = p;
				vigrapix_set_port(handle_, p);
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "contrast") == 0) {
			contrast(atof(argv[2]));
			return (TCL_OK);
		}
	} else if (argc == 2) {
#ifdef notdef
		if (strcmp(argv[1], "normalize") == 0)
			normalize();
		else
#endif
		if (strcmp(argv[1], "format") == 0) {
			Tcl& tcl = Tcl::instance();
			switch (format_) {

			case VIGRAPIX_NOLOCK:
				tcl.result("no-lock");
				break;

			case VIGRAPIX_NTSC:
				tcl.result("ntsc");
				break;

			case VIGRAPIX_PAL:
				tcl.result("pal");
				break;

			case VIGRAPIX_SECAM:
				tcl.result("secam");
				break;

			default:
				tcl.result("");
				break;
			}
			return (TCL_OK);
		}
	}
	return (Grabber::command(argc, argv));
}

/*
 * 4:2:2 transfer
 */
int VigraPixGrabber::capture()
{
	if (vigrapix_next_field(handle_) < 0)
		return (-1);

	volatile u_int *fifo = (u_int *)handle_->fifo;
	u_char* yp = frame_;
	u_char* up = yp + framesize_;
	u_char* vp = up + (framesize_ >> 1);

	for (int k = framesize_; k > 0; ) {
		/* Y0 U Y1 V */
		u_int v = *fifo;
		yp[0] = v >> 24;
		yp[1] = v >> 8;
		yp += 2;
		*up++ = (v >> 16) ^ 0x80;
		*vp++ = v ^ 0x80;
		k -= 2;
	}
	return (0);
}

/*
 * 4:1:1 transfer
 */
int CIFVigraPixGrabber::capture()
{
	if (vigrapix_next_field(handle_) < 0)
		return (-1);

	volatile u_int *fifo = (u_int *)handle_->fifo;
	
	u_char* yp = frame_;
	u_char* up = yp + framesize_;
	u_char* vp = up + (framesize_ >> 2);

	if (format_ == VIGRAPIX_NTSC && decimate_ <= 2) {
		int off = vstart_ * outw_;
		yp += (off + hstart_) * 16;
		off = ((off >> 1) + hstart_) * 8;
		up += off;
		vp += off;
		int ypinc = outw_ - inw_;
		int h = (vstop_ - vstart_) * 8;
		while (--h >= 0) {
			int w;
			for (w = inw_ >> 1; --w >= 0; ) {
				/* Y0 U Y1 V */
				u_int v = *fifo;
				yp[0] = v >> 24;
				yp[1] = v >> 8;
				yp += 2;
				*up++ = (v >> 16) ^ 0x80;
				*vp++ = v ^ 0x80;
			}
			yp += ypinc;
			up += ypinc >> 1;
			vp += ypinc >> 1;
			for (w = inw_ >> 1; --w >= 0; ) {
				u_int v = *fifo;
				yp[0] = v >> 24;
				yp[1] = v >> 8;
				yp += 2;
			}
			yp += ypinc;
		}
	} else {
		int h;
		for (h = outh_ >> 1; --h >= 0; ) {
			int w;
			for (w = outw_ >> 1; --w >= 0; ) {
				/* Y0 U Y1 V */
				u_int v = *fifo;
				yp[0] = v >> 24;
				yp[1] = v >> 8;
				yp += 2;
				*up++ = (v >> 16) ^ 0x80;
				*vp++ = v ^ 0x80;
			}
			for (w = outw_ >> 1; --w >= 0; ) {
				u_int v = *fifo;
				yp[0] = v >> 24;
				yp[1] = v >> 8;
				yp += 2;
			}
		}
	}
	return (0);
}

int VigraPixGrabber::grab()
{
	if (format_ == VIGRAPIX_NOLOCK) {
		int fmt = vigrapix_get_format(handle_);
		if (fmt != VIGRAPIX_NOLOCK)
			setsize();
	}
	if (format_ != VIGRAPIX_NOLOCK) {
		if (capture() < 0)
			return (0);
		suppress(frame_);
		saveblks(frame_);
	}
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

void CIFVigraPixGrabber::setsize()
{
	if (format_ == VIGRAPIX_NOLOCK)
		format_ = vigrapix_get_format(handle_);

	int inw, inh, off;
	switch (format_) {

	default:
	case VIGRAPIX_NTSC:
		inw = 640;
		inh = 480;
		off = 15;
		break;

	case VIGRAPIX_PAL:
	case VIGRAPIX_SECAM:
		inw = 768;
		inh = 576;
		off = 19;
		break;
	}
	/* decimate is 2 or 4 */
	set_size_cif(inw / decimate_, inh / decimate_);
	allocref();

	if (format_ == VIGRAPIX_NTSC && decimate_ <= 2)
		vigrapix_set_scaler(handle_, 0, off, inw, inh / 2,
				    inw / decimate_, inh / decimate_);
	else if (format_ != VIGRAPIX_NOLOCK)
		vigrapix_set_scaler(handle_, 0, off, inw, inh / 2,
				    outw_, outh_);
}
