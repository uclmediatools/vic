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

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdio.h>
#include <sys/param.h>
#include <netinet/in.h>
#ifdef USE_SHM
extern "C" {
#include <sys/ipc.h>
#include <sys/shm.h>
#if defined(sun) && !defined(__svr4__)
int shmget(key_t, int, int);
char *shmat(int, char*, int);
int shmdt(char*);
int shmctl(int, int, struct shmid_ds*);
#endif
}
#endif

#include <osfcn.h>

#include "inet.h"
#include "rtp.h"
#include "p64.h"
#include "p64dump.h"
#include "color.h"
#include "renderer.h"
#include "vw.h"
#include "vic_tcl.h"

/* RTP v1 packet header */
struct rtp1hdr {
	u_char rh_chan;         /* version/channel */
	u_char rh_flags;        /* O/S/content */
	u_short rh_seqno;       /* sequence number */
	u_int32_t  rh_ts;
};
#define	RTPCONT_H261            31      /* CCITT H.261 */
#define	RTP_CONTENT_MASK	0x3f
#define	RTP_OPTION_BIT		0x80

int cnt = 0;
int aflag = 0;
int cflag = 0;
int Dflag = 0;
int iflag = 0;
int nflag = 0;
int qflag = 0;
int sflag = 0;
int vflag = 0;

int dumpno = -1;

u_int packets;
u_int frames;
u_int pictures;
u_int32_t tzero;
timeval start;

u_int firstseq;
u_int maxseq;
u_int seqwraps;
u_int32_t maxts;
int lastSeqno = 0;

#define H261_SLOTS 32
#define H261_SLOTMASK (H261_SLOTS - 1)
/*
 * packet 'slots' for resequencing & reassembling gobs split
 * across packet boundaries.
 */
struct h261_pslot {
	int len;
	u_short flags;
	u_short seqno;
	u_char* bp;
	int filler;
};
h261_pslot slot_[H261_SLOTS];

int fmt_ = -1;
u_int seqno_;
int packetsize_;
u_char* wrkbuf;
int wrkbuflen;
int width;
int height;

Renderer* renderer;
P64Decoder* decoder;
FILE* in;

void
usage()
{
	fprintf(stderr, "usage\n");
	exit(1);
}

char* fmtts(u_int tsdelta)
{
	static char tswrk[64];

	register u_int sec = tsdelta >> 16;
	register u_int h = sec / 3600;
	register u_int m = (sec % 3600) / 60;
	register u_int s = sec % 60;
	register u_int frac = tsdelta & 0xffff;
	register float f = float(s) + float(frac) * (1./65536.);
	sprintf(tswrk, "%02d:%02d:%06.3f", h, m, f);
	return (tswrk);
}

