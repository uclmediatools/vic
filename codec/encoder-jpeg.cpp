/*
 * Copyright (c) 1994-1995 The Regents of the University of California.
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
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

static const char rcsid[] =
"@(#) $Header$";


#include <stdio.h>
#include <errno.h>
#include "inet.h"
#include "net.h"
#include "rtp.h"
#include "dct.h"
#include "bsd-endian.h"
#include "vic_tcl.h"
#include "crdef.h"
#include "transmitter.h"
#include "pktbuf-rtp.h"
#include "module.h"

#define HDRSIZE (sizeof(rtphdr) + 8)

#ifdef INT_64
#define NBIT 64
#define BB_INT INT_64
#else
#define NBIT 32
#define BB_INT u_int
#endif

#if NBIT == 64
#define STORE_BITS(bb, bc)						\
{									\
    u_char t = bb>>56;							\
    *bc++ = t;								\
    if (t == 0xff) *bc++ = 0;						\
    t = bb>>48;								\
    *bc++ = t;								\
    if (t == 0xff) *bc++ = 0;						\
    t = bb>>40;								\
    *bc++ = t;								\
    if (t == 0xff) *bc++ = 0;						\
    t = bb>>32;								\
    *bc++ = t;								\
    if (t == 0xff) *bc++ = 0;						\
    t = bb>>24;								\
    *bc++ = t;								\
    if (t == 0xff) *bc++ = 0;						\
    t = bb>>16;								\
    *bc++ = t;								\
    if (t == 0xff) *bc++ = 0;						\
    t = bb>>8;								\
    *bc++ = t;								\
    if (t == 0xff) *bc++ = 0;						\
    t = bb;								\
    *bc++ = t;								\
    if (t == 0xff) *bc++ = 0;						\
}

#else

#define STORE_BITS(bb, bc)						\
{									\
    u_char t = bb>>24;							\
    *bc++ = t;								\
    if (t == 0xff) *bc++ = 0;						\
    t = bb>>16;								\
    *bc++ = t;								\
    if (t == 0xff) *bc++ = 0;						\
    t = bb>>8;								\
    *bc++ = t;								\
    if (t == 0xff) *bc++ = 0;						\
    t = bb;								\
    *bc++ = t;								\
    if (t == 0xff) *bc++ = 0;						\
}

#endif

#define PUT_BITS(bits, n, nbb, bb, bc)					\
{									\
    nbb += (n);								\
    if (nbb > NBIT)  {							\
		u_int extra = (nbb) - NBIT;					\
		bb |= (BB_INT)(bits) >> extra;					\
		STORE_BITS(bb, bc)						\
		bb = (BB_INT)(bits) << (NBIT - extra);				\
		nbb = extra;							\
    } else								\
		bb |= (BB_INT)(bits) << (NBIT - (nbb));				\
}

class JpegEncoder : public TransmitterModule {
  public:
    JpegEncoder();
    void setq(int q);
    void size(int w, int h);
    int consume(const VideoFrame*);

    struct huffentry {
	u_short val;
	u_short nb;
    };

  protected:
    int command(int argc, const char*const* argv);

    int flush(pktbuf* pb, int nbit, pktbuf* npb);
    int encode(const VideoFrame*);
    void encode_blk(const short* blk, short* dcpred,
		    struct JpegEncoder::huffentry* dcht,
		    struct JpegEncoder::huffentry* acht);
    void encode_mcu(u_int mcu, const u_char* frm);
    void fdct(const u_char* in, int stride, short* out, const float* qt);

    /* bit buffer */
    BB_INT bb_;
    u_int nbb_;

    u_char* bs_;
    u_char* bc_;
    u_int offset_;

    u_char quant_;
    float lqt_[64];
    float cqt_[64];

    short lpred_;	// dc predictors
    short crpred_;
    short cbpred_;

    u_int nmcu_;
    u_char w_;			/* divided by 8 */
    u_char h_;
};

static class JpegEncoderMatcher : public Matcher {
    public:
	JpegEncoderMatcher() : Matcher("module") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "jpeg") == 0)
			return (new JpegEncoder);
		return (0);
	}
} encoder_matcher_jpeg;

static const int stdlqt[] = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99 };

static const int stdcqt[] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99 };

