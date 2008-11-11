/* $Header$ */

#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tcl.h"

#include "config.h"
#include "net.h"
#include "rtp.h"
#include "crdef.h"
#include "pktbuf-rtp.h"
#include "module.h"
#include "transmitter.h"

#define HDRSIZE (sizeof(rtphdr) + sizeof(bvchdr))

class BvcEncoder : public TransmitterModule {
 public:
	BvcEncoder();
	~BvcEncoder();
	virtual int consume(const VideoFrame*);
	virtual int command(int argc, const char*const* argv);
 protected:
	void size(int w, int h);
	pktbuf* getpkt(u_int32_t ts, int layer);
	int flush(pktbuf* pb, int sync);
	void quantizer(int n, int q);
	void encode_sbc(const u_char* blk);
	void encode_block(const u_char* blk);
	void encode_color(const u_char* blk);
	void decompose_block(const u_char* in, int8_t* out);
	int prune_layers(int active, int cm);
	void encode_coef(int v, int active);

	u_int32_t quant_;

	/* bit buffer */
	u_int bb_;
	u_int nbb_;

	int blkno_;

	u_char* bs_;
	u_char* es_;

	u_char qs_[64];
};

static class BvcEncoderMatcher : public Matcher {
public:
	BvcEncoderMatcher() : Matcher("module") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "bvc") == 0)
			return (new BvcEncoder);
		return (0);
	}
} encoder_matcher_bvc;

/*XXX use 64-bits if available */
#define NBIT 32

#if BYTE_ORDER == LITTLE_ENDIAN
#define STORE_BITS(bs, bb) \
	((u_char*)bs)[0] = bb >> 24; \
	((u_char*)bs)[1] = bb >> 16; \
	((u_char*)bs)[2] = bb >> 8; \
	((u_char*)bs)[3] = bb;
#else
#define STORE_BITS(bs, bb) *(u_int*)(bs) = (bb);
#endif

#define PUT_BITS(bits, n, nbb, bb, bs) \
{ \
	nbb += (n); \
	if (nbb > NBIT)  { \
		int extra = (nbb) - NBIT; \
		bb |= (bits) >> extra; \
		STORE_BITS(bs, bb) \
		bs += (NBIT / 8); \
		bb = (bits) << (NBIT - extra);\
		nbb = extra; \
	} else \
		bb |= (bits) << (NBIT - (nbb)); \
}

BvcEncoder::BvcEncoder() : TransmitterModule(FT_YUV_420)
{
	bs_ = es_ = 0;
	quant_ = 0;

	quantizer(0, 0);
	quantizer(1, 0);
	quantizer(2, 0);
	quantizer(3, 2);
	quantizer(4, 2);
	quantizer(5, 2);
	quantizer(6, 3);
	quantizer(7, 3);
	quantizer(8, 4);
	quantizer(9, 4);
}

BvcEncoder::~BvcEncoder()
{
}

/*XXX share with h261*/
int BvcEncoder::flush(pktbuf* pb, int sync)
{
	/* flush bit buffer */
	STORE_BITS(bs_, bb_);

	//int cc = bs_ - (u_char*)pb->iov[1].iov_base;
	int cc = bs_ - (u_char*)&pb->data[HDRSIZE];
	cc += nbb_ >> 3;
	int nbit = nbb_ & 7;
	if (nbit != 0)
		cc += 1;
	//SV-XXX:unused: int ebit = (8 - nbit) & 7;

	/*XXX*/
	if (cc == 0 && sync == 0)
		abort();

	//pb->iov[0].iov_len = sizeof(rtphdr) + sizeof(bvchdr);
	//pb->iov[1].iov_len = cc;
	pb->len = cc + HDRSIZE;
	//rtphdr* rh = (rtphdr*)pb->iov[0].iov_base;
	rtphdr* rh = (rtphdr*)pb->data;
	if (sync)
		rh->rh_flags |= htons(RTP_M);

	//SV-XXX: unused: bvchdr* bh = (bvchdr*)(rh + 1);

	tx_->send(pb);

	return (cc + HDRSIZE);
}

/*XXX*/
extern void setquant(u_char*, int n, int q);

/*
 * encoder q $n $q
 */
int BvcEncoder::command(int argc, const char*const* argv)
{
	if (argc == 4) {
		if (strcmp(argv[1], "q") == 0) {
			int n = atoi(argv[2]);
			int q = atoi(argv[3]);
			quantizer(n, q);
			return (TCL_OK);
		}
	}
	return (TransmitterModule::command(argc, argv));
}

