/*
 * FILE:   rtp.c
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

#include "config_unix.h"
#include "config_win32.h"
#include "memory.h"
#include "debug.h"
#include "net_udp.h"
#include "crypt_random.h"
#include "rtp.h"

/*
 * Definitions for the RTP/RTCP packets on the wire. These are mostly
 * copied from draft-ietf-avt-rtp-new-01
 */

#define RTP_MAX_PACKET_LEN 1500
#define RTP_SEQ_MOD        1<<16
#define RTP_MAX_SDES_LEN   256

#define RTCP_SR   200
#define RTCP_RR   201
#define RTCP_SDES 202
#define RTCP_BYE  203
#define RTCP_APP  204

typedef struct {
#ifndef DIFF_BYTE_ORDER
	unsigned short  version:2;	/* packet type            */
	unsigned short  p:1;		/* padding flag           */
	unsigned short  count:5;	/* varies by payload type */
	unsigned short  pt:8;		/* payload type           */
#else
	unsigned short  count:5;	/* varies by payload type */
	unsigned short  p:1;		/* padding flag           */
	unsigned short  version:2;	/* packet type            */
	unsigned short  pt:8;		/* payload type           */
#endif
	u_int16         length;		/* packet length          */
} rtcp_common;

typedef struct {
	rtcp_common   common;	
	union {
		struct {
			rtcp_sr		sr;
			rtcp_rr       	rr[1];		/* variable-length list */
		} sr;
		struct {
			u_int32         ssrc;		/* source this RTCP packet is coming from */
			rtcp_rr       	rr[1];		/* variable-length list */
		} rr;
		struct rtcp_sdes_t {
			u_int32		ssrc;
			rtcp_sdes_item 	item[1];	/* list of SDES */
		} sdes;
		struct {
			u_int32         src[1];		/* list of sources */
							/* can't express the trailing text... */
		} bye;
	} r;
} rtcp_t;

/*
 * The RTP database contains source-specific information needed 
 * to make it all work. 
 */

typedef struct _source {
	struct _source	*l_link;	/* The l_link and r_link are used to  */
	struct _source	*r_link;	/* construct the binary tree database */
	u_int32		 ssrc;
	char		*cname;
	char		*name;
	char		*email;
	char		*phone;
	char		*loc;
	char		*tool;
	char		*note;
	rtcp_sr		*sr;
} source;

/*
 * The "struct rtp" defines an RTP session.
 */

struct rtp {
	socket_udp	*rtp_socket;
	socket_udp	*rtcp_socket;
	u_int32		 my_ssrc;
	source		*db;
	int		 invalid_rtp;
	int		 invalid_rtcp;
	int		 ssrc_count;
	void (*callback)(struct rtp *session, rtp_event *event);
};

static source *get_source(struct rtp *session, u_int32 ssrc)
{
	source	*curr = session->db;

	while (curr != NULL) {
		if (curr->ssrc == ssrc) {
			return curr;
		} else if (curr->ssrc < ssrc) {
			curr = curr->l_link;
		} else if (curr->ssrc > ssrc) {
			curr = curr->r_link;
		}
	}
	return NULL;
}

static void create_source(struct rtp *session, u_int32 ssrc)
{
	/* Create a new source in the database, if that source doesn't exist already. */
	/* If the source is pre-existing, then we do nothing.                         */
	source	*new_source ;

	if (get_source(session, ssrc) != NULL) {
		return;
	}
	new_source = (source *) xmalloc(sizeof(source));
	if (new_source == NULL) {
		debug_msg("Unable to create source 0x%08x, insufficient memory\n", ssrc);
		return;
	}
	debug_msg("Created source 0x%08x\n", ssrc);
	new_source->l_link = NULL;
	new_source->r_link = NULL;
	new_source->ssrc   = ssrc;
	new_source->cname  = NULL;
	new_source->name   = NULL;
	new_source->email  = NULL;
	new_source->phone  = NULL;
	new_source->loc    = NULL;
	new_source->tool   = NULL;
	new_source->note   = NULL;
	new_source->sr     = NULL;
	/* Add the new_source to the database. We build up a binary tree of sources */
	/* indexed by ssrc.  This is likely to work, on average, since ssrc values  */
	/* are chosen at random, but has really bad worst case behaviour.           */
	session->ssrc_count++;
	if (session->db == NULL) {
		session->db = new_source;
		return;
	} else {
		source	*tmp = session->db;
		while (1) {
			if (tmp->ssrc < ssrc) {
				if (tmp->l_link != NULL) {
					tmp = tmp->l_link;
				} else {
					tmp->l_link = new_source;
					return;
				}
			} else {
				if (tmp->r_link != NULL) {
					tmp = tmp->r_link;
				} else {
					tmp->r_link = new_source;
					return;
				}
			}
		}
	}
}

