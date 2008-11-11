/* $Header$ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "decoder.h"
#include "inet.h"
#include "rtp.h"
#include "renderer.h"

class BvcDecoder : public Decoder {
 public:
	BvcDecoder();
	~BvcDecoder();
	virtual void recv(pktbuf* pb);
	void resize(int width, int height);
	int colorhist(u_int* hist) const;
 protected:
	virtual void redraw();
	void sync();
	void decode(const u_char* bp, int cc, int blkno);
	void decode_block(int x, int y);
	void decode_sbc(int8_t* ll, int8_t* hl, int8_t* lh, int stride);
	int prune_layers(int active);
	void decode_coef(int8_t* p, int active, int qs);
	void reconstruct_block(int x, int y);
	void quantizer(u_int32_t nquant, u_int32_t oquant);
	void compute_dchild();

	const u_char *inb_;		/* input buffer XXX make u_long */
	const u_char *end_;
	u_int bb_;		/* bit buffer */
	int nbb_;		/* # bits in bit buffer */

	u_int32_t quant_;
	u_char qs_[64];
	/*XXX*/
	int8_t* level0_;
	int8_t* ll_;
	int8_t* lh_;
	int8_t* hl_;
	/*XXX*/
	u_char* blk_;

	u_char* frm_;

	/*XXX*/
	u_short dchild_[64];
};

static class BvcDecoderMatcher : public Matcher {
public:
	BvcDecoderMatcher() : Matcher("decoder") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "bvc") == 0)
			return (new BvcDecoder());
		else
			return (0);
	}
} dm_bvc;

#define STAT_BAD_SYNTAX	0
#define STAT_BAD_GEOM	1

//SV-XXX: rearranged intialistaion order to shut upp gcc4
BvcDecoder::BvcDecoder() : Decoder(sizeof(bvchdr)),
	level0_(0), blk_(0), frm_(0)
{
	/*XXX*/
	for (int i = 0; i < 64; ++i)
		qs_[i] = 0;
	quant_ = 0;

	stat_[STAT_BAD_SYNTAX].name = "Bvc-Bad-Syntax";
	stat_[STAT_BAD_GEOM].name = "Bvc-Bad-Geom";
	nstat_ = 2;
	decimation_ = 420;

	/*XXX*/
	inw_ = 320;
	inh_ = 240;

	compute_dchild();
	resize(inw_, inh_);
}

BvcDecoder::~BvcDecoder()
{
	delete[] level0_; //SV-XXX: Debian
	delete[] frm_; //SV-XXX: Debian
	delete[] blk_; //SV-XXX: Debian
}

#define LINEPAD 8

/*XXX*/
extern u_char child[];

/*XXX*/
void BvcDecoder::compute_dchild()
{
	int stride = (inw_ + LINEPAD) >> 1;
	/*XXX need to update dchild when qs_ changes */
	for (int i = 0; i < 64; ++i) {
		int v = child[i];
		if (v < 64)
			dchild_[i] = (v << 5) | qs_[i];
		else if (v >= 192) {
			v -= 156;
			v >>= 1;
			dchild_[i] = (v << 5) | (3 << 3) | qs_[i];
		} else {
			int m;
			if (v >= 128) {
				m = 1;
				v -= 128;
			} else {
				m = 2;
				v -= 64;
			}
			int x = v & 7;
			int y = v >> 3;
			v = y * stride + x;
			if ((v & 1) || (v >> 1) >= 0x7ff)
				abort();/*XXX*/
			v >>= 1;
			dchild_[i] = (v << 5) | (m << 3) | qs_[i];
		}
	}
}

