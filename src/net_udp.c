/*
* FILE:     net_udp.c
* AUTHOR:   Colin Perkins 
* MODIFIED: Orion Hodson & Piers O'Hanlon
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
#include "inet_pton.h"
#include "inet_ntop.h"
#include "net_udp.h"

#define IPv4	4
#define IPv6	6

/* This is pretty nasty but it's the simplest way to get round */
/* the Detexis bug that means their MUSICA IPv6 stack uses     */
/* IPPROTO_IP instead of IPPROTO_IPV6 in setsockopt calls      */
#ifdef MUSICA_IPV6
#define IPPROTO_IPV6 IPPROTO_IP
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

struct _socket_udp {
	int	 	 mode;	/* IPv4 or IPv6 */
	char		*addr;
	u_int16	 	 rx_port;
	u_int16		 tx_port;
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
	return (addr->s_addr != (in_addr_t) INADDR_NONE);
}
#endif

#ifdef NEED_IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(addr) ((addr)->s6_addr[0] == 0xffU)
#endif


/*****************************************************************************/
/* IPv4 specific functions...                                                */
/*****************************************************************************/

static socket_udp *udp_init4(char *addr, u_int16 rx_port, u_int16 tx_port, int ttl)
{
	int                 reuse = 1;
	struct sockaddr_in  s_in;
	socket_udp         *s = (socket_udp *) malloc(sizeof(socket_udp));
	s->mode    = IPv4;
	s->addr    = addr;
	s->rx_port = rx_port;
	s->tx_port = tx_port;
	s->ttl     = ttl;
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
	s_in.sin_port        = htons(rx_port);
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
	return s;
}

static void udp_exit4(socket_udp *s)
{
	if (IN_MULTICAST(ntohl(s->addr4.s_addr))) {
		struct ip_mreq  imr;
		imr.imr_multiaddr.s_addr = s->addr4.s_addr;
		imr.imr_interface.s_addr = INADDR_ANY;
		if (SETSOCKOPT(s->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &imr, sizeof(struct ip_mreq)) != 0) {
			socket_error("setsockopt IP_DROP_MEMBERSHIP");
			abort();
		}
	}
	close(s->fd);
	free(s);
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
	s_in.sin_port        = htons(s->tx_port);
	if ((ret = sendto(s->fd, buffer, buflen, 0, (struct sockaddr *) &s_in, sizeof(s_in))) < 0) {
		socket_error("udp_send4");
	}
	return ret;
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

/*****************************************************************************/
/* IPv6 specific functions...                                                */
/*****************************************************************************/

static socket_udp *udp_init6(char *addr, u_int16 rx_port, u_int16 tx_port, int ttl)
{
#ifdef HAVE_IPv6
	int                 reuse = 1;
	struct sockaddr_in6 s_in;
	socket_udp         *s = (socket_udp *) malloc(sizeof(socket_udp));
	s->mode    = IPv6;
	s->addr    = addr;
	s->rx_port = rx_port;
	s->tx_port = tx_port;
	s->ttl     = ttl;
	
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
	s_in.sin6_port   = htons(rx_port);
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
#ifdef MUSICA_IPV6
		imr.i6mr_interface = 1;
		imr.i6mr_multiaddr = s->addr6;
#else
		imr.ipv6mr_multiaddr = s->addr6;
		imr.ipv6mr_interface = 0;
#endif
		
		if (SETSOCKOPT(s->fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *) &imr, sizeof(struct ipv6_mreq)) != 0) {
			socket_error("setsockopt IPV6_ADD_MEMBERSHIP");
			abort();
		}
		
		if (SETSOCKOPT(s->fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char *) &loop, sizeof(loop)) != 0) {
			socket_error("setsockopt IPV6_MULTICAST_LOOP");
			abort();
		}
		if (SETSOCKOPT(s->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char *) &ttl, sizeof(ttl)) != 0) {
			socket_error("setsockopt IPV6_MULTICAST_HOPS");
			abort();
		}
	}
	assert(s != NULL);
	return s;
#else
	UNUSED(addr);
	UNUSED(rx_port);
	UNUSED(tx_port);
	UNUSED(ttl);
	return NULL;
#endif
}

