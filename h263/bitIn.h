#ifndef BITIO_UWE_H
#define BITIO_UWE_H

#include <stdio.h>

#include "defs.h"

#define BUFSIZE 32768


#define GOB_EMPTY    0
#define GOB_PENDING  1
#define GOB_COMPLETE 2
#define GOB_LOADED   3


typedef struct BitstreamStruct {
  int ValidBits;
  unsigned long Cache;
  unsigned char *ByteBuffer;
  unsigned char *BufferPtr;
  unsigned char *GOBend;
  int EOB;
  int BufState;
  int BufferAllocated;
} Bitstream;



/* Include prototype file */
#include "bitIn.p"

#endif   /* #ifndef BITIO_UWE_H */

