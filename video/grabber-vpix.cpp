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

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include "config.h"
#include <fcntl.h>
//#include <sys/types.h>
#include <sys/ioctl.h>

extern "C" {
#include <vfc_lib.h>
}

#include "grabber.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "module.h"

/*XXX*/
#define NTSC_WIDTH 320
#define NTSC_HEIGHT 240
#define PAL_WIDTH 384
#define PAL_HEIGHT 288
#define CIF_WIDTH 352
#define CIF_HEIGHT 288

/*XXX*/
#if defined(sun) && defined(__GNUC__)
#define VOLATILE register volatile
#else
/* sun's C++ compiler doesn't handle 'register volatile' correctly */
#define VOLATILE volatile
#endif


class VideoPixGrabber : public Grabber {
    public:
	VideoPixGrabber();
	virtual ~VideoPixGrabber();
	virtual void start();
    protected:
	virtual int command(int argc, const char*const* argv);
	int capture();
	virtual int grab();
	virtual void NTSCgrabSmall();
	virtual void NTSCgrabMedium();
	virtual void PALgrabSmall();
	virtual void PALgrabMedium();
	void format();
	void normalize();
	virtual void setsize();

	int format_;		/* video format: NTSC or PAL */
	int rformat_;		/* requested format, above + AUTO */
	VfcDev* vfcdev_;
	int port_;		/* videopix input port */
	u_int basewidth_;
	u_int baseheight_;
	u_int decimate_;
};

class VideoPix420Grabber : public VideoPixGrabber {
    public:
	VideoPix420Grabber();
    protected:
	virtual void NTSCgrabSmall();
	virtual void NTSCgrabMedium();
	virtual void PALgrabSmall();
	virtual void PALgrabMedium();
	virtual void setsize();
	int loff_;		/* offset from start of frame to scan */
	int coff_;		/* offset from start of frame to scan */
	int hwrap_;		/* amount to skip on each output line */
	int hskip_;		/* amount of input to throw out on each line */
};

class CIFVideoPixGrabber : public VideoPix420Grabber {
    protected:
	virtual void PALgrabSmall();
	virtual void PALgrabMedium();
	virtual void setsize();
};

class VideoPixDevice : public InputDevice {
    public:
	VideoPixDevice(const char*);
	virtual int command(int argc, const char*const* argv);
};

static VideoPixDevice vpix_device("videopix");

VideoPixDevice::VideoPixDevice(const char* name) : InputDevice(name)
{
	if (access("/dev/vfc0", R_OK) == 0)
		attributes_ = "\
format { 420 422 } \
size { small cif } \
port { Composite-1 Composite-2 S-Video}";
	else
		attributes_ = "disabled";
}

int VideoPixDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			TclObject* o = 0;
			if (strcmp(argv[2], "422") == 0)
				o = new VideoPixGrabber;
			else if (strcmp(argv[2], "420") == 0)
				o = new VideoPix420Grabber;
			else if (strcmp(argv[2], "cif") == 0)
				o = new CIFVideoPixGrabber;
			if (o != 0)
				tcl.result(o->name());
			return (TCL_OK);
		}
	}
	return (InputDevice::command(argc, argv));
}

VideoPixGrabber::VideoPixGrabber()
{
	/*
	 * This is a horrible hack to prevent the VFC library from printing
	 * error messages that we would rather not print.
	 */
	int fd = dup(2);
	close(2);
	open("/dev/null", O_RDONLY);
	vfcdev_ = vfc_open("/dev/vfc0", VFC_LOCKDEV);
	dup2(fd, 2);
	close(fd);
	if (vfcdev_ == 0) {
		status_ = -1;
		return;
	}
	port_ = 1;
	/*XXX*/
	format_ = -1;
	rformat_ = VFC_AUTO;
	decimate_ = 2;
	basewidth_ = 0;
	baseheight_ = 0;
}

VideoPixGrabber::~VideoPixGrabber()
{
	if (vfcdev_ != 0) {
		vfc_destroy(vfcdev_);
		vfcdev_ = 0;
	}
}

void VideoPixGrabber::setsize()
{
	if (format_ < 0)
		return;
	set_size_422(basewidth_ / decimate_, baseheight_ / decimate_);
	allocref();
}

