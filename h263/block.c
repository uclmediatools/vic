#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "structs.h"
#include "Util.h"
#include "Motion.h"


/* Subsampling factor for SAD measurement */
int subsampleSAD = 1;    /* may be 1, 2 or 4 */

/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- MacroBlockPictureIntraSAD -- Compute the Intra SAD of a MB in a Picture
 *
 * Author:              K.S.
 *
 * Created:             6-Nov-97
 *
 * Purpose:             Intra SAD of y component
 *
 * Arguments in:        int      xPos       x position of macroblock
 *                      int      yPos       y position of macroblock
 *                      Picture  *pict      picture
 *                      int      max        stop computation when 'max' is
 *                                          exceeded.
 *
 * Arguments in/out:    -
 *
 * Arguments out:       -
 *
 * Return values:       int      sadI        variance.
 *
 * Example:             sadI = MacroBlockPictureIntraSAD(i*16, j*16, pict,max);
 *
 * Side effects:        The measurement depends on the global variable
 *                      'subsampleSAD'.
 *
 * Description:         -
 *
 * See also:            MacroBlockPictureIntraSAD
 *
 * Modified:            -
 *
 *****************************************************************************/
int MacroBlockPictureIntraSAD(int xPos, int yPos, Picture *pict, int max)
 /**********************************************************CommentEnd********/
{
  const int  w = pict->w;
  const int  wm1 = subsampleSAD * w - 16;
  int  sadI = 0;
  int  sum = 0;
  int  tmp;
  int  x, y;
  Byte *p;


  p = pict->y + xPos + yPos * w;
  for (y = subsampleSAD / 2; y < 16; y += subsampleSAD) {
    for (x = subsampleSAD / 2; x < 16; x += subsampleSAD) {
      sum += (int)(*p);
      p += subsampleSAD;
    }
    p += wm1;
  }
  if (subsampleSAD > 1)
    sum *= SQR(subsampleSAD);
  sum = (sum + 128) >> 8;

  p = pict->y + xPos + yPos * w;
  for (y = subsampleSAD / 2; y < 16; y += subsampleSAD) {
    for (x = subsampleSAD / 2; x < 16; x += subsampleSAD) {
      tmp = ABS((int)(*p) - sum);
      p += subsampleSAD;
      sadI += tmp;
    }
    p += wm1;
    if (sadI > max)
      break;
  }

  if (subsampleSAD > 1)
    sadI *= SQR(subsampleSAD);

  return(sadI);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- MacroBlockPictureIntraVar -- Compute the Intra Var of a MB in a Picture
 *
 * Author:              K.S.
 *
 * Created:             12-Jun-98
 *
 * Purpose:             Intra Varianz of y component
 *
 * Arguments in:        int      xPos       x position of macroblock
 *                      int      yPos       y position of macroblock
 *                      Picture  *pict      picture
 *                      int      max        stop computation when 'max' is
 *                                          exceeded.
 *
 * Arguments in/out:    -
 *
 * Arguments out:       -
 *
 * Return values:       int      var        variance.
 *
 * Example:             var = MacroBlockPictureIntraVar(i*16, j*16, pict,max);
 *
 * Side effects:        The measurement depends on the global variable
 *                      'subsampleSAD'.
 *
 * Description:         -
 *
 * See also:            MacroBlockPictureIntraVar
 *
 * Modified:            -
 *
 *****************************************************************************/
int MacroBlockPictureIntraVar(int xPos, int yPos, Picture *pict, int max)
 /**********************************************************CommentEnd********/
{
  const int  w = pict->w;
  const int  wm1 = subsampleSAD * w - 16;
  int  varI = 0;
  int  sum = 0;
  int  tmp;
  int  x, y;
  Byte *p;


  p = pict->y + xPos + yPos * w;
  for (y = subsampleSAD / 2; y < 16; y += subsampleSAD) {
    for (x = subsampleSAD / 2; x < 16; x += subsampleSAD) {
      sum += (int)(*p);
      p += subsampleSAD;
    }
    p += wm1;
  }
  if (subsampleSAD > 1)
    sum *= SQR(subsampleSAD);
  sum = (sum + 128) >> 8;

  p = pict->y + xPos + yPos * w;
  for (y = subsampleSAD / 2; y < 16; y += subsampleSAD) {
    for (x = subsampleSAD / 2; x < 16; x += subsampleSAD) {
      tmp = SQR((int)(*p) - sum);
      p += subsampleSAD;
      varI += tmp;
    }
    p += wm1;
    if (varI > max)
      break;
  }

  if (subsampleSAD > 1)
    varI *= SQR(subsampleSAD);

  return(varI);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- MacroBlockPictureSAD -- Compute the SAD of a MB in two Pictures
 *
 * Author:              K.S.
 *
 * Created:             6-Nov-97
 *
 * Purpose:             SAD of y component
 *
 * Arguments in:        int      xPos       x position of macroblock
 *                      int      yPos       y position of macroblock
 *                      Picture  *pict1     first picture
 *                      Picture  *pict2     second picture
 *                      int      max        stop computation when 'max' is
 *                                          exceeded.
 *
 * Arguments in/out:    -
 *
 * Arguments out:       -
 *
 * Return values:       int      SAD        summed absolute difference.
 *
 * Example:             sad = MacroBlockPictureSAD(i*16, j*16, pict, predPict,
 *                                                 max);
 *
 * Side effects:        The measurement depends on the global variable
 *                      'subsampleSAD'.
 *
 * Description:         -
 *
 * See also:            MacroBlockPictureVar
 *
 * Modified:            -
 *
 *****************************************************************************/
int MacroBlockPictureSAD(int xPos, int yPos, Picture *pict1, Picture *pict2,
			 int max)
 /**********************************************************CommentEnd********/
{
  const int  w = pict1->w;
  const int  wm1 = subsampleSAD * w - 16;
  int  sad = 0;
  int  tmp;
  int  x, y;
  Byte *p1, *p2;


  p1 = pict1->y + xPos + yPos * w;
  p2 = pict2->y + xPos + yPos * w;

  for (y = subsampleSAD / 2; y < 16; y += subsampleSAD) {
    for (x = subsampleSAD / 2; x < 16; x += subsampleSAD) {
      tmp = (int)(*p1) - (int)(*p2);
      p1 += subsampleSAD;
      p2 += subsampleSAD;
      sad += ABS(tmp);
    }
    p1 += wm1;
    p2 += wm1;
    if (sad > max)
      break;
  }

  if (subsampleSAD > 1)
    sad *= SQR(subsampleSAD);

  return(sad);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- FPMotionMacroBlockPictureSAD -- Compute SAD of a MB with full pel motion
 *
 * Author:              K.S.
 *
 * Created:             17-Nov-97
 *
 * Purpose:             SAD of y component between displaced blocks
 *
 * Arguments in:        int      xPos       x position of macroblock
 *                      int      yPos       y position of macroblock
 *                      Picture  *pict      picture
 *                      Picture  *refPict   reference picture
 *                      MVector  *mv        motion vector
 *                      int      max        stop computation when 'max' is
 *                                          exceeded.
 *
 * Arguments in/out:    -
 *
 * Arguments out:       -
 *
 * Return values:       int      SAD        summed absolute difference.
 *
 * Example:             sad = FPMotionMacroBlockPictureSAD(i*16, j*16, pict,
 *                                                         predPict, mv, max);
 *
 * Side effects:        The measurement depends on the global variable
 *                      'subsampleSAD'.
 *
 * Description:         It is not checked, if the blocks are inside the
 *                      pictures. This has to be done outside before.
 *
 * See also:            HPMotionMacroBlockPictureSAD, MacroBlockPictureSAD,
 *                      MacroBlockPictureVar
 *
 * Modified:            -
 *
 *****************************************************************************/
int FPMotionMacroBlockPictureSAD(int xPos, int yPos, Picture *pict,
				 Picture *refPict, MVector *mv, int max)
 /**********************************************************CommentEnd********/
{
  const int  w = pict->w;
  const int  wm1 = subsampleSAD * w - 16;
  int  sad = 0;
  int  tmp;
  int  x, y;
  Byte *p, *rp;


  p = pict->y + xPos + yPos * w;
  rp = refPict->y + xPos + mv->x + (yPos + mv->y) * w;

  for (y = subsampleSAD / 2; y < 16; y += subsampleSAD) {
    for (x = subsampleSAD / 2; x < 16; x += subsampleSAD) {
      tmp = (int)(*p) - (int)(*rp);
      p += subsampleSAD;
      rp += subsampleSAD;
      sad += ABS(tmp);
    }
    p += wm1;
    rp += wm1;
    if (sad > max)
      break;
  }

  if (subsampleSAD > 1)
    sad *= SQR(subsampleSAD);

  return(sad);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- HPMotionMacroBlockPictureSAD -- Compute SAD of a MB with half pel motion
 *
 * Author:              K.S.
 *
 * Created:             17-Nov-97
 *
 * Purpose:             SAD of y component between displaced blocks
 *
 * Arguments in:        int      xPos          x position of macroblock
 *                      int      yPos          y position of macroblock
 *                      Picture  *pict         picture
 *                      Byte     *interpol[3]  half pel interpolated versions
 *                                             of reference picture
 *                      MVector  *mv           motion vector
 *                      int      max           stop computation when 'max' is
 *                                             exceeded.
 *
 * Arguments in/out:    -
 *
 * Arguments out:       -
 *
 * Return values:       int      SAD        summed absolute difference.
 *
 * Example:             sad = HPMotionMacroBlockPictureSAD(i*16, j*16, pict,
 *                                                         interpol, mv, max);
 *
 * Side effects:        The measurement depends on the global variable
 *                      'subsampleSAD'.
 *
 * Description:         It is not checked, if the blocks are inside the
 *                      pictures. This has to be done outside before.
 *                      The half pel position is determined by checking
 *                      which half pel component of the motion vector is 0.
 *
 * See also:            FPMotionMacroBlockPictureSAD, MacroBlockPictureSAD,
 *                      MacroBlockPictureVar
 *
 * Modified:            -
 *
 *****************************************************************************/
int HPMotionMacroBlockPictureSAD(int xPos, int yPos, Picture *pict,
				 Byte *interpol[3], MVector *mv, int max)
 /**********************************************************CommentEnd********/
{
  const int  w = pict->w;
  const int  wm1 = subsampleSAD * w - 16;
  int  sad = 0;
  int  tmp;
  int  x, y;
  Byte *p, *rp;


  p = pict->y + xPos + yPos * w;
  if (mv->sy == 0)
    rp = interpol[0] + xPos + mv->x + (yPos + mv->y) * w;
  else if (mv->sx == 0)
    rp = interpol[1] + xPos + mv->x + (yPos + mv->y) * w;
  else
    rp = interpol[2] + xPos + mv->x + (yPos + mv->y) * w;

  for (y = subsampleSAD / 2; y < 16; y += subsampleSAD) {
    for (x = subsampleSAD / 2; x < 16; x += subsampleSAD) {
      tmp = (int)(*p) - (int)(*rp);
      p += subsampleSAD;
      rp += subsampleSAD;
      sad += ABS(tmp);
    }
    p += wm1;
    rp += wm1;
    if (sad > max)
      break;
  }

  if (subsampleSAD > 1)
    sad *= SQR(subsampleSAD);

  return(sad);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- MacroBlockPictureSSE -- Compute the SSE of a MB in two Pictures
 *
 * Author:              K.S.
 *
 * Created:             21-Jan-98
 *
 * Purpose:             summed squared error SSE of y component
 *
 * Arguments in:        int      xPos       x position of macroblock
 *                      int      yPos       y position of macroblock
 *                      Picture  *pict1     first picture
 *                      Picture  *pict2     second picture
 *                      int      max        stop computation when 'max' is
 *                                          exceeded.
 *
 * Arguments in/out:    -
 *
 * Arguments out:       -
 *
 * Return values:       int      SSE        summed squared error.
 *
 * Example:             sse = MacroBlockPictureSSE(i*16, j*16, pict, predPict,
 *                                                 max);
 *
 * Side effects:        The measurement depends on the global variable
 *                      'subsampleSAD'.
 *
 * Description:         -
 *
 * See also:            MacroBlockPictureSAD
 *
 * Modified:            -
 *
 *****************************************************************************/
int MacroBlockPictureSSE(int xPos, int yPos, Picture *pict1, Picture *pict2,
			 int max)
 /**********************************************************CommentEnd********/
{
  const int  w = pict1->w;
  const int  wm1 = subsampleSAD * w - 16;
  int  sse = 0;
  int  tmp;
  int  x, y;
  Byte *p1, *p2;


  p1 = pict1->y + xPos + yPos * w;
  p2 = pict2->y + xPos + yPos * w;

  for (y = subsampleSAD / 2; y < 16; y += subsampleSAD) {
    for (x = subsampleSAD / 2; x < 16; x += subsampleSAD) {
      tmp = (int)(*p1) - (int)(*p2);
      p1 += subsampleSAD;
      p2 += subsampleSAD;
      sse += SQR(tmp);
    }
    p1 += wm1;
    p2 += wm1;
    if (sse > max)
      break;
  }

  if (subsampleSAD > 1)
    sse *= SQR(subsampleSAD);

  return(sse);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- ResetMacroBlockPicture -- Reset a macroblock in a picture
 *
 * Author:              K.S.
 *
 * Created:             6-Nov-97
 *
 * Purpose:             -
 *
 * Arguments in:        int      xPos       x position of macroblock
 *                      int      yPos       y position of macroblock
 *                      Picture  *pict      picture
 *
 * Arguments in/out:    -
 *
 * Arguments out:       -
 *
 * Return values:       -
 *
 * Example:             ResetMacroBlockPicture(i*16, j*16, pict);
 *
 * Side effects:        -
 *
 * Description:         -
 *
 * See also:            -
 *
 * Modified:            -
 *
 *****************************************************************************/
void ResetMacroBlockPicture(int xPos, int yPos, Picture *pict)
 /**********************************************************CommentEnd********/
{
  const int  w = pict->w;
  const int  wc = w >> 1;
  int  z;
  Byte *p;


  /* Y */
  p = pict->y + xPos + yPos * w;
  for (z = 0; z < 16; z++) {
    memset(p, 0, 16 * sizeof(Byte));
    p += w;
  }

  /* Position in color components */
  xPos /= 2;
  yPos /= 2;

  /* U */
  p = pict->u + xPos + yPos * wc;
  for (z = 0; z < 8; z++) {
    memset(p, 0, 8 * sizeof(Byte));
    p += wc;
  }

  /* V */
  p = pict->v + xPos + yPos * wc;
  for (z = 0; z < 8; z++) {
    memset(p, 0, 8 * sizeof(Byte));
    p += wc;
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- CopyMacroBlockPicture -- Copy a macroblock from one picture to the other
 *
 * Author:              K.S.
 *
 * Created:             6-Nov-97
 *
 * Purpose:             -
 *
 * Arguments in:        int      xPos       x position of macroblock
 *                      int      yPos       y position of macroblock
 *                      Picture  *pictIn    input picture
 *                      Picture  *pictOut   output picture
 *
 * Arguments in/out:    -
 *
 * Arguments out:       -
 *
 * Return values:       -
 *
 * Example:             CopyMacroBlockPicture(xPos, yPos, predPict, decPict);
 *
 * Side effects:        -
 *
 * Description:         -
 *
 * See also:            -
 *
 * Modified:            -
 *
 *****************************************************************************/
void CopyMacroBlockPicture(int xPos, int yPos, Picture *pictIn,
			   Picture *pictOut)
 /**********************************************************CommentEnd********/
{
  const int  w = pictIn->w;
  const int  wc = w >> 1;
  int  z;
  Byte *pIn, *pOut;


  /* Y */
  pIn = pictIn->y + xPos + yPos * w;
  pOut = pictOut->y + xPos + yPos * w;
  for (z = 0; z < MACROBLOCK_SIZE; z++) {
    memcpy(pOut, pIn, MACROBLOCK_SIZE * sizeof(Byte));
    pIn += w;
    pOut += w;
  }

  /* Position in color components */
  xPos /= 2;
  yPos /= 2;

  /* U */
  pIn = pictIn->u + xPos + yPos * wc;
  pOut = pictOut->u + xPos + yPos * wc;
  for (z = 0; z < MACROBLOCK_SIZE/2; z++) {
    memcpy(pOut, pIn, MACROBLOCK_SIZE/2 * sizeof(Byte));
    pIn += wc;
    pOut += wc;
  }

  /* V */
  pIn = pictIn->v + xPos + yPos * wc;
  pOut = pictOut->v + xPos + yPos * wc;
  for (z = 0; z < MACROBLOCK_SIZE/2; z++) {
    memcpy(pOut, pIn, MACROBLOCK_SIZE/2 * sizeof(Byte));
    pIn += wc;
    pOut += wc;
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- MCMacroBlockPicture -- Motion compensate a macroblock
 *
 * Author:              K.S.
 *
 * Created:             17-Nov-97
 *
 * Purpose:             -
 *
 * Arguments in:        int      xPos          x position of macroblock
 *                      int      yPos          y position of macroblock
 *                      Picture  *pictIn       input picture
 *                      Byte     *interpol[3]  interpolated versions of pictIn
 *                      MVector  *mv           motion vector
 *
 * Arguments in/out:    -
 *
 * Arguments out:       Picture  *pictOut      output picture
 *
 * Return values:       -
 *
 * Example:             MCMacroBlockPicture(xPos, yPos, predPict, interpol, mv,
 *                                          decPict);
 *
 * Side effects:        -
 *
 * Description:         It is not checked if the MB lies inside the picture.
 *
 * See also:            -
 *
 * Modified:            -
 *
 *****************************************************************************/
void MCMacroBlockPicture(int xPos, int yPos, Picture *pictIn,
			 Byte *interpol[3], MVector *mv, Picture *pictOut)
 /**********************************************************CommentEnd********/
{
  const int  w = pictIn->w;
  const int  wc = w >> 1;
  const int  wcm = wc - 8;
  int  x, y, z, tmp;
  const int  scx = mv->x % 2 != 0;
  const int  scy = mv->y % 2 != 0;
  const int  sx = mv->sx || scx;
  const int  sy = mv->sy || scy;
  Byte *pIn, *pIn1, *pOut;


  /* Y */
  if (mv->sx == 0 && mv->sy == 0)  /* no half pel */
    pIn = pictIn->y + xPos + mv->x + (yPos + mv->y) * w;
  else if (mv->sy == 0)    /* horizontal half pel */
    pIn = interpol[0] + xPos + mv->x + (yPos + mv->y) * w;
  else if (mv->sx == 0)    /* vertical half pel */
    pIn = interpol[1] + xPos + mv->x + (yPos + mv->y) * w;
  else                     /* horizontal and vertical half pel */
    pIn = interpol[2] + xPos + mv->x + (yPos + mv->y) * w;
  pOut = pictOut->y + xPos + yPos * w;
  for (z = 0; z < 16; z++) {
    memcpy(pOut, pIn, 16 * sizeof(Byte));
    pIn += w;
    pOut += w;
  }

  /* Position in color components */
  xPos /= 2;
  yPos /= 2;

  /* U */
  pOut = pictOut->u + xPos + yPos * wc;
  if (!sx && !sy) {   /* no half pel */
    pIn = pictIn->u + xPos + mv->x / 2 + (yPos + mv->y / 2) * wc;
    for (z = 0; z < 8; z++) {
      memcpy(pOut, pIn, 8 * sizeof(Byte));
      pIn += wc;
      pOut += wc;
    }
  } else if (!sy) {
    pIn = pictIn->u + xPos + mv->x / 2 + (yPos + mv->y / 2) * wc;
    if (mv->x < 0 && scx)
      pIn--;
    for (y = 0; y < 8; y++) {
      for (x = 0; x < 8; x++) {
	tmp = (int)(*pIn++) + 1;
	tmp += (int)(*pIn);
	tmp /= 2;
	*pOut++ = (Byte)tmp;
      }
      pIn += wcm;
      pOut += wcm;
    }
  } else if (!sx) {
    pIn = pictIn->u + xPos + mv->x / 2 + (yPos + mv->y / 2) * wc;
    if (mv->y < 0 && scy)
      pIn -= wc;
    pIn1 = pIn + wc;
    for (y = 0; y < 8; y++) {
      for (x = 0; x < 8; x++) {
	tmp = (int)(*pIn++) + 1;
	tmp += (int)(*pIn1++);
	tmp /= 2;
	*pOut++ = (Byte)tmp;
      }
      pIn += wcm;
      pIn1 += wcm;
      pOut += wcm;
    }
  } else {
    pIn = pictIn->u + xPos + mv->x / 2 + (yPos + mv->y / 2) * wc;
    if (mv->x < 0 && scx)
      pIn--;
    if (mv->y < 0 && scy)
      pIn -= wc;
    pIn1 = pIn + wc;
    for (y = 0; y < 8; y++) {
      for (x = 0; x < 8; x++) {
	tmp = (int)(*pIn++) + 2;
	tmp += (int)(*pIn);
	tmp += (int)(*pIn1++);
	tmp += (int)(*pIn1);
	tmp /= 4;
	*pOut++ = (Byte)tmp;
      }
      pIn += wcm;
      pIn1 += wcm;
      pOut += wcm;
    }
  }

  /* V */
  pOut = pictOut->v + xPos + yPos * wc;
  if (!sx && !sy) {   /* no half pel */
    pIn = pictIn->v + xPos + mv->x / 2 + (yPos + mv->y / 2) * wc;
    for (z = 0; z < 8; z++) {
      memcpy(pOut, pIn, 8 * sizeof(Byte));
      pIn += wc;
      pOut += wc;
    }
  } else if (!sy) {
    pIn = pictIn->v + xPos + mv->x / 2 + (yPos + mv->y / 2) * wc;
    if (mv->x < 0 && scx)
      pIn--;
    for (y = 0; y < 8; y++) {
      for (x = 0; x < 8; x++) {
	tmp = (int)(*pIn++) + 1;
	tmp += (int)(*pIn);
	tmp /= 2;
	*pOut++ = (Byte)tmp;
      }
      pIn += wcm;
      pOut += wcm;
    }
  } else if (!sx) {
    pIn = pictIn->v + xPos + mv->x / 2 + (yPos + mv->y / 2) * wc;
    if (mv->y < 0 && scy)
      pIn -= wc;
    pIn1 = pIn + wc;
    for (y = 0; y < 8; y++) {
      for (x = 0; x < 8; x++) {
	tmp = (int)(*pIn++) + 1;
	tmp += (int)(*pIn1++);
	tmp /= 2;
	*pOut++ = (Byte)tmp;
      }
      pIn += wcm;
      pIn1 += wcm;
      pOut += wcm;
    }
  } else {
    pIn = pictIn->v + xPos + mv->x / 2 + (yPos + mv->y / 2) * wc;
    if (mv->x < 0 && scx)
      pIn--;
    if (mv->y < 0 && scy)
      pIn -= wc;
    pIn1 = pIn + wc;
    for (y = 0; y < 8; y++) {
      for (x = 0; x < 8; x++) {
	tmp = (int)(*pIn++) + 2;
	tmp += (int)(*pIn);
	tmp += (int)(*pIn1++);
	tmp += (int)(*pIn1);
	tmp /= 4;
	*pOut++ = (Byte)tmp;
      }
      pIn += wcm;
      pIn1 += wcm;
      pOut += wcm;
    }
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- ExtractMacroBlock -- Extract a macroblock from a picture
 *
 * Author:              K.S.
 *
 * Created:             20-Jan-98
 *
 * Purpose:             -
 *
 * Arguments in:        int      xPos          x position of macroblock
 *                      int      yPos          y position of macroblock
 *                      Picture  *pict         picture from
 *                                             which the block shall be
 *                                             extracted.
 *
 * Arguments in/out:    -
 *
 * Arguments out:       Byte     block[6*64]   extracted block.
 *                                             The memory must be big enough
 *                                             to hold data of 6 blocks.
 *
 * Return values:       -
 *
 * Example:             ExtractMacroBlock(xPos, yPos, pict, block);
 *
 * Side effects:        -
 *
 * Description:         It is not checked if the block lies inside the picture.
 *                      First 16x16 pels are copied from 'pict->y' to 'block'.
 *                      Then 8x8 pels are copied from 'pict->u' to
 *                      'block+4*64' and from 'pict->v' to 'block+5*64'.
 *
 * See also:            -
 *
 * Modified:            -
 *
 *****************************************************************************/
void ExtractMacroBlock(int xPos, int yPos, Picture *pict, Byte *block)
 /**********************************************************CommentEnd********/
{
  const int w = pict->w;
  const int wc = w / 2;
  int       y;
  Byte      *bp;


  /* Y */
  bp = pict->y + xPos + yPos * w;
  for (y = 0; y < 16; y++) {
    memcpy(block, bp, 16 * sizeof(Byte));
    block += 16 * sizeof(Byte);
    bp += w;
  }
  /* Colors */
  xPos /= 2;
  yPos /= 2;
  /* U */
  bp = pict->u + xPos + yPos * wc;
  for (y = 0; y < 8; y++) {
    memcpy(block, bp, 8 * sizeof(Byte));
    block += 8 * sizeof(Byte);
    bp += wc;
  }
  /* V */
  bp = pict->v + xPos + yPos * wc;
  for (y = 0; y < 8; y++) {
    memcpy(block, bp, 8 * sizeof(Byte));
    block += 8 * sizeof(Byte);
    bp += wc;
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- PlaceMacroBlock -- Place a macroblock in a picture
 *
 * Author:              K.S.
 *
 * Created:             20-Jan-98
 *
 * Purpose:             -
 *
 * Arguments in:        int      xPos          x position of macroblock
 *                      int      yPos          y position of macroblock
 *                      Byte     block[6*64]   block that shall be placed.
 *                                             into the picture.
 *
 * Arguments in/out:    -
 *
 * Arguments out:       Picture  *pict         picture in which the block
 *                                             shall be placed.
 *
 * Return values:       -
 *
 * Example:             PlaceMacroBlock(xPos, yPos, block, pict);
 *
 * Side effects:        -
 *
 * Description:         It is not checked if the block lies inside the picture.
 *                      First 16x16 pels are copied from 'block' to 'pict->y'.
 *                      Then 8x8 pels are copied from 'block+4*64' to
 *                      'pict->u' and from 'block+5*64' to 'pict->v'.
 *
 * See also:            -
 *
 * Modified:            -
 *
 *****************************************************************************/
void PlaceMacroBlock(int xPos, int yPos, Byte *block, Picture *pict)
 /**********************************************************CommentEnd********/
{
  const int w = pict->w;
  const int wc = w / 2;
  int       y;
  Byte      *bp;


  /* Y */
  bp = pict->y + xPos + yPos * w;
  for (y = 0; y < 16; y++) {
    memcpy(bp, block, 16 * sizeof(Byte));
    block += 16 * sizeof(Byte);
    bp += w;
  }
  /* Colors */
  xPos /= 2;
  yPos /= 2;
  /* U */
  bp = pict->u + xPos + yPos * wc;
  for (y = 0; y < 8; y++) {
    memcpy(bp, block, 8 * sizeof(Byte));
    block += 8 * sizeof(Byte);
    bp += wc;
  }
  /* V */
  bp = pict->v + xPos + yPos * wc;
  for (y = 0; y < 8; y++) {
    memcpy(bp, block, 8 * sizeof(Byte));
    block += 8 * sizeof(Byte);
    bp += wc;
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- ExtractBlockInt -- Extract an 8x8 integer block from a picture component
 *
 * Author:              K.S.
 *
 * Created:             20-Jan-98
 *
 * Purpose:             -
 *
 * Arguments in:        int      xPos          x position of block
 *                      int      yPos          y position of block
 *                      Byte     *in           component of the picture from
 *                                             which the block shall be
 *                                             extracted.
 *                      int      w             width of the picture component
 *
 * Arguments in/out:    -
 *
 * Arguments out:       int      block[64]     extracted block.
 *
 * Return values:       -
 *
 * Example:             ExtractBlockInt(xPos, yPos, *in, w, block);
 *
 * Side effects:        -
 *
 * Description:         It is not checked if the block lies inside the picture.
 *
 * See also:            -
 *
 * Modified:            -
 *
 *****************************************************************************/
void ExtractBlockInt(int xPos, int yPos, Byte *in, int w, int *block)
 /**********************************************************CommentEnd********/
{
  const int wm = w - 8;
  int x, y;


  in = in + xPos + yPos * w;
  for (y = 0; y < 8; y++) {
    for (x = 0; x < 8; x++) {
      *block = (int)(*in);
      block++;
      in++;
    }
    in += wm;
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- ExtractDiffBlockInt -- Extract an 8x8 int. block as difference between two picture components
 *
 * Author:              K.S.
 *
 * Created:             20-Jan-98
 *
 * Purpose:             -
 *
 * Arguments in:        int      xPos          x position of block
 *                      int      yPos          y position of block
 *                      Byte     *inP          component of the picture
 *                                             which is taken positive.
 *                      Byte     *inN          component of the picture
 *                                             which is taken negative.
 *                      int      w             width of the picture component
 *
 * Arguments in/out:    -
 *
 * Arguments out:       int      block[64]     extracted block.
 *
 * Return values:       -
 *
 * Example:             ExtractDiffBlockInt(xPos, yPos, *inP, *inN, w, block);
 *
 * Side effects:        -
 *
 * Description:         It is not checked if the block lies inside the picture.
 *                            *block = (int)(*inP) - (int)(*inN);
 *
 * See also:            -
 *
 * Modified:            -
 *
 *****************************************************************************/
void ExtractDiffBlockInt(int xPos, int yPos, Byte *inP, Byte *inN, int w,
			 int *block)
 /**********************************************************CommentEnd********/
{
  const int wm = w - 8;
  int x, y;


  inP = inP + xPos + yPos * w;
  inN = inN + xPos + yPos * w;
  for (y = 0; y < 8; y++) {
    for (x = 0; x < 8; x++) {
      *block = (int)(*inP) - (int)(*inN);
      block++;
      inP++;
      inN++;
    }
    inP += wm;
    inN += wm;
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- PlaceBlockShortPicture -- Place an 8x8 short block into a pict. component
 *
 * Author:              K.S.
 *
 * Created:             20-Jan-98
 *
 * Purpose:             -
 *
 * Arguments in:        int      xPos          x position of block
 *                      int      yPos          y position of block
 *                      short    block[64]     block to be placed.
 *                      int      w             width of the picture component
 *
 * Arguments in/out:    Byte     *picComp      component of the picture
 *
 * Arguments out:       -
 *
 * Return values:       -
 *
 * Example:             PlaceBlockShortPicture(xPos, yPos, *picComp, w, block);
 *
 * Side effects:        -
 *
 * Description:         It is not checked if the block lies inside the picture.
 *                            *picComp = (Byte)CROPBYTE(*block);
 *
 * See also:            -
 *
 * Modified:            -
 *
 *****************************************************************************/
void PlaceBlockShortPicture(int xPos, int yPos, Byte *picComp, int w,
			    short *block)
 /**********************************************************CommentEnd********/
{
  const int wm = w - 8;
  int x, y;


  picComp = picComp + xPos + yPos * w;
  for (y = 0; y < 8; y++) {
    for (x = 0; x < 8; x++) {
      *picComp = CROPBYTE(*block);
      picComp++;
      block++;
    }
    picComp += wm;
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- AddBlockShortPicture -- Add an 8x8 short block to a picture component
 *
 * Author:              K.S.
 *
 * Created:             20-Jan-98
 *
 * Purpose:             -
 *
 * Arguments in:        int      xPos          x position of block
 *                      int      yPos          y position of block
 *                      short    block[64]     block to be added.
 *                      int      w             width of the picture component
 *
 * Arguments in/out:    Byte     *picComp      component of the picture
 *
 * Arguments out:       -
 *
 * Return values:       -
 *
 * Example:             AddBlockShortPicture(xPos, yPos, *picComp, w, block);
 *
 * Side effects:        -
 *
 * Description:         It is not checked if the block lies inside the picture.
 *                       *picComp = (Byte)CROPBYTE((short)(*picComp) - *block);
 *
 * See also:            -
 *
 * Modified:            -
 *
 *****************************************************************************/
void AddBlockShortPicture(int xPos, int yPos, Byte *picComp, int w,
			  short *block)
 /**********************************************************CommentEnd********/
{
  const int wm = w - 8;
  int x, y;
  short tmp;


  picComp = picComp + xPos + yPos * w;
  for (y = 0; y < 8; y++) {
    for (x = 0; x < 8; x++) {
      tmp = *block + (short)(*picComp);
      *picComp = CROPBYTE(tmp);
      picComp++;
      block++;
    }
    picComp += wm;
  }
}
