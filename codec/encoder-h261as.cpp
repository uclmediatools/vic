
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "inet.h"
#include "net.h"
#include "rtp.h"
#include "dct.h"
#include "p64/p64-huff.h"
#include "bsd-endian.h"
#include "vic_tcl.h"
#include "crdef.h"
#include "transmitter.h"
#include "pktbuf-rtp.h"
#include "module.h"

#define HDRSIZE (sizeof(rtphdr) + 4)
#define DEFAULT_THRESHOLD 48
#define	CIF_WIDTH	352
#define	CIF_HEIGHT	288
#define	QCIF_WIDTH	176
#define	QCIF_HEIGHT	144
#define	BMB		6	/* # blocks in a MB */
#define MBPERGOB	33	/* # of Macroblocks per GOB */

#ifdef INT_64
#define NBIT 64
#define BB_INT INT_64
#else
#define NBIT 32
#define BB_INT u_int
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
#if NBIT == 64
#define STORE_BITS(bb, bc) \
	bc[0] = bb >> 56; \
	bc[1] = bb >> 48; \
	bc[2] = bb >> 40; \
	bc[3] = bb >> 32; \
	bc[4] = bb >> 24; \
	bc[5] = bb >> 16; \
	bc[6] = bb >> 8; \
	bc[7] = bb;
#define LOAD_BITS(bc) \
	((BB_INT)bc[0] << 56 | \
	 (BB_INT)bc[1] << 48 | \
	 (BB_INT)bc[2] << 40 | \
	 (BB_INT)bc[3] << 32 | \
	 (BB_INT)bc[4] << 24 | \
	 (BB_INT)bc[5] << 16 | \
	 (BB_INT)bc[6] << 8 | \
	 (BB_INT)bc[7])
#else
#define STORE_BITS(bb, bc) \
	bc[0] = bb >> 24; \
	bc[1] = bb >> 16; \
	bc[2] = bb >> 8; \
	bc[3] = bb;
#define LOAD_BITS(bc) (ntohl(*(BB_INT*)(bc)))
#endif
#else
#define STORE_BITS(bb, bc) *(BB_INT*)bc = (bb);
#define LOAD_BITS(bc) (*(BB_INT*)(bc))
#endif

#define PUT_BITS(bits, n, nbb, bb, bc) \
{ \
	nbb += (n); \
	if (nbb > NBIT)  { \
		u_int extra = (nbb) - NBIT; \
		bb |= (BB_INT)(bits) >> extra; \
		STORE_BITS(bb, bc) \
		bc += sizeof(BB_INT); \
		bb = (BB_INT)(bits) << (NBIT - extra); \
		nbb = extra; \
	} else \
		bb |= (BB_INT)(bits) << (NBIT - (nbb)); \
}


class H261ASEncoder : public TransmitterModule {
    public:
	H261ASEncoder();
	~H261ASEncoder();
	void setq(int q);
	int consume(const VideoFrame*);
    protected:
	int encode(const VideoFrame*, const u_int8_t *crvec);
	int command(int argc, const char*const* argv);
	void encode_blk(const short* blk, const char* lm);
	int flush(pktbuf* pb, int nbit, pktbuf* npb);
	char* make_level_map(int q, u_int fthresh);
	void setquantizers(int lq, int mq, int hq);
	void size(int w, int h);
	void encode_mb(u_int mba, const u_char* frm,
		       u_int loff, u_int coff, int how);

        u_int width;
        u_int height;
        u_char *frame_buffer;
        u_int frame_buffer_size;

        u_char *crref;
        u_char *crvec;

        int nblk;
        int blkw;
        int blkh;
        int rover;
        int threshold;
        int scan;

        u_char *bs;
        u_char *bc;
        int bb;
        int nbb;
        int sbit;
        int tx;
        int lq;
        int mq;
        int hq;
        u_int ngob;
        u_int nblocks;
        int mquant;
        int mba;

        int *loff_table;
        int *coff_table;
        int *blkno_table;

        float lqt[64];
        float mqt[64];
        float hqt[64];

        const char *llm[32];
        const char *clm[32];
};

static class H261ASEncoderMatcher : public Matcher {
    public:
	H261ASEncoderMatcher() : Matcher("module") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "h261as") == 0 ||
		    strcasecmp(fmt, "h.261as") == 0)
			return (new H261ASEncoder);
		return (0);
	}
} encoder_matcher_h261as;


