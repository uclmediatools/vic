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
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#if defined(__osf__) || defined(__ultrix__)
/*XXX they didn't get this one right */
extern "C" {
#include <sys/types.h>
#include <sys/uio.h>
}
#else
#include <sys/types.h>
#ifndef WIN32
#include <sys/uio.h>
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "grabber.h"
#include "vic_tcl.h"
#include "crdef.h"

#if defined(sun) && !defined(__svr4__)
extern "C" int gettimeofday(struct timeval*, struct timezone*);
#endif

//SV-XXX: rearranged initialisation order to shut up gcc4
Grabber::Grabber()
	: vstart_(0), vstop_(0), 
	  hstart_(0), hstop_(0),
	  threshold_(48),
	  framebase_(0), frame_(0), crvec_(0), ref_(0),
	  inw_(0), inh_(0), outw_(0), outh_(0),
	  target_(0), tx_(0), rover_(0),
	  running_(0), status_(0), delta_(0.)
{
	bps(128);
	fps(1);

	/*XXX*/
	idle_low_ = 2;
	idle_high_ = 2;

	/* CCIR 601 */
	ymin_ = 16;
	ymax_ = 235;
	contrast_ = 1.0;
	for (int i = 0; i < 256; ++i)
		ynorm_[i] = i;

	// time measurement
	start_grab_ = 0.;
	end_grab_ = 0.;
}

Grabber::~Grabber()
{
	delete[] framebase_; //SV-XXX: Debian
	delete[] crvec_; //SV-XXX: Debian
	delete[] ref_; //SV-XXX: Debian
}

int Grabber::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "status") == 0) {
			sprintf(tcl.buffer(), "%d", status_);
			tcl.result(tcl.buffer());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "need-capwin") == 0) {
			tcl.result("0");
			return (TCL_OK);
		}
	}
	if (argc == 3) {
		if (strcmp(argv[1], "send") == 0) {
			if (atoi(argv[2])) {
				if (!running_) {
					start();
					running_ = 1;
				}
			} else {
				if (running_) {
					stop();
					running_ = 0;
				}
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "threshold") == 0) {
			threshold_ = 8*atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "fps") == 0) {
			/*XXX assume value in range */
			fps(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "bps") == 0) {
			/*XXX assume value in range */
			bps(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "fillrate") == 0) {
			/*XXX assume value in range */
			fillrate(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "target") == 0) {
			target_ = (Module*)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "transmitter") == 0) {
			tx_ = (Transmitter*)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		/*XXX*/
		if (strcmp(argv[1], "decimate") == 0 ||
		    strcmp(argv[1], "port") == 0 ||
		    strcmp(argv[1], "type") == 0 ||
			/* windows only */
			strcmp(argv[1], "useconfig") ==0)
			/* ignore */
			return (TCL_OK);
	}
	return (TclObject::command(argc, argv));
}

void Grabber::contrast(double c)
{
	/* map min to ccir-601 black (16) & max to ccir-601 white (235) */
	u_int min = ymin_, max = ymax_;
	double dmin = min, dmax = max;
	double dslope = 219. / (dmax - dmin) * c;
	double dy = 16.;
	u_int i;
	for (i = 0; i < min; ++i)
		ynorm_[i] = u_char(dy);
	for ( ; i < max; ++i) {
		ynorm_[i] = u_char(dy);
		if (dy < 235.)
			dy += dslope;
	}
	for ( ; i < 256; ++i)
		ynorm_[i] = u_char(dy);

	contrast_ = c;
}

void Grabber::fps(int v)
{
	fps_ = v;
	frametime_ = 1e6 / double(v);
}

void Grabber::bps(int kbps)
{
	bps_ = 1000 * kbps;
}

/*
 * Advance the frame clock and return the amount of time we
 * need to wait before sending the next frame.  We compute
 * this time according to the desired bit and frame rates,
 * favoring the more restrictive metric.  If we're more than
 * 200ms behind (e.g., the cpu is saturated or we've been
 * suspended), give up and reset the frame clock.
 */
