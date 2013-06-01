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

/*
 * ppmtolut - perform various transformation from colormaps (in the
 * form of a ppm file) to lookup tables.
 */

#include <stdlib.h> //SV-XXX: for exit()
#include <stdio.h>
#include <sys/types.h> 
#include <math.h>
#ifdef WIN32
#include <winsock.h>
extern int getopt(int, char * const *, const char *);
#else
#include <getopt.h> //SV-XXX: for getopt()
#endif

/* D65 Reference Whites */
#define REFX	(0.981*255.)
#define REFY	(1.0*255.)
#define REFZ	(1.177*255.)

#define UFORM(x, y, z)	(4.0 * (x) / ((x) + 15.0 * (y) + 3.0 * (z)))
#define VFORM(x, y, z)  (9.0 * (y) / ((x) + 15.0 * (y) + 3.0 * (z)))

#define Un UFORM(REFX, REFY, REFZ)
#define Vn VFORM(REFX, REFY, REFZ)

double factor;

struct color {
	double r;
	double g;
	double b;
	double y;
	double u;
	double v;
	double Lstar;
	double ustar;
	double vstar;
	double X;
	double Y;
	double Z;
};

void
XYZ_to_Luv(struct color *c)
{
	double x, y, z, t;

	x = c->X;
	y = c->Y;
	z = c->Z;

	if (x == 0. && y == 0. && z == 0.) {
		c->Lstar = 0.;
		c->ustar = 0.;
		c->vstar = 0.;
		return;
	}
	t = y / REFY;
	if (t > 0.008856)
		c->Lstar = 116. * pow(t, (double)(1 / 3.)) - 16.;
	else
		c->Lstar = 903.3 * t;

	c->ustar = 13. * c->Lstar * (UFORM(x, y, z) - Un);
	c->vstar = 13. * c->Lstar * (VFORM(x, y, z) - Vn);
}

void
rgb_to_XYZ(struct color *c)
{
	double r, g, b;

	r = (double)c->r;
	g = (double)c->g;
	b = (double)c->b;

	c->X = 0.607 * r + 0.174 * g + 0.200 * b;
	c->Y = 0.299 * r + 0.587 * g + 0.114 * b;
	c->Z =             0.066 * g + 1.111 * b;
}

void
yuv_to_rgb(struct color* c)
{
	double y = c->y;
	double u = c->u;
	double v = c->v;

	/* CCIR 709 YCbCr */
	double r = y + 1.402 * v;
	double g = y - 0.34414 * u - 0.71414 * v;
	double b = y + 1.772 * u;

#define LIM(x) ((x) >= 255. ? 255. : ((x) < 0. ? 0. : (x)))
	c->r = LIM(r);
	c->g = LIM(g);
	c->b = LIM(b);
}

void
rgb_to_yuv(struct color *p)
{
	double r = p->r;
	double g = p->g;
	double b = p->b;

	p->y = 0.299 * r + 0.587 * g + 0.114 * b;
	p->u = -0.1687 * r - 0.3313 * g + 0.5 * b;
	p->v = 0.5 * r - 0.4187 * g - 0.0813 * b;
	if (p->y < 0. || p->y >= 256.) {
		fprintf(stderr, "ppmtolut: y overflow %f\n", p->y); //SV-XXX: %d -> %f
		exit(1);
	}
	if (p->u < -128. || p->u >= 128.) {
		fprintf(stderr, "ppmtolut: u overflow %f\n", p->u); //SV-XXX: %d -> %f
		exit(1);
	}
	if (p->v < -128. || p->v >= 128.) {
		fprintf(stderr, "ppmtolut: v overflow %f\n", p->v); //SV-XXX: %d -> %f
		exit(1);
	}
}

void
rgb_to_all(struct color *p)
{
	rgb_to_yuv(p);
	rgb_to_XYZ(p);
	XYZ_to_Luv(p);
}

double
yuv_distance(struct color *c, struct color *p)
{
	double d, t;

	d = factor * (c->y - p->y);
	d *= d;
	t = (c->u - p->u);
	t *= t;
	d += t;
	t = (c->v - p->v);
	t *= t;
	d += t;

	return (d);
}

double
rgb_distance(struct color *c, struct color *p)
{
	double d, t;

	d = (c->r - p->r);
	d *= d;
	t = (c->g - p->g);
	t *= t;
	d += t;
	t = (c->b - p->b);
	t *= t;
	d += t;

	return (d);
}


double
luv_distance(struct color *c, struct color *p)
{
	double d, t;

	d = factor * (c->Lstar - p->Lstar);
	d *= d;
	t = (c->ustar - p->ustar);
	t *= t;
	d += t;
	t = (c->vstar - p->vstar);
	t *= t;
	d += t;

	return (d);
}

