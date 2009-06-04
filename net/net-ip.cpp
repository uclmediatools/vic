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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#define close closesocket
extern "C" {
char *find_win32_interface(const char *addr, int ttl);
}
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
#include "vic_tcl.h"

#include "net-addr.h"

#ifndef INET_ADDRSTRLEN
// IPv4 Address len = 4*3(addr bytes)+3(dots)+1(null terminator)=16
#define INET_ADDRSTRLEN (16)
#endif

class IPAddress : public Address {
public:
	IPAddress() { text_ = new char[INET_ADDRSTRLEN]; text_[0]='\0';}
  	virtual int operator=(const char* text);
	int operator=(const struct in_addr& addr);

	virtual Address* copy() const;
	virtual size_t length() const { return sizeof(addr_); }
	virtual operator const void*() const { return &addr_; }

	operator struct in_addr() const { return addr_; }
	operator const struct in_addr&() const { return addr_; }
	operator u_int32_t() const { return addr_.s_addr; }

private:
	struct in_addr addr_;
};


static class IPAddressType : public AddressType {
public:
  virtual Address* resolve(const char* name) {
    struct in_addr addr;
    IPAddress * result = 0;
    if((addr.s_addr = LookupHostAddr(name)) != 0) {
      result = new IPAddress;
      *result = addr;
    } else {
      result = 0;
    }
    return (result);
  }
} ip_address_type;


class IPNetwork : public Network {
    public:
		IPNetwork() : Network(*(new IPAddress), *(new IPAddress), *(new IPAddress)) {;}
	virtual int command(int argc, const char*const* argv);
	virtual void reset();
	virtual Address* alloc(const char* name) { 
    		struct in_addr addr;
    		IPAddress * result = 0;
    		if((addr.s_addr = LookupHostAddr(name)) != 0) {
    		  result = new IPAddress;
    		  *result = addr;
    		} else {
    		  result = 0;
    		}
    		return (result);
  	}
    protected:
	struct sockaddr_in sin;
	virtual int dorecv(u_char* buf, int len, Address &from, int fd);
	int open(const char * host, int port, int ttl);
	int close();
	int localname(sockaddr_in*);
	int openssock(Address & addr, u_short port, int ttl);
	int openrsock(Address & g_addr, Address & s_addr_ssm, u_short port, Address & local);
	void dosend(u_char* buf, int len, int fd);
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


Address * IPAddress::copy() const {
  IPAddress * result = new IPAddress;
  *result = addr_;
  return (result);
}

int IPAddress::operator=(const char* text) {
  addr_.s_addr = LookupHostAddr(text);
  strcpy(text_, intoa(addr_.s_addr));
  return (0);
}

int IPAddress::operator=(const struct in_addr& addr) {
  memcpy(&addr_, &addr, sizeof(addr));
  strcpy(text_, intoa(addr_.s_addr));
  return (0);
}

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
			strcpy(cp, g_addr_);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "interface") == 0) {
			strcpy(cp, local_);
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
			u_int32_t addri = (IPAddress&)g_addr_;
			tcl.result(IN_CLASSD(ntohl(addri))? "1" : "0");
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
			int port = htons(atoi(argv[3]));
			int ttl = atoi(argv[4]);
			if (strlen(tcl.attr("ifAddr"))>1)
				(IPAddress&)local_ = tcl.attr("ifAddr");
			if (open(argv[2], port, ttl) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
	}
	return (Network::command(argc, argv));
}

