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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <pwd.h>
#include <signal.h>
#include <ctype.h>

#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>   /* abs() */
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>
extern int h_errno;

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif /* HAVE_STROPTS_H */

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>  
#endif /* HAVE_SYS_FILIO_H */

#ifdef HAVE_IPv6

#ifdef HAVE_NETINET6_IN6_H
/* Expect IPV6_{JOIN,LEAVE}_GROUP in in6.h, otherwise expect                 */
/* IPV_{ADD,DROP}_MEMBERSHIP in in.h                                         */
#include <netinet6/in6.h>
#else
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN6_H */

#ifndef IPV6_ADD_MEMBERSHIP
#ifdef  IPV6_JOIN_GROUP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#else
#error  No definition of IPV6_ADD_MEMBERSHIP, please report to mm-tools@cs.ucl.ac.uk.
#endif /* IPV6_JOIN_GROUP     */
#endif /* IPV6_ADD_MEMBERSHIP */

#ifndef IPV6_DROP_MEMBERSHIP
#ifdef  IPV6_LEAVE_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#else
#error  No definition of IPV6_LEAVE_GROUP, please report to mm-tools@cs.ucl.ac.uk.
#endif  /* IPV6_LEAVE_GROUP     */
#endif  /* IPV6_DROP_MEMBERSHIP */

#endif /* HAVE_IPv6 */

#include <net/if.h>

typedef u_char	ttl_t;
typedef int	fd_t;

#ifndef TRUE
#define FALSE	0
#define	TRUE	1
#endif /* TRUE */

#define USERNAMELEN	8

#define max(a, b)	(((a) > (b))? (a): (b))
#define min(a, b)	(((a) < (b))? (a): (b))

#ifdef NDEBUG
#define assert(x) if ((x) == 0) fprintf(stderr, "%s:%u: failed assertion\n", __FILE__, __LINE__)
#else
#include <assert.h>
#endif

#ifdef Solaris
#ifdef __cplusplus
extern "C" {
#endif
int gettimeofday(struct timeval *tp, void * );
int gethostname(char *name, int namelen);
#ifdef __cplusplus
}
#endif
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif /* EXIT_SUCCESS */

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 7
#endif /* EXIT_FAILURE */

#ifndef STDC_HEADERS
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

#endif /* _CONFIG_UNIX_H_ */
#endif /* WIN32 */
