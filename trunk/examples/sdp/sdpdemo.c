/*
 * sdpdemo: A simple sdp application that recieves SAP/SDP data.
 *
 * (c) 2002 Argonne National Laboratory/University of Chicago
 */

#include <sys/time.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "uclconf.h"
#include "config_unix.h"
#include "config_win32.h"
#include "debug.h"
#include "memory.h"
#include "sap.h"
#include "sdp.h"

static void
sap_handler(sap_packet *packet)
{
  sdp *session = NULL;
  char *new_payload = NULL;

  print_sap_packet(packet); 

  session = sdp_parse(packet->payload);

  printf("Original Packet: \n++++\n%s++++\n", session->original);

  sdp_print(session);

  new_payload = sdp_make(session);

  if(strcmp(packet->payload, new_payload) != 0)
    printf("The two sdp payloads are different!\n");

  xfree(new_payload);

  sdp_free(session);

  /* This is causing a seg fault, and a memory leak */
  /*  xfree(packet); */
}


int
main()
{
  struct sap *session = NULL;
  struct timeval timeout;

  session = sap_init("224.2.127.254", 9875, 127, sap_handler);

  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  while(1) 
    sap_recv(session, &timeout);

  sap_done(session);

  return 0;
}
