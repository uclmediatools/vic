/***********************************************************HeaderBegin*******
 *                                                                         
 * File:        bitio.h
 *
 * Author:      T.W. M.F.
 * Created:     23-July-96 
 *                                                                         
 * Description: Routines for buffering and bit-io
 *
 * Notes: 
 *
 * Modified:  
 *
 ***********************************************************HeaderEnd*********/

#ifndef _BITIO_H
#define _BITIO_H

#include <stdio.h>



/***********************************************************CommentBegin******
 *
 * -- Bitstr -- Bitstream Type
 *
 * Author:       T.W.
 *
 * Created:      23-July-96
 *
 * Description:  int    size          buffer size in bit
 *               int    ind           indicator; number of bits in buffer
 *               int    actualSize    actual size of the buffer (read in)
 *               Byte   *b            pointer to buffer
 *               FILE   *fp           pointer to file
 *
 * Note:         -
 *
 * See also:     -
 *
 * Modified:     13-Aug-96 K.S.
 *
 *****************************************************************************/
typedef struct{
  unsigned int   size; //SV-XXX
  unsigned int   ind; //SV-XXX
  int   actualSize;
  Byte  *b;
  FILE  *fp;
} Bitstr;
/***********************************************************CommentEnd********/
 
/***********************************************************CommentBegin******
 *
 * -- READBIT -- Macro: extracts bit
 *
 * Arguments in:   x, l
 *
 * Return values:  bit number l (starting from MSB) out of x
 *****************************************************************************/
#define READBIT(x,l)          (unsigned long) (((x) & (1UL << (l))) ? 1 : 0)
/***********************************************************CommentEnd********/

/***********************************************************CommentBegin******
 *
 * -- SETBIT -- Macro: set bit number l to 1 in y
 *
 * Arguments in:    y, l
 *
 * Return values:
 *****************************************************************************/
#define SETBIT(y,l)           ((y) |= (1UL << (l)))
/***********************************************************CommentEnd********/

/***********************************************************CommentBegin******
 *
 * -- UNSETBIT -- Macro: set bit number l to 0 in y
 *
 * Arguments in:    y, l
 *
 * Return values:
 *****************************************************************************/
#define UNSETBIT(y,l)         ((y) &= (~(1UL << (l))))
/***********************************************************CommentEnd********/


        /*-------------------------------------*
        |       #defines
        \--------------------------------------*/
#define BUFFER_BLOCK_SIZE 512    /* Stepsize for reallocation of buffer */


/* Include prototype file */
#include "bitOut.p"

#endif /* _BITIO_H  */
