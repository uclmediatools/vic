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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#ifndef __DECCXX
#include <osfcn.h>
#endif
#include <sys/file.h>
#include <sys/stat.h>

extern "C" {
/*XXX*/
#undef VOID
#include <mme/mme_api.h>
}

#include "inet.h"
#include "vic_tcl.h"
#include "rtp.h"
#include "grabber.h"
#include "iohandler.h"
#include "device-input.h"
#include "crdef.h"
#include "module.h"

#define NTSC_WIDTH 640
#define NTSC_HEIGHT 480

extern "C" {
#include <tcl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
}

class MmeGrabber : public Grabber, public IOHandler {
public:
	MmeGrabber();
	virtual ~MmeGrabber();
	virtual int command(int argc, const char*const* argv);
	virtual void fps(int);
	virtual void start();
	virtual void stop();
protected:
	virtual void SetComp() = 0;
	void setport(const char *port);
	void setstandard(const char *standard);
	virtual void dispatch(int mask);
	virtual void timeout();
	virtual int consume(const u_char* dmabuf, int length) = 0;
	virtual const char* type() const;
	virtual void startup();
	virtual void shutdown();
	virtual int grab();
	inline int mme_error(MMRESULT, const char* msg) const;
	static int server_;	/* fd of server socket */
	HVIDEO handle_;		/* handle for video connection */
	int x_decimate_;	/* horizontal downsampling factor */
	int y_decimate_;	/* vertical downsampling factor */
	struct info {
		EXBMINFOHEADER b;
		JPEGINFOHEADER jpeg;
	} *bmh;
private:
	void init();
	VIDEOHDR* vh_;
	static void mme_callback(HVIDEO handle, DWORD code, DWORD instance,
				 LPARAM p1, LPARAM p2);
	int callback(int code, VIDEOHDR* vh);
	void waitgrab();

	int port_;
	int standard_;
	int fps_;
	int delay_;
	int last_grab_;
	volatile int outstanding_;

	volatile int running_;
	volatile int valid_;
	volatile int wantgrab_;
	int buffers_;
};

class MmeJpegGrabber : public MmeGrabber {
public:
	MmeJpegGrabber();
	virtual ~MmeJpegGrabber();
	virtual int command(int argc, const char*const* argv);
protected:
	int consume(const u_char* buffer, int length);
	virtual void SetComp();
private:
	int q_;
	int nq_;
};

class MmeYuvGrabber : public MmeGrabber {
public:
	MmeYuvGrabber();
	virtual void SetComp();
	virtual int consume(const u_char* buffer, int length);
protected:
	void suppress(const u_char* devbuf, const u_char* lumbuf);
	virtual void saveblks(const u_char* in) = 0;
	virtual void setsize(int w, int h) = 0;
};

class Mme422Grabber : public MmeYuvGrabber {
protected:
	virtual void saveblks(const u_char* in);
	void saveblk(const u_char* in, u_char* yp, u_char* up,
		     u_char* vp, int stride);
	void setsize(int w, int h);
};

class MmeCIFGrabber : public MmeYuvGrabber {
protected:
	virtual void saveblks(const u_char* in);
	void saveblk(const u_char* in, u_char* yp, u_char* up,
		     u_char* vp, int stride, int istride);
	void setsize(int w, int h);
};


class Mme420Grabber : public MmeCIFGrabber {
    protected:
	virtual void setsize(int xsize, int ysize);
};

class MmeDevice : public InputDevice {
 public:
	MmeDevice(const char* name);
	virtual int command(int argc, const char*const* argv);
	static int havedev();
};

/***********************************************************************/

static MmeDevice mme_device("mme");

int MmeDevice::havedev()
{
    /* tfm - need to do a more thorough check here */
    if( videoGetNumDevs() > 0 )
      return (1);
    else
      return (0);
}

MmeDevice::MmeDevice(const char* name) : InputDevice(name)
{
    /* fprintf(stderr, "vic: Called MmeDevice::MmeDevice\n"); */
    if (havedev())
      attributes_ = "format { 422 jpeg } size { small large cif } port {Comp-1 Comp-2 S-Video525 S-Video625}";
    else
      attributes_ = "disabled";
}

