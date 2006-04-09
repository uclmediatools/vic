/************************************************************************
 *
 *  mot_est.c, part of tmn (TMN encoder)
 *
 *  Copyright (C) 1997  University of BC, Canada
 *
 *  Contacts: 
 *  Michael Gallant                   <mikeg@ee.ubc.ca>
 *  Guy Cote                          <guyc@ee.ubc.ca>
 *  Berna Erol                        <bernae@ee.ubc.ca>
 *
 *  UBC Image Processing Laboratory   http://www.ee.ubc.ca/image
 *  2356 Main Mall                    tel.: +1 604 822 4051
 *  Vancouver BC Canada V6T1Z4        fax.: +1 604 822 5949
 *
 *  Copyright (C) 1995, 1996  Telenor R&D, Norway
 *
 *  Contacts:
 *  Robert Danielsen                  <Robert.Danielsen@nta.no>
 *
 *  Telenor Research and Development  http://www.nta.no/brukere/DVC/
 *  P.O.Box 83                        tel.:   +47 63 84 84 00
 *  N-2007 Kjeller, Norway            fax.:   +47 63 81 00 76
 *
 ************************************************************************/

/* Disclaimer of Warranty
 * 
 * These software programs are available to the user without any license fee
 * or royalty on an "as is" basis. The University of British Columbia
 * disclaims any and all warranties, whether express, implied, or
 * statuary, including any implied warranties or merchantability or of
 * fitness for a particular purpose.  In no event shall the
 * copyright-holder be liable for any incidental, punitive, or
 * consequential damages of any kind whatsoever arising from the use of
 * these programs.
 * 
 * This disclaimer of warranty extends to the user of these programs and
 * user's customers, employees, agents, transferees, successors, and
 * assigns.
 * 
 * The University of British Columbia does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any
 * third-party patents.
 * 
 * Commercial implementations of H.263, including shareware, are subject to
 * royalty fees to patent holders.  Many of these patents are general
 * enough such that they are unavoidable regardless of implementation
 * design.
 * 
 */


#include "sim.h"

/**********************************************************************
 *
 *	Name:        MotionEstimation
 *	Description:	Estimate all motion vectors for one MB
 *
 *	Input:        pointers to current and reference (previous)
 *                    image, pointers to current slice and
 *                    current MB
 *	Returns:
 *	Side effects:	motion vector information in MB changed
 *
 *	Date: 930118	Author: Karl.Lillevold@nta.no
 *            940203    Mod. to use PGB's faster search
 *            941208    Mod to use spiral search from mpeg2encode
 *            951001    Mod for extended motion vector range
 *            970526	Fast motion estimation by mikeg@ee.ubc.ca,
 *                      modified by guyc@ee.ubc.ca.
 *            970831    Added support for true-B pictures,
 *                      modified by mikeg@ee.ubc.ca
 *            971025    Added new motion vector range for RVLC
 *                      modified by guyc@ee.ubc.ca.
 *
 ***********************************************************************/


void MotionEstimation (unsigned char *curr, unsigned char *reference, int x_curr,
                        int y_curr, int xoff, int yoff, int seek_dist,
                        MotionVector * MV[7][MBR + 1][MBC + 2], int
                       *SAD_0, int estimation_type)