struct rtp *rtp_init(char *addr, u_int16 port, int ttl, void (*callback)(struct rtp *session, rtp_event *e))
{
	struct rtp *session;

	assert(ttl >= 0 && ttl < 128);
	assert(port % 2 == 0);

	session = (struct rtp *) xmalloc(sizeof(struct rtp));
	session->rtp_socket   = udp_init(addr, port, ttl);
	session->rtcp_socket  = udp_init(addr, port+1, ttl);
	session->my_ssrc      = (u_int32) lbl_random();
	session->db           = NULL;
	session->callback     = callback;
	session->invalid_rtp  = 0;
	session->invalid_rtcp = 0;
	session->ssrc_count   = 1;
	return session;
}

u_int32 rtp_my_ssrc(struct rtp *session)
{
	assert(session != NULL);
	return session->my_ssrc;
}

static int validate_rtp(rtp_packet *packet, int len)
{
	/* This function checks the header info to make sure that the packet */
	/* is valid. We return TRUE if the packet is valid, FALSE otherwise. */
	/* See Appendix A.1 of the RTP specification.                        */

	/* We only accept RTPv2 packets... */
	if (packet->v != 2) {
		debug_msg("rtp_header_validation: v != 2\n");
		return FALSE;
	}
	/* Check for valid payload types..... 72-76 are RTCP payload type numbers, with */
	/* the high bit missing so we report that someone is running on the wrong port. */
	if (packet->pt >= 72 && packet->pt <= 76) {
		debug_msg("rtp_header_validation: payload-type invalid");
		if (packet->m) {
			debug_msg(" (RTCP packet on RTP port?)");
		}
		debug_msg("\n");
		return FALSE;
	}
	/* Check that the length of the packet is sensible... */
	if (len < (12 + (4 * packet->cc))) {
		debug_msg("rtp_header_validation: packet length is smaller than the header\n");
		return FALSE;
	}
	/* Check that the length of any header extension is sensible... */
        if (packet->x) {
		if (packet->extn_len > (len - 12 - (4 * packet->cc) - packet->p?packet->data[packet->data_len-1]:0)) {
			debug_msg("rtp_header_validation: extension header is too big!\n");
			return FALSE;
		}
        }
	/* Check that the amount of padding specified is sensible. */
	/* Note: have to include the size of any extension header! */
	if (packet->p) {
		int	payload_len = len - 12 - (packet->cc * 4) - (packet->extn_len * 4) - packet->extn_len>0?4:0;
                if (packet->data[packet->data_len-1] > payload_len) {
                        debug_msg("rtp_header_validation: padding greater than payload length\n");
                        return FALSE;
                }
                if (packet->data[packet->data_len-1] < 1) {
			debug_msg("rtp_header_validation: padding zero\n");
			return FALSE;
		}
        }
	return TRUE;
}

static void rtp_recv_data(struct rtp *session)
{
	/* This routine processes incoming RTP packets */
	rtp_packet	*packet = (rtp_packet *) xmalloc(RTP_MAX_PACKET_LEN);
	u_int8		*buffer = ((u_int8 *) packet) + RTP_PACKET_HEADER_SIZE;
	int		 buflen;
	rtp_event	 event;
	struct timeval	 event_ts;

	buflen = udp_recv(session->rtp_socket, buffer, RTP_MAX_PACKET_LEN - RTP_PACKET_HEADER_SIZE);
	if (buflen > 0) {
		/* Convert header fields to host byte order... */
		packet->seq      = ntohs(packet->seq);
		packet->ts       = ntohl(packet->ts);
		packet->ssrc     = ntohl(packet->ssrc);
		/* Setup internal pointers, etc... */
		if (packet->cc) {
			packet->csrc = (u_int32 *)buffer + 12;
		} else {
			packet->csrc = NULL;
		}
		if (packet->x) {
			packet->extn     = buffer + 12 + packet->cc;
			packet->extn_len = (buffer[12 + (packet->cc * 4) + 2] << 16) | buffer[12 + (packet->cc * 4) + 3];
		} else {
			packet->extn     = NULL;
			packet->extn_len = 0;
		}
		packet->data     = buffer + 12 + packet->cc + packet->extn_len;
		packet->data_len = buflen - packet->extn_len - packet->cc - 12;
		/* Check that this packet is valid, and comes from a known source...  */
		/* This implies that we discard RTP packets from a source until we've */
		/* received a valid RTCP packet from that source.                  */
		if (validate_rtp(packet, buflen) && (get_source(session, packet->ssrc) != NULL)) {
			gettimeofday(&event_ts, NULL);
			event.ssrc = packet->ssrc;
			event.type = RX_RTP;
			event.data = (void *) packet;	/* The callback function MUST free this! */
			event.ts   = &event_ts;
			session->callback(session, &event);
			return;
		}
		session->invalid_rtp++;
		debug_msg("Invalid RTP packet discarded\n");
	}
	xfree(packet);
}

