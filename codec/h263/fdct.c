
/***********************************************************HeaderBegin*******
 *                                                                         
 * File:	fdct.c
 *
 * Author:	unknown
 *
 * Created:	unknown
 *                                                                         
 * Description: forward discrete cosine transform
 *
 ***********************************************************HeaderEnd*********/

#include <string.h>
#include "defs.h"


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- FdctFast -- Fast DCT-Transform 
 *
 * Author :            Robert Danielsen
 *	
 * Purpose :           DCT-transform		
 *	
 * Arguments in :      int *block : pointer to an image block;
 *
 * Arguments in/out :  int *coeff : pointer to (to int) quantized coefficients.
 *                                  No zigzag-scan!
 *
 * Arguments out :     none
 *
 * Return values :     none
 *	
 * Side effects :      -
 *	
 * Description :       Performs fast DCT-transformation.
 *                     No zigzag-scan!
 *	
 * See also :          FdctRef, IdctRef, IdctFast
 *	
 * Modified :          23.09.96 Cor Quist: adapted to the MoMuSys VM
 *	               T.W., 11-Nov-96, adapted from Momusys VM
 *
 ****************************************************************************/
void FdctFast(int *block, int *coeff)
/***********************************************************CommentEnd********/
{
  int   j1, i, j, k;
  float	b[8];
  float b1[8];
  float d[8][8];
  float f0=(float).7071068;
  float f1=(float).4903926;
  float f2=(float).4619398;
  float f3=(float).4157348;
  float f4=(float).3535534;
  float f5=(float).2777851;
  float f6=(float).1913417;
  float f7=(float).0975452;

  for (i = 0, k = 0; i < 8; i++, k += 8) {
    for (j = 0; j < 8; j++) {
      b[j] = (float)block[k+j];
    }
    /* Horizontal transform */
    for (j = 0; j < 4; j++) {
      j1 = 7 - j;
      b1[j] = b[j] + b[j1];
      b1[j1] = b[j] - b[j1];
    }
    b[0] = b1[0] + b1[3];
    b[1] = b1[1] + b1[2];
    b[2] = b1[1] - b1[2];
    b[3] = b1[0] - b1[3];
    b[4] = b1[4];
    b[5] = (b1[6] - b1[5]) * f0;
    b[6] = (b1[6] + b1[5]) * f0;
    b[7] = b1[7];
    d[i][0] = (b[0] + b[1]) * f4;
    d[i][4] = (b[0] - b[1]) * f4;
    d[i][2] = b[2] * f6 + b[3] * f2;
    d[i][6] = b[3] * f6 - b[2] * f2;
    b1[4] = b[4] + b[5];
    b1[7] = b[7] + b[6];
    b1[5] = b[4] - b[5];
    b1[6] = b[7] - b[6];
    d[i][1] = b1[4] * f7 + b1[7] * f1;
    d[i][5] = b1[5] * f3 + b1[6] * f5;
    d[i][7] = b1[7] * f7 - b1[4] * f1;
    d[i][3] = b1[6] * f3 - b1[5] * f5;
  }
  /* Vertical transform */
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 4; j++) {
      j1 = 7 - j;
      b1[j] = d[j][i] + d[j1][i];
      b1[j1] = d[j][i] - d[j1][i];
    }
    b[0] = b1[0] + b1[3];
    b[1] = b1[1] + b1[2];
    b[2] = b1[1] - b1[2];
    b[3] = b1[0] - b1[3];
    b[4] = b1[4];
    b[5] = (b1[6] - b1[5]) * f0;
    b[6] = (b1[6] + b1[5]) * f0;
    b[7] = b1[7];
    d[0][i] = (b[0] + b[1]) * f4;
    coeff[0] = (int)d[0][i];
    d[4][i] = (b[0] - b[1]) * f4;
    coeff[4*8] = (int)d[4][i];
    d[2][i] = b[2] * f6 + b[3] * f2;
    coeff[2*8] = (int)d[2][i];
    d[6][i] = b[3] * f6 - b[2] * f2;
    coeff[6*8] = (int)d[6][i];
    b1[4] = b[4] + b[5];
    b1[7] = b[7] + b[6];
    b1[5] = b[4] - b[5];
    b1[6] = b[7] - b[6];
    d[1][i] = b1[4] * f7 + b1[7] * f1;
    coeff[1*8] = (int)d[1][i];
    d[5][i] = b1[5] * f3 + b1[6] * f5;
    coeff[5*8] = (int)d[5][i];
    d[7][i] = b1[7] * f7 - b1[4] * f1;
    coeff[7*8] = (int)d[7][i];
    d[3][i] = b1[6] * f3 - b1[5] * f5;
    coeff[3*8] = (int)d[3][i];
    coeff++;
  }
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- FdctFast -- Fast DCT-Transform 
 *
 * Author :            Robert Danielsen
 *	
 * Purpose :           DCT-transform		
 *	
 * Arguments in :      Byte *block     pointer to an image block;
 *                     int  w          picture width = skip between block lines
 *
 * Arguments in/out :  int *coeff : pointer to (to int) quantized coefficients.
 *
 * Arguments out :     none
 *
 * Return values :     none
 *	
 * Side effects :      -
 *	
 * Description :       Performs fast DCT-transformation
 *                     No zigzag-scan!
 *	
 * See also :          FdctRef, IdctRef, IdctFast
 *	
 * Modified :          23.09.96 Cor Quist: adapted to the MoMuSys VM
 *	               T.W., 11-Nov-96, adapted from Momusys VM
 *
 ****************************************************************************/
