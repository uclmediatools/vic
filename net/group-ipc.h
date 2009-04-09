/*
 * Copyright (c) 1991-1994 The Regents of the University of California.
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

#ifndef vat_group_ipc_h
#define vat_group_ipc_h

#include "config.h"
#include "inet.h"
#include "iohandler.h"

#define GIPC_MTU 512
#define GIPC_MAGIC 0x0ef5ee0a

#define		GIPC_AA_WANT		1
#define		GIPC_AA_RELEASE		2
#define		GIPC_RANK		3
#define		GIPC_TALK		4

struct ipchdr {
	u_int32_t magic;	/* detect bogus traffic from inpcb bug */
	u_short type;
	u_short pid;
};

class IPCHandler {
 protected:
	IPCHandler(int typemask);
	virtual ~IPCHandler(){}; //SV-XXX: This solves the "missing" virtual destructor warning from gcc4
 public:
	virtual void ipc_input(int type, int frompid,
			       u_char* msg, int len) = 0;
	IPCHandler* next_;
	inline int mask() const { return (mask_); }
 private:
	int mask_;
};

/*
 * A class for group ipc.  We use a IP multicast socket with ttl 0
 * to effect multi-process communication.  A single, reserved multicast
 * address and unique port numbers gives us a set of communication
 * "channels" (the port number is GROUP_IPC_PORT + channel).
 * The convention is that channel 0 is used globally, among all
 * conference applications, while other channels are private to
 * individual conferences.  Channels numbers only need to be unique
 * at the local host, and can be easily specified with a conference
 * control tool (e.g., sd).
 */
class GroupIPC : public IOHandler {
    public:
	GroupIPC(int channel);
	~GroupIPC();
	void attach(IPCHandler*);
	void send(int type, int len);
	void send(int type, const char* msg);
	inline u_char* buffer() { return ((u_char*)buffer_ + sizeof(ipchdr)); }
    protected:
	virtual void dispatch(int);
    private:
	IPCHandler* handlers_;
	int rsock_;
	int ssock_;
	int pid_;
	u_long buffer_[GIPC_MTU / sizeof(u_long)];
};

#endif
