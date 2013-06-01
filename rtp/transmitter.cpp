/*-
 * Copyright (c) 1993-1994 The Regents of the University of California.
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
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
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

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>
#ifdef WIN32
//#include <winsock.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <sys/param.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/stat.h>
#endif
#include "ntp-time.h"
#include "pktbuf-rtp.h"
#include "transmitter.h"
#include "net.h"
#include "source.h"
#include "decoder.h"
#include "vic_tcl.h"

#if defined(sun) && !defined(__svr4__) || (defined(_AIX) && !defined(_AIX41))
extern "C" writev(int, iovec*, int);
#endif

#define NO_XR_RECV 0
#define XR_RECV 1

//Transmitter::pktbuf* Transmitter::freehdrs_;
//Transmitter::buffer* Transmitter::freebufs_;
int Transmitter::nbufs_;
int Transmitter::nhdrs_;

/*
 * Sequence number is static so when we change the encoding (which causes
 * new encoder to be allocated) we don't reset the sequence counter.
 * Otherwise, receivers will get confused, reset their stats, and generate
 * odd looking streams of reception reports (i.e., the packet counts will
 * drop back to 0).
 */
u_int16_t Transmitter::seqno_ = 1;

extern int cc_type__;

Transmitter::Transmitter() :
	mtu_(1024),
	nf_(0),
	nb_(0),
	np_(0),
	kbps_(128),
	nextpkttime_(0.),
	busy_(0),
	head_(0),
	tail_(0),
	loop_layer_(1000),
	loopback_(0),
	is_cc_active_(1),
	is_buf_empty_(1),
	cc_type_(cc_type__),
	cwnd_mode_(PKM)
{
	memset((char*)&mh_, 0, sizeof(mh_));
	mh_.msg_iovlen = 2;

	// CC related...
	switch(cc_type_) {
	  case WBCC:
		tfwc_sndr_ = TfwcSndr::instance();
		tfwc_rcvr_ = TfwcRcvr::instance();

		tfwc_sndr_.manager(this);
		tfwc_rcvr_.manager(this);
	        debug_msg("CC type: WBCC\n");

	  break;
	  case RBCC:
		tfrc_sndr_ = TfrcSndr::instance();
		tfrc_rcvr_ = TfrcRcvr::instance();

		tfrc_sndr_.manager(this);
		tfrc_rcvr_.manager(this);
	        debug_msg("CC type: RBCC\n");

	  break;
	  default:
	        debug_msg("CC type: NOCC\n");
	}
	
	epc_ = 0;	// experimental packet counter
}

/* Return time of day in seconds */
double Transmitter::gettimeofday_secs() const
{
	timeval tv;
	::gettimeofday(&tv, 0);
	return (tv.tv_sec + 1e-6 * tv.tv_usec);
}

void Transmitter::loopback(pktbuf* pb)
{
	int layer = pb->layer;
	rtphdr* rh = (rtphdr*)pb->data;
        debug_msg( "loopback received frame_no: %d\n", rh->frame_no);
	int cc = pb->len;
	/*
	 * Update statistics.
	 */
	if (layer >= loop_layer_) {
		/*XXX*/
		pb->release();
		return;
	}
	nb_ += cc;
	++np_;

	SourceManager& sm = SourceManager::instance();
	Source* s = sm.localsrc();
	timeval now = unixtime();
	Source::Layer& sl = s->layer(pb->layer);

	sl.lts_data(now);
	s->action();
	sl.sts_data(rh->rh_ts);
	sl.np(1);
	sl.nb(cc);
	sl.cs((u_int16_t)ntohs(rh->rh_seqno),s);

	int flags = ntohs(rh->rh_flags);
	if (flags & RTP_M) {
		++nf_;
		sl.nf(1);
                debug_msg("now: %f loopback received last packet of frame_no: %d ",tx_get_now(), rh->frame_no);
	}
	int fmt = flags & 0x7f;
	/*
	 * Handle initialization of loopback decoder
	 * and changes in the stream.
	 */
	PacketHandler* h = s->handler();
	if (h == 0)
		h = s->activate(fmt);
	else if (s->format() != fmt) {
		h = s->change_format(fmt);
	}

	if (s->mute()) {
		pb->release();
		return;
	}
	h->recv(pb);
}

int Transmitter::dumpfd_ = -1;
void Transmitter::dump(int fd)
{
	dumpfd_ = fd;
#define MAGIC "RTPCLIP 1.0"
	(void)write(fd, MAGIC, sizeof(MAGIC));
}

