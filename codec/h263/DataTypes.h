#ifndef _DATATYPES_
#define _DATATYPES_

typedef unsigned short uint16;
typedef char          Boolean;
typedef char *        String;
typedef char          Str255[255];
typedef int           Block256[256];

#define INTRA 1
#define MOTCO 2
#define FWDTP 4
#define BAKTP 8

#endif
