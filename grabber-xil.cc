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
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "module.h"
#include "Tcl.h"

#include <tk.h>
#include <sys/param.h>
#include <xil/xil.h>
#include "grabber.h"
#include "crdef.h"
#include "iohandler.h"
#include "device-input.h"

class XILGrabber : public Grabber, public IOHandler {
 protected:
	XILGrabber();
 public:
	virtual ~XILGrabber();
	virtual void start();
	virtual void stop();
	virtual int command(int argc, const char*const* argv);
	virtual void fps(int);
	inline int is_pal() { return (max_fps_ == 25); }
 protected:
	void dispatch(int mask);
	int attr(const char* name, int value);
	int attr(const char* name);
	const char* cattr(const char* name);
	virtual void setsize() {}
	void suppress(const u_char* devbuf, int istride);

	int port_;		/* XIL input port */
	XilSystemState xil_;
	XilImage source_;
	XilImage image_;
	double scale_;
	double nextframetime_;
	int fd_;		/* rtvc data fd */
	u_int decimate_;
	u_int basewidth_;
	u_int baseheight_;
	u_int max_fps_;		/* 25 (PAL) or 30 (NTSC) */
};

class XILYuvGrabber : public XILGrabber {
 public:
	XILYuvGrabber();
	virtual ~XILYuvGrabber();
 protected:
	virtual int grab();
	virtual void setsize();
	void saveblk(const u_char* in, u_char* yp, u_char* up, u_char* vp,
		     int stride, int istride);
	void saveblks(const u_char* in, int istride);
};


class XILCIFGrabber : public XILYuvGrabber {
 public:
	XILCIFGrabber();
 protected:
	virtual int grab();
	virtual void setsize();
	void saveblk(const u_char* in, u_char* yp, u_char* up, u_char* vp,
		     int stride, int istride);
	void saveblks(const u_char* in, int istride);
};

#ifdef notdef
class XIL411Grabber : public XILCIFGrabber {
    public:
	XIL411Grabber();
    protected:
	virtual void setsize(int xsize, int ysize);
};
#endif

class XILCodecGrabber : public XILGrabber {
 public:
	XILCodecGrabber(const char* type);
	virtual ~XILCodecGrabber();
 protected:
	int cattr(const char* name, int value);
	u_char* capture(int& length);
	XilCis cis_;
};

class XILJpegGrabber : public XILCodecGrabber {
 public:
	XILJpegGrabber();
 protected:
	virtual int command(int argc, const char*const* argv);
	void setq(int q);
	int grab();
	int q_;
};

#ifdef notdef
class XILp64Grabber : public XILCodecGrabber {
 public:
	XILp64Grabber();
 protected:
	int grab();
};
#endif

class XILDevice : public InputDevice {
 public:
	XILDevice(const char* s);
	virtual int command(int argc, const char*const* argv);
};

static XILDevice xil_device("xil");

/*
 * XXX gcc 2.5.8 needs this useless stub.
 */
XILDevice::XILDevice(const char* s) : InputDevice(s)
{
	/*XXX ports should be queried from xil */
	attributes_ = "\
format { 411 422 jpeg } \
size { small large cif } \
port { Composite-1 Composite-2 S-Video }";
}

int XILDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			TclObject* o = 0;
			if (strcmp(argv[2], "422") == 0)
				o = new XILYuvGrabber;
			else if (strcmp(argv[2], "cif") == 0)
				o = new XILCIFGrabber;
			else if (strcmp(argv[2], "jpeg") == 0)
				o = new XILJpegGrabber;
			if (o != 0)
				tcl.result(o->name());
			return (TCL_OK);
		}
	}
	return (InputDevice::command(argc, argv));
}

