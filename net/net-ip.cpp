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
		IPNetwork() : Network(*(new IPAddress), *(new IPAddress), *(new IPAddress)), local_preset_(0) {
			tvlen = sizeof(struct timeval);
		}
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
	struct sockaddr_in sin_; // sockaddr setup in ssock, used by sendto in dosend
	time_t last_reset_;
	int local_preset_; // indicates if local_ has been set on cmd line
	virtual int dorecv(u_char* buf, int len, Address &from, int fd);
	int open(const char * host, int port, int ttl);
	int close();
	int localname(sockaddr_in*);
	int openssock(Address & addr, u_short port, int ttl);
	int disconnect_sock(int fd);
	int openrsock(Address & g_addr, Address & s_addr_ssm, u_short port, Address & local);
	void dosend(u_char* buf, int len, int fd);
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
			if (strlen(tcl.attr("ifAddr"))>1) {
				(IPAddress&)local_ = tcl.attr("ifAddr");
				local_preset_ = 1;
			}
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
	sockaddr_in local;
	int on;

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
	if (localname(&local) < 0) {
#ifdef WIN32
		(IPAddress&)local_ = find_win32_interface(g_addr_, ttl);
		debug_msg("find_win32_interface localname:%s\n",(const char*)local_);
#endif
		if (local.sin_addr.s_addr == 0) {
			(IPAddress&)local_ = "127.0.0.1";
			printf("Can NOT determine local IP address - using loopback address. If you want to be able to receive packets from other machines add this command line option: -i local_ip_addr \n");
		}
	} else
		(IPAddress&)local_ = local.sin_addr;

	disconnect_sock(ssock_);
	rsock_ = openrsock(g_addr_, s_addr_ssm_, port, local_);
	if (rsock_ < 0) {
		rsock_ = ssock_;
	}
	/*
	 * Enable the TOS value from received packets to be
	 * returned along with the payload.
	 */
#ifdef IP_RECVTOS
	on = 1;
	debug_msg("Enabling IP_RECVTOS on recv socket\n");
	if (setsockopt(rsock_, IPPROTO_IP, IP_RECVTOS, (char *)&on, 
		sizeof(on)) < 0) {
		perror("IP_RECVTOS");
		exit(1);
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

int IPNetwork::localname(sockaddr_in* p)
{
	memset((char *)p, 0, sizeof(*p));
	p->sin_family = AF_INET;
#ifndef WIN32
	unsigned int len = sizeof(*p); int result =0; //SV-XXX: redefined "result" as int avoids gcc4 warning further down (see if)
#else
	int len = sizeof(*p), result =0;
#endif

	if ((result = getsockname(ssock_, (struct sockaddr *)p, &len)) < 0) {
		perror("getsockname");
		p->sin_addr.s_addr = 0;
		p->sin_port = 0;
	}
	// Use Local interface name if already set via command line
	if (local_preset_) {
		printf("localname:Using manually assigned src address: %s\n", (const char*)local_);
		p->sin_addr.s_addr=(IPAddress&)local_;
		return (0);
	}

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
		disconnect_sock(ssock_);
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
/*
 * SO_TIMESTAMP
 */
#ifdef SO_TIMESTAMP
	on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, (char *)&on, 
			sizeof(on)) < 0) {
		perror("SO_TIMESTAMP");
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
				if (local_preset_) {
					mr.imr_interface.s_addr = locali;
					if (setsockopt(fd, IPPROTO_IP,IP_ADD_MEMBERSHIP,
								(char *)&mr, sizeof(mr)) < 0) {
						perror("IP_ADD_MEMBERSHIP");
						debug_msg("Failed to join multicast group using preset local addr?\n");
					}
				} else {
					mr.imr_interface.s_addr = INADDR_ANY;
					if (setsockopt(fd, IPPROTO_IP,IP_ADD_MEMBERSHIP,
								(char *)&mr, sizeof(mr)) < 0) {
						perror("IP_ADD_MEMBERSHIP");
						debug_msg("Failed to join multicast group- exiting\n");
						exit(1);
					}
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

	int on = 1;
	if (IN_CLASSD(ntohl(addri))) {
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
			sizeof(on)) < 0) {
			perror("SO_REUSEADDR");
		}
	}

#ifdef SO_REUSEPORT
	on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&on,
		sizeof(on)) < 0) {
		perror("SO_REUSEPORT");
		exit(1);
	}
#endif

	memset((char *)&sin_, 0, sizeof(sin_));
	sin_.sin_family = AF_INET;
	sin_.sin_port = port;
	// Use Local interface name if already set via command line
	if (local_preset_) {
		printf("Openssock:Using manually assigned src address: %s\n", (const char*)local_);
		sin_.sin_addr.s_addr=(IPAddress&)local_;
	} else 
		sin_.sin_addr.s_addr = INADDR_ANY;
	if (bind(fd, (struct sockaddr *)&sin_, sizeof(sin_)) < 0) {
		sin_.sin_port = 0;
		if (bind(fd, (struct sockaddr *)&sin_, sizeof(sin_)) < 0) {
		  perror("bind");
		  exit(1);
		}
	}

