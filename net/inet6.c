/*
 * inet6.c -- netdb functions for IPv6 addresses
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

static const char rcsid[] =
    "@(#) $Header$";

#ifdef HAVE_IPV6

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
//#include <winsock2.h>
#else
#include <sys/param.h>
#include <netdb.h>
#include <sys/socket.h>
#endif
#include <stdio.h>

#include "config.h"

#include "inet6.h"

#ifdef NEED_INET_PTON
#include "inet_pton.h"
#endif

#ifdef NEED_INET_NTOP
#include "inet_ntop.h"
#endif

#ifdef NEED_ADDRINFO
#include "addrinfo.h"
#endif

int 
inet6_LookupHostAddr(struct in6_addr *addr, const char* hostname) {
  if (inet_pton(AF_INET6, hostname, addr->s6_addr) != 1) {
    struct addrinfo hints, *ai;
    int i;
    memset(&hints, 0, sizeof(struct addrinfo));
	
	hints.ai_protocol  = 0;
	//hints.ai_flags   = AI_CANONNAME;
	hints.ai_flags     = 0;
	hints.ai_family    = AF_INET6;
	hints.ai_socktype  = SOCK_DGRAM;
	hints.ai_protocol  = IPPROTO_IPV6;
	hints.ai_addrlen   = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr      = NULL;
	hints.ai_next      = NULL;

    if (i = getaddrinfo(hostname, NULL, &hints, &ai)) {
      fprintf(stderr, "vic: getaddrinfo: %s: %s\n", hostname, gai_strerror(i));
      return (-1);
    }
    memcpy(addr->s6_addr, &(((struct sockaddr_in6 *)(ai->ai_addr))->sin6_addr),  sizeof(struct in6_addr));
  }
  return (0);
}


int 
inet6_LookupLocalAddr(struct in6_addr *addr) {
  static int once = 0, len;
  static struct in6_addr local_addr;

  if (!once) {
    char name[MAXHOSTNAMELEN];
    int gh=gethostname(name, sizeof(name));
    if (inet6_LookupHostAddr(&local_addr, name) != 0)
      return (-1);
    once++;
  }
  len=sizeof(local_addr.s6_addr);
  memcpy(addr->s6_addr, local_addr.s6_addr, sizeof(local_addr.s6_addr));
  return (0);
}


int 
inet6_InetNtoa(char *address, struct in6_addr *addr) {
  address = (char *)malloc(INET6_ADDRSTRLEN);
  return ((inet_ntop(AF_INET6, addr, address, INET6_ADDRSTRLEN) != NULL) 
	  ? (-1)
	  : (0));
}

int
inet6_LookupHostName(char* name, struct in6_addr *addr) {
  struct hostent* hp;

  if (IN6_IS_ADDR_MULTICAST(addr))  return (inet6_InetNtoa(name, addr));

  hp = gethostbyaddr(addr->s6_addr, sizeof(addr->s6_addr), AF_INET6);

  if (hp == 0) return inet6_InetNtoa(name, addr);

  name = (char *)malloc(strlen(hp->h_name) + 1);
  strcpy(name, hp->h_name);
  return (0);
}



#endif /* HAVE_IPV6 */