void VideoPixGrabber::format()
{
 	int format;
	vfc_set_port(vfcdev_, port_);
	vfc_set_format(vfcdev_, rformat_, &format);
	int w, h;
	switch (format) {

	default:
 	case NO_LOCK:
		if (rformat_ == VFC_AUTO)
			fprintf(stderr,
			"vic: videopix sees no signal - using ntsc.\n");
		/* fall through */
	case NTSC_COLOR:
	case NTSC_NOCOLOR:
		format_ = VFC_NTSC;
		w = NTSC_WIDTH * 2;
		h = NTSC_HEIGHT * 2;
		break;

	case PAL_COLOR:
	case PAL_NOCOLOR:
		format_ = VFC_PAL;
		w = PAL_WIDTH * 2;
		h = PAL_HEIGHT * 2;
		break;
	}
	basewidth_ = w;
	baseheight_ = h;
	setsize();
}

/*XXX*/
void VideoPixGrabber::normalize()
{
	if (! running_)
		return;

	/* set the intensity map to an identity map & capture a frame. */
	int i;
	for (i = 0; i < 256; ++i)
		ynorm_[i] = u_char(i);

	capture();

	/* find the min & max y values */
	u_char* fp = frame_;
	u_int min = *fp++;
	u_int max = min;
	for (i = framesize_; --i > 0; ) {
		u_int p = *fp++;
		if (p < min)
			min = p;
		else if (p > max)
			max = p;
	}
	/*
	 * if the range of the signal is small (i.e., all black) don't
	 * do anything.  Otherwise reset the min & max based on the
	 * frame & compute a new intensity map.
	 */
	if (max - min < 64)
		return;

	ymin_ = min;
	ymax_ = max;
	contrast(contrast_);
}

void VideoPixGrabber::start()
{
	format();
	Grabber::start();
}