void BvcEncoder::quantizer(int n, int q)
{
	int m = 7 << (3 * n);
	quant_ = (quant_ & ~m) | q << (3 * n);
	setquant(qs_, n, q);
}

#define CMAX(a,b) ((a) > (b) ? (a) : (b))
void BvcEncoder::decompose_block(const u_char* in, int8_t* out)
{
	int32_t scratch[CMAX(2*8*18/4, 64)];
	u_int8_t* l = (u_int8_t*)scratch;
	int8_t* h = (int8_t*)&l[8*18];
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
			v >>= 2;
			if (v & ~0xff)
				v = (v < 0) ? 0 : 255;
			*cl = v;
			cl += 18;

			/* 1 -3 3 -1 */
			v = in0 - m1 + m2 - in3;
			v >>= 2;
			if ((v + 127) & ~0xff)
				v = (v < -128) ? -128 : 127;
			*ch = v;
			ch += 18;

			in0 = in2;
			in1 = in3;
		}
	}
	/* rows */
	const u_int8_t* p = (u_int8_t*)scratch;
	l = (u_int8_t*)out;
	h = out + 2*64;
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
			v >>= 2;
			if (v & ~0xff)
				v = (v < 0) ? 0 : 255;
			*l++ = v;
			/* 1 -3 3 -1 */
			v = in0 - m1 + m2 - in3;
			v >>= 2;
			if ((v + 127) & ~0xff)
				v = (v < -128) ? -128 : 127;
			*h++ = v;

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
			*l++ = v >> 2;
#ifdef HAVE_HH
			/* 1 -3 3 -1 */
			v = in0 - m1 + m2 - in3;
			*h++ = v >> 2;
#endif

			in0 = in2;
			in1 = in3;
		}
	}

	/* now apply fully separable Haar tranform remaining 8x8 subimage */

	/* columns */
	l = (u_int8_t*)out;
	int32_t* o = scratch;
	int k;
	for (k = 8; --k >= 0; ) {
		/* use fixed point arithmetic with 2 bits of fraction */
		int t0 = l[0*8];
		int t1 = l[1*8];
		int t2 = t0 + t1;
		o[4*8] = (t1 - t0) << 2;

		t0 = l[2*8];
		t1 = l[3*8];
		int t3 = t0 + t1;
		o[5*8] = (t1 - t0) << 2;

		t0 = l[4*8];
		t1 = l[5*8];
		int t4 = t0 + t1;
		o[6*8] = (t1 - t0) << 2;

		t0 = l[6*8];
		t1 = l[7*8];
		int t5 = t0 + t1;
		o[7*8] = (t1 - t0) << 2;

		t0 = t2 + t3;
		t1 = t4 + t5;
		o[0*8]  = t0 + t1;
		o[1*8]  = t1 - t0;
		o[2*8] = (t3 - t2) << 1;
		o[3*8] = (t5 - t4) << 1;

		++o;
		++l;
	}
	/* rows */
	o = scratch;
	l = (u_int8_t*)out;
	for (k = 8; --k >= 0; ) {
		int t0 = o[0];
		int t1 = o[1];
		int t2 = t0 + t1;
		l[4] = (t1 - t0 + 7) >> 4;

		t0 = o[2];
		t1 = o[3];
		int t3 = t0 + t1;
		l[5] = (t1 - t0 + 7) >> 4;

		t0 = o[4];
		t1 = o[5];
		int t4 = t0 + t1;
		l[6] = (t1 - t0 + 7) >> 4;

		t0 = o[6];
		t1 = o[7];
		int t5 = t0 + t1;
		l[7] = (t1 - t0 + 7) >> 4;

		t0 = t2 + t3;
		t1 = t4 + t5;
		l[0] = (t0 + t1 + 31) >> 6;
		l[1] = (t1 - t0 + 31) >> 6;
		l[2] = (t3 - t2 + 15) >> 5;
		l[3] = (t5 - t4 + 15) >> 5;

		l += 8;
		o += 8;
	}
}

