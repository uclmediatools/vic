/* Copyright 1998 Marcus Meissner
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "bsd-endian.h"
#include "transmitter.h"
#include "module.h"
#include "crdef.h"
//#include "p64/p64-huff.h"
 
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
	/* Erster Frame muss intrakodiert werden */
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

	/* Globale maximale rate fuer den Encoder.
	 * Der Encoder selber hat keine zuverlaessige Bitratenbegrenzung, so
	 * das wir einen sehr grossen Buffer allozieren und einfach hoffen,
	 * das der Encoder nicht mehr braucht.
	 */
	maxrate_ = 100000;
	bitstr_.b = new Byte[maxrate_*10/8];
	bitstr_.size = maxrate_*10;
	bitstr_.ind = 0;
	bitstr_.actualSize = 0;
	bitstr_.fp = NULL;

	/* Wir verwenden nur codingtime 3, siehe H263 Encoder Dokumentation,
	 * da dies der beste time/quality/space tradeoff ist derzeit
	 */
	codingtime_ = 3;

	maycheck_ = 0;

	/* Default Quantisierer ist 10 */
	q_ = 10;
}

H263Encoder::~H263Encoder()
{
/* Bus Errors...
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
 * Die Hauptinterface Funktion zum VIC. Wird fuer jeden gegrabbten Frame
 * aufgerufen.
 * Sie kodiert diese Frames in H.263, paketiert ihn gemaess RFC 2190 und
 * verschickt die RTP Pakete mit der RTP schicht des VIC Tools.
 *
 * Liefert die Anzahl an gesendeten Bytes zurueck.
 */
int H263Encoder::consume(const VideoFrame* vf)
{
	YuvFrame*	yuv = (YuvFrame*)vf;	/* eigentlicher YUV Frame */
	Picture		thispic;		/* das aktuelle Bild */
	u_int		xfps,kbps;		/* Parameter vom Tcl */
	u_int		i;
	u_int		sentbytes=0;		/* in diesem consume gesendete
						 * Bytes
						 */
	u_int		bps;			/* Angestrebte bitrate */
	int		k,mbs_per_gob,gobs_per_frame;
	int		timespent,w,h;
	Tcl&		tcl = Tcl::instance();
	Transmitter::pktbuf* pb;
	rtphdr*		rh;
	struct timeval tv1,tv2;

	w = yuv->width_;
	h = yuv->height_;


        k = 1;
        if (w == CIF4_WIDTH)         k = 2;
        if (w == 2*CIF4_WIDTH)       k = 4;
 
	/* nicht korrekt fuer CIF*2 und hoeher */
	mbs_per_gob = w/16/k;
	gobs_per_frame = h/16/k;

	/* Groesse veraendert? */
	if (w!=lastw_ || h!=lasth_) {
		/* Groesse der Strukturen usw. anpassen */
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

	/* HACK: Abfragen der Tcl Schicht nach den eingestellten 'bps' und 
	 * 'fps' Werten.
	 * Hack deshalb, weil kein anderer Encoder diese Werte so verwendet.
	 * (Evt. koennte es Regelungsrueckkopplung etc. geben)
	 */
	tcl.evalc("$fps_slider get");sscanf(tcl.result(),"%d",&xfps);
	tcl.evalc("$bps_slider get");sscanf(tcl.result(),"%d",&kbps);

	/* in bits/frame umrechnen */
	bps = kbps*1024/xfps;

	/* temporaeres Bild aus uebergebenen YUV Frame */
	thispic.w = w;
	thispic.h = h;
	thispic.y = (Byte*)yuv->bp_;
	thispic.u = (Byte*)yuv->bp_+w*h;
	thispic.v = (Byte*)yuv->bp_+w*h+(w*h)/4;

	/* Bitstream start to 0 */
	bitstr_.ind = 0;

	/* und GO */
	bitstr_.b[bitstr_.size/8-1] = 0x42;
	gettimeofday(&tv1,NULL);
	EncodeH263Q(
		q_,				/* [in] Quantisierer */
		codingtime_,			/* [in] Codingtime */
		nexttype_,			/* [in] Kodiertype */
		vf->ts_,			/* [in] Timestamp */
		5,				/* [in] Prozent Intra */
		&thispic,			/* [in] aktuelles Bild */
		&oldorigpic_,
		&oldpic_,			/* [in] vorher kodiertes Bild */
		(unsigned char*)yuv->crvec_,	/* [in] condit. replenishment */
		&decpict_,			/* [out] neues kodiertes Bild */
		&bitstr_,			/* [out] Bitstream */
		mbind_,				/* [out] MB Indizes */
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

	/* Markiere das Ende des Bitstreams */
	mbind_[mbs_per_gob*gobs_per_frame]=bitstr_.ind;

	/* Die eigentliche Splitfunktion */
	h263_rtp_packet	*packets = NULL;
	int nrofpackets = split_h263stream(
		bitstr_.b,			/* [in] bitstream */
		bitstr_.ind,			/* [in] Laenge in Bits */
		gobs_per_frame,
		mbs_per_gob,
		mbind_,
		mbquant_,
		mvfield_,
		yuv->ts_,			/* [in] timestamp */
		tx_->mtu(),			/* [in] aktuelle Packetgroesse*/
		&packets			/* [out] pakete */
	);
	/* Und hier verschicken wir die Pakete 
	 * Wir verwenden die 2 Standard iov Buffer, Buffer 1 nur fuer
	 * den RTP Header, Buffer 2 fuer den H263 Extraheader und die H263 Daten
	 * Der Grund dafuer ist, das VIC Decoder verschieden grosse Header nicht
	 * verstehen koennen (sie sind nur ueber den loopback richtig.)
	 */
	for (i=0;i<nrofpackets;i++) {
		Transmitter::pktbuf *pb = tx_->alloc(yuv->ts_,RTP_PT_H263);
		rtphdr *rh = (rtphdr*)pb->iov[0].iov_base;

		pb->iov[0].iov_len = sizeof(rtphdr);

		memcpy(pb->iov[1].iov_base,&(packets[i].header.h263a),packets[i].headersize);
		memcpy(pb->iov[1].iov_base+packets[i].headersize,packets[i].data,packets[i].datasize);
		pb->iov[1].iov_len = packets[i].datasize+packets[i].headersize;

		/* Markiere das letzte Paket */
		if (i==nrofpackets-1)
			rh->rh_flags |= htons(RTP_M);
		/* Sendfunktion */
		tx_->send(pb);
		sentbytes+=packets[i].headersize+packets[i].datasize+sizeof(rtphdr);
	}
	/* alle anderen Frames sind interkodiert mit prozentuellem intra MB 
	 * Anteil 
	 */
	nexttype_ = PICTURE_CODING_TYPE_INTER;
	/* Die gebufferten RTP Pakete flush()en, weil wir die Pakete
	 * invalidieren
	 */
	tx_->flush();
	free(packets);

	/* und STOP. */
	gettimeofday(&tv2,NULL);
	
	timespent = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec-tv1.tv_usec);
#ifdef TIME_ADAPTION
	if ((++maycheck_)>20) {
		maycheck_ = 0;
		/* Haben wir viel zu viel Zeit verbraucht? */
		if ((1000000/xfps*3)<timespent) {
			/* Yup, zuerst Bildgroesse drastisch verringern */
			if (w==CIF_WIDTH) {
				tcl.evalc("grabber decimate 4");
				tcl.evalc("set inputSize 4");
				fprintf(stderr,"TOO SLOW: shrunk to QCIF\n");
				return 0;
			} else {
				/* no more ways to do it YET */
			}
		}
		/* Haben wir viel zu wenig Zeit verbraucht? */
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
