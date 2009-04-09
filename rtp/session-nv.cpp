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
 * A class for backward compat with the old nv variant
 * of RTPv1 (nv-3.x and earlier).
 */
class NvSessionManager : public RTPv1SessionManager {
 public:
	virtual void send_bye();
	virtual void send_report();
};

static class NvSessionMatcher : public Matcher {
public:
	NvSessionMatcher() : Matcher("session") {}
	TclObject* match(const char* id) {
		if (strcmp(id, "video/nv") == 0)
			return (new NvSessionManager);
		else
			return (0);
	}
} nv_session_matcher;

/*
 * Send an NV report packet.
 */
void NvSessionManager::send_report()
{
	/* only send name if we're sending video */
	Source& s = *SourceManager::instance().localsrc();
	s.lts_ctrl(unixtime());
	if (s.np() != last_np_) {
		last_np_ = s.np();
		rtp1hdr* rh = (rtp1hdr*)pktbuf_;
		rh->rh_chan = 0x60; /* v1, chan 32 */
		rh->rh_flags = RTP1_OPTION_BIT | 28;
		rh->rh_seqno = s.cs();
		rh->rh_ts = htonl(ntohl(s.sts_data()) - 1);
		u_char* opt = (u_char*)(rh + 1);

		opt[0] = 33 | 0x80;	/* nv SDESC option + Final*/
		const char* name = s.sdes(RTCP_SDES_NAME);
		int len = strlen(name);
		int optlen = ((len + 8) >> 2) + 1;
		u_char* ep = opt + (optlen << 2);
		opt[1] = optlen;
		opt[2] = 0;		/*id*/
		opt[3] = 0;
		Network* net =  dh_.net();
		*(u_int*)&opt[4] = net->interface();
		opt += 8;
		strcpy((char*)opt, name);
		opt += len;
		while (opt < ep)
			*opt++ = 0;

		len = ep - pktbuf_;
		net->send(pktbuf_, len);
	}

	/* schedule a timer for the next report */
	double rint = rtcp_inv_bw_ * 64.;
	if (rint < RTCP_MIN_RPT_TIME * 1000.)
		rint = RTCP_MIN_RPT_TIME * 1000.;
	rt_.msched(int(fmod(double(random()), rint) + rint * .5 + .5));
}

void NvSessionManager::send_bye()
{
	/* only send bye if we've sent video */
	Source& s = *SourceManager::instance().localsrc();
	if (s.ns() != 0) {
		rtp1hdr* rh = (rtp1hdr*)pktbuf_;
		rh->rh_chan = 0x60; /* v1, chan 32 */
		rh->rh_flags = RTP1_OPTION_BIT | 28;
		rh->rh_seqno = s.cs();
		rh->rh_ts = htonl(ntohl(s.sts_data()) - 1);
		u_char* opt = (u_char*)(rh + 1);
		opt[0] = 35 | 0x80;	/*BYE option + Final*/
		opt[1] = 1;
		opt[2] = 0;		/*id*/
		opt[3] = 0;
		dh_.send(pktbuf_, sizeof(*rh) + 4);
	}
}