XILGrabber::XILGrabber()
{
	xil_ = xil_open();
	if (xil_ == 0) {
		status_ = -1;
		return;
	}
	source_ = xil_create_from_device(xil_, "SUNWrtvc", 0);
	if (source_ == 0) {
		xil_close(xil_);
		status_ = -1;
		return;
	}

	attr("PORT_V", 1);
	attr("IMAGE_SKIP", 1);
	attr("MAX_BUFFERS", 1);

	XilDataType datatype;
	u_int nbands;
	xil_get_info(source_, &basewidth_, &baseheight_, &nbands, &datatype);
	image_ = xil_create(xil_, basewidth_ >> 1, baseheight_ >> 1,
			    nbands, datatype);
	decimate_ = 2;
	scale_ = .5;
	frame_ = 0;

	fd_ = attr("FILE_DESCRIPTOR");
	max_fps_ = attr("FORMAT_V") == 1? 25 : 30;
}

XILGrabber::~XILGrabber()
{
	if (status_ >= 0)
		xil_close(xil_);
}

int XILGrabber::attr(const char* name, int value)
{
	int s = xil_set_device_attribute(source_, (char*)name, (void*)value);
	if (s != XIL_SUCCESS) {
		fprintf(stderr, "vic: can't set XIL attribute: %s\n", name);
		return (-1);
	}
	return (0);
}

int XILGrabber::attr(const char* name)
{
	int value;
	int s = xil_get_device_attribute(source_, (char*)name, (void**)&value);
	if (s != XIL_SUCCESS) {
		fprintf(stderr, "vic: can't set XIL attribute: %s\n", name);
		return (-1);
	}
	return (value);
}

const char* XILGrabber::cattr(const char* name)
{
	const char* value;
	int s = xil_get_device_attribute(source_, (char*)name, (void**)&value);
	if (s != XIL_SUCCESS) {
		fprintf(stderr, "vic: can't set XIL attribute: %s\n", name);
		return (0);
	}
	return (value);
}

int XILGrabber::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "port") == 0) {
			int newport;
			if (strcasecmp(argv[2], "composite-1") == 0)
				newport = 1;
			else if (strcasecmp(argv[2], "composite-2") == 0)
				newport = 2;
			else 
				newport = 0;

			attr("PORT_V", newport);
			/* video format may have changed when port changed */
			XilDataType datatype;
			u_int nbands;
			xil_get_info(source_, &basewidth_, &baseheight_,
				     &nbands, &datatype);
			setsize();
			/* need to kick-start frame grabbing process */
			if (running_) {
				stop();
				start();
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "decimate") == 0) {
			int d = atoi(argv[2]);
			if (d <= 0) {
				Tcl& tcl = Tcl::instance();
				tcl.resultf("%s: divide by zero", argv[0]);
				return (TCL_ERROR);
			}
			scale_ = 1. / double(d);
			decimate_ = d;
			setsize();
			return (TCL_OK);
		}
	}
	return (Grabber::command(argc, argv));
}

void XILGrabber::fps(int f)
{
	if (f <= 0)
		f = 1;
	else if (f > max_fps_)
		f = max_fps_;
	/* convert to skip count then back */
	int sc = max_fps_ / f;
	attr("IMAGE_SKIP", sc - 1);
	Grabber::fps(max_fps_ / sc);
}

void XILGrabber::start()
{
	link(fd_, TK_READABLE);
	double now = gettimeofday();
	frameclock_ = now;
	nextframetime_ = now + tick(grab());
}

void XILGrabber::stop()
{
	unlink();
}

void XILGrabber::dispatch(int mask)
{
	double now = gettimeofday();
	if (nextframetime_ > now) {
		/*
		 * the frame is too early & we want to flush it.
		 * unfortunately, the sunvideo driver doesn't provide
		 * a hook for flushing a frame.  So, we have to do
		 * a 2MB read to get rid of the sucker.
		 */
		char* buf = new char[768 * 576 * 3];
		(void)pread(fd_, (void*)buf, sizeof(buf), (off_t)0x01000000);
		delete buf;
	} else
		nextframetime_ = tick(grab()) + now;
}

