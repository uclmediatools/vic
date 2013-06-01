/*
 * Copyright (c) 1995 Jim Lowe
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

/*
 * 1.0	11/09/95
 *	Initial Release.
 * 1.1	12/20/95
 *	Changed to use new mode in meteor driver (METEOR_GEO_YUV_422) and
 *	allow even only fields to be captured under certain conditions.
 *	Added FRAME_CNTS debugging option.
 * 1.2	3/25/96
 *	Changed the conditions EVEN_ONLY mode was specified.  We now check
 *	the status to determine if we can run in EVEN_ONLY mode.
 * 1.3	5/13/96
 *	Added the fps code to the grabber so we only transfer what we need.
 *	
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "grabber.h"
#include "crdef.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "module.h"
#include "bsd-endian.h"

#if defined(__FreeBSD__)
 #include <machine/ioctl_meteor.h>
 #include <machine/ioctl_bt848.h>
#elif defined(__NetBSD__) || defined(__OpenBSD__)
 #include <dev/ic/bt8xx.h>
#endif

/*XXX*/
#define NTSC_WIDTH 320
#define NTSC_HEIGHT 240
#define PAL_WIDTH 384
#define PAL_HEIGHT 288
#define CIF_WIDTH 352
#define CIF_HEIGHT 288


class MeteorGrabber : public Grabber {
    public:
	MeteorGrabber(const char* name);
	virtual ~MeteorGrabber();
	virtual int command(int argc, const char*const* argv);
	virtual void fps(int);
	virtual void start();
	virtual void stop();
	virtual int grab();
    protected:
	void format();
	virtual void setsize() = 0;
	void suppress(const u_char* in, int istride);
	virtual void saveblks(const u_char* in, int istride) = 0;
	void set_size_meteor(int w, int h);

	int video_format_;	/* video input format: NTSC or PAL */
	int dev_;		/* device fd */
	int port_;		/* video input port */
	u_int basewidth_;	/* Height of frame to be captured */
	u_int baseheight_;	/* Width of frame to be captured */
	u_int decimate_;	/* division of base sizes */
	volatile u_int* pyuv_;	/* pointer to yuv data */
	int tuner_ ;		/* tuner device... */
};

class Meteor422Grabber : public MeteorGrabber {
    public:
	Meteor422Grabber(const char* name);
    protected:
	void setsize();
	void saveblk(const u_char* in, u_char* yp, u_char* up, u_char* vp,
		     int stride, int istride);
	void saveblks(const u_char* in, int istride);
};

class MeteorCIFGrabber : public MeteorGrabber {
    public:
	MeteorCIFGrabber(const char* name);
    protected:
	void setsize();
	void saveblk(const u_char* in, u_char* yp, u_char* up, u_char* vp,
		     int stride, int istride);
	void saveblks(const u_char* in, int istride);
};

class MeteorDevice : public InputDevice {
    public:
	MeteorDevice(const char* nickname, const char* devname, int free);
	virtual int command(int argc, const char*const* argv);
    protected:
	const char* name_;
};

class MeteorScanner {
    public:
	MeteorScanner(const int n);
};

static MeteorScanner find_meteor_devices(4);

MeteorScanner::MeteorScanner(const int n)
{
    static char *d[] = { "/dev/bktr%d", "/dev/meteor%d", NULL };
    char *nickname_template = "meteor-%d";

	for(int i = 0; i < n; i++) {
	for (int j = 0 ; d[j] != NULL ; j++) {
	    char *devname  = new char[strlen(d[j]) + 3];
		char	*nickname = new char[strlen(nickname_template) + 3];

	    sprintf(devname, d[j], i);
		sprintf(nickname, nickname_template, i + 1);
		if(access(devname, R_OK) == 0) {
			int fd = open(devname, O_RDONLY);
			if(fd < 0) {
				new MeteorDevice(nickname, devname, 0);
			} else {
				(void)close(fd);
				new MeteorDevice(nickname, devname, 1);
			}
		} else {
			delete[] nickname; //SV-XXX: Debian
			delete[] devname; //SV-XXX: Debian
		}
			}
		}
	}

MeteorDevice::MeteorDevice(const char* nickname, const char *devname, int free):
					InputDevice(nickname), name_(devname)
{
	if(free)
		attributes_ = "\
format {422 420 cif} \
type {pal ntsc secam auto} \
size {large normal small cif} \
port {RCA Port-1 Port-2 Port-3 S-Video RGB}";
	else
		attributes_ = "disabled";
}

int MeteorDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if ((argc == 3) && (strcmp(argv[1], "open") == 0)) {
		TclObject* o = 0;
		if (strcmp(argv[2], "422") == 0)
			o = new Meteor422Grabber(name_);
		else if (strcmp(argv[2], "420") == 0)
			o = new MeteorCIFGrabber(name_);
		else if (strcmp(argv[2], "cif") == 0)
			o = new MeteorCIFGrabber(name_);
		if (o != 0)
			tcl.result(o->name());
		return (TCL_OK);
	}
	return (InputDevice::command(argc, argv));
}

MeteorGrabber::MeteorGrabber(const char* name)
{
	int devnum;
	if (sscanf(name, "/dev/bktr%d", &devnum) == 1) {
		char *tunerdev  = new char[strlen(name) + 3];
		sprintf(tunerdev, "/dev/tuner%d", devnum);
		tuner_ = open(tunerdev, O_RDONLY);
	} else
		tuner_ = -1;
	dev_ = open(name, O_RDONLY);
	if (dev_ == -1) {
		status_ = -1;
		return;
	}
	port_ = METEOR_INPUT_DEV0;
	video_format_ = METEOR_FMT_AUTOMODE;
	decimate_ = 2;
	basewidth_ = PAL_WIDTH * 2;
	baseheight_ = PAL_HEIGHT * 2;
	
	int temp = ((basewidth_ * baseheight_ * 2 + 4095)/4096)*4096;
	pyuv_ = (u_int*) mmap((caddr_t)0, temp, PROT_READ, 0, dev_, (off_t)0);
	if(pyuv_ == (u_int*)-1)
		pyuv_ = 0;
}

MeteorGrabber::~MeteorGrabber()
{
	if (dev_ != -1) {
		close(dev_);
	}
	if (tuner_ != -1)
		close(tuner_);
}

void MeteorGrabber::set_size_meteor(int w, int h)
{
	struct meteor_geomet geom;

	geom.rows = h &~0xf;	/* 0xf, ugh! */
	geom.columns = w  &~0xf;
	geom.frames = 1;
	geom.oformat = METEOR_GEO_UNSIGNED | METEOR_GEO_YUV_PACKED;
	/*
	 * If we can get by with only reading even fields, then by all
	 * means do so.
	 */
	unsigned short status;
	// ioctl(dev_, METEORSTATUS, &status);
	if ( video_format_ == METEOR_FMT_NTSC ) {
		if(geom.rows <= NTSC_HEIGHT && geom.columns <= NTSC_WIDTH)
			geom.oformat |= METEOR_GEO_EVEN_ONLY;
	} else {
			if(geom.rows<=PAL_HEIGHT && geom.columns<=PAL_WIDTH)
				geom.oformat |= METEOR_GEO_EVEN_ONLY;
		}
	if(ioctl(dev_, METEORSETGEO, &geom) < 0) 
		perror("vic: METERSETGEO: ");
}

void MeteorGrabber::format()
{
	unsigned short status;
	int	fmt;

	ioctl(dev_, METEORSINPUT, &port_);
	ioctl(dev_, METEORSFMT, &video_format_);
	sleep(1);	/* wait for signal lock... */
	ioctl(dev_, METEORSTATUS, &status);

	if(video_format_ == METEOR_FMT_AUTOMODE) {
		switch (status & (METEOR_STATUS_HCLK|METEOR_STATUS_FIDT)) {
		default:
			fprintf(stderr,
				"vic: meteor sees no signal(%d)-using ntsc.\n",
				(status&METEOR_STATUS_HCLK) > 0 );
			/* fall through */
		case METEOR_STATUS_FIDT:
			fmt = METEOR_FMT_NTSC;
			break;
		case 0:
			fmt= METEOR_FMT_PAL;
			break;
		}
	} else
		fmt = video_format_;

	if(fmt == METEOR_FMT_NTSC) {
		baseheight_ = NTSC_HEIGHT * 2;
		basewidth_ = NTSC_WIDTH * 2;
	} else { 	/* PAL and SECAM */
		baseheight_ = PAL_HEIGHT * 2;
		basewidth_ = PAL_WIDTH * 2;
	}
	setsize();
}


void MeteorGrabber::start()
{
	format();
	int cmd = METEOR_CAP_SINGLE;
	ioctl(dev_, METEORCAPTUR, (char*)&cmd);

	cmd = METEOR_CAP_CONTINOUS;
	ioctl(dev_, METEORCAPTUR, (char*)&cmd);
	Grabber::start();
}

