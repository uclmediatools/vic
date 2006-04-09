/*
* Copyright (c) 1994-1995 The Regents of the University of California.
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
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
*      This product includes software developed by the University of
*      California, Berkeley and the Network Research Group at
*      Lawrence Berkeley Laboratory.
* 4. Neither the name of the University nor of the Laboratory may be used
*    to endorse or promote products derived from this software without
*    specific prior written permission.
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
*/
static const char rcsid[] =
"@(#) $Header$ (LBL)";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "inet.h"
#include "net.h"
#include "rtp.h"
#include "dct.h"
#include "bsd-endian.h"
#include "vic_tcl.h"
#include "crdef.h"
#include "transmitter.h"
#include "pktbuf-rtp.h"
#include "module.h"

#include "h263coder.h"

#define HDRSIZE (sizeof(rtphdr) + 4)
#define	CIF_WIDTH	352
#define	CIF_HEIGHT	288
#define	QCIF_WIDTH	176
#define	QCIF_HEIGHT	144

/*#ifndef WIN32
extern "C" int QP,QPI;
extern "C" unsigned char *h263_frame;          // * encoder input */
//extern "C" int            h263_streamcount;    // * encoder output */
//extern "C" char          *h263_bitstream;
//#else
extern "C" int QP,QPI;
extern "C" unsigned char *h263_frame;          /* encoder input */
extern "C" int            h263_streamcount;    /* encoder output */
extern "C" char          *h263_bitstream;
//#endif

class H263plusEncoder : public TransmitterModule {
public:
    void setq(int q);
	
    H263plusEncoder();
    ~H263plusEncoder();
    int command(int argc, const char*const* argv);
    
    void size(int w, int h);
    int  consume(const VideoFrame*);
	
protected:
    struct ENCODER_STATE *state;
    int    i, qp, start_ts, last_iframe;
};

static class H263plusEncoderMatcher : public Matcher {
public:
	H263plusEncoderMatcher() : Matcher("module") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "h263+") == 0)
			return (new H263plusEncoder);
		return (0);
	}
} encoder_matcher_h263plus;


H263plusEncoder::H263plusEncoder() : TransmitterModule(FT_YUV_CIF)
{
    /*fprintf(stderr,"send-h263: constructor\n");*/
    state = NULL;
    start_ts = 0;
    qp = 10;
}

H263plusEncoder::~H263plusEncoder()
{
    /*fprintf(stderr,"send-h263: destructor\n");*/
	// Added by Piers in case codec hasn't been initialised
	if (state)
		h263_cleanup(state);
    free(h263_bitstream);
}

void
H263plusEncoder::size(int w, int h)
{
    /*fprintf(stderr,"send-h263: new frame size: %dx%d\n",w,h);*/
    Module::size(w, h);
	
    sed_pels = w;
    sed_lines = h;
    base_sed_pels = w;
    base_sed_lines = h;
    if (w == CIF_WIDTH && h == CIF_HEIGHT) {
		state->pic->source_format = 3;
    } else if (w == QCIF_WIDTH && h == QCIF_HEIGHT) {
		state->pic->source_format = 2;
    } else {
		/*fprintf(stderr,"send-h263: unsupported format\n");*/
		exit(1);
    }
}

int
H263plusEncoder::command(int argc, const char*const* argv)
{
    /*fprintf(stderr,"send-h263: command: %s\n",argv[1]);*/
	
    if (argc == 3 && strcmp(argv[1], "q") == 0) {
		qp = QP = QPI = atoi(argv[2]);
		return (TCL_OK);
    }
    return (TransmitterModule::command(argc, argv));
}

void
dump_paket(const unsigned char *data, int len)
{
#ifndef DEBUG263
	UNUSED(data); //SV-XXX: unused if DEBUG263 not defined
	UNUSED(len); //SV-XXX: unused if DEBUG263 not defined
#else
	int x,y;
	
    for (y = 0; y < len; y += 16) {
		fprintf(stderr,"\t");
		for (x = y; x < y+16; x++) {
			if (x < len)
				fprintf(stderr,"%02x ",data[x]);
			else
				fprintf(stderr,"   ");
		}
		fprintf(stderr," ");
		for (x = y; x < y+16; x++) {
			if (x < len)
				fprintf(stderr,"%c", ((data[x] & 0x7f) < 32) ? '.' : data[x]);
			else
				fprintf(stderr," ");
		}
		fprintf(stderr,"\n");
    }
#endif
}

int H263plusEncoder::consume(const VideoFrame *vf)
{
    pktbuf* pb;
    rtphdr* rh;
    int n,ps,send_psize = tx_->mtu() - 14;      /* 12 RTP + 2 Payload */
	
	//moved up here by Piers
    tx_->flush();

    if (!samesize(vf) && state) {
		h263_cleanup(state);
		free(h263_bitstream);
		state = NULL;
    }
	
    if (!state) {
		if (!start_ts)
			start_ts = vf->ts_;
		h263_bitstream = (char*)malloc(65536);
		state = h263_init_encoder_1();
		size(vf->width_, vf->height_);
		/* set other encoding options here */
		QP = QPI = qp;
		h263_init_encoder_2(state);
		last_iframe = vf->ts_;
		i = 0;
    }
    h263_frame = vf->bp_;
    h263_streamcount = 0;
	
    if (vf->ts_ - last_iframe > 10 * 90000) {
		/* one I-Frame every 10 sec */
		last_iframe = vf->ts_;
		i = 0;
    }
    /* XXX: ts wraparound */
    h263_encode_one_frame(state,i++,((vf->ts_ - start_ts)/3000) & 0xff);
	
    for (n = 2; n < h263_streamcount; n += ps) {
		//pb = tx_->alloc(vf->ts_, RTP_PT_H263P);
		pb = pool_->alloc(vf->ts_, RTP_PT_H263P);
		//rh = (rtphdr*)pb->iov[0].iov_base;
		rh = (rtphdr*)pb->data;
		
		*(u_int*)(rh + 1) = 0;
		if (2 == n)
			*(u_int*)(rh + 1) |= htonl(0x04000000); /* set P bit */
		
		ps = (h263_streamcount) - n;
		if (ps > send_psize) {
			if (ps > send_psize + 64)
				ps = send_psize;
			else
				ps = send_psize-64;
		} else
			rh->rh_flags |= htons(RTP_M);
		
		//memcpy(pb->iov[1].iov_base, h263_bitstream+n, ps);
		memcpy(&pb->data[14], h263_bitstream+n, ps);
#ifdef DEBUG263
		//dump_paket((const unsigned char *)pb->iov[0].iov_base,16);
		//dump_paket((const unsigned char *)pb->iov[1].iov_base,16);
#endif
		//I think 14 is wrong.(should be 12) Piers
		//pb->iov[0].iov_len = 14;
		//pb->iov[1].iov_len = ps;
		i=sizeof(rtphdr);
		pb->len=ps+14;
		tx_->send(pb);
    }
    //tx_->flush();
	
    return n;
}
