#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include "../config.h"
//#include <winsock.h>
#else
#include <sys/param.h>
#include <sys/file.h>
#endif
#include <sys/stat.h>

#include "p64as.h"
#include "p64-huff.h"
#include "dct.h"
#include "bsd-endian.h"

#define inverseDCT( block, mask, image, rowstride, merge ) \
  rdct( block, mask, image, rowstride, merge )

void P64ASDecoder::err(const char* msg ...) const
{
#ifdef DEVELOPMENT_VERSION
	va_list ap;
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	fprintf(stderr, " @g%d m%d %d/%d of %d/%d: %04x %04x %04x %04x|%04x\n",
		gob_, mba_,
		(int)((u_char*)bs_ - (u_char*)ps_), nbb_,
		(int)((u_char*)es_ - (u_char*)ps_), pebit_,
	       bs_[-4], bs_[-3], bs_[-2], bs_[-1], bs_[0]);
#else
	UNUSED(msg);
#endif
}

P64ASDecoder::P64ASDecoder() : 
  fs_(0), 
  front_(0), 
  back_(0),
  width_(0),
  height_(0),
  ngob_(0), 
  maxgob_(0),
  ndblk_(0),
  gobquant_(0), 
  mt_(0), 
  gob_(0), 
  mba_(0), 
  mvdh_(0), 
  mvdv_(0), 
  marks_(0), 
  mark_(0),
  bad_psc_(0), 
  bad_bits_(0), 
  bad_GOBno_(0), 
  bad_fmt_(0),
  coord_(0),
  mb_state_(0)
{
  inithuff();
  initquant();
}

P64ASDecoder::~P64ASDecoder()
{
  delete [] fs_;
}

void 
P64ASDecoder::init()
{
  //memset(mb_state_, MBST_OLD, sizeof(mb_state_));

  minx_ = width_;
  miny_ = height_;
  maxx_ = 0;
  maxy_ = 0;
  
  front_ = 0;
  back_ = 0;

}

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

/*
 * Set up the huffman tables.
 */
void 
P64ASDecoder::inithuff()
{
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

int 
P64ASDecoder::quantize(int v, int q)
{
  if (v > 0) 
    return (((v << 1) + 1) * q) - (~q & 1);
  else 
    return (((v << 1) - 1) * q) + (~q & 1);
}

/*
 * Build quantization lookup table.
 * One for each possible MQUANT paramenter.
 */
void 
P64ASDecoder::initquant()
{
  for (int mq = 0; mq < 32; ++mq) {
    short* qt = &quant_[mq << 8];
    for (int v = 0; v < 256; ++v) {
      int s = (v << 24) >> 24;
      qt[v] = quantize(s, mq);
    }
  }
}

/*
 * Decode the next block of transform coefficients
 * from the input stream.
 * Return number of non-zero ac coefficients.
 */
#ifdef INT_64
int P64ASDecoder::parse_block(short* blk, INT_64* mask)
#else
int P64ASDecoder::parse_block(short* blk, u_int* mask)
#endif
{
#ifdef INT_64
  INT_64 m0 = 0;
#else
  u_int m1 = 0, m0 = 0;
#endif
  /*
   * Cache bit buffer in registers.
   */
  register int nbb = nbb_;
  register int bb = bb_;
  register short* qt = qt_;

  for (int i = 0; i < 64; i ++ ) {
    blk[i] = 0;
  }
  int k;
  if ((mt_ & MT_CBP) == 0) {
    int v;
    GET_BITS(bs_, 8, nbb, bb, v);
    if (v == 255)
      v = 128;
    if (mt_ & MT_INTRA)
      v <<= 3;
    else
      v = qt[v];
    blk[0] = v;
    k = 1;
    m0 |= 1;
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
    int v;
    GET_BITS(bs_, 2, nbb, bb, v);
    /*XXX quantize?*/
    blk[0] = qt[(v & 1) ? 0xff : 1];
    k = 1;
    m0 |= 1;
  } else {
    k = 0;
#ifndef INT_64
    blk[0] = 0;/*XXX need this because the way we set bits below*/
#endif
  }
  int nc = 0;
  for (;;) {
    int r, v;
    HUFF_DECODE(bs_, ht_tcoeff_, nbb, bb, r);
    if (r <= 0) {
      /* SYM_EOB, SYM_ILLEGAL, or SYM_ESCAPE */
      if (r == SYM_ESCAPE) {
	GET_BITS(bs_, 14, nbb, bb, r);
	v = r & 0xff;
	r >>= 8;
      } else {
	if (r == SYM_ILLEGAL) {
	  bb_ = bb;
	  nbb_ = nbb;
	  err("illegal symbol in block");
	}
	/* EOB */
	break;
      }
    } else {
      v = (r << 22) >> 27;
      r = r & 0x1f;
    }
    k += r;
    if (k >= 64) {
      bb_ = bb;
      nbb_ = nbb;
      err("bad run length %d (r %d, v %d)", k, r, v);
      break;
    }
    r = COLZAG[k++];
    //r = ROWZAG[k++];
    blk[r] = qt[v & 0xff];
    ++nc;
#ifdef INT_64
    m0 |= (INT_64)1 << r;
#else
    if (r < 32)
      m0 |= 1 << r;
    else
      m1 |= 1 << (r - 32);
#endif
  }
  /*
   * Done reading input.  Update bit buffer.
   */
  bb_ = bb;
  nbb_ = nbb;

  *mask = m0;
#ifndef INT_64
  mask[1] = m1;
#endif
  return (nc);
}

/*
 * Parse a GOB header, which consists of the GOB quantiation
 * factor (GQUANT) and spare bytes that we ignore.
 */
int P64ASDecoder::parse_gob_hdr(int ebit)
{
  UNUSED(ebit);
  int gobu, gobm, gobl;

  mba_ = -1;

  GET_BITS(bs_, 7, nbb_, bb_, gobu);
  GET_BITS(bs_, 6, nbb_, bb_, gobm);
  GET_BITS(bs_, 7, nbb_, bb_, gobl);
  gob_ = (gobu << 13) | (gobm << 7) | (gobl);

  int mq;
  GET_BITS(bs_, 5, nbb_, bb_, mq);
  gobquant_ = mq;
  qt_ = &quant_[mq << 8];  

  return gob_;
}

/*
 * Parse a macroblock header.  If there is no mb header because
 * we hit the next start code, return -1, otherwise 0.
 */
int P64ASDecoder::parse_mb_hdr(u_int& cbp)
{
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
    return (SYM_ILLEGAL);
  }

  HUFF_DECODE(bs_, ht_mtype_, nbb_, bb_, mt_);
  if (mt_ & MT_MQUANT) {
    int mq;
    GET_BITS(bs_, 5, nbb_, bb_, mq);
    qt_ = &quant_[mq << 8];
  }

  /*
   * Coded block pattern.
   */
  if (mt_ & MT_CBP) {
    HUFF_DECODE(bs_, ht_cbp_, nbb_, bb_, cbp);
    if (cbp > 63) {
      err("cbp invalid %x", cbp);
      return (SYM_ILLEGAL);
    }
  } else
    cbp = 0x3f;
  
  return (1);
}

