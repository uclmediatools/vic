/*
 * Copyright (c) 1998-1999 Marcus Meissner and The Regents of the University of
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
 * This h263 packager was written for a DFN project (http://www.dfn.de/) at the
 * University of Erlangen-Nuremberg, Lehrstuhl fuer Nachrichtentechnik by
 * Marcus Meissner in 1998.
 *
 * Merged into UCL vic development tree in 1998 by ?
 *
 * Cleanups, endianess, translation of comments into english, bugfixes,
 * done by Marcus Meissner in June 1999.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "defs.h"

#include "h263.h"
 
#define HDRSIZE_A	4
#define HDRSIZE_B	8

/* FIXME: bitextraction can be done faster, yes */
static inline u_long 
extract_bits(const u_char *bp,const int x,const int n) {
        u_long xx;
        int     i;
#define BIT(x) !!(bp[x>>3]&(1<<(7-(x&7))))

        xx = 0;
        for (i=x;i<x+n;i++)
                xx=(xx<<1)|BIT(i);
        return xx;
#undef BIT
}


void copy_buf_to_streamheader(const u_char *bp,h263streamheader *h263sh) {
	memset(h263sh,0,sizeof(*h263sh));
#define BIT(x,n) extract_bits(bp,x,n)
	h263sh->syncword			= BIT(0,17);
	h263sh->gobnr				= BIT(17,5);
	h263sh->tr				= BIT(22,8);
	h263sh->reserved0			= BIT(30,1);
	h263sh->reserved1			= BIT(31,1);

	h263sh->splitscreen			= BIT(32,1);
	h263sh->doccamera			= BIT(33,1);
	h263sh->freeze_picture_release		= BIT(34,1);
	h263sh->srcformat			= BIT(35,3);
	h263sh->picture_coding_type		= BIT(38,1);
	h263sh->unrestricted_motion_vector	= BIT(39,1);
	h263sh->syntax_based_arithmetic		= BIT(40,1);
	h263sh->advanced_prediction		= BIT(41,1);
	h263sh->pb_frames			= BIT(42,1);
	h263sh->pquant				= BIT(43,5);
	h263sh->cpm				= BIT(48,1);
	h263sh->psbi				= BIT(49,1);
	h263sh->trb				= BIT(50,1);
	h263sh->dbq				= BIT(51,2);
}

#define SBIT(x,to) bp[x>>3]|=(to<<(7-(x&7)))
static inline void xset_bits(u_char *bp,int x,int to,int n) {
	int	i;

	for (i=n;i--;) { SBIT((x+i),(to&1)); to>>=1; }
}
#undef SBIT

void copy_headerB_to_buf(h263rtpheader_B *h263rh,u_char *bp) {
	memset(bp,0,8);

#define SBIT(x,to,n) xset_bits(bp,x,to,n)

	SBIT(0,1,1);
	SBIT(1,h263rh->pbframes,1);
	SBIT(2,h263rh->sbit,3);
	SBIT(5,h263rh->ebit,3);
	SBIT(8,h263rh->srcformat,3);
	SBIT(11,h263rh->quant,5);
	SBIT(16,h263rh->gobn,5);
	SBIT(21,h263rh->mba,9);
	SBIT(30,h263rh->reserved,2);
	SBIT(32,h263rh->picture_coding_type,1);
	SBIT(33,h263rh->unrestricted_motion_vector,1);
	SBIT(34,h263rh->syntax_based_arithmetic,1);
	SBIT(35,h263rh->advanced_prediction,1);
	SBIT(36,h263rh->hmv1,7);
	SBIT(43,h263rh->vmv1,7);
	SBIT(50,h263rh->hmv2,7);
	SBIT(57,h263rh->vmv2,7);
}

