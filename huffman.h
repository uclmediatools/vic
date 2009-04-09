/*
 * Copyright (c) 1993-1994 The Regents of the University of California.
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

#ifndef lib_huffman_h
#define lib_huffman_h

struct huffent {
	int val;
	int nb;
};

#define HUFF_SYM_ILLEGAL (0x8000 >> 5)
#define HUFF_MASK(s) ((1 << (s)) - 1)

/*
 * Read the next 16 bits off the bit string into the bit buffer.
 * Skip over zero-stuffed ff's but make no attempt to verify
 * that they aren't some other marker (which should not be in the
 * middle of a block anyway).
 */
#define HUFF_READ(bb, bs) \
{ \
	register int v; \
	register const u_char *cp = (bs); \
 \
	bb <<= 16; \
	v = *cp++; \
	bb |= v << 8; \
	v = *cp++; \
	bb |= v; \
	(bs) = cp; \
 \
}

#define GET_BITS(n, nbb, bb, bs, result) \
{ \
	nbb -= n; \
	if (nbb < 0) { \
		HUFF_READ(bb, bs); \
		nbb += 16; \
	} \
	(result) = (((bb) >> (nbb)) & HUFF_MASK(n)); \
/*printf("gb %d(%x)\n", n, (result));*/\
}

#define SKIP_BITS(n, nbb, bb) \
{ \
	nbb -= n; \
	if (nbb < 0)  { \
		HUFFRQ(bb); \
		nbb += 16; \
	} \
}

#define HUFF_DECODE(ht, maxlen, nbb, bb, bs, result) { \
	register int s__, v__; \
 \
	 /*XXX*/ \
	while (nbb < maxlen) { \
		bb <<= 8; \
		bb |= *(bs)++; \
		nbb += 8; \
	} \
	s__ = maxlen; \
	v__ = (bb >> (nbb - s__)) & HUFF_MASK(s__); \
	s__ = (ht)[v__]; \
	nbb -= (s__ & 0x1f); \
	result = s__ >> 5; \
}

/*XXX use 64-bits if available */
#define HUFF_NBIT 32

#if BYTE_ORDER == LITTLE_ENDIAN
#define HUFF_STORE_BITS(bs, bb) \
	((u_char*)bs)[0] = bb >> 24; \
	((u_char*)bs)[1] = bb >> 16; \
	((u_char*)bs)[2] = bb >> 8; \
	((u_char*)bs)[3] = bb;
#else
#define HUFF_STORE_BITS(bs, bb) *(u_int*)(bs) = (bb);
#endif

#define PUT_BITS(bits, n, nbb, bb, bs) \
{ \
if (0) printf("pb %d(%x %d%d%d)\n", n, bits, (bits >> 2) & 1, (bits >> 1) & 1, bits & 1); \
	nbb += (n); \
	if (nbb > HUFF_NBIT)  { \
		int extra = (nbb) - HUFF_NBIT; \
		bb |= (bits) >> extra; \
		HUFF_STORE_BITS(bs, bb) \
		bs += (HUFF_NBIT / 8); \
		bb = (bits) << (HUFF_NBIT - extra); \
		nbb = extra; \
	} else \
		bb |= (bits) << (HUFF_NBIT - (nbb)); \
}

#endif