/*
 * Handle the next block in the current macroblock.
 * If tc is non-zero, then coefficients are present
 * in the input stream and they are parsed.  Otherwise,
 * coefficients are not present, but we take action
 * according to the type macroblock that we have.
 */
void P64ASDecoder::decode_block(u_int tc, 
				u_int x, 
				u_int y, 
				u_int stride,
				u_char* front, 
				u_char* back, 
				int sf)
{
  UNUSED(sf);
  short blk[64];
#ifdef INT_64
  INT_64 mask;
#define MASK_VAL	mask
#define MASK_REF	&mask
#else
  u_int mask[2];
#define MASK_VAL	mask[0], mask[1]
#define MASK_REF	mask
#endif
  int nc = 0;
  if (tc != 0)
    nc = parse_block(blk, MASK_REF);
  
  int off = y * stride + x;
  u_char* out = front + off;
  
  if (mt_ & MT_INTRA) {
    if (tc != 0) {
      if (nc == 0) {
	dcfill((blk[0] + 4) >> 3, out, stride);
      } else {
	inverseDCT(blk, MASK_VAL, out, stride, (u_char*)0);
      }
    } else {
      u_char* in = back + off;
      mvblka(in, out, stride);
    }
    return;
  }
}

/*
 * Decompress the next macroblock.  Return 0 if the macroblock
 * was present (with no errors).  Return SYM_STARTCODE (-1),
 * if there was no macroblock but instead the start of the
 * next GOB or picture (in which case the start code has
 * been consumed).  Return SYM_ILLEGAL (-2) if there was an error.
 */
