/*-
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

#ifndef vic_rtp_h
#define vic_rtp_h

#ifndef WIN32
#include <sys/param.h>
#endif
#include "sys-time.h"

#define RTP_PT_RAW		126	/* XXX */
#define RTP_PT_BVC		22	/* Berkeley video codec */

/* RTP standard content encodings for video */
#define RTP_PT_LDCT		20 	/* Layered DCT */
#define RTP_PT_PVH		21 	/* Steve McCanne's  */
#define RTP_PT_RGB8		23 	/* 8-bit dithered RGB */
#define RTP_PT_HDCC		24 	/* SGI proprietary */
#define RTP_PT_CELLB		25 	/* Sun CellB */
#define RTP_PT_JPEG		26	/* JPEG */
#define RTP_PT_CUSEEME		27	/* Cornell CU-SeeMe */
#define RTP_PT_NV		28	/* Xerox PARC nv */
#define RTP_PT_PICW		29	/* BB&N PictureWindow */
#define RTP_PT_CPV		30	/* Concept/Bolter/Viewpoint codec */
#define RTP_PT_H261		31	/* ITU H.261 */
#define RTP_PT_MPEG		32 	/* MPEG-I & MPEG-II */
#define RTP_PT_MP2T		33 	/* MPEG-II either audio or video */
#define RTP_PT_H263P	42	/* ITU H.263 */
#define RTP_PT_H263		34 	/* ITU H.263 */
#define RTP_PT_DV               123     /* DV */
#define RTP_PT_MPEG4		45
#define RTP_PT_H264		96	/* RFC3984 compliant H.264 */
#define RTP_PT_H264_IOCOM	107	/* IOCOM.com's IG2 proprietary H.264*/

/* backward compat hack for decoding RTPv1 ivs streams */
#define RTP_PT_H261_COMPAT 127

/* non-standard arbitrary sized h261 */
#define RTP_PT_H261AS           77

/* RTP standard content encodings for audio */
#define RTP_PT_PCMU		0
#define RTP_PT_CELP		1
#define RTP_PT_GSM		3
#define RTP_PT_DVI		5
#define RTP_PT_LPC		7


/* Offset from UNIX's epoch to the NTP epoch in seconds (NTP's JAN_1970) */
#define RTP_EPOCH_OFFSET	2208988800UL
#define RTP_VERSION 2

#define RTP_M	0x0080	/* Marker: significant event <e.g. frame boundary> */
#define RTP_P	0x2000	/* Padding is present */
#define RTP_X	0x1000	/* Extension Header is present */

/* Basic RTP header */
struct rtphdr {
	u_int16_t rh_flags;	/* T:2 P:1 X:1 CC:4 M:1 PT:7 */
	u_int16_t rh_seqno;	/* sequence number */
	u_int32_t rh_ts;	/* media-specific time stamp */
	u_int32_t rh_ssrc;	/* synchronization src id */
	u_int32_t frame_no;	/* frame_no */
	/* data sources follow per cc */
};

/* RTCP based header */
struct rtcphdr {
	u_int16_t rh_flags;	/* T:2 P:1 CNT:5 PT:8 */
	u_int16_t rh_len;	/* length of the message is the number of 
				   32-bit words in the extension, excluding the
				   four-octet extension header, therefore zero
				   is a valid length*/
	u_int32_t rh_ssrc;	/* synchronization src id of packet sender */
};

typedef struct {
	u_int32_t upper;	/* more significant 32 bits */
	u_int32_t lower;	/* less significant 32 bits */
} ntp64;

/*
 * Sender report.
 */
struct rtcp_sr {
	ntp64 sr_ntp;		/* 64-bit ntp timestamp */
	u_int32_t sr_ts;	/* reference media timestamp */
	u_int32_t sr_np;	/* no. packets sent */
	u_int32_t sr_nb;	/* no. bytes sent */
};

/*
 * Receiver report.
 * Time stamps are middle 32-bits of ntp timestamp.
 */
struct rtcp_rr {
	u_int32_t rr_srcid;	/* sender being reported */
	u_int32_t rr_loss;	/* loss stats (8:fraction, 24:cumulative)*/
	u_int32_t rr_ehsr;	/* ext. highest seqno received */
	u_int32_t rr_dv;	/* jitter (delay variance) */
	u_int32_t rr_lsr;	/* orig. ts from last rr from this src  */
	u_int32_t rr_dlsr;	/* time from recpt of last rr to xmit time */
};

/*
 * ECN for RTP - Feedback packet (AVPF [rfc4585])
 * http://tools.ietf.org/html/draft-ietf-avt-ecn-for-rtp
 *
 */
#define RTPFB_FMT_ECN	0x06	// ECN Feedback Packet - temp from rev-03

struct rtcp_fb_ecn {
	u_int32_t media_ssrc;    /* SSRC of media source */
	u_int32_t seq_n_loss;    /* loss stats (20:Ext Highest Seq No, 12:Lost pkts)*/
	u_int16_t not_ect;       /* Not ECT counter */
	u_int16_t ect0;          /* ECT(0) counter */
	u_int16_t ect1;          /* ECT(1) counter */
	u_int16_t ecn_ce;        /* CE Counter */
};

