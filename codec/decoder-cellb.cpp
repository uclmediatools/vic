/* This code was originally derived from a cellb reference decoder from
 * Sun Microsystems with the following copyright:
 *
 * Copyright (c) Sun Microsystems, Inc.  1992, 1993. All rights reserved. 
 *
 * License is granted to copy, to use, and to make and to use derivative 
 * works for research and evaluation purposes, provided that Sun Microsystems is
 * acknowledged in all documentation pertaining to any such copy or derivative
 * work. Sun Microsystems grants no other licenses expressed or implied. The
 * Sun Microsystems  trade name should not be used in any advertising without
 * its written permission.
 *
 * SUN MICROSYSTEMS MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF
 * THIS SOFTWARE FOR ANY PARTICULAR PURPOSE.  The software is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this software.
 */

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include <stdio.h>
#include <string.h>
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "bsd-endian.h"
#include "pktbuf.h"

class CellbDecoder : public PlaneDecoder {
public:
	CellbDecoder();
protected:
	virtual void recv(pktbuf*);
	void decode(const u_char*, int cc, int x, int y, int w, int h);
};

static class CellbDecoderMatcher : public Matcher {
public:
	CellbDecoderMatcher() : Matcher("decoder") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "cellb") == 0)
			return (new CellbDecoder());
		else
			return (0);
	}
} dm_cellb;

#define STAT_BAD_GEOM 0
#define STAT_BAD_COORD 1

CellbDecoder::CellbDecoder() : PlaneDecoder(sizeof(cellbhdr))
{
	stat_[STAT_BAD_GEOM].name = "Cellb-Bad-Geom";
	stat_[STAT_BAD_COORD].name = "Cellb-Bad-Coord";
	nstat_ = 2;

	color_ = 1;
	inw_ = 0;
	inh_ = 0;
}

/*
 * Inverse VQ tables.
 */
extern "C" u_short cellb_yytable[256], cellb_uvtable[256];

void CellbDecoder::decode(const u_char* p, int cc, int cellx, int celly,
			  int width, int height)
{
	/* Convert width to count in cells */
	width >>= 2;
	height >>= 2;

	while (cc >= 4) {
		int mask = *p;
		if (mask & 0x80) {
			cellx += mask - 127;
			while (cellx >= width) {
				cellx -= width;
				++celly;
			}
			++p;
			--cc;
		} else {
			mask <<= 8;
			mask |= p[1];

			/*XXX yytable is backward so flip mask bits*/
			mask ^= 0xffff;
			int yy = cellb_yytable[p[3]];

			if (celly >= height) {
				/*XXX*/
				fprintf(stderr, "vic: bad cellb packet\n");
				return;
			}
			int k = 4 * (celly * 4 * width + cellx);
			u_int* yp = (u_int*)&frm_[k];

#if BYTE_ORDER == LITTLE_ENDIAN
#define MSET(y, yy, mask, pos) \
			{ \
			  	int t = (((mask) >> (pos)) & 1) << 3; \
				y >>= 8; \
				y |= ((yy) >> t) << 24; \
		  	}
#else
#define MSET(y, yy, mask, pos) \
			{ \
			  	int t = (((mask) >> (pos)) & 1) << 3; \
				y <<= 8; \
				y |= ((yy) >> t) & 0xff; \
		  	}
#endif
			/* First bit of mask is clear. */
#if BYTE_ORDER == LITTLE_ENDIAN
			u_int y = (yy >> 8) << 24;
#else
			u_int y = yy >> 8;
#endif
			MSET(y, yy, mask, 14);
			MSET(y, yy, mask, 13);
			MSET(y, yy, mask, 12);
			*yp = y;
			yp += width;

			MSET(y, yy, mask, 11);
			MSET(y, yy, mask, 10);
			MSET(y, yy, mask, 9);
			MSET(y, yy, mask, 8);
			*yp = y;
			yp += width;

			MSET(y, yy, mask, 7);
			MSET(y, yy, mask, 6);
			MSET(y, yy, mask, 5);
			MSET(y, yy, mask, 4);
			*yp = y;
			yp += width;

			MSET(y, yy, mask, 3);
			MSET(y, yy, mask, 2);
			MSET(y, yy, mask, 1);
			MSET(y, yy, mask, 0);
			*yp = y;
#undef MSET
			int s = (width << 2) * (height << 2);/*XXX*/
			u_char* up = frm_ + s + k / 2;
			u_char* vp = up + (s >> 1);

			k = p[2];
			int uv = cellb_uvtable[k];
			int t = uv >> 8;
			t |= t << 8;
			*(u_short*)up = t; up += width << 1;
			*(u_short*)up = t; up += width << 1;
			*(u_short*)up = t; up += width << 1;
			*(u_short*)up = t;
			t = uv & 0xff;
			t |= t << 8;
			*(u_short*)vp = t; vp += width << 1;
			*(u_short*)vp = t; vp += width << 1;
			*(u_short*)vp = t; vp += width << 1;
			*(u_short*)vp = t;

			/* mark block as changed */
			/*XXX should maintain offset using loop */
			int o = (cellx >> 1) + (celly >> 1) * (width >> 1);
			rvts_[o] = now_;

			if (++cellx >= width) {
				cellx -= width;
				++celly;
			}
			p += 4;
			cc -= 4;
		}
		ndblk_++;
	}
}

void CellbDecoder::recv(pktbuf* pb)
{
	rtphdr* rh = (rtphdr*)pb->dp;
	cellbhdr* ph = (cellbhdr*)(rh + 1);
	int w = ntohs(ph->width) & 0x7fff;
	int h = ntohs(ph->height);
	if (w != inw_ || h != inh_) {
		if ((unsigned)w > 1000 || (unsigned)h > 1000) {
			/*XXX*/
			w = 320;
			h = 240;
			count(STAT_BAD_GEOM);
		}
		resize(w, h);
	}
	int cc = pb->len - (sizeof(*rh) + sizeof(*ph));
	decode((u_char*)(ph + 1), cc, ntohs(ph->x), ntohs(ph->y), w, h);
	if (ntohs(rh->rh_flags) & RTP_M) {
		render_frame(frm_);
		resetndblk();
	}
	pb->release();
}
