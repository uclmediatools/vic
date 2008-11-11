/*
 * Copyright (c) 1996 The Regents of the University of California.
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
 * 	This product includes software developed by the MASH Research
 *	Group at the University of California, Berkeley and the Network
 *	Research Group at Lawrence Berkeley National Laboratory.
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$";
#endif

#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "vic_tcl.h"
#include "dct.h"
#include "huffman.h"
extern "C"
{
#include "pvh-huff.h"
}
#include "config.h"
#include "rtp.h"
#include "pvh.h"
#include "crdef.h"
#include "module.h"
#include "transmitter.h"
#include "pktbuf-rtp.h"

#ifdef HAVE_HH
#define NC (4*64)
#else
#define NC (3*64)
#endif

struct bit_counter {
	int zt[7];
	int sbc[7];
	int dc;
	int mba;
	int vlc;
	int esc;
	int mq;
	int sing;
	int refine;
	int layout;
} cntr_;

#ifdef notnow
#define BUMP(fld, n) cntr_.fld += (n);
#else
#define BUMP(fld, n)
#endif

#define PVH_PUT_BITS(bits, n, nbb, bb, bs, fld) \
{ \
	PUT_BITS(bits, n, nbb, bb, bs); \
	BUMP(fld, n); \
}

/*
 * Conditional-free macro to translate a 32-bit signed quantity
 * in the range [-127,127] into an 8-bit sign/magnitude form.
 */
#define SM8(t) (((t) - (((t) >> 31 & (t)) << 1)) | ((t) & 0x80))

/*
 * Saturate 32-bit signed integer to [-127,+127] (NOT [-128,+127])
 */
#define SM8_SAT(v) \
{ \
	if ((((v) + 128) | ((v) + 127)) & ~0xff) \
		(v) = ((v) < -127) ? -128 : 127; \
}

#define BB_NBIT(lp) ((((lp)->bs - &(lp)->pb->data[HLEN]) << 3) + (lp)->nbb)

struct pvh_layer {
	/* bit buffer */
	int nbb;
	u_int bb;
	u_char* bs;
	u_char* es;
	pktbuf* pb;
	int layer;		/* network layer */
};

/*XXX make this dynamic */
#define NSH 8
#define NT 4

class PvhEncoder : public TransmitterModule {
 public:
	PvhEncoder();
	~PvhEncoder();
    int consume(const VideoFrame*);
	virtual int command(int argc, const char*const* argv);
 protected:
	void size(int w, int h);
	void getpkt(int spatial_layer, int sblk);
	int check_buffers(int sblk);
	void setup_layers(spatial_hierarchy*, int sblk);
	void flush(pvh_layer*, int sync);
	void flush();
	void encode_dct_base(pvh_layer*, int nb);
	void encode_dct_refinement(pvh_layer*, int bit, int nr);
	void encode_blk(pvh_layer*, const int16_t* blk);
	void refine_old(pvh_layer*, int off);
	void refine_new(pvh_layer*, int off);
	void encode_dct(spatial_hierarchy*);
	void encode_lum(const u_int8_t* in);
	void encode_chm(const u_int8_t* in);
	void decompose_block(const u_int8_t* in, u_int8_t* out);
	void build_tree_43(const u_int8_t* p);
	void build_tree_21(const u_int8_t* p);
	void encode_sbc(const spatial_hierarchy*, u_int8_t* sbc);
	void encode_sbc_base(pvh_layer*, u_int8_t* p, u_int m);
	void encode_sbc_refinement(pvh_layer*, u_int8_t* p, u_int m, int bit);
	void encode_tree(pvh_layer*, u_int m);
	void set_shs(int comp, int shid, const char* s);
	void encode_resumption_ptrs(pvh_layer* lp, int spatial_layer);
	void encode_bit_layout(spatial_hierarchy*);
	void encode_comp_layout(spatial_hierarchy*);
	int sh_refinement(spatial_hierarchy* sh, int layer) const;

	/* DC predictor */
#ifdef notyet
	int dc_;
#endif
	int blkno_;

	int tree_[5];/*XXX*/

	pvh_layer channels_[NLAYER];
	spatial_hierarchy shs_[NSH][NCOMP];
	spatial_hierarchy* sh_;
	int nlayer_;
	int layer_map_[NLAYER];
	void build_layer_map(spatial_hierarchy*);

	int shmap_[NT];		/* map from temporal index to SH number */

	void setq();

	float qt_[64];

	/*
	 * place to store current block's DCT refinement bits.
	 * XXX coef_ can be 8 instead of 16-bits but need smarts in fdct
	 */
	u_int16_t sc_[64];
	int16_t coef_[64];

	bit_counter cntr_;

	int cc_;
	u_int32_t ts_;
	int pt_;
};

static class PvhEncoderClass : public Matcher {
public:
	PvhEncoderClass() : Matcher("module") {}
	//TclClass("Module/VideoEncoder/Pixel/PVH") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "pvh") == 0)
			return (new PvhEncoder);
		return (0);
	}
} encoder_matcher_pvh;

PvhEncoder::PvhEncoder() : TransmitterModule(FT_PVH),
 sh_(0), pt_(RTP_PT_PVH)
{
//	bind("pt_", &pt_);
	memset(&cntr_, 0, sizeof(cntr_));
	memset(shs_, 0, sizeof(shs_));

	int i;
	for (i = 0; i < NLAYER; ++i)
		channels_[i].pb = 0;
	
	/*
	 * Set the DC quantizer to 1, since we want to do this
	 * coefficient differently (i.e., the DC is rounded while
	 * the AC terms are truncated).
	 */
	int qt[64];
	for (i = 0; i < 64; ++i)
		qt[i] = 1;

	fdct_fold_q(qt, qt_);

	setq();
}

