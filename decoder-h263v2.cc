#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "renderer.h"

#include "libh263.h"

class H263Decoder : public Decoder {
public:
    H263Decoder();
    virtual ~H263Decoder();
    virtual void info(char* wrk) const;
    virtual void recv(const rtphdr* rh, const u_char* bp, int cc);
    int colorhist(u_int* hist) const;
    virtual void stats(char* wrk);
protected:
    void decode(const u_char* vh, const u_char* bp, int cc);
    virtual void redraw();

    /* for ucb h263 decoder */
    struct H263_STATE *decoder_state;

    /* packet statistics */
    u_int16_t  last_seq; /* sequence number */

    /* collecting data for a frame */
    char       bitstream[65536];   /* bitstream data */
    int        b_tr;               /* temporal reference */
    int        b_ts;               /* timestamp */
    int        b_off;              /* how much bitstream data we have */
    int        b_all;              /* got all packets for so far? */

    /* image */
    u_char     xxx_frame[160000];
};

static class H263DecoderMatcher : public Matcher {
    public:
	H263DecoderMatcher() : Matcher("decoder") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "h263") == 0)
			return (new H263Decoder());
		return (0);
	}
} dm_h263;

#define STAT_BAD_PSC	0
#define STAT_BAD_GOB	1
#define STAT_BAD_SYNTAX	2
#define STAT_BAD_FMT	3
#define STAT_FMT_CHANGE 4	/* # times fmt changed */
#define STAT_BAD_HEADER 5


H263Decoder::H263Decoder() : Decoder(2) /* , codec_(0), */ 
{
        /* no stats yet... */
#if 0
	stat_[STAT_BAD_PSC].name = "H261-Bad-PSC";
	stat_[STAT_BAD_GOB].name = "H261-Bad-GOB";
	stat_[STAT_BAD_SYNTAX].name = "H261-Bad-Syntax";
	stat_[STAT_BAD_FMT].name = "H261-Bad-fmt";
	stat_[STAT_FMT_CHANGE].name = "H261-Fmt-change";
	stat_[STAT_BAD_HEADER].name = "H261-Bad-Header";
	nstat_ = 6;
#endif

	decimation_ = 411;
	/*
	 * Assume CIF.  Picture header will trigger a resize if
	 * we encounter QCIF instead.
	 */
	inw_ = 0;
	inh_ = 0;

	/*XXX*/
	resize(inw_, inh_);

	decoder_state = h263_start();
}

H263Decoder::~H263Decoder()
{
        h263_stop(decoder_state);
}

void H263Decoder::info(char* wrk) const
{
	/* nop */
}

void H263Decoder::stats(char* wrk)
{
#if 0
	/* pull stats out of vic indepdendent P64Decoder */
	setstat(STAT_BAD_PSC, codec_->bad_psc());
	setstat(STAT_BAD_GOB, codec_->bad_GOBno());
	setstat(STAT_BAD_SYNTAX, codec_->bad_bits());
	setstat(STAT_BAD_FMT, codec_->bad_fmt());
#endif
	Decoder::stats(wrk);
}

int H263Decoder::colorhist(u_int* hist) const
{
#if 0
	const u_char* frm = codec_->frame();
	int w = inw_;
	int h = inh_;
	int s = w * h;
	colorhist_411_556(hist, frm, frm + s, frm + s + (s >> 2), w, h);
#endif
	return (1);
}

extern void
dump_paket(const unsigned char *data, int len);

void H263Decoder::recv(const rtphdr* rh, const u_char* bp, int cc)
{
    /* RTP header  */
    int mbit  = ntohs(rh->rh_flags) >> 7 & 1;
    int seq   = ntohs(rh->rh_seqno);
    int ts    = ntohl(rh->rh_ts);

    /* H.263 payload */
    u_char *pl = (u_char*)(rh + 1);
    /* 5 bits reserved */
    int pbit  = (pl[0] >> 2) & 1;          /* 1 bit */
    int vbit  = (pl[0] >> 1) & 1;          /* 1 bit */
    int plen  = ((pl[0] << 5) & 0x20) |    /* 6 bits */
	        ((pl[1] >> 3) & 0x1f);
    int pebit = pl[1] & 0x07;              /* 3 bits */

    int     bs    = 0;
    int     tid   = 0;
    int     trun  = 0;
    int     tsync = 0;
    int     psc   = 0;
    int     size;
    u_char* bts;

    if (vbit) {
	tid   = (bp[bs] >> 5) & 7;   /* 3 bits */
	trun  = (bp[bs] >> 1) & 15;  /* 4 bits */
	tsync = (bp[bs]     ) & 1;   /* 1 bit  */
	bs++;
    }

    /* H.263 bitstream */
    if (pbit && (bp[bs] & 0xfc) == 0x80) {
	/* start of a new frame */
	psc   = 1;
	b_tr  = (bp[bs] << 6) & 0xc0 | (bp[bs+1] >> 2) & 0x3f;
	b_ts  = ts;
	b_all = 1;

	bitstream[0] = 0;
	bitstream[1] = 0;
	b_off = 2;
    }

#if 1
    /* DEBUG: dump packet data */
    fprintf(stderr,
	    "recv-h263: cc=%4d  "
	    "%s #0x%x ts=0x%x  "
	    "plen=%d pe=%d v=%d p=%d  "
	    "%s tr=%d\n",
	    cc,
	    mbit?"M":"-",seq,ts,
	    plen,pebit,vbit,pbit,
	    psc?"P":"-",b_tr);
    /* dump_paket((const unsigned char *)rh,64); */
#endif
    
    if (b_ts == ts && (psc || (last_seq+1 == seq))) {
	/* copy packet */
	memcpy(bitstream+b_off,bp,cc-bs);
	b_off += cc-bs;
    } else {
	/* oops - missing packet */
	b_all = 0;
    }
    last_seq = seq;

    if (mbit && b_all) {
	/* yea! have a complete frame */
	memcpy(bitstream+b_off,"\x00\x00\x80",3); /* add PSC */
	if (-1 != h263_decode_frame(decoder_state,bitstream)) {
	    if (decoder_state->width != inw_) {
		fprintf(stderr,"recv-h263: resize to %dx%d\n",
			decoder_state->width,decoder_state->height);
		resize(decoder_state->width, decoder_state->height);
		h263_stop(decoder_state);
		decoder_state = h263_start();
		h263_decode_frame(decoder_state,bitstream);
	    }

	    /* FIXME: avoid that memcpy -- hmm, not that easy :-( */
	    size = decoder_state->width*decoder_state->height;
	    memcpy(xxx_frame,            decoder_state->frame[0],size);
	    memcpy(xxx_frame + size,     decoder_state->frame[1],size/4);
	    memcpy(xxx_frame + size*5/4, decoder_state->frame[2],size/4);
#if 0
	    /* mark all changed */    
	    bts = rvts_;
	    for (int k = nblk_; --k >= 0; bts++)
		*bts = now_;
	    render_frame(xxx_frame);
#else
	    Decoder::redraw(xxx_frame);
#endif
	}
    }
}

void H263Decoder::redraw()
{
    if (decoder_state != 0)
	Decoder::redraw(xxx_frame);
}
