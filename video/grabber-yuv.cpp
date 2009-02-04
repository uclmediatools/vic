/*
 * FILE:	grabber-yuv.cpp
 * AUTHOR:	Soo-Hyun Choi <s.choi@cs.ucl.ac.uk>
 *
 * Copyright (c) 2009 University College London
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and the Network Research Group at
 *      Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifndef WIN32
#include <sys/file.h>
#endif
#include <sys/stat.h>

#include "inet.h"
#include "vic_tcl.h"
#include "rtp.h"
#include "grabber.h"
#include "crdef.h"
#include "device-input.h"
#include "transmitter.h"
#include "module.h"

//#define DEBUG 1
#undef DEBUG

class StillYuvGrabber : public Grabber {
public:
	StillYuvGrabber();
	virtual ~StillYuvGrabber();
	virtual int command(int argc, const char* const* argv);
protected:
	void start();
	void stop();
	int grab();
	void setsize();

	int decimate_;
	int width_;			// width in pixel
	int height_;		// height in pixel
	int num_frame_;		// current frame number
};

class StillYuvDevice : public InputDevice {
public:
    StillYuvDevice(const char* s);
    virtual int command(int argc, const char * const * argv);
    virtual Grabber* still_yuv_grabber();

    void load_file(const char * const file);
    char *frame_;
    int len_;
	int devstat_;		// device status
private:
};

static StillYuvDevice yuv_device("yuv");

StillYuvDevice::StillYuvDevice(const char* s) : InputDevice(s),
		frame_(NULL), len_(0), devstat_(-1)
{
    attributes_ = "format { 420 422 jpeg cif } size { small large cif }";

#ifdef DEBUG
    debug_msg("StillYuvDevice::StillYuvDevice name=%s\n", s);
#endif /* DEBUG */
}

/*
 * StillYuvDevice
 */
int StillYuvDevice::command(int argc, const char*const* argv) {
#ifdef DEBUG
	for (int i = 0; i < argc; i++)
		debug_msg("StillYuvDevice\t%s\n", argv[i]);
#endif
    if (argc == 3)
    {
		if (strcmp(argv[1], "open") == 0)
		{
		    const char* fmt = argv[2];
			TclObject* o = 0;
			if (strcmp(fmt, "cif") == 0) 
				o = still_yuv_grabber();
		    if (o != 0)
				Tcl::instance().result(o->name());
		    return (TCL_OK);
		}
		else if (strcmp(argv[1], "file") == 0)
		{
		    debug_msg("Loading %s\n", argv[2]);
		    load_file(argv[2]);
		}
    }
    return (InputDevice::command(argc, argv));
}

Grabber* StillYuvDevice::still_yuv_grabber() {
	return (new StillYuvGrabber());
}

/*
 * File loading
 */
void StillYuvDevice::load_file(const char * const f) {
    FILE *fp;
    struct stat s;
    
    fp = fopen(f, "r");
    if (fp == (FILE *) NULL)
    {
		perror("cannot load file");
		return;
    }
    if (fstat(fileno(fp), &s) < 0)
    {
		perror("cannot stat frame");
		fclose(fp);
		return;
    }
    
    len_ = s.st_size;	// file length
	if (len_ == 0) return;

    if (frame_)
		delete[] frame_; 
    
    frame_ = new char[len_ + 1];
	fread(frame_, len_, 1, fp);

	devstat_ = 0;	// device is now ready
    fclose(fp);
}

/*
 * StillYuvGraber
 */
int StillYuvGrabber::command(int argc, const char* const* argv) {
#ifdef DEBUG
	debug_msg("StillYuvGrabber::command argc=%d\n", argc);
	for (int i = 0; i < argc; i++)
		debug_msg("\"%s\"\n", argv[i]);
#endif
	Tcl& tcl = Tcl::instance();

    if (argc == 2)
    {
        if (strcmp(argv[1], "status") == 0)
        {
            sprintf(tcl.buffer(), "%d", status_);
            tcl.result(tcl.buffer());
            return (TCL_OK);
        }
        if (strcmp(argv[1], "need-capwin") == 0)
        {
            tcl.result("0");
            return (TCL_OK);
        }
    }

    if (argc == 3)
    {
        if (strcmp(argv[1], "q") == 0)
        {
            return (TCL_OK);
        }
        if (strcmp(argv[1], "decimate") == 0)
        {
            decimate_ = atoi(argv[2]);
            setsize();
            if (running_)
                start();
        }
    }
    return (Grabber::command(argc, argv));
}

StillYuvGrabber::StillYuvGrabber() :
	width_(0), height_(0), num_frame_(0)
{
	// set device status 
	status_ = yuv_device.devstat_;
}

StillYuvGrabber::~StillYuvGrabber() {
#ifdef DEBUG
    debug_msg("Destroy StillYuvGrabber\n");
#endif
}

void StillYuvGrabber::start() {
	Grabber::start();
}

void StillYuvGrabber::stop() {
    cancel();
}

void StillYuvGrabber::setsize() {
#ifdef DEBUG
	debug_msg("StillYuvGrabber::setsize()\n");
#endif

	if(running_)
		stop();

	// CIF frame size in pixel
	width_ = 352;
	height_ = 288;

	set_size_420(width_, height_);
	//crinit(width_, height_);
	allocref();
}

int StillYuvGrabber::grab() {
#ifdef DEBUG
	debug_msg("StillYuvGrabber::grab() called\n");
#endif

    int frc = 0; 

	// "framesize_" is just the number of pixels, 
	// so the number of bytes becomes "3 * framesize_ / 2"
	memcpy (frame_, yuv_device.frame_ + num_frame_, 
			framesize_ + (framesize_ >> 1));

	if ((num_frame_ += framesize_ + (framesize_ >> 1)) 
			< yuv_device.len_) {
		// we are good here
	} else {
		num_frame_=0;
	}
 	
#ifdef DEBUG
	debug_msg(" number of frames: %d\n", num_frame_);
#endif

	suppress(frame_);
	saveblks(frame_);
	YuvFrame f(media_ts(), (u_int8_t *) frame_, crvec_, outw_, outh_);

	frc = target_->consume(&f);
    return frc;
}