/*
 * define these for REPLENISH macro used below
 */
#define DIFF4(in, frm, v) \
	v += (in)[0] - (frm)[0]; \
	v += (in)[3] - (frm)[1]; \
	v += (in)[6] - (frm)[2]; \
	v += (in)[9] - (frm)[3];

#define DIFFLINE(in, frm, left, center, right) \
	DIFF4(in, frm, left); \
	DIFF4(in + 1*12, frm + 1*4, center); \
	DIFF4(in + 2*12, frm + 2*4, center); \
	DIFF4(in + 3*12, frm + 3*4, right); \
	if (right < 0) \
		right = -right; \
	if (left < 0) \
		left = -left; \
	if (center < 0) \
		center = -center;

void XILGrabber::suppress(const u_char* devbuf, int is)
{
	const u_char* start = frame_ + 16 * vstart_ * outw_ + 16 * hstart_;
	REPLENISH(devbuf, start, is, 3,
		  hstart_, hstop_, vstart_, vstop_);
}

XILYuvGrabber::XILYuvGrabber()
{
	setsize();
}

XILYuvGrabber::~XILYuvGrabber()
{
	xil_destroy(image_);
	delete frame_;
}

void XILYuvGrabber::setsize()
{
	set_size_422(basewidth_ / decimate_, baseheight_ / decimate_);
}

inline void 
XILYuvGrabber::saveblk(const u_char* in,
	u_char* yp, u_char* up, u_char* vp, int stride, int istride)
{
	for (int i = 16; --i >= 0; ) {
		/*
		 * Each iteration of this loop grabs 16 Ys & 8 U/Vs.
		 */
		register u_int y0, y1, u, v;

		u  = in[1]  << 24 | in[7]  << 16 | in[13] << 8 | in[19];
		v  = in[2]  << 24 | in[8] << 16  | in[14] << 8 | in[20];
		y0 = in[0]  << 24 | in[3]  << 16 | in[6]  << 8 | in[9];
		y1 = in[12] << 24 | in[15] << 16 | in[18] << 8 | in[21];

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;
		*(u_int*)up = u;
		*(u_int*)vp = v;

		u  = in[24+1]  << 24 | in[24+7]  << 16 | in[24+13] << 8 | in[24+19];
		v  = in[24+2]  << 24 | in[24+8] << 16  | in[24+14] << 8 | in[24+20];
		y0 = in[24+0]  << 24 | in[24+3]  << 16 | in[24+6]  << 8 | in[24+9];
		y1 = in[24+12] << 24 | in[24+15] << 16 | in[24+18] << 8 | in[24+21];

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;
		((u_int*)up)[1] = u;
		((u_int*)vp)[1] = v;

		in += istride;
		yp += stride;
		up += stride >> 1;
		vp += stride >> 1;
	}
}

void XILYuvGrabber::saveblks(const u_char* in, int is)
{
	u_char* crv = crvec_;
	int off = framesize_;
	u_char* lum = frame_;
	u_char* chm = lum + off;
	off >>= 1;
	int stride = 15 * outw_;
	int istride = is * 15;
	for (int y = 0; y < blkh_; ++y) {
		for (int x = 0; x < blkw_; ++x) {
			int s = *crv++;
			if ((s & CR_SEND) != 0)
				saveblk(in, lum, chm, chm + off, outw_, is);

			in += 48;
			lum += 16;
			chm += 8;
		}
		lum += stride;
		chm += stride >> 1;
		in += istride;
	}
}