int P64ASDecoder::decode_mb()
{
  u_int cbp;
  register int v;

  if ((v = parse_mb_hdr(cbp)) <= 0)
    return (v);
  
  /*
   * Lookup the base coordinate for this MBA.
   * Convert from a block to a pixel coord.
   */
  register u_int x, y;
  x = coord_[33 * gob_ + mba_];
  y = (x & 0xffff);
  x >>= 16;
  
  /* Update bounding box */
  if (x < minx_)
    minx_ = x;
  if (x > maxx_)
    maxx_ = x;
  if (y < miny_)
    miny_ = y;
  if (y > maxy_)
    maxy_ = y;

  /*
   * Decode the six blocks in the MB (4Y:1U:1V).
   * (This code assumes MT_TCOEFF is 1.)
   */
  register u_int tc = mt_ & MT_TCOEFF;
  register u_int s = width_;
  decode_block(tc & (cbp >> 5), x, y, s, front_, back_, 1);
  decode_block(tc & (cbp >> 4), x + 8, y, s, front_, back_, 1);
  decode_block(tc & (cbp >> 3), x, y + 8, s, front_, back_, 1);
  decode_block(tc & (cbp >> 2), x + 8, y + 8, s, front_, back_, 1);
  s >>= 1;
  int off = size_;
  decode_block(tc & (cbp >> 1), x >> 1, y >> 1, s,
	       front_ + off, back_ + off, 2);
  off += size_ >> 2;
  decode_block(tc & (cbp >> 0), x >> 1, y >> 1, s,
	       front_ + off, back_ + off, 2);
  
  mbst_[mba_] = MBST_NEW;
	
  /*
   * If a marking table was attached, take note.
   * This allows us to dither only the blocks that have changed,
   * rather than the entire image on each frame.
   */
  if (marks_) {
    /* convert to 8x8 block offset */
    off = (x >> 3) + (y >> 3) * (width_ >> 3);
    int m = mark_;
    marks_[off] = m;
    marks_[off + 1] = m;
    off += width_ >> 3;
    marks_[off] = m;
    marks_[off + 1] = m;
  }
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
int P64ASDecoder::decode(const u_char* bp, 
			 int cc,
			 int ebit,
			 int quant)
{
	ps_ = (u_short*)bp;
	/*
	 * If cc is odd, ignore 8 extra bits in last short.
	 */
	int odd = cc & 1;
	ebit += odd << 3;
	pebit_ = ebit;
	es_ = (u_short*)(bp + ((cc - 1) &~ 1));

	/*
	 * If input buffer not aligned, prime bit-buffer
	 * with 8 bits; otherwise, prime it with a 16.
	 */
	if ((long)bp & 1) {
		bs_ = (u_short*)(bp + 1);
		bb_ = *bp;
		nbb_ = 8;
	} else {
		bs_ = (u_short*)bp;
		HUFFRQ(bs_, bb_);
		nbb_ = 16;
	}

	int gob = 0;
	qt_ = &quant_[quant << 8];
	
	while (bs_ < es_ || (bs_ == es_ && nbb_ > ebit)) {
		mbst_ = &mb_state_[gob * 33];

		ndblk_++;
		int v = decode_mb();
		if (v == 0)
			continue;

		if (v != SYM_STARTCODE) {
			err("expected GOB startcode");
			++bad_bits_;
			return (0);
		}
		gob = parse_gob_hdr(ebit);
		if (gob < 0) {
			/*XXX*/
			++bad_bits_;
			return (0);
		}
	}
	return (1);
}

void P64ASDecoder::sync()
{
	bbx_ = minx_;
	bby_ = miny_;
	bbw_ = maxx_ - minx_ + 16;
	bbh_ = maxy_ - miny_ + 16;

	minx_ = width_;
	miny_ = height_;
	maxx_ = 0;
	maxy_ = 0;

	maxgob_ = 0;
}

void P64ASDecoder::getPreviewLevelSize( u_short level, u_int &width, u_int &height )
{
  width = width_ >> level;
  height = height_ >> level;
}

void P64ASDecoder::mvblka(u_char* in, u_char* out, u_int stride)
{
#ifdef INT_64
	*(INT_64*)out = *(INT_64*)in;
	out += stride; in += stride;
	*(INT_64*)out = *(INT_64*)in;
	out += stride; in += stride;
	*(INT_64*)out = *(INT_64*)in;
	out += stride; in += stride;
	*(INT_64*)out = *(INT_64*)in;
	out += stride; in += stride;
	*(INT_64*)out = *(INT_64*)in;
	out += stride; in += stride;
	*(INT_64*)out = *(INT_64*)in;
	out += stride; in += stride;
	*(INT_64*)out = *(INT_64*)in;
	out += stride; in += stride;
	*(INT_64*)out = *(INT_64*)in;
#else
	for (int k = 8; --k >= 0; ) {
		*(u_int*)out = *(u_int*)in;
		*(u_int*)(out + 4) = *(u_int*)(in + 4);
		in += stride;
		out += stride;
	}
#endif
}

void 
P64ASDecoder::set_size(int w, int h)
{
  width_ = w;
  height_ = h;
  size_ = w * h;

  int hblks = w/16;
  int vblks = h/16;

  int blks = hblks * vblks;

  if (mb_state_) {
    delete [] mb_state_;
  }
  mb_state_ = new u_char[blks];
  memset(mb_state_, MBST_OLD, blks);

  if (coord_) {
    delete [] coord_;
  }

  coord_ = new u_int[blks];
  
  int i = 0;
  for (int y = 0; y < vblks; y ++) {
    for (int x = 0; x < hblks; x ++, i ++) {
      coord_[i] = (0xffff & (x * 16)) << 16 | (0xffff & (y * 16));
    }
  }

  if (front_) {
    delete [] front_;
  }
  if (back_) {
    delete [] back_;
  }

  int buffer_size = size_ + size_/2;

  front_ = new u_char[buffer_size];
  back_ = new u_char[buffer_size];

  memset(front_, 127, buffer_size);
  memset(back_, 127, buffer_size);

  sync();
}