static void udp_exit6(socket_udp *s)
{
#ifdef HAVE_IPv6
	if (IN6_IS_ADDR_MULTICAST(&(s->addr6))) {
		struct ipv6_mreq  imr;
#ifdef MUSICA_IPV6
		imr.i6mr_interface = 1;
		imr.i6mr_multiaddr = s->addr6;
#else
		imr.ipv6mr_multiaddr = s->addr6;
		imr.ipv6mr_interface = 0;
#endif
		
		if (SETSOCKOPT(s->fd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (char *) &imr, sizeof(struct ipv6_mreq)) != 0) {
			socket_error("setsockopt IPV6_DROP_MEMBERSHIP");
			abort();
		}
	}
	close(s->fd);
	free(s);
#else
	UNUSED(s);
#endif  /* HAVE_IPv6 */
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
	s_in.sin6_port   = htons(s->tx_port);
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

static char *udp_host_addr6(socket_udp *s)
{
#ifdef HAVE_IPv6
	char	       		*hname;
	struct hostent 		*hent;
#if !defined(WIN32) || defined(MUSICA_IPV6)
	int			 error_num;
#endif
#if defined(Linux) || defined(Solaris)
	int gai_err;
	struct addrinfo hints, *ai;
#endif
	struct sockaddr_in6 local;
	int len = sizeof(local), result = 0;
	
	hname = (char *) xmalloc(MAXHOSTNAMELEN);
	
	memset((char *)&local, 0, len);
	local.sin6_family = AF_INET6;
	
	if ((result = getsockname(s->fd,(struct sockaddr *)&local, &len)) < 0){
		local.sin6_addr = in6addr_any;
		local.sin6_port = 0;
		debug_msg("getsockname failed\n");
	}
#ifdef MUSICA_IPV6
	/* Unfortunately MUSICA stack returns the multicast addr to above 
	getsockname call, thus we have to check for and do DNS lookup instead */
	if (IS_UNSPEC_IN6_ADDR(local.sin6_addr) || IN6_IS_ADDR_MULTICAST(&local.sin6_addr)) {
#else
	if (IN6_IS_ADDR_UNSPECIFIED(&(local.sin6_addr))) {
#endif
		if (gethostname(hname, MAXHOSTNAMELEN) != 0) {
			debug_msg("gethostname failed\n");
			abort();
		}
		
#if defined(WIN32) && !defined(MUSICA_IPV6)
		hent = getnodebyname(hname, AF_INET6, AI_DEFAULT);
		assert(hent->h_addrtype == AF_INET6);
		hname = xstrdup(inet6_ntoa((const struct in6_addr *) hent->h_addr_list[0]));
#else
#if defined(Linux) || defined(Solaris)
		
		memset(&hints, 0, sizeof(struct addrinfo));
		
		hints.ai_protocol = IPPROTO_IPV6;
		/*hints.ai_family = AF_INET6; */
		/*hints.ai_socktype = SOCK_DGRAM;*/
		
		if ((gai_err=getaddrinfo(hname, NULL, &hints, &ai))) {
			debug_msg("getaddrinfo: %s: %s\n", hname, gai_strerror(gai_err));
			abort();
		}
		
		if (inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)(ai->ai_addr))->sin6_addr), hname, MAXHOSTNAMELEN) == NULL) {
			freeaddrinfo(ai);
			debug_msg("inet_ntop: %s: \n", hname);
			abort();
		}
		freeaddrinfo(ai);
		UNUSED(error_num);
		UNUSED(hent);
#else
#ifdef MUSICA_IPV6
		hent = gethostbyname2(hname, AF_INET6);
#else
		hent = getipnodebyname(hname, AF_INET6, AI_DEFAULT, &error_num);
#endif /*MUSICA_IPV6*/
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
		if (inet_ntop(AF_INET6, hent->h_addr_list[0], hname, MAXHOSTNAMELEN) == NULL) {
			debug_msg("inet_ntop: %s: \n", hname);
			abort();
		}
#endif /*Linux || Solaris7 IPv6 */
#endif /*MS_IPV6*/
		return hname;
	}
	if (inet_ntop(AF_INET6, &local.sin6_addr, hname, MAXHOSTNAMELEN) == NULL) {
		debug_msg("inet_ntop: %s: \n", hname);
		abort();
	}
	return hname;
#else  /* HAVE_IPv6 */
	UNUSED(s);
	return "::";	/* The unspecified address... */
#endif /* HAVE_IPv6 */
}
	
/*****************************************************************************/
/* Generic functions, which call the appropriate protocol specific routines. */
/*****************************************************************************/

socket_udp *udp_init(char *addr, u_int16 rx_port, u_int16 tx_port, int ttl)
{
	socket_udp *res;
	
	if (strchr(addr, ':') == NULL) {
		res = udp_init4(addr, rx_port, tx_port, ttl);
	} else {
		res = udp_init6(addr, rx_port, tx_port, ttl);
	}
	return res;
}

void udp_exit(socket_udp *s)
{
    switch(s->mode) {
    case IPv4 : udp_exit4(s); break;
    case IPv6 : udp_exit6(s); break;
    default   : abort();
    }
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

char *udp_host_addr(socket_udp *s)
{
	switch (s->mode) {
	case IPv4 : return udp_host_addr4();
	case IPv6 : return udp_host_addr6(s);
	default   : abort();
	}
	return NULL;
}
