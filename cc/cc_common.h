/*
 * Copyright (c) 2010 University College London
 * All rights reserved.
 * 
 * AUTHOR: Soo-Hyun Choi <s.choi@cs.ucl.ac.uk>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor of the Laboratory may be used
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
 *
 * $Id$
 */

#ifndef vic_cc_common_h
#define vic_cc_common_h

#define DUPACKS 3   // simulating TCP's 3 dupacks
#define TSZ 1000    // tsvec_ size
#define SSZ 1000    // seqvec_ size
#define RSZ 1000    // refvec_ size
#define PSR 10000   // packet size record

#define SHORT_HISTORY       // history size = 8
#ifdef  SHORT_HISTORY
#define HSZ 8   // history size for avg loss history
#else
#define HSZ 16  // history size for avg loss history
#endif

#define T_RTTVAR_BITS   2
#define T_SRTT_BITS     3

#define BITLEN  16

// timestamp skew from Vic to Network Device 
// (approximately 10 usec)
#define SKEW 0.000010

#endif /* vic_cc_common_h */