/*
 * RTCP Extended Report.
 * (RFC 3611)
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      BT       | type-specific |         block length          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * :             type-specific block contents                      :
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#define XR_BT_1		0x01	// Loss RLE Report Block
#define XR_BT_2		0x02	// Duplicate RLE Report Block
#define XR_BT_3		0x03	// Packet Receipt Times Report Block
#define XR_BT_4		0x04	// Receiver Reference Time Report Block
#define XR_BT_ECN	0x44	// ECN Report block - TEST

// extended report block header
struct rtcp_xr {
	u_int8_t BT;		/* Block Type */
	u_int8_t xr_flag;	/* xr_flag */
	u_int16_t xr_len;	/* XR report block length (in bytes)*/
};

// extended report block 1 header
struct rtcp_xr_BT_1_hdr {
	// Block Type
	u_int8_t BT;
	// type-specific flags
	struct {
		u_int8_t rsvd:4;	/* Reserved field */
		u_int8_t T:4;		/* Thinning flag */
	} xr_flags;
	// XR report block length (in bytes)
	u_int16_t xr_len;

	// type-specific block contents
	u_int32_t ssrc;/* ssrc of the RTP data pkt being reported upon by this */
	u_int16_t begin_seq;/* first seqno that this block report */
	u_int16_t end_seq;	/* last seqno that this block report plus 1 */
};

#define RTCP_PT_SR	200	/* sender report */
#define RTCP_PT_RR	201	/* receiver report */
#define RTCP_PT_SDES	202	/* source description */
#define 	RTCP_SDES_CNAME	1	/* official name (mandatory) */
#define 	RTCP_SDES_NAME	2	/* personal name (optional) */
#define 	RTCP_SDES_EMAIL	3	/* e-mail addr (optional) */
#define 	RTCP_SDES_PHONE	4	/* telephone # (optional) */
#define 	RTCP_SDES_LOC	5	/* geographical location */
#define 	RTCP_SDES_TOOL	6	/* name/(vers) of app */
#define 	RTCP_SDES_NOTE	7	/* transient messages */
#define 	RTCP_SDES_PRIV	8	/* private SDES extensions */
#define RTCP_PT_BYE	203	/* end of participation */
#define RTCP_PT_APP	204	/* application specific functions */
#define RTCP_PT_RTPFB	205	/* Transport layer FB message RFC4585 */
#define RTCP_PT_XR	207	/* extended report */

#define		RTCP_SDES_MIN	1
#define		RTCP_SDES_MAX	7

/*
 * Parameters controling the RTCP report rate timer.
 */
#define RTCP_SESSION_BW_FRACTION (0.05)
#define RTCP_MIN_RPT_TIME (5.)
#define RTCP_SENDER_BW_FRACTION (0.25)
#define RTCP_RECEIVER_BW_FRACTION (1. - RTCP_SENDER_BW_FRACTION)
#define RTCP_SIZE_GAIN (1./8.)

/*
 * Largest (user-level) packet size generated by our rtp applications.
 * Individual video formats may use smaller mtu's.
 */
#define RTP_MTU 1024

//now defined in pktbuf.h
//#define MAXHDR 24

/*
 * Motion JPEG encapsulation.
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      MBZ      |                frag offset                    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      Type     |       Q       |     Width     |     Height    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 * Type = Index into a table of predefined JPEG parameters
 * Width = Width of input in 8-pixel blocks
 * Height = Height of input in 8-pixel blocks
 * Q = Quality factor (0-100 = std, >100 = custom)
 * Frag offset = The byte offset into the frame for the data in 
 * this packet
 */
struct jpeghdr {
	u_int32_t off;		/* fragment offset */
	u_int8_t type;		/* id of jpeg decoder params */
	u_int8_t q;		/* quantization factor (or table id) */
	u_int8_t width;		/* 1/8 frame width */
	u_int8_t height;	/* 1/8 frame height */
};

/*
 * NV encapsulation.
 */
struct nvhdr {
	u_int16_t width;
	u_int16_t height;
	/* nv data */
};

/*
 * CellB encapsulation.
 */
struct cellbhdr {
	u_int16_t x;
	u_int16_t y;
	u_int16_t width;
	u_int16_t height;
	/* cells */
};

/*
 * H.261 encapsulation.
 * See Internet draft.
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |SBIT |EBIT |I|V| GOBN  |  MBAP   |  QUANT  |  HMVD   |  VMVD   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifdef notdef
struct h261hdr {
	u_int16_t	flags;
	u_int16_t off;
};
#endif

struct bvchdr {
	u_int8_t version;
	u_int8_t pad;
	u_int8_t width;
	u_int8_t height;
	u_int32_t quant;
	u_int8_t sbit;
	u_int8_t ebit;
	u_int16_t blkno;
};

#endif
