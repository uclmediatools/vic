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

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <dmedia/cl.h>
#include <dmedia/cl_cosmo.h>

#include "vic_tcl.h"
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "rcvr.h"
#include "vw.h"
#include "iohandler.h"

class CosmoDecoder : public Decoder {
protected:
	CosmoDecoder(Receiver&);
public:
	~CosmoDecoder();
	void sync() {}
	int colorhist(u_int* histogram) const { return (0); }
	void release();
	void reset(const u_char* vh);
	void resize(int w, int h) { reset(inq_, w, h); }
protected:
	int command(int ac, const char*const* av) { return (-1); }
	virtual void reset(int inq, int inw, int inh) = 0;
	void shutdown();
	void startup();

	int rw_;		/* width of rendered image */
	int rh_;		/* height of rendered image */
	int inq_;		/* input quantization */
	int vs_;		/* vl server */
	static CL_Handle ch_;	/* cosmo handle */
};

#ifdef notdef
class CosmoOutboardDecoder : public CosmoDecoder {
protected:
	CosmoOutboardDecoder(Receiver& r) : CosmoDecoder(r) { }
	virtual void dispatch(int);
	void start();
	void reset(int inq, int inw, int inh);
};
#endif

class CosmoWindowDecoder : public CosmoDecoder {
public:
	CosmoWindowDecoder(Receiver&, VideoWindow*);
	~CosmoWindowDecoder();
protected:
	void reset(int inq, int inw, int inh);
	void decode(const u_char* vh, const u_char* data, int len);
	VideoWindow* window_;
	void allocimage();
	int imagesize_;
	SharedVideoImage* vbx_;

};

static class JpegHardwareDecoderMatcher : public HardwareDecoderMatcher {
 protected:
	Decoder* match(int fmt, Receiver& r, VideoWindow* vw) {
		if (fmt == RTP_PT_JPEG && r.decimation() == 422)
			return (new CosmoWindowDecoder(r, vw));
		return (0);
	}
} hwd_jpeg_;
/*XXX*/
CL_Handle CosmoDecoder::ch_;   /* cosmo handle */

CosmoDecoder::CosmoDecoder(Receiver& r) :
	Decoder(r), inq_(0)
{
	rw_ = -1;
}

void CosmoDecoder::shutdown()
{
	if (ch_ != 0) {
		clCloseDecompressor(ch_);
		ch_ = 0;
	}
}

void CosmoDecoder::startup()
{
	int s = clOpenDecompressor(CL_JPEG_COSMO, &ch_);
	if (s != 0) {
		printf("clOpenDecompressor failed\n");
		exit(1);
	}
}

CosmoDecoder::~CosmoDecoder()
{
#ifdef notdef
	if (server_ >= 0) {
		if (decompressing_)
			(void)waitd();
		freedma(pbase_[0].db);
		freedma(pbase_[1].db);
		freedma(pbase_[2].db);
		/* ~IOHandler() has already unlinked us */
		(void)close(server_);
	}
#endif
}

void CosmoDecoder::release()
{
	/*XXX*/
	delete this;
}

#ifdef notdef
void CosmoOutboardDecoder::reset(int inq, int inw, int inh)
{
	if (decompressing_)
		waitd();

	inq_ = inq;

	if (CosmosSetDecomp(server_, inq_, rcvr_.width(), rcvr_.height(),
			 &inw, &inh, 8, 0, 0, &pad_) < 0)
		fprintf(stderr, "vic: CosmosSetDecomp failed\n");

	if (qh_ != 0)
		decompress();
}
#endif

#define JFIF_OFF_LUMA_QUANT 25
#define JFIF_OFF_CHROMA_QUANT (25+67)
#define M_SOI 0xd8
#define M_APP0 0xe0
#define M_DQT 0xdb
#define M_SOF0 0xc0
#define M_DHT 0xc4
#define M_SOS 0xda

u_char jfifhdr[] = {
	0xff, M_SOI,
	/* APP0 */
	0xff, M_APP0, /* len */ 0, 16,
	'J', 'F', 'I', 'F',
	0, /* v1.01 */ 1, 1,
	0, 0, 1, 0, 1, 0, 0,
	/* luma quantizer */
#define JFIF_OFF_LUMA_QUANT 25
#define JFIF_OFF_CHROMA_QUANT (25+67)
	0xff, M_DQT, 0, 67, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	/* chroma quantizer */
	0xff, M_DQT, 0, 67, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 

	0xff, M_SOF0,

	/* DC luma huffman code */
	0xff, M_DHT,
	0, 19 + 12,
	0, /* DC/luma */
	/* code lengths */
	0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	/* values */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,

	0xff, M_DHT,
	0, 19 + 162,
	0x10, /* AC/luma */
	/* code lengths */
	0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d,
	/* values */
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa,


	0xff, M_DHT,
	0, 19 + 12,
	0x01, /* DC/chroma */
	/* code lengths */
	0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
	/* values */
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	

	0xff, M_DHT,
	0, 19 + 162,
	0x11, /* AC/chroma */
	/* code lengths */
	0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77,
	/* values */
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa,

	/* start-of-scan */
	0xff, M_SOS,
	0, 14,
	3,
	1, 0,
	2, 0x11,
	3, 0x11,
	0, 63, 0,
};