PvhEncoder::~PvhEncoder()
{
}

/*
 * Compute the "layer map", which is a compact table mapping
 * a sequential range of spatial layer numbers onto the
 * (possibly) discontiguous range of network output channels.
 * "sh" is the array of spatial hierachy structures, one
 * for each component (LUM-DCT, LUM-SBC, and CHM-DCT)
 * XXX should only do this once at startup, not on each frame.
 * in this case, we'd have to keep an layer_map_ per shid.
 */
void PvhEncoder::build_layer_map(spatial_hierarchy* sh)
{
	int map[NLAYER];
	memset(map, 0, sizeof(map));
	int i;
	for (i = 0; i < NCOMP; ++i) {
		if (sh->base_channel >= 0)
			map[sh->base_channel] = 1;
		for (int k = 0; k < sh->n; ++k) {
			int L = sh->ref[k].channel;
			if (L >= 0)
				map[L] = 1;
		}
		++sh;
	}
	int n = 0;
	for (i = 0; i < NLAYER; ++i) {
		if (map[i])
			layer_map_[n++] = i;
	}
	nlayer_ = n;
}

/*
 * Given a spatial-hierarchy descriptor string, fill in the
 * spatial-hierarchy data structure corresponding to
 * component "comp" and entry number "shid".
 */
void PvhEncoder::set_shs(int comp, int shid, const char* s)
{
	spatial_hierarchy* sh = &shs_[shid][comp];
	int chan = 0;
	int first = 1;
	int n = 0;
	for (; *s != 0; ++s, ++chan) {
		if (*s == '-')
			continue;
		if (first) {
			first = 0;
			sh->base_channel = chan;
			sh->bq = *s - '0';
			if (*++s == 0)
				break;
			if (*s == '-')
				continue;
		}
		sh->ref[n].channel = chan;
		sh->ref[n].nr = *s - '0';
		++n;
	}
	sh->n = n;
}

void PvhEncoder::setq()
{
	/*XXX should be programmable from Tcl */

	shmap_[0] = 0;
	shmap_[1] = 1;
	shmap_[2] = 2;
	shmap_[3] = 1;

	set_shs(COMP_LUM_DCT, 0, "5-1--1-");
	set_shs(COMP_LUM_DCT, 1, "---511-");
	set_shs(COMP_LUM_DCT, 2, "--51-1-");

	set_shs(COMP_LUM_SBC, 0, "----4--");
	set_shs(COMP_LUM_SBC, 1, "----4--");
	set_shs(COMP_LUM_SBC, 2, "----4--");

	set_shs(COMP_CHM, 0, "-5---11");
	set_shs(COMP_CHM, 1, "---5-11");
	set_shs(COMP_CHM, 2, "--5--11");
}

#define HLEN (sizeof(rtphdr) + sizeof(pvhhdr))

void PvhEncoder::flush(pvh_layer* lp, int sync)
{
	/* flush bit buffer */
	HUFF_STORE_BITS(lp->bs, lp->bb);

	pktbuf* pb = lp->pb;
	int nbit = ((lp->bs - &pb->data[HLEN]) << 3) + lp->nbb;
	int cc = (nbit + 7) >> 3;
	int ebit = (cc << 3) - nbit;

	/*
	 * XXX
	 * shouldn't try to send zero-length packet
	 * unless we're just marking end of frame.
	 */
	if (cc == 0 && sync == 0)
		abort();

	pb->len = cc + HLEN;
	rtphdr* rh = (rtphdr*)pb->data;
	pvhhdr* ph = (pvhhdr*)(rh + 1);
	ph->ebit = ebit;
	ph->eblk = htons(blkno_);

	/*
	 * Set marker bit to indicate end of frame,
	 * but only spatial layer 0.
	 */
	int chan = lp - channels_;
	if (chan == layer_map_[0]) {
		ph->base = 1;
		if (sync)
			rh->rh_flags |= htons(RTP_M);
	} else
		ph->base = 0;

//	target_->recv(pb);
	tx_->send(pb);

	/* clear out packet buffer so we no longer own it */
	lp->pb = 0;

	/*XXX do rate control only on network layer 0 */
	if (chan == 0)
		cc_ += cc + sizeof(rtphdr) + sizeof(pvhhdr);
}

void PvhEncoder::flush()
{
	if (blkno_ < 0) {
		/*
		 * No blocks were coded (i.e., whole frame is static).
		 * Smash state so we just send a null pkt with the
		 * marker bit set.
		 * XXX this is a HACK
		 */
		nlayer_ = 1;
		getpkt(0, 0);
		int k = layer_map_[0];
		blkno_ = 0;
		channels_[k].nbb = 0;/*XXX clear out resump ptr*/
		flush(&channels_[k], 1);
		return;
	}
	for (int n = nlayer_; --n >= 0; )
		flush(&channels_[layer_map_[n]], 1);
}

int PvhEncoder::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "frame-format") == 0) {
		    	tcl.result("420");
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "shmap") == 0) {
			int off = atoi(argv[2]);
			int value = atoi(argv[3]);
			if ((unsigned)off >= NT) {
				tcl.result("bad shmap index");
				return (TCL_ERROR);
			}
			shmap_[off] = value;
			return (TCL_OK);
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "comp") == 0) {
			const char* which = argv[2];
			int shID = atoi(argv[3]);
			const char* descriptor = argv[4];
			int compID;
			if (strcasecmp(which, "lum-dct") == 0)
				compID = COMP_LUM_DCT;
			else if (strcasecmp(which, "lum-sbc") == 0)
				compID = COMP_LUM_SBC;
			else if (strcasecmp(which, "chm") == 0)
				compID = COMP_CHM;
			else {
				tcl.result("bad comp id");
				return (TCL_ERROR);
			}
			set_shs(compID, shID, descriptor);
			return (TCL_OK);
		}
	}
	return (TransmitterModule::command(argc, argv));
}

