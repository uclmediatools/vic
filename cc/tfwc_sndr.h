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

#ifndef vic_tfwc_sndr_h
#define vic_tfwc_sndr_h

// set AckVec bitmap
#define SET_BIT_VEC(ackvec_, bit) (ackvec_ = ((ackvec_ << 1) | bit))

// see AckVec bitmap
#define SEE_BIT_VEC(ackvec_, ix, seqno) ((1 << (seqno - ix)) & ackvec_)

class TfwcSndr {
public:
	TfwcSndr();
	// parse RTP data packet from Transmitter module
	void tfwc_sndr_send(pktbuf*);
	void tfwc_sndr_recv(u_int32_t ackv, u_int32_t ts_echo);
	inline u_int16_t tfwc_sndr_get_seqno() { return seqno_; }
	inline u_int16_t tfwc_sndr_get_aoa() { return aoa_; }
	inline u_int32_t tfwc_sndr_now() {
		timeval tv;
		::gettimeofday(&tv, 0);
		return ((u_int32_t) tv.tv_sec + tv.tv_usec);
	}
	inline u_int32_t tfwc_sndr_get_ts() { return tfwc_sndr_now(); }
	void ackofack();	// set ack of ack
	u_int16_t seqno_;	// packet sequence number

protected:
	u_int32_t mvec_;	// margin vec (simulatinmg TCP 3 dupacks)
	u_int32_t ackv_;	// received AckVec (from TfwcRcvr)
	u_int32_t pvec_;	// sent packet list
	u_int16_t aoa_;		// ack of ack
	u_int32_t ts_;		// time stamp
	u_int32_t ts_echo_;	// echo time stamp from the receiver
	u_int32_t tao_;		// sampled RTT
private:
	int npkt_;		// number of packet sent
};

#endif
