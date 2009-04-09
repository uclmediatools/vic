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

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <vl/vl.h>

#include "grabber.h"
#include "crdef.h"
#include "module.h"
#include "device-input.h"
#include "vic_tcl.h"

class vlDevice;

class VLGrabber : public Grabber {
 public:
	VLGrabber(vlDevice&);
	virtual ~VLGrabber();
	virtual int command(int argc, const char*const* argv);
	virtual void fps(int);
	virtual void start();
	virtual void stop();
 protected:
	void startup_vl();
	void shutdown_vl();
	virtual void saveblks(const u_char*);
	void suppress(const u_char*);
	virtual int grab();
	virtual void setsize(int xsize, int ysize);
	u_char* next_frame();
	int closest_rate(int fps) const;

	VLServer vlServer_;
	VLPath vlPath_;
	VLBuffer transferBuf_;
	VLNode src_;
	VLNode drn_;
	int decimate_;
	int current_decimate_;
	int current_fps_;
	int current_port_;
	int is_pal_;

	int port_;
	vlDevice& device_;
};

class VLCIFGrabber : public VLGrabber {
    public:
	VLCIFGrabber(vlDevice&);
    protected:
	virtual void setsize(int xsize, int ysize);
	void saveblk(const u_char* in, u_char* yp, u_char* up,
		     u_char* vp, int stride, int is);
	virtual void saveblks(const u_char* in);
};

class VL420Grabber : public VLCIFGrabber {
    public:
	VL420Grabber(vlDevice&);
    protected:
	virtual void setsize(int xsize, int ysize);
};

class vlDevice : public InputDevice {
 public:
	vlDevice(const char* name, VLDev, VLNodeInfo* ports, int nport);
	virtual int command(int argc, const char*const* argv);
	inline int device() const { return (device_); }
	int lookup_port(const char*) const;
protected:
	VLDev device_;
	VLNodeInfo* ports_;
	int nport_;
};

static class vlBuilder {
public:
	vlBuilder();
	static VLNodeInfo* findports(VLNodeInfo* node, int& n);
} vlb;

vlDevice::vlDevice(const char* name, VLDev device,
		   VLNodeInfo* ports, int nport)
	: InputDevice(name), device_(device),
	  ports_(ports), nport_(nport)
{
	/* smash "ev1" insto galileo for better familiarity */
	if (nickname_[0] == 'e' && nickname_[1] == 'v') {
		nickname_ = "galileo";
	}
	char* cp = new char[80 + nport * (VL_NAME_SIZE + 1)];
	attributes_ = cp;
	strcpy(cp, "format { 420 422 } size { small cif } port { ");
	cp += strlen(cp);
	*cp++ = ' ';
	for (int i = 0; i < nport; ++i) {
		strcpy(cp, ports[i].name);
		cp += strlen(cp);
		*cp++ = ' ';
	}
	*cp++ = '}';
	*cp = 0;
}

int vlDevice::lookup_port(const char* name) const
{
	for (int i = 0; i < nport_; ++i) {
		if (strcasecmp(name, ports_[i].name) == 0)
			return (ports_[i].number);
	}
	abort();
}

int vlDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			TclObject* o = 0;
			if (strcmp(argv[2], "422") == 0)
				o = new VLGrabber(*this);
			else if (strcmp(argv[2], "420") == 0)
				o = new VL420Grabber(*this);
			else if (strcmp(argv[2], "cif") == 0)
				o = new VLCIFGrabber(*this);
			if (o != 0)
				tcl.result(o->name());
			return (TCL_OK);
		}
	}
	return (InputDevice::command(argc, argv));
}

vlBuilder::vlBuilder()
{
	VLServer s = vlOpenVideo("");
	if (s == 0)
		return;
	VLDevList devlist;
	if (vlGetDeviceList(s, &devlist) == 0) {
		for (int i = 0; i < devlist.numDevices; ++i) {
			VLDevice* p = &devlist.devices[i];
			int n = p->numNodes;
			VLNodeInfo* ni = findports(p->nodes, n);
			if (n > 0)
				new vlDevice(p->name, p->dev, ni, n);
		}
	}
	vlCloseVideo(s);
}

/*
 * Lookup all video sources and smash spaces in names to dashes
 * to avoid tcl list problems.
 */
VLNodeInfo* vlBuilder::findports(VLNodeInfo* node, int& n)
{
	VLNodeInfo* result = new VLNodeInfo[n];
	int k = 0;
	for (int i = 0; i < n; ++i) {
		if (node[i].type == VL_SRC && node[i].kind == VL_VIDEO) {
			result[k] = node[i];
			for (char* cp = result[k].name; *cp != 0; ++cp)
				if (isspace(*cp))
					*cp = '-';
			++k;
		}
	}
	n = k;
	return (result);
}

