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

#ifndef vic_tfwc_rcvr_h
#define vic_tfwc_rcvr_h

#include <vector>
#include <algorithm>
#include "tfwc_sndr.h"

class TfwcRcvr {
public:
	TfwcRcvr();
	void tfwc_rcvr_recv_aoa(u_int16_t type, u_int16_t *chunk);
	void tfwc_rcvr_recv_seqno(u_int16_t seqno);

protected:
	// AckVec clone
	inline u_int16_t tfwc_rcvr_getvec(int i) { return tfwcAV[i]; }
	// ts echo
	inline u_int32_t tfwc_rcvr_ts_echo() { return ts_echo_; }

	// AckVec begin seqno
	inline u_int16_t tfwc_rcvr_begins() { return begins_; }
	// AckVec end seqno plus one
	inline u_int16_t tfwc_rcvr_ends() { return ends_; }
	// number of AckVec array
	inline u_int16_t tfwc_rcvr_numvec() { return numVec_; }

	/*
	 * Variables
	 */
	u_int16_t *tfwcAV;		// AckVec array (bit vector array)
	u_int16_t ackofack_;	// ackofack
    u_int16_t begins_;      // begin seqno that XR chunk is reporting
    u_int16_t ends_;        // end seqno + 1 that XR chunk is reporting
	int numElm_;			// number of tfwcAV elements
	int numVec_;			// number of tfwcAV chunks
private:
	// TFWC sender's AckVec Routine
	void tfwc_ackvec();
	void reset();

	// calculate the number of AckVec chunks
	// (based on the number of given elements, i.e, numelm)
	inline int getNumVec (int numelm) {
		int num = numelm/BITLEN + 1;
		if (numelm%BITLEN == 0) num--;
		return num;
	}

	// returning AckOfAck
	inline u_int16_t ackofack() { return ackofack_; }

	// clear tfwcAV
	inline void clear_avec(int num) {
		for (int i = 0; i < num; i++)
			tfwcAV[i] = 0;
	}

	// print built AckVec
	void print_tfwcAV();
	void print_vec(std::vector<int> v);

	/*
	 * Variables (private)
	 */
	u_int32_t ts_echo_;	// for time stamp echoing

	// actual AckVec and its iterator
	std::vector<int> avec_;
	std::vector<int>::iterator avit_;
	// reference vector and its iterator
	std::vector<int> rvec_;
	std::vector<int>::iterator rvit_;
};

#endif
