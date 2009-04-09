/*
 * Copyright (c) 1994 Friedrich-Alexander Universitaet Erlangen-Nuernberg (FAU)
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

// Description:
// Framegrabber module for SlicVideo SBus framegrabber card for sparc.
// Written 1995 for vic-2.7 by toerless.eckert@informatik.uni-erlangen.de,
// based on the grabber-vigra module.
//
// The SlicVideo framegrabber is a dump framegrabber for SBus, very similar
// to the Vigrapix framegrabber card. It uses a different SBus interface
// chip, the Motorola SLIC (MC92005) and has 2 * FBAS and 1 Y/C input.
// Besides the grabber library used in this module the SlicVideo framegrabber
// also supports XIL under SunOS 5.x. Currently tested only with SunOS 5.x.
//
// This module supports yuv420 and yuv422 framgrabbing in any size,
// including cif. When using cif the images will be centered and probably
// cropped, depending on the video signal. Both PAL and NTSC video signal
// have been tested. The module will search for installed SlicVideo boards
// upon startup. Also supported are commands to change attribute of the
// video signal: luma brightness/contrast, chroma gain/saturation, hue.
//
// Speed
// Measured on a SS20/502, SunOS 5.4, vic-2.7a22, no preview, still image:
//
// All numbers in fps     NTSC NTSC NTSC  PAL  PAL  PAL  QCIF CIF  QCIF  CIF
//                        1/16  1/4  1/1 1/16  1/4  1/1   PAL PAL  NTSC NTSC
//
// SlicVideo,     NV:       30   27  3.5   25   20  2.5     -   -     -    -
// SlicVideo,   H261:        -    -    -    -    -    -    25  22    30   28
//
// SunVideo/RTVC, NV:       25*  25*  10   25   23    C     -   -     -    -
// SunVideo/RTVC, H261:      -    -    -    -    -    -    25  25    25*  30
//
// C means that vic crashed when trying this resolution.
// * seems to be a bug in vic.
//
// It should always kept in mind that these numbers turn out to be much worse
// for the SlicVideo (or any other dump framegrabber) on a slower machine,
// whereas the SunVideo RTVC driver probably keeps some work on the DSP
// on the SunVideo itself, so it's more likely independant of CPU speed.
//
// Remarks:
// This module SHOULD be compiled with the highest optimization possible
// with your compiler, otherwise the framegrabbing will be _slow_.
// The difference between -O and -O2 with gcc is about 40% speed.
//
// Todo:
// Grabbing full size frames requires the grabbing of two fields. It seems
// to be quite tricky to get two matching fields (one even, one odd) from the
// same frame. That code should be improved to use interrupts.

static const char rcsid[] =
    "@(#) $Id$ (FAU)";

#include <osfcn.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include "grabber.h"
#include "module.h"
#include "vic_tcl.h"
#include "device-input.h"

// Configurable variables:
//
// Utilizing the external api introduces one
// further step of copying because there is no function
// in the external api to pass data out in the format needed by vic
// This is about 20% slower, but is left in the code
// to act as a fallback if there is a bug in the implementation
// of the grabbing routines utilizing the internal api.

static const int use_internal_api = 1;
static const int slv_debug        = 0;


// SlicVideo specific declarations 

extern "C" {
#include <slvlib_int.h>
extern int slv_bytespp[]; /* forgotten in <slvlib_int.h> */
}
#include <slvlib.h>

static int slv_read_frame_yuv(SLV *slvptr, int is420, u_char *yp, int y_stride,
		   u_char *up, int u_stride, u_char *vp, int v_stride);

static int slv_read_field_yuv(SLV *slvptr, int is420, u_char *yp, int y_stride,
		   u_char *up, int u_stride, u_char *vp, int v_stride);

static int slv_copyin_yuv(SLV *slvptr, u_int fbuf, int is420, u_char *yp,
	int y_stride, u_char *up, int u_stride, u_char *vp, int v_stride);

static const int  f_420	= 0;
static const int  f_422	= 1;
static const int  f_cif	= 2;

//****************************************************************************
//*  Interfaces
//****************************************************************************

class SlicVideoGrabber : public Grabber {
 public:
	SlicVideoGrabber(const char* name, const char* format);
	virtual ~SlicVideoGrabber();
	virtual void start();
 protected:
	virtual int command(int argc, const char*const* argv);
	virtual int capture();
	virtual int grab();
	virtual void reformat();
	virtual void slv_assert(char* desc, int command);
 
