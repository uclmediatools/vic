/*
 * Copyright (c) 1996 The Regents of the University of California.
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
 *
 * This module contributed by Koji OKAMURA <oka@is.aist-nara.ac.jp>:
 *  My grabber-qcam.cc is depend on "ftp://ftp.nas.com/laird/"'s APIs.
 *  It works on linux box.
 *  I put its binary on ftp://ftp.jain.ad.jp/pub/linux.
 *
 * Modified for more generic driver-versus-library detection by John Bashinski
 * <jbash@cisco.com>.  This version works with QuickCam driver on Solaris 2.x.
 * Also added QuickCam GUI controls, and removed old auto-contrast system.
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>  
#include <sys/ioctl.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <errno.h>

/* XXX why so much disagreement about qcam.h? */
extern "C" {
#ifdef __FreeBSD__
#include <machine/qcam.h>
#else
#include <qcam.h>
#endif
#ifdef __linux__
#include <qcam-os.h>
#endif
}

#include "grabber.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "module.h"

/*XXX*/
#define NTSC_WIDTH 320
#define NTSC_HEIGHT 240
#define PAL_WIDTH 384
#define PAL_HEIGHT 288
#define CIF_WIDTH 352
#define CIF_HEIGHT 288

/*XXX*/
#ifndef QC_MAX_CONTRAST
#define	QC_MAX_CONTRAST	255
#endif

/*XXX*/
#define VOLATILE volatile

class QcamGrabber : public Grabber {
 public:
	QcamGrabber();
	virtual ~QcamGrabber();
	virtual void start();
	struct qcam qcam_;
 protected:
	virtual int command(int argc, const char*const* argv);
	int capture();
	virtual int grab();
	virtual void NTSCgrabSmall();
	virtual void NTSCgrabMedium();
	void format();
	void normalize();
	virtual void setsize();
	void contrast (double c);

	int format_;		/* video format: NTSC or PAL */
	int rformat_;		/* requested format, above + AUTO */
	int port_;
	int fd;
	char buf[320*240];
	u_int basewidth_;
	u_int baseheight_;
	u_int decimate_;
};

class CIFQcamGrabber : public QcamGrabber {
 protected:
	virtual void setsize();
	int loff_;		/* offset from start of frame to scan */
	int coff_;		/* offset from start of frame to scan */
	int hwrap_;		/* amount to skip on each output line */
	int hskip_;		/* amount of input to throw out on each line */
};

class QcamDevice : public InputDevice {
 public:
	QcamDevice(const char*);
	virtual int command(int argc, const char*const* argv);
};

static QcamDevice qcam_device("qcam");

QcamDevice::QcamDevice(const char* name) : InputDevice(name)
{
#if defined (QCAMLIB)
	if(access("/usr/local/etc/qcam.conf", R_OK) == 0)
#elif defined (QCAMDEV)
	if(access("/dev/qcam0",R_OK) == 0)
#endif
   	{
    	attributes_ = " \
		format { 422 } \
		size { small cif  } \
		port { qcam0 } ";
  	} else
    	attributes_ = "disabled";
}

int QcamDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			TclObject* o = 0;
			if (strcmp(argv[2], "422") == 0)
				o = new QcamGrabber;
			else if (strcmp(argv[2], "cif") == 0)
				o = new CIFQcamGrabber;
			if (o != 0)
				tcl.result(o->name());
			return (TCL_OK);
		}
	}
	return (InputDevice::command(argc, argv));
}

