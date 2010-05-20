/*
 * Copyright (c) 2010 University College London
 * All rights reserved.
 *
 * AUTHOR: Soo-Hyun Choi <s.choi@.cs.ucl.ac.uk>
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

#ifndef vic_cc_formula_h
#define vic_cc_formula_h

#define MAXRATE 25000000.0 
#define SAMLLFLOAT 0.0000001

/*
 * This takes as input the packet drop rate, and outputs the sending 
 *   rate in bytes per second.
 */
static double p_to_b(double p, double rtt, double tzero, int psize, int bval)
{
	double tmp1, tmp2, res;

	if (p < 0 || rtt < 0) {
		return MAXRATE ;
	}
	res = rtt * sqrt(2.0*bval*p/3.0);
	tmp1 = 3.0 * sqrt(3.0*bval*p/8.0);

	if (tmp1>1.0) tmp1=1.0;

	tmp2 = tzero * p * (1.0+32.0 * p * p);
	res += tmp1 * tmp2;

	// At this point, 1/res gives the sending rate in pps:
	// 1/(rtt*sqrt(2*bval*p/3) + 3*sqrt(3*bval*p/8)*tzero*p*(1+32*p*p))
	if (res < SAMLLFLOAT) {
		res=MAXRATE;
	} else {
		// change from 1/pps to Bps.
		res=psize/res;
	}
	if (res > MAXRATE) {
		res = MAXRATE ;
	}

	// the unit of rate is (bits/sec)
	//double brate = 8.0 * res;

	return res;
}

#endif /* vic_cc_formula_h */
