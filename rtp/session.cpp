/*
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

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include "config.h"
#include <math.h>
#include <errno.h>
#include <string.h>
#ifdef WIN32
extern "C" int getpid();
#endif
#include "source.h"
#include "vic_tcl.h"
#include "media-timer.h"
#include "crypt.h"
#include "timer.h"
#include "ntp-time.h"
#include "session.h"

/* added to support the mbus 
#include "mbus_handler.h"*/


static class SessionMatcher : public Matcher {
    public:
		SessionMatcher() : Matcher("session") {}
		TclObject* match(const char* id) {
			if (strcmp(id, "audio/rtp") == 0)
				return (new AudioSessionManager);
			else if (strcmp(id, "video/rtp") == 0)
				return (new VideoSessionManager);
			return (0);
		}
} session_matcher;

int VideoSessionManager::check_format(int fmt) const
{
	switch(fmt) {
		case RTP_PT_RAW:
		case RTP_PT_CELLB:
		case RTP_PT_JPEG:
		case RTP_PT_CUSEEME:
		case RTP_PT_NV:
		case RTP_PT_CPV:
		case RTP_PT_H261:
		case RTP_PT_BVC:
		case RTP_PT_H261_COMPAT:/*XXX*/
		case RTP_PT_H263:
		case RTP_PT_MPEG4:
		case RTP_PT_H264:
		case RTP_PT_H264_IOCOM:
		case RTP_PT_H263P:
		case RTP_PT_LDCT:
		case RTP_PT_PVH:
	        case RTP_PT_DV:
		case RTP_PT_H261AS:
		return (1);
	}
	return (0);
}

int AudioSessionManager::check_format(int fmt) const
{
	switch (fmt) {
	case RTP_PT_PCMU:
	case RTP_PT_CELP:
	case RTP_PT_GSM:
	case RTP_PT_DVI:
	case RTP_PT_LPC:
		return (1);
	}
	return (0);
}


static SessionManager* manager;

void
adios()
{
	if (SourceManager::instance().localsrc() != 0)
		manager->send_bye();
	exit(0);
}

/*void ReportTimer::timeout()
{
sm_.send_report();
}*/

void DataHandler::dispatch(int)
{
	sm_->recv(this);
}

void CtrlHandler::dispatch(int)
{
	sm_->recv(this);
}

CtrlHandler::CtrlHandler()
: ctrl_inv_bw_(0.),
ctrl_avg_size_(128.),
rint_(0.0) //SV-XXX: Debian
{
}

inline void CtrlHandler::schedule_timer()
{
	msched(int(fmod(double(random()), rint_) + rint_ * .5 + .5));
}

void CtrlHandler::net(Network* n)
{
	DataHandler::net(n);
	cancel();
	if (n != 0) {
	/*
	* schedule a timer for our first report using half the
	* min ctrl interval.  This gives us some time before
	* our first report to learn about other sources so our
	* next report interval will account for them.  The avg
	* ctrl size was initialized to 128 bytes which is
	* conservative (it assumes everyone else is generating
	* SRs instead of RRs).
		*/
		double rint = ctrl_avg_size_ * ctrl_inv_bw_;
		if (rint < CTRL_MIN_RPT_TIME / 2. * 1000.)
			rint = CTRL_MIN_RPT_TIME / 2. * 1000.;
		rint_ = rint;
		schedule_timer();
	}
}

void CtrlHandler::sample_size(int cc)
{
	ctrl_avg_size_ += CTRL_SIZE_GAIN * (double(cc + 28) - ctrl_avg_size_);
}

void CtrlHandler::adapt(int nsrc, int nrr, int we_sent)
{
/*
* compute the time to the next report.  we do this here
* because we need to know if there were any active sources
* during the last report period (nrr above) & if we were
* a source.  The bandwidth limit for ctrl traffic was set
* on startup from the session bandwidth.  It is the inverse
* of bandwidth (ie., ms/byte) to avoid a divide below.
	*/
	double ibw = ctrl_inv_bw_;
	if (nrr) {
		/* there were active sources */
		if (we_sent) {
			ibw *= 1./CTRL_SENDER_BW_FRACTION;
			nsrc = nrr;
		} else {
			ibw *= 1./CTRL_RECEIVER_BW_FRACTION;
			nsrc -= nrr;
		}
	}
	double rint = ctrl_avg_size_ * double(nsrc) * ibw;
	if (rint < CTRL_MIN_RPT_TIME * 1000.)
		rint = CTRL_MIN_RPT_TIME * 1000.;
	rint_ = rint;
}

void CtrlHandler::timeout()
{
	sm_->announce(this);
	schedule_timer();
}

