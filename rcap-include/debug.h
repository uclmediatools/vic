#ifndef DEBUG_H
#define DEBUG_H

#include "internal.h"

extern void HexPrint(unsigned char *theBytes, int count);
extern void PrintRcapHeader(RcapHeader *theRcapHeader);
extern void PrintRmtpHeaderRecord(RmtpHeaderRecord *theRmtpHeaderRecord);
extern void PrintRcapInternetNSR(RcapInternetNSR *theRcapInternetNSR);

#endif
