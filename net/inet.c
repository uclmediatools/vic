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
   if (hp == 0)
       return (0);
   return *((u_int32_t **)hp->h_addr_list)[0];
}

u_int32_t
LookupLocalAddr(void)
{
	static u_int32_t local_addr;
	char name[MAXHOSTNAMELEN];
	
	if (local_addr == 0) {
		gethostname(name, sizeof(name));
		local_addr = LookupHostAddr(name);
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