int nextpacket(u_char* pktbuf, int cc)
{
	if (packets == 0)
		gettimeofday(&start, 0);

	int sts = 0;
	u_int seqno;
	u_int32_t ts;
	register u_char* bp;

	u_int vers = *pktbuf >> 6;
	switch (vers) {
	case 1: {
			rtp1hdr* rh = (rtp1hdr*)pktbuf;
			cc -= sizeof(*rh);
			if ((rh->rh_chan & 0xc0) != 0x40 ||
			    (rh->rh_flags & RTP_CONTENT_MASK) != RTPCONT_H261)
				/* wrong version or content type */
				return (sts);

			seqno = ntohs(rh->rh_seqno);
			ts = ntohl(rh->rh_ts);
			bp = (u_char*)(rh + 1);
			if (rh->rh_flags & RTP_OPTION_BIT) {
				/* skip over options */
				register u_int optflags;
				do {
					optflags = bp[0];
					register u_int optlen = bp[1] << 2;
					if (optlen >= cc)
						/* empty packet or bad optlen */
						return (sts);
					bp += optlen;
					cc -= optlen;
				} while ((optflags & 0x80) == 0);
			}
		}
		break;

	case 2: {
			rtphdr* rh = (rtphdr*)pktbuf;
			int flags = ntohs(rh->rh_flags);
			if ((flags & 0x7f) != RTP_PT_H261)
				/* wrong content type */
				return (sts);
			if ((flags & 0x0f00) != 0)
				/* XXX - can't deal with csrcs */
				return (sts);
			seqno = ntohs(rh->rh_seqno);
			ts = ntohl(rh->rh_ts);
			cc -= sizeof(*rh);
			bp = (u_char*)(rh + 1);
		}
		break;

	default:
		/* unknown protocol version */
		return (sts);
	}
	if (cc <= 2)
		/* wrong size */
		return (sts);

	u_int flags = ntohs(*(u_short*)bp);
	bp += 2;
	cc -= 2;
	/*
	 * if S is set, we have a start-of-GOB, but the RTPv1/H.261
	 * spec strips the start code.  put it back.
	 */
	if (flags & 0x8000) {
		bp -= 2;
		cc += 2;
		bp[0] = 0;
		int sbit = (flags >> 12) & 7;
		if (sbit == 0)
			bp[1] = 1;
		else {
			bp[1] = 0;
			bp[2] |= 0x100 >> sbit;
		}
	}
	u_int fmt = flags & 7;
	int hascolor = flags & 0x80;
	flags >>= 8;
	u_int sbit = (flags >> 4) & 7;
	u_int ebit = flags & 7;
	if (packets == 0) {
		tzero = ts;
		maxts = ts;
		firstseq = seqno;
		maxseq = seqno;
		seqno_ = (seqno  - 1) & 0xffff;
	}
	if (Dflag || vflag) {
		printf("packet %d bytes %d fmt %d%s sbit %d%s ebit %d%s",
			seqno, cc, fmt,
			(flags & 0x80)? " S":"", sbit,
			(flags & 0x08)? " E":"", ebit,
			hascolor? " color" : "");
		if (vflag) {
			if (aflag == 0) {
				printf(" ts %s", fmtts(ts - tzero));
			} else 
				printf(" ts 0x%08x", ts);
		}
		if (((seqno_ + 1) & 0xffff) != seqno)
			printf(" *");
		printf("\n");
	}
	register int dt = int(ts - maxts);
	register int ds = (seqno - maxseq) & 0xffff;
	if (dt > 0 || (dt == 0 && ds && ds < 32768)) {
		maxts = ts;
		if (seqno < maxseq)
			/* keep track of seq no. wraps */
			++seqwraps;
		maxseq = seqno;
	}
	seqno_ = seqno;
	++packets;

	/*
	 * if the packet contains both start & end gobs we can
	 * just render it.  Otherwise we wait until we get the
	 * start and end markers and all the packets in between.
	 */
	if ((flags & 0x88) != 0x88) {
		/*
		 * if the packet has a non-zero sbit but no SOG or
		 * a non-zero ebit but no EOG, toss it.
		 */
		if ((sbit && ((flags & 0x80) == 0)) ||
		    (ebit && ((flags & 0x08) == 0)))
			return (sts);
		/*
		 * Packets waiting for reassembly are kept in 'slots'
		 * indexed by the low bits of the sequence number.
		 * To try to avoid copying packets twice, we assume
		 * that the packet source is breaking packets at some
		 * fixed, max size (which we can determine from the size
		 * of a packet with the SOG marker) and we allocate the buffer 
		 * for slots to be this size.  As long as a packet doesn't
		 * wrap, this should make the final packets contiguous.
		 */
		if (packetsize_ != cc) {
			if (packetsize_ == 0) {
				if ((flags & 0x80) == 0)
					return (sts);
				packetsize_ = cc;
				u_int len = cc * H261_SLOTS;
				u_char* wp = (u_char*)malloc(len);
				if (wp == 0) {
					fprintf(stderr,
       "vic: h261decoder: can't get %d bytes for reassembly buffer\n", len);
					packetsize_ = 0;
					return (sts);
				}
				for (int i = 0; i < H261_SLOTS; ++i) {
					slot_[i].seqno = ~0;
					slot_[i].bp = wp;
					wp += cc;
				}
				if (len > wrkbuflen) {
					if (wrkbuflen == 0)
						wrkbuf = (u_char*)malloc(len);
					else
						wrkbuf =
						  (u_char*)realloc(wrkbuf, len);
					if (wrkbuf == 0) {
						fprintf(stderr,
	       "vic: h261decoder: can't get %d bytes for work buffer\n", len);
						free(slot_[0].bp);
						packetsize_ = 0;
						wrkbuflen = 0;
						return (sts);
					}
					wrkbuflen = len;
				}
			} else if ((flags & 0x08) == 0 || cc > packetsize_) {
				static int msgcnt;
				if (++msgcnt < 20)
					fprintf(stderr,
	"vic: h261decoder: expected %d byte frags, got %d\n", packetsize_, cc);
				return (sts);
			}
		}
		register u_int l = seqno & H261_SLOTMASK;
		h261_pslot& slot = slot_[l];
		if (slot.seqno == seqno)
			/* got a duplicate */
			return (sts);
		slot.len = cc;
		slot.seqno = seqno;
		slot.flags = flags | (hascolor << 8);
		memcpy((char*)slot.bp, (char*)bp, cc);

		/*
		 * Don't bother trying to reassemble packet unless it
		 * has an EOG marker or we have the next packet.
		 */
		if ((flags & 0x08) == 0 &&
		    ((seqno + 1) & 0xffff) !=
		     slot_[(l + 1) & H261_SLOTMASK].seqno)
			return (sts);

		/* scan backward for BOG */
		register u_int pseq = seqno;
		register u_int f = pseq & H261_SLOTMASK;
		while ((slot_[f].flags & 0x80) == 0) {
			pseq = (pseq - 1) & 0xffff;
			f = pseq & H261_SLOTMASK;
			if (slot_[f].seqno != pseq)
				/* packet missing */
				return (sts);
		}

		/* scan forward for EOG */
		while ((slot_[l].flags & 0x08) == 0) {
			seqno = (seqno + 1) & 0xffff;
			l = seqno & H261_SLOTMASK;
			if (slot_[l].seqno != seqno)
				/* packet missing */
				return (sts);
		}

		if (l < f) {
			/* packet wraps around buffer, have to copy it */
			u_char* wp = wrkbuf;
			register u_int i = f - 1;
			do {
				i = (i + 1) & H261_SLOTMASK;
				register int slen = slot_[i].len;
				memcpy((char*)wp, (char*)slot_[i].bp, slen);
				wp += slen;
			} while (i != l);
			bp = wrkbuf;
			cc = wp - bp;
		} else {
			bp = slot_[f].bp;
			cc = slot_[l].bp - bp + slot_[l].len;
		}
		hascolor = slot_[f].flags >> 8;
		sbit = (slot_[f].flags >> 4) & 7;
		ebit = slot_[l].flags & 7;
	}
	lastSeqno = seqno;
	(void)decoder->decode(bp, cc, sbit, ebit, 0, 0, 0, 0, 0);
	/*
	 * This check only works if GOBs appear in order and
	 * are not fragmented. XXX i.e., it doesn't work for RTPv2/H.261
	 */
	int eop;
	if (decoder->seenMaxGob()) {
		eop = 1;
		decoder->sync();
	} else
		eop = 0;
	++frames;
	if (Dflag || vflag)
		printf("frame %d picture %d\n", frames, pictures);
	if (! nflag && (eop || iflag)) {
		Renderer* r = renderer;
		if (width != decoder->width() ||
		    height != decoder->height()) {
			width = decoder->width();
			height = decoder->height();
			r->resize(width, height, 1);
		}
#ifdef notdef
		int x, y, w, h;
		decoder->bb(x, y, w, h);
		int off = y * decoder->width() + x;
#endif
		/*XXX*/
		u_char rvts[640 * 480 / 64];
		memset(rvts, 1, sizeof(rvts));
		r->render_frame(decoder->frame(), rvts, 0);
		sts = 1;
	}
	return (sts);
}

