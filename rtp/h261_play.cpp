/*
 * Copyright (c) 1994 The Regents of the University of California.
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

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include <stdio.h>
#ifdef WIN32
#include <winsock.h>
#include "config.h"
#else
#include <sys/param.h>
#include <sys/time.h>
#include <netinet/in.h>
#endif
#ifdef USE_SHM
extern "C" {
#include <sys/ipc.h>
#include <sys/shm.h>
#if defined(sparc) && !defined(__svr4__)
int shmget(key_t, int, int);
char *shmat(int, char*, int);
int shmdt(char*);
int shmctl(int, int, struct shmid_ds*);
#endif
}
#endif

#ifdef WIN32
#include <stdlib.h>
//#else
//#include <osfcn.h>
#endif

#include "p64.h"
#include "p64dump.h"
#include "color.h"
#include "renderer.h"
#include "vw.h"
#include "vic_tcl.h"

void
usage()
{
	fprintf(stderr, "usage\n");
	exit(1);
}

#ifndef USE_SHM
int use_shm = 0;
#else
int use_shm = 1;
static int
noXShm(Display* , XErrorEvent*)
{
	use_shm = 0;
	fprintf(stderr, "h261_play: warning: not using shared memory\n");
	return (0);
}

/*
 * XXX this is a hack to see if we can used shared memory.
 * if the X server says we can, and we\'re on the same
 * host as the X server, then we are golden.
 */
static void
checkXShm(Display* dpy, const char* )
{
	XShmSegmentInfo si;
	if ((si.shmid = shmget(IPC_PRIVATE, 512, IPC_CREAT|0777)) < 0) {
		perror("shmget");
		exit(1);
	}
	si.readOnly = 1;
	XSync(dpy, 0);
	XSetErrorHandler(noXShm);
	XShmAttach(dpy, &si);
	XSync(dpy, 0);
	XSetErrorHandler(NULL);
	(void)shmctl(si.shmid, IPC_RMID, 0);
}
#endif

P64Decoder* decoder;
Renderer* renderer;
FILE* in;

int nflag = 0;

int nframe = 0;
timeval start;

struct offset {
	off_t off;
	int bit;
};
#define MAXFRAME 100000
offset offs[MAXFRAME];

void
dump_ppm(char* file, const u_char* yp, const u_char* up, const u_char* vp,
	 int w, int h)
{
	FILE *f;
	if (file == 0)
		f = stdout;
	else {
		(void)unlink(file);
		f = fopen(file, "w");
		if (f == 0)
			/*XXX*/
			return;
	}
	fprintf(f, "P3\n%d %d\n255\n", w, h);
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			double Y = (double)yp[w * y + x];
			double U = up[(w / 2) * (y / 2) + x / 2] - 128.;
			double V = vp[(w / 2) * (y / 2) + x / 2] - 128.;
			
			int r = int(Y + 1.402 * V);
			int g = int(Y - 0.34414 * U - 0.71414 * V);
			int b = int(Y + 1.772 * U);
#define LIM(x) ((x) >= 255 ? 255 : ((x) < 0 ? 0 : (x)))
			r = LIM(r);
			g = LIM(g);
			b = LIM(b);
			fprintf(f, "%d %d %d\n", r, g, b);
		}
	}
	if (f == stdout)
		fflush(stdout);
	else
		fclose(f);
}

void
buildindex(FILE* f)
{
	off_t off = 0;
	int n = 0;
	u_int bb;

	bb = getc(f);
	bb <<= 8;
	bb |= getc(f);
	int v = getc(f);
	bb <<= 4;
	bb |= v >> 4;
	v <<= 4;
	for (;;) {
		int bit;
		for (bit = 0; bit < 4; ++bit) {
			if ((bb & 0xfffff) == 0x00010) {
				offset* p = &offs[n++];
				p->bit = bit;
				p->off = off;
			}
			bb <<= 1;
			v <<= 1;
			bb |= (v >> 8) & 1;
		}
		v = getc(f);
		if (feof(f))
			break;
		for (; bit < 8; ++bit) {
			if ((bb & 0xfffff) == 0x00010) {
				offset* p = &offs[n++];
				p->bit = bit;
				p->off = off;
			}
			bb <<= 1;
			v <<= 1;
			bb |= (v >> 8) & 1;
		}
		++off;
	}
	/*XXX skip last frame, since we use offs[cfrm+1] below*/
	nframe = n - 1;
	rewind(f);
}

int lflag;
int cfrm;

/*XXX*/
u_char crvec[768 * 576 / 64];
u_int now = 1;

void render_frame(const u_char* frm, int w, int h)
{
	/*
	 * Go through all the timestamps and smash the time that
	 * is about to wrap from the past into the future to the present,
	 * so that the block gets updated just once.  If we let timestamps
	 * wrap without doing anything, the algorithm would still work,
	 * but we'd end up rendering a stationary block on every call here
	 * (until we surpassed now + 128 again).  In other words, for
	 * an unchanging block, this approach renders it 2 times out of 256,
	 * rather than 128 out of 256.
	 */
	int wraptime = now ^ 0x80;
	u_char* ts = crvec;
	for (int k = sizeof(crvec); --k >= 0; ++ts) {
		if (*ts == wraptime)
			*ts = now;
	}

	YuvFrame f(now, (u_int8_t*)frm, crvec, w, h);
	renderer->consume(&f);
	now = (now + 1) & 0xff;
	decoder->mark(now);
}