static void pone(const char* s, int v, double t)
{
	printf("%s\t%d\t(%.1f%%)\n", s, v, 100. * v / t);
}

/*SV-XXX: this function is never used...
static void pbits(bit_counter* b)
{
	double t = b->dc + b->mba + b->esc + b->vlc + b->mq + 
		b->sing + b->refine;
	int i;
	for (i = 0; i < 7; ++i) {
		t += b->zt[i] + b->sbc[i];
	}
	pone("dc", b->dc, t);
	pone("mba", b->mba, t);
	pone("mq", b->mq, t);
	pone("vlc", b->vlc, t);
	pone("esc", b->esc, t);
	pone("refine", b->refine, t);
	pone("sing", b->sing, t);

	pone("zt0", b->zt[0], t);
	pone("zt1", b->zt[1], t);
	pone("zt2", b->zt[2], t);
	pone("zt3", b->zt[3], t);
	pone("zt4", b->zt[4], t);
	pone("zt5", b->zt[5], t);
	pone("zt6", b->zt[6], t);
	int s = 0;
	for (i = 0; i < 7; ++i) {
		s += b->zt[i];
	}
	pone("zt", s, t);

	pone("sbc0", b->sbc[0], t);
	pone("sbc1", b->sbc[1], t);
	pone("sbc2", b->sbc[2], t);
	pone("sbc3", b->sbc[3], t);
	pone("sbc4", b->sbc[4], t);
	pone("sbc5", b->sbc[5], t);
	pone("sbc6", b->sbc[6], t);
	s = 0;
	for (i = 0; i < 7; ++i) {
		s += b->sbc[i];
	}
	pone("sbc", s, t);

	pone("tot", int(t), t);
}
*/

void PvhEncoder::refine_old(pvh_layer* lp, int off)
{
	/*
	 * compute a mask of the bits left to
	 * the bit position in question.
	 */
	int mask = (0xf << (off + 1)) & 0x3f;
	/*
	 * spin through all the refinement coefficients and
	 * send bits that apply to this level
	 */
	u_int16_t* sc = sc_;
	int code;
	while ((code = *sc++) != 0) {
		if ((code & mask) != 0) {
			int t = (code >> off) & 1;
			PVH_PUT_BITS(t, 1, lp->nbb, lp->bb, lp->bs, refine);
		}
	}
}

/*
 * From pvh-rc.hc
 */
#define RC_ESC_CODE	15
#define RC_ESC_LEN	5
#define RC_EOB_CODE	5
#define RC_EOB_LEN	3

void PvhEncoder::refine_new(pvh_layer* lp, int off)
{
	u_int16_t* sc = sc_;
	u_int bb = lp->bb;
	int nbb = lp->nbb;
	/*
	 * compute a mask of the bits left to
	 * the bit position in question.
	 */
	int mask = (0xf << (off + 1)) & 0x3f;
	/*
	 * spin through all the refinement coefficients and send
	 * all coefficients that become significant at this level
	 */
	int code;
	int pos = 0;
	while ((code = *sc++) != 0) {
		if ((code & mask) != 0)
			/*
			 * this coeff already significant -- don't
			 * include it in the run-length computation
			 */
			++pos;
		else {
			int t = (code >> off) & 1;
			if (t) {
				/*
				 * this coeff becomes significant
				 * at this point -- entropy code the
				 * run-length and sign.
				 */
				int k = code >> 6;
				int runlen = k - pos;
				pos = k;
				int nb, v;
				if (runlen >= 9) {
					nb = RC_ESC_LEN + 6;
					v = (RC_ESC_CODE << 6) | runlen;
				} else {
					const huffent* he = &hte_pvh_rc[runlen];
					nb = he->nb;
					v = he->val;
				}
				v = (v << 1) | (code & 1);
				nb += 1;
				PVH_PUT_BITS(v, nb, nbb, bb,
					      lp->bs, refine);
			}
		}
	}
	/*
	 * end of block
	 */
	PVH_PUT_BITS(RC_EOB_CODE, RC_EOB_LEN, nbb, bb, lp->bs, refine);

	lp->bb = bb;
	lp->nbb = nbb;
}

/*
 * XXX assumes q=16.
 */