static struct JpegEncoder::huffentry ldht[] = {
    {0x0, 2}, {0x2, 3}, {0x3, 3}, {0x4, 3}, {0x5, 3}, {0x6, 3},
    {0xe, 4}, {0x1e, 5}, {0x3e, 6}, {0x7e, 7}, {0xfe, 8}, {0x1fe, 9}};

static struct JpegEncoder::huffentry cdht[] = {
    {0x0, 2}, {0x1, 2}, {0x2, 2}, {0x6, 3}, {0xe, 4}, {0x1e, 5}, {0x3e, 6},
    {0x7e, 7}, {0xfe, 8}, {0x1fe, 9}, {0x3fe, 10}, {0x7fe, 11}};

static struct JpegEncoder::huffentry laht[] = {
    {0x0a, 4}, {0x00, 2}, {0x01, 2}, {0x04, 3}, 
    {0x0b, 4}, {0x1a, 5}, {0x78, 7}, {0xf8, 8}, 
    {0x3f6, 10}, {0xff82, 16}, {0xff83, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x0c, 4}, {0x1b, 5}, {0x79, 7}, 
    {0x1f6, 9}, {0x7f6, 11}, {0xff84, 16}, {0xff85, 16}, 
    {0xff86, 16}, {0xff87, 16}, {0xff88, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x1c, 5}, {0xf9, 8}, {0x3f7, 10}, 
    {0xff4, 12}, {0xff89, 16}, {0xff8a, 16}, {0xff8b, 16}, 
    {0xff8c, 16}, {0xff8d, 16}, {0xff8e, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x3a, 6}, {0x1f7, 9}, {0xff5, 12}, 
    {0xff8f, 16}, {0xff90, 16}, {0xff91, 16}, {0xff92, 16}, 
    {0xff93, 16}, {0xff94, 16}, {0xff95, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x3b, 6}, {0x3f8, 10}, {0xff96, 16}, 
    {0xff97, 16}, {0xff98, 16}, {0xff99, 16}, {0xff9a, 16}, 
    {0xff9b, 16}, {0xff9c, 16}, {0xff9d, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x7a, 7}, {0x7f7, 11}, {0xff9e, 16}, 
    {0xff9f, 16}, {0xffa0, 16}, {0xffa1, 16}, {0xffa2, 16}, 
    {0xffa3, 16}, {0xffa4, 16}, {0xffa5, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x7b, 7}, {0xff6, 12}, {0xffa6, 16}, 
    {0xffa7, 16}, {0xffa8, 16}, {0xffa9, 16}, {0xffaa, 16}, 
    {0xffab, 16}, {0xffac, 16}, {0xffad, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0xfa, 8}, {0xff7, 12}, {0xffae, 16}, 
    {0xffaf, 16}, {0xffb0, 16}, {0xffb1, 16}, {0xffb2, 16}, 
    {0xffb3, 16}, {0xffb4, 16}, {0xffb5, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x1f8, 9}, {0x7fc0, 15}, {0xffb6, 16}, 
    {0xffb7, 16}, {0xffb8, 16}, {0xffb9, 16}, {0xffba, 16}, 
    {0xffbb, 16}, {0xffbc, 16}, {0xffbd, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x1f9, 9}, {0xffbe, 16}, {0xffbf, 16}, 
    {0xffc0, 16}, {0xffc1, 16}, {0xffc2, 16}, {0xffc3, 16}, 
    {0xffc4, 16}, {0xffc5, 16}, {0xffc6, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x1fa, 9}, {0xffc7, 16}, {0xffc8, 16}, 
    {0xffc9, 16}, {0xffca, 16}, {0xffcb, 16}, {0xffcc, 16}, 
    {0xffcd, 16}, {0xffce, 16}, {0xffcf, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x3f9, 10}, {0xffd0, 16}, {0xffd1, 16}, 
    {0xffd2, 16}, {0xffd3, 16}, {0xffd4, 16}, {0xffd5, 16}, 
    {0xffd6, 16}, {0xffd7, 16}, {0xffd8, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x3fa, 10}, {0xffd9, 16}, {0xffda, 16}, 
    {0xffdb, 16}, {0xffdc, 16}, {0xffdd, 16}, {0xffde, 16}, 
    {0xffdf, 16}, {0xffe0, 16}, {0xffe1, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x7f8, 11}, {0xffe2, 16}, {0xffe3, 16}, 
    {0xffe4, 16}, {0xffe5, 16}, {0xffe6, 16}, {0xffe7, 16}, 
    {0xffe8, 16}, {0xffe9, 16}, {0xffea, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0xffeb, 16}, {0xffec, 16}, {0xffed, 16}, 
    {0xffee, 16}, {0xffef, 16}, {0xfff0, 16}, {0xfff1, 16}, 
    {0xfff2, 16}, {0xfff3, 16}, {0xfff4, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x7f9, 11}, {0xfff5, 16}, {0xfff6, 16}, {0xfff7, 16}, 
    {0xfff8, 16}, {0xfff9, 16}, {0xfffa, 16}, {0xfffb, 16}, 
    {0xfffc, 16}, {0xfffd, 16}, {0xfffe, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0} };