static int validate_rtcp(u_int8 *packet, int len)
{
	/* Validity check for a compound RTCP packet. This function returns */
	/* TRUE if the packet is okay, FALSE if the validity check fails.   */
        /*                                                                  */
	/* The following checks can be applied to RTCP packets [RFC1889]:   */
        /* o RTP version field must equal 2.                                */
        /* o The payload type field of the first RTCP packet in a compound  */
        /*   packet must be equal to SR or RR.                              */
        /* o The padding bit (P) should be zero for the first packet of a   */
        /*   compound RTCP packet because only the last should possibly     */
        /*   need padding.                                                  */
        /* o The length fields of the individual RTCP packets must total to */
        /*   the overall length of the compound RTCP packet as received.    */

	rtcp_t	*pkt  = (rtcp_t *) packet;
	rtcp_t	*end  = (rtcp_t *) (((char *) pkt) + len);
	rtcp_t	*r    = pkt;
	int	 l    = 0;
	int	 last = 0;

	/* All RTCP packets must be compound packets (RFC1889, section 6.1) */
	if (((ntohs(pkt->common.length) + 1) * 4) == len) {
		debug_msg("Bogus RTCP packet: not a compound packet\n");
		return FALSE;
	}

	/* Check the RTCP version, payload type and padding of the first in  */
	/* the compund RTCP packet...                                        */
	if (pkt->common.version != 2) {
		debug_msg("Bogus RTCP packet: version number != 2 in the first sub-packet\n");
		return FALSE;
	}
	if (pkt->common.p != 0) {
		debug_msg("Bogus RTCP packet: padding bit is set on first packet in compound\n");
		return FALSE;
	}
	if ((pkt->common.pt != RTCP_SR) && (pkt->common.pt != RTCP_RR)) {
		debug_msg("Bogus RTCP packet: compund packet does not start with SR or RR\n");
		return FALSE;
	}

	/* Check all following parts of the compund RTCP packet. The RTP version */
	/* number must be 2, and the padding bit must be zero on all apart from  */
	/* the last packet.                                                      */
	do {
		if (r->common.version != 2) {
			debug_msg("Bogus RTCP packet: version number != 2\n");
			return FALSE;
		}
		if (last == 1) {
			debug_msg("Bogus RTCP packet: padding bit set before last in compound\n");
			return FALSE;
		}
		if (r->common.p == 1) last = 1;
		l += (ntohs(r->common.length) + 1) * 4;
		r  = (rtcp_t *) (((u_int32 *) r) + ntohs(r->common.length) + 1);
	} while (r < end);

	/* Check that the length of the packets matches the length of the UDP */
	/* packet in which they were received...                              */
	if ((r != end) || (l != len))  {
		debug_msg("Bogus RTCP packet: RTCP packet length does not match UDP packet\n");
		return FALSE;
	}

	return TRUE;
}

static void process_rtcp_sr(struct rtp *session, rtcp_t *packet, struct timeval *event_ts)
{
	u_int32		 ssrc;
	rtp_event	 event;
	rtcp_sr		*sr;
	source		*s;

	ssrc = ntohl(packet->r.sr.sr.ssrc);
	create_source(session, ssrc);
	s = get_source(session, ssrc);
	if (s == NULL) {
		debug_msg("Source 0x%08x invalid, skipping...\n", ssrc);
		return;
	}

	/* Process the SR... */
	sr = (rtcp_sr *) xmalloc(sizeof(rtcp_sr));
	sr->ssrc          = ntohl(packet->r.sr.sr.ssrc);
	sr->ntp_sec       = ntohl(packet->r.sr.sr.ntp_sec);
	sr->ntp_frac      = ntohl(packet->r.sr.sr.ntp_frac);
	sr->rtp_ts        = ntohl(packet->r.sr.sr.rtp_ts);
	sr->sender_pcount = ntohl(packet->r.sr.sr.sender_pcount);
	sr->sender_bcount = ntohl(packet->r.sr.sr.sender_bcount);

	event.ssrc = ntohl(packet->r.sr.sr.ssrc);
	event.type = RX_SR;
	event.data = (void *) sr;
	event.ts   = event_ts;
	session->callback(session, &event);

	/* Store the SR for later retrieval... */
	if (s->sr != NULL) {
		xfree(s->sr);
	}
	s->sr = sr;

	/* ...process RRs... */
}

