#ifndef _DATATYPES_
#define _DATATYPES_

typedef unsigned short uint16;
typedef char          Boolean;
typedef unsigned char Byte;
typedef Byte *        Ptr;
typedef char *        String;
typedef char          Str255[255];
typedef int           Block256[256];

#define TRUE 1
#define true 1
#define FALSE 0
#define false 0

#define INTRA 1
#define MOTCO 2
#define FWDTP 4
#define BAKTP 8

#endif
