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

TfwcSndr::TfwcSndr() :
	seqno_(0),
	cwnd_(1),
	aoa_(0),
	now_(0),
	ts_(0),
	ts_echo_(0),
	last_ack_(0),
	ndtp_(0),
	nakp_(0),
	ntep_(0),
	epoch_(1)
{
	// for simulating TCP's 3 dupack rule
	u_int32_t mvec_ = 0x07;
	UNUSED(mvec_);	// to shut up gcc-4.x
}

void TfwcSndr::tfwc_sndr_send(pktbuf* pb) {

	// get RTP hearder information
	rtphdr* rh =(rtphdr*) pb->data;

	// get seqno and mark timestamp for this data packet
	seqno_ = ntohs(rh->rh_seqno);
	now_ = tfwc_sndr_now();

	// timestamp vector for loss history update
	tsvec_[seqno_%TSZ] = now_;

	// sequence number must be greater than zero
	assert (seqno_ > 0);
	debug_msg("sent seqno:		%d\n", seqno_);

	ndtp_++;	// number of data packet sent
}

void TfwcSndr::tfwc_sndr_recv(u_int16_t type, u_int32_t ackv, u_int32_t ts_echo)
{
	// retrieve ackvec
	if (type == XR_BT_1) {
		nakp_++;		// number of ackvec packet received
		ackv_ = ackv;	// store ackvec

		// store head of ackvec as last ack (real number)
		last_ack_ = get_head_pos(ackv_) * epoch_;

		// generate margin vector
		marginvec(ackv_);
		ackv_ |= mvec_;		// masking ackvec

		// set ackofack (real number)
		aoa_ = ackofack(mvec_) * epoch_;

	}
	// retrieve ts echo
	else if (type == XR_BT_3) {
		ntep_++;		// number of ts echo packet received
		ts_echo_ = ts_echo;

		tao_ = now_ - ts_echo_;
		debug_msg(" ts echo:	%d\n", ts_echo_);
	}
}