double Grabber::tick(int frameSize)
{
	double bpsframetime = 8e6 * double(frameSize) / double(bps_); // uSecs
	if (bpsframetime < frametime_) {
		if (bpsframetime * 2. < frametime_)
			delta_ += (bpsframetime - delta_) * .25;
		else
			delta_ = bpsframetime;
		bpsframetime = frametime_;
	} else
		delta_ = bpsframetime;

	frameclock_ += bpsframetime;
	double now = gettimeofday_usecs();
	double delta = frameclock_ - now;
	if (delta < -0.2e6) {
		delta = bpsframetime;
		frameclock_ = now;
	} else if (delta < 0)
		/*
		 * We're not too far behind.
		 * Try to catch up.
		 */
		delta = 0.;

        debug_msg("tick(): now: %f delta: %f (delta_ %f) frametime_ %f frametime %f frameclock_ %f, frameSize :%d\n",now, delta, delta_, frametime_, bpsframetime, frameclock_, frameSize);
	return (delta);
}

void Grabber::start()
{
	frameclock_ = gettimeofday_usecs();
#ifndef WIN32
	timeout();
#endif
}

void Grabber::stop()
{
	cancel();
}

void Grabber::timeout()
{
	for (;;) {
		double delta = tick(grab());
		if (delta != 0.) {
			usched(delta);
			return;
		}
	}
}

int Grabber::grab()
{
	abort();
	return (0);
}

void Grabber::crinit(int w, int h)
{
	blkw_ = w >> 4;
	blkh_ = h >> 4;
	scan_ = 0;
	nblk_ = blkw_ * blkh_;
	delete[] crvec_; //SV-XXX: Debian
	crvec_ = new u_char[nblk_];
	for (int i = 0; i < nblk_; ++i)
		crvec_[i] = CR_MOTION|CR_SEND;
}

/* must call after set_size_xxx */
void Grabber::allocref()
{
	delete[] ref_; //SV-XXX: Debian
	ref_ = new u_char[framesize_];
	memset((char*)ref_, 0, framesize_);
}

/*
 * define these for REPLENISH macro used below
 */
#define ABS(v) if (v < 0) v = -v;

#define DIFF4(in, frm, v) \
	v += (in)[0] - (frm)[0]; \
	v += (in)[1] - (frm)[1]; \
	v += (in)[2] - (frm)[2]; \
	v += (in)[3] - (frm)[3];

#define DIFFLINE(in, frm, left, center, right) \
	DIFF4(in, frm, left); \
	DIFF4(in + 1*4, frm + 1*4, center); \
	DIFF4(in + 2*4, frm + 2*4, center); \
	DIFF4(in + 3*4, frm + 3*4, right); \
	ABS(right); \
	ABS(left); \
	ABS(center);

void Grabber::suppress(const u_char* devbuf)
{
	REPLENISH(devbuf, ref_, outw_, 1, 0, blkw_, 0, blkh_);
}

inline void save(const u_char* lum, u_char* cache, int stride)
{
	for (int i = 16; --i >= 0; ) {
		((u_int*)cache)[0] = ((u_int*)lum)[0];
		((u_int*)cache)[1] = ((u_int*)lum)[1];
		((u_int*)cache)[2] = ((u_int*)lum)[2];
		((u_int*)cache)[3] = ((u_int*)lum)[3];
		cache += stride;
		lum += stride;
	}
}

/*
 * Default save routine -- stuff new luma blocks into cache.
 */
void Grabber::saveblks(u_char* lum)
{
	u_char* crv = crvec_;
	u_char* cache = ref_;
	int stride = outw_;
	stride = (stride << 4) - stride;
	for (int y = 0; y < blkh_; y++) {
		for (int x = 0; x < blkw_; x++) {
			if ((*crv++ & CR_SEND) != 0)
				save(lum, cache, outw_);
			cache += 16;
			lum += 16;
		}
		lum += stride;
		cache += stride;
	}
}

void Grabber::age_blocks()
{
	for (int i = 0; i < nblk_; ++i) {
		int s = CR_STATE(crvec_[i]);
		/*
		 * Age this block.
		 * Once we hit the age threshold, we
		 * set CR_SEND as a hint to send a
		 * higher-quality version of the block.
		 * After this the block will stop aging,
		 * until there is motion.  In the meantime,
		 * we might send it as background fill
		 * using the highest quality.
		 */
		if (s <= CR_AGETHRESH) {
			if (s == CR_AGETHRESH)
				s = CR_IDLE;
			else {
				if (++s == CR_AGETHRESH)
					s |= CR_SEND;
			}
			crvec_[i] = s;
		} else if (s == CR_BG)
			/*
			 * reset the block to IDLE if it was sent
			 * as a BG block in the last frame.
			 */
			crvec_[i] = CR_IDLE;
	}
	/*
	 * Now go through and look for some idle blocks to send
	 * as background fill.
	 */
	int blkno = rover_;
	int n = (delta_ * 2. < frametime_)? idle_high_ : idle_low_;
	while (n > 0) {
		int s = CR_STATE(crvec_[blkno]);
		if (s == CR_IDLE) {
			crvec_[blkno] = CR_SEND|CR_BG;
			--n;
		}
		if (++blkno >= nblk_) {
			blkno = 0;
			/* one way to guarantee loop termination */
			break;
		}
	}
	rover_ = blkno;

	/*
	 * Bump the CR scan pointer.  This variable controls which
	 * scan line of a block we use to make the replenishment
	 * decision.  We skip 3 lines at a time to quickly precess
	 * over the block.  Since 3 and 8 are coprime, we will
	 * sweep out every line.
	 */
	scan_ = (scan_ + 3) & 7;
}

