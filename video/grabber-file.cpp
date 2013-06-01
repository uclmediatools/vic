/*
 * FILE:	grabber-file.cpp
 * AUTHOR:	Soo-Hyun Choi <s.choi@cs.ucl.ac.uk>
 * 		Piers O'Hanlon <p.ohanlon@cs.ucl.ac.uk>
 *
 * Copyright (c) 2011 University College London
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

class FileGrabber : public Grabber {
public:
	FileGrabber();
	virtual ~FileGrabber();
	virtual int command(int argc, const char* const * argv);
protected:
	void start();
	void stop();
	int	grab();
};

class FileYuvGrabber : public FileGrabber {
public:
	FileYuvGrabber();
	virtual ~FileYuvGrabber();
	virtual int command(int argc, const char* const* argv);
	
protected:
	void start();
	void stop();
	int grab();
	void setsize();

	int decimate_;
	int width_;			// width in pixel
	int height_;		// height in pixel
	int nbytes_;		// current bytes
private:
	void fps(int v);
	double ini_ftime_;	// initial frametime
};

class FileDevice : public InputDevice {
public:
    FileDevice(const char* s);
    virtual int command(int argc, const char * const * argv);
    virtual Grabber* jpeg_grabber();
    virtual Grabber* yuv_grabber();

    void load_file(const char * const file);
    u_char *frame_;
    int len_;
    int devstat_;	// device status
    Grabber* grabber_;
private:
};

static FileDevice filegrab_device("filegrab");

FileDevice::FileDevice(const char* s) : InputDevice(s),
		frame_(NULL), len_(0), devstat_(-1)
{
    attributes_ = "format { 411 cif } size { small large cif }";
    
#ifdef DEBUG
    debug_msg("FileDevice::FileDevice name=%s\n", s);
#endif /* DEBUG */
}

/*
 * FileDevice
 */
int FileDevice::command(int argc, const char*const* argv)
{
#ifdef DEBUG
	for (int i = 0; i < argc; i++)
		debug_msg("FileDevice\t%s\n", argv[i]);
#endif
    if (argc == 3)
    {
	if (strcmp(argv[1], "open") == 0)
	{
	    if (frame_ != 0) {
		const char* fmt = argv[2];
		TclObject* o = 0;
		if (strcmp(fmt, "cif") == 0)
			    o = yuv_grabber();
		if (strcmp(fmt, "jpeg") == 0)
			    o = jpeg_grabber();
		if (o != 0) {
			    Tcl::instance().result(o->name());
			    grabber_=(Grabber*)o;
		}
	    }
	    return (TCL_OK);
	}
	else if (strcmp(argv[1], "file") == 0)
	{
	    debug_msg("Loading %s\n", argv[2]);
	    load_file(argv[2]);
	    return (TCL_OK);
	}
    }
    return (InputDevice::command(argc, argv));
}

Grabber* FileDevice::jpeg_grabber()
{
    return (new FileGrabber());
}

Grabber* FileDevice::yuv_grabber()
{
	return (new FileYuvGrabber());
}

void FileDevice::load_file(const char * const f)
{
    FILE *fp;
    struct stat s;
	size_t r;
    
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
	delete[] frame_; //SV-XXX: Debian
    
    frame_ = new u_char[len_ + 1];
    fread(frame_, len_, 1, fp);

    debug_msg("Successfully loaded %s\n", f);
    devstat_ = 0;	// device is now ready
    fclose(fp);
}

/*
 * FileGrabber
 */
int FileGrabber::command(int argc, const char * const * argv)
{
    //SV-XXX: unused: Tcl& tcl = Tcl::instance();
    
#ifdef DEBUG
    debug_msg("FileGrabber::command argc=%d\n", argc);
    int i;
    for (i = 0; i < argc; i++)
	debug_msg("\"%s\"\n", argv[i]);
    debug_msg("\n");
#endif /* DEBUG */

    if (argc == 3)
    {
		if (strcmp(argv[1], "q") == 0)
		    return (TCL_OK);
	}
    return (Grabber::command(argc, argv));
}

FileGrabber::FileGrabber() 
{
	// set device status
	status_ = filegrab_device.devstat_;
}

FileGrabber::~FileGrabber()
{
#ifdef DEBUG
    debug_msg("Destroy FileGrabber\n");
#endif
}

void FileGrabber::start()
{
	Grabber::start();
}