/*XXX*/
#ifdef WIN32
int writev(int fd, iovec* iov, int iovlen)
{
	int len = 0, n;
	for (int i = 0; i < iovlen; i++) {
		if ((n = write(fd, iov[i].iov_base, iov[i].iov_len)) == -1) {
			perror("writev");
			exit(1);
		}
		len += n;
	}
	return(len);
}
#endif

void Transmitter::dump(int fd, iovec* iov, int iovlen) const
{
	register int length = 0;
	for (int i = iovlen; --i >= 0; )
		length += iov[i].iov_len;

	char cliphdr[4];
	*(short*)cliphdr = htons(length);
	cliphdr[2] = 0; /* data packet (i.e., not an rtcp packet) */
	cliphdr[3] = 0; /* ? */

	(void)write(fd, cliphdr, 4);
	if (writev(fd, iov, iovlen) < 0) {
		perror("write");
		exit(1);
	}
}

/*
 * Time it takes in seconds to send this
 * packet at the configured bandwidth.
 */
double Transmitter::txtime(pktbuf* pb)
{
	int cc = pb->len;

	/* Commented out this with change setting bps(xrate)
        fprintf(stderr,"TFRC txtime: %f, (cc: %d / xrate: %f) \n", cc/tfrc_sndr_.xrate(),
            cc,tfrc_sndr_.xrate());
	if (is_tfrc()){
	fprintf(stderr,"txtime: %f, (cc: %d / xrate: %f) \n", cc/tfrc_sndr_.xrate(),
            cc,tfrc_sndr_.xrate());
	return ( cc/tfrc_sndr_.xrate() );
        }else */
	debug_msg("txtime: psize: %d, kbps_: %d\n", cc ,kbps_);
	return (8 * cc / (1000. * kbps_));
}

/*
 * Tx pktbuf size
 */
int Transmitter::tx_buf_size() {
	int size = 0;
	pktbuf *pb = head_, *pbnext;
	while (pb) {
		pbnext = pb->next;
		pb = pbnext;
		size++;
	}
	return size;
}

// Entry point from codec
void Transmitter::send(pktbuf* pb)
{
	switch (cc_type_) {
	//
	// window-based congestion control (TFWC)
	//
	case WBCC:
	  // pb is empty - try sending a packet
  	  if(is_buf_empty_) {
		if (head_ != 0) {
		  tail_->next = pb;
		  tail_ = pb;
		} else
		  tail_ = head_ = pb;
		pb->next = 0;
		tfwc_output(0);
		is_buf_empty_ = false;
	  } 
	  // if not, check if cwnd allows send this packet
	  else {
		if (head_ != 0) {
		  tail_->next = pb;
		  tail_ = pb;
		} else
		  tail_ = head_ = pb;
		pb->next = 0;
		tfwc_output(pb, 0);
	  }
	  break;

	//
	// rate-based congestion control (TFRC)
	//
          // Disable this version as I don't think it works - Piers
	/*case RBCC:
	  // pb is empty
	  if(is_buf_empty_) {
		if (head_ != 0) {
		  tail_->next = pb;
		  tail_ = pb;
		} else
		  tail_ = head_ = pb;
		pb->next = 0;
		tfrc_output();
		is_buf_empty_ = false;
	  }
	  // pb is not emtpy
	  else {
		if (head_ != 0) {
		  tail_->next = pb;
		  tail_ = pb;
		} else
		  tail_ = head_ = pb;
		pb->next = 0;
		tfrc_output(pb);
	  }
	  break;*/
	//
	// without congestion control
	//
	case RBCC:
	case NOCC:
	default:
	  // CC is not active, so just go for the normal operation
	  if (!busy_) {
		double delay = txtime(pb);
		nextpkttime_ = gettimeofday_secs() + delay;
                int intdelay = int(delay * 1e3);
		output(pb);
		/*
		 * emulate a transmit interrupt --
		 * assume we will have more to send.
		 */
                debug_msg("Send:ing packet, delay till next send: %f(%d)\n",delay,intdelay);

		if (intdelay) {
                  msched(intdelay);
		  busy_ = 1;
                } 
	  } else {
                debug_msg("Send: Adding packet to txq (time: %f)\n", gettimeofday_secs());
		if (head_ != 0) {
		  tail_->next = pb;
		  tail_ = pb;
		} else
		  tail_ = head_ = pb;
		pb->next = 0;
	  }
	} // switch (cc_type)
}
 
