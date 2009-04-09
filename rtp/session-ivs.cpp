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

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <math.h>
#include "session-rtpv1.h"
#include "source.h"
#include "rtp.h"
#include "ntp-time.h"

/*
 * A class for backward compat with the old ivs variant
 * of RTPv1 (ivs-3.x and earlier).
 */
class  IvsSessionManager : public RTPv1SessionManager {
 public:
	virtual void recv(CtrlHandler*);
	virtual void send_bye();
	virtual void send_report();
 protected:
};

static class IvsSessionMatcher : public Matcher {
public:
	IvsSessionMatcher() : Matcher("session") {}
	TclObject* match(const char* id) {
		if (strcmp(id, "video/ivs") == 0)
			return (new IvsSessionManager);
		else
			return (0);
	}
} ivs_matcher;

void IvsSessionManager::recv(CtrlHandler* ch)
{
	/*
	 * handle in incoming ivs session packet
	 */
	u_int32_t src;
	int cc = ch->recv(pktbuf_, 2 * RTP_MTU, src);
	if (cc <= 0)
		return;

	rtp1hdr* rh = (rtp1hdr*)pktbuf_;
	if (cc < sizeof(*rh)) {
		++nrunt_;
		return;
	}
	if ((rh->rh_chan & 0xc0) != (1 << 6) ||
	    (rh->rh_flags & RTP1_OPTION_BIT) == 0) {
		++badversion_;
		return;
	}
	u_int optflags;
	u_char* bp = (u_char*)(rh + 1);
	u_char* ep = bp + cc - sizeof(*rh);;
	u_int32_t addr = (u_int32_t)src;
	do {
		optflags = bp[0];
		u_int optlen = bp[1] << 2;
		if (bp + optlen > ep || optlen == 0) {
			/* empty packet or bad optlen */
			++badoptions_;
			return;
		}
		switch (optflags & 0x7f) {
			Source* s;
			u_int len;
			char buf[256 + 32];

		case 34:	/* ivs's rtp v1 SDES opt */
			len = bp[17] << 2;
			if (optlen < 20 || len > optlen - 16 || len < 4) {
				++badoptions_;
				return;
			}
			len -= 2;
			s = SourceManager::instance().lookup(addr, addr, addr);
			if (s == 0)
				return;
			strncpy(buf, (char*)bp + 18, len);
			buf[len] = 0;
			if (bp[16] != 16) {
				len = strlen(buf);
				buf[len] = '@';
				strcpy(buf + len + 1, intoa(addr));
				s->sdes(RTCP_SDES_CNAME, buf);
			} else {
				if (s->sdes(RTCP_SDES_CNAME) == 0)
					s->sdes(RTCP_SDES_CNAME, intoa(addr));
				s->sdes(RTCP_SDES_NAME, buf);
			}
			s->lts_ctrl(unixtime());
			break;

		case 35:	/* bye option */
			if (optlen != 4) {
				++badoptions_;
				return;
			}
			s = SourceManager::instance().consult(addr);
			if (s)
				s->lts_done(unixtime());
		}
		bp += optlen;
	} while ((optflags & 0x80) == 0);
}

/*
 * Send an IVS report packet.
 */
void IvsSessionManager::send_report()
{
	SourceManager& sm = SourceManager::instance();
	Source& s = *sm.localsrc();
	s.lts_ctrl(unixtime());
	rtp1hdr* rh = (rtp1hdr*)pktbuf_;
	rh->rh_chan = 1 << 6;
	rh->rh_flags = RTP1_OPTION_BIT | RTP_PT_H261;
	rh->rh_seqno = 0;
	rh->rh_ts = 0;
	u_char* opt = (u_char*)(rh + 1);

	opt[0] = 127;	/*APP option*/
	opt[1] = 3;	/*opt len*/
	opt[2] = 0;
	opt[3] = 0;
	opt[4] = 'I';
	opt[5] = 'V';
	opt[6] = 'S';
	opt[7] = ' ';
	opt[8] = 0x33;	/*ivs version*/
	opt[9] = (s.lts_data().tv_sec &&
		  int(s.lts_data().tv_sec - s.lts_ctrl().tv_sec) >= 0)?
			(1 << 6) : 0;
	opt[10] = 0;
	opt[11] = 0;
	opt += 12;

	opt[0] = 34 | 0x80;	/*SDES option + Final*/
	const char* name = s.sdes(RTCP_SDES_NAME);
	int len = strlen(name);
	int optlen = (len + (19 + 3)) >> 2;
	u_char* ep = opt + (optlen << 2);
	opt[1] = optlen;
	opt[2] = 0;		/*id*/
	opt[3] = 0;
	opt[4] = 2;		/*port*/
	opt[5] = 1;		/*port len*/
	Network* net = ch_.net();
	opt[6] = ntohs(net->port()) >> 8;
	opt[7] = ntohs(net->port());
	opt[8] = 1;		/*addr*/
	opt[9] = 2;		/*addr len*/
	opt[10] = 0;
	opt[11] = 1;		/*addr type = ipv4*/
	*(u_int*)&opt[12] = net->interface();
	opt[16] = 16;		/*class user text*/
	opt[17] = optlen - 4;
	opt += 18;
	strcpy((char*)opt, name);
	opt += len;
	while (opt < ep)
		*opt++ = 0;

	len = ep - pktbuf_;
	net->send(pktbuf_, len);

	/* schedule a timer for the next report */
	double rint = rtcp_inv_bw_ * 64.;
	if (rint < RTCP_MIN_RPT_TIME * 1000.)
		rint = RTCP_MIN_RPT_TIME * 1000.;
	rt_.msched(int(fmod(double(random()), rint) + rint * .5 + .5));
}

void IvsSessionManager::send_bye()
{
	rtp1hdr* rh = (rtp1hdr*)pktbuf_;
	rh->rh_chan = 1 << 6;
	rh->rh_flags = RTP1_OPTION_BIT | RTP_PT_H261;
	rh->rh_seqno = 0;
	rh->rh_ts = 0;
	u_char* opt = (u_char*)(rh + 1);
	opt[0] = 35 | 0x80;	/*BYE option + Final*/
	opt[1] = 1;
	opt[2] = 0;		/*id*/
	opt[3] = 0;
	ch_.send(pktbuf_, sizeof(*rh) + 4);
}

