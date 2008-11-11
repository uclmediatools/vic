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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "jvs.h"

#include "inet.h"
#include "vic_tcl.h"
#include "rtp.h"
#include "grabber.h"
#include "iohandler.h"
#include "crdef.h"
#include "device-input.h"
#include "module.h"

extern "C" {
#include <sys/ipc.h>
#include <sys/shm.h>
#ifdef mips
#include <mips/cachectl.h>
int cacheflush(char* addr, int len, int cache);
#endif
}

class JvGrabber : public Grabber, public IOHandler {
protected:
	JvGrabber();
public:
	virtual ~JvGrabber();
	virtual void start();
	virtual void stop();
	virtual int command(int argc, const char*const* argv);
	/*
	 * This structure should be private,
	 * but DEC's C++ compiler cannot hack it.
	 */
	struct dmabuf {
		int shmid;
		u_char* bp;
	};
protected:
	virtual void SetComp() = 0;
	virtual void dispatch(int mask);
	virtual void timeout();
	virtual int consume(const dmabuf&, int length) = 0;

	void kick();		/* ask jvdriver to compress next frame */
	void sync();		/* synchronize with rpc compression reply */

	int server_;		/* fd of socket to jvdriver */
	dmabuf* allocdma(int type);
	void freedma(dmabuf*);
	dmabuf* cb0_;		/* capture buffer #1 */
	dmabuf* cb1_;		/* capture buffer #2 */
	dmabuf* cb_;		/* double-buffer capture pointer */
	int x_decimate_;	/* horizontal downsampling factor */
	int y_decimate_;	/* vertical downsampling factor */
	int grabbing_;		/* true when grab request pending */

	int j300_;
};

class JvJpegGrabber : public JvGrabber {
public:
	JvJpegGrabber();
	virtual ~JvJpegGrabber();
	virtual int command(int argc, const char*const* argv);
protected:
	int consume(const dmabuf&, int length);
	virtual void SetComp();
private:
	int q_;
};

class JvYuvGrabber : public JvGrabber {
public:
	JvYuvGrabber();
	virtual void SetComp();
	virtual int consume(const dmabuf&, int length);
protected:
	void suppress(const u_char* devbuf, const u_char* lumbuf);
	virtual void saveblks(const u_char* in) = 0;
	virtual void setsize(int w, int h) = 0;
};

class Jv422Grabber : public JvYuvGrabber {
protected:
	virtual void saveblks(const u_char* in);
	void saveblk(const u_char* in, u_char* yp, u_char* up,
		     u_char* vp, int stride);
	void setsize(int w, int h);
};

class JvCIFGrabber : public JvYuvGrabber {
protected:
	virtual void saveblks(const u_char* in);
	void saveblk(const u_char* in, u_char* yp, u_char* up,
		     u_char* vp, int stride, int istride);
	void setsize(int w, int h);
};

class Jv420Grabber : public JvCIFGrabber {
    protected:
	virtual void setsize(int xsize, int ysize);
};

class JvDevice : public InputDevice {
 public:
	JvDevice(const char* s);
	virtual int command(int argc, const char*const* argv);
	void j300(int v);
};

static JvDevice jv_device("J300");

JvDevice::JvDevice(const char* s) : InputDevice(s)
{
	/* start out assuming j300 */
	j300(1);
}

void JvDevice::j300(int v)
{
	attributes_ = (v == 0) ? "format { jpeg } size { small large }" :
		"format { 420 422 jpeg } size { small large cif }";
}

int JvDevice::command(int argc, const char*const* argv)
{
	if (argc == 3 && strcmp(argv[1], "open") == 0) {
		const char* fmt = argv[2];
		TclObject* o = 0;
		if (strcmp(fmt, "jpeg") == 0)
			o = new JvJpegGrabber;
		else if (strcmp(fmt, "422") == 0)
			o = new Jv422Grabber;
		else if (strcmp(fmt, "420") == 0)
			o = new Jv420Grabber;
		else if (strcmp(fmt, "cif") == 0)
			o = new JvCIFGrabber;
		if (o != 0)
			Tcl::instance().result(o->name());
		return (TCL_OK);
	}
	return (InputDevice::command(argc, argv));
}

JvGrabber::JvGrabber()
{
	char *ports = getenv("JVIN_PORT");
	int port = 0;
	if (ports)
		sscanf(ports, ":%d", &port);
	port += JVS_SOCKET;
	server_ = JvsOpen("", port);
	if (server_ < 0) {
		status_ = -1;
		if (ports != 0)
			fprintf(stderr,
		"vic: couldn't connect to jvdriver with JVIN_PORT=%s\n",
				ports);
		return;
	}
	grabbing_ = 0;
	running_ = 0;
	x_decimate_ = 2;
	y_decimate_ = 2;

	char id[JVS_IDENTITY_STRING];
	id[0] = 0;
	JvsIdentity(server_, id);
	j300_ = strcmp(id, "J300") == 0;
	jv_device.j300(j300_);
}

