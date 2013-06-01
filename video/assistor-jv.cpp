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
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>

#include "jvs.h"
#include "inet.h"
#include "vic_tcl.h"
#include "rtp.h"
#include "decoder.h"
#include "renderer.h"
#include "vw.h"
#include "iohandler.h"
#include "device-output.h"

extern "C" {
#include <sys/shm.h>
#ifdef mips
#include <mips/cachectl.h>
int cacheflush(char* addr, int len, int cache);
#endif
}

class JvDecoder : public IOHandler {
protected:
	JvDecoder();
public:
	~JvDecoder();
	void decode(const JpegFrame*);
	inline int valid() const { return (server_ >= 0); }
protected:
	struct pbuf {
		dmabuf db;
		int len;
		pbuf* next;
	};
	virtual void start() = 0;
	virtual void reset(int inq, int inw, int inh) = 0;

	void decompress();
	int waitd();

	int setcolors(int ncolor);

	int decompressing_;	/* true when decomp request is pending */
	int rw_;		/* width of rendered image */
	int rh_;		/* height of rendered image */
	int inq_;		/* input quantization */
	int inw_;		/* input width */
	int inh_;		/* input height */
	int server_;		/* J-Video server */
	int pad_;		/* horizontal padding for output image */

	void allocdma(dmabuf&, int type, int w, int h, int rmid = 1);
	void freedma(dmabuf&);

	pbuf pbase_[3];
	pbuf* pf_;
	pbuf* pi_;
	pbuf* pbd_;

	void enque(pbuf* pb);
	pbuf* deque();
	pbuf* qh_;
	pbuf* qt_;
};

class JvWindowAssistor : public Renderer, public JvDecoder {
public:
	JvWindowAssistor();
	~JvWindowAssistor();
	int command(int argc, const char*const* argv);
protected:
	virtual int consume(const VideoFrame* vf) {
		decode((JpegFrame*)vf);
		need_update_ = 0;
		return (0);
	}
	virtual void dispatch(int);
	void reset(int inq, int inw, int inh);
	void start();
	void allocimage();
	int imagesize_;
	SharedVideoImage* vbd_;
	SharedVideoImage* vbx_;
	dmabuf vb0_;
	dmabuf vb1_;
	VideoWindow* window_;

};

static class JvWindowAssistorMatcher : public Matcher {
public:
	JvWindowAssistorMatcher() : Matcher("assistor") {}
	TclObject* match(const char* id) {
		if (strcmp(id, "jpeg/422") == 0) {
			JvWindowAssistor* p = new JvWindowAssistor();
			if (p->valid())
				return (p);
			delete p;
		}
		return (0);
	}
} assistor_jv;

static int jvport()
{
	char *ports = getenv("JVOUT_PORT");
	int port = 0;
	if (ports)
		sscanf(ports, ":%d", &port);
	port += JVS_SOCKET;
	return (port);
}

JvDecoder::JvDecoder()
	: inq_(20), inw_(0), inh_(0), rw_(-1), rh_(-1),
	  decompressing_(0), qh_(0), qt_(0)
{
	/*
	 * Open server connection.  Note that we
	 * create one connection per video window.
	 * We need to do this because the J-Video
	 * server caches the decoding state on
	 * a per socket basis.  For instance, the
	 * server automatically reprograms the
	 * hardware scaling unit each time we issue
	 * a decompression request on a different socket.
	 */
	server_ = JvsOpen("", jvport());
	if (server_ < 0)
		return;

	/*XXX width/height bogus*/
	allocdma(pbase_[0].db, JVS_JPEG, 640, 480);
	allocdma(pbase_[1].db, JVS_JPEG, 640, 480);
	allocdma(pbase_[2].db, JVS_JPEG, 640, 480);

	pbase_[0].next = 0;
	pbase_[1].next = &pbase_[0];
	pf_ = &pbase_[1];
	pi_ = &pbase_[2];

	/*
	 * If jvdriver already has an installed colormap, use those
	 * colors.  Otherwise, set our default.
	 */
	if (setcolors(0) == 0)
		setcolors(atoi(Tcl::instance().attr("jvColors")));

	link(server_, TK_READABLE);
}

JvDecoder::~JvDecoder()
{
	if (server_ >= 0) {
		if (decompressing_)
			(void)waitd();
		freedma(pbase_[0].db);
		freedma(pbase_[1].db);
		freedma(pbase_[2].db);
		/* ~IOHandler() will unlink us */
		(void)close(server_);
	}
}

int JvDecoder::setcolors(int ncolors)
{
	int mono = 0;
	if (JvsColormap(server_, ":0", Tk_Colormap(Tcl::instance().tkmain()),
			&ncolors, mono) < 0) {
		fprintf(stderr, "vic: JvsColormap failed\n");
		/*XXX*/
		exit(1);
	}
	return (ncolors);
}

