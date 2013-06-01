/*
 * Copyright (c) 1995 The Regents of the University of California.
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

#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#else
#include <sys/file.h>
#endif
#include "module.h"
#include "crdef.h"

class Overlay : public TclObject {
public:
	Overlay();
	inline int width() const { return (width_); }
	inline int height() const { return (height_); }
	inline const u_char* image() const { return (image_); }
	inline int transparent() const { return (transparent_); }
protected:
	int command(int argc, const char*const* argv);
	int load(const char* file, int w, int h);
	int width_;
	int height_;
	u_char* image_;
	int transparent_; /* transparent luminance */
};

static class OverlayMatcher : public Matcher {
public:
	OverlayMatcher() : Matcher("overlay") {}
	TclObject* match(const char*) {
		return (new Overlay());
	}
} matcher_overlay;

class Compositor : public Module {
 public:
	Compositor(int ft);
	~Compositor();
 protected:
	void reset();
	int command(int argc, const char*const* argv);
	void crinit(int w, int h);
	u_char* frm_;
	u_char* framebase_;
	u_char* damage_;
	struct onode {
		Overlay* overlay;
		int x;
		int y;
		int depth;
		onode* next;
	};
	void attach(Overlay* o, int x, int y, int depth);
	void move(Overlay*, int x, int y);
	void detach(Overlay*);
	void damage(onode*);
	onode* overlays_;
};

class Compositor422 : public Compositor {
public:
	Compositor422();
	virtual int consume(const VideoFrame*);
	void size(int w, int h);
	void copy_block(u_char* ofrm, u_char* ochm, 
			const u_char* frm, const u_char* chm);
	void composite(Overlay* o, int x, int y);
};

class Compositor420 : public Compositor {
public:
	Compositor420();
	virtual int consume(const VideoFrame*);
	void size(int w, int h);
	void copy_block(u_char* ofrm, u_char* ochm, 
			const u_char* frm, const u_char* chm);
	void composite(Overlay* o, int x, int y);
};

static class CompositorMatcher : public Matcher {
public:
	CompositorMatcher() : Matcher("module") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "compositor/422") == 0)
			return (new Compositor422);
		if (strcasecmp(fmt, "compositor/420") == 0)
			return (new Compositor420);
		return (0);
	}
} compositor;

//SV-XXX: rearrange initialisation order to shut up gcc4
Overlay::Overlay() : width_(0), height_(0), image_(0), transparent_(0)
{
}

int Overlay::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "transparent") == 0) {
			transparent_ = atoi(argv[2]);
			return (TCL_OK);
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "load") == 0) {
			const char* file = argv[2];
			int w = atoi(argv[3]);
			int h = atoi(argv[4]);
			if (load(file, w, h) < 0) {
				tcl.result("overlay load() returned negative");
				return (TCL_ERROR);
			}
			else
				tcl.result("0");
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

int Overlay::load(const char* file, int w, int h)
{
	delete[] image_; //SV-XXX: Debian
	image_ = 0;
	int fd = open(file, O_RDONLY);
	if (fd < 0)
		return (-1);
	width_ = w;
	height_ = h;
	int s = 2 * w * h;
	image_ = new u_char[s];
	int cc = read(fd, image_, s);
	if (cc != s) {
		delete[] image_; //SV-XXX: Debian
		image_ = 0;
		return (-1);
	}
	close(fd);
	return (0);
}

Compositor::Compositor(int ft)
	: Module(ft), frm_(0), framebase_(0), damage_(0)
{
	width_ = 0;
	height_ = 0;
	framesize_ = 0;
	overlays_ = 0;
}

Compositor::~Compositor()
{
	delete[] framebase_; //SV-XXX: Debian
	delete[] damage_; //SV-XXX: Debian
	onode* p = overlays_;
	while (p != 0) {
		onode* n = p->next;
		delete p;
		p = n;
	}
	overlays_=0;
}

int Compositor::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "reset") == 0) {
			reset();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "detach") == 0) {
			Overlay* o = (Overlay*)TclObject::lookup(argv[2]);
			if (o == 0) {
				tcl.result("no such overlay");
				return (TCL_ERROR);
			}
			detach(o);
			return (TCL_OK);
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "move") == 0) {
			Overlay* o = (Overlay*)TclObject::lookup(argv[2]);
			if (o == 0) {
				tcl.result("no such overlay");
				return (TCL_ERROR);
			}
			int x = atoi(argv[3]);
			int y = atoi(argv[4]);
			move(o, x, y);
			return (TCL_OK);
		}
	} else if (argc == 6) {
		if (strcmp(argv[1], "attach") == 0) {
			Overlay* o = (Overlay*)TclObject::lookup(argv[2]);
			if (o == 0) {
				tcl.result("no such overlay");
				return (TCL_ERROR);
			}
			int x = atoi(argv[3]);
			int y = atoi(argv[4]);
			int depth = atoi(argv[5]);
			attach(o, x, y, depth);
			return (TCL_OK);
		}
	}
	return (Module::command(argc, argv));
}

