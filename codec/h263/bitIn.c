#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bitIn.h"

static unsigned int msk[33] =
{
  0x00000000,0x00000001,0x00000003,0x00000007,
  0x0000000f,0x0000001f,0x0000003f,0x0000007f,
  0x000000ff,0x000001ff,0x000003ff,0x000007ff,
  0x00000fff,0x00001fff,0x00003fff,0x00007fff,
  0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
  0x000fffff,0x001fffff,0x003fffff,0x007fffff,
  0x00ffffff,0x01ffffff,0x03ffffff,0x07ffffff,
  0x0fffffff,0x1fffffff,0x3fffffff,0x7fffffff,
  0xffffffff
};


#ifdef DEBUG
static void DumpBuffer(Bitstream *bs) {
  FILE *out;
  Byte *data;
	
  data = bs->BufferPtr;
	
  out = fopen("dump","w");
  while(data != bs->GOBend) {
    putc(*data++, out);
  }
  fclose(out);
	
}

static void PrintBits(int val, int n) {

  int i;

  for (i=n; i>0; i--) {
    fprintf(stderr,"%d", (val&(1<<(i-1))) ? 1 : 0);
  }
  fprintf(stderr,"-");
}
#endif


/* ========================================================================= */


static void InitCache(Bitstream *bs) {
	
  bs->Cache = (bs->Cache<<8) | *bs->BufferPtr++ ;
  bs->Cache = (bs->Cache<<8) | *bs->BufferPtr++ ;
  bs->Cache = (bs->Cache<<8) | *bs->BufferPtr++ ;
  bs->Cache = (bs->Cache<<8) | *bs->BufferPtr++ ;
  bs->ValidBits = 32;
}

static Byte *CopyBufferUntilH263Sync(Bitstream *bs, Byte *start, Byte *end) {
  /* Copies the data between start and end into the stream buffer.
     Initializes some variables if a sync word has been found.
   */
  while (start < end) {	
    bs->Cache = (bs->Cache<<8) | (*bs->BufferPtr++ = *start++);
    if ((bs->Cache & 0x00ffff80) == 0x0000080) {
      InitStreamParsingH263(bs);
      return(start - 3);
    }
  }
  return(start);
}

static Byte *CopyBufferUntilPyraSync(Bitstream *bs, Byte *start, Byte *end) {
  /* Copies the data between start and end into the stream buffer.
     Initializes some variables if a sync word has been found.
   */
  while (start < end) {
    bs->Cache = (bs->Cache<<8) | (*bs->BufferPtr++ = *start++);
    if ((bs->Cache & 0x00ffffff) == 0x000001) {
      InitStreamParsingPyra(bs);
      return(start - 3);
    }
  }
  return(start);
}
 
static void CheckAndHandleUnterflow(Bitstream *bs) {
  /* Reloads the cache if it contains less than 24 valid bits. */
  while (bs->ValidBits < 24) {
    /* Check end of block condition */
    bs->EOB = (bs->BufferPtr == bs->GOBend);
    /* Get byte from buffer */
    bs->Cache = (bs->Cache<<8) | *(bs->BufferPtr++);
    /* Increment valid bits counter */
    bs->ValidBits += 8;
  }
}

/* ========================================================================= */
/* ========================================================================= */
/* Public functions */

Byte *FindH263PictSync(Byte *start, Byte *end) {
  unsigned long cache = 0xffffffff;


  while (start != end) {
    cache = (cache<<8) | *start++;
    if ((cache & 0x00fffffc) == 0x0000080) {/*00000000 00000000 100000xx */
      return(start - 3);
    }
  }

  return(NULL);
  
}

Byte *FindPyraPictSync(Byte *start, Byte *end) {
  unsigned long cache = 0xffffffff;

  while (start != end) {
    cache = (cache<<8) | *start++;
    if ((cache & 0xfffffff8) == 0x000100) {
      return(start - 4);
    }
  }
  
  return(NULL);
  
}

Byte *FindH263Sync(Byte *start, Byte *end) {
  unsigned long cache = 0xffffffff;


  while (start != end) {
    cache = (cache<<8) | *start++;
    if ((cache & 0x00ffff80) == 0x0000080) {/*00000000 00000000 1xxxxxxx*/
      return(start - 3);
    }
  }

  return(NULL);
  
}

Byte *FindPyraSync(Byte *start, Byte *end) {
  unsigned long cache = 0xffffffff;

  while (start != end) {
    cache = (cache<<8) | *start++;
    if ((cache & 0x00ffffff) == 0x000001) {
      return(start - 3);
    }
  }
  
  return(NULL);
  
}

int ShowH263TempRef(Byte *buffer)
     /* This function works only if buffer points to the picture sync of an
	H.263 stream */
{
  int tr;

  tr = (int)(buffer[2] & 0x03) << 6;
  tr |= (int)(buffer[3] & 0xfc) >> 2;

  return(tr);
}


