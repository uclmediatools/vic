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

/*
 * histtolut - compute a colormap using Heckbert's median cut.
 * input is a 5V:5U:6Y histogram table and number of desired
 * colors.  output is a colormap and lookup table as generated
 * by ppmtolut.
 *
 * Written by Elan Amir, August 1994.
 * Modified for stand-alone operation by Steven McCanne, August 1994.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <unistd.h>
#include <strings.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#ifdef WIN32
#include <winsock.h>
#include <io.h>
#else
#include <sys/file.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring> //SV-XXX: FreeBSD

#ifdef WIN32
extern "C" {
int getopt(int, char * const *, const char *);
extern int optind;
extern char *optarg;
}
#endif
  
extern "C" int convex_hull_ncolor;
extern "C" u_char convex_hull[];

struct color {
	short r;
	short g;
	short b;
	short y;
	short u;
	short v;
};

struct histItem {
	color c;
	int value;
};

struct box {
	int ind;
	int colors;
	int sum;
};

void rgb_to_yuv(color& c)
{
	double r = c.r;
	double g = c.g;
	double b = c.b;

	c.y = short(0.299 * r + 0.587 * g + 0.114 * b + 0.5);
	c.u = short(-0.1687 * r - 0.3313 * g + 0.5 * b + 0.5) + 128;
	c.v = short(0.5 * r - 0.4187 * g - 0.0813 * b + 0.5) + 128;
}

int
yuv_to_rgb(color& c)
{
	double y = c.y;
	double u = c.u - 128.;
	double v = c.v - 128.;

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
	c.r = r;
	CHECK(g)
	c.g = g;
	CHECK(b)
	c.b = b;

	return (valid);
}

/*
 * The following sort routines implement a counting sort which is a linear
 * time sort in the case that the values to be sorted are bounded and
 * is therefore siginificantly faster than qsort.
 * For further details see "Introduction to Algorithms" by Carmen,
 * Lieserson and Rivest.
 */
//extern void *std::memset(void *, int, size_t); //SV-XXX: FreeBSD
//SV-XXX: FreeBSD: replaced all occurences of memset with std::memset() further down

/* XXX should pass backup array in as argument, that way can create it
   only once. */
#define COUNTING_SORT(rtn, fld) \
void \
rtn(histItem** h, int idx, int n) \
{ \
	int aux[256]; \
 \
	register histItem** hist = &h[idx]; \
	int j; \
	std::memset((void *)aux, 0, 256 * sizeof(int)); \
	for (j = 0; j < n; j++) \
		aux[hist[j]->c.fld]++; \
	 \
	for (j = 1; j < 256; j++) \
		aux[j] = aux[j] + aux[j - 1]; \
	 \
	register histItem** outhist = new histItem*[n]; \
	for (j = n - 1; j >= 0; j--) { \
		outhist[aux[hist[j]->c.fld] - 1] = hist[j]; \
		aux[hist[j]->c.fld]--; \
	} \
	 \
	for (j = 0; j < n; j++) \
		hist[j] = outhist[j]; \
	      \
	delete outhist; \
}

COUNTING_SORT(crsort, r)
COUNTING_SORT(cgsort, g)
COUNTING_SORT(cbsort, b)
COUNTING_SORT(cysort, y)
COUNTING_SORT(cusort, u)
COUNTING_SORT(cvsort, v)

int
sumcomp(const void* b1, const void* b2)
{
	return (((const box*)b2)->sum - ((const box*)b1)->sum);
}

/*
 * The median-cut colormap generator, derived from Jef Pokanzer's
 * ppmquant(1) utility in the ppm(5) package.  This algorithm
 * is based on Paul Heckbert's paper "Color Image Quantization
 * for Frame Buffer Display", SIGGRAPH '82 Proceedings, page 297.
 */
