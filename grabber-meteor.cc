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

/*#define FRAME_CNTS /* print frame counts and fps when device stops -- debug */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#ifdef FRAME_CNTS
#include <sys/time.h>
#endif

#include "grabber.h"
#include "Tcl.h"
#include "device-input.h"
#include "module.h"

#include <machine/ioctl_meteor.h>

/*XXX*/
#define NTSC_WIDTH 320
#define NTSC_HEIGHT 240
#define PAL_WIDTH 384
#define PAL_HEIGHT 288
#define CIF_WIDTH 352
#define CIF_HEIGHT 288


class MeteorGrabber : public Grabber {
 public:
	MeteorGrabber(const char* name, const char* format);
	virtual ~MeteorGrabber();
	virtual void start();
	virtual void stop();
	virtual void fps(int);
 protected:
	virtual int command(int argc, const char*const* argv);
	virtual int capture();
	virtual int grab();
	void format();
	void setsize();

	int video_format_;	/* video input format: NTSC or PAL */
	int dev_;		/* device fd */
	int port_;		/* video input port */
	int coder_format_;	/* 411, 422, or cif */
	u_int basewidth_;	/* Height of frame to be captured */
	u_int baseheight_;	/* Width of frame to be captured */
	u_int decimate_;	/* division of base sizes */
	volatile u_int* pyuv_;	/* pointer to yuv data */
#ifdef FRAME_CNTS
	struct meteor_counts cnts_;	/* pointer to counters */
	double	start_time_;
#endif

	int count;
	long first;
	double values[1500];

};

static const int	f_411 = 0;	/* coder_format_s */
static const int	f_422 = 1;
static const int	f_cif = 2;

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
	char*	devname_template  = "/dev/meteor%d";
	char*	nickname_template = "Matrox Meteor %d";

	for(int i = 0; i < n; i++) {
		char	*devname  = new char[strlen(devname_template)  + 3];
		char	*nickname = new char[strlen(nickname_template) + 3];

		sprintf(nickname, nickname_template, i + 1);
		sprintf(devname, devname_template, i);
		if(access(devname, R_OK) == 0) {
			int fd = open(devname, O_RDONLY);
			if(fd < 0) {
				new MeteorDevice(nickname, devname, 0);
			} else {
				(void)close(fd);
				new MeteorDevice(nickname, devname, 1);
			}
		} else {
			delete nickname;
			delete devname;
		}
	}
}

MeteorDevice::MeteorDevice(const char* nickname, const char *devname, int free):
					InputDevice(nickname), name_(devname)
{
	if(free)
		attributes_ = "\
format {422 411} \
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
		o = new MeteorGrabber(name_, argv[2]);
		if (o != 0)
			tcl.result(o->name());
		return (TCL_OK);
	}
	return (InputDevice::command(argc, argv));
}

MeteorGrabber::MeteorGrabber(const char* name, const char* format)
{
	count = 0;

	coder_format_ = -1;
	if(!strcmp(format, "411")) coder_format_ = f_411;
	if(!strcmp(format, "422")) coder_format_ = f_422;
	if(!strcmp(format, "cif")) coder_format_ = f_cif;
	if(coder_format_ == -1) {
		fprintf(stderr,
			"vic: MeteorGrabber: unsupported format: %s\n",
			format);
		abort();
	}

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
}

void MeteorGrabber::setsize()
{
	struct meteor_geomet geom;

	geom.rows = (baseheight_ / decimate_) &~0xf;	/* 0xf, ugh! */
	geom.columns = (basewidth_ / decimate_)  &~0xf;
	geom.frames = 1;
	geom.oformat = METEOR_GEO_UNSIGNED;
	geom.oformat |= METEOR_GEO_YUV_422;
	/*
	 * If we can aet by with only reading even fields, then by all
	 * means do so.
	 */
	unsigned short status;
	ioctl(dev_, METEORSTATUS, &status);
	if(status & METEOR_STATUS_HCLK) {	/* do we have a source? */
						/* No source, assume ntsc*/
		if(geom.rows <= NTSC_HEIGHT && geom.columns <= NTSC_WIDTH)
			geom.oformat |= METEOR_GEO_EVEN_ONLY;
	} else {
		if(status & METEOR_STATUS_FIDT) { /* is it pal or ntsc? */
						/* 60 hz */
			if(geom.rows<=NTSC_HEIGHT && geom.columns<=NTSC_WIDTH)
				geom.oformat |= METEOR_GEO_EVEN_ONLY;
		} else {			/* 50 hz */
			if(geom.rows<=PAL_HEIGHT && geom.columns<=PAL_WIDTH)
				geom.oformat |= METEOR_GEO_EVEN_ONLY;
		}
	}

	if(ioctl(dev_, METEORSETGEO, &geom) < 0) 
		perror("vic: METERSETGEO: ");

	switch(coder_format_) {
	case f_422:
		set_size_422(geom.columns, geom.rows);
		break;
	case f_cif:
	case f_411:
		set_size_411(geom.columns, geom.rows);
		break;
	}

	allocref();	/* allocate reference frame */
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
		
	if(coder_format_ == f_cif) {
		baseheight_ = CIF_HEIGHT * 2;
		basewidth_ = CIF_WIDTH * 2;
	}
	setsize();
}


