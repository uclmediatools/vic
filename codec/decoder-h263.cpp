/*
 * Copyright (c) 1998 Marcus Meissner and The Regents of the University of
 * Erlangen.
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

/* History:
 *
 * This decoder was written for a DFN project (http://www.dfn.de/) at the
 * University of Erlangen-Nuremberg, Lehrstuhl fuer Nachrichtentechnik by
 * Marcus Meissner in 1998.
 *
 * Merged into UCL vic development tree in 1998 by ?
 *
 * Cleanups, endianess, translation of comments into english, bugfixes,
 * done by Marcus Meissner in June 1999.
 */

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "module.h"
#include "renderer.h"

extern "C" {
#define DEFINE_GLOBALS
#include "h263/Util.h"
#include "h263/h263decoder.h"
};

#include "h263/h263.h"
#include "decoder-h263.h"

H263Decoder::H263Decoder() : Decoder(0 /* 0 byte extra header */)
{
	h263decoder = NewH263Decoder();

	nstat_ = 0;
	decimation_	= 420;
	srcformat_	= 0;
	
	h263streamsize_ = 0;
	h263stream_	= new u_char[1];
	for (int i = 0; i < H263_SLOTS; ++i) {
		memset(slot_+i,0,sizeof(slot_[i]));
		slot_[i].bp = new u_char[2000];
		memset(slot_[i].bp,0,2000);
	}
}

H263Decoder::~H263Decoder()
{
	delete[] h263stream_;
	for (int i=0;i<H263_SLOTS;i++)
		delete[] slot_[i].bp;
	if (!h263decoder->first)
		DisinitH263Decoder(h263decoder);
	FreeH263Decoder(h263decoder);
}

/* Returns informations about the current decoder back to Tcl.
 * FIXME: We currently do not return useful information. Hmm.
 */
void H263Decoder::info(char* wrk) const
{
	sprintf(wrk, "[q=42]");
}

/* Calculates the colorhistory. Used in the palette color modes. */
int H263Decoder::colorhist(u_int* hist) const
{
	if (backframe_)
		colorhist_420_556(hist,backframe_,backframe_+inw_*inh_,backframe_+inw_*inh_*3/2,inw_,inh_);
	return (1);
}

/* Searches for complete GOB sequences in our ringbuffer.
 * Mode A blocks are returned instantly, mode B blocks are reassembled first
 * (if possible).
 */
