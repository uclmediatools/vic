/* H263 Packetizer 
 * Copyright 1998 Marcus Meissner
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
	h263streamheader	*h263sh = (h263streamheader*)bs;
	int			i,sendmodeb,plen,lastgobind,lastsentbit,curind;
	int			lastind,psize=headersize-HDRSIZE_B,nrofrtps=0;

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
			h263rtpheader_B	 *h263rhb = &(curpacket->header.h263b);
			curpacket->headersize	= HDRSIZE_B;

			/* fill in header stuff */
			h263rhb->ftype		= 1;	/* mode b */
			h263rhb->pbframes	= 0;	/* no pb frames */
			h263rhb->srcformat	= h263sh->srcformat;
			assert(h263rhb->srcformat);
			h263rhb->picture_coding_type	= h263sh->picture_coding_type;
			h263rhb->unrestricted_motion_vector= h263sh->unrestricted_motion_vector;
			h263rhb->syntax_based_arithmetic= h263sh->syntax_based_arithmetic;
			h263rhb->advanced_prediction	= h263sh->advanced_prediction;
			h263rhb->reserved	= 0;
			h263rhb->quant		= 0;
			h263rhb->hmv2		= 0;
			h263rhb->vmv2		= 0;
			h263rhb->gobn		= gobs_per_frame-1;
			if (sendmodeb) { 
				/* Motion vectors/mba are only relevant
				 * if the packet before was modeB too
				 */
				h263rhb->mba	= lastind % mbs_per_gob;
				h263rhb->hmv1	= mvfield.mx[lastind];
				h263rhb->vmv1	= mvfield.my[lastind];
				h263rhb->quant	= mbquant[lastind];
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
				h263rhb->mba	= 0;
				h263rhb->quant	= 0;
				h263rhb->hmv1	= 0;
				h263rhb->vmv1	= 0;
				for (	i=0				    ;
					(i<=mbs_per_gob*gobs_per_frame) &&
					(mbind[i]<lastsentbit-8+(psize<<3)) ;
					i++
				)
					lastgobind = mbind[i];
				lastind		= i;
				sendmodeb	= 1;
			}

			h263rhb->sbit		= lastsentbit&7;
			h263rhb->ebit		= 7-((lastgobind-1) &7);
			plen = ((lastgobind+7)>>3)-(lastsentbit>>3);
			assert(plen>0);
			assert(plen<=psize);
			assert(lastsentbit!=lastgobind);
			curpacket->datasize	= plen;
			curpacket->data		= bs+(lastsentbit>>3);
			lastsentbit 		= lastgobind;
		} else {
			h263rtpheader_A	 *h263rh = &(curpacket->header.h263a);
			curpacket->headersize	= HDRSIZE_A;
			h263rh->ftype		= 0;		/*only mode a*/
			h263rh->pbframes	= 0;		/*no pb frames*/
			h263rh->srcformat	= h263sh->srcformat;
			assert(h263rh->srcformat);
			h263rh->picture_coding_type	= h263sh->picture_coding_type;
			h263rh->unrestricted_motion_vector= h263sh->unrestricted_motion_vector;
			h263rh->syntax_based_arithmetic	= h263sh->syntax_based_arithmetic;
			h263rh->advanced_prediction	= h263sh->advanced_prediction;
			h263rh->reserved	= 0;
			h263rh->dbq		= h263sh->dbq;
			h263rh->trb		= h263sh->trb;
			h263rh->tr		= h263sh->tr;
			h263rh->sbit		= lastsentbit&7;
			h263rh->ebit		= 7-((lastgobind-1) &7);
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
