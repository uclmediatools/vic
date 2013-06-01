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
static const char rcsid[] = "@(#) $Header$ (LBL)";
#endif

#include "vic_tcl.h"
#include <tk.h>


class PPM : public TclObject {
public:
	PPM();
	~PPM();
	int command(int argc, const char*const* argv);
protected:
	int load(Tcl_Channel tclChannel);
	int dump(Tcl_Channel tclChannel);
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
	char* TCLresult = tcl.buffer();

	if (argc == 2) {
		if (strcmp(argv[1], "width") == 0) {
			sprintf(TCLresult, "%d", width_);
			tcl.result(TCLresult);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "height") == 0) {
			sprintf(TCLresult, "%d", height_);
			tcl.result(TCLresult);
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "load") == 0) {
			Tcl_Channel Ch;
			if ( (Ch = Tcl_OpenFileChannel(tcl.interp(), (char*)argv[2], "r", 0777)) == NULL)
			{
				sprintf(TCLresult, "ppm.cpp: Can't obtain channel to open file %s. ErrMsg = %s\n", (char*)argv[2], Tcl_ErrnoMsg(Tcl_GetErrno()));
				tcl.result(TCLresult);
				return (TCL_ERROR);
			}
			else {
				sprintf(TCLresult, "ppm.cpp: Opened file %s\n", (char*)argv[2]);
				tcl.result(TCLresult);
			}
			if (load(Ch) < 0) {
				sprintf(TCLresult, "ppm.cpp: Can't load overlay image file %s from channel. Is the .ppm the right format?\n", (char*)argv[2]);
				tcl.result(TCLresult);
				return (TCL_ERROR);
			} else  {
				Tcl_Close(tcl.interp(), Ch);
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "dump-yuv") == 0) {
			Tcl_Channel Ch;
			if ((Ch = Tcl_OpenFileChannel(tcl.interp(), (char*)argv[2], "w+", 0777)) == NULL)
			{
				sprintf(TCLresult, "ppm.cpp: Can't obtain channel to dump overlay image file %s.\nErrMsg = \"%s\"\n", (char*)argv[2], Tcl_ErrnoMsg(Tcl_GetErrno()));
				tcl.result(TCLresult);
				return (TCL_ERROR);
			}
			else {
				tcl.result("1");
			}
			if (dump(Ch) < 0) {
				sprintf(TCLresult, "ppm.cpp: Can't dump overlay image file %s into channel %ld\n", (char*)argv[2], (long)Ch);
				tcl.result(TCLresult);
				return (TCL_ERROR);
			} else {
				Tcl_Close(tcl.interp(), Ch);
			}
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}
	
static int
readline(Tcl_Channel tclChannel, char *cline)
{
	Tcl_DString line;

	for (;;) {
		Tcl_DStringInit(&line);

		if (Tcl_Gets(tclChannel, &line) == 0)
			return (-1);

		sprintf(cline, "%s", Tcl_DStringValue(&line));

		Tcl_DStringFree(&line);

		if (cline[0] != '#')
			return (0);
	}
}

static int
readhdr(Tcl_Channel tclChannel, int *width, int *height, int *maxval)
{
	int gotLine;
	char cline[1000];

	gotLine = readline(tclChannel, cline);
	if ( gotLine < 0 || cline[0] != 'P' || cline[1] != '6')
		return (-1);

	gotLine = readline(tclChannel, cline);
	if (gotLine < 0 || sscanf(cline, "%d %d", width, height) != 2)
		return (-1);

	gotLine = readline(tclChannel, cline);
	if (gotLine < 0 || sscanf(cline, "%d", maxval) != 1)
		return (-1);

	return (0);
}

int PPM::load(Tcl_Channel tclChannel)
{
	//Tcl& tcl = Tcl::instance();

	int maxval;

	if (readhdr(tclChannel, &width_, &height_, &maxval) < 0)
		return (-1);
	int n = width_ * height_;
	if (n > 1024*1024)
		return (-1);
	delete image_;
	image_ = new u_char[2 * n];
	u_char* p = image_;

	for (int k = n >> 1; --k >= 0; ) {
		
		char R, G, B;
		int ret;
		
		ret = Tcl_Read(tclChannel, &R, sizeof(char));
		if ( ret < 0)
		{
			printf("Tcl_Read: ret = %d. Err = \"%s\"\n", ret, Tcl_ErrnoMsg(Tcl_GetErrno()));
			return (-1);
		}

		ret = Tcl_Read(tclChannel, &G, sizeof(char));
		if ( ret < 0)
		{
			printf("Tcl_Read: ret = %d. Err = \"%s\"\n", ret, Tcl_ErrnoMsg(Tcl_GetErrno()));		
		}

		ret = Tcl_Read(tclChannel, &B, sizeof(char));
		if ( ret < 0)
		{
			printf("Tcl_Read: ret = %d. Err = \"%s\"\n", ret, Tcl_ErrnoMsg(Tcl_GetErrno()));		
		}

		// can't have overflow in this direction
		double y0 = 0.299 * (u_char)R + 0.587 * (u_char)G + 0.114 * (u_char)B;
		double u = -0.1687 * (u_char)R - 0.3313 * (u_char)G + 0.5 * (u_char)B;
		double v = 0.5 * (u_char)R - 0.4187 * (u_char)G - 0.0813 * (u_char)B;
		

		ret = Tcl_Read(tclChannel, &R, sizeof(char));
		if ( ret < 0)
		{
			printf("Tcl_Read: ret = %d. Err = \"%s\"\n", ret, Tcl_ErrnoMsg(Tcl_GetErrno()));
			return (-1);
		}

		ret = Tcl_Read(tclChannel, &G, sizeof(char));
		if ( ret < 0)
		{
			printf("Tcl_Read: ret = %d. Err = \"%s\"\n", ret, Tcl_ErrnoMsg(Tcl_GetErrno()));		
		}

		ret = Tcl_Read(tclChannel, &B, sizeof(char));
		if ( ret < 0)
		{
			printf("Tcl_Read: ret = %d. Err = \"%s\"\n", ret, Tcl_ErrnoMsg(Tcl_GetErrno()));		
		}

		double y1 = 0.299 * (u_char)R + 0.587 * (u_char)G + 0.114 * (u_char)B;

		p[0] = int(y0);
		p[1] = int(u + 128);
		p[2] = int(y1);
		p[3] = int(v + 128);
		p += 4;
	}
	return 0;
}

int PPM::dump(Tcl_Channel tclChannel)
{
	int n = width_ * height_;
	if (n <= 0)
		return (-1);

	if (Tcl_Write(tclChannel, (char *)image_, 2 * n) < 0)
	{
		printf("ppm.cpp: Tcl_Write() failed. ErrMsg = %s\n", Tcl_ErrnoMsg(Tcl_GetErrno()));
		return (-1);
	}

	return (0);
}