QcamGrabber::QcamGrabber()
{

// fprintf(stderr,"new \n");
#if defined (QCAMLIB)

  struct qcam *q;
  q = qc_init();
  qcam_= *q;
  free((char *)q);
  qc_open(&qcam_);

#elif defined(QCAMDEV)
  fd=open("/dev/qcam0",O_RDONLY);
  if(fd<0){
    perror("open");
    exit(1);
  }

  ioctl(fd, QC_GET, &qcam_);
#endif

  (Tcl::instance()).evalf
    ("if [info exists qcamwindow] {\n"
     "    eval \"$qcamwindow(setbright) %d\"\n"
     "    eval \"$qcamwindow(setcont) %f\"\n"
     "    eval \"$qcamwindow(setwbal) %d\"\n"
     "    eval \"$qcamwindow(setbpp) %d\"\n"
     "}\n",
     qcam_.qc_brightness,
     ((double) qcam_.qc_contrast / (double) QC_MAX_CONTRAST),
     qcam_.qc_whitebalance,
     qcam_.qc_bpp);

  basewidth_ = NTSC_WIDTH*2;
  baseheight_ =NTSC_HEIGHT*2;

  format_=0;
  decimate_ = 2;
}

QcamGrabber::~QcamGrabber()
{

#if defined (QCAMLIB)
  qc_close(&qcam_);
#elif defined(QCAMDEV)
	close(fd);
#endif

}

void QcamGrabber::setsize()
{
  set_size_422(basewidth_ / decimate_, baseheight_ / decimate_);
  allocref();
}

void QcamGrabber::format()
{
  setsize();
}

/*XXX*/
void QcamGrabber::normalize()
{
  fprintf(stderr,"normalize \n");
}

void QcamGrabber::start()
{
	format();
	Grabber::start();
}

int QcamGrabber::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

        if (argc == 4) {
	    if (strcmp (argv[1], "set") == 0) {
		if (strcmp (argv[2], "BPP") == 0) {
		    qcam_.qc_bpp = atoi(argv[3]);
		    return (TCL_OK);
		} else if (strcmp (argv[2], "BRIGHT") == 0) {
		    qcam_.qc_brightness = atoi(argv[3]);
		    return (TCL_OK);
		} else if (strcmp (argv[2], "WBAL") == 0) {
		    if (strcmp (argv[3], "auto") == 0) {

#ifdef QC_WHITEBALANCE_AUTO    /* Implies QCAMDEV */
			qcam_.qc_whitebalance = QC_WHITEBALANCE_AUTO;
			ioctl (fd, QC_SET, &qcam_);
			ioctl (fd, QC_GET, &qcam_);
			tcl.evalf
			  ("if [info exists qcamwindow] {\n"
			   "    eval \"$qcamwindow(setwbal) %d\"\n"
			   "}\n",
			   qcam_.qc_whitebalance);
#else
			qcam_.qc_whitebalance = 225;
#endif

		    } else {
			qcam_.qc_whitebalance = atoi(argv[3]);
		    }
		    return (TCL_OK);
		} else {
		    tcl.resultf("%s: unknown set command for Quickcam: %s",
				argv[0], argv[2]);
		    return (TCL_ERROR);
		}
	    }
	} else if (argc == 3) {
	        if (strcmp(argv[1], "get") == 0) {
		        if (strcmp (argv[2], "BPP") == 0) {
			    tcl.resultf("%d", qcam_.qc_bpp);
			    return (TCL_OK);
			} else if (strcmp (argv[2], "BRIGHT") == 0) {
			    tcl.resultf("%d", qcam_.qc_brightness);
			    return (TCL_OK);
			} else if (strcmp (argv[2], "CONT") == 0) {
			    tcl.resultf ("%f", (double) qcam_.qc_contrast /
				               (double) QC_MAX_CONTRAST);
			} else if (strcmp (argv[2], "WBAL") == 0) {
			    tcl.resultf("%d", qcam_.qc_whitebalance);
			    return (TCL_OK);
			}
		} else if (strcmp(argv[1], "decimate") == 0) {
			int dec = atoi(argv[2]);
			if (dec <= 0) {
				tcl.resultf("%s: divide by zero", argv[0]);
				return (TCL_ERROR);
			}
			if (dec != decimate_) {
				decimate_ = dec;
				setsize();
			}
			return (TCL_OK);	
		} else if (strcmp(argv[1], "port") == 0) {
			int p = atoi(argv[2]);
			if (p != port_) {
				port_ = p;
			}
			return (TCL_OK);	
		} else if (strcmp(argv[1], "format") == 0) {
/*
			if (strcmp(argv[2], "auto") == 0)
				rformat_ = VFC_AUTO;
			else if (strcmp(argv[2], "pal") == 0)
				rformat_ = VFC_PAL;
			else
				rformat_ = VFC_NTSC;
*/
			if (running_)
				format();
			return (TCL_OK);	
		} else if (strcmp(argv[1], "contrast") == 0) {
			contrast(atof(argv[2]));
			return (TCL_OK);	
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "normalize") == 0) {
			normalize();
			return (TCL_OK);	
		} else if (strcmp(argv[1], "format") == 0) {
			Tcl& tcl = Tcl::instance();
			switch (format_) {

			default:
				tcl.result("");
				break;
			}
			return (TCL_OK);
			
		}
	}

	return (Grabber::command(argc, argv));
}

