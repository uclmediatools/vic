/*-
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and the Network Research Group at
 *      Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#define close closesocket
#else
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#endif
#if defined(sun) && defined(__svr4__)
#include <sys/systeminfo.h>
#endif

#include "config.h"
#include "net.h"
#include "Tcl.h"

class IPNetwork : public Network {
    public:
	virtual int command(int argc, const char*const* argv);
	virtual void reset();
    protected:
	int open(u_int32_t addr, int port, int ttl);
	int close();
	void localname(sockaddr_in*);
	int openssock(u_int32_t addr, u_short port, int ttl);
	int openrsock(u_int32_t addr, u_short port,
		      const struct sockaddr_in& local);
	time_t last_reset_;
};

static class IPNetworkMatcher : public Matcher {
    public:
	IPNetworkMatcher() : Matcher("network") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "ip") == 0)
			return (new IPNetwork);
		else
			return (0);
	}
} nm_ip;

int IPNetwork::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "close") == 0) {
			close();
			return (TCL_OK);
		}
		char* cp = tcl.result();
		if (strcmp(argv[1], "addr") == 0) {
			strcpy(cp, intoa(addr_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "interface") == 0) {
			strcpy(cp, intoa(local_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "port") == 0) {
			sprintf(cp, "%d", ntohs(port_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "localport") == 0) {
			sprintf(cp, "%d", ntohs(lport_));
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ttl") == 0) {
			sprintf(cp, "%d", ttl_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "ismulticast") == 0) {
			tcl.result(IN_CLASSD(ntohl(addr_))? "1" : "0");
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "loopback") == 0) {
			char c = atoi(argv[2]);
			if (setsockopt(ssock_, IPPROTO_IP, IP_MULTICAST_LOOP,
				       &c, 1) < 0) {
				/*
				 * If we cannot turn off loopback (Like on the
				 * Microsoft TCP/IP stack), then declare this
				 * option broken so that our packets can be
				 * filtered on the recv path.
				 */
				if (c == 0)
					noloopback_broken_ = 1;
			}
			return (TCL_OK);
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "open") == 0) {
			u_int32_t addr = LookupHostAddr(argv[2]);
			int port = htons(atoi(argv[3]));
			int ttl = atoi(argv[4]);
			if (open(addr, port, ttl) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
	}
	return (Network::command(argc, argv));
}

int IPNetwork::open(u_int32_t addr, int port, int ttl)
{
	addr_ = addr;
	port_ = port;
	ttl_ = ttl;

	ssock_ = openssock(addr, port, ttl);
	if (ssock_ < 0)
		return (-1);
	/*
	 * Connecting the send socket also bound the local address.
	 * On a multihomed host we need to bind the receive socket
	 * to the same local address the kernel has chosen to send on.
	 */
	sockaddr_in local;
	localname(&local);
	rsock_ = openrsock(addr, port, local);
	if (rsock_ < 0) {
		(void)::close(ssock_);
		return (-1);
	}
	local_ = local.sin_addr.s_addr;
#if defined(sun) && defined(__svr4__)
	/*
	 * gethostname on solaris prior to 2.6 always returns 0 for
	 * udp sockets.  do a horrible kluge that often fails on
	 * multihomed hosts to get the local address (we don't use
	 * this to open the recv sock, only for the 'interface'
	 * tcl command).
	 */
	if (local_ == 0) {
		char myhostname[1024];
		int error;

		error = sysinfo(SI_HOSTNAME, myhostname, sizeof(myhostname));
		if (error == -1) {
			perror("Getting my hostname");
			exit(-1);
		}
		local_ = LookupHostAddr(myhostname);
	}
#endif
	lport_ = local.sin_port;
	last_reset_ = 0;
	return (0);
}

int IPNetwork::close()
{
	if (ssock_ >= 0) {
		::close(ssock_);
		::close(rsock_);
		ssock_ = rsock_ = -1;
	}
	return (0);
}

void IPNetwork::localname(sockaddr_in* p)
{
	memset((char *)p, 0, sizeof(*p));
	p->sin_family = AF_INET;
	int len = sizeof(*p);
	if (getsockname(ssock_, (struct sockaddr *)p, &len) < 0) {
		perror("getsockname");
		p->sin_addr.s_addr = 0;
		p->sin_port = 0;
	}
#ifdef WIN32
	if (p->sin_addr.s_addr == 0) {
		char hostname[80];
		struct hostent *hp;

		if (gethostname(hostname, sizeof(hostname)) >= 0) {
			if ((hp = gethostbyname(hostname)) >= 0) {
				p->sin_addr.s_addr = ((struct in_addr *)hp->h_addr)->s_addr;
			}
		}
	}
#endif	
}

