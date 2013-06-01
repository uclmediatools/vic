/*
 * Copyright (c) 1995 The Regents of the University of California.
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

#ifndef vic_session_h
#define vic_session_h

#include "net.h"
#include "transmitter.h"
#include "timer.h"
#include "iohandler.h"
#include "source.h"
#include "mbus_handler.h"
#include "grabber.h"

class Source;
class SessionManager;
/* 
 * global variable representing data sender or receiver
 * (by default, it is set to a data receiver.)
 *
 * e.g., for data sender, it must change this variable 
 * 		using i_am_sender() method.
*/
bool is_sender_ = false;

class DataHandler : public IOHandler {
public:
	DataHandler* next;
	inline DataHandler() : next(0), sm_(0), net_(0), addrp_(0) {}
//	inline DataHandler(SessionManager& sm) : sm_(sm), net_(0), addrp_(0) {}
	virtual void dispatch(int mask);
	inline Network* net() const { return (net_); }
	virtual void net(Network* net) {
		unlink();
		link(net->rchannel(), TK_READABLE);
		net_ = net;
		if (addrp_) delete addrp_;
		addrp_ = net->addr().copy(); // get right type of address
	}
	inline int recv(u_char* bp, int len, Address*& addrp) {
		return (net_->recv(bp, len, *(addrp = addrp_)));
	}
	inline void send(u_char* bp, int len) {
		net_->send(bp, len);
	}
	inline void manager(SessionManager* sm) { sm_ = sm; }

protected:
	SessionManager *sm_;
	Network* net_;
	Address *addrp_;

private:
};
/*
 * Parameters controling the RTCP report rate timer.
 */
#define CTRL_SESSION_BW_FRACTION (0.05)
#define CTRL_MIN_RPT_TIME (5.)
#define CTRL_SENDER_BW_FRACTION (0.25)
#define CTRL_RECEIVER_BW_FRACTION (1. - CTRL_SENDER_BW_FRACTION)
#define CTRL_SIZE_GAIN (1./8.)

class CtrlHandler : public DataHandler, public Timer {
    public:
	CtrlHandler();
//	inline CtrlHandler(SessionManager& sm) : DataHandler(sm) {}
	virtual void dispatch(int mask);
	inline Network* net() const { return (net_); }
// new for layering - individual report timers for each layer 
	virtual void timeout();
	virtual void net(Network* net);
	void adapt(int nsrc, int nrr, int we_sent);
	void sample_size(int cc);
	inline double rint() const { return (rint_); }
	void send_aoa();	// send ackofack (TfwcSndr side)
	void send_ts();		// send timestamp (TfwcSndr side)
	void send_ackv();	// send ackvec (TfwcRcvr side)
	void send_p();
	void send_ts_echo();

	// i am an RTP data sender/receiver
	inline void i_am_sender() { is_sender_ = true; }
	inline void i_am_receiver() { is_sender_ = false; }

 protected:
	void schedule_timer();
	double ctrl_inv_bw_;
	double ctrl_avg_size_;	/* (estimated) average size of ctrl packets */
	double rint_;		/* current session report rate (in ms) */

	// control channel dispatch debug info
	inline void dispatch_info(double now, int nbytes, int i) {
	debug_msg( "  \tnow: %f\tdispatched[%d]: %d\n",now,i,nbytes);
	}
};

class ReportTimer : public Timer {
    public:
	inline ReportTimer(SessionManager& sm) : sm_(sm) {}
	void timeout();
    protected:
	SessionManager& sm_;
};

class SessionManager : public Transmitter, public MtuAlloc {
public:
	SessionManager();
	virtual ~SessionManager();
	virtual int command(int argc, const char*const* argv);
	virtual int recv(CtrlHandler*);
	virtual void recv(DataHandler*);
	virtual void announce(CtrlHandler*); //LLL
//	virtual void send_bye();
	virtual inline void send_bye() { send_report(&ch_[0], 1); }
//	virtual void send_report();
	virtual void send_report(CtrlHandler*, int bye, int app = 0);
	virtual void build_xreport(CtrlHandler*, int bt);
	virtual void send_xreport(CtrlHandler* ch, 
		u_int8_t bt, u_int8_t rsvd, u_int8_t thin, 
		u_int16_t begin_seq, u_int16_t end_seq, 
		u_int16_t *chunks, u_int16_t num_chunks, 
		u_int32_t xrssrc);

	// receive XR
	int recv_xreport(CtrlHandler*, pktbuf*, bool);

	void build_aoa_pkt(CtrlHandler* ch);
	void build_ts_pkt(CtrlHandler* ch);
	void build_ackv_pkt(CtrlHandler* ch);
	void build_p_pkt(CtrlHandler* ch);
	void build_ts_echo_pkt(CtrlHandler* ch);

	// am i a data sender?
	inline bool am_i_sender() { return is_sender_; }
	// see if any XR has arrived
	virtual int check_xr_arrival(pktbuf*, bool);

protected:
//	void demux(rtphdr* rh, u_char* bp, int cc, Address & addr, int layer);
	void demux(pktbuf* pb, Address & addr);
	virtual int check_format(int fmt) const = 0;
	virtual void transmit(pktbuf* pb, bool ack_clock=0);
	virtual void tx_data_only(pktbuf* pb, bool ack_clock);
	void send_report(int bye);
        void send_fb_ecn(CtrlHandler* ch, Source::Layer& sl, u_int32_t seqno, u_int32_t media_ssrc);
	int build_bye(rtcphdr* rh, Source& local);
	u_char* build_sdes_item(u_char* p, int code, Source&);
	int build_sdes(rtcphdr* rh, Source& s);
	int build_app(rtcphdr* rh, Source& ls, const char *name, 
			void *data, int datalen);

