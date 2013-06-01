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

#include "session-rtpv1.h"
#include "source.h"
#include "ntp-time.h"

/*
 * parse rtp v1 options (actually, nv options).
 */
int RTPv1SessionManager::parseopts(const u_char* bp,
				   int cc, u_int32_t addr) const
{
	register int t = 0;
	register u_int optflags;
	do {
		optflags = bp[0];
		register u_int optlen = bp[1] << 2;
		t += optlen;
		if (t > cc || optlen == 0)
			/* bad optlen */
			return (-1);
		switch (optflags & 0x7f) {
		case 33:
			if (optlen > 8) {
				/* extract names from nv's rtp v1 SDES opt */
				char buf[256];
				u_int len = optlen - 9;
				if (len > 255)
					len = 255;
				SourceManager& sm = SourceManager::instance();
				Source* s = sm.lookup(addr, addr, addr);
				if (s) {
					strncpy(buf, (char*)bp + 8, len);
					buf[len] = 0;
					if (s->sdes(RTCP_SDES_CNAME) == 0)
						s->sdes(RTCP_SDES_CNAME,
							intoa(addr));
					s->sdes(RTCP_SDES_NAME, buf);
					timeval tv = unixtime();
					s->lts_ctrl(tv);
				}
			}
			break;

		case 35:	/* bye option */
			if (optlen == 4) {
				Source* s = SourceManager::instance().consult(addr);
				if (s)
					s->lts_done(unixtime());
			}
			break;
		}
		bp += optlen;
	} while ((optflags & 0x80) == 0);
	return (t);
}

/*
 * Called for sessions that implement RTPv1 (i.e., nv and ivs)
 */
void RTPv1SessionManager::recv(DataHandler* dh)
{
	rtphdr hdr;
	u_int32_t addr;
	/* leave room in case we need to expand rtpv1 into an rtpv2 header */
	u_char* bp = &pktbuf_[4];
	int cc = dh->recv(bp, 2 * RTP_MTU - 4, addr);
	if (cc <= 0)
		return;

	rtphdr* rh = (rtphdr*)bp;
	int version = *(u_char*)rh >> 6;
	if (version != 1) {
		++badversion_;
		return;
	}
	/* RTP v1 hdr */
	rtp1hdr* rh1 = (rtp1hdr*)rh;
	int flags = rh1->rh_flags;
	flags = (flags & 0x40) << 1 | flags & 0x3f;
	if ((flags & 0x3f) == RTP_PT_H261) {
		/*
		 * smash RTPv1/H.261 payload type to undefined
		 * value so we can distinguish it from RTPv2/H.261
		 */
		flags = (flags &~ 0x3f) | RTP_PT_H261_COMPAT;
	}
	
	hdr.rh_flags = htons(flags & 0xf0ff);
	hdr.rh_seqno = rh1->rh_seqno;
	hdr.rh_ts = rh1->rh_ts;
	hdr.rh_ssrc = addr;
	bp += sizeof(*rh1);
	cc -= sizeof(*rh1);
	if (rh1->rh_flags & RTP1_OPTION_BIT) {
		int t = parseopts(bp, cc, addr);
		if (t <= 0) {
			++badoptions_;
			return;
		}
		bp += t;
		cc -= t;
		if (cc == 0)
			return;
	}
	/* make sure rtp and profile headers are contiguus */
	rh = (rtphdr*)(bp - sizeof(*rh));
	*rh = hdr;
	demux(rh, bp, cc, addr);
}

void RTPv1SessionManager::transmit(pktbuf* pb)
{
	mh_.msg_iov = pb->iov;
	Network* net = dh_.net();
	/*
	 * Smash the packet for RTPv1 compat.
	 * (put the contents back because it gets looped back above)
	 */
	rtphdr rhs = *(rtphdr*)pb->iov[0].iov_base;
	/* RTP v1 hdr */
	struct rtp1hdr {
		u_char rh_chan;		/* version/channel */
		u_char rh_flags;	/* O/S/content */
		u_int16_t rh_seqno;	/* sequence number */
		u_int32_t rh_ts;
	}* rh1;
	pb->iov[0].iov_base = (caddr_t)pb->iov[0].iov_base + 4;
	rh1 = (rtp1hdr*)pb->iov[0].iov_base;
	pb->iov[0].iov_len -= 4;
	rh1->rh_chan = 0x60; // vers 1, chan 32
	int flags = ((u_char*)&rhs)[1];
	/*XXX lose a bit in PT -> content type */
	flags = ((flags & 0x80) >> 1) | (flags & 0x3f);
	rh1->rh_flags = flags;
	rh1->rh_seqno = rhs.rh_seqno;
	rh1->rh_ts = rhs.rh_ts;
	net->send(mh_);
	pb->iov[0].iov_len += 4;
	pb->iov[0].iov_base = (caddr_t)pb->iov[0].iov_base - 4;
	*(rtphdr*)pb->iov[0].iov_base = rhs;
}