int
closest(int ncolor, struct color *colors, struct color *c, 
	double (*distfn)(struct color *, struct color *))
{
	int i;
	int winner = ncolor - 1;
	double best = distfn(c, &colors[winner]);

	for (i = winner - 1;  --i >= 0; ) {
		double d = distfn(c, &colors[i]);
		if (d < best) {
			best = d;
			winner = i;
		}
	}
	return (winner);
}

int
readhdr(FILE *fp, int *width, int *height, int *maxval)
{
	char line[1024];
	if (fgets(line, sizeof(line), fp) == 0 ||
	    line[0] != 'P' || line[1] != '3')
		return (-1);

	if (fgets(line, sizeof(line), fp) == 0 || 
	    sscanf(line, "%d %d", width, height) != 2)
		return (-1);

	if (fgets(line, sizeof(line), fp) == 0 || 
	    sscanf(line, "%d", maxval) != 1)
		return (-1);

	return (0);
}

void
readcolors(FILE *fp, int ncolor, struct color *p)
{
	int i;
	char line[1024];

	for (i = 0; i < ncolor; ++i) {
		int r, g, b;
		if (fgets(line, sizeof(line), fp) == 0) {
			fprintf(stderr, "ppmtolut: premature EOF at %d\n", i);
			exit(1);
		}
		if (sscanf(line, "%d %d %d", &r, &g, &b) != 3) {
			fprintf(stderr, "ppmtolut: bad pixel %d\n", i);
			exit(1);
		}
		p->r = (double)r;
		p->g = (double)g;
		p->b = (double)b;

		rgb_to_all(p);
		++p;
	}
}

void
quantize_colors(int ncolor, struct color *p, int nbit)
{
	int i;

	for (i = 0; i < ncolor; ++i) {
		p->y = (int)p->y >> nbit << nbit;
		p->u = (int)p->u >> nbit << nbit;
		p->v = (int)p->v >> nbit << nbit;
		yuv_to_rgb(p);
		rgb_to_all(p);
		++p;
	}
}

/*
 * Pack rgb values into a single byte array.
 */
void
output_cmap(FILE *f, const char *id, int ncolor, struct color *colors)
{
	int i;

	fprintf(f, "const int %s_ncolor = %d;\n", id, ncolor);
	fprintf(f, "unsigned char %s_cmap[3 * %d] = {", id, ncolor);
	for (i = 0; i < ncolor; ++i) {
		char *tab = ((i % 3) == 0) ? "\n\t" : " ";
		fprintf(f, "%s%d, %d, %d,", tab,
			(int)colors[i].r, (int)colors[i].g, (int)colors[i].b);
	}
	if ((i % 3) == 0)
		fprintf(f, "\n");
	fprintf(f, "};\n");
}

/*
 * Pack rgb values into a single byte array.
 */
void
dump_cmap(FILE *f, int ncolor, struct color *colors)
{
	int i;
	char c;
	u_char *p;
	u_char cmap[3 * 256];

	p = cmap;
	for (i = 0; i < ncolor; ++i) {
		*p++ = (int)colors[i].r;
		*p++ = (int)colors[i].g;
		*p++ = (int)colors[i].b;
	}
	c = ncolor;
	(void)fwrite(&c, 1, 1, f);
	(void)fwrite((char*)cmap, 3 * ncolor, 1, f);
}

/*
 * Output a 64K table such that if we index it with <5U:5V:6Y> tuple,
 * we will get the corresponding pixel entry in the colormap output
 * above.
 */
void
output_lut(FILE *f, const char* id, u_char* yuvtopix, int size)
{
	int i;

	fprintf(f, "unsigned char %s_lut[] = {", id);
	for (i = 0; i < size; ++i) {
		char *tab = ((i & 7) == 0) ? "\n\t" : " ";
		fprintf(f, "%s%d,", tab, yuvtopix[i]);
	}
	if ((i & 7) == 0)
		fprintf(f, "\n");
	fprintf(f, "};\n");
}

/*
 * Build a 64K table such that if we index it with <5U:5V:6Y> tuple,
 * we will get the corresponding pixel entry in the colormap output
 * above.
 */
void
build_quant_lut(int ncolor, struct color *colors, u_char* yuvtopix, 
		double (*distfn)(struct color *, struct color *))
{
	int i, y, u, v;

	struct color c;
	i = 0;
	for (u = -128; u < 128; u += 8) {
		c.u = (double)u;
		for (v = -128; v < 128; v += 8) {
			c.v = (double)v;
			for (y = 0; y < 256; y += 4) {
				int pixel;
				c.y = (double)y;
				yuv_to_rgb(&c);
				if (distfn == luv_distance) {
					rgb_to_XYZ(&c);
					XYZ_to_Luv(&c);
				}
				pixel = closest(ncolor, colors, &c, distfn);
				yuvtopix[i++] = pixel;
			}
		}
	}
}

