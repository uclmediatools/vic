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

#ifndef vic_tfwc_rcvr_h
#define vic_tfwc_rcvr_h

#include "tfwc_sndr.h"

// AckVec Size (AVSZ)
// - AckVec can manage "16 * AVSZ" outstanding packets
#define	AVSZ	256		// tfwcAV size

class TfwcRcvr {
public:
	TfwcRcvr();
	void tfwc_rcvr_recv_aoa(u_int16_t type, u_int16_t *chunk, int num_chunks);
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
	inline u_int16_t tfwc_rcvr_numvec() { return currNumVec_; }

	/*
	 * Variables
	 */
	u_int16_t *tfwcAV;		// AckVec array (bit vector array)
	u_int16_t currseq_;		// current sequence number
	u_int16_t prevseq_;		// previous sequence number
	u_int16_t ackofack_;	// ackofack
	u_int16_t begins_;		// begin seqno that XR chunk is reporting
	u_int16_t ends_;		// end seqno + 1 that XR chunk is reporting
	u_int16_t currNumElm_;	// number of current AckVec elements
	u_int16_t prevNumElm_;	// number of previous AckVec elements
	int	currNumVec_;		// numver of current AckVec array
	int prevNumVec_;		// numver of previous AckVec array

private:
	// calculate the number of AckVec chunks
	// (based on the number of given elements, i.e, numelm)
	inline int getNumVec (int numelm) {
		int num = numelm/16 + 1;
		if (numelm%16 == 0) num -= 1;
		return num;
	}

	// returning AckOfAck
	inline u_int16_t ackofack() { return ackofack_; }

	// print built AckVec
	void print_ackvec(u_int16_t *ackv);

	/*
	 * Variables (private)
	 */
	u_int32_t ts_echo_;	// for time stamp echoing
};

#endif