void
mediancut(u_char* cmap, histItem* hist[], int colors, 
	  int sum, int maxval, int newcolors)
{
	box* bv = new box[sizeof(box) * newcolors];

	register u_char* colormap = cmap;
	std::memset(colormap, 0, 3 * newcolors); //SV-XXX: FreeBSD

	/* Set up the initial box. */
	bv[0].ind = 0;
	bv[0].colors = colors;
	bv[0].sum = sum;
	int boxes = 1;
	/* Main loop: split boxes until we have enough. */
	while (boxes < newcolors) {
		/* Find the first splittable box. */
		int bi;
		for (bi = 0; bi < boxes; ++bi)
			if (bv[bi].colors >= 2)
				break;
		if (bi == boxes)
			break;	/* ran out of colors! */

		register int indx = bv[bi].ind;
		register int clrs = bv[bi].colors;
		register int sm = bv[bi].sum;

		/*
		 * Go through the box finding the minimum and maximum of each
		 * component - the boundaries of the box.
		 */
		register int minr, maxr, ming, maxg, minb, maxb;
		minr = maxr = hist[indx]->c.r;
		ming = maxg = hist[indx]->c.g;
		minb = maxb = hist[indx]->c.b;
		int i;
		for (i = 1; i < clrs; ++i) {
			register int v = hist[indx + i]->c.r;
			if (v < minr) 
				minr = v;
			if (v > maxr) 
				maxr = v;
			v = hist[indx + i]->c.g;
			if (v < ming) 
				ming = v;
			if (v > maxg) 
				maxg = v;
			v = hist[indx + i]->c.b;
			if (v < minb) 
				minb = v;
			if (v > maxb) 
				maxb = v;
		}

		/*
		 * Find the largest dimension, and sort by that component.  
		 * I have included two methods for determining the "largest" 
		 * dimension; first by simply comparing the range in RGB 
		 * space, and second by transforming into luminosities before 
		 * the comparison. 
		 */
/* #define LARGE_LUM */
#ifdef LARGE_NORM
		if (maxr - minr >= maxg - ming && maxr - minr >= maxb - minb)
			crsort(hist, indx, clrs);
		else if (maxg - ming >= maxb - minb)
			cgsort(hist, indx, clrs);
		else
			cbsort(hist, indx, clrs);
#endif /*LARGE_NORM*/
#ifdef LARGE_LUM
	{
		/*XXX Elan, these should just be multipliers.*/

		color c;
		c.r = maxr - minr;
		c.g = c.b = 0;
		rgb_to_yuv(c);
		double rl = c.y;
		
		c.g = maxg - ming;
		c.r = 0;
		rgb_to_yuv(c);
		double gl = c.y;

		c.b = maxb - minb;
		c.g = 0;
		rgb_to_yuv(c);
		double bl = c.y;
		
		if (rl >= gl && rl >= bl)
			crsort(hist, indx, clrs);
		else if (gl >= bl)
			cgsort(hist, indx, clrs);
		else
			cbsort(hist, indx, clrs);
	}
#endif /*LARGE_LUM*/
	
		/*
		 * Now find the median based on the counts, so that about 
		 * half the pixels (not colors -- *pixels*) are in each 
		 * subdivision.
		 */
		int lowersum = hist[indx]->value;
		int halfsum = sm >> 1;
		for (i = 1; i < clrs - 1; ++i) {
			if (lowersum >= halfsum)
				break;
			lowersum += hist[indx + i]->value;
		}

		/*
		 * Split the box, and sort to bring the biggest boxes to the 
		 * top.
		 */
		bv[bi].colors = i;
		bv[bi].sum = lowersum;
		bv[boxes].ind = indx + i;
		bv[boxes].colors = clrs - i;
		bv[boxes].sum = sm - lowersum;
		++boxes;
		qsort((char*) bv, boxes, sizeof(box), sumcomp);
	}

	/*
	 * Ok, we've got enough boxes.  Now choose a representative color for
	 * each box.  There are a number of possible ways to make this choice.
	 * One would be to choose the center of the box; this ignores any 
	 * structure within the boxes.  Another method would be to average 
	 * all the colors in the box - this is the method specified in 
	 * Heckbert's paper.  A third method is to average all the pixels in 
	 * the box.  
	 */
	u_char *cp = colormap;
	for (int bi = 0; bi < boxes; ++bi) {
#define REP_AVERAGE_COLORS
#ifdef REP_CENTER_BOX
		register int indx = bv[bi].ind;
		register int clrs = bv[bi].colors;

		register int minr, maxr, ming, maxg, minb, maxb;
		minr = maxr = hist[indx]->c.r;
		ming = maxg = hist[indx]->c.g;
		minb = maxb = hist[indx]->c.b;
		for (int i = 1; i < clrs; ++i) {
			int v = hist[indx + i]->c.r;
			minr = min(minr, v);
			maxr = max(maxr, v);
			v = hist[indx + i]->c.g;
			ming = min(ming, v);
			maxg = max(maxg, v);
			v = hist[indx + i]->c.b;
			minb = min(minb, v);
			maxb = max(maxb, v);
		}
		*cp++ = (minr + maxr) >> 1;
		*cp++ = (ming + maxg) >> 1;
		*cp++ = (minb + maxb) >> 1;
#endif /*REP_CENTER_BOX*/

#ifdef REP_AVERAGE_COLORS
		register int indx = bv[bi].ind;
		register int clrs = bv[bi].colors;
		register u_long r = 0, g = 0, b = 0;
		
		for (int i = 0; i < clrs; ++i) {
			r += hist[indx + i]->c.r;
			g += hist[indx + i]->c.g;
			b += hist[indx + i]->c.b;
		}
		r /= clrs;
		g /= clrs;
		b /= clrs;
		*cp++ = (u_char)r;
		*cp++ = (u_char)g;
		*cp++ = (u_char)b;
#endif /*REP_AVERAGE_COLORS*/

#ifdef REP_AVERAGE_PIXELS
		register int indx = bv[bi].ind;
		register int clrs = bv[bi].colors;
		register u_long r = 0, g = 0, b = 0, sum = 0;
		
		for (int i = 0; i < clrs; ++i) {
			r += hist[indx + i]->c.r * hist[indx + i]->value;
			g += hist[indx + i]->c.g * hist[indx + i]->value;
			b += hist[indx + i]->c.b * hist[indx + i]->value;
			sum += hist[indx + i]->value;
		}
		r /= sum;
		if (r > maxval) 
			r = maxval;	/* avoid math errors */
		g /= sum;
		if (g > maxval) 
			g = maxval;
		b /= sum;
		if (b > maxval) 
			b = maxval;
		*cp++ = r;
		*cp++ = g;
		*cp++ = b;
#else
/*  UNUSED*/
maxval=maxval;
#endif /*REP_AVERAGE_PIXELS*/
	}
}