/*XXX*/
extern void jpeg_luma_qt(int q, int* qt);
extern void jpeg_chroma_qt(int q, int* qt);

void
setq(int q)
{
	int qt[64];
	jpeg_luma_qt(q, qt);
	for (int i = 0; i < 64; ++i)
		jfifhdr[i + JFIF_OFF_LUMA_QUANT] = qt[i];
	jpeg_chroma_qt(q, qt);
	for (i = 0; i < 64; ++i)
		jfifhdr[i + JFIF_OFF_CHROMA_QUANT] = qt[i];
}


void CosmoWindowDecoder::decode(const u_char* vh, const u_char* frame, int cc)
{
/*XXX*/
if (window_->width() < 300)
	return;

#ifdef notdef
static int n = 0;
char wrk[8];
sprintf(wrk, "f%d", n++);
int fd = open(wrk, O_WRONLY|O_CREAT|O_TRUNC, 0644);
if (fd < 0) {
	perror(wrk);
	exit(1);
}
write(fd, frame, cc);
close(fd);
#endif

	const jpeghdr* p = (const jpeghdr*)vh;
	/*XXX need to handle p->type changes too!*/
	if (p->q != inq_ || ch_ == 0)
		reset(p->q, rcvr_.width(), rcvr_.height());

static char wrk[64*1024];
memcpy(wrk, jfifhdr, sizeof(jfifhdr));
memcpy(wrk + sizeof(jfifhdr), frame, cc);
cc += sizeof(jfifhdr);
frame = wrk;

if (vbx_ == 0)
	abort();

	/*Single frame decompression*/
	int s = clDecompress(ch_, 1, cc, frame, (void*)vbx_->buffer());
	if (s != 1) {
		/*XXX*/
		printf("clDecompress failed (%d)\n", s);
		exit(1);
	}
#ifdef notdef
	window_->render(vbx_);
	window_->complete();
#endif
	window_->render(vbx_);
#ifdef notdef
	lrectwrite(0, 0, window_->width() - 1, window_->height() - 1,
		   (const u_long*)fb_);
#endif
}

CosmoWindowDecoder::CosmoWindowDecoder(Receiver& r, VideoWindow* vw)
	: CosmoDecoder(r), window_(vw), imagesize_(0), vbx_(0)
{
}

CosmoWindowDecoder::~CosmoWindowDecoder()
{
	/*
	 * Clear out the image so that the Video class does
	 * not try to use it before another window gets attached
	 * and replaces the image.
	 */
	window_->setimage(0);

/*XXX vbx_ etc */
#ifdef notdef
	if (vbx_ != 0) {
		if (decompressing_)
			(void)waitd();
		/*XXX*/
		int s0 = vb0_.shmid;
		int s1 = vb1_.shmid;
		delete vbx_;
		delete vbd_;
		(void)CosmosDeallocBuf(server_, s0);
		(void)CosmosDeallocBuf(server_, s1);
	}
#endif
}

void CosmoWindowDecoder::reset(int inq, int inw, int inh)
{
	/*
	 * Make sure all outstanding putimage's finish.
	 * Syncing with the first window, syncs the server,
	 * and hence all of them.
	 */
	window_->sync();
	
/*XXX*/
if (inq == 0)
	return;

	inq_ = inq;
setq(inq);

	int w = window_->width();
	int h = window_->height();

/*XXX*/
if (w < 300)
	return;

	shutdown();
	startup();

	int options[10];
	int* p = options;
	*p++ = CL_IMAGE_WIDTH;
	*p++ = w;
	*p++ = CL_INTERNAL_IMAGE_WIDTH;
	*p++ = inw;
	*p++ = CL_IMAGE_HEIGHT;
	*p++ = h;
	*p++ = CL_INTERNAL_IMAGE_HEIGHT;
	*p++ = inh;
	/*XXX*/
	*p++ = CL_STREAM_HEADERS;
	*p++ = 0;
#ifdef notdef
	*p++ = CL_ORIGINAL_FORMAT;
	*p++ = ?;
#endif
	int s = clSetParams(ch_, options, 10);
	if (s != 0) {
		printf("clSetParams failed\n");
		exit(1);
	}
	if (vbx_ == 0 || vbx_->width() != w || vbx_->height() != h)
		vbx_ = new SharedVideoImage(window_->tkwin(), w, h);
}
