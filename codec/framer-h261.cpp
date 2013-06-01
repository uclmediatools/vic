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

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include "net.h"
#include "framer-h261.h"
#include <netinet/in.h>

H261Framer::H261Framer() : Framer("h261")
{
	format_ = RTP_PT_H261;
}

H261Framer::~H261Framer()
{
}

#ifdef notyet
void H261Framer::setparams(int q, int fw, int fh)
{
	q_ = htons(q);
	nw_ = htons(fw);
	nh_ = htons(fh);
}
#endif

int H261Framer::send(u_char* bp, int n, u_int32_t ts)
{
	pktbuf* pb = alloch(ts);
	rtphdr* rh = (rtphdr*)pb->hdr;
	u_short* h261 = (u_short*)(rh + 1);

	/*XXX FIX THIS! this will only work with loopback because
	 packets will be bigger than VIC_MTU (and will be frag'd) */
	*h261 = 0x8881;
	rh->rh_flags |= htons(RTP_M);
	pb->iov[0].iov_len = sizeof(*rh) + 2;
	pb->iov[1].iov_base = (caddr_t)bp;
	pb->iov[1].iov_len = n;
	/*XXX DEC C++ needs scope qualifier here */
	Framer::send(pb);

	return (n + sizeof(*rh) + 2);
}