void FileGrabber::stop()
{
    cancel();
}

int FileGrabber::grab()
{
#ifdef DEBUG
	debug_msg("FileGrabber::grab() called\n");
#endif
    int frc=0; //SV-XXX: gcc4 warns for initialisation
    if (filegrab_device.frame_) {
	JpegFrame f(media_ts(), (u_int8_t *) filegrab_device.frame_,
		    filegrab_device.len_,
		    80, 0, 320, 240);
	frc = target_->consume(&f);
	}

    return frc;
}

/*
 * FileYuvGraber
 */
int FileYuvGrabber::command(int argc, const char* const* argv)
{
#ifdef DEBUG
	debug_msg("FileYuvGrabber::command argc=%d\n", argc);
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
		if (strcmp(argv[1], "send") == 0) {
			if (atoi(argv[2])) {
				if (!running_) {
				start();
				running_ = 1;
				}
			} else {
				if (running_) {
				stop();
				running_ = 0;
				}
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "fps") == 0)
		{
			fps(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "decimate") == 0)
		{
			decimate_ = atoi(argv[2]);
			setsize();
		}
	}
	return (Grabber::command(argc, argv));
}

FileYuvGrabber::FileYuvGrabber() :
	width_(0), height_(0), nbytes_(0)
{
	grabber_ts_off_ = grabber_now();
	frame_=filegrab_device.frame_;
	fprintf(stderr,"FileYuvGrabber::FileYuvGrabber:frame_:%x\n",frame_);
}

FileYuvGrabber::~FileYuvGrabber()
{
#ifdef DEBUG
    debug_msg("Destroy FileYuvGrabber\n");
#endif
}

void FileYuvGrabber::fps(int v) {
	Grabber::fps(v);
	ini_ftime_ = 1e6 / double(v);
}

void FileYuvGrabber::start()
{
	target_->offset_ = grabber_ts_off_;
	// XXX Need to change frame_ - it used for storage of video frame and (re)allocated elsewhere
	frame_=filegrab_device.frame_;
	fprintf(stderr,"FileYuvGrabber::start:frame_:%x\n",frame_);
	Grabber::start();
}

void FileYuvGrabber::stop()
{
    cancel();
}

void FileYuvGrabber::setsize()
{
#ifdef DEBUG
	debug_msg("FileYuvGrabber::setsize()\n");
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

int FileYuvGrabber::grab()
{
#ifdef DEBUG
	debug_msg("FileYuvGrabber::grab() called\n");
#endif

        int frc=0; //SV-XXX: gcc4 warns for initialisation

#ifdef SUSPEND_GRAB
	// check if Tx queue is growing too much.
	// if so, we should suspend grabbing more frames.
	if (target_->suspend_grabbing(5)){
          if (fps_ > 0) fps(--fps_);
        } else if (fps_ < 30) fps(++fps_);
          //return (frc);
	debug_msg("now: %f, fps_: %d\n", fps_);
#endif

	// time measurement
	debug_msg("grab: fps %d, bps: %d\n", fps_, bps_);
	start_grab_ = grabber_now() - grabber_ts_off_;
	fprintf(stderr, "start_grab\tnow: %f\n", start_grab_);

	// "framesize_" is just the number of pixels, 
	// so the number of bytes becomes "3 * framesize_ / 2"

	int stride= framesize_ + (framesize_ >> 1);
	int frame_no=nbytes_/stride;
	fprintf(stderr,"Frame_no %d, number of bytes: %d (fsz:%d)\n",frame_no,  nbytes_, framesize_);
	memcpy (frame_, filegrab_device.frame_ + nbytes_, stride);

	if ((nbytes_ += stride) >= filegrab_device.len_) {
		nbytes_=0;
	}
 	
#ifdef DEBUG
	debug_msg(" number of bytes: %d\n", nbytes_);
#endif

	suppress(frame_);
	saveblks(frame_);
	YuvFrame f(media_ts(), (u_int8_t *) frame_, crvec_, outw_, outh_,0,frame_no);

	// time measurement
	end_grab_ = grabber_now() - grabber_ts_off_;
	fprintf(stderr, "end_grab\tnow: %f\n", end_grab_);
	fprintf(stderr, "num: %f\tgrab_time: %f\n",
		end_grab_, end_grab_ - start_grab_);

	frc = target_->consume(&f);
        return frc;
}
