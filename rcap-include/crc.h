#ifndef CRC_H
#define CRC_H
/*
 * $Header$
 * 
 * crc.h
 * $Log$
 * Revision 1.1  1998/02/18 18:03:59  ucacsva
 * Initial revision
 *
 * Revision 1.5  1992/07/29  20:47:14  bmah
 * ushort->u_short for SunOS compatability.
 *
 * Revision 1.4  1992/05/12  20:53:49  bmah
 * Split...daemon and library files are now separate.
 *
 * Revision 1.3  1992/01/18  22:42:44  bmah
 * UNIX-ized dtaa types.
 *
 * Revision 1.2  1991/12/15  23:04:40  bmah
 * Headers reflect 16-bit CRCs instead of 32- and 31-bit.
 *
 */
#include <sys/types.h>

typedef u_short crc_table[256];

void CrcTable16(u_short poly, crc_table table);
u_short CrcCompute16(u_char *data, int len, crc_table table);

#endif CRC_H
