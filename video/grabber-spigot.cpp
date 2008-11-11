/*
 * Copyright (c) Jim Lowe, 1995, All rights reserved.
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
 *      This product includes software developed by Jim Lowe
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * 1.0	11/28/95
 *	Initial Release.
 *	Spigot library available from:
 *			ftp.cs.uwm.edu:/pub/FreeBSD/spigot/spigot.tar.gz
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "grabber.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "module.h"

extern "C" {
#	include <machine/spigot.h>
#	include <spigot_saa7191.h>
} ;

/*XXX*/
#define NTSC_WIDTH 320
#define NTSC_HEIGHT 240
#define PAL_WIDTH 384
#define PAL_HEIGHT 288
#define CIF_WIDTH 352
#define CIF_HEIGHT 288


class SpigotGrabber : public Grabber {
 public:
	SpigotGrabber(const char* name, const char* format);
	virtual ~SpigotGrabber();
	virtual void start();
 protected:
	virtual int command(int argc, const char*const* argv);
	virtual int capture();
	virtual int grab();
	void format();
	void setsize();

	int video_format_;	/* video input format: NTSC or PAL */
	int port_;		/* video input port */
	int coder_format_;	/* 420, 422, or cif */
	int vtof_;		/* Vertical top of frame (lines to skip) */
	int secam_;		/* secam mode ? */
	u_int basewidth_;	/* Height of frame to be captured */
	u_int baseheight_;	/* Width of frame to be captured */
	u_int decimate_;	/* division of base sizes */
	volatile u_short* pyuv_;/* pointer to yuv data */
	int 	loff_;		/* offset from start of frame to scan */
	int	coff_;		/* offset from start of frame to scan */
	int	hwrap_;		/* amount to skip on each output line */
	int	hskip_;		/* amount of input to toss on each line */
};

static const int	f_420 = 0;	/* coder_format_s */
static const int	f_422 = 1;
static const int	f_cif = 2;

static const int	vf_pal = 0;	/* video_format_s */
static const int	vf_ntsc = 1;
static const int	vf_automode = 2;

static const int	port_rca = 0;	/* port_s */
static const int	port_svideo = 1;

static const int	vtof = 10;	/* XXX */

class SpigotDevice : public InputDevice {
 public:
	SpigotDevice(const char* nickname, const char* devname);
	virtual int command(int argc, const char*const* argv);
 protected:
	const char* name_;
};


static SpigotDevice spigot_device("Video Spigot", "/dev/spigot");

SpigotDevice::SpigotDevice(const char* nickname, const char *devname):
					InputDevice(nickname), name_(devname)
{
	if(access(devname, R_OK) == 0)
		attributes_ = "\
format {422 420} \
size {normal small cif} \
port {RCA S-Video}";
	else
		attributes_ = "disabled";
}

int SpigotDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if ((argc == 3) && (strcmp(argv[1], "open") == 0)) {
		TclObject* o = 0;
		o = new SpigotGrabber(name_, argv[2]);
		if (o != 0)
			tcl.result(o->name());
		return (TCL_OK);
	}
	return (InputDevice::command(argc, argv));
}

SpigotGrabber::SpigotGrabber(const char* name, const char* format)
{
	coder_format_ = -1;
	if(!strcmp(format, "420")) coder_format_ = f_420;
	if(!strcmp(format, "422")) coder_format_ = f_422;
	if(!strcmp(format, "cif")) coder_format_ = f_cif;
	if(coder_format_ == -1) {
		fprintf(stderr,
			"vic: SpigotGrabber: unsupported format: %s\n",
			format);
		abort();
	}

	pyuv_ = spigot_open((char *)name);
	if (pyuv_ == (u_short *)-1) {
		pyuv_ = 0;
		status_ = -1;
		return;
	}
	(void)saa7191_init(NULL, 0);	/* initialize phillips saa 7191 chip */
	(void)saa7191_setv("OFTS", 1);	/* yuv 4:2:2 output mode */
	port_ = saa7191_getv("GPSW1");
	video_format_ = saa7191_getv("FSEL");
	secam_ = saa7191_getv("SECS");	/* when should this be set ? */
	vtof_ = vtof;
	if(saa7191_getv("AUFD"))
		video_format_ = vf_automode;
	decimate_ = 2;
	basewidth_ = NTSC_WIDTH * 2;
	baseheight_ = NTSC_HEIGHT * 2;
}

SpigotGrabber::~SpigotGrabber()
{
	if (pyuv_ != 0) {
		pyuv_ = 0;
		spigot_close();
	}
}

void SpigotGrabber::setsize()
{

	int	w = basewidth_ / decimate_;
	int	h = baseheight_ / decimate_;

	spigot_set_capture_size(w, vtof_);

	switch(coder_format_) {
	case f_422:
		set_size_422(w, h);
		hwrap_ = 0;
		loff_ = 0;
		coff_ = 0;
		hskip_ = 0;
		break;
	case f_cif:
		set_size_cif(w, h);
		/* XXX */
		vstart_ = 0;
		vstop_ = blkh_;
		hstart_ = 1;
		hstop_ = blkw_ - 1;

		{
		int	voff = (outh_ - inh_) / 2;
		int	hoff = hwrap_ / 2;

		hwrap_ = outw_ - inw_ ;
		loff_ = outw_ * voff + hoff;
		coff_ = (outw_ >> 1) * (voff >> 1) + (hoff >> 1);
		}
		hskip_ = 0;
		break;
	case f_420:
		set_size_420(w, h);
		hwrap_ = 0;
		loff_ = 0;
		coff_ = 0;
		hskip_ = 0;
		break;
	}
	allocref();	/* allocate reference frame */

}

