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
	ackofack_(0),
	begins_(0),
	ends_(0),
	currNumElm_(0),
	prevNumElm_(0),
	currNumVec_(0),
	prevNumVec_(0)
{
	tfwcAV = (u_int16_t *) malloc(sizeof(u_int16_t *));;
}

void TfwcRcvr::tfwc_rcvr_recv(u_int16_t type, u_int16_t seqno, 
				u_int16_t ackofack, u_int32_t ts) 
{
	// variables
	int numLoss		= 0;	// number of packet loss count
	int diffNumElm	= 0;	// difference of AckVec elements (curr vs. prev)
	int diffNumVec	= 0;	// difference of AckVec array (curr vs. prev)
	int addiNumVec	= 0;	// additional AckVec array required

	// parse the received seqno and ackofack
	if (type == XR_BT_1) {
		//debug_msg("received seqno:  %d\n", seqno);
		currseq_ = seqno;
		ackofack_ = ackofack;

		// number of required AckVec element
		currNumElm_	= currseq_ - ackofack_;
		diffNumElm	= currNumElm_ - prevNumElm_;

		// number of array required for building tfwcAV
		currNumVec_	= getNumVec(currNumElm_);
		diffNumVec	= currNumVec_ - prevNumVec_;

		// there is no packet loss
		if (currseq_ == prevseq_ + 1) {
			// set next bit to 1
			SET_BIT_VEC(tfwcAV[currNumVec_-1], 1);
		} 
		// we have one or more packet losses
		else {
			// number of packet loss
			numLoss = currseq_ - prevseq_ - 1;

			// we need more AckVec array (maybe one or more)
			if (currNumVec_ != prevNumVec_) {
				// currently available spaces in the previous tfwcAV array
				int numAvail = 16 - prevNumElm_%16;

				// first, fill up zeros into those available spaces
				for (int i = 0; i < numAvail; i++) {
					SET_BIT_VEC(tfwcAV[prevNumVec_-1], 0);
					numLoss--;
				}

				// then, calculate "additional" AckVec array required
				addiNumVec = getNumVec(numLoss);

				// fill up zeros accordingly if addiNumVec is greater than 1
				for (int i = 0; i < (addiNumVec - 1); i++) {
					for (int j = 0; j < 16; j++) {
						SET_BIT_VEC(tfwcAV[prevNumVec_ + i], 0);
						numLoss--;
					}
				}

				// finally, fill up zeros at the latest AckVec array
				for (int i = 0; i < (numLoss%16); i++) {
					SET_BIT_VEC(tfwcAV[prevNumVec_ + addiNumVec - 1], 0);
				}
			}
			// current num of AckVeck array can cope with the elements
			else {
				// set next bit 0 into AckVec (# of packet loss)
				for (int i = 0; i < numLoss; i++) 
					SET_BIT_VEC(tfwcAV[currNumVec_-1], 0);
			}

			// then, set this packet as received (this is important)
			SET_BIT_VEC(tfwcAV[currNumVec_-1], 1);
		}

		// print ackvec
		//print_ackvec(ackofack_, currseq_, tfwcAV);

		// start seqno that this AckVec is reporting
		if (ackofack_ != 0)
			begins_ = ackofack_ + 1;
		else
			begins_ = 1;

		// end seqno is current seqno plus one (according to RFC 3611)
		ends_ = currseq_ + 1;
	
		// store seqno, num of AckVec elem, and num of AckVec array
		prevseq_ = currseq_;
		prevNumElm_ = currNumElm_;
		prevNumVec_ = currNumVec_;
	}
	else if (type == XR_BT_2) {
		UNUSED(ts);	
	}
}

void TfwcRcvr::print_ackvec(u_int16_t begin, u_int16_t end, 
		u_int16_t bitvec) {

	int elm[64];
	int cnt = end - begin;

	printf("\tAckVec Built: ");
	for (int i = 0; i < cnt; i++) {
		if (CHECK_BIT_AT(bitvec, i+1))
			elm[i] = (begin + 1) + i;
		printf(" %d", elm[i]);
	}
	printf(" \n");
}
