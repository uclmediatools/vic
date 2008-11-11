/*
 * Copyright (c) 1996 Isidor Kouvelas (University College London)
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
 *
 * @(#) $Header$ (LBL)
 */

#ifndef rgb_converter_h
#define rgb_converter_h

#include "config.h"

class Converter {
public:
	virtual void convert(u_int8_t* in, int inw, int inh, u_int8_t* frm, int outw, int outh, int invert)= 0;
	virtual ~Converter(){}; //SV-XXX: This solves the "missing" virtual destructor warning from gcc4
};

class RGB_Converter;

typedef void (RGB_Converter::*ConvertMethod)(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);

class RGB_Converter : public Converter {
public:
	RGB_Converter(int bpp, u_int8_t* map, int ncol);
	~RGB_Converter();
	virtual void convert(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw = 0, int outh = 0, int invert = 0);
protected:
	virtual void convert32(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert) = 0;
	virtual void convert24(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert) = 0;
	virtual void convert16(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert) = 0;
	virtual void convert8(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert) = 0;
	virtual void convert4(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert) = 0;
	virtual void convert1(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert) = 0;

	static u_int32_t r2yuv_[256];
	static u_int32_t g2yuv_[256];
	static u_int32_t b2yuv_[256];

	int bpp_;
	int ncol_;
	u_int8_t *ymap_;
	u_int8_t *umap_;
	u_int8_t *vmap_;

	ConvertMethod method_;
};

class RGB_Converter_422 : public RGB_Converter {
public:
	RGB_Converter_422(int bpp = 32, u_int8_t* map = NULL, int ncol = 0) :
		RGB_Converter(bpp, map, ncol) {}
	static RGB_Converter* instance() { return (&instance_); }
protected:
	virtual void convert32(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);
	virtual void convert24(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);
	virtual void convert16(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);
	virtual void convert8(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);
	virtual void convert4(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);
	virtual void convert1(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);

	/* Backwards compatibility :-) */
	static RGB_Converter_422 instance_;
};

class RGB_Converter_420 : public RGB_Converter {
public:
	RGB_Converter_420(int bpp = 32, u_int8_t* map = NULL, int ncol = 0) :
		RGB_Converter(bpp, map, ncol) {}
	static RGB_Converter* instance() { return (&instance_); }
protected:
	virtual void convert32(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);
	virtual void convert24(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);
	virtual void convert16(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);
	virtual void convert8(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);
	virtual void convert4(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);
	virtual void convert1(u_int8_t* rgb, int w, int h, u_int8_t* frm, int outw, int outh, int invert);

	/* Backwards compatibility :-) */
	static RGB_Converter_420 instance_;
};

#endif
