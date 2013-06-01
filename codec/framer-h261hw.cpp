/*
 * This code is derived from the P64 software implementation by the
 * Stanford PVRG group:
 * 
 * Copyright (C) 1990, 1991, 1993 Andy C. Hung, all rights reserved.
 * PUBLIC DOMAIN LICENSE: Stanford University Portable Video Research
 * Group. If you use this software, you agree to the following: This
 * program package is purely experimental, and is licensed "as is".
 * Permission is granted to use, modify, and distribute this program
 * without charge for any purpose, provided this license/ disclaimer
 * notice appears in the copies.  No warranty or maintenance is given,
 * either expressed or implied.  In no event shall the author(s) be
 * liable to you or a third party for any special, incidental,
 * consequential, or other damages, arising out of the use or inability
 * to use the program for any purpose (or the loss of data), even if we
 * have been advised of such possibilities.  Any public reference or
 * advertisement of this source code should refer to it as the Portable
 * Video Research Group (PVRG) code, and not by any author(s) (or
 * Stanford University) name.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "bsd-endian.h"
#include "p64/p64-huff.h"
#include "transmitter.h"
#include "module.h"

#define IT_QCIF	0
#define IT_CIF	1

#define MBPERGOB	33

#if BYTE_ORDER == LITTLE_ENDIAN
#define HUFFRQ(bs, bb) \
 { \
	register int t = *bs++; \
	bb <<= 16; \
	bb |= (t & 0xff) << 8; \
	bb |= t >> 8; \
}
#else
#define HUFFRQ(bs, bb) \
 { \
	bb <<= 16; \
	bb |= *bs++; \
}
#endif

#define MASK(s) ((1 << (s)) - 1)

#define HUFF_DECODE(bs, ht, nbb, bb, result) { \
	register int s__, v__; \
 \
	if (nbb < 16) { \
		HUFFRQ(bs, bb); \
		nbb += 16; \
	} \
	s__ = ht.maxlen; \
	v__ = (bb >> (nbb - s__)) & MASK(s__); \
	s__ = (ht.prefix)[v__]; \
	nbb -= (s__ & 0x1f); \
	result = s__ >> 5; \
 }

#define GET_BITS(bs, n, nbb, bb, result) \
{ \
	nbb -= n; \
	if (nbb < 0)  { \
		HUFFRQ(bs, bb); \
		nbb += 16; \
	} \
	(result) = ((bb >> nbb) & MASK(n)); \
}

#define SKIP_BITS(bs, n, nbb, bb) \
{ \
	nbb -= n; \
	if (nbb < 0)  { \
		HUFFRQ(bs, bb); \
		nbb += 16; \
	} \
}


class H261hwFramer : public TransmitterModule {
 public:
	H261hwFramer();
	~H261hwFramer();
	virtual int consume(const VideoFrame*);
	virtual int command(int argc,const char *const *argv);
//	void setq(int x);
 protected:
	int parse_block();
	int parse_gob_hdr(int ebit);
	int parse_mb_hdr(unsigned int &);
	int decode_mb();
	int decode(const unsigned char *, int, int, int);

	int	mq_;
	struct hufftab {
		int maxlen;
		const short* prefix;
	};
	hufftab ht_mba_;
	hufftab ht_mvd_;
	hufftab ht_cbp_;
	hufftab ht_tcoeff_;
	hufftab ht_mtype_;
	int	nbb_;	// number of bits in buffer
	int	bb_;	// bitbuffer
	u_int	fmt_;	// format
	int	mt_;	// mtype
	int	mba_;	//macroblock address
	const u_short	*bs_;	// bit stream to read from...
	int	mvdv_,mvdh_;	//motion vector
	int	gob_;		//current gob

	const u_short* es_;     /* pointer to end if input stream */
	char	transmitter_;	// tclobject transmitter
};

static class H261hwFramerMatcher : public Matcher {
public:
	H261hwFramerMatcher() : Matcher("module") {}
	TclObject* match(const char* fmt) {
		//fprintf(stderr,"H261hwFramerMatcher::match(%s)\n",fmt);
		if (strcasecmp(fmt, "h261/hw") == 0)
			return (new H261hwFramer);
		return (0);
	}
} framer_matcher_h261hw;