JvGrabber::~JvGrabber()
{
	if (server_ >= 0) {
		if (running_)
			stop();
		freedma(cb0_);
		freedma(cb1_);
		close(server_);
	}
}

JvGrabber::dmabuf* JvGrabber::allocdma(int type)
{
	dmabuf* p = new dmabuf;
	int w = 640;
	int h = 480;
	if (JvsAllocateBuf(server_, JVS_INPUT, type, w, h, &p->shmid) < 0) {
		fprintf(stderr, "vic: JvsAllocateBuf failed\n");
		exit(1);
	}
	p->bp = (u_char *)shmat(p->shmid, 0, 0);
	if (p->bp == (u_char*)-1) {
		perror("vic: shmat");
		exit(1);
	}
	/*
	 * remove the shared memory segment now -- right after
	 * it has been allocated.  the idea is that we'd like
	 * the shared memory segments to go away if we exit
	 * abonormally, and since they will still be attached to
	 * our address space the kernel won't deallocate it
	 * until we exit.  the ultrix man page doesn't seems
	 * to indicate that this is necessarily the case, but
	 * a quick glance at the kernel code for the smctl()
	 * system call verifies it.
	 */
	(void)shmctl(p->shmid, IPC_RMID, 0);

	return (p);
}

void JvGrabber::freedma(JvGrabber::dmabuf* p)
{
	(void)JvsDeallocBuf(server_, p->shmid);
	if (shmdt((char*)p->bp) < 0) {
		perror("vic: shmdt");
		exit(1);
	}
	delete[] p; //SV-XXX: Debian
}

void JvGrabber::start()
{
	link(server_, TK_READABLE);
	frameclock_ = gettimeofday_usecs();
	kick();
}

void JvGrabber::stop()
{
	unlink();
	if (grabbing_)
		sync();
}

void JvGrabber::sync()
{
	/*XXX wait for last completion so nothing is left
	  in the socket buffer */
	int length, shmid;
	(void)JvsWaitComp(server_, &shmid, &length);
	grabbing_ = 0;
}

void JvGrabber::kick()
{
	cb_ = (cb_ == cb0_) ? cb1_ : cb0_;
	if (JvsStartComp(server_, cb_->shmid) < 0) {
		fprintf(stderr, "vic: JvsStartComp failed\n");
		return;
	}
	grabbing_ = 1;
}

void JvGrabber::timeout()
{
	/*
	 * Check state.  We might have stopped since
	 * the timeout was scheduled.
	 */
	if (!grabbing_ && running_)
		kick();
}

/*
 * Called by IOHandler when rpc reply arrives
 * on server socket.
 */
void JvGrabber::dispatch(int mask)
{
	int length;
	int shmid;
	/*
	 * Since we have input waiting on this socket, this
	 * rpc should return immediately.
	 */
	if (JvsWaitComp(server_, &shmid, &length) < 0) {
		fprintf(stderr, "vic: JvsWaitComp failed\n");
		/*
		 * (maybe) the compression timed out.
		 * The hardware can be flaky.
		 * Try again.
		 */
		kick();
		return;
	}
	grabbing_ = 0;
	dmabuf* p = cb_;
#ifdef mips
	/*XXX I think jvdriver does this for us.  But is the
	 cache physical or virtual? */
	if (cacheflush((char*)p->bp, length, DCACHE) < 0) {
		perror("cacheflush");
		exit(1);
	}
#endif
	int n = consume(*p, length);
	double t = tick(n);
	/*
	 * If we're running at the max-frame rate, fire off
	 * the next compression instead of setting a timer.
	 * It would be an optimization to kick off the next
	 * capture before calling consume() above, but on a
	 * uniprocessor we're likely to run to completion
	 * before the video server runs anyway.  So kicking
	 * after consume() is in practice just as efficient.
	 */
	if (t < 34000.)
		kick();
	else
		usched(t);
}

int JvGrabber::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3 && strcmp(argv[1], "decimate") == 0) {
		int d = atoi(argv[2]);
		/*
		 * XXX shouldn't happen but be paranoid because
		 * jvdriver will crash
		 */
		if (d == 0) {
			tcl.resultf("%s: divide by zero", argv[0], 0);
			return (TCL_ERROR);
		}
		x_decimate_ = d;
		y_decimate_ = d;
		SetComp();
		return (TCL_OK);
	}
	return (Grabber::command(argc, argv));
}

