/*
 *  config-unix.h
 *
 *  Unix specific definitions and includes
 *  
 *  $Revision$
 *  $Date$
 *
 * Copyright (c) 1995-98 University College London
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

#ifndef WIN32
#ifndef _CONFIG_UNIX_H
#define _CONFIG_UNIX_H

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <pwd.h>
#include <signal.h>
#include <ctype.h>

#ifndef __FreeBSD__
#include <malloc.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>   /* abs() */
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>
extern int h_errno;
#if !defined(HPUX) && !defined(Linux) && !defined(__FreeBSD__)
#include <stropts.h>
#include <sys/filio.h>  
#endif /* HPUX */
#include <net/if.h>

typedef unsigned char	byte;

/*
 * the definitions below are valid for 32-bit architectures and will have to
 * be adjusted for 16- or 64-bit architectures
 */
typedef u_char  u_int8;
typedef u_short u_int16;
typedef u_long  u_int32;
typedef char	int8;
typedef short	int16;
typedef long	int32;
typedef long long int64;

typedef char	ttl_t;
typedef int	fd_t;

#ifndef TRUE
#define FALSE	0
#define	TRUE	1
#endif /* TRUE */

#define USERNAMELEN	8

#define max(a, b)	(((a) > (b))? (a): (b))
#define min(a, b)	(((a) < (b))? (a): (b))

#ifdef NEED_IN_ADDR_T
typedef unsigned long   in_addr_t;
#endif

#ifdef NDEBUG
#define assert(x) if ((x) == 0) fprintf(stderr, "%s:%u: failed assertion\n", __FILE__, __LINE__)
#else
#include <assert.h>
#endif

#ifdef __NetBSD__
#include <fcntl.h>
#include <sys/audioio.h>
#endif

#ifdef FreeBSD
#include <unistd.h>
#include <stdlib.h>
#define DIFF_BYTE_ORDER  1
#endif /* FreeBSD */

#ifdef Solaris
#include <fcntl.h>
#include <sys/sockio.h>
#ifdef __cplusplus
extern "C" {
#endif
int gettimeofday(struct timeval *tp, void * );
int gethostname(char *name, int namelen);
#ifdef __cplusplus
}
#endif
#endif

#ifdef SunOS
#define NEED_INET_PTON
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 7
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <memory.h>
int 	gethostname(char *name, int namelen);
int 	gettimeofday(struct timeval *tp, struct timezone *tzp);
double	drand48();
void 	srand48(long seedval);
long	lrand48();
int	setsockopt(int s, int level, int optname, const char *optval, int optlen);
void	perror();
int	printf(char *format, ...);
int	fprintf(FILE *stream, char *format, ...);
int	fclose(FILE *stream);
int	fread(void *ptr, int size, int nitems, FILE *stream);
int	fwrite(void *ptr, int size, int nitems, FILE *stream);
int	fflush(FILE *stream);
void	bzero(char *b, int length);
void	bcopy(char *b1, char *b2, int length);
int	connect(int s, struct sockaddr *name, int namelen);
int	select(int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int	bind(int s, struct sockaddr *name, int namelen);
int	socket(int domain, int type, int protocol);
int	sendto(int s, char *msg, int len, int flags, struct sockaddr *to, int tolen);
int	writev(int fd, struct iovec *iov, int iovcnt);
int	recvfrom(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen);
int	close(int fd);
int	ioctl(int fd, int request, caddr_t arg);
int 	sscanf(char *s, char *format, ...);
time_t	time(time_t *tloc);
int	strcasecmp(char *s1, char *s2);
long	strtol(char *str, char **ptr, int base);
int	toupper(int c);
#define	memmove(dst, src, len)	bcopy((char *) src, (char *) dst, len)
#endif

#ifdef IRIX
#include <bstring.h>     /* Needed for FDZERO on IRIX only */
int gethostname(char *name, int namelen);
#endif

#ifdef HPUX
#define NEED_INET_PTON
#include <unistd.h>
int gethostname(char *hostname, size_t size);
#endif

#ifdef Linux
#define DIFF_BYTE_ORDER  1
#include <fcntl.h>
void *memcpy(void *dest, const void *src, size_t n);
int   memcmp(const void *s1, const void *s2, size_t n);
#endif /* Linux */

#endif /* _CONFIG_UNIX_H_ */
#endif /* WIN32 */
