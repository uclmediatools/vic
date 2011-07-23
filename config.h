/*
 * Copyright (c) 1995 The Regents of the University of California.
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

#ifndef vic_config_h
#define vic_config_h

#ifdef HAVE_CONFIG_H
#include "uclconf.h"
#endif

#if defined(sgi) || defined(__bsdi__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
#include <sys/types.h>

#ifndef uint16_t
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
#endif

#elif defined(sun)

#include <sys/types.h>
#define int8_t	char
#define int16_t	short
#define int32_t	long
typedef unsigned char  u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned int   u_int32_t;

#elif defined(linux)

#include <sys/bitypes.h>

#else /* ie non of the above - e.g. WIN32 */

#ifdef ultrix
#include <sys/types.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
typedef char 		int8_t;
typedef short 		int16_t;
typedef unsigned short	uint16_t;
typedef int   		int32_t;
#endif 
//<sys/types.h>
typedef unsigned char	u_int8_t;
typedef unsigned short	u_int16_t;
typedef unsigned int	u_int32_t;

#endif /* end of checks for stdint */

#if defined(sun) || defined(_AIX)
#if defined(__cplusplus)
extern "C" {
#endif
void srandom(unsigned);
long random(void);
#if defined(__cplusplus)
}
#endif
#endif

#ifdef sgi
#include <math.h>
#endif

#ifdef __linux__
#define DEFINED_ERRLIST
#endif

#include "debug.h"
#include <stdlib.h>
#include <time.h>		/* For clock_t */

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */

#if defined(NEED_SUNOS_PROTOS)
#if defined(__cplusplus)
extern "C" {
#endif
struct timeval;
struct timezone;
struct msghdr;
struct sockaddr;

int bind(int s, struct sockaddr*, int);
int close(int);
int connect(int s, struct sockaddr*, int);
int gethostid();
int gethostname(char*, int);
int getpid();
int getsockname(int, struct sockaddr*, int*);
int gettimeofday(struct timeval*, struct timezone*);
int ioctl(int fd, int request, ...);
int recv(int, void*, int len, int flags);
int recvfrom(int, void*, int len, int flags, struct sockaddr*, int*);
int send(int s, void*, int len, int flags);
int sendmsg(int, struct msghdr*, int);
int setsockopt(int s, int level, int optname, void* optval, int optlen);
int socket(int, int, int);
int strcasecmp(const char*, const char*);
#if defined(__cplusplus)
}
#endif
#endif

#ifndef WIN32
#include <unistd.h>
#if defined(__cplusplus)
extern "C" {
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
clock_t clock(void);
#if !defined(sco) && !defined(sgi) && !defined(__bsdi__) && !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(sun) && !defined(__linux__) && !defined(__APPLE__)
int		gethostid(void);
#endif
time_t	time(time_t *);
char   *ctime(const time_t *);
#if defined(__cplusplus)
}
#endif
#endif /*ndef WIN32*/


#ifdef WIN32 
//#include <windef.h>
//#include <winbase.h>

#include <limits.h>
#include <malloc.h>
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <process.h>
#include <math.h>
#include <stdlib.h>   /* abs() */
#include <string.h>

/* Must be included before <Windows.h> otherwise winsock.h gets included instead */
#include <winsock2.h>


#ifdef HAVE_IPV6

#if !defined(MUSICA_IPV6) && defined(WIN32) /* MS_IPV6 */

#ifdef WINXP_IPV6
#include <ws2tcpip.h>
#else
#ifdef WIN2K_IPV6
#include <ws2tcpip.h>
#include <tpipv6.h>
#else
#include <ws2ip6.h>
#include <ws2tcpip.h>
#endif
#endif

#define IN6_ARE_ADDR_EQUAL IN6_ADDR_EQUAL

#define IN6_IS_ADDR_UNSPECIFIED(addr) \
        (((addr)->s6_addr[0] == 0) && \
        ((addr)->s6_addr[1] == 0) && \
        ((addr)->s6_addr[2] == 0) && \
        ((addr)->s6_addr[3] == 0) && \
        ((addr)->s6_addr[4] == 0) && \
        ((addr)->s6_addr[5] == 0) && \
        ((addr)->s6_addr[6] == 0) && \
        ((addr)->s6_addr[7] == 0) && \
        ((addr)->s6_addr[8] == 0) && \
        ((addr)->s6_addr[9] == 0) && \
        ((addr)->s6_addr[10] == 0) && \
        ((addr)->s6_addr[11] == 0) && \
        ((addr)->s6_addr[12] == 0) && \
        ((addr)->s6_addr[13] == 0) && \
        ((addr)->s6_addr[14] == 0) && \
        ((addr)->s6_addr[15] == 0))