static struct JpegEncoder::huffentry caht[] = {
    {0x00, 2}, {0x01, 2}, {0x04, 3}, {0x0a, 4}, 
    {0x18, 5}, {0x19, 5}, {0x38, 6}, {0x78, 7}, 
    {0x1f4, 9}, {0x3f6, 10}, {0xff4, 12}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x0b, 4}, {0x39, 6}, {0xf6, 8}, 
    {0x1f5, 9}, {0x7f6, 11}, {0xff5, 12}, {0xff88, 16}, 
    {0xff89, 16}, {0xff8a, 16}, {0xff8b, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x1a, 5}, {0xf7, 8}, {0x3f7, 10}, 
    {0xff6, 12}, {0x7fc2, 15}, {0xff8c, 16}, {0xff8d, 16}, 
    {0xff8e, 16}, {0xff8f, 16}, {0xff90, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x1b, 5}, {0xf8, 8}, {0x3f8, 10}, 
    {0xff7, 12}, {0xff91, 16}, {0xff92, 16}, {0xff93, 16}, 
    {0xff94, 16}, {0xff95, 16}, {0xff96, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x3a, 6}, {0x1f6, 9}, {0xff97, 16}, 
    {0xff98, 16}, {0xff99, 16}, {0xff9a, 16}, {0xff9b, 16}, 
    {0xff9c, 16}, {0xff9d, 16}, {0xff9e, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x3b, 6}, {0x3f9, 10}, {0xff9f, 16}, 
    {0xffa0, 16}, {0xffa1, 16}, {0xffa2, 16}, {0xffa3, 16}, 
    {0xffa4, 16}, {0xffa5, 16}, {0xffa6, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x79, 7}, {0x7f7, 11}, {0xffa7, 16}, 
    {0xffa8, 16}, {0xffa9, 16}, {0xffaa, 16}, {0xffab, 16}, 
    {0xffac, 16}, {0xffad, 16}, {0xffae, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x7a, 7}, {0x7f8, 11}, {0xffaf, 16}, 
    {0xffb0, 16}, {0xffb1, 16}, {0xffb2, 16}, {0xffb3, 16}, 
    {0xffb4, 16}, {0xffb5, 16}, {0xffb6, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0xf9, 8}, {0xffb7, 16}, {0xffb8, 16}, 
    {0xffb9, 16}, {0xffba, 16}, {0xffbb, 16}, {0xffbc, 16}, 
    {0xffbd, 16}, {0xffbe, 16}, {0xffbf, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x1f7, 9}, {0xffc0, 16}, {0xffc1, 16}, 
    {0xffc2, 16}, {0xffc3, 16}, {0xffc4, 16}, {0xffc5, 16}, 
    {0xffc6, 16}, {0xffc7, 16}, {0xffc8, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x1f8, 9}, {0xffc9, 16}, {0xffca, 16}, 
    {0xffcb, 16}, {0xffcc, 16}, {0xffcd, 16}, {0xffce, 16}, 
    {0xffcf, 16}, {0xffd0, 16}, {0xffd1, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x1f9, 9}, {0xffd2, 16}, {0xffd3, 16}, 
    {0xffd4, 16}, {0xffd5, 16}, {0xffd6, 16}, {0xffd7, 16}, 
    {0xffd8, 16}, {0xffd9, 16}, {0xffda, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x1fa, 9}, {0xffdb, 16}, {0xffdc, 16}, 
    {0xffdd, 16}, {0xffde, 16}, {0xffdf, 16}, {0xffe0, 16}, 
    {0xffe1, 16}, {0xffe2, 16}, {0xffe3, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x7f9, 11}, {0xffe4, 16}, {0xffe5, 16}, 
    {0xffe6, 16}, {0xffe7, 16}, {0xffe8, 16}, {0xffe9, 16}, 
    {0xffea, 16}, {0xffeb, 16}, {0xffec, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x00, 0}, {0x3fe0, 14}, {0xffed, 16}, {0xffee, 16}, 
    {0xffef, 16}, {0xfff0, 16}, {0xfff1, 16}, {0xfff2, 16}, 
    {0xfff3, 16}, {0xfff4, 16}, {0xfff5, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0}, 
    {0x3fa, 10}, {0x7fc3, 15}, {0xfff6, 16}, {0xfff7, 16}, 
    {0xfff8, 16}, {0xfff9, 16}, {0xfffa, 16}, {0xfffb, 16}, 
    {0xfffc, 16}, {0xfffd, 16}, {0xfffe, 16}, {0x00, 0}, 
    {0x00, 0}, {0x00, 0}, {0x00, 0}, {0x00, 0} };