int
gen_cmap(u_int* hist, u_char* cmap, int desired_colors)
{
	histItem table[65536];
	histItem* p = table;
	
	int i = 0;
	for (int v = 0; v < 1 << 5; ++v) {
		for (int u = 0; u < 1 << 5; ++u) {
			for (int y = 0; y < 1 << 6; ++y) {
				if (hist[i] != 0) {
					p->value = hist[i];
					p->c.y = y << 2;
					p->c.u = u << 3;
					p->c.v = v << 3;
					yuv_to_rgb(p->c);
					++p;
				}
				++i;
			}
		}
	}
	int ncolors = p - table;
	if (ncolors == 0)
		return (-1);
	

	/* Arrange all the hash table entries into a histogram. */
	
	histItem** histogram = new histItem*[ncolors];
	for (i = 0; i < ncolors; ++i)
		histogram[i] = &table[i];

	mediancut(cmap, histogram, ncolors,
		  /*XXX*/320*240, 255, desired_colors);

	delete histogram;

	return (0);
}

void
yuv_mediancut(u_char* cmap, histItem* hist[], int colors, 
	      int sum, int maxval, int newcolors)
{
	box* bv = new box[sizeof(box) * newcolors];

	register u_char* colormap = cmap;
	std::memset(colormap, 0, 3 * newcolors); //SV-XXX: FreeBSD

	/* Set up the initial box. */
	bv[0].ind = 0;
	bv[0].colors = colors;
	bv[0].sum = sum;
	int boxes = 1;
	/* Main loop: split boxes until we have enough. */
	while (boxes < newcolors) {
		/* Find the first splittable box. */
		int bi;
		for (bi = 0; bi < boxes; ++bi)
			if (bv[bi].colors >= 2)
				break;
		if (bi == boxes)
			break;	/* ran out of colors! */

		register int indx = bv[bi].ind;
		register int clrs = bv[bi].colors;
		register int sm = bv[bi].sum;

		/*
		 * Go through the box finding the minimum and maximum of each
		 * component - the boundaries of the box.
		 */
		register int miny, maxy, minu, maxu, minv, maxv;
		miny = maxy = hist[indx]->c.y;
		minu = maxu = hist[indx]->c.u;
		minv = maxv = hist[indx]->c.v;
		int i;
		for (i = 1; i < clrs; ++i) {
			register int t = hist[indx + i]->c.y;
			if (t < miny) 
				miny = t;
			if (t > maxy) 
				maxy = t;
			t = hist[indx + i]->c.u;
			if (t < minu) 
				minu = t;
			if (t > maxu) 
				maxu = t;
			t = hist[indx + i]->c.v;
			if (t < minv) 
				minv = t;
			if (t > maxv) 
				maxv = t;
		}

		/*
		 * Find the largest dimension, and sort by that component.  
		 */
		if (maxy - miny >= maxu - minu && maxy - miny >= maxv - minv)
			cysort(hist, indx, clrs);
		else if (maxu - minu >= maxv - minv)
			cusort(hist, indx, clrs);
		else
			cvsort(hist, indx, clrs);

		/*
		 * Now find the median based on the counts, so that about 
		 * half the pixels (not colors -- *pixels*) are in each 
		 * subdivision.
		 */
		int lowersum = hist[indx]->value;
		int halfsum = sm >> 1;
		for (i = 1; i < clrs - 1; ++i) {
			if (lowersum >= halfsum)
				break;
			lowersum += hist[indx + i]->value;
		}

		/*
		 * Split the box, and sort to bring the biggest boxes to the 
		 * top.
		 */
		bv[bi].colors = i;
		bv[bi].sum = lowersum;
		bv[boxes].ind = indx + i;
		bv[boxes].colors = clrs - i;
		bv[boxes].sum = sm - lowersum;
		++boxes;
		qsort((char*) bv, boxes, sizeof(box), sumcomp);
	}

	/*
	 * Ok, we've got enough boxes.  Now choose a representative color for
	 * each box.  There are a number of possible ways to make this choice.
	 * One would be to choose the center of the box; this ignores any 
	 * structure within the boxes.  Another method would be to average 
	 * all the colors in the box - this is the method specified in 
	 * Heckbert's paper.  A third method is to average all the pixels in 
	 * the box.  
	 */
	u_char *cp = colormap;
	for (int bi = 0; bi < boxes; ++bi) {
#ifdef REP_CENTER_BOX
		register int indx = bv[bi].ind;
		register int clrs = bv[bi].colors;

		register int miny, maxy, minu, maxu, minv, maxv;
		miny = maxy = hist[indx]->c.y;
		minu = maxu = hist[indx]->c.u;
		minv = maxv = hist[indx]->c.v;
		for (int i = 1; i < clrs; ++i) {
			int t = hist[indx + i]->c.y;
			miny = min(miny, t);
			maxy = max(maxy, t);
			t = hist[indx + i]->c.u;
			minu = min(minu, t);
			maxu = max(maxu, t);
			t = hist[indx + i]->c.v;
			minv = min(minv, t);
			maxv = max(maxv, t);
		}
		color c;
		c.y = (miny + maxy) >> 1;
		c.u = (minu + maxu) >> 1;
		c.v = (minv + maxv) >> 1;
		yuv_to_rgb(c);
		*cp++ = c.r;
		*cp++ = c.g;
		*cp++ = c.b;
#endif /*REP_CENTER_BOX*/

#ifdef REP_AVERAGE_COLORS
		register int indx = bv[bi].ind;
		register int clrs = bv[bi].colors;
		register u_long y = 0, u = 0, v = 0;
		
		for (int i = 0; i < clrs; ++i) {
			y += hist[indx + i]->c.y;
			u += hist[indx + i]->c.u;
			v += hist[indx + i]->c.v;
		}
		color c;
		c.y = y / clrs;
		c.y &=~ 0xf;/*XXX*/
		c.u = u / clrs;
		c.v = v / clrs;
		yuv_to_rgb(c);
		*cp++ = c.r;
		*cp++ = c.g;
		*cp++ = c.b;
#endif /*REP_AVERAGE_COLORS*/

#ifdef REP_AVERAGE_PIXELS
		register int indx = bv[bi].ind;
		register int clrs = bv[bi].colors;
		register u_long y = 0, u = 0, v = 0, sum = 0;
		
		for (int i = 0; i < clrs; ++i) {
			y += hist[indx + i]->c.y * hist[indx + i]->value;
			u += hist[indx + i]->c.u * hist[indx + i]->value;
			v += hist[indx + i]->c.v * hist[indx + i]->value;
			sum += hist[indx + i]->value;
		}
		y /= sum;
		if (y > maxval) 
			y = maxval;	/* avoid math errors */
		u /= sum;
		if (u > maxval) 
			u = maxval;
		v /= sum;
		if (v > maxval) 
			v = maxval;
		color c;
		c.y = y;
		c.u = u;
		c.v = v;
		yuv_to_rgb(c);
		*cp++ = c.r;
		*cp++ = c.g;
		*cp++ = c.b;
#else
/*  UNUSED*/
maxval=maxval;
#endif /*REP_AVERAGE_PIXELS*/
	}
}