static void process_rtcp_rr(struct rtp *session, rtcp_t *packet, struct timeval *event_ts)
{
	u_int32		 ssrc;
	rtp_event	 event;
	rtcp_rr		*rr;
	source		*s;
	int		 i;

	ssrc = ntohl(packet->r.rr.ssrc);
	create_source(session, ssrc);
	s = get_source(session, ssrc);
	if (s == NULL) {
		debug_msg("Source 0x%08x invalid, skipping...\n", ssrc);
		return;
	}

	for (i = 0; i < packet->common.count; i++) {
		rr = (rtcp_rr *) xmalloc(sizeof(rtcp_rr));
		rr->ssrc          = ntohl(packet->r.rr.rr[i].ssrc);
		rr->fract_lost    = packet->r.rr.rr[i].fract_lost;	/* Endian conversion handled in the */
		rr->total_lost    = packet->r.rr.rr[i].total_lost;	/* definition of the rtcp_rr type.  */
		rr->last_seq      = ntohl(packet->r.rr.rr[i].last_seq);
		rr->jitter        = ntohl(packet->r.rr.rr[i].jitter);
		rr->lsr           = ntohl(packet->r.rr.rr[i].lsr);
		rr->dlsr          = ntohl(packet->r.rr.rr[i].dlsr);

		event.ssrc = ssrc;
		event.type = RX_RR;
		event.data = (void *) rr;
		event.ts   = event_ts;
		session->callback(session, &event);
		
		/* Store the RR for later use... */
		xfree(rr);
	}
}

static void process_rtcp_sdes(struct rtp *session, rtcp_t *packet, struct timeval *event_ts)
{
	int 			count = packet->common.count;
	struct rtcp_sdes_t 	*sd   = &packet->r.sdes;
	rtcp_sdes_item 		*rsp; 
	rtcp_sdes_item		*rspn;
	rtcp_sdes_item 		*end  = (rtcp_sdes_item *) ((u_int32 *)packet + packet->common.length + 1);
	source 			*s;
	rtp_event		 event;

	while (--count >= 0) {
		rsp = &sd->item[0];
		if (rsp >= end) {
			break;
		}
		sd->ssrc = ntohl(sd->ssrc);
		create_source(session, sd->ssrc);
		s = get_source(session, sd->ssrc);
		if (s == NULL) {
			debug_msg("Can't get valid source entry for 0x%08x, skipping...\n", sd->ssrc);
		} else {
			for (; rsp->type; rsp = rspn ) {
				rspn = (rtcp_sdes_item *)((char*)rsp+rsp->length+2);
				if (rspn >= end) {
					rsp = rspn;
					break;
				}
				if (rtp_set_sdes(session, sd->ssrc, rsp->type, rsp->data, rsp->length)) {
					event.ssrc = sd->ssrc;
					event.type = RX_SDES;
					event.data = (void *) rsp;
					event.ts   = event_ts;
					session->callback(session, &event);
				} else {
					debug_msg("Invalid sdes item for source 0x%08x, skipping...\n", sd->ssrc);
				}
			}
		}
		sd = (struct rtcp_sdes_t *) ((u_int32 *)sd + (((char *)rsp - (char *)sd) >> 2)+1);
	}
	if (count >= 0) {
		debug_msg("Invalid RTCP SDES packet, some items ignored.\n");
	}
}

static void process_rtcp_bye(struct rtp *session, rtcp_t *packet, struct timeval *event_ts)
{
	UNUSED(session);
	UNUSED(packet);
	debug_msg("Got RTCP BYE\n");
}

static void rtp_recv_ctrl(struct rtp *session)
{
	/* This routine processes incoming RTCP packets */
	struct timeval	event_ts;
	u_int8		buffer[RTP_MAX_PACKET_LEN];
	int		buflen;

	buflen = udp_recv(session->rtcp_socket, buffer, RTP_MAX_PACKET_LEN);
	gettimeofday(&event_ts, NULL);
	if (buflen > 0) {
		if (validate_rtcp(buffer, buflen)) {
			rtcp_t	*packet  = (rtcp_t *) buffer;
			while (packet < (rtcp_t *) (buffer + buflen)) {
				switch (packet->common.pt) {
					case RTCP_SR:
						process_rtcp_sr(session, packet, &event_ts);
						break;
					case RTCP_RR:
						process_rtcp_rr(session, packet, &event_ts);
						break;
					case RTCP_SDES:
						process_rtcp_sdes(session, packet, &event_ts);
						break;
					case RTCP_BYE:
						process_rtcp_bye(session, packet, &event_ts);
						break;
					default: 
						debug_msg("RTCP packet with unknown type (%d) ignored.\n", packet->common.pt);
						break;
				}
				packet = (rtcp_t *) ((char *) packet + (4 * (ntohs(packet->common.length) + 1)));
			}
		} else {
			debug_msg("Invalid RTCP packet discarded\n");
			session->invalid_rtcp++;
		}
	}
}