static const char ss[] = {
    0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };

JpegEncoder::JpegEncoder() : TransmitterModule(FT_JPEG), 
    bb_(0), nbb_(0), bs_(0), bc_(0), offset_(0), quant_(0),
    lpred_(0), crpred_(0), cbpred_(0), nmcu_(0), w_(0), h_(0)
{
    setq(30);
}

void
JpegEncoder::setq(int q)
{
    int lqt[64], cqt[64];
    quant_ = q;

    int s = (q>50) ? 200-q*2 : 5000/q;
    for (int i=0; i<64; i++) {
	int v = (stdlqt[i] * s + 50)/ 100;
	if (v<1)
	    lqt[i] = 1;
	else if (v>255)
	    lqt[i] = 255;
	else
	    lqt[i] = v;
		    
	v = (stdcqt[i] * s + 50)/ 100;
	if (v<1)
	    cqt[i] = 1;
	else if (v>255)
	    cqt[i] = 255;
	else 
	    cqt[i] = v;
    }
    fdct_fold_q(lqt, lqt_);
    fdct_fold_q(cqt, cqt_);
}

void
JpegEncoder::size(int w, int h)
{
    Module::size(w, h);
    if (w&0xf || h&0xf) {
	fprintf(stderr, "JpegEncoder: bad geometry: %dx%d\n", w, h);
	exit(1);
    }
    w_ = w>>3;
    h_ = h>>3;
    nmcu_ = (w_>>1) * h_;
}

int
JpegEncoder::command(int argc, const char*const* argv)
{
    if (argc == 2 && strcmp(argv[1], "frame-format") == 0) {
	Tcl& tcl = Tcl::instance();
	tcl.result("422");
	return (TCL_OK);
    }
    if (argc == 3 && strcmp(argv[1], "q") == 0) {
	setq(atoi(argv[2]));
	return (TCL_OK);
    }
    return (TransmitterModule::command(argc, argv));
}

int
JpegEncoder::consume(const VideoFrame *vf)
{
    if (!samesize(vf))
	size(vf->width_, vf->height_);

    return(encode(vf));
}

int
JpegEncoder::flush(pktbuf* pb, int nbit, pktbuf* npb)
{
    u_char* obc = bc_;

    /* flush bit buffer */
    STORE_BITS(bb_, bc_);

    int cc = (nbit + 7) >> 3;

    /*XXX*/
    if (cc == 0 && npb != 0)
	abort();

    pb->len = cc + HDRSIZE;
    rtphdr* rh = (rtphdr*)pb->data;
    if (npb == 0)
	rh->rh_flags |= htons(RTP_M);

    if (npb != 0) {
	u_char* nbs = &npb->data[HDRSIZE];
	int extra = obc - (bs_ + (nbit >> 3));
	if (extra > 0)
	    memcpy(nbs, bs_ + (nbit >> 3), extra);
	bs_ = nbs;
	bc_ = nbs + extra;
    }
    tx_->send(pb);


    return (cc + HDRSIZE);
}

#define SSSS(n) (((n)&~0xff) ? (8+ss[(n)>>8]) : ss[n])