int MmeDevice::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();
    if (argc == 3) {
	if(strcmp(argv[1], "open") == 0) {
	    const char* fmt = argv[2];
	    TclObject* o = 0;
	    /* fprintf(stderr, "vic: Called MmeDevice::command with %s %s\n", 
		    argv[1],argv[2]); */
	    if (strcmp(fmt, "jpeg") == 0)
	      o = new MmeJpegGrabber;
	    else if (strcmp(fmt, "422") == 0)
	      o = new Mme422Grabber;
	    else if (strcmp(fmt, "420") == 0)
	      o = new Mme420Grabber;
	    else if (strcmp(fmt, "cif") == 0)
	      o = new MmeCIFGrabber;
	    if (o != 0) {
		Tcl::instance().result(o->name());
		return (TCL_OK);
	    }
	}
    }
    return (InputDevice::command(argc, argv));
}


/***********************************************************************/

MmeGrabber* mmeMaster;


MmeGrabber::MmeGrabber() 
{
	/* fprintf(stderr, "vic: Called MmeGrabber constructor\n"); */
	mmeMaster = this;

	/*XXX defer videoOpen until start? */
	if (videoGetNumDevs() <= 0 || 
	    videoOpen(&handle_, 0, VIDEO_IN) != DV_ERR_OK) {
		valid_ = 0;
		fprintf(stderr,"vic : Failed to open video device\n");
		return;
	}

	port_=1;
	standard_=VIDEO_STANDARD_NTSC;

	valid_ = 1;
	running_ = 0;
	wantgrab_ = 0;
	buffers_ = 1;
    
	fps(5);
	init();
}

MmeGrabber::~MmeGrabber()
{
    /* fprintf(stderr, "vic: Called MmeGrabber::~MmeGrabber\n"); */
	if (valid_) {
		if (running_)
			stop();
		mmeFreeMem(vh_);
		videoClose(handle_);
	}
}

const char* MmeGrabber::type() const
{
	return ("mme");
}

int MmeGrabber::server_ = -1;

void MmeGrabber::init()
{
	int size;
	MMRESULT s;

    /* fprintf(stderr, "vic: Called MmeGrabber::init\n"); */
	/*
	 * Allocate & zero some memory for the video header and bitmap
	 * info header.  We allocate it as a single block to keep it
	 * all in one shared memory segment.
	 * (Make sure everything's aligned on quadword boundaries)
	 */
	size = ((sizeof(*bmh)+sizeof(void*)-1) / sizeof(void*)) * sizeof(void*);
	bmh = (info*)mmeAllocMem(size+sizeof(*vh_));
	if (bmh == 0) {
		fprintf(stderr, "vic: cannot allocate mme memory\n");
		valid_ = 0;
		return;
	}
	memset(bmh, 0, size+sizeof(*vh_));
	vh_ = (VIDEOHDR*)((char*)bmh+size);


	if (server_ < 0) {
		server_ = mmeServerFileDescriptor();
		link(server_, TK_READABLE);
	}

	return;
 bail:
	mmeFreeMem(bmh);
	bmh = 0;
	videoClose(handle_);
	vh_ = 0;
	valid_ = 0;
	/*XXX*/
}

/* Override Grabber's timeout method with an empty method */
void MmeGrabber::timeout()
{
}

int MmeGrabber::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();
    if (argc == 3) {
	if(strcmp(argv[1], "decimate") == 0) {
	    int d = atoi(argv[2]);
	    /* fprintf(stderr, "vic: Called MmeGrabber::command with %s %s\n",
		    argv[1],argv[2]); */
	    if (d == 0) {
		tcl.resultf("%s: divide by zero", argv[0], 0);
		return (TCL_ERROR);
	    }
	    x_decimate_ = d;
	    y_decimate_ = d;
	    if(running_) {
		shutdown();
		SetComp();
		startup();
	    }
	    else
	      SetComp();
	    
	    return (TCL_OK);
	}
	else if (strcmp(argv[1], "port") == 0) {
	    setport(argv[2]);
	    return(TCL_OK);
	}
	else if (strcmp(argv[1], "type") == 0) {
	    setstandard(argv[2]);
	    return(TCL_OK);
	}
    }
    return (Grabber::command(argc, argv));
}

void MmeGrabber::start()
{
    /* fprintf(stderr, "vic: Called MmeGrabber::start\n"); */
	startup();
	Grabber::start();
}

void MmeGrabber::stop()
{
    /* fprintf(stderr, "vic: Called MmeGrabber::stop\n"); */
	shutdown();
	Grabber::stop();
}

void MmeGrabber::fps(int f)
{
	/* fprintf(stderr, "vic: Called MmeGrabber::fps\n"); */
        fps_ = f;
	delay_ = 1000 / f;
	Grabber::fps(f);
}


