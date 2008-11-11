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

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <vl/vl.h>

#include "renderer.h"
#include "device-output.h"

class VLOutputDevice : public OutputDevice {
 public:
	VLOutputDevice(const char* name, VLDev, VLNodeInfo* ports, int nport);
	virtual int command(int argc, const char*const* argv);
	inline int device() const { return (device_); }
	int lookup_port(const char* name) const;
 protected:
	VLDev device_;
	VLNodeInfo* ports_;
	int nport_;
};

static class VLOutputBuilder {
public:
	VLOutputBuilder();
	static VLNodeInfo* findports(VLNodeInfo* node, int& n);
} vlob;

class VLOutboard : public Renderer {
public:
	VLOutboard(VLOutputDevice& device, int ft);
	~VLOutboard();
	int command(int argc, const char*const* argv);
	void startup_vl();
	void shutdown_vl();

	virtual int consume(const VideoFrame*);
	virtual void resize(int w, int h);
	/* XXX color easily make u/v planes 0x80 */
	virtual void setcolor(int c) {}
protected:
	virtual void scaleframe(u_char* bp, const u_char* frm) const = 0;
	virtual void upscaleframe(u_char* bp, const u_char* frm) const = 0;
	VLOutputDevice& device_;
	VLServer vl_;
	VLPath path_;
	VLBuffer rb_;
	int xfersize_;
	VLNode src_;
	VLNode drn_;
	int port_;
	int current_port_;

	int outw_;
	int outh_;
	int scale_;
	int oh0_;
	int oh1_;
	int ov0_;
	int ov1_;
	int ih0_;
	int iv0_;
	int background_;		// background pixel

	void blankrow(u_char* bp) const;
};

class VLOutboard_420 : public VLOutboard {
public:
	VLOutboard_420(VLOutputDevice& d) : VLOutboard(d, FT_YUV_420) {}
protected:
	virtual void scaleframe(u_char* bp, const u_char* frm) const;
	virtual void upscaleframe(u_char* bp, const u_char* frm) const;
	void fill_two_rows(u_char* bp, const u_char* yp,
			   const u_char* up, const u_char* vp,
			   int skip0, int skip1, int os) const;
	void fill_four_rows(u_char* bp, const u_char* yp,
			    const u_char* up, const u_char* vp,
			    int skip0, int skip1, int os) const;
};

class VLOutboard_422 : public VLOutboard {
public:
	VLOutboard_422(VLOutputDevice& d) : VLOutboard(d, FT_YUV_422) {}
protected:
	virtual void scaleframe(u_char* bp, const u_char* frm) const;
	virtual void upscaleframe(u_char* bp, const u_char* frm) const;
	void fill_row(u_char* bp, const u_char* yp,
		      const u_char* up, const u_char* vp,
		      int skip0, int skip1, int os) const;
	void fill_two_rows(u_char* bp, const u_char* yp,
			   const u_char* up, const u_char* vp,
			   int skip0, int skip1, int os) const;
};

VLOutputDevice::VLOutputDevice(const char* name, VLDev device,
			       VLNodeInfo* ports, int nport)
	: OutputDevice(name), device_(device),
	  ports_(ports), nport_(nport)
{
	/* smash "ev1" insto galileo for better familiarity */
	if (nickname_[0] == 'e' && nickname_[1] == 'v') {
		nickname_ = "galilelo";
	}
	char* cp = new char[80 + nport * (VL_NAME_SIZE + 1)];
	attributes_ = cp;
	strcpy(cp, "port { ");
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

int VLOutputDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "renderer") == 0) {
			if (strcmp(argv[2], "422") == 0) {
				VLOutboard* p = new VLOutboard_422(*this);
				tcl.result(p->name());
				return (TCL_OK);
			}
			if (strcmp(argv[2], "420") == 0) {
				VLOutboard* p = new VLOutboard_420(*this);
				tcl.result(p->name());
				return (TCL_OK);
			}
		}
	}
	return (OutputDevice::command(argc, argv));
}