{

  int Min_FRAME[7];
  MotionVector MV_FRAME[7];
  unsigned char *act_block, *aa, *ii;
  unsigned char *search_area, *adv_search_area = NULL, *zero_area = NULL;
  int sxy, i, k, j, l, m;
  int ihigh, ilow, jhigh, jlow, h_length, v_length;
  int adv_ihigh, adv_ilow, adv_jhigh, adv_jlow, adv_h_length, adv_v_length;
  int xmax, ymax, block, sad, lx;
  int adv_x_curr, adv_y_curr, xvec, yvec;
#ifndef FULLSEARCH
  static int htp[4] = {-1, 0, 1, 0}; //SV-XXX: added default type int
  static int vtp[4] = {0, 1, 0, -1}; //SV-XXX: added default type int
  int j_min_now, i_min_now, i_min_next=0, j_min_next=0, sad_layr=65536; //SV-XXX: inited to 0 and 65536
  int distortion_0, distortion_1, distortion_2;
#endif

  xmax = sed_pels;
  ymax = sed_lines;
  sxy = seek_dist;

  if (!long_vectors || (estimation_type == PB_PICTURE_ESTIMATION))
  {
    /* Maximum normal search range centered around _zero-vector_ */
    sxy = mmin (15, sxy);
  } else
  {
    if(EPTYPE)
    {
      /* if RVLC codes of H.263+ are used, new motion vector range  *
       * is used.                                                   */
      sxy = (sed_pels < 353) ? 31 : (sed_pels < 705) ? 63 : (sed_pels < 1409) ? 127 : 255;
    }
    else
    {
     

      /* Maximum extended search range centered around _predictor_ */
      sxy = mmin (15 - (2 * DEF_8X8_WIN), sxy);

      /* NB! */

      /* It is only possible to transmit motion vectors within a 15x15
       * window around the motion vector predictor for any 8x8 or 16x16
       * block */

      /* The reason for the search window's reduction above with
       * 2*DEF_8X8_WIN+1 is that the 8x8 search may change the MV predictor
       * for some of the blocks within the macroblock. When we impose the
       * limitation above, we are sure that any 8x8 vector we might find is
       * possible to transmit */

      /* We have found that with OBMC, DEF_8X8_WIN should be quite small for 
       * two reasons: (i) a good filtering effect, and (ii) not too many
       * bits used for transferring the vectors. As can be seen above this
       * is also useful to avoid a large limitation on the MV search range */

      /* It is possible to make sure the motion vectors found are legal in
       * other less limiting ways than above, but this would be more
       * complicated as well as time-consuming. Any good suggestions for 
       * improvement is welcome, though */
    } 
    
    xoff = mmin (16, mmax (-16, xoff));
    yoff = mmin (16, mmax (-16, yoff));

    /* There is no need to check if (xoff + x_curr) points outside the
     * picture, since the Extended Motion Vector Range is always used
     * together with the Unrestricted MV mode */
  }

  lx = (mv_outside_frame ? sed_pels + (long_vectors ? 64 : 32) : sed_pels);

  ilow = x_curr + xoff - sxy;
  ihigh = x_curr + xoff + sxy;

  jlow = y_curr + yoff - sxy;
  jhigh = y_curr + yoff + sxy;

  if (!mv_outside_frame)
  {
    if (ilow < 0)
      ilow = 0;
    if (ihigh > xmax - 16)
      ihigh = xmax - 16;
    if (jlow < 0)
      jlow = 0;
    if (jhigh > ymax - 16)
      jhigh = ymax - 16;
  }
  h_length = ihigh - ilow + 16;
  v_length = jhigh - jlow + 16;
  act_block = LoadArea (curr, x_curr, y_curr, 16, 16, sed_pels);
  search_area = LoadArea (reference, ilow, jlow, h_length, v_length, lx);

  for (k = 0; k < 7; k++)
  {
    Min_FRAME[k] = INT_MAX;
    MV_FRAME[k].x = 0;
    MV_FRAME[k].y = 0;
    MV_FRAME[k].x_half = 0;
    MV_FRAME[k].y_half = 0;
  }

  /* Zero vector search */
  if (x_curr - ilow < 0 || y_curr - jlow < 0 ||
      x_curr - ilow + MB_SIZE > h_length || y_curr - jlow + MB_SIZE > v_length)
  {
    /* in case the zero vector is outside the loaded area in search_area */
    zero_area = LoadArea (reference, x_curr, y_curr, 16, 16, lx);
    *SAD_0 = SAD_Macroblock (zero_area, act_block, 16, Min_FRAME[0]) -
      PREF_NULL_VEC;
    free (zero_area);
  } else
  {
    /* the zero vector is within search_area */
    ii = search_area + (x_curr - ilow) + (y_curr - jlow) * h_length;
    *SAD_0 = SAD_Macroblock (ii, act_block, h_length, Min_FRAME[0]) -
      PREF_NULL_VEC;
  }

  if (xoff == 0 && yoff == 0)
  {
    Min_FRAME[0] = *SAD_0;
    MV_FRAME[0].x = 0;
    MV_FRAME[0].y = 0;
  } else
  {
    ii = search_area + (x_curr + xoff - ilow) + (y_curr + yoff - jlow) * h_length;
    Min_FRAME[0] = SAD_Macroblock (ii, act_block, h_length, Min_FRAME[0]);
    MV_FRAME[0].x = xoff;
    MV_FRAME[0].y = yoff;
  }
  /* NB: if xoff or yoff != 0, the Extended MV Range is used. If we allow
   * the zero vector to be chosen prior to the half pel search in this
   * case, the half pel search might lead to a non-transmittable vector
   * (on the wrong side of zero). If SAD_0 turns out to be the best SAD,
   * the zero-vector will be chosen after half pel search instead.  The
   * zero-vector can be transmitted in all modes, no matter what the MV
   * predictor is */

/* Spiral search for full search motion estimation */
#ifdef FULLSEARCH
  for (l = 1; l <= sxy; l++) {
    i = x_curr + xoff - l;
    j = y_curr + yoff - l;
    for (k = 0; k < 8*l; k++) {
      if (i>=ilow && i<=ihigh && j>=jlow && j<=jhigh) {

        /* 16x16 integer pel MV */
        ii = search_area + (i-ilow) + (j-jlow)*h_length;
        sad = SAD_Macroblock(ii, act_block, h_length, Min_FRAME[0]);
        if (sad < Min_FRAME[0]) {
          MV_FRAME[0].x = i - x_curr;
          MV_FRAME[0].y = j - y_curr;
          Min_FRAME[0] = sad;
        }

      }
      if      (k<2*l) i++;
      else if (k<4*l) j++;
      else if (k<6*l) i--;
      else            j--;
    }      
  }
#else
/* Fast search motion estimation */
  i_min_now = x_curr + xoff;
  j_min_now = y_curr + yoff;

  distortion_0 = distortion_1 = distortion_2 = 65536;

  for (l = 1; l <= 2 * sxy; l++)
  {
    sad_layr = 65536;

    for (m = 0; m < 4; m++)
    {

      i = i_min_now + htp[m];
      j = j_min_now + vtp[m];

      if (i >= ilow && i <= ihigh && j >= jlow && j <= jhigh)
      {
        /* 16x16 integer pel MV */
        ii = search_area + (i - ilow) + (j - jlow) * h_length;
        sad = SAD_Macroblock (ii, act_block, h_length, sad_layr);

        /* set search map to 1 for this location */
        if (sad < Min_FRAME[0])
        {
          MV_FRAME[0].x = i - x_curr;
          MV_FRAME[0].y = j - y_curr;
          Min_FRAME[0] = sad;
        }
        if (sad < sad_layr)
        {
          sad_layr = sad;
          i_min_next = i;
          j_min_next = j;
        }
      }
    }

    i_min_now = i_min_next;
    j_min_now = j_min_next;


    distortion_2 = distortion_1;
    distortion_1 = distortion_0;
    distortion_0 = sad_layr;
    if (distortion_1 <= distortion_0 && distortion_2 <= distortion_0)
    {
      break;
    }
  }
#endif

  if (use_4mv)
  {

    /* Center the 8x8 search around the 16x16 vector.  This is different
     * than in TMN5 where the 8x8 search is also a full search. The
     * reasons for this is: (i) it is faster, and (ii) it generally gives
     * better results because of a better OBMC filtering effect and less
     * bits spent for vectors, and (iii) if the Extended MV Range is used,
     * the search range around the motion vector predictor will be less
     * limited */

    xvec = MV_FRAME[0].x;
    yvec = MV_FRAME[0].y;

    if (!long_vectors || estimation_type == PB_PICTURE_ESTIMATION)
    {
      if (xvec > 15 - DEF_8X8_WIN)
      {
        xvec = 15 - DEF_8X8_WIN;
      }
      if (yvec > 15 - DEF_8X8_WIN)
      {
        yvec = 15 - DEF_8X8_WIN;
      }
      if (xvec < -15 + DEF_8X8_WIN)
      {
        xvec = -15 + DEF_8X8_WIN;
      }
      if (yvec < -15 + DEF_8X8_WIN)
      {
        yvec = -15 + DEF_8X8_WIN;
      }
    }
    adv_x_curr = x_curr + xvec;
    adv_y_curr = y_curr + yvec;

    sxy = DEF_8X8_WIN;

    adv_ilow = adv_x_curr - sxy;
    adv_ihigh = adv_x_curr + sxy;

    adv_jlow = adv_y_curr - sxy;
    adv_jhigh = adv_y_curr + sxy;

    adv_h_length = adv_ihigh - adv_ilow + 16;
    adv_v_length = adv_jhigh - adv_jlow + 16;

    adv_search_area = LoadArea (reference, adv_ilow, adv_jlow,
                                adv_h_length, adv_v_length, lx);

    for (block = 0; block < 4; block++)
    {
      ii = adv_search_area + (adv_x_curr - adv_ilow) + ((block & 1) << 3) +
        (adv_y_curr - adv_jlow + ((block & 2) << 2)) * adv_h_length;
      aa = act_block + ((block & 1) << 3) + ((block & 2) << 2) * 16;
      Min_FRAME[block + 1] = SAD_Block (ii, aa, adv_h_length, Min_FRAME[block + 1]);
      MV_FRAME[block + 1].x = MV_FRAME[0].x;
      MV_FRAME[block + 1].y = MV_FRAME[0].y;
    }

    /* Spiral search for full search motion estimation */
#ifdef FULLSEARCH
    for (l = 1; l <= sxy; l++) {
      i = adv_x_curr - l;
      j = adv_y_curr - l;
      for (k = 0; k < 8*l; k++) {
        if (i>=adv_ilow && i<=adv_ihigh && j>=adv_jlow && j<=adv_jhigh) {
          
          /* 8x8 integer pel MVs */
          for (block = 0; block < 4; block++) {
            ii = adv_search_area + (i-adv_ilow) + ((block&1)<<3) + 
              (j-adv_jlow + ((block&2)<<2) )*adv_h_length;
            aa = act_block + ((block&1)<<3) + ((block&2)<<2)*16;
            sad = SAD_Block(ii, aa, adv_h_length, Min_FRAME[block+1]);
            if (sad < Min_FRAME[block+1]) {
              MV_FRAME[block+1].x = i - x_curr;
              MV_FRAME[block+1].y = j - y_curr;
              Min_FRAME[block+1] = sad;
            }
          }
          
        }
        if      (k<2*l) i++;
        else if (k<4*l) j++;
        else if (k<6*l) i--;
        else            j--;
      }      
    }
#else
    /* Fast search motion estimation */
    for (block = 0; block < 4; block++)
    {
      i_min_now = adv_x_curr;
      j_min_now = adv_y_curr;

      distortion_0 = distortion_1 = distortion_2 = 65536;

      for (l = 1; l <= 2 * sxy; l++)
      {
        sad_layr = 65536;
        for (m = 0; m < 4; m++)
        {

          i = i_min_now + htp[m];
          j = j_min_now + vtp[m];

          if (i >= adv_ilow && i <= adv_ihigh && j >= adv_jlow && j <= adv_jhigh)
          {
            /* 8x8 integer pel MVs */
            ii = adv_search_area + (i - adv_ilow) + ((block & 1) << 3) +
              (j - adv_jlow + ((block & 2) << 2)) * adv_h_length;
            aa = act_block + ((block & 1) << 3) + ((block & 2) << 2) * 16;
            sad = SAD_Block (ii, aa, adv_h_length, sad_layr);
            if (sad < Min_FRAME[block + 1])
            {
              MV_FRAME[block + 1].x = i - x_curr;
              MV_FRAME[block + 1].y = j - y_curr;
              Min_FRAME[block + 1] = sad;
            }
            if (sad < sad_layr)
            {
              sad_layr = sad;
              i_min_next = i;
              j_min_next = j;
            }
          }
        }
      }

      i_min_now = i_min_next;
      j_min_now = j_min_next;

      distortion_2 = distortion_1;
      distortion_1 = distortion_0;
      distortion_0 = sad_layr;
      if (distortion_1 <= distortion_0 && distortion_2 <= distortion_0)
      {
        break;
      }
    }
#endif

  }
  i = x_curr / MB_SIZE + 1;
  j = y_curr / MB_SIZE + 1;

  if (!use_4mv)
  {
    MV[0][j][i]->x = MV_FRAME[0].x;
    MV[0][j][i]->y = MV_FRAME[0].y;
    MV[0][j][i]->min_error = Min_FRAME[0];
  } else
  {
    for (k = 0; k < 5; k++)
    {
      MV[k][j][i]->x = MV_FRAME[k].x;
      MV[k][j][i]->y = MV_FRAME[k].y;
      MV[k][j][i]->min_error = Min_FRAME[k];
    }
  }

  free (act_block);
  free (search_area);
  if (use_4mv)
    free (adv_search_area);
  return;
}

