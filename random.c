/*
 * Copyright (c) 1993 The Regents of the University of California.
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

/*
 * LBL random number generator.
 *
 * Written by Steve McCanne & Chris Torek (mccanne@ee.lbl.gov,
 * torek@ee.lbl.gov), November, 1992.
 *
 * This implementation is based on ``Two Fast Implementations of
 * the "Minimal Standard" Random Number Generator", David G. Carta,
 * Communications of the ACM, Jan 1990, Vol 33 No 1.
 */

static int randseed = 1;

void
srandom(int seed)
{
	randseed = seed;
}

#if sparc && !__SUNPRO_C
#ifdef __svr4__
#define RANDOM "random"
#define RANDSEED "randseed"
#else
#define RANDOM "_random"
#define RANDSEED "_randseed"
#endif
asm("\
	.global	" RANDOM "			;\
" RANDOM ":					;\
	sethi	%hi(16807), %o1			;\
	wr	%o1, %lo(16807), %y		;\
	 sethi	%hi(" RANDSEED "), %g1		;\
	 ld	[%g1 + %lo(" RANDSEED ")], %o0	;\
	 andcc	%g0, 0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %o0, %o2			;\
	mulscc  %o2, %g0, %o2			;\
	rd	%y, %o3				;\
	srl	%o2, 16, %o1			;\
	set	0xffff, %o4			;\
	and	%o4, %o2, %o0			;\
	sll	%o0, 15, %o0			;\
	srl	%o3, 17, %o3			;\
	or	%o3, %o0, %o0			;\
	addcc	%o0, %o1, %o0			;\
	bneg	1f				;\
	 sethi	%hi(0x7fffffff), %o1		;\
	retl					;\
	 st	%o0, [%g1 + %lo(" RANDSEED ")]	;\
1:						;\
	or	%o1, %lo(0x7fffffff), %o1	;\
	add	%o0, 1, %o0			;\
	and	%o1, %o0, %o0			;\
	retl					;\
	 st	%o0, [%g1 + %lo(" RANDSEED ")] ");
#else
int
random()
{
	register int x = randseed;
	register int hi, lo, t;

	hi = x / 127773;
	lo = x % 127773;
	t = 16807 * lo - 2836 * hi;
	if (t <= 0)
		t += 0x7fffffff;
	randseed = t;
	return (t);
}
#endif
