/*
 * FILE:   rtp.h
 * AUTHOR: Colin Perkins <c.perkins@cs.ucl.ac.uk>
 *
 * $Revision$ 
 * $Date$
 * 
 * Copyright (c) 1998-99 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London.
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __RTP_H__
#define __RTP_H__

#define RTP_VERSION 2
#define RTP_PACKET_HEADER_SIZE	((sizeof(char *) * 2) + sizeof(u_int32 *) + (2 * sizeof(int)))
#define RTP_MAX_PACKET_LEN 1500

typedef struct {
	/* The following are pointers to the data in the packet as    */
	/* it came off the wire. The packet it read in such that the  */
	/* header maps onto the latter part of this struct, and the   */
	/* fields in this first part of the struct point into it. The */
	/* entire packet can be freed by freeing this struct, without */
	/* having to free the csrc, data and extn blocks separately.  */
	/* WARNING: Don't change the size of the first portion of the */
	/* struct without changing RTP_PACKET_HEADER_SIZE to match.   */
	u_int32		*csrc;
	char		*data;
	int		 data_len;
	char		*extn;
	int		 extn_len;
	/* The following map directly onto the RTP packet header...   */
#ifndef DIFF_BYTE_ORDER
	unsigned short   v:2;		/* packet type                */
	unsigned short   p:1;		/* padding flag               */
	unsigned short   x:1;		/* header extension flag      */
	unsigned short   cc:4;		/* CSRC count                 */
	unsigned short   m:1;		/* marker bit                 */
	unsigned short   pt:7;		/* payload type               */
#else
	unsigned short   cc:4;		/* CSRC count                 */
	unsigned short   x:1;		/* header extension flag      */
	unsigned short   p:1;		/* padding flag               */
	unsigned short   v:2;		/* packet type                */
	unsigned short   pt:7;		/* payload type               */
	unsigned short   m:1;		/* marker bit                 */
#endif
	u_int16          seq;		/* sequence number            */
	u_int32          ts;		/* timestamp                  */
	u_int32          ssrc;		/* synchronization source     */
	/* The csrc list, header extension and data follow, but can't */
	/* be represented in the struct.                              */
} rtp_packet;

typedef struct {
	u_int32         ssrc;
	u_int32         ntp_sec;
	u_int32         ntp_frac;
	u_int32         rtp_ts;
	u_int32         sender_pcount;
	u_int32         sender_bcount;
} rtcp_sr;

typedef struct {
	u_int32		ssrc;		/* The ssrc to which this RR pertains */
#ifndef DIFF_BYTE_ORDER
	u_int32		fract_lost:8;
	u_int32		total_lost:24;
#else
	u_int32		total_lost:24;
	u_int32		fract_lost:8;
#endif	
	u_int32		last_seq;
	u_int32		jitter;
	u_int32		lsr;
	u_int32		dlsr;
} rtcp_rr;

typedef struct {
	u_int8          type;		/* type of SDES item              */
	u_int8          length;		/* length of SDES item (in bytes) */
	char            data[1];	/* text, not zero-terminated      */
} rtcp_sdes_item;

typedef struct {
#ifndef DIFF_BYTE_ORDER
	unsigned short  version:2;	/* RTP version            */
	unsigned short  p:1;		/* padding flag           */
	unsigned short  subtype:5;	/* application dependent  */
#else
	unsigned short  subtype:5;	/* application dependent  */
	unsigned short  p:1;		/* padding flag           */
	unsigned short  version:2;	/* RTP version            */
#endif
	unsigned short  pt:8;		/* packet type            */
	u_int16         length;		/* packet length          */
	u_int32         ssrc;
	char            name[4];        /* four ASCII characters  */
	char            data[1];        /* variable length field  */
} rtcp_app;

typedef struct {
	u_int32		 ssrc;
	int		 type;
	void		*data;
	struct timeval	*ts;
} rtp_event;

/* rtp_event type values... */
#define RX_RTP  	100
#define RX_SR		101
#define RX_RR   	102
#define RX_SDES 	103
#define RX_BYE  	104	/* Source is leaving the session, database entry is still valid */
#define SOURCE_DELETED	105	/* Source has been removed from the database                    */
#define SOURCE_CREATED	106
#define RX_RR_EMPTY	107	/* We've received an empty reception report block                                               */
#define RX_RTCP_START	108	/* We're about to start processing a compound RTCP packet. The SSRC is not valid in this event. */
#define RX_RTCP_FINISH	109	/* We've just finished processing a compound RTCP packet. The SSRC is not valid in this event.  */
#define RR_TIMEOUT	110
#define RX_APP  	111

/* RTP options */
#define RTP_OPT_PROMISC     	1
#define RTP_OPT_WEAK_VALIDATION	2

/* SDES packet types... */
#define RTCP_SDES_END   0
#define RTCP_SDES_CNAME 1
#define RTCP_SDES_NAME  2
#define RTCP_SDES_EMAIL 3
#define RTCP_SDES_PHONE 4
#define RTCP_SDES_LOC   5
#define RTCP_SDES_TOOL  6
#define RTCP_SDES_NOTE  7
#define RTCP_SDES_PRIV  8

struct rtp;

struct rtp	*rtp_init(char *addr, u_int16 rx_port, u_int16 tx_port, int ttl, double rtcp_bw, 
			  void (*callback)(struct rtp *session, rtp_event *e),
			  void *user_data);
void 		 rtp_setopt(struct rtp *session, int optname, int optval);
int 		 rtp_getopt(struct rtp *session, int optname);
void 		*rtp_get_userdata(struct rtp *session);
int 		 rtp_recv(struct rtp *session, struct timeval *timeout, u_int32 curr_time);
int 		 rtp_send_data(struct rtp *session, u_int32 ts, char pt, int m, int cc, u_int32 csrc[], 
                               char *data, int data_len, char *extn, int extn_len);
void 		 rtp_send_ctrl(struct rtp *session, u_int32 ts, 
			       rtcp_app *(*appcallback)(struct rtp *session, u_int32 ts, int max_size));
void 		 rtp_update(struct rtp *session);

u_int32		 rtp_my_ssrc(struct rtp *session);
int		 rtp_add_csrc(struct rtp *session, u_int32 csrc);
int		 rtp_valid_ssrc(struct rtp *session, u_int32 ssrc);
int		 rtp_set_sdes(struct rtp *session, u_int32 ssrc, u_int8 type, char *value, int length);
const char	*rtp_get_sdes(struct rtp *session, u_int32 ssrc, u_int8 type);
const rtcp_sr	*rtp_get_sr(struct rtp *session, u_int32 ssrc);
const rtcp_rr	*rtp_get_rr(struct rtp *session, u_int32 reporter, u_int32 reportee);
void		 rtp_send_bye(struct rtp *session);
void		 rtp_done(struct rtp *session);
int              rtp_set_encryption_key(struct rtp *session, const char *key);
const char      *rtp_get_encryption_key(struct rtp *session);

char 		*rtp_get_addr(struct rtp *session);
u_int16		 rtp_get_rx_port(struct rtp *session);
u_int16		 rtp_get_tx_port(struct rtp *session);
int		 rtp_get_ttl(struct rtp *session);

int              rtp_set_my_ssrc(struct rtp *session, u_int32 ssrc);

#endif /* __RTP_H__ */