void MmeGrabber::setport(const char *port)
{
    if(port) {
	if(strcmp(port,"Comp-1") == 0)
	  port_=1;
	else if(strcmp(port,"Comp-2") == 0)
	  port_=2;
	else if(strcmp(port,"S-Video525") == 0) {
	    port_ = 0;
	    standard_ = VIDEO_STANDARD_SVIDEO525;
	}
	else if(strcmp(port,"S-Video625") == 0) {
	    port_ = 0;
	    standard_ = VIDEO_STANDARD_SVIDEO625;
	}
    }
}
void MmeGrabber::setstandard(const char *standard)
{
    if(standard) {
	if(strcmp(standard,"ntsc") == 0) {
	    standard_=VIDEO_STANDARD_NTSC;
	    if(!port_)
	      port_=1;
	}
	else if(strcmp(standard,"pal") == 0) {
	    standard_=VIDEO_STANDARD_PAL;
	    if(!port_)
	      port_=1;
	}
	else if(strcmp(standard,"secam") == 0) {
	    standard_=VIDEO_STANDARD_SECAM;
	    if(!port_)
	      port_=1;
	}
	else if(strcmp(standard,"svideo525") == 0)
	  standard_=VIDEO_STANDARD_SVIDEO525;
	else if(strcmp(standard,"svideo625") == 0)
	  standard_=VIDEO_STANDARD_SVIDEO625;
    }
}

void MmeGrabber::startup()
{
    MMRESULT s;
    int size;
    
    /* fprintf(stderr, "vic: Called MmeGrabber::startup\n"); */
    
    if (running_)
      return;
    
    if(port_) {
	s=videoSetPortNum(handle_,port_);
	if (mme_error(s, "couldn't configure mme"))
	  goto bail;
    }

    if(standard_) {
	s=videoSetStandard(handle_,standard_);
	if (mme_error(s, "couldn't configure mme"))
	  goto bail;
    }

    s = videoConfigure(handle_, DVM_FORMAT,
		       VIDEO_CONFIGURE_GET|VIDEO_CONFIGURE_MIN,
		       0, bmh, sizeof(*bmh), 0, 0);
    size = bmh->b.bmi.biSizeImage;/*XXX ridiculously large*/
    s = videoConfigure(handle_, DVM_FORMAT, VIDEO_CONFIGURE_SET,
		       0, bmh, sizeof(*bmh), 0, 0);
    if (mme_error(s, "couldn't configure mme"))
      goto bail;
	
#ifdef notdef
	size = 320 * 240 * 3;
#endif
	vh_->lpData = (LPSTR)mmeAllocBuffer(size);
	if (vh_->lpData == 0) {
		fprintf(stderr, "vic: couldn't allocate mme frame memory\n");
		goto bail;
	}
	vh_->dwBufferLength = size;
	
	
	s = videoStreamInit(handle_, 1000000 /*10fps - ignored */,
			    mme_callback,
			    0, CALLBACK_FUNCTION);
	if (mme_error(s, "couldn't initialize mme stream\n"))
		return;
	s = videoStreamPrepareHeader(handle_, vh_, sizeof(*vh_));
	if (mme_error(s, "couldn't prepare mme video hdr"))
		return;
	s = videoStreamStart(handle_);
	if (mme_error(s, "couldn't start mme video stream\n"))
		return;
	running_ = 1;
	frameclock_ = gettimeofday_usecs();

	outstanding_ = 0;
	last_grab_ = 0;

	grab();
	return;

bail:
	mmeFreeMem(bmh);
        bmh = 0;
	videoClose(handle_);
	vh_ = 0;
	valid_ = 0;
}

void MmeGrabber::shutdown()
{
	MMRESULT s;

	/* fprintf(stderr, "vic: Called MmeGrabber::shutdown\n"); */
	if (!running_)
		return;

	wantgrab_ = 0;
	running_ = 0;
	s = videoStreamReset(handle_);
	(void)mme_error(s, "couldn't reset mme stream");
	while(outstanding_) {
		mmeWaitForCallbacks();
		mmeProcessCallbacks();
	}
	s = videoStreamUnprepareHeader(handle_, vh_, sizeof(*vh_));
	(void)mme_error(s, "couldn't unprepare mme video header");
	s = videoStreamFini(handle_);
	(void)mme_error(s, "couldn't shutdown mme stream");
	running_ = 0;


	mmeFreeBuffer(vh_->lpData);
}