int VideoPixGrabber::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "decimate") == 0) {
			int dec = atoi(argv[2]);
			Tcl& tcl = Tcl::instance();
			if (dec <= 0) {
				tcl.resultf("%s: divide by zero", argv[0]);
				return (TCL_ERROR);
			}
			if (u_int(dec) != decimate_) {
				decimate_ = dec;
				setsize();
			}
			return (TCL_OK);	
		} else if (strcmp(argv[1], "port") == 0) {
			int p = 0;
			if (strcmp(argv[2], "Composite-1") == 0)
				p = VFC_PORT1;
			else if (strcmp(argv[2], "Composite-2") == 0)
				p = VFC_PORT2;
			else if (strcmp(argv[2], "S-Video") == 0)
				p = VFC_SVIDEO;
			else {
				Tcl& tcl = Tcl::instance();
				tcl.resultf("%s: unknown port", argv[2]);
				return (TCL_ERROR);
			}
			if (p != port_) {
				port_ = p;
				vfc_set_port(vfcdev_, p);
			}
			return (TCL_OK);	
		} else if (strcmp(argv[1], "format") == 0) {
			if (strcmp(argv[2], "auto") == 0)
				rformat_ = VFC_AUTO;
			else if (strcmp(argv[2], "pal") == 0)
				rformat_ = VFC_PAL;
			else
				rformat_ = VFC_NTSC;
			if (running_)
				format();
			return (TCL_OK);	
		} else if (strcmp(argv[1], "contrast") == 0) {
			contrast(atof(argv[2]));
			return (TCL_OK);	
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "normalize") == 0) {
			normalize();
			return (TCL_OK);	
		} else if (strcmp(argv[1], "format") == 0) {
			Tcl& tcl = Tcl::instance();
			switch (format_) {

			case VFC_AUTO:
				tcl.result("auto");
				break;

			case VFC_NTSC:
				tcl.result("ntsc");
				break;

			case VFC_PAL:
				tcl.result("pal");
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

#ifdef VPIX_SUN4C
typedef unsigned long long u_ll;

#define PIX0 (d0.i.h)
#define PIX1 (d0.i.l)
#define PIX2 (d1.i.h)
#define PIX3 (d1.i.l)
#define BW_CAPTURE_DEFS \
	union { \
		u_ll l; \
		struct { \
			u_int h; \
			u_int l; \
		} i; \
	} d0, d1; \
	register u_int y; \
	register u_char* ynorm = ynorm_;
#define CAPTURE_DEFS \
	BW_CAPTURE_DEFS \
	register u_short u, v;
#define NEXT4 \
	d0.l = *(volatile u_ll*)iochan; \
	d0.i.h >>= 16; \
	d0.i.l >>= 16; \
	d1.l = *(volatile u_ll*)iochan; \
	d1.i.h >>= 16; \
	d1.i.l >>= 16;
#define PRESKIP(n) \
	for (int i = ((n) - 1) / 2; --i >= 0; ) \
		*(volatile u_ll*)iochan; \
	*iochan;
#define LINESKIP(n) \
{ \
	for (int i = (n) / 2; --i >= 0; ) \
		*(volatile u_ll*)iochan;  \
}

#else
#define PIX0 (d0)
#define PIX1 (d1)
#define PIX2 (d2)
#define PIX3 (d3)
#define BW_CAPTURE_DEFS \
	register u_int y, d0, d1, d2, d3; \
	register u_char* ynorm = ynorm_;
#define CAPTURE_DEFS \
	BW_CAPTURE_DEFS \
	register u_short u, v;
#define NEXT4 \
	d0 = *iochan >> 16; \
	d1 = *iochan >> 16; \
	d2 = *iochan >> 16; \
	d3 = *iochan >> 16;
#define PRESKIP(n) \
	for (int i = (n) - 1; --i >= 0; ) \
		*iochan;
#define LINESKIP(n) \
{ \
	for (int i = (n); --i >= 0; ) \
		*iochan; \
}
#endif

#define YSTART(r) \
	y = ynorm[(r) >> 8];
#define YADD(r) \
	y <<= 8; \
	y |= ynorm[(r) >> 8];
#define UVHIGH \
	u  = (PIX0 & 0xc0) << 8; \
	u |= (PIX1 & 0xc0) << 6; \
	u |= (PIX2 & 0xc0) << 4; \
	v  = (PIX0 & 0x30) << 10; \
	v |= (PIX1 & 0x30) << 8; \
	v |= (PIX2 & 0x30) << 6;
#define UVLOW \
	u |= (PIX0 & 0xc0); \
	u |= (PIX1 & 0xc0) >> 2; \
	u |= (PIX2 & 0xc0) >> 4; \
	v |= (PIX0 & 0x30) << 2; \
	v |= (PIX1 & 0x30); \
	v |= (PIX2 & 0x30) >> 2; \
	u ^= 0x8080; v ^= 0x8080;

void VideoPixGrabber::NTSCgrabMedium()
{
	VOLATILE u_int* iochan = (u_int*)vfcdev_->vfc_port1;
	PRESKIP(VFC_OSKIP_NTSC)

	u_char* lum = frame_;
	int off = framesize_;
	u_char* chm = frame_ + off;
	off >>= 1;

	for (int h = NTSC_HEIGHT; --h >= 0; ) {
		LINESKIP(16)
		for (int w = NTSC_WIDTH / 20; --w >= 0; ) {
			CAPTURE_DEFS

			NEXT4 YSTART(PIX0) YADD(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			((u_int*)lum)[0] = y;
			((u_short*)chm)[0] = u;
			((u_short*)(chm+off))[0] = v;

			NEXT4 YSTART(PIX0) YADD(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			((u_int*)lum)[1] = y;
			((u_short*)chm)[1] = u;
			((u_short*)(chm+off))[1] = v;

			NEXT4 YSTART(PIX0) UVHIGH /*did 9, skip 1*/
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			NEXT4 YADD(PIX0)
			((u_int*)lum)[2] = y;
			((u_short*)chm)[2] = u;
			((u_short*)(chm+off))[2] = v;

			YSTART(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			NEXT4 YADD(PIX0)
			((u_int*)lum)[3] = y;
			((u_short*)chm)[3] = u;
			((u_short*)(chm+off))[3] = v;

			YSTART(PIX2) UVHIGH
			NEXT4 YADD(PIX0) /* did 9, skip 1 */
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			((u_int*)lum)[4] = y;
			((u_short*)chm)[4] = u;
			((u_short*)(chm+off))[4] = v;

			lum += 4*5; chm += 2*5;
		}
	}
}

void VideoPixGrabber::NTSCgrabSmall()
{
	VOLATILE u_int* iochan = (u_int*)vfcdev_->vfc_port1;
	PRESKIP(VFC_OSKIP_NTSC)

	u_char* lum = frame_;
	int off = framesize_;
	u_char* chm = frame_ + off;
	off >>= 1;

	for (int h = NTSC_HEIGHT / 2; --h >= 0; ) {
		LINESKIP(720 + 16)
		for (int w = NTSC_WIDTH / 40; --w >= 0; ) {
			CAPTURE_DEFS

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0)
			((u_int*)lum)[0] = y;
			((u_short*)chm)[0] = u;
			((u_short*)(chm+off))[0] = v;

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0)
			((u_int*)lum)[1] = y;
			((u_short*)chm)[1] = u;
			((u_short*)(chm+off))[1] = v;

			/*after 9, skip 1*/
			NEXT4 YSTART(PIX0) NEXT4 UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0)
			((u_int*)lum)[2] = y;
			((u_short*)chm)[2] = u;
			((u_short*)(chm+off))[2] = v;

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0)
			((u_int*)lum)[3] = y;
			((u_short*)chm)[3] = u;
			((u_short*)(chm+off))[3] = v;

			/*after 9, skip 1*/
			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0) NEXT4
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0)
			((u_int*)lum)[4] = y;
			((u_short*)chm)[4] = u;
			((u_short*)(chm+off))[4] = v;

			lum += 4*5; chm += 2*5;
		}
	}
}

