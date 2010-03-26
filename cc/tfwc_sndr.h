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

#ifndef vic_tfwc_sndr_h
#define vic_tfwc_sndr_h

#include "bitmap.h"	// bitmap operations
#include "cc_timer.h"

#define DUPACKS 3   // simulating TCP's 3 dupacks
#define TSZ	1000	// tsvec_ size
#define SSZ 1000	// seqvec_ size
#define RSZ 1000	// refvec_ size

#define SHORT_HISTORY		// history size = 8
#ifdef  SHORT_HISTORY
#define HSZ 8   // history size for avg loss history
#else
#define HSZ 16  // history size for avg loss history
#endif

#define T_RTTVAR_BITS	2
#define T_SRTT_BITS		3

#define BITLEN	16

// timer related
#define TFWC_TIMER_RTX		0
#define TFWC_TIMER_RESET	1

class TfwcSndr;
class Transmitter;

// re-transmission timer
class TfwcRtxTimer : public CcTimerHandler {
public:
	TfwcRtxTimer(TfwcSndr *s) : CcTimerHandler() { s_ = s;}
	virtual void timeout();

protected:
	TfwcSndr *s_;
};

// TFWC sender class
class TfwcSndr {
public:
	// constructor
	TfwcSndr();
	virtual ~TfwcSndr() {};

	virtual void cc_tfwc_output() = 0;
	virtual double tx_ts_offset() = 0;

	// parse seqno and timestamp
	void tfwc_sndr_send(int, double);

	// main reception path (XR packet)
	void tfwc_sndr_recv(u_int16_t type, u_int16_t begin, u_int16_t end,
			u_int16_t *chunk, double so_rtime);

	// return ackofack
	inline u_int16_t tfwc_sndr_get_aoa() { return aoa_; }

	// return just acked seqno
	inline u_int16_t tfwc_sndr_jacked() { return jacked_; }

	// return tfwc controlled cwnd value
	inline u_int32_t tfwc_magic() { return cwnd_; };

	// set timestamp in u_int32_t type (TfwcSndr)
	inline u_int32_t tfwc_sndr_t_now() {
		timeval tv;
		::gettimeofday(&tv, 0);
		return (tv.tv_sec + tv.tv_usec);
	}

	// set timestamp in double type (TfwcSndr)
	inline double tfwc_sndr_now() {
		timeval tv;
		::gettimeofday(&tv, NULL);
		return ((double) tv.tv_sec + 1e-6 * (double) tv.tv_usec);
	}

	// return the current time
	inline double now() { return (tfwc_sndr_now()-ts_off_); }

	// return timestamp in u_int32_t type
	inline u_int32_t tfwc_sndr_get_ts() { return t_now_; }

	// variables
	u_int16_t seqno_;	// packet sequence number
	u_int32_t cwnd_;	// congestion window

	// Rtx timer
	void expire(int option);

protected:
	// generate sequence numbers
	void gen_seqvec(u_int16_t *v, int n);

	// generate reference seqno
	void gen_refvec(int end, int begin);

	// reset variables
	void reset_var();

	// get the first position in ackvec where 1 is marked
	inline u_int16_t get_head_pos(u_int16_t ackvec) {
		int l;
		for (l = 0; l < BITLEN; l++) {
			if(GET_HEAD_VEC(ackvec, l))
				break;
		}
		return (BITLEN - l);
	}
	// get the last position in ackvec where 1 is marked
	inline u_int16_t get_tail_pos(u_int16_t ackvec) {
		int l;
		for (l = 0; l < BITLEN; l++) {
			if(GET_TAIL_VEC(ackvec, l))
				break;
		}
		return (l + 1);
	}
	// generate margin vector
	inline void marginvec(u_int16_t hseq) {
		for (int i = 0; i < DUPACKS; i++) 
			// round up if it is less than zero
			mvec_[i] = ((hseq - i) < 0) ? 0 : (hseq - i);
	}
	// ackofack
	inline u_int16_t ackofack () {
		return ((mvec_[DUPACKS - 1] - 1) <= 0) ?
			0 : (u_int16_t) (mvec_[DUPACKS - 1] - 1);
	}
	// print mvec
	inline void print_mvec() {
		fprintf(stderr, "\tmargin numbers: ( %d %d %d )\n", 
				mvec_[0], mvec_[1], mvec_[2]);
	}
	// printf seqvec
	inline void print_seqvec(int numelm) {
		fprintf(stderr, "\tsequence numbers: (");
		for (int i = 0; i < numelm; i++)
			fprintf(stderr, " %d", seqvec_[i]);
		fprintf(stderr, " )\n");
	}
	// print vec
	inline void print_vec(u_int16_t *vec, int c) {
		fprintf(stderr, "\t(");
		for (int i = 0; i < c; i++)
			fprintf(stderr, " %d", vec[i]);
		fprintf(stderr, " )\n");
	}

	// retransmission timer
	TfwcRtxTimer rtx_timer_;
	void set_rtx_timer();
	void reset_rtx_timer(int backoff);
	void backoff_timer();

	int mvec_[DUPACKS]; // margin vec (simulatinmg TCP 3 dupacks)
	u_int16_t *ackv_;	// received AckVec (from TfwcRcvr)
	u_int16_t *pvec_;	// previous (stored) AckVec
	u_int16_t aoa_;		// ack of ack
	u_int32_t t_now_;	// the time when the data packet sent
	u_int32_t t_ts_;		// time stamp (u_int32_t type)
	u_int32_t t_ts_echo_;	// echo time stamp from the receiver
	double ts_;			// time stamp (double type)
	double ts_echo_;	// time stamp echo (double type)
	double now_;		// real-time now
	double so_recv_;	// SO_TIMESTAMP (XR packet reception)
	double tao_;		// sampled RTT

