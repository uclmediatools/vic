/***********************************************************HeaderBegin*******
 *                                                                         
 * File:        bitOut.c
 *
 * Author:      K.S.
 * Created:     5-Nov-97
 *                                                                         
 * Description: source code of routines for buffering and bit-io
 *
 * Notes: 
 *
 * Modified:  
 *
 ***********************************************************HeaderEnd*********/


#include <stdlib.h>
#include <string.h> //SV-XXX: avoids implicit declaration of strlen, line 454

#include "defs.h"
#include "structs.h"

#include "Util.h"

#include "bitOut.h"
#include "bitOut.p" //SV-XXX: to avoid implicit declarations


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- AllocBitstr -- Alloc bitstream with buffer but without filepointer
 *
 * Author:           T.W.
 *
 * Created:          23-July-96		
 *	
 * Purpose:          Creates a new 'Bitstr' with a buffer size of 'n' bytes
 *                   and sets the filepointer to NULL. 
 *
 * Arguments in:     int      n     size of buffer in byte (min. sizeof(Int)).
 *
 * Arguments in/out: -
 *
 * Arguments out:    -
 *
 * Return values:    Bitstr *       pointer to allocated bitstream structure.
 *                                  NULL in case of error.
 *
 * Example:          b = AllocBitstr(1024);
 *
 * Side effects:     -
 *
 * Description:      The memory of a 'Bitstr' structure is allocated.
 *                   No file is opened!
 *                   If a file shall be associated with the bitstream, use
 *                   FopenBitstr! 
 *                   This function is included in FopenBitstr.
 *
 * See also:         FopenBitstr
 *	
 * Modified:         -
 *	
 *****************************************************************************/
