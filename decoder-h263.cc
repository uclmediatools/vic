/*
 * (c) Marcus Meissner
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
	decimation_	= 411;
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

/* Liefert Informationen ueber den aktuellen Decoder an den Tcl Code.
 * FIXME: Richtige Informationen zurueckliefern ;)
 */
void H263Decoder::info(char* wrk) const
{
	sprintf(wrk, "[q=42]");
}

/* Berechne eine Art Colorhistory. Wird wohl im PseudoColor Modus verwendet */
int H263Decoder::colorhist(u_int* hist) const
{
	if (backframe_)
		colorhist_411_556(hist,backframe_,backframe_+inw_*inh_,backframe_+inw_*inh_*3/2,inw_,inh_);
	return (1);
}

/* Sucht komplette GOB Sequenzen im Ringbuffer.
 * Mode A Bloecke werden sofort zurueckgeliefert, Mode B Bloecke erst zusammen-
 * gesetzt.
 */
inline int
H263Decoder::reassemble_gobs(u_char **newbp,int *newcc)
{
	u_int	startblock,endblock,j,i,xi,x,l,last,size,sbit,seqno,firstlong;
	u_char	*p;

	/* FIXME: sollte wahrscheinlich beim aeltesten Paket anfangen...  */
	for (i=0;i<H263_SLOTS;i++) {
		xi = i & H263_SLOTMASK;
		seqno = slot_[xi].seqno;
		/* Laenge 0 markiert leere Eintraege */
		if (!slot_[xi].cc)
			continue;
		/* Mode A Bloecke enthalten komplette GOB Sequenzen -> return */
		if (!slot_[xi].h263rh.ftype)  {
			*newbp = slot_[xi].bp;
			*newcc = slot_[xi].cc;
			slot_[xi].cc = 0;
			return 1;
		}
		/* In den ersten Bytes muss der 22 Bit PSC sein */
		/* 0000 0000 0000 0000 100000  == 0x00008000 */
		p = slot_[xi].bp;
		firstlong = (p[0]<<24)+(p[1]<<16)+(p[2]<<8)+p[3];
		if ((firstlong & 0xffff8000) != 0x00008000)
			continue;

		/* Finde den laengsten Mode B Verkettungspfad, der diesen 
		 * Block enthaelt. Wenn wir dies nicht tun, gibt es Probleme
		 * mit um den Buffer wrappenden Mode B Splits.
		 */
		startblock = xi;
		/* Rueckwaerts Suche wird durch (n-1) -> (0) Addition und
		 * Modulo erreicht. Wir erhalten also xi-1,xi-2 ...
		 */
		for (j=H263_SLOTS;j--;) {
			x=(xi+j)&H263_SLOTMASK;
			
			p = slot_[x].bp;
			firstlong = (p[0]<<24)+(p[1]<<16)+(p[2]<<8)+p[3];
			/* Mode A Block unterbricht die Kette */
			if (!slot_[x].h263rh.ftype)
				break;
			/* Leerer Eintrag -> dito */
			if (!slot_[x].cc)
				break;
			/* Falsche Sequenznummer -> dito */
			if ((slot_[x].seqno&0xffff)!= (seqno+(j-H263_SLOTS))&0xffff)
				break;
			/* Falsche Magic am Anfang (nicht GSC) -> dito */
			if ((firstlong & 0xffff8000) != 0x00008000)
				continue;
			/* alles korrekt, neuen Start merken und weitersuchen */
			startblock = x;
		}
		/* neue startsequenznummer */
		if (xi!=startblock)
			seqno=slot_[startblock].seqno;

		endblock = -1;
		/* jetzt vorwaerts nach dem ersten Ende suchen */
		for (j=0;j<H263_SLOTS;j++) {
			x=(startblock+j)&H263_SLOTMASK;

			/* Sequenznummer falsch -> raus */
			if ((slot_[x].seqno&0xffff)!= (seqno+j)&0xffff)
				break;
			/* Leerer Eintrag ist eine gueltige Endemarkerung */
			if (!slot_[x].cc) {
				/* ... aber nur, wenn wir mindestens zwei 
				 * Mode B bloecke haben 
				 */
				if (j)
					endblock = j-1;
				break;
			}
			p = slot_[x].bp;
			firstlong = (p[0]<<24)+(p[1]<<16)+(p[2]<<8)+p[3];
			/* GSC ist gueltige Endemarkierung */
			if ((firstlong & 0xffff8000) == 0x00008000)
				if (j) {
					endblock=j-1;
					break;
				}

			/* Mode A Block ist gueltige Endemarkierung */
			if (!slot_[x].h263rh.ftype) {
				endblock = j-1;
				break;
			} else {
				/* Und RTP Endemarkierung natuerlich auch */
				if (slot_[x].rtflags&RTP_M) {
					endblock = j;
					break;
				}
			}
		}
		/* kein Endpunkt gefunden? Schade, weitersuchen */
		if (endblock==-1)
			continue;

		/* Groesse der resultierenden Kette bestimmen */
		size = 0;
		for (l=0;l<=endblock;l++) {
			x=(startblock+l)&H263_SLOTMASK;
			size+=slot_[x].cc;
			if (slot_[x].h263rh.ebit)
				size--;
		}
		/* evt. temporaeren Buffer vergroessern */
		size+=16;
		if (size>h263streamsize_) {
			delete[] h263stream_;
			h263stream_ = new u_char[size];
			memset(h263stream_,0x42,size);
			h263streamsize_ = size;
		}
		memset(h263stream_,0,h263streamsize_);

		/* und zusammenkopieren */
		last=0;
		for (l=0;l<=endblock;l++) {
			x=(startblock+l)&H263_SLOTMASK;
			if (slot_[x].h263rh.sbit) {
				/* Ueberlappende Bytes */
				h263stream_[last]|=slot_[x].bp[0];
				last++;
				memcpy(h263stream_+last,slot_[x].bp+1,slot_[x].cc-1);
				last+=slot_[x].cc-1;
			} else {
				memcpy(h263stream_+last,slot_[x].bp,slot_[x].cc);
				last+=slot_[x].cc;
			}
			/* Bei ueberlappenden Bytes noch nicht ueber das letzte
			 * Byte hinausgehen
			 */
			if ((l<endblock) && slot_[x].h263rh.ebit)
				last--;
			/* 'leer' */
			slot_[x].cc = 0;
		}
		/* und fertig */
		*newbp = h263stream_;
		*newcc = last;
		return 1;
	}
	/* nix gefunden ... */
	return 0;
}