H261hwFramer::H261hwFramer() : TransmitterModule(FT_H261)
{
	/*
	 * Set up the huffman tables.
	 */
	ht_mtype_.prefix = htd_mtype;
	ht_mtype_.maxlen = htd_mtype_width;
	ht_mba_.prefix = htd_mba;
	ht_mba_.maxlen = htd_mba_width;
	ht_mvd_.prefix = htd_dvm;
	ht_mvd_.maxlen = htd_dvm_width;
	ht_cbp_.prefix = htd_cbp;
	ht_cbp_.maxlen = htd_cbp_width;
	ht_tcoeff_.prefix = htd_tcoeff;
	ht_tcoeff_.maxlen = htd_tcoeff_width;
}

H261hwFramer::~H261hwFramer()
{
/*
	if (tx_ != 0)
		tx_->flush();
 */
}

/*
void
H261hwFramer::setq(int arg)
{
	//fprintf(stderr,"H261hwFramer::setq(%d)\n",arg);
}
*/

int
H261hwFramer::command(int argc,const char* const *argv)
{
/*	fprintf(stderr,"H261hwFramer::command(");
	for (int i=0;i<argc;i++)
		fprintf(stderr,"%s,",argv[i]);
	fprintf(stderr,")\n");
*/
	if (argc==3) {
		if (!strcasecmp(argv[1],"transmitter")) {
			
		}
		if (!strcasecmp(argv[1],"q")) {
			return 0;
		}
	}
	return TransmitterModule::command(argc,argv);
}

#define HDRSIZE (sizeof(rtphdr)+4)


/*
 * Decode the next block of transform coefficients
 * from the input stream.
 * Return number of non-zero ac coefficients.
 */
int H261hwFramer::parse_block()
{
	/*
	 * Cache bit buffer in registers.
	 */
	register int nbb = nbb_;
	register int bb = bb_;
	register const u_short *bs = bs_;

	if ((mt_ & MT_CBP) == 0) {
		SKIP_BITS(bs, 8, nbb, bb);
	} else if ((bb >> (nbb - 1)) & 1) {
		/*
		 * In CBP blocks, the first block present must be
		 * non-empty (otherwise it's mask bit wouldn't
		 * be set), so the first code cannot be an EOB.
		 * CCITT optimizes this case by using a huffman
		 * table equivalent to ht_tcoeff_ but without EOB,
		 * in which 1 is coded as "1" instead of "11".
		 * We grab two bits, the first bit is the code
		 * and the second is the sign.
		 */
		SKIP_BITS(bs, 2, nbb, bb);
	}
	int nc = 0;
	for (;;) {
		register int r;
		HUFF_DECODE(bs, ht_tcoeff_, nbb, bb, r);
		if (r <= 0) {
			/* SYM_EOB, SYM_ILLEGAL, or SYM_ESCAPE */
			if (r == SYM_ESCAPE) {
				SKIP_BITS(bs, 14, nbb, bb);
			} else {
				if (r == SYM_ILLEGAL) {
					bb_ = bb;
					nbb_ = nbb;
					bs_ = bs;
				}
				/* EOB */
				break;
			}
		}
	}
	/*
	 * Done reading input.  Update bit buffer.
	 */
	bb_ = bb;
	nbb_ = nbb;
	bs_ = bs;
	return (nc);
}

/*
 * Parse a GOB header, which consists of the GOB quantiation
 * factor (GQUANT) and spare bytes that we ignore.
 */
