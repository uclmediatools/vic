/*
From oka@nanotsu.kobe-u.ac.jp Tue Jan  6 20:20:43 1998
Date: Mon, 27 Oct 1997 11:32:03 +0900
From: Koji OKAMURA <oka@nanotsu.kobe-u.ac.jp>
To: Gerd Knorr <kraxel@goldbach.isdn.cs.tu-berlin.de>,
    Werner Almesberger <almesber@lrc.di.epfl.ch>, alan@lxorguk.ukuu.org.uk,
    rjkm@thp.uni-koeln.de, chandran@lrc.di.epfl.ch
Subject: Current grabber-bttv.cc 

    [The following text is in the "ISO-2022-JP" character set]
    [Your display is set for the "ISO-8859-1" character set]
    [Some characters may be displayed incorrectly]


Hi, all,

Grabber-bttv.cc has been updated by Gerd's advise.
Im' going to modify to capture with interlace off, when encoded by H.261 
on NTSC Video.

--
Koji
*/

/* =========================================================================

     Copyright (c) 1997 Regents of Koji OKAMURA, oka@kobe-u.ac.jp
     All rights reserved.

     Advised by Gerd Knorr <kraxel@goldbach.isdn.cs.tu-berlin.de>

   ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/fcntl.h>  
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>


extern "C" {
#if defined(__linux__)
#include <bttv.h>
#include <bt848.h>
#endif
}

#include "grabber.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "module.h"

#define NTSC_WIDTH  320
#define NTSC_HEIGHT 240
#define PAL_WIDTH   384
#define PAL_HEIGHT  288
#define CIF_WIDTH   352
#define CIF_HEIGHT  288

#define CF_422 0
#define CF_420 1
#define CF_CIF 2

class BTTVGrabber : public Grabber {
 public:
	BTTVGrabber(const char * cformat);
	virtual ~BTTVGrabber();
	virtual void start();
	struct bttv_window wtw_;
	int format_;
 protected:
	virtual int command(int argc, const char*const* argv);
	virtual int grab();
	virtual void BTTVgrab();
	virtual void format();
	virtual void setsize();

	int fd;
	unsigned char *tm_;
	u_int basewidth_;
	u_int baseheight_;
	u_int decimate_;
	int cformat_;
        int port_;
};

class BTTVDevice : public InputDevice {
 public:
	BTTVDevice(const char*);
	virtual int command(int argc, const char*const* argv);
};

static BTTVDevice BTTV_device("bttv");

BTTVDevice::BTTVDevice(const char* name) : InputDevice(name)
{

  if(access("/dev/bttv",R_OK) == 0)
   	{
    	attributes_ = " \
		format { 420 422 } \
		size { small cif large } \
		port { TV Port-1 Port-2 S-Video}";
  	} else
    	attributes_ = "disabled";
}

int BTTVDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			TclObject* o = 0;
			o = new BTTVGrabber(argv[2]);
			if (o != 0)
				tcl.result(o->name());
			return (TCL_OK);
		}
	}
	return (InputDevice::command(argc, argv));
}

BTTVGrabber::BTTVGrabber(const char *cformat)
{

  fd=open("/dev/bttv",O_RDWR);
  if(fd<0){
    fprintf(stderr,"open failed: %s %s \n",
	    "/dev/bttv",strerror(errno));
    exit(1);
  }

  if(!strcmp(cformat, "422")) cformat_ = CF_422;
  if(!strcmp(cformat, "420")) cformat_ = CF_420;
  if(!strcmp(cformat, "cif")) cformat_ = CF_CIF;

  decimate_ =2;
  basewidth_ = CIF_WIDTH*decimate_;
  baseheight_= CIF_HEIGHT*decimate_;

  if(ioctl(fd, BTTV_GETWTW, &wtw_)){
    perror("ioctl:BTTV_GETWTW");
    exit(1);
  }

  wtw_.color_fmt = BT848_COLOR_FMT_YUY2;
  wtw_.bpp       =2;
  wtw_.width = basewidth_ /decimate_;
  wtw_.height= baseheight_/decimate_;
  wtw_.bpl       = wtw_.width*wtw_.bpp;

  if(!wtw_.norm)
    wtw_.interlace =
      ((wtw_.width > PAL_WIDTH) || (wtw_.height > PAL_HEIGHT)) ? 1:0;
  else
    wtw_.interlace =
      ((wtw_.width > NTSC_WIDTH) || (wtw_.height > NTSC_HEIGHT)) ? 1:0;

/*
  wtw_.norm = 0;
*/
  if(ioctl(fd, BTTV_SETWTW, &wtw_)){
    perror("ioctl:BTTV_SETWTW");
    exit(1);
  }

  tm_ =(unchar *)malloc(0x200000);

  if (tm_==NULL)  {
    printf("Cannot alloc memory\n");
    exit(1);
  }

#ifndef BTTV_DONT_MLOCK
  if (-1 == mlock(tm_, 0x200000)) {
      perror("mlock");
      /* DANGER: we continue and hope touch-every-page before grab
	 is enouth... */
      /* exit(1); */
  }
#endif

    port_=0;
    ioctl(fd, BTTV_INPUT, &port_);
}

BTTVGrabber::~BTTVGrabber()
{
#ifndef BTTV_DONT_MLOCK
    if (-1 == munlock(tm_, 0x200000))
	perror("munlock");
#endif
  free(tm_);
  close(fd);
}

void BTTVGrabber::start()
{
  format();
  Grabber::start();
}

