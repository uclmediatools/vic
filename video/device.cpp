/*
 * Copyright (c) 1995 The Regents of the University of California.
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include "grabber.h"
#include "device-input.h"
#include "device-output.h"

InputDevice::InputDevice(const char* nickname, const char* api) 
	: nickname_(nickname), api_(api), attributes_("")
{
}

void InputDevice::inception()
{
	Tcl::instance().evalf("lappend inputDeviceList %s", name());
}

int InputDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "nickname") == 0) {
			tcl.result(nickname_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "api") == 0) {
			tcl.result(api_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "attributes") == 0) {
			tcl.result(attributes_);
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

OutputDevice::OutputDevice(const char* nickname, const char* api)
	: nickname_(nickname), api_(api), attributes_("")
{
}

void OutputDevice::inception()
{
	Tcl::instance().evalf("lappend outputDeviceList %s", name());
}

int OutputDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "nickname") == 0) {
			tcl.result(nickname_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "api") == 0) {
			tcl.result(api_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "attributes") == 0) {
			tcl.result(attributes_);
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "assistor") == 0 ||
		    strcmp(argv[1], "renderer") == 0)
			/*
			 * subclass didn't recognize.  return an
			 * emptry string instead of an error
			 */
			return (TCL_OK);
	}
	return (TclObject::command(argc, argv));
}
