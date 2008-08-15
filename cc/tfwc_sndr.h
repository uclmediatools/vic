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

#define DUPACKS 3   // simulating TCP's 3 dupacks
#define CHB	0x80000000	// ackvec check bit (head search)
#define CTB	0x01		// ackvec check bit (tail search)
#define TSZ	1000		// tsvec_ size
#define SSZ 1000		// seqvec_ size

#ifdef  SHORT_HISTORY
#define HSZ 8   // history size for avg loss history
#else
#define HSZ 16  // history size for avg loss history
#endif

// set AckVec bitmap from LSB
#define SET_BIT_VEC(ackvec_, bit) (ackvec_ = ((ackvec_ << 1) | bit))

// AckVec bitmap at i-th location
#define GET_BIT_VEC(ackvec_, i, seqno) ((1 << (seqno - i)) & ackvec_)

// AckVec head search
#define GET_HEAD_VEC(ackvec_, i) ( ackvec_ & (CHB >> i) )

// AckVec tail search
#define GET_TAIL_VEC(ackvec_, i) ( ackvec_ & (CTB << i) )

// check bit at i-th location
#define CHECK_BIT_AT(vec, i) ( vec & (1 << (i-1)) )

class TfwcSndr {
public:
	TfwcSndr();
	// parse RTP data packet from Transmitter module
	void tfwc_sndr_send(pktbuf*);

	// main reception path (XR packet)
	void tfwc_sndr_recv(u_int16_t type, u_int32_t ackv, u_int32_t ts_echo);

	// return current data packet's seqno
	inline u_int16_t tfwc_sndr_get_seqno() { return seqno_; }

	// return ackofack
	inline u_int16_t tfwc_sndr_get_aoa() { return aoa_; }

	// set timestamp in u_int32_t type (TfwcSndr)
	inline u_int32_t tfwc_sndr_t_now() {
		timeval tv;
		::gettimeofday(&tv, 0);
		return (tv.tv_sec + tv.tv_usec);
	}

	// set timestamp in double type (TfwcSndr)
	inline double tfwc_sndr_now() {
		timeval tv;
		::gettimeofday(&tv, 0);
		return ((double) tv.tv_sec + 1e-6 * (double) tv.tv_usec);
	}

	// return timestamp in u_int32_t type
	inline u_int32_t tfwc_sndr_get_ts() { return t_now_; }

	// variables
	u_int16_t seqno_;	// packet sequence number
	u_int32_t cwnd_;	// congestion window

protected:
	// get the first position in ackvec where 1 is marked (mod 32)
	inline u_int32_t get_head_pos(u_int32_t ackvec) {
		int l;
		for (l = 0; l < 32; l++) {
			if(GET_HEAD_VEC(ackvec, l))
				break;
		}
		return (32 - l);
	}
	// get the last position in ackvec where 1 is marked
	inline u_int32_t get_tail_pos(u_int32_t ackvec) {
		int l;
		for (l = 0; l < 32; l++) {
			if(GET_TAIL_VEC(ackvec, l))
				break;
		}
		return (l + 1);
	}
	// generate margin vector
	inline void marginvec(u_int32_t vec) {
		int hseq = get_head_pos(vec) + aoa_;	// ackvec head seqno

		for (int i = 0; i < DUPACKS; i++) {
			mvec_[i] = hseq - i;

			// round up if it is less than zero
			mvec_[i] = (mvec_[i] <= 0) ? 0 : mvec_[i];
		}
	}
	// generate seqno vector (interpret ackvec to real sequence numbers)
	inline void gen_seqno_vec(u_int32_t vec) {
		int hseq = get_head_pos(vec) + aoa_;	// ackvec head seqno
		int cnt = hseq - aoa_;
		
		for (int i = 0; i < cnt; i++) {
			seqvec_[i%SSZ]	 = hseq - i;
		}
	}
	// ackofack
	inline u_int16_t ackofack () {
		return (mvec_[DUPACKS - 1] - 1);
	}

	u_int32_t mvec_[DUPACKS]; // margin vec (simulatinmg TCP 3 dupacks)
	u_int32_t ackv_;	// received AckVec (from TfwcRcvr)
	u_int32_t pvec_;	// sent packet list
	u_int16_t aoa_;		// ack of ack
	u_int32_t t_now_;	// the time when the data packet sent
	u_int32_t ts_;		// time stamp
	u_int32_t ts_echo_;	// echo time stamp from the receiver
	double now_;		// real-time now
	double tao_;		// sampled RTT
private:
	// update RTT
	void update_rtt(double tao);

	// detect packet loss
	bool detect_loss(u_int32_t*, u_int16_t, u_int16_t);

	// control congestion window
	void control(u_int32_t* seqvec);

	// calcuate average loss interval
	void avg_loss_interval();

	// calculate loss history
	void loss_history(u_int32_t* seqvec);

	// estimate loss history and loss probability
	void pseudo_p();
	void pseudo_history();

	// generate weight factors
	void gen_weight();

	// dupack action
	void dupack_action();

	u_int16_t lastest_ack_;	// lastest seqno from ackvec
	u_int32_t *seqvec_;		// generated seqno vec
	double *tsvec_;	// timestamp vector
	int ndtp_;		// number of data packet sent
	int nakp_;		// number of ackvec packet received
	int ntep_;		// number of ts echo packet received
	int nsve_;		// number of seqvec element
	int epoch_;		// communication epoch
	bool is_loss_;
	bool is_first_loss_seen_;
	bool is_tfwc_on_;
	double f_p_;	// f(p) = sqrt(2/3)*p + 12*p*(1+32*p^2)*sqrt(3/8)*p
	double p_;		// packet loss probability
	double t_win_;      // temporal cwin size to get p_ value
	int tmp_cwnd_;      // temporary cwnd value
	double pseudo_p_;	// faked packet loss probability
	double pseudo_interval_;// faked loss interval
	double avg_interval_;	// average loss interval
	double history_[HSZ+1];	// loss interval history
	double weight_[HSZ+1];	// weight for calculating avg loss interval
	double I_tot_;		// total sum
	double I_tot0_;		// from 0 to n-1
	double I_tot1_;		// form 1 to n
	double tot_weight_;	// total weight
	int hsz_;		// current history size

	// RTT related variables
	double srtt_;	// smoothed RTT
	double rttvar_;	// RTT variation
	double rto_;	// retransmission timeout
	double minrto_;	// min RTO allowed
	double maxrto_;	// max RTO
	double alpha_;	// smoothing factor for RTT/RTO calculation
	double beta_;	// smoothing factor for RTT/RTO calculation
	double g_;		// timer granularity
	int k_;			// k value
	double t0_;		// t0 value at TCP throughput equation
	double df_;		// decay factor
	double sqrtrtt_;	// the mean of the sqrt of RTT

	// first lost packet (used only at the very first packet loss)
	int first_lost_pkt_;
};

#endif