#ifdef SO_TIMESTAMP
	on = 1;
	if(setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, (char *)&on, 
		sizeof(on)) < 0) {
		perror("SO_TIMESTAMP");
		exit(1);
	}
#endif

#ifdef IP_TOS // This can work unidirectionally with Sender ECT only
        on = ECT0; //Preferred ECT(0) = 10(binary)  Whilst ECT(1) = 01
	debug_msg("Setting IP_TOS on send socket:%d\n",on);
	if(setsockopt(fd, IPPROTO_IP, IP_TOS, (char *)&on,
		sizeof(on)) < 0) {
		perror("IP_TOS");
		exit(1);
	}
#endif
	memset((char *)&sin_, 0, sizeof(sin_));
	sin_.sin_family = AF_INET;
	sin_.sin_port = port;
	sin_.sin_addr.s_addr = addri;

    /* Connect() is useful for localname() to find the interface addr
	 * being used. Also because of a problem with OSX we disconnect
	 * this socket once localname() has found out the ip addr of the iface 
	 */
	if (connect(fd, (struct sockaddr *)&sin_, sizeof(sin_)) < 0) {
		perror("connect");
		exit(1);
	}
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
		if (local_preset_) {
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

int IPNetwork::disconnect_sock(int fd)
{
	struct sockaddr_in sin;

	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_UNSPEC;
	return connect(fd, (struct sockaddr *)&sin, sizeof(sin));
}


int IPNetwork::dorecv(u_char* buf, int len, Address & from, int fd)
{
	sockaddr_in sfrom;
#ifndef WIN32
	unsigned int fromlen = sizeof(sfrom);
#else
	int fromlen = sizeof(sfrom);
#endif

	int cc = 0;
	bool found = false;

	struct msghdr mh;
	struct iovec iov;
	unsigned char cbuf[1024];
	//unsigned char cbuf[CMSG_SPACE(tvlen)];
	struct cmsghdr *cm = (struct cmsghdr *)&cbuf;

#ifdef NORECVMSG 
	cc = ::recvfrom(fd, (char*)buf, len, 0, (sockaddr*)&sfrom, &fromlen);
	//debug_msg("recvfrom successful: %d\n", cc);
	if (cc < 0) {
		if (errno != EWOULDBLOCK)
			perror("recvfrom");
		return (-1);
	}
#else

	(void)memset(&mh, 0, sizeof(mh));
	(void)memset(&iov, 0, sizeof(iov));

	iov.iov_base = buf;	// buffer for packet payload
	iov.iov_len = len;	// expected packet length

	mh.msg_name = (sockaddr_in *)&sfrom;
	mh.msg_namelen = sizeof(sfrom);
	mh.msg_iov = &iov;
	mh.msg_iovlen = 1;
	mh.msg_control = (caddr_t)cbuf;
	mh.msg_controllen = sizeof(cbuf);
	//mh.msg_flags = 0;

	// receive SO_TIMESTAMP
	if ((cc = ::recvmsg(fd, &mh, 0)) == -1) {
		//printf("recvmsg unsuccessful: %d\n", cc);
		return (cc);
	}

	for (cm = CMSG_FIRSTHDR(&mh); cm != NULL; cm = CMSG_NXTHDR(&mh, cm)) {
	    switch (cm->cmsg_level) {
#ifdef SO_TIMESTAMP
		case SOL_SOCKET:
		    if (cm->cmsg_type == SCM_TIMESTAMP
			    && cm->cmsg_len == CMSG_LEN(tvlen)) {
			found = true;
			memcpy(&tvrecv, CMSG_DATA(cm), tvlen);
			debug_msg("FOUND TIMESTAMP : %f\n",
				(double) tvrecv.tv_sec + 1e-6 * (double) tvrecv.tv_usec);
			break;
		    }
#endif
#ifdef IP_RECVTOS
		case IPPROTO_IP:
		    if (cm->cmsg_type == IP_TOS ) {
			//cm->cmsg_len == CMSG_LEN(sizeof(struct in_addr))) {
			found = true;
			recv_tos_ = *(uint8_t *)CMSG_DATA(cm);
			debug_msg("FOUND TOS: %d\n", recv_tos_);
			break;
		    } 
#endif /* RECVTOS */
		default:
		    debug_msg("recvmsg problem: no IP_TOS nor TIMESTAMP found (type:%d,len:%d, Level:%d)\n", cm->cmsg_type, cm->cmsg_len, cm->cmsg_level);
	    }
	}
	if (cm == NULL && !found)
		debug_msg("recvmsg problem: no cm\n");
#endif /* NORECVMSG */

	(IPAddress&)from = sfrom.sin_addr;

	if (noloopback_broken_ && from == local_ && sfrom.sin_port == lport_)
		return (0);

	return (cc);
}

void IPNetwork::dosend(u_char* buf, int len, int fd)
{
	int cc = ::sendto(fd, (char*)buf, len, 0, (struct sockaddr *)&sin_, sizeof(sin_));
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
