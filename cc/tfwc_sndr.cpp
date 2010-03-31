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
#include "math.h"
#include "rtp.h"
#include "inet.h"
#include "pktbuf-rtp.h"
#include "vic_tcl.h"
#include "module.h"
#include "transmitter.h"
#include "tfwc_sndr.h"

// timestamp skew from Vic to Network Device 
// (approximately 10 usec)
#define SKEW 0.000010

/*
 * retransmission timer
 */
void TfwcRtxTimer::timeout() {
	debug_msg("\t*------ TIMEOUT! ------*\n");
	s_ -> expire(TFWC_TIMER_RTX);
}

/* 
 * TFWC sender definition
 */
TfwcSndr::TfwcSndr() :
	seqno_(0),
	cwnd_(1),
	rtx_timer_(this),
	aoa_(0),
	now_(0),
	so_recv_(0),
	ndtp_(0),
	nakp_(0),
	ntep_(0),
	nsve_(0),
	jacked_(0),
	begins_(0),
	ends_(0),
	num_elm_(1),
	num_vec_(1)
{
	// allocate tsvec_ in memory
	tsvec_ = (double *)malloc(sizeof(double) * TSZ);
	clear_tsv(TSZ);

	// allocate seqvec in memory
	seqvec_ = (u_int32_t *)malloc(sizeof(u_int32_t) * SSZ);
	clear_sqv(SSZ);
	num_seqvec_ = 0;

	// allocate refvec in memory
	refvec_ = (u_int32_t *)malloc(sizeof(u_int32_t) * RSZ);
	clear_refv(RSZ);
	num_refvec_ = 0;

	// for simulating TCP's 3 dupack rule
	// (allowing packet re-ordering issue)
	for (int i = 0; i < DUPACKS; i++)
		mvec_[i] = 0;

	minrto_ = 0.0;
	maxrto_ = 100000.0;
	srtt_ = -1.0;
	rto_ = 3.0;		// RFC 1122
	rttvar_ = 0.0;
	df_ = 0.95;
	sqrtrtt_ = 1.0;
	t0_ = 6.0;
	alpha_ = 0.125;
	beta_ = 0.25;
	g_ = 0.01;
	k_ = 4;
	ts_ = 0.0;
	ts_echo_ = 0.0;

	is_tfwc_on_ = false;
	is_first_loss_seen_ = false;
	first_lost_pkt_ = -1;
	num_missing_ = 0;

	avg_interval_ = 0.0;
	I_tot0_ = 0.0;
	I_tot1_ = 0.0;
	tot_weight_ = 0.0;

	to_driven_ = false;
	tcp_tick_ = 0.01;
	srtt_init_ = 12;
	rttvar_exp_ = 2;
	t_srtt_ = int(srtt_init_/tcp_tick_) << T_SRTT_BITS;
	t_rttvar_ = int(rttvar_init_/tcp_tick_) << T_RTTVAR_BITS;

	// allocate previously received ackvec in memory
	pvec_ = (u_int16_t *)malloc(sizeof(u_int16_t) * num_vec_);
	clear_pvec(num_vec_);
	__jacked_ = 0;
	// previous average loss intervals
	prev_interval_ = (double *)malloc(sizeof(double) * RSZ);
	clear_prev_interval(RSZ);
	new_hist_seqno_ = (u_int16_t *)malloc(sizeof(u_int16_t) * RSZ);
	clear_new_hist_seqno(RSZ);
	new_hist_seqno_size_ = 0;

	// packet reordering
	reorder_ = false;

	// record of packet size in bytes
	record_ = (u_int16_t *)malloc(sizeof(u_int16_t) * RECORD);
	clear_record(RECORD);
}

void TfwcSndr::tfwc_sndr_send(pktbuf* pb, double now) {
	// the very first data packet
	if(seqno_ == 0)
	ts_off_ = tx_ts_offset();

	// number of bytes for this packet
	record_[seqno_%RECORD] = pb->len;
	// parse seqno and mark timestamp for this data packet
	rtphdr* rh = (rtphdr *) pb->data;
	seqno_	= ntohs(rh->rh_seqno);
	now_	= now;

	// timestamp vector for loss history update
	tsvec_[seqno_%TSZ] = now_-SKEW;
	//print_packet_tsvec();

	// sequence number must be greater than zero
	assert (seqno_ > 0);
	// number of total data packet sent
	ndtp_++;
	
	// set retransmission timer
	set_rtx_timer();
}

/*
 * main TFWC reception path
 */