int H261hwFramer::parse_gob_hdr(int ebit)
{
	mba_ = -1;
	mvdh_ = 0;
	mvdv_ = 0;

	/*
	 * Get the next GOB number (or 0 for a picture header).
	 * The invariant at the top of this loop is that the
	 * bit stream is positioned immediately past the last
	 * start code.
	 */
	int gob;
	for (;;) {
		u_int	tempref;
		GET_BITS(bs_, 4, nbb_, bb_, gob);
		if (gob != 0)
			break;
		/*
		 * should happen only on first iteration
		 * (if at all).  pictures always start on
		 * packet boundaries per section 5 of the
		 * Internet Draft.
		 */

		/* throw away the temporal reference */
		GET_BITS(bs_, 5, nbb_, bb_, tempref);
		/* Picture Type */
		int pt;
		GET_BITS(bs_, 6, nbb_, bb_, pt);
		u_int fmt = (pt >> 2) & 1;
		if (fmt_ != fmt) {
			/* change formats */
			fmt_ = fmt;
		}
		/* Extra information PEI */
		int v;
		GET_BITS(bs_, 1, nbb_, bb_, v);
		while (v != 0) {
			u_int	ei;
			GET_BITS(bs_, 8 , nbb_, bb_, ei);
			GET_BITS(bs_, 1, nbb_, bb_, v);
		}
		/*
		 * Check to see that the next 16 bits
		 * are a start code and throw them away.
		 * But first check that we have the bits.
		 */
		int nbit = ((es_ - bs_) << 4) + nbb_ - ebit;
		if (nbit < 20)
			return (0);

		GET_BITS(bs_, 16, nbb_, bb_, v);
		if (v != 0x0001) {
			fprintf(stderr,"[bad start code %04x]", v);
			return (-1);
		}
	}
	gob -= 1;
	if (fmt_ == IT_QCIF)
		/*
		 * Number QCIF GOBs 0,1,2 instead of 0,2,4.
		 */
		gob >>= 1;

	/* MQUANT */ 
	GET_BITS(bs_, 5, nbb_, bb_, mq_);

	/* GEI Gob Extra Information */
	int v;
	GET_BITS(bs_, 1, nbb_, bb_, v);
	while (v != 0) {
		SKIP_BITS(bs_, 8, nbb_, bb_);
		GET_BITS(bs_, 1, nbb_, bb_, v);
	}
	gob_ = gob;
	return (gob);
}

/*
 * Parse a macroblock header.  If there is no mb header because
 * we hit the next start code, return -1, otherwise 0.
 */
int H261hwFramer::parse_mb_hdr(u_int& cbp)
{
	register u_int	omt = mt_;
	/*
	 * Read the macroblock address (MBA)
	 */
	int v;
	HUFF_DECODE(bs_, ht_mba_, nbb_, bb_, v);
	if (v <= 0) {
		/*
		 * (probably) hit a start code; either the
		 * next GOB or the next picture header.
		 * If we got MBA stuffing (0) we need to return
		 * so the outer loop can check if we're at the
		 * end of the buffer (lots of codecs put stuffing
		 * at the end of a picture to byte align the psc).
		 */
		return (v);
	}

	/*
	 * MBA is differentially encoded.
	 */
	mba_ += v;
	if (mba_ >= MBPERGOB) {
		fprintf(stderr,"[mba too big %d]", mba_);
		return (SYM_ILLEGAL);
	}

	HUFF_DECODE(bs_, ht_mtype_, nbb_, bb_, mt_);
	if (mt_ & MT_MQUANT) {
		GET_BITS(bs_, 5, nbb_, bb_, mq_);
	}
	if (mt_ & MT_MVD) {
		int	dh,dv;
		/*
		 * Read motion vector.
		 */
		HUFF_DECODE(bs_, ht_mvd_, nbb_, bb_, dh);
		HUFF_DECODE(bs_, ht_mvd_, nbb_, bb_, dv);
		/*
		 * Section 4.2.3.4
		 * The vector is differentially coded unless any of:
		 *   - the current mba delta isn't 1
		 *   - the current mba is 1, 12, or 23 (mba mod 11 = 1)
		 *   - the last block didn't have motion vectors.
		 *
		 * This arithmetic is twos-complement restricted
		 * to 5 bits.
		 */
		if ((omt&MT_MVD)!=0 && v==1 && mba_!=0 && mba_!=11 && mba_!=22){
			dh += mvdh_;
			dv += mvdv_;
		}
		mvdh_ = (dh << 27) >> 27;
		mvdv_ = (dv << 27) >> 27;
	} else {
		mvdh_ = mvdv_ = 0;
	}
	/*
	 * Coded block pattern.
	 */
	if (mt_ & MT_CBP) {
		HUFF_DECODE(bs_, ht_cbp_, nbb_, bb_, cbp);
		if (cbp > 0x3f) {
			fprintf(stderr,"[cbp invalid %x]", cbp);
			return (SYM_ILLEGAL);
		}
	} else
		cbp = 0x3f;

	return (1);
}


/*
 * Decompress the next macroblock.  Return 0 if the macroblock
 * was present (with no errors).  Return SYM_STARTCODE (-1),
 * if there was no macroblock but instead the start of the
 * next GOB or picture (in which case the start code has
 * been consumed).  Return SYM_ILLEGAL (-2) if there was an error.
 */