void copy_headerA_to_buf(h263rtpheader_A *h263rh,u_char *bp) {
	memset(bp,0,4);

	SBIT(0,0,1);
	SBIT(1,h263rh->pbframes,1);
	SBIT(2,h263rh->sbit,3);
	SBIT(5,h263rh->ebit,3);
	SBIT(8,h263rh->srcformat,3);
	SBIT(11,h263rh->picture_coding_type,1);
	SBIT(12,h263rh->unrestricted_motion_vector,1);
	SBIT(13,h263rh->syntax_based_arithmetic,1);
	SBIT(15,h263rh->reserved,4);
	SBIT(19,h263rh->dbq,2);
	SBIT(21,h263rh->trb,3);
	SBIT(24,h263rh->tr,8);
}

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
) {
	h263streamheader	h263sh;
	int			i,sendmodeb,plen,lastgobind,lastsentbit,curind;
	int			lastind,psize=headersize-HDRSIZE_B,nrofrtps=0;

	copy_buf_to_streamheader(bs,&h263sh);
	*packets = NULL;lastsentbit=0;curind=0;sendmodeb=0;

	while (lastsentbit<bitcount) {
		h263_rtp_packet	*curpacket;

		lastgobind = 0;

		/* Look for next GOB that fits whole into an RTP packet 
		 * The last+1 block is the complete bitstreamlength.
		 */
		for (i=0;i<=gobs_per_frame;i++)
			if (mbind[i*mbs_per_gob]<lastsentbit-8+(psize<<3))
				lastgobind = mbind[i*mbs_per_gob];


		if (lastgobind<=lastsentbit) {
			/* Next GOB doesn't fit */
			lastgobind = 0;
		}
		/* allocate one more RTP packet */
		nrofrtps++;
		if (*packets)
			*packets=(h263_rtp_packet*)realloc(*packets,sizeof(*packets[0])*nrofrtps);
		else
			*packets=(h263_rtp_packet*)malloc(sizeof(*packets[0])*nrofrtps);
		curpacket = (*packets)+(nrofrtps-1);

		/* Wir senden das naechste Paket im Mode B, wenn:
		 * - Das letzte im Mode B gesendet wurde und nicht an einer GOB
		 *   Grenze endete.
		 * - Der aktuelle GOB nicht vollstaendig in ein Paket passt.
		 */
		if (sendmodeb || !lastgobind) {
			/* RTP/H.263 header ,Mode B (64bit) */
			h263rtpheader_B	 h263rhb;
			curpacket->headersize	= HDRSIZE_B;

			/* fill in header stuff */
			h263rhb.ftype		= 1;	/* mode b */
			h263rhb.pbframes	= 0;	/* no pb frames */
			h263rhb.srcformat	= h263sh.srcformat;
			assert(h263rhb.srcformat);
			h263rhb.picture_coding_type	= h263sh.picture_coding_type;
			h263rhb.unrestricted_motion_vector= h263sh.unrestricted_motion_vector;
			h263rhb.syntax_based_arithmetic= h263sh.syntax_based_arithmetic;
			h263rhb.advanced_prediction	= h263sh.advanced_prediction;
			h263rhb.reserved	= 0;
			h263rhb.quant		= 0;
			h263rhb.hmv2		= 0;
			h263rhb.vmv2		= 0;
			h263rhb.gobn		= gobs_per_frame-1;
			if (sendmodeb) { 
				/* Motion vectors/mba are only relevant
				 * if the packet before was modeB too
				 */
				h263rhb.mba	= lastind % mbs_per_gob;
				h263rhb.hmv1	= mvfield.mx[lastind];
				h263rhb.vmv1	= mvfield.my[lastind];
				h263rhb.quant	= mbquant[lastind];
				/* look if we can split at a GOB */
				lastgobind = -1;
				for (i=0;i<=gobs_per_frame;i++)
					if (mbind[i*mbs_per_gob]<lastsentbit-8+(psize<<3))
						lastgobind = mbind[i*mbs_per_gob];
				if (lastgobind>lastsentbit) {
					/* got (at least) one GOB. */
					sendmodeb = 0;
				} else {
					/* no GOB end in this block(again) 
					 * look for maximum number of fitting
					 * macroblocks
					 */
					lastgobind = -1;
					for (i=lastind;(i<=mbs_per_gob*gobs_per_frame) && (mbind[i]<lastsentbit-8+(psize<<3));i++)
						lastgobind = mbind[i];
					if (lastgobind<=lastsentbit) {
						fprintf(stderr,"macroblock length of %d (exceed packetbitlength %d)!\n",mbind[i]-lastsentbit,psize*8);
						assert(lastgobind>lastsentbit);
					}
					sendmodeb = 1;
					lastind = i;
				}
			} else {
				h263rhb.mba	= 0;
				h263rhb.quant	= 0;
				h263rhb.hmv1	= 0;
				h263rhb.vmv1	= 0;
				for (	i=0				    ;
					(i<=mbs_per_gob*gobs_per_frame) &&
					(mbind[i]<lastsentbit-8+(psize<<3)) ;
					i++
				)
					lastgobind = mbind[i];
				lastind		= i;
				sendmodeb	= 1;
			}
			h263rhb.sbit		= lastsentbit&7;
			h263rhb.ebit		= 7-((lastgobind-1) &7);

			copy_headerB_to_buf(&h263rhb,curpacket->header);
			plen = ((lastgobind+7)>>3)-(lastsentbit>>3);
			assert(plen>0);
			assert(plen<=psize);
			assert(lastsentbit!=lastgobind);
			curpacket->datasize	= plen;
			curpacket->data		= bs+(lastsentbit>>3);
			lastsentbit 		= lastgobind;
		} else {
			h263rtpheader_A	 h263rh;

			curpacket->headersize	= HDRSIZE_A;
			h263rh.ftype		= 0;	/*only mode a*/
			h263rh.pbframes		= 0;	/*no pb frames*/
			h263rh.srcformat	= h263sh.srcformat;
			assert(h263rh.srcformat);
			h263rh.picture_coding_type	= h263sh.picture_coding_type;
			h263rh.unrestricted_motion_vector= h263sh.unrestricted_motion_vector;
			h263rh.syntax_based_arithmetic	= h263sh.syntax_based_arithmetic;
			h263rh.advanced_prediction	= h263sh.advanced_prediction;
			h263rh.reserved		= 0;
			h263rh.dbq		= h263sh.dbq;
			h263rh.trb		= h263sh.trb;
			h263rh.tr		= h263sh.tr;
			h263rh.sbit		= lastsentbit&7;
			h263rh.ebit		= 7-((lastgobind-1) &7);
			copy_headerA_to_buf(&h263rh,curpacket->header);
			plen = ((lastgobind+7)>>3)-(lastsentbit>>3);
			assert(plen>0);
			assert(lastgobind>lastsentbit);
			assert(plen<=psize);
			curpacket->data		= bs+(lastsentbit>>3);
			curpacket->datasize	= plen;
			lastsentbit		= lastgobind;
			sendmodeb = 0;
		}
	}
	return nrofrtps;
}