void Compositor::reset()
{
	while (overlays_)
		detach(overlays_->overlay);
}

void Compositor::attach(Overlay* o, int x, int y, int depth)
{
	x &=~ 1;
	y &=~ 1;
	onode* p = new onode;
	p->overlay = o;
	p->x = x;
	p->y = y;
	p->depth = depth; //SV-XXX: there was no assignment, just p->depth !!!
	onode** op;
	for (op = &overlays_; *op != 0; op = &(*op)->next)
		if (depth > (*op)->depth)
			break;
	p->next = *op;
	*op = p;
	damage(p);
}

void Compositor::detach(Overlay* o)
{
	UNUSED(o); //SV-XXX: unused

	for (onode** op = &overlays_; *op != 0; op = &(*op)->next) {
		onode* p = (*op);
		if (p->overlay == o) {
			//damage(p);
			*op = p->next;
			delete p;
			return;
		}
	}
}

void Compositor::move(Overlay* o, int x, int y)
{
	x &=~ 1;
	y &=~ 1;
	for (onode* p = overlays_; p != 0; p = p->next) {
		if (p->overlay == o) {
			damage(p);
			p->x = x;
			p->y = y;
			damage(p);
			return;
		}
	}
}

/*
 * the overlay in onode will be added, deleted or moved.
 * update the damage vector so we send the corresponding
 * blocks to reflect change.
 */
void Compositor::damage(onode* on)
{
	int blkw = width_ >> 4;
	int blkh = height_ >> 4;
	int bx = on->x >> 4;
	int bw = ((on->x + on->overlay->width() + 15) >> 4) - bx;
	if (bx + bw > blkw)
		bw = blkw - bx;
	int by = on->y >> 4;
	int bh = ((on->y + on->overlay->height() + 15) >> 4) - by;
	if (by + bh > blkh)
		bh = blkh - by;

	u_char* p = &damage_[blkw * by + bx];
	while (--bh >= 0) {
		for (int k = 0; k < bw; ++k)
			p[k] = 1;
		p += blkw;
	}
}

void Compositor::crinit(int w, int h)
{
	int blkw = w >> 4;
	int blkh = h >> 4;
	int n = blkw * blkh;
	delete[] damage_; //SV-XXX: Debian
	damage_ = new u_char[n];
	memset(damage_, 0, n);
}

Compositor422::Compositor422() : Compositor(FT_YUV_422)
{
}

/*XXX*/
#define GRABBER_VPAD 1
void Compositor422::size(int w, int h)
{
	Module::size(w, h);
	int n = 2 * framesize_ + 2 * GRABBER_VPAD * w;
	delete[] framebase_; //SV-XXX: Debian
	framebase_ = new u_char[n];
	frm_ = framebase_ + GRABBER_VPAD * w;
	Compositor::crinit(w, h);
}

