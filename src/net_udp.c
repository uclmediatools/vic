/*
 * FILE:    net_udp.c
 * AUTHORS: Colin Perkins
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
 *      Department at University College London
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

/* If this machine supports IPv6 the symbol HAVE_IPv6 should */
/* be defined in either config_unix.h or config_win32.h. The */
/* appropriate system header files should also be included   */
/* by those files.                                           */

#include "config_unix.h"
#include "config_win32.h"
#include "debug.h"
#include "memory.h"
#include "net_udp.h"

#define IPv4	4
#define IPv6	6

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

struct _socket_udp {
	int	 	 mode;	/* IPv4 or IPv6 */
	char		*addr;
	u_int16	 	 port;
	ttl_t	 	 ttl;
	fd_t	 	 fd;
	struct in_addr	 addr4;
#ifdef HAVE_IPv6
	struct in6_addr	 addr6;
#endif /* HAVE_IPv6 */
};

#ifdef WIN32
/* Want to use both Winsock 1 and 2 socket options, but since
 * ipv6 support requires Winsock 2 we have to add own backwards
 * compatibility for Winsock 1.
 */
#define SETSOCKOPT winsock_versions_setsockopt
#else
#define SETSOCKOPT setsockopt
#endif /* WIN32 */

/*****************************************************************************/
/* Support functions...                                                      */
/*****************************************************************************/

static void
socket_error(char *msg)
{
#ifdef WIN32
#define WSERR(x) {#x,x}
        struct wse {
                char  errname[20];
                int errno;
        };
        struct wse ws_errs[] = {
                WSERR(WSANOTINITIALISED), WSERR(WSAENETDOWN),     WSERR(WSAEACCES),
                WSERR(WSAEINVAL),         WSERR(WSAEINTR),        WSERR(WSAEINPROGRESS),
                WSERR(WSAEFAULT),         WSERR(WSAENETRESET),    WSERR(WSAENOBUFS),
                WSERR(WSAENOTCONN),       WSERR(WSAENOTSOCK),     WSERR(WSAEOPNOTSUPP),
                WSERR(WSAESHUTDOWN),      WSERR(WSAEWOULDBLOCK),  WSERR(WSAEMSGSIZE),
                WSERR(WSAEHOSTUNREACH),   WSERR(WSAECONNABORTED), WSERR(WSAECONNRESET),
                WSERR(WSAEADDRNOTAVAIL),  WSERR(WSAEAFNOSUPPORT), WSERR(WSAEDESTADDRREQ),
                WSERR(WSAENETUNREACH),    WSERR(WSAETIMEDOUT),    WSERR(0)
        };
	
        int i, e = WSAGetLastError();
	i = 0;
        while(ws_errs[i].errno && ws_errs[i].errno != e) {
               i++;
        }
        printf("ERROR: %s, (%d - %s)\n", msg, e, ws_errs[i].errname);
#else
	perror(msg);
#endif
	abort();
}

#ifdef WIN32
/* ws2tcpip.h defines these constants with different values from
 * winsock.h so files that use winsock 2 values but try to use 
 * winsock 1 fail.  So what was the motivation in changing the
 * constants ?
 */
#define WS1_IP_MULTICAST_IF     2 /* set/get IP multicast interface   */
#define WS1_IP_MULTICAST_TTL    3 /* set/get IP multicast timetolive  */
#define WS1_IP_MULTICAST_LOOP   4 /* set/get IP multicast loopback    */
#define WS1_IP_ADD_MEMBERSHIP   5 /* add  an IP group membership      */
#define WS1_IP_DROP_MEMBERSHIP  6 /* drop an IP group membership      */

/* winsock_versions_setsockopt tries 1 winsock version of option 
 * optname and then winsock 2 version if that failed.
 */