	Transmitter *tx_;

private:
	// update RTT
	void update_rtt(double tao);

	// detect packet loss
	// (to capture the very first lost packet loss)
	bool detect_loss();

	// control congestion window
	void control();

	// calcuate average loss interval
	void avg_loss_interval();
	void print_history_item (int);
	void print_history_item (int, int);

	// calculate loss history
	void loss_history();

	// estimate loss history and loss probability
	void pseudo_p();
	void pseudo_history();

	// generate weight factors
	void gen_weight();

	// dupack action
	void dupack_action();

	// new RTO
	void new_rto(double rtt);

	// determine out-of-ordered ack delivery
	bool out_of_ack (u_int16_t, u_int32_t*, int);

	// AckVec clone from Vic 
	inline void clone_ackv(u_int16_t *c, int n) {
		for (int i = 0; i < n; i++)
			ackv_[i] = ntohs(c[i]);
	}

	// copy AckVec to store
	inline void copy_ackv(int n) {
		for(int i = 0; i < n; i++)
			pvec_[i] = ackv_[i];
	}

	// clear timestamp vector
	inline void clear_tsv (int n) {
		for (int i = 0; i < n; i++)
			tsvec_[i] = 0;
	}

	// clear seqvec
	inline void clear_sqv (int n) {
		for (int i = 0; i < n; i++)
			seqvec_[i] = 0;
	}

	// clear ackvec
	inline void clear_ackv (int n) {
		for (int i = 0; i < n; i++)
			ackv_[i] = 0;
	}

	// clear ackvec
	inline void clear_pvec (int n) {
		for (int i = 0; i < n; i++)
			pvec_[i] = 0;
	}

	// clear refvec
	inline void clear_refv (int n) {
		for (int i = 0; i < n; i++)
			refvec_[i] = 0;
	}

	// number of ackvec chunks
	inline int get_numvec(int n) {
		return (n/BITLEN + (n%BITLEN > 0));	
	}

	// number of ackvec elements
	inline int get_numelm (int begin, int end) {
		return (end - begin + 1);
	}

	// replace jack'ed 
	inline void replace (u_int16_t highest) {
		jacked_ = highest;
	}

	// store jack'ed 
	inline void store (u_int16_t highest) {
		__jacked_ = highest;
	}

	int ndtp_;		// number of data packet sent
	int nakp_;		// number of ackvec packet received
	int ntep_;		// number of ts echo packet received
	int nsve_;		// number of seqvec element
	int epoch_;		// communication epoch

	bool is_running_;	// is TFWC running? 
	double ts_off_;		// timestamp offset for gettimeofday
	u_int32_t ref_t_time_;	// reference time (uint32 format)

	u_int32_t *seqvec_;		// generated seqno vec
	int	num_seqvec_;		// number of seqvec elements
	u_int32_t *refvec_;		// reference seqno vec
	int num_refvec_;		// number of refvec elements
	double *tsvec_;			// timestamp vector
	u_int16_t jacked_;		// just acked seqno (head of ackvec)
	bool is_first_loss_seen_;
	bool is_tfwc_on_;
	int num_missing_;	// number of missing seqno
	double f_p_;	// f(p) = sqrt(2/3)*p + 12*p*(1+32*p^2)*sqrt(3/8)*p
	double p_;		// packet loss probability
	double t_win_;      // temporal cwin size to get p_ value
	int tmp_cwnd_;      // temporary cwnd value
	double pseudo_p_;	// faked packet loss probability
	double pseudo_interval_;// faked loss interval
	double avg_interval_;	// average loss interval
	int history_[HSZ+1];	// loss interval history
	double weight_[HSZ+1];	// weight for calculating avg loss interval
	double I_tot_;		// total sum
	double I_tot0_;		// from 0 to n-1
	double I_tot1_;		// form 1 to n
	double tot_weight_;	// total weight
	int hsz_;		// current history size
	bool to_driven_;	// is TFWC being driven by timer-out?

	// RTT related variables
	double srtt_;	// smoothed RTT
	double rttvar_;	// RTT variation
	double rto_;	// retransmission timeout
	double minrto_;	// min RTO allowed
	double maxrto_;	// max RTO
	double df_;		// decay factor
	double sqrtrtt_;	// the mean of the sqrt of RTT

	// first lost packet (used only at the very first packet loss)
	int first_lost_pkt_;

	// XR chunk begin/end
	u_int16_t begins_;	// start seqno that this XR chunk reports
	u_int16_t ends_;	// end seqno + 1 that this XR chunk reports
	int	num_elm_;		// number of ackvec elements
	int num_vec_;		// numver of ackvec chunks

	// TCP's RTO calculation
	double alpha_;	// smoothing factor for RTT/RTO calculation
	double beta_;	// smoothing factor for RTT/RTO calculation
	double g_;		// timer granularity
	int k_;			// k value
	int t_rtt_;		// RTT
	int t_rttvar_;	// RTT variance
	int t_srtt_;	// smoothed RTT
	int srtt_init_;	// initial val for t_srtt_
	int rttvar_init_;	// initial val for t_rttvar_
	int rttvar_exp_;	// exponent of multiple for t0_
	double t0_;		// t0 value at TCP throughput equation
	double tcp_tick_;

	// highest/lowest packet sequence numbers (prev ackvec)
	u_int16_t __jacked_;	// previous highest packet sequence number
};

#endif
