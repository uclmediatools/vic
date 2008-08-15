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
	ts_(0),
	ts_echo_(0),
	now_(0),
	lastest_ack_(0),
	ndtp_(0),
	nakp_(0),
	ntep_(0),
	nsve_(0),
	epoch_(1)
{
	// allocate tsvec_ in memory
	tsvec_ = (double *)malloc(sizeof(double)* TSZ);
	// allocate seqvec in memory
	seqvec_ = (u_int32_t *)malloc(sizeof(u_int32_t)* SSZ );

	// for simulating TCP's 3 dupack rule
	u_int32_t mvec_ = 0x00;
	UNUSED(mvec_);	// to shut up gcc-4.x

	minrto_ = 0.0;
	maxrto_ = 100000.0;
	srtt_ = -1.0;
	rto_ = 3.0;		// RFC 1122
	rttvar_ = 0.0;
	df_ = 0.95;
	sqrtrtt_ = 1.0;
	alpha_ = 0.125;
	beta_ = 0.25;
	g_ = 0.01;
	k_ = 4;
}

void TfwcSndr::tfwc_sndr_send(pktbuf* pb) {

	// get RTP hearder information
	rtphdr* rh =(rtphdr*) pb->data;

	// get seqno and mark timestamp for this data packet
	seqno_ = ntohs(rh->rh_seqno);
	now_ = tfwc_sndr_now();		// double type (reference time)
	t_now_ = tfwc_sndr_t_now();	// u_int32_t type (reference time)

	// timestamp vector for loss history update
	tsvec_[seqno_%TSZ - 1] = now_;

	// sequence number must be greater than zero
	assert (seqno_ > 0);
	debug_msg("sent seqno:		%d\n", seqno_);

	ndtp_++;	// number of data packet sent
}

void TfwcSndr::tfwc_sndr_recv(u_int16_t type, u_int32_t ackv, u_int32_t ts_echo)
{
	// retrieve ackvec
	if (type == XR_BT_1) {
		UNUSED(ts_echo);
		nakp_++;		// number of ackvec packet received
		//ackv_ = ackv;	// store ackvec

		// lastest ack (head of ackvec)
		lastest_ack_ = get_head_pos(ackv) + aoa_;

		// generate seqno vec
		gen_seqno_vec(ackv);

		// generate margin vector
		marginvec(ackv);

		// detect loss
		is_loss_ = detect_loss(seqvec_, mvec_[DUPACKS-1] - 1, aoa_);

		// congestion window control 
		control(seqvec_);

		// set ackofack (real number)
		aoa_ = ackofack();

		// update RTT with the sampled RTT
		tao_ = tfwc_sndr_now() - tsvec_[seqno_%TSZ];
		update_rtt(tao_);
	}
	// retrieve ts echo
	else if (type == XR_BT_3) {
		ntep_++;		// number of ts echo packet received
		/*
		   ts_echo_ = ts_echo;
		   debug_msg(" ts echo:	%d\n", ts_echo_);

		   tao_ = 1e-6 * (double)(tfwc_sndr_now() - ts_echo_);

		// update RTT
		update_rtt(tao_);
		*/
	}
}

bool TfwcSndr::detect_loss(u_int32_t* vec, u_int16_t end, u_int16_t begin) {
	bool ret;	// 'true' when there is a loss
	int lc = 0;	// counter

	// number of tempvec element
	int numvec = (end - begin < 0) ? 0 : end - begin;
	int tempvec[numvec];
	bool is_there;

	for (int i = 0; i < numvec; i++) {
		tempvec[i] = (begin + 1) + i;
		// is there stuff
	}
	return ret = (lc > 0) ? true : false;
}

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

void TfwcSndr::control(u_int32_t* seqvec) {
	loss_history(seqvec);

}

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

void TfwcSndr::loss_history(u_int32_t* seqvec) {
	pseudo_interval_ = 1 / p_;

	/* bzero for all history information */
	for(int i = 0; i <= HSZ+1; i++)
		history_[i] = 0;

	/* (let) most recent history information be 0 */
	history_[0] = 0;

	/* (let) the pseudo interval be the first history information */
	history_[1] = (int) pseudo_interval_;
}

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
