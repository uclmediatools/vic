/*
 * Copyright (c) 1998-1999 Marcus Meissner and
 *     The Regents of the University of Erlangen.
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

class H263Encoder : public TransmitterModule {
 public:
	H263Encoder();
	~H263Encoder();
	virtual int consume(const VideoFrame*);
	virtual int command(int argc,const char *const *argv);
 protected:
	Picture	oldpic_;	/* previous coded frame */
	Picture	oldorigpic_;	/* previous original frame */
	Picture	decpict_;	/* current decoded frame */
	Bitstr	bitstr_;	/* bitstream */
	MVField	mvfield_;	/* MotionVectorfield */
	int	*mbind_;	/* MacroBlock startindizes */
	int	*mbquant_;	/* MacroBlock Quantiser */
	u_int	maxrate_;	/* maximum bitrate per frame */
	u_int	lastw_,lasth_;	/* last imagesize of frame */
	u_int	nexttype_;	/* next frametype */
	int	maycheck_;	/* nr of frames since last timecheck */
	int	codingtime_;	/* current codingtime */
	int	q_;		/* current quantiser */
};

/*  H263EncoderMatcher, creates H263Encoder on request by Tcl */
static class H263EncoderMatcher : public Matcher {
public:
	H263EncoderMatcher() : Matcher("module") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "h263") == 0)
			return (new H263Encoder);
		return (0);
	}
} encoder_matcher_h263;
