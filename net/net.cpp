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

#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#ifdef WIN32
//#include <winsock.h>
#else
#include <sys/param.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/uio.h>
#endif
#include "net.h"
#include "crypt.h"
#include "pktbuf.h"

#include "net-addr.h"

/*
 * Linux does not have sendmsg
 */
/* SV-XXX: Linux seems to have sendmsg now...
#if defined(__linux__) || defined(WIN32)
#define MAXPACKETSIZE (1500-28)

static int
sendmsg(int s, struct msghdr* mh, int flags)
{
	u_char wrkbuf[MAXPACKETSIZE];
	int len = mh->msg_iovlen;
	struct iovec* iov = mh->msg_iov;
	u_char* cp;
	u_char* ep;

	for (cp = wrkbuf, ep = wrkbuf + MAXPACKETSIZE; --len >= 0; ++iov) {
		int plen = iov->iov_len;
		if (cp + plen >= ep) {
			errno = E2BIG;
			return (-1);
		}
		memcpy(cp, iov->iov_base, plen);
		cp += plen;
	}
	return (send(s, (char*)wrkbuf, cp - wrkbuf, flags));
}
#endif
*/

/*
 * Address::operator==() -- compare two addresses
 */
int Address::operator==(const Address & addr) const {
	return (memcmp(*this, addr, length()) == 0);
}

/*
 * Address::alloc() -- allocate address with type based upon name
 */
Address * Address::alloc(const char * name) {
	return AddressType::alloc(name); 
}

/*
 * Address::defautl_alloc() -- allocate address with default type
 */
Address * Address::default_alloc() {
	char name[MAXHOSTNAMELEN];
	gethostname(name, sizeof(name));
	Address * result = alloc(name);
	return (result ? result : new Address());
}
int Address::is_set() {
	if (text_!=0) 
		if (*text_!='\0') return 1;
	return 0;
}

Network::Network() :
	g_addr_(*(Address::default_alloc())),
	s_addr_ssm_(*(Address::default_alloc())),
	local_(*(Address::default_alloc())),
	lport_(0),
	port_(0),
	ttl_(0),
	rsock_(-1),
	ssock_(-1),
	noloopback_broken_(0),
	crypt_(0)
{
}

Network::Network(Address & g_addr, Address & s_addr_ssm, Address & local) :
	g_addr_(g_addr),
	s_addr_ssm_(s_addr_ssm),
	local_(local),
	lport_(0),
	port_(0),
	ttl_(0),
	rsock_(-1),
	ssock_(-1),
	noloopback_broken_(0),
	crypt_(0)
{
}

Network::~Network()
{
	if (&g_addr_) delete &g_addr_; 
	if (&s_addr_ssm_) delete &s_addr_ssm_; 
	if (&local_) delete &local_;
}