int VLOutputDevice::lookup_port(const char* name) const
{
	for (int i = 0; i < nport_; ++i) {
		if (strcasecmp(name, ports_[i].name) == 0)
			return (ports_[i].number);
	}
	abort();
}

/*XXX share this class with vlgrabber module*/
VLOutputBuilder::VLOutputBuilder()
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
				new VLOutputDevice(p->name, p->dev, ni, n);
		}
	}
	vlCloseVideo(s);
}

/*
 * Lookup all video drains (i.e., output ports on the board) and smash
 * spaces in names to dashes to avoid tcl list problems.
 */
VLNodeInfo* VLOutputBuilder::findports(VLNodeInfo* node, int& n)
{
	VLNodeInfo* result = new VLNodeInfo[n];
	int k = 0;
	for (int i = 0; i < n; ++i) {
		if (node[i].type == VL_DRN && node[i].kind == VL_VIDEO) {
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

VLOutboard::VLOutboard(VLOutputDevice& device, int ft)
	: Renderer(ft), device_(device), port_(VL_ANY), 
	  vl_(0), rb_(0), path_(0),
	  outw_(0), outh_(0),
	  current_port_(-1), background_(0x80808080)
{
	startup_vl();
}

VLOutboard::~VLOutboard()
{
	shutdown_vl();
}

int VLOutboard::command(int argc, const char*const* argv)
{
		
	if (argc == 3) {
		if (strcmp(argv[1], "scale") == 0) {
			int s = atoi(argv[2]);
			if (scale_ != s) {
				scale_ = s;
				resize(width_, height_);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "port") == 0) {
			port_ = device_.lookup_port(argv[2]);
			startup_vl();
			return (TCL_OK);
		}
	}
	return (Renderer::command(argc, argv));
}

void VLOutboard::shutdown_vl()
{
	if (vl_) {
		if (path_) {
			if (rb_) {
				vlEndTransfer(vl_, path_);
				vlDeregisterBuffer(vl_, path_, src_, rb_);
				vlDestroyBuffer(vl_, rb_);
				rb_ = 0;
			}
			vlDestroyPath(vl_, path_);
			path_ = 0;
		}
		vlCloseVideo(vl_);
		vl_ = 0;
	}
}

void VLOutboard::startup_vl()
{
	if (vl_ != 0 && port_ == current_port_)
		return;

	VLControlValue val;

	current_port_ = port_;
	shutdown_vl();

	vl_ = vlOpenVideo("");
	if (vl_ == 0)
		goto failed;
	src_ = vlGetNode(vl_, VL_SRC, VL_MEM, VL_ANY);
	drn_ = vlGetNode(vl_, VL_DRN, VL_VIDEO, port_);
	path_ = vlCreatePath(vl_, device_.device(), src_, drn_);
	if (path_ < 0) {
		vlPerror("vic: create path");
		goto failed;
	}
	if (vlSetupPaths(vl_, (VLPathList)&path_, 1, VL_SHARE, VL_SHARE) < 0) {
		vlPerror("vic: set up paths");
		goto failed;
	}

	xfersize_ = vlGetTransferSize(vl_, path_);

	/* get width and height of output */
	vlGetControl(vl_, path_, drn_, VL_SIZE, &val);
	outw_ = val.xyVal.x;
	outh_ = val.xyVal.y;

	if (xfersize_ != 2 * outw_ * outh_) {
		fprintf(stderr,
	"vic: vl output transfer size and geometry are inconsistent\n");
		goto failed;
	}

	/* Create ring buffers to hold captured data of new size. */
	rb_ = vlCreateBuffer(vl_, path_, src_, 2);
	if (rb_ == 0) {
		vlPerror("vic: create Buffer");
		goto failed;
	}
	/* Tell VL to use newly created ring buffer. */
	if (vlRegisterBuffer(vl_, path_, src_, rb_) < 0) {
		vlPerror("vic: buffer registration");
		goto failed;
	}

	vlBeginTransfer(vl_, path_, 0, NULL);

	return;
 failed:
	shutdown_vl();
#ifdef notdef
	status_ = -1;
#endif
}

int VLOutboard::consume(const VideoFrame* vf)
{
	if (update_interval_ != 0) {
		if (need_update_ == 0)
			return (0);
		need_update_ = 0;
	}
	if (!samesize(vf))
		resize(vf->width_, vf->height_);
	VLInfoPtr vip;
	vip = vlGetNextFree(vl_, rb_, xfersize_);
	if (vip == 0) {
#ifdef notdef
		fprintf(stderr, "vl outboard render: no buffer!\n");
#endif
		return (0);
	}
	/* Get a pointer to where the data will go */
	u_char* bp = (u_char*)vlGetActiveRegion(vl_, rb_, vip);
	if (scale_)
		upscaleframe(bp, vf->bp_);
	else
		scaleframe(bp, vf->bp_);

	vlPutValid(vl_, rb_);

	return (0);
}

void VLOutboard::blankrow(u_char* bp) const
{
	int m = background_;
	for (int k = outw_ >> 1; --k >= 0; ) {
		*(u_int32_t*)bp = m;
		bp += 4;
	}
}

void VLOutboard_422::fill_row(u_char* bp, const u_char* yp,
			      const u_char* up, const u_char* vp,
			      int skip0, int skip1, int os) const
{
	int m = background_;
	int k;
	for (k = skip0 >> 1; --k >= 0; ) {
		*(u_int32_t*)bp = m;
		bp += 4;
	}
	/*XXX assumes even number */
	for (k = outw_ - (skip0 + skip1); k > 0; k -= 2) {
		int uv = *up++ << 24 | *vp++ << 8;
		int pixel = uv | yp[0] << 16 | yp[1];
		*(u_int32_t*)bp = pixel;
		bp += 4;
		yp += 2;
	}
	for (k = skip1 >> 1; --k >= 0; ) {
		*(u_int32_t*)bp = m;
		bp += 4;
	}
}

void VLOutboard_422::scaleframe(u_char* bp, const u_char* frm) const
{
	int off = iv0_ * width_ + ih0_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);

	/* two bytes per bixel */
	int os = outw_ << 1;
	int k;
	for (k = ov0_; --k >= 0; ) {
		blankrow(bp);
		bp += os;
	}
	for (k = outh_ - ov1_- ov0_; --k >= 0; ) {
		fill_row(bp, yp, up, vp, oh0_, oh1_, os);
		bp += os;
		yp += width_;
		up += width_ >> 1;
		vp += width_ >> 1;
	}
	for (k = ov1_; --k >= 0; ) {
		blankrow(bp);
		bp += os;
	}
}

void VLOutboard_422::fill_two_rows(u_char* bp, const u_char* yp,
				   const u_char* up, const u_char* vp,
				   int skip0, int skip1, int os) const
{
	int m = background_;
	int k;
	for (k = skip0 >> 1; --k >= 0; ) {
		*(u_int32_t*)bp = m;
		*(u_int32_t*)(bp + os) = m;
		bp += 4;
	}
	/*XXX assumes width multiple of 4 */
	for (k = (outw_ - (skip0 + skip1)) >> 2; --k >= 0; ) {
		int uv = *up++ << 24 | *vp++ << 8;
		int pixel = yp[0];
		pixel |= uv | (pixel << 16);
		*(u_int32_t*)bp = pixel;
		*(u_int32_t*)(bp + os) = pixel;
		bp += 4;
		yp += 1;

		pixel = yp[0];
		pixel |= uv | (pixel << 16);
		*(u_int32_t*)bp = pixel;
		*(u_int32_t*)(bp + os) = pixel;
		bp += 4;
		yp += 1;
	}
	for (k = skip1 >> 1; --k >= 0; ) {
		*(u_int32_t*)bp = m;
		*(u_int32_t*)(bp + os) = m;
		bp += 4;
	}
}

void VLOutboard_422::upscaleframe(u_char* bp, const u_char* frm) const
{
	int off = iv0_ * width_ + ih0_;
	register const u_char* yp = frm + off;
	register const u_char* up = frm + framesize_ + (off >> 1);
	register const u_char* vp = up + (framesize_ >> 1);

	/* two bytes per bixel */
	int os = outw_ << 1;
	int k;
	for (k = ov0_; --k >= 0; ) {
		blankrow(bp);
		bp += os;
	}
	/*XXX assumes multiple of 2 */
	for (k = outh_ - ov1_- ov0_; k > 0; k -= 2) {
		fill_two_rows(bp, yp, up, vp, oh0_, oh1_, os);
		bp += os << 1;
		yp += width_;
		up += width_ >> 1;
		vp += width_ >> 1;
	}
	for (k = ov1_; --k >= 0; ) {
		blankrow(bp);
		bp += os;
	}
}

void VLOutboard_420::fill_two_rows(u_char* bp, const u_char* yp,
				   const u_char* up, const u_char* vp,
				   int skip0, int skip1, int os) const
{
	int m = background_;
	int k;
	for (k = skip0 >> 1; --k >= 0; ) {
		*(u_int32_t*)bp = m;
		*(u_int32_t*)(bp + os) = m;
		bp += 4;
	}
	/*XXX assumes even number */
	k = outw_ - (skip0 + skip1);
	for (k >>= 1; --k >= 0; ) {
		int uv = *up++ << 24 | *vp++ << 8;
		int pixel = uv | yp[0] << 16 | yp[1];
		*(u_int32_t*)bp = pixel;
		pixel = uv | yp[width_] << 16 | yp[width_ + 1];
		*(u_int32_t*)(bp + os) = pixel;
		bp += 4;
		yp += 2;
	}
	for (k = skip1 >> 1; --k >= 0; ) {
		*(u_int32_t*)bp = m;
		*(u_int32_t*)(bp + os) = m;
		bp += 4;
	}
}

void VLOutboard_420::scaleframe(u_char* bp, const u_char* frm) const
{
	int voff = iv0_ * width_;
	register const u_char* yp = frm + ih0_ + voff;
	register const u_char* up = frm + framesize_ + (ih0_ >> 1) + (voff >> 2);
	register const u_char* vp = up + (framesize_ >> 2);

	/* two bytes per bixel */
	int os = outw_ << 1;
	int k;
	for (k = ov0_; --k >= 0; ) {
		blankrow(bp);
		bp += os;
	}
	for (k = outh_ - ov1_- ov0_; k > 0; k -= 2) {
		fill_two_rows(bp, yp, up, vp, oh0_, oh1_, os);
		bp += os << 1;
		yp += width_ << 1;
		up += width_ >> 1;
		vp += width_ >> 1;
	}
	for (k = ov1_; --k >= 0; ) {
		blankrow(bp);
		bp += os;
	}
}

void VLOutboard_420::fill_four_rows(u_char* bp, const u_char* yp,
				    const u_char* up, const u_char* vp,
				    int skip0, int skip1, int os) const
{
	int m = background_;
	int k;
	int os3 = os + (os << 1);

	for (k = skip0 >> 1; --k >= 0; ) {
		*(u_int32_t*)bp = m;
		*(u_int32_t*)(bp + os) = m;
		*(u_int32_t*)(bp + (os << 1)) = m;
		*(u_int32_t*)(bp + os3) = m;
		bp += 4;
	}
	/*XXX assumes width multiple of 4 */
	for (k = (outw_ - (skip0 + skip1)) >> 2; --k >= 0; ) {
		int uv = *up++ << 24 | *vp++ << 8;
		int pixel = yp[0];
		pixel |= uv | (pixel << 16);
		*(u_int32_t*)bp = pixel;
		*(u_int32_t*)(bp + os) = pixel;
		pixel = yp[width_];
		pixel |= uv | (pixel << 16);
		*(u_int32_t*)(bp + (os << 1)) = pixel;
		*(u_int32_t*)(bp + os3) = pixel;
		bp += 4;
		yp += 1;

		pixel = yp[0];
		pixel |= uv | (pixel << 16);
		*(u_int32_t*)bp = pixel;
		*(u_int32_t*)(bp + os) = pixel;
		pixel = yp[width_];
		pixel |= uv | (pixel << 16);
		*(u_int32_t*)(bp + (os << 1)) = pixel;
		*(u_int32_t*)(bp + os3) = pixel;
		bp += 4;
		yp += 1;
	}
	for (k = skip1 >> 1; --k >= 0; ) {
		*(u_int32_t*)bp = m;
		*(u_int32_t*)(bp + os) = m;
		*(u_int32_t*)(bp + (os << 1)) = m;
		*(u_int32_t*)(bp + os3) = m;
		bp += 4;
	}
}

void VLOutboard_420::upscaleframe(u_char* bp, const u_char* frm) const
{
	int voff = iv0_ * width_;
	register const u_char* yp = frm + ih0_ + voff;
	register const u_char* up = frm + framesize_ + (ih0_ >> 1) + (voff >> 2);
	register const u_char* vp = up + (framesize_ >> 2);

	/* two bytes per bixel */
	int os = outw_ << 1;
	int k;
	for (k = ov0_; --k >= 0; ) {
		blankrow(bp);
		bp += os;
	}
	/*XXX assumes multiple of 4 */
	for (k = outh_ - ov1_- ov0_; k > 0; k -= 4) {
		fill_four_rows(bp, yp, up, vp, oh0_, oh1_, os);
		bp += os << 2;
		yp += width_ << 1;
		up += width_ >> 1;
		vp += width_ >> 1;
	}
	for (k = ov1_; --k >= 0; ) {
		blankrow(bp);
		bp += os;
	}
}

void VLOutboard::resize(int w, int h)
{
	width_ = w;
	height_ = h;
	framesize_ = w * h;

	/*
	 * Special case H.261 scaled up.  We need to crop away some edge,
	 * which we do to make the vic-style NTSC centered in CIF image
	 * look okay.
	 */
	if (width_ << scale_ == 352 << 1 &&
	    height_ << scale_ == 288 << 1) {
		oh0_ = 0;
		oh1_ = 0;
		ov0_ = 0;
		ov1_ = 0;
		ih0_ = 16;
		iv0_ = 16;
	} else {
		int d = outw_ - (width_ << scale_);
		if (d >= 0) {
			oh0_ = (d >> 1) &~ 1;
			oh1_ = d - oh0_;
			ih0_ = 0;
		} else {
			d = -d;
			ih0_ = (d >> 1) &~ 1;
			oh0_ = 0;
			oh1_ = 0;
		}
		d = outh_ - (height_ << scale_);
		if (d >= 0) {
			ov0_ = (d >> 1) &~ 1;
			ov1_ = d - ov0_;
			iv0_ = 0;
		} else {
			d = -d;
			iv0_ = (d >> 1) &~ 1;
			ov0_ = 0;
			ov1_ = 0;
		}
	}
	/*
	 * make sure rendered number of lines is a multiple of 4
	 * by increasing the number of blanked lines at the bottom
	 * if necessary.
	 */
	int nline = outh_ - (ov0_ + ov1_);
	ov1_ += nline & 3;

#ifdef notdef
	printf("inw %d inh %d outw %d outh %d\n", 
	       width_, height_, outw_, outh_);
	printf("ov0 %d ov1 %d iv0 %d iv1 %d\n",
	       ov0_,ov1_,iv0_,iv1_);
	printf("oh0 %d oh1 %d ih0 %d ih1 %d\n",
	       oh0_,oh1_,ih0_,ih1_);
#endif
}
