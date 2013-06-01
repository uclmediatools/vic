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

#ifndef vic_decoder_h
#define vic_decoder_h

extern "C" {
#include <tk.h>
}

#ifndef WIN32
#include <sys/param.h>
#include <sys/uio.h>
#endif
#include "sys-time.h"
#include "inet.h"
//#include "rv.h"
#include "vic_tcl.h"
#include "source.h"
#include "ntp-time.h"
#include "grabber.h"

/*
 * Rendering vector.  We keep a vector of timestamps of when each individual
 * block of frame has been updated by a decoder.  Then, different renderers
 * can keep track of the last time they've updated their view of the frame
 * (and dither only those blocks that need to be updated).
 */

#define RV_PAST(now, ts) ((((now) - (ts)) & 0x80) == 0)

class Renderer;
class Assistor;
class VideoWindow;

struct dmabuf {
	int shmid;
	u_char* bp;
};

class Decoder : public PacketHandler {
 protected:
	Decoder(int hdrlen);
 public:
	virtual ~Decoder();

	virtual int colorhist(u_int* histogram) const = 0;

	/* statistics */
	virtual void info(char* wrk) const;
	virtual void stats(char* wrk);

	static void allocshm(dmabuf&, int, int flag = 0);
	static void freeshm(dmabuf&);
	static double gettimeofday_usecs();

	virtual int command(int argc, const char*const* argv);

	virtual void resize(int inw, int inh);

	inline int width() const { return (inw_); }
	inline int height() const { return (inh_); }
	inline int decimation() const { return (decimation_); }
	inline int ndblk() const { return (ndblk_); }
	inline void resetndblk() { ndblk_ = 0; }
	void setcolor(int color);
 protected:
/*XXX*/
#define MAXSTAT 16
	struct statcntr {
		const char* name;
		u_int cnt;
	} stat_[MAXSTAT];
	int nstat_;
	inline void count(int statno) { ++stat_[statno].cnt; }
	inline void setstat(int statno, u_int v) { stat_[statno].cnt = v; }

	/*XXX steal back from rcvr */
	int color_;
	int decimation_;	/* 422 or 420 */
	int inw_;		/* native image width */
	int inh_;		/* native image height */
        Grabber* grabber_;

 public:
	/*
	 * Add or remove a window to the set of windows
	 * that this decoder outputs to.
	 */
	virtual void attach(Renderer*);
	virtual void detach(Renderer*);
 protected:
	void redraw(const u_char* frm);
	virtual void redraw() = 0;
	Renderer* engines_;

	void render_frame(const u_char* frm, int frame_no=0);
	int now_;
	u_char* rvts_;
	int nblk_;		/* number of 8x8 blocks */
	int ndblk_;	/* number of blocks decoded in most recent frame */

	void colorhist_420_556(u_int* hist, const u_char* y, const u_char* u,
			       const u_char* v, int width, int height) const;
	void colorhist_422_556(u_int* hist, const u_char* y, const u_char* u,
			       const u_char* v, int width, int height) const;
};

class PlaneDecoder : public Decoder {
 public:
	PlaneDecoder(int hdrlen);
	virtual ~PlaneDecoder();
	virtual int colorhist(u_int* histogram) const;
	void sync();
 protected:
	void resize(int width, int height);
	virtual void redraw();

	u_char* frm_;		/* storage for YUV representation */
};

#endif
