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

static void
sap_handler(sap_packet *packet)
{
  print_sap_packet(packet);
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