u_char* VLGrabber::next_frame()
{
	register u_char* p = 0;

	if (vlServer_ && transferBuf_) {
		/*
		 * wait at most one frame time (30ms) for a frame to show up.
		 */
		VLInfoPtr info = 0;
		for (register int i = 3; --i >= 0; ) {
			info = vlGetLatestValid(vlServer_, transferBuf_);
			if (info != 0)
				break;
			sginap(1);
		}
		if (info) {
			p = (u_char*)vlGetActiveRegion(vlServer_, transferBuf_,
						      info);
			if (p == NULL)
				vlPerror("vic: vlGetActiveRegion");
		}
	}
	return (p);
}

/* 422 */
inline void saveblk(const u_char* in,
		    u_char* yp, u_char* up, u_char* vp, int stride)
{
	int is = stride << 2;
	int cs = stride >> 1;

	for (int i = 16; --i >= 0; ) {
		/*
		 * Each iteration of this loop grabs 16 Ys & 8 U/Vs.
		 */
		register u_int y0, y1, u, v;

		u  = in[0]  << 24 | in[8]  << 16 | in[16] << 8 | in[24];
		v  = in[2]  << 24 | in[10] << 16 | in[18] << 8 | in[26];
		y0 = in[1]  << 24 | in[5]  << 16 | in[9]  << 8 | in[13];
		y1 = in[17] << 24 | in[21] << 16 | in[25] << 8 | in[29];

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;
		*(u_int*)up = u;
		*(u_int*)vp = v;

		u  = in[32+0]  << 24 | in[32+8]  << 16 | in[32+16] << 8 | in[32+24];
		v  = in[32+2]  << 24 | in[32+10] << 16 | in[32+18] << 8 | in[32+26];
		y0 = in[32+1]  << 24 | in[32+5]  << 16 | in[32+9]  << 8 | in[32+13];
		y1 = in[32+17] << 24 | in[32+21] << 16 | in[32+25] << 8 | in[32+29];

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;
		((u_int*)up)[1] = u;
		((u_int*)vp)[1] = v;

		in += is;
		yp += stride;
		up += cs;
		vp += cs;
	}
}

void VLGrabber::saveblks(const u_char* in)
{
	const u_int8_t* crv = crvec_;
	u_int8_t* yp = frame_;
	u_int8_t* up = yp + framesize_;
	u_int off = framesize_ >> 1;
	int stride = 15 * outw_;
	for (int y = 0; y < blkh_; ++y) {
		for (int x = 0; x < blkw_; ++x) {
			int s = *crv++;
			if ((s & CR_SEND) != 0)
				saveblk(in, yp, up, up + off, outw_);

			in += 64;
			yp += 16;
			up += 8;
		}
		yp += stride;
		up += stride >> 1;
		in += stride << 2;
	}
}

/*
 * define these for REPLENISH macro used below
 */
#define DIFF4(in, frm, v) \
	v += (in)[1] - (frm)[0]; \
	v += (in)[5] - (frm)[1]; \
	v += (in)[9] - (frm)[2]; \
	v += (in)[13] - (frm)[3];

#define DIFFLINE(in, frm, left, center, right) \
	DIFF4(in, frm, left); \
	DIFF4(in + 1*16, frm + 1*4, center); \
	DIFF4(in + 2*16, frm + 2*4, center); \
	DIFF4(in + 3*16, frm + 3*4, right); \
	if (right < 0) \
		right = -right; \
	if (left < 0) \
		left = -left; \
	if (center < 0) \
		center = -center;


void VLGrabber::suppress(const u_char* devbuf)
{
	const u_char* start = frame_ + 16 * vstart_ * outw_ + 16 * hstart_;
	REPLENISH(devbuf, start, inw_ << 2, 4,
		  hstart_, hstop_, vstart_, vstop_);
}

int VLGrabber::grab()
{
	u_char* p = next_frame();
	if (p == 0)
		return (0);
	suppress(p);
	saveblks(p);
	vlPutFree(vlServer_, transferBuf_);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

void VLGrabber::setsize(int w, int h)
{
	set_size_422(w, h);
}

int VLGrabber::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "decimate") == 0) {
			int d = atoi(argv[2]);
			Tcl& tcl = Tcl::instance();
			if (d <= 0) {
				tcl.result("divide by zero");
				return (TCL_ERROR);
			}
			decimate_ = d;
			startup_vl();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "port") == 0) {
			port_ = device_.lookup_port(argv[2]);
			startup_vl();
			return (TCL_OK);
		}
	}
	return (Grabber::command(argc, argv));
}

void VLGrabber::start()
{
	startup_vl();
	Grabber::start();
}