/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

inline int MmeGrabber::mme_error(MMRESULT code, const char* msg) const
{
	if (code != DV_ERR_OK) {
	    char *buffer;
	    if(buffer=(char *)mmeAllocMem(MAXERRORLENGTH)) {
		videoGetErrorText(handle_,code,buffer,MAXERRORLENGTH);
		fprintf(stderr, "vic: %s - %s (%d)\n", msg, buffer, code);
		mmeFreeMem(buffer);
	    }

	    return (1);
	}
	return (0);
}

extern "C" {
void MmeGrabber::mme_callback(HVIDEO handle, DWORD code, DWORD instance,
			      LPARAM vh, LPARAM p2)
{
#ifdef notyet
	MmeGrabber* p = (MmeGrabber*)instance;
    fprintf(stderr, "vic: Called MmeGrabber::mme_callback\n");
	p->callback();
#else
	(void)mmeMaster->callback(code, (VIDEOHDR*)vh);
#endif
}
}

int MmeGrabber::callback(int code, VIDEOHDR *vh)
{
	int n;

	/* fprintf(stderr, "vic: Called MmeGrabber::callback\n"); */

        /* Nothing to do for open or close - just dismiss */
	if (code == MM_DRVM_OPEN || code == MM_DRVM_CLOSE)
		return (1);

	if (code != MM_DRVM_DATA) {
		fprintf(stderr,"vic: mme callback code %d\n", code);
		return (0);
	}

	if (vh->lpData != vh_->lpData)
		fprintf(stderr,"vic: buffer address changed in callback\n");
		
	if ((vh->dwFlags & VHDR_DONE) == 0 && running_) {
		fprintf(stderr, "vic: mme callback: done bit = 0\n");
	}

 	/* Check for wraparound */
	if( vh->dwTimeCaptured < last_grab_ )
		last_grab_ = last_grab_ - sizeof(vh->dwTimeCaptured)*8;

	/*
	** See if we want to keep this frame
	** The capture overhead is low enough that we just capture them all
	** then throw away the ones we don't want
	*/
	if(vh->dwTimeCaptured - last_grab_ > delay_ ) {
		last_grab_ = vh->dwTimeCaptured;
		int length = vh->dwBytesUsed;
		u_char* p = (u_char*)vh->lpData;
		n = consume(p, length ); 
		delay_ = (int)(tick(n)/1000);
	}

	outstanding_--;
	wantgrab_ = 1;

	return (1);
}

int MmeGrabber::grab()
{
    MMRESULT s;
    
    if(outstanding_ >= buffers_ )      
      /*  fprintf(stderr, " skipping grab\n") */ ;
    else {
	/* fprintf(stderr, " grab - outstanding = %d ",outstanding_); */
	/* tfm - add multibuffer code here */
	if(s = videoStreamAddBuffer(handle_, vh_, sizeof(*vh_)))
	  (void)mme_error(s, "couldn't queue mme buffer");
	else
	  outstanding_++;
    }
    return TRUE;
}

void MmeGrabber::dispatch(int mask)
{
    /*    fprintf(stderr, "vic: Called MmeGrabber::dispatch\n"); */
    while (mmeCheckForCallbacks())
      mmeProcessCallbacks();
    if (wantgrab_ && running_ ) {
	wantgrab_ = 0;
	mmeMaster->grab();
    }
}





/***********************************************************************/

MmeJpegGrabber::MmeJpegGrabber() : q_(80)
{
    /* fprintf(stderr, "vic: Called MmeJpegGrabber constructor\n"); */

}

MmeJpegGrabber::~MmeJpegGrabber()
{
    /* fprintf(stderr, "vic: Called MmeJpegGrabber destructor\n"); */

}

int MmeJpegGrabber::command(int argc, const char*const* argv)
{
    /* fprintf(stderr, "vic: Called MmeJpegGrabber::command\n"); */
	if (argc == 3) {
	    /* fprintf(stderr, "vic: Called MmeGrabber::command with %s %s\n",
		    argv[1],argv[2]); */
		if (strcmp(argv[1], "q") == 0) {
			/* assume value is in range */
			nq_ = atoi(argv[2]);
			if (nq_ != q_) {
			    q_ = nq_;
			    MMRESULT s = videoSetQuality(handle_, 
							 10000 * q_ / 100 );
			    (void)mme_error(s, "couldn't set quality\n");
			}

			return (TCL_OK);
		}
	}
	return (MmeGrabber::command(argc, argv));
}