int ShowPyraTempRef(Byte *buffer)
     /* This function works only if buffer points to the picture sync of an
	Pyra stream */
{
  int tr;

  tr = (int)(buffer[3] & 0x07) << 5;
  tr |= (int)(buffer[4] & 0xf8) >> 3;

  return(tr);
}


int ShowH263GOBnumber(Byte *buffer)
     /* This function works only if buffer points to the sync of an
	H.263 stream */
{
  int gn;

  gn = (int)((buffer[2] & 0x7c) >> 2);

  return(gn);
}


int ShowPyraGOBnumber(Byte *buffer)
     /* This function works only if buffer points to the sync of an
	Pyra stream */
{
  int gn;

  gn = (int)((buffer[3] & 0xf8) >> 3);

  return(gn);
}


int ShowH263PicSize(Byte *buffer)
     /* This function works only if buffer points to the picture sync of an
	Pyra stream */
{
  int ct;

  ct = (int)((buffer[4] & 0x1c) >> 2);

  return(ct);
}


int ShowPyraPicSize(Byte *buffer)
     /* This function works only if buffer points to the picture sync of an
	Pyra stream */
{
  int ct;

  ct = (int)(buffer[4] & 0x07);

  return(ct);
}


int ShowH263CodingType(Byte *buffer)
     /* This function works only if buffer points to the picture sync of an
	H.263 stream */
     /* Does not work for extended PTYPE yet */
{
  int ct;

  ct = (int)((buffer[4] & 0x02) >> 1);

  return(ct);
}

int ShowPyraCodingType(Byte *buffer)
     /* This function works only if buffer points to the picture sync of an
	Pyra stream */
{
  int ct;

  if ((int)(buffer[4] & 0x07) == 7)
    ct = (int)((buffer[7] & 0x02) >> 7);   /* custom picture format */
  else
    ct = (int)((buffer[5] & 0x80) >> 7);

  return(ct);
}


int ShowH263GFID(Byte *buffer)
     /* This function works only if buffer points to the GOB sync of an
	H.263 stream */
{
  int gfid;

  gfid = (int)(buffer[2] & 0x03);

  return(gfid);
}

int ShowPyraGFID(Byte *buffer)
     /* This function works only if buffer points to the GOB sync of an
	Pyra stream */
{
  int gfid;

  gfid = (int)((buffer[3] & 0x06) >> 1);

  return(gfid);
}


void CheckInBuffer(FILE *fp, Byte *buffer, int bufferSize,
		   Byte **start_p, Byte **end_p)
{
  int n;
  int got;

  for (n = 0; *start_p < *end_p; n++, (*start_p)++)
    buffer[n] = *(*start_p);
  got = fread(buffer + n, 1, bufferSize - n, fp);
  *end_p = buffer + n + got;
  *start_p = buffer;
}


/* This routine expects that the pointer *start_p never points to one of the
   first 3 bytes after a picture startcode when you call this routine */
/* If a picture startcode is detected in one of the lost packets or the
   next sync following the last lost packet is a picture startcode, the
   temporal reference is returned in '*tr_p'. Otherwise '*tr_p' is unchanged.
   */
/* Returns the number of lost packets */
int ErrorProneCheckInBufferH263(FILE *fp, Byte *buffer, int bufferSize,
				Byte **start_p, Byte **end_p,
				int percentLoss, int *tr_p)
{
  int loss;
  int lostPackets = 0;


  do {
    do {
      CheckInBuffer(fp, buffer, bufferSize, start_p, end_p);

      /* Should we lose the packet? */
      if ((loss = (rand() <= (percentLoss * RAND_MAX) / 100)) == 1) {
	/* Count lost packets */
	lostPackets++;
	/* Ok. But before dropping the packet, we are looking for a new
	   temporal reference. If we don't find any we keep our hands of
	   'tr_p'. */
	while ((*start_p = FindH263PictSync(*start_p, *end_p)) != NULL) {
	  /* OK, we've got a picture start code */
	  if (*start_p < *end_p - 3)  /* Enough bytes left to look for tr? */
	    *tr_p = ShowH263TempRef(*start_p);
	  else
	    break;
	  /* So lets skip this sync */
	  *start_p += 3;
	}
	/* Keep the last 3 bytes in mind */
	*start_p = *end_p - 3;
      }
    } while (loss);

    //SV-XXX: Added explicit braces to avoid ambiguous else
    if (lostPackets > 0) {
      /* Resync */
      if ((*start_p = FindH263Sync(*start_p, *end_p)) != NULL) {
	if (*start_p < *end_p - 3) {   /* Enough bytes left to look for tr? */
	  if (!ShowH263GOBnumber(*start_p)) {
	    /* OK, we've got a picture start code */
	    *tr_p = ShowH263TempRef(*start_p);
	  }
	}
      } else {
		*start_p = *end_p;
	}
    }
  } while (!(*start_p < *end_p - 3) && !feof(fp));
  /* Loop until we've got at least enough bytes to look at the
     temporal reference if there is any */

  return(lostPackets);
}