//SV-XXX: rearranged initialization order to shut up gcc4
SessionManager::SessionManager()
//	: dh_(*this), ch_(*this), rt_(*this), 
: mb_(mbus_handler_engine, NULL),
lipSyncEnabled_(0),
badversion_(0), 
badoptions_(0), 
badfmt_(0), 
badext_(0),
nrunt_(0),
last_np_(0), 
sdes_seq_(0),
rtcp_inv_bw_(0.),
rtcp_avg_size_(128.),
confid_(-1)
{
	/*XXX For adios() to send bye*/
	manager = this;
	
	for (int i = 0; i < NLAYER; ++i) {
		dh_[i].manager(this);
		ch_[i].manager(this);
	}
	
	/*XXX*/
	pktbuf_ = new u_char[2 * RTP_MTU];
	pool_ = new BufferPool;
	
	/*
	* schedule a timer for our first report using half the
	* min rtcp interval.  This gives us some time before
	* our first report to learn about other sources so our
	* next report interval will account for them.  The avg
	* rtcp size was initialized to 128 bytes which is
	* conservative (it assumes everyone else is generating
	* SRs instead of RRs).
	*/
	double rint = rtcp_avg_size_ * rtcp_inv_bw_;
	if (rint < RTCP_MIN_RPT_TIME / 2. * 1000.)
		rint = RTCP_MIN_RPT_TIME / 2. * 1000.;
	rint_ = rint;
	//rt_.msched(int(fmod(double(random()), rint) + rint * .5 + .5));
}

SessionManager::~SessionManager()
{
	if (pktbuf_) 
		delete[] pktbuf_;
	
	delete pool_;
}

u_int32_t SessionManager::alloc_srcid(Address & addr) const
{
	timeval tv;
	::gettimeofday(&tv, 0);
	u_int32_t srcid = u_int32_t(tv.tv_sec + tv.tv_usec);
	srcid += (u_int32_t)getuid();
	srcid += (u_int32_t)getpid();
/* __IPv6 changed srcid computation */
	for(unsigned int i = 0; i < (addr.length() % sizeof(u_int32_t)); i++) {
		srcid += ((u_int32_t*)((const void*)addr))[i];
	}
	return (srcid);
}

extern char* onestat(char* cp, const char* name, u_long v);

char* SessionManager::stats(char* cp) const
{
	cp = onestat(cp, "Bad-RTP-version", badversion_);
	cp = onestat(cp, "Bad-RTPv1-options", badoptions_);
	cp = onestat(cp, "Bad-Payload-Format", badfmt_);
	cp = onestat(cp, "Bad-RTP-Extension", badext_);
	cp = onestat(cp, "Runts", nrunt_);
	Crypt* p = dh_[0].net()->crypt();
	if (p != 0) {
		cp = onestat(cp, "Crypt-Bad-Length", p->badpktlen());
		cp = onestat(cp, "Crypt-Bad-P-Bit", p->badpbit());
	}
	/*XXX*/
	if (ch_[0].net() != 0) {
		Crypt* p = ch_[0].net()->crypt();
		if (p != 0) {
			cp = onestat(cp, "Crypt-Ctrl-Bad-Length", p->badpktlen());
			cp = onestat(cp, "Crypt-Ctrl-Bad-P-Bit", p->badpbit());
		}
	}
	*--cp = 0;
	return (cp);
}