void IPNetwork::reset()
{
	time_t t = time(0);
	int d = int(t - last_reset_);
	if (d > 3) {
		last_reset_ = t;
		(void)::close(ssock_);
		ssock_ = openssock(addr_, port_, ttl_);
	}
}

int IPNetwork::openrsock(u_int32_t addr, u_short port,
			    const struct sockaddr_in& local)
{
	int fd;
	struct sockaddr_in sin;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}
	nonblock(fd);
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
		perror("SO_REUSEADDR");
	}
#ifdef SO_REUSEPORT
	on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&on, sizeof(on)) < 0) {
		perror("SO_REUSEPORT");
		exit(1);
	}
#endif
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port   = htons(port);
	if (IN_CLASSD(ntohl(addr))) {
		/*
		 * Try to bind the multicast address as the socket
		 * dest address.  On many systems this won't work
		 * so fall back to a destination of INADDR_ANY if
		 * the first bind fails.
		 */
		sin.sin_addr.s_addr = htonl(addr);
		if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
			sin.sin_addr.s_addr = htonl(INADDR_ANY);
			if (bind(fd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
				perror("bind (INADDR_ANY)");
				abort();
			}
		}
		/* 
		 * XXX This is bogus multicast setup that really
		 * shouldn't have to be done (group membership should be
		 * implicit in the IP class D address, route should contain
		 * ttl & no loopback flag, etc.).  Steve Deering has promised
		 * to fix this for the 4.4bsd release.  We're all waiting
		 * with bated breath.
		 */
		struct ip_mreq mr;

		mr.imr_multiaddr.s_addr = addr;
		mr.imr_interface.s_addr = INADDR_ANY;
		if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mr, sizeof(mr)) < 0) {
			perror("IP_ADD_MEMBERSHIP");
			exit(1);
		}
	} else {
		/*
		 * bind the local host's address to this socket.  If that
		 * fails, another vic probably has the addresses bound so
		 * just exit.
		 */
		sin.sin_addr.s_addr = local.sin_addr.s_addr;
		if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
			perror("bind");
			exit(1);
		}
		/*
		 * Despite several attempts on our part to get this fixed,
		 * Microsoft Windows isn't complient with the Internet Host
		 * Requirements standard (RFC-1122) and won't let us include
		 * the source address in the receive socket demux state.
		 * (The consequence of this is that all conversations have
		 * to be assigned a unique local port so the vat 'side
		 * conversation' (middle click on site name) function is
		 * essentially useless under windows.)
		 */
#ifndef WIN32
		/*
		 * (try to) connect the foreign host's address to this socket.
		 */
		sin.sin_port = 0;
		sin.sin_addr.s_addr = addr;
		connect(fd, (struct sockaddr *)&sin, sizeof(sin));
#endif
	}
	/*
	 * XXX don't need this for the session socket.
	 */	
	int bufsize = 80 * 1024;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)) < 0) {
		bufsize = 32 * 1024;
		if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)) < 0) {
			perror("SO_RCVBUF");
		}
	}
	return fd;
}

int IPNetwork::openssock(u_int32_t addr, u_short port, int ttl)
{
	int fd;
	struct sockaddr_in sin;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}
	nonblock(fd);

#ifdef WIN32
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = INADDR_ANY;
	if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("bind");
		exit(1);
	}
#endif
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = port;
	sin.sin_addr.s_addr = addr;
	if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("connect");
		exit(1);
	}
	if (IN_CLASSD(ntohl(addr))) {
#ifdef IP_ADD_MEMBERSHIP
		char c;

		/* turn off loopback */
		c = 0;
		if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &c, 1) < 0) {
			/*
			 * If we cannot turn off loopback (Like on the
			 * Microsoft TCP/IP stack), then declare this
			 * option broken so that our packets can be
			 * filtered on the recv path.
			 */
			if (c == 0)
				noloopback_broken_ = 1;
		}
		/* set the multicast TTL */
#ifdef WIN32
		u_int t;
#else
		u_char t;
#endif
		t = (ttl > 255) ? 255 : (ttl < 0) ? 0 : ttl;
		if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL,
			       (char*)&t, sizeof(t)) < 0) {
			perror("IP_MULTICAST_TTL");
			exit(1);
		}
#else
		fprintf(stderr, "\
not compiled with support for IP multicast\n\
you must specify a unicast destination\n");
		exit(1);
#endif
	}
	/*
	 * XXX don't need this for the session socket.
	 */
	int bufsize = 80 * 1024;
	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize,
		       sizeof(bufsize)) < 0) {
		bufsize = 48 * 1024;
		if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize,
			       sizeof(bufsize)) < 0)
			perror("SO_SNDBUF");
	}
	return (fd);
}

