/*
 * $Header$
 *
 * ipc.h
 * Bruce A. Mah (bmah@tenet.berkeley.edu)
 *
 * $Log$
 * Revision 1.1  1998/02/18 18:04:00  ucacsva
 * Initial revision
 *
 * Revision 1.3  1992/05/12  21:00:57  bmah
 * Fixed a typo in function declaration of IpcRecv.  Split; versions of
 * this file for library and daemon are now separate.
 *
 * Revision 1.2  1992/01/18  22:45:29  bmah
 * Added prototypes for ipc routines.
 *
 * Revision 1.1  1991/12/16  02:55:13  bmah
 * Initial revision
 *
 */

#ifndef IPC_H
#define IPC_H

#define RCAP_PORT 10000
#define RCAP_HOST "localhost"

extern int IpcInit(void);
extern int IpcSend(unsigned char *theBytes, int length);
extern int IpcRecv(unsigned char *theBytes, int *readLength, int bufferLength);

#endif