void VideoPixGrabber::PALgrabMedium()
{
	VOLATILE u_int* iochan = (u_int*)vfcdev_->vfc_port1;
	PRESKIP(VFC_ESKIP_PAL)

	u_char* lum = frame_;
	int off = framesize_;
	u_char* chm = frame_ + off;
	off >>= 1;

	for (int h = PAL_HEIGHT; --h >= 0; ) {
		LINESKIP(16)
		for (int w = PAL_WIDTH / 24; --w >= 0; ) {
			CAPTURE_DEFS

			NEXT4 YSTART(PIX0) YADD(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			((u_int*)lum)[0] = y;
			((u_short*)chm)[0] = u;
			((u_short*)(chm+off))[0] = v;

			NEXT4 YSTART(PIX0) YADD(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			((u_int*)lum)[1] = y;
			((u_short*)chm)[1] = u;
			((u_short*)(chm+off))[1] = v;

			NEXT4 YSTART(PIX0) YADD(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX1) UVLOW	/*dup 11th*/
			((u_int*)lum)[2] = y;
			((u_short*)chm)[2] = u;
			((u_short*)(chm+off))[2] = v;

			YSTART(PIX2) NEXT4 YADD(PIX0) UVHIGH
			YADD(PIX2) NEXT4 YADD(PIX0) UVLOW
			((u_int*)lum)[3] = y;
			((u_short*)chm)[3] = u;
			((u_short*)(chm+off))[3] = v;

			YSTART(PIX2) NEXT4 YADD(PIX0) UVHIGH
			YADD(PIX2) NEXT4 YADD(PIX0) UVLOW
			((u_int*)lum)[4] = y;
			((u_short*)chm)[4] = u;
			((u_short*)(chm+off))[4] = v;

			YSTART(PIX2) NEXT4 YADD(PIX0) UVHIGH
			YADD(PIX2) YADD(PIX3) UVLOW		/*dup 11th*/
			((u_int*)lum)[5] = y;
			((u_short*)chm)[5] = u;
			((u_short*)(chm+off))[5] = v;

			lum += 4*6; chm += 2*6;
		}
	}
}

void VideoPixGrabber::PALgrabSmall()
{
	VOLATILE u_int* iochan = (u_int*)vfcdev_->vfc_port1;
	PRESKIP(VFC_ESKIP_PAL)

	u_char* lum = frame_;
	int off = framesize_;
	u_char* chm = frame_ + off;
	off >>= 1;

	for (int h = PAL_HEIGHT / 2; --h >= 0; ) {
		LINESKIP(720 + 16)
		for (int w = PAL_WIDTH / 24; --w >= 0; ) {
			CAPTURE_DEFS

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0) 
			((u_int*)lum)[0] = y;
			((u_short*)chm)[0] = u;
			((u_short*)(chm+off))[0] = v;

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0) 
			((u_int*)lum)[1] = y;
			((u_short*)chm)[1] = u;
			((u_short*)(chm+off))[1] = v;

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW YADD(PIX2)	/*dup 11th*/
			((u_int*)lum)[2] = y;
			((u_short*)chm)[2] = u;
			((u_short*)(chm+off))[2] = v;

			lum += 4*3; chm += 2*3;
		}
	}
}

