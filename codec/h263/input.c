#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "h263decoder.h"
#include "bitIn.h"


/* ========================================================================= */
/* Returns the 'temp_ref' of the found intra picture or -1 if no intra
   picture header has been found */
int ParseH263JunkForIntra(H263Global *h263Data, Byte **next_p, Byte *end,
			  int lastjunk)
{
  int codingType;


  do {

    *next_p = PassToStreamBufferH263(h263Data->bs, *next_p, end);

    if (h263Data->bs->BufState == GOB_PENDING && lastjunk)
      InitStreamParsingH263(h263Data->bs);

    if (h263Data->bs->BufState == GOB_LOADED) {
      /* We have already loaded a GOB; is it GOB 0? */
      if (ShowH263GOBnumber(h263Data->bs->ByteBuffer) == 0) {
	/* Yeah, it is GOB 0; Which coding type has the new picture? */
	/* Check the temporal reference */
	h263Data->next_temp_ref = ShowH263TempRef(h263Data->bs->ByteBuffer);
	codingType = ShowH263CodingType(h263Data->bs->ByteBuffer);
	if (codingType == PICTURE_CODING_TYPE_INTRA)
	  return h263Data->next_temp_ref;
	else {
	  if (*next_p == end && lastjunk)
	    h263Data->bs->BufState = GOB_EMPTY;
	  else {
	    h263Data->bs->BufState = GOB_COMPLETE;
	    h263Data->bs->BufferPtr = h263Data->bs->GOBend - 3;
	    h263Data->bs->ValidBits = 0;
	  }
	}
      } else {
	if (*next_p == end && lastjunk)
	  h263Data->bs->BufState = GOB_EMPTY;
	else {
	  h263Data->bs->BufState = GOB_COMPLETE;
	  h263Data->bs->BufferPtr = h263Data->bs->GOBend - 3;
	  h263Data->bs->ValidBits = 0;
	}
      }
    } else if (h263Data->bs->BufState == GOB_PENDING ||
	       h263Data->bs->BufState == GOB_EMPTY) {
      return -1;
    }
  } while (1);
}


/* ========================================================================= */
/* This Funktion returns '1' if h263Data->newframe should be displayed
   and '0' if it needs more data */
/* if 'lastjunk' is set, the data after the last found Sync is also passed
   to 'H263DecodeGOB'. So be careful that there is a complete GOB after the
   last Sync in the buffer! */

int HandleH263DataJunk (H263Global *h263Data, Byte **next_p, Byte *end,
			int lastjunk) {

    /* Copies the data buffer to the stream buffer.
     * Copy stops at a sync mark.
     * next points to the first data byte within the
     * data buffer which has not been copied.
     */

  const int dec_until_sync = 1;
  int       ngob;
  int       returncode = 1;


  if (h263Data->decGOBsCounter == h263Data->gobs_in_pict) {
    /* We have decoded successfully all GOBs in the last picture */
    /* Reset the GOB counters */
    h263Data->decGOBsCounter = 0;
    memset(h263Data->decGOBs, 0, MAX_GOBS * sizeof(unsigned char));
  }

  *next_p = PassToStreamBufferH263(h263Data->bs, *next_p, end);

  if (lastjunk && h263Data->bs->BufState == GOB_PENDING) {
    InitStreamParsingH263(h263Data->bs);
  }

  while (h263Data->bs->BufState == GOB_LOADED) {

    ngob = ShowH263GOBnumber(h263Data->bs->ByteBuffer);
    if (ngob>=MAX_GOBS) {
    	fprintf(stderr,"ngob %d is larger than %d!\n",ngob,MAX_GOBS);
    	assert(ngob<MAX_GOBS);
    }
    if (h263Data->decGOBs[ngob] == 1) {
      /* We have already decoded a GOB with this number */
      /* So let's finish the picture */
      H263FinishPicture(h263Data);
      return 2;
    }
    if (ngob == 0) {
      if (h263Data->decGOBsCounter > 0 && !h263Data->first) {
	/* A new picture start; and we didn't finish the last picture! */
	/* So lets do concealment */
	H263FinishPicture(h263Data);
	return 2;
      }
    }

    /* We have a complete GOB which can be decoded now. */
    if (!H263DecodeGOB(h263Data, dec_until_sync))             /* h263dec.c */
      /* We had a decoding error! */
      returncode = 2;

    if (*next_p == end && lastjunk)
      h263Data->bs->BufState = GOB_EMPTY;

    if (h263Data->decGOBsCounter == h263Data->gobs_in_pict) {
      /* We have decoded successfully all GOBs in this picture */
      return returncode;
    }

    /* Copy the remaining data bytes to the stream buffer. */
    *next_p = PassToStreamBufferH263(h263Data->bs, *next_p, end);

    if (lastjunk && h263Data->bs->BufState == GOB_PENDING) {
      InitStreamParsingH263(h263Data->bs);
    }
  }

  return 0;
}

/* ========================================================================= */
/* ========================================================================= */
int HandleH263CompleteGOBs(H263Global *h263Data, Byte *next, Byte *end)
{
  if (h263Data->decGOBsCounter == h263Data->gobs_in_pict) { /*!!MS*/
      /* We have decoded successfully all GOBs in this picture */
      /* Reset the GOB counters */
      h263Data->decGOBsCounter = 0;
      memset(h263Data->decGOBs, 0, MAX_GOBS * sizeof(unsigned char));
  }

  h263Data->bs->ByteBuffer = next;
  h263Data->bs->BufferPtr = end;

  do {
      InitStreamParsingH263(h263Data->bs);
      /* We have a complete GOB which can be decoded now. */
      if (!H263DecodeGOB(h263Data, 1))
          return 0;
      h263Data->bs->ByteBuffer = FindH263Sync(h263Data->bs->BufferPtr - sizeof(unsigned long), end);
  } while (h263Data->bs->BufferPtr < end);

  return 1;
}