inline int
H263Decoder::reassemble_gobs(u_char **newbp,int *newcc)
{
	u_int	startblock,j,i,xi,x,l,last,size,seqno,firstlong;
	u_char	*p;
	int	endblock;

	/* FIXME: should start with oldest packet (vfy timestamp/sequence nr) */
	for (i=0;i<H263_SLOTS;i++) {
		xi = i & H263_SLOTMASK;
		seqno = slot_[xi].seqno;
		/* length 0 -> empty/already handled */
		if (!slot_[xi].cc)
			continue;
		/* mode A blocks always contain complete GOB sequences */
		if (!slot_[xi].h263rh.ftype)  {
			*newbp = slot_[xi].bp;
			*newcc = slot_[xi].cc;
			slot_[xi].cc = 0;
			return 1;
		}
		/* The first 22 bits must be the PSC if we want to reassemble
		 * a GOB sequence.
		 */
		/* 0000 0000 0000 0000 100000  == 0x00008000 */
		p = slot_[xi].bp;
		firstlong = (p[0]<<24)+(p[1]<<16)+(p[2]<<8)+p[3];
		if ((firstlong & 0xffff8000) != 0x00008000)
			continue;

		/* Good. Now find the longest mode B chain in the ringbuffer,
		 * which contains this block. (if we do not do this, we get
		 * problems with buffer-wrapping mode B splits).
		 */
		startblock = xi;
		/* backwards search done by (n-1) -> (0) addition and modulo
		 * operatin. We get xi-1,xi-2 ...
		 */
		for (j=H263_SLOTS;j--;) {
			x=(xi+j)&H263_SLOTMASK;
			
			p = slot_[x].bp;
			firstlong = (p[0]<<24)+(p[1]<<16)+(p[2]<<8)+p[3];
			/* A mode A block breaks the mode B chain */
			if (!slot_[x].h263rh.ftype)
				break;
			/* An empty entry too. */
			if (!slot_[x].cc)
				break;
			/* A wrong sequence number too. */
			if ((slot_[x].seqno & 0xffff) != ((seqno+(j-H263_SLOTS)) & 0xffff)) //SV: vs7 warn fix
				break;
			/* A wrong magic at the start (not GSC) is not useful
			 * as starting block, continue. */
			if ((firstlong & 0xffff8000) != 0x00008000)
				continue;
			/* all correct, maybe we can use this as startblock,
			 * continue searching. */
			startblock = x;
		}
		/* new sequence startnr? */
		if (xi!=startblock)
			seqno=slot_[startblock].seqno;

		endblock = -1;
		/* look forward for the next endblock */
		for (j=0;j<H263_SLOTS;j++) {
			x=(startblock+j)&H263_SLOTMASK;

			/* wrong sequence number -> leave */
			if ((slot_[x].seqno & 0xffff)!= ((seqno+j) & 0xffff)) //SV: vs7 warn fix
				break;
			/* empty entry is a valid endmarker */
			if (!slot_[x].cc) {
				/* ... but only if we got at least 2 mode b
				 * blocks.*/
				if (j)
					endblock = j-1;
				break;
			}
			p = slot_[x].bp;
			firstlong = (p[0]<<24)+(p[1]<<16)+(p[2]<<8)+p[3];
			/* GSC is a valid endmarker */
			if ((firstlong & 0xffff8000) == 0x00008000)
				if (j) {
					endblock=j-1;
					break;
				}

			/* A Mode A block ist a valid endmarker */
			if (!slot_[x].h263rh.ftype) {
				endblock = j-1;
				break;
			} else {
				/* And the RTP endmarker too */
				if (slot_[x].rtflags&RTP_M) {
					endblock = j;
					break;
				}
			}
		}
		/* no endmarker found? Look for next Mode B chain ... */
		if (endblock==-1)
			continue;

		/* Determine size of this mode B chain */
		size = 0;
		for (l=0;l<=endblock;l++) {
			x=(startblock+l)&H263_SLOTMASK;
			size+=slot_[x].cc;
			if (slot_[x].h263rh.ebit)
				size--;
		}
		/* If necessary, increase temporary buffer */
		size+=16;
		if (size>h263streamsize_) {
			delete[] h263stream_;
			h263stream_ = new u_char[size];
			memset(h263stream_,0x42,size);
			h263streamsize_ = size;
		}
		memset(h263stream_,0,h263streamsize_);

		/* and reassemble */
		last=0;
		for (l=0;l<=endblock;l++) {
			x=(startblock+l)&H263_SLOTMASK;
			if (slot_[x].h263rh.sbit) {
				/* overlapping bytes */
				h263stream_[last]|=slot_[x].bp[0];
				last++;
				memcpy(h263stream_+last,slot_[x].bp+1,slot_[x].cc-1);
				last+=slot_[x].cc-1;
			} else {
				memcpy(h263stream_+last,slot_[x].bp,slot_[x].cc);
				last+=slot_[x].cc;
			}
			/* For overlapping bytes stay on that last byte. */
			if ((l<endblock) && slot_[x].h263rh.ebit)
				last--;
			/* 'empty' */
			slot_[x].cc = 0;
		}
		/* and done! */
		*newbp = h263stream_;
		*newcc = last;
		return 1;
	}
	/* nothing found ... */
	return 0;
}

/* For exact format, check out RFC 2190, I just extract the relevant bits of
 * information.
 *
 * NOTE: The bit extraction method used will NOT work for some of the commented
 *       out fields which overlap bytes (e.g. mba)!!
 *
 * ASSUMES: layout of h263rtpheader_B and h263rtpheader_A is the same for
 *          structure members ftype,sbit,ebit and srcformat.
 */
