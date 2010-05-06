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
#define PSR 10000	// packet size record

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

	// virtual functions
	virtual void cc_tfwc_output(bool recv_by_ch=0) {};
	virtual void cc_tfwc_output(pktbuf*) {};
	virtual void cc_tfwc_trigger(pktbuf* pb=0) {};
	virtual double tx_ts_offset() {};
	virtual int tx_buf_size() {};

	// parse seqno and timestamp
	void send(pktbuf*, double);

	// main reception path (XR packet)
	void recv(u_int16_t type, u_int16_t begin, u_int16_t end,
	u_int16_t *chunk, double so_rtime, bool recv_by_ch, pktbuf* pb);

	// return ackofack
	inline u_int16_t get_aoa() { return aoa_; }

	// just acked seqno in packets and in bytes
	inline u_int16_t jacked() { return jacked_; }
	inline int b_jacked() { return record_[bjacked_%PSR]; }

	// tfwc controlled cwnd value in packets and in bytes
	inline u_int32_t magic() { return cwnd_; }
	inline int b_magic() { return bcwnd_; }

	// set timestamp in double type (TfwcSndr)
	inline double tfwc_sndr_now() {
		timeval tv;
		::gettimeofday(&tv, NULL);
		return ((double) tv.tv_sec + 1e-6 * (double) tv.tv_usec);
	}

	// return the current time
	inline double now() { return (tfwc_sndr_now()-ts_off_); }

	// variables
	u_int16_t seqno_;	// packet sequence number
	u_int32_t cwnd_;	// congestion window
	int bcwnd_;			// congestion window in bytes
	int bjacked_;		// just ack'd in bytes

	// Rtx timer
	void expire(int option);

	// TfwcSndr instance
	static inline TfwcSndr& instance() { return instance_; }

