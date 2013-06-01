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

#ifndef lib_jpeg_h
#define lib_jpeg_h

#include "../config.h"

#ifdef WIN32
//#include <winsock.h>
#else
#include <sys/param.h>
#endif
#include <sys/types.h>

/*XXX*/
#define NUM_HUFF_TBLS       4	/* Huffman tables are numbered 0..3 */

#define	JO_PIXEL		1	/* JPEG output -> pixels */
#define	JO_DCT			2	/* JPEG output -> DCT coefs */

class JpegDecoder {
public:
	/*
	 * Baseline JPEG decompression parameters
	 * (which is the only mode we can handle).
	 * (describes the JPEG input stream)
	 */
	struct config {
		int width;
		int height;
		int precision;
		int ncomp;
		struct {
			int id;
			int hsf; /* horiz sample factor */
			int vsf; /* vertical sample factor */
			int qno;
			/*
			 * XXX Assume all scans the same.
			 */
			int ac_tbl_no;
			int dc_tbl_no;
		} comp[4];
		int qtab[4][64];
		const u_char *ac_huffbits[4];
		const u_char *ac_huffval[4];
		const u_char *dc_huffbits[4];
		const u_char *dc_huffval[4];
	};
	struct margin {
		int top;
		int right;
		int left;
		int ytopskip;
		int uvtopskip;
		int marktopskip;
		int ylskip;
		int uvlskip;
		int marklskip;
		int yrskip;
		int uvrskip;
		int markrskip;

		int ftopskip;
		int flskip;
		int frskip;

	} margin_;
	static void defaults(config&);
	static void quantizer(config&, int q);
	static int q_to_thresh(int q);
 protected:
	JpegDecoder(const config&, int decimation, int ow, int oh);
 public:
	virtual ~JpegDecoder();
	virtual int decode(const u_char* in, int len, u_char* marks, 
			   int mark) = 0;
	inline int width() const { return (width_); }
	inline int height() const { return (height_); }
	inline void color(int c) { color_ = c; }
	inline int decimation() const { return (decimation_); }
	inline void thresh(int v) { thresh_ = v; }
	inline void cthresh(int v) { cthresh_ = v; }
	inline void resetndblk() { ndblk_ = 0; }
	inline int ndblk() const { return (ndblk_); }
protected:
	virtual void setlrskips(void) = 0;
	struct component {
		int hsf;	/* horizontal sampling factor (1..4) */
		int vsf;	/* vertical sampling factor (1..4) */
		int qno;	/* quantization table selector (0..3) */
		/* for decompression, they are read from the SOS marker. */
		int dc_tbl_no;	/* DC entropy table selector (0..3) */
		int ac_tbl_no;	/* AC entropy table selector (0..3) */
		int dc;		/* dc predictor */
	};
	void init(const config&);
#ifdef INT_64
	void rdct(int nc, register short *bp, INT_64 mask,
		  u_char* pixels, int stride, int qno);
#else
	void rdct(int nc, register short *bp, u_int mask0, u_int mask1,
		  u_char* pixels, int stride, int qno);
#endif
	int compute_margins(int, int);
	void idlefill(void);
	const u_char *inb_;		/* input buffer XXX make u_long */
	const u_char *end_;
	u_int bb_;		/* bit buffer */
	int nbb_;		/* # bits in bit buffer */
	int ndblk_;	/* # blks decoded for this frame */
	const u_char* parseJFIF(const u_char*);

	int dct_unbias_;	/* true to unbias DCT DC coefs */
	int decimation_;
	int width_;
	int height_;
	int size_;
	int ncomp_;
	int owidth_;
	int oheight_;
	int osize_;
	component comp_[3];

	int rlen_;		/* restart interval length (0 if not using) */
	int rcnt_;		/* restart interval counter */

	/*
	 * Quantization tables.  qt_ is always stored in 
	 * natural order (i.e., not zigzagged).  fqt_ is
	 * transformed by the DCT module to be in whatever
	 * form it finds most convenient.
	 */
	int qt_[4][64];
	int fqt_[4][64];
	int rdqt(const u_char* p);

	/*
	 * Huffman tables.
	 */
	u_short* dcht_[NUM_HUFF_TBLS];
	u_short* acht_[NUM_HUFF_TBLS];
	void freehufftab();

	int ncol_;		/* no. of mcu's across scan */
	int nrow_;		/* no. of mcu's down scan */

	int huffdc(component&);
	int huffskip(component&);
	void huffreset();
	u_short* huffbuild(const u_char* bits, const u_char* vals) const;
	int huffblock(int ci, int n, u_int *code, short* dctcoef) const;
	void mix(u_int dc, const u_char* bp, u_char* out, int stride) const;

	void restart();

	int color_;

	/* state for suppressing work when blocks don't change much */
	int thresh_;
	int cthresh_;

	int rcrop_;
	int lcrop_;
	int topcrop_;
	int botcrop_;
};

class JpegPixelDecoder : public JpegDecoder {
public:
	JpegPixelDecoder(const config&, int, int, int);
	~JpegPixelDecoder();
	static JpegPixelDecoder* create(const config&, int, int);
	inline u_char* frame(void) const { return (frm_); }
protected:
#ifdef INT_64
	int huffparse(component&, short* out, short* ref, INT_64* mask,
		      int dontskip = 0);
#else
	int huffparse(component&, short* out, short* ref, u_int* mask,
		      int dontskip = 0);
#endif
	void setlrskips(void);
	u_char* frm_;
	short* cache_;
};

class JpegDCTDecoder : public JpegDecoder {
public:
	JpegDCTDecoder(const config&, int, int, int);
	~JpegDCTDecoder();
	static JpegDCTDecoder* create(const config&, int, int);
	inline short* frame(void) const { return (frm_); }
protected:
#ifdef INT_64
	int huffparse(component&, short* out, INT_64* mask,
		     const int *,  int dontskip = 0);
#else
	int huffparse(component&, short* out, u_int* mask,
		      const int *, int dontskip = 0);
#endif
	void setlrskips(void);
	short* frm_;
};

#endif