int nextframe()
{
	if (cfrm >= nframe-1) {
		if (!lflag) {
			fprintf(stderr, "h261_play: eof\n");
			timeval end;
			gettimeofday(&end, 0);
			double v = end.tv_sec - start.tv_sec;
			v += 1e-6 * (end.tv_usec - start.tv_usec);
			fprintf(stderr,
			"h261_play: %d frames in %g seconds (%g fps)\n",
				nframe, v, (double)nframe / v);
			exit(0);
		}
		cfrm = 0;
	}
	if (cfrm == 0)
		gettimeofday(&start, 0);

	offset* p = &offs[cfrm++];
	offset* nxt = &offs[cfrm];

	if (fseek(in, p->off, 0) < 0) {
		perror("fseek");
		exit(1);
	}
	int len = nxt->off - p->off + 1;
	int ebit = 8 - nxt->bit;
	if (ebit == 8) {
		len -= 1;
		ebit = 0;
	}

	u_char buffer[80*1024];
	if (fread((char*)buffer, len, 1, in) == 0) {
		perror("fread");
		exit(1);
	}

	P64Decoder* d = decoder;
	d->decode(buffer, len, p->bit, ebit, 0, 0, 0, 0, 0);
	d->sync();
	if (nflag)
		return (TCL_OK);

	render_frame(d->frame(), d->width(), d->height());
	return (TCL_OK);
}

extern "C" char *optarg;
extern "C" int optind;
extern "C" int opterr;

const char*
disparg(int argc, const char** argv, const char* optstr)
{
	const char* display = 0;
	int op;
	while ((op = getopt(argc, (char**)argv, (char*)optstr)) != -1) {
		if (op == 'd') {
			display = optarg;
			break;
		}
		else if (op == '?')
			usage();
	}
	optind = 1;
	return (display);
}

/*
 * decoder decode $frmno
 * decoder dump [file]
 * decoder next
 * decoder frame
 */
int
cmd_decoder(ClientData , Tcl_Interp*, int argc, const char** argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "frame") == 0) {
			char* bp = tcl.buffer();
			sprintf(bp, "%d", cfrm);
			tcl.result(bp);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "next") == 0) {
			nextframe();
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "renderer") == 0) {
			renderer = (Renderer*)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
#ifdef notdef
		if (strcmp(argv[1], "dump") == 0) {
			dump_ppm(argv[2],
				 decoder->yplane(),
				 decoder->uplane(),
				 decoder->vplane(), 
				 decoder->width(), decoder->height());
			return (TCL_OK);
		}
#endif
	}
	return (TCL_ERROR);
}

#ifndef WIN32
extern "C" {
int
TkPlatformInit(Tcl_Interp *interp)
{
	extern void TkCreateXEventSource(void);
	Tcl_SetVar(interp, "tk_library", "", TCL_GLOBAL_ONLY);
	TkCreateXEventSource();
	return (TCL_OK);
}
}
#endif

int
main(int argc, const char **argv)
{
	int op;
	int Dflag = 0;
	int osize = 2;

	const char* optstr = "bCc:Dd:lnpSstu:V:";
	const char* display = disparg(argc, argv, optstr);
	Tcl::init("h261_play");
	Tcl& tcl = Tcl::instance();
#ifdef WIN32
	if (display == NULL)
		display = "localhost:0";
#endif
	tcl.evalf(display?
		    "set argv \"-name h261_play -display %s\"" :
		    "set argv \"-name h261_play\"",
		  display);
	Tk_Window tk = 0;
	if (Tk_Init(tcl.interp()) == TCL_OK)
		tk = Tk_MainWindow(tcl.interp());
	if (tk == 0) {
		fprintf(stderr, "h261_play: %s\n", tcl.result());
		exit(1);
	}
	tcl.tkmain(tk);
	EmbeddedTcl::init();
	tcl.evalc("init_resources");

	while ((op = getopt(argc, (char**)argv, optstr)) != -1) {
		switch (op) {

		default:
			usage();

		case 'b':
			osize = 3;
			break;

		case 'C': 
			tcl.add_option("showChanges", "true");
			break; 

		case 'c':
			tcl.add_option("dither", optarg);
			break;

		case 'D': 
			++Dflag;
			break; 

		case 'd':
			break;

		case 'l':
			++lflag;
			break;

		case 'n':
			++nflag;
			break;

		case 'p':
			tcl.add_option("pause", "false");
			break;

		case 's':
			osize = 1;
			break;

		case 'S':
			use_shm = 0;
			break;

		case 't':
			osize = 0;
			break;

		case 'u':
			tcl.add_option("startupScript", optarg);
			break;

		case 'V':
			tcl.add_option("visual", optarg);
			break;

		}
	}
#ifdef USE_SHM
	if (use_shm)
		checkXShm(Tk_Display(tk), display);
#endif

	if (optind >= argc || argc <= 1)
		usage();
	const char* infile = argv[optind];

	if (Dflag)
		decoder = new P64Dumper;
	else
		decoder = new FullP64Decoder;

	decoder->marks(crvec);
	decoder->mark(now);

#ifdef WIN32
	in = fopen(infile, "rb");
#else
	in = fopen(infile, "r");
#endif
	if (in == 0) {
		perror(infile);
		exit(1);
	}
	/*
	 * Check that the file begins with a picture header.
	 */
	u_char phdr[3];
	if (fread(phdr, 3, 1, in) != 1 ||
	    phdr[0] != 0 || phdr[1] != 1 || (phdr[2] >> 4) != 0) {
		fprintf(stderr, "h261_play: %s not a raw H.261 bit stream\n",
			infile);
		exit(1);
	}
	rewind(in);
	fprintf(stderr, "h261_play: building frame index...");
	buildindex(in);
	fprintf(stderr, "ready - %d frames\n", nframe);

	tcl.CreateCommand("decoder", cmd_decoder);
	tcl.evalf("h261_main %d %d", 88 << osize, 72 << osize);

	Tk_MainLoop();
	exit(0);
}