void BvcDecoder::resize(int width, int height)
{
	if ((width | height) & 0xf) {
		printf("vic: bad geometry in bvc coder (%dx%d)\n",
		       width, height);
		abort();
	}

	delete[] frm_; //SV-XXX: Debian
	int size = width * height;
	/*XXX 4:1:1*/
	frm_ = new u_char[2 * size];
	/* 
	 * Initialize image to gray.
	 */
	memset(frm_, 0x80, 2 * size);

	/*XXX*/
	int w = width >> 1;
	int h = height >> 1;
	int s = (w + LINEPAD) * (h + 2);
	delete[] level0_; //SV-XXX: Debian
	level0_ = new int8_t[3 * s];
	/*XXX*/
	memset(level0_, 0, 3 * s);
	int off = w + LINEPAD / 2;
	ll_ = level0_ + 0 * s + off;
	lh_ = level0_ + 1 * s + off;
	hl_ = level0_ + 2 * s + off;
	delete[] blk_; //SV-XXX: Debian
	blk_ = new u_char[(width >> 4) * (height >> 4)];

	Decoder::resize(width, height);
}

u_char quantno[] = {
	9, 0, 2, 2, 4, 4, 4, 4,
	0, 1, 3, 3, 5, 5, 5, 5,
	2, 3, 6, 6, 7, 7, 7, 7,
	2, 3, 6, 6, 7, 7, 7, 7,
	4, 5, 7, 7, 8, 8, 8, 8,
	4, 5, 7, 7, 8, 8, 8, 8,
	4, 5, 7, 7, 8, 8, 8, 8,
	4, 5, 7, 7, 8, 8, 8, 8,
};
	
void
setquant(u_char* p, int n, int q)
{
	for (int i = 0; i < 64; ++i)
		if (n == quantno[i])
			p[i] = q;
}

void BvcDecoder::quantizer(u_int32_t nquant, u_int32_t oquant)
{
	for (int i = 0; i < 10; ++i) {
		int nq = nquant & 7;
		int oq = oquant & 7;
		if (nq != oq)
			setquant(qs_, i, nq);
		nquant >>= 3;
		oquant >>= 3;
	}
	compute_dchild();
}

#define MASK(s) ((1 << (s)) - 1)

/*
 * Read the next 16 bits off the bit string into the bit buffer.
 * Skip over zero-stuffed ff's but make no attempt to verify
 * that they aren't some other marker (which shouldn't be in the
 * middle of a block anyway).
 */
#define HUFFRQ(bb) \
 { \
	register int v; \
	register const u_char *cp = inb_; \
 \
	bb <<= 16; \
	v = *cp++; \
	bb |= v << 8; \
	v = *cp++; \
	bb |= v; \
	inb_ = cp; \
 \
}

#define HUFF_DECODE(ht, nbb, bb, result) { \
	register int s_, v_; \
 \
	if (nbb < 16) { \
		HUFFRQ(bb); \
		nbb += 16; \
	} \
	v_ = (bb >> (nbb - 16)) & 0xffff; \
	s_ = (ht)[v_]; \
	nbb -= (s_ >> 8); \
	result = s_ & 0xff; \
 }

#define GET_BITS(n, nbb, bb, result) \
{ \
	nbb -= n; \
	if (nbb < 0)  { \
		HUFFRQ(bb); \
		nbb += 16; \
	} \
	(result) = ((bb >> nbb) & MASK(n)); \
}

#define SKIP_BITS(n, nbb, bb) \
{ \
	nbb -= n; \
	if (nbb < 0)  { \
		HUFFRQ(bb); \
		nbb += 16; \
	} \
}

