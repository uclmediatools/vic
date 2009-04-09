/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is was contributed by Anastasio Andrea Scalisi
 * (scalisi@mailer.cefriel.it), of CEFRIEL, Milan, Italy.
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
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/uio.h>
#include "net.h"
#include <fore_atm/fore_atm_user.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/sockio.h>

struct quality
{
  int pbat; /* peak bandwidth target */
  int pbam; /* peak bandwidth mean   */
  int mbat; /* mean bandwidth target */
  int mbam; /* mean bandwidth mean   */
  int mbut; /* burst lenght target   */ 
  int mbum; /* burst lenght mean     */
};

class ForeATM : public Network {
 public:
  ~ForeATM();
  virtual int command(int argc, const char*const* argv);
  void send(const msghdr& mh); 
protected:  
  int open(Atm_endpoint, int sap, quality&);
  int close();

  virtual void dosend(u_char* buf, int len, int fd);
  virtual int dorecv(u_char* buf, int len, u_int32_t& from, int fd);
  int atmconnect(Atm_endpoint dst, int sap, quality& qos);
};

static class ATMNetworkMatcher : public Matcher {
public:
	ATMNetworkMatcher() : Matcher("network") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "atm") == 0)
			return (new ForeATM);
		else
			return (0);
	}
} nm_atm;

int ForeATM::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 2) {
		if (strcmp(argv[1], "close") == 0) {
			close();
			return (TCL_OK);
		}
	} else if (argc == 10) {
		/*
		 * $net open dst sap pbat pbam mbat mbam mbut mbum
		 */
		if (strcmp(argv[1], "open") == 0) {
			quality qos;
			const char* dstName = argv[2];
			int sap = atoi(argv[3]);
			qos.pbat = atoi(argv[4]);
			qos.pbam = atoi(argv[5]);
			qos.mbat = atoi(argv[6]);
			qos.mbam = atoi(argv[7]);
			qos.mbut = atoi(argv[8]);
			qos.mbum = atoi(argv[9]);
			
			Atm_endpoint dst;
			if ((atm_gethostbyname(argv[2], &dst.nsap)) < 0) {
				fprintf(stderr,
					"vic: unknown ATM host %s\n", argv[2]);
				exit(1);
			}
			if (open(dst, sap, qos) < 0)
				tcl.result("0");
			else
				tcl.result("1");
			return (TCL_OK);
		}
	}
	return (Network::command(argc, argv));
}

int ForeATM::open(Atm_endpoint dest, int sap, quality& qos)
{
#ifdef notyet
  Atm_endpoint local;
  int s;
  if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
      perror("get local.nsap: socket");
    }
  
  ifreq ifr;
  strncpy(ifr.ifr_name, "fa0", sizeof (ifr.ifr_name));
  
  if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) 
    {
      perror("ioctl (SIOCGIFADDR)");
    }
  
  sockaddr_in* sin = (struct sockaddr_in *)&ifr.ifr_addr;
  if((atm_gethostbyname(inet_ntoa(sin->sin_addr),&local.nsap))<0)
    {
       atm_error("atm_gethostbyname failed\n");
    }
#endif
  
#define CONTROL_SAP    4090
#define VIDEO_SAP    4092
  ssock_ = atmconnect(dest, sap, qos);
  rsock_ = ssock_;
  return (ssock_);
  
#ifdef notdef
  cssock_ = atmconnect(dest, CONTROL_SAP, qosctrl);
  crsock_ = cssock_;
  ssock_ = atmconnect(dest, VIDEO_SAP, qosvideo);
  rsock_ = ssock_;
  nonblock(cssock_);
  nonblock(ssock_);  
#endif
}

ForeATM::~ForeATM()
{
	close();
}

int ForeATM::close()
{
	if (ssock_ >= 0) {
		atm_close(ssock_);
		ssock_ = -1;
	}
	return (0);
}