void PvhEncoder::encode_blk(pvh_layer* lp, const int16_t* blk)
{
#ifdef notdef
printf("DCX %d\n", blk[0]);
printf("DCD %d\n", blk[0] - dc_);
dc_ = blk[0];
#endif
	u_int bb = lp->bb;
	int nbb = lp->nbb;
	/*
	 * Quantize DC.  Round instead of truncate.
	 */
	int dc = (blk[0] + 4) >> 3;
	/* Code DC */
	PVH_PUT_BITS(dc, 8, nbb, bb, lp->bs, dc);
	int run = 0;
	u_int16_t* sc = sc_;
	for (int i = 1; i < 64; ++i) {
		/*XXX*/
		if (lp->bs >= lp->es) {
			fprintf(stderr,
				"PvhEncoder::encode_blk: buffer overflow\n");
			exit(1);
		}

		/* XXX this can be tightened up */
		int k = COLZAG[i];
		int level = blk[k];
		int bits; /* <pos:6><dom:1><bits:3><unused:1><sign:1> */
		if (level < 0) {
			level = -level;
			bits = (level & 0x1c) | 1;
			level >>= 5;
		} else {
			bits = level & 0x1c;
			level >>= 5;
		}
		if (level == 0) {
			++run;
			/*
			 * If there are non-zero bits at a later
			 * refinement stage, remember them.
			 * XXX condition this test on the
			 * actual refinement layers we're generating
			 */
			if ((bits & 0x1c) != 0)
				*sc++ = i << 6 | bits;
			continue;
		}
		int val, nb;
		const huffent* he;
		if (level <= 15 &&
		    (nb = (he = &hte_pvh_tc[(level & 0x1f) | (bits & 1) << 4 | run << 5])->nb) != 0)
			/* We can use a VLC. */
			val = he->val;
		else {
			/* Can't use a VLC.  Escape it. */
			val = (1 << 14) | (run << 8) | (level & 0x7f) |
				(bits & 1) << 7;
			nb = 20;
		}
		PVH_PUT_BITS(val, nb, nbb, bb, lp->bs, vlc);
		run = 0;

		/* remember the refinement bits */
		*sc++ = i << 6 | bits | 0x20;
	}
	/* EOB */
	PVH_PUT_BITS(2, 2, nbb, bb, lp->bs, vlc);

	lp->bb = bb;
	lp->nbb = nbb;
	/* terminate the list of refinement positions */
	*sc = 0;
}

/*XXX*/
void PvhEncoder::encode_dct_base(pvh_layer* lp, int nb)
{
	if (nb != 5)
		/* we hard code a 5 bit quantizer */
		abort();

	encode_blk(lp, coef_);
}

/*
 * Send DCT coefficient refinement bits.  "bit" is the bit position
 * that we are refining and "nr" is the number of bit-planes (including
 * the current plane) that we should send.  e.g., bit=4 and nr=2
 * will send bit-planes 4 and 3.
 */
void PvhEncoder::encode_dct_refinement(pvh_layer* lp, int bit, int nr)
{
	while (--nr >= 0) {
		refine_old(lp, bit);
		refine_new(lp, bit);
		--bit;
	}
}

/*
 * Perform a single-stage subband decomposition over the 16x16 subimage
 * of the frame buffer whose upper left corner is pointed to by "in".
 * The analysis filter bank is:
 *
 *	H1(z) = -1 + 3z + 3z^2 - z^3
 *	H2(z) =  1 - 3z + 3z^2 - z^3
 *
 * This biorthogonal filter pair will result in a single pixel of
 * overlap with adjacent blocks.  Single extension at the edges of
 * the image is carried out by the caller.
 *
 * The LL, LH, and HL outpus are contiguously stored in the
 * output buffer "out" (the arrays are not interleaved).
 *
 * A note on rounding: many of the cases below could do a better
 * job on the quantization step (e.g., shift right doesn't
 * round toward 0 for negative values).  Rather than take
 * the extra cycles to do it right, we simply live with the
 * error since these errors amount to very small effects
 * in the pixel domain (a couple tenths of a dB at high rates,
 * and are inconsequential at lower rates).
 */