/*
 * Handle the next block in the current macroblock.
 * If tc is non-zero, then coefficients are present
 * in the input stream and they are parsed.  Otherwise,
 * coefficients are not present, but we take action
 * according to the type macroblock that we have.
 */
int H261hwFramer::decode_mb()
{
	u_int cbp;
	register int v;

	if ((v = parse_mb_hdr(cbp)) <= 0)
		return (v);

	/*
	 * Decode the six blocks in the MB (4Y:1U:1V).
	 * (This code assumes MT_TCOEFF is 1.)
	 */
	register u_int tc = mt_ & MT_TCOEFF;

	if(tc & (cbp >> 5)) parse_block();
	if(tc & (cbp >> 4)) parse_block();
	if(tc & (cbp >> 3)) parse_block();
	if(tc & (cbp >> 2)) parse_block();
	if(tc & (cbp >> 1)) parse_block();
	if(tc & (cbp >> 0)) parse_block();
	return (0);
}

/*
 * Decode H.261 stream.  Decoding can begin on either
 * a GOB or macroblock header.  All the macroblocks of
 * a given frame can be decoded in any order, but chunks
 * cannot be reordered across frame boundaries.  Since data
 * can be decoded in any order, this entry point can't tell
 * when a frame is fully decoded (actually, we could count
 * macroblocks but if there is loss, we would not know when
 * to sync).  Instead, the callee should sync the decoder
 * by calling the sync() method after the entire frame 
 * has been decoded (modulo loss).
 *
 * This routine should not be called with more than
 * one frame present since there is no callback mechanism
 * for renderering frames (i.e., don't call this routine
 * with a buffer that has a picture header that's not
 * at the front).
 */
