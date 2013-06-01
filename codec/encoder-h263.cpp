/*
 * Copyright (c) 1998-1999 Marcus Meissner and
 *     The Regents of the University of Erlangen.
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

/* History:
 *
 * This encoder was written for a DFN project (http://www.dfn.de/) at the
 * University of Erlangen-Nuremberg, Lehrstuhl fuer Nachrichtentechnik by
 * Marcus Meissner in 1998.
 *
 * Merged into UCL vic development tree in 1998
 *
 * Cleanups, endianess, translation of comments into english, bugfixes,
 * done by Marcus Meissner in June 1999.
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header$ (LBL)";
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "defs.h"
#include "bsd-endian.h"
#include "transmitter.h"
#include "pktbuf-rtp.h"
#include "module.h"
#include "crdef.h"

#include "h263/bitOut.h"
#include "h263/h263.h"
#include "encoder-h263.h"

extern "C" {
#include "h263/h263encoder.h"
#include "h263/h263encoder.p"

	int split_h263stream(
		u_char *bs,			/* bitstream [in] */
		u_int bitcount,			/* bitstream length in bits [in] */
		u_int gobs_per_frame,		/* gobs per frame [in] */
		u_int mbs_per_gob,		/* mbs per gob [in] */
		int *mbind,			/* macroblock indices [in] */
		int *mbquant,			/* quantifiers [in] */
		MVField mvfield,		/* motion vectors [in] */
		u_int ts_,			/* timestamp [in] */
		int headersize,			/* headerlength [in] */
		h263_rtp_packet	**packets	/* rtp packets [out] */
		);
}

H263Encoder::H263Encoder() : TransmitterModule(FT_YUV_CIF)
{
	/* first frame must be coded INTRA */
	nexttype_ = PICTURE_CODING_TYPE_INTRA;
	
	lastw_ = 0;lasth_ = 0;
	oldpic_.w = 0;
	oldpic_.h = 0;
	oldpic_.y = new Byte[3];
	oldpic_.u = oldpic_.y+1;
	oldpic_.v = oldpic_.u+1;
	oldorigpic_.w = 0;
	oldorigpic_.h = 0;
	oldorigpic_.y = new Byte[3];
	oldorigpic_.u = oldorigpic_.y+1;
	oldorigpic_.v = oldorigpic_.u+1;
	
	mbind_=(int*)malloc(1);
	mbquant_=(int*)malloc(1);
	
	mvfield_.mx = (short*)malloc(1);
	mvfield_.my = (short*)malloc(1);
	mvfield_.mode = (short*)malloc(1);
	mvfield_.w = 1;
	mvfield_.h = 1;
	
	decpict_.w = 0; decpict_.h = 0;
	decpict_.y = new Byte[3];
	decpict_.u = decpict_.y+1;
	decpict_.v = decpict_.u+1;
	
	/* global maximum rate for the encoder.
	* The encoder itself does not have a fixed bitrate delimiter, so
	* we just allocate a very big buffer and hope that the encoder does
	* not overflow it.
	*/
	maxrate_ = 100000;
	bitstr_.b = new Byte[maxrate_*10/8];
	bitstr_.size = maxrate_*10;
	bitstr_.ind = 0;
	bitstr_.actualSize = 0;
	bitstr_.fp = NULL;
	
	/* We use only codingtime 3, since it is the best time/quality/space
	* tradeoff currently. 
	*/
	codingtime_ = 3;
	
	maycheck_ = 0;
	
	/* default Quantisierer is 10 */
	q_ = 10;
}

H263Encoder::~H263Encoder()
{
/* Bus Errors... Caused by destructors called out of sequence.
if (tx_ != 0)
tx_->flush();
	*/
}

int
H263Encoder::command(int argc,const char* const *argv)
{
	if (argc==3) {
		if (!strcasecmp(argv[1],"q")) {
			q_ = atoi(argv[2]);
			return 0;
		}
	}
	return TransmitterModule::command(argc,argv);
}