protected:

	static TfwcSndr instance_;

	// generate sequence numbers
	void gen_seqvec(u_int16_t *v, int n);

	// generate reference seqno
	void gen_refvec(int end, int begin);

	// reset variables
	void reset_var(bool reverted);

	// get the first position in ackvec where 1 is marked
	inline u_int16_t get_head_pos(u_int16_t ackvec) {
		int l;
		for (l = 0; l < BITLEN; l++) {
		if(GET_HEAD_VEC(ackvec, l)) break;
		}
		return (BITLEN - l);
	}
	// get the last position in ackvec where 1 is marked
	inline u_int16_t get_tail_pos(u_int16_t ackvec) {
		int l;
		for (l = 0; l < BITLEN; l++) {
		if(GET_TAIL_VEC(ackvec, l)) break;
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

	// retransmission timer
	TfwcRtxTimer rtx_timer_;

	void set_rtx_timer();
	void reset_rtx_timer(int backoff);
	void backoff_timer();

	int mvec_[DUPACKS]; // margin vec (simulatinmg TCP 3 dupacks)
	u_int16_t *ackv_;	// received AckVec (from TfwcRcvr)
	u_int16_t *pvec_;	// previous (stored) AckVec
	u_int16_t aoa_;		// ack of ack
	double ts_;			// time stamp (double type)
	double ts_echo_;	// time stamp echo (double type)
	double now_;		// real-time now
	double so_recv_;	// SO_TIMESTAMP (XR packet reception)
	double tao_;		// sampled RTT
	double prev_ts_;

	// packet size
	int asize_;		// average packet size per frame
	int pcnt_;		// packet counter per frame
	int psize_;		// EWMA packet size 
	double lambda1_;	// EWMA coeff
	double lambda2_;	// EWMA coeff

private:
	// update RTT
	void update_rtt(double tao);

	// TCP-like Additive Increase
	// (until the very first packet loss)
	void tcp_like_increase();

	// detect packet loss
	// (to capture the very first lost packet loss)
	bool detect_loss();

	// TFWC congestion window in packets
	void window_in_packets(bool revert);
	void cwnd_in_packets(bool revert);
	// TFWC congestion window in bytes
	inline void window_in_bytes() { 
		bcwnd_ = psize_ * cwnd_;
	}

	// calcuate average loss interval
	void avg_loss_interval();
	void print_history_item (int);
	void print_history_item (int, int);
	bool revert_interval(int reseq);
	void record_history(int seqno, double interval, double ts);

	// calculate loss history
	void loss_history();

	// estimate loss history and loss probability
	double pseudo_p(int cwnd);
	void pseudo_history(double p);

	// generate weight factors
	void gen_weight();

	// dupack action
	void dupack_action(int seqno);

	// new RTO
	void new_rto(double rtt);

	// determine out-of-ordered ack delivery
	bool out_of_ack (u_int16_t, u_int32_t*, int);

	// keep packet conservation rule
	void packet_clocking (pktbuf* pb, bool flag);

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

	// clear previous ackvec
	inline void clear_pvec (int n) {
		for (int i = 0; i < n; i++)
		pvec_[i] = 0;
	}

	// clear refvec
	inline void clear_refv (int n) {
		for (int i = 0; i < n; i++)
		refvec_[i] = 0;
	}

	// clear record for packet size in bytes
	inline void clear_record (int n) {
		for (int i = 0; i < n; i++)
		record_[i] = 0;
	}

	// clear seqno that triggered a new loss event
	inline void clear_prev_interval (int n) {
		for (int i = 0; i < n; i++)
		prev_interval_[i] = 0;
	}

	// clear seqno that triggered a new loss event
	inline void clear_new_hist_seqno (int n) {
		for (int i = 0; i < n; i++)
		new_hist_seqno_[i] = 0;
		new_hist_seqno_size_ = 0;
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

	// find seqno
	bool find_seqno(u_int16_t *v, int n, int target);
	bool find_seqno(u_int32_t *v, int n, u_int32_t target);

	// print cwnd for debugging
	inline void print_cwnd() {
	fprintf(stderr, "\tnow: %f\tcwnd: %d\n", so_recv_, cwnd_);
	}

	// print bcwnd for debugging
	inline void print_bcwnd (double now, int w) {
	fprintf(stderr, "\tnow: %f\tbcwnd: %d\n", now, w);
	}

	// print received XR chunk info
	inline void print_xr_info(const char* str, const int i) {
	fprintf(stderr,
	"    [%s +%d] begins: %d ends: %d jacked: %d\n",
	str, i, begins_, ends_, jacked_);
	}

	// print RTT related info for debugging
	inline void print_rtt_info() {
	fprintf(stderr,
	"\t>> now_: %f tsvec_[%d]: %f rtt: %f srtt: %f\n",
	so_recv_, jacked_%TSZ, tsvec_[jacked_%TSZ], tao_, srtt_);
	}
	inline void print_rtt_info(const char* str) {
	fprintf(stderr,
	"\t%s now_: %f tsvec_[%d]: %f rtt: %f srtt: %f\n",
	str, so_recv_, jacked_%TSZ, tsvec_[jacked_%TSZ], tao_, srtt_);
	}

	// print ALI for debugging
	inline void print_ALI() {
	fprintf(stderr, "\tnow: %f\tALI: %f\n\n", so_recv_, avg_interval_);
	}

	// print packet's timestamp record
	inline void print_packet_tsvec() {
	fprintf(stderr, "\t>> now: %f tsvec_[%d]: %f\n",
	now_, seqno_%TSZ, tsvec_[seqno_%TSZ]);
	}

	// print mvec
	inline void print_mvec() {
	fprintf(stderr, "\tmargin numbers: ( %d %d %d )\n", 
	mvec_[0], mvec_[1], mvec_[2]);
	}
	// print vec
	inline void print_vec(const char* str, u_int32_t *vec, int c) {
	fprintf(stderr, "\t%s: (", str);
		for (int i = 0; i < c; i++)
		fprintf(stderr, " %d", vec[i]);
	fprintf(stderr, " )\n");
	}
	inline void print_vec(const char* str, u_int16_t *vec, int c) {
	fprintf(stderr, "\t%s: (", str);
		for (int i = 0; i < c; i++)
		fprintf(stderr, " %d", vec[i]);
	fprintf(stderr, " )\n");
	}

	// print the actual packet size and EWMA estimated one
	inline void print_psize(double now, int size, int len) {
	fprintf(stderr, "\tnow: %f psize: %d actual: %d\n", now, size, len);
	}

	int ndtp_;		// number of data packet sent
	int nakp_;		// number of ackvec packet received
	int ntep_;		// number of ts echo packet received
	int nsve_;		// number of seqvec element

	double ts_off_;		// timestamp offset for gettimeofday

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
	double avg_interval_;	// average loss interval
	int history_[HSZ+1];	// loss interval history
	int prev_history_[HSZ];// previous loss interval history
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

	// packet reordering
	bool reorder_;
	// highest/lowest packet sequence numbers (prev ackvec)
	u_int16_t __jacked_;	// previous highest packet sequence number
	double *prev_interval_; // previous avgerage intervals
	u_int16_t *new_hist_seqno_;	// seqno that introduced a new loss event
	int new_hist_seqno_size_;

	// record of packet size in bytes
	u_int16_t *record_;
};

#endif