void finish(int sts)
{
	if (!qflag) {
		timeval end;
		gettimeofday(&end, 0);

		int expected = (seqwraps << 16) + maxseq - firstseq + 1;
		int nlost = expected - packets;
		int plost = (nlost * 100) / expected;
		fprintf(stderr,
	     "got %d packets of %d (%d%% lost), %d frames, %d pictures in %s\n",
			packets, expected, plost, frames, pictures,
			fmtts(maxts - tzero));
		double s = end.tv_sec - start.tv_sec;
		s += 1e-6 * (end.tv_usec - start.tv_usec);
		fprintf(stderr, "running time %g sec (%g fps)\n", s,
			pictures / s);
	}
	exit(sts);
}

int nextrec()
{
	if (cnt > 0 && --cnt == 0)
		finish(0);;

	if (feof(in))
		finish(0);;

	struct ivs_record_hdr {
		u_short length;
		u_short delay;
	} irh;
	fread(&irh, sizeof(irh), 1, in);
	int length = ntohs(irh.length);
	if (length > 8000 || length <= 0) {
		printf("ridiculous packet length in file\n");
		finish(1);;
	}
	u_char pktbuf[8192];
	register int len = fread(pktbuf, 1, length, in);
	if (len != length) {
		if (len > 0) {
			fprintf(stderr,
				"-truncated read: expected %d, got %d\n",
				length, len);
		}
		finish(2);;
	}
	if (ntohs(irh.delay) & 0x8000)
		/* got a session packet (ignore for now) */
		return (-1);

	return (nextpacket(pktbuf, length));
}

#ifndef USE_SHM
int use_shm = 0;
#else
int use_shm = 1;
static int
noXShm(Display* dpy, XErrorEvent*)
{
	use_shm = 0;
	fprintf(stderr, "vdd: warning: not using shared memory\n");
	return (0);
}

/*
 * XXX this is a hack to see if we can used shared memory.
 * if the X server says we can, and we\'re on the same
 * host as the X server, then we are golden.
 */
