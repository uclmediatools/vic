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

// set AckVec bitmap
//#define SET_BIT_VEC(ackvec_, bit) (ackvec_ = ((ackvec_ << 1) | bit))

// see AckVec bitmap
//#define SEE_BIT_VEC(ackvec_, ix, seqno) ((1 << (seqno - ix)) & ackvec_)

class TfwcRcvr {
public:
	TfwcRcvr();
	void tfwc_rcvr_recv(u_int16_t seqno, u_int16_t ackofack, u_int32_t ts);

protected:
	inline u_int32_t tfwc_rcvr_getvec() { return tfwcAV; }
	inline u_int32_t tfwc_rcvr_ts_echo() { return ts_echo_; }
	u_int32_t tfwcAV;	// AckVec (bit vector)
	u_int16_t currseq_;	// current sequence number
	u_int16_t prevseq_;	// previous sequence number
	u_int16_t ackofack_;	// ackofack
private:
	u_int32_t ts_echo_;	// for time stamp echoing
};

#endif