int
uv_closest(int ncolor, struct color *colors, struct color* c)
{
	int best_dist;
	int best_color;
	int i = ncolor - 1;
	int y = (int)(c->y + 0.5);
	while ((int)(colors[i].y + 0.5) != y) {
		if (--i < 0) {
			fprintf(stderr,
				"ppmtolut: couldn't find color at y = %f\n",
				c->y); //SV-XXX: %d -> %f
			exit(1);
		}
	}
	best_dist = yuv_distance(c, &colors[i]);
	best_color = i;
	while (--i >= 0) {
		int d;
		if ((int)(colors[i].y + 0.5) != y)
			continue;
		d = yuv_distance(c, &colors[i]);
		if (d < best_dist) {
			best_dist = d;
			best_color = i;
		}
	}
	return (best_color);
}

void
build_ed_lut(int ncolor, struct color *colors, u_char* lut, int ybits)
{
	int y, u, v;
	int i = 0;
	for (v = 0; v < 1 << 6; ++v) {
		struct color c;
		c.v = (double)(v << 2) - 128.;
		for (u = 0; u < 1 << 6; ++u) {
			c.u = (double)(u << 2) - 128.;
			for (y = 0; y < 1 << ybits; ++y) {
				int pix;
				c.y = (double)(y << (8-ybits));
				pix = uv_closest(ncolor, colors, &c);
				lut[i++] = pix;
			}
		}
	}
}

void
usage()
{
	fprintf(stderr, "Usage: ppmtolut [-dervxy] [-f file] [-n id] [map]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	FILE *f;
	int width;
	int height;
	int maxval;
	int ncolor;
	int ybits = 3;
	int eflag = 0;
	int quant = 0;
	int xflag = 0;
	int vflag = 0;
	char *cmap = 0;
	char *file = 0;
	int size;
	struct color *colors;
	char *id = "builtin";
	u_char *lut;
	int op;
	double (*distfn)(struct color *, struct color *);

	extern char *optarg;
	extern int optind; //SV-XXX: unused: opterr;

	distfn = yuv_distance;
	factor = 1.0;
	while ((op = getopt(argc, argv, "ef:ln:q:rs:vxyY:")) != -1) {
		switch (op) {
		case 'e':
			++eflag;
			break;
		case 'f':
			file = optarg;
			break;
		case 'l':
			distfn = luv_distance;
			break;
		case 'n':
			id = optarg;
			break;
		case 'q':
			quant = atoi(optarg);
			break;
		case 'r':
			distfn = rgb_distance;
			break;
		case 's':
			factor = atof(optarg);
			break;
		case 'v':
			++vflag;
			break;
		case 'x':
			++xflag;
			break;
		case 'y':
			distfn = yuv_distance;
			break;
		case 'Y':
			ybits = atoi(optarg);
			break;
		default:
			usage();
			break;
		}
		
	}
	if (optind < argc && argc > 1) {
		if (argc - optind > 1)
			usage();
		cmap = argv[optind];
	}
	if (cmap == 0)
		f = stdin;
	else {
		f = fopen(cmap, "r");;
		if (f == 0) {
			perror(cmap);
			exit(1);
		}
	}
	if (readhdr(f, &width, &height, &maxval) < 0) {
		fprintf(stderr, "ppmtolut: bad ppm header\n");
		exit(1);
	}
	ncolor = width * height;
	if (vflag) {
		if (maxval!= 255)
			fprintf(stderr, "ppmtolut: warning: maxval != 255\n");
		fprintf(stderr, "ppmtolut: found %d colors\n", ncolor);
	}
	if (ncolor > 1024 || ncolor <= 0) {
		fprintf(stderr, "ppmtolut: bad ppm header (w %d, h %d)\n",
			width, height);
		exit(1);
	}
	colors = (struct color *)malloc(ncolor * sizeof(*colors));
	if (colors == 0) {
		fprintf(stderr, "ppmtolut: no swap\n");
		exit(1);
	}
	readcolors(f, ncolor, colors);
	fclose(f);

	if (quant != 0)
		quantize_colors(ncolor, colors, quant);

	if (file == 0)
		f = stdout;
	else {
		f = fopen(file, "w");
		if (f == 0)
			perror(file);
	}

	if (eflag) {
		/*
		 * Build (ybits):6:6 lookup table for
		 * error-diffusion algorithm.
		 */
		size = 1 << (ybits+6+6);
		lut = (u_char*)malloc(size);
		build_ed_lut(ncolor, colors, lut, ybits);
	} else {
		/*
		 * Build 6:5:5 quantization table.
		 * Indexing by a <5V:5U:6Y> tuple gives you
		 * the closest color in the input colormap.
		 */
		size = 1 << (6+5+5);
		lut = (u_char*)malloc(size);
		build_quant_lut(ncolor, colors, lut, distfn);
	}
	if (!xflag) {
		/*
		 * Output c code
		 */
		output_cmap(f, id, ncolor, colors);
		output_lut(f, id, lut, size);
	} else {
		/*
		 * Dump a binary file.
		 */
		dump_cmap(f, ncolor, colors);
		(void)fwrite((char*)lut, size, 1, f);
	}
	exit(0);
}