H261ASEncoder::H261ASEncoder() : TransmitterModule(FT_YUV_420),
				 frame_buffer(0),
				 frame_buffer_size(0),
				 crref(0),
				 crvec(0),
				 nblk(0),
				 blkw(0),
				 blkh(0),
				 rover(0),
				 threshold(DEFAULT_THRESHOLD),
				 scan(0),
				 loff_table(0),
				 coff_table(0)
{
	for (int q = 0; q < 32; ++q) {
		llm[q] = 0;
		clm[q] = 0;
	}
	setq(10);
}

H261ASEncoder::~H261ASEncoder()
{
	for (int q = 0; q < 32; ++q) {
		if (llm[q] != 0)
			delete llm[q]; //SV-XXX: Debian
		if (clm[q] != 0)
			delete clm[q]; //SV-XXX: Debian
	}
}

/*
 * Set up the forward DCT quantization table for
 * INTRA mode operation.
 */
void
H261ASEncoder::setquantizers(int _lq, int _mq, int _hq)
{
	int qt[64];
        lq = _lq;
	if (lq > 31)
		lq = 31;
	if (lq <= 0)
		lq = 1;

        mq = _mq;
	if (mq > 31)
		mq = 31;
	if (mq <= 0)
		mq = 1;

        hq = _hq;
	if (hq > 31)
		hq = 31;
	if (hq <= 0)
		hq = 1;

        /*
         * Set the DC quantizer to 1, since we want to do this
         * coefficient differently (i.e., the DC is rounded while
         * the AC terms are truncated).
         */
        qt[0] = 1;
        int i;
        for (i = 1; i < 64; ++i)
          qt[i] = lq << 1;
        fdct_fold_q(qt, lqt);
        
        qt[0] = 1;
        for (i = 1; i < 64; ++i)
          qt[i] = mq << 1;
        fdct_fold_q(qt, mqt);
        
        qt[0] = 1;
        for (i = 1; i < 64; ++i)
          qt[i] = hq << 1;
        fdct_fold_q(qt, hqt);
}

void
H261ASEncoder::setq(int q)
{
	setquantizers(q, q / 2, 1);
}

void
H261ASEncoder::size(int w, int h)
{
        Module::size(w, h);
  
        if (w % 16 != 0) {
                w += 16 - (w % 16);
        }
        if (h % 16 != 0) {
                h += 16 - (h % 16);
        }

        width = w;
        height = h;

        if (frame_buffer) {
                delete [] frame_buffer;
        }
        frame_buffer_size = (width * height * 3)/2;
        frame_buffer = new u_char[frame_buffer_size];
        memset(frame_buffer, 127, frame_buffer_size);

        if (crref) {
                delete [] crref;
        }
        crref = new u_char[width * height];
        
        if (crvec) {
                delete [] crvec;
        }
        nblk = (width * height) >> 8;
        crvec = new u_char[nblk];

        blkw = width/16;
        blkh = height/16;

        nblocks = blkw * blkh;
        ngob = nblocks/33;
        if (ngob * 33 < nblocks) {
                ngob ++;
        }

        if (loff_table) {
                delete [] loff_table;
        }
        loff_table = new int[nblocks];
        if (coff_table) {
                delete [] coff_table;
        }
        coff_table = new int[nblocks];

        int i = 0;
        int loff = 0;
        int coff = 0;
        int l_rs = 15 * width;
        int c_rs = 7 * width/2;
        for (int y = 0; y < blkh; y ++) {
                for (int x = 0; x < blkw; x ++, i ++) {
                        loff_table[i] = loff;
                        coff_table[i] = coff;

                        loff += 16;
                        coff += 8;
                }

                loff += l_rs;
                coff += c_rs;
        }
}

int
H261ASEncoder::command(int argc, const char*const* argv)
{
	if (argc == 3 && strcmp(argv[1], "q") == 0) {
		setq(atoi(argv[2]));
		return (TCL_OK);
	}
	return (TransmitterModule::command(argc, argv));
}

/*
 * Make a map to go from a 12 bit dct value to an 8 bit quantized
 * 'level' number.  The 'map' includes both the quantizer (for the
 * dct encoder) and the perceptual filter 'threshhold' (for both
 * the pixel & dct encoders).  The first 4k of the map is for the
 * unfiltered coeff (the first 20 in zigzag order; roughly the
 * upper left quadrant) and the next 4k of the map are for the
 * filtered coef.
 */
char*
H261ASEncoder::make_level_map(int q, u_int fthresh)
{
	/* make the luminance map */
	char* lm = new char[0x2000];
	char* flm = lm + 0x1000;
	int i;
	lm[0] = 0;
	flm[0] = 0;
	q = 0;
	for (i = 1; i < 0x800; ++i) {
		int l = i;
		if (q)
			l /= q;
		lm[i] = l;
		lm[-i & 0xfff] = -l;

		if ((u_int)l <= fthresh)
			l = 0;
		flm[i] = l;
		flm[-i & 0xfff] = -l;
	}
	return (lm);
}

