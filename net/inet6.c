/*
 * inet6.cc -- netdb functions for IPv6 addresses
 */

/*
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Lawrence Berkeley Laboratory,
 * Berkeley, CA.  The name of the University may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

static const char rcsid[] =
    "@(#) $Header$";

#ifdef HAVE_IPV6

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
//#include <winsock.h>
#else
#include <sys/param.h>
#include <netdb.h>
#include <sys/socket.h>
#endif
#include <stdio.h>

#include "config.h"

#include "inet6.h"

int 
inet6_LookupHostAddr(struct in6_addr *addr, const char* hostname) {
  if (inet_pton(AF_INET6, hostname, addr->s6_addr) != 1) {
    struct hostent *hp;
#ifdef SOLARIS7_IPV6_OLD
    int error_num;
    hp = getipnodebyname(hostname, AF_INET6, AI_DEFAULT, &error_num);
#else
#if  defined(LINUX_IPV6) ||  defined(SOLARIS7_IPV6)
    struct addrinfo hints, *ai, *ai2;
    int i;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;

    if (i = getaddrinfo(hostname, NULL, &hints, &ai)) {
      fprintf(stderr, "vic: getaddrinfo: %s: %s\n", hostname, gai_strerror(i));
      return (-1);
    }
    memcpy(addr->s6_addr, &(((struct sockaddr_in6 *)(ai->ai_addr))->sin6_addr),  sizeof(struct in6_addr));
    return (0);
#else
#ifdef MUSICA_IPV6
    hp = gethostbyname2(hostname, AF_INET6);
#else
    hp = getnodebyname(hostname, AF_INET6,AI_DEFAULT);
#endif /*MUSICA_IPV6*/
#endif /*LINUX_IPV6*/
#endif /*SOLARIS7_IPV6*/
    if (hp == 0)  return (-1);
    memcpy(addr->s6_addr, *(hp->h_addr_list), hp->h_length);
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