/*16x16*/
void BvcDecoder::reconstruct_block(int x, int y)
{
	int stride = (inw_ + LINEPAD) >> 1;
	int off = (y >> 1) * stride + (x >> 1);
	int16_t blk[2*10*16];

	int16_t* o = blk;
	u_int8_t* lu = (u_int8_t*)ll_ + off - stride;
	int8_t* h = lh_ + off - stride;
	o = blk;
	int th;
	for (th = 10; --th >= 0; ) {
		int l0 = lu[-1];
		int h0 = h[-1];
		int l1 = *lu++;
		int h1 = *h++;
		for (int w = 8; --w >= 0; ) {
			int l3 = l1 + (l1 << 1);
			int h3 = h1 + (h1 << 1);
			int v = l0 + l3 + h0 - h3;
			*o++ = v >> 2;

			l0 = l1;
			h0 = h1;
			l1 = *lu++;
			h1 = *h++;

			v = l3 + l1 + h3 - h1;
			*o++ = v >> 2;
		}
		lu += stride - 9;
		h += stride - 9;
	}
	int8_t* l = hl_ + off - stride;
#ifdef HAVE_HH
	h = hh_ + off - stride;
#endif
	for (th = 10; --th >= 0; ) {
		int l0 = l[-1];
		int h0 = h[-1];
		int l1 = *l++;
#ifdef HAVE_HH
		int h1 = *h++;
#else
		int h1 = 0;
#endif
		for (int w = 8; --w >= 0; ) {
			int l3 = l1 + (l1 << 1);
			int h3 = h1 + (h1 << 1);
			int v = l0 + l3 + h0 - h3;
			*o++ = v >> 2;

			l0 = l1;
			h0 = h1;
			l1 = *l++;
#ifdef HAVE_HH
			h1 = *h++;
#endif
			v = l3 + l1 + h3 - h1;
			*o++ = v >> 2;
		}
		l += stride - 9;
#ifdef HAVE_HH
		h += stride - 9;
#endif
	}

	o = blk;
	off = y * inw_ + x;
	u_int8_t* out = frm_ + off;
	for (int w = 16; --w >= 0; ) {
		const int16_t* cl = o++;
		const int16_t* ch = cl + 10*16;
		u_int8_t* p = out++;

		int l0 = *cl;
		cl += 16;
		int h0 = *ch;
		ch += 16;
		int l1 = *cl;
		cl += 16;
		int h1 = *ch;
		ch += 16;
		
		for (int th = 8; --th >= 0; ) {
			int l3 = l1 + (l1 << 1);
			int h3 = h1 + (h1 << 1);
			int v = l3 + l0 + h0 - h3;
			v >>= 2;
			/*XXX*/
			if (v & ~0xff)
				v = (v < 0) ? 0 : 255;
			*p = v;
			p += inw_;

			l0 = l1;
			h0 = h1;
			l1 = *cl;
			cl += 16;
			h1 = *ch;
			ch += 16;

			v = l1 + l3 + h3 - h1;
			v >>= 2;
			/*XXX*/
			if (v & ~0xff)
				v = (v < 0) ? 0 : 255;
			*p = v;
			p += inw_;
		}
	}
}

void haar_rev(const int8_t* in, int8_t* out, int stride)
{
	int blk[64];
	int* o = blk;

#ifdef notdef
	if (nohaar)
		return;
#endif

	int m = 0xff;/*XXX make sure DC is unsigned*/

	/* inverse Haar transform 8x8 base image */
	int k;
	for (k = 8; --k >= 0; ) {
		int t4 = in[0];
		t4 &= m;
		m = ~0;
		int t5 = in[1];
		int t0 = t4 - t5;
		int t1 = t4 + t5;
			
		t4 = in[2];
		t5 = in[3];
		int t2 = t0 - t4;
		int t3 = t0 + t4;
		t4 = t1 - t5;
		t5 = t1 + t5;

		t0 = in[4];
		t1 = in[5];
		o[0] = t2 - t0;
		o[1] = t2 + t0;
		o[2] = t3 - t1;
		o[3] = t3 + t1;

		t0 = in[6];
		t1 = in[7];
		o[4] = t4 - t0;
		o[5] = t4 + t0;
		o[6] = t5 - t1;
		o[7] = t5 + t1;

		o += 8;
		in += 8;
	}
	/* columns */
	o = blk;
	for (k = 8; --k >= 0; ) {
		int t4 = o[0*8];
		int t5 = o[1*8];
		int t0 = t4 - t5;
		int t1 = t4 + t5;

		t4 = o[2*8];
		t5 = o[3*8];
		int t2 = t0 - t4;
		int t3 = t0 + t4;
		t4 = t1 - t5;
		t5 = t1 + t5;

		t0 = o[4*8];
		t1 = o[5*8];

		/*
		 * Saturate the reconstructed values, since quantization
		 * of the transform coefficients can result in values
		 * outside [0,255].
		 */
#define UCLIMIT(x) ((t = (x)), (t &= ~(t>>31)), (t | ~((t-256) >> 31)))
		int t;
		*out = UCLIMIT(t2 - t0);
		out += stride;
		*out = UCLIMIT(t2 + t0);
		out += stride;
		*out = UCLIMIT(t3 - t1);
		out += stride;
		*out = UCLIMIT(t3 + t1);
		out += stride;

		t0 = o[6*8];
		t1 = o[7*8];
		*out = UCLIMIT(t4 - t0);
		out += stride;
		*out = UCLIMIT(t4 + t0);
		out += stride;
		*out = UCLIMIT(t5 - t1);
		out += stride;
		*out = UCLIMIT(t5 + t1);
		out += stride;

		out -= stride << 3;
		++out;
		++o;
	}
}

