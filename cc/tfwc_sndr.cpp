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

TfwcSndr::TfwcSndr() :
	seqno_(0),
	cwnd_(1),
	aoa_(0),
	t_now_(0),
	t_ts_(0),
	t_ts_echo_(0),
	now_(0),
	ndtp_(0),
	nakp_(0),
	ntep_(0),
	nsve_(0),
	epoch_(1),
	jacked_(0),
	begins_(0),
	ends_(0)
{
	// allocate tsvec_ in memory
	tsvec_ = (double *)malloc(sizeof(double) * TSZ);
	// allocate seqvec in memory
	seqvec_ = (u_int32_t *)malloc(sizeof(u_int32_t) * SSZ);

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
	is_loss_ = false;
	num_loss_ = 0;

	avg_interval_ = 0.0;
	I_tot0_ = 0.0;
	I_tot1_ = 0.0;
	tot_weight_ = 0.0;
}

void TfwcSndr::tfwc_sndr_send(pktbuf* pb) {

	// get RTP hearder information
	rtphdr* rh =(rtphdr*) pb->data;

	// get seqno and mark timestamp for this data packet
	seqno_	= ntohs(rh->rh_seqno);
	now_	= tfwc_sndr_now();		// double type (reference time)
	t_now_	= tfwc_sndr_t_now();	// u_int32_t type (reference time)

	// timestamp vector for loss history update
	tsvec_[seqno_%TSZ - 1] = now_;

	// sequence number must be greater than zero
	assert (seqno_ > 0);
	//debug_msg("sent seqno:		%d\n", seqno_);

	ndtp_++;	// number of data packet sent
}

/*
 * main TFWC reception path
 */
void TfwcSndr::tfwc_sndr_recv(u_int16_t type, u_int16_t begin, u_int16_t end,
		u_int16_t *chunk, int num_chunks)
{
	// retrieve ackvec
	if (type == XR_BT_1) {
		nakp_++;		// number of ack packet received

		// get start/end seqno that this XR chunk reports
		begins_ = begin;
		ends_ = end;

		// just acked seqno (head seqno of this ackvec)
		jacked_ = ends_ - 1;

		// declared AckVec
		ackv_ = (u_int16_t *) malloc (sizeof(u_int16_t) * num_chunks);

		// clone AckVec from Vic application
		for (int i = 0; i < num_chunks; i++) {
			ackv_[i] = ntohs(chunk[i]);	
		}

		// XXX generate seqno vec
		printf("    [%s +%d] begins:%d, ends:%d, jacked:%d\n", 
				__FILE__, __LINE__, begins_, ends_, jacked_);
		gen_seqvec(num_chunks, ackv_);
		free(ackv_);

		// generate margin vector
		marginvec(jacked_);
		print_mvec();

		// detect loss
		// 	@begin: aoa_
		// 	@end: mvec_[DUPACKS] - 1
		is_loss_ = detect_loss(mvec_[DUPACKS-1]-1, aoa_);

		// TFWC is not turned on (i.e., no packet loss yet)
		if(!is_tfwc_on_) {
			if(is_loss_) {
				is_tfwc_on_ = true;
				dupack_action();
				ts_ = tsvec_[first_lost_pkt_%TSZ];
			} else {
				// TCP-like AIMD control
				cwnd_ += 1;
			}
		} 
		// TFWC is turned on, so control that way
		else {
			control();
		}

		// set ackofack (real number)
		aoa_ = ackofack(); 

		// update RTT with the sampled RTT
		tao_ = tfwc_sndr_now() - tsvec_[seqno_%TSZ];
		update_rtt(tao_);

		// initialize variables for the next pkt reception
		init_loss_var();
	}
	// retrieve ts echo
	else if (type == XR_BT_3) {
		ntep_++;		// number of ts echo packet received

		ts_echo_ = chunk[num_chunks - 1];
		printf("    [%s +%d] ts echo:	%f\n", __FILE__,__LINE__, ts_echo_);

		tao_ = 1e-6 * (double)(tfwc_sndr_now() - ts_echo_);

		// update RTT
		//update_rtt(tao_);
	}
}

/*
 * generate seqno vector 
 * (interpret the received AckVec to real sequence numbers)
 * @num_chunks: number of AckVec chunks
 * @ackvec: received AckVec
 */
void TfwcSndr::gen_seqvec (u_int16_t num_chunks, u_int16_t *ackvec) {
	// number of seqvec elements 
	// (i.e., number of packets in AckVec)
	int numElm = ends_ - begins_;
	int x = numElm%BITLEN;

	// start of seqvec
	int start = jacked_;

	int i, j, k = 0;
	for (i = 0; i < num_chunks-1; i++) {
		for (j = 0; j < BITLEN; j++) {
			if ( CHECK_BIT_AT(ackvec[i], (j+1)) )
				seqvec_[k%SSZ] = start;
		else num_loss_++;
			k++; start--;
		}
	}

	int a = (x == 0) ? BITLEN : x;
	for (i = 0; i < a; i++) {
		if ( CHECK_BIT_AT(ackvec[num_chunks-1], i+1 ))
			seqvec_[k++%SSZ] = start;
		else num_loss_++;
		start--;
	}

	// printing retrieved sequence numbers from received AckVec
	print_seqvec(numElm - num_loss_);
}

/*
 * detect packet loss in the received vector
 * @ret: true when there is a loss
 */