/* Eigentliche Dekoderfunktion.
 * Erhaelt RTP Pakete von der VIC Netzwerkschicht, setzt diese ggfalls in 
 * GOB Sequenzen zusammen, dekodiert sie und stellt das Ergebnis dar.
 */
void H263Decoder::recv(const rtphdr* rh, const u_char* bp, int cc)
 {
	u_int	h263streamsize,k;
	int	targetcc,i,ret,seq,disp,gobbytes,gobnr;
	u_char	*targetbp,*next;
	h263rtpheader_B	*h263rh = (h263rtpheader_B*)bp;
	int	offset,l = ntohs(rh->rh_seqno) & H263_SLOTMASK;

	/* Wie lang ist der Extra Header ? */
	if (h263rh->ftype)
		offset = 8;
	else
		offset = 4;
	assert(cc < 2000);

	/* Ringbuffer Eintrag ausfuellen, headersize abziehen von den Daten */
	slot_[l].cc	= cc-offset;
	slot_[l].seqno = ntohs(rh->rh_seqno) & 0xffff;
	slot_[l].rtflags = ntohs(rh->rh_flags);
	memcpy(&(slot_[l].h263rh),h263rh,offset);
	memcpy((char*)slot_[l].bp, bp+offset,cc-offset);

	seq = slot_[l].seqno;

	/* Hat sich das (Bild)Format geaendert? */
	if (srcformat_!=h263rh->srcformat) {
		switch (h263rh->srcformat) {
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
			fprintf(stderr,"illegal sourceformat %d!\n",h263rh->srcformat);
			break;
		}
		/* 0000 0000 0000 0000 100000  == 0x00008000 */
		u_char *p = slot_[l].bp;
		long firstlong = (p[0]<<24)+(p[1]<<16)+(p[2]<<8)+p[3];
		if ((firstlong & 0xfffffe00) != 0x00008000) {
			return;
		}
		srcformat_ = h263rh->srcformat;
		backframe_ = NULL;
		resize(inw_,inh_);
		for (i=0;i<H263_SLOTS;i++)
			slot_[i].cc = 0;
	
		/* Decoder disinitialisieren */
		if (!h263decoder->first)
			DisinitH263Decoder(h263decoder);
		FreeH263Decoder(h263decoder);
		h263decoder=NewH263Decoder();
		/* Und neu initialisieren, wir kennen die Groesse schon */
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

	/* zur erkennung von dekodierten GOBs fuer die Darstellung */
	int gobn=gobnr;
	/* XXX */
	int	decgobs[18];

	/* solange wir noch GOB Sequenzen finden im Ringbuffer */
	while (reassemble_gobs(&targetbp,&targetcc)) {
		next = targetbp;
		assert (!next[0] && !next[1]);
		for (i=0;i<gobnr;i++)
			decgobs[i] = h263decoder->decGOBs[i];
		int tempref = h263decoder->temp_ref;
		while (next<targetbp+targetcc) {
			if (h263decoder->first) {
				long firstlong = (next[0]<<24)+(next[1]<<16)+(next[2]<<8)+next[3];
				if ((firstlong&0xfffffc00)!=0x00008000)
					continue;
			}

			disp=HandleH263DataJunk(h263decoder,&next,targetbp+targetcc,0);
		}

		/* markiere die dekodierten GOBs als zu updatende in rvts_ */
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
	/* stelle den aktuell dekodierten Frame dar */
	backframe_ = h263decoder->newframe[0];
	if (backframe_)
		render_frame(backframe_);
}

/* Redraw Funktion */
void H263Decoder::redraw() {
	if (backframe_)
		Decoder::redraw(backframe_);
}