#define DECODE(v, qs) \
{ \
	int t; \
	GET_BITS(2, nbb, bb, t); \
	if (t == 0) \
		(v) = 0; \
	else if (t == 2) \
		(v) = 1 << (qs); \
	else if (t == 3) \
		(v) = -1 << (qs); \
	else { \
		GET_BITS(8 - (qs), nbb, bb, t); \
		t <<= (qs); \
		if (t & 0x80) \
			t = -t | ~0x7f; \
		(v) = t; \
	} \
}

void BvcDecoder::decode_coef(int8_t* p, int active, int qs)
{
	int n;
	for (n = 0; ((1 << n) & active) != 0; ++n)
		;
	int v;
	GET_BITS(n, nbb_, bb_, v);
	if (v != 0) {
		int sign;
		GET_BITS(1, nbb_, bb_, sign);
		v <<= (7 - qs);
		if (qs < 7)
			v |= 1 << (6 - qs);
		if (sign)
			v = -v;
	}
	*p = v;
}

int BvcDecoder::prune_layers(int active)
{
	/*XXX do this with lookup table? */
	while (active != 0) {
		int v;
		GET_BITS(1, nbb_, bb_, v);
		if (v != 0)
			break;
		active >>= 1;
	}
	return (active);
}

void BvcDecoder::decode_sbc(int8_t* ll, int8_t* hl, int8_t* lh, int stride)
{
#ifdef notyet
	u_int bb = bb_;
	int nbb = nbb_;
#endif
	u_char stack[3*64];
	u_char* sp = stack;

	/*XXX dpcm & entropy */
	int v;
	GET_BITS(8, /*XXX*/nbb_, bb_, v);
	ll[0] = v;

	int hs = qs_[0];

	int active = prune_layers(0x7f);
	if (active == 0)
		return;
	int ga = prune_layers(active);

	*sp++ = ga;
	*sp++ = active|0x80;
	*sp++ = 9;
	*sp++ = ga;
	*sp++ = active|0x80;
	*sp++ = 8;
	*sp++ = ga;
	*sp++ = active|0x80;
	*sp++ = 1;

	do {
		int k = *--sp;
		int active = *--sp;
		int v = dchild_[k];
		int s = v & 7;

		decode_coef(&ll[k], active & 0x7f, s);
		if (active & 0x80) {
			active = *--sp;
			if (active & 0x80)
				abort();
			if (active == 0)
				continue;
			active = prune_layers(active);
			if (active == 0)
				continue;
			k = (v >> 3) & 3;
			if (k != 0) {
				int off;
				int8_t* p;
				if (k == 1) {
					p = lh;
					off = stride;
					s = hs;
				} else if (k == 2) {
					p = hl;
					off = stride;
					s = hs;
				} else {
					p = ll;
					off = 8;
					s = qs_[36];
				}
				k = (v >> 5) << 1;
				decode_coef(&p[k], active, s);
				decode_coef(&p[k + 1], active, s);
				decode_coef(&p[k + 8], active, s);
				decode_coef(&p[k + 9], active, s);
			} else {
				/*XXX*/
				k = v >> 5;
				if (k == 0)
					abort();

				*sp++ = active;
				*sp++ = k + 9;
				*sp++ = active;
				*sp++ = k + 8;
				*sp++ = active;
				*sp++ = k + 1;
				*sp++ = active;
				*sp++ = k;
			}
		} else {
			active = prune_layers(active);
			if (active == 0)
				continue;

			k = (v >> 3) & 3;
			if (k != 0) {
				int off;
				int8_t* p;
				if (k == 1) {
					p = lh;
					off = stride;
					s = hs;
				} else if (k == 2) {
					p = hl;
					off = stride;
					s = hs;
				} else {
					p = ll;
					off = 8;
					s = qs_[36];
				}
				k = (v >> 5) << 1;
				decode_coef(&p[k], active, s);
				decode_coef(&p[k + 1], active, s);
				decode_coef(&p[k + 8], active, s);
				decode_coef(&p[k + 9], active, s);
			} else {
				/*XXX*/
				k = v >> 5;
				if (k == 0)
					abort();

				int ga = prune_layers(active);
				*sp++ = ga;
				*sp++ = active|0x80;
				*sp++ = k + 9;
				*sp++ = ga;
				*sp++ = active|0x80;
				*sp++ = k + 8;
				*sp++ = ga;
				*sp++ = active|0x80;
				*sp++ = k + 1;
				*sp++ = ga;
				*sp++ = active|0x80;
				*sp++ = k;
			}
		}
	} while (sp > stack);

#ifdef notyet
	bb_ = bb;
	nbb_ = nbb;
#endif
}