#define MAX_YLEVELS 16
int
gen_ed_cmap(u_int* hist, u_char* cmap, int ncolors, int ybits)
{
	int ny = 1 << ybits;
	if (ny > MAX_YLEVELS)
		abort();

	int ncol[MAX_YLEVELS];
	std::memset(ncol, 0, sizeof(ncol)); //SV-XXX: FreeBSD
	/* 
	 * We do this in two passes so that the size of the table can be 
	 * calculated.  Otherwise it would be have to be too big:
	 * 1M * sizeof(histItem).
	 */
	int v, i = 0;
	for (v = 0; v < 1 << 5; ++v)
		for (int u = 0; u < 1 << 5; ++u)
			for (int y = 0; y < 1 << 6; ++y)
				if (hist[i++] != 0)
					ncol[y >> 2]++;

	histItem* table[MAX_YLEVELS];
	for (i = 0; i < ny; i++)
		table[i] = new histItem[ncol[i]];

	i = 0;
	std::memset(ncol, 0, sizeof(ncol)); //SV-XXX: FreeBSD
	for (v = 0; v < 1 << 5; ++v) {
		for (int u = 0; u < 1 << 5; ++u) {
			for (int y = 0; y < 1 << 4; ++y) {
				if (hist[i] != 0 || hist[i+1] != 0 ||
				    hist[i + 2] != 0 || hist[i + 3] != 0) {
					histItem* p = 
						&table[y][ncol[y]];
					p->value = hist[i];
					p->value += hist[i + 1];
					p->value += hist[i + 2];
					p->value += hist[i + 3];

					if (i >= 4) {
						p->value += hist[i - 1];
						p->value += hist[i - 2];
						p->value += hist[i - 3];
						p->value += hist[i - 4];
					}
					p->c.y = y << 4;
					p->c.u = u << 3;
					p->c.v = v << 3;	
					yuv_to_rgb(p->c);	
					ncol[y]++;
				}
				i += 4;
			}
		}
	}

	histItem** histogram[MAX_YLEVELS];
	std::memset(histogram, 0, sizeof(histogram)); //SV-XXX: FreeBSD
	int totcol = 0;
	for (i = 0; i < ny; ++i) {
		if (ncol[i] != 0) {
			histogram[i] = new histItem*[ncol[i]];
			totcol += ncol[i];
			for (int j = 0; j < ncol[i]; j++)
				histogram[i][j] = &table[i][j];
		} else
			histogram[i] = 0;
			
	}
	if (totcol == 0)
		return (-1);

	/* XXX */
	double desired_percent[MAX_YLEVELS] = {0.01, 0.03, 0.03, 0.03,
					       0.05, 0.05, 0.1, 0.2,
					       0.2, 0.1, 0.05, 0.05,
					       0.03, 0.03, 0.03, 0.01,};
	int desired_colors[MAX_YLEVELS];
	for (i = 0; i < ny; i++)
		desired_colors[i] = (int)(desired_percent[i] * ncolors + 0.5);

	int ndc = 0;
	for(i = 0; i < ny; i++) {
		if (ncol[i] == 0)
			continue;
		int dc = desired_colors[i];
		if (dc > ncol[i]) {
			yuv_mediancut(&cmap[3*ndc], histogram[i], ncol[i],
				  /*XXX*/320*240, 255, ncol[i]);
			ndc += ncol[i];
		} else {
			yuv_mediancut(&cmap[3*ndc], histogram[i], ncol[i],
				  /*XXX*/320*240, 255, dc);
			ndc += dc;
		}
	}

	for (i = 0; i < ny; ++i)
		delete histogram[i];

	return (ndc);
}

