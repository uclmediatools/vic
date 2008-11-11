/*
 * Copyright (c) 1995 The Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

//
// transcoder-jpeg:
//	idea:
//
//	produce JPEG coefs using DCT JPEG decoder
//	perform CR, and hand to DCT version of h261 encoder
//

#include <stdlib.h>
#include "jpeg/jpeg.h"
#include "module.h"
#include "crdef.h"

#define CIF_WIDTH	352
#define CIF_HEIGHT	288
#define QCIF_WIDTH      176
#define QCIF_HEIGHT     144

class JpegTranscoder : public Module {
private:
	u_int8_t q_;			// input q factor
	int decimation_;		// input decimation

	u_int8_t *crvec_;		// CRvec (refers to 16x16 blks)
	int crvlen_;			// length of decode CR

	int norow_;			// output rows
	int nocol_;			// output cols
	int owidth_;			// output width
	int oheight_;			// output height
	int oblocks_;			// # of output blocks produced

	JpegDCTDecoder *decoder_;	// jpeg DCT decoder
	JpegDecoder::config config_;	// jpeg parameters
	int changed(JpegFrame *);	// different kind/size of frame?
	void configure(JpegFrame *);	// reconfigure decoder
public:
	JpegTranscoder();
	~JpegTranscoder();

	int consume(const VideoFrame*);			// main data path
	int command(int argc, const char*const* argv);	// for TCL
};

static class JpegTranscoderMatcher : public Matcher {
public:
	JpegTranscoderMatcher() : Matcher("transcoder") {}
	TclObject* match(const char* fmt) {
		if (strcasecmp(fmt, "jpeg/dct") == 0)
			return (new JpegTranscoder);
		return (0);
	}
} transcoder_matcher_jpeg;

JpegTranscoder::JpegTranscoder() : Module(FT_JPEG)
{
	decoder_ = NULL;
	crvec_ = NULL;
	owidth_ = oheight_ = oblocks_ = width_ = height_ = framesize_ = -1;
}

JpegTranscoder::~JpegTranscoder()
{
}

int
JpegTranscoder::command(int argc, const char*const* argv)
{
	return (Module::command(argc, argv));
}

inline int
JpegTranscoder::changed(JpegFrame *jf)
{
	int ss = samesize(jf);	// checks sizes [module.h]

	if (!ss || (jf->q_ != q_))
		return (1);	// new q or sizes

	if ((jf->type_ == 1) && (decimation_ != 420))
		return (1);	// new decimation/scaling factor

	return (0);		// apparently no change
}

int
JpegTranscoder::consume(const VideoFrame *vf)
{
	JpegFrame *jf = (JpegFrame *) vf;
	int rval = 0;

	if (changed(jf))
		configure(jf);

	decoder_->decode(jf->bp_, jf->len_, crvec_, CR_MOTION|CR_SEND);

	DCTFrame *df = new DCTFrame(jf->ts_, decoder_->frame(), crvec_,
		owidth_, oheight_, jf->q_);

	rval = target_->consume(df);

	// update CR vec
	register int blkno;
	int s;
	for (blkno = 0; blkno < crvlen_; blkno++) {
		s = CR_STATE(crvec_[blkno]);
		if (s == CR_AGETHRESH || s == CR_BG) {
			s = CR_IDLE;
		} else if (s < CR_AGETHRESH) {
			if (++s == CR_AGETHRESH)
				s |= CR_SEND;
		}
		crvec_[blkno] = s;
	}

	return (rval);
}

void
JpegTranscoder::configure(JpegFrame *jf)
{

	decimation_ = (jf->type_ == 1) ? 420 : 422;

	/*
	 * set up JPEG configuration block
	 */

	JpegDecoder::defaults(config_);

	config_.comp[0].hsf = 2;
	config_.comp[0].vsf = (decimation_ == 420) ? 2 : 1;
	config_.comp[1].hsf = 1;
	config_.comp[1].vsf = 1;
	config_.comp[2].hsf = 1;
	config_.comp[2].vsf = 1;
	config_.width = jf->width_;	// [in VideoFrame class]
	config_.height = jf->height_;	// [in VideoFrame class]
	size(jf->width_, jf->height_);

	if ((jf->width_ <= QCIF_WIDTH) && (jf->height_ <= QCIF_HEIGHT)) {
		owidth_ = QCIF_WIDTH;
		oheight_ = QCIF_HEIGHT;
	} else {
		owidth_ = CIF_WIDTH;
		oheight_ = CIF_HEIGHT;
	}

	JpegDecoder::quantizer(config_, jf->q_);
	q_ = jf->q_;

	delete decoder_;
	decoder_ = JpegDCTDecoder::create(config_, owidth_, oheight_);

	if (decoder_ == NULL) {
		fprintf(stderr, "unable to create %dx%d JPegDecoder\n",
			owidth_, oheight_);
		exit(1);
	}

	Tcl& tcl = Tcl::instance();
	const char *p = tcl.attr("softJPEGThresh");

	int q;
	if (p == NULL || ((q = atoi(p)) < 0))
		q = 50; 

	decoder_->thresh(q);	// for CRVEC

	nocol_ = owidth_ / 8;
	norow_ = oheight_ / 8;
	oblocks_ = nocol_ * norow_;

	delete[] crvec_; //SV-XXX: Debian
	crvlen_  = oblocks_ / 4;
	crvec_ = new u_char[crvlen_];

	memset(crvec_, CR_SEND|CR_MOTION, crvlen_);
}