int SessionManager::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	char* cp = tcl.buffer();
	if (argc == 2) {
		if (strcmp(argv[1], "active") == 0) {
			SourceManager::instance().sortactive(cp);
			tcl.result(cp);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "local-addr-heuristic") == 0) {
			strcpy(cp, intoa(LookupLocalAddr()));
			tcl.result(cp);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "stats") == 0) {
			stats(cp);
			tcl.result(cp);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "nb") == 0) {
			sprintf(cp, "%u", 8 * nb_);
			tcl.result(cp);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "nf") == 0) {
			sprintf(cp, "%u", nf_);
			tcl.result(cp);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "np") == 0 ||
		    strcmp(argv[1], "ns") == 0) {
			sprintf(cp, "%u", np_);
			tcl.result(cp);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "lip-sync") == 0) {
			sprintf(cp, "%u", lipSyncEnabled_);
			tcl.result(cp);
			return (TCL_OK);
		}

	} else if (argc == 3) {
		if (strcmp(argv[1], "sm") == 0) {
			sm_ = (SourceManager*)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "name") == 0) {
			Source* s = SourceManager::instance().localsrc();
			s->sdes(RTCP_SDES_NAME, argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "email") == 0) {
			Source* s = SourceManager::instance().localsrc();
			s->sdes(RTCP_SDES_EMAIL, argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "random-srcid") == 0) {
			Address * addrp;
			if ((addrp = (dh_[0].net())->alloc(argv[2])) !=0 ) { //SV-XXX: placed ()'s against truth check == NULL
			  sprintf(cp, "%u", alloc_srcid(*addrp));
			  delete addrp;
			}
			tcl.result(cp);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "data-net") == 0) {
			dh_[0].net((Network*)TclObject::lookup(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ctrl-net") == 0) {
			ch_[0].net((Network*)TclObject::lookup(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "max-bandwidth") == 0) {
			double bw = atof(argv[2]) / 8.;
			rtcp_inv_bw_ = 1. / (bw * RTCP_SESSION_BW_FRACTION);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "confid") == 0) {
			confid_ = atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "data-bandwidth") == 0) {
			/*XXX assume value in range */
			bps(atoi(argv[2]));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "mtu") == 0) {
			mtu_ = atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "loopback") == 0) {
			loopback_ = atoi(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "lip-sync") == 0) {
			lipSyncEnabled_ = atoi(argv[2]);
			return (TCL_OK);
		}

	}  else if (argc == 4) {
		if (strcmp(argv[1], "data-net") == 0) {
			u_int layer = atoi(argv[3]);
			if (layer >= NLAYER)
				abort();
			if (*argv[2] == 0) {
				dh_[layer].net(0);
				return (TCL_OK);
			}
			Network* net = (Network*)TclObject::lookup(argv[2]);
			if (net == 0) {
				tcl.resultf("no network %s", argv[2]);
				return (TCL_ERROR);
			}
			if (net->rchannel() < 0) {
				tcl.resultf("network not open");
				return (TCL_ERROR);
			}
			dh_[layer].net(net);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ctrl-net") == 0) {
			u_int layer = atoi(argv[3]);
			if (layer >= NLAYER)
				abort();
			if (*argv[2] == 0) {
				ch_[layer].net(0);
				return (TCL_OK);
			}
			Network* net = (Network*)TclObject::lookup(argv[2]);
			if (net == 0) {
				tcl.resultf("no network %s", argv[2]);
				return (TCL_ERROR);
			}
			if (net->rchannel() < 0) {
				tcl.resultf("network not open");
				return (TCL_ERROR);
			}
			ch_[layer].net(net);
			return (TCL_OK);
		}
		return (Transmitter::command(argc, argv));
	}
	//should not be reached
	return (TCL_ERROR);
}

void SessionManager::transmit(pktbuf* pb)
{
	//mh_.msg_iov = pb->iov;
	//	dh_[.net()->send(mh_);
		//debug_msg("L %d,",pb->layer);
	// Using loop_layer for now to restrict transmission as well
	if (pb->layer < loop_layer_) {
	//	if ( pb->layer <0 ) exit(1);
		Network* n = dh_[pb->layer].net();
		if (n != 0)
			n->send(pb);
	}
}

u_char* SessionManager::build_sdes_item(u_char* p, int code, Source& s)
{
	const char* value = s.sdes(code);
	if (value != 0) {
		int len = strlen(value);
		*p++ = code;
		*p++ = len;
		memcpy(p, value, len);
		p += len;
	}
	return (p);
}

int SessionManager::build_sdes(rtcphdr* rh, Source& ls)
{
	int flags = RTP_VERSION << 14 | 1 << 8 | RTCP_PT_SDES;
	rh->rh_flags = htons(flags);
	rh->rh_ssrc = ls.srcid();
	u_char* p = (u_char*)(rh + 1);
	p = build_sdes_item(p, RTCP_SDES_CNAME, ls);

	/*
	 * We always send a cname plus one other sdes
	 * There's a schedule for what we send sequenced by sdes_seq_:
	 *   - send 'email' every 0th & 4th packet
	 *   - send 'note' every 2nd packet
	 *   - send 'tool' every 6th packet
	 *   - send 'name' in all the odd slots
	 * (if 'note' is not the empty string, we switch the roles
	 *  of name & note)
	 */
	int nameslot, noteslot;
	const char* note = ls.sdes(RTCP_SDES_NOTE);
	if (note) {
		if (*note) {
			nameslot = RTCP_SDES_NOTE;
			noteslot = RTCP_SDES_NAME;
		} else {
			nameslot = RTCP_SDES_NAME;
			noteslot = RTCP_SDES_NOTE;
		}
	} else {
		nameslot = RTCP_SDES_NAME;
		noteslot = RTCP_SDES_NAME;
	}
	u_int seq = (++sdes_seq_) & 0x7;
	switch (seq) {

	case 0:  case 4:
		p = build_sdes_item(p, RTCP_SDES_EMAIL, ls);
		break;

	case 2:
		p = build_sdes_item(p, noteslot, ls);
		break;
	case 6:
		p = build_sdes_item(p, RTCP_SDES_TOOL, ls);
		break;
	default:
		p = build_sdes_item(p, nameslot, ls);
	}
	int len = p - (u_char*)rh;
	int pad = 4 - (len & 3);
	len += pad;
	rh->rh_len = htons((len >> 2) - 1);
	while (--pad >= 0)
		*p++ = 0;

	return (len);
}