void TfwcSndr::tfwc_sndr_recv(u_int16_t type, u_int16_t begin, u_int16_t end,
		u_int16_t *chunk, double so_rtime, bool recv_by_ch, pktbuf* pb)
{
  switch (type) {
  // retrieve ackvec
  case XR_BT_1: 
  {
	// number of ack received
	nakp_++;
	// so_timestamp (timestamp for packet reception)
	so_recv_ = so_rtime;
	// packet reordering?
	reorder_ = false;
	// reordered ack delivery?
	bool outofack = false; 
	UNUSED(outofack);
	// revert to the previous history?
	bool revert = false;

	// get start/end seqno that this XR chunk reports
	begins_ = begin;	// lowest packet seqno
	ends_ = end;		// highest packet seqno plus one

	// just acked seqno 
	// i.e.,) head seqno(= highest seqno) of this ackvec
	jacked_ = ends_ - 1;

	//print_xr_info(__FILE__,__LINE__);

	// get the number of AckVec chunks
	//   use seqno space to work out the num chunks
	//   (add one to num unless exactly divisible by BITLEN
	//   - so it is large enough to accomodate all the bits
	num_elm_ = get_numelm(begins_, jacked_);
	num_vec_ = get_numvec(num_elm_);

	// declared AckVec
	ackv_ = (u_int16_t *) malloc (sizeof(u_int16_t) * num_vec_);
	// clear the existing AckVec
	clear_ackv(num_vec_);
	// clone AckVec from Vic
	clone_ackv(chunk, num_vec_);
	//print_vec("ackvec", ackv_, num_vec_);

	//---------------------------------------------------------*
	// detect packet reordering and reordered ack delivery
	int shift = abs(__jacked_ - jacked_);
	if (jacked_ < __jacked_) {
		//
		// this ack is deprecated message (e.g., too old).
		//
		if(jacked_ < aoa_) {
		  debug_msg("warning: this ack(%d) is older than AoA(%d)!\n", jacked_,aoa_);
		  // trigger a packet out to keep Jacob's packet conservation rule
		  packet_clocking(pb, recv_by_ch);
		  return;
		}
		//
		// this ack is delivered out-of-order
		//
		else if(out_of_ack(jacked_, seqvec_, num_seqvec_)) {
		  debug_msg("warning: this ack(%d) itself is out-of-order!\n",jacked_);
		  // if the disorder is beyond 3 dupack rule,
		  // revert to the earlier history
		  if(shift >= DUPACKS)
		  revert = revert_interval(jacked_);
		  // then, update cwnd
		  cwnd_in_packets(revert);
		  print_cwnd();
		  return;
		}
		//
		// packet is out-of-order, so adjust ackvec re-construction
		//
		else {
		  debug_msg("warning: packet reordering occurred!\n");
		  // replace just ack'ed seqno
		  replace(__jacked_);
		  // re-calculate numelm and numvec
		  num_elm_ = get_numelm(begins_, jacked_);
		  num_vec_ = get_numvec(num_elm_);
		  reorder_ = true;
		}
	}
	else {
		free(pvec_);
	}
	//---------------------------------------------------------*

	// if packet reordering occurred, insert re-ordered seqno
	// into the received ackvec using previously received ackvec
	if(reorder_) {
		for (int i = 0; i < num_vec_; i++) {
		ackv_[i] = (ackv_[i] << shift) | pvec_[i];
		}
	}
	// this will ensure constructing ackv correctly,
	// when there is packet re-ordering.
	// (e.g., if packet reordering, then ack will be ordered that way.)
	else {
		for (int i = 0; i < num_vec_; i++) {
		ackv_[i] = (pvec_[i] << shift) | ackv_[i];
		}
	}

	// generate seqno vector
	//print_vec("ackvec", ackv_, num_vec_);
	gen_seqvec(ackv_, num_vec_);

	// generate margin vector
	marginvec(jacked_);
	print_mvec();

	// generate reference vector
	// (it represents seqvec when there are no losses)
	// 	@begin: aoa_+1 (lowest seqno)
	// 	@end: mvec_[DUPACKS-1] - 1
	gen_refvec(mvec_[DUPACKS-1]-1, aoa_+1);

	// we have detected packet re-ordering!!!
	// so, we've updated/corrected received ackvec
	// by inserting "jacked" to the previous ackvec.
	// finally, we only need to clock packets out.
	// (i.e., do NOT update cwnd and RTT)
	if(reorder_) {
		// revert to the earlier history if the disorder is beyond 3 dupack rule
		if (shift >= DUPACKS)
		revert = revert_interval(jacked_);
		// then, update cwnd
		cwnd_in_packets(revert);
		print_cwnd();
		reset_var();
		return;
	}

	// TFWC is not turned on (i.e., no packet loss yet)
	if(!is_tfwc_on_) {
		if(detect_loss())
		dupack_action(first_lost_pkt_); 
		else
		cwnd_++; // TCP-like AIMD
	} 
	// TFWC is turned on, so compute congestion window
	else {
		cwnd_in_packets(revert);
	}
	print_cwnd();

	// set ackofack (real number)
	aoa_ = ackofack(); 

	// sampled RTT
	tao_ = so_recv_ - tsvec_[jacked_%TSZ];
	// update RTT with the sampled RTT
	update_rtt(tao_);

	// is TFWC being driven by timeout mechanism?
	if(to_driven_ && is_tfwc_on_)
		new_rto(tao_);
	
	// reset variables for the next pkt reception
	reset_var();
  }
  break;

  // retrieve ts echo
  case XR_BT_3:
  {
	ntep_++;		// number of ts echo packet received
	ts_echo_ = chunk[num_vec_ - 1];
	//fprintf(stderr, "    [%s +%d] ts echo:	%f\n", 
	//	__FILE__,__LINE__, ts_echo_);

	tao_ = now() - ts_echo_;
  }
  break;

  default:
  break;
  } // end switch (type)

  return;
}