void
JpegEncoder::encode_blk(const short* blk, short* dcpred,
			struct huffentry* dcht, struct huffentry* acht)
{
    BB_INT bb = bb_;
    u_int nbb = nbb_;
    u_char* bc = bc_;

    /* code dc */
    short diff = blk[0] - *dcpred;
    *dcpred = blk[0];

    u_char len;
    
    if (diff < 0){
	len = SSSS(-diff);
	huffentry e = dcht[len];
	PUT_BITS(e.val, e.nb, nbb, bb, bc);
	PUT_BITS((diff-1)&(~(-1<<len)), len, nbb, bb, bc);
    } else {
	len = SSSS(diff);
	huffentry e = dcht[len];
	PUT_BITS(e.val, e.nb, nbb, bb, bc);
	PUT_BITS(diff, len, nbb, bb, bc);
    }

    /* code ac terms */
    int run = 0;
    const u_char* colzag = &COLZAG[0];
    for (int zag; (zag = *++colzag) != 0; ) {
	int level = blk[zag];
	if (level != 0) {
	    if (level < 0) {
		len = SSSS(-level);
		while (run & ~0xf) {
		    huffentry e = acht[0xf0];
		    PUT_BITS(e.val, e.nb, nbb, bb, bc);
		    run -= 16;
		}
		huffentry e = acht[(run<<4) | len];
		PUT_BITS(e.val, e.nb, nbb, bb, bc);
		PUT_BITS((level-1)&(~(-1<<len)), len, nbb, bb, bc);
	    } else {
		len = SSSS(level);
		while (run & ~0xf) {
		    huffentry e = acht[0xf0];
		    PUT_BITS(e.val, e.nb, nbb, bb, bc);
		    run -= 16;
		}
		huffentry e = acht[(run<<4) | len];
		PUT_BITS(e.val, e.nb, nbb, bb, bc);
		PUT_BITS(level, len, nbb, bb, bc);
	    }
	    run = 0;
	} else {
	    run++;
	}
    }

    if (run > 0) {
	/* EOB */
	huffentry e = acht[0];
	PUT_BITS(e.val, e.nb, nbb, bb, bc);
    }

    bb_ = bb;
    nbb_ = nbb;
    bc_ = bc;
}

void
JpegEncoder::encode_mcu(u_int mcu, const u_char* frm)
{
    short blk[64];
    register int stride = w_ << 3;

    u_char mx = mcu % (w_ >> 1);
    u_char my = mcu / (w_ >> 1);

    /* luminance */
    const u_char* p = &frm[8*stride*my + 16*mx];
    fdct(p, stride, blk, lqt_);
    encode_blk(blk, &lpred_, ldht, laht);

    p += 8;
    fdct(p, stride, blk, lqt_);
    encode_blk(blk, &lpred_, ldht, laht);

    /* chrominance */
    stride >>= 1;
    int fs = framesize_;
    p = &frm[fs + 8*stride*my + 8*mx];
    fdct(p, stride, blk, cqt_);
    encode_blk(blk, &crpred_, cdht, caht);

    p += (fs>>1);
    fdct(p, stride, blk, cqt_);
    encode_blk(blk, &cbpred_, cdht, caht);
}

int
JpegEncoder::encode(const VideoFrame* vf)
{
    tx_->flush();
	pktbuf* pb = pool_->alloc(vf->ts_, RTP_PT_JPEG);
    bs_ = &pb->data[HDRSIZE];
    bc_ = bs_;
    int fragsize = tx_->mtu() - HDRSIZE;
    u_int ec = fragsize << 3;

    bb_ = 0;
    nbb_ = 0;
    offset_ = 0;
    lpred_ = 0;
    crpred_ = 0;
    cbpred_ = 0;

    /* RTP/JPEG header */
    rtphdr* rh = (rtphdr*)pb->data;
    u_int* h = (u_int*)(rh+1);
    h[0] = htonl(offset_);
    h[1] = htonl(quant_ << 16 | w_ << 8 | h_);

    int cc = 0;

    u_int8_t* frm = vf->bp_;
    for (u_int mcu = 0; mcu < nmcu_; mcu++) {
	encode_mcu(mcu, frm);
	u_int cbits = (bc_ - bs_) << 3;
	if (cbits > ec) {
	    pktbuf* npb = pool_->alloc(vf->ts_, RTP_PT_JPEG);
	    cc += flush(pb, fragsize<<3, npb);
	    offset_ += fragsize;
	    pb = npb;

	    /* RTP/JPEG header */
	    rh = (rtphdr*)pb->data;
	    u_int* h = (u_int*)(rh+1);
	    h[0] = htonl(offset_);
	    h[1] = htonl(quant_ << 16 | w_ << 8 | h_);
	}
    }

    cc += flush(pb, ((bc_ - bs_)<<3) + nbb_, 0);
    return (cc);
}