JvYuvGrabber::JvYuvGrabber()
{
	if (status_ < 0)
		return;
	if (!j300_) {
		close(server_);
		server_ = -1;
		status_ = -2;
		return;
	}
	cb0_ = allocdma(JVS_YUV);
	cb1_ = allocdma(JVS_YUV);
	cb_ = cb0_;
}

int JvYuvGrabber::consume(const JvGrabber::dmabuf& db, int length)
{
	suppress(db.bp, frame_);
	saveblks(db.bp);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

/*
 * define these for REPLENISH macro used below
 */
#define DIFF4(in, frm, v) \
	v += (in)[0] - (frm)[0]; \
	v += (in)[2] - (frm)[1]; \
	v += (in)[4] - (frm)[2]; \
	v += (in)[6] - (frm)[3];

#define DIFFLINE(in, frm, left, center, right) \
	DIFF4(in, frm, left); \
	DIFF4(in + 1*8, frm + 1*4, center); \
	DIFF4(in + 2*8, frm + 2*4, center); \
	DIFF4(in + 3*8, frm + 3*4, right); \
	if (right < 0) \
		right = -right; \
	if (left < 0) \
		left = -left; \
	if (center < 0) \
		center = -center;

void JvYuvGrabber::suppress(const u_char* devbuf, const u_char* lumbuf)
{
	const u_char* start = frame_ + 16 * vstart_ * outw_ + 16 * hstart_;
	REPLENISH(devbuf, start, inw_ << 1, 2,
		  hstart_, hstop_, vstart_, vstop_);
}

void JvYuvGrabber::SetComp()
{
	int grabbing = grabbing_;
	if (grabbing)
		sync();

	JvsSetCompressReq r;
	r.requestCode = JVS_SETCOMPRESS;
	r.qfactor = 32; /* n/a since non-jpeg */
	r.xdec = x_decimate_;
	r.ydec = y_decimate_;
	r.frameskip = 1;
	r.type = JVS_YUV;
	JvsSetCompressRep rep;
	if (JvsSetCompRaw(server_, &r, &rep) < 0) {
		fprintf(stderr, "vic: JvsSetCompRaw failed\n");
		exit(1);
	}
	int w = rep.width;
	int h = rep.height;
	/*XXX*/
	if (x_decimate_ == 1 && y_decimate_ == 1 && 3 * w != 4 * h) {
		r.xdec = x_decimate_ = 2;
		r.ydec = y_decimate_ = 2;
		if (JvsSetCompRaw(server_, &r, &rep) < 0) {
			fprintf(stderr, "vic: JvsSetCompRaw failed\n");
			exit(1);
		}
		w = rep.width;
		h = rep.height;
	}
	if (w != inw_ || h != inh_)
		setsize(w, h);

	if (grabbing)
		/*
		 * If we interrupted an in-progress grab,
		 * kick off a new request.
		 */
		kick();
}

inline void Jv422Grabber::saveblk(const u_char* in, u_char* yp, u_char* up,
				  u_char* vp, int stride)
{
	int is = stride << 1;
	int cs = stride >> 1;

	for (int i = 16; --i >= 0; ) {
		/*
		 * Each iteration of this loop grabs 16 Ys & 8 U/Vs.
		 */
		register u_int y0, y1, u, v;

		u  = in[1] | in[5]  << 8 | in[9]  << 16 | in[13] << 24;
		v  = in[3] | in[7]  << 8 | in[11] << 16 | in[15] << 24;
		y0 = in[0] | in[2]  << 8 | in[4]  << 16 | in[6]  << 24;
		y1 = in[8] | in[10] << 8 | in[12] << 16 | in[14] << 24;

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;
		*(u_int*)up = u;
		*(u_int*)vp = v;

		u  = in[16+1] | in[16+5]  << 8 | in[16+9]  << 16 | in[16+13] << 24;
		v  = in[16+3] | in[16+7]  << 8 | in[16+11] << 16 | in[16+15] << 24;
		y0 = in[16+0] | in[16+2]  << 8 | in[16+4]  << 16 | in[16+6] << 24;
		y1 = in[16+8] | in[16+10] << 8 | in[16+12] << 16 | in[16+14] << 24;

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

void Jv422Grabber::saveblks(const u_char* in)
{
	u_char* crv = crvec_;
	u_char* lum = frame_;
	int off = framesize_;
	u_char* chm = lum + off;
	off >>= 1;
	int stride = 15 * inw_;
	for (int y = 0; y < blkh_; ++y) {
		for (int x = 0; x < blkw_; ++x) {
			int s = *crv++;
			if ((s & CR_SEND) != 0)
				saveblk(in, lum, chm, chm + off, outw_);

			in += 32;
			lum += 16;
			chm += 8;
		}
		lum += stride;
		chm += stride >> 1;
		in += stride << 1;
	}
}

void Jv422Grabber::setsize(int w, int h)
{
	set_size_422(w, h);
}

void JvCIFGrabber::setsize(int w, int h)
{
	set_size_cif(w, h);
}

inline void JvCIFGrabber::saveblk(const u_char* in, u_char* yp, u_char* up,
				  u_char* vp, int stride, int is)
{
	int cs = stride >> 1;

	for (int i = 8; --i >= 0; ) {
		/*
		 * Each iteration of this loop grabs 16 Ys & 8 U/Vs.
		 */
		register u_int y0, y1, u, v;

		u  = in[1] | in[5]  << 8 | in[9]  << 16 | in[13] << 24;
		v  = in[3] | in[7]  << 8 | in[11] << 16 | in[15] << 24;
		y0 = in[0] | in[2]  << 8 | in[4]  << 16 | in[6] << 24;
		y1 = in[8] | in[10] << 8 | in[12] << 16 | in[14] << 24;

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;
		*(u_int*)up = u;
		*(u_int*)vp = v;

		u  = in[16+1] | in[16+5]  << 8 | in[16+9]  << 16 | in[16+13] << 24;
		v  = in[16+3] | in[16+7]  << 8 | in[16+11] << 16 | in[16+15] << 24;
		y0 = in[16+0] | in[16+2]  << 8 | in[16+4]  << 16 | in[16+6] << 24;
		y1 = in[16+8] | in[16+10] << 8 | in[16+12] << 16 | in[16+14] << 24;

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;
		((u_int*)up)[1] = u;
		((u_int*)vp)[1] = v;

		in += is;
		yp += stride;
		up += cs;
		vp += cs;

		/* do the 2nd (y only instead of yuv) line */
		y0 = in[0] | in[2]  << 8 | in[4]  << 16 | in[6] << 24;
		y1 = in[8] | in[10] << 8 | in[12] << 16 | in[14] << 24;

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;

		y0 = in[16+0] | in[16+2]  << 8 | in[16+4]  << 16 | in[16+6] << 24;
		y1 = in[16+8] | in[16+10] << 8 | in[16+12] << 16 | in[16+14] << 24;

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;

		in += is;
		yp += stride;
	}
}

void JvCIFGrabber::saveblks(const u_char* in)
{
	int is = 2 * inw_;
	u_char* crv = crvec_;
	u_char* lum = frame_;
	u_char* chm = frame_ + framesize_;
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

			in += 32;
			lum += 16;
			chm += 8;
		}
		crv += skip;
		lum += 15 * outw_ + skip * 16;
		chm += 7 * (outw_ >> 1) + skip * 8;
		in = nin + 16 * is;
	}
}

void Jv420Grabber::setsize(int w, int h)
{
	set_size_420(w, h);
}
JvJpegGrabber::JvJpegGrabber() : q_(50)
{
	if (status_ < 0)
		return;
	cb0_ = allocdma(JVS_JPEG);
	cb1_ = allocdma(JVS_JPEG);
	cb_ = cb0_;
}

JvJpegGrabber::~JvJpegGrabber()
{
}

int JvJpegGrabber::consume(const JvGrabber::dmabuf& p, int length)
{
	/*
	 * We should get a better timestamp, probably from jvdriver.
	 */
	JpegFrame f(media_ts(), p.bp, length, q_, 0, inw_, inh_);
	return (target_->consume(&f));
}

void JvJpegGrabber::SetComp()
{
	int grabbing = grabbing_;
	if (grabbing)
		sync();

	/*
	 * Don't bother doing anything fancy with frame subsampling.
	 * On the j300 this doesn't work anyway, and on the jvideo
	 * we set it to one so we get every other field.
	 */
	int frameskip = 1;
	int w, h;
	if (JvsSetComp(server_, q_, x_decimate_, y_decimate_,
		       frameskip, &w, &h) < 0) {
		fprintf(stderr, "vic: JvsSetComp failed\n");
		exit(1);
	}
	inw_ = w;
	inh_ = h;

	if (grabbing)
		/*
		 * If we interrupted an in-progress grab,
		 * kick off a new request.
		 */
		kick();
}

int JvJpegGrabber::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "q") == 0) {
			/* assume value is in range */
			q_ = atoi(argv[2]);
			SetComp();
			return (TCL_OK);
		}
	}
	return (JvGrabber::command(argc, argv));
}