void MeteorGrabber::start()
{
	format();
	int cmd = METEOR_CAP_SINGLE;
	ioctl(dev_, METEORCAPTUR, (char*)&cmd);
#ifdef FRAME_CNTS
	cnts_.fifo_errors = 0;
	cnts_.dma_errors = 0;
	cnts_.frames_captured = 0;
	cnts_.even_fields_captured = 0;
	cnts_.odd_fields_captured = 0;
	ioctl(dev_, METEORSCOUNT, &cnts_);
	start_time_ = gettimeofday();
#endif

	cmd = METEOR_CAP_CONTINOUS;
	ioctl(dev_, METEORCAPTUR, (char*)&cmd);
	Grabber::start();
}

void MeteorGrabber::stop()
{

	int cmd = METEOR_CAP_STOP_CONT;
	ioctl(dev_, METEORCAPTUR, (char*)&cmd);
#ifdef FRAME_CNTS
	double endtime = gettimeofday() ;
	ioctl(dev_, METEORGCOUNT, &cnts_);
	int diff = (int)((endtime-start_time_) * 1e-6 + 0.5);
	printf("frames = %d, even fields = %d, odd fields = %d,\n\
fifo errors = %d, dma errors = %d, seconds = %d",
		cnts_.frames_captured, cnts_.even_fields_captured,
		cnts_.odd_fields_captured, cnts_.fifo_errors, cnts_.dma_errors,
		diff);
	if(diff)
		printf(",fps = %d", cnts_.frames_captured/diff);
	printf("\n");
#endif
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
					stop();
					setsize();
					start();
				}
			}
			return (TCL_OK);	
		} else if (strcmp(argv[1], "port") == 0) {
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
		} else if (strcmp(argv[1], "format") == 0 ||
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
		} else if (strcmp(argv[1], "contrast") == 0) {
			contrast(atof(argv[2]));
			return (TCL_OK);	
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "format") == 0 ||
			   strcmp(argv[1], "type") == 0) {
			Tcl& tcl = Tcl::instance();
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
	}
	return (Grabber::command(argc, argv));
}

int MeteorGrabber::capture()
{
	if(pyuv_ == 0) return 0;

	volatile u_int* py   = pyuv_;
	volatile u_int* pu   = (u_int *)((u_int)py + (u_int)framesize_);
	volatile u_int* pv   = (u_int *)((u_int)pu + (framesize_ >> 1));
	u_int* 		lum  = (u_int *)frame_;
	u_int*		uoff = (u_int *)((u_int)lum + (u_int)framesize_);
	int		f422 = coder_format_ == f_422;
	u_int*	 	voff = (u_int *)((u_int)uoff + 
					 (u_int)(framesize_>>(f422?1:2)));
	int 		numc = ((basewidth_/decimate_) &~0xf) >> 3;

	for (int row = 0; row < (((baseheight_/decimate_)&~0xf) >> 1); row++) {
		for(int col = 0; col < numc; col++) {
			*lum++ = *py++;
			*lum++ = *py++;
			*uoff++ = *pu++;
			*voff++ = *pv++;
		}
		for(col = 0; col < numc; col++) {   
                        *lum++ = *py++;
                        *lum++ = *py++;
                        if(f422) {	/* only copy odd in 4:2:2 format */
                                *uoff++ = *pu++; 
                               	*voff++ = *pv++;
			
                        }
                }
                if(!f422) {	/* skip odd if 4:1:1 or cif format */
			pu += numc;
			pv += numc;
		}
	}
	return 1;
}

int MeteorGrabber::grab()
{
	int temp;
	timeval s, e;
	if (capture() == 0)
		return (0);
	suppress(frame_);
	saveblks(frame_);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);

#define diff(x, y) \
               (double) (1000 * (y.tv_sec - x.tv_sec) + (double)(y.tv_usec - x.tv_usec) / 1000)
	::gettimeofday(&s , 0);
	if (count == 0)
		first = s.tv_sec;
	temp = target_->consume(&f);
	::gettimeofday(&e , 0);
	values[count++] = diff(s, e);
	if (e.tv_sec - first > 60) {  /* 30 sec expirement */
		for (int i=0; i<count; i++)
			printf("%f\n", values[i]);
		exit(0);
	}	
	return(temp); 
	//return (target_->consume(&f));
}
