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

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

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

#include "config.h"
#include "inet.h"

u_int32_t
LookupHostAddr(const char *s)
{
   u_int32_t result;
   struct hostent *hp;

   if (isdigit(*s)) {
     result = inet_addr(s);
     if (result != INADDR_NONE) {
       return (result);
     }
   }
   hp = gethostbyname(s);
   if (hp == 0){
#ifndef WIN32
	   herror("Failed to lookup address using gethostbyname()");
#endif
	   return (0);
   }
   return *((u_int32_t **)hp->h_addr_list)[0];
}

u_int32_t
LookupLocalAddr(void)
{
	static u_int32_t local_addr=0;
	char name[MAXHOSTNAMELEN];
		
	if (!gethostname(name, sizeof(name))) {
		local_addr = LookupHostAddr(name);
	    debug_msg("gethostname Name:%s (looked up addr[Hex]:%x)\n",name, local_addr);
	}
	return (local_addr);
}

/*
 * A faster replacement for inet_ntoa().
 * Extracted from tcpdump 2.1.
 */
const char *
intoa(u_int32_t addr)
{
	register char *cp;
	register u_int byte;
	register int n;
	static char buf[sizeof(".xxx.xxx.xxx.xxx")];

	NTOHL(addr);
	cp = &buf[sizeof buf];
	*--cp = '\0';

	n = 4;
	do {
		byte = addr & 0xff;
		*--cp = byte % 10 + '0';
		byte /= 10;
		if (byte > 0) {
			*--cp = byte % 10 + '0';
			byte /= 10;
			if (byte > 0)
				*--cp = byte + '0';
		}
		*--cp = '.';
		addr >>= 8;
	} while (--n > 0);

	return cp + 1;
}

char *
InetNtoa(u_int32_t addr)
{
	const char *s = intoa(addr);
	char *p = (char *)malloc(strlen(s) + 1);
	strcpy(p, s);
	return p;
}

char *
LookupHostName(u_int32_t addr)
{
	char *p;
	struct hostent* hp;

	/*XXX*/
	if (IN_MULTICAST(ntohl(addr)))
		return (InetNtoa(addr));

	hp = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
	if (hp == 0) 
		return InetNtoa(addr);
	p = (char *)malloc(strlen(hp->h_name) + 1);
	strcpy(p, hp->h_name);
	return p;
}