void haar_fwd(const u_int8_t* in, u_int8_t* out, int stride)
{
	int32_t scratch[64];
	u_int8_t* l = (u_int8_t*)in;
	int32_t* o = scratch;
	/* XXX would be more efficient here to do rows first */
	int k;
	for (k = 8; --k >= 0; ) {
		/* use fixed point arithmetic with 2 bits of fraction */
		int t0 = l[0];
		int t1 = l[stride];
		int t2 = t0 + t1;
		o[4*8] = (t1 - t0) << 2;

		t0 = l[2*stride];
		t1 = l[3*stride];
		int t3 = t0 + t1;
		o[5*8] = (t1 - t0) << 2;

		t0 = l[4*stride];
		t1 = l[5*stride];
		int t4 = t0 + t1;
		o[6*8] = (t1 - t0) << 2;

		t0 = l[6*stride];
		t1 = l[7*stride];
		int t5 = t0 + t1;
		o[7*8] = (t1 - t0) << 2;

		t0 = t2 + t3;
		t1 = t4 + t5;
		o[0*8]  = t0 + t1;
		o[1*8]  = t1 - t0;
		o[2*8] = (t3 - t2) << 1;
		o[3*8] = (t5 - t4) << 1;

		++o;
		++l;
	}
	/* rows */
	o = scratch;
	l = (u_int8_t*)out;
	for (k = 8; --k >= 0; ) {
		int t0 = o[0];
		int t1 = o[1];
		int t2 = t0 + t1;
		l[4] = (t1 - t0 + 7) >> 4;

		t0 = o[2];
		t1 = o[3];
		int t3 = t0 + t1;
		l[5] = (t1 - t0 + 7) >> 4;

		t0 = o[4];
		t1 = o[5];
		int t4 = t0 + t1;
		l[6] = (t1 - t0 + 7) >> 4;

		t0 = o[6];
		t1 = o[7];
		int t5 = t0 + t1;
		l[7] = (t1 - t0 + 7) >> 4;

		t0 = t2 + t3;
		t1 = t4 + t5;
		l[0] = (t0 + t1 + 31) >> 6;
		l[1] = (t1 - t0 + 31) >> 6;
		l[2] = (t3 - t2 + 15) >> 5;
		l[3] = (t5 - t4 + 15) >> 5;

		l += 8;
		o += 8;
	}
}

struct { 
	int zt[4];
	int sbc[4];
	int sbz[4];
	int dc;
	int mba;
} b;

u_char child[] = {
	0, 2, 4, 6, 64, 66, 68, 70,
	16, 18, 20, 22, 80, 82, 84, 86,
	32, 34, 36+156, 38+156, 96, 98, 100, 102,
	48, 50, 52+156, 54+156, 112, 114, 116, 118,
	128, 130, 132, 134, 0, 0, 0, 0,
	144, 146, 148, 150, 0, 0, 0, 0,
	160, 162, 164, 166, 0, 0, 0, 0,
	176, 178, 180, 182, 0, 0, 0, 0,
};

/*XXX*/
static u_char depth[] = {
	0, 0, 1, 1, 2, 2, 2, 2,
	0, 0, 1, 1, 2, 2, 2, 2,
	1, 1, 1, 1, 2, 2, 2, 2,
	1, 1, 1, 1, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2,
};

FILE* f[4];

/*
 * quantize and point into sign-magnitude representation
 */
void smquant(u_int8_t* p, u_char* qs)
{
	for (int i = 1; i < 3*64; ++i) {
		int q = (i < 64) ? qs[i] : qs[0];
		int v = p[i];
		int s = v & 0x80;
		if (s != 0) {
			v = -v;
			v &= 0x7f;
		}
		v >>= (7 - q);
		p[i] = v | s;
	}
}

u_char msb[] = {
0x00, 0x01, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04,
0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
};

#undef BVC7
#ifdef BVC7
#define MSB(v) (v)
#else
#define MSB(v) msb[(v) & 0x7f]
#endif

void
find_children_0(const u_char* p, u_char* z)
{
	for (int k = 4; --k >= 0; ) {
		for (int i = 0; i < 8; i += 2) {
			int v = MSB(p[i]);
			v |= MSB(p[i + 1]);
			v |= MSB(p[i + 8]);
			v |= MSB(p[i + 9]);
			z[i >> 1] = v;
		}
		z += 8;
		p += 16;
	}
}

void
find_children_1(const u_char* p, u_char* iz, u_char* oz)
{
#define ONE_SET(k) \
	oz[k >> 1] = iz[k+0] | iz[k+1] | iz[k+8] | iz[k+9] |  \
		MSB(p[k+0]) | MSB(p[k+1]) | MSB(p[k+8]) | MSB(p[k+9]);

	ONE_SET(0);
	ONE_SET(2);
	ONE_SET(16);
	ONE_SET(18);
}

void
find_children_2(const u_char* p, u_char* iz, u_char* oz)
{
	ONE_SET(0);
}

