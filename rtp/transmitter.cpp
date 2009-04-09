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
	loopback_(0)
{
	memset((char*)&mh_, 0, sizeof(mh_));
	mh_.msg_iovlen = 2;
}

/* Return time of day in seconds */
inline double Transmitter::gettimeofday_secs() const
{
	timeval tv;
	::gettimeofday(&tv, 0);
	return (tv.tv_sec + 1e-6 * tv.tv_usec);
}

void Transmitter::loopback(pktbuf* pb)
{
	int layer = pb->layer;
	rtphdr* rh = (rtphdr*)pb->data;
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
//	int cc = pb->iov[0].iov_len + pb->iov[1].iov_len;
	int cc = pb->len;
	return (8 * cc / (1000. * kbps_));
}

void Transmitter::send(pktbuf* pb)
{
	if (!busy_) {
		double delay = txtime(pb);
		nextpkttime_ = gettimeofday_secs() + delay;
		output(pb);
		/*
		 * emulate a transmit interrupt --
		 * assume we will have more to send.
		 */
		msched(int(delay * 1e-3));
		busy_ = 1;
	} else {
		if (head_ != 0) {
			tail_->next = pb;
			tail_ = pb;
		} else
			tail_ = head_ = pb;
		pb->next = 0;
	}
}

void Transmitter::timeout()
{
	double now = gettimeofday_secs();
	for (;;) {
		pktbuf* p = head_;
		if (p != 0) {
			head_ = p->next;
			nextpkttime_ += txtime(p);
			output(p);
			int ms = int(1e-3 * (nextpkttime_ - now));
			/* make sure we will wait more than 10ms */
			if (ms > 1000) {
				msched(ms);
				return;
			}
		} else {
			busy_ = 0;
			break;
		}
	}
}

void Transmitter::flush()
{
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

void Transmitter::output(pktbuf* pb)
{
	//if (dumpfd_ >= 0)
	//	dump(dumpfd_, pb->iov, mh_.msg_iovlen);
//dprintf("layer: %d \n",pb->layer);
	transmit(pb);
	loopback(pb);
//	pb->release() is called by decoder in loopback;
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