/* This routine expects that the pointer *start_p never points to one of the
   first 3 bytes after a picture startcode when you call this routine */
/* If a picture startcode is detected in one of the lost packets or the
   next sync following the last lost packet is a picture startcode, the
   temporal reference is returned in '*tr_p'. Otherwise '*tr_p' is unchanged.
   */
/* Returns the number of lost packets */
int ErrorProneCheckInBufferPyra(FILE *fp, Byte *buffer, int bufferSize,
				Byte **start_p, Byte **end_p,
				int percentLoss, int *tr_p)
{
  int loss;
  int lostPackets = 0;


  do {
    do {
      CheckInBuffer(fp, buffer, bufferSize, start_p, end_p);

      /* Should we lose the packet? */
      if ((loss = (rand() <= (percentLoss * RAND_MAX) / 100)) == 1) {
	/* Count lost packets */
	lostPackets++;
	/* Ok. But before dropping the packet, we are looking for a new
	   temporal reference. If we don't find any we keep our hands of
	   'tr_p'. */
	while ((*start_p = FindPyraPictSync(*start_p, *end_p)) != NULL) {
	  /* OK, we've got a picture start code */
	  if (*start_p < *end_p - 4)  /* Enough bytes left to look for tr? */
	    *tr_p = ShowPyraTempRef(*start_p);
	  else
	    break;
	  /* So lets skip this sync */
	  *start_p += 4;
	}
	/* Keep the last 4 bytes in mind */
	*start_p = *end_p - 4;
      }
    } while (loss);

    //SV-XXX: Added explicit braces to avoid ambiguous else
    if (lostPackets > 0) {
      /* Resync */
      if ((*start_p = FindPyraSync(*start_p, *end_p)) != NULL) {
	if (*start_p < *end_p - 4) {  /* Enough bytes left to look for tr? */
	  if (!ShowPyraGOBnumber(*start_p)) {
	    /* OK, we've got a picture start code */
	    *tr_p = ShowPyraTempRef(*start_p);
          }
        }
      } else {
		*start_p = *end_p;
	}
    }
  } while (!(*start_p < *end_p - 4) && !feof(fp));
  /* Loop until we've got at least enough bytes to look at the
     temporal reference if there is any */

  return(lostPackets);
}


/* ========================================================================= */

int GetBits(Bitstream *bs, int n) {
  /* Removes n bits from the bitstream and returns the value */
  int val;

  /* PrintBits(ShowBits(bs, n),n); */

  bs->ValidBits -= n;
  val = (bs->Cache >> bs->ValidBits) & msk[n];
  CheckAndHandleUnterflow(bs);
  return(val);
}

int GetOneBit(Bitstream *bs) {
  /* PrintBits(ShowBits(bs, 1),1); */
  return(GetBits(bs,1));
}

int ShowBits(Bitstream *bs, int n) {
  /* Returns the value of the next n bits. No bits are removed. */
  return ((bs->Cache >> (bs->ValidBits-n)) & msk[n]);
}


void ConsumeBits(Bitstream *bs, int n) {
  /* Removes n bits from the bitstream, nothing is returned. */

  /* PrintBits(ShowBits(bs, n),n); */

  bs->ValidBits -= n;
  CheckAndHandleUnterflow(bs);
} 

/* ========================================================================= */

void InitStreamBuffer(Bitstream *bs, int alloc_f) {

  /* Initializes the stream buffer. */

  if (alloc_f) {
    bs->ByteBuffer = (unsigned char *) malloc(BUFSIZE);
    bs->BufferAllocated = 1;

    if (bs->ByteBuffer == NULL) {
      fprintf(stderr,"Cannot allocate Stream Buffer memory\n");
      exit(1);
    }
  } else {
    bs->ByteBuffer = NULL;
    bs->BufferAllocated = 0;
  }

  bs->BufferPtr = bs->ByteBuffer;
  bs->BufState  = GOB_EMPTY;
  bs->Cache = 0xffffffff;
}

void FreeStreamBuffer(Bitstream *bs) {
  if (bs->BufferAllocated)
    free(bs->ByteBuffer);
  bs->BufferAllocated = 0;
}

