/*
 * Copyright (c) 2008-2010 University College London
 * All rights reserved.
 * 
 * AUTHOR: Soo-Hyun Choi <s.choi@cs.ucl.ac.uk>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor of the Laboratory may be used
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
	ackofack_(0),
	begins_(1),
	ends_(1),
	numElm_(1),
	numVec_(1)
{
	// tfwcAV (bit vector)
	tfwcAV = (u_int16_t *) malloc(sizeof(u_int16_t *));
	clear_avec(numVec_);
}

// retrive ackofack from RTCP control channel
void TfwcRcvr::tfwc_rcvr_recv_aoa(u_int16_t type, u_int16_t *chunk)
{
	int num_chunks = 1;
	switch (type) {
	case XR_BT_1:
	  {
	  // received ackofack 
	  ackofack_ = ntohs(chunk[num_chunks-1]);
	  }
	  break;
	case XR_BT_3:
	  {
	  // set timestamp echo
	  ts_echo_ = chunk[num_chunks-1];
	  }
	  break;
	default:
	  break;
	} // end switch (type)

	return;
}

// retrieve data packet sequence number from RTP data channel
void TfwcRcvr::tfwc_rcvr_recv_seqno(u_int16_t seqno)
{
	// required number of AckVec elements
	numElm_ = seqno - ackofack_;

	// required number of AckVec chunks
	numVec_ = numElm_/BITLEN + (numElm_%BITLEN > 0);

	// reset necessary variables before start
	reset();

	// reference vector
	for (int i = 1; i <= numElm_; i++)
		rvec_.push_back(ackofack_ + i);

	// push back the current seqno
	// (if this is duplicate seqno, skip adding it)
	if (find(avec_.begin(), avec_.end(), seqno) == avec_.end())
		avec_.push_back(seqno);
	sort(avec_.begin(), avec_.end());

	// then, trim upto ackofack (inclusive)
	avit_ = find(avec_.begin(), avec_.end(), ackofack_);
	if (avit_ != avec_.end()) 
		avec_.erase(avec_.begin(), ++avit_);

	// now, build tfwcAV chunks
	tfwc_ackvec();

	// set 'start seqno' that this AckVec reports
	begins_ = ackofack_ + 1;
	// set 'end seqno plus one' that this AckVec report
	ends_ = seqno + 1;

	// print refvec, actual vec, and bitvec
	//print_vec(rvec_);
	//print_vec(avec_);
	//print_tfwcAV();
}

// build tfwcAV chunks
void TfwcRcvr::tfwc_ackvec() {
	int cv = 0;	// vector counter
	int cb = 0;	// bit counter

	// find reference vector elements from the actual sequence vector
	for (rvit_ = rvec_.begin(); rvit_ != rvec_.end(); rvit_++) {
		// find rvec_ elements in avec_
		avit_ = find(avec_.begin(), avec_.end(), *rvit_);

		// found
		if (avit_ != avec_.end()) {
			if (cb < BITLEN) {
				tfwcAV[cv] = (tfwcAV[cv] << 1) | 1;
				cb++;
			} 
			if (cb == BITLEN) {
				cb = 0;
				cv++;
			}
		}
		// not found
		else {
			if (cb < BITLEN) {
				tfwcAV[cv] = (tfwcAV[cv] << 1) | 0;
				cb++;
			}
			if (cb == BITLEN) {
				cb = 0;
				cv++;
			}
		}
	} // end for(;;)
}

// print vector elements
void TfwcRcvr::print_vec(std::vector<int> v) {
	std::vector<int>::iterator iter;
	fprintf(stderr, "\t>> vec: ");
	for (iter = v.begin(); iter != v.end(); iter++)
		fprintf(stderr, "%d ", *iter);
	fprintf(stderr, "\n");
}

// print bit vectors (in decimal format)
void TfwcRcvr::print_tfwcAV() {
	fprintf(stderr, "\t>> tfwcAV: ");
	for (int i = 0; i < numVec_; i++)
		fprintf(stderr, "[%d:%d] ", i, tfwcAV[i]);
	fprintf(stderr, "\n");
}

// reset
void TfwcRcvr::reset() {
	rvec_.clear();
	clear_avec(numVec_);
}