// main TFWC CC output routines (called when NOT buff_is_empty)

void Transmitter::tfwc_output(pktbuf* _pb, bool ack_clock) 
{
        // Set pb to head_ of list (the arg pb is at the tail - Piers)
	pktbuf *pb = head_;
	//cc_output_banner_top("tfwc");
	// byte mode? or packet mode?
	switch (cwnd_mode_) {
	case BYM:
	{
	  // see if any XR has arrived to pick up
	  int cc = check_xr_arrival(pb, 1);
          int i=0;

	  debug_msg("B4 tfwc_output: %d cc:%d, pblen: %d\n", tfwc_sndr_.b_magic(), cc, pb->len);
	  while(pb->len <= tfwc_sndr_.b_magic() + cc) {
		// move head pointer
		head_ = pb->next;
		// call Transmitter::output_data_only w/ XR reception
		debug_msg("tfwc_output: %d cc:%d, pblen: %d\n", tfwc_sndr_.b_magic(), cc, pb->len);
		output_data_only(pb, ack_clock);
                i++;

		if (head_ != 0)
			pb = head_;
		else
			break;
	  }
	  debug_msg("tfwc_outputed: %d (txq: %d)\n", i, tx_buf_size());
	}
	break;
	case PKM:
	{
	  // pb is not null, hence parse it.
	  rtphdr* rh = (rtphdr *) pb->data;
	  // see if any XR has arrived to pick up
	  check_xr_arrival(pb, 1);

	  while (ntohs(rh->rh_seqno) <= tfwc_sndr_.magic() + tfwc_sndr_.jacked()) {
		debug_msg("cwnd: %d\n", tfwc_sndr_.magic());
		debug_msg("jack: %d\n", tfwc_sndr_.jacked());
			
		// move head pointer
		head_ = pb->next;
		// call Transmitter::output_data_only w/ XR reception
		output_data_only(pb, ack_clock);

		if (head_ != 0) {
			pb = head_;
			rh = (rtphdr *) pb->data;
                } else
			break;
	  }
	}
	break;
	} // switch (cwnd_mode_)
	//cc_output_banner_bottom();
}

/*
 * main TFWC CC output routines (called when buff_is_empty)
 */
void Transmitter::tfwc_output(bool ack_clock)
{
	//cc_output_banner_top("tfwc");
	// head of the RTP data packet buffer (pb)
	pktbuf* pb = head_;

	// if pb is null, then set the next available packet as the first packet of
	// the packet buffer. and then, return - i.e., do not try sending packets.
	if (pb == 0) {
		is_buf_empty_ = true;
//		fprintf(stderr,
//		"\t=========== PACKET NOT AVAILABLE ===========\n\n");
		return;
	}

	// byte mode? or packet mode?
	switch (cwnd_mode_) {
	case BYM:
	{
	  int len = 0;
	  // cwnd (in bytes)
	  //int b_magic = tfwc_sndr_.b_magic();
	  // see if any XR has arrived to pick up
	  int cc = check_xr_arrival(pb, 1);

	  while(pb->len <= tfwc_sndr_.b_magic() + cc - len) {
		len += pb->len;
		// move head pointer
		head_ = pb->next;
		// call Transmitter::output(pb)
		output(pb, ack_clock);
		// inflate nbytes as we receive ack
		cc += tot_num_acked_;

		if (head_ != 0)
			pb = head_;
		else
			break;
	  }
	}
	break;
	case PKM:
	{
	  // pb is not null, hence parse it.
	  rtphdr* rh = (rtphdr *) pb->data;
	  // see if any XR has arrived to pick up
	  check_xr_arrival(pb, 1);

	  // while packet seqno is within "cwnd + jack", send that packet
	  while (ntohs(rh->rh_seqno) <= tfwc_sndr_.magic() + tfwc_sndr_.jacked()) {
		debug_msg("cwnd: %d\n", tfwc_sndr_.magic());
		debug_msg("jack: %d\n", tfwc_sndr_.jacked());

		// move head pointer
		head_ = pb->next;
		// call Transmitter::output(pb)
		output(pb, ack_clock);

		// if the moved head pointer is not null, parse packet buffer.
		// otherwise, break while statement.
		if (head_ != 0) {
			pb = head_;
			rh = (rtphdr *) pb->data;
		} else {
			break;
		}
	  } // end while ()
	}
	break;
	} // switch (cwnd_mode_)
	//cc_output_banner_bottom();
}

/*
 * trigger packet out forcefully
 */
