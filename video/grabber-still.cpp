/*
 * Copyright (c) 1993-1995 Regents of the University of California.
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
 * Contributed by Bob Olson (olson@mcs.anl.gov) September 1995.
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header$ (LBL)";
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
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

#define DEBUG 1

class StillGrabber : public Grabber {
	public:
		StillGrabber();
		virtual ~StillGrabber();
		virtual int command(int argc, const char* const * argv);
	protected:
		void start();
		void stop();
		int grab();
		virtual void setsize() = 0;
};

class StillJpegGrabber : public StillGrabber {
	public:
		StillJpegGrabber();
		virtual ~StillJpegGrabber();
		virtual int command(int argc, const char* const * argv);
		virtual void setsize();
};

class StillYuvGrabber : public StillGrabber {
	public: 
		StillYuvGrabber();
		virtual ~StillYuvGrabber();
		virtual int command(int argc, const char* const * argv);
		virtual void setsize();
		virtual int grab();
	protected:
		//virtual void start();
		//virtual void stop();
		u_char* frame_;
		u_int	decimate_;		// division of base size for down-sampling
		u_int	basewidth_;		// frame height to be captured
		u_int	baseheight_;	// frame width to be camptured
};

class StillDevice : public InputDevice {
	public:
		StillDevice(const char* s);
		virtual int command(int argc, const char * const * argv);
		virtual Grabber* jpeg_grabber();
		virtual Grabber* yuv_grabber();

		void load_file(const char * const file);
		char *frame_;
		int len_;
	protected:
		FILE *fp_;
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
	struct stat s;

	fp_ = fopen(f, "r");

	if (fp_ == (FILE *) NULL)
	{
		perror("\tcannot open file");
		return;
	}

	if (fstat(fileno(fp_), &s) < 0)
	{
		perror("\tcannot stat file");
		fclose(fp_);
		return;
	}

	len_ = s.st_size;
	debug_msg("len: %d bytes\n", len_);	// file size in bytes
	
	if (frame_)
		delete[] frame_; //SV-XXX: Debian

	frame_ = new char[len_ + 1];
	fread(frame_, len_, 1, fp_);

	fclose(fp_);
	debug_msg("Loaded file successfully!\n");
}

int StillDevice::command(int argc, const char*const* argv)
{
	if (argc == 3)
	{
		#ifdef DEBUG
			for (int i = 0; i < argc; i++)
				debug_msg("argv[%d]: %s\n",i, argv[i]);
		#endif
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

			TclObject* o = new StillYuvGrabber();
			o = yuv_grabber();
			if(o != 0)
				Tcl::instance().result(o->name());
			return (TCL_OK);
		}
	}
	return (InputDevice::command(argc, argv));
}


Grabber* StillDevice::jpeg_grabber()
{
	debug_msg("Grabber* StillDevice::jpeg_grabber() called\n");
	return (new StillJpegGrabber());
}

Grabber* StillDevice::yuv_grabber()
{
	debug_msg("Grabber* StillDevice::yuv_grabber() called\n");
	return (new StillYuvGrabber());
}

//
// StillGrabber Constructor/Destructor
//
StillGrabber::StillGrabber()
{
#ifdef DEBUG
	debug_msg("StillGrabber called\n");
#endif
}

StillGrabber::~StillGrabber()
{
#ifdef DEBUG
	debug_msg("Destroy StillGrabber\n");
#endif
}

//
// StillJpegGrabber Constructor/Destructor
//
StillJpegGrabber::StillJpegGrabber()
{
#ifdef DEBUG
	debug_msg("StillJpegGrabber called\n");
#endif
}

StillJpegGrabber::~StillJpegGrabber()
{
#ifdef DEBUG
	debug_msg("Destroy StillJpegGrabber\n");
#endif
}

//
// StillYuvGrabber Constructor/Destructor
//
StillYuvGrabber::StillYuvGrabber() : frame_(0)
{
#ifdef DEBUG
	debug_msg("StillYuvGrabber called\n");
#endif
}

StillYuvGrabber::~StillYuvGrabber()
{
#ifdef DEBUG
	debug_msg("Destroy StillYuvGrabber\n");
#endif
}

int StillGrabber::command(int argc, const char * const * argv)
{
	//SV-XXX: unused: Tcl& tcl = Tcl::instance();

#ifdef DEBUG
	debug_msg("\t\tStillGrabber::command argc = %d \n", argc);
	int i;
	for (i = 0; i < argc; i++)
		debug_msg("\t\t\t\t\"%s\" ", argv[i]);
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

int StillJpegGrabber::command(int argc, const char * const * argv) {

	debug_msg("StillJpegGrabber::command argc = %d\n");
	for (int i = 0; i < argc; i++)
		debug_msg("\"%s\"", argv[i]);
}

void StillJpegGrabber::setsize() {
}

int StillYuvGrabber::command(int argc, const char * const * argv) {

	debug_msg("\t\tStillYuvGrabber::command argc = %d\n");
	for (int i = 0; i < argc; i++)
		debug_msg("\t\t\t\t\"%s\"", argv[i]);

	if (argc == 3) {
		if (strcmp(argv[1], "decimate") == 0) {
			int d = atoi(argv[2]);
			//Tcl& tcl = Tcl::instance();

			decimate_ = d;
			setsize();

			return (TCL_OK);
		}
	}
}

void StillYuvGrabber::setsize() {

	assert(!running_);

	u_int w, h;
	w = basewidth_ / decimate_;
	h = baseheight_ / decimate_;

	set_size_cif(w, h);
	allocref();
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

int StillYuvGrabber::grab() {

	suppress(frame_);
	saveblks(frame_);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);

	return (target_->consume(&f));
}