/**********************************************************************
 *
 *	Name:        MotionEstimationBackward
 *	Description: Estimate backward motion vector for one MB
 *
 *	Input:       pointers to current and reference (future)
 *                   image, pointers to current slice and
 *                   current MB
 *	Returns:
 *	Side effects:	motion vector information in MB changed
 *
 *	Date: 970728    Author: Michael Gallant --- mikeg@ee.ubc.ca
 *                      Essentially the same as MotionEstimation.
 *                      Written as a separate routine for clarity and
 *                      readability.
 *
 *************************************************************************/

void MotionEstimationBackward (unsigned char *curr, unsigned char *reference,
                              int x_curr, int y_curr, int xoff, int yoff,
                   int seek_dist, MotionVector * MV[7][MBR + 1][MBC + 2],
                                int *SAD_0)

{
  int Min_FRAME;
  MotionVector MV_FRAME;
  unsigned char *act_block, *ii;
  unsigned char *search_area, *zero_area = NULL;
  int sxy, i, j, k, l;
  int ihigh, ilow, jhigh, jlow, h_length, v_length;
  int sad, lx;
  int xmax, ymax;
#ifndef FULLSEARCH
  static int htp[4] = {-1, 0, 1, 0}; //SV-XXX: added default type int
  static int vtp[4] = {0, 1, 0, -1}; //SV-XXX: added default type int
  int j_min_now, i_min_now, i_min_next=0, j_min_next=0, sad_layr; //SV-XXX: inited to 0
  int distortion_0, distortion_1, distortion_2;
#endif

  xmax = sed_pels;
  ymax = sed_lines;
  sxy = seek_dist;

  if (!long_vectors)
  {
    /* Maximum normal search range centered around _zero-vector_ */
    sxy = mmin (15, sxy);
  } else
  {
    if(EPTYPE)
    {
      /* if RVLC codes of H.263+ are used, new motion vector range  *
       * is used.                                                   */
      sxy = (sed_pels < 353) ? 31 : (sed_pels < 705) ? 63 : (sed_pels < 1409) ? 127 : 255;
    }
    else
    {
      /* Maximum extended search range centered around _predictor_ */
      sxy = mmin (15 - (2 * DEF_8X8_WIN), sxy);
    }
    xoff = mmin (16, mmax (-16, xoff));
    yoff = mmin (16, mmax (-16, yoff));

    /* There is no need to check if (xoff + x_curr) points outside the
     * picture, since the Extended Motion Vector Range is always used
     * together with the Unrestricted MV mode */
  }

  lx = (mv_outside_frame ? sed_pels + (long_vectors ? 64 : 32) : sed_pels);

  ilow = x_curr + xoff - sxy;
  ihigh = x_curr + xoff + sxy;

  jlow = y_curr + yoff - sxy;
  jhigh = y_curr + yoff + sxy;

  if (!mv_outside_frame)
  {
    if (ilow < 0)
      ilow = 0;
    if (ihigh > xmax - 16)
      ihigh = xmax - 16;
    if (jlow < 0)
      jlow = 0;
    if (jhigh > ymax - 16)
      jhigh = ymax - 16;
  }
  h_length = ihigh - ilow + 16;
  v_length = jhigh - jlow + 16;
  act_block = LoadArea (curr, x_curr, y_curr, 16, 16, sed_pels);
  search_area = LoadArea (reference, ilow, jlow, h_length, v_length, lx);

  Min_FRAME = INT_MAX;
  MV_FRAME.x = 0;
  MV_FRAME.y = 0;
  MV_FRAME.x_half = 0;
  MV_FRAME.y_half = 0;

  /* Zero vector search */
  if (x_curr - ilow < 0 || y_curr - jlow < 0 ||
      x_curr - ilow + MB_SIZE > h_length || y_curr - jlow + MB_SIZE > v_length)
  {
    /* in case the zero vector is outside the loaded area in search_area */
    zero_area = LoadArea (reference, x_curr, y_curr, 16, 16, lx);
    *SAD_0 = SAD_Macroblock (zero_area, act_block, 16, Min_FRAME) -
      PREF_NULL_VEC;
    free (zero_area);
  } else
  {
    /* the zero vector is within search_area */
    ii = search_area + (x_curr - ilow) + (y_curr - jlow) * h_length;
    *SAD_0 = SAD_Macroblock (ii, act_block, h_length, Min_FRAME) -
      PREF_NULL_VEC;
  }

  if (xoff == 0 && yoff == 0)
  {
    Min_FRAME = *SAD_0;
    MV_FRAME.x = 0;
    MV_FRAME.y = 0;
  } else
  {
    ii = search_area + (x_curr + xoff - ilow) + (y_curr + yoff - jlow) * h_length;
    Min_FRAME = SAD_Macroblock (ii, act_block, h_length, Min_FRAME);
    MV_FRAME.x = xoff;
    MV_FRAME.y = yoff;
  }
  /* NB: if xoff or yoff != 0, the Extended MV Range is used. If we allow
   * the zero vector to be chosen prior to the half pel search in this
   * case, the half pel search might lead to a non-transmittable vector
   * (on the wrong side of zero). If SAD_0 turns out to be the best SAD,
   * the zero-vector will be chosen after half pel search instead.  The
   * zero-vector can be transmitted in all modes, no matter what the MV
   * predictor is */

#ifdef FULLSEARCH
  /* Spiral search for full search motion estimation */
  for (l = 1; l <= sxy; l++) {
    i = x_curr + xoff - l;
    j = y_curr + yoff - l;
    for (k = 0; k < 8*l; k++) {
      if (i>=ilow && i<=ihigh && j>=jlow && j<=jhigh) {

        /* 16x16 integer pel MV */
        ii = search_area + (i-ilow) + (j-jlow)*h_length;
        sad = SAD_Macroblock(ii, act_block, h_length, Min_FRAME);
        if (sad < Min_FRAME) {
          MV_FRAME.x = i - x_curr;
          MV_FRAME.y = j - y_curr;
          Min_FRAME = sad;
        }

      }
      if      (k<2*l) i++;
      else if (k<4*l) j++;
      else if (k<6*l) i--;
      else            j--;
    }      
  }
#else
  /* Fast search motion estimation */

  i_min_now = x_curr + xoff;
  j_min_now = y_curr + yoff;

  distortion_0 = distortion_1 = distortion_2 = 65536;

  for (l = 1; l <= 2 * sxy; l++)
  {
    sad_layr = 65536;

    for (k = 0; k < 4; k++)
    {

      i = i_min_now + htp[k];
      j = j_min_now + vtp[k];

      if (i >= ilow && i <= ihigh && j >= jlow && j <= jhigh)
      {
        /* 16x16 integer pel MV */
        ii = search_area + (i - ilow) + (j - jlow) * h_length;
        sad = SAD_Macroblock (ii, act_block, h_length, sad_layr);

        /* set search map to 1 for this location */
        if (sad < Min_FRAME)
        {
          MV_FRAME.x = i - x_curr;
          MV_FRAME.y = j - y_curr;
          Min_FRAME = sad;
        }
        if (sad < sad_layr)
        {
          sad_layr = sad;
          i_min_next = i;
          j_min_next = j;
        }
      }
    }

    i_min_now = i_min_next;
    j_min_now = j_min_next;


    distortion_2 = distortion_1;
    distortion_1 = distortion_0;
    distortion_0 = sad_layr;
    if (distortion_1 <= distortion_0 && distortion_2 <= distortion_0)
    {
      break;
    }
  }
#endif

  i = x_curr / MB_SIZE + 1;
  j = y_curr / MB_SIZE + 1;

  MV[5][j][i]->x = MV_FRAME.x;
  MV[5][j][i]->y = MV_FRAME.y;
  MV[5][j][i]->min_error = Min_FRAME;

  free (act_block);
  free (search_area);
  return;
}