void Grabber::set_size_422(int w, int h)
{
	delete[] framebase_; //SV-XXX: Debian

	inw_ = w;
	inh_ = h;
	w &=~ 0xf;
	h &=~ 0xf;
	outw_ = w;
	outh_ = h;

	framesize_ = w * h;
	int ny = framesize_ + GRABBER_VPAD * w; // Y size
	int nuv = ny;  // U + V size
	framebase_ = new u_char[ny + nuv];
	/* initialize to black */
	memset(framebase_, 0, ny);
	memset(framebase_ + ny, 0x80, nuv);
	frame_ = framebase_ + GRABBER_VPAD * w;
	crinit(w, h);

	vstart_ = 0;
	vstop_ = blkh_;
	hstart_ = 0;
	hstop_ = blkw_;
}

void Grabber::set_size_420(int w, int h)
{
	delete[] framebase_; //SV-XXX: Debian

	inw_ = w;
	inh_ = h;
	w &=~ 0xf;
	h &=~ 0xf;
	outw_ = w;
	outh_ = h;

	int s = w * h;
	framesize_ = s;
	int ny = s + GRABBER_VPAD * outw_;	// Y size
	int nuv = (s >> 1) + GRABBER_VPAD * outw_;	// U + V size
	framebase_ = new u_char[ny + nuv];
	/* initialize to black */
	memset(framebase_, 0, ny);
	memset(framebase_ + ny, 0x80, nuv);
	frame_ = framebase_ + GRABBER_VPAD * outw_;
	crinit(w, h);

	vstart_ = 0;
	vstop_ = blkh_;
	hstart_ = 0;
	hstop_ = blkw_;
}

void Grabber::set_size_cif(int w, int h)
{
	delete[] framebase_; //SV-XXX: Debian
        framebase_ = 0;
	inw_ = w;
	inh_ = h;

	int ispal;
	switch (h) {
	case 240:
		/* 1/4 NTSC */
		ispal = 0;
		outw_ = 352;
		outh_ = 288;
		break;

	case 120:
		/* 1/8 NTSC */
		ispal = 0;
		outw_ = 176;
		outh_ = 144;
		break;

	case 288:
		/* 1/4 CIF */
		ispal = 1;
		outw_ = 352;
		outh_ = 288;
		break;

	case 144:
		/* 1/8 CIF */
		ispal = 1;
		outw_ = 176;
		outh_ = 144;
		break;

	default:
		/* XXX this shouldn't happen */
		debug_msg("vic: CIF grabber: bad geometry - trying 420\n");
		set_size_420(w,h);
		return;
		//abort();
	}
	int s = outw_ * outh_;
	framesize_ = s;
	int ny = s + GRABBER_VPAD * outw_;  // Y size
	int nuv = (s >> 1) + GRABBER_VPAD * outw_; //U + V size
	framebase_ = new u_char[ny + nuv];
	/* initialize to black */
	memset(framebase_, 0, ny);
	memset(framebase_ + ny, 0x80, nuv);
	frame_ = framebase_ + GRABBER_VPAD * outw_;
	crinit(outw_, outh_);

	if (ispal) {
		/* PAL: field is bigger than CIF */
		vstart_ = 0;
		vstop_ = blkh_;
		hstart_ = 0;
		hstop_ = blkw_;
	} else {
		/* NTSC: field is smaller than CIF */
		vstart_ = 1;
		vstop_ = vstart_ + inh_ / 16;
		int nw = inw_ / 16;
		int margin = (blkw_ - nw + 1) / 2;
		hstart_ = margin;
		hstop_ = blkw_ - margin;
	}
}