	void parse_sr(rtcphdr* rh, int flags, u_char* ep,
		      Source* ps, Address & addr, int layer);
	void parse_rr(rtcphdr* rh, int flags, u_char* ep,
		      Source* ps, Address & addr, int layer);
        void parse_fb(rtcphdr* rh, int flags, u_char* ep,
                        Source* ps, Address & addr, int layer);
	void parse_xr(rtcphdr* rh, int flags, u_char* ep,
		      Source* ps, Address & addr, int layer, bool ack_clock, pktbuf* pb=0);
	void parse_rr_records(u_int32_t ssrc, rtcp_rr* r, int cnt,
			      const u_char* ep, Address & addr);
	void parse_xr_records(u_int32_t ssrc, rtcp_xr* xr, int cnt,
			      const u_char* ep, Address & addr, bool ack_clock, pktbuf* pb);
	int sdesbody(u_int32_t* p, u_char* ep, Source* ps,
		     Address & addr, u_int32_t ssrc, int layer);
	void parse_sdes(rtcphdr* rh, int flags, u_char* ep, Source* ps,
			Address & addr, u_int32_t ssrc, int layer);
	void parse_bye(rtcphdr* rh, int flags, u_char* ep, Source* ps);

	int parseopts(const u_char* bp, int cc, Address & addr) const;
	int ckid(const char*, int len);

	u_int32_t alloc_srcid(Address & addr) const;

	int lipSyncEnabled() { return (lipSyncEnabled_);}
	void lipSyncEnabled(int v) { lipSyncEnabled_=v;}

	char* stats(char* cp) const;

	DataHandler dh_[NLAYER];
	CtrlHandler ch_[NLAYER];

	// MBus stuff
	MBusHandler mb_; // Handles mbus and interfaces to mbus
					 // code in common libraries
	int lipSyncEnabled_;

	/*XXX cleanup*/
	u_int badversion_;
	u_int badoptions_;
	u_int badfmt_;
	u_int badext_;
	u_int nrunt_;

	u_int32_t last_np_;
	u_int32_t sdes_seq_;

	double rtcp_inv_bw_;
	double rtcp_avg_size_;	/* (estimated) average size of rtcp packets */
	double rint_;		/* current session report rate (in ms) */

	int confid_;

	BufferPool* pool_;
	u_char* pktbuf_;

	SourceManager *sm_;

	// RTP packet sequence number (for the use of AckVec)
	u_int16_t seqno_;		// RTP packet sequence number
	u_int16_t lastseq_;		// last packet's seqno
	u_int16_t ackofack_;	// Ack of ack

	// AckVector
	u_int16_t ackvec_;	// this is a bit vector
	// timestamp
	double recv_ts_;	// receive timestamp
	Grabber *grabber_;	// grabber pointer - set by a tcl command piers

private:
	// print RTP data packet's seqno
	inline void print_rtp_seqno(u_int16_t seqno) {
	debug_msg( "\n\tnow: %f\tseqno: %d\n\n",tx_get_now(),seqno);
	}
	inline void print_rtp_seqno(pktbuf* pb) {
	rtphdr* rh = (rtphdr *) pb->data;
	debug_msg( "\n\tnow: %f\tseqno: %d\n\n",
	    tx_get_now(),ntohs(rh->rh_seqno));
	}
	// print sender's XR info
	inline void sender_xr_info(const char* str, const int i,
	u_int16_t b, u_int16_t e, rtcp_xr_BT_1_hdr* xrh, u_int16_t l) {
	debug_msg( "  [%s +%d] beg:%d, end:%d, xr1len:%d (xrlen:%d)\n",
		str, i, b, e, ntohs(xrh->xr_len),l);
	}
	// print sender's XR info
	inline void sender_xr_ts_info(double ts) {
	debug_msg( "*** recv_ts: %f so_rtime: %f diff: %f\n",
		recv_ts_, ts, recv_ts_-ts);
	}
	// print receiver's XR info
	inline void receiver_xr_info(const char* str, const int i, u_int16_t *c) {
	debug_msg( "  [%s +%d] chunk[0]:%d\n",str, i, ntohs(c[0]));
	}
	// print parse XR banner
	inline void parse_xr_banner_top() {
	debug_msg(
	"~~~~~~~~~~~~~~~~~~entering parse_xr_records()~~~~~~~~~~~~~~~~~~\n");
	}
	inline void parse_xr_banner_bottom() {
	debug_msg(
	"-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-\n");
	}
	inline void xr_arrival_info(int nbytes, int i) {
	debug_msg( "  \tnow: %f\tnbytes[%d]: %d\n",tx_get_now(),i,nbytes);
	}
	inline void send_xr_info(int bt, u_int16_t b, u_int16_t e){
	debug_msg( "\tBT: %d\tnow: %f begin: %d end: %d\n",
	bt, tx_get_now(), b, e);
	}
};

class AudioSessionManager : public SessionManager {
    protected:
	int check_format(int fmt) const;
};

class VideoSessionManager : public SessionManager {
    protected:
	int check_format(int fmt) const;
};


#endif
