/*
 * inet6.h -- netdb functions for IPv6 addresses
 */

/*
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

#ifndef vic_inet6_h
#define vic_inet6_h

#ifdef HAVE_IPV6

#include <sys/types.h>
#ifdef WIN32
//#include <winsock.h>
#endif
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

//#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif
extern int inet6_LookupHostName(char* name, struct in6_addr *addr);
extern int inet6_InetNtoa(char *address, struct in6_addr *addr);
extern int inet6_LookupLocalAddr(struct in6_addr *addr);
extern int inet6_LookupHostAddr(struct in6_addr *, const char* host);
#ifdef __cplusplus
}
#endif

#endif /* HAVE_IPV6 */

#endif /* vic_inet6_h */
