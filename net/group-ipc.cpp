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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#ifndef WIN32
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
//#include <winsock.h>
#include <io.h>
#define close closesocket
#else
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#endif
#include <fcntl.h>

#include "group-ipc.h"

#define GROUP_IPC_ADDR 0xe0ffdeef
#define GROUP_IPC_PORT 0xdeaf

static void nonblock(int fd)
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

IPCHandler::IPCHandler(int mask) : mask_(mask) { }

#ifdef IP_ADD_MEMBERSHIP
GroupIPC::GroupIPC(int channel) : handlers_(0)
{
	rsock_ = socket(AF_INET, SOCK_DGRAM, 0);
	if (rsock_ < 0) {
		perror("socket");
		exit(1);
	}
	int on = 1;
	if (setsockopt(rsock_, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
		       sizeof(on)) < 0) {
		perror("SO_REUSEADDR");
		exit(1);
	}
#ifdef SO_REUSEPORT
	on = 1;
	if (setsockopt(rsock_, SOL_SOCKET, SO_REUSEPORT, (char *)&on,
		       sizeof(on)) < 0) {
		perror("SO_REUSEPORT");
		exit(1);
	}
#endif
	sockaddr_in sin;
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	int port = htons(GROUP_IPC_PORT + channel);
	sin.sin_port = port;
	sin.sin_addr.s_addr = htonl(GROUP_IPC_ADDR);
	if (bind(rsock_, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		sin.sin_addr.s_addr = INADDR_ANY;
		if (bind(rsock_, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
			perror("GroupIPC bind");
			close(rsock_);
			rsock_ = -1;
			ssock_ = -1;
		}
	}
	if (rsock_ >= 0) {
		nonblock(rsock_);

		struct ip_mreq mr;
		mr.imr_multiaddr.s_addr = htonl(GROUP_IPC_ADDR);
		mr.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(rsock_, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			       (char *)&mr, sizeof(mr)) < 0) {
			/*
			 * host probably doesn't have multicast
			 */
			close(rsock_);
			rsock_ = -1;
			ssock_ = -1;
		}
	} 
	if (rsock_ >= 0) {
		link(rsock_, TK_READABLE);
		ssock_ = socket(AF_INET, SOCK_DGRAM, 0);
		if (ssock_ < 0) {
			perror("GroupIPC socket");
			exit(1);
		}
		sin.sin_port = 0;
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(ssock_, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
			perror("GroupIPC bind");
			exit(1);
		}
		sin.sin_port = port;
		sin.sin_addr.s_addr = htonl(GROUP_IPC_ADDR);
		if (connect(ssock_, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
			perror("GroupIPC connect");
			exit(1);
		}
#ifdef WIN32
		u_int ttl=0;
#else
		u_char ttl=0;
#endif
		if (setsockopt(ssock_, IPPROTO_IP, IP_MULTICAST_TTL,
			       (char*)&ttl, sizeof(ttl)) < 0) {
			perror("GroupIPC: IP_MULTICAST_TTL");
			exit(1);
		}
		u_int32_t addr = htonl(INADDR_ANY);
		if (setsockopt(ssock_, IPPROTO_IP, IP_MULTICAST_IF,
				(char *)&addr, 4) < 0) {
			perror("GroupIPC: IP_MULTICAST_IF");
			exit(1);
		}
#ifdef ultrix
		ttl = 1;
		if (setsockopt(ssock_, IPPROTO_IP, IP_MULTICAST_LOOP,
				&ttl, 1) < 0) {
			perror("GroupIPC: IP_MULTICAST_LOOP");
			exit(1);
		}
#endif
	}
	pid_ = getpid();
}
#else
GroupIPC::GroupIPC(int channel) : handlers_(0), rsock_(-1), ssock_(-1)
{
}
#endif

GroupIPC::~GroupIPC()
{
	if (rsock_ >= 0)
		(void)close(rsock_);
	if (ssock_ >= 0)
		(void)close(ssock_);
}

void GroupIPC::send(int type, int len)
{
	ipchdr* h = (ipchdr*)buffer_;
	h->type = type;
	h->pid = pid_;
	h->magic = GIPC_MAGIC;
	len += sizeof(*h);
	if (ssock_ >= 0 && ::send(ssock_, (char*)buffer_, len, 0) < 0)
		perror("GroupIPC send");
}

void GroupIPC::send(int type, const char* msg)
{
	strcpy((char*)buffer(), msg);
	/* include null terminator */
	send(type, strlen(msg) + 1);
}

void GroupIPC::attach(IPCHandler *h)
{
	h->next_ = handlers_;
	handlers_ = h;
}

void GroupIPC::dispatch(int)
{
	int cc = recv(rsock_, (char*)buffer_, sizeof(buffer_), 0);
	if (cc < 0) {
		perror("GroupIPC: recv");
		return;
	}
	cc -= sizeof(ipchdr);
	if (cc < 0)
		return;

	ipchdr* h = (ipchdr*)buffer_;
	if (h->pid == pid_)
		return;
		
	int bit = 1 << h->type;
	for (IPCHandler* p = handlers_; p != 0; p = p->next_) {
		if (p->mask() & bit)
			p->ipc_input(h->type, h->pid, (u_char*)(h + 1), cc);
	}
}
