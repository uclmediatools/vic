/*
 * Copyright (c) 2008 University College London
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
 *  This product includes software developed by the MASH Research
 *  Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
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

#include "assert.h"
#include "config.h"
#include "timer.h"
#include "rtp.h"
#include "inet.h"
#include "pktbuf-rtp.h"
#include "vic_tcl.h"
#include "module.h"
#include "transmitter.h"
#include "tfwc_sndr.h"

#define DUPACKS	3	// simulating TCP's 3 dupacks

TfwcSndr::TfwcSndr() :
	seqno_(0),
	aoa_(0),
	ts_(0),
	ts_echo_(0),
	npkt_(0)
{
	// for simulating TCP's 3 dupack rule
	u_int32_t mvec_ = 0x07;
}

void TfwcSndr::tfwc_sndr_send(pktbuf* pb) {

	// get RTP hearder information
	rtphdr* rh =(rtphdr*) pb->data;
	seqno_ = ntohs(rh->rh_seqno);

	// sequence number must be greater than zero
	assert (seqno_ > 0);
	debug_msg("sent seqno:		%d\n", seqno_);

	npkt_++;	// number of packet sent
}

void TfwcSndr::tfwc_sndr_recv(u_int32_t ackv, u_int32_t ts_echo)
{
	// retrieve ackvec and ts echo
	ackv_ = ackv;
	ts_echo_ = ts_echo;

	// mask most 3 recent packets
	if (npkt_ > DUPACKS)
		ackv_ = ackv | mvec_;

	tao_ = tfwc_sndr_now() - ts_echo_;

	debug_msg(" ts echo:	%d\n", ts_echo_);
}

void TfwcSndr::ackofack() {
	aoa_ = mvec_ | 0x01000000;
}