/*
 * encode_blk:
 *	encode a block of DCT coef's
 */
void
H261ASEncoder::encode_blk(const short* blk, const char* lm)
{
	BB_INT t_bb = bb;
	u_int t_nbb = nbb;
	u_char* t_bc = bc;

	/*
	 * Quantize DC.  Round instead of truncate.
	 */
	int dc = (blk[0] + 4) >> 3;

	if (dc <= 0)
		/* shouldn't happen with CCIR 601 black (level 16) */
		dc = 1;
	else if (dc > 254)
		dc = 254;
	else if (dc == 128)
		/* per Table 6/H.261 */
		dc = 255;
	/* Code DC */
	PUT_BITS(dc, 8, t_nbb, t_bb, t_bc);
	int run = 0;
	const u_char* colzag = &COLZAG[0];
	for (int zag; (zag = *++colzag) != 0; ) {
		if (colzag == &COLZAG[20])
			lm += 0x1000;
		int level = lm[((const u_short*)blk)[zag] & 0xfff];
		if (level != 0) {
			int val, nb;
			huffent* he;
			if (u_int(level + 15) <= 30 &&
			    (nb = (he = &hte_tc[((level&0x1f) << 6)|run])->nb))
				/* we can use a VLC. */
				val = he->val;
			else {
				 /* Can't use a VLC.  Escape it. */
				val = (1 << 14) | (run << 8) | (level & 0xff);
				nb = 20;
			}
			PUT_BITS(val, nb, t_nbb, t_bb, t_bc);
			run = 0;
		} else
			++run;
	}
	/* EOB */
	PUT_BITS(2, 2, t_nbb, t_bb, t_bc);

	bb = t_bb;
	nbb = t_nbb;
	bc = t_bc;
}

/*
 * H261ASEncoder::encode_mb
 *	encode a macroblock given a set of input YUV pixels
 */
void
H261ASEncoder::encode_mb(u_int mbal, const u_char* frm,
			    u_int loff, u_int coff, int how)
{
	register int q;
	float* qt;
	if (how == CR_MOTION) {
		q = lq;
		qt = lqt;
	} else if (how == CR_BG) {
		q = hq;
		qt = hqt; 
	} else {
		/* must be at age threshold */
		q = mq;
		qt = mqt; 
	}

	/*
	 * encode all 6 blocks of the macro block to find the largest
	 * coef (so we can pick a new quantizer if gquant doesn't have
	 * enough range).
	 */
	/*XXX this can be u_char instead of short but need smarts in fdct */
	short blk[64 * 6];
	register int stride = width;
	/* luminance */
	const u_char* p = &frm[loff];
	fdct(p, stride, blk + 0, qt);
	fdct(p + 8, stride, blk + 64, qt);
	fdct(p + 8 * stride, stride, blk + 128, qt);
	fdct(p + (8 * stride + 8), stride, blk + 192, qt);
	/* chominance */
	int fs = stride * height;
	p = &frm[fs + coff];
	stride >>= 1;
	fdct(p, stride, blk + 256, qt);
	fdct(p + (fs >> 2), stride, blk + 320, qt);

	/*
	 * if the default quantizer is too small to handle the coef.
	 * dynamic range, spin through the blocks and see if any
	 * coef. would significantly overflow.
	 */
	if (q < 8) {
		register int cmin = 0, cmax = 0;
		register short* bp = blk;
		for (register int i = 6; --i >= 0; ) {
			++bp;	// ignore dc coef
			for (register int j = 63; --j >= 0; ) {
				register int v = *bp++;
				if (v < cmin)
					cmin = v;
				else if (v > cmax)
					cmax = v;
			}
		}
		if (cmax < -cmin)
			cmax = -cmin;
		if (cmax >= 128) {
			/* need to re-quantize */
			register int s;
			for (s = 1; cmax >= (128 << s); ++s) {
			}
			q <<= s;
			register short* bp = blk;
			for (register int i = 6; --i >= 0; ) {
				++bp;	// ignore dc coef
				for (register int j = 63; --j >= 0; ) {
					register int v = *bp;
					*bp++ = v >> s;
				}
			}
		}
	}

	u_int m = mbal - mba;
	mba = mbal;
	huffent* he = &hte_mba[m - 1];
	/* MBA */
	PUT_BITS(he->val, he->nb, nbb, bb, bc);
	if (q != mquant) {
		/* MTYPE = INTRA + TC + MQUANT */
		PUT_BITS(1, 7, nbb, bb, bc);
		PUT_BITS(q, 5, nbb, bb, bc);
		mquant = q;
	} else {
		/* MTYPE = INTRA + TC (no quantizer) */
		PUT_BITS(1, 4, nbb, bb, bc);
	}

	/* luminance */
	const char* lm = llm[q];
	if (lm == 0) {
		lm = make_level_map(q, 1);
		llm[q] = lm;
		clm[q] = make_level_map(q, 2);
	}
	encode_blk(blk + 0, lm);
	encode_blk(blk + 64, lm);
	encode_blk(blk + 128, lm);
	encode_blk(blk + 192, lm);
	/* chominance */
	lm = clm[q];
	encode_blk(blk + 256, lm);
	encode_blk(blk + 320, lm);
}