int XILYuvGrabber::grab()
{
	/*XXX*/
	xil_scale(source_, image_, "nearest", scale_, scale_);
	if (xil_export(image_) != XIL_SUCCESS) {
		fprintf(stderr, "vic: xil_export failed\n");
		abort();
	}
	XilMemoryStorage layout;
	if (xil_get_memory_storage(image_, &layout) == 0) {
		fprintf(stderr, "vic: xil_get_memory_storage failed\n");
		abort();
	}
	int istride = layout.byte.scanline_stride;
	if (layout.byte.pixel_stride != 3) {
		fprintf(stderr, "vic: xil: bad pixel stride\n");
		abort();
	}
	register u_char* data = layout.byte.data;
	suppress(data, istride);
	saveblks(data, istride);
	xil_import(image_, 0);
	xil_toss(image_);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

XILCIFGrabber::XILCIFGrabber()
{
	setsize();
}

void XILCIFGrabber::setsize()
{
	set_size_cif(basewidth_ / decimate_, baseheight_ / decimate_);
}

/* 411 */
inline void 
XILCIFGrabber::saveblk(const u_char* in,
	u_char* yp, u_char* up, u_char* vp, int stride, int istride)
{
	for (int i = 8; --i >= 0; ) {
		/*
		 * Each iteration of this loop grabs 16 Ys & 8 U/Vs.
		 */
		register u_int y0, y1, u, v;

		u  = in[1]  << 24 | in[7]  << 16 | in[13] << 8 | in[19];
		v  = in[2]  << 24 | in[8] << 16  | in[14] << 8 | in[20];
		y0 = in[0]  << 24 | in[3]  << 16 | in[6]  << 8 | in[9];
		y1 = in[12] << 24 | in[15] << 16 | in[18] << 8 | in[21];

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;
		*(u_int*)up = u;
		*(u_int*)vp = v;

		u  = in[24+1]  << 24 | in[24+7]  << 16 | in[24+13] << 8 | in[24+19];
		v  = in[24+2]  << 24 | in[24+8] << 16  | in[24+14] << 8 | in[24+20];
		y0 = in[24+0]  << 24 | in[24+3]  << 16 | in[24+6]  << 8 | in[24+9];
		y1 = in[24+12] << 24 | in[24+15] << 16 | in[24+18] << 8 | in[24+21];

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;
		((u_int*)up)[1] = u;
		((u_int*)vp)[1] = v;

		in += istride;
		yp += stride;
		up += stride >> 1;
		vp += stride >> 1;

		/* do the 2nd (y only instead of yuv) line */
		y0 = in[0]  << 24 | in[3]  << 16 | in[6]  << 8 | in[9];
		y1 = in[12] << 24 | in[15] << 16 | in[18] << 8 | in[21];

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;

		y0 = in[24+0]  << 24 | in[24+3]  << 16 | in[24+6]  << 8 | in[24+9];
		y1 = in[24+12] << 24 | in[24+15] << 16 | in[24+18] << 8 | in[24+21];

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;

		in += istride;
		yp += stride;
	}
}

void XILCIFGrabber::saveblks(const u_char* in, int is)
{
	u_char* crv = crvec_;
	int off = framesize_;
	u_char* lum = frame_;
	u_char* chm = lum + off;
	off >>= 2;

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

			in += 48;
			lum += 16;
			chm += 8;
		}
		crv += skip;
		lum += 15 * outw_ + skip * 16;
		chm += 7 * (outw_ >> 1) + skip * 8;
		in = nin + 16 * is;
	}
}

