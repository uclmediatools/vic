/* $Id$ */

#ifndef vic_cc_test_hdr_h
#define vic_cc_test_hdr_h

#include "config.h"

struct rtcp_xr_hdr {
	u_int16_t xr_flags;
	u_int16_t xr_len;
};

int build_hdr_test();

#endif /* vic_cc_test_hdr_h */