void QcamGrabber::NTSCgrabSmall()
{
  int i,j;
  char *scan;
  int d;

#if defined(QCAMLIB)
  scan=qc_scan(&qcam_);
  d=8-qcam_.bpp;
#elif defined(QCAMDEV)
  scan = buf;
  qcam_.qc_xsize=160;
  qcam_.qc_ysize=120;
  ioctl(fd, QC_SET, &qcam_);
  d=8-qcam_.qc_bpp; 
  lseek(fd,0,SEEK_SET);
  j=read(fd,scan,qcam_.qc_xsize*qcam_.qc_ysize);
#endif

//  fprintf(stderr,"%d %d %d %d\n",inh_,inw_,outh_,outw_); 

  switch(outw_){
    
  case 160: /* NTSC */
    for(i=0;i<outh_;i++)
      for(j=0;j<outw_;j++)
	frame_[i*outw_+ j] 
	  = scan[(i+4)*inw_+j] << d;
    break;

  case 176: /* CIF */

    for(i=0;i<inh_;i++)
      for(j=0;j<inw_;j++)
	frame_[(i+(outh_-inh_)/2)*outw_+((outw_-inw_)/2+j)] 
	  = scan[i*inw_+j] << d;
    break;

  }

#if defined (QCAMLIB)
  free(scan);
#endif
}

void QcamGrabber::NTSCgrabMedium()
{

  int i,j;
  char *scan;
  int d;

#if defined (QCAMLIB)
  scan=qc_scan(&qcam_);
  d=8-qcam_.bpp;
#elif defined(QCAMDEV)
  scan=buf;
  qcam_.qc_xsize=320;
  qcam_.qc_ysize=240;
  ioctl(fd, QC_SET, &qcam_);
  d=8-qcam_.qc_bpp;
  lseek(fd,0,SEEK_SET);
  j=read(fd,scan,qcam_.qc_xsize*qcam_.qc_ysize);
#endif

  for(i=0;i<inh_;i++)
    for(j=0;j<inw_;j++)
      frame_[(i+(outh_-inh_)/2)*outw_+((outw_-inw_)/2+j)] 
	= scan[i*inw_+j] << d;

#if defined (QCAMLIB)
  free(scan);
#endif

}

int QcamGrabber::capture()
{

  int i;

//  fprintf(stderr,"%d %d\n",format_,decimate_);

#if defined(QCAMLIB)
  qcam_.width=inw_;
  qcam_.height=inh_;
  qc_set(&qcam_);
#endif

  switch (decimate_) {

  case 2 :
    NTSCgrabMedium();
    break;

  case 4 :
    NTSCgrabSmall();
    break;

  default:
    return (0);
  }

  return (1);
}

int QcamGrabber::grab()
{

  if (capture() == 0)
    return (0);

  suppress(frame_);
  saveblks(frame_);
  YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
  return (target_->consume(&f));
}

void QcamGrabber::contrast (double c) 
{
    qcam_.qc_contrast = (u_char) (QC_MAX_CONTRAST * c);
    contrast_ = c;
}

void CIFQcamGrabber::setsize()
{
	if (format_ < 0)
		return;
	set_size_cif(basewidth_ / decimate_, baseheight_ / decimate_);
	allocref();
}
