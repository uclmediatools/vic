/***********************************************************HeaderBegin*******
 *                                                                         
 * File: idct.c
 *
 * Author: Telenor, TMN
 * Created: 
 *                                                                         
 * Description: inverse discrete cosine transform
 *
 * Notes:  
 *
 * Modified: T.W., adopted from MoMuSys VM 96Nov07
 *
 ***********************************************************HeaderEnd*********/


/************************    INCLUDE FILES    ********************************/

#include <math.h>
#include <string.h> //SV-XXX: added for memcpy, line 533
#include "defs.h"
#include "Util.h"


#define MNINT(a)               ((a) < 0 ? (int)(a - 0.5) : (int)(a + 0.5))
#define PI                    3.14159265358979323846

/**********************************************************/
/* inverse two dimensional DCT, Chen-Wang algorithm       */
/* (cf. IEEE ASSP-32, pp. 803-816, Aug. 1984)             */
/* 32-bit integer arithmetic (8 bit coefficients)         */
/* 11 mults, 29 adds per DCT                              */
/*                                      sE, 18.8.91       */
/**********************************************************/
/* coefficients extended to 12 bit for IEEE1180-1990      */
/* compliance                           sE,  2.1.94       */
/**********************************************************/

/* this code assumes >> to be a two's-complement arithmetic */
/* right shift: (-2)>>1 == -1 , (-3)>>1 == -2               */


#define W1 2841 /* 2048*sqrt(2)*cos(1*pi/16) */
#define W2 2676 /* 2048*sqrt(2)*cos(2*pi/16) */
#define W3 2408 /* 2048*sqrt(2)*cos(3*pi/16) */
#define W5 1609 /* 2048*sqrt(2)*cos(5*pi/16) */
#define W6 1108 /* 2048*sqrt(2)*cos(6*pi/16) */
#define W7 565  /* 2048*sqrt(2)*cos(7*pi/16) */


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- IdctFastRow -- fast inverse DCT-transform of a row
 *
 * Author :            Robert Danielsen
 *
 * Purpose :           Fast inverse DCT-transform of a block row with
 *                     fixed point arithmetic.
 *                     Two dimensional inverse discrete cosine transform.
 *
 * Arguments in :      -
 *                     
 * Arguments in/out :  short *block : pointer to a block
 *
 * Arguments out :     -
 * 
 * Return values :     none
 *
 * Side effects :      none
 *
 * Description :       row (horizontal) IDCT
 *
 *                                 7                       pi         1
 *                       dst[k] = sum c[l] * src[l] * cos( -- * ( k + - ) * l )
 *                                l=0                      8          2
 *
 *                     where: c[0]    = 128
 *                            c[1..7] = 128*sqrt(2)
 *
 * See also :          IdctRef, IdctFastFloat, IdctFast, IdctFastClipCol
 *
 * Modified :          T.W., 11-Nov-1996, Adopted from Momusys VM
 *                     K.S., 23-Jun-1997, Adopted towards CIDS
 *
 ****************************************************************************/