void MeteorGrabber::stop()
{

	int cmd = METEOR_CAP_STOP_CONT;
	ioctl(dev_, METEORCAPTUR, (char*)&cmd);
	Grabber::stop();
}

void MeteorGrabber::fps(int f)
{
	u_short met_fps = (u_short)f;
	(void)ioctl(dev_, METEORSFPS, &met_fps);

	Grabber::fps(f);
}

int MeteorGrabber::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "decimate") == 0) {
			int dec = atoi(argv[2]);
			if (dec <= 0) {
				tcl.resultf("%s: divide by zero", argv[0]);
				return (TCL_ERROR);
			}
			if ((u_int)dec != decimate_) {
				decimate_ = dec;
				if(running_) {
					stop();
					setsize();
					start();
				}
			}
			return (TCL_OK);	
		}
		if (strcmp(argv[1], "port") == 0) {
			int p = port_;
                        if(!strcmp(argv[2], "RCA")) p = METEOR_INPUT_DEV0;
                        if(!strcmp(argv[2], "Port-1")) p = METEOR_INPUT_DEV1;
                        if(!strcmp(argv[2], "Port-2")) p = METEOR_INPUT_DEV2;
                        if(!strcmp(argv[2], "Port-3")) p = METEOR_INPUT_DEV3;
                        if(!strcmp(argv[2], "S-Video"))
				p = METEOR_INPUT_DEV_SVIDEO;
                        if(!strcmp(argv[2], "RGB")) p = METEOR_INPUT_DEV_RGB;

			if (p != port_) {
				port_ = p;
				ioctl(dev_, METEORSINPUT, &port_);
			}
			return (TCL_OK);	
		}
		if (strcmp(argv[1], "freeze") == 0) {
			int cmd = METEOR_CAP_CONTINOUS ;
			if ( atoi(argv[2]) != 0 )
			    cmd = METEOR_CAP_STOP_CONT;
			ioctl(dev_, METEORCAPTUR, (char*)&cmd);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "chan") == 0) {
			int p = port_;
			int c = atoi(argv[2]);
			if (c > 0 && c < 199)
			    p = METEOR_INPUT_DEV1 ;
			else
			    p = METEOR_INPUT_DEV0 ;
			if (p != port_) {
				port_ = p;
				ioctl(dev_, METEORSINPUT, &port_);
			}
			if (p == METEOR_INPUT_DEV1)
			    ioctl(tuner_, TVTUNER_SETCHNL, &c);
			return (TCL_OK);	
		}
		if (strcmp(argv[1], "format") == 0 ||
			   strcmp(argv[1], "type") == 0) {
			if (strcmp(argv[2], "auto") == 0)
				video_format_ = METEOR_FMT_AUTOMODE;
			else if (strcmp(argv[2], "pal") == 0)
				video_format_ = METEOR_FMT_PAL;
			else if (strcmp(argv[2], "secam") == 0)
				video_format_ = METEOR_FMT_SECAM;
			else
				video_format_ = METEOR_FMT_NTSC;
			if (running_)
				format();
			return (TCL_OK);	
		}
		if (strcmp(argv[1], "brightness") == 0) {
			u_char val = atoi(argv[2]);
			ioctl(dev_, METEORSBRIG, &val);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "contrast") == 0) {
			u_char val = atoi(argv[2]);
			ioctl(dev_, METEORSCONT, &val);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "hue") == 0) {
			char val = atoi(argv[2]);
			ioctl(dev_, METEORSHUE, &val);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "saturation") == 0) {
			u_char val = atoi(argv[2]);
			ioctl(dev_, METEORSCSAT, &val);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "uvgain") == 0) {
			u_char val = atoi(argv[2]);
			ioctl(dev_, METEORSCHCV, &val);
			return (TCL_OK);
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "format") == 0 ||
			   strcmp(argv[1], "type") == 0) {
			switch (video_format_) {

			case METEOR_FMT_AUTOMODE:
				tcl.result("auto");
				break;

			case METEOR_FMT_NTSC:
				tcl.result("ntsc");
				break;

			case METEOR_FMT_PAL:
				tcl.result("pal");
				break;

			case METEOR_FMT_SECAM:
				tcl.result("secam");
				break;

			default:
				tcl.result("");
				break;
			}
			return (TCL_OK);
			
		}
		if (strcmp(argv[1], "brightness") == 0) {
			u_char val;
			ioctl(dev_, METEORGBRIG, &val);
			tcl.resultf("%d", (unsigned int)val);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "contrast") == 0) {
			u_char val;
			ioctl(dev_, METEORGCONT, &val);
			tcl.resultf("%d", (int)val);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "hue") == 0) {
			char val;
			ioctl(dev_, METEORGHUE, &val);
			tcl.resultf("%d", (int)val);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "saturation") == 0) {
			u_char val;
			ioctl(dev_, METEORGCSAT, &val);
			tcl.resultf("%d", (int)val);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "uvgain") == 0) {
			u_char val;
			ioctl(dev_, METEORGCHCV, &val);
			tcl.resultf("%d", (int)val);
			return (TCL_OK);
		}
	}
	return (Grabber::command(argc, argv));
}