int SessionManager::build_app(rtcphdr* rh, Source& ls, const char *name, 
		void *data, int datalen)
{
  int flags = RTP_VERSION << 14 | 1 << 8 | RTCP_PT_APP;
  rh->rh_flags = htons(flags);
  rh->rh_ssrc = ls.srcid();
  u_char* p = (u_char*)(rh + 1);
    int len;
    
    // write the name field
    len = strlen(name);
    if( len < 4 ) {
        memcpy(p,name,len);
        p += len;
        
        // pad short names
        while( p - (u_char*)(rh+1) < 4 )
            *p++ = 0;
    }
    else {
        // use first four chars of given name
        memcpy(p,name,4);
        p += 4;
    }
    
    // write the app data
    memcpy(p,data,datalen);
    p += datalen;
    
    // pad as needed
    len = p - (u_char*)rh;
    int pad = 4 - (len & 3);
    while( --pad >= 0 )
        *p++ = 0;
  len = p - (u_char*)rh;

    // set the length now that it's known
  rh->rh_len = htons((len >> 2) - 1);

  return (len);
}

/*void SessionManager::send_report()
{
	send_report(0);
}

  void SessionManager::send_bye()
  {
  send_report(1);
}*/

// SessionManager is no longer used as Timer - Each
// CtrlHandler has its own Timer which calls this;
void SessionManager::announce(CtrlHandler* ch)
{
	send_report(ch, 0);
}

/*XXX check for buffer overflow*/
/*
* Send an RTPv2 report packet.
*/
//void SessionManager::send_report(int bye)