int BTTVGrabber::command(int argc, const char*const* argv)
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

	wtw_.width = basewidth_ /decimate_;
	wtw_.height= baseheight_/decimate_;
	wtw_.bpl       = wtw_.width*wtw_.bpp;

	if(!wtw_.norm) 
	  wtw_.interlace =
	    ((wtw_.width > PAL_WIDTH) || (wtw_.height > PAL_HEIGHT)) ? 1:0;
	else
	  wtw_.interlace =
	    ((wtw_.width > NTSC_WIDTH) || (wtw_.height > NTSC_HEIGHT)) ? 1:0;

	if(ioctl(fd, BTTV_SETWTW, &wtw_)){
	  perror("ioctl:BTTV_SETWTW");
	  exit(1);
	}
	setsize();
      }
    }else if (strcmp(argv[1], "port") == 0) {
			int p = port_;
                        if(!strcmp(argv[2], "TV")) p = 0;
                        if(!strcmp(argv[2], "Port-1")) p = 1;
                        if(!strcmp(argv[2], "Port-2")) p = 2;
                        if(!strcmp(argv[2], "S-Video"))p = 3;
			if (p != port_) {
				port_ = p;
				ioctl(fd, BTTV_INPUT, &port_);
			}
			return (TCL_OK);	
/*
    }else if (strcmp(argv[1], "norm") == 0) {
			int n = wtw_.norm;
                        if(!strcmp(argv[2], "PAL")) n = 0;
                        if(!strcmp(argv[2], "NTSC")) n = 1;
			if (n != wtw_.norm) {
				wtw_.norm = n;
				if(ioctl(fd, BTTV_SETWTW, &wtw_)){
				  perror("ioctl:BTTV_SETWTW");
				  exit(1);
				}
			}
			return (TCL_OK);	
*/
    } else if (strcmp(argv[1], "format") == 0) {
      if (running_)
	format();
      return (TCL_OK);	
    } else if (strcmp(argv[1], "contrast") == 0) {
      contrast(atof(argv[2]));
      return (TCL_OK);	
    }
  }
  
  return (Grabber::command(argc, argv));

}

void BTTVGrabber::BTTVgrab()
{
  int i,j, yy, uv;
  int n=0;
  unsigned char *y,*u,*v;

  for (i=0; i<0x200000; i+=4096)
    tm_[i]=1;
  tm_[0x1fffff]=1;

  ioctl(fd, BTTV_TESTM, &tm_);

  y=frame_;
  u=frame_+outh_*outw_;

  if(cformat_ == CF_422)
    v=u+outh_*outw_/2;
  else
    v=u+outh_*outw_/4;

  for(i=0;i<inh_;i++)
    for(j=0;j<inw_;){

      yy=(i+(outh_-inh_)/2)*outw_+(outw_-inw_)/2+j;

      if(cformat_ == CF_422)
	uv=i*outw_/2+j/2;
      else
	uv=i*outw_/4+j/2+(outh_-inh_)*outw_/8+(outw_-inw_)/4;

      if(!(i%2)){

	y[yy]   = tm_[(i*inw_+j)*2];
	u[uv]   = tm_[(i*inw_+j)*2+1];
	j++;
	y[yy+1] = tm_[(i*inw_+j)*2];
	v[uv]   = tm_[(i*inw_+j)*2+1];
	j++;

      } else {

	if(cformat_==CF_422){

	  y[yy]   = tm_[(i*inw_+j)*2];
	  u[uv]   = tm_[(i*inw_+j)*2+1];
	  j++;
	  y[yy+1] = tm_[(i*inw_+j)*2];
	  v[uv]   = tm_[(i*inw_+j)*2+1];
	  j++;

	}else{ // CF_CIF

	  y[yy]   = tm_[(i*inw_+j)*2];
	  j++;
	  y[yy+1] = tm_[(i*inw_+j)*2];
	  j++;
	  
	}
      }
    }

}


int BTTVGrabber::grab()
{

  BTTVgrab();

  suppress(frame_);
  saveblks(frame_);
  YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
  return (target_->consume(&f));
}

void BTTVGrabber::setsize()
{

  switch(cformat_){
  case CF_422:
    set_size_422(basewidth_ / decimate_, baseheight_ / decimate_);
    break;
  case CF_420:
    set_size_420(basewidth_ / decimate_, baseheight_ / decimate_);
    break;
  case CF_CIF:
    set_size_cif(basewidth_ / decimate_, baseheight_ / decimate_);
    break;
  }

  allocref();
}

void BTTVGrabber::format()
{

  if(wtw_.norm){
    baseheight_ = NTSC_HEIGHT * 2;
    basewidth_  = NTSC_WIDTH * 2;
  }else{
    baseheight_ = PAL_HEIGHT * 2;
    basewidth_  = PAL_WIDTH * 2;
  }

  if(cformat_ == CF_CIF) {
    baseheight_ = CIF_HEIGHT * 2;
    basewidth_ = CIF_WIDTH * 2;
  }
  
  wtw_.width = basewidth_ /decimate_;
  wtw_.height= baseheight_/decimate_;
  wtw_.bpl       = wtw_.width*wtw_.bpp;

  if(!wtw_.norm) 
    wtw_.interlace =
      ((wtw_.width > PAL_WIDTH) || (wtw_.height > PAL_HEIGHT)) ? 1:0;
  else
    wtw_.interlace =
      ((wtw_.width > NTSC_WIDTH) || (wtw_.height > NTSC_HEIGHT)) ? 1:0;

  if(ioctl(fd, BTTV_SETWTW, &wtw_)){
    perror("ioctl:BTTV_SETWTW");
    exit(1);
  }

  setsize();
}
