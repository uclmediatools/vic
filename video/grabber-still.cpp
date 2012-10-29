/*
 * Copyright (c) 1993-1995 The Regents of the University of California.
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
 * Contributed by Bob Olson (olson@mcs.anl.gov) September 1995.
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

/* #define DEBUG 1 */

class StillGrabber : public Grabber {
public:
	StillGrabber();
	virtual ~StillGrabber();
	virtual int command(int argc, const char* const * argv);
protected:
	void start();
	void stop();
	int grab();
};

class StillDevice : public InputDevice {
public:
    StillDevice(const char* s);
    virtual int command(int argc, const char * const * argv);
    virtual Grabber* jpeg_grabber();

    void load_file(const char * const file);
    char *frame_;
    int len_;
};

static StillDevice still_device("still");

StillDevice::StillDevice(const char* s) : InputDevice(s),
		frame_(NULL), len_(0)
{
    attributes_ = "format { jpeg }";
    
#ifdef DEBUG
    debug_msg("StillDevice::StillDevice name=%s\n", s);
#endif /* DEBUG */
}

void StillDevice::load_file(const char * const f)
{
    FILE *fp;
    struct stat s;
    
    fp = fopen(f, "r");
    if (fp == (FILE *) NULL)
    {
	perror("cannot stat frame");
	return;
    }
    if (fstat(fileno(fp), &s) < 0)
    {
	perror("cannot stat frame");
	fclose(fp);
	return;
    }
    
    len_ = s.st_size;
    if (frame_)
	delete[] frame_; //SV-XXX: Debian
    
    frame_ = new char[len_ + 1];
    fread(frame_, len_, 1, fp);

    fclose(fp);
}

int StillDevice::command(int argc, const char*const* argv)
{
#ifdef DEBUG
    debug_msg("StillDevice %s\n", argv[2]);
#endif
    if (argc == 3)
    {
	if (strcmp(argv[1], "open") == 0)
	{
	    const char* fmt = argv[2];
	    TclObject* o = 0;
	    if (strcmp(fmt, "jpeg") == 0)
		o = jpeg_grabber();
	    if (o != 0)
		Tcl::instance().result(o->name());
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


Grabber* StillDevice::jpeg_grabber()
{
    return (new StillGrabber());
}

StillGrabber::StillGrabber()
{
}

StillGrabber::~StillGrabber()
{
#ifdef DEBUG
    debug_msg("Destroy StillGrabber\n");
#endif
}


int StillGrabber::command(int argc, const char * const * argv)
{
    //SV-XXX: unused: Tcl& tcl = Tcl::instance();
    
#ifdef DEBUG
    debug_msg("StillGrabber::command argc=%d ", argc);
    int i;
    for (i = 0; i < argc; i++)
	debug_msg("\"%s\" ", argv[i]);
    debug_msg("\n");
#endif /* DEBUG */

    if (argc == 3)
    {
	if (strcmp(argv[1], "q") == 0)
	{
	    return TCL_OK;
	}
    }

    return (Grabber::command(argc, argv));
}

void StillGrabber::start()
{
    frameclock_ = gettimeofday_usecs();

    timeout();
}

void StillGrabber::stop()
{
    cancel();
}

int StillGrabber::grab()
{
    int frc=0; //SV-XXX: gcc4 warns for initialisation

    if (still_device.frame_) {
	JpegFrame f(media_ts(), (u_int8_t *) still_device.frame_,
		    still_device.len_,
		    80, 0, 320, 240);
	frc = target_->consume(&f);
    }

    return frc;
}


