/*
 * ipaddr.h
 *
 * $Log$
 * Revision 1.1  1998/02/18 18:04:00  ucacsva
 * Initial revision
 *
 * Revision 1.4  1992/05/21  03:34:03  bmah
 * Adjusted function prototypes, added InterfaceIsLocal.
 *
 * Revision 1.3  1992/04/25  20:50:24  bmah
 * *** empty log message ***
 *
 * Revision 1.2  1992/04/25  20:49:46  bmah
 * Added support for new AddressIsLocal.
 *
 * Revision 1.1  1992/04/04  01:05:27  bmah
 * Initial revision
 *
 * Revision 1.1  1992/01/18  22:44:18  bmah
 * Initial revision
 *
 */
#ifndef IPADDR_H
#define IPADDR_H

unsigned long GetLocalIpAddress();
int AddressIsLocal(unsigned long ipAddr);
int InterfaceIsLocal(int line);

#endif
