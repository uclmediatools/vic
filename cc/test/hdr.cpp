/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "hdr.h"

struct rtcp_xr_hdr;

int build_hdr_test() {
	// packet
	u_char *pkt = (u_char *) malloc(sizeof(u_char *));
	rtcp_xr_hdr* hdr = (rtcp_xr_hdr *) pkt;

	// type specific block contents
	u_int32_t ssrc = 99;
	u_int16_t begin = 11;
	u_int16_t end = 256;
	u_int16_t chunk1 = 65535;
	u_int16_t chunk2 = 16;

	// number of type specific block contents
	int num_ssrc = 1;
	int num_begins = 1;
	int num_ends = 1;
	int num_chunks = 2;

	pkt = (u_char *) malloc(sizeof(hdr) + num_ssrc + 
			num_begins + num_ends + num_chunks * 2);

	hdr->xr_flags = 0x08;
	hdr->xr_len = 3;

	int pkt_len = sizeof (pkt);

	// test hdr length
	printf("xr_flags: %d xr_len: %d pkt_len: %d\n", 
			hdr->xr_flags, hdr->xr_len, pkt_len);

	u_int32_t *ptr = (u_int32_t *) (sizeof(hdr) + pkt);

	// test ssrc
	ptr[0] = ssrc;
	printf("ssrc: %d\n", ptr[0]);

	// test begin/end
	ptr[1] = 0;		// initialize
	ptr[1] |= begin;
	ptr[1] <<= 16;
	ptr[1] |= end;

	printf("begin: %d, end: %d\n", (ptr[1] >> 16), (ptr[1] & 0x0000FFFF));

	// chunks
	ptr[2] = 0;		// initialize
	ptr[2] |= chunk1;
	ptr[2] <<= 16;
	ptr[2] |= chunk2;

	printf("chunk 1: %d, chunk 2: %d\n", 
			(ptr[2] >> 16), (ptr[2] & 0x0000FFFF));
	
	return 0;
}