static int
winsock_versions_setsockopt(SOCKET s, int level, int optname, const char FAR * optval, int optlen)
{
        int success = -1;
        switch (optname) {
        case IP_MULTICAST_IF:
                success = setsockopt(s, level, WS1_IP_MULTICAST_IF, optval, optlen);
                break;
        case IP_MULTICAST_TTL:
                success = setsockopt(s, level, WS1_IP_MULTICAST_TTL, optval, optlen);
                break;
        case IP_MULTICAST_LOOP:
                success = setsockopt(s, level, WS1_IP_MULTICAST_LOOP, optval, optlen);
                break;
        case IP_ADD_MEMBERSHIP: 
                success = setsockopt(s, level, WS1_IP_ADD_MEMBERSHIP, optval, optlen);
                break;
        case IP_DROP_MEMBERSHIP: 
                success = setsockopt(s, level, WS1_IP_DROP_MEMBERSHIP, optval, optlen);
                break;
        }
        if (success != -1) {
                return success;
        }
        return setsockopt(s, level, optname, optval, optlen);
}
#endif

#ifdef NEED_INET_ATON
#ifdef NEED_INET_ATON_STATIC
static 
#endif
int inet_aton(const char *name, struct in_addr *addr)
{
	addr->s_addr = inet_addr(name);
	return (addr->s_addr != (in_addr_t) INADDR_ANY);
}
#endif

#ifdef NEED_INET_PTON
#ifdef NEED_INET_PTON_STATIC
static 
#endif
int inet_pton(int family, const char *name, void *addr)
{
	if (family == AF_INET) {
		struct in_addr	in_val;

		if (inet_aton(name, &in_val)) {
			memcpy(addr, &in_val, sizeof(struct in_addr));
			return 1;
		}
		return 0;
#if defined (NEED_INET_PTON) && defined(HAVE_IPv6)
	} else if (family == AF_INET6) {
		return inet6_addr(name, addr);
#endif
	} else {
		debug_msg("Unknown address family\n");
		return -1;
	}
}
#endif

#ifdef NEED_IN6_IS_ADDR_MULTICAST
/*static int IN6_IS_ADDR_MULTICAST(unsigned char addr[16])
 *{
 *	return addr[0] == 0xff;
 *}
 */
#define IN6_IS_ADDR_MULTICAST(addr) ((addr)->s6_addr[0] == 0xffU)
#endif


/*****************************************************************************/
/* IPv4 specific functions...                                                */
/*****************************************************************************/

static socket_udp *udp_init4(char *addr, u_int16 port, int ttl)
{
	int                 reuse = 1;
	struct sockaddr_in  s_in;
	socket_udp         *s = (socket_udp *) malloc(sizeof(socket_udp));
	s->mode  = IPv4;
	s->addr  = addr;
	s->port  = port;
	s->ttl   = ttl;
	if (inet_pton(AF_INET, addr, &s->addr4) != 1) {
		struct hostent *h = gethostbyname(addr);
		if (h == NULL) {
			return NULL;
		}
		memcpy(&(s->addr4), h->h_addr_list[0], sizeof(s->addr4));
	}
	s->fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (s->fd < 0) {
		socket_error("socket");
		abort();
	}
	if (SETSOCKOPT(s->fd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) != 0) {
		socket_error("setsockopt SO_REUSEADDR");
		abort();
	}
#ifdef SO_REUSEPORT
	if (SETSOCKOPT(s->fd, SOL_SOCKET, SO_REUSEPORT, (char *) &reuse, sizeof(reuse)) != 0) {
		socket_error("setsockopt SO_REUSEPORT");
		abort();
        }
#endif
	s_in.sin_family      = AF_INET;
	s_in.sin_addr.s_addr = INADDR_ANY;
	s_in.sin_port        = htons(port);
	if (bind(s->fd, (struct sockaddr *) &s_in, sizeof(s_in)) != 0) {
		socket_error("bind");
		abort();
	}
	if (IN_MULTICAST(ntohl(s->addr4.s_addr))) {
		char            loop = 1;
		struct ip_mreq  imr;

		imr.imr_multiaddr.s_addr = s->addr4.s_addr;
		imr.imr_interface.s_addr = INADDR_ANY;

		if (SETSOCKOPT(s->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &imr, sizeof(struct ip_mreq)) != 0) {
			socket_error("setsockopt IP_ADD_MEMBERSHIP");
			abort();
		}
#ifndef WIN32
		if (SETSOCKOPT(s->fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) != 0) {
			socket_error("setsockopt IP_MULTICAST_LOOP");
			abort();
		}
#endif
		if (SETSOCKOPT(s->fd, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &s->ttl, sizeof(s->ttl)) != 0) {
			socket_error("setsockopt IP_MULTICAST_TTL");
			abort();
		}
	}
        debug_msg("Inited socket %d port %d\n", s->fd, port);
	return s;
}

