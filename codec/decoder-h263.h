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

class H263Decoder : public Decoder {
public:
	H263Decoder();
	~H263Decoder();
	int colorhist(u_int* hist) const;
	virtual void recv(pktbuf*);
	virtual void info(char* wrk) const;

protected:
	int reassemble_gobs(u_char **newbp,int *newcc);
	virtual void redraw();

	/*
	 * Reassembly buffer 'slots' for resequencing & reassembling
	 * gobs split across packet boundaries.
	 */
#define H263_SLOTS	64
#define H263_SLOTMASK	(H263_SLOTS - 1)
	int	h263streamsize_;	/* max streamsize */
	u_char	*h263stream_;		/* h263stream */

	struct slot {
		u_short 	seqno;
		u_int		rtflags;
		h263rtpheader_B	h263rh;
		u_int		cc;
		u_char* 	bp;
	} slot_[H263_SLOTS];

	u_char	*frame_,*backframe_;
	u_int	srcformat_;
	u_int	inw_,inh_;
	u_int	lastknowngood_;

	H263Global *h263decoder;
};

static class H263DecoderMatcher : public Matcher {
public:
	H263DecoderMatcher() : Matcher("decoder") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "h263") == 0)
			return (new H263Decoder());
		else
			return (0);
	}
} dm_h263;
