/* 
 * rtpdemo: A simple rtp application that sends and receives data.
 *
 * (C) 2000-2001 University College London.
 */

#include <sys/time.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "uclconf.h"
#include "debug.h"
#include "memory.h"
#include "rtp.h"

static void 
usage() 
{
	printf("Usage: rtpdemo [switches] address port\n");
	printf("Valid switches are:\n");
	printf("  -f\t\tFilter local packets out of receive stream.");
	printf("  -l\t\tListen and do not transmit data.\n");
	exit(-1);
}

/* ------------------------------------------------------------------------- */
/* RTP callback related */

static void
sdes_print(struct rtp *session, uint32_t ssrc, rtcp_sdes_type stype) {
	const char *sdes_type_names[] = {
		"end", "cname", "name", "email", "telephone", 
		"location", "tool", "note", "priv"
	};
	const uint8_t n = sizeof(sdes_type_names) / sizeof(sdes_type_names[0]);

	if (stype > n) {
		/* Theoretically impossible */
		printf("boo! invalud sdes field %d\n", stype);
		return;
	}
	
	printf("SSRC 0x%08x reported SDES type %s - ", ssrc, 
	       sdes_type_names[stype]);

	if (stype == RTCP_SDES_PRIV) {
		/* Requires extra-handling, not important for example */
		printf("don't know how to display.\n");
	} else {
		printf("%s\n", rtp_get_sdes(session, ssrc, stype));
	}
}

static void
packet_print(struct rtp *session, rtp_packet *p) 
{
	printf("Received data (payload %d timestamp %06d size %d) ", p->pt, p->ts, p->data_len);

	if (p->ssrc == rtp_my_ssrc(session)) {
		/* Unless filtering is enabled we are likely to see
		   out packets if sending to a multicast group. */
		printf("that I just sent.\n");
	} else {
		printf("from SSRC 0x%08x\n", p->ssrc); 
	} 
}

static void
rtp_event_handler(struct rtp *session, rtp_event *e) 
{
	rtp_packet	*p;
	rtcp_sdes_item	*r;

	switch(e->type) {
	case RX_RTP: 	
		p = (rtp_packet*)e->data;
		packet_print(session, p);
		xfree(p); /* xfree() is mandatory to release RTP packet data */
		break;
	case RX_SDES:
		r = (rtcp_sdes_item*)e->data;
		sdes_print(session, e->ssrc, r->type);
		break;
	case RX_BYE:
		break;
	case SOURCE_CREATED:
		printf("New source created, SSRC = 0x%08x\n", e->ssrc);
		break;
	case SOURCE_DELETED:
		printf("Source deleted, SSRC = 0x%08x\n", e->ssrc);
		break;
	case RX_SR:
	case RX_RR:
	case RX_RR_EMPTY:
	case RX_RTCP_START:
	case RX_RTCP_FINISH:
	case RR_TIMEOUT:
	case RX_APP:
	}
	fflush(stdout);
}

/* ------------------------------------------------------------------------- */
/* Send and receive loop.  Sender use 20ms audio mulaw packets */

#define MULAW_BYTES	4 * 160
#define MULAW_PAYLOAD	0
#define MULAW_MS	4 * 20

#define MAX_ROUNDS	1000

static void
rxtx_loop(struct rtp* session, int send_enable) 
{
	struct timeval	timeout;
	uint32_t	rtp_ts, round;
	uint8_t		mulaw_buffer[MULAW_BYTES];

	if (send_enable) {
		printf("Sending and listening to ");
	} else {
		printf("Listening to ");
	}
	printf("%s port %d (local SSRC = 0x%08x)\n", 
	       rtp_get_addr(session), 
	       rtp_get_rx_port(session),
	       rtp_my_ssrc(session));

	round = 0;

	for(round = 0; round < MAX_ROUNDS; round++) {
		rtp_ts = round * MULAW_MS;

		/* Send control packets */
		rtp_send_ctrl(session, rtp_ts, NULL);

		/* Send data packets */
		if (send_enable) {
			rtp_send_data(session, rtp_ts, MULAW_PAYLOAD, 
				      0, 0, 0,
				      (char*)mulaw_buffer, MULAW_BYTES, 
				      0, 0, 0);
		}

		/* Receive control and data packets */
		timeout.tv_sec  = 0;
		timeout.tv_usec = 0;
		rtp_recv(session, &timeout, rtp_ts);

		/* State maintenance */
		rtp_update(session);

		usleep(MULAW_MS * 1000);
		xmemchk();
	}
}

/* ------------------------------------------------------------------------- */
/* Main loop: parses command line and initializes RTP session */

int 
main(int argc, const char *argv[]) 
{
	const char	*address = NULL;
	struct rtp	*session = NULL;
	uint16_t	port = 0;
	int8_t		ac, filter_me = 0, send_enable = 1;

	ac = 1;
	while (argv[ac][0] == '-') {
		switch(tolower(argv[ac][1])) {
		case 'f':
			filter_me = 1;
			break;
		case 'l':
			send_enable = 0;
			break;
		}
		ac++;
	}

	if (argc - ac != 2) {
		usage();
	}

	address	= argv[ac];
	port	= atoi(argv[ac + 1]);

	session = rtp_init(address,		/* Host/Group IP address */ 
			   port,		/* receive port */
			   port,		/* transmit port */
			   16,			/* time-to-live */
			   64000,		/* B/W estimate */
			   rtp_event_handler,	/* RTP event callback */
			   NULL);		/* App. specific data */

	if (session) {
		const char 	*username  = "Malcovich Malcovitch";
		const char	*telephone = "1-800-RTP-DEMO";
		const char	*toolname  = "RTPdemo";

		uint32_t 	my_ssrc = rtp_my_ssrc(session);

		/* Set local participant info */
		rtp_set_sdes(session, my_ssrc, RTCP_SDES_NAME,
			     username, strlen(username));
		rtp_set_sdes(session, my_ssrc, RTCP_SDES_PHONE,
			     telephone, strlen(telephone));
		rtp_set_sdes(session, my_ssrc, RTCP_SDES_TOOL,
			     toolname, strlen(toolname));

		/* Filter out local packets if requested */
		rtp_set_option(session, RTP_OPT_FILTER_MY_PACKETS, filter_me);

		/* Run main loop */
		rxtx_loop(session, send_enable);

		/* Say bye-bye */
		rtp_send_bye(session);
		rtp_done(session);
	} else {
		printf("Could not initialize session for %s port %d\n",
		       address,
		       port);
	}

	return 0;
}
