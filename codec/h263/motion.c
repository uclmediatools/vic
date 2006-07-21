/***********************************************************HeaderBegin*******
 *                                                                         
 * File:	motion.c
 *
 * Author:	K.S.
 * Created:	6-Nov-97
 *                                                                         
 * Description: Operations for Motion.
 *
 * Notes: 	-
 *
 * Modified:	-
 *
 ***********************************************************HeaderEnd*********/

#include <string.h>
#include <limits.h>

#include "Motion.h"
#include "defs.h"
#include "structs.h"
#include "Util.h"

#include "bitOut.h"
#include "code.h"
#include "common.p"

#include "H263_mvd.h"

static const int h263_mvd_bit_cost[] = H263_MVD_BIT_COST;
static const int *mvd_bit_cost = h263_mvd_bit_cost + 32;


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- AllocMVector -- Allocate memory for a MVector structure
 *
 * Author:             T.W., X.Z.
 *
 * Created:            8-Oct-96
 *	
 *
 * Purpose:            Allocate all memory for a motion structure.
 * 
 * Arguments in:       -
 *
 * Arguments in/out:   -	
 *	
 * Arguments out:      -
 *
 * Return values:      A pointer to the allocated MVector structure.
 *
 * Example:            mv = AllocMVector();
 *
 * Side effects:       -
 *
 * Description:        see above
 *
 * See also:           FreeMVector
 *	
 * Modified:	        -
 *
 *****************************************************************************/
