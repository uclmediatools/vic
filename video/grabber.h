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

#ifndef vic_grabber_h
#define vic_grabber_h

#include "config.h"
#include "timer.h"
#include "media-timer.h"
#include "vic_tcl.h"

#ifdef USE_FASTMEMCPY
extern "C" {
#include "postproc/fastmemcpy.h"
}
#endif

class Module;
class Transmitter;

/*
 * Number of horizontal lines to pad at the top and bottom of the
 * framer buffers that are handed to the Encoder encode method.
 * Some encoders can operate more efficiently with this scratch
 * space (i.e., the bvc coder).
 */
#define GRABBER_VPAD 1

class Grabber : public TclObject, public Timer, public MediaTimer {
 protected:
	Grabber();
 public:
	virtual ~Grabber();
	virtual int command(int argc, const char*const* argv);
	virtual void fps(int);
	inline int fps() const { return (fps_); }
	virtual void bps(int);
	inline int bps() const { return (bps_); }
	inline void fillrate(int v) { idle_high_ = v; }
	double tick(int n);
	virtual void start();
	virtual void stop();
	inline int status() const { return (status_); }

	virtual int grab();
	virtual void timeout();
	int background() const;

	// time measurement
	inline double grabber_now() {
        timeval tv;
        ::gettimeofday(&tv, NULL);
        return ((double) tv.tv_sec + 1e-6 * (double) tv.tv_usec);
	}
	double grabber_ts_off_;
	double start_grab_;
	double end_grab_;

	u_int framesize_;
	u_char* frame_;
 protected:
	/* hooks for conditional replenishment algorithm */
	void suppress(const u_char* devbuf);
	void saveblks(u_char* lum);
	void crinit(int w, int h);
	void age_blocks();
	double advance();

	void set_size_422(int w, int h);
	void set_size_420(int w, int h);
	void set_size_cif(int w, int h);
	void allocref();

	int vstart_;
	int vstop_;
	int hstart_;
	int hstop_;
	int threshold_ ; // when a block is changed ? (was constant 48)

	u_char* framebase_;
	u_char* crvec_;
	u_char* ref_;/*XXX*/
	int inw_;
	int inh_;
	int outw_;
	int outh_;
	int blkw_;
	int blkh_;

	Module* target_;
	Transmitter* tx_;
	
	int scan_;
	int nblk_;
	int rover_;
	int idle_low_;
	int idle_high_;

	double frametime_;	/* 1 / fps_ */
	double frameclock_;	/* logical clock for pumping out frames */

	int fps_;		/* desired frames per second */
	int bps_;		/* desired bits per second */
	int running_;		/* true when grabber active */
	int status_;		/* true if grabber okay */

	double delta_;		/* most recent inter-frame time */

	/* brightness/contrast controls */
	void contrast(double c);
	u_int ymin_;
	u_int ymax_;
	double contrast_;
	u_char ynorm_[256];
};

#define REPLENISH(devbuf, refbuf, ds, bpp, hstart, hstop, vstart, vstop) \
{ \
	/* \
	 * First age the blocks from the previous frame. \
	 */ \
	age_blocks(); \
\
	register int _ds = ds; \
	register int _rs = outw_; \
	const u_char* rb = &(refbuf)[scan_ * _rs]; \
	const u_char* db = &(devbuf)[scan_ * _ds]; \
	int w = blkw_; \
	u_char* crv = crvec_; \
 \
	crv += (vstart) * w; \
	for (int y = vstart; y < vstop; ++y) { \
		const u_char* ndb = db; \
		const u_char* nrb = rb; \
		u_char* ncrv = crv; \
		crv += hstart; \
		for (int x = hstart; x < hstop; x++) { \
			int left = 0; \
			int right = 0; \
			int top = 0; \
			int bottom = 0; \
			DIFFLINE(db, rb, left, top, right); \
			db += _ds << 3; \
			rb += _rs << 3; \
			DIFFLINE(db, rb, left, bottom, right); \
			db -= _ds << 3; \
			rb -= _rs << 3; \
 \
			int center = 0; \
			if (left >= threshold_ && x > 0) { \
				crv[-1] = CR_MOTION|CR_SEND; \
				center = 1; \
			} \
			if (right >= threshold_ && x < w - 1) { \
				crv[1] = CR_MOTION|CR_SEND; \
				center = 1; \
			} \
			if (bottom >= threshold_ && y < blkh_ - 1) { \
				crv[w] = CR_MOTION|CR_SEND; \
				center = 1; \
			} \
			if (top >= threshold_ && y > 0) { \
				crv[-w] = CR_MOTION|CR_SEND; \
				center = 1; \
			} \
			if (center) \
				crv[0] = CR_MOTION|CR_SEND; \
 \
			db += 16 * (bpp); \
			rb += 16; \
			++crv; \
		} \
		db = ndb + (_ds << 4); \
		rb = nrb + (_rs << 4); \
		crv = ncrv + w; \
	} \
}

#endif