inline double
yuvdist(color& c, color& nc)
{
	double d = c.y - nc.y;
	d *= d;
	double t = c.u - nc.u;
	t *= t;
	d += t;
	t = c.v - nc.v;
	t *= t;
	d += t;

	return (d);
}

int
closest(color& c, color* colors, int ncolor)
{
	double best = yuvdist(c, colors[0]);
	int winner = 0;
	for (int i = 1; i < ncolor; i++) {
		double d = yuvdist(c, colors[i]);
		if (d < best) {
			best = d;
			winner = i;
		}
	}
	return (winner);
}

void
cmap_to_color(const u_char* cmap, color* colors, int n)
{
	for (int i = 0; i < n; ++i) {
		colors[i].r = cmap[0];
		colors[i].g = cmap[1];
		colors[i].b = cmap[2];
		rgb_to_yuv(colors[i]);
		cmap += 3;
	}
}

void
gen_ed_lut(u_char* lut, const u_char* cmap, int n)
{
	color colors[256];
	cmap_to_color(cmap, colors, n);
	int i = 0;
	for (u_int v = 0; v < 1 << 6; v++) {
		color c;
		c.v = v << 2;
		for (u_int u = 0; u < 1 << 6; u++) {
			c.u = u << 2;
			for (u_int y = 0; y < 1 << 4; y++) { 
				c.y = y << 4;
				lut[i++] = closest(c, colors, n);
			}
		}
	}
}