void SessionManager::send_report(CtrlHandler* ch, int bye, int app)
{
	UNUSED(app); //SV-XXX: unused

	SourceManager& sm = SourceManager::instance();
	Source& s = *sm.localsrc();
	rtcphdr* rh = (rtcphdr*)pktbuf_;
	rh->rh_ssrc = s.srcid();
	int flags = RTP_VERSION << 14;
	int layer = ch - ch_; //LLL
	Source::Layer& sl = s.layer(layer);
	timeval now = unixtime();
	sl.lts_ctrl(now);
	int we_sent = 0;
	rtcp_rr* rr;
	Tcl& tcl = Tcl::instance();

	/*
	 * If we've sent data since our last sender report send a
	 * new report.  The MediaTimer check is to make sure we still
	 * have a grabber -- if not, we won't be able to interpret the
	 * media timestamps so there's no point in sending an SR.
	 */
	MediaTimer* mt = MediaTimer::instance();
	if (sl.np() != last_np_ && mt) {
		last_np_ = sl.np();
		we_sent = 1;
		flags |= RTCP_PT_SR;
		rtcp_sr* sr = (rtcp_sr*)(rh + 1);
		sr->sr_ntp = ntp64time(now);
		HTONL(sr->sr_ntp.upper);
		HTONL(sr->sr_ntp.lower);
		sr->sr_ts = htonl(mt->ref_ts());
		sr->sr_np = htonl(sl.np());
		sr->sr_nb = htonl(sl.nb());
		rr = (rtcp_rr*)(sr + 1);
	} else {
		flags |= RTCP_PT_RR;
		rr = (rtcp_rr*)(rh + 1);
	}
	int nrr = 0;
	int nsrc = 0;
	/*
	* we don't want to inflate report interval if user has set
	* the flag that causes all sources to be 'kept' so we
	* consider sources 'inactive' if we haven't heard a control
	* msg from them for ~32 reporting intervals.
	*/
	//LLL	u_int inactive = u_int(rint_ * (32./1000.));
	u_int inactive = u_int(ch->rint() * (32./1000.));
	if (inactive < 2)
		inactive = 2;
	for (Source* sp = sm.sources(); sp != 0; sp = sp->next_) {
		++nsrc;
		Source::Layer& sl = sp->layer(layer);
		//		int received = sp->np() - sp->snp();
		int received = sl.np() - sl.snp();
		if (received == 0) {
			//			if (u_int(now.tv_sec - sp->lts_ctrl().tv_sec) > inactive)
			if (u_int(now.tv_sec - sl.lts_ctrl().tv_sec) > inactive)
				--nsrc;
			continue;
		}
		//		sp->snp(sp->np());
		sl.snp(sl.np());
		rr->rr_srcid = sp->srcid();
		//		int expected = sp->ns() - sp->sns();
		int expected = sl.ns() - sl.sns();
		//		sp->sns(sp->ns());
		sl.sns(sl.ns());
		u_int32_t v;
		int lost = expected - received;
		if (lost <= 0)
			v = 0;
		else
			/* expected != 0 if lost > 0 */
			v = ((lost << 8) / expected) << 24;
		/* XXX should saturate on over/underflow */
		//		v |= (sp->ns() - sp->np()) & 0xffffff;
		v |= (sl.ns() - sl.np()) & 0xffffff;
		rr->rr_loss = htonl(v);
		//		rr->rr_ehsr = htonl(sp->ehs());
		rr->rr_ehsr = htonl(sl.ehs());
		rr->rr_dv = (sp->handler() != 0) ? sp->handler()->delvar() : 0;
		//		rr->rr_lsr = htonl(sp->sts_ctrl());
		rr->rr_lsr = htonl(sl.sts_ctrl());
		//		if (sp->lts_ctrl().tv_sec == 0)
		if (sl.lts_ctrl().tv_sec == 0)
			rr->rr_dlsr = 0;
		else {
			u_int32_t ntp_now = ntptime(now);
			//			u_int32_t ntp_then = ntptime(sp->lts_ctrl());
			u_int32_t ntp_then = ntptime(sl.lts_ctrl());
			rr->rr_dlsr = htonl(ntp_now - ntp_then);
		}
		++rr;
		if (++nrr >= 31)
			break;
	}
	flags |= nrr << 8;
	rh->rh_flags = htons(flags);
	int len = (u_char*)rr - pktbuf_;
	rh->rh_len = htons((len >> 2) - 1);

	if (bye)
		len += build_bye((rtcphdr*)rr, s);
	else
		len += build_sdes((rtcphdr*)rr, s);
	
	// build "site" app data if specified
	const char *data = tcl.attr("site");
	if(data) 
	{
	    rr = (rtcp_rr*)(((u_char*)rh) + len);
	    len += build_app((rtcphdr*)rr, s, "site", (void *)data, strlen(data));
	}
	//LLL	ch_.send(pktbuf_, len);
	ch->send(pktbuf_, len);
	
	/*
      rtcp_avg_size_ += RTCP_SIZE_GAIN * (double(len + 28) - rtcp_avg_size_);
	  
	  // compute the time to the next report.  we do this here
	  // because we need to know if there were any active sources
	  // during the last report period (nrr above) & if we were
	  // a source.  The bandwidth limit for rtcp traffic was set
	  // on startup from the session bandwidth.  It is the inverse
	  // of bandwidth (ie., ms/byte) to avoid a divide below.
	
	//	double ibw = rtcp_inv_bw_;
	if (nrr) {
	// there were active sources 
	//		if (we_sent) {
	ibw *= 1./RTCP_SENDER_BW_FRACTION;
	nsrc = nrr;
	} else {
	ibw *= 1./RTCP_RECEIVER_BW_FRACTION;
	nsrc -= nrr;
	}
	}
	double rint = rtcp_avg_size_ * double(nsrc) * ibw;
	if (rint < RTCP_MIN_RPT_TIME * 1000.)
		rint = RTCP_MIN_RPT_TIME * 1000.;
	rint_ = rint;
	rt_.msched(int(fmod(double(random()), rint) + rint * .5 + .5));
	*/
	
	// Call timer adaption for each layer
	ch->adapt(nsrc, nrr, we_sent);
	ch->sample_size(len);
	
	//	sm.CheckActiveSources(rint);
	if (layer == 0)
		sm.CheckActiveSources(ch->rint());
	
}

int SessionManager::build_bye(rtcphdr* rh, Source& ls)
{
	int flags = RTP_VERSION << 14 | 1 << 8 | RTCP_PT_BYE;
	rh->rh_flags = ntohs(flags);
	rh->rh_len = htons(1);
	rh->rh_ssrc = ls.srcid();
	return (8);
}