int VideoPixGrabber::capture()
{
	int cmd = CAPTRCMD;
	if (ioctl(vfcdev_->vfc_fd, VFCSCTRL, (char*)&cmd) == -1)
		return (0);

	switch (format_ | (decimate_ << 1)) {

	case VFC_PAL | (2 << 1):
		PALgrabMedium();
		break;

	case VFC_PAL | (4 << 1):
		PALgrabSmall();
		break;

	case VFC_NTSC | (2 << 1):
		NTSCgrabMedium();
		break;

	case VFC_NTSC | (4 << 1):
		NTSCgrabSmall();
		break;

	default:
		return (0);
	}
	return (1);
}

int VideoPixGrabber::grab()
{
	if (capture() == 0)
		return (0);
	suppress(frame_);
	saveblks(frame_);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

VideoPix420Grabber::VideoPix420Grabber()
	: loff_(0), coff_(0), hwrap_(0), hskip_(0)
{
}

void VideoPix420Grabber::setsize()
{
	if (format_ < 0)
		return;
	set_size_420(basewidth_ / decimate_, baseheight_ / decimate_);
	allocref();
	loff_ = 0;
	coff_ = 0;
	hskip_ = 0;
	hwrap_ = 0;
}

void CIFVideoPixGrabber::setsize()
{
	if (format_ < 0)
		return;
	set_size_cif(basewidth_ / decimate_, baseheight_ / decimate_);
	/*XXX*/
	vstart_ = 0;
	vstop_ = blkh_;
	hstart_ = 1;
	hstop_ = blkw_ - 1;

	allocref();

	int voff = (outh_ - inh_) / 2;
	hwrap_ = outw_ - inw_;
	int hoff = hwrap_ / 2;
	loff_ = outw_ * voff + hoff;
	coff_ = (outw_ >> 1) * (voff >> 1) + (hoff >> 1);
}

void VideoPix420Grabber::NTSCgrabMedium()
{
	VOLATILE u_int* iochan = (u_int*)vfcdev_->vfc_port1;
	PRESKIP(VFC_OSKIP_NTSC)

	u_char* lum = frame_;
	int off = framesize_;
	u_char* chm = frame_ + off;
	off >>= 2;

	lum += loff_;
	chm += coff_;

	int hwrap = hwrap_;
	for (int h = NTSC_HEIGHT; h > 0; h -= 2) {
		LINESKIP(16)
		int w;
		for (w = NTSC_WIDTH / 20; --w >= 0; ) {
			CAPTURE_DEFS

			NEXT4 YSTART(PIX0) YADD(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			((u_int*)lum)[0] = y;
			((u_short*)chm)[0] = u;
			((u_short*)(chm+off))[0] = v;

			NEXT4 YSTART(PIX0) YADD(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			((u_int*)lum)[1] = y;
			((u_short*)chm)[1] = u;
			((u_short*)(chm+off))[1] = v;

			NEXT4 YSTART(PIX0) UVHIGH /*did 9, skip 1*/
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			NEXT4 YADD(PIX0)
			((u_int*)lum)[2] = y;
			((u_short*)chm)[2] = u;
			((u_short*)(chm+off))[2] = v;

			YSTART(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			NEXT4 YADD(PIX0)
			((u_int*)lum)[3] = y;
			((u_short*)chm)[3] = u;
			((u_short*)(chm+off))[3] = v;

			YSTART(PIX2) UVHIGH
			NEXT4 YADD(PIX0) /* did 9, skip 1 */
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			((u_int*)lum)[4] = y;
			((u_short*)chm)[4] = u;
			((u_short*)(chm+off))[4] = v;

			lum += 4*5; chm += 2*5;
		}
		lum += hwrap;
		chm += hwrap >> 1;

		LINESKIP(16)
		for (w = NTSC_WIDTH / 20; --w >= 0; ) {
			BW_CAPTURE_DEFS

			NEXT4 YSTART(PIX0) YADD(PIX2)
			NEXT4 YADD(PIX0) YADD(PIX2)
			((u_int*)lum)[0] = y;

			NEXT4 YSTART(PIX0) YADD(PIX2)
			NEXT4 YADD(PIX0) YADD(PIX2)
			((u_int*)lum)[1] = y;

			NEXT4 YSTART(PIX0) /*did 9, skip 1*/
			NEXT4 YADD(PIX0) YADD(PIX2)
			NEXT4 YADD(PIX0)
			((u_int*)lum)[2] = y;

			YSTART(PIX2)
			NEXT4 YADD(PIX0) YADD(PIX2)
			NEXT4 YADD(PIX0)
			((u_int*)lum)[3] = y;

			YSTART(PIX2)
			NEXT4 YADD(PIX0) /* did 9, skip 1 */
			NEXT4 YADD(PIX0) YADD(PIX2)
			((u_int*)lum)[4] = y;

			lum += 4*5;
		}
		lum += hwrap;
	}
}

void VideoPix420Grabber::NTSCgrabSmall()
{
	VOLATILE u_int* iochan = (u_int*)vfcdev_->vfc_port1;
	PRESKIP(VFC_OSKIP_NTSC)

	u_char* lum = frame_;
	int off = framesize_;
	u_char* chm = frame_ + off;
	off >>= 2;

	lum += loff_;
	chm += coff_;

	int hwrap = hwrap_;
	for (int h = NTSC_HEIGHT / 2; h > 0; h -= 2) {
		LINESKIP(720 + 16)
		int w;
		for (w = NTSC_WIDTH / 40; --w >= 0; ) {
			CAPTURE_DEFS

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0)
			((u_int*)lum)[0] = y;
			((u_short*)chm)[0] = u;
			((u_short*)(chm+off))[0] = v;

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0)
			((u_int*)lum)[1] = y;
			((u_short*)chm)[1] = u;
			((u_short*)(chm+off))[1] = v;

			/*after 9, skip 1*/
			NEXT4 YSTART(PIX0) NEXT4 UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0)
			((u_int*)lum)[2] = y;
			((u_short*)chm)[2] = u;
			((u_short*)(chm+off))[2] = v;

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0)
			((u_int*)lum)[3] = y;
			((u_short*)chm)[3] = u;
			((u_short*)(chm+off))[3] = v;

			/*after 9, skip 1*/
			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0) NEXT4
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0)
			((u_int*)lum)[4] = y;
			((u_short*)chm)[4] = u;
			((u_short*)(chm+off))[4] = v;

			lum += 4*5; chm += 2*5;
		}
		lum += hwrap;
		chm += hwrap >> 1;

		LINESKIP(720 + 16)
		for (w = NTSC_WIDTH / 40; --w >= 0; ) {
			BW_CAPTURE_DEFS

			NEXT4 YSTART(PIX0) NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) NEXT4 YADD(PIX0)
			((u_int*)lum)[0] = y;

			NEXT4 YSTART(PIX0) NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) NEXT4 YADD(PIX0)
			((u_int*)lum)[1] = y;

			/*after 9, skip 1*/
			NEXT4 YSTART(PIX0) NEXT4 NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) NEXT4 YADD(PIX0)
			((u_int*)lum)[2] = y;

			NEXT4 YSTART(PIX0) NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) NEXT4 YADD(PIX0)
			((u_int*)lum)[3] = y;

			/*after 9, skip 1*/
			NEXT4 YSTART(PIX0) NEXT4 YADD(PIX0) NEXT4
			NEXT4 YADD(PIX0) NEXT4 YADD(PIX0)
			((u_int*)lum)[4] = y;

			lum += 4*5;
		}
		lum += hwrap;
	}
}