#undef FULL_PRECISION
#ifdef FULL_PRECISION
void PvhEncoder::decompose_block(const u_int8_t* in, u_int8_t* out)
{
	/*
	 * Run one loop over the columns, then two loops
	 * over the rows.  We separate out the row decomposition
	 * into two passes since LL band quantization is different
	 * than LH, HL (i.e., singed vs. unsigned), and we don't
	 * bother computing and storing the HH band.
	 */
	int16_t blk[2*8*18];
	int16_t* l = blk;
	int16_t* h = &blk[8*18];
	int stride = width_;
	in -= 1;
	/* XXX can easily do multiple columns in parallel */
	for (int w = 18; --w >= 0; ) {
		int16_t* cl = l++;
		int16_t* ch = h++;
		const u_char* p = in++;
		int in0 = p[-stride];
		int in1 = *p;
		p += stride;
		for (int th = 8; --th >= 0; ) {
			int in2 = *p;
			p += stride;
			int in3 = *p;
			p += stride;

			/* multiply by 3 */
			int m1 = in1 + (in1 << 1);
			int m2 = in2 + (in2 << 1);

			/* -1 3 3 -1 */
			int v = m1 + m2 - in0 - in3;
			*cl = v;
			cl += 18;

			/* 1 -3 3 -1 */
			v = in0 - m1 + m2 - in3;
			*ch = v;
			ch += 18;

			in0 = in2;
			in1 = in3;
		}
	}
	/* rows */
	const int16_t* p = blk;
	u_int8_t* sl = (u_int8_t*)out;
	int8_t* sh = out + 2*64;
	int th;
	for (th = 8; --th >= 0; ) {
		int in0 = *p++;
		int in1 = *p++;
		for (int w = 8; --w >= 0; ) {
			int in2 = *p++;
			int in3 = *p++;

			/* multiply by 3 */
			int m1 = in1 + (in1 << 1);
			int m2 = in2 + (in2 << 1);

			/* -1 3 3 -1 */
			int v = m1 + m2 - in0 - in3;
			v = (v + 8) >> 4;
			if (v & ~0xff)
				v = (v < 0) ? 0 : 255;
			*sl++ = v;
			/* 1 -3 3 -1 */
			v = in0 - m1 + m2 - in3;
			v = (v < 0) ? -((-v) >> 4) : (v) >> 4;
			if ((v + 128) & ~0xff)
				v = (v < -128) ? -128 : 127;
			*sh++ = SM8(v);

			in0 = in2;
			in1 = in3;
		}
	}
	for (th = 8; --th >= 0; ) {
		int in0 = *p++;
		int in1 = *p++;
		for (int w = 8; --w >= 0; ) {
			int in2 = *p++;
			int in3 = *p++;

			/* multiply by 3 */
			int m1 = in1 + (in1 << 1);
			int m2 = in2 + (in2 << 1);

			/* -1 3 3 -1 */
			int v = m1 + m2 - in0 - in3;
			v = (v + 7) >> 4;
			if ((v + 128) & ~0xff)
				v = (v < -128) ? -128 : 127;
			*sl++ = SM8(v);
#ifdef HAVE_HH
			/* 1 -3 3 -1 */
			v = in0 - m1 + m2 - in3;
			v = (v + 7) >> 4;
			if ((v + 128) & ~0xff)
				v = (v < -128) ? -128 : 127;
			*sh++ = SM8(v);
#endif

			in0 = in2;
			in1 = in3;
		}
	}
}
#else
#define CMAX(a,b) ((a) > (b) ? (a) : (b))
void PvhEncoder::decompose_block(const u_char* in, u_int8_t* out)
{
	int8_t wrk[2*8*18];
	u_int8_t* l = (u_int8_t*)wrk;
	int8_t* h = &wrk[8*18];
	int stride = width_;
	in -= 1;
	for (int w = 18; --w >= 0; ) {
		int8_t* cl = (int8_t*)(l++);
		int8_t* ch = h++;
		const u_char* p = in++;
		int in0 = p[-stride];
		int in1 = *p;
		p += stride;
		for (int th = 8; --th >= 0; ) {
			int in2 = *p;
			p += stride;
			int in3 = *p;
			p += stride;

			/* multiply by 3 */
			int m1 = in1 + (in1 << 1);
			int m2 = in2 + (in2 << 1);

			/* -1 3 3 -1 */
			int v = m1 + m2 - in0 - in3;
			v = (v + 2) >> 2;
			if (v & ~0xff)
				v = (v < 0) ? 0 : 255;
			*cl = v;
			cl += 18;

			/* 1 -3 3 -1 */
			v = in0 - m1 + m2 - in3;
			v >>= 2;
			if ((v + 128) & ~0xff)
				v = (v < -128) ? -128 : 127;
			*ch = v;
			ch += 18;

			in0 = in2;
			in1 = in3;
		}
	}
	/* rows */
	const u_int8_t* p = (u_int8_t*)wrk;
	l = out;
	h = (int8_t*)out + 2*64;
	int th;
	for (th = 8; --th >= 0; ) {
		int in0 = *p++;
		int in1 = *p++;
		for (int w = 8; --w >= 0; ) {
			int in2 = *p++;
			int in3 = *p++;

			/* multiply by 3 */
			int m1 = in1 + (in1 << 1);
			int m2 = in2 + (in2 << 1);

			/* -1 3 3 -1 */
			int v = m1 + m2 - in0 - in3;
			v = (v + 2) >> 2;
			if (v & ~0xff)
				v = (v < 0) ? 0 : 255;
			*l++ = v;
			/* 1 -3 3 -1 */
			v = in0 - m1 + m2 - in3;
			v >>= 2;
			SM8_SAT(v);
			*h++ = SM8(v);

			in0 = in2;
			in1 = in3;
		}
	}
	for (th = 8; --th >= 0; ) {
		int in0 = *(int8_t*)(p++);
		int in1 = *(int8_t*)(p++);
		for (int w = 8; --w >= 0; ) {
			int in2 = *(int8_t*)(p++);
			int in3 = *(int8_t*)(p++);

			/* multiply by 3 */
			int m1 = in1 + (in1 << 1);
			int m2 = in2 + (in2 << 1);

			/* -1 3 3 -1 */
			int v = m1 + m2 - in0 - in3;
			v >>= 2;
			SM8_SAT(v);
			*l++ = SM8(v);
#ifdef HAVE_HH
			/* 1 -3 3 -1 */
			v = in0 - m1 + m2 - in3;
			v >>= 2;
			SM8_SAT(v);
			*h++ = SM8(v);
#endif

			in0 = in2;
			in1 = in3;
		}
	}
}
#endif

void PvhEncoder::build_tree_43(const u_int8_t* p)
{
	u_int m4 = 0;
	u_int m3 = 0;
	int k = 0;
	for (int i = 4; --i >= 0; ) {
		int m;
		m = p[0] | p[1] | p[8] | p[9];
		m4 |= ((m >> 4 | m >> 5 | m >> 6) & 1) << k;
		m3 |= ((m >> 3) & 1) << k++;

		m = p[2] | p[3] | p[10] | p[11];
		m4 |= ((m >> 4 | m >> 5 | m >> 6) & 1) << k;
		m3 |= ((m >> 3) & 1) << k++;

		m = p[4] | p[5] | p[12] | p[13];
		m4 |= ((m >> 4 | m >> 5 | m >> 6) & 1) << k;
		m3 |= ((m >> 3) & 1) << k++;

		m = p[6] | p[7] | p[14] | p[15];
		m4 |= ((m >> 4 | m >> 5 | m >> 6) & 1) << k;
		m3 |= ((m >> 3) & 1) << k++;

		p += 16;
	}
	tree_[4] = m4;
	tree_[3] = m3;
}