inline void zeroblk(int8_t* p, int stride)
{
	for (int h = 8; --h >= 0; ) {
		*(u_int32_t*)p = 0;
		*(u_int32_t*)(p + 4) = 0;
		p += stride;
	}
}

void BvcDecoder::decode_block(int x, int y)
{
	int stride = (inw_ + LINEPAD) >> 1;
	int off = (y >> 1) * stride + (x >> 1);

	int8_t* ll = ll_ + off;
	int8_t* hl = hl_ + off;
	int8_t* lh = lh_ + off;

	zeroblk(hl, stride);
	zeroblk(lh, stride);
	int8_t LL[64];
	memset(LL, 0, sizeof(LL));
	decode_sbc(LL, hl, lh, stride);
	haar_rev(LL, ll, stride);

	int8_t junk[64];
	memset(LL, 0, sizeof(LL));
	decode_sbc(LL, junk, junk, 8);
	int s = inw_ * inh_;
	int8_t* chm = (int8_t*)frm_ + s;
	chm += (y >> 1) * (inw_ >> 1) + (x >> 1);
	haar_rev(LL, chm, inw_ >> 1);
	memset(LL, 0, sizeof(LL));
	decode_sbc(LL, junk, junk, 8);
	haar_rev(LL, chm + (s >> 2), inw_ >> 1);

	/*
	 * XXX this needs a comment; esp. subtlety about sign change
	 * on lh extension.
	 */

	if (y == 0) {
		memcpy(ll - stride, ll, 8 * sizeof(*ll));
		memcpy(hl - stride, hl, 8 * sizeof(*hl));
		for (int k = 0; k < 8; ++k)
			lh[-stride + k] = -lh[k];
	} else if (y == inh_ - 16 /*XXX*/) {
		ll += stride << 3;
		lh += stride << 3;
		hl += stride << 3;
		memcpy(ll, ll - stride, 8 * sizeof(*ll));
		memcpy(hl, hl - stride, 8 * sizeof(*hl));
		for (int k = 0; k < 8; ++k)
			lh[k] = -lh[-stride + k];
		ll -= stride << 3;
		lh -= stride << 3;
		hl -= stride << 3;
	}
	if (x == 0) {
		for (int k = 0; k < 8; ++k) {
			ll[-1] = ll[0];
			ll += stride;
			hl[-1] = hl[0];
			hl += stride;
			lh[-1] = -lh[0];
			lh += stride;
		}
	} else if (x == inw_ - 16 /*XXX*/) {
		ll += 8;
		lh += 8;
		hl += 8;
		for (int k = 0; k < 8; ++k) {
			ll[0] = ll[-1];
			ll += stride;
			hl[0] = hl[-1];
			hl += stride;
			lh[0] = -lh[-1];
			lh += stride;
		}
	}
}

