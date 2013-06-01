/*
 * FILE:    mbus_handler.c
 * AUTHORS: Colin Perkins, Piers O'Hanlon
 * Modified by Dimitrios Miras
 * 
 * Copyright (c) 1997,1999 University College London
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

/*#ifndef WIN32
#include <strings.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
  */
#include "mbus_handler.h"

MBusHandler::MBusHandler(void (*cmd_handler)(char *src, char *cmd, char *arg, void *dat), 
		       void (*err_handler)(int seqnum, int reason))
{
	char mbus_vic_addr[100];

        sprintf(mbus_vic_addr, "(media:video module:engine app:vic instance:%lu)", (long unsigned)getpid()); //SV-XXX: was: (u_int32_t) getpid()); check windows to see what happens...
	mbusp_ = mbus_init(cmd_handler, err_handler, mbus_vic_addr);
	//unlink();

        if (mbusp_) {
          debug_msg("Mbus enabled\n");
	  // Schedule a heartbeat
          msched(50);
        } else
          debug_msg("Mbus DISABLED - Probably due to multicast/Network problem\n");
}

void MBusHandler::listen_audio() {
	mbus_audio_addr = strdup("(media:audio module:engine)");
}



MBusHandler::~MBusHandler()
{
	if (mbusp_) mbus_exit(mbusp_);
}


void MBusHandler::dispatch(int)
{
	//mbus_recv(mbusp_, this);
}


void MBusHandler::timeout()
{
	//SV-XXX: double now = gettimeofday();
	struct timeval     timeout;

	timeout.tv_sec  = 0;
	timeout.tv_usec = 0;

	if (mbusp_ == NULL) return;
	mbus_send(mbusp_);
	mbus_recv(mbusp_, (void *) this, &timeout);
	mbus_retransmit(mbusp_);
	mbus_heartbeat(mbusp_, 1);
	mbus_send(mbusp_);

	msched(50);

	return;
}
