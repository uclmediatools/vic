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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef WIN32
#include <sys/param.h>
#endif
#include "sys-time.h"
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "renderer.h"
#include "color-hist.h"

extern "C" {
#ifdef USE_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#if defined(sun) && !defined(__svr4__)
int shmget(key_t, int, int);
char *shmat(int, char*, int);
int shmdt(char*);
int shmctl(int, int, struct shmid_ds*);
#endif
#endif
}


//SV-XXX: rearranged intialistaion order to shut upp gcc4
Decoder::Decoder(int hdrlen) : PacketHandler(hdrlen),
	nstat_(0), color_(1), decimation_(422), inw_(0), inh_(0), 
	engines_(0), rvts_(0), nblk_(0), ndblk_(0), grabber_(0)
{
	/*XXX*/
	now_ = 1;

	for (int i = 0; i < MAXSTAT; ++i)
		stat_[i].cnt = 0;
}


Decoder::~Decoder()
{
	if (rvts_)
		delete[] rvts_; //SV-XXX: Debian
}

int Decoder::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "width") == 0) {
			tcl.resultf("%d", width());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "height") == 0) {
			tcl.resultf("%d", height());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "info") == 0) {
			char* bp = tcl.buffer();
			info(bp);
			tcl.result(bp);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "stats") == 0) {
			char* bp = tcl.buffer();
			tcl.result(bp);
			stats(bp);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "decimation") == 0) {
			tcl.resultf("%d", decimation());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "redraw") == 0) {
			redraw();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "playout") == 0) {
			/*XXX*/
			tcl.result("0");
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "color") == 0) {
			setcolor(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "attach") == 0) {
			Renderer* r = (Renderer*)TclObject::lookup(argv[2]);
			if (r == 0) {
				tcl.resultf("%s attach: no such renderer: %s",
					    argv[0], argv[2]);
				return (TCL_ERROR);
			}
			attach(r);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "detach") == 0) {
			Renderer* r = (Renderer*)TclObject::lookup(argv[2]);
			if (r == 0) {
				tcl.resultf("%s detach: no such target: %s",
					    argv[0], argv[2]);
				return (TCL_ERROR);
			}
			detach(r);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "histogram") == 0) {
			ColorHist* ch = ColorHist::lookup(argv[2]);
			/*XXX*/
			if (ch == 0)
				abort();
			colorhist(ch->histogram());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "grabber") == 0) {
			grabber_ = (Grabber*)TclObject::lookup(argv[2]);
			fprintf(stderr,"Decoder:Setting this %d grabber_:%d\n", this,grabber_);
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

void Decoder::info(char* wrk) const
{
	*wrk = 0;
}

void Decoder::stats(char* bp)
{
	if (nstat_ == 0) {
		*bp = 0;
		return;
	}
	for (int i = 0; i < nstat_; ++i) {
		sprintf(bp, "%s %d ", stat_[i].name, stat_[i].cnt);
		bp += strlen(bp);
	}
	bp[-1] = 0;
}

void Decoder::allocshm(dmabuf& d, int size, int flag)
{
#ifdef USE_SHM
	d.shmid = shmget(IPC_PRIVATE, size, IPC_CREAT|0777);
	if (d.shmid < 0) {
		perror("shmget");
		exit(1);
	}
	d.bp = (u_char *)shmat(d.shmid, 0, flag);
	if (d.bp == (u_char*)-1) {
		perror("shmat");
		exit(1);
	}
#endif
}

/*XXX not used*/
void Decoder::freeshm(dmabuf& d)
{
#ifdef USE_SHM
	if (shmdt((char*)d.bp) < 0)
		perror("shmdt");
#endif
}

/*
 * Return time of day in microseconds.
 */
double Decoder::gettimeofday_usecs()
{
	timeval tv;
	::gettimeofday(&tv, 0);
	return (1e6 * double(tv.tv_sec) + double(tv.tv_usec));
}

void Decoder::attach(Renderer* r)
{
	r->next_ = engines_;
	engines_ = r;
	r->setcolor(color_);
	r->now(0);
	redraw();
}

void Decoder::redraw(const u_char* frm)
{
	/* make sure all the renderer's are synced */
	now_ = 1;
	memset(rvts_, 1, nblk_);
	for (Renderer* p = engines_; p != 0; p = p->next_)
		p->now(0);
	render_frame(frm);
}

void Decoder::detach(Renderer* r)
{
	Renderer** p;	
	for (p = &engines_; *p != r; p = &(*p)->next_)
		if (*p == 0)
			abort();
	*p = (*p)->next_;
}

void Decoder::colorhist_422_556(u_int* hist, const u_char* yp,
					const u_char* up, const u_char* vp,
					int width, int h) const
{
	while (--h >= 0) {
		for (int w = width; w > 0; w -= 2) {
			/* 5V:5U:6Y */
			int uv = vp[0] >> 3 << 11 | up[0] >> 3 << 6;
			++hist[uv | yp[0] >> 2];
			++hist[uv | yp[1] >> 2];
			yp += 2;
			up += 1;
			vp += 1;
		}
	}
}

void Decoder::colorhist_420_556(u_int* hist, const u_char* yp,
					const u_char* up, const u_char* vp,
					int width, int h) const
{
	for (; h > 0; h -= 2) {
		for (int w = width; w > 0; w -= 2) {
			/* 5V:5U:6Y */
			int uv = vp[0] >> 3 << 11 | up[0] >> 3 << 6;
			++hist[uv | yp[0] >> 2];
			++hist[uv | yp[1] >> 2];
			++hist[uv | yp[width] >> 2];
			++hist[uv | yp[width + 1] >> 2];
			yp += 2;
			up += 1;
			vp += 1;
		}
		yp += width;
	}
}


static inline unsigned long long int
sqr (unsigned long long int x)
{
        return (x*x);
}

static inline double
psnr_equ (unsigned long long int ssd, unsigned long long int pixels) {
        double mse = (double)ssd / (double)pixels;
        double psnr = 10.0 * log10 (sqr(255.0)/mse);

        return psnr;
}

double
calc_psnr(u_int8_t *recv_frame, u_int8_t *orig_frame, int width, int height) {
  double psnr, psnr_y, psnr_u, psnr_v;
  unsigned long long int total_ssd_y, total_ssd_u, total_ssd_v;
  unsigned long long int ssd, local_ssd_y = 0, local_ssd_u = 0, local_ssd_v = 0;
  //  *ssd_y, *ssd_u, *ssd_v, 
  //int    image1_u0, image1_y0, image1_v0, image1_y1, i, 
  //       image2_u0, image2_y0, image2_v0, image2_y1;
  int i;

  for (i=0; i<width*height; i++) 
     local_ssd_y += sqr( abs( *recv_frame++ - *orig_frame++ ));
  for (i=0; i<width/2*height/2; i++) 
     local_ssd_v += sqr( abs( *recv_frame++ - *orig_frame++ ));
  for (i=0; i<width/2*height/2; i++) 
     local_ssd_u += sqr( abs( *recv_frame++ - *orig_frame++ ));
  
  ssd            = local_ssd_y + local_ssd_u + local_ssd_v;
  total_ssd_y    += local_ssd_y;
  total_ssd_u    += local_ssd_u;
  total_ssd_v    += local_ssd_v;

  psnr           = psnr_equ (ssd,        width * height * 2);
  psnr_y         = psnr_equ (local_ssd_y, width * height);
  psnr_u         = psnr_equ (local_ssd_u, (width * height) / 2);
  psnr_v         = psnr_equ (local_ssd_v, (width * height) / 2);
 
  return psnr;
}

void Decoder::render_frame(const u_char* frm, int frame_no)
{
	/*
	 * Go through all the timestamps and smash the time that
	 * is about to wrap from the past into the future to the present,
	 * so that the block gets updated just once.  If we let timestamps
	 * wrap without doing anything, the algorithm would still work,
	 * but we'd end up rendering a stationary block on every call here
	 * (until we surpassed now + 128 again).  In other words, for
	 * an unchanging block, this approach renders it 2 times out of 256,
	 * rather than 128 out of 256.
	 */
        double psnr=0;
	int wraptime = now_ ^ 0x80;
	u_char* ts = rvts_;
	for (int k = nblk_; --k >= 0; ++ts) {
		if (*ts == wraptime)
			*ts = now_;
	}

	YuvFrame f(now_, (u_int8_t*)frm, rvts_, inw_, inh_,0, frame_no);
	if (grabber_!=0 && frame_no != -1) {
	     int framesize=inw_*inh_;
	     psnr = calc_psnr((u_int8_t*)frm, (u_int8_t*)grabber_->frame_+frame_no*(framesize+(framesize>>1)), inw_, inh_);
	     //psnr = calc_psnr((u_int8_t*)frm, (u_int8_t*)grabber_->frame_+frame_no*(grabber_->framesize_+(grabber_->framesize_>>1)), inw_, inh_);
	     debug_msg("Frame (%dx%d;fsz:%d(%d),Grabber:%x): #%d, PSNR: %f\n",inw_,inh_,inw_*inh_, grabber_->framesize_,grabber_, frame_no, psnr);
	}
	for (Renderer* p = engines_; p != 0; p = p->next_)
		if ((p->ft() & FT_HW) == 0)
			p->consume(&f);

	now_ = (now_ + 1) & 0xff;
}

void Decoder::setcolor(int color)
{
	if (color != color_) {
		color_ = color;
		for (Renderer* p = engines_; p != 0; p = p->next_)
			p->setcolor(color_);
	}
}

void Decoder::resize(int width, int height)
{
	inw_ = width;
	inh_ = height;
	nblk_ = (width * height) / 64;
	delete[] rvts_; //SV-XXX: Debian
	rvts_ = new u_char[nblk_];
	memset(rvts_, 0, nblk_);
	redraw();
}

/*
 * Dummy class for streams that we recognize but cannot decode (for lack
 * of software support).  We still want to put all the stats etc. --
 * especially the format -- but can't decode it.  XXX We need to somehow
 * mark the stream in the user-interface so that the user knows that
 * vic cannot decode the stream (to avoid assuming something else is
 * wrong with the tranmission)
 */
class NullDecoder : public Decoder {
public:
	NullDecoder() : Decoder(0) {}
	int colorhist(u_int* histogram) const { UNUSED(histogram); return (0); } //SV-XXX: unused histogram
	void redraw() {}
//	virtual void recv(const struct rtphdr*, const u_char* data, int len) {}
	virtual void recv(pktbuf* pb) { pb->release();}
};

static class NullDecoderMatcher : public Matcher {
public:
	NullDecoderMatcher() : Matcher("decoder") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "null") == 0)
			return (new NullDecoder());
		return (0);
	}
} dm_null;

PlaneDecoder::PlaneDecoder(int hdrlen) : Decoder(hdrlen), frm_(0)
{
}

PlaneDecoder::~PlaneDecoder()
{
	delete[] frm_; //SV-XXX: Debian
}

void PlaneDecoder::redraw()
{
	Decoder::redraw(frm_);
}

void PlaneDecoder::resize(int width, int height)
{
	delete[] frm_; //SV-XXX: Debian
	int size = width * height;
	frm_ = new u_char[2 * size];
	/* 
	 * Initialize image to gray.
	 */
	memset(frm_, 0x80, 2 * size);
	Decoder::resize(width, height);
}

int PlaneDecoder::colorhist(u_int* histogram) const
{
	int w = inw_;
	int h = inh_;
	int s = w * h;
	u_char* up = frm_ + s;
	colorhist_422_556(histogram, frm_, up, up + (s >> 1), w, h);
	return (1);
}
