/*
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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <signal.h>

#include "net.h"
extern "C" {
#include "rcap.h"
#include "rt.h"
}
#include "vic_tcl.h"

#ifdef __osf__
extern "C" {
int getsockname(int, struct sockaddr*, int*);
int setsockopt (int, int, int, char*, int);
int socket(int, int, int);
int connect(int, struct sockaddr*, int);
}
#endif

class rtipChannel : public Network {
public:
	rtipChannel();
	~rtipChannel();
	int command(int argc, const char*const* argv);
protected:
	int open(u_int32_t addr, int port, rcapTraffic*, rcapRequirements*);
	int openssock(u_int32_t addr, u_short port,
		      rcapTraffic*, rcapRequirements*,
		      int& lcid) const;
	int openrsock(u_int32_t addr, u_short port, int& lcid) const;
	virtual int dorecv(u_char* buf, int len, u_int32_t& from, int fd);
	u_int32_t findifaddr(u_int32_t addr, u_short port) const;
	int rlcid_;
	int slcid_;
	/*XXX*/
	u_int32_t dst_;
};

static class RTIPNetworkMatcher : public Matcher {
public:
	RTIPNetworkMatcher() : Matcher("network") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "rtip") == 0)
			return (new rtipChannel);
		else
			return (0);
	}
} nm_rtip;

int rtipChannel::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "close") == 0) {
			close();
			return (TCL_OK);
		}
	} else if (argc == 14) {
		/*
		 * $net open dst port xmin xave I smax D J Z W U type
		 */
		if (strcmp(argv[1], "open") == 0) {
			u_int32_t addr = LookupHostAddr(argv[2]);
			if (addr == 0) {
				tcl.result("0");
				return (TCL_OK);
			}
			int port = htons(atoi(argv[3]));
			rcapTraffic rt;
			rcapRequirements rr;
			rt.xmin = atoi(argv[4]);
			rt.xave = atoi(argv[5]);
			rt.I = atoi(argv[6]);
			rt.smax = atoi(argv[7]);
			rr.D = atoi(argv[8]);
			rr.J = atoi(argv[9]);
			rr.Z = atoi(argv[10]);
			rr.W = atoi(argv[11]);
			rr.U = atoi(argv[12]);
			rr.type = atoi(argv[13]);

			if (open(addr, port, &rt, &rr) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
	}
	return (Network::command(argc, argv));
}

rtipChannel::rtipChannel() 
	: rlcid_(-1), slcid_(-1)
{
}

int rtipChannel::open(u_int32_t addr, int port,
		      rcapTraffic* traffic, rcapRequirements* requirements)
{
	u_int32_t local = findifaddr(addr, port);

	if (local < addr) {
		rsock_ = openrsock(addr, port, rlcid_);
		ssock_ = openssock(addr, port, traffic, requirements, slcid_);
	} else {
		ssock_ = openssock(addr, port, traffic, requirements, slcid_);
		rsock_ = openrsock(addr, port, rlcid_);
	}
	if (ssock_ >= 0)
		nonblock(ssock_);
	if (rsock_ >= 0)
		nonblock(rsock_);
		
	/*XXX*/
	dst_ = addr;
}

rtipChannel::~rtipChannel()
{
	if (slcid_ >= 0) {
		int st = RcapCloseRequest(u_short(slcid_), 0);
		if (st != 1)
			printf("rcap close: %d\n", st);
	}
	if (rlcid_ >= 0) {
		int st = RcapCloseRequest(u_short(rlcid_), 0);
		if (st != 1)
			printf("rcap close: %d\n", st);
	}
}

/*
 * Open an rtip datagram connection for output.  
 * addr is in network order; port is in host order.
 * XXX port should be in network order
 */
int rtipChannel::openssock(u_int32_t addr, u_short port,
			   rcapTraffic* rt, rcapRequirements* rr,
			   int& lcid_) const
{
	/*XXX bruce says this is host order */
	port = ntohs(port);

	fprintf(stderr, "vic: rtip-traffic: xmin %d xave %d I %d smax %d\n",
		rt->xmin, rt->xave, rt->I, rt->smax);
	fprintf(stderr, "vic: rtip-req: D %d J %d Z %d W %d U %d type %d\n",
		rr->D, rr->J, rr->Z, rr->W, rr->U, rr->type);

	rcapAddress dst;
	dst.ipAddr.s_addr = addr;
	dst.port = port;

	rcapUserControl control;
	control.userControl = NULL;	/* no user control data */
	control.userControlLength = 0;

	parmblock pb;
	pb.traffic = rt;
	pb.requirements = rr;
	pb.destination = &dst;
	pb.control = &control;

	/* XXX error structure not right here */
	/*
	 * Do the channel establishment using the parameters initialized
	 * above.
	 */
	fprintf(stderr, "vic: waiting for rcap sender connect to %s\n",
		intoa(addr));
	u_short lcid;
 again:
	int rc = RcapEstablishRequest(&pb, &lcid, (struct in_addr *)&dst);
	if (rc == RCAP_ERROR_UNREGISTERED_PORT) {
		fprintf(stderr, "vic: no rcap receiver at destination\n");
		fprintf(stderr, "vic: trying to connect again...\n");
		sleep(1);
		goto again;
	}
	if (rc != RCAP_SUCCESS) {
		fprintf(stderr, "vic: RcapEstablishRequest (rcap errno %d)\n",
			rc);
		exit(1);
	}
	lcid_ = lcid;
	/*
	 * Now get a real-time socket and create the socket-to-channel
	 * association using a socket option.
	 */
	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_RMTP);
	if (s < 0) {
		perror("vic: socket");
		return (-1);
	}
	if (setsockopt(s, IPPROTO_RTIP, RTIP_ASSOC, (char *)&lcid,
		       sizeof(lcid)) < 0) {
		perror("vic: setsockopt");
		return (-1);
	}
	fprintf(stderr, "vic: rcap: sender connection established\n");

	int bufsize = 32 * 1024;
	if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize,
			sizeof(bufsize)) < 0)
		perror("vic: setsockopt (SO_SNDBUF)");

	return (s);
}