void VLGrabber::stop()
{
	shutdown_vl();
	Grabber::stop();
}

void VLGrabber::fps(int f)
{
	Grabber::fps(f);
	startup_vl();
}

void VLGrabber::shutdown_vl()
{
	if (vlServer_) {
		if (vlPath_) {
			if (transferBuf_) {
				vlEndTransfer(vlServer_, vlPath_);
				vlDeregisterBuffer(vlServer_, vlPath_, drn_,
						   transferBuf_);
				vlDestroyBuffer(vlServer_, transferBuf_);
				transferBuf_ = 0;
			}
			vlDestroyPath(vlServer_, vlPath_);
			vlPath_ = 0;
		}
		vlCloseVideo(vlServer_);
		vlServer_ = 0;
	}
}

static int vl_ntsc_rates[] = { 30, 25, 24, 20, 18, 15, 12, 10, 6, 5 };
static int vl_pal_rates[] = { 25, 20, 15, 10, 5 };

int VLGrabber::closest_rate(int fps) const
{
	if (fps <= 5)
		return (5);
	int* rate = is_pal_ ? vl_pal_rates : vl_ntsc_rates;
	while (rate[1] > fps)
		++rate;
	return (*rate);
}

void VLGrabber::startup_vl()
{
	if (vlServer_ != 0 &&
	    decimate_ == current_decimate_ && fps_ == current_fps_ &&
	    port_ == current_port_)
		return;

	current_decimate_ = decimate_;
	current_fps_ = fps_;
	current_port_ = port_;
	shutdown_vl();

	VLControlValue val;
	int xsize;
	int ysize;

	vlServer_ = vlOpenVideo("");
	if (vlServer_ == 0)
		goto failed;
	src_ = vlGetNode(vlServer_, VL_SRC, VL_VIDEO, port_);
	drn_ = vlGetNode(vlServer_, VL_DRN, VL_MEM, VL_ANY);
	vlPath_ = vlCreatePath(vlServer_, device_.device(), src_, drn_);
	if (vlPath_ < 0) {
		vlPerror("vic: create path");
		goto failed;
	}
	if (vlSetupPaths(vlServer_, (VLPathList)&vlPath_, 1,
			 VL_SHARE, VL_SHARE) < 0) {
		vlPerror("vic: set up paths");
		goto failed;
	}
	val.intVal = VL_PACKING_YVYU_422_8;
	if (vlSetControl(vlServer_, vlPath_, drn_, VL_PACKING, &val) < 0) {
		vlPerror("vic: set control packing");
		goto failed;
	}

	vlGetControl(vlServer_, vlPath_, drn_, VL_SIZE, &val);
	xsize = val.xyVal.x;
	ysize = val.xyVal.y &~ 7;
	if (xsize != 640)	/*XXX there must be a better way */
		is_pal_ = 1;
	else
		is_pal_ = 0;
	xsize /= decimate_;
	ysize /= decimate_;

	val.fractVal.numerator = closest_rate(fps_);
	val.fractVal.denominator = 1;
	if (vlSetControl(vlServer_, vlPath_, drn_, VL_RATE, &val) < 0)
		vlPerror("vic: set rate");

	val.intVal = decimate_ == 1? VL_CAPTURE_INTERLEAVED :
				     VL_CAPTURE_EVEN_FIELDS;
	if (vlSetControl(vlServer_, vlPath_, drn_, VL_CAP_TYPE, &val) < 0) {
		vlPerror("vic: vl set captype");
		goto failed;
	}
	if (decimate_ != 1) {
		val.fractVal.numerator = 1;
		val.fractVal.denominator = decimate_ >> 1;
		if (vlSetControl(vlServer_, vlPath_, drn_, VL_ZOOM, &val) < 0)
			vlPerror("vic: zoom");
	}

	/* Create ring buffers to hold captured data of new size. */
	transferBuf_ = vlCreateBuffer(vlServer_, vlPath_, drn_, 2);
	if (transferBuf_ == 0) {
		vlPerror("vic: create Buffer");
		goto failed;
	}
	/* Tell VL to use newly created ring buffer. */
	if (vlRegisterBuffer(vlServer_, vlPath_, drn_, transferBuf_) < 0) {
		vlPerror("vic: buffer registration");
		goto failed;
	}
	/*
	 * Try to get into frame sync for the first transfer to minimize
	 * interlace artifacts in the captured picture.
	 * it would be nice to use VL_TRANSFER_MODE_AUTOTRIGGER here so
	 * we'd stay in frame sync but it seems to do nothing but make
	 * videod hang with a galileo.
	 */
#ifdef notdef
	VLTransferDescriptor xfer;
	xfer.mode = VL_TRANSFER_MODE_CONTINUOUS;
	xfer.trigger = VLFrameVerticalRetraceMask;
	xfer.delay = 0;
	xfer.count = 1;
	if (vlBeginTransfer(vlServer_, vlPath_, 1, &xfer) < 0)
#endif
	if (vlBeginTransfer(vlServer_, vlPath_, 0, NULL) < 0) {
		vlPerror("vic: begin transfer");
		goto failed;
	}
#ifdef notdef
	/*
	 * it would be nice to wait on frame available events from
	 * videod rather than busy-waiting in next_frame() but the VL
	 * interface doeson't expose the fd so you can't select on it.
	 */
	vlSelectEvents(vlServer_, vlPath_, VLNoEventsMask);
#endif
	setsize(xsize, ysize);
	return;
 failed:
	shutdown_vl();
	status_ = -1;
}