/**********************************************************************
 *
 *	Name:        LoadArea
 *	Description:    fills array with a square of image-data
 *
 *	Input:	       pointer to image and position, x and y size
 *	Returns:       pointer to area
 *	Side effects:  memory allocated to array
 *
 *	Date: 940203	Author: PGB
 *                      Mod: KOL
 *
 ***********************************************************************/


unsigned char *LoadArea (unsigned char *im, int x, int y,
                          int x_size, int y_size, int lx)
{
  unsigned char *res = (unsigned char *) malloc (sizeof (char) * x_size * y_size);
  unsigned char *in;
  unsigned char *out;
  int i = x_size;
  int j = y_size;

  in = im + (y * lx) + x;
  out = res;

  while (j--)
  {
    while (i--)
      *out++ = *in++;
    i = x_size;
    in += lx - x_size;
  };
  return res;
}

/**********************************************************************
 *
 *	Name:        SAD_Macroblock
 *	Description:    fast way to find the SAD of one vector
 *
 *	Input:	        pointers to search_area and current block,
 *                      Min_F1/F2/FR
 *	Returns:        sad_f1/f2
 *	Side effects:
 *
 *	Date: 940203        Author: PGB
 *                      Mod:    KOL
 *
 ***********************************************************************/


int SAD_Macroblock (unsigned char *ii, unsigned char *act_block,
                     int h_length, int Min_FRAME)
{
  int i;
  int sad = 0;
  unsigned char *kk;

  kk = act_block;
  i = 16;
  while (i--)
  {
    sad += (abs (*ii - *kk) + abs (*(ii + 1) - *(kk + 1))
            + abs (*(ii + 2) - *(kk + 2)) + abs (*(ii + 3) - *(kk + 3))
            + abs (*(ii + 4) - *(kk + 4)) + abs (*(ii + 5) - *(kk + 5))
            + abs (*(ii + 6) - *(kk + 6)) + abs (*(ii + 7) - *(kk + 7))
            + abs (*(ii + 8) - *(kk + 8)) + abs (*(ii + 9) - *(kk + 9))
          + abs (*(ii + 10) - *(kk + 10)) + abs (*(ii + 11) - *(kk + 11))
          + abs (*(ii + 12) - *(kk + 12)) + abs (*(ii + 13) - *(kk + 13))
        + abs (*(ii + 14) - *(kk + 14)) + abs (*(ii + 15) - *(kk + 15)));

    ii += h_length;
    kk += 16;
    if (sad > Min_FRAME)
      return INT_MAX;
  }
  return sad;
}