int rtipChannel::openrsock(u_int32_t addr, u_short port, int& lcid_) const
{
	/*XXX bruce says this is host order */
	port = ntohs(port);

	/*
	 * RCAP-related initialization...register application on a port.
	 */
	int rc = RcapRegister(port, 1);
	if (rc != RCAP_SUCCESS) {
		fprintf(stderr, "vic: RcapRegister failed\n");
		exit(1);
	}

	parmblock pb;
	rcapTraffic traffic;
	rcapRequirements requirements;
	rcapAddress src;
	rcapUserControl control;
	pb.traffic = &traffic;
	pb.requirements = &requirements;
	pb.control = &control;
	/*
	 * Get next channel establishment request.  If there are not
	 * any, we block in the library call until we get one.
	 */
	fprintf(stderr, "vic: waiting for rcap receiver connect to %s\n",
		intoa(addr));
	/*XXX ignore return values? */
	u_short lcid;
	(void)RcapReceiveRequest(port, &pb, &src, &lcid);
	(void)RcapEstablishReturn(lcid, RCAP_SUCCESS, 0, &control);
	lcid_ = lcid;

	/*
	 * Create a new data socket and associate it with the
	 * newly-created channel.  Also do a bit of bookkeeping
	 * so we knew what sockets to look for.
	 */
	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_RMTP);
	if (s < 0) {
		perror("socket");
		return (-1);
	}
	if (setsockopt(s, IPPROTO_RTIP, RTIP_DESTINATION_ASSOC,
		       (char *)&lcid, sizeof(lcid)) < 0) {
		perror("setsockopt");
	}
	/*XXX*/
	int rbuf = 40 * 1024;
	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&rbuf,
			sizeof(rbuf)) < 0) {
		perror("vic: SO_RCVBUF");
		exit(1);
	}
	fprintf(stderr, "vic: rcap: receiver connection established\n");
	return (s);
}

/* XXX find the address of the interface that we will connect to */
u_int32_t rtipChannel::findifaddr(u_int32_t addr, u_short port) const
{
	/*XXX bruce says this is host order */
	port = ntohs(port);

	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("vic: socket");
		exit(1);
	}
	sockaddr_in sin;
        memset((char *)&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        sin.sin_addr.s_addr = addr;
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
                perror("vic: connect");
                exit(1);
        }
	sockaddr_in local;
	int sinlen = sizeof(local);
        if (getsockname(s, (struct sockaddr *)&local, &sinlen) ||
            local.sin_addr.s_addr == 0)
                local.sin_addr.s_addr = LookupLocalAddr();
	close(s);
	
	return (local.sin_addr.s_addr);
}

int rtipChannel::dorecv(u_char* buf, int len, u_int32_t& from, int fd)
{
	int cc = Network::dorecv(buf, len, from, fd);
	/*XXX workaround until rtip/recvfrom() gets fixed! */
	from = dst_;

	return (cc);
}
