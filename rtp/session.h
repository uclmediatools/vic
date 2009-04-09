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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef vic_session_h
#define vic_session_h

#include "net.h"
#include "transmitter.h"
#include "timer.h"
#include "iohandler.h"
#include "source.h"
#include "mbus_handler.h"

class Source;
class SessionManager;

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
 protected:
	void schedule_timer();
	double ctrl_inv_bw_;
	double ctrl_avg_size_;	/* (estimated) average size of ctrl packets */
	double rint_;		/* current session report rate (in ms) */
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
	virtual void recv(CtrlHandler*);
	virtual void recv(DataHandler*);
	virtual void announce(CtrlHandler*); //LLL
//	virtual void send_bye();
	virtual inline void send_bye() { send_report(&ch_[0], 1); }
//	virtual void send_report();
	virtual void send_report(CtrlHandler*, int bye, int app = 0);

protected:
//	void demux(rtphdr* rh, u_char* bp, int cc, Address & addr, int layer);
	void demux(pktbuf* pb, Address & addr);
	virtual int check_format(int fmt) const = 0;
	virtual void transmit(pktbuf* pb);
	void send_report(int bye);
	int build_bye(rtcphdr* rh, Source& local);
	u_char* build_sdes_item(u_char* p, int code, Source&);
	int build_sdes(rtcphdr* rh, Source& s);
	int build_app(rtcphdr* rh, Source& ls, const char *name, 
			void *data, int datalen);

	void parse_sr(rtcphdr* rh, int flags, u_char* ep,
		      Source* ps, Address & addr, int layer);
	void parse_rr(rtcphdr* rh, int flags, u_char* ep,
		      Source* ps, Address & addr, int layer);
	void parse_rr_records(u_int32_t ssrc, rtcp_rr* r, int cnt,
			      const u_char* ep, Address & addr);
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