void rtp_recv(struct rtp *session, struct timeval *timeout)
{
	udp_fd_zero();
	udp_fd_set(session->rtp_socket);
	udp_fd_set(session->rtcp_socket);
	if (udp_select(timeout) > 0) {
		if (udp_fd_isset(session->rtp_socket)) {
			rtp_recv_data(session);
		}
		if (udp_fd_isset(session->rtcp_socket)) {
			rtp_recv_ctrl(session);
		}
	}
}

int rtp_add_csrc(struct rtp *session, u_int32 csrc)
{
	UNUSED(session);
	UNUSED(csrc);
	return FALSE;
}

int rtp_set_sdes(struct rtp *session, u_int32 ssrc, u_int8 type, char *value, int length)
{
	source	*s = get_source(session, ssrc);
	char	*v;

	if (s == NULL) {
		debug_msg("Invalid source 0x%08x\n", ssrc);
		return FALSE;
	}

	v = (char *) xmalloc(length + 1);
	memset(v, '\0', length + 1);
	memcpy(v, value, length);

	switch (type) {
		case RTCP_SDES_CNAME: 
			if (s->cname) xfree(s->cname);
			s->cname = v; 
			break;
		case RTCP_SDES_NAME:
			if (s->name) xfree(s->name);
			s->name = v; 
			break;
		case RTCP_SDES_EMAIL:
			if (s->email) xfree(s->email);
			s->email = v; 
			break;
		case RTCP_SDES_PHONE:
			if (s->phone) xfree(s->phone);
			s->phone = v; 
			break;
		case RTCP_SDES_LOC:
			if (s->loc) xfree(s->loc);
			s->loc = v; 
			break;
		case RTCP_SDES_TOOL:
			if (s->tool) xfree(s->tool);
			s->tool = v; 
			break;
		case RTCP_SDES_NOTE:
			if (s->note) xfree(s->note);
			s->note = v; 
			break;
		default :
			debug_msg("Unknown SDES item (type=%d, value=%s)\n", type, v);
                        xfree(v);
			return FALSE;
	}
	return TRUE;
}

char *rtp_get_sdes(struct rtp *session, u_int32 ssrc, u_int8 type)
{
	source	*s = get_source(session, ssrc);

	if (s == NULL) {
		debug_msg("Invalid source 0x%08x\n", ssrc);
		return NULL;
	}

	switch (type) {
		case RTCP_SDES_CNAME: 
			return s->cname;
		case RTCP_SDES_NAME:
			return s->name;
		case RTCP_SDES_EMAIL:
			return s->email;
		case RTCP_SDES_PHONE:
			return s->phone;
		case RTCP_SDES_LOC:
			return s->loc;
		case RTCP_SDES_TOOL:
			return s->tool;
		case RTCP_SDES_NOTE:
			return s->note;
	}
	debug_msg("Unknown SDES item (type=%d)\n", type);
	return NULL;
}

rtcp_sr *rtp_get_sr(struct rtp *session, u_int32 ssrc)
{
	/* Return the last SR received from this ssrc */
	UNUSED(session);
	UNUSED(ssrc);
	return NULL;
}

int rtp_send_data(struct rtp *session, u_int32 ts, char pt, int m, int cc, u_int32 csrc[16], char *data, int data_len)
{
	UNUSED(session);
	   UNUSED(ts);
	UNUSED(pt);
	UNUSED(m);
	UNUSED(cc);
	UNUSED(csrc);
	UNUSED(data);
	UNUSED(data_len);
	return TRUE;
}

int rtp_send_ctrl(struct rtp *session, u_int32 ts)
{
	/* Expire sources which we haven't heard from for a while   ... */
	/* Send an RTCP packet, if one is due... */
	UNUSED(session);
	UNUSED(ts);
	return -1;
}

void rtp_send_bye(struct rtp *session)
{
	UNUSED(session);
}

int rtp_sent_bye(struct rtp *session)
{
	UNUSED(session);
	return TRUE;
}

void rtp_done(struct rtp *session)
{
	UNUSED(session);
}