MVector *AllocMVector(void)
/***********************************************************CommentEnd********/
{
  return(NEW(MVector));
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- FreeMVector -- Free memory of MVector structure
 *
 * Author:            T.W., X.Z.
 *	
 * Created:           8-Oct-96
 *
 * Purpose:	      Frees up all the memory associated with a MVector struct
 * 
 * Arguments in:      -
 *
 * Arguments in/out:  MVector **mv_p   pointer to the MVector struct to be 
 *                                    freed. The pointer is set to NULL.
 *
 * Arguments out:     -
 *
 * Return values:     -
 *
 * Example:           FreeMVector(&mv);
 *
 * Side effects:      -
 *
 * Description:       -
 *
 * See also:          AllocMVector
 *
 * Modified:          -
 *
 *****************************************************************************/
void FreeMVector(MVector **mv_p)
/***********************************************************CommentEnd********/
{
  free(*mv_p);
  *mv_p = NULL;
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- ResetMVector -- Reset an MVector
 *
 * Author:             K.S.
 *
 * Created:            7-Aug-96
 *	
 * Purpose:            Reset an MVector
 * 
 * Arguments in:       -
 *
 * Arguments in/out:   MVector *mv          MVector to be reset.
 *
 * Arguments out:      -
 *
 * Return values:      -
 *
 * Example:            ResetMVector(mv);
 *
 * Side effects:       -
 *	
 * Description:        Fullpel and subpel components of the vector are set
 *                     to 0.
 *                     The subpel-fraction 'sF' is not changed!
 *
 * See also:           -
 *
 * Modified:           -
 *
 *****************************************************************************/
void ResetMVector(MVector *mv)
/***********************************************************CommentEnd********/
{
  memset(mv, 0, 4 * sizeof(short));
  mv->t = 0;
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- ClipMVector -- Clip an MVector
 *
 * Author:             K.S.
 *
 * Created:            1-Sep-97
 *	
 * Purpose:            Clips an MVector to the specified range.
 * 
 * Arguments in:       int     minX         minimal x value.
 *                     int     maxX         maximal x value.
 *                     int     minY         minimal y value.
 *                     int     maxY         maximal y value.
 *
 * Arguments in/out:   MVector *mv          MVector to be clipped.
 *                                          Fullpel and subpel component are
 *                                          adjusted.
 *
 * Arguments out:      -
 *
 * Return values:      -
 *
 * Example:            ClipMVector(-16, 15, -16, 15, mv);
 *
 * Side effects:       -
 *	
 * Description:        The fullpel components are clipped to be within the
 *                     specified range.
 *                     The subpel components are set to zero in case of
 *                     clipping.
 *
 * See also:           -
 *
 * Modified:           -
 *
 *****************************************************************************/
void ClipMVector(int minX, int maxX, int minY, int maxY, MVector *mv)
/***********************************************************CommentEnd********/
{
  /* X Component */
  if (mv->x < minX) {
    mv->x = 0;
    mv->sx = 0;
  }
  if (mv->x == maxX)
    mv->sx = 0;
  if (mv->x > maxX) {
    mv->x = maxX;
    mv->sx = 0;
  }
  /* Y Component */
  if (mv->y < minY) {
    mv->y = 0;
    mv->sy = 0;
  }
  if (mv->y == maxY)
    mv->sy = 0;
  if (mv->y > maxY) {
    mv->y = maxY;
    mv->sy = 0;
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- CopyMVector -- Copy an MVector
 *
 * Author:             K.S.
 *
 * Created:            7-Aug-96
 *
 * Purpose:            Copy an MVector to another MVector struct
 * 
 * Arguments in:       MVector *mvIn        Input MVector that is to be copied.
 *
 * Arguments in/out:   MVector *mvOut       MVector struct that is filled
 *                                          with the values of mvIn.
 *
 * Arguments out:      -
 *
 * Return values:      0  if error
 *                     1  if ok.
 *
 * Example:            CopyMVector(mvIn, mvOut);
 *
 * Side effects:       -
 *
 * Description:        The MVector mvIn is copied to the MVector mvOut.
 *
 * See also:           -
 *
 * Modified:           -
 *
 *****************************************************************************/
int CopyMVector(MVector *mvIn, MVector *mvOut)
/***********************************************************CommentEnd********/
{
  memcpy(mvOut, mvIn, sizeof(MVector));

  return(1);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- ZeroMVector2D -- Verify if a motion vector is zero in 2 dimensions (x,y)
 *
 * Author:             K.S.
 *
 * Created:            23-May-97
 *
 * Purpose:            Verifies if a motion vector ('MVector') is zero in
 *                     2 dimensions, i.e. if x, y, sx and sy are zero.
 *                     Note: the time displacement is not checked.
 * 
 * Arguments in:       MVector *mv          MVector to be verified.
 *
 * Arguments in/out:   -
 *
 * Arguments out:      -
 *
 * Return values:      0  if any component of 'mv' is not 0.
 *                     1  if all components of 'mv' are 0.
 *
 * Example:            zero = ZeroMVector2D(mv);
 *
 * Side effects:       -
 *
 * Description:        The MVectors m is checked.
 *
 * See also:           ZeroMVector
 *
 * Modified:           -
 *
 *****************************************************************************/
int ZeroMVector2D(MVector *mv)
/***********************************************************CommentEnd********/
{
  return(mv->x == 0 && mv->sx == 0 && mv->y == 0 && mv->sy == 0);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263MVPrediction -- Motion vector prediction for H.263
 *
 * Author:           K.S.
 *
 * Created:          30-Nov-98
 *
 * Purpose:          Motion vector prediction as described in H.263
 * 
 * Arguments in:     Int          x             MB index horizontal
 *                   Int          y             MB index vertical
 *                   Int          newgob        1 if the mb row above belongs
 *                                                   to another GOB.
 *                                              0 if the mb row above belongs
 *                                                   to the same GOB.
 *                                              To be correct: 'belongs to
 *                                              another GOB' means this rows
 *                                              GOB header is not empty.
 *
 * Arguments in/out: MVField      *mvf          field of macroblocks
 *
 * Arguments out:    MVector      *pv           predicted motion vector
 *                                              if 'NULL', the result is
 *                                              written to 'mvf'
 *
 * Return values:    0 if error
 *                   1 if ok
 *
 * Example:          H263MVPrediction(mvf, x, y, newgob, pv);
 *
 * Side effects:     -
 *
 * Description:      -
 *
 * See also:         -
 *
 * Modified:         Adapted from CIDS
 *      
 *****************************************************************************/
int H263MVPrediction(MVField *mvf, int x, int y, int newgob, MVector *pv)
/***********************************************************CommentEnd********/
{
  int i;
  int px, py;
  int px1,px2,px3,py1,py2,py3;


  if (y == 0)
    newgob = 1;

  /* Set index */
  i = y * mvf->w + x - 1;

  /* Left predictor */
  if (x > 0) {
    px1 = mvf->mx[i];
    py1 = mvf->my[i];
  } else {
    px1 = 0;
    py1 = 0;
  }

  /* Upper predictors */
  if (newgob) {
    px2 = px3 = px1;
    py2 = py3 = py1;
  } else {
    i -= mvf->w;
    i++;
    px2 = mvf->mx[i];
    py2 = mvf->my[i];
    i++;
    /* Upper right predictor */
    if (i % mvf->w != 0) {
      px3 = mvf->mx[i];
      py3 = mvf->my[i];
    } else {
      px3 = 0;
      py3 = 0;
    }
  }

  px = px1+px2+px3 - MAX(px1,MAX(px2,px3)) - MIN(px1,MIN(px2,px3));
  py = py1+py2+py3 - MAX(py1,MAX(py2,py3)) - MIN(py1,MIN(py2,py3));

  if (pv != NULL) {
    pv->x = (px < 0) ? (px - 1) / 2 : px / 2;
    pv->sx = ABS(px % 2);
    pv->y = (py < 0) ? (py - 1) / 2 : py / 2;
    pv->sy = ABS(py % 2);
  } else {
    i = y * mvf->w + x;
    mvf->mx[i] = px;
    mvf->my[i] = py;
  }

  return 1;
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263subtractMVPrediction -- Compute the H.263 difference vector
 *
 * Author:             K.S.
 *
 * Created:            17-Nov-97
 *
 * Purpose:            Computes the difference vector between a motion vector
 *                     and its predictor for H.263.
 * 
 * Arguments in:       MVector *mv          motion vector.
 *                     MVector *pv          predictor.
 *                     int     umv          unrestricted motion vector mode
 *                                          (UMV mode) flag.
 *
 * Arguments in/out:   -
 *
 * Arguments out:      MVector *dv          difference vector.
 *
 * Return values:      0  if error
 *                     1  if ok
 *
 * Example:            H263subtractMVPrediction(mv, pv, umv, dv);
 *
 * Side effects:       -
 *
 * Description:        -
 *
 * See also:           -
 *
 * Modified:           -
 *
 *****************************************************************************/
int H263subtractMVPrediction(MVector *mv, MVector *pv, int umv, MVector *dv)
/***********************************************************CommentEnd********/
{
  int px,dx,mx,py,dy,my;

/*   assert( mv->sF==2 && pv->sF==2 && dv->sF==2 ); */

  /* scale to intger values */
  mx = (mv->x * 2) + mv->sx;
  px = (pv->x * 2) + pv->sx;
  my = (mv->y * 2) + mv->sy;
  py = (pv->y * 2) + pv->sy;
  
  /* make subtraction */
  if(!umv) {
    dx = mx - px;
    if(dx<-32) dx+=64; else if(dx>31) dx-=64;
    dy = my - py;
    if(dy<-32) dy+=64; else if(dy>31) dy-=64;
  } else {
    dx = mx - px;
    if( px<-31 || px>32) { if(dx<-32) dx+=64; else if(dx>31) dx-=64; } 
    dy = my - py;
    if( py<-31 || py>32) { if(dy<-32) dy+=64; else if(dy>31) dy-=64; }
  }

  /* rescale into subpel values */
  dv->x = dx/2; 
  dv->sx = dx%2;
  if(dv->sx == -1) {dv->sx += 2; dv->x -= 1;}

  dv->y = dy/2; 
  dv->sy = dy%2;
  if(dv->sy == -1) {dv->sy += 2; dv->y -= 1;}

  return 1;
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- GetMVDRate -- Compute the rate for the H.263 difference vector
 *
 * Author:             K.S.
 *
 * Created:            19-Jan-98
 *
 * Purpose:            Computes rate that is needed for the difference vector
 *                     resulting from the motion vector
 *                     and its predictor for H.263.
 * 
 * Arguments in:       MVector *pv          predictor.
 *                     MVector *mv          motion vector.
 *
 * Arguments in/out:   -
 *
 * Arguments out:      -
 *
 * Return values:      int     rate         number of bits needed.
 *
 * Example:            GetMVDRate(pv, mv);
 *
 * Side effects:       -
 *
 * Description:        If one of the vectors is 'NULL', it is assumed to
 *                     be the zero vector. It is not allowed to set both
 *                     to 'NULL'.
 *
 * See also:           -
 *
 * Modified:           -
 *
 *****************************************************************************/
int GetMVDRate(MVector *pv, MVector *mv)
/***********************************************************CommentEnd********/
{
  if (pv == NULL)
    return(mvd_bit_cost[2*mv->x + mv->sx] + mvd_bit_cost[2*mv->y + mv->sy]);
  else if (mv == NULL)
    return(mvd_bit_cost[-2*pv->x - pv->sx] + mvd_bit_cost[-2*pv->y - pv->sy]);
  else {
    int dx = 2 * (mv->x - pv->x) + mv->sx - pv->sx;
    int dy = 2 * (mv->y - pv->y) + mv->sy - pv->sy;

    if(dx<-32) dx+=64; else if(dx>31) dx-=64;
    if(dy<-32) dy+=64; else if(dy>31) dy-=64;

    return(mvd_bit_cost[dx] + mvd_bit_cost[dy]);
  }
}


/***********************************************************CommentBegin*******
 ******************************************************************************
 *
 * -- FullSearchFullPelME -- Full search on integer-pel positions
 *
 * Author:           K.S.
 *
 * Created:          8-Oct-97
 *
 * Purpose:          Search on integer-pel positions
 * 
 * Arguments in:     int xPos, yPos  : Position of blk in prevImg corrsponding 
 *                                     to MV = (0,0);
 *                   int *minCost_p  : Cost of previously found motion vector, 
 *                                     otherwise remains unchanged,
 *                   int lambda      : lambda for rate-distortion search
 *                                      SAD + lambda * rate.
 *                   int searchRange : search range for difference vectors to
 *                                     mvOut, (max. 15);
 *                   int rMin        : range minimum.
 *                   int rMax        : range maximum.
 *                   Picture *pict   : Image for which the search is
 *                                     conducted. not modified.
 *                   Picture *prevPict:Image wherein the match is to be found; 
 *                                     not modified.
 *                   MVector *pv     : motion vector predictor.
 *                   MVector *mv     : motion vector which's integer-pel part
 *                                     is indicating the center of the search
 *                                     and containing the motion vector
 *                                     corresponding to the best match if one
 *                                     was found, otherwise remains unchanged.
 *
 * Arguments in/out: 
 *
 * Arguments out:    -
 *
 * Return values:    1: a better match was found,
 *                   0: failure
 *
 * Example:          FullSearchFullPelME(xPos+offset, yPos+offset, &sad,
 *                                       lambda, searchRange,
 *                                       -15, 15, origBlk, prevPict,
 *                                       mb->pv[0], mb->mv[0]);
 *
 * Side effects:     Uses the global array 'spiral'
 *
 * Description:      Searches motion vector that minimizes D at 
 *                   integer-pel positions. Routine returns one if a better 
 *                   match than the one corresponding to minCost is found.
 *
 * See also:         h263integerPelBME
 *
 * Modified:         -
 *
 *****************************************************************************/
int FullSearchFullPelME(int xPos, int yPos, int *minCost_p, int lambda,
			int searchRange, int rMin, int rMax,
			Picture *pict, Picture *predPict,
			MVector *pv, MVector *mvOut) 
/**********************************************************CommentEnd*********/
{
  int foundBetterMatch = FALSE;
  int l;
  int i0, j0;
  int rate = 0;
  int cost;
  /* Length of spiral */
  int ll = (2*searchRange+1)*(2*searchRange+1);

  static MVector *mv;
  static MVector *mvMin;
  static int virgin = TRUE;


  if(virgin) {
    virgin = FALSE;
    mv = AllocMVector();
    mv->sF = 2; mv->sx = 0; mv->sy = 0;
    mvMin = AllocMVector();
    mvMin->sF = 2;
  }

  /* Find best MV for current frame given cost of past MV */
  for(l=0;l<ll;++l) {
    /* Determine candidate motion vector mv */
    mv->x = spiral[l][0] + mvOut->x;
    mv->y = spiral[l][1] + mvOut->y;

    if (mv->x >= rMin && mv->x <= rMax && mv->y >= rMin && mv->y <= rMax) {
      /* Motion vector is within allowed range */
      i0 = xPos + mv->x;
      j0 = yPos + mv->y;
      /* Check whether candidate is inside the prevPict */
      if(i0 >= 0 && i0 + 16 <= predPict->w &&
	 j0 >= 0 && j0 + 16 <= predPict->h) {
	/* Compute cost for candidate mv using startCost, 
	   break if minCost is exceeded */
	if (lambda != 0)
	  rate = lambda * GetMVDRate(pv, mv);
	cost = FPMotionMacroBlockPictureSAD(xPos, yPos, pict, predPict, mv,
					    *minCost_p - rate) + rate;
	if(cost < *minCost_p) {
	  /* Yippie we found one ! */
	  foundBetterMatch = TRUE;
	  *minCost_p = cost;
	  CopyMVector(mv, mvMin);
	}
      }
    }
  }

  /* Only in case we found a better match than the one corresponding 
     to minCost mvOut is modified. */
  if (foundBetterMatch)
    CopyMVector(mvMin, mvOut);

  return(foundBetterMatch);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- FastFullPelMotionEstimationMB -- Estimate quickly full pel mot. for a MB
 *
 * Author:             K.S.
 *
 * Created:            29-Dec-97
 *
 * Purpose:            Estimates the full pel motion vector for a macroblock
 *                     by using a fast search algorithm.
 * 
 * Arguments in:       int     xPos         x position of MB
 *                     int     yPos         y position of MB
 *                     Picture *pict        original picture to be coded.
 *                     Picture *refPict     reference picture.
 *                     int     cost0        cost for the 0 vector.
 *                                          If 'cost0 < 0', it is computed
 *                                          in this routine.
 *                     int lambda           lambda for rate-distortion search
 *                                          SAD + lambda * rate.
 *                     MVector *pv          predicted motion vector.
 *
 * Arguments in/out:   MVector *mv          best motion vector.
 *                                          If no half pel motion vector with
 *                                          a SAD smaller than 'sadFP' is
 *                                          found, 'mv' is set to '0'.
 *
 * Arguments out:      -
 *
 * Return values:      int     cost         cost of best motion vector.
 *
 * Example:            cost = FastFullPelMotionEstimationMB(i*16, j*16, pict,
 *                                                         prevPict, cost0,
 *                                                         lambda, pv, mv);
 *
 * Side effects:       -
 *
 * Description:        The search is done starting at 'pv' following the
 *                     direction of the best match.
 *                     See DA Baese: Bi Area Subsampled Estimation (BASE)
 *                     Algorithm.
 *
 * See also:           HalfPelMotionEstimationMB
 *
 * Modified:           -
 *
 *****************************************************************************/
int FastFullPelMotionEstimationMB(int xPos, int yPos,
				  Picture *pict, Picture *refPict,
				  int cost0, int lambda,
				  MVector *pv, MVector *mv)
/***********************************************************CommentEnd********/
{
  int rate = 0;
  int cost = INT_MAX, costMin = cost0;
  int dxMin, dxMax, dyMin, dyMax;
  int dir, prevDir = -1;  /* -1: invalid, 0: up, 3: right, 6: down, 9: left */


  dxMin = MAX(-xPos, -16);
  dxMax = MIN(pict->w - 16 - xPos, 15);
  dyMin = MAX(-yPos, -16);
  dyMax = MIN(pict->h - 16 - yPos, 15);

  /* Compute SAD0 if necessary */
  if (cost0 < 0) {
    if (lambda != 0)
      rate = lambda * GetMVDRate(pv, NULL);
    cost0 = MacroBlockPictureSAD(xPos, yPos, pict, refPict, INT_MAX) + rate;
  }

  /* Compute SAD for predictor unless it is equal 0 and only if it is inside
     the picture */
  if (pv->x != 0 || pv->y != 0)
    if (pv->x >= dxMin && pv->x <= dxMax && pv->y >= dyMin && pv->y <= dyMax) {
      mv->x = pv->x;
      mv->y = pv->y;
      mv->sx = 0;
      mv->sy = 0;
      if (lambda != 0)
	rate = lambda * GetMVDRate(pv, mv);
      costMin = FPMotionMacroBlockPictureSAD(xPos, yPos, pict, refPict, pv,
					     cost0 - rate) + rate;
    }

  if (cost0 <= costMin) {
    ResetMVector(mv);
    costMin = cost0;
  }

  /* Do search */
  do {
    dir = -1;
    /* right */
    if (prevDir != 9) {         /* Did we come from that direction? */
      mv->x++;
      if (mv->x <= dxMax) {
	if (lambda != 0)
	  rate = lambda * GetMVDRate(pv, mv);
	cost = FPMotionMacroBlockPictureSAD(xPos, yPos, pict, refPict, mv,
					    costMin - rate) + rate;
	if (cost < costMin) {
	  costMin = cost;
	  dir = 3;
	}
      }
      mv->x--;
    }
    /* left */
    if (prevDir != 3) {         /* Did we come from that direction? */
      mv->x--;
      if (mv->x >= dxMin) {
	if (lambda != 0)
	  rate = lambda * GetMVDRate(pv, mv);
	cost = FPMotionMacroBlockPictureSAD(xPos, yPos, pict, refPict, mv,
					   costMin - rate) + rate;
	if (cost < costMin) {
	  costMin = cost;
	  dir = 9;
	}
      }
      mv->x++;
    }
    /* down */
    if (prevDir != 0) {         /* Did we come from that direction? */
      mv->y++;
      if (mv->y <= dyMax) {
	if (lambda != 0)
	  rate = lambda * GetMVDRate(pv, mv);
	cost = FPMotionMacroBlockPictureSAD(xPos, yPos, pict, refPict, mv,
					   costMin - rate) + rate;
	if (cost < costMin) {
	  costMin = cost;
	  dir = 6;
	}
      }
      mv->y--;
    }
    /* up */
    if (prevDir != 6) {         /* Did we come from that direction? */
      mv->y--;
      if (mv->y >= dyMin) {
	if (lambda != 0)
	  rate = lambda * GetMVDRate(pv, mv);
	cost = FPMotionMacroBlockPictureSAD(xPos, yPos, pict, refPict, mv,
					   costMin - rate) + rate;
	if (cost < costMin) {
	  costMin = cost;
	  dir = 0;
	}
      }
      mv->y++;
    }
    switch (dir) {
    case 0:
      mv->y--;
      break;
    case 3:
      mv->x++;
      break;
    case 6:
      mv->y++;
      break;
    case 9:
      mv->x--;
      break;
    }
    prevDir = dir;
  } while (dir >= 0);

  return(costMin);
}



/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- InitHalfPelInterpolation -- Init half pel interpolation images
 *
 * Author:           K.S.
 *
 * Created:          17-Nov-97
 *
 * Purpose:          Initializes the half pel interpolation images.
 * 
 * Arguments in:     Picture   *pict         picture to be interpolated.
 *
 * Arguments in/out: -
 *
 * Arguments out:    Byte      *interpol[3]  the three halfpel pictures.
 *                                           interpol[0]:  horizontal
 *                                           interpol[1]:  vertical
 *                                           interpol[2]:  horizontal and vert.
 *
 * Return values:    -
 *
 * Example:          InitHalfPelInterpolation(pict, interpol);
 *
 * Side effects:     -
 *
 * Description:      'interpol' contains the interpolated values for
 *                   right or down half pel interpolation, e.g. the
 *                   value for point (.5,0) can be found at 'interpol[0][0]'.
 *                   The value for point (14.5,.5) can be found at
 *                   'interpol[2][14]'.
 *                   Attention: The most right column and the lowest row
 *                              contain invalid values.
 *
 * See also:         -
 *	
 * Modified:         -
 *	
 *****************************************************************************/
void InitHalfPelInterpolation(Picture *pict, Byte *interpol[3])
/***********************************************************CommentEnd********/
{
#if 0
{
  int  w = pict->w;
  int  size = w * pict->h;
  int  i, tmp;
  Byte *p0, *p1, *ip;


  /* horizontal */
  p0 = pict->y;
  ip = interpol[0];
  for (i = 0; i < size - 1; i++) {
    tmp = (int)(*p0++) + 1;
    tmp += (int)(*p0);
    tmp /= 2;
    *ip++ = (Byte)tmp;
  }

  /* vertical */
  p0 = pict->y;
  p1 = pict->y + w;
  ip = interpol[1];
  for (i = 0; i < size - w; i++) {
    tmp = (int)(*p0++) + 1;
    tmp += (int)(*p1++);
    tmp /= 2;
    *ip++ = (Byte)tmp;
  }

  /* horizontal & vertical */
  p0 = pict->y;
  p1 = pict->y + w;
  ip = interpol[2];
  for (i = 0; i < size - w - 1; i++) {
    tmp = (int)(*p0++) + 2;
    tmp += (int)(*p0);
    tmp += (int)(*p1++);
    tmp += (int)(*p1);
    tmp /= 4;
    *ip++ = (Byte)tmp;
  }
}
#endif
  int  w = pict->w;
  int  size1 = w * pict->h - 1;
  int  size0 = size1 - w;
  int  i, tmp_h, tmp_v, tmp_hv;
  Byte *p0, *p1, *ip_h, *ip_v, *ip_hv;


  p0 = pict->y;
  p1 = pict->y + w;
  ip_h = interpol[0];
  ip_v = interpol[1];
  ip_hv = interpol[2];
  tmp_v = (int)(*p0) + (int)(*p1++) + 1;
  /* All three directions */
  for (i = 0; i < size0; i++) {
    tmp_hv = tmp_v;
    tmp_v = tmp_v >> 1;
    *ip_v++ = (Byte)tmp_v;

    tmp_h = (int)(*p0++) + 1;
    tmp_h += (int)(*p0);
    tmp_h = tmp_h >> 1;
    *ip_h++ = (Byte)tmp_h;

    tmp_v = (int)(*p0) + (int)(*p1++) + 1;

    tmp_hv += tmp_v;
    tmp_hv = tmp_hv >> 2;
    *ip_hv++ = (Byte)tmp_hv;
  }

  /* vertical (last point); i = size0; */
  tmp_v = tmp_v >> 1;
  *ip_v = (Byte)tmp_v;

  ip_h++;
  p0++;
  /* horizontal (last line) */
  for (i = size0 + 1; i < size1; i++) {
    tmp_h = (int)(*p0++) + 1;
    tmp_h += (int)(*p0);
    tmp_h = tmp_h >> 1;
    *ip_h++ = (Byte)tmp_h;
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- HalfPelMotionEstimationMB -- Estimate half pel motion for a macroblock
 *
 * Author:             K.S.
 *
 * Created:            17-Nov-97
 *
 * Purpose:            Estimates the half pel motion vector for a macroblock
 *                     by using the interpolated half pel images.
 * 
 * Arguments in:       int     xPos         x position of MB
 *                     int     yPos         y position of MB
 *                     Picture *pict        original picture to be coded.
 *                     Byte    *interpol[3] reference picture with half pel
 *                                          interpolations.
 *                     int     left_f       flag if the halfpel positions to
 *                                          the left shall be tested;
 *                                          unless it is outside the ref. pict.
 *                     int     right_f      flag if the halfpel positions to
 *                                          the right shall be tested.
 *                                          unless it is outside the ref. pict.
 *                     int     top_f         flag if the halfpel positions to
 *                                          the top shall be tested.
 *                                          unless it is outside the ref. pict.
 *                     int     bottom_f     flag if the halfpel positions to
 *                                          the bottom shall be tested.
 *                                          unless it is outside the ref. pict.
 *                     int     costFP       RD-cost of full pel position.
 *                     int     lambda       lambda for rate-distortion search
 *                                           SAD + lambda * rate.
 *                     MVector *pv          motion vector predictor.
 *
 * Arguments in/out:   MVector *mv          best motion vector.
 *                                          If no half pel motion vector with
 *                                          a cost smaller than 'costFP' is
 *                                          found, 'mv' is left unchanged.
 *
 * Arguments out:      -
 *
 * Return values:      int     cost          cost of best motion vector.
 *
 * Example:            cost = HalfPelMotionEstimationMB(i*16, j*16,
 *                                                     pict, interpol,
 *                                                     1, 1, 1, 1,
 *                                                     costFP, lambda, pv, mv);
 *
 * Side effects:       -
 *
 * Description:        The search is done around the full pel position of 'mv'.
 *
 * See also:           InitHalfPelInterpolation
 *
 * Modified:           -
 *
 *****************************************************************************/
int HalfPelMotionEstimationMB(int xPos, int yPos,
			      Picture *pict, Byte *interpol[3],
			      int left_f, int right_f, int top_f, int bottom_f,
			      int costMin, int lambda,MVector *pv, MVector *mv)
/***********************************************************CommentEnd********/
{
  static int     positions[8][4] = {{0, -1, 0, 1},  /* up */
				    {0, -1, 1, 1},  /* right, up */
				    {0, 0, 1, 0},   /* right */
				    {0, 0, 1, 1},  /* right, down */
				    {0, 0, 0, 1},   /* down */
				    {-1, 0, 1, 1}, /* left, down */
				    {-1, 0, 1, 0},  /* left */
				    {-1, -1, 1, 1}}; /* left, up */
  static MVector mvTmp;
  int inside[8] = {1, 1, 1, 1, 1, 1, 1, 1}; /* start at top; clockwise */
  int x = xPos + mv->x;
  int y = yPos + mv->y;
  int n, nMin = -1;
  int rate = 0;
  int cost;


  /* Check if one of the half pel positions lies outside the ref. picture
     and this side shall be checked */
  if (!left_f || x <= 0) {           /* left */
    inside[5] = 0;
    inside[6] = 0;
    inside[7] = 0;
  } else if (!right_f || x >= pict->w - MACROBLOCK_SIZE - 1) { /* right */
    inside[1] = 0;
    inside[2] = 0;
    inside[3] = 0;
  }
  if (!top_f || y <= 0) {           /* top */
    inside[0] = 0;
    inside[1] = 0;
    inside[7] = 0;
  } else if (!bottom_f || y >= pict->h - MACROBLOCK_SIZE - 1) { /* bottom */
    inside[3] = 0;
    inside[4] = 0;
    inside[5] = 0;
  }

  for (n = 0; n < 8; n++)
    if (inside[n]) {
      mvTmp.x = mv->x + positions[n][0];
      mvTmp.sx = positions[n][2];
      mvTmp.y = mv->y + positions[n][1];
      mvTmp.sy = positions[n][3];
      if (lambda != 0)
	rate = lambda * GetMVDRate(pv, &mvTmp);
      cost = HPMotionMacroBlockPictureSAD(xPos, yPos, pict, interpol, &mvTmp,
					 costMin - rate) + rate;
      if (cost < costMin) {
	costMin = cost;
	nMin = n;
      }
    }

  if (nMin >= 0) {
    mv->x += positions[nMin][0];
    mv->sx = positions[nMin][2];
    mv->y += positions[nMin][1];
    mv->sy = positions[nMin][3];
  } else {
    mv->sx = 0;
    mv->sy = 0;
  }

  return(costMin);
}


/*****************************************************************************/
/*****************************************************************************/
/* Display motion vectors */


static void PlotLine(int x1, int y1, int x2, int y2, int value, Picture *pic)
{
  int w = pic->w;
  int h = pic->h;
  int x = x1, y = y1;
  int dx = x2 - x1, dy = y2 - y1;
  int sdx = SGN(dx), sdy = SGN(dy);
  int adx = ABS(dx), ady = ABS(dy);
  int e = 0;
  int res;


  res = (ady < adx);
  switch (res) {
  case 0:
    for (y = y1; (sdy >= 0) ? y <= y2 : y >= y2; y += sdy) {
      e += 2 * adx;
      if (e >= ady) {
	x += sdx;
	e -= 2 * ady;
      }
      /* Plot if the point is inside the image */
      if (x >= 0 && x < w && y >= 0 && y < h)
	pic->y[x + y * w] = (Byte)value;
    }
    break;
  case 1:
    for (x = x1; (sdx >= 0) ? x <= x2 : x >= x2; x += sdx) {
      e += 2 * ady;
      if (e >= adx) {
	y += sdy;
	e -= 2 * adx;
      }
      /* Plot if the point is inside the image */
      if (x >= 0 && x < w && y >= 0 && y < h)
	pic->y[x + y * w] = (Byte)value;
    }
    break;
  }
}


#if 0
static void PlotMVector(int x, int y, int value, float scale, MVector *mv,
		       Picture *pic)
{
  PlotLine(x, y,
	   x + ROUND(scale*((float)(mv->x) + (float)(mv->sx)/(float)(mv->sF))),
	   y + ROUND(scale*((float)(mv->y) + (float)(mv->sy)/(float)(mv->sF))),
	   value, pic);
}
#endif


/*****************************************************************************/
void PlotMVField(MVField *mvf, int value, float scale, Picture *pic)
{
  int x, y;
  int w = pic->w;
  int n;


  ASSERT(16 * mvf->w == w && 16 * mvf->h == pic->h);

  for (n = 0, y = 8; y < pic->h; y += 16)
    for (x = 8; x < w; x += 16, n++)
      if (mvf->mode[n] == MODE_INTER || mvf->mode[n] == MODE_INTER_Q)
	PlotLine(x, y, x + (scale * (float)(mvf->mx[n])) / 2,
		 y + (scale * (float)(mvf->my[n])) / 2, value, pic);
}


/*****************************************************************************/
void PrintMVField(MVField *mvf, FILE *fp)
{
  int x, y, mn;


  for (y = 0; y < mvf->h; y++) {
    for (x = 0; x < mvf->w; x++) {
      mn = x + y * mvf->w;
      if (mvf->mode[mn] == MODE_INTER || mvf->mode[mn] == MODE_INTER_Q)
	fprintf(fp, "%3d,%3d", mvf->mx[mn], mvf->my[mn]);
      else
	fprintf(fp, " - ; - ");
      fprintf(fp, "|");
    }
    fprintf(fp, "\n");
  }
}