void Transmitter::tfwc_trigger(pktbuf* pb) {
	// if pb is null here, it means this routine was called 
	// by SessionManager::recv(CtrlHandler* ch).
	// therefore, assign pktbuf's head to pb.
	if (pb == 0)
	        pb = head_;

	// if pb is null here, it means the actual pkbuf is empty!
	if (pb == 0) {
		is_buf_empty_ = true;
		return;
	}

	// see if any XR has arrived in the mean time
	check_xr_arrival(pb, 1);

	// parse pb data
	//rtphdr* rh = (rtphdr *) pb->data;
	// move head pointer
	head_ = pb->next;
	// call Transmitter::output_data_only w/o XR reception
	output_data_only(pb, NO_XR_RECV);
}

/*
 * main TFRC CC output NOT USED - pkts sent in timeout()
 *                     ~~~~~~~~
 */
// main TFRC CC output routine (called when NOT buff_is_empty)
void Transmitter::tfrc_output(pktbuf* pb) {
	cc_output_banner_top("tfrc");
	// move head pointer
	head_ = pb->next;

	// call Transmitter::output_data_only w/ XR reception
	output_data_only(pb, XR_RECV);
	cc_output_banner_bottom();
}

// main TFRC CC output routine (called when buff_is_empty)
void Transmitter::tfrc_output(bool ack_clock) {
	cc_output_banner_top("tfrc");
	// head of the RTP data packet buffer
	pktbuf* pb = head_;

	// if pb is null, then set the next available packet as the first packet of
	// the packet buffer. and then, return - i.e., do not try sending packets.
	if (pb == 0) {
		is_buf_empty_ = true;
		return;
	}

    // move head pointer
    head_ = pb->next;
    // call Transmitter::output(pb)
    output(pb, ack_clock);  // BUG? This just calls net->send()....??? Piers
	cc_output_banner_bottom();
}

void Transmitter::timeout()
{
	double now = gettimeofday_secs();

        debug_msg("Timeout: nextpkttime_: %f, now: %f \n",nextpkttime_, now);
	switch (cc_type_) {
	/*case RBCC:
          for (;;) {
            pktbuf* p = head_;
            if(p != 0) {
	      head_ = p->next;
              nextpkttime_ += txtime(p);
              // call send(pktbuf *) here
              //send(p);
              output(p);
              // goto sleep
              int ms = int(1e-3 * (nextpkttime_ - now));
              msched(ms);
            }
          }
	  break;*/
	case WBCC:
          // nothing to do when WBCC mode
          break;
	case NOCC:
	case RBCC:
	default:
	  for (;;) {
                pktbuf* p = head_;
		if (p != 0) {
			head_ = p->next;
			nextpkttime_ += txtime(p);
			output(p);
			int ms = int(1e3 * (nextpkttime_ - gettimeofday_secs()));
                        debug_msg("Timeout: Sent packet, delay till next send: %d, nextpkttime_: %f, now: %f\n",ms,nextpkttime_, now);
			/* make sure we will wait more than 10ms */
			if (ms > 1000) {
				msched(ms);
				return;
			}
		} else {
                        debug_msg("Timeout: Send Q empty\n");
			busy_ = 0;
			break;
		}
	  }
	} // switch (cc_type_)
}

void Transmitter::flush()
{
	if (!is_cc_on()) {
		if (busy_) {
			busy_ = 0;
			cancel();
		}

		pktbuf* p = head_;
		while (p != 0) {
			pktbuf* n = p->next;
			output(p);
			p = n;
		}
		head_ = 0;
	}
}

// Transmit RTP and DO send AoA
void Transmitter::output(pktbuf* pb, bool ack_clock)
{
	//fprintf(stderr, "\n\tTransmitter::output()\n");
	//if (dumpfd_ >= 0)
	//	dump(dumpfd_, pb->iov, mh_.msg_iovlen);
//dprintf("layer: %d \n",pb->layer);
//SessionManager::transmit()
	transmit(pb, ack_clock);
	loopback(pb);
//	pb->release() is called by decoder in loopback;
}

// Transmit RTP and DO NOT send AoA
void Transmitter::output_data_only(pktbuf* pb, bool flag)
{
	tx_data_only(pb, flag);
	loopback(pb);
}

/*void Transmitter::release(pktbuf* pb)
{
	pb->next = freehdrs_;
	freehdrs_ = pb;
	buffer* p = pb->buf;
	if (p != 0) {
		p->next = freebufs_;
		freebufs_ = p;
	}
}
*/