/*
 * detect out-of-ordered ack delivery
 * -- if just ack'ed seqno (jack) is already in sequence vector,
 *    then, this ack should've been arrived earlier.
 *    (i.e., this ack is out-of-ordered)
 */
bool TfwcSndr::out_of_ack(u_int16_t target, u_int32_t *sqv, int n) {
	for (int i = 0; i < n; i++) {
		if (sqv[i] == target)
		return true;
	}
	return false;
}

void TfwcSndr::reset_var() {
	// init vars------------*
	num_missing_ = 0;
	//----------------------*

	// store jack'ed
	store(jacked_);
	// declare pvec to store ackv
	pvec_ = (u_int16_t *)malloc(sizeof(u_int16_t) * num_vec_);
	clear_pvec(num_vec_);
	// store ackv
	copy_ackv(num_vec_);
	//print_vec("stored ackvec", pvec_, num_vec_);

	// finally, free ackvec
	free(ackv_);
}

/*
 * generate seqno vector 
 * (interpret the received AckVec to real sequence numbers)
 * @ackvec: received AckVec
 */
void TfwcSndr::gen_seqvec (u_int16_t *v, int n) {
	// clear seqvec before starts
	clear_sqv(num_seqvec_);

	int i, j, k = 0;
	int x = num_elm_%BITLEN;

	// start of seqvec (lowest seqno)
	int start = begins_;

	for (i = 0; i < n-1; i++) {
		for (j = BITLEN; j > 0; j--) {
			if( CHECK_BIT_AT(v[i], j) )
				seqvec_[k++%SSZ] = start;
			else num_missing_++;
			start++;
		}
	}

	int a = (x == 0) ? BITLEN : x;
	for (i = a; i > 0; i--) {
		if( CHECK_BIT_AT(v[n-1], i) )
			seqvec_[k++%SSZ] = start;
		else num_missing_++;
		start++;
	}

	// therefore, the number of seqvec elements is:
	num_seqvec_ = num_elm_ - num_missing_;
	// printing retrieved sequence numbers from received AckVec
	print_vec("sequence numbers", seqvec_, num_seqvec_);
}

/*
 * generate reference vector
 * (it represents the seqno vector when no losses)
 * @end:    end seqno	(highest)
 * @begin:  begin seqno (lowest)
 */
void TfwcSndr::gen_refvec(int end, int begin) {
	// clear previous reference vector
	clear_refv(num_refvec_);
	// number of reference element - when no loss
	num_refvec_ = end - begin + 1;

	// generate refvec elements
	fprintf(stderr, "\tcomparing numbers: (");
	for (int i = 0; i < num_refvec_; i++) {
		refvec_[i] = begin + i;
		fprintf(stderr, " %d", refvec_[i]);
	} fprintf(stderr, " )\n");
}

/*
 * detect packet loss in the received vector
 * @ret: true when there is a loss
 */