#define U 0
#define Y0 1
#define V 2
#define Y1 3

/*
 * define these for REPLENISH macro used below
 */
#define DIFF4(in, frm, v) \
	v += (in)[Y0] - (frm)[0]; \
	v += (in)[Y1] - (frm)[1]; \
	v += (in)[Y0+4] - (frm)[2]; \
	v += (in)[Y1+4] - (frm)[3];

#define DIFFLINE(in, frm, left, center, right) \
	DIFF4(in + 0*8, frm + 0*4, left); \
	DIFF4(in + 1*8, frm + 1*4, center); \
	DIFF4(in + 2*8, frm + 2*4, center); \
	DIFF4(in + 3*8, frm + 3*4, right); \
	if (right < 0) \
		right = -right; \
	if (left < 0) \
		left = -left; \
	if (center < 0) \
		center = -center;

void MeteorGrabber::suppress(const u_char* devbuf, int is)
{
	const u_char* start = frame_ + 16 * vstart_ * outw_ + 16 * hstart_;
	REPLENISH(devbuf, start, is, 2,
		  hstart_, hstop_, vstart_, vstop_);
}

int MeteorGrabber::grab()
{
	if (pyuv_ == 0)
		return 0;

	int istride = inw_ * 2;
	suppress((u_char*)pyuv_, istride);
	saveblks((u_char*)pyuv_, istride);
	u_int32_t ts = media_ts();
	YuvFrame f(ts, frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

Meteor422Grabber::Meteor422Grabber(const char* name)
	: MeteorGrabber(name)
{
}

MeteorCIFGrabber::MeteorCIFGrabber(const char* name)
	: MeteorGrabber(name)
{
}

void Meteor422Grabber::setsize()
{
	int w = basewidth_ / decimate_;
	int h = baseheight_ / decimate_;
	set_size_meteor(w, h);
	set_size_422(w, h);
}

inline void 
Meteor422Grabber::saveblk(const u_char* in,
	u_char* yp, u_char* up, u_char* vp, int stride, int istride)
{
	for (int i = 16; --i >= 0; ) {
		/*
		 * Each iteration of this loop grabs 16 Ys & 8 U/Vs.
		 */
		register u_int y0, y1, u, v;

		u  = in[U + 0*4] << SHIFT(24) |
		     in[U + 1*4] << SHIFT(16) |
		     in[U + 2*4] << SHIFT(8) |
		     in[U + 3*4] << SHIFT(0);
		v  = in[V + 0*4] << SHIFT(24) |
		     in[V + 1*4] << SHIFT(16) |
		     in[V + 2*4] << SHIFT(8) |
		     in[V + 3*4] << SHIFT(0);
		y0 = in[Y0 + 0*4] << SHIFT(24) |
		     in[Y1 + 0*4] << SHIFT(16) |
		     in[Y0 + 1*4] << SHIFT(8) |
		     in[Y1 + 1*4] << SHIFT(0);
		y1 = in[Y0 + 2*4] << SHIFT(24) |
		     in[Y1 + 2*4] << SHIFT(16) |
		     in[Y0 + 3*4] << SHIFT(8) |
		     in[Y1 + 3*4] << SHIFT(0);

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;
		((u_int*)up)[0] = u;
		((u_int*)vp)[0] = v;

		u  = in[U + 4*4] << SHIFT(24) |
		     in[U + 5*4] << SHIFT(16) |
		     in[U + 6*4] << SHIFT(8) |
		     in[U + 7*4] << SHIFT(0);
		v  = in[V + 4*4] << SHIFT(24) |
		     in[V + 5*4] << SHIFT(16) |
		     in[V + 6*4] << SHIFT(8) |
		     in[V + 7*4] << SHIFT(0);
		y0 = in[Y0 + 4*4] << SHIFT(24) |
		     in[Y1 + 4*4] << SHIFT(16) |
		     in[Y0 + 5*4] << SHIFT(8) |
		     in[Y1 + 5*4] << SHIFT(0);
		y1 = in[Y0 + 6*4] << SHIFT(24) |
		     in[Y1 + 6*4] << SHIFT(16) |
		     in[Y0 + 7*4] << SHIFT(8) |
		     in[Y1 + 7*4] << SHIFT(0);

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

void Meteor422Grabber::saveblks(const u_char* devbuf, int is)
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
				saveblk(devbuf, lum, chm, chm + off, outw_, is);

			devbuf += 32;
			lum += 16;
			chm += 8;
		}
		lum += stride;
		chm += stride >> 1;
		devbuf += istride;
	}
}

void MeteorCIFGrabber::setsize()
{
	int w = basewidth_ / decimate_;
	int h = baseheight_ / decimate_;
	set_size_meteor(w, h);
	set_size_cif(w, h);
}

inline void 
MeteorCIFGrabber::saveblk(const u_char* in,
	u_char* yp, u_char* up, u_char* vp, int stride, int istride)
{
	for (int i = 8; --i >= 0; ) {
		/*
		 * Each iteration of this loop grabs 32 Ys & 16 U/Vs.
		 */
		register u_int y0, y1, u, v;

		u  = in[U + 0*4] << SHIFT(24) |
		     in[U + 1*4] << SHIFT(16) |
		     in[U + 2*4] << SHIFT(8) |
		     in[U + 3*4] << SHIFT(0);
		v  = in[V + 0*4] << SHIFT(24) |
		     in[V + 1*4] << SHIFT(16) |
		     in[V + 2*4] << SHIFT(8) |
		     in[V + 3*4] << SHIFT(0);
		y0 = in[Y0 + 0*4] << SHIFT(24) |
		     in[Y1 + 0*4] << SHIFT(16) |
		     in[Y0 + 1*4] << SHIFT(8) |
		     in[Y1 + 1*4] << SHIFT(0);
		y1 = in[Y0 + 2*4] << SHIFT(24) |
		     in[Y1 + 2*4] << SHIFT(16) |
		     in[Y0 + 3*4] << SHIFT(8) |
		     in[Y1 + 3*4] << SHIFT(0);

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;
		((u_int*)up)[0] = u;
		((u_int*)vp)[0] = v;

		u  = in[U + 4*4] << SHIFT(24) |
		     in[U + 5*4] << SHIFT(16) |
		     in[U + 6*4] << SHIFT(8) |
		     in[U + 7*4] << SHIFT(0);
		v  = in[V + 4*4] << SHIFT(24) |
		     in[V + 5*4] << SHIFT(16) |
		     in[V + 6*4] << SHIFT(8) |
		     in[V + 7*4] << SHIFT(0);
		y0 = in[Y0 + 4*4] << SHIFT(24) |
		     in[Y1 + 4*4] << SHIFT(16) |
		     in[Y0 + 5*4] << SHIFT(8) |
		     in[Y1 + 5*4] << SHIFT(0);
		y1 = in[Y0 + 6*4] << SHIFT(24) |
		     in[Y1 + 6*4] << SHIFT(16) |
		     in[Y0 + 7*4] << SHIFT(8) |
		     in[Y1 + 7*4] << SHIFT(0);

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;
		((u_int*)up)[1] = u;
		((u_int*)vp)[1] = v;

		in += istride;
		yp += stride;
		up += stride >> 1;
		vp += stride >> 1;

		/* do the 2nd (y only instead of yuv) line */

		y0 = in[Y0 + 0*4] << SHIFT(24) |
		     in[Y1 + 0*4] << SHIFT(16) |
		     in[Y0 + 1*4] << SHIFT(8) |
		     in[Y1 + 1*4] << SHIFT(0);
		y1 = in[Y0 + 2*4] << SHIFT(24) |
		     in[Y1 + 2*4] << SHIFT(16) |
		     in[Y0 + 3*4] << SHIFT(8) |
		     in[Y1 + 3*4] << SHIFT(0);

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;

		y0 = in[Y0 + 4*4] << SHIFT(24) |
		     in[Y1 + 4*4] << SHIFT(16) |
		     in[Y0 + 5*4] << SHIFT(8) |
		     in[Y1 + 5*4] << SHIFT(0);
		y1 = in[Y0 + 6*4] << SHIFT(24) |
		     in[Y1 + 6*4] << SHIFT(16) |
		     in[Y0 + 7*4] << SHIFT(8) |
		     in[Y1 + 7*4] << SHIFT(0);

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;

		in += istride;
		yp += stride;
	}
}

void MeteorCIFGrabber::saveblks(const u_char* in, int is)
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
