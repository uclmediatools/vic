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
 */

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include <stdlib.h>
#include <ctype.h>
#include "config.h"
#include "inet.h"
#include "group-ipc.h"
#include "vic_tcl.h"

class ConfBus : public TclObject, public IPCHandler {
    public:
	ConfBus(int channel);
	~ConfBus();
	void ipc_input(int type, int frompid, u_char* p, int len);
    protected:
	int command(int argc, const char*const* argv);
	GroupIPC* ipc_;
	char* callback_;
	int value1, value2;
};

static class ConfBusMatcher : public Matcher {
    public:
	ConfBusMatcher() : Matcher("confbus") { }
	TclObject* match(const char* channel) {
		return (new ConfBus(atoi(channel)));
	}
} confbus;

ConfBus::ConfBus(int channel)
	: TclObject(0), IPCHandler(~0), ipc_(0)
{
	value1=0;
	value2=0;
	callback_ = 0;
	ipc_ = new GroupIPC(channel);
	ipc_->attach(this);
}

ConfBus::~ConfBus()
{
	delete ipc_;
	delete[] callback_; //SV-XXX: Debian
}

void ConfBus::ipc_input(int, int, u_char* msg, int cc)
{
	//int value1, value2;
	if (cc <= 0 || callback_ == 0)
		return;

	if (cc == 4) {
		/*
		 * XXX backward compatibility with vat-3 voice-switching
		 * messages (i.e., pre-RTPv2 protocol).  If the message
		 * is four bytes, then it must be the binary coded IP address
		 * of the site who's audio channel became active.  Otherwise,
		 * it must be an new-style ascii conference bus command.
		 * Note an ascii command is necessarily larger than
		 * four bytes.
		 */
		u_int32_t addr = *(u_int32_t*)msg;
		sprintf((char*)msg, "focus %s", InetNtoa(addr));
	}

	/* if this is a power message then compare it with the last 
	message and if the value is not significantly different then return */
	if (sscanf((char*)msg, "relate_power %*s %d",&value1)!=0) {
		if (value1>0) {
			if (value1<value2) {
				if (value1+10>=value2) return;
				} else
				if (value1-10<=value2) return;
			value2=value1;
		}
	}

	Tcl& tcl = Tcl::instance();
	/*
	 * make sure message is null terminated and check that
	 * it's regular text that tcl can handle.
	 */
	int last = cc - 1;
	if (msg[last] != 0)
		return;
	for (int i = 0; i < last; ++i) {
		if (!isascii(msg[i]) && !isprint(msg[i]))
			return;
	}
	debug_msg("%s %s {%s}\n", callback_, name(), (char*)msg);
	tcl.evalf("%s %s {%s}", callback_, name(), (char*)msg);
}

/*
 * $bus write $type $msg
 * $bus handler $proc
 */
int ConfBus::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "handler") == 0) {
			const char* proc = argv[2];
			delete[] callback_; //SV-XXX: Debian
			callback_ = new char[strlen(proc) + 1];
			strcpy(callback_, proc);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "send") == 0) {
			/*XXX*/
			ipc_->send(0, argv[2]);
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}