void VideoPix420Grabber::PALgrabMedium()
{
	VOLATILE u_int* iochan = (u_int*)vfcdev_->vfc_port1;
	PRESKIP(VFC_ESKIP_PAL)

	u_char* lum = frame_;
	int off = framesize_;
	u_char* chm = frame_ + off;
	off >>= 2;

	for (int h = PAL_HEIGHT / 2; --h >= 0; ) {
		LINESKIP(16)
		int w;
		for (w = PAL_WIDTH / 24; --w >= 0; ) {
			CAPTURE_DEFS

			NEXT4 YSTART(PIX0) YADD(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			((u_int*)lum)[0] = y;
			((u_short*)chm)[0] = u;
			((u_short*)(chm+off))[0] = v;

			NEXT4 YSTART(PIX0) YADD(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			((u_int*)lum)[1] = y;
			((u_short*)chm)[1] = u;
			((u_short*)(chm+off))[1] = v;

			NEXT4 YSTART(PIX0) YADD(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX1) UVLOW	/*dup 11th*/
			((u_int*)lum)[2] = y;
			((u_short*)chm)[2] = u;
			((u_short*)(chm+off))[2] = v;

			YSTART(PIX2) NEXT4 YADD(PIX0) UVHIGH
			YADD(PIX2) NEXT4 YADD(PIX0) UVLOW
			((u_int*)lum)[3] = y;
			((u_short*)chm)[3] = u;
			((u_short*)(chm+off))[3] = v;

			YSTART(PIX2) NEXT4 YADD(PIX0) UVHIGH
			YADD(PIX2) NEXT4 YADD(PIX0) UVLOW
			((u_int*)lum)[4] = y;
			((u_short*)chm)[4] = u;
			((u_short*)(chm+off))[4] = v;

			YSTART(PIX2) NEXT4 YADD(PIX0) UVHIGH
			YADD(PIX2) YADD(PIX3) UVLOW		/*dup 11th*/
			((u_int*)lum)[5] = y;
			((u_short*)chm)[5] = u;
			((u_short*)(chm+off))[5] = v;

			lum += 4*6; chm += 2*6;
		}
		LINESKIP(16)
		for (w = PAL_WIDTH / 24; --w >= 0; ) {
			BW_CAPTURE_DEFS

			NEXT4 YSTART(PIX0) YADD(PIX2)
			NEXT4 YADD(PIX0) YADD(PIX2)
			((u_int*)lum)[0] = y;

			NEXT4 YSTART(PIX0) YADD(PIX2)
			NEXT4 YADD(PIX0) YADD(PIX2)
			((u_int*)lum)[1] = y;

			NEXT4 YSTART(PIX0) YADD(PIX2)
			NEXT4 YADD(PIX0) YADD(PIX1)	/*dup 11th*/
			((u_int*)lum)[2] = y;

			YSTART(PIX2) NEXT4 YADD(PIX0)
			YADD(PIX2) NEXT4 YADD(PIX0)
			((u_int*)lum)[3] = y;

			YSTART(PIX2) NEXT4 YADD(PIX0)
			YADD(PIX2) NEXT4 YADD(PIX0)
			((u_int*)lum)[4] = y;

			YSTART(PIX2) NEXT4 YADD(PIX0)
			YADD(PIX2) YADD(PIX3)		/*dup 11th*/
			((u_int*)lum)[5] = y;

			lum += 4*6;
		}
	}
}

void VideoPix420Grabber::PALgrabSmall()
{
	VOLATILE u_int* iochan = (u_int*)vfcdev_->vfc_port1;
	PRESKIP(VFC_ESKIP_PAL)

	u_char* lum = frame_;
	int off = framesize_;
	u_char* chm = frame_ + off;
	off >>= 2;

	for (int h = PAL_HEIGHT / 2; h > 0; h -= 2) {
		LINESKIP(720 + 16)
		int w;
		for (w = PAL_WIDTH / 24; --w >= 0; ) {
			CAPTURE_DEFS

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0) 
			((u_int*)lum)[0] = y;
			((u_short*)chm)[0] = u;
			((u_short*)(chm+off))[0] = v;

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0) 
			((u_int*)lum)[1] = y;
			((u_short*)chm)[1] = u;
			((u_short*)(chm+off))[1] = v;

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW YADD(PIX2)	/*dup 11th*/
			((u_int*)lum)[2] = y;
			((u_short*)chm)[2] = u;
			((u_short*)(chm+off))[2] = v;

			lum += 4*3; chm += 2*3;
		}

		LINESKIP(720 + 16)
		for (w = PAL_WIDTH / 24; --w >= 0; ) {
			BW_CAPTURE_DEFS

			NEXT4 YSTART(PIX0) NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) NEXT4 YADD(PIX0) 
			((u_int*)lum)[0] = y;

			NEXT4 YSTART(PIX0) NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) NEXT4 YADD(PIX0) 
			((u_int*)lum)[1] = y;

			NEXT4 YSTART(PIX0) NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) YADD(PIX2)	/*dup 11th*/
			((u_int*)lum)[2] = y;

			lum += 4*3;
		}
	}
}