void PvhEncoder::build_tree_21(const u_int8_t* p)
{
	u_int m2 = 0;
	u_int m1 = 0;
	int k = 0;
	for (int i = 4; --i >= 0; ) {
		int m;
		m = p[0] | p[1] | p[8] | p[9];
		m2 |= ((m >> 2) & 1) << k;
		m1 |= ((m >> 1) & 1) << k++;

		m = p[2] | p[3] | p[10] | p[11];
		m2 |= ((m >> 2) & 1) << k;
		m1 |= ((m >> 1) & 1) << k++;

		m = p[4] | p[5] | p[12] | p[13];
		m2 |= ((m >> 2) & 1) << k;
		m1 |= ((m >> 1) & 1) << k++;

		m = p[6] | p[7] | p[14] | p[15];
		m2 |= ((m >> 2) & 1) << k;
		m1 |= ((m >> 1) & 1) << k++;

		p += 16;
	}
	tree_[2] = m2;
	tree_[1] = m1;
}

static u_int8_t cf_code[16] = {
	0x2, 0x11, 0xc8, 0xcc, 0xd0, 0xd4, 0xd8, 0xdc,
	0xe0, 0xe4, 0xe8, 0xec, 0xf0, 0xf4, 0xf8, 0xfc
};

#define ENCODE_COEF(cf, n, code) \
{ \
	int _t = cf_code[(cf) >> 4]; \
	int _n = 6 >> (_t & 3); \
	(code) <<= _n; \
	(n) += _n; \
	(code) |= _t >> 2; \
} \

/*
 * Encode the top four bits of each subband coefficient that we
 * should send according to the quad-tree mask.  The huffman code is:
 *
 *	0	0
 *	1	100
 *	-1	101
 *	v	110xxx
 *	-v	111xxx
 *
 * where x = |v| is in [2,7]
 */
void PvhEncoder::encode_sbc_base(pvh_layer* lp, u_int8_t* p, u_int m)
{
	u_int bb = lp->bb;
	int nbb = lp->nbb;
	for (int k = 0; m != 0; ++k, m >>= 1) {
		if ((m & 1) != 0) {
			/* send this quad set */
			u_int8_t* q = &p[(k & 3) << 1 | (k & 0xc) << 2];
			u_int code = 0;
			int nb = 0;
			ENCODE_COEF(q[0], nb, code);
			ENCODE_COEF(q[1], nb, code);
			ENCODE_COEF(q[8], nb, code);
			ENCODE_COEF(q[9], nb, code);
			PVH_PUT_BITS(code, nb, nbb, bb, lp->bs, sbc[0]);
		}
	}
	lp->bb = bb;
	lp->nbb = nbb;
}

#define ENCODE_REFINEMENT(cf, n, code, mask, bit) \
{ \
	int _t = (cf); \
	++n; \
	code <<= 1; \
	code |= (_t >> (bit)) & 1; \
	if ((((cf) & (mask)) >> (bit)) == 1) { \
		++n; \
		code <<= 1; \
		code |= _t >> 7; \
	} \
}

void PvhEncoder::encode_sbc_refinement(pvh_layer* lp, u_int8_t* p,
				       u_int m, int bit)
{
	u_int bb = lp->bb;
	int nbb = lp->nbb;
	int mask = 0x80 - (1 << bit);
	for (int k = 0; m != 0; ++k, m >>= 1) {
		if ((m & 1) != 0) {
			/* send this quad set */
			u_int8_t* q = &p[(k & 3) << 1 | (k & 0xc) << 2];
			u_int code = 0;
			int nb = 0;
			ENCODE_REFINEMENT(q[0], nb, code, mask, bit);
			ENCODE_REFINEMENT(q[1], nb, code, mask, bit);
			ENCODE_REFINEMENT(q[8], nb, code, mask, bit);
			ENCODE_REFINEMENT(q[9], nb, code, mask, bit);
			PVH_PUT_BITS(code, nb, nbb, bb, lp->bs, sbc[1]);
		}
	}
	lp->bb = bb;
	lp->nbb = nbb;
}

void PvhEncoder::encode_tree(pvh_layer* lp, u_int m)
{
	u_int sm = m;
	sm |= sm >> 2;
	sm |= sm >> 1;
	sm = sm & 1 | sm >> 3 & 2 | sm >> 6 & 4 | sm >> 9 & 8;
	int code = sm;
	int n = 4;
	for (int i = 4; --i >= 0; ) {
		if ((sm & (1 << i)) != 0) {
			code <<= 4;
			code |= (m >> 12) & 0xf;
			n += 4;
		}
		m <<= 4;
	}
	PVH_PUT_BITS(code, n, lp->nbb, lp->bb, lp->bs, zt[1]);
}

void PvhEncoder::encode_sbc(const spatial_hierarchy* sh, u_int8_t* sbc)
{
	/*
	 * Find base layer of subband coefficients, encode
	 * base layer, then encode refinements.
	 */
	if (sh->bq <= 0)
		return;

	pvh_layer* lp = &channels_[sh->base_channel];
	int bit = sh->bq;
	if (bit != 4)
		/*XXX can only handle 4-bit SBC base quantizer */
		abort();
	build_tree_43(sbc);
	u_int m = tree_[4];
	if (m == 0) {
		PVH_PUT_BITS(0, 1, lp->nbb, lp->bb, lp->bs, zt[0]);
	} else {
		PVH_PUT_BITS(1, 1, lp->nbb, lp->bb, lp->bs, zt[1]);
		encode_tree(lp, m);
		encode_sbc_base(lp, sbc, m);
	}
	bit -= 1;
	for (int i = 0; i < sh->n; ++i) {
		for (int nr = sh->ref[i].nr; --nr >= 0; ) {
			lp = &channels_[sh->ref[i].channel];
			m = tree_[bit];
			encode_tree(lp, m);
			encode_sbc_refinement(lp, sbc, m, bit);
			--bit;
		}
	}
}