void
gen_lut(u_char* lut, const u_char* cmap, int n)
{
	color colors[256];
	cmap_to_color(cmap, colors, n);
	int i = 0;
	for (u_int u = 0; u < 1 << 5; u++) {
		color c;
		c.u = u << 3;
		for (u_int v = 0; v < 1 << 5; v++) {
			c.v = v << 3;
			for (u_int y = 0; y < 1 << 6; y++) { 
				c.y = y << 2;
				lut[i++] = closest(c, colors, n);
			}
		}
	}
}

void
usage()
{
	fprintf(stderr,
"usage: histtolut [ -e ] [ -n ncolors ] [ -o outfile ] histfile\n");
	exit(1);
}

void
Perror(const char* s)
{
	fprintf(stderr, "histtolut: ");
	perror(s);
	exit(1);
}

void
readhist(const char* file, u_int* hist)
{
	int fd = open(file, O_RDONLY);
	if (fd < 0)
		Perror(file);

	struct stat st;
	if (fstat(fd, &st) < 0)
		Perror("fstat");

	int cc = sizeof(*hist) * 64*1024;
	if (st.st_size != cc) {
		fprintf(stderr, "histtolut: bogus histogram file\n");
		exit(1);
	}
	(void)read(fd, (char*)hist, cc);
	close(fd);
}

int
main(int argc, char** argv)
{
	int eflag = 0;
	int ncolors = 128;
	int Hflag = 0;
	int ybits = 4;
	const char* outfile = 0;

	extern char *optarg;
	extern int optind;
	int op;
	while ((op = getopt(argc, argv, "eHn:o:Y:")) != -1) {
		switch (op) {
		case 'e':
			++eflag;
			break;

		case 'H':
			++Hflag;
			break;

		case 'n':
			ncolors = atoi(optarg);
			if (ncolors > 255)
				ncolors = 255;
			else if (ncolors < 2)
				ncolors = 2;
			break;

		case 'o':
			outfile = optarg;
			break;

		case 'Y':
			ybits = atoi(optarg);
			break;

		default:
			usage();
		}
	}
	if (argc == 1 || argc - optind > 1)
		usage();

	const char* infile = argv[optind];
	u_int hist[65536];
	u_char cmap[3*256];
	readhist(infile, hist);
	/*
	 * try to open output file before we do all the work.
	 */
	int out;
	if (outfile != 0) {
		out = open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0644);
		if (out < 0)
			Perror(outfile);
	} else
		/* stdout */
		out = 1;

	if (Hflag && ncolors + convex_hull_ncolor > 255) {
		fprintf(stderr, "histtolut: too many colors for -H\n");
		exit(1);
	}
	if (eflag) {
		/* XXX why does ncolors change?! */
		ncolors = gen_ed_cmap(hist, cmap, ncolors, ybits);
		if (ncolors < 0) {
			close(out);
			unlink(outfile);
			exit(1);
		}
	} else if (gen_cmap(hist, cmap, ncolors) < 0) {
		close(out);
		unlink(outfile);
		exit(1);
	}
	if (Hflag) {
		ncolors += convex_hull_ncolor;
		u_char *cp = &cmap[3*ncolors];
		u_char *hp = convex_hull;
		for (int i = 3 * convex_hull_ncolor; --i >= 0; )
			*cp++ = *hp++;
	}
	u_char c = (u_char)ncolors;
	(void)write(out, &c, 1);
	(void)write(out, (char*)cmap, 3*ncolors);
	u_char lut[65536];
	if (eflag)
		gen_ed_lut(lut, cmap, ncolors);
	else
		gen_lut(lut, cmap, ncolors);

	(void)write(out, (char*)lut, sizeof(lut));
	close(out);
	close(1);
	exit(0);
}