int IPNetwork::open(const char * host, int port, int ttl)
{
	char *g_addr;
	// Check for SSM src address: Src,Group
	if ((g_addr=(char*)strchr(host,(int)','))!=NULL) {
		char s_addr_ssm[MAXHOSTNAMELEN];
		int i=0;
		while (&host[i]<g_addr) {
			s_addr_ssm[i]=host[i];
                        i++;
		}
		s_addr_ssm[i]='\0';
		g_addr_=++g_addr;
		s_addr_ssm_=s_addr_ssm;
	} else {
		// No SSM address found - just use group host address
		g_addr_=host;
	}
	port_ = port;
	ttl_ = ttl;


	ssock_ = openssock(g_addr_, port, ttl);
	if (ssock_ < 0)
		return (-1);
	/*
	 * Connecting the send socket also bound the local address.
	 * On a multihomed host we need to bind the receive socket
	 * to the same local address the kernel has chosen to send on.
	 */
	sockaddr_in local;
	if (localname(&local) < 0) {
		return (-1);
	}
	(IPAddress&)local_ = local.sin_addr;

#ifdef WIN32
	//if (!local_.is_set()) {
		(IPAddress&)local_ = find_win32_interface(g_addr_, ttl);
		//(IPAddress&)local_ = "127.0.0.1";
		debug_msg("find_win32_interface localname:%s\n",(const char*)local_);
	//}
#endif

	rsock_ = openrsock(g_addr_, s_addr_ssm_, port, local_);
	if (rsock_ < 0) {
		rsock_ = ssock_;
	}

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

int IPNetwork::localname(sockaddr_in* p)
{
	memset((char *)p, 0, sizeof(*p));
	p->sin_family = AF_INET;
#ifndef WIN32
	unsigned int len = sizeof(*p); int result =0; //SV-XXX: redefined "result" as int avoids gcc4 warning further down (see if)
#else
	int len = sizeof(*p), result =0;
#endif

	// Use Local interface name if already set via command line
	if (local_.is_set()) {
		p->sin_addr.s_addr=(IPAddress&)local_;
		debug_msg("Setting localname from cmd line:%s\n",(const char*)local_);
		return (0);
	}

	if ((result = getsockname(ssock_, (struct sockaddr *)p, &len)) < 0) {
		perror("getsockname");
		p->sin_addr.s_addr = 0;
		p->sin_port = 0;
	} else 
	    debug_msg("getsockname localname:%s\n",(const char*)local_);

	if (p->sin_addr.s_addr == 0) {
		p->sin_addr.s_addr = LookupLocalAddr();
		result = ((p->sin_addr.s_addr != 0) ? (0) : (-1));
	}

	return (result);
}

void IPNetwork::reset()
{
	time_t t = time(0);
	int d = int(t - last_reset_);
	if (d > 3) {
		last_reset_ = t;
		(void)::close(ssock_);
		ssock_ = openssock(g_addr_, port_, ttl_);
	}
}

int IPNetwork::openrsock(Address & g_addr, Address & s_addr_ssm, u_short port, Address & local)
{
	int fd;
	struct sockaddr_in sin;

	u_int32_t g_addri = (IPAddress&)g_addr;
	u_int32_t g_addri_ssm = (IPAddress&)s_addr_ssm;
	u_int32_t locali = (IPAddress&)local;

	Tcl tcl = Tcl::instance();
	const char *noBindStr;

	noBindStr = tcl.attr("noMulticastBind");
        if(noBindStr != NULL && strcasecmp(noBindStr,"true") == 0) {
            return -1;
        }

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}
	nonblock(fd);
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
			sizeof(on)) < 0) {
		perror("SO_REUSEADDR");
	}
#ifdef SO_REUSEPORT
	on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&on,
		       sizeof(on)) < 0) {
		perror("SO_REUSEPORT");
		exit(1);
	}
#endif
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = port;
#ifdef IP_ADD_MEMBERSHIP
	if (IN_CLASSD(ntohl(g_addri))) {
		/*
		 * Try to bind the multicast address as the socket
		 * dest address.  On many systems this won't work
		 * so fall back to a destination of INADDR_ANY if
		 * the first bind fails.
		 */
		sin.sin_addr.s_addr = g_addri;
		if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
			sin.sin_addr.s_addr = INADDR_ANY;
			if (bind(fd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
				perror("bind");
				exit(1);
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

		/* SSM code */
#ifdef IP_ADD_SOURCE_MEMBERSHIP  
        struct ip_mreq_source mrs;
		/* Check if an Src addr - as in S,G has been set */
        if (s_addr_ssm.is_set()) {
                mrs.imr_sourceaddr.s_addr = g_addri_ssm;
                mrs.imr_multiaddr.s_addr = g_addri;
                mrs.imr_interface.s_addr = INADDR_ANY;
                if (setsockopt(fd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP,
                                (char*)&mrs, sizeof(mrs)) < 0) {
                        perror("IP_ADD_SOURCE_MEMBERSHIP");
                        exit (1);
                }
        } else
                        
#endif /* IP_ADD_SOURCE_MEMBERSHIP */
		{
				/* 
				* XXX This is bogus multicast setup that really
				* shouldn't have to be done (group membership should be
				* implicit in the IP class D address, route should contain
				* ttl & no loopback flag, etc.).  Steve Deering has promised
				* to fix this for the 4.4bsd release.  We're all waiting
				* with bated breath.
				*/
				struct ip_mreq mr;

				mr.imr_multiaddr.s_addr = g_addri;
				mr.imr_interface.s_addr = locali;
				if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
						(char *)&mr, sizeof(mr)) < 0) {
					perror("IP_ADD_MEMBERSHIP");
					exit(1);
				}
		}
	} else