/*
 * fdct() from dct.cc slightly modified to fold in the jpeg 128 bias.
 */

#define FA1 (0.707106781f)
#define FA2 (0.541196100f)
#define FA3 FA1
#define FA4 (1.306562965f)
#define FA5 (0.382683433f)
#define FWD_DandQ(v, iq) short((v) * qt[iq] + 0.5)

void
JpegEncoder::fdct(const u_char* in, int stride, short* out, const float* qt)
{
    float tmp[64];
    float* tp = tmp;

    int i;
    for (i = 8; --i >= 0; ) {
	float x0, x1, x2, x3, t0, t1, t2, t3, t4, t5, t6, t7;
	t0 = float(in[0] + in[7] - 256);
	t7 = float(in[0] - in[7]);
	t1 = float(in[1] + in[6] - 256);
	t6 = float(in[1] - in[6]);
	t2 = float(in[2] + in[5] - 256);
	t5 = float(in[2] - in[5]);
	t3 = float(in[3] + in[4] - 256);
	t4 = float(in[3] - in[4]);

	/* even part */
	x0 = t0 + t3;
	x2 = t1 + t2;
	tp[8*0] = x0 + x2;
	tp[8*4] = x0 - x2;
    
	x1 = t0 - t3;
	x3 = t1 - t2;
	t0 = (x1 + x3) * FA1;
	tp[8*2] = x1 + t0;
	tp[8*6] = x1 - t0;

	/* odd part */
	x0 = t4 + t5;
	x1 = t5 + t6;
	x2 = t6 + t7;

	t3 = x1 * FA1;
	t4 = t7 - t3;

	t0 = (x0 - x2) * FA5;
	t1 = x0 * FA2 + t0;
	tp[8*3] = t4 - t1;
	tp[8*5] = t4 + t1;

	t7 += t3;
	t2 = x2 * FA4 + t0;
	tp[8*1] = t7 + t2;
	tp[8*7] = t7 - t2;

	in += stride;
	tp += 1;
    }
    tp -= 8;

    for (i = 8; --i >= 0; ) {
	float x0, x1, x2, x3, t0, t1, t2, t3, t4, t5, t6, t7;
	t0 = tp[0] + tp[7];
	t7 = tp[0] - tp[7];
	t1 = tp[1] + tp[6];
	t6 = tp[1] - tp[6];
	t2 = tp[2] + tp[5];
	t5 = tp[2] - tp[5];
	t3 = tp[3] + tp[4];
	t4 = tp[3] - tp[4];

	/* even part */
	x0 = t0 + t3;
	x2 = t1 + t2;
	out[0] = FWD_DandQ(x0 + x2, 0);
	out[4] = FWD_DandQ(x0 - x2, 4);
    
	x1 = t0 - t3;
	x3 = t1 - t2;
	t0 = (x1 + x3) * FA1;
	out[2] = FWD_DandQ(x1 + t0, 2);
	out[6] = FWD_DandQ(x1 - t0, 6);

	/* odd part */
	x0 = t4 + t5;
	x1 = t5 + t6;
	x2 = t6 + t7;

	t3 = x1 * FA1;
	t4 = t7 - t3;

	t0 = (x0 - x2) * FA5;
	t1 = x0 * FA2 + t0;
	out[3] = FWD_DandQ(t4 - t1, 3);
	out[5] = FWD_DandQ(t4 + t1, 5);

	t7 += t3;
	t2 = x2 * FA4 + t0;
	out[1] = FWD_DandQ(t7 + t2, 1);
	out[7] = FWD_DandQ(t7 - t2, 7);

	out += 8;
	tp += 8;
	qt += 8;
    }
}