void JvDecoder::allocdma(dmabuf& d, int type, int w, int h, int rmid)
{
	if (JvsAllocateBuf(server_, JVS_OUTPUT, type, w, h, &d.shmid) < 0) {
		fprintf(stderr, "vic: JvsAllocateBuf failed\n");
		exit(1);
	}
	d.bp = (u_char *)shmat(d.shmid, 0, 0);
	if (d.bp == (u_char*)-1) {
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
	 * a quick glance at the kernel code for the shmctl()
	 * system call verifies it.
	 *
	 * we don't always do this -- if we need to share them
	 * with X we must do the XShmAttach first.
	 * This might file since SharedVideoImage does it too.
	 */
	if (rmid)
		(void)shmctl(d.shmid, IPC_RMID, 0);
}

void JvDecoder::freedma(dmabuf& d)
{
	(void)JvsDeallocBuf(server_, d.shmid);
	if (shmdt((char*)d.bp) < 0)
		perror("shmdt");
}

void JvDecoder::decompress()
{
	decompressing_ = 1;
	pbd_ = deque();
	start();
}

/*
 * Wait for the most recently issued J-Video decompression
 * request to finish.  On exit, vbd is null, vbx contains
 * the image being copied to the framebuffer.  Returns 0
 * if vbx is a new image, or -1 if vbx is an old image (e.g.,
 * the last ShmPutImage has not finished yet).
 */
int JvDecoder::waitd()
{
	decompressing_ = 0;
	/*
	 * Decompression is done.  Put the packet
	 * buffer back on the free list.
	 */
	pbd_->next = pf_;
	pf_ = pbd_;
	pbd_ = 0;

	int cshmid;
	int dshmid;
	if (JvsWaitDecomp(server_, &cshmid, &dshmid) < 0) {
		/*
		 * The decompression timed out.
		 * The hardware can be flaky.
		 * Oh well, don't do anything drastic.
		 */
		fprintf(stderr, "vic: JvsWaitDecomp failed\n");/*XXX*/
		
		return (-1);
	}
	/*XXX sanity check shmid's? */
	return (0);
}

void JvDecoder::enque(pbuf* pb)
{
	if (qh_ == 0)
		qh_ = qt_ = pb;
	else {
		qt_->next = pb;
		qt_ = pb;
	}
	pb->next = 0;
	if (!decompressing_)
		decompress();
}

JvDecoder::pbuf* JvDecoder::deque()
{
	pbuf* p = qh_;
	qh_ = p->next;
	return (p);
}

void JvDecoder::decode(const JpegFrame* p)
{
	/*XXX need to handle p->type changes too!*/
	int w = p->width_;
	int h = p->height_;
	if (p->q_ != inq_ || w != inw_ || h != inh_)
		reset(p->q_, w, h);

	/*
	 * Copy packet data into a dma buffer.  If we had a more
	 * intelligent buffering scheme, we could make sure
	 * reassembly occured in-place.  But copying a compressed
	 * frame an extra time is noise compared to copying
	 * the uncompressed data which happens at least once
	 * (to the frame buffer).
	 */
	memcpy((char*)pi_->db.bp, (char*)p->bp_, p->len_);
	pi_->len = p->len_;
	/*XXX this is a gross test condition */
	if (pf_ != 0) {
		enque(pi_);
		pi_ = pf_;
		pf_ = pf_->next;
	}
}

JvWindowAssistor::JvWindowAssistor()
	: Renderer(FT_JPEG), imagesize_(0), vbx_(0), vbd_(0), window_(0)
{
}

JvWindowAssistor::~JvWindowAssistor()
{
	if (vbx_ != 0) {
		/*
		 * Clear out the image so that the Video class does
		 * not try to use it before another window gets attached
		 * and replaces the image.
		 */
		window_->setimage(0);

		if (decompressing_)
			(void)waitd();
		/*XXX*/
		int s0 = vb0_.shmid;
		int s1 = vb1_.shmid;
		delete vbx_;
		delete vbd_;
		(void)JvsDeallocBuf(server_, s0);
		(void)JvsDeallocBuf(server_, s1);
	}
}

int JvWindowAssistor::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "window") == 0) {
			window_ = VideoWindow::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return (Renderer::command(argc, argv));
}