#endif /* IP_ADD_MEMBERSHIP */
	{
		/*
		 * bind the local host's address to this socket.  If that
		 * fails, another vic probably has the addresses bound so
		 * just exit.
		 */
		sin.sin_addr.s_addr = locali;
		if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
			perror("bind");
            ::close(fd);
            return (-1);
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
		sin.sin_addr.s_addr = g_addri;
		connect(fd, (struct sockaddr *)&sin, sizeof(sin));
#endif
	}
	/*
	 * XXX don't need this for the session socket.
	 */	
	int bufsize = 80 * 1024;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize,
			sizeof(bufsize)) < 0) {
		bufsize = 32 * 1024;
		if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize,
				sizeof(bufsize)) < 0)
			perror("SO_RCVBUF");
	}
	return (fd);
}

int IPNetwork::openssock(Address & addr, u_short port, int ttl)
{
	int fd;
//	struct sockaddr_in sin;

	u_int32_t addri = (IPAddress&)addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}
	nonblock(fd);

        if (IN_CLASSD(ntohl(addri))) {
                int on = 1;
                if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
                                sizeof(on)) < 0) {
                        perror("SO_REUSEADDR");
                }
        }
#ifdef SO_REUSEPORT
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&on,
		       sizeof(on)) < 0) {
		perror("SO_REUSEPORT");
		exit(1);
	}
#endif

	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = port;
	sin.sin_addr.s_addr = INADDR_ANY;
	if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        sin.sin_port = 0;
        if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            perror("bind");
            exit(1);
        }
	}

	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = port;
	sin.sin_addr.s_addr = addri;

/*	Got rid of connect and vic then uses sin in the sendto() function in
 *	the dosend() method
 *
 *	if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("connect");
		exit(1);
	}*/
	if (IN_CLASSD(ntohl(addri))) {
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
		/* Slightly nasty one here - set Mcast iface if local inteface
		 * is specified on command line
		 */
		if (local_.is_set()) {
			u_int32_t locali = (IPAddress&)local_;
			if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF,
						   (char*)&locali, sizeof(locali)) < 0) {
				perror("IP_MULTICAST_IF");
			}
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



int IPNetwork::dorecv(u_char* buf, int len, Address & from, int fd)
{
	sockaddr_in sfrom;
#ifndef WIN32
	unsigned int fromlen = sizeof(sfrom);
#else
	int fromlen = sizeof(sfrom);
#endif
	int cc = ::recvfrom(fd, (char*)buf, len, 0,
			    (sockaddr*)&sfrom, &fromlen);
	if (cc < 0) {
		if (errno != EWOULDBLOCK)
			perror("recvfrom");
		return (-1);
	}
	(IPAddress&)from = sfrom.sin_addr;

	if (noloopback_broken_ && from == local_ && sfrom.sin_port == lport_)
		return (0);

	return (cc);
}

void IPNetwork::dosend(u_char* buf, int len, int fd)
{
	int cc = ::sendto(fd, (char*)buf, len, 0, (struct sockaddr *)&sin, sizeof(sin));
	if (cc < 0) {
		switch (errno) {
		case ECONNREFUSED:
			/* no one listening at some site - ignore */
#if defined(__osf__) || defined(_AIX)
			/*
			 * Due to a bug in kern/uipc_socket.c, on several
			 * systems, datagram sockets incorrectly persist
			 * in an error state on receipt of an ICMP
			 * port-unreachable.  This causes unicast connection
			 * rendezvous problems, and worse, multicast
			 * transmission problems because several systems
			 * incorrectly send port unreachables for 
			 * multicast destinations.  Our work around
			 * is to simply close and reopen the socket
			 * (by calling reset() below).
			 *
			 * This bug originated at CSRG in Berkeley
			 * and was present in the BSD Reno networking
			 * code release.  It has since been fixed
			 * in 4.4BSD and OSF-3.x.  It is know to remain
			 * in AIX-4.1.3.
			 *
			 * A fix is to change the following lines from
			 * kern/uipc_socket.c:
			 *
			 *	if (so_serror)
			 *		snderr(so->so_error);
			 *
			 * to:
			 *
			 *	if (so->so_error) {
			 * 		error = so->so_error;
			 *		so->so_error = 0;
			 *		splx(s);
			 *		goto release;
			 *	}
			 *
			 */
			reset();
#endif
			break;

		case ENETUNREACH:
		case EHOSTUNREACH:
			/*
			 * These "errors" are totally meaningless.
			 * There is some broken host sending
			 * icmp unreachables for multicast destinations.
			 * UDP probably aborted the send because of them --
			 * try exactly once more.  E.g., the send we
			 * just did cleared the errno for the previous
			 * icmp unreachable, so we should be able to
			 * send now.
			 */
			(void)::send(ssock_, (char*)buf, len, 0);
			break;

		default:
			/*perror("send");*/
			return;
		}
	}
}