static int udp_send4(socket_udp *s, char *buffer, int buflen)
{
	struct sockaddr_in	s_in;
	int			ret;

	assert(s != NULL);
	assert(s->mode == IPv4);
	assert(buffer != NULL);
	assert(buflen > 0);

	s_in.sin_family      = AF_INET;
        s_in.sin_addr.s_addr = s->addr4.s_addr;
	s_in.sin_port        = htons(s->port);
	if ((ret = sendto(s->fd, buffer, buflen, 0, (struct sockaddr *) &s_in, sizeof(s_in))) < 0) {
		socket_error("udp_send4");
	}
        return ret;
}

/*****************************************************************************/
/* IPv6 specific functions...                                                */
/*****************************************************************************/

static socket_udp *udp_init6(char *addr, u_int16 port, int ttl)
{
#ifdef HAVE_IPv6
	int                 reuse = 1;
#ifdef WIN32
	struct in6_addr     in6addr_any = {0};
#endif
	struct sockaddr_in6 s_in;
	socket_udp         *s = (socket_udp *) malloc(sizeof(socket_udp));
	s->mode  = IPv6;
	s->addr  = addr;
	s->port  = port;
	s->ttl   = ttl;

	if (inet_pton(AF_INET6, addr, &s->addr6) != 1) {
		/* We should probably try to do a DNS lookup on the name */
		/* here, but I'm trying to get the basics going first... */
		debug_msg("IPv6 address conversion failed\n");
		return NULL;	
	}
	s->fd = socket(AF_INET6, SOCK_DGRAM, 0);
	if (s->fd < 0) {
		socket_error("socket");
		abort();
	}
	if (SETSOCKOPT(s->fd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) != 0) {
		socket_error("setsockopt SO_REUSEADDR");
		abort();
	}
#ifdef SO_REUSEPORT
	if (SETSOCKOPT(s->fd, SOL_SOCKET, SO_REUSEPORT, (char *) &reuse, sizeof(reuse)) != 0) {
		socket_error("setsockopt SO_REUSEPORT");
		abort();
	}
#endif

	s_in.sin6_family = AF_INET6;
	s_in.sin6_port   = htons(port);
	memcpy(s_in.sin6_addr.s6_addr, &s->addr6, sizeof(struct in6_addr));
	if (bind(s->fd, (struct sockaddr *) &s_in, sizeof(s_in)) != 0) {
             /* bind to group address failed, try generic address. */
             s_in.sin6_addr = in6addr_any;
             if (bind(s->fd, (struct sockaddr *) &s_in, sizeof(s_in)) != 0) {
                  socket_error("bind");
                  abort();
             }
        }

	if (IN6_IS_ADDR_MULTICAST(&(s->addr6))) {
		unsigned int      loop = 1;
		struct ipv6_mreq  imr;
		imr.ipv6mr_multiaddr = s->addr6;
		imr.ipv6mr_interface = 0;

		if (SETSOCKOPT(s->fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *) &imr, sizeof(struct ipv6_mreq)) != 0) {
			socket_error("setsockopt IPV6_ADD_MEMBERSHIP");
			abort();
		}
		if (SETSOCKOPT(s->fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof(loop)) != 0) {
			socket_error("setsockopt IPV6_MULTICAST_LOOP");
			abort();
		}
		if (SETSOCKOPT(s->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (int *) &ttl, sizeof(ttl)) != 0) {
			socket_error("setsockopt IPV6_MULTICAST_HOPS");
			abort();
		}
	}
	assert(s != NULL);
	return s;
#else
	UNUSED(addr);
	UNUSED(port);
	UNUSED(ttl);
	return NULL;
#endif
}

static int udp_send6(socket_udp *s, char *buffer, int buflen)
{
#ifdef HAVE_IPv6
	struct sockaddr_in6	s_in;
	int			ret;

	assert(s != NULL);
	assert(s->mode == IPv6);
	assert(buffer != NULL);
	assert(buflen > 0);

	s_in.sin6_family = AF_INET6;
	s_in.sin6_addr   = s->addr6;
	s_in.sin6_port   = htons(s->port);
	if ((ret = sendto(s->fd, buffer, buflen, 0, (struct sockaddr *) &s_in, sizeof(s_in))) < 0) {
		socket_error("udp_send6");
	}
	return ret;
#else
	UNUSED(s);
	UNUSED(buffer);
	UNUSED(buflen);
	return -1;
#endif
}