bool TfwcSndr::detect_loss() {
	bool ret;	// 'true' when there is a loss
	bool is_there = false;
	int count = 0; // packet loss counter

	// compare refvec and seqvec
	for (int i = 0; i < num_refvec_; i++) {
		for (int j = num_seqvec_-1; j >= 0; j--) {
			if (refvec_[i] == seqvec_[j]) {
				is_there = true;
				// we found it, so reset count
				count = 0; break;
			} else {
				is_there = false; 
				count++;
			}
		} // packet loss should be captured by now

		// record the very first lost packet seqno
		if(!is_there) {
			if(!is_first_loss_seen_) 
			first_lost_pkt_ = refvec_[i];
		}
	}
	return ret = (count > 0) ? true : false;
}

/*
 * update RTT using the sampled RTT value
 */
void TfwcSndr::update_rtt(double rtt_sample) {
	// calculate t0_ 
	t_rtt_ = int(rtt_sample/tcp_tick_ + .5);
	if(t_rtt_ == 0) t_rtt_ = 1;

	if(t_srtt_ != 0) {
		register short rtt_delta;
		rtt_delta = t_rtt_ - (t_srtt_ >> T_SRTT_BITS);

		if ((t_srtt_ += rtt_delta) <= 0)
			t_srtt_ = 1;

		if (rtt_delta < 0)
			rtt_delta = -rtt_delta;

		rtt_delta -= (t_rttvar_ >> T_RTTVAR_BITS);
		if((t_rttvar_ += rtt_delta) <= 0)
			t_rttvar_ = 1;
	}
	else {
		t_srtt_ = t_rtt_ << T_SRTT_BITS;
		t_rttvar_ = t_rtt_ << (T_RTTVAR_BITS-1);
	}

	// finally, t0_ = (smoothed RTT) + 4 * (rtt variance)
	t0_ = (((t_rttvar_ << (rttvar_exp_ + (T_SRTT_BITS - T_RTTVAR_BITS)))
			+ t_srtt_)  >> T_SRTT_BITS ) * tcp_tick_;
	
	if (t0_ < minrto_)
		t0_ = minrto_;

	// calculate smoothed RTT
	if (srtt_ < 0) {
		// the first RTT observation
		srtt_ = rtt_sample;
		rttvar_ = rtt_sample/2.0;
		sqrtrtt_ = sqrt(rtt_sample);
	} else {
		srtt_ = df_ * srtt_ + (1 - df_) * rtt_sample;
		rttvar_ = rttvar_ + beta_ * (fabs(srtt_ - rtt_sample) - rttvar_);
		sqrtrtt_ = df_ * sqrtrtt_ + (1 - df_) * sqrt(rtt_sample);
	}

	// update the current RTO
	if(!to_driven_) {
		if (k_ * rttvar_ > g_) 
		rto_ = srtt_ + k_ * rttvar_;
		else
		rto_ = srtt_ + g_;
	}

	// 'rto' could be rounded by 'maxrto'
	if (rto_ > maxrto_)
		rto_ = maxrto_;

	print_rtt_info();
}

/*
 * core part for congestion window control
 * (cwnd is in packets)
 */
void TfwcSndr::cwnd_in_packets(bool revert) {
	if(!revert) {
	loss_history();
	avg_loss_interval();
	}

	// loss event rate (p)
	p_ = 1.0 / avg_interval_;

	// simplified TCP throughput equation
	double tmp1 = 12.0 * sqrt(p_ * 3.0/8.0);
	double tmp2 = p_ * (1.0 + 32.0 * pow(p_, 2.0));
	double term1 = sqrt(p_ * 2.0/3.0);
	double term2 = tmp1 * tmp2;
	f_p_ = term1 + term2;

	// TFWC congestion window
	t_win_ = 1. / f_p_;
	cwnd_ = (int) (t_win_ + .5);

	// timeout driven when cwnd is less than 2
	if (t_win_ < 2.)
		to_driven_ = true;
	else
		to_driven_ = false;

	// cwnd should always be greater than 1
	if (cwnd_ < 1)
		cwnd_ = 1;
}

/*
 * core part for congestion window control
 * (cwnd is in bytes)
 */
void cwnd_in_bytes() {
}

/*
 * generate weighting factors
 */