        // initialized by the constructor routine

	const char *name_;
	SLV  *handle_; 
	int   video_format_;
	int   coder_format_;

	int port_;
	u_int decimate_;

	// initialised by reformat(). Output size of frames grabbed.

	int x_output_size_, y_output_size_, x_output_border_, y_output_border_;

	char *fbuf_;
	int   fbufl_;
};


class SlicVideoDevice : public InputDevice {
 public:
	SlicVideoDevice(const char* nickname, const char *devname, int free);
	virtual int command(int argc, const char*const* argv);
 protected:
	const char *name_;
};

class SlicVideoScanner {
 public:
	SlicVideoScanner(const int n);
};

static SlicVideoScanner find_slv_devices(4);

//***************************************************************************
//*  Implementation of class SlicVideoDevice / SlicVideoScanner
//***************************************************************************

SlicVideoScanner::SlicVideoScanner(const int n)
{
	// scan for <n> SlicVideo devices and instanciate
	// an appropriate number of "SlicVideoDevice" InputDevices.

	int i;
	SLV *slv;
	char *devname_template  = "/dev/slv%d";
	char *nickname_template = "slicvideo-%d";

	for(i = 0; i < n; i++) {
		char *nickname =  new char[strlen(nickname_template) + 3];
		char *devname  =  new char[strlen(devname_template)  + 3];

		sprintf(nickname, nickname_template, i + 1);
		sprintf(devname,  devname_template,  i);
		if ((access(devname, R_OK) == 0) &&
		    (access(devname, W_OK) == 0)) {
			// Device may exist 
			slv = slv_open(devname, SLV_PORT1, SLV_AUTO_INPUT,
			       SLV_EVEN_FIELDS_OUTPUT, SLV_YUV422_OUTPUT);
			if((int)(slv)) {
				slv_close(slv);
				new SlicVideoDevice(nickname, devname, 1);
			} else {
				new SlicVideoDevice(nickname, devname, 0);
			}
		} else {
			delete[] nickname; //SV-XXX: Debian
			delete[] devname; //SV-XXX: Debian
		}
	}
}

SlicVideoDevice::SlicVideoDevice(const char* nickname, const char* devname, int free) : InputDevice(nickname), name_(devname)
{
	if(free)
		attributes_ = "\
format { 420 422 } \
size { small large cif } \
port { Composite-1 Composite-2 S-Video}";
	else
		attributes_ = "disabled";
}

int SlicVideoDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if ((argc == 3) && (strcmp(argv[1], "open") == 0)) {
		TclObject* o = 0;
		o = new SlicVideoGrabber(name_, argv[2]);
		if (o != 0)
			tcl.result(o->name());
		return (TCL_OK);
	}
	return (InputDevice::command(argc, argv));
}

//***************************************************************************
//*  Implementation of class SlicVideoGrabber
//***************************************************************************

SlicVideoGrabber::SlicVideoGrabber(const char* name, const char* format)
{
	// initialize format_ variable.
	// this is later used when converting grabed frames
	// into the format needed by vic
	 
	if(slv_debug)
		fprintf(stderr, "slv: new grabber %s, %s\n", name, format);

	coder_format_ = -1;
	if(!strcmp(format,"420")) coder_format_ = f_420;
	if(!strcmp(format,"422")) coder_format_ = f_422;
	if(!strcmp(format,"cif")) coder_format_ = f_cif;
	if(coder_format_ == -1) {
		fprintf(stderr,
		  "vic: SlicVideoGrabber: unsupported format \"%s\"\n", 
		  coder_format_);
		abort();
	}

	decimate_  = 2;
	port_      = SLV_PORT1;
	fbuf_      = NULL;

	handle_ = slv_open((char *) name, port_, SLV_AUTO_INPUT,
				SLV_EVEN_FIELDS_OUTPUT, SLV_YUV422_OUTPUT);
	if (handle_ == 0) {
		status_ = -1;
		return;
	}

}

SlicVideoGrabber::~SlicVideoGrabber()
{
	if(slv_debug)
		fprintf(stderr, "slv: close grabber\n");

	if (handle_ != 0) {
		slv_assert("slv_close", slv_close(handle_));
		handle_ = 0;
	}
}