/***********************************************************************
* H263Encoder::consume()
*
* Maininterface function to VIC is called for every grabbed frame.
* Encodes the frames into h.263, paketises them according to RFC 2190 and
* sends the RTP packets using the RTP networklayer of VIC.
*
* Returns number of sent bytes.
*/
int H263Encoder::consume(const VideoFrame* vf)
{
	YuvFrame*	yuv = (YuvFrame*)vf;	/* real YUV Frame */
	Picture		thispic;		/* current image */
	u_int		xfps,kbps;		/* parameter from Tcl */
	u_int		i;
	u_int		sentbytes=0;		/* sent bytes in this consume */
	u_int		bps;			/* target bitrate */
	int			k, mbs_per_gob, gobs_per_frame;
	int			w, h;
	Tcl&		tcl = Tcl::instance();
	pktbuf*		pb;
	rtphdr*		rh;

#ifdef TIME_ADAPTION
	struct timeval tv1,tv2;
	int	timespent;
#endif //SV-XXX: don't need "#endif TIME_ADAPTION" in this case, just #endif is enough

	//moved up here by Piers
	tx_->flush();
	
	w = yuv->width_;
	h = yuv->height_;
	
	
	k = 1;
	if (w == CIF4_WIDTH)         k = 2;
	if (w == 2*CIF4_WIDTH)       k = 4;
	
	/* FIXME: incorrect for CIF*2 and up! */
	mbs_per_gob = w/16/k;
	gobs_per_frame = h/16/k;
	
	/* did we the size of the grabbed frame change? */
	if (w!=lastw_ || h!=lasth_) {
		/* readjust structzures, temp. arrays, et.al. */
		delete[] oldpic_.y;
		delete[] oldorigpic_.y;
		delete[] decpict_.y;
		
		free(mvfield_.mx);
		free(mvfield_.my);
		free(mvfield_.mode);
		
		oldpic_.w = w;oldpic_.h = h;
		oldpic_.y = new Byte[w*h*3/2];
		oldpic_.u = oldpic_.y+w*h;
		oldpic_.v = oldpic_.u+w*h/4;
		
		oldorigpic_.w = w;oldpic_.h = h;
		oldorigpic_.y = new Byte[w*h*3/2];
		oldorigpic_.u = oldorigpic_.y+w*h;
		oldorigpic_.v = oldorigpic_.u+w*h/4;
		
		decpict_.w = w;decpict_.h = h;
		decpict_.y = new Byte[w*h*3/2];
		decpict_.u = decpict_.y+w*h;
		decpict_.v = decpict_.u+w*h/4;
		
		mvfield_.mx = (short*)malloc(mbs_per_gob*gobs_per_frame*sizeof(short));
		mvfield_.my = (short*)malloc(mbs_per_gob*gobs_per_frame*sizeof(short));
		mvfield_.mode = (short*)malloc(mbs_per_gob*gobs_per_frame*sizeof(short));
		mvfield_.w = mbs_per_gob;
		mvfield_.h = gobs_per_frame;
		nexttype_ = PICTURE_CODING_TYPE_INTRA;
		lastw_ = w;
		lasth_ = h;
		free(mbind_);free(mbquant_);
		mbind_=(int*)malloc((mbs_per_gob*gobs_per_frame+1)*sizeof(int));
		mbquant_=(int*)malloc(mbs_per_gob*gobs_per_frame*sizeof(int));
		
		
	}
	
	/* HACK: We query the Tcl layers for the specified bps and fps values.
	* Hack due to the fact, that no other encoder uses this values in this
	* way and we could get feedback loops from the network code (who uses
	* them). Bummer.
	*/
	tcl.evalc("$fps_slider get");sscanf(tcl.result(),"%d",&xfps);
	tcl.evalc("$bps_slider get");sscanf(tcl.result(),"%d",&kbps);
	
	/* calculate bits/frame */
	bps = kbps*1024/xfps;
	
	/* temporary picture from passed YUV Frame */
	thispic.w = w;
	thispic.h = h;
	thispic.y = (Byte*)yuv->bp_;
	thispic.u = (Byte*)yuv->bp_+w*h;
	thispic.v = (Byte*)yuv->bp_+w*h+(w*h)/4;
	
	/* Bitstream starts at 0 */
	bitstr_.ind = 0;
	
	/* and GO */
	bitstr_.b[bitstr_.size/8-1] = 0x42;

#ifdef TIME_ADAPTION
	gettimeofday(&tv1,NULL);
#endif //SV-XXX: don't need "#endif TIME_ADAPTION" in this case, just #endif is enough

	EncodeH263Q(
		q_,					/* [in] Quantisierer */
		codingtime_,		/* [in] Codingtime */
		nexttype_,			/* [in] codingtype */
		vf->ts_,			/* [in] Timestamp */
		5,				    /* [in] percent intra blocks */
		&thispic,			/* [in] current image4 */
		&oldorigpic_,
		&oldpic_,			/* [in] prev. coded image */
		(unsigned char*)yuv->crvec_,	/* [in] condit. replenishment */
		&decpict_,			/* [out] new coded image */
		&bitstr_,			/* [out] bitstream */
		mbind_,				/* [out] MB indices */
		mbquant_,			/* [out] MacroBlock Quant. */
		&mvfield_			/* [out] MotionVector field */
		);
	
	{
		Picture tmppict;
		tmppict = oldpic_; oldpic_ = decpict_; decpict_ = tmppict;
	}
	memcpy(oldorigpic_.y,thispic.y,w*h*3/2);
	
	assert(bitstr_.ind < bitstr_.size);
	assert(bitstr_.b[bitstr_.size/8-1]== 0x42);
	
	/* mark the end of the bitstream */
	mbind_[mbs_per_gob*gobs_per_frame]=bitstr_.ind;
	
	/* The actual splitfunction */
	h263_rtp_packet	*packets = NULL;
	int nrofpackets = split_h263stream(
		bitstr_.b,			/* [in] bitstream */
		bitstr_.ind,		/* [in] length in bits*/
		gobs_per_frame,
		mbs_per_gob,
		mbind_,
		mbquant_,
		mvfield_,
		yuv->ts_,			/* [in] timestamp */
		tx_->mtu(),			/* [in] current MTU */
		&packets			/* [out] packets... */
		);
		/* And now we send the packets.
		* We use the 2 standard iov buffers, buffer 1 just for the RTP header,
		* buffer 2 for the H263 payloadheader and the h.263 bits.
		* Reason for this is that the VIC decoder structure does not like 
		* like differently sized headers and acts differently between loopback
		* and network-in decoding.
	*/
	for (i=0;i<nrofpackets;i++) {
		//pb = tx_->alloc(yuv->ts_,RTP_PT_H263);
		pb = pool_->alloc(yuv->ts_,RTP_PT_H263);
		//rh = (rtphdr*)pb->iov[0].iov_base;
		rh = (rtphdr*)pb->data;
		
		//pb->iov[0].iov_len = sizeof(rtphdr);
		
		//memcpy(pb->iov[1].iov_base,packets[i].header,packets[i].headersize);
		memcpy(&pb->data[sizeof(rtphdr)],packets[i].header,packets[i].headersize);
		//memcpy((char*)pb->iov[1].iov_base+packets[i].headersize,packets[i].data,packets[i].datasize);
		memcpy(&pb->data[sizeof(rtphdr)]+packets[i].headersize,packets[i].data,packets[i].datasize);
		//pb->iov[1].iov_len = packets[i].datasize+packets[i].headersize;
		pb->len = sizeof(rtphdr) + packets[i].datasize+packets[i].headersize;
		
		/* mark the last packet */
		if (i==nrofpackets-1)
			rh->rh_flags |= htons(RTP_M);
		/* sending ...  */
		tx_->send(pb);
		sentbytes+=packets[i].headersize+packets[i].datasize+sizeof(rtphdr);
	}
	/* all other frames are coded with percentual INTRA part */
	nexttype_ = PICTURE_CODING_TYPE_INTER;
	/* flush the buffered RTP packets, so we can free the packets.
	* Bad style probably */
	//I think yes. Piers
	//tx_->flush();
	free(packets);

#ifdef TIME_ADAPTION
	
	/* and TIME! */
	gettimeofday(&tv2,NULL);
	
	timespent = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
	if ((++maycheck_)>20) {
		maycheck_ = 0;
		/* Did we use too much time? */
		if ((1000000/xfps*3)<timespent) {
			/* Yes, first decrease imagesize */
			if (w==CIF_WIDTH) {
				tcl.evalc("grabber decimate 4");
				tcl.evalc("set inputSize 4");
				fprintf(stderr,"TOO SLOW: shrunk to QCIF\n");
				return 0;
			} else {
				/* no more ways to do speed it up YET */
			}
		}
		/* Did we use much less time as expected? */
		if ((1000000/xfps)>timespent*3) {
			if (w==QCIF_WIDTH) {
				tx_->flush();
				tcl.evalc("grabber decimate 2");
				tcl.evalc("set inputSize 2");
				fprintf(stderr,"TOO FAST: grown to CIF\n");
				return 0;
			}
		}
	}
#endif
	return sentbytes;
}
