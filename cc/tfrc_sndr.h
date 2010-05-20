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

#ifndef vic_tfrc_sndr_h
#define vic_tfrc_sndr_h

#include "config.h"
#include "bitmap.h"
#include "cc_common.h"
#include "cc_timer.h"

class TfrcSndr;

// TFRC sender class
class TfrcSndr {
public:
	// constructor
	TfrcSndr();
	virtual ~TfrcSndr() {};

	// virtual functions
	virtual void tfrc_output(bool ack_clock=0) {UNUSED(ack_clock);};
	virtual void tfrc_output(pktbuf*) {};
	virtual double tx_ts_offset() {};
	virtual int tx_buf_size() {};

	// parse seqno and timestamp
	void send(pktbuf*, double);

	// main reception path
	void recv(u_int16_t, u_int16_t, u_int16_t,
		u_int16_t*, double, bool, pktbuf*);

	// return ackofack
	inline u_int16_t get_aoa() { return aoa_; }

	u_int16_t seqno_;	// packet sequence number
	double x_rate_;		// send rate (bytes/sec)

	// TfrcSndr instance
	static inline TfrcSndr& instance() { return instance_; }

protected:

	static TfrcSndr instance_;

	// generate sequence numbers
	void gen_seqvec(u_int16_t *v, int n);
	// generate reference seqno
	void gen_refvec(int end, int begin);
	// reset variables
	void reset_var(bool reverted);

	u_int16_t *ackv_;	// received AckVec
	u_int16_t aoa_;	// ack of ack
	double ts_;			// timestamp
	double now_;		// real-time now
	double so_recv_;	// SO_TIMESTAMP
	double tao_;		// sampled RTT

	// packet size
	int asize_;		// average packet size per frame
	int pcnt_;		// packet counter per frame
	int psize_;		// EWMA packet size
	double lambda1_;	// EWMA coeff
	double lambda2_;	// EWMA coeff

private:
	// update RTT
	void update_rtt(double tao);

	// TFRC congestion control
	void update_xrate();
	// calculate send rate
	void calc_xrate();
	// average loss interval
	void avg_loss_interval();
	void print_history_item (int);
	void print_history_item (int, int);
	// loss history
	void loss_history();

	// TCP-like increase function
	void tcp_like_increase();
	// detect the first loss
	bool detect_loss();
	// TCP-like dupack action
	void dupack_action(int seqno);
	// TC_like slow start
	void slow_start();

	// generate weight factors
	void gen_weight();

	// estimated loss history/loss probability
	double pseudo_p(double rate);
	void pseudo_history(double p);

	// find seqno
	bool find_seqno(u_int16_t *v, int n, int target);
	bool find_seqno(u_int32_t *v, int n, u_int32_t target);

	// AckVec clone from Vic
	inline void clone_ackv(u_int16_t *c, int n) {
		for (int i = 0; i < n; i++)
		ackv_[i] = ntohs(c[i]);
	}
	// number of ackvec elements
	inline int get_numvec(int n) {
	return (n/BITLEN + (n%BITLEN > 0));
	}
	// number of ackvec elements
	inline int get_numelm (int begin, int end) {
	return (end - begin + 1);
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
	// clear refvec
	inline void clear_refv(int n) {
		for (int i = 0; i < n; i++)
		refvec_[i] = 0;
	}
	// clear record for packet size in bytes
	inline void clear_record (int n) {
		for (int i = 0; i < n; i++)
		record_[i] = 0;
	}

	int ndtp_;	// number of data packet sent
	int nakp_;	// number of ackvec packet received
	int ntep_;	// number of ts_echo packet received
	int nsve_;	// number of seqvec element

	double ts_off_;	// timestamp offset for gettimeofday

	u_int32_t *seqvec_;	// generated seqno vec
	int num_seqvec_;	// number of seqvec elements
	u_int32_t *refvec_;	// reference seqno vec
	int num_refvec_;	// number of refvec elements
	double *tsvec_;		// timestamp vector
	u_int16_t jacked_;	// just acked seqno (head of ackvec)
	bool is_first_loss_seen_;
	bool is_tfrc_on_;
	int first_lost_pkt_;// very first lost pkt seqno
	int num_missing_;	// number of missing seqno
	double p_;			// packet loss probability
	double avg_interval_;	// average loss interval
	int history_[HSZ+1];	// loss interval history
	double weight_[HSZ+1];	// weight for calculating avg loss interval
	double I_tot_;		// total sum
	double I_tot0_;		// from 0 to n-1
	double I_tot1_;		// from 1 to n
	double tot_weight_;	// total weight
	int hsz_;		// current history size

	// RTT related variables
	double srtt_;	// smoothed RTT
	double rttvar_;	// RTT variation
	double rto_;	// retransmission timeout
	double minrto_;	// min RTO allowed
	double maxrto_;	// max RTO
	double df_;		// decay factor
	double sqrtrtt_;	// the mean of the sqrt of RTT

	// TCP's RTO calculation
	double alpha_;  // smoothing factor for RTT/RTO calculation
	double beta_;   // smoothing factor for RTT/RTO calculation
	double g_;      // timer granularity
	int k_;         // k value
	int t_rtt_;     // RTT
	int t_rttvar_;  // RTT variance
	int t_srtt_;    // smoothed RTT
	int srtt_init_; // initial val for t_srtt_
	int rttvar_init_;   // initial val for t_rttvar_
	int rttvar_exp_;    // exponent of multiple for t0_
	double t0_;     // t0 value at TCP throughput equation
	double tcp_tick_;

	// XR chunk begin/end
	u_int16_t begins_;	// start seqno that this XR chunk reports
	u_int16_t ends_;	// end seqno + 1 that this XR chunk reports
	int num_elm_;		// number of ackvec elements
	int num_vec_;		// number of ackvec chunks

	// record packet size in bytes
	u_int16_t *record_;


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
	// print the actual packet size and EWMA estimated one
	inline void print_psize(double now, int size, int len) {
	fprintf(stderr, "\tnow: %f psize: %d actual: %d\n", now, size, len);
	}
	// print packet's timestamp record
	inline void print_packet_tsvec() {
	fprintf(stderr, "\t>> now: %f tsvec_[%d]: %f\n",
	now_, seqno_%TSZ, tsvec_[seqno_%TSZ]);
	}
	// print ALI for debugging
	inline void print_ALI() {
	fprintf(stderr, "\tnow: %f\tALI: %f\n\n", so_recv_, avg_interval_);
	}
};

#endif