void Compositor422::copy_block(u_char* ofrm, u_char* ochm, 
			 const u_char* frm, const u_char* chm)
{
	int stride = width_;
	int k;
	for (k = 16; --k >= 0; ) {
		*(int32_t*)&ofrm[0] = *(int32_t*)&frm[0];
		*(int32_t*)&ofrm[4] = *(int32_t*)&frm[4];
		*(int32_t*)&ofrm[8] = *(int32_t*)&frm[8];
		*(int32_t*)&ofrm[12] = *(int32_t*)&frm[12];
		ofrm += stride;
		frm += stride;
	}
	stride >>= 1;
	for (k = 16; --k >= 0; ) {
		*(int32_t*)&ochm[0] = *(int32_t*)&chm[0];
		*(int32_t*)&ochm[4] = *(int32_t*)&chm[4];
		ochm += stride;
		chm += stride;
	}
	ochm -= stride << 4;
	ochm += framesize_ >> 1;
	chm -= stride << 4;
	chm += framesize_ >> 1;
	for (k = 16; --k >= 0; ) {
		*(int32_t*)&ochm[0] = *(int32_t*)&chm[0];
		*(int32_t*)&ochm[4] = *(int32_t*)&chm[4];
		ochm += stride;
		chm += stride;
	}
}

int Compositor422::consume(const VideoFrame* vf)
{
	if (!samesize(vf))
		size(vf->width_, vf->height_);
	YuvFrame* p = (YuvFrame*)vf;

	/*
	 * 1. update our copy of the frame
	 * 2. composite all compositor items (and update crvec)
	 */
	int blkw = width_ >> 4;
	int blkh = height_ >> 4;
	int blkno = 0;

	int loff = 0;
	int coff = 0;
	int fs = framesize_;
	u_int8_t* dam = damage_;
	const u_int8_t* crv = p->crvec_;
	u_int8_t* frm = p->bp_;
	for (int y = 0; y < blkh; ++y) {
		for (int x = 0; x < blkw; ++blkno, loff += 16, coff += 8,
		     ++x, ++crv, ++dam) {
			/* smash damage array into a crvec */
			int d = *dam;
			if (d)
				d = CR_SEND|CR_MOTION;
			else
				d = *crv;
			*dam = d;
			if (d & CR_SEND) {
				copy_block(frm_ + loff, frm_ + fs + coff,
					   frm + loff, frm + fs + coff);
			}
		}
		loff += 15 * width_;
		coff += 15 * (width_ >> 1);
	}
	for (onode* o = overlays_; o != 0; o = o->next) 
		composite(o->overlay, o->x, o->y);
	YuvFrame nf(p->ts_, frm_, damage_, p->width_, p->height_);
	int cc = target_->consume(&nf);
	memset(damage_, 0, blkw * blkh);
	return (cc);
}

void Compositor422::composite(Overlay* o, int x, int y)
{
	if (x >= width_ || y >= height_)
		return;

	int off = y * width_ + x;
	u_char* yp = frm_ + off;
	u_char* up = frm_ + framesize_ + (off >> 1);
	u_char* vp = up + (framesize_ >> 1);
	int w = o->width();
	int h = o->height();
	if (x + w > width_)
		w = width_ - x;
	if (y + h > height_)
		h = height_ - y;
	const u_char* p = o->image();
	if (p == 0)
		return;
	int trans = o->transparent();
	while (--h >= 0) {
		/* two pixels at a time */
		for (int k = 0; k < w; k += 2) {
			if (p[0] != trans) {
				yp[k] = p[0];
				yp[k + 1] = p[2];
				up[k >> 1] = p[1];
				vp[k >> 1] = p[3];
			}
			p += 4;
		}
		yp += width_;
		up += width_ >> 1;
		vp += width_ >> 1;
		p += (o->width() - w) << 1;
	}
}

Compositor420::Compositor420() : Compositor(FT_YUV_420)
{
}

void Compositor420::size(int w, int h)
{
	Module::size(w, h);
	int fs = framesize_;
	int n = fs + (fs >> 1) + 2 * GRABBER_VPAD * w;
	delete[] framebase_; //SV-XXX: Debian
	framebase_ = new u_char[n];
	frm_ = framebase_ + GRABBER_VPAD * w;
	Compositor::crinit(w, h);
}