void find_children(const u_int8_t* p, u_char* z)
{
	find_children_0(p + 64, z + 4);
	find_children_0(p + 128, z + 32);
#ifdef HAVE_HH
	find_children_0(p + 192, z + 36);
#else
	/*XXX*/
	z[36] = z[37] = z[38] = z[39] = 0;
	z[44] = z[45] = z[46] = z[47] = 0;
	z[52] = z[53] = z[54] = z[55] = 0;
	z[60] = z[61] = z[62] = z[63] = 0;
#endif
	find_children_1(p + 4, z + 4, z + 2);
	find_children_1(p + 32, z + 32, z + 16);
	find_children_1(p + 36, z + 36, z + 18);

	find_children_2(p + 2, z + 2, z + 1);
	find_children_2(p + 16, z + 16, z + 8);
	find_children_2(p + 18, z + 18, z + 9);

	int ll = z[1] | z[8] | z[9];
	ll |= MSB(p[1]) | MSB(p[8]) | MSB(p[9]);
	z[0] = ll;
}

void BvcEncoder::encode_coef(int v, int active)
{
	int sign = v >> 7;
	v &= 0x7f;
	if ((v & ~active) != 0 || active == 0)
		abort();
	int n;
	for (n = 0; ((1 << n) & active) != 0; ++n)
		;
	PUT_BITS(v, n, nbb_, bb_, bs_);
	if (v != 0) {
		PUT_BITS(sign, 1, nbb_, bb_, bs_);
	}
}

/*
 * "active" is of form 000...011...1
 */
int BvcEncoder::prune_layers(int active, int cm)
{
	if (active == 0)
		abort();

	int next;
	int n = 0;
	while (((next = active >> 1) & cm) == cm) {
		++n;
		active = next;
		if (active == 0)
			break;
	}
	if (active == 0) {
		PUT_BITS(0, n, nbb_, bb_, bs_);
	} else {
		PUT_BITS(1, n + 1, nbb_, bb_, bs_);
	}
	return (active);
}

void find_grandchildren(const u_char* children, u_char* grandchildren)
{
	/* LL term has only 3 children */

	grandchildren[0] = children[1];
	grandchildren[0] |= children[8];
	grandchildren[0] |= children[9];

	for (int i = 1; i < 64; ++i) {
		int k = child[i];
		if (k > 64)
			grandchildren[i] = 0;
		else {
			grandchildren[i] = children[k];
			grandchildren[i] |= children[k + 1];
			grandchildren[i] |= children[k + 8];
			grandchildren[i] |= children[k + 9];
		}
	}
}

void BvcEncoder::encode_sbc(const u_int8_t* p)
{
#ifdef notyet
	u_int bb = bb_;
	int nbb = nbb_;
#endif
	u_char stack[3*64];
	u_char* sp = stack;
	u_char children[64];
	u_char grandchildren[64];
	
        smquant((u_int8_t*)p, qs_);
	find_children(p, children);
	find_grandchildren(children, grandchildren);

	/*XXX dpcm & entropy */
	PUT_BITS(p[0], 8, nbb_, bb_, bs_);
	b.dc += 8;

	int active = prune_layers(0x7f, children[0]);
	if (active == 0)
		return;
	int ga = prune_layers(active, grandchildren[0]);
	
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

		encode_coef(p[k], active & 0x7f);
		if (active & 0x80) {
			active = *--sp;
			if (active & 0x80)
				abort();
			if (active == 0)
				continue;
			active = prune_layers(active, children[k]);
			if (active == 0)
				continue;
			k = child[k];
			if (k < 64) {
				*sp++ = active;
				*sp++ = k + 9;
				*sp++ = active;
				*sp++ = k + 8;
				*sp++ = active;
				*sp++ = k + 1;
				*sp++ = active;
				*sp++ = k;
			} else {
#ifndef HAVE_HH
				if (k >= 192)
					k -= 156;
#endif
				encode_coef(p[k], active);
				encode_coef(p[k + 1], active);
				encode_coef(p[k + 8], active);
				encode_coef(p[k + 9], active);
			}
		} else {
			active = prune_layers(active, children[k]);
			if (active == 0)
				continue;
			int c = child[k];
			if (c < 64) {
				int ga = prune_layers(active, grandchildren[k]);
				k = c;
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
			} else {
				k = c;
#ifndef HAVE_HH
				if (k >= 192)
					k -= 156;
#endif
				encode_coef(p[k], active);
				encode_coef(p[k + 1], active);
				encode_coef(p[k + 8], active);
				encode_coef(p[k + 9], active);
			}
		}

	} while (sp > stack);

#ifdef notyet
	bb_ = bb;
	nbb_ = nbb;
#endif
}

extern "C" void malloc_verify();

void BvcEncoder::encode_block(const u_char* in)
{
	int8_t out[3*8*8];
	decompose_block(in, out);
	encode_sbc((u_int8_t*)out);
}