void SessionManager::recv(DataHandler* dh)
{
	int layer = dh - dh_;
	pktbuf* pb = pool_->alloc(layer);
	Address * addrp;
	/* leave room in case we need to expand rtpv1 into an rtpv2 header */
	/* XXX the free mem routine didn't like it ... */
	//u_char* bp = &pktbuf_[4];
	//u_char* bp = pktbuf_;
	
	int cc = dh->recv(pb->data, sizeof(pb->data), addrp);
	//int cc = dh->recv(bp, 2 * RTP_MTU - 4, addrp);
	if (cc <= 0) {
		pb->release();
		return;
	}

    // Ignore loopback packets
	if (!loopback_) {
		rtphdr* rh = (rtphdr*)pb->data;
		SourceManager& sm = SourceManager::instance();
		if (rh->rh_ssrc == (*sm.localsrc()).srcid()) {
			pb->release();
			return;
		}
	}

	//rtphdr* rh = (rtphdr*)pb->data;
	int version = pb->data[0] >> 6;
	//int version = *(u_char*)rh >> 6;
	if (version != 2) {
		++badversion_;
		pb->release();
		return;
	}
	if (cc < (int)sizeof(rtphdr)) {
		++nrunt_;
		pb->release();
		return;
	}
	pb->len = cc;
	
	//bp += sizeof(*rh);
	//cc -= sizeof(*rh);
	demux(pb, *addrp);
}

void SessionManager::demux(pktbuf* pb, Address & addr)
{
	rtphdr* rh = (rtphdr*)pb->data;
	u_int32_t srcid = rh->rh_ssrc;
	int flags = ntohs(rh->rh_flags);
	// for LIP SYNC
	//SV-XXX: unused: u_char *pkt = pb->data - sizeof(*rh);

	if ((flags & RTP_X) != 0) {
	/*
	* the minimal-control audio/video profile
	* explicitly forbids extensions
		*/
		++badext_;
		pb->release();
		return;
	}

	/*
	 * Check for illegal payload types.  Most likely this is
	 * a session packet arriving on the data port.
	 */
	int fmt = flags & 0x7f;
	if (!check_format(fmt)) {
		++badfmt_;
		pb->release();
		return;
	}

	SourceManager& sm = SourceManager::instance();
	u_int16_t seqno = ntohs(rh->rh_seqno);
	Source* s = sm.demux(srcid, addr, seqno, pb->layer);
	if (s == 0) {
	/*
	* Takes a pair of validated packets before we will
	* believe the source.  This prevents a runaway
	* allocation of Source data structures for a
	* stream of garbage packets.
		*/
		pb->release();
		return;
	}
	/* inform this source of the mbus */
	s->mbus(&mb_);
	
	Source::Layer& sl = s->layer(pb->layer);
	timeval now = unixtime();
	//	s->lts_data(now);
	sl.lts_data(now);
	//	s->sts_data(rh->rh_ts);
	sl.sts_data(rh->rh_ts);
	
	// extract CSRC count (CC field); increment pb->dp data pointer & adjust length accordingly
	int cnt = (flags >> 8) & 0xf;
	if (cnt > 0) {
		//u_char* nh = (u_char*)rh + (cnt << 2);
		rtphdr hdr = *rh;
		pb->dp += (cnt << 2);
		pb->len -= (cnt << 2);
		u_int32_t* bp = (u_int32_t*)(rh + 1);
		while (--cnt >= 0) {
			u_int32_t csrc = *(u_int32_t*)bp;
			bp += 4;
			Source* cs = sm.lookup(csrc, srcid, addr);
			//			cs->lts_data(now);
			cs->layer(pb->layer).lts_data(now);
			cs->action();
		}
		//		rtphdr hdr = *rh;
		//		rh = (rtphdr*)nh;
		/*XXX move header up so it's contiguous with data*/
		rh = (rtphdr*)pb->dp;
		*rh = hdr;
	} else
		s->action();

	if (s->sync() && lipSyncEnabled()) {  
		/*
		 * Synchronisation is enabled on this source; 
		 * video packets have to 
		 * be buffered, their playout point scheduled, and the 
		 * playout delays communicated with the audio tool ...
		 */ 

		/*
		* XXX bit rate doesn't include rtpv1 options;
		* but v1 is going away anyway.
		*/
		//		int dup = s->cs(seqno);
		//		s->np(1);
		//		s->nb(cc + sizeof(*rh));
		int dup = sl.cs(seqno, s);
		sl.np(1);
		sl.nb(pb->len);
		if (dup) {
			pb->release();
			return;
		}
		if (flags & RTP_M) // check if reach frame boundaries
			//			s->nf(1);
			sl.nf(1);
		
		//s->recv(pkt, rh, pb, pb->len); // this should invoke Source::recv and buffer
		//s->recv(bp); // this should invoke Source::recv and buffer
		
		pktbuf_ = (u_char*)new_blk(); 
	} /* synced */

	else { /* ... playout video packets as they arrive */
	        /*
       	 	 * This is a data packet.  If the source needs activation,
	         * or the packet format has changed, deal with this.
		 * Then, hand the packet off to the packet handler.
	         * XXX might want to be careful about flip-flopping
	         * here when format changes due to misordered packets
	         * (easy solution -- keep rtp seqno of last fmt change).
	         */
		PacketHandler* h = s->handler();
		if (h == 0)
			h = s->activate(fmt);
		else if (s->format() != fmt)
			h = s->change_format(fmt);
		
			/*
			* XXX bit rate doesn't include rtpv1 options;
			* but v1 is going away anyway.
		*/
		//	        int dup = s->cs(seqno);
		//	        s->np(1);
		//	        s->nb(cc + sizeof(*rh));
		int dup = sl.cs(seqno, s);
		sl.np(1);
		sl.nb(pb->len);
		if (dup){
			pb->release();
			return;
		}
		if (flags & RTP_M)
			//	            s->nf(1);
			sl.nf(1);
#ifdef notdef
	/* This should move to the handler */
	/*XXX could get rid of hdrlen and move run check into recv method*/
		
		int hlen = h->hdrlen();
		cc -= hlen;
		if (cc < 0) {
			//	            s->runt(1);
			sl.runt(1);
			pb->release();
			return;
		}
#endif
		if (s->mute()) {
			pb->release();
			return;
		}
		//h->recv(rh, bp + hlen, cc);
		h->recv(pb);
	} /* not sync-ed */

}