VLGrabber::VLGrabber(vlDevice& device)
	: device_(device), port_(VL_ANY)
{
	frame_ = 0;
	framebase_ = 0;
	current_decimate_ = -1;
	current_fps_ = -1;
	current_port_ = -1;
	transferBuf_ = 0;
	Grabber::fps(2);
	decimate_ = 2;
	vlServer_ = 0;
	transferBuf_ = 0;
	vlPath_ = 0;
}

VLGrabber::~VLGrabber()
{
	shutdown_vl();
}

VLCIFGrabber::VLCIFGrabber(vlDevice& device)
	: VLGrabber(device)
{
}

void VLCIFGrabber::setsize(int w, int h)
{
	set_size_cif(w, h);
}

/* 420 */
inline void 
VLCIFGrabber::saveblk(const u_char* in, u_char* yp, u_char* up,
		      u_char* vp, int stride, int is)
{
	int cs = stride >> 1;

	for (int i = 8; --i >= 0; ) {
		register u_int y0, y1, u, v;

		u  = in[0]  << 24 | in[8]  << 16 | in[16] << 8 | in[24];
		v  = in[2]  << 24 | in[10] << 16 | in[18] << 8 | in[26];
		y0 = in[1]  << 24 | in[5]  << 16 | in[9]  << 8 | in[13];
		y1 = in[17] << 24 | in[21] << 16 | in[25] << 8 | in[29];

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;
		((u_int*)up)[0] = u;
		((u_int*)vp)[0] = v;

		u  = in[32+0]  << 24 | in[32+8]  << 16 | in[32+16] << 8 | in[32+24];
		v  = in[32+2]  << 24 | in[32+10] << 16 | in[32+18] << 8 | in[32+26];
		y0 = in[32+1]  << 24 | in[32+5]  << 16 | in[32+9]  << 8 | in[32+13];
		y1 = in[32+17] << 24 | in[32+21] << 16 | in[32+25] << 8 | in[32+29];

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;
		((u_int*)up)[1] = u;
		((u_int*)vp)[1] = v;

		in += is;
		yp += stride;
		up += cs;
		vp += cs;

		/* do the 2nd (y only instead of yuv) line */
		y0 = in[1]  << 24 | in[5]  << 16 | in[9]  << 8 | in[13];
		y1 = in[17] << 24 | in[21] << 16 | in[25] << 8 | in[29];

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;

		y0 = in[32+1]  << 24 | in[32+5]  << 16 | in[32+9]  << 8 | in[32+13];
		y1 = in[32+17] << 24 | in[32+21] << 16 | in[32+25] << 8 | in[32+29];

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;

		in += is;
		yp += stride;
	}
}

void VLCIFGrabber::saveblks(const u_char* in)
{
	int is = 4 * inw_;
	const u_int8_t* crv = crvec_;
	u_int8_t* lum = frame_;
	u_int8_t* chm = frame_ + framesize_;
	u_int off = framesize_ >> 2;

	crv += vstart_ * blkw_ + hstart_;
	lum += vstart_ * outw_ * 16 + hstart_ * 16;
	chm += vstart_ * (outw_ >> 1) * 8 + hstart_ * 8;

	int skip = hstart_ + (blkw_ - hstop_);

	for (int y = vstart_; y < vstop_; ++y) {
		const u_char* nin = in;
		for (int x = hstart_; x < hstop_; ++x) {
			int s = *crv++;
			if ((s & CR_SEND) != 0)
				saveblk(in, lum, chm, chm + off, outw_, is);

			in += 64;
			lum += 16;
			chm += 8;
		}
		crv += skip;
		lum += 15 * outw_ + skip * 16;
		chm += 7 * (outw_ >> 1) + skip * 8;
		in = nin + 16 * is;
	}
}

VL420Grabber::VL420Grabber(vlDevice& device)
	: VLCIFGrabber(device)
{
}

void VL420Grabber::setsize(int w, int h)
{
	set_size_420(w, h);
}