void BvcDecoder::decode(const u_char* bp, int cc, int blkno)
{
	inb_ = bp;
	end_ = bp + cc;
	nbb_ = 0;

	/*XXX*/
	int blkw = inw_ >> 4;

	while (inb_ + 5 < end_) {
		int v;
		GET_BITS(1, nbb_, bb_, v);
		if (v == 1)
			blkno += 1;
		else {
			GET_BITS(1, nbb_, bb_, v);
			if (v == 1) {
				GET_BITS(4, nbb_, bb_, v);
				v += 2;
			} else {
				GET_BITS(11, nbb_, bb_, v);
			}
			blkno += v;
		}
		int y = blkno / blkw;
		int x = blkno - y * blkw;

		x <<= 4;
		y <<= 4;
		if ((unsigned)x > inw_ || (unsigned)y > inh_) {
			count(STAT_BAD_SYNTAX);
			return;
		}
		ndblk_++;
		decode_block(x, y);

		/*XXX*/
		/* remember that we need to reconstruct this block */
		blk_[(x >> 4) + blkw * (y >> 4)] = 1;

		/*XXX*/
		int k = (x >> 3) + (inw_ >> 3) * (y >> 3);
		rvts_[k] = now_;
		rvts_[k + 1] = now_;
		k += inw_ >> 3;
		rvts_[k] = now_;
		rvts_[k + 1] = now_;
	}

#ifdef notdef
	if (Lflag) {
		u_char* p = frm_;
		int16_t* ll = ll_;
		int16_t* lh = lh_;
		int16_t* hl = hl_;
		for (int y = 0; y < 120; ++y) {
			for (int x = 0; x < 160; ++x) {
#define LPIX(v) (((v) < 0) ? 0 : ((v) > 255) ? 255 : (v))
#define HPIX(v) LPIX(4 * (v) + 128)
				int v = ll[x];
				p[0] = HPIX(v);
				v = lh[x];
				p[160] = HPIX(v);
				v = hl[x];
				p[320*120] = HPIX(v);
				p[320*120 + 160] = HPIX(128);

				++p;
			}
			ll += (inw_ + LINEPAD) >> 1;
			lh += (inw_ + LINEPAD) >> 1;
			hl += (inw_ + LINEPAD) >> 1;
			p += 160;
		}
	}
#endif
}

void BvcDecoder::recv(pktbuf* pb)
{
	rtphdr* rh = (rtphdr*)pb->dp;
	int hdrsize = sizeof(rtphdr) + hdrlen();
	u_char* bp = pb->dp + hdrsize;
	int cc = pb->len - hdrsize;

	const bvchdr* ph = (bvchdr*)(rh + 1);
	int w = ph->width << 3;
	int h = ph->height << 3;
	if (w != inw_ || h != inh_) {
		if ((unsigned)w > 1000 || (unsigned)h > 1000) {
			/*XXX*/
			w = 320;
			h = 240;
			count(STAT_BAD_GEOM);
		}
		resize(w, h);
	}

	if (ph->quant != quant_) {
		quantizer(ntohl(ph->quant), ntohl(quant_));
		quant_ = ph->quant;
	}
	int blkno = htons(ph->blkno);
	//NOT-USED timeval t = unixtime();
	decode(bp, cc, blkno << 16 >> 16);
	if (ntohs(rh->rh_flags) & RTP_M) {
		sync();
		render_frame(frm_);
		resetndblk();
	}
	pb->release();
}

void BvcDecoder::sync()
{
	u_char* p = blk_;
	int blkw = inw_ >> 4;
	int blkh = inh_ >> 4;
	for (int y = 0; y < blkh; ++y) {
		for (int x = 0; x < blkw; ++x) {
			if (*p != 0) {
				*p = 0;
				reconstruct_block(x << 4, y << 4);
			}
			++p;
		}
	}
}

/*
 * XXX should use PlaneDecoder class for 420 also
 */

int BvcDecoder::colorhist(u_int* hist) const
{
	int s = inw_ * inh_;
	colorhist_420_556(hist, frm_, frm_ + s, frm_ + s + (s >> 2),
			  inw_, inh_);
	return (1);
}

void BvcDecoder::redraw()
{
	Decoder::redraw(frm_);
}