Byte *PassToStreamBufferH263(Bitstream *bs, Byte *bufstart, Byte *bufend) {

  /* bufstart and bufend are pointers to the start and the end of
     the data buffer. Data is passed to the bitstream buffer.
     Copy operation stops behind a sync word.
     A pointer to the first byte within in the data buffer which
     has not been copied is returned.
  */

  Byte *start;

  /* If the function is called and buffer state is GOB_COMPLETE
     we have just decoded one GOB. Reset some variables and turn
     the buffer state into GOB_PENDING.
  */
  /* If the function is call and buffer state is GOB_LOADED
     we have already initialized the stream buffer for decoding a
     GOB but didn't touch it by now.
     */

  if (bs->BufState == GOB_COMPLETE) {
    if (bufstart <= bufend - 3) {
      bs->BufferPtr = bs->ByteBuffer;
      /* Copy the sync at the beginning */
      *bs->BufferPtr++ = *bufstart++;
      *bs->BufferPtr++ = *bufstart++;
      *bs->BufferPtr++ = *bufstart++;
      bs->BufState  = GOB_PENDING;
      bs->Cache = 0xffffffff;
    } else
      bs->BufState = GOB_EMPTY;
  }
	
  if (bs->BufState == GOB_EMPTY) {
  	/* This happens if we haven't seen any sync word so far. */
    start = FindH263Sync(bufstart, bufend);
    if (start) {
      /* The very first sync has been found */
      /* Copy the sync at the beginning */
      bs->BufferPtr = bs->ByteBuffer;
      *bs->BufferPtr++ = *start++;
      *bs->BufferPtr++ = *start++;
      *bs->BufferPtr++ = *start++;
      bs->BufState = GOB_PENDING;
      bs->Cache = 0xffffffff;
      return(CopyBufferUntilH263Sync(bs, start, bufend));
    }
  } else if (bs->BufState == GOB_PENDING) {
  	/* We have seen a sync word. Copy everythings into the
  	   stream buffer until we see the next sync word.
  	 */
    return(CopyBufferUntilH263Sync(bs, bufstart, bufend));
  } else if (bs->BufState == GOB_LOADED) {
    return(bufstart);
  }

  return(bufend);
}

Byte *PassToStreamBufferPyra(Bitstream *bs, Byte *bufstart, Byte *bufend) {
 
  /* bufstart and bufend are pointers to the start and the end of
     the data buffer. Data is passed to the bitstream buffer.
     Copy operation stops behind a sync word.
     A pointer to the first byte within in the data buffer which
     has not been copied is returned.
  */
 
  Byte *start;
 
  /* If the function is called and buffer state is GOB_COMPLETE
     we have just decoded one GOB. Reset some variables and turn
     the buffer state into GOB_PENDING.
  */
  if (bs->BufState == GOB_COMPLETE) {
    if (bufstart <= bufend - 3) {
      bs->BufferPtr = bs->ByteBuffer;
      /* Copy the sync at the beginning */
      *bs->BufferPtr++ = *bufstart++;
      *bs->BufferPtr++ = *bufstart++;
      *bs->BufferPtr++ = *bufstart++;
      bs->BufState  = GOB_PENDING;
      bs->Cache = 0xffffffff;
    } else
      bs->BufState = GOB_EMPTY;
  }
 
  if (bs->BufState == GOB_EMPTY) {
        /* This happens if we haven't seen any sync word so far. */
    start = FindPyraSync(bufstart, bufend);
    if (start) {
      /* The very first sync has been found */
      /* Copy the sync at the beginning */
      bs->BufferPtr = bs->ByteBuffer;
      *bs->BufferPtr++ = *start++;
      *bs->BufferPtr++ = *start++;
      *bs->BufferPtr++ = *start++;
      bs->BufState = GOB_PENDING;
      bs->Cache = 0xffffffff;
      return(CopyBufferUntilPyraSync(bs, start, bufend));
    }
  } else if (bs->BufState == GOB_PENDING) {
        /* We have seen a sync word. Copy everythings into the
           stream buffer until we see the next sync word.
         */
    return(CopyBufferUntilPyraSync(bs, bufstart, bufend));
  } else if (bs->BufState == GOB_LOADED) {
    return(bufstart);
  }

  return(bufend);
}
 
void InitStreamParsingH263(Bitstream *bs) {
  bs->BufState  = GOB_LOADED;
  bs->GOBend    = bs->BufferPtr - 2;
  assert(!bs->BufferAllocated || bs->BufferPtr - bs->ByteBuffer < BUFSIZE);
  bs->BufferPtr = bs->ByteBuffer;
  InitCache(bs);
}

void InitStreamParsingPyra(Bitstream *bs) {
  bs->BufState  = GOB_LOADED;
  bs->GOBend    = bs->BufferPtr - 3;
  assert(!bs->BufferAllocated || (bs->BufferPtr - bs->ByteBuffer < BUFSIZE));
  bs->BufferPtr = bs->ByteBuffer;
  InitCache(bs);
}
 
/* ========================================================================= */
