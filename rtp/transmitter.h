/*
 * Copyright (c) 1993-1995 The Regents of the University of California.
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
 * @(#) $Header$ (LBL)
 * $Id$
 */

#ifndef vic_transmitter_h
#define vic_transmitter_h

#ifdef WIN32
//#include <winsock.h>
#else
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/uio.h>
#endif
#include "vic_tcl.h"
#include "timer.h"
#include "rtp.h"
#include "inet.h"
#include "pktbuf-rtp.h"
#include "cc/tfwc_sndr.h"
#include "cc/tfwc_rcvr.h"
#include "cc/tfrc_sndr.h"
#include "cc/tfrc_rcvr.h"

#define NOCC	100 // No Congestion Control
#define WBCC	101 // Window Based - TFWC
#define RBCC	102 // Rate Based - TFRC

// cwnd mode for TFWC
#define PKM		1	// PacKet Mode
#define BYM		2	// BYtes Mode

//ecn_mode settings
#define ECN_AS_LOSS 0
#define ECN_IGNORE 1
#define ECN_ACTION 2

class TfwcSndr;
class TfwcRcvr;
class TfrcSndr;
class TfrcRcvr;

/*
 * The base object for performing the outbound path of
 * the application level protocol.
 */
class Transmitter : public TclObject, public Timer {
    public:
	Transmitter();
	virtual void timeout();

	struct buffer {
		struct buffer* next;
		/*
		 * make buffer twice as big as necessary so we can
		 * run off end while doing in-place encoding.
		 */
		u_char data[2 * RTP_MTU];
	};
/*	struct pktbuf {
		struct pktbuf* next;
		iovec iov[2];
		u_char hdr[MAXHDR];
		buffer* buf;
		int layer;
	};*/
	static void dump(int fd);
	static inline void seqno(u_int16_t s) { seqno_ = s; }
	inline void bps(int kbps) { kbps_ = kbps; }
	inline void loop_layer(int loop_layer) { loop_layer_ = loop_layer; }
	inline int loop_layer() { return loop_layer_; }
	inline int mtu() { return (mtu_); }
	void flush();
	void send(pktbuf*);
	inline bool is_cc_on() { return is_cc_active_; }
	inline bool is_tfrc() {
		if(is_cc_on() && (cc_type_ == RBCC))
		return true;
		else
		return false;
	}
    inline double packet_interval() {
        return ( tfrc_sndr_.psize()/tfrc_sndr_.xrate() );
    }

	// TFWC output
	void tfwc_output(bool ack_clock=0);
	void tfwc_output(pktbuf*, bool ack_clock);
	void tfwc_trigger(pktbuf* pb=0);
	// TFRC output
	void tfrc_output(bool ack_clock=0);
	void tfrc_output(pktbuf*);

	/*
	 * Buffer allocation hooks.
	 */
	//pktbuf* alloch(u_int32_t, int fmt);
	//void release(pktbuf*);
	//pktbuf* alloc(u_int32_t, int fmt);

	inline double tx_now() {
		timeval tv;
		::gettimeofday(&tv, NULL);
		return ((double) tv.tv_sec + 1e-6 * (double) tv.tv_usec);
	}
	double tx_now_offset_;
	inline double tx_get_now() { return (tx_now()-tx_ts_offset()); }
	virtual inline double tx_ts_offset() { return (tx_now_offset_); }

	// Tx pktbuf size
	virtual int tx_buf_size();

	TfwcSndr tfwc_sndr_;
	TfwcRcvr tfwc_rcvr_;
	TfrcSndr tfrc_sndr_;
	TfrcRcvr tfrc_rcvr_;

protected:
	void update(int nbytes);
	void dump(int fd, iovec*, int iovel) const;
	void loopback(pktbuf*);
	void output(pktbuf* pb, bool ack_clock=0);
	void output_data_only(pktbuf* pb, bool flag);
	virtual void transmit(pktbuf* pb, bool ack_clock=0) = 0;
	virtual void tx_data_only(pktbuf* pb, bool flag) = 0;
	double gettimeofday_secs() const;
	double txtime(pktbuf* pb);

	// check if new XR(s) have arrived
	virtual int check_xr_arrival(pktbuf*, bool) = 0;
	int tot_num_acked_;

	int mtu_;		/* mtu of wire (as seen by application) */
	msghdr mh_;

	int nf_;		/* number of frames sent */
	int nb_;		/* number of bytes sent */
	int np_;		/* number of packets sent */
	
	int kbps_;		/* bit-rate for interpkt spacing */
	double nextpkttime_;

	/* packet transmission queue */
	int busy_;
	pktbuf* head_;
	pktbuf* tail_;

	int loop_layer_;	/* # of layers to loop back (for testing) */

	int loopback_;		/* true to loopback data packets */
	int ecn_mode_;		/* true to loopback data packets */
	static int dumpfd_;	/* fd to dump packet stream to */
	static u_int16_t seqno_;

	/* Cc related variables */
	bool is_cc_active_;	/* is Cc module activated?		*/
	bool is_buf_empty_;		/* is pktbuf empty?	*/
	int cc_type_;
	int cwnd_mode_;	/* byte-mode? or packet-mode */
	int epc_;		/* experimental packet counter */

    private:
	static pktbuf* freehdrs_;
	static buffer* freebufs_;
	static int nbufs_;
	static int nhdrs_;

	// print banner
	inline void cc_output_banner_top(const char* str) {
	fprintf(stderr,"\t-------- entering %s_output() -----------\n", str);
	fprintf(stderr,"\t|                                          |\n");
	fprintf(stderr,"\tV                                          V\n");
	}
	inline void cc_output_banner_bottom() {
	fprintf(stderr,"\t^                                          ^\n");
	fprintf(stderr,"\t|                                          |\n");
	fprintf(stderr,"\t============================================\n");
	}
};

#endif