Bitstr *AllocBitstr(unsigned int n) //SV-XXX: changed arg definition to uint
/***********************************************************CommentEnd********/
{
  Bitstr *b = (Bitstr *) malloc(sizeof(Bitstr));

  n = MAX(n, sizeof(int));

  b->size = n * 8;
  b->ind = 0;
  b->actualSize = 0;
  b->b = (Byte *) malloc(n * sizeof(Byte));
  b->fp = NULL;

  return(b);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- FreeBitstr -- Free a bitstream structure (file is  n o t  closed!)
 *
 * Author:           T.W.
 *
 * Created:          23-July-96		
 *	
 * Purpose:          Frees a 'Bitstr' and sets the pointer to NULL.
 *
 * Arguments in:     -
 *
 * Arguments in/out: Bitstr **b_p    pointer to bitstream structure
 *
 * Arguments out:    -
 *
 * Return values:    -
 *
 * Example:          FreeBitstr(&b);
 *
 * Side effects:     -
 *
 * Description:      The memory of a 'Bitstr' structure is freed.
 *                   No file closing is done!
 *                   If a file is associated with the bitstream, use
 *                   FcloseBitstr! 
 *                   This function is included in FcloseBitstr.
 *
 * See also:         FcloseBitstr
 *	
 * Modified:         -
 *	
 *****************************************************************************/
void FreeBitstr(Bitstr **b_p)
/***********************************************************CommentEnd********/
{
  free((*b_p)->b);
  free(*b_p);
  *b_p = NULL;
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- CheckOutBuffer -- Write bytes from buffer to file
 *
 * Author:           T.W.
 *
 * Created:          23-July-96
 *	
 * Purpose:          Writes the bytes from the partially filled buffer 'b'
 *                   to the file addressed by 'b->fp'. The last byte is
 *                   filled with zeros.
 *
 * Arguments in:     Bitstr *b    bitstream structure
 *
 * Arguments in/out: -
 *
 * Arguments out:    -
 *
 * Return values:    FALSE  (0)  on 'EOF' or 'b->fp == NULL'
 *                   TRUE   (1)  on success
 *
 * Example:          CheckOutBuffer(b);
 *
 * Side effects:     Content of 'b->b' is written to file 'b->fp'.
 *
 * Description:      The buffer is flashed to the file. The last byte is
 *                   filled with zeros.
 *                   The buffer indicater 'b->ind' is reset.
 *                   This function is called automatically if a file is
 *                   associated with the bitstream and you use
 *                   PutBit or PutCodeword.
 *
 * See also:         CheckInBuffer, PutBit, PutCodeword
 *	
 * Modified:         -
 *	
 *****************************************************************************/
int CheckOutBuffer(Bitstr *b)
/***********************************************************CommentEnd********/
{
  unsigned int i; //SV-XXX
  unsigned int r = b->ind; //SV-XXX


  if (b->fp == NULL)
    return (FALSE);

  if(b->size!=r) {
    unsigned int rem = b->ind % 8; //SV-XXX
    unsigned int quot = b->ind / 8; //SV-XXX
    if(rem > 0) {
      for(i=rem;i<8;++i) {
        UNSETBIT(b->b[quot],7-i);
      }
      r = (quot+1)*8;
    }
  }

  b->ind = 0;

  if (fwrite(&b->b[0],1,r/8,b->fp) < r/8)
    return (FALSE);
  if (fflush(b->fp) == EOF)
    return (FALSE);

  return (TRUE);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- FopenBitstr -- New bitstream with buffer and filepointer
 *
 * Author:           T.W.
 *
 * Created:          23-July-96		
 *	
 * Purpose:          Creates a new 'Bitstr' with a buffer size of 'n' bytes
 *                   and opens file 'name' with mode 'mode' (see fopen()).
 *
 * Arguments in:     int  n        buffer size in bytes
 *                   char *name    file name of bitstream
 *                   char *mode    "w" (write), "a" (append) or
 *                                 "r" (read) (see fopen()!)
 *
 * Arguments in/out: -
 *
 * Arguments out:    -
 *
 * Return values:    Bitstr *      allocated bitstream structure
 *
 * Example:          b = FopenBitstr(1024, "bit.stream", "r");
 *
 * Side effects:     If mode is "w", the file 'name' is created or,
 *                   if it exists, its contents are deleted.
 *
 * Description:      -
 *
 * See also:         FcloseBitstr
 *	
 * Modified:
 *	
 *****************************************************************************/
Bitstr *FopenBitstr(int n, char *name, char *mode)
/***********************************************************CommentEnd********/
{
  Bitstr *b = AllocBitstr(n);

  b->fp = fopen(name,mode);

  return(b);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- FcloseBitstr -- Free bitstream buffer and close filepointer
 *
 * Author:           T.W.
 *
 * Created:          23-July-96		
 *	
 * Purpose:          Closes the file associated with the bitstream
 *                   (see fclose()) and frees the bitstream structure.
 *
 * Arguments in:     -
 *
 * Arguments in/out: Bitstr **b_p      pointer to the bitstream structure
 *
 * Arguments out:    -
 *
 * Return values:    -
 *
 * Example:          FcloseBitstr(&b);
 *
 * Side effects:     -
 *
 * Description:      The file associated with the bitstream is closed
 *                   (see fclose()) and the bitstream structure is freed.
 *                   The pointer to the bitstream structure is set to NULL.
 *
 * See also:         FopenBitstr
 *	
 * Modified:         -
 *	
 *****************************************************************************/
void FcloseBitstr(Bitstr **b_p)
/***********************************************************CommentEnd********/
{
  CheckOutBuffer(*b_p);
  if ((*b_p)->fp != NULL)
    fclose((*b_p)->fp);
  FreeBitstr(b_p);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- BitsToByteAlignment -- Return remaining bits in current byte
 *
 * Author:           Niko Faerber
 *
 * Created:          08-March-97
 *
 * Purpose:          Calculate the number of bits to complete the current byte
 *
 * Arguments in:     Bitstr *bs : bitstream
 *
 * Arguments in/out: -
 *
 * Arguments out:    -
 *
 * Return values:    Int    n   : number of bits to complete the current byte
 *
 * Example:          n = BitsToByteAlignment(bs);
 *
 * Side effects:     -
 *
 * Description:      -
 *
 * See also:         -
 *
 * Modified:         -
 *
 *****************************************************************************/
int BitsToByteAlignment(Bitstr *bs)
/***********************************************************CommentEnd********/
{
  int n;
  
  n = 8-(bs->ind)%8;
  if(n==8) n=0;
 
  return n;
}
 
 
/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- ByteAlign0X -- Fill the remaining incomplete byte of the stream with 0
 *
 * Author:           K.S.
 *
 * Created:          31-Aug-97
 *	
 * Purpose:          Sets the stream indicator to the first bit of the
 *                   next byte. The skipped bits are set to zero.
 *
 * Arguments in:     Bitstr *b    bitstream structure
 *
 * Arguments in/out: -
 *
 * Arguments out:    -
 *
 * Return values:    Number of bits to byte alignment that are sent.
 *
 * Example:          ByteAlign0X(b);
 *
 * Side effects:     -
 *
 * Description:      Zeros are written to the bitstream 'b' until the actual
 *                   byte is full.
 *
 * See also:         ByteAlign0, ByteAlign, BitsToByteAlignment
 *
 * Modified:         -
 *
 *****************************************************************************/
int ByteAlign0X(Bitstr *b)
/***********************************************************CommentEnd********/
{
  int n;

  n = BitsToByteAlignment(b);
  PutNumberX(0, n, b);

  return(n);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- PutBitX -- Put bit to bitstream without checking buffersize
 *
 * Author:           K.S.
 *
 * Created:          23-Aug-97
 *	
 * Purpose:          Writes a bit to the bit-stream 'b' at the current
 *                   pointer stage 'b->ind' of the bit-stream.
 *                   It is not checked if the buffersize is exceeded.
 *
 * Arguments in:     int    bit      bitvalue (0 or 1)
 *                                   (if != 0 --> 1 is emitted)
 *
 * Arguments in/out: Bitstr *b       bitstream structure
 *
 * Arguments out:    -
 *
 * Return values:    -
 *
 * Example:          PutBitX(1, b);
 *
 * Side effects:     -
 *
 * Description:      If the buffer is filled.
 *
 * See also:         PutBit, GetBitX
 *	
 * Modified:         -
 *	
 *****************************************************************************/
void PutBitX(int bit, Bitstr *b)
/***********************************************************CommentEnd********/
{
  int quot = b->ind >> 3;
  int bitP = 7 - (b->ind & 0x7);


  if(bit) {
    SETBIT(b->b[quot], bitP);
  } else {
    UNSETBIT(b->b[quot], bitP);
  }

  b->ind++;
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- PutCodewordX -- Add codeword to actual buffer pointer position (w/o file)
 *
 * Author:           K.S.
 *
 * Created:          27-Aug-97
 *	
 * Purpose:          Writes the codeword 'cw' to the bit-stream 'b' at
 *                   the current pointer stage 'b->ind' of the bit-stream.
 *                   Buffersize and file are not regarded (-> 'PutBitX()').
 *
 * Arguments in:     char   *cv        codeword as a string of 0 and 1.
 *
 * Arguments in/out: Bitstr *b         bitstream structure
 *
 * Arguments out:    -
 *
 * Return values:    -
 *
 * Example:          PutCodewordX("01001000", b);
 *
 * Side effects:     -
 *
 * Description:      -
 *
 * See also:         PutCodeword, GetCodewordX, GetCodewordIndexX, PutBitX
 *	
 * Modified:         -
 *	
 *****************************************************************************/
void PutCodewordX(char *cw, Bitstr *b)
/***********************************************************CommentEnd********/
{
  int l = strlen(cw);
  int i;
  
  for(i=0;i<l;++i) {
    PutBitX(cw[i]-'0', b);
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- PutNumberX -- Add some bits of a number to buffer w/o regarding buf-size
 *
 * Author:           K.S.
 *
 * Created:          27-Aug-97		
 *	
 * Purpose:          Writes the 'n' least significant bits of the integer
 *                   'num' to the bit-stream 'b' at
 *                   the current pointer stage 'b->ind' of the bit-stream.
 *                   Buffersize and file are not regarded.
 *
 * Arguments in:     int   num         number from which the 'n' least
 *                                     significant bits are written to 'b'.
 *                                     The bits are put in the bitstream
 *                                     in descending order
 *                                     (from left to right).
 *                   int   n           number of bits that are written.
 *                                     (max. sizeof(Int) || sizeof(Long) - 7).
 *
 * Arguments in/out: Bitstr *b         bitstream structure
 *
 * Arguments out:    -
 *
 * Return values:    -
 *
 * Example:          PutNumberX(19, 6, b);
 *                   puts the bits '010011' to 'b'.
 *
 * Side effects:     -
 *
 * Description:      -
 *
 * See also:         PutNumber, GetNumberX, PutCodewordX, PutBitX
 *
 * Modified:         -
 *
 *****************************************************************************/
void PutNumberX(int num, int n, Bitstr *b)
/***********************************************************CommentEnd********/
{
  if (n > 0) {
    int  i;
    long numl = (long)num;
    int  rem = b->ind & 0x7;
    long mask = (0x1 << (8 - rem)) - 1;
    int  ind = b->ind + n - 1;
    int  byteInd = ind >> 3;
    int  bitPEnd = 7 - (ind & 0x7);


    numl <<= bitPEnd;

    for (i = rem + n - 8; i > 0; i -= 8) {
      b->b[byteInd--] = (Byte)numl;
      numl >>= 8;
    }

    b->b[byteInd] &= ~mask;
    b->b[byteInd] |= (numl & mask);

    b->ind += n;
  }
}
