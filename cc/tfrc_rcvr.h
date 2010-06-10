/*
 * Copyright (c) 2010 University College London
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

#ifndef vic_tfrc_rcvr_h
#define vic_tfrc_rcvr_h

#include <vector>
#include <algorithm>
#include "tfrc_sndr.h"

class TfrcRcvr {
public:
	TfrcRcvr();

	// receive AoA
	void recv_aoa(u_int16_t type, u_int16_t *chunk);
	// receive RTP data 
	void recv_seqno(u_int16_t seqno);
    // AckVec clone
    inline u_int16_t getvec(int i) { return tfrcAV[i]; }
    // AckVec begin seqno
    inline u_int16_t begins() { return begins_; }
    // AckVec end seqno plus one
    inline u_int16_t ends() { return ends_; }
    // number of AckVec array
    inline u_int16_t numvec() { return numVec_; }

	// TfrcRcvr instance
	static inline TfrcRcvr& instance() { return instance_; }
	// Transmitter 
	inline void manager(Transmitter* tm) { tm_ = tm; }

protected:

	static TfrcRcvr instance_;
	Transmitter *tm_;

	/*
	 * Variables
	 */
	u_int16_t *tfrcAV;		// AckVec array (bit vector array)
	u_int16_t ackofack_;	// ackofack
	u_int16_t begins_;		// begin seqno that XR chunk is reporting
	u_int16_t ends_;		// end seqno + 1 that XR chunk is reporting
	int numElm_;			// number of tfrcAV elements
	int numVec_;			// number of tfrcAV chunks

private:
	// TFRC sender's AckVec Routine
	void tfrc_ackvec();
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

	// clear tfrcAV
	inline void clear_avec(int num) {
		for (int i = 0; i < num; i++)
		tfrcAV[i] = 0;
	}

	// print built AckVec
	void print_tfrcAV();
	void print_vec(std::vector<int> v);

	// actual AckVec and its iterator
	std::vector<int> avec_;
	std::vector<int>::iterator avit_;
	// reference vector and its iterator
	std::vector<int> rvec_;
	std::vector<int>::iterator rvit_;
};

#endif