void SlicVideoGrabber::reformat()
{

	int vformat_xsize, vformat_ysize, vformat_xstart, vformat_ystart;
	int size_x, size_y;
	int x_input_start, x_input_count, y_input_count, y_input_start;

	// Resync video signal and determine current input video size.
	 
	slv_assert("slv set INPUT_PORT",
		slv_set(handle_, "INPUT_PORT", (void*) port_));
	slv_assert("slv set INPUT_FORMAT",
		slv_set(handle_, "INPUT_FORMAT", (void*) SLV_AUTO_INPUT));

	slv_assert("slv get INPUT_FORMAT",
		slv_get(handle_, "INPUT_FORMAT", (void**) &video_format_));

	switch(video_format_) {
	case SLV_PAL_INPUT:
	case SLV_SECAM_INPUT:
		vformat_xsize   = SLV_PAL_X_SIZE;
		vformat_ysize   = SLV_PAL_Y_SIZE * 2;
		vformat_xstart  = SLV_PAL_X_START;
		vformat_ystart  = SLV_PAL_Y_START;
		break;
	default:
	case SLV_NTSC_INPUT:
		vformat_xsize   = SLV_NTSC_X_SIZE;
		vformat_ysize   = SLV_NTSC_Y_SIZE * 2;
		vformat_xstart  = SLV_NTSC_X_START;
		vformat_ystart  = SLV_NTSC_Y_START;
		break;
	}

        // Determine output sizes:
	// size_x and size_y are the frame size we actually
	// want to have, due to the video format sizes and the selected
	// decimate_. Usually they will be taken. An exception is
	// cif format which has fixed dimensions. These dimensions are
	// determined by the set_size_XXX methods called below and stored
	// in the outw_ and outh_ instance variables.

	size_x = vformat_xsize / decimate_;
	size_y = vformat_ysize / decimate_;

	switch(coder_format_) {
		case f_420: set_size_420(size_x , size_y); break;
		case f_422: set_size_422(size_x , size_y); break;
		case f_cif: set_size_cif(size_x , size_y); break;
		default:
	  		fprintf(stderr, "vic: SlicVideoGrabber::reformat: illegal coder_format (%d) detected\n", coder_format_);
	  		abort();
			break;
	}
	allocref(); // allocates reference frame memory

	// According to size_x, size_y, outh_ and outw_ 
	// the scaler of the SlicVideo board is initialised.
	// If the format is not cif there will be no cropping or
	// borders, otherwise these necessary artifacts have to be
	// taken into account. The input window will take care of
	// cropping, the output window of the borders.

	if(outw_ >=  size_x) {       // x borders  

		x_input_start    =  0;
		x_input_count    =  vformat_xsize;

		x_output_border_ = (outw_ - size_x) / 2;
		x_output_size_   = size_x;

	} else {                    // x cropping

		x_input_start    =  (size_x - outw_) / 2 * decimate_;
		x_input_count    =  vformat_xsize - x_input_start * 2;

		x_output_border_ = 0;
		x_output_size_   = outw_;
	}
	
	if(outh_ >=  size_y) {       // y borders

		y_input_start    =  0;
		y_input_count    =  vformat_ysize;

		y_output_border_ = (outh_ - size_y) / 2;
		y_output_size_   = size_y;
		
	} else {                     // y cropping
	
		y_input_start    =  (size_y - outh_) / 2 * decimate_;
		y_input_count    =  vformat_ysize - y_input_start * 2;

		y_output_border_ = 0;
		y_output_size_   = outh_;
	}
	
	// The SlicVideo library uses field size dimensions so
	// everything in y is off by a factor of 2

	slv_input_size(handle_, x_input_start + vformat_xstart,
				x_input_count,
				y_input_start / 2 + vformat_ystart,
				y_input_count / 2);

	slv_output_size(handle_, x_output_size_,
				 (decimate_ > 1) ?
				 y_output_size_ : y_output_size_ / 2);
	
	if(slv_debug) {
		char *fmt[] = {"420", "422", "cif"};

		fprintf(stderr, "slv: reformat:\n");
		fprintf(stderr, "  input window:  %d X %d (offset: %d + %d, %d + %d)\n",
				x_input_count, y_input_count / 2,
				x_input_start, vformat_xstart,
				y_input_start / 2,  vformat_ystart);
		fprintf(stderr, "  output window: %d X %d (offset: %d, %d)\n", 
				x_output_size_, (decimate_ > 1) ?
				y_output_size_ : y_output_size_ / 2,
				x_output_border_, y_output_border_);
		fprintf(stderr, "  vic buffer:    %d X %d\n", outw_, outh_);
		fprintf(stderr, "  coder_format = %s\n", fmt[coder_format_]);
	}
	
	slv_assert("slv_set OUTPUT_MODE",
		slv_set(handle_, "OUTPUT_MODE",
			(decimate_ > 1) ?
			(void*) SLV_EVEN_FIELDS_OUTPUT :
			(void*) SLV_INTERLACE_OUTPUT));

	// MCT = 0 will make the grabbed UV color signals unsigned 

	slv_assert("slv_set MCT",
		slv_set(handle_, "MCT", (void*) 0));

	slv_assert("slv_set DEGAMMA",
		slv_set(handle_, "DEGAMMA", (void*) 1));

	slv_flush(handle_, 0);

	if(! use_internal_api) {
		if(!fbuf_) {
			fbufl_ = SLV_PAL_X_COUNT * SLV_PAL_Y_COUNT * 2;
			fbuf_  = new char[fbufl_];
		}
		memset(fbuf_, 0x80, fbufl_);
	}
}