void ForeATM::send(const msghdr& mh)
{
   /* Fore API has no scatter/gather hook.  Copy the packet. */
   int cc = cpmsg(mh);
   Network::send(wrkbuf_, cc);
}

void ForeATM::dosend(u_char* buf, int len, int fd)
{
  int cc = atm_send(fd, (char*)buf, len);
  if (cc < 0) 
    {
      atm_error("vic: atm_send");
    }
}

int ForeATM::dorecv(u_char* buf, int len, u_int32_t& from, int fd)
{
  from = 0;
  int cc = atm_recv(fd, (char*)buf, len);
  if (cc < 0) 
    {
      atm_error("vic: atm_recv");
      return (-1);
    }

  return (cc);
}

int ForeATM::atmconnect(Atm_endpoint dst, int sap, quality& service)
{

  int fd, conn_id, portid, switchid;
  int qlen;
  Atm_info info;
  Atm_endpoint calling;
  Atm_endpoint local;
  Atm_qos qos;
  Atm_qos_sel qos_selected;
  Aal_type aal = aal_type_5;
  Atm_dataflow dataflow = duplex;
  int counter;
 
  local.asap = sap;
  dst.asap = sap;
  qos.peak_bandwidth.target = service.pbat;   
  qos.peak_bandwidth.minimum = service.pbam; 
  qos.mean_bandwidth.target = service.mbat; 
  qos.mean_bandwidth.minimum = service.mbam;
  qos.mean_burst.target = service.mbut;
  qos.mean_burst.minimum = service.mbum;
 
  qlen = 0;
  
  if ((fd = atm_open("/dev/fa0", O_RDWR, &info)) < 0) 
    {
      atm_error("atm_open_client");
      exit(1);
    }

  if (atm_bind(fd, local.asap, &local.asap, qlen) < 0) 
    {
      atm_error("atm_bind_client");
      exit(1);
    }
  
 /* printf("SAP assigned (client role) =%d\n", local.asap);*/
  
  if (atm_connect(fd, &dst, &qos, &qos_selected, aal, dataflow) < 0) 
    {
   /*   printf("\nCan't connect...\n");*/
      atm_close(fd);
      qlen =1;
      
      if ((fd = atm_open("/dev/fa0", O_RDWR, &info)) < 0) 
	{
	  atm_error("atm_open_server");
	  exit(1);
	}
      
      if (atm_bind(fd, local.asap, &local.asap, qlen) < 0) 
	{
	  atm_error("atm_bind_server");
	  exit(1);
	}
      
    /*  printf("SAP assigned (server role) =%d\n", local.asap);
      printf("\nWaiting for incoming atm connection...\n");*/
      
      if (atm_listen(fd, &conn_id, &calling, &qos, &aal)<0)
	{
	  atm_error("atm_listen");
	  exit(1);
	}
 /*
      GET_SWITCH(switchid, calling.nsap);
      GET_PORT(portid, calling.nsap);
      
      printf("calling switch=%d, port=%d, sap=%d, aal=%d\n",
	     switchid, portid, calling.asap, aal);
      
      printf("qos target peak=%d, mean=%d, burst=%d\n",
	     qos.peak_bandwidth.target,
	     qos.mean_bandwidth.target,
	     qos.mean_burst.target);
      
      printf("qos minimum peak=%d, mean=%d, burst=%d\n",
	     qos.peak_bandwidth.minimum,
	     qos.mean_bandwidth.minimum,
	     qos.mean_burst.minimum);
      
      printf("connect conn_id=%d\n", conn_id);*/
      
      if(atm_accept (fd,fd ,conn_id, &qos, dataflow)<0)
	{
	  atm_error("atm_accept");
	  exit(1);
	}
        
    /*  printf("\nConnection setup (server)...\n");*/
      
    }     
  else
    {
     
      /* printf("\nConnection setup (client)...\n");*/
      
      sleep(1);
      
    }
  return (fd);
}