int
H261ASEncoder::flush(pktbuf* pb, int nbit, pktbuf* npb)
{
	/* flush bit buffer */
	STORE_BITS(bb, bc);

	int cc = (nbit + 7) >> 3;
	int ebit = (cc << 3) - nbit;

	/*XXX*/
	if (cc == 0 && npb != 0)
		abort();

	pb->len = cc + HDRSIZE;
	rtphdr* rh = (rtphdr*)pb->data;
	if (npb == 0)
		rh->rh_flags |= htons(RTP_M);

	int h = *(u_int*)(rh + 1) | ebit << 26;
	*(u_int*)(rh + 1) = htonl(h);

	if (npb != 0) {
		bs = &npb->data[HDRSIZE];
		bc = bs;
		bb = 0;
		nbb = 0;
		sbit = 0;
	}
	tx_->send(pb);

	return (cc + HDRSIZE);
}

int H261ASEncoder::consume(const VideoFrame *vf)
{
	if (!samesize(vf))
		size(vf->width_, vf->height_);

	YuvFrame* p = (YuvFrame*)vf;
	return(encode(p, p->crvec_));
}
		

int
H261ASEncoder::encode(const VideoFrame* vf, const u_int8_t *crvec)
{
	tx_->flush();

	pktbuf* pb = pool_->alloc(vf->ts_, RTP_PT_H261AS);
	bs = &pb->data[HDRSIZE];
	bc = bs;
	u_int ec = (tx_->mtu() - HDRSIZE) << 3;
	bb = 0;
	nbb = 0;

	/* RTP/H.261 header */
	rtphdr* rh = (rtphdr*)pb->data;
	*(u_int*)(rh + 1) = lq << 24 | ((width >> 4) - 1) << 12 | ((height >> 4) - 1);

	int cc = 0;
	u_int8_t* frm = vf->bp_;
	u_int nbit;
	u_int blkno;

	for (u_int gob = 0; gob < ngob; gob ++) {

		blkno = 33 * gob;

		/* GSC/GN */
		PUT_BITS(0x0001, 16, nbb, bb, bc);
		PUT_BITS(gob, 20, nbb, bb, bc);

		/* GQUANT */
		mquant = lq;
		PUT_BITS(mquant, 5, nbb, bb, bc);
		mba = 0;


		for (u_int mbal = 1; mbal <= 33 && blkno < nblocks; ++mbal, blkno++) {
			/*
			 * If the conditional replenishment algorithm
			 * has decided to send any of the blocks of
			 * this macroblock, code it.
			 */
			u_int s = crvec[blkno];

			if ((s & CR_SEND) != 0) {

				nbit = ((bc - bs) << 3) + nbb;
				encode_mb(mbal, 
					  frm, 
					  loff_table[blkno], 
					  coff_table[blkno], 
					  CR_STATE(crvec[blkno]));
				u_int32_t cbits = ((bc - bs) << 3) + nbb;
				if (cbits > ec) {
					pktbuf *npb;
					npb = pool_->alloc(vf->ts_, 
							   RTP_PT_H261AS);
					cc += flush(pb, 
						    nbit, 
						    npb);
					pb = npb;
	  
					bs = &pb->data[HDRSIZE];
					bc = bs;
					bb = 0;
					nbb = 0;
					sbit = 0;

					rtphdr* rh = (rtphdr*)pb->data;
					*(u_int*)(rh + 1) = lq << 24 | ((width >> 4) - 1) << 12 | ((height >> 4) - 1);



					PUT_BITS(0x0001, 16, nbb, bb, bc);
					PUT_BITS(gob, 20, nbb, bb, bc);
					PUT_BITS(mquant, 5, nbb, bb, bc);

					mba = 0;
					encode_mb(mbal, 
						  frm, 
						  loff_table[blkno], 
						  coff_table[blkno], 
						  CR_STATE(crvec[blkno]));
				} 
			}
		}
	}

	cc += flush(pb, ((bc - bs) << 3) + nbb, 0);

	return (cc);
}