void Compositor420::copy_block(u_char* ofrm, u_char* ochm, 
			    const u_char* frm, const u_char* chm)
{
	int stride = width_;
	int k;
	for (k = 16; --k >= 0; ) {
		*(int32_t*)&ofrm[0] = *(int32_t*)&frm[0];
		*(int32_t*)&ofrm[4] = *(int32_t*)&frm[4];
		*(int32_t*)&ofrm[8] = *(int32_t*)&frm[8];
		*(int32_t*)&ofrm[12] = *(int32_t*)&frm[12];
		ofrm += stride;
		frm += stride;
	}
	stride >>= 1;
	for (k = 8; --k >= 0; ) {
		*(int32_t*)&ochm[0] = *(int32_t*)&chm[0];
		*(int32_t*)&ochm[4] = *(int32_t*)&chm[4];
		ochm += stride;
		chm += stride;
	}
	ochm -= stride << 3;
	ochm += framesize_ >> 2;
	chm -= stride << 3;
	chm += framesize_ >> 2;
	for (k = 8; --k >= 0; ) {
		*(int32_t*)&ochm[0] = *(int32_t*)&chm[0];
		*(int32_t*)&ochm[4] = *(int32_t*)&chm[4];
		ochm += stride;
		chm += stride;
	}
}

int Compositor420::consume(const VideoFrame* vf)
{
	if (!samesize(vf))
		size(vf->width_, vf->height_);
	YuvFrame* p = (YuvFrame*)vf;

	int blkw = width_ >> 4;
	int blkh = height_ >> 4;
	int blkno = 0;

	int loff = 0;
	int coff = 0;
	int fs = framesize_;
	u_char* dam = damage_;
	const u_int8_t* crv = p->crvec_;
	u_int8_t* frm = p->bp_;
	for (int y = 0; y < blkh; ++y) {
		for (int x = 0; x < blkw; ++blkno, loff += 16, coff += 8,
		     ++x, ++crv, ++dam) {
			/* smash damage array into a crvec */
			int d = *dam;
			if (d)
				d = CR_SEND|CR_MOTION;
			else
				d = *crv;
			*dam = d;
			if (d & CR_SEND) {
				copy_block(frm_ + loff, frm_ + fs + coff,
					   frm + loff, frm + fs + coff);
			}
		}
		loff += 15 * width_;
		coff += 7 * (width_ >> 1);
	}
	for (onode* o = overlays_; o != 0; o = o->next) 
		composite(o->overlay, o->x, o->y);

	YuvFrame nf(p->ts_, frm_, damage_, p->width_, p->height_);
	int cc = target_->consume(&nf);
	memset(damage_, 0, blkw * blkh);
	return (cc);
}

void Compositor420::composite(Overlay* o, int x, int y)
{
	if (x >= width_ || y >= height_)
		return;

	int off = y * width_;
	u_char* yp = frm_ + off + x;
	off = (off >> 2) + (x >> 1);
	u_char* up = frm_ + framesize_ + off;
	u_char* vp = up + (framesize_ >> 2);
	int ovw = o->width();
	int w = ovw;
	int h = o->height();
	if (x + w > width_)
		w = width_ - x;
	if (y + h > height_)
		h = height_ - y;
	const u_char* p = o->image();
	if (p == 0)
		return;
	int trans = o->transparent();

	/* convert to byte offset */
	ovw <<= 1;

	for (; h > 0; h -= 2) {
		/* two pixels at a time, both horiz and vertically */
		for (int k = 0; k < w; k += 2) {
			if (p[0] != trans) {
				yp[k] = p[0];
				yp[k + width_] = p[ovw];
				yp[k + 1] = p[2];
				yp[k + 1 + width_] = p[ovw + 2];
				up[k >> 1] = p[1];
				vp[k >> 1] = p[3];
			}
			p += 4;
		}
		yp += width_ << 1;
		up += width_ >> 1;
		vp += width_ >> 1;
		p += (ovw - w) << 1;
	}
}