int Network::command(int argc, const char*const* argv)
{
	if (argc == 2) {
		Tcl& tcl = Tcl::instance();
		char* cp = tcl.buffer();
		if (strcmp(argv[1], "addr") == 0 || 
		    strcmp(argv[1], "interface") == 0 ||
		    strcmp(argv[1], "port") == 0 ||
		    strcmp(argv[1], "ttl") == 0 ||
		    strcmp(argv[1], "ismulticast") == 0)
			strcpy(cp, "0");
		else
			return (TclObject::command(argc, argv));
		tcl.result(cp);
		return (TCL_OK);
	} else if (argc == 3) {
		if (strcmp(argv[1], "crypt") == 0) {
			/*
			 * 'crypt ""' will turn of encryption because
			 * lookup() will return null.
			 */
			crypt_ = (Crypt*)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return (TclObject::command(argc, argv));
}

void Network::nonblock(int fd)
{       
#ifdef WIN32
	u_long flag = 1;
	if (ioctlsocket(fd, FIONBIO, &flag) == -1) {
		fprintf(stderr, "ioctlsocket: FIONBIO: %lu\n", GetLastError());
		exit(1);
	}
#else
        int flags = fcntl(fd, F_GETFL, 0);
#if defined(hpux) || defined(__hpux)
        flags |= O_NONBLOCK;
#else
        flags |= O_NONBLOCK|O_NDELAY;
#endif
        if (fcntl(fd, F_SETFL, flags) == -1) {
                perror("fcntl: F_SETFL");
                exit(1);
        }
#endif
}

u_char* Network::wrkbuf_;
int Network::wrkbuflen_;

void Network::expand_wrkbuf(int len)
{
	if (wrkbuflen_ == 0)
		wrkbuf_ = (u_char*)malloc(len);
	else
		wrkbuf_ = (u_char*)realloc((u_char*)wrkbuf_, len);
	wrkbuflen_ = len;
}

void Network::dosend(u_char* buf, int len, int fd)
{
	int cc = ::send(fd, (char*)buf, len, 0);
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

void Network::send(u_char* buf, int len)
{
	if (crypt_)
		buf = crypt_->Encrypt(buf, len);
	dosend(buf, len, ssock_);
}

/*
 * Copy a scatter/gather packet to the work buffer and
 * return it's length.
 */
int Network::cpmsg(const msghdr& mh)
{
	int len, i;
	for (len = 0, i = 0; i < int(mh.msg_iovlen); ++i)
		len += mh.msg_iov[i].iov_len;

	if (len > wrkbuflen_)
		expand_wrkbuf(len);
	u_char* cp = wrkbuf_;
	for (i = 0; i < int(mh.msg_iovlen); ++i) {
		int cc = mh.msg_iov[i].iov_len;
		memcpy(cp, mh.msg_iov[i].iov_base, cc);
		cp += cc;
	}
	return (len);
}

void Network::send(const pktbuf* pb)
{
	/*XXX*/
	send(pb->dp, pb->len);
}
/*
void Network::send(const msghdr& mh)
{
	if (crypt_) {
		int cc = cpmsg(mh);
		send(wrkbuf_, cc);
		return;
	}
	int cc = ::sendmsg(ssock_, (msghdr*)&mh, 0);
	if (cc < 0) {
		switch (errno) {
		case ECONNREFUSED:
			// no one listening at some site - ignore *
#if defined(__osf__) || defined(_AIX)
			reset();
#endif
			break;

		case ENETUNREACH:
		case EHOSTUNREACH:
			//
			// * These "errors" are totally meaningless.
			// * There is some broken host sending
			// * icmp unreachables for multicast destinations.
			// * UDP probably aborted the send because of them --
			// * try exactly once more.  E.g., the send we
			// * just did cleared the errno for the previous
			// * icmp unreachable, so we should be able to
			// * send now.
			// *
			(void)::sendmsg(ssock_, (msghdr*)&mh, 0);
			break;

		default:
			//perror("sendmsg");*
			return;
		}
	}
}
*/


int Network::dorecv(u_char* buf, int len, u_int32_t& from, int fd)
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
	from = sfrom.sin_addr.s_addr;
	if (noloopback_broken_ &&  memcmp(local_, &from, fromlen) == 0
	    && sfrom.sin_port == lport_)
		return (0);

	return (cc);
}



int Network::recv(u_char* buf, int len, u_int32_t& from)
{
	if (crypt_) {
		if (len > wrkbuflen_)
			expand_wrkbuf(len);
		int cc = dorecv(wrkbuf_, len, from, rsock_);
		if (cc!=0) {
			return (crypt_->Decrypt(wrkbuf_, cc, buf));
		} else return 0;
	}
	return (dorecv(buf, len, from, rsock_));
}


int Network::recv(u_char* buf, int len, Address & from)
{
	if (crypt_) {
		if (len > wrkbuflen_)
			expand_wrkbuf(len);
		int cc = dorecv(wrkbuf_, len, from, rsock_);
		if (cc!=0) {
			return (crypt_->Decrypt(wrkbuf_, cc, buf));
		} else return 0;
	}
	return (dorecv(buf, len, from, rsock_));
}

void Network::reset()
{
}

