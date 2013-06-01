/*
 * Copyright (c) 1991-1994 The Regents of the University of California.
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

#ifndef vic_inet_h
#define vic_inet_h


#include "config.h"

#include <sys/types.h>

#if defined(__osf__)
#include <machine/endian.h>
#endif

#if defined(__linux__)
#include <endian.h>
#endif

#ifndef IPPROTO_IP
#if defined(__ultrix__) && defined(__cplusplus)
extern "C" {
#include <netinet/in.h>
}
#else
#include <netinet/in.h>
#endif
#endif

#ifndef WIN32
#include <arpa/inet.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif
extern char *LookupHostName(u_int32_t addr);
extern char *InetNtoa(u_int32_t addr);
extern u_int32_t LookupLocalAddr(void);
extern u_int32_t LookupHostAddr(const char* host);
extern const char* intoa(u_int32_t addr);
#ifdef __cplusplus
}
#endif

#ifndef NTOHL
#if BYTE_ORDER==LITTLE_ENDIAN
#define NTOHL(d) ((d) = ntohl((d)))
#define NTOHS(d) ((d) = ntohs((d)))
#define HTONL(d) ((d) = htonl((d)))
#define HTONS(d) ((d) = htons((d)))
#elif BYTE_ORDER==BIG_ENDIAN
#define NTOHL(d)
#define NTOHS(d)
#define HTONL(d)
#define HTONS(d)
#else
#error error - BYTE_ORDER not defined.
#endif
#endif

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK (u_int32_t)0x7F000001
#endif

#if defined(WIN32)

/* XXX winsock.h should have these !! */
/* winsock2.h or ws2def.h does have these */
#if !defined(_WINSOCK2_H) && !defined(_WS2DEF_)

#define	IN_CLASSD(i)		(((u_long)(i) & ((u_long)0xf0000000)) == \
				  ((u_long)0xe0000000))
#define	IN_CLASSD_NET		((u_long)0xf0000000)/* These aren't really    */
#define	IN_CLASSD_NSHIFT	28		    /* net and host fields,but*/
#define	IN_CLASSD_HOST		((u_long)0x0fffffff)/* routing needn't know.  */
#endif

#define	IN_MULTICAST(i)		IN_CLASSD(i)
struct msghdr {
	caddr_t	msg_name;		/* optional address */
	u_int	msg_namelen;		/* size of address */
	struct	iovec *msg_iov;		/* scatter/gather array */
	u_int	msg_iovlen;		/* # elements in msg_iov */
	caddr_t	msg_control;		/* ancillary data, see below */
	u_int	msg_controllen;		/* ancillary data buffer len */
	int	msg_flags;		/* flags on received message */
};
#endif

#endif