void TfwcSndr::gen_weight() {
#ifdef SHORT_HISTORY
	// this is just weighted moving average (WMA)
	for(int i = 0; i <= HSZ; i++){
		if(i < HSZ/2)
			weight_[i] = 1.0;
		else
			weight_[i] = 1.0 - (i-(HSZ/2 - 1.0)) / (HSZ/2 + 1.0);
	}
#else
	// this is exponentially weighted moving average (EWMA)
	for (int i=0; i <= HSZ; i++) {
		if (i < HSZ/4)
			weight_[i] = 1.0;
		else
			weight_[i] = 2.0 / (i - 1.0);
	}
#endif
}

/*
 * compute packet loss history
 */
void TfwcSndr::pseudo_history(double p) {
	double pseudo = 1 / p;

	/* bzero for all history information */
	for(int i = 0; i <= HSZ+1; i++)
		history_[i] = 0;

	/* (let) most recent history information be 0 */
	history_[0] = 0;

	/* (let) the pseudo interval be the first history information */
	history_[1] = pseudo;
}

/*
 * dupack action
 *   o  halve cwnd_
 *   o  marking pseudo loss history and loss rate
 */
void TfwcSndr::dupack_action(int seqno) {
	// this is the very first packet loss
	is_first_loss_seen_ = true;

	// we now have just one meaningful history information
	hsz_ = 1;

	// halve the current cwnd_
	cwnd_ = cwnd_ / 2;

	// congestion window never goes below 1
	if (cwnd_ < 1)
		cwnd_ = 1;

	// creating simulated loss history and loss rate
	p_ = pseudo_p(cwnd_);
	pseudo_history(p_);

	// generate weight factors
	gen_weight();

	// finally, record the very first lost packet's timestamp
	ts_ = tsvec_[seqno%TSZ];
	// then, turn on TFWC algo
	is_tfwc_on_ = true;
}

/*
 * compute simulated loss rate
 */
double TfwcSndr::pseudo_p(int cwnd) {
	double pseudo;
	for (pseudo = 0.00001; pseudo < 1.0; pseudo += 0.00001) {
	f_p_ = sqrt((2.0/3.0) * pseudo) + 12.0 * pseudo *
	(1.0 + 32.0 * pow(pseudo, 2.0)) * sqrt((3.0/8.0) * pseudo);

	t_win_ = 1 / f_p_;

	if(t_win_ < cwnd)
		break;
	}

	return (pseudo);
}

/*
 * compute simulated loss history
 */
void TfwcSndr::loss_history() {
	bool is_loss = false;		// is there a loss found in seqvec?
	bool is_new_event = false;	// is this a new loss event?

	// compare reference with seqvec
	for (int i = 0; i < num_refvec_; i++) {
		// is there a loss found?? and, is this a new loss event??
		if (!find_seqno(seqvec_, num_seqvec_, refvec_[i])) {
			is_loss = true;
			if (tsvec_[refvec_[i]%TSZ] - ts_ > srtt_)
			is_new_event = true;
			else
			is_new_event = false;
		}

		// compute loss history (compare refvec and seqvec)
		// o  everytime it sees a loss
		//    it will compare the timestamp with smoothed RTT
		//
		// o  if the time difference is greater than RTT,
		//    then this loss starts a new loss event
		// o  if the time difference is less than RTT,
		//    then we do nothing
		//
		// o  if there is no loss, 
		//    simply increment the loss interval by one
		if (is_loss && is_new_event) {
			// this is a new loss event!

			// store previous ALI before changing history
			record_history(refvec_[i], avg_interval_, ts_);

			// increase current history size
			hsz_ = (hsz_ < HSZ) ? ++hsz_ : HSZ;

			// shift history information
			for (int k = HSZ; k > 0; k--)
				history_[k] = history_[k-1];

			// record lost packet's timestamp
			ts_ = tsvec_[refvec_[i]%TSZ];

			// let the most recent history information be one
			history_[0] = 1;
		} 
		else {
			// this is not a new loss event
			// increase the current history information
			history_[0]++;
		}
	}
}

/*
 * average loss interval
 */
void TfwcSndr::avg_loss_interval() {

	I_tot0_ = 0;
	I_tot1_ = 0;
	tot_weight_ = 0;
	int i = 0, j = 0;

	// make a decision whether to include the most recent loss interval
	//fprintf(stderr, "\n\tHIST_0 [");
	for (i = 0; i < hsz_; i++) {
		I_tot0_ += weight_[i] * history_[i];
		tot_weight_ += weight_[i];
	//	print_history_item(i);
	}
	//fprintf(stderr, "]\n");
	//fprintf(stderr, "\tHIST_1 [");
	for (i = 1, j = 0; i < hsz_ + 1; i++, j++) {
		I_tot1_ += weight_[i-1] * history_[i];
	//	print_history_item(i, j);
	}
	//fprintf(stderr, "]\n");

	// compare I_tot0_ and I_tot1_ and use larger value
	if (I_tot0_ < I_tot1_)
		I_tot_ = I_tot1_;
	else
		I_tot_ = I_tot0_;

	// this is average loss interval
	avg_interval_ = I_tot_ / tot_weight_;
	print_ALI();
}

