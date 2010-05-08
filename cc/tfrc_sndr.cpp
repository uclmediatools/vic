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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include "assert.h"
#include "rtp.h"
#include "inet.h"
#include "pktbuf-rtp.h"
#include "vic_tcl.h"
#include "module.h"
#include "transmitter.h"
#include "tfrc_sndr.h"
#include "formula.h"

// TfrcSndr instance
TfrcSndr TfrcSndr::instance_;

TfrcSndr::TfrcSndr() :
	seqno_(0),
	x_rate_(0),
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
	tsvec_ = (double *) malloc(sizeof(double) * TSZ);
	clear_tsv(TSZ);
	// allocate seqvec in memory
	seqvec_ = (u_int32_t *) malloc(sizeof(u_int32_t) * SSZ);
	clear_sqv(SSZ);
	num_seqvec_ = 0;
	// allocate refvec in memory
	refvec_ = (u_int32_t *) malloc(sizeof(u_int32_t) * RSZ);
	clear_refv(RSZ);
	num_refvec_ = 0;

	// initialize variables
	ts_ = 0.0;
	num_missing_ = 0;

	// EWMA packet size
	asize_ = 0;
	pcnt_ = 0;
	psize_ = 1000;
	lambda1_ = .75;
	lambda2_ = .15;
}

/*
 * TFRC send
 */
void TfrcSndr::send(pktbuf* pb, double now) {
	// the very first data packet
	if(seqno_ == 0)
	ts_off_ = tx_ts_offset();

	// parse seqno and mark timestamp for this data packet
	rtphdr* rh = (rtphdr *) pb->data;
	seqno_	= ntohs(rh->rh_seqno);
	now_	= now;

	// alrithmetic average packet size (per frame)
	asize_ += pb->len;
	pcnt_++;

	// tag finished (end of frame)
	if (!(pb->tag)) {
		asize_ /= pcnt_;
		// EWMA'd packet size
		if (pcnt_ != 1)
		psize_ = lambda1_ * asize_ + (1 - lambda1_) * psize_;
		else
		psize_ = lambda2_ * asize_ + (1 - lambda2_) * psize_;

		asize_ = 0; pcnt_ = 0;
	}

	// timestamp vector for loss history update
	tsvec_[seqno_%TSZ]  = now_-SKEW;

	// sequence number must be greater than zero
	assert (seqno_ > 0);
	// number of total data packet sent
	ndtp_++;
}

/*
 * main TFRC reception path
 */
void TfrcSndr::recv(u_int16_t type, u_int16_t begin, u_int16_t end,
	u_int16_t *chunk, double so_rtime, bool recv_by_ch, pktbuf* pb) 
{
  UNUSED(recv_by_ch);
  UNUSED(pb);

  switch(type) {
  // XR block type 1
  case XR_BT_1:
  {
	// number of ack received
	nakp_++;
	// so_timestamp
	so_recv_ = so_rtime;

	// get start/end seqno that this XR chunk reports
	begins_ = begin;	// lowest packet seqno
	ends_ = end;		// highest packet seqno plus one

	// just acked seqno
	// i.e.,) head seqno(= highest seqno) of this ackvec
	jacked_ = ends_ - 1;

	// get the number of AckVec chunks
	//   use seqno space to work out the num chunks
	//   (add one to num unless exactly divisible by BITLEN
	//   - so it is large enough to accomodate all the bits)
	num_elm_ = get_numelm(begins_, jacked_);
	num_vec_ = get_numvec(num_elm_);

	// declared AckVec
	ackv_ = (u_int16_t *) malloc (sizeof(u_int16_t) * num_vec_);
	// clear the existing AckVec
	clear_ackv(num_vec_);
	// clone AckVec from Vic
	clone_ackv(chunk, num_vec_);

	// generate seqno vector
	gen_seqvec(ackv_, num_vec_);
	// generate reference vector
	// (it represents seqvec when no losses)
	// @begin: aoa_+1 (lowest seqno)
	// @end: jacked_
	gen_refvec(jacked_, aoa_+1);

	// TFRC congestioin control
	calc_rate();

	// set ackofack (real number)
	aoa_ = jacked_;

	// sampled RTT
	tao_ = so_recv_ - tsvec_[jacked_%TSZ];
	// update RTT with the sampled RTT
	update_rtt(tao_);
  }
  break;

  // XR block type 3
  case XR_BT_3:
  {}
  break;
  }
}

/*
 * generate seqno vector
 * (interpret the received AckVec to real sequence numbers)
 * @ackvec: receivec AckVec
 */
void TfrcSndr::gen_seqvec(u_int16_t *v, int n) {
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
}

/*
 * generate reference vector
 * (it represents seqno vector when no losses)
 * @end:	end seqno (highest)
 * @begin:	begin seqno (lowest)
 */
void TfrcSndr::gen_refvec(int end, int begin) {
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
 * calculate sending rate
 */
void TfrcSndr::calc_rate() {
}

/*
 * update RTT using sampled RTT value
 */
void TfrcSndr::update_rtt(double tao) {
}
