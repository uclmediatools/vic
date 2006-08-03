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
static const char rcsid[] = "@(#) $Header$ (LBL)";
#endif

#include <tk.h>
#include "vic_tcl.h"

class PPM : public TclObject {
public:
	PPM();
	~PPM();
	int command(int argc, const char*const* argv);
protected:
	int load(FILE* f);
	int dump(FILE* f);
	int width_;
	int height_;
	u_char* image_;
};

static class PPM_Matcher : public Matcher {
public:
	PPM_Matcher() : Matcher("ppm") {}
	TclObject* match(const char*) {
		return (new PPM);
	}
} ppm;

PPM::PPM() : width_(0), height_(0), image_(0)
{
}

PPM::~PPM()
{
	delete image_;
}

int PPM::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "width") == 0) {
			sprintf(tcl.buffer(), "%d", width_);
			tcl.result(tcl.buffer());
			return (TCL_OK);
		}
		if (strcmp(argv[1], "height") == 0) {
			sprintf(tcl.buffer(), "%d", height_);
			tcl.result(tcl.buffer());
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "load") == 0) {
			FILE* f;
			if (Tcl_GetOpenFile(tcl.interp(), (char*)argv[2],
					    0, 1, (void **)&f) != TCL_OK ||
			    load(f) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
		if (strcmp(argv[1], "dump-yuv") == 0) {
			FILE* f;
			if (Tcl_GetOpenFile(tcl.interp(), (char*)argv[2],
					    1, 1, (void **)&f) != TCL_OK ||
			    dump(f) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}
	
static int
readline(FILE *fp, char* line, int size)
{
	for (;;) {
		if (fgets(line, size, fp) == 0)
			return (-1);
		if (*line != '#')
			return (0);
	}
}

static int
readhdr(FILE *fp, int *width, int *height, int *maxval)
{
	char line[1024];
	if (readline(fp, line, sizeof(line)) < 0 ||
	    line[0] != 'P' || line[1] != '6')
		return (-1);

	if (readline(fp, line, sizeof(line)) < 0 ||
	    sscanf(line, "%d %d", width, height) != 2)
		return (-1);

	if (readline(fp, line, sizeof(line)) < 0 ||
	    sscanf(line, "%d", maxval) != 1)
		return (-1);

	return (0);
}

int PPM::load(FILE* f)
{
	int maxval;
	if (readhdr(f, &width_, &height_, &maxval) < 0)
		return (-1);
	int n = width_ * height_;
	if (n > 1024*1024)
		return (-1);
	delete image_;
	image_ = new u_char[2 * n];
	u_char* p = image_;
	for (int k = n >> 1; --k >= 0; ) {
		double r = getc(f);
		double g = getc(f);
		double b = getc(f);

		printf("%d:\t%d %d %d\n", k, int(r), int(g), int(b));
		
		/* can't have overflow in this direction */
		double y0 = 0.299 * r + 0.587 * g + 0.114 * b;
		double u = -0.1687 * r - 0.3313 * g + 0.5 * b;
		double v = 0.5 * r - 0.4187 * g - 0.0813 * b;
		
		r = getc(f);
		g = getc(f);
		b = getc(f);

		printf("%d:\t%d %d %d\n", k, int(r), int(g), int(b));

		double y1 = 0.299 * r + 0.587 * g + 0.114 * b;

	//	printf("%d:\t%d %d %d\n", k, int(r), int(g), int(b));

		p[0] = int(y0);
		p[1] = int(u + 128);
		p[2] = int(y1);
		p[3] = int(v + 128);
		p += 4;
	}
}

int PPM::dump(FILE* f)
{
	int n = width_ * height_;
	if (n <= 0)
		return (-1);

	if (fwrite(image_, 2 * n, 1, f) == 0)
		return (-1);

	return (0);
}