static inline int
copy_to_h263rtpheader( h263rtpheader_B *h263rh, const u_char *bp) {
	u_char const mask[9] = {0x0,0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff};

#define BIT(x,n) ((bp[x>>3]>>(8-n-(x&7)))&mask[n])

	/* They have the same position in the RTP header,
	 * and in our structures too 
	 */
	h263rh->ftype		= BIT(0,1);
	h263rh->sbit		= BIT(2,3);
	h263rh->ebit		= BIT(5,3);
	h263rh->srcformat	= BIT(8,3);
	return h263rh->ftype;

#if 0
	if (h263rh->ftype) {
	    h263rh->pbframes			= BIT(1,1);
	    h263rh->sbit			= BIT(2,3);
	    h263rh->ebit			= BIT(5,3);
	    h263rh->srcformat			= BIT(8,3);
	    h263rh->quant			= BIT(11,5);
	    h263rh->gobn			= BIT(16,5);
	    h263rh->mba				= BIT(21,9);
	    h263rh->reserved			= BIT(30,2);
	    h263rh->picture_coding_type		= BIT(32,1);
	    h263rh->unrestricted_motion_vector	= BIT(33,1);
	    h263rh->syntax_based_arithmetic	= BIT(34,1);
	    h263rh->advanced_prediction		= BIT(35,1);
	    h263rh->hmv1			= BIT(36,7);
	    h263rh->vmv1			= BIT(43,7);
	    h263rh->hmv2			= BIT(50,7);
	    h263rh->vmv2			= BIT(57,7);
	    return 1;
	} else {
	    h263rtpheader_A *h263rha = (h263rtpheader_A*)h263rh;
	    h263rha->pbframes			= BIT(1,1);
	    h263rha->sbit			= BIT(2,3);
	    h263rha->ebit			= BIT(5,3);
	    h263rha->srcformat			= BIT(8,3);
	    h263rha->picture_coding_type	= BIT(11,1);
	    h263rha->unrestricted_motion_vector	= BIT(12,1);
	    h263rha->syntax_based_arithmetic	= BIT(13,1);
	    h263rha->advanced_prediction	= BIT(14,1);
	    h263rha->reserved			= BIT(15,4);
	    h263rha->dbq			= BIT(19,2);
	    h263rha->trb			= BIT(21,3);
	    h263rha->tr				= BIT(24,8);
	    return 0;
	}
#endif
}

/* The decoderfunction itself.
 * Gets RTP packets from the VIC networking layer, reassembles them to GOB 
 * sequences, decodes them and displays the result. (and more magic ;)
 */