void SpigotGrabber::format()
{
	u_char status = spigot_status();
	if(pyuv_ == 0) {
		fprintf(stderr, "vic: spigot not active\n");
		return;
	}

	int w, h;
	switch (status & (SPIGOT_COLOR|SPIGOT_60HZ|SPIGOT_NO_HORIZONTAL_LOCK)) {
	default:
		fprintf(stderr, "vic: spigot sees no signal - using ntsc.\n");
		/* fall through */
	case 2:	/* NTSC no color */
	case 3: /* NTSC color */
		w = NTSC_WIDTH * 2;
		h = NTSC_HEIGHT * 2;
		break;

	case 0:	/* PAL no color */
	case 1: /* PAL color */
		w = PAL_WIDTH * 2;
		h = PAL_HEIGHT * 2;
		break;
	}
	basewidth_ = w;
	baseheight_ = h;
	setsize();
}


void SpigotGrabber::start()
{
	format();
	Grabber::start();
}

int SpigotGrabber::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "decimate") == 0) {
			int dec = atoi(argv[2]);
			Tcl& tcl = Tcl::instance();
			if (dec <= 0) {
				tcl.resultf("%s: divide by zero", argv[0]);
				return (TCL_ERROR);
			}
			if (dec != decimate_) {
				decimate_ = dec;
				if(running_) {
					setsize();
				}
			}
			return (TCL_OK);	
		} else if (strcmp(argv[1], "port") == 0) {
			int p = port_;
                        if(!strcmp(argv[2], "RCA")) p = port_rca;
			if(!strcmp(argv[2], "S-Video")) p = port_svideo;
			if (p != port_) {
				port_ = p;
				(void) saa7191_setv("GPSW1", p);
			}
			return (TCL_OK);	
		} else if (strcmp(argv[1], "format") == 0) {
			if (strcmp(argv[2], "auto") == 0) {
				video_format_ = vf_automode;
				(void)saa7191_setv("AUFD", 1);
			} else if (strcmp(argv[2], "pal") == 0) {
				video_format_ = vf_pal;
				(void)saa7191_setv("FSEL", 0);
				(void)saa7191_setv("AUFD", 0);
			} else {
				video_format_ = vf_ntsc;
				(void)saa7191_setv("FSEL", 1);
				(void)saa7191_setv("AUFD", 0);
			}
			if (running_)
				format();
			return (TCL_OK);	
		} else if (strcmp(argv[1], "contrast") == 0) {
			contrast(atof(argv[2]));
			return (TCL_OK);	
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "normalize") == 0) {
#ifdef notdef
			normalize();
#endif
			return (TCL_OK);	
		} else if (strcmp(argv[1], "format") == 0) {
			Tcl& tcl = Tcl::instance();
			switch (video_format_) {

			case vf_automode:
				tcl.result("auto");
				break;

			case vf_ntsc: 
				tcl.result("ntsc");
				break;

			case vf_pal:
				tcl.result("pal");
				break;

			default:
				tcl.result("");
				break;
			}
			return (TCL_OK);
			
		}
	}
	return (Grabber::command(argc, argv));
}

int SpigotGrabber::capture()
{
	if(pyuv_ == 0) return 0;

	volatile u_short*	pyuv 	 = pyuv_;
	u_char* 		lum  	 = frame_ + loff_;
	u_char*			u    	 = frame_ + framesize_ + coff_;
	int			f422 	 = (coder_format_ == f_422);
	u_char*			v    	 = u + (framesize_ >> (f422 ? 1:2));
	int 			numc 	 = ((basewidth_/decimate_) & ~0xf) >> 2;
	int			numr 	 = (baseheight_/decimate_) & ~0xf;
	int			chm_wrap = hwrap_ >> 1;

	(void)spigot_start_xfer(1);
	for(int y=0; y < numr ; y++) {
		for(int x=0; x < numc; x++) {
			/*
			 * We read these as unsigned shorts and set y data
			 * right after the read because of some weird timing
			 * problems with the spigot.  Originally, I tried
			 * reading the data as 2 unsigned ints and it didn't
			 * work on my machine.
			 *
			 * There also seems to be some word swapping going
			 * on between the 7191 and the fifo's.  Anyways,
			 * I think this is the correct order for data.
			 */
			u_short d0 = *pyuv;
			*lum++ = d0 & 0xff;
			u_short d1 = *pyuv;
			*lum++ = d1 & 0xff;
			u_short d2 = *pyuv;
			*lum++ = d2 & 0xff;
			u_short d3 = *pyuv;
			*lum++ = d3 & 0xff;
                        if(f422) { 
                       		/*
				 * Data comes from the saa7191
				 * in the format:
				 * Y0 V01
				 * Y1 U01
				 * Y2 V23
				 * Y3 U23.
				 */
				*v++ = (d0 >> 8) & 0xff;
				*u++ = (d1 >> 8) & 0xff;
				*v++ = (d2 >> 8) & 0xff;
				*u++ = (d3 >> 8) & 0xff;
			} else {        /* YUV 4:1:1, skip odd chm data */
				if((y & 1) == 0) {
					*v++ = (d0 >> 8) & 0xff;
					*u++ = (d1 >> 8) & 0xff;
					*v++ = (d2 >> 8) & 0xff;
					*u++ = (d3 >> 8) & 0xff;
				}
			}
		}
		lum += hwrap_;
		if((y & 1) == 0) {
			u += chm_wrap;
			v += chm_wrap;
		}
	}

	/* remove trailing garbage */
	for(y=0; y < ((baseheight_/decimate_)&0xf)* (basewidth_/decimate_); y++)
		u_short d0 = *pyuv;

	(void)spigot_stop_xfer();

	return 1;
}

int SpigotGrabber::grab()
{
	if (capture() == 0)
		return (0);
	suppress(frame_);
	saveblks(frame_);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}
