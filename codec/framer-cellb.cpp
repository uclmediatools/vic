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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
//#include <winsock.h>
#else
#include <sys/param.h>
#include <sys/file.h>
#endif
#include <sys/stat.h>

#include "bsd-endian.h"
#include "transmitter.h"
#include "module.h"

#define HLEN	(sizeof(rtphdr)+sizeof(cellbhdr))

class CellBFramer : public TransmitterModule {
 public:
	CellBFramer();
	~CellBFramer();
	virtual int consume(const VideoFrame*);
//	virtual int command(int argc,const char *const *argv);
 protected:
	void send(pktbuf* pb, int sync,int x, int y, int cc);
 	void size(int w,int h);
	char	transmitter_;	// tclobject transmitter
	int	nw_,nh_;
};

static class CellBFramerMatcher : public Matcher {
public:
        CellBFramerMatcher() : Matcher("module") {}
        TclObject* match(const char* fmt) {
                if (strcasecmp(fmt, "cellb/hw") == 0)
                        return (new CellBFramer);
                return (0);
        }
} framer_matcher_cellb;
 
CellBFramer::CellBFramer() : TransmitterModule(FT_CELLB) {
	nw_ = 0;
	nh_ = 0;
	width_ = 0;
	height_ = 0;
}
 
CellBFramer::~CellBFramer() {
	if (tx_ != 0)
		tx_->flush();
}
void CellBFramer::size(int w, int h)
{
	Module::size(htons(w), htons(h));
	nw_ = htons(w);
	nh_ = htons(h);
}

void CellBFramer::send(pktbuf* pb, int sync,
                        int x, int y, int cc)
{
	//pb->iov[0].iov_len = HLEN;
	//pb->iov[1].iov_len = cc;
	pb->len = cc+HLEN;
	//rtphdr* rh = (rtphdr*)pb->hdr;
	rtphdr* rh = (rtphdr*)pb->data;
	if (sync)
		rh->rh_flags |= htons(RTP_M);

	cellbhdr* ch = (cellbhdr*)(rh + 1);
	ch->width = nw_;
	ch->height = nh_;
	ch->x = htons(x);
	ch->y = htons(y);
	tx_->send(pb);
}

int CellBFramer::consume(const VideoFrame* vf)
{
	CellBFrame*	p = (CellBFrame*)vf;
	unsigned char	*lastbp,*bp = p->bp_;

	if (!samesize(vf))
		size(vf->width_, vf->height_);
	tx_->flush();
	pktbuf* pb = pool_->alloc(p->ts_, RTP_PT_CELLB);

	int	nb=0,i,thislen;
	int	cc = p->len_;
	int	cursize,wrapsize = tx_->mtu()-HLEN; // do not exceed this
	int	h,w,x,y,lastx,lasty,x0=0,y0=0;

	h = nh_ >> 2;
	w = nw_ >> 2;
	cursize=0;
	i=0;

	y=0;x=0;lastbp=bp;
	while (y<h) {
		lastx=x;lasty=y;
		if (bp[0]&0x80) {
			x+=bp[0]-128+1;
//				fprintf(stderr,"skip[%d]",bp[0]-128+1);
			while (x>=w) {
				y++;
				x-=w;
			}
			thislen=1;
		} else {
//				fprintf(stderr,"pattern[%lx]",*(long*)bp);
			thislen=4;
			if (++x==w) {
				x=0;
				y++;
			}
		}
		if (cursize>cc) {
			fprintf(stderr,"ARGH (x=%d,y=%d,w=%d,h=%d)!\n",
				x,y,w,h
			);
			break;
		}
		if (cursize+thislen>=wrapsize) {
			// time to transmit
			//memcpy(pb->iov[1].iov_base,lastbp,cursize);
			memcpy(&pb->data[HLEN],lastbp,cursize);
			lastbp=bp;
			send(pb,0,x0,y0,cursize);
			nb+=cursize+HLEN;
			cc-=cursize;
			cursize=0;
			pb = pool_->alloc(p->ts_, RTP_PT_CELLB);
			wrapsize = tx_->mtu()-HLEN;
			x0=lastx;
			y0=lasty;
		}
		bp+=thislen;
		cursize+=thislen;
	}
	//memcpy(pb->iov[1].iov_base,bp-cursize,cursize);
	memcpy(&pb->data[HLEN],bp-cursize,cursize);
	send(pb,1,x0,y0,cursize);
	nb+=cursize+HLEN;
	return nb;
}