#define IN6_IS_ADDR_MULTICAST(addr) \
        ((addr)->s6_addr[0] == 0xffU)

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN        46      /* max len of IPv6 addr in ascii */
#endif                                  /* standard colon-hex notation. */ 

#define EAFNOSUPPORT -1
//#define IP_MULTICAST_LOOP       0x12    /* set/get IP multicast loopback   */

#endif /* MS_IPV6 */

#endif /* HAVE_IPV6 */

#ifdef MUSICA_IPV6
#if defined(__cplusplus)
extern "C" {
#endif
const char *inet_ntop(int af, const void *src, char *dst, size_t size);
int			inet_pton(int af,const char *src,void *dst);
#if defined(__cplusplus)
}
#endif 

#include <winsock6.h>
/* These DEF's are required as MUSICA's winsock6.h causes a clash with some of the 
 * standard ws2tcpip.h definitions (eg struct in_addr6).
 * Note: winsock6.h defines AF_INET6 as 24 NOT 23 as in winsock2.h - I have left it
 * set to the MUSICA value as this is used in some of their function calls. 
 */
//#define AF_INET6        23
#define IP_MULTICAST_LOOP      11 /*set/get IP multicast loopback */
#define	IP_MULTICAST_IF		9 /* set/get IP multicast i/f  */
#define	IP_MULTICAST_TTL       10 /* set/get IP multicast ttl */
#define	IP_MULTICAST_LOOP      11 /*set/get IP multicast loopback */
#define	IP_ADD_MEMBERSHIP      12 /* add an IP group membership */
#define	IP_DROP_MEMBERSHIP     13/* drop an IP group membership */
/*#define IN6ADDR_ANY_INIT { (unsigned char)0, (unsigned char)0, (unsigned char)0, (unsigned char)0, \
                            (unsigned char)0, (unsigned char)0, (unsigned char)0, (unsigned char)0, \
                            (unsigned char)0, (unsigned char)0, (unsigned char)0, (unsigned char)0, \
                            (unsigned char)0, (unsigned char)0, (unsigned char)0, (unsigned char)0 } */
struct ip_mreq {
	struct in_addr imr_multiaddr;	/* IP multicast address of group */
	struct in_addr imr_interface;	/* local IP address of interface */
};
#else 
#include <ws2tcpip.h>
typedef int pid_t;
#endif

typedef int uid_t;
typedef int gid_t;

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	256
#endif
typedef char *caddr_t;
/*
 * the definitions below are valid for 32-bit architectures and will have to
 * be adjusted for 16- or 64-bit architectures
 */
typedef u_char		u_int8;
typedef u_short		u_int16;
typedef u_long		u_int32;
typedef char		int8;
typedef short		int16;
typedef long		int32;
typedef __int64		int64;
typedef unsigned long	in_addr_t;

#define _SYS_NMLN	9
struct utsname {
	char sysname[_SYS_NMLN];
	char nodename[_SYS_NMLN];
	char release[_SYS_NMLN];
	char version[_SYS_NMLN];
	char machine[_SYS_NMLN];
};


struct iovec {
	caddr_t iov_base;
	int	    iov_len;
};


struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

    
#if defined(__cplusplus)
extern "C" {
#endif

int uname(struct utsname *);
int getopt(int, char * const *, const char *);
int strcasecmp(const char *, const char *);
int srandom(int);
int random(void);
int gettimeofday(struct timeval *p, struct timezone *z);
int gethostid(void);
int getuid(void);
int getgid(void);
#define getpid _getpid
int nice(int);
int sendmsg(int, struct msghdr*, int);
time_t time(time_t *);

#if defined(__cplusplus)
}
#endif

#ifdef ECONNREFUSED
#undef ECONNREFUSED
#endif
#define ECONNREFUSED	WSAECONNREFUSED

#ifdef ENETUNREACH
#undef ENETUNREACH
#endif
#define ENETUNREACH		WSAENETUNREACH

#ifdef EHOSTUNREACH
#undef EHOSTUNREACH
#endif
#define EHOSTUNREACH	WSAEHOSTUNREACH

#ifdef EWOULDBLOCK
#undef EWOULDBLOCK
#endif
#define EWOULDBLOCK		WSAEWOULDBLOCK

#define M_PI		3.14159265358979323846

#include <windows.h>

#endif /* WIN32 */

#ifndef TRUE
#define FALSE   0
#define TRUE    1
#endif /* TRUE */

#endif /* vic_config_h */