void SessionManager::parse_rr_records(u_int32_t, rtcp_rr*, int,
				      const u_char*, Address &)
{
}
				      

void SessionManager::parse_sr(rtcphdr* rh, int flags, u_char*ep,
							  Source* ps, Address & addr, int layer)
{
	rtcp_sr* sr = (rtcp_sr*)(rh + 1);
	Source* s;
	u_int32_t ssrc = rh->rh_ssrc;
	if (ps->srcid() != ssrc)
		s = SourceManager::instance().lookup(ssrc, ssrc, addr);
	else
		s = ps;
	
	Source::Layer& sl = s->layer(layer);
	
	timeval now = unixtime();

	sl.lts_ctrl(now);
	sl.sts_ctrl(ntohl(sr->sr_ntp.upper) << 16 |
		ntohl(sr->sr_ntp.lower) >> 16);
	
	//int cnt = flags >> 8 & 0x1f;
	//parse_rr_records(ssrc, (rtcp_rr*)(sr + 1), cnt, ep, addr);
	
	/*s->lts_ctrl(now);
	s->sts_ctrl(ntohl(sr->sr_ntp.upper) << 16 |
	ntohl(sr->sr_ntp.lower) >> 16);*/
	
	s->rtp_ctrl(ntohl(sr->sr_ts));
	u_int32_t t = ntptime(now);
	s->map_ntp_time(t);
	s->map_rtp_time(s->convert_time(t));
	s->rtp2ntp(1);
	//printf("Got SR\n");

	int cnt = flags >> 8 & 0x1f;
	parse_rr_records(ssrc, (rtcp_rr*)(sr + 1), cnt, ep, addr);
}

void SessionManager::parse_rr(rtcphdr* rh, int flags, u_char* ep,
							  Source* ps, Address & addr, int layer)
{
	Source* s;
	u_int32_t ssrc = rh->rh_ssrc;
	if (ps->srcid() != ssrc)
		s = SourceManager::instance().lookup(ssrc, ssrc, addr);
	else
		s = ps;
	
	s->layer(layer).lts_ctrl(unixtime());
	int cnt = flags >> 8 & 0x1f;
	parse_rr_records(ssrc, (rtcp_rr*)(rh + 1), cnt, ep, addr);
}

int SessionManager::sdesbody(u_int32_t* p, u_char* ep, Source* ps,
						Address & addr, u_int32_t ssrc, int layer)
{
	Source* s;
	u_int32_t srcid = *p;
	if (ps->srcid() != srcid)
		s = SourceManager::instance().lookup(srcid, ssrc, addr);
	else
		s = ps;
	if (s == 0)
		return (0);
		/*
		* Note ctrl packet since we will never see any direct ctrl packets
		* from a source through a mixer (and we don't want the source to
		* time out).
	*/
	s->layer(layer).lts_ctrl(unixtime());
	
	u_char* cp = (u_char*)(p + 1);
	while (cp < ep) {
		char buf[256];

		u_int type = cp[0];
		if (type == 0) {
			/* end of chunk */
			return (((cp - (u_char*)p) >> 2) + 1);
		}
		u_int len = cp[1];
		u_char* eopt = cp + len + 2;
		if (eopt > ep)
			return (0);

		if (type >= RTCP_SDES_MIN && type <= RTCP_SDES_MAX) {
			memcpy(buf, (char*)&cp[2], len);
			buf[len] = 0;
			s->sdes(type, buf);
		} else
			/*XXX*/;

		cp = eopt;
	}
	return (0);
}