int SAD_Block (unsigned char *ii, unsigned char *act_block,
                int h_length, int min_sofar)
{
  int i;
  int sad = 0;
  unsigned char *kk;

  kk = act_block;
  i = 8;
  while (i--)
  {
    sad += (abs (*ii - *kk) + abs (*(ii + 1) - *(kk + 1))
            + abs (*(ii + 2) - *(kk + 2)) + abs (*(ii + 3) - *(kk + 3))
            + abs (*(ii + 4) - *(kk + 4)) + abs (*(ii + 5) - *(kk + 5))
            + abs (*(ii + 6) - *(kk + 6)) + abs (*(ii + 7) - *(kk + 7)));

    ii += h_length;
    kk += 16;
    if (sad > min_sofar)
      return INT_MAX;
  }
  return sad;
}

int SAD_MB_Bidir (unsigned char *ii, unsigned char *aa, unsigned char *bb,
                   int width, int min_sofar)
{
  int i, sad = 0;
  unsigned char *ll, *kk;
  kk = aa;
  ll = bb;
  i = 16;
  while (i--)
  {
    sad += (abs (*ii - ((*kk + *ll) >> 1)) +
            abs (*(ii + 1) - ((*(kk + 1) + *(ll + 1)) >> 1)) +
            abs (*(ii + 2) - ((*(kk + 2) + *(ll + 2)) >> 1)) +
            abs (*(ii + 3) - ((*(kk + 3) + *(ll + 3)) >> 1)) +
            abs (*(ii + 4) - ((*(kk + 4) + *(ll + 4)) >> 1)) +
            abs (*(ii + 5) - ((*(kk + 5) + *(ll + 5)) >> 1)) +
            abs (*(ii + 6) - ((*(kk + 6) + *(ll + 6)) >> 1)) +
            abs (*(ii + 7) - ((*(kk + 7) + *(ll + 7)) >> 1)) +
            abs (*(ii + 8) - ((*(kk + 8) + *(ll + 8)) >> 1)) +
            abs (*(ii + 9) - ((*(kk + 9) + *(ll + 9)) >> 1)) +
            abs (*(ii + 10) - ((*(kk + 10) + *(ll + 10)) >> 1)) +
            abs (*(ii + 11) - ((*(kk + 11) + *(ll + 11)) >> 1)) +
            abs (*(ii + 12) - ((*(kk + 12) + *(ll + 12)) >> 1)) +
            abs (*(ii + 13) - ((*(kk + 13) + *(ll + 13)) >> 1)) +
            abs (*(ii + 14) - ((*(kk + 14) + *(ll + 14)) >> 1)) +
            abs (*(ii + 15) - ((*(kk + 15) + *(ll + 15)) >> 1)));

    ii += width;
    kk += width;
    ll += width;
    if (sad > min_sofar)
      return INT_MAX;
  }
  return sad;
}