int XILCIFGrabber::grab()
{
	/*
	 * This is similar to XILGrabber:grab() except it converts
	 * an PAL image to CIF dimensions (by dropping the last
	 * 32 pixels of each line) and it decimates the
	 * uv info by 2 vertically to convert 4:2:2 to 4:1:1.
	 */
	xil_scale(source_, image_, "nearest", scale_, scale_);
	if (xil_export(image_) != XIL_SUCCESS) {
		fprintf(stderr, "vic: xil_export failed\n");
		abort();
	}
	XilMemoryStorage layout;
	if (xil_get_memory_storage(image_, &layout) == 0) {
		fprintf(stderr, "vic: xil_get_memory_storage failed\n");
		abort();
	}
	register u_int istride = layout.byte.scanline_stride;
	register u_char* p = layout.byte.data;
	suppress(p, istride);
	saveblks(p, istride);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

XILCodecGrabber::XILCodecGrabber(const char* type)
{
	cis_ = xil_cis_create(xil_, (char*)type);
	xil_cis_set_keep_frames(cis_, 1);
	xil_cis_set_max_frames(cis_, 1);
}

XILCodecGrabber::~XILCodecGrabber()
{
	xil_destroy(image_);
	xil_cis_destroy(cis_);
}

int XILCodecGrabber::cattr(const char* name, int value)
{
	(void)xil_cis_set_attribute(cis_, (char*)name, (void*)value);
	return (0);
}

u_char* XILCodecGrabber::capture(int& length)
{
	xil_scale(source_, image_, "nearest", scale_, scale_);
	xil_compress(image_, cis_);
	xil_cis_sync(cis_);
	xil_toss(image_);
	if (!xil_cis_has_frame(cis_))
		return (0);

	int cc, nf;
	u_char* p = (u_char*)xil_cis_get_bits_ptr(cis_, &cc, &nf);
	length = cc;
	return (p);
}

XILJpegGrabber::XILJpegGrabber()
	: XILCodecGrabber("Jpeg"), q_(50)
{
	cattr("ENCODE_411_INTERLEAVED", 1);
}

int XILJpegGrabber::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "q") == 0) {
			int q = atoi(argv[2]);
			setq(q);
			return (TCL_OK);
		}
	} else if (argc == 2 && strcmp(argv[1], "decimate") == 0) {
		// silently ignore decimate command
		return (TCL_OK);
	}
	return (XILGrabber::command(argc, argv));
}

extern void jpeg_chroma_qt(int q, int* qt);
extern void jpeg_luma_qt(int q, int* qt);

void XILJpegGrabber::setq(int q)
{
	/*
	 * NB - if any entry of any quantization table is < 8, XIL
	 * will refuse to use the CL4000 for compression & instead
	 * do a raw capture followed by a software jpeg conversion.
	 * This will flush performance straight down the toilet.
	 */
	q_ = q;
	int qt[8][8];
	XilJpegQTable xq;
	jpeg_luma_qt(q, (int*)qt);
	xq.table = 0;
	xq.value = qt;
	(void)xil_cis_set_attribute(cis_, "QUANTIZATION_TABLE", (void*)&xq);
	jpeg_chroma_qt(q, (int*)qt);
	xq.table = 1;
	xq.value = qt;
	(void)xil_cis_set_attribute(cis_, "QUANTIZATION_TABLE", (void*)&xq);
}

int XILJpegGrabber::grab()
{
	/*XXX can get timestamp from xil */
	int cc;
	u_char* p = capture(cc);
	if (p == 0)
		return (0);
	/* get rid of the jfif header that xil prepends to each frame */
	u_char* ep;
	for (ep = p + cc; p < ep; ++p) {
		if (*p == 0xff) {
			++p;
			if  (*p == 0xda) {
				/* found start-of-scan marker */
				++p;
				if (p + 2 <= ep) {
					/* skip over SOS */
					u_int t = (p[0] << 8) | p[1];
					p += t;
					break;
				}
			}
		}
	}
	if (p >= ep)
		return (0);

	JpegFrame f(media_ts(), p, ep - p, q_, 1,
		    basewidth_ / decimate_, baseheight_ / decimate_);
	return (target_->consume(&f));
}

#ifdef notdef
XILp64Grabber::XILp64Grabber()
	: XILCodecGrabber("H261")
{
}

int XILp64Grabber::grab()
{
	/*XXX can get timestamp from xil */
	int cc;
	u_char* p = capture(cc);
	return (framer_->send(p, cc, media_ts()));
}
#endif