/*
 * store loss interval and timestamp
 */
void TfwcSndr::record_history(int seqno, double interval, double ts) {
	// store seqno
	new_hist_seqno_[new_hist_seqno_size_++] = seqno;
	// store average loss interval
	prev_interval_[seqno%RSZ] = interval;
	// store timestamp
	prev_ts_ = ts;
	// copy history
	for(int i = 0; i < hsz_; i++)
	prev_history_[i] = history_[i];
}

/*
 * revert average loss interval on packet re-ordering
 */
bool TfwcSndr::revert_interval(int reseq) {
	// we didn't see the first lost packet yet.
	if(!is_first_loss_seen_) {
		dupack_action(reseq);
		return (false);
	}

	// check if this re-ordered seqno actually triggered a new loss event
	// if yes, revert to the previous state
	if(find_seqno(new_hist_seqno_, new_hist_seqno_size_, reseq)) {
		// reverting to the previous ALI
		avg_interval_ = prev_interval_[reseq%RSZ];
		// reverting to the previous timestamp
		ts_ = prev_ts_;
		// reverting to the previous history
		for (int i = 0; i < hsz_; i++)
		history_[i] = prev_history_[i];

		print_ALI();

		// finally, clear up the state variables
		clear_prev_interval(RSZ);
		clear_new_hist_seqno(new_hist_seqno_size_);
		return (true);
	}
	return (false);
}

/*
 * find seqno in the array
 */
bool TfwcSndr::find_seqno (u_int16_t *v, int n, int target) {
	for (int i = 0; i < n; i++) {
		if (v[i] == target)
		return true;
	}
	return false;
}
bool TfwcSndr::find_seqno(u_int32_t *v, int n, u_int32_t target) {
	for (int i = 0; i < n; i++) {
		if(v[i] == target)
		return true;
	}
	return false;
}

/*
 * print history item
 */
void TfwcSndr::print_history_item (int i) {
	fprintf(stderr, "%d", history_[i]);
	if (i < hsz_ - 1) fprintf(stderr, ", ");
}

void TfwcSndr::print_history_item (int i, int j) {
	fprintf(stderr, "%d", history_[i]);
	if (j < hsz_ - 1) fprintf(stderr, ", ");
}

/*
 * retransmission timer-out
 */
void TfwcSndr::expire(int option) {
	if (option == TFWC_TIMER_RTX) {
		if(!to_driven_)
		reset_rtx_timer(1);
		else
		reset_rtx_timer(0);

		// artificially inflate the latest ack
		if(!to_driven_)
			jacked_++;

		// trigger packet sending
		cc_tfwc_output();
	}
}

/*
 * reset Rtx Timer
 */
void TfwcSndr::reset_rtx_timer (int backoff) {
	if(backoff)
		backoff_timer();

	set_rtx_timer();
}

/*
 * backoff Rtx Timer
 */
void TfwcSndr::backoff_timer() {
	if (srtt_ < 0) srtt_ = 1.0;
	rto_ = 2.0 * srtt_;

	if (rto_ > maxrto_)
		rto_ = maxrto_;
}

/*
 * set Rtx Timer
 */
void TfwcSndr::set_rtx_timer() {
	// resched() is basically msched(miliseconds)
	rtx_timer_.resched(rto_ * 1000.);
}

/*
 * new RTO calculation
 */
void TfwcSndr::new_rto(double rtt) {
	double tmp1 = 3. * sqrt(p_ * 3./8.);
	double tmp2 = t0_ * p_ * (1. + 32. * pow(p_, 2.));

	if(tmp1 > 1.)
		tmp1 = 1.;

	double term1 = rtt * sqrt(p_ * 2./3.);
	double term2 = tmp1 * tmp2;

	rto_ = (term1 + term2) * sqrt(rtt)/sqrtrtt_;
}

/*
 * clokcing packet out on re-ordering detection
 */
void TfwcSndr::packet_clocking (pktbuf* pb, bool flag) {
	if (flag)
		cc_tfwc_trigger();
	else
		cc_tfwc_trigger(pb);
}