/*****************************************************************************/
/* Generic functions, which call the appropriate protocol specific routines. */
/*****************************************************************************/

socket_udp *udp_init(char *addr, u_int16 port, int ttl)
{
	socket_udp *res;

	if (strchr(addr, ':') == NULL) {
		res = udp_init4(addr, port, ttl);
	} else {
		res = udp_init6(addr, port, ttl);
	}
	return res;
}

int udp_send(socket_udp *s, char *buffer, int buflen)
{
	switch (s->mode) {
		case IPv4 : return udp_send4(s, buffer, buflen);
		case IPv6 : return udp_send6(s, buffer, buflen);
		default   : abort();
	}
	return -1;
}

int udp_recv(socket_udp *s, char *buffer, int buflen)
{
	/* Reads data into the buffer, returning the number of bytes read.   */
	/* If no data is available, this returns the value zero immediately. */
	/* Note: since we don't care about the source address of the packet  */
	/* we receive, this function becomes protocol independent.           */
	int		len;

	assert(buffer != NULL);
	assert(buflen > 0);

	len = recvfrom(s->fd, buffer, buflen, 0, 0, 0);
	if (len > 0) {
		return len;
	}
	socket_error("recvfrom");
	return 0;
}

static fd_set	rfd;
static fd_t	max_fd;

void udp_fd_zero(void)
{
	FD_ZERO(&rfd);
	max_fd = 0;
}

void udp_fd_set(socket_udp *s)
{
	FD_SET(s->fd, &rfd);
	if (s->fd > (fd_t)max_fd) {
		max_fd = s->fd;
	}
}

int udp_fd_isset(socket_udp *s)
{
	return FD_ISSET(s->fd, &rfd);
}

int udp_select(struct timeval *timeout)
{
	return select(max_fd + 1, &rfd, NULL, NULL, timeout);
}

static char *udp_host_addr4(void)
{
	char	       		*hname;
	struct hostent 		*hent;
	struct in_addr  	 iaddr;

	hname = (char *) xmalloc(MAXHOSTNAMELEN);
	if (gethostname(hname, MAXHOSTNAMELEN) != 0) {
		debug_msg("Cannot get hostname!");
		abort();
	}
	hent = gethostbyname(hname);
	assert(hent->h_addrtype == AF_INET);
	memcpy(&iaddr.s_addr, hent->h_addr, sizeof(iaddr.s_addr));
	strcpy(hname, inet_ntoa(iaddr));
	return hname;
}

#ifdef HAVE_IPv6
static char *udp_host_addr6(void)
{
	char	       		*hname;
	struct hostent 		*hent;
	int			 error_num;

	hname = (char *) xmalloc(MAXHOSTNAMELEN);
	if (gethostname(hname, MAXHOSTNAMELEN) != 0) {
		debug_msg("Cannot get hostname!");
		abort();
	}
	debug_msg("%s\n", hname);

	hent = getipnodebyname(hname, AF_INET6, AI_DEFAULT, &error_num);
	if (hent == NULL) {
		switch (error_num) {
		case HOST_NOT_FOUND:
			debug_msg("host not found\n");
			break;
		case NO_ADDRESS:
			debug_msg("no address\n");
			break;
		case NO_RECOVERY:
			debug_msg("no recovery\n");
			break;
		case TRY_AGAIN:
			debug_msg("try again\n");
			break;
		default:
			debug_msg("unknown error\n");
			break;
		}
		abort();
	}
	assert(hent->h_addrtype == AF_INET6);

	if (inet_ntop(AF_INET6, hent->h_addr, hname, MAXHOSTNAMELEN) == NULL) {
		abort();
	}
	return hname;
}
#endif /* HAVE_IPv6 */

char *udp_host_addr(socket_udp *s)
{
	switch (s->mode) {
		case IPv4 : return udp_host_addr4();
#ifdef HAVE_IPv6
		case IPv6 : return udp_host_addr6();
#endif /* HAVE_IPv6 */
		default   : abort();
	}
	return NULL;
}