bool TfwcSndr::detect_loss(int end, int begin) {
	bool ret;	// 'true' when there is a loss
	bool gotIn = false;
	int count = 0; // packet loss counter

	// number of tempvec element when no loss
	int numelm = (end - begin < 0) ? 0 : end - begin;
	u_int32_t tempvec[numelm];

	// generate tempvec elements
	printf("\tcomparing numbers: (");
	for (int i = 0; i < numelm; i++) {
		tempvec[i] = (begin + 1) + i;
		printf(" %d", tempvec[i]);
	} printf(" )\n");

	// number of seqvec element
	int numseq = ends_ - begins_ - num_loss_;

	// compare tempvec and seqvec
	for (int i = 0; i < numelm; i++) {
		for (int j = numseq-1; j >= 0; j--) {
			if (tempvec[i] == seqvec_[j]) {
				gotIn = true;
				// we found it, so reset count
				count = 0; break;
			} else {
				gotIn = false; 
				count++;
			}
		} // packet loss should be captured by now

		// record the very first lost packet seqno
		if(!gotIn) {
			if(!is_first_loss_seen_) 
				first_lost_pkt_ = tempvec[i];
		}
	}
	
	// store tempvec elements for updating loss history
	first_elm_ = tempvec[0];
	last_elm_ = first_elm_ + (numelm - 1);

	return ret = (count > 0) ? true : false;
}

/*
 * update RTT using the sampled RTT value
 */
void TfwcSndr::update_rtt(double rtt_sample) {

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
	if (k_ * rttvar_ > g_) 
		rto_ = srtt_ + k_ * rttvar_;
	else
		rto_ = srtt_ + g_;

	// 'rto' could be rounded by 'maxrto'
	if (rto_ > maxrto_)
		rto_ = maxrto_;
}

/*
 * core part for congestion window control
 */
void TfwcSndr::control() {
	loss_history();
	avg_loss_interval();

	// loss event rate (p)
	p_ = 1.0 / avg_interval_;

	// simplified TCP throughput equation
	double tmp1 = 12.0 * sqrt(p_ * 3.0/8.0);
	double tmp2 = p_ * (1.0 + 32.0 * pow(p_, 2.0));
	double term1 = sqrt(p_ * 2.0/3.0);
	double term2 = tmp1 * tmp2;
	f_p_ = term1 + term2;

	// TFWC congestion window
	t_win_ = 1 / f_p_;

	cwnd_ = (int) (t_win_ + .5);

	// cwnd should always be greater than 1
	if (cwnd_ < 1)
		cwnd_ = 1;
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
void TfwcSndr::pseudo_history() {
	pseudo_interval_ = 1 / p_;

	/* bzero for all history information */
	for(int i = 0; i <= HSZ+1; i++)
		history_[i] = 0;

	/* (let) most recent history information be 0 */
	history_[0] = 0;

	/* (let) the pseudo interval be the first history information */
	history_[1] = (int) pseudo_interval_;
}

/*
 * dupack action
 *   o  halve cwnd_
 *   o  marking pseudo loss history and loss rate
 */
void TfwcSndr::dupack_action() {
	// this is the very first packet loss
	is_first_loss_seen_ = true;

	// we now have just one meaningful history information
	hsz_ = 1;

	// halve the current cwnd_
	cwnd_ = cwnd_ / 2;

	// congestion window never goes below 1
	if (cwnd_ < 1)
		cwnd_ = 1;

	// temp cwnd to compute the pseudo values
	tmp_cwnd_ = cwnd_;

	// creating simulated loss history and loss rate
	pseudo_p();
	pseudo_history();

	// generate weight factors
	gen_weight();
}

/*
 * compute simulated loss rate
 */
void TfwcSndr::pseudo_p() {
	for (pseudo_p_ = 0.00001; pseudo_p_ < 1.0; pseudo_p_ += 0.00001) {
		f_p_ = sqrt((2.0/3.0) * pseudo_p_) + 12.0 * pseudo_p_ *
			(1.0 + 32.0 * pow(pseudo_p_, 2.0)) * sqrt((3.0/8.0) * pseudo_p_);

		t_win_ = 1 / f_p_;

		if(t_win_ < tmp_cwnd_)
			break;
	}
	p_ = pseudo_p_;
}

/*
 * compute simulated loss history
 */
void TfwcSndr::loss_history() {
	bool is_loss;		// is there a loss found in seqvec?
	bool is_new_event;	// is this a new loss event?
	int numvec = last_elm_ - first_elm_ + 1;
	u_int32_t tempvec[numvec];

	for (int i = 0; i < numvec; i++)
		tempvec[i] = first_elm_ + i;

	// compare tempvec[] with seqvec
	for (int i = 0; i < numvec; i++) {
		// is there a loss found?
		for (int j = 0; j < numvec; j++) {
			if (tempvec[i] == seqvec_[j]) {
				is_loss = false;
				break;
			} else 
				is_loss = true;
		}

		// is this a new loss event?
		if (tsvec_[tempvec[i]%TSZ] - ts_ > srtt_)
			is_new_event = true;
		else
			is_new_event = false;

		// compute loss history (compare tempvec and seqvec)
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

			// increase current history size
			hsz_ = (hsz_ < HSZ) ? ++hsz_ : HSZ;

			// shift history information
			for (int k = HSZ; k > 0; k--)
				history_[k] = history_[k-1];

			// record lost packet's timestamp
			ts_ = tsvec_[tempvec[i]%TSZ];

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

	// make a decision whether to include the most recent loss interval
	for (int i = 0; i < hsz_; i++) {
		I_tot0_ += weight_[i] * history_[i];
		tot_weight_ += weight_[i];
	}
	for (int i = 1; i < hsz_ + 1; i++) {
		I_tot1_ += weight_[i-1] * history_[i];
		tot_weight_ += weight_[i];
	}

	// compare I_tot0_ and I_tot1_ and use larger value
	if (I_tot0_ < I_tot1_)
		I_tot_ = I_tot1_;
	else
		I_tot_ = I_tot0_;

	// this is average loss interval
	avg_interval_ = I_tot_ / tot_weight_;
}