void IdctFastRow(short *blk)
/***********************************************************CommentEnd********/
{
  int x0, x1, x2, x3, x4, x5, x6, x7, x8;


  /* shortcut */
  if (!((x1 = blk[4]<<11) | (x2 = blk[6]) | (x3 = blk[2]) |
        (x4 = blk[1]) | (x5 = blk[7]) | (x6 = blk[5]) | (x7 = blk[3])))
  {
    blk[0]=blk[1]=blk[2]=blk[3]=blk[4]=blk[5]=blk[6]=blk[7]=blk[0]<<3;
    return;
  }

  x0 = (blk[0]<<11) + 128; /* for proper rounding in the fourth stage */

  /* first stage */
  x8 = W7*(x4+x5);
  x4 = x8 + (W1-W7)*x4;
  x5 = x8 - (W1+W7)*x5;
  x8 = W3*(x6+x7);
  x6 = x8 - (W3-W5)*x6;
  x7 = x8 - (W3+W5)*x7;
  
  /* second stage */
  x8 = x0 + x1;
  x0 -= x1;
  x1 = W6*(x3+x2);
  x2 = x1 - (W2+W6)*x2;
  x3 = x1 + (W2-W6)*x3;
  x1 = x4 + x6;
  x4 -= x6;
  x6 = x5 + x7;
  x5 -= x7;
  
  /* third stage */
  x7 = x8 + x3;
  x8 -= x3;
  x3 = x0 + x2;
  x0 -= x2;
  x2 = (181*(x4+x5)+128)>>8;
  x4 = (181*(x4-x5)+128)>>8;
  
  /* fourth stage */
  blk[0] = (x7+x1)>>8;
  blk[1] = (x3+x2)>>8;
  blk[2] = (x0+x4)>>8;
  blk[3] = (x8+x6)>>8;
  blk[4] = (x8-x6)>>8;
  blk[5] = (x0-x4)>>8;
  blk[6] = (x3-x2)>>8;
  blk[7] = (x7-x1)>>8;
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- IdctFastColClip -- fast inverse DCT-transform of a column and clipping
 *
 * Author :            Robert Danielsen
 *
 * Purpose :           Fast inverse DCT-transform of a block column with
 *                     fixed point arithmetic.
 *                     One dimensional vertical inverse discrete cosine
 *                     transform.
 *                     The result is clipped to the range -256...255.
 *
 * Arguments in :      -
 *                     
 * Arguments in/out :  short *block : pointer to a block
 *
 * Arguments out :     -
 * 
 * Return values :     none
 *
 * Side effects :      none
 *
 * Description :       column (vertical) IDCT
 *
 *                              7                         pi         1
 *                  dst[8*k] = sum c[l] * src[8*l] * cos( -- * ( k + - ) * l )
 *                             l=0                        8          2
 *
 *                     where: c[0]    = 1/1024
 *                            c[1..7] = (1/1024)*sqrt(2)
 *
 * See also :          IdctRef, IdctFastFloat, IdctFast, IdctFastRow
 *
 * Modified :          T.W., 11-Nov-1996, Adopted from Momusys VM
 *                     K.S., 23-Jun-1997, Adopted towards CIDS
 *
 ****************************************************************************/
void IdctFastColClip(short *blk)
/***********************************************************CommentEnd********/
{
  int          x0, x1, x2, x3, x4, x5, x6, x7, x8;


  /* shortcut */
  if (!((x1 = (blk[8*4]<<8)) | (x2 = blk[8*6]) | (x3 = blk[8*2]) |
        (x4 = blk[8*1]) | (x5 = blk[8*7]) | (x6 = blk[8*5]) | (x7 = blk[8*3])))
  {
    blk[8*0]=blk[8*1]=blk[8*2]=blk[8*3]=blk[8*4]=blk[8*5]=blk[8*6]=blk[8*7]=
      ICROPBYTE((blk[8*0]+32)>>6);
    return;
  }

  x0 = (blk[8*0]<<8) + 8192;

  /* first stage */
  x8 = W7*(x4+x5) + 4;
  x4 = (x8+(W1-W7)*x4)>>3;
  x5 = (x8-(W1+W7)*x5)>>3;
  x8 = W3*(x6+x7) + 4;
  x6 = (x8-(W3-W5)*x6)>>3;
  x7 = (x8-(W3+W5)*x7)>>3;
  
  /* second stage */
  x8 = x0 + x1;
  x0 -= x1;
  x1 = W6*(x3+x2) + 4;
  x2 = (x1-(W2+W6)*x2)>>3;
  x3 = (x1+(W2-W6)*x3)>>3;
  x1 = x4 + x6;
  x4 -= x6;
  x6 = x5 + x7;
  x5 -= x7;

  /* third stage */
  x7 = x8 + x3;
  x8 -= x3;
  x3 = x0 + x2;
  x0 -= x2;
  x2 = (181*(x4+x5)+128)>>8;
  x4 = (181*(x4-x5)+128)>>8;

  /* fourth stage */
  blk[8*0] = ICROPBYTE((x7+x1)>>14);
  blk[8*1] = ICROPBYTE((x3+x2)>>14);
  blk[8*2] = ICROPBYTE((x0+x4)>>14);
  blk[8*3] = ICROPBYTE((x8+x6)>>14);
  blk[8*4] = ICROPBYTE((x8-x6)>>14);
  blk[8*5] = ICROPBYTE((x0-x4)>>14);
  blk[8*6] = ICROPBYTE((x3-x2)>>14);
  blk[8*7] = ICROPBYTE((x7-x1)>>14);
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- IdctFast -- fast inverse DCT-transform with fixed point arith.
 *
 * Author :            Robert Danielsen
 *
 * Purpose :           Fast inverse DCT-transform with fixed point
 *                     arithmetic.
 *                     Two dimensional inverse discrete cosine transform.
 *
 * Arguments in :      int *coeff : pointer to coefficients.
 *                     
 * Arguments in/out :  none
 *
 * Arguments out :     short *block : pointer to a block.
 * 
 * Return values :     none
 *
 * Side effects :      none
 *
 * Description :       Does inverse DCT-transform on 64 coefficients.
 *                     No descaning of zigzag-scanned coefficients!
 *                     
 *
 * See also :          IdctRef, IdctFastFloat, IdctFastRow, IdctFastColClip
 *
 * Modified :          T.W., 11-Nov-1996, Adopted from Momusys VM
 *
 ****************************************************************************/
void IdctFast(int *coeff, short *block)
/***********************************************************CommentEnd********/
{
  int          i;
  short        *bp = block;

  for (i = 0; i < 64; i++, bp++)
    *bp = (short)(*coeff++);

  /* Do DCTs */
  for (i = 0; i < 8; i++)
    IdctFastRow(block + 8 * i);
  for (i = 0; i < 8; i++)
    IdctFastColClip(block + i);
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- IdctFastFloat -- fast inverse DCT-transform with floating point arith.
 *
 * Author :            Robert Danielsen
 *
 * Purpose :           Fast inverse DCT-transform with floating point
 *                     arithmetic.
 * 
 * Arguments in :      int *coeff : pointer to coefficients.
 *                     
 * Arguments in/out :  none
 *
 * Arguments out :     int *block : pointer to a block.
 * 
 * Return values :     none
 *
 * Side effects :      none
 *
 * Description :       Does inverse DCT-transform on 64 coefficients.
 *                     No descaning of zigzag-scanned coefficients!
 *                     
 *
 * See also :          IdctRef, IdctFast
 *
 * Modified :          T.W., 11-Nov-1996, Adopted from Momusys VM
 *
 ****************************************************************************/
void IdctFastFloat(int *coeff, int *block) 
/***********************************************************CommentEnd********/
{
  int                j1, i, j;
  double b[8], b1[8], dd[8][8];
  double f0=.7071068;
  double f1=.4903926;
  double f2=.4619398;
  double f3=.4157348;
  double f4=.3535534;
  double f5=.2777851;
  double f6=.1913417;
  double f7=.0975452;
  double e, f, g, h;

  /* Horizontal */

  /* Descan coefficients first */

  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++) {
      b[j] = coeff[8 * i + j];
    }
    e = b[1] * f7 - b[7] * f1;
    h = b[7] * f7 + b[1] * f1;
    f = b[5] * f3 - b[3] * f5;
    g = b[3] * f3 + b[5] * f5;

    b1[0] = (b[0] + b[4]) * f4;
    b1[1] = (b[0] - b[4]) * f4;
    b1[2] = b[2] * f6 - b[6] * f2;
    b1[3] = b[6] * f6 + b[2] * f2;
    b[4] = e + f;
    b1[5] = e - f;
    b1[6] = h - g;
    b[7] = h + g;
    
    b[5] = (b1[6] - b1[5]) * f0;
    b[6] = (b1[6] + b1[5]) * f0;
    b[0] = b1[0] + b1[3];
    b[1] = b1[1] + b1[2];
    b[2] = b1[1] - b1[2];
    b[3] = b1[0] - b1[3];

    for (j = 0; j < 4; j++) {
      j1 = 7 - j;
      dd[i][j] = b[j] + b[j1];
      dd[i][j1] = b[j] - b[j1];
    }
  }

  /* Vertical */
  
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++) {
      b[j] = dd[j][i];
    }
    e = b[1] * f7 - b[7] * f1;
    h = b[7] * f7 + b[1] * f1;
    f = b[5] * f3 - b[3] * f5;
    g = b[3] * f3 + b[5] * f5;

    b1[0] = (b[0] + b[4]) * f4;
    b1[1] = (b[0] - b[4]) * f4;
    b1[2] = b[2] * f6 - b[6] * f2;
    b1[3] = b[6] * f6 + b[2] * f2;
    b[4] = e + f;
    b1[5] = e - f;
    b1[6] = h - g;
    b[7] = h + g;

    b[5] = (b1[6] - b1[5]) * f0;
    b[6] = (b1[6] + b1[5]) * f0;
    b[0] = b1[0] + b1[3];
    b[1] = b1[1] + b1[2];
    b[2] = b1[1] - b1[2];
    b[3] = b1[0] - b1[3];

    for (j = 0; j < 4; j++) {
      j1 = 7 - j;
      dd[j][i] = b[j] + b[j1];
      dd[j1][i] = b[j] - b[j1];
    }
  }

  for (i = 0; i < 8; i++) {
    for (j = 0; j < 8; j++) {
      block[i*8+j] = MNINT(dd[i][j]);
    }
  }
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- IdctRef -- inverse DCT-transform with double precision
 *
 * Author :            unknown
 *
 * Purpose :           Inverse DCT-transform
 * 
 * Arguments in :      double *coeff : pointer to coefficients.
 *                     
 * Arguments in/out :  none
 * 
 * Arguments out :     int *block : pointer to a block.
 * 
 * Return values :     none
 *
 * Side effects :      none
 *
 * Description :       Perform IDCT matrix multiply for 8x8 coefficient block.
 *                     Perform IEEE 1180 reference (64-bit floating point, 
 *                     separable 8x1 direct matrix multiply) Inverse Discrete 
 *                     Cosine Transform.
 *                     Here we use math.h to generate constants.  
 *                     Compiler results may vary a little.
 *
 * See also :
 *
 * Modified :          T.W., 11-Nov-1996, Adopted from Momusys VM
 *
 ****************************************************************************/