void BvcEncoder::encode_color(const u_char* in)
{
	int8_t out[3*8*8];

	memset(&out[64], 0, 128);
	haar_fwd(in, (u_int8_t*)out, width_ >> 1);
	encode_sbc((u_int8_t*)out);
}

pktbuf* BvcEncoder::getpkt(u_int32_t ts, int layer)
{
	//Transmitter::pktbuf* pb = tx_->alloc(ts, RTP_PT_BVC);
	pktbuf* pb = pool_->alloc(ts, RTP_PT_BVC);
	pb->layer = layer;
	//rtphdr* rh = (rtphdr*)pb->iov[0].iov_base;
	rtphdr* rh = (rtphdr*)pb->data;
	bvchdr* bh = (bvchdr*)(rh + 1);
	bh->version = 0;
	bh->pad = 0;
	bh->width = width_ >> 3;
	bh->height = height_ >> 3;
	bh->quant = htonl(quant_);
	bh->blkno = htons(blkno_);
	//bs_ = (u_char*)pb->iov[1].iov_base;
	bs_ = (u_char*)&pb->data[HDRSIZE];
	es_ = bs_ + tx_->mtu() - HDRSIZE;
	bb_ = 0;
	nbb_ = 0;
	return (pb);
}

/*XXX*/
int BvcEncoder::consume(const VideoFrame* vf)
{
	if (!samesize(vf))
		size(vf->width_, vf->height_);
	YuvFrame* p = (YuvFrame*)vf;
	tx_->flush();
	int layer = p->layer_;

	u_char* frm = (u_char*)p->bp_;
	const u_char* chm = frm + framesize_;
	blkno_ = -1;
	pktbuf* pb = getpkt(p->ts_, layer);
	const u_int8_t* crv = p->crvec_;
	memset(&b, 0, sizeof(b));

#ifdef SBC_STAT
	if (f[0] == 0) {
		f[0] = fopen("sbc0", "w");
		f[1] = fopen("sbc1", "w");
		f[2] = fopen("sbc2", "w");
		f[3] = fopen("sbc3", "w");
	}
#endif
	int cc = 0;
	int blkw = width_ >> 4;
	int blkh = height_ >> 4;
	int blkno = 0;
	for (int y = 0; y < blkh; ++y) {
		for (int x = 0; x < blkw; ++blkno, frm += 16, chm += 8,
		     ++x, ++crv) {
			int s = crv[0];
			if ((s & CR_SEND) == 0)
				continue;

#define MAXMBSIZE (16*16+2*8*8+4)
			if (bs_ + MAXMBSIZE >= es_) {
				cc += flush(pb, 0);
				pb = getpkt(p->ts_, layer);
			}

			int dblk = blkno - blkno_;
			blkno_ = blkno;
			if (dblk <= 0)
				abort();

			if (dblk == 1) {
				PUT_BITS(1, 1, nbb_, bb_, bs_);
b.mba += 1;
			} else if (dblk <= 17) {
				PUT_BITS(0x10 | (dblk - 2), 6, nbb_, bb_, bs_);
b.mba += 6;
			} else {
				PUT_BITS(dblk, 13, nbb_, bb_, bs_);
b.mba += 13;
			}
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
			else if (y == blkh - 1) {
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
			} else if (x == blkw - 1) {
				u_char* s = xsave;
				u_char* p = frm + 16;
				for (int k = 0; k < 16; ++k) {
					*s++ = p[0];
					p[0] = p[-1];
					p += width_;
				}
			}
			encode_block(frm);
			encode_color(chm);
			encode_color(chm + (framesize_ >> 2));
			/*
			 * Now restore the pixels.  We don't bother
			 * with the "y = 0" case because the affected
			 * memory will not be used.
			 */
			if (y == blkh - 1) {
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
			} else if (x == blkw - 1) {
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
#ifdef notdef
	double t = b.dc + b.mba;
	for (int i = 0; i < 4; ++i)
		t += b.zt[i] + b.sbc[i] + b.sbz[i];
	printf("dc\t%.3f\n", b.dc / t);
	printf("mba\t%.3f\n", b.mba / t);
	for (i = 0; i < 4; ++i) {
		printf("sbc%d\t%.3f (%.3f)\n", i, (b.sbc[i] + b.sbz[i]) / t,
		       b.sbz[i] / double(b.sbc[i] + b.sbz[i]));
		printf("zt%d\t%.3f\n", i, b.zt[i] / t);
	}
#endif
	cc += flush(pb, 1);
	return (cc);
}

void BvcEncoder::size(int w, int h)
{
	if ((w | h) & 0xf) {
		printf("vic: bad size in bvc coder (%dx%d)\n", w, h);
		abort();
	}
	Module::size(w, h);
}