void CIFVideoPixGrabber::PALgrabMedium()
{
	VOLATILE u_int* iochan = (u_int*)vfcdev_->vfc_port1;
	PRESKIP(VFC_ESKIP_PAL)

	u_char* lum = frame_;
	int off = framesize_;
	u_char* chm = frame_ + off;
	off >>= 2;

	for (int h = PAL_HEIGHT / 2; --h >= 0; ) {
		LINESKIP(16)
		int w;
		for (w = CIF_WIDTH / 8; --w >= 0; ) {
			CAPTURE_DEFS

			NEXT4 YSTART(PIX0) YADD(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			((u_int*)lum)[0] = y;
			((u_short*)chm)[0] = u;
			((u_short*)(chm+off))[0] = v;

			NEXT4 YSTART(PIX0) YADD(PIX2) UVHIGH
			NEXT4 YADD(PIX0) YADD(PIX2) UVLOW
			((u_int*)lum)[1] = y;
			((u_short*)chm)[1] = u;
			((u_short*)(chm+off))[1] = v;

			lum += 4*2; chm += 2*2;
		}
		LINESKIP(16)
		for (w = CIF_WIDTH / 8; --w >= 0; ) {
			BW_CAPTURE_DEFS

			NEXT4 YSTART(PIX0) YADD(PIX2)
			NEXT4 YADD(PIX0) YADD(PIX2)
			((u_int*)lum)[0] = y;

			NEXT4 YSTART(PIX0) YADD(PIX2)
			NEXT4 YADD(PIX0) YADD(PIX2)
			((u_int*)lum)[1] = y;

			lum += 4*2;
		}
	}
}

void CIFVideoPixGrabber::PALgrabSmall()
{
	VOLATILE u_int* iochan = (u_int*)vfcdev_->vfc_port1;
	PRESKIP(VFC_ESKIP_PAL)

	u_char* lum = frame_;
	int off = framesize_;
	u_char* chm = frame_ + off;
	off >>= 2;

	for (int h = PAL_HEIGHT / 4; --h > 0; ) {
		LINESKIP(720 + 16)
		int w;
		for (w = CIF_WIDTH / 16; --w >= 0; ) {
			CAPTURE_DEFS

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0) 
			((u_int*)lum)[0] = y;
			((u_short*)chm)[0] = u;
			((u_short*)(chm+off))[0] = v;

			NEXT4 YSTART(PIX0) UVHIGH NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) UVLOW NEXT4 YADD(PIX0) 
			((u_int*)lum)[1] = y;
			((u_short*)chm)[1] = u;
			((u_short*)(chm+off))[1] = v;

			lum += 4*2; chm += 2*2;
		}

		LINESKIP(720 + 16)
		for (w = CIF_WIDTH / 16; --w >= 0; ) {
			BW_CAPTURE_DEFS

			NEXT4 YSTART(PIX0) NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) NEXT4 YADD(PIX0) 
			((u_int*)lum)[0] = y;

			NEXT4 YSTART(PIX0) NEXT4 YADD(PIX0)
			NEXT4 YADD(PIX0) NEXT4 YADD(PIX0) 
			((u_int*)lum)[1] = y;

			lum += 4*2;
		}
	}
}