void H263Decoder::recv(pktbuf* pb)
 {
	rtphdr* rh = (rtphdr*)pb->dp;
	int hdrsize = sizeof(rtphdr) + hdrlen();
	u_char* bp = pb->dp + hdrsize;
	int cc = pb->len - hdrsize;

	u_int	k,newsrcformat;
	int		targetcc, i, seq, disp, gobbytes, gobnr;
	u_char	*targetbp,*next;
	int		ismodeB, offset;
	int		l = ntohs(rh->rh_seqno) & H263_SLOTMASK;
	h263rtpheader_B h263rhb;

	ismodeB = copy_to_h263rtpheader(&h263rhb,bp);
	newsrcformat = h263rhb.srcformat;

	/* How long is the RTP payload header? */
	if (ismodeB)
		offset = 8;
	else
		offset = 4;
	assert(cc < 2000);

	/* Fill out ringbuffer entry, leave out header from payload */
	slot_[l].cc	= cc-offset;
	slot_[l].seqno = ntohs(rh->rh_seqno) & 0xffff;
	slot_[l].rtflags = ntohs(rh->rh_flags);
	memcpy(&(slot_[l].h263rh),&h263rhb,sizeof(h263rhb));
	memcpy((char*)slot_[l].bp, bp+offset,cc-offset);

	seq = slot_[l].seqno;

	/* Did the srcformat change? */
	if (srcformat_!=newsrcformat) {
		switch (newsrcformat) {
		case 1: inw_ = SQCIF_WIDTH;
			inh_ = SQCIF_HEIGHT;
			break;
		case 2: inw_ = QCIF_WIDTH;
			inh_ = QCIF_HEIGHT;
			break;
		case 3: inw_ = CIF_WIDTH;
			inh_ = CIF_HEIGHT;
			break;
		case 4: inw_ = CIF4_WIDTH;
			inh_ = CIF4_HEIGHT;
			break;
		case 5: inw_ = 2*CIF4_WIDTH;
			inh_ = 2*CIF4_HEIGHT;
			break;
		case 0:
		default:
			fprintf(stderr,"illegal sourceformat %d!\n",newsrcformat);
			break;
		}
		/* 0000 0000 0000 0000 100000  == 0x00008000 */
		u_char *p = slot_[l].bp;
		long firstlong = (p[0]<<24)+(p[1]<<16)+(p[2]<<8)+p[3];
		if ((firstlong & 0xfffffe00) != 0x00008000) {
			pb->release();
			return;
		}
		srcformat_ = newsrcformat;
		backframe_ = NULL;
		resize(inw_,inh_);
		for (i=0;i<H263_SLOTS;i++)
			slot_[i].cc = 0;
	
		/* Decoder disinit */
		if (!h263decoder->first)
			DisinitH263Decoder(h263decoder);
		FreeH263Decoder(h263decoder);
		h263decoder=NewH263Decoder();
		/* And initialise new, we do know the new size */
		h263decoder->source_format = srcformat_;
		InitH263Decoder(h263decoder);
	}
	k = 1;
	if (inw_ == CIF4_WIDTH)		k = 2;
	if (inw_ == 2*CIF4_WIDTH)	k = 4;

	if (inw_ == QCIF_WIDTH)
		gobnr = 9;
	else
		gobnr = 18;
	
	gobbytes = k*16*inw_;

	/* For detecting GOBs to display. */
	/* XXX */
	int	decgobs[18];

	/* as long as we find GOB sequences in the ringbuffer */
	while (reassemble_gobs(&targetbp,&targetcc)) {
		next = targetbp;
		assert(!next[0] && !next[1]);
		for (i=0;i<gobnr;i++)
			decgobs[i] = h263decoder->decGOBs[i];
		int tempref = h263decoder->temp_ref;
		while (next<targetbp+targetcc) {
			if (h263decoder->first) {
				long firstlong = (next[0]<<24)+(next[1]<<16)+(next[2]<<8)+next[3];
				if ((firstlong&0xfffffc00)!=0x00008000) {
					/* skip this block, we can't use it*/
					next = targetbp+targetcc;
					continue;
				}
			}

			disp=HandleH263DataJunk(h263decoder,&next,targetbp+targetcc,0);
		}

		/* mark the decoded GOBs as "to update" in rvts_ */
		for (i=0;i<gobnr;i++) {
			/* lastdec	nowdec	disp
			 * 0		1	yes
			 * 0		0	no
			 * 1		0	no
			 * 1		1	hmm, no
			 */
			if (h263decoder->decGOBs[i] && !decgobs[i])
				memset(rvts_+(i*16*k*inw_)/64, now_^0x80, 16*k*inw_/64);
			decgobs[i] = h263decoder->decGOBs[i];
		}
		if (tempref!=h263decoder->temp_ref) {
			if (!h263decoder->first)
				H263FinishPicture(h263decoder);
			for (i=0;i<gobnr;i++)
				decgobs[i]=0;
		}
	}
	/* display the current decoded frame */
	backframe_ = h263decoder->newframe[0];
	if (backframe_)
		render_frame(backframe_);
	
	pb->release();
}

void H263Decoder::redraw() {
	if (backframe_)
		Decoder::redraw(backframe_);
}