int H261hwFramer::consume(const VideoFrame* vf)
{
	H261Frame*	p = (H261Frame*)vf;
	int		i;
	int		cc = p->len_;
	int		sentbytes = 0;
	char		*frameend;

	pktbuf* pb = pool_->alloc(p->ts_, RTP_PT_H261);
	/* RTP/H.261 header */
	rtphdr*		rh = (rtphdr*)pb->data;
	*(u_int*)(rh + 1) = (1<<24) | (mq_ << 10);
	char		*lastaddr,*lastsentaddr;
	u_int		lastmt,lastgob,lastmquant,lastmba,lastbit,lastsentbit;
	int		lastmvdv,lastmvdh;
	u_int		ec = tx_->mtu() - HDRSIZE ; // payload byte
	int		stuffzero;
	struct {
		int	mask;
		int	comp;
	} tab[] = {
		{0xff,0x01},
		{0xfe,0x02},
		{0xfc,0x04},
		{0xf8,0x08},
		{0xf0,0x10},
		{0xe0,0x20},
		{0xc0,0x40},
		{0x80,0x80}
	};

	tx_->flush();
	bs_ = (u_short*)p->bp_;
	es_ = (u_short*)(p->bp_ + ((cc - 1) &~ 1));

	frameend = (char*)p->bp_+cc-1;

	/*
	 * If input buffer not aligned, prime bit-buffer
	 * with 8 bits; otherwise, prime it with a 16.
	 */
	lastaddr	= (char*)p->bp_;
	lastsentaddr	= (char*)p->bp_;
	lastmquant	= 0;
	lastbit 	= 0;
	lastsentbit 	= 0;
	lastmvdv	= lastmvdh = 0;
	stuffzero	= 0;
	if ((int)p->bp_ & 1) {
		bs_ = (u_short*)(p->bp_ + 1);
		bb_ = *p->bp_;
		for (i=0;i<8;i++) {
			if (((*(u_char*)bs_) & tab[i].mask) == tab[i].comp)
				break;
		}
		/* 0000 0000 00X0 0000
		 *           7654 3210
		 */
		lastsentbit  = 8-i;
		if (lastsentbit==8)
			lastsentbit=0;
		else
			stuffzero=1;
		nbb_= 8+i;
	} else {
		bs_ = (u_short*)p->bp_;
		bb_ = 0;
		for (i=0;i<8;i++) {
			if (((*(((u_char*)bs_)+1)) & tab[i].mask) == tab[i].comp)
				break;
		}
		nbb_= i; // already in bitbuffer...so we read 0001 as start.
		lastsentbit  = 8-i;
		if (lastsentbit==8)
			lastsentbit = 0;
		else
			stuffzero=1;
		/* 0000 0000 00X0 0000
		 *           7654 3210
		 */
	}
	gob_ = 0; // will be reread before doing anything serious
	while (bs_ < es_ || (bs_ == es_ && nbb_ > 0)) {
		int v = decode_mb();

		if ((char*)bs_ - (char*)lastsentaddr >= ec-1) {
			u_int	cc = lastaddr-lastsentaddr+1;

			if (stuffzero) {
				//*(pb->iov[1].iov_base)=0;
				pb->data[HDRSIZE]=0;
				//memcpy(pb->iov[1].iov_base+1,lastsentaddr,cc);
				memcpy(&pb->data[HDRSIZE+1],lastsentaddr,cc);
				//pb->iov[1].iov_len = cc+1;
				pb->len = cc+HDRSIZE+1;
				stuffzero = 0;
			} else {
				//memcpy(pb->iov[1].iov_base,lastsentaddr,cc);
				memcpy(&pb->data[HDRSIZE],lastsentaddr,cc);
				//pb->iov[1].iov_len = cc;
				pb->len = cc+HDRSIZE;
			}
			//pb->iov[0].iov_len = HDRSIZE;
			u_int h=(*(u_int*)(rh+1))|(lastbit<<26)|(lastsentbit<<29);
			*(u_int*)(rh+1) = htonl(h);
			tx_->send(pb);
			sentbytes+= HDRSIZE+cc;
			pb = pool_->alloc(vf->ts_, RTP_PT_H261);
			//rh = (rtphdr*)pb->iov[0].iov_base;
			rh = (rtphdr*)pb->data;
			*(u_int*)(rh + 1) =	(lastgob << 20)	|
						(lastmba << 15)	|
						(mq_ << 10);
			/* motion vector data: */
			*(u_int*)(rh+1)|=1<<24; /* M Bit */
			if (lastmt & MT_MVD) {
				if (lastmvdv>=0)
					*(u_int*)(rh+1)|=lastmvdv;
				else
					*(u_int*)(rh+1)|=(32+lastmvdv);

				if (lastmvdh>=0)
					*(u_int*)(rh+1)|=lastmvdh<<5;
				else
					*(u_int*)(rh+1)|=(32+lastmvdh)<<5;
			}
			lastsentbit = lastbit;
			lastsentaddr = lastaddr;
			if (!lastbit) {
				lastsentaddr++;
			} else {
				lastsentbit = 8-lastsentbit;
			}
		}
		if (v == 0) {
			// store the current address as GOOD latest address.

			// lastaddr is the last byte read from the bytestream.
			lastaddr = ((char*)bs_) - ((nbb_+8) >> 3);
			// how much bits do we have NOT used from the last byte.
			lastbit = nbb_&7;

			lastmquant	= mq_;
			lastmvdv	= mvdv_;
			lastmvdh	= mvdh_;
			lastmba		= mba_;
			lastgob		= gob_+1;
			lastmt		= mt_;
			continue;
		}
		if (v != SYM_STARTCODE) {
			fprintf(stderr,"[expected GOB startcode,v=%d]",v);
			break;/*return (0);*/
		}
		gob_ = parse_gob_hdr(lastsentbit);
		lastgob = gob_;
		lastmba	= 0;
		lastmvdv= lastmvdh = 0;
		lastmt	= mt_;
		if (gob_ < 0) {
			fprintf(stderr,"[GOB < 0]");
			return (0);
		}
	}
	/* just dump the rest... don't care about cc and bitshifts,
	 * but trust the grabbing device... */
	cc = frameend-lastsentaddr+1;

	//pb->iov[0].iov_len = HDRSIZE;
	//memcpy(pb->iov[1].iov_base,lastsentaddr,cc);
	memcpy(&pb->data[HDRSIZE],lastsentaddr,cc);
	//pb->iov[1].iov_len = cc;
	pb->len = cc+HDRSIZE;
	rh->rh_flags |= htons(RTP_M);
	u_int	h = *(u_int*)(rh+1)|((nbb_&7)<<26) | (lastsentbit<<29);
	*(u_int*)(rh+1) = htonl(h);
	tx_->send(pb);
	sentbytes+= HDRSIZE+cc;
	//tx_->flush();
	return sentbytes;
}
