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
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#ifdef sgi
#include <getopt.h>
#endif
#ifdef WIN32
extern "C" {
int getopt(int, char * const *, const char *);
extern int optind;
extern char *optarg;
}
#endif

struct color {
	short r;
	short g;
	short b;
	short y;
	short u;
	short v;
};

int yuv_to_rgb(struct color* c)
{
	double y = c->y;
	double u = c->u - 128.;
	double v = c->v - 128.;

	int r = int(y + 1.402 * v + 0.5);
	int g = int(y - 0.34414 * u - 0.71414 * v + 0.5);
	int b = int(y + 1.772 * u + 0.5);

	int valid = 1;
#define CHECK(v) \
	if (v > 255) { \
		v = 255; \
		valid = 0; \
	} else if (v < 0) { \
		v = 0; \
		valid = 0; \
	}
	CHECK(r)
	c->r = r;
	CHECK(g)
	c->g = g;
	CHECK(b)
	c->b = b;

	return (valid);
}

void
rgb()
{
	int r, g, b;

	printf("P3\n%d 1\n255\n", 4 * 8 * 4);
	for (b = 0; b < 4; ++b) {
		for (g = 0; g < 8; ++g) {
			for (r = 0; r < 4; ++r) {
				int red = r * 16384 + 16383;
				int green = g * 8192 + 8191;
				int blue = b * 16384 + 16383;
				printf("%d %d %d\n", red >> 8,
				       green >> 8, blue >> 8);
			}
		}
	}
}

void
cklimit(int n)
{
	if (n >= 256) {
		fprintf(stderr, "mkcube: too many colors\n");
		exit(1);
	}
}

void
yuv(int ystep, int ustep, int vstep, int Fflag)
{
	int i;
	int ncolor = 0;
	struct color colors[256];
	color c;

	/*
	 * uv grids - start from gray levels and work outward until
	 * we hit infeasible colors.
	 */
	int v;
	for (v = 128; v < 256; v += vstep) {
		c.v = v;
		int u;
		for (u = 128; u < 256; u += ustep) {
			c.u = u;
			for (int y = 0; y < 256; y += ystep) {
				c.y = y;
				if (yuv_to_rgb(&c)) {
					cklimit(ncolor);
					colors[ncolor++] = c;
				}
			}
		}
		for (u = 128 - ustep; u >= 0; u -= ustep) {
			c.u = u;
			for (int y = 0; y < 256; y += ystep) {
				c.y = y;
				if (yuv_to_rgb(&c)) {
					cklimit(ncolor);
					colors[ncolor++] = c;
				}
			}
		}
	}
	for (v = 128 - vstep; v >= 0; v -= vstep) {
		c.v = v;
		int u;
		for (u = 128; u < 256; u += ustep) {
			c.u = u;
			for (int y = 0; y < 256; y += ystep) {
				c.y = y;
				if (yuv_to_rgb(&c)) {
					cklimit(ncolor);
					colors[ncolor++] = c;
				}
			}
		}
		for (u = 128 - ustep; u >= 0; u -= ustep) {
			c.u = u;
			for (int y = 0; y < 256; y += ystep) {
				c.y = y;
				if (yuv_to_rgb(&c)) {
					cklimit(ncolor);
					colors[ncolor++] = c;
				}
			}
		}
	}

	if (Fflag) {
		/* some extra flesh tones */
		for (v = 140; v <= 190; v += 15) {
			c.v = v;
			for (int u = 80; u <= 110; u += 15) {
				c.u = u;
				/* note y starts on grid for YBITS=3 or 4 */
				for (int y = 96; y < 192; y += ystep) {
					c.y = y;
					if (yuv_to_rgb(&c)) {
						cklimit(ncolor);
						colors[ncolor++] = c;
					}
				}
			}
		}
	}

	printf("P3\n%d 1\n255\n", ncolor);
	for (i = 0; i < ncolor; ++i)
		printf("%d %d %d\n", colors[i].r, colors[i].g, colors[i].b);
}

int
ystep_to_ybits(int step)
{
	if (step > 0 && step < 256) {
		int ybits = 8;
		while ((step & 1) == 0) {
			step >>= 1;
			--ybits;
		}
		if ((step >> 1) == 0)
			return (ybits);
	}
	fprintf(stderr, "mkcube: -y arg must be power of 2 in [2,128]\n");
	exit(1);
	return (0);
}

void
usage()
{
	fprintf(stderr,
		"usage: mkcube [-y step] [-u step] [-v step] rgb|yuv\n");
	exit(1);
}

extern char *optarg;
extern int optind;

int
main(int argc, char **argv)
{
	int udelta = 40;
	int vdelta = 40;
	int ybits = 3;
	int Fflag = 0;

	int op;
	while ((op = getopt(argc, argv, "FU:V:Y:")) != -1) {
		switch (op) {
		case 'F':
			++Fflag;
			break;
		case 'U':
			udelta = atoi(optarg);
			break;

		case 'V':
			vdelta = atoi(optarg);
			break;

		case 'Y':
			ybits = atoi(optarg);
			break;

		case '?':
			usage();
		}
	}
	char* which = 0;
	if (optind < argc && argc > 1) {
		if (argc - optind > 1)
			usage();
		which = argv[optind];
	} else 
		usage();
	if (strcmp(which, "rgb") == 0)
		rgb();
	else if (strcmp(which, "yuv") == 0)
		yuv(1 << (8 - ybits), udelta, vdelta, Fflag);
	else
		usage();

	return (0);
}