int MmeJpegGrabber::consume(const u_char* p, int length)
{
/*    fprintf(stderr, "vic: Called MmeJpegGrabber::consume()\n"); */
	/*
	 * We should get a better timestamp XXX
	 */
	JpegFrame f(media_ts(), (u_char*)p, length, q_, 0, inw_, inh_);
	return (target_->consume(&f));
}

void MmeJpegGrabber::SetComp()
{
    /* fprintf(stderr,"vic: Called MmeJpegGrabber::SetComp()\n"); */


    inw_ = NTSC_WIDTH / x_decimate_;
    inh_ = NTSC_HEIGHT / y_decimate_;

    if( bmh == 0 )
      fprintf(stderr,"Bitmap infoheader pointer is null\n");
    memset(bmh, 0, sizeof(*bmh));
    bmh->b.bmi.biSize = sizeof(*bmh);
    bmh->b.bmi.biWidth = inw_;
    bmh->b.bmi.biHeight = inh_;
    bmh->b.bmi.biPlanes = 1;
    bmh->b.bmi.biBitCount = 24;
    bmh->b.bmi.biCompression = MJPG_DIB;
    bmh->b.biExtDataOffset = sizeof(bmh->b);
    bmh->jpeg.JPEGSize = sizeof(bmh->jpeg);
    bmh->jpeg.JPEGProcess = JPEG_PROCESS_BASELINE;
    bmh->jpeg.JPEGColorSpaceID = JPEG_YCbCr;
    bmh->jpeg.JPEGBitsPerSample = 8;
    bmh->jpeg.JPEGHSubSampling = 2;
    bmh->jpeg.JPEGVSubSampling = 1;

}


/***********************************************************************/



MmeYuvGrabber::MmeYuvGrabber()
{
    /* fprintf(stderr, "vic: Called MmeYuvGrabber constructor\n"); */
}

int MmeYuvGrabber::consume(const u_char* buffer, int length)
{
	suppress(buffer, frame_);
	saveblks(buffer);
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

void MmeYuvGrabber::suppress(const u_char* devbuf, const u_char* lumbuf)
{
	const u_char* start = frame_ + 16 * vstart_ * outw_ + 16 * hstart_;
	REPLENISH(devbuf, start, inw_ << 1, 2,
		  hstart_, hstop_, vstart_, vstop_);
}


void MmeYuvGrabber::SetComp()
{
    /* fprintf(stderr,"vic: Called MmeYUVGrabber::SetComp()\n"); */

    int w = NTSC_WIDTH / x_decimate_;
    int h = NTSC_HEIGHT / y_decimate_;
    if (w != inw_ || h != inh_) {
	setsize(w, h);
    }

    if( bmh == 0 )
      fprintf(stderr,"Bitmap infoheader pointer is null\n"); /* Uh oh! */
    else {
	memset(bmh, 0, sizeof(*bmh));
	bmh->b.bmi.biSize = sizeof(BITMAPINFOHEADER);
	bmh->b.bmi.biWidth = inw_;
	bmh->b.bmi.biHeight = inh_;
	bmh->b.bmi.biPlanes = 1;
	bmh->b.bmi.biBitCount = 16;
	bmh->b.bmi.biCompression = BICOMP_DECYUVDIB;
    }
}


/***********************************************************************/



/*
** Take YUV 4:2:2 packed data and unpack it into YUV 4:2:2 with each
** component in a separate block of memory
*/

inline void Mme422Grabber::saveblk(const u_char* in, u_char* yp, u_char* up,
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

void Mme422Grabber::saveblks(const u_char* in)
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

void Mme422Grabber::setsize(int w, int h)
{
	set_size_422(w, h);
}

/***********************************************************************/

void MmeCIFGrabber::setsize(int w, int h)
{
	set_size_cif(w, h);
}

/*
** Take YUV 4:2:2 packed data and unpack it into YUV 4:1:1 with each
** components in a separate block of memory.  
**
** NB: The chroma is subsampled with no prefiltering.  The samples are
**     cosited with the first of the pair of scan lines rather than
**     being positioned between them as specified for true CIF
*/

inline void MmeCIFGrabber::saveblk(const u_char* in, u_char* yp, u_char* up,
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

void MmeCIFGrabber::saveblks(const u_char* in)
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

/***********************************************************************/


void Mme420Grabber::setsize(int w, int h)
{
	set_size_420(w, h);
}

/***********************************************************************/