void SlicVideoGrabber::start()
{
	if (running_) return;

	reformat();
	Grabber::start();
}

int SlicVideoGrabber::command(int argc, const char*const* argv)
{
	if(slv_debug) fprintf(stderr, "vic: slicvideo command: %s\n", argv[1]);

	if ((argc == 4) &&
	    !strcmp(argv[1], "set")) {
		char *argv2 = (char *) argv[2];
		char *argv3 = (char *) argv[3];

		slv_assert(argv2,
		    slv_set(handle_, argv2, (void*)(atoi(argv3))));
		return (TCL_OK);

	} else
	if ((argc == 3) &&
	    !strcmp(argv[1], "decimate")) {

		int dec = atoi(argv[2]);
		Tcl& tcl = Tcl::instance();

		if (dec <= 0) {
			tcl.resultf("%s: divide by zero", argv[0]);
			return (TCL_ERROR);
		}
		if (dec != decimate_) {
			decimate_ = dec;
			reformat();
		}
		return (TCL_OK);
	} else
	if ((argc == 3) &&
	    !strcmp(argv[1], "port")) {

		const char* name = argv[2];
		int port = SLV_PORT1;
		if (!strcasecmp(name, "composite-2")) port = SLV_PORT2;
		if (!strcasecmp(name, "s-video"))     port = SLV_PORT0;
		if (port != port_) {
			port_ = port;
			reformat();
		}
		return (TCL_OK);
	} else
	if ((argc == 3) &&
	    !strcmp(argv[1], "contrast")) {

		contrast(atof(argv[2]));

		return (TCL_OK);
	} else
	if ((argc == 3) &&
	    !strcmp(argv[1], "luma-contrast")) {

		return (TCL_OK);
	} else

	if ((argc == 2) &&
	    !strcmp(argv[1], "format")) {

		int vformat = -1;
		Tcl& tcl = Tcl::instance();
		slv_assert("slv_get X_INPUT_FORMAT",
			slv_get(handle_, "INPUT_FORMAT", (void**) &vformat));

		switch (vformat) {
		case SLV_NTSC_INPUT:
			tcl.result("ntsc");
			break;

		case SLV_PAL_INPUT:
			tcl.result("pal");
			break;

		case SLV_SECAM_INPUT:
			tcl.result("secam");
			break;

		default:
			tcl.result("no-lock");
			break;
		}
		return (TCL_OK);
	}

	return (Grabber::command(argc, argv));
}