void IdctRef(double *coeff, int *block) 
/***********************************************************CommentEnd********/
{
  int i, j, k, v;
  double partial_product;
  double tmp[64];
  static double c[8][8];
  static int virgin = TRUE;

  if(virgin) {
    int freq, time;
    double scale;

    virgin = FALSE;
    for (freq=0; freq < 8; freq++) {
      scale = (freq == 0) ? sqrt(0.125) : 0.5;
      
      for (time=0; time<8; time++) {
	c[freq][time] = scale*cos((PI/8.0)*freq*(time + 0.5));
      }
    }
  }

  for (i=0; i<8; i++) {
    for (j=0; j<8; j++) {     
      partial_product = 0.0;
      for (k=0; k<8; k++) {
	partial_product+= c[k][j] * coeff[8 * i + j];
      }
      tmp[8*i+j] = partial_product;
    }	
  }
  
  /* Transpose operation is integrated into address mapping by switching 
     loop order of i and j */

  for (j=0; j<8; j++) {    
    for (i=0; i<8; i++) {
      partial_product = 0.0;
      for (k=0; k<8; k++) {
	partial_product+= c[k][i]*tmp[8*k+j];
      }
      v = (int) floor (partial_product + 0.5);
      block[8*i+j] = (v<-256) ? -256 : ((v>255) ? 255 : v);
    }
  }
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- Idct3coef -- fast inverse DCT-transform using only the first 3 coefs.
 *
 * Author :            K.S.
 *
 * Purpose :           Fast inverse DCT-transform using only the first 3
 *                     DCT coefficients
 *                     Two dimensional inverse discrete cosine transform.
 *
 * Arguments in :      int *coeff : pointer to coefficients.
 *
 * Arguments in/out :  none
 *
 * Arguments out :     short *block : pointer to a block.
 * 
 * Return values :     none
 *
 * Side effects :      none
 *
 * Description :       Does inverse DCT-transform on the first 3 coefficients.
 *                     No descaning of zigzag-scanned coefficients!
 *                     
 *
 * See also :          IdctFast, IdctRef, IdctFastFloat,
 *                     IdctFastRow, IdctFastColClip
 *
 * Modified :          -
 *
 ****************************************************************************/
void Idct3coef(int *coeff, short *block)
/***********************************************************CommentEnd********/
{
  int          i;
  int          dc = (coeff[0] << 11) + 8192;
  short        c;


  /* Transform first row */
  block[0] = (dc + W1 * coeff[1]) >> 14;
  block[1] = (dc + W3 * coeff[1]) >> 14;
  block[2] = (dc + W5 * coeff[1]) >> 14;
  block[3] = (dc + W7 * coeff[1]) >> 14;
  block[4] = (dc - W7 * coeff[1]) >> 14;
  block[5] = (dc - W5 * coeff[1]) >> 14;
  block[6] = (dc - W3 * coeff[1]) >> 14;
  block[7] = (dc - W1 * coeff[1]) >> 14;

  memcpy(block + 8, block, 8 * sizeof(short));
  memcpy(block + 16, block, 16 * sizeof(short));
  memcpy(block + 32, block, 32 * sizeof(short));

  /* Transform columns */  
  c = ( W1 * coeff[8] + 8192) >> 14;
  for (i = 0; i < 8; i++) {
    *block = ICROPBYTE(*block + c);
    block++;
  }
  c = ( W3 * coeff[8] + 8192) >> 14;
  for (i = 0; i < 8; i++) {
    *block = ICROPBYTE(*block + c);
    block++;
  }
  c = ( W5 * coeff[8] + 8192) >> 14;
  for (i = 0; i < 8; i++) {
    *block = ICROPBYTE(*block + c);
    block++;
  }
  c = ( W7 * coeff[8] + 8192) >> 14;
  for (i = 0; i < 8; i++) {
    *block = ICROPBYTE(*block + c);
    block++;
  }
  c = (-W7 * coeff[8] + 8192) >> 14;
  for (i = 0; i < 8; i++) {
    *block = ICROPBYTE(*block + c);
    block++;
  }
  c = (-W5 * coeff[8] + 8192) >> 14;
  for (i = 0; i < 8; i++) {
    *block = ICROPBYTE(*block + c);
    block++;
  }
  c = (-W3 * coeff[8] + 8192) >> 14;
  for (i = 0; i < 8; i++) {
    *block = ICROPBYTE(*block + c);
    block++;
  }
  c = (-W1 * coeff[8] + 8192) >> 14;
  for (i = 0; i < 8; i++) {
    *block = ICROPBYTE(*block + c);
    block++;
  }
}