static void
checkXShm(Display* dpy)
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
cmd_decoder(ClientData cd, Tcl_Interp*, int argc, char** argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "next") == 0) {
			while (nextrec() == 0) {
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "frame") == 0) {
			char* bp = tcl.buffer();
			sprintf(bp, "%d %d", frames, pictures);
			tcl.result(bp);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "nextpacket") == 0) {
			nextrec();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "format") == 0) {
			tcl.result(decoder->width() == 352 ? "cif" : "qcif");
			return (TCL_OK);
		}
		if (strcmp(argv[1], "dump") == 0) {
#ifdef notyet
			dump_ppm(0,
				 decoder->yplane(),
				 decoder->uplane(),
				 decoder->vplane(), 
				 decoder->width(), decoder->height());
			return (TCL_OK);
#endif
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "dump") == 0) {
#ifdef notyet
			dump_ppm(argv[2],
				 decoder->yplane(),
				 decoder->uplane(),
				 decoder->vplane(), 
				 decoder->width(), decoder->height());
			return (TCL_OK);
#endif
		}
		if (strcmp(argv[1], "target") == 0) {
			renderer = (Renderer*)TclObject::lookup(argv[2]);
			renderer->resize(decoder->width(),
					 decoder->height(), 1);
			return (TCL_OK);
		}
	}
	return (TCL_ERROR);
}

char*
parse_assignment(char* cp)
{
	cp = strchr(cp, '=');
	if (cp != 0) {
		*cp = 0;
		return (cp + 1);
	} else
		return ("true");
}

int
main(int argc, const char **argv)
{
	Tcl::init("vdd");

	char clipvers[8];
	int op;
	int osize = 2;

	const char* optstr = "abC:c:Dd:F:inSstu:vqX:";
	const char* display = disparg(argc, argv, optstr);
	Tcl::init("vdd");
	Tcl& tcl = Tcl::instance();
	Tk_Window tk = Tk_CreateMainWindow(tcl.interp(), (char*)display,
					   "vdd", "Vdd");
	if (tk == 0) {
		fprintf(stderr, "vdd: %s\n", tcl.result());
		exit(1);
	}
	tcl.tkmain(tk);

	EmbeddedTcl::init();
	tcl.evalc("init_resources");

	while ((op = getopt(argc, (char**)argv, optstr)) != -1) {
		switch (op) {

		default:
			usage();

		case 'a':
			aflag = 1;
			break;

		case 'b':
			osize = 3;
			break;

		case 'd':
			break;

		case 'c':
			tcl.add_option("dither", optarg);
			break;

		case 'C': 
			cnt = atoi(optarg);
			break; 

		case 'F':
			dumpno = atoi(optarg);
			break;

		case 'D': 
			++Dflag;
			break; 

		case 'i':
			++iflag;
			break;

		case 'n':
			++nflag;
			break;

		case 'q':
			qflag = 1;
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

		case 'v':
			vflag = 1;
			break;
			
		case 'X':
			{
				const char* value = parse_assignment(optarg);
				tcl.add_option(optarg, value);
			}
			break;
		}
	}
#ifdef USE_SHM
	if (use_shm)
		checkXShm(Tk_Display(tk));
#endif

	if (optind >= argc || argc <= 1)
		usage();
	const char* infile = argv[optind];

	if (Dflag)
		decoder = new P64Dumper;
	else
		decoder = new FullP64Decoder;

	/*XXX*/
	in = fopen(infile, "r");
	if (in == 0) {
		perror(infile);
		exit(1);
	}
	in = fopen(infile, "r");
	if (in == 0) {
		perror(infile);
		exit(1);
	}

	/*
	 * Check to see which of the 3 incompatible ivs_record
	 * formats this file uses.
	 * XXX seems we handle only the latest now
	 */
	op = fread(clipvers, 1, sizeof(clipvers), in);
	if (op != sizeof(clipvers)) {
		fprintf(stderr, "-%s: truncated read of %s (%d bytes)\n",
			argv[0], infile, op);
		exit(2);
	}
	if (strncmp(clipvers, "IVS CLIP", 8) == 0 ||
	    strncmp(clipvers, "RTPCLIP ", 8) == 0) {
		while ((op = getc(in)) != 0 && op != EOF) {
		}
		if (op == EOF) {
			fprintf(stderr, "-%s: unexpected eof in %s\n", argv[0],
				infile);
			exit(3);
		}
	} else {
		fseek(in, 0L, 0);
	}

	tcl.CreateCommand("decoder", cmd_decoder);
	tcl.evalf("vdd_main %d %d", 88 << osize, 72 << osize);

	Tk_MainLoop();
	exit(0);
}
