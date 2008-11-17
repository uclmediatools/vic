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
#include "tfwc_rcvr.h"

TfwcRcvr::TfwcRcvr() :
	currseq_(0),
	prevseq_(0),
	ackofack_(0)
{
	tfwcAV = 0;
}

void TfwcRcvr::tfwc_rcvr_recv(u_int16_t type, u_int16_t seqno, 
				u_int16_t ackofack, u_int32_t ts) 
{
	// count and offset
	int cnt, offset;

	// parse the received seqno and ackofack
	if (type == XR_BT_1) {
		//debug_msg("received seqno:  %d\n", seqno);
		currseq_ = seqno;
		ackofack_ = ackofack;

		// there is no packet loss
		if (currseq_ == prevseq_ + 1) {
			// set next bit to 1
			SET_BIT_VEC(tfwcAV, 1);
		} 
		// we have one or more packet loss
		else {
			// number of packet loss
			cnt = currseq_ - prevseq_ - 1;

			// set next bit to 0 equal to the number of lost packets
			for (int i = 0; i < cnt; i++) {
				SET_BIT_VEC(tfwcAV, 0);
			}

			// then, set this packet as received (this is important)
			SET_BIT_VEC(tfwcAV, 1);
		}

		// trim ackvec
		offset = currseq_ - ackofack_;
		if (ackofack_)
			trimvec(tfwcAV, offset);

		// set this seqno to the prevseq before exit
		prevseq_ = currseq_;
	}
	// parse timestamp
	else if (type == XR_BT_3) {
		ts_echo_ = ts;
	}
}