void PvhEncoder::encode_dct(spatial_hierarchy* sh)
{
	/*
	 * Encode base-layer of DCT coefficients, then
	 * DCT coefficient refinements.
	 */
	pvh_layer* lp = &channels_[sh->base_channel];
	int bit = sh->bq;
	encode_dct_base(lp, bit);
	bit -= 1;
	for (int i = 0; i < sh->n; ++i) {
		pvh_refinement* rp = &sh->ref[i];
		int nr = rp->nr;
		lp = &channels_[rp->channel];
		encode_dct_refinement(lp, bit, nr);
		bit -= nr;
	}
}

void PvhEncoder::encode_lum(const u_int8_t* in)
{
	/*
	 * Perform subband decomposition into three bands --- 
	 * LL, LH, and HL.  The transform discards the HH band.
	 * Then apply a DCT to the LL band.
	 */
	u_int8_t out[NC];
	decompose_block(in, out);
	fdct(out, 8, coef_, qt_);

	encode_dct(&sh_[COMP_LUM_DCT]);
	encode_sbc(&sh_[COMP_LUM_SBC], out + 64);
	encode_sbc(&sh_[COMP_LUM_SBC], out + 128);
}

void PvhEncoder::encode_chm(const u_int8_t* in)
{
	/*XXX this can be u_char instead of short but need smarts in fdct */
	fdct(in, width_ >> 1, coef_, qt_);
	encode_dct(&sh_[COMP_CHM]);
}

void PvhEncoder::encode_resumption_ptrs(pvh_layer* lp, int spatial_layer)
{
	for (int k = spatial_layer + 1; k < nlayer_; ++k) {
		PUT_BITS(1, 1, lp->nbb, lp->bb, lp->bs);
		u_int chan = layer_map_[k];
		pvh_layer* target = &channels_[chan];
		u_int off = BB_NBIT(target);
		if (chan >= 16)
			abort();/*XXX*/
		PUT_BITS(chan, 4, lp->nbb, lp->bb, lp->bs);
		if (off >= (1 << 16))
			abort();/*XXX*/
		PUT_BITS(off, 16, lp->nbb, lp->bb, lp->bs);
#ifdef notdef
printf("E L%d RP +%d @%d\n", lp->pb->layer, chan, off);
#endif
	}
	PUT_BITS(0, 1, lp->nbb, lp->bb, lp->bs);
}

void PvhEncoder::getpkt(int spatial_layer, int sblk)
{
	int chan = layer_map_[spatial_layer];
	pvh_layer* lp = &channels_[chan];
	pktbuf* pb = pool_->alloc(ts_, pt_, chan);
	/*XXX*/
	if (lp->pb != 0)
		abort();
	lp->pb = pb;
	rtphdr* rh = (rtphdr*)pb->data;
	pvhhdr* bh = (pvhhdr*)(rh + 1);
	bh->version = 0;
	bh->width = width_ >> 3;
	bh->height = height_ >> 3;
	bh->sblk = htons(sblk);
	lp->bs = &pb->data[HLEN];
	lp->es = lp->bs + tx_->mtu() - HLEN;
	lp->bb = 0;
	lp->nbb = 0;

	/*
	 * XXX for now, just use resumption pointers on base layer
	 * BUG: this is out of conformance
	 */
	if (spatial_layer == 0)
		encode_resumption_ptrs(lp, spatial_layer);
}

/*
 * Build the layer-map from the spatial-hieararchy
 * and then allocate a packet buffer for each
 * active output layer.
 */
void PvhEncoder::setup_layers(spatial_hierarchy* sh, int sblk)
{
	build_layer_map(sh);
	for (int n = nlayer_; --n >= 0; )
		getpkt(n, sblk);
}

int PvhEncoder::sh_refinement(spatial_hierarchy* sh, int chan) const
{
	for (int i = 0; i < sh->n; ++i) {
		if (sh->ref[i].channel == chan)
			return (sh->ref[i].nr);
	}
	return (0);
}

extern void dumpSH(const char* c, spatial_hierarchy* sh);

void PvhEncoder::encode_comp_layout(spatial_hierarchy* sh)
{
#ifdef notdef
dumpSH("E", sh);
#endif
	pvh_layer* lp = &channels_[layer_map_[0]];
	int k;
	for (k = 0; k < nlayer_; ++k) {
		int chan = layer_map_[k];
		if (chan < sh->base_channel) {
			PVH_PUT_BITS(0, 1, lp->nbb, lp->bb, lp->bs, layout);
		} else {
			PVH_PUT_BITS(1, 1, lp->nbb, lp->bb, lp->bs, layout);
			PVH_PUT_BITS(sh->bq, 3, lp->nbb, lp->bb, lp->bs, layout);
#ifdef notdef
printf("E L%d BQ %d\n", lp->pb->layer, sh->bq);
#endif
			break;
		}
	}
	for (; k < nlayer_; ++k) {
		int chan = layer_map_[k];
		int nr = sh_refinement(sh, chan);
		PVH_PUT_BITS(nr, 3, lp->nbb, lp->bb, lp->bs, layout);
#ifdef notdef
printf("E L%d REF %d\n", lp->pb->layer, nr);
#endif
		if (nr >= 8)
			abort();
	}
}

void PvhEncoder::encode_bit_layout(spatial_hierarchy* sh)
{
	for (int i = 0; i < 3; ++i, ++sh)
		encode_comp_layout(sh);
}