int SlicVideoGrabber::grab()
{
	if (capture() < 0) return (0);
	suppress(frame_);
	saveblks(frame_);

	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

// Capture a frame. 

int SlicVideoGrabber::capture()
{
  if(use_internal_api) {

	int y_stride, y_offset, uv_stride, uv_offset, is420;
	u_char *y_ptr, *u_ptr, *v_ptr;

	// grab a frame or field of dimensions
	// (x_output_size_ X y_output_size_) into a buffer of dimensions
	// (outw_ X outh_). Calculate offsets so that the frame/field will
	// be centered. If decimate_ == 1 we need to call a routine to
	// grab a whole frame consisting of two interleced fields,
	// otherwise we will grab only the odd fields.
	
	if(coder_format_ == f_422) {
		y_stride  = outw_;
		y_offset  = x_output_border_ + y_output_border_ * y_stride;
		uv_stride = outw_ / 2;
		uv_offset = x_output_border_/2 + y_output_border_ * uv_stride;

		y_ptr = frame_ + y_offset;
		u_ptr = frame_ + framesize_ + uv_offset;
		v_ptr = frame_ + framesize_ * 3 / 2 + uv_offset;
		is420 = 0;
	} else {
		y_stride  = outw_;
		y_offset  = x_output_border_ + y_output_border_ * y_stride;
		uv_stride = outw_ / 2;
		uv_offset = x_output_border_/2 + y_output_border_/2 * uv_stride;

		y_ptr = frame_ + y_offset;
		u_ptr = frame_ + framesize_ + uv_offset;
		v_ptr = frame_ + framesize_ * 5 / 4 + uv_offset;
		is420 = 1;
	}

	if(decimate_ == 1) {
		slv_read_frame_yuv(handle_, is420,
			       y_ptr, y_stride,
			       u_ptr, uv_stride,
			       v_ptr, uv_stride);
	} else {
		slv_read_field_yuv(handle_, is420,
			       y_ptr, y_stride,
			       u_ptr, uv_stride,
			       v_ptr, uv_stride);
	}

	return (0);

  } else {

  //      ! use_internal_api

	u_char *yp, *up, *vp;
	u_int  *fbufp, v;
	int    i, h, w;
	char   *ptr;
	int    offset, stride, len;

	// grab a frame or field of dimensions
	// (x_output_size_ X y_output_size_) into a buffer of dimensions
	// (outw_ X outh_). Calculate offsets so that the frame/field will
	// be centered. If decimate_ == 1 we need to call a routine to
	// grab a whole frame consisting of two interleced fields,
	// otherwise we will grab only the odd fields.

	          // offset in bytes, 2 bytes hold one pixel
	offset  = 2 * (x_output_border_ + y_output_border_ * outw_);
	stride  = outw_ * 2;

	ptr     = fbuf_ + offset;
	len     = fbufl_ - offset;

	if(decimate_ == 1) {
		slv_assert("slv_read_frame",
			slv_read_frame(handle_, ptr, len, stride));
	} else {
		slv_assert("slv_read_field",
		      slv_read_field(handle_, ptr, len, (char *) 0, 0, stride));
	}

	// Last step is to convert the frame format as retrieved
	// by the SlicVideo external API calls into the format
	// required by vic. The API provides only calls to retrieve
	// byte interleaved data (UYVY), whereas vic needs separate
	// buffers.
	// This additional step of copying is ugly so it's not a big deal
	// to also convert the borders (unnecessarily).

	fbufp = (u_int *) fbuf_;

	if(coder_format_ == f_422) {
		yp = frame_;
		up = yp + framesize_;
		vp = up + (framesize_ >> 1);

		for (int k = framesize_; k > 0; ) {

			// MSB   LSB - Sun Byteorder
			// U Y0 V Y1 - Sun Byteorder

			 v    = *fbufp++;
			*yp++ =  v >> 16;
			*yp++ =  v;
			*up++ =  v >> 24;
			*vp++ =  v >> 8;
			k -= 2;
		}
	} else {
		yp = frame_;
		up = yp + framesize_;
		vp = up + (framesize_ >> 2);
		for (h = outh_ / 2; --h >= 0; ) {
			for (w = outw_ / 2; --w >= 0; ) {

				 v    = *fbufp++;
				*yp++ =  v >> 16;
				*yp++ =  v;
				*up++ =  v >> 24;
				*vp++ =  v >> 8;
			}
			// Skip color information on every second line 
			for (w = outw_ / 2; --w >= 0; ) {

				 v    = *fbufp++;
				*yp++ =  v >> 16;
				*yp++ =  v;
			}
		}

	}
	return (0);
  }
}

// Check exit status of slv_library commands

void SlicVideoGrabber::slv_assert(char* desc, int command)
{
	if(command != SLV_SUCCESS) {
		char *msg1, *msg2;
		msg1 = "slv: %s failed:";
		msg2 = new char[strlen(msg1)+strlen(desc)+1];
		sprintf(msg2, msg1, desc);
		perror(msg2);
	}
}

//***************************************************************************
//* A faster field reading routing
//* Taken from the SLIC-Video User's Guide and modified
//* to transfer data out in VIC format
//* Should actually be part of the SlicVideo API, so it's kept separate.
//***************************************************************************

//  Try to help the compiler a little bit
//  4 longwords contain 4 U values, 4 V values and 8 Y values

inline void copy(volatile u_long * &source, u_char * &yp, u_char * &up, u_char * &vp)
{
       u_int v;

        v   = *source;

       *yp++ = v >> 16;
       *yp++ = v;
       *up++ = v >> 24;
       *vp++ = v >> 8;
}

inline void copy4_inc(volatile u_long * &source, u_char * &yp, u_char * &up, u_char * &vp)
{
       u_int s1, s2, s3, s4;

       // MSB   LSB - Sun Byteorder
       // U Y0 V Y1 - Sun Byteorder

        s1    = *source++;
        s2    = *source++;
        s3    = *source++;
        s4    = *source++;
       *((u_long *)yp)++ = ((s1>>16)&0xff)<<24 | (s1&0xff)<<16 |
			   ((s2>>16)&0xff)<< 8 | (s2&0xff)     ;
       *((u_long *)yp)++ = ((s3>>16)&0xff)<<24 | (s3&0xff)<<16 |
			   ((s4>>16)&0xff)<< 8 | (s4&0xff)     ;
       *((u_long *)up)++ = ((s1>>24)&0xff)<<24 | ((s2>>24)&0xff)<<16 |
			   ((s3>>24)&0xff)<< 8 | ((s4>>24)&0xff)     ;
       *((u_long *)vp)++ = ((s1>> 8)&0xff)<<24 | ((s2>> 8)&0xff)<<16 |
			   ((s3>> 8)&0xff)<< 8 | ((s4>> 8)&0xff)     ;
}

static int slv_read_frame_yuv(SLV *slvptr, int is420, u_char *yp, int y_stride,
		   u_char *up, int u_stride, u_char *vp, int v_stride)
{
	u_int fbuf_e, fbuf_o;
	int deltat, last, prev, waiting;
	struct slv_info slv_info;

	int status = SLV_FAILURE;

	// Grabbing a frame requires to find two out of three buffers
	// containing both fields of a frame. The information which kind
	// of frame is contained in a buffer is only set at the interrupt
	// indicating the completion of a buffer fill. Somehow it seems
	// as if this interrupts gets lost quite often, so this routine
	// sometimes loops around for a while...

	waiting = 1;
	while(waiting) {

		slv_disb_FRAM_intr(slvptr);
		
		slv_getstat(slvptr, &slv_info);
		
		last   = ( slv_info.last_csr & SLV_CSR_VALID_M &
	          	  ~slv_info.prev_csr);
		prev   = ( slv_info.last_csr & SLV_CSR_VALID_M & ~last);
		
		deltat = slv_info.last_time.tv_usec -
		 	slv_info.prev_time.tv_usec;
		if(deltat < 0)
			deltat += 1000000;

		if( !(slv_info.last_csr & SLV_CSR_INLOCK_M) &&
	     	     (slv_info.prev_csr & SLV_CSR_INLOCK_M) &&
	     	     (deltat > 0) && (deltat < 30000) ) {

			// we've found two buffers which are part of one frame
			// (1/50 second for PAL, 1/60 for NTSC)

			fbuf_e = last >> SLV_CSR_VALID_S;
			fbuf_o = prev >> SLV_CSR_VALID_S;

			slv_acquire_fbufs(slvptr, fbuf_e | fbuf_o);
			slv_enb_FRAM_intr(slvptr);
			waiting = 0;
		} else {
			slv_enb_FRAM_intr(slvptr);
		}
	}

	status = slv_copyin_yuv(slvptr, fbuf_o, is420,
		                yp, y_stride * 2,
		                up, u_stride * 2,
		                vp, v_stride * 2);
	slv_release_fbufs(slvptr, fbuf_o);
	status = slv_copyin_yuv(slvptr, fbuf_e, is420,
		                yp + y_stride, y_stride * 2,
		                up + u_stride, u_stride * 2,
		                vp + v_stride, v_stride * 2);
	slv_release_fbufs(slvptr, fbuf_e);

	return(SLV_SUCCESS);
}

static int slv_read_field_yuv(SLV *slvptr, int is420, u_char *yp, int y_stride,
		   u_char *up, int u_stride, u_char *vp, int v_stride)
{
	u_int fbuf;
	int status = SLV_FAILURE;

	fbuf = slv_acquire_next_fbuf(slvptr);
	if(fbuf != 0) {
		status = slv_copyin_yuv(slvptr, fbuf, is420,
			    yp, y_stride, up, u_stride, vp, v_stride);
		slv_release_fbufs(slvptr, fbuf);
	}
	return(status);
}

static int slv_copyin_yuv(SLV *slvptr, u_int fbuf, int is420, u_char *yp,
	int y_stride, u_char *up, int u_stride, u_char *vp, int v_stride)
{
	u_int	 i, j;
	u_int	 leftover;
	int	 fram_index;
	int	 linebytes;
	u_long   v;
	volatile u_long	*framp;
	
	slv_pipe_begin(slvptr);

	// Read a buffer (unrolled, double-buffered, and
	// pipelined prefetch).
	
	linebytes = (slvptr->dst_x * slv_bytespp[slvptr->output_format]);
	switch (fbuf) {
		case SLV_FRAM_BUF0: fram_index = 0; break;
		case SLV_FRAM_BUF1: fram_index = 1; break;
		case SLV_FRAM_BUF2: fram_index = 2; break;
		default:slv_pipe_end(slvptr);
			goto failure;
	}
	framp =	slvptr->fram[fram_index];

	for(i = 0; i < slvptr->dst_y; i++) {
		// Issue prefetch on both channel's buffers.
		if( linebytes > 16*4) {
			slv_prefetch_chan(slvptr, fbuf, SLV_CHAN_A);
			slv_wait_chan(slvptr, SLV_CHAN_A);
		}

		j = linebytes;
		for(; j >= (32 * 4); j -= (32 * 4)) {

			volatile u_long *channelA = slvptr->slic->bufA;
			volatile u_long *channelB = slvptr->slic->bufB;

			slv_prefetch_chan(slvptr, fbuf, SLV_CHAN_B);

			copy4_inc(channelA, yp, up, vp);
			copy4_inc(channelA, yp, up, vp);
			copy4_inc(channelA, yp, up, vp);
			copy4_inc(channelA, yp, up, vp);

			slv_wait_chan(slvptr, SLV_CHAN_B);

			if(j >= ((32*4)+(16*4))) {
				// Issue prefetch on A channel's buffer. 
				slv_prefetch_chan(slvptr, fbuf, SLV_CHAN_A);
			}

			copy4_inc(channelB, yp, up, vp);
			copy4_inc(channelB, yp, up, vp);
			copy4_inc(channelB, yp, up, vp);
			copy4_inc(channelB, yp, up, vp);

			slv_wait_chan(slvptr, SLV_CHAN_A);
		}

		// Do the leftover bit.
		if(j >= (16*4)) {
			volatile u_long *channelA = slvptr->slic->bufA;

			copy4_inc(channelA, yp, up, vp);
			copy4_inc(channelA, yp, up, vp);
			copy4_inc(channelA, yp, up, vp);
			copy4_inc(channelA, yp, up, vp);

			j -= (16*4);
		}

		// Clear both channels of the SLIC state machine. 
		slvptr->slic->statusA = 0;
		slvptr->slic->statusB = 0;

		// Read the last bit from the FRAM 

		for(; j > 0; j -= 4) {
			// read (un-prefetched) from FRAM
			copy(framp, yp, up, vp);
		}
		
		// Skip to next line.
		yp += y_stride - linebytes / 2;
		up += u_stride - linebytes / 4;
		vp += v_stride - linebytes / 4;

		if(is420 && !(i & 0x1)) {
			up -= u_stride;
			vp -= v_stride;
		}

		// Every few lines, we should call slv_pipe_update 
		if ((i&3) == 3)
			slv_pipe_update(slvptr);
	}
	slv_pipe_end(slvptr);
	return(SLV_SUCCESS);

failure:
	return(SLV_FAILURE);
}
