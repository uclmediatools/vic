#ifndef RCAP_H
#define RCAP_H
/*
 * rcap.h
 * Bruce A. Mah (bmah@tenet.berkeley.edu)
 *
 * Exports RCAP-User interface to client programs.
 *
 * $Log$
 * Revision 1.1  1998/02/18 18:04:00  ucacsva
 * Initial revision
 *
 * Revision 1.1  1993/03/31  00:32:39  mccanne
 * Initial revision
 *
 * Revision 1.3  1992/08/17  17:46:39  bmah
 * Added copyright stuff to keep the lawyers happy.
 *
 * Revision 1.2  1992/05/21  03:32:23  bmah
 * Misc fixes.
 *
 * Revision 1.1  1992/01/18  22:47:01  bmah
 * Initial revision
 *
 * Copyright (c) 1992 Regents of the University of California
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for educational, research, and non-profit purposes and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of the University of California not be used in advertising
 * or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Permission to incorporate this
 * software into commercial products can be obtained from the Campus
 * Software Office, 295 Evans Hall, University of California, Berkeley,
 * Ca., 94720 provided only that the the requestor give the University
 * of California a free licence to any derived software for educational
 * and research purposes.  The University of California makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#include "consts.h"

/*
 * Data types for RCAP structures.
 *
 * rcapTraffic is a structure which holds traffic characteristics.
 * This characterization is on a message (not packet) level.
 */
typedef struct {
    u_int xmin;			/* minimum interarrival time */
    u_int xave;			/* average interarrival time */
    u_int I;			/* averaging interval */
    u_int smax;			/* maximum message size in bytes */
} rcapTraffic;

/*
 * rcapRequirements is a structure holding performance requirements.
 * As for traffic characteristics, these figures are on a per-message
 * basis.
 */
typedef struct {
    u_int D;			/* end-to-end delay */
    u_int J;			/* delay jitter */
    u_int Z;			/* statistical delay probability */
    u_int W;			/* statistical no-drop probability */
    u_int U;			/* statistical jitter probability */
    u_char type;		/* channel type */
} rcapRequirements;

#define RCAP_CHANTYPE_DET 1
#define RCAP_CHANTYPE_STAT 2
#define RCAP_CHANTYPE_JITTER 3
#define RCAP_CHANTYPE_BESTEFF 4

/*
 * rcapAddress contains the IP address and port number of an entity in
 * the network.
 */
typedef struct {
	struct in_addr ipAddr;
	u_short port;
} rcapAddress;

/*
 * rcapUserControl contains information on the user control info sent
 * in an establishment request.
 */
typedef struct {
    void *userControl;
    u_short userControlLength;
} rcapUserControl;

/*
 * A parmblock is just an easy way of getting at the different
 * parameters of a call establishment.
 */
typedef struct {
    rcapTraffic *traffic;
    rcapRequirements *requirements;
    rcapAddress *destination;
    rcapUserControl *control;
} parmblock;

/*
 * User-callable RCAP routines
 */
extern u_int RcapEstablishRequest(parmblock *parms, u_short *lcid,
				  struct in_addr *ipAddr);
extern u_int RcapStatusRequest(u_short lcid, void *statusBuffer, u_short
			*statusBufferLength);

extern u_int RcapCloseRequest(u_short lcid, u_short reasonCode);

extern u_int RcapRegister(u_short port, u_int queue_length);
extern u_int RcapReceiveRequest(u_short port, parmblock *parms, rcapAddress
			 *source, u_short *lcid);
extern u_int RcapEstablishReturn(u_short lcid, u_short result, u_short
			  reasonCode, rcapUserControl *control);
extern u_int RcapUnregister(u_short port);

#define RCAP_RECEIVE_BUFFER_SIZE 10240

#endif