void SessionManager::parse_sdes(rtcphdr* rh, int flags, u_char* ep, Source* ps,
								Address & addr, u_int32_t ssrc, int layer)
{
	int cnt = flags >> 8 & 0x1f;
	u_int32_t* p = (u_int32_t*)&rh->rh_ssrc;
	while (--cnt >= 0 && (u_char*)p < ep) {
		int n = sdesbody(p, ep, ps, addr, ssrc, layer);
		if (n == 0)
			break;
		p += n;
	}
	if (cnt >= 0)
		ps->badsdes(1);
}

void SessionManager::parse_bye(rtcphdr* rh, int flags, u_char* ep, Source* ps)
{
	int cnt = flags >> 8 & 0x1f;
	u_int32_t* p = (u_int32_t*)&rh->rh_ssrc;

	while (--cnt >= 0) {
		if (p >= (u_int32_t*)ep) {
			ps->badbye(1);
			return;
		}
		Source* s;
		if (ps->srcid() != rh->rh_ssrc)
			s = SourceManager::instance().consult(*p);
		else
			s = ps;
		if (s != 0)
			s->lts_done(unixtime());
		++p;
	}
}

/*
 * Receive an rtcp packet (from the control port).
 */
void SessionManager::recv(CtrlHandler* ch)
{
	Address * srcp;
	int cc = ch->recv(pktbuf_, 2 * RTP_MTU, srcp);
	if (cc <= 0)
		return;

	rtcphdr* rh = (rtcphdr*)pktbuf_;

    // Ignore loopback packets
	if (!loopback_) {
		SourceManager& sm = SourceManager::instance();
		if (rh->rh_ssrc == (*sm.localsrc()).srcid())
			return;
	}

	if (cc < int(sizeof(*rh))) {
		++nrunt_;
		return;
	}
	/*
	 * try to filter out junk: first thing in packet must be
	 * sr, rr or bye & version number must be correct.
	 */
	switch(ntohs(rh->rh_flags) & 0xc0ff) {
	case RTP_VERSION << 14 | RTCP_PT_SR:
	case RTP_VERSION << 14 | RTCP_PT_RR:
	case RTP_VERSION << 14 | RTCP_PT_BYE:
		break;
	default:
		/*
		 * XXX should further categorize this error -- it is
		 * likely that people mis-implement applications that
		 * don't put something other than SR,RR,BYE first.
		 */
		++badversion_;
		return;
	}
	/*
	 * at this point we think the packet's valid.  Update our average
	 * size estimator.  Also, there's valid ssrc so charge errors to it
	 */
	rtcp_avg_size_ += RTCP_SIZE_GAIN * (double(cc + 28) - rtcp_avg_size_);
	Address & addr = *srcp;

	/*
	 * First record in compount packet must be the ssrc of the
	 * sender of the packet.  Pull it out here so we can use
	 * it in the sdes parsing, since the sdes record doesn't
	 * contain the ssrc of the sender (in the case of mixers).
	 */
	u_int32_t ssrc = rh->rh_ssrc;
	Source* ps = SourceManager::instance().lookup(ssrc, ssrc, addr);
	if (ps == 0)
		return;
	
	int layer = ch - ch_;
		/*
		* Outer loop parses multiple RTCP records of a "compound packet".
		* There is no framing between records.  Boundaries are implicit
		* and the overall length comes from UDP.
	*/
	u_char* epack = (u_char*)rh + cc;
	while ((u_char*)rh < epack) {
		u_int len = (ntohs(rh->rh_len) << 2) + 4;
		u_char* ep = (u_char*)rh + len;
		if (ep > epack) {
			ps->badsesslen(1);
			return;
		}
		u_int flags = ntohs(rh->rh_flags);
		if (flags >> 14 != RTP_VERSION) {
			ps->badsessver(1);
			return;
		}
		switch (flags & 0xff) {

		case RTCP_PT_SR:
			parse_sr(rh, flags, ep, ps, addr, layer);
			break;

		case RTCP_PT_RR:
			parse_rr(rh, flags, ep, ps, addr, layer);
			break;

		case RTCP_PT_SDES:
			parse_sdes(rh, flags, ep, ps, addr, ssrc, layer);
			break;

		case RTCP_PT_BYE:
			parse_bye(rh, flags, ep, ps);
			break;

		default:
			ps->badsessopt(1);
			break;
		}
		rh = (rtcphdr*)ep;
	}
}