void JvWindowAssistor::allocimage()
{
	if (vbx_ != 0) {
		/*XXX*/
		int s0 = vb0_.shmid;
		int s1 = vb1_.shmid;
		if (JvsDeallocBuf(server_, s0) < 0 || 
		    JvsDeallocBuf(server_, s1) < 0) {
			fprintf(stderr, "vic: JvsDeallocBuf failed\n");
			/*XXX*/
			exit(1);
		}
		delete vbx_;
		delete vbd_;
	}
	Tk_Window tk = Tcl::instance().tkmain();

	imagesize_ = (rw_ + pad_) * rh_;
	allocdma(vb0_, JVS_DITHERED, rw_ + pad_, rh_, 0);
	allocdma(vb1_, JVS_DITHERED, rw_ + pad_, rh_, 0);
	vbd_ = new SharedVideoImage(tk, rw_ + pad_, rh_, vb0_.bp, vb0_.shmid);
	vbx_ = new SharedVideoImage(tk, rw_ + pad_, rh_, vb1_.bp, vb1_.shmid);

	/*
	 * Once the X server has attached the shm segments,
	 * we rmid them so they will go away when we exit.
	 * The sync is to make the X server do the attach
	 * before we do the rmid.
	 */
	XSync(Tk_Display(tk), 0);
	(void)shmctl(vbx_->shmid(), IPC_RMID, 0);
	(void)shmctl(vbd_->shmid(), IPC_RMID, 0);
}

void JvWindowAssistor::start()
{
	if (JvsStartDecomp(server_, pbd_->db.shmid, vbd_->shmid(),
			   pbd_->len) < 0)
		fprintf(stderr, "vic: JvsStartDecomp failed\n");
#ifdef mips
	if (cacheflush((char*)vbd_->buffer(), imagesize_, DCACHE) < 0) {
		perror("cacheflush");
		exit(1);
	}
#endif
}

void JvWindowAssistor::dispatch(int)
{
	if (waitd() >= 0) {
		/*
		 * Swap the decompression and displayed
		 * images, and fire off the XShmPutImage.
		 */
		SharedVideoImage* t = vbx_;
		vbx_ = vbd_;
		vbd_ = t;

		window_->render(vbx_);
		window_->complete();
	}
	/*
	 * If another frame is ready, continue decompression.
	 */
	if (qh_ != 0)
		decompress();
}

void JvWindowAssistor::reset(int inq, int inw, int inh)
{
	if (decompressing_)
		waitd();

	/*
	 * Make sure all outstanding putimage's finish.
	 * Syncing with the first window, syncs the server,
	 * and hence all of them.
	 */
	window_->sync();
	
	Module::size(inw, inh);
	inw_ = inw;
	inh_ = inh;
	inq_ = inq;

	int w = window_->width();
	int h = window_->height();

	if (JvsSetDecomp(server_, inq_, inw_, inh_,
			 &w, &h, 8, 0, 0, &pad_) < 0)
		fprintf(stderr, "vic: JvsSetDecomp failed\n");

	if (w != rw_ || h != rh_) {
		rw_ = w;
		rh_ = h;
		allocimage();
	}
	if (qh_ != 0)
		decompress();
}

/*
 * A class for sending JPEG compressed video to the external
 * analog port of the j300.  The hardware supports an uncompressed
 * video path as well, but jv2driver does not implement it.
 * Also, this outboard path and the capture path share frame
 * memory on the board, so they can't run simultaneously.
 * We don't bother checking.  Instead, the user will just
 * get bad results.
 *
 */
class JvOutboardAssistor : public Renderer, public JvDecoder {
public:
	inline JvOutboardAssistor() : Renderer(FT_JPEG) {}
protected:
	virtual int command(int argc, const char*const* argv);
	virtual void dispatch(int);
	void start();
	void reset(int inq, int inw, int inh);
	virtual int consume(const VideoFrame* vf) {
		decode((JpegFrame*)vf);
		need_update_ = 0;
		return (0);
	}
};

int JvOutboardAssistor::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "scale") == 0) {
			/* ignore */
			return (TCL_OK);
		}
	}
	return (Renderer::command(argc, argv));
}


void JvOutboardAssistor::dispatch(int)
{
	(void)waitd();
	/*
	 * If another frame is ready, continue decompression.
	 */
	if (qh_ != 0)
		decompress();
}

void JvOutboardAssistor::start()
{
	if (JvsStartAnalogDecomp(server_, pbd_->db.shmid,
				 pbd_->len) < 0)
		fprintf(stderr, "vic: JvsStartAnalogDecomp failed\n");
}

void JvOutboardAssistor::reset(int inq, int inw, int inh)
{
	if (decompressing_)
		waitd();

	inq_ = inq;
	inw_ = inw;
	inh_ = inh;
	Module::size(inw, inh);

	if (JvsSetDecomp(server_, inq_, inw_, inh_,
			 &inw, &inh, 8, 0, 0, &pad_) < 0)
		fprintf(stderr, "vic: JvsSetDecomp failed\n");

	if (qh_ != 0)
		decompress();
}

static class JvOutputDevice : public OutputDevice {
 public:
	JvOutputDevice() : OutputDevice("J300") {}
	virtual int command(int argc, const char*const* argv);
} jv_output;

int JvOutputDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "assistor") == 0 &&
		    strcmp(argv[2], "jpeg/422") == 0) {
			JvOutboardAssistor* p = new JvOutboardAssistor();
			if (p->valid())
				tcl.result(p->name());
			else
				delete p;
			return (TCL_OK);
		}
	}
	return (OutputDevice::command(argc, argv));
}