int SAD_MB_integer (int *ii, int *act_block, int h_length, int min_sofar)
{
  int i, sad = 0, *kk;

  kk = act_block;
  i = 16;
  while (i--)
  {
    sad += (abs (*ii - *kk) + abs (*(ii + 1) - *(kk + 1))
            + abs (*(ii + 2) - *(kk + 2)) + abs (*(ii + 3) - *(kk + 3))
            + abs (*(ii + 4) - *(kk + 4)) + abs (*(ii + 5) - *(kk + 5))
            + abs (*(ii + 6) - *(kk + 6)) + abs (*(ii + 7) - *(kk + 7))
            + abs (*(ii + 8) - *(kk + 8)) + abs (*(ii + 9) - *(kk + 9))
          + abs (*(ii + 10) - *(kk + 10)) + abs (*(ii + 11) - *(kk + 11))
          + abs (*(ii + 12) - *(kk + 12)) + abs (*(ii + 13) - *(kk + 13))
        + abs (*(ii + 14) - *(kk + 14)) + abs (*(ii + 15) - *(kk + 15)));

    ii += h_length;
    kk += 16;
    if (sad > min_sofar)
      return INT_MAX;
  }
  return sad;
}

/**********************************************************************
 *
 *	Name:        FindMB
 *	Description:	Picks out one MB from picture
 *
 *	Input:        position of MB to pick out,
 *        pointer to frame data, empty 16x16 array
 *	Returns:
 *	Side effects:	fills array with MB data
 *
 *	Date: 930119	Author: Karl Olav Lillevold
 *
 ***********************************************************************/

void FindMB (int x, int y, unsigned char *image, int MB[16][16])

{
  int n;
  register int m;

  for (n = 0; n < MB_SIZE; n++)
    for (m = 0; m < MB_SIZE; m++)
      MB[n][m] = *(image + x + m + (y + n) * sed_pels);
}