#define MAXMBSIZE (16*16+2*8*8+4)

/*
 * Return non-zero if we reset the base layer.
 */
int PvhEncoder::check_buffers(int sblk)
{
	int st = 0;
	/*
	 * Check if we're near the end of any buffers
	 * and reset them.  The loop is structured from
	 * the top to bottom layers to make sure
	 * that the resumption pointers are properly
	 * filled in by getpkt().
	 */
	for (int i = nlayer_; --i >= 0; ) {
		pvh_layer* lp = &channels_[layer_map_[i]];
		/*
		 * XXX too conservative -
		 * should instead walk off end and copy back
		 * (like H.261)
		 */
		if (lp->bs + MAXMBSIZE >= lp->es) {
			flush(lp, 0);
			getpkt(i, sblk);
			if (i == 0)
				st = 1;
		}
	}
	return (st);
}

//void PvhEncoder::recv(Buffer* bp)
int PvhEncoder::consume(const VideoFrame *vf)
{
	//const VideoFrame* vf = (VideoFrame*)bp;
	if (!samesize(vf))
		size(vf->width_, vf->height_);
//#ifdef notdef
	tx_->flush();
//#endif
	YuvFrame* p = (YuvFrame*)vf;
	ts_ = p->ts_;
	cc_ = 0;
	/*
	 * Lookup the spatial hierarchy data structure.
	 * We map the temporal layer number to a spatial
	 * layer number and point sh_ at the appropriate entry.
	 */
	//sh_ = shs_[shmap_[p->layer_ & (NT - 1)]];
	sh_ = shs_[shmap_[(ts_>>8) & (NT - 1)]];

	u_char* frm = (u_char*)p->bp_;
	const u_char* chm = frm + framesize_;
	blkno_ = -1;

	const u_int8_t* crv = p->crvec_;

	int blkw = width_ >> 4;
	int blkh = height_ >> 4;
	int blkno = 0;
	for (int y = 0; y < blkh; ++y) {
		for (int x = 0; x < blkw; ++blkno, frm += 16, chm += 8,
		     ++x, ++crv) {
			int s = crv[0];
			if ((s & CR_SEND) == 0)
				continue;

			if (blkno_ < 0) {
				setup_layers(sh_, blkno);
				encode_bit_layout(sh_);
			} else if (check_buffers(blkno)) {
				encode_bit_layout(sh_);
			} else {
				int dblk = blkno - blkno_;
				if (dblk <= 0)
					abort();

				/* look up base layer (of the LUM-DCT component) */
				pvh_layer* lp = &channels_[sh_->base_channel];
				if (dblk == 1) {
					PUT_BITS(1, 1, lp->nbb, lp->bb, lp->bs);
				} else if (dblk <= 17) {
					PUT_BITS(0x10 | (dblk - 2), 6, lp->nbb, lp->bb, lp->bs);
				} else {
					PUT_BITS(dblk, 13, lp->nbb, lp->bb, lp->bs);
				}
			}
			blkno_ = blkno;

			/*
			 * Deal with boundaries of image.  This is
			 * simply ugly, but it's the price we have
			 * to pay for using a four tap filter stage.
			 * For the 1-3-3-1 filter set, we use symmetric
			 * extension at both analysis and synthesis
			 * to give perfect reconstruction.
			 * If we're at the top of the image, we simply
			 * copy the top row up (since the grabber's
			 * reserve one line of extra space).  Otherwise,
			 * we save pixels, do the symmetrization,
			 * encode the block, then restore the pixels.
			 */
			u_char ysave[16];
			u_char xsave[16];
			if (y == 0)
				memcpy(frm - width_, frm, 16);
			else if (y == height_ - 16 /*XXX*/) {
				u_char* p = frm + (width_ << 4);
				memcpy(ysave, p, 16);
				memcpy(p, p - width_, 16);
			}
			if (x == 0) {
				u_char* s = xsave;
				u_char* p = frm;
				for (int k = 0; k < 16; ++k) {
					*s++ = p[-1];
					p[-1] = p[0];
					p += width_;
				}
			} else if (x == width_ - 16/*XXX*/) {
				u_char* s = xsave;
				u_char* p = frm + 16;
				for (int k = 0; k < 16; ++k) {
					*s++ = p[0];
					p[0] = p[-1];
					p += width_;
				}
			}
			encode_lum(frm);
			encode_chm(chm);
			encode_chm(chm + (framesize_ >> 2));
			/*
			 * Now restore the pixels.  We don't bother
			 * with the "y = 0" case because the affected
			 * memory will not be used.
			 */
			if (y == height_ - 16 /*XXX*/) {
				u_char* p = frm + (width_ << 4);
				memcpy(p, ysave, 16);
			}
			if (x == 0) {
				u_char* s = xsave;
				u_char* p = frm;
				for (int k = 0; k < 16; ++k) {
					p[-1] = *s++;
					p += width_;
				}
			} else if (x == width_ - 16/*XXX*/) {
				u_char* s = xsave;
				u_char* p = frm + 16;
				for (int k = 0; k < 16; ++k) {
					p[0] = *s++;
					p += width_;
				}
			}
		}
		frm += 15 * width_;
		chm += 7 * (width_ >> 1);
	}
	flush();
	//nb_ += cc_;
    return (cc_);
}

void PvhEncoder::size(int w, int h)
{
	if ((w | h) & 0xf) {
		printf("vic: bad size in pvh coder (%dx%d)\n", w, h);
		abort();
	}
	Module::size(w, h);
}