void FdctFastByte(Byte *block, int w, int *coeff)
/***********************************************************CommentEnd********/
{
  int   j1, i, j, k;
  float	b[8];
  float b1[8];
  float d[8][8];
  float f0=(float).7071068;
  float f1=(float).4903926;
  float f2=(float).4619398;
  float f3=(float).4157348;
  float f4=(float).3535534;
  float f5=(float).2777851;
  float f6=(float).1913417;
  float f7=(float).0975452;

  for (i = 0, k = 0; i < 8; i++, k += w) {
    for (j = 0; j < 8; j++) {
      b[j] = (float)block[k+j];
    }
    /* Horizontal transform */
    for (j = 0; j < 4; j++) {
      j1 = 7 - j;
      b1[j] = b[j] + b[j1];
      b1[j1] = b[j] - b[j1];
    }
    b[0] = b1[0] + b1[3];
    b[1] = b1[1] + b1[2];
    b[2] = b1[1] - b1[2];
    b[3] = b1[0] - b1[3];
    b[4] = b1[4];
    b[5] = (b1[6] - b1[5]) * f0;
    b[6] = (b1[6] + b1[5]) * f0;
    b[7] = b1[7];
    d[i][0] = (b[0] + b[1]) * f4;
    d[i][4] = (b[0] - b[1]) * f4;
    d[i][2] = b[2] * f6 + b[3] * f2;
    d[i][6] = b[3] * f6 - b[2] * f2;
    b1[4] = b[4] + b[5];
    b1[7] = b[7] + b[6];
    b1[5] = b[4] - b[5];
    b1[6] = b[7] - b[6];
    d[i][1] = b1[4] * f7 + b1[7] * f1;
    d[i][5] = b1[5] * f3 + b1[6] * f5;
    d[i][7] = b1[7] * f7 - b1[4] * f1;
    d[i][3] = b1[6] * f3 - b1[5] * f5;
  }
  /* Vertical transform */
  for (i = 0; i < 8; i++) {
    for (j = 0; j < 4; j++) {
      j1 = 7 - j;
      b1[j] = d[j][i] + d[j1][i];
      b1[j1] = d[j][i] - d[j1][i];
    }
    b[0] = b1[0] + b1[3];
    b[1] = b1[1] + b1[2];
    b[2] = b1[1] - b1[2];
    b[3] = b1[0] - b1[3];
    b[4] = b1[4];
    b[5] = (b1[6] - b1[5]) * f0;
    b[6] = (b1[6] + b1[5]) * f0;
    b[7] = b1[7];
    d[0][i] = (b[0] + b[1]) * f4;
    coeff[0] = (int)d[0][i];
    d[4][i] = (b[0] - b[1]) * f4;
    coeff[4*8] = (int)d[4][i];
    d[2][i] = b[2] * f6 + b[3] * f2;
    coeff[2*8] = (int)d[2][i];
    d[6][i] = b[3] * f6 - b[2] * f2;
    coeff[6*8] = (int)d[6][i];
    b1[4] = b[4] + b[5];
    b1[7] = b[7] + b[6];
    b1[5] = b[4] - b[5];
    b1[6] = b[7] - b[6];
    d[1][i] = b1[4] * f7 + b1[7] * f1;
    coeff[8] = (int)d[1][i];
    d[5][i] = b1[5] * f3 + b1[6] * f5;
    coeff[5*8] = (int)d[5][i];
    d[7][i] = b1[7] * f7 - b1[4] * f1;
    coeff[7*8] = (int)d[7][i];
    d[3][i] = b1[6] * f3 - b1[5] * f5;
    coeff[3*8] = (int)d[3][i];
    coeff++;
  }
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- Fdct3coef --  Very fast DCT-Transform computing only the first 3 coeffs
 *
 * Author :            K.S.
 *
 * Purpose :           Extremly fast DCT-transform
 *
 * Arguments in :      int *block : pointer to an image block;
 *
 * Arguments in/out :  int *coeff : pointer to (to int) quantized coefficients.
 *                                  No zigzag-scan!
 *
 * Arguments out :     none
 *
 * Return values :     none
 *
 * Side effects :      -
 *
 * Description :       Performs fast DCT-transformation.
 *                     No zigzag-scan. Only the DC and the first hor. and vert.
 *                     coefficients are computed by using only the border
 *                     pels of the block. The other coefficients are set to
 *                     zero.
 *
 * See also :          FdctVeryFast, FdctFast, FdctRef, IdctRef, IdctFast
 *
 * Modified :          
 *
 ****************************************************************************/
void Fdct3coef(int *block, int *coeff)
/***********************************************************CommentEnd********/
{
  int xu, xd, xl, xr;


  /* Sum over border pels */
  xu = block[0] + block[1] + block[2] + block[3] +
    block[4] + block[5] + block[6] + block[7];
  xd = block[56] + block[57] + block[58] + block[59] +
    block[60] + block[61] + block[62] + block[63];
  xl = block[0] + block[8] + block[16] + block[24] +
    block[32] + block[40] + block[48] + block[56];
  xr = block[7] + block[15] + block[23] + block[31] +
    block[39] + block[47] + block[55] + block[63];

  *coeff++ = (xu + xd + xl + xr) >> 2;   /* /4 */
  *coeff++ = ((xl - xr) * 21) >> 6;      /* /64 */
  /* Reset the remaining coefficients */
  memset(coeff, 0, (64 - 2) * sizeof(int));
  coeff += 6;
  *coeff = ((xu - xd) * 21) >> 6;      /* /64 */
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- Fdct3coefByte --  Very fast DCT computing only the first 3 coeffs 
 *
 * Author :            K.S.
 *
 * Purpose :           Extremly fast DCT-transform
 *
 * Arguments in :      Byte *block : pointer to an image block;
 *                     int  w      : image width
 *
 * Arguments in/out :  int *coeff : pointer to (to Int) quantized coefficients
 *                                  in zigzag-scan order.
 *
 * Arguments out :     none
 *
 * Return values :     none
 *
 * Side effects :      -
 *
 * Description :       Performs fast DCT-transformation orders the transform
 *                     coefficients in zigzag-scan. Only the first three
 *                     coefficients are computed by using only the border
 *                     pels of the block. The other coefficients are set to
 *                     zero.
 *
 * See also :          Fdct3coef, FdctFast, FdctRef, IdctRef, IdctFast
 *
 * Modified :          
 *
 ****************************************************************************/
void Fdct3coefByte(Byte *block, int w, int *coeff) 
/***********************************************************CommentEnd********/
{
  int xu, xd, xl, xr;
  Byte *b;


  /* Sum over border pels */
  xu = block[0] + block[1] + block[2] + block[3] +
    block[4] + block[5] + block[6] + block[7];
  b = block + 7;
  xr = *b; b += w;
  xr += *b; b += w;
  xr += *b; b += w;
  xr += *b; b += w;
  xr += *b; b += w;
  xr += *b; b += w;
  xr += *b;
  b = block;
  xl = *b; b += w;
  xl += *b; b += w;
  xl += *b; b += w;
  xl += *b; b += w;
  xl += *b; b += w;
  xl += *b; b += w;
  xl += *b;
  xd = b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + b[7];

  *coeff++ = (xu + xd + xl + xr) >> 2;   /* /4 */
  *coeff++ = ((xl - xr) * 21) >> 6;      /* /64 */
  /* Reset the remaining coefficients */
  memset(coeff, 0, (64 - 2) * sizeof(int));
  coeff += 6;
  *coeff = ((xu - xd) * 21) >> 6;      /* /64 */
}
