/************************************************************************
 *
 *  pred.c, part of tmn (TMN encoder)
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


#include"sim.h"

static int roundtab[] = {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2};

/**********************************************************************
 *
 *	Name:        Predict_P
 *	Description:    Predicts P macroblock in advanced or normal
 *                      mode
 *
 *	Input:        pointers to current and previous frames
 *        and previous interpolated image,
 *                      position and motion vector array
 *	Returns:	pointer to MB_Structure of data to be coded
 *	Side effects:	allocates memory to MB_Structure
 *
 *	Date: 9501        Author: <klillevo@mailbox.jf.intel.com>
 *
 ***********************************************************************/

MB_Structure *Predict_P (PictImage * curr_image, PictImage * prev_image,
                         unsigned char *prev_ipol, MB_Structure * pred_macroblock,
                         int x, int y,
                         MotionVector * MV[7][MBR + 1][MBC + 2], int PB)
{
  int m, n;
  int curr[16][16];
  MotionVector *fr0, *fr1, *fr2, *fr3, *fr4;
  int sum, dx, dy;
  int xmb, ymb;

  MB_Structure *pred_error = (MB_Structure *) malloc (sizeof (MB_Structure));

  xmb = x / MB_SIZE + 1;
  ymb = y / MB_SIZE + 1;

  fr0 = MV[0][ymb][xmb];
  fr1 = MV[1][ymb][xmb];
  fr2 = MV[2][ymb][xmb];
  fr3 = MV[3][ymb][xmb];
  fr4 = MV[4][ymb][xmb];

  /* Find MB in current image */
  FindMB (x, y, curr_image->lum, curr);

  /* Find prediction based on half pel MV    */

  if (overlapping_MC)
  {
    FindPredOBMC (x, y, MV, prev_ipol, &pred_macroblock->lum[0][0], 0, PB);
    FindPredOBMC (x, y, MV, prev_ipol, &pred_macroblock->lum[0][8], 1, PB);
    FindPredOBMC (x, y, MV, prev_ipol, &pred_macroblock->lum[8][0], 2, PB);
    FindPredOBMC (x, y, MV, prev_ipol, &pred_macroblock->lum[8][8], 3, PB);
  } else if (fr0->Mode == MODE_INTER4V || fr0->Mode == MODE_INTER4V_Q)
  {
    /* Luma */
    FindPred (x, y, fr1, prev_ipol, &pred_macroblock->lum[0][0], 8, 0);
    FindPred (x, y, fr2, prev_ipol, &pred_macroblock->lum[0][8], 8, 1);
    FindPred (x, y, fr3, prev_ipol, &pred_macroblock->lum[8][0], 8, 2);
    FindPred (x, y, fr4, prev_ipol, &pred_macroblock->lum[8][8], 8, 3);
  } else
  {
    FindPred (x, y, fr0, prev_ipol, &pred_macroblock->lum[0][0], 16, 0);
  }

  /* Do the actual prediction */
  if (fr0->Mode == MODE_INTER || fr0->Mode == MODE_INTER_Q)
  {
    for (n = 0; n < MB_SIZE; n++)
      for (m = 0; m < MB_SIZE; m++)
        pred_error->lum[n][m] = (int) (curr[n][m] - pred_macroblock->lum[n][m]);

    dx = 2 * fr0->x + fr0->x_half;
    dy = 2 * fr0->y + fr0->y_half;
    dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
    dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);

    DoPredChrom_P (x, y, dx, dy, curr_image, prev_image, pred_macroblock, pred_error);

  } else if (fr0->Mode == MODE_INTER4V || fr0->Mode == MODE_INTER4V_Q)
  {
    for (n = 0; n < MB_SIZE; n++)
      for (m = 0; m < MB_SIZE; m++)
        pred_error->lum[n][m] = (int) (curr[n][m] - pred_macroblock->lum[n][m]);

    sum = 2 * fr1->x + fr1->x_half + 2 * fr2->x + fr2->x_half +
      2 * fr3->x + fr3->x_half + 2 * fr4->x + fr4->x_half;
    dx = sign (sum) * (roundtab[abs (sum) % 16] + (abs (sum) / 16) * 2);

    sum = 2 * fr1->y + fr1->y_half + 2 * fr2->y + fr2->y_half +
      2 * fr3->y + fr3->y_half + 2 * fr4->y + fr4->y_half;
    dy = sign (sum) * (roundtab[abs (sum) % 16] + (abs (sum) / 16) * 2);

    DoPredChrom_P (x, y, dx, dy, curr_image, prev_image, pred_macroblock, pred_error);
  } else
    fprintf (stderr, "Illegal Mode in Predict_P (pred.c)\n");

  return pred_error;
}


/***********************************************************************
 *
 *	Name:        Predict_B
 *	Description:    Predicts the B macroblock in PB-frame prediction
 *
 *	Input:	        pointers to current frame, previous recon. frame,
 *                      pos. in image, MV-data, reconstructed macroblock
 *                      from image ahead
 *	Returns:        pointer to differential MB data after prediction
 *	Side effects:   allocates memory to MB_structure
 *
 *	Date: 950113        Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

MB_Structure *Predict_B (PictImage * curr_image, PictImage * prev_image,
  unsigned char *prev_ipol, MB_Structure * pred_macroblock, int x, int y,
                          MotionVector * MV[7][MBR + 1][MBC + 2], MotionVector * B_f_MV[7][MBR + 1][MBC + 2],
  MB_Structure * recon_P, int TRD, int TRB, int PB, int *im_PB_pred_type)
{
  int i, j, k, xx, yy;
  int dx, dy, sad, sad_min = INT_MAX, curr[16][16], bdx = 0, bdy = 0;
  MB_Structure *p_err = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *frw_pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MotionVector *f[7];
  MotionVector *frw[7];
  int xvec, yvec, mvx, mvy;
  int SADbidir, SADforw,SADbackw,min_SAD;
  int forward_pred = 0;
  int bidir_pred = 0;
  int backward_pred = 0;

  for (k = 0; k <= 4; k++)
    f[k] = MV[k][y / MB_SIZE + 1][x / MB_SIZE + 1];

  for (k = 0; k <= 4; k++)
    frw[k] = B_f_MV[k][y / MB_SIZE + 1][x / MB_SIZE + 1];

  /* Find MB in current image */
  FindMB (x, y, curr_image->lum, curr);

  if (f[0]->Mode == MODE_INTER4V || f[0]->Mode == MODE_INTER4V_Q)
  {
    /* Mode INTER4V */
    if (PB == IM_PB_FRAMES)
    {
      bdx = 0;
      bdy = 0;
    } else
    {
      /* Find forward prediction. Luma */
      for (j = -DEF_PBDELTA_WIN; j <= DEF_PBDELTA_WIN; j++)
      {
        for (i = -DEF_PBDELTA_WIN; i <= DEF_PBDELTA_WIN; i++)
        {

          FindForwLumPredPB (prev_ipol, x, y, f[1], &pred->lum[0][0], TRD, TRB, i, j, 8, 0);
          FindForwLumPredPB (prev_ipol, x, y, f[2], &pred->lum[0][8], TRD, TRB, i, j, 8, 1);
          FindForwLumPredPB (prev_ipol, x, y, f[3], &pred->lum[8][0], TRD, TRB, i, j, 8, 2);
          FindForwLumPredPB (prev_ipol, x, y, f[4], &pred->lum[8][8], TRD, TRB, i, j, 8, 3);

          sad = SAD_MB_integer (&curr[0][0], &pred->lum[0][0], 16, INT_MAX);
          if (i == 0 && j == 0)
            sad -= PREF_PBDELTA_NULL_VEC;
          if (sad < sad_min)
          {
            sad_min = sad;
            bdx = i;
            bdy = j;
          }
        }
      }
    }

    FindForwLumPredPB (prev_ipol, x, y, f[1], &pred->lum[0][0], TRD, TRB, bdx, bdy, 8, 0);
    FindForwLumPredPB (prev_ipol, x, y, f[2], &pred->lum[0][8], TRD, TRB, bdx, bdy, 8, 1);
    FindForwLumPredPB (prev_ipol, x, y, f[3], &pred->lum[8][0], TRD, TRB, bdx, bdy, 8, 2);
    FindForwLumPredPB (prev_ipol, x, y, f[4], &pred->lum[8][8], TRD, TRB, bdx, bdy, 8, 3);

    /* Find bidirectional prediction */
    FindBiDirLumPredPB (&recon_P->lum[0][0], f[1], &pred->lum[0][0],
                        TRD, TRB, bdx, bdy, 0, 0);
    FindBiDirLumPredPB (&recon_P->lum[0][8], f[2], &pred->lum[0][8],
                        TRD, TRB, bdx, bdy, 1, 0);
    FindBiDirLumPredPB (&recon_P->lum[8][0], f[3], &pred->lum[8][0],
                        TRD, TRB, bdx, bdy, 0, 1);
    FindBiDirLumPredPB (&recon_P->lum[8][8], f[4], &pred->lum[8][8],
                        TRD, TRB, bdx, bdy, 1, 1);

  } else
  {
    /* Mode INTER or INTER_Q */
    if (PB == IM_PB_FRAMES)
    {
      bdx = 0;
      bdy = 0;
    } else
    {
      /* Find forward prediction */
      for (j = -DEF_PBDELTA_WIN; j <= DEF_PBDELTA_WIN; j++)
      {
        for (i = -DEF_PBDELTA_WIN; i <= DEF_PBDELTA_WIN; i++)
        {

          dx = i;
          dy = j;

          /* To keep things simple I turn off PB delta vectors at the
           * edges */
          if (!mv_outside_frame)
          {
            if (x == 0)
              dx = 0;
            if (x == pels - MB_SIZE)
              dx = 0;
            if (y == 0)
              dy = 0;
            if (y == lines - MB_SIZE)
              dy = 0;
          }
          if (f[0]->Mode == MODE_INTRA || f[0]->Mode == MODE_INTRA_Q)
          {
            dx = dy = 0;
          }
          if ((f[0]->x == 0 && f[0]->y == 0) &&
              (f[0]->x_half == 0 && f[0]->y_half == 0))
          {
            dx = dy = 0;
          }
          FindForwLumPredPB (prev_ipol, x, y, f[0], &pred->lum[0][0],
                             TRD, TRB, dx, dy, 16, 0);

          sad = SAD_MB_integer (&curr[0][0], &pred->lum[0][0], 16, INT_MAX);
          if (i == 0 && j == 0)
          {
            sad -= PREF_PBDELTA_NULL_VEC;
          }
          if (sad < sad_min)
          {
            sad_min = sad;
            bdx = dx;
            bdy = dy;
          }
        }
      }
    }

    FindForwLumPredPB (prev_ipol, x, y, f[0], &pred->lum[0][0], TRD, TRB,
                       bdx, bdy, 16, 0);

    /* Find bidirectional prediction */
    FindBiDirLumPredPB (&recon_P->lum[0][0], f[0], &pred->lum[0][0],
                        TRD, TRB, bdx, bdy, 0, 0);
    FindBiDirLumPredPB (&recon_P->lum[0][8], f[0], &pred->lum[0][8],
                        TRD, TRB, bdx, bdy, 1, 0);
    FindBiDirLumPredPB (&recon_P->lum[8][0], f[0], &pred->lum[8][0],
                        TRD, TRB, bdx, bdy, 0, 1);
    FindBiDirLumPredPB (&recon_P->lum[8][8], f[0], &pred->lum[8][8],
                        TRD, TRB, bdx, bdy, 1, 1);
  }

  if (PB == IM_PB_FRAMES)
  {
    SADbidir = SAD_MB_integer (&curr[0][0], &pred->lum[0][0], 16, INT_MAX);

    FindPred (x, y, frw[0], prev_ipol, &frw_pred->lum[0][0], 16, 0);
    SADforw = SAD_MB_integer (&curr[0][0], &frw_pred->lum[0][0], 16, INT_MAX);
    SADbackw = SAD_MB_integer (&curr[0][0], &recon_P->lum[0][0], 16, INT_MAX);

    /* make decision about using forward prediction, backward prediction or bidir pre
diction */
    min_SAD=mmin(mmin(SADforw,SADbackw),(SADbidir-100));

    if (min_SAD == (SADbidir - 100))
    {
      /* bidirectional prediction is chosen */
      bidir_pred = 1;
      (*im_PB_pred_type) = BIDIRECTIONAL_PREDICTION;
    } 
    else if (min_SAD==SADbackw)
    {
      /* backward prediction is chosen */
      backward_pred = 1;
      (*im_PB_pred_type) = BACKWARD_PREDICTION;
    }
    else 
    {
      /* forward prediction is chosen */
      forward_pred = 1;
      (*im_PB_pred_type) = FORWARD_PREDICTION;
    }
  }
  if (PB == IM_PB_FRAMES && forward_pred)
  {
    dx = 2 * frw[0]->x + frw[0]->x_half;
    dy = 2 * frw[0]->y + frw[0]->y_half;
    dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
    dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);

    /* predict B as if it is P */
    DoPredChrom_P (x, y, dx, dy, curr_image, prev_image, frw_pred, p_err);

    for (j = 0; j < MB_SIZE; j++)
      for (i = 0; i < MB_SIZE; i++)
      {
        p_err->lum[j][i] = *(curr_image->lum + x + i + (y + j) * pels) - frw_pred->lum[j][i];
        pred_macroblock->lum[j][i] = frw_pred->lum[j][i];
      }
    xx = x >> 1;
    yy = y >> 1;
    for (j = 0; j < MB_SIZE >> 1; j++)
      for (i = 0; i < MB_SIZE >> 1; i++)
      {
        p_err->Cr[j][i] = *(curr_image->Cr + xx + i + (yy + j) * cpels) - frw_pred->Cr[j][i];
        p_err->Cb[j][i] = *(curr_image->Cb + xx + i + (yy + j) * cpels) - frw_pred->Cb[j][i];
        pred_macroblock->Cr[j][i] = frw_pred->Cr[j][i];
        pred_macroblock->Cb[j][i] = frw_pred->Cb[j][i];
      }
  } 
  else if (PB == IM_PB_FRAMES && backward_pred)
  {
    dx = 0;
    dy = 0;
    
    for (j = 0; j < MB_SIZE; j++)
      for (i = 0; i < MB_SIZE; i++)
      {
        p_err->lum[j][i] = *(curr_image->lum + x + i + (y + j) * pels) - recon_P->lum
[j][i];
        pred_macroblock->lum[j][i] = recon_P->lum[j][i];
      }

    xx = x >> 1;
    yy = y >> 1;

    for (j = 0; j < MB_SIZE >> 1; j++)
      for (i = 0; i < MB_SIZE >> 1; i++)
      {
        p_err->Cr[j][i] = *(curr_image->Cr + xx + i + (yy + j) * cpels) - recon_P->Cr[j][i];
        p_err->Cb[j][i] = *(curr_image->Cb + xx + i + (yy + j) * cpels) - recon_P->Cb[j][i];

        pred_macroblock->Cr[j][i] = recon_P->Cr[j][i];
        pred_macroblock->Cb[j][i] = recon_P->Cb[j][i];
      }
  }
  else
  {
    /* bidir prediction */
    if (f[0]->Mode == MODE_INTER4V || f[0]->Mode == MODE_INTER4V_Q)
    {
      /* Mode INTER4V chroma vectors are sum of B luma vectors divided and
       * rounded */
      xvec = yvec = 0;
      for (k = 1; k <= 4; k++)
      {
        xvec += TRB * (2 * f[k]->x + f[k]->x_half) / TRD + bdx;
        yvec += TRB * (2 * f[k]->y + f[k]->y_half) / TRD + bdy;
      }

      /* round values according to TABLE 16/H.263 */
      dx = sign (xvec) * (roundtab[abs (xvec) % 16] + (abs (xvec) / 16) * 2);
      dy = sign (yvec) * (roundtab[abs (yvec) % 16] + (abs (yvec) / 16) * 2);
      FindChromBlock_P (x, y, dx, dy, prev_image, pred);

      /* chroma vectors are sum of B luma vectors divided and rounded */
      xvec = yvec = 0;
      for (k = 1; k <= 4; k++)
      {
        mvx = 2 * f[k]->x + f[k]->x_half;
        mvy = 2 * f[k]->y + f[k]->y_half;
        xvec += bdx == 0 ? (TRB - TRD) * mvx / TRD : TRB * mvx / TRD + bdx - mvx;
        yvec += bdy == 0 ? (TRB - TRD) * mvy / TRD : TRB * mvy / TRD + bdy - mvy;
      }

      /* round values according to TABLE 16/H.263 */
      dx = sign (xvec) * (roundtab[abs (xvec) % 16] + (abs (xvec) / 16) * 2);
      dy = sign (yvec) * (roundtab[abs (yvec) % 16] + (abs (yvec) / 16) * 2);

      FindBiDirChrPredPB (recon_P, dx, dy, pred);
    } 
    else
    {
      xvec = 4 * (TRB * (2 * f[0]->x + f[0]->x_half) / TRD + bdx);
      yvec = 4 * (TRB * (2 * f[0]->y + f[0]->y_half) / TRD + bdy);
      /* round values according to TABLE 16/H.263 */
      dx = sign (xvec) * (roundtab[abs (xvec) % 16] + (abs (xvec) / 16) * 2);
      dy = sign (yvec) * (roundtab[abs (yvec) % 16] + (abs (yvec) / 16) * 2);

      /* chroma vectors */
      FindChromBlock_P (x, y, dx, dy, prev_image, pred);

      mvx = 2 * f[0]->x + f[0]->x_half;
      xvec = bdx == 0 ? (TRB - TRD) * mvx / TRD : TRB * mvx / TRD + bdx - mvx;
      xvec *= 4;

      mvy = 2 * f[0]->y + f[0]->y_half;
      yvec = bdy == 0 ? (TRB - TRD) * mvy / TRD : TRB * mvy / TRD + bdy - mvy;
      yvec *= 4;

      /* round values according to TABLE 16/H.263 */
      dx = sign (xvec) * (roundtab[abs (xvec) % 16] + (abs (xvec) / 16) * 2);
      dy = sign (yvec) * (roundtab[abs (yvec) % 16] + (abs (yvec) / 16) * 2);

      FindBiDirChrPredPB (recon_P, dx, dy, pred);
    }

    /* Do the actual prediction */
    for (j = 0; j < MB_SIZE; j++)
      for (i = 0; i < MB_SIZE; i++)
      {
        p_err->lum[j][i] = *(curr_image->lum + x + i + (y + j) * pels) - pred->lum[j][i];
        pred_macroblock->lum[j][i] = pred->lum[j][i];
      }

    xx = x >> 1;
    yy = y >> 1;
    for (j = 0; j < MB_SIZE >> 1; j++)
      for (i = 0; i < MB_SIZE >> 1; i++)
      {
        p_err->Cr[j][i] = *(curr_image->Cr + xx + i + (yy + j) * cpels) - pred->Cr[j][i];
        p_err->Cb[j][i] = *(curr_image->Cb + xx + i + (yy + j) * cpels) - pred->Cb[j][i];

        pred_macroblock->Cr[j][i] = pred->Cr[j][i];
        pred_macroblock->Cb[j][i] = pred->Cb[j][i];
      }
  }

  /* store PB-deltas */
  MV[6][y / MB_SIZE + 1][x / MB_SIZE + 1]->x = bdx; /* is in half pel format */
  MV[6][y / MB_SIZE + 1][x / MB_SIZE + 1]->y = bdy;
  MV[6][y / MB_SIZE + 1][x / MB_SIZE + 1]->x_half = 0;
  MV[6][y / MB_SIZE + 1][x / MB_SIZE + 1]->y_half = 0;

  free (pred);
  free (frw_pred);

  return p_err;
}

/***********************************************************************
 *
 *	Name:           Predict_EI
 *	Description:    Predicts macroblock in EI picture 
 *
 *	Input:	        pointers to current frame, base recon. base
 *                      interpolated, pos. in image, MV data, 
 *                      and MB prediction type.
 *	Returns:        pointer to differential MB data after prediction
 *	Side effects:
 *
 *	Date: 970831    Author: Michael Gallant ---mikeg@ee.ubc.ca
 *
 ***********************************************************************/
MB_Structure *Predict_EI( PictImage *curr_image, PictImage *base_recon, 
                          unsigned char *base_ipol, int x, int y,
                          int *prediction_type , int *mode)
{
  int curr[16][16];
  MB_Structure *upward_pred = (MB_Structure *)malloc(sizeof(MB_Structure));
  MB_Structure *pred_error = (MB_Structure *) malloc (sizeof (MB_Structure));
  MotionVector ZERO = {0,0,0,0,0};
  int i,j;
  int SADup;

  /* Find MB in current image */
  FindMB (x, y, curr_image->lum, curr);

  /* Find predicted MB for current MB (upward prediction so MV=0 */
  FindPred (x, y, &ZERO, base_ipol, &upward_pred->lum[0][0], 16, 0);

  SADup = SAD_MB_integer (&curr[0][0], &upward_pred->lum[0][0], 16, INT_MAX);
  SADup -= 150;

  *(mode) = ChooseMode( curr_image->lum, x, y, SADup);
                                
  if (MODE_INTRA == *(mode) || MODE_INTRA_Q == *(mode)) 
  {
    *(prediction_type) = EI_INTRA_PREDICTION;
  }
  else
  {
    *(prediction_type) = EI_UPWARD_PREDICTION;
  }

  switch (*(prediction_type))
  {
    case EI_UPWARD_PREDICTION:
    
      DoPredChrom_P (x, y, 0, 0, curr_image, base_recon, upward_pred, pred_error);
    
      for (j = 0; j < MB_SIZE; j++)
      {
        for (i = 0; i < MB_SIZE; i++)
        { 
          pred_error->lum[j][i] = *(curr_image->lum + x + i + (y + j) * pels) - 
                                    upward_pred->lum[j][i];
        }
      }

      break;

    case EI_INTRA_PREDICTION:
 
      break;

    default:
 
      break;

  }

  free (upward_pred);

  return pred_error;
}

/***********************************************************************
 *
 *	Name:           MB_Recon_EI
 *	Description:    Reconstructs EI macroblocks
 *
 *	Input:	        
 *	Returns:        pointer to differential MB data after prediction
 *	Side effects:
 *
 *	Date: 970831    Author: Michael Gallant ---mikeg@ee.ubc.ca
 *
 ***********************************************************************/
MB_Structure *MB_Recon_EI( PictImage *base_recon, unsigned char *base_ipol, 
                           MB_Structure *diff, int x, int y,
                           int prediction_type)
{
  MB_Structure *upward = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *recon_data = (MB_Structure *) malloc (sizeof (MB_Structure));
  MotionVector ZERO = {0,0,0,0,0};

  switch (prediction_type)
  {

    case EI_UPWARD_PREDICTION:

      /* Inter 16x16 */
      ReconLumBlock_P (x, y, &ZERO, base_ipol, &diff->lum[0][0], 16, 0);
      ReconChromBlock_P (x, y, 0, 0, base_recon, diff);

      break;

    case EI_INTRA_PREDICTION:
    
      break;

    default:

      fprintf (stderr, "Illegal scalable prediction type in MB_Recon_EP (pred.c)\n");
      exit (-1);

      break;

  }

  memcpy (recon_data, diff, sizeof (MB_Structure));

  free (upward);

  return recon_data;
}
                                   
/***********************************************************************
 *
 *	Name:           Predict_True_B
 *	Description:    Predicts B macroblock in true B frame prediction
 *
 *	Input:	        pointers to current frame, previous recon. previous
 *                      interpolated, next recon, next interpolated,
 *                      pos. in image, MV data, Direct Mode MV data,
 *                      TRB for Direct Mode prediction, and MB prediction type.
 *	Returns:        pointer to differential MB data after prediction
 *	Side effects:
 *
 *	Date: 970831    Author: Michael Gallant ---mikeg@ee.ubc.ca
 *
 ***********************************************************************/

MB_Structure *Predict_True_B (PictImage * curr_image, PictImage * prev_image,
                              unsigned char *prev_ipol, PictImage * next_image, 
                              unsigned char *next_ipol, MB_Structure * pred_macroblock, 
                              int x, int y, MotionVector * MV[7][MBR + 1][MBC + 2], 
                              MotionVector * Direct_MV[MBR][MBC],
                              int TRB, int *prediction_type, int *mode)
{
  int curr[16][16];
  MB_Structure *forward_pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *backward_pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *direct_f_pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *direct_b_pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *direct_pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *bidir_pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *pred_error = (MB_Structure *) malloc (sizeof (MB_Structure));
  MotionVector *forward_MV, *backward_MV, *direct_MV;
  int SADbackw, SADbidir, SADforw, SADdirect, SADmin;
  int dx, dy, xmb, ymb, i, j;
  int mvx, mvy;

  xmb = x / MB_SIZE + 1;
  ymb = y / MB_SIZE + 1;

  forward_MV = MV[0][ymb][xmb];
  backward_MV = MV[5][ymb][xmb];
  direct_MV = Direct_MV[ymb - 1][xmb - 1];

  /* Find MB in current image */
  FindMB (x, y, curr_image->lum, curr);

  /* Forward prediction. */
  FindPred (x, y, forward_MV, prev_ipol, &forward_pred->lum[0][0], 16, 0);

  /* Backward prediction. */
  FindPred (x, y, backward_MV, next_ipol, &backward_pred->lum[0][0], 16, 0);

  /* Direct prediction, forward and backward components. */
  FindForwLumPredDirectTrueB (prev_ipol, x, y, direct_MV, &direct_f_pred->lum[0][0], TRB);
  FindBackwLumPredDirectTrueB (next_ipol, x, y, direct_MV, &direct_b_pred->lum[0][0], TRB);

  for (j = 0; j < MB_SIZE; j++)
  {
    for (i = 0; i < MB_SIZE; i++)
    {
      /* Bidirectional prediction. */
      bidir_pred->lum[j][i] = (forward_pred->lum[j][i] +
                               backward_pred->lum[j][i]) / 2;

      /* Direct prediction. */
      direct_pred->lum[j][i] = (direct_f_pred->lum[j][i] +
                                direct_b_pred->lum[j][i]) / 2;
    }
  }

  SADforw = SAD_MB_integer (&curr[0][0], &forward_pred->lum[0][0], 16, INT_MAX);
  SADforw -= 50;

  SADbackw = SAD_MB_integer (&curr[0][0], &backward_pred->lum[0][0], 16, INT_MAX);

  SADdirect = SAD_MB_integer (&curr[0][0], &direct_pred->lum[0][0], 16, INT_MAX);
  SADdirect -= 100;

  SADbidir = SAD_MB_integer (&curr[0][0], &bidir_pred->lum[0][0], 16, INT_MAX);
  SADbidir += 75;

  /* Prediction direction decision. */
  if ((SADdirect <= SADforw) && (SADdirect <= SADbackw) &&
      (SADdirect <= SADbidir))
  {
    *prediction_type = B_DIRECT_PREDICTION;
    SADmin = SADdirect;
  } 
  else if ((SADforw < SADdirect) && (SADforw <= SADbackw) &&
             (SADforw <= SADbidir))
  {
    *prediction_type = B_FORWARD_PREDICTION;
    SADmin = SADforw;
  } 
  else if ((SADbackw < SADdirect) && (SADbackw < SADforw) &&
             (SADbackw <= SADbidir))
  {
    *prediction_type = B_BACKWARD_PREDICTION;
    SADmin = SADbackw;
  } 
  else
  {
    *prediction_type = B_BIDIRECTIONAL_PREDICTION;
    SADmin = SADbidir;
  }

  *(mode) = ChooseMode( curr_image->lum, x, y, SADmin);
                                
  if (MODE_INTRA == *(mode) || MODE_INTRA_Q == *(mode)) 
  {
    *prediction_type = B_INTRA_PREDICTION;
  }

  switch (*prediction_type)
  {
    case B_FORWARD_PREDICTION:

      /* Translate MV to chrominance-resolution (downsampled). */
      dx = 2 * forward_MV->x + forward_MV->x_half;
      dy = 2 * forward_MV->y + forward_MV->y_half;
      dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
      dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);

      /* Predict B as if P. */
      DoPredChrom_P (x, y, dx, dy, curr_image, prev_image, forward_pred, pred_error);

      for (j = 0; j < MB_SIZE; j++)
      {
        for (i = 0; i < MB_SIZE; i++)
        { 
          pred_error->lum[j][i] = *(curr_image->lum + x + i + (y + j) * pels) - forward_pred->lum[j][i];
          pred_macroblock->lum[j][i] = forward_pred->lum[j][i];
        }
      }
      for (j = 0; j < MB_SIZE >> 1; j++)
      {
        for (i = 0; i < MB_SIZE >> 1; i++)
        {
          pred_macroblock->Cr[j][i] = forward_pred->Cr[j][i];
          pred_macroblock->Cb[j][i] = forward_pred->Cb[j][i];
        }
      }

      forward_MV->Mode = MODE_INTER;
      backward_MV->Mode = MODE_INTRA;

      /* Set backward MV data to 0 for future MV prediction. */
      backward_MV->x = backward_MV->x_half = 0;
      backward_MV->y = backward_MV->y_half = 0;
    
      break;

    case B_BACKWARD_PREDICTION:    

      /* Translate MV to chrominance-resolution (downsampled). */
      dx = 2 * backward_MV->x + backward_MV->x_half;
      dy = 2 * backward_MV->y + backward_MV->y_half;
      dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
      dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);

      /* Predict B as if P. */
      DoPredChrom_P (x, y, dx, dy, curr_image, next_image, backward_pred, pred_error);

      for (j = 0; j < MB_SIZE; j++)
      {
        for (i = 0; i < MB_SIZE; i++)
        {
          pred_error->lum[j][i] = *(curr_image->lum + x + i + (y + j) * pels) - backward_pred->lum[j][i];
          pred_macroblock->lum[j][i] = backward_pred->lum[j][i];
        }
      }

      for (j = 0; j < MB_SIZE >> 1; j++)
      {
        for (i = 0; i < MB_SIZE >> 1; i++)
        {
          pred_macroblock->Cr[j][i] = backward_pred->Cr[j][i];
          pred_macroblock->Cb[j][i] = backward_pred->Cb[j][i];
        }
      }

      forward_MV->Mode = MODE_INTRA;
      backward_MV->Mode = MODE_INTER;

      /* Set forward MV data to 0 for future MV prediction. */
      forward_MV->x = forward_MV->x_half = 0;
      forward_MV->y = forward_MV->y_half = 0;

      break;

    case B_DIRECT_PREDICTION:  

      mvx = 2 * direct_MV->x + direct_MV->x_half;
      mvy = 2 * direct_MV->y + direct_MV->y_half;

      /* Translate MV to chrominance-resolution (downsampled). */
      dx = TRB * mvx / TRP;
      dy = TRB * mvy / TRP;
      dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
      dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);
    
      /* Predict B as if P. */
      DoPredChrom_P (x, y, dx, dy, curr_image, prev_image, direct_f_pred, pred_error);

      /* Translate MV to chrominance-resolution (downsampled). */
      dx = (TRB - TRP) * mvx / TRP;
      dy = (TRB - TRP) * mvy / TRP;
      dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
      dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);

      /* Predict B as if P. */
      DoPredChrom_P (x, y, dx, dy, curr_image, next_image, direct_b_pred, pred_error);

      for (j = 0; j < MB_SIZE; j++)
      {
        for (i = 0; i < MB_SIZE; i++)
        {
          pred_error->lum[j][i] = *(curr_image->lum + x + i + (y + j) * pels) -
                                  direct_pred->lum[j][i];
          pred_macroblock->lum[j][i] = direct_pred->lum[j][i];
        }
      }
      for (j = 0; j < MB_SIZE >> 1; j++)
      {
        for (i = 0; i < MB_SIZE >> 1; i++)
        {
          pred_macroblock->Cr[j][i] = (direct_f_pred->Cr[j][i] +
                                       direct_b_pred->Cr[j][i]) / 2;
          pred_macroblock->Cb[j][i] = (direct_f_pred->Cb[j][i] +
                                       direct_b_pred->Cb[j][i]) / 2;
        }
      }

      forward_MV->Mode = MODE_INTER;
      backward_MV->Mode = MODE_INTER;

      /* Set forward MV data to 0 for future MV prediction. */
      forward_MV->x = forward_MV->x_half = 0;
      forward_MV->y = forward_MV->y_half = 0;

      /* Set backward MV data to 0 for future MV prediction. */
      backward_MV->x = backward_MV->x_half = 0;
      backward_MV->y = backward_MV->y_half = 0;

      break;

    case B_BIDIRECTIONAL_PREDICTION:  

      /* Translate MV to chrominance-resolution (downsampled). */
      dx = 2 * forward_MV->x + forward_MV->x_half;
      dy = 2 * forward_MV->y + forward_MV->y_half;
      dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
      dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);

      /* Predict B as if P. */
      DoPredChrom_P (x, y, dx, dy, curr_image, prev_image, forward_pred, pred_error);

      /* Translate MV to chrominance-resolution (downsampled). */
      dx = 2 * backward_MV->x + backward_MV->x_half;
      dy = 2 * backward_MV->y + backward_MV->y_half;
      dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
      dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);

      /* Predict B as if P. */
      DoPredChrom_P (x, y, dx, dy, curr_image, next_image, backward_pred, pred_error);

      for (j = 0; j < MB_SIZE; j++)
      {
        for (i = 0; i < MB_SIZE; i++)
        {
          pred_error->lum[j][i] = *(curr_image->lum + x + i + (y + j) * pels) -
                                  bidir_pred->lum[j][i];
          pred_macroblock->lum[j][i] = bidir_pred->lum[j][i];
        }
      }
      for (j = 0; j < MB_SIZE >> 1; j++)
      {
        for (i = 0; i < MB_SIZE >> 1; i++)
        {
          pred_macroblock->Cr[j][i] = (forward_pred->Cr[j][i] +
                                       backward_pred->Cr[j][i]) / 2;
          pred_macroblock->Cb[j][i] = (forward_pred->Cb[j][i] +
                                       backward_pred->Cb[j][i]) / 2;
        }
      }

      forward_MV->Mode = MODE_INTER;
      backward_MV->Mode = MODE_INTER;

      break;

    case B_INTRA_PREDICTION:
    
      /* Set forward MV data to 0 for future MV prediction. */
      forward_MV->x = forward_MV->x_half = 0;
      forward_MV->y = forward_MV->y_half = 0;

      /* Set backward MV data to 0 for future MV prediction. */
      backward_MV->x = backward_MV->x_half = 0;
      backward_MV->y = backward_MV->y_half = 0;

      forward_MV->Mode = MODE_INTRA;
      backward_MV->Mode = MODE_INTRA;

      break;

    default:
    
      fprintf (stderr, "Illegal scalable prediction type in MB_Recon_True_B (pred.c)\n");
      exit (-1);

  }

  free (backward_pred);
  free (forward_pred);
  free (bidir_pred);
  free (direct_f_pred);
  free (direct_b_pred);
  free (direct_pred);

  return pred_error;
}

/**********************************************************************
 *
 *	Name:           MB_Recon_True_B
 *	Description:	Reconstructs MB after quantization for true B frames
 *
 *	Input:	        pointers to decoded residual, previous recon, previous
 *                      interpolated, next recon, next interpolated,
 *                      pos. in image, MV data, Direct Mode MV data,
 *                      TRB for Direct Mode prediction, and MB prediction type.
 *	Returns:        pointer to reconstructed MB data after motion compensation
 *	Side effects:   allocates memory to MB_structure
 *
 *	Date: 970831	Author: Michael Gallant --- mikeg@ee.ubc.ca
 *
 ***********************************************************************/

MB_Structure *MB_Recon_True_B (PictImage * prev_image, unsigned char *prev_ipol,
                               MB_Structure *diff, PictImage * next_image,
                               unsigned char *next_ipol, int x_curr, int y_curr,
                               MotionVector * MV[7][MBR + 1][MBC + 2],
                               MotionVector * Direct_MV[MBR][MBC], int TRB,
                               int prediction_type)
{
  MB_Structure *bidir_forward = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *bidir_backward = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *direct_f_pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *direct_b_pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *recon_data = (MB_Structure *) malloc (sizeof (MB_Structure));
  MotionVector *forward_MV, *backward_MV, *direct_MV;
  int dxf, dyf, dxb, dyb;
  int i, j, mvx, mvy;

  direct_MV = Direct_MV[y_curr / MB_SIZE][x_curr / MB_SIZE];

  switch (prediction_type)
  {

    case B_DIRECT_PREDICTION:

      FindForwLumPredDirectTrueB (prev_ipol, x_curr, y_curr, direct_MV, &direct_f_pred->lum[0][0], TRB);
      FindBackwLumPredDirectTrueB (next_ipol, x_curr, y_curr, direct_MV, &direct_b_pred->lum[0][0], TRB);

      mvx = 2 * direct_MV->x + direct_MV->x_half;
      mvy = 2 * direct_MV->y + direct_MV->y_half;

      for (j = 0; j < MB_SIZE; j++)
      {
        for (i = 0; i < MB_SIZE; i++)
        {
          diff->lum[j][i] += (direct_f_pred->lum[j][i] +
                              direct_b_pred->lum[j][i]) / 2;
        }
      }

      /* Translate MV to chrominance-resolution (downsampled). */
      dxf = TRB * mvx / TRP;
      dyf = TRB * mvy / TRP;

      dxf = (dxf % 4 == 0 ? dxf >> 1 : (dxf >> 1) | 1);
      dyf = (dyf % 4 == 0 ? dyf >> 1 : (dyf >> 1) | 1);

      /* Predict B as if P. */
      FindChromBlock_P (x_curr, y_curr, dxf, dyf, prev_image, direct_f_pred);

      /* Translate MV to chrominance-resolution (downsampled). */
      dxb = (TRB - TRP) * mvx / TRP;
      dyb = (TRB - TRP) * mvy / TRP;

      dxb = (dxb % 4 == 0 ? dxb >> 1 : (dxb >> 1) | 1);
      dyb = (dyb % 4 == 0 ? dyb >> 1 : (dyb >> 1) | 1);

      /* Predict B as if P. */
      FindChromBlock_P (x_curr, y_curr, dxb, dyb, next_image, direct_b_pred);

      for (j = 0; j < MB_SIZE >> 1; j++)
      {
        for (i = 0; i < MB_SIZE >> 1; i++)
        {
          diff->Cr[j][i] += (direct_f_pred->Cr[j][i] +
                             direct_b_pred->Cr[j][i]) / 2;
          diff->Cb[j][i] += (direct_f_pred->Cb[j][i] +
                             direct_b_pred->Cb[j][i]) / 2;
        }
      }

      break;

    case B_FORWARD_PREDICTION:

      forward_MV = MV[0][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];

      if (forward_MV->Mode == MODE_INTER || forward_MV->Mode == MODE_INTER_Q)
      {
        /* Inter 16x16 */
        ReconLumBlock_P (x_curr, y_curr, forward_MV, prev_ipol, &diff->lum[0][0], 16, 0);
        dxf = 2 * forward_MV->x + forward_MV->x_half;
        dyf = 2 * forward_MV->y + forward_MV->y_half;
        dxf = (dxf % 4 == 0 ? dxf >> 1 : (dxf >> 1) | 1);
        dyf = (dyf % 4 == 0 ? dyf >> 1 : (dyf >> 1) | 1);
        ReconChromBlock_P (x_curr, y_curr, dxf, dyf, prev_image, diff);
      }
      break;

    case B_BACKWARD_PREDICTION:

      backward_MV = MV[5][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];

      if (backward_MV->Mode == MODE_INTER || backward_MV->Mode == MODE_INTER_Q)
      {
        /* Inter 16x16 */
        ReconLumBlock_P (x_curr, y_curr, backward_MV, next_ipol, &diff->lum[0][0], 16, 0);
        dxb = 2 * backward_MV->x + backward_MV->x_half;
        dyb = 2 * backward_MV->y + backward_MV->y_half;
        dxb = (dxb % 4 == 0 ? dxb >> 1 : (dxb >> 1) | 1);
        dyb = (dyb % 4 == 0 ? dyb >> 1 : (dyb >> 1) | 1);
        ReconChromBlock_P (x_curr, y_curr, dxb, dyb, next_image, diff);
      }
      break;

    case B_BIDIRECTIONAL_PREDICTION:

      forward_MV = MV[0][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];
      backward_MV = MV[5][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];

      if ((forward_MV->Mode == MODE_INTER || forward_MV->Mode == MODE_INTER_Q) ||
          (backward_MV->Mode == MODE_INTER || backward_MV->Mode == MODE_INTER_Q))
      {
        /* Forward prediction. */
        FindPred (x_curr, y_curr, forward_MV, prev_ipol, &bidir_forward->lum[0][0], 16, 0);

        /* Backward prediction. */
        FindPred (x_curr, y_curr, backward_MV, next_ipol, &bidir_backward->lum[0][0], 16, 0);

        for (j = 0; j < MB_SIZE; j++)
        {
          for (i = 0; i < MB_SIZE; i++)
          {
            diff->lum[j][i] += (bidir_forward->lum[j][i] +
                                bidir_backward->lum[j][i]) / 2;
          }
        }

        dxf = 2 * forward_MV->x + forward_MV->x_half;
        dyf = 2 * forward_MV->y + forward_MV->y_half;
        dxf = (dxf % 4 == 0 ? dxf >> 1 : (dxf >> 1) | 1);
        dyf = (dyf % 4 == 0 ? dyf >> 1 : (dyf >> 1) | 1);

        dxb = 2 * backward_MV->x + backward_MV->x_half;
        dyb = 2 * backward_MV->y + backward_MV->y_half;
        dxb = (dxb % 4 == 0 ? dxb >> 1 : (dxb >> 1) | 1);
        dyb = (dyb % 4 == 0 ? dyb >> 1 : (dyb >> 1) | 1);

        FindChromBlock_P (x_curr, y_curr, dxf, dyf, prev_image, bidir_forward);

        FindChromBlock_P (x_curr, y_curr, dxb, dyb, next_image, bidir_backward);

        for (j = 0; j < MB_SIZE / 2; j++)
        {
          for (i = 0; i < MB_SIZE / 2; i++)
          {
            diff->Cr[j][i] += (bidir_forward->Cr[j][i] +
                               bidir_backward->Cr[j][i]) / 2;
            diff->Cb[j][i] += (bidir_forward->Cb[j][i] +
                               bidir_backward->Cb[j][i]) / 2;
          }
        }
      }
      break;

    case B_INTRA_PREDICTION:
    
      break;

    default:

      fprintf (stderr, "Illegal scalable prediction type in MB_Recon_True_B (pred.c)\n");
      exit (-1);
      break;

  }

  memcpy (recon_data, diff, sizeof (MB_Structure));

  free (bidir_forward);
  free (bidir_backward);
  free (direct_f_pred);
  free (direct_b_pred);

  return recon_data;
}

/***********************************************************************
 *
 *	Name:        MB_Recon_B
 *	Description:    Reconstructs the B macroblock in PB-frame
 *                      prediction
 *
 *	Input:	        pointers previous recon. frame, pred. diff.,
 *                      pos. in image, MV-data, reconstructed macroblock
 *                      from image ahead
 *	Returns:        pointer to reconstructed MB data
 *	Side effects:   allocates memory to MB_structure
 *
 *	Date: 950114        Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

MB_Structure *MB_Recon_B (PictImage * prev_image, MB_Structure * diff,
                           unsigned char *prev_ipol, int x, int y,
                           MotionVector * MV[5][MBR + 1][MBC + 2],
                           MB_Structure * recon_P, int TRD, int TRB)
{
  int i, j, k;
  int dx, dy, bdx, bdy, mvx, mvy, xvec, yvec;
  MB_Structure *recon_B = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MotionVector *f[5];

  for (k = 0; k <= 4; k++)
    f[k] = MV[k][y / MB_SIZE + 1][x / MB_SIZE + 1];

  bdx = MV[6][y / MB_SIZE + 1][x / MB_SIZE + 1]->x;
  bdy = MV[6][y / MB_SIZE + 1][x / MB_SIZE + 1]->y;

  if (f[0]->Mode == MODE_INTER4V || f[0]->Mode == MODE_INTER4V_Q)
  {                             /* Mode INTER4V */
    /* Find forward prediction */

    /* Luma */
    FindForwLumPredPB (prev_ipol, x, y, f[1], &pred->lum[0][0], TRD, TRB, bdx, bdy, 8, 0);
    FindForwLumPredPB (prev_ipol, x, y, f[2], &pred->lum[0][8], TRD, TRB, bdx, bdy, 8, 1);
    FindForwLumPredPB (prev_ipol, x, y, f[3], &pred->lum[8][0], TRD, TRB, bdx, bdy, 8, 2);
    FindForwLumPredPB (prev_ipol, x, y, f[4], &pred->lum[8][8], TRD, TRB, bdx, bdy, 8, 3);

    /* chroma vectors are sum of B luma vectors divided and rounded */
    xvec = yvec = 0;
    for (k = 1; k <= 4; k++)
    {
      xvec += TRB * (2 * f[k]->x + f[k]->x_half) / TRD + bdx;
      yvec += TRB * (2 * f[k]->y + f[k]->y_half) / TRD + bdy;
    }

    /* round values according to TABLE 16/H.263 */
    dx = sign (xvec) * (roundtab[abs (xvec) % 16] + (abs (xvec) / 16) * 2);
    dy = sign (yvec) * (roundtab[abs (yvec) % 16] + (abs (yvec) / 16) * 2);

    FindChromBlock_P (x, y, dx, dy, prev_image, pred);

    /* Find bidirectional prediction */
    FindBiDirLumPredPB (&recon_P->lum[0][0], f[1], &pred->lum[0][0],
                        TRD, TRB, bdx, bdy, 0, 0);
    FindBiDirLumPredPB (&recon_P->lum[0][8], f[2], &pred->lum[0][8],
                        TRD, TRB, bdx, bdy, 1, 0);
    FindBiDirLumPredPB (&recon_P->lum[8][0], f[3], &pred->lum[8][0],
                        TRD, TRB, bdx, bdy, 0, 1);
    FindBiDirLumPredPB (&recon_P->lum[8][8], f[4], &pred->lum[8][8],
                        TRD, TRB, bdx, bdy, 1, 1);

    /* chroma vectors are sum of B luma vectors divided and rounded */
    xvec = yvec = 0;
    for (k = 1; k <= 4; k++)
    {
      mvx = 2 * f[k]->x + f[k]->x_half;
      mvy = 2 * f[k]->y + f[k]->y_half;
      xvec += bdx == 0 ? (TRB - TRD) * mvx / TRD : TRB * mvx / TRD + bdx - mvx;
      yvec += bdy == 0 ? (TRB - TRD) * mvy / TRD : TRB * mvy / TRD + bdy - mvy;
    }

    /* round values according to TABLE 16/H.263 */
    dx = sign (xvec) * (roundtab[abs (xvec) % 16] + (abs (xvec) / 16) * 2);
    dy = sign (yvec) * (roundtab[abs (yvec) % 16] + (abs (yvec) / 16) * 2);

    FindBiDirChrPredPB (recon_P, dx, dy, pred);

  } else
  {                             /* Mode INTER or INTER_Q */
    /* Find forward prediction */

    FindForwLumPredPB (prev_ipol, x, y, f[0], &pred->lum[0][0], TRD, TRB,
                       bdx, bdy, 16, 0);

    xvec = 4 * (TRB * (2 * f[0]->x + f[0]->x_half) / TRD + bdx);
    yvec = 4 * (TRB * (2 * f[0]->y + f[0]->y_half) / TRD + bdy);
    /* round values according to TABLE 16/H.263 */
    dx = sign (xvec) * (roundtab[abs (xvec) % 16] + (abs (xvec) / 16) * 2);
    dy = sign (yvec) * (roundtab[abs (yvec) % 16] + (abs (yvec) / 16) * 2);

    FindChromBlock_P (x, y, dx, dy, prev_image, pred);

    /* Find bidirectional prediction */
    FindBiDirLumPredPB (&recon_P->lum[0][0], f[0], &pred->lum[0][0],
                        TRD, TRB, bdx, bdy, 0, 0);
    FindBiDirLumPredPB (&recon_P->lum[0][8], f[0], &pred->lum[0][8],
                        TRD, TRB, bdx, bdy, 1, 0);
    FindBiDirLumPredPB (&recon_P->lum[8][0], f[0], &pred->lum[8][0],
                        TRD, TRB, bdx, bdy, 0, 1);
    FindBiDirLumPredPB (&recon_P->lum[8][8], f[0], &pred->lum[8][8],
                        TRD, TRB, bdx, bdy, 1, 1);

    /* chroma vectors */
    mvx = 2 * f[0]->x + f[0]->x_half;
    xvec = bdx == 0 ? (TRB - TRD) * mvx / TRD : TRB * mvx / TRD + bdx - mvx;
    xvec *= 4;

    mvy = 2 * f[0]->y + f[0]->y_half;
    yvec = bdy == 0 ? (TRB - TRD) * mvy / TRD : TRB * mvy / TRD + bdy - mvy;
    yvec *= 4;

    /* round values according to TABLE 16/H.263 */
    dx = sign (xvec) * (roundtab[abs (xvec) % 16] + (abs (xvec) / 16) * 2);
    dy = sign (yvec) * (roundtab[abs (yvec) % 16] + (abs (yvec) / 16) * 2);

    FindBiDirChrPredPB (recon_P, dx, dy, pred);

  }

  /* Reconstruction */
  for (j = 0; j < MB_SIZE; j++)
    for (i = 0; i < MB_SIZE; i++)
      recon_B->lum[j][i] = pred->lum[j][i] + diff->lum[j][i];

  for (j = 0; j < MB_SIZE >> 1; j++)
    for (i = 0; i < MB_SIZE >> 1; i++)
    {
      recon_B->Cr[j][i] = pred->Cr[j][i] + diff->Cr[j][i];
      recon_B->Cb[j][i] = pred->Cb[j][i] + diff->Cb[j][i];
    }

  free (pred);
  return recon_B;
}

/***********************************************************************
 *
 *	Name:        MB_Reconstruct
 *	Description:    Reconstructs the macroblock by using
 *                  previously calculated prediction array
 *
 *	Input:	        pointers prediction diff and prediction arrays
 *	Returns:        pointer to reconstructed MB data
 *	Side effects:   allocates memory to MB_structure
 *
 *	Date: 97/06/23        Author: Berna Erol <bernae@ee.ubc.ca>
 *
 ***********************************************************************/

MB_Structure *MB_Reconstruct (MB_Structure * diff, MB_Structure * pred)
{
  int i, j;
  MB_Structure *recon = (MB_Structure *) malloc (sizeof (MB_Structure));

  /* Reconstruction */
  for (j = 0; j < MB_SIZE; j++)
    for (i = 0; i < MB_SIZE; i++)
      recon->lum[j][i] = pred->lum[j][i] + diff->lum[j][i];

  for (j = 0; j < MB_SIZE >> 1; j++)
    for (i = 0; i < MB_SIZE >> 1; i++)
    {
      recon->Cr[j][i] = pred->Cr[j][i] + diff->Cr[j][i];
      recon->Cb[j][i] = pred->Cb[j][i] + diff->Cb[j][i];
    }

  return recon;
}

/***********************************************************************
 *
 *	Name:           Predict_EP
 *	Description:    Predicts macroblock in EP frame prediction
 *
 *	Input:	        pointers to current frame, previous recon. previous
 *                      interpolated, base recon, base interpolated,
 *                      pos. in image, MV data, and MB prediction type.
 *	Returns:        pointer to differential MB data after prediction
 *	Side effects:
 *
 *	Date: 971001    Author: Michael Gallant ---mikeg@ee.ubc.ca
 *
 ***********************************************************************/
MB_Structure *Predict_EP( PictImage *curr_image, PictImage *prev_recon, unsigned char *prev_ipol, 
                         PictImage *base_recon, unsigned char *base_ipol, MB_Structure *pred_macroblock, 
                         int x, int y, MotionVector *MV[7][MBR+1][MBC+2], 
                         int *prediction_type, int *mode)
{
  int curr[16][16];
  MB_Structure *forward_pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *bidir_pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *upward_pred = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *pred_error = (MB_Structure *) malloc (sizeof (MB_Structure));
  MotionVector *forward_MV;
  MotionVector ZERO = {0,0,0,0,0};
  int SADbidir, SADforw, SADup, SADmin;
  int dx, dy, xmb, ymb, i, j;

  xmb = x / MB_SIZE + 1;
  ymb = y / MB_SIZE + 1;

  forward_MV = MV[0][ymb][xmb];

  /* Find MB in current image */
  FindMB (x, y, curr_image->lum, curr);

  /* Forward prediction. */
  FindPred (x, y, forward_MV, prev_ipol, &forward_pred->lum[0][0], 16, 0);

  FindPred (x, y, &ZERO, base_ipol, &upward_pred->lum[0][0], 16, 0);
  
  for (j = 0; j < MB_SIZE; j++)
  {
    for (i = 0; i < MB_SIZE; i++)
    {
      /* Bidirectional prediction. */
      bidir_pred->lum[j][i] = (forward_pred->lum[j][i] +
                               upward_pred->lum[j][i]) / 2;
    }
  }

  SADforw = SAD_MB_integer (&curr[0][0], &forward_pred->lum[0][0], 16, INT_MAX);

  SADbidir = SAD_MB_integer (&curr[0][0], &bidir_pred->lum[0][0], 16, INT_MAX);
  SADbidir += 100;

  SADup = SAD_MB_integer (&curr[0][0], &upward_pred->lum[0][0], 16, INT_MAX);
  SADup -= 50;

  /* Prediction direction decision. */
  if ((SADup <= SADforw) && (SADup <= SADbidir))
  {
    *prediction_type = EP_UPWARD_PREDICTION;
    SADmin = SADup;
  } 
  else if ((SADforw < SADup) && (SADforw <= SADbidir))
  {
    *prediction_type = EP_FORWARD_PREDICTION;
    SADmin = SADforw;
  } 
  else
  {
    *prediction_type = EP_BIDIRECTIONAL_PREDICTION;
    SADmin = SADbidir;
  }

  *(mode) = ChooseMode( curr_image->lum, x, y, SADmin);
                                
  if (MODE_INTRA == *(mode) || MODE_INTRA_Q == *(mode)) 
  {
    *prediction_type = EP_INTRA_PREDICTION;
  }

  switch (*prediction_type)
  {
    case EP_FORWARD_PREDICTION:

      /* Translate MV to chrominance-resolution (downsampled). */
      dx = 2 * forward_MV->x + forward_MV->x_half;
      dy = 2 * forward_MV->y + forward_MV->y_half;
      dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
      dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);

      /* Predict B as if P. */
      DoPredChrom_P (x, y, dx, dy, curr_image, prev_recon, forward_pred, pred_error);

      for (j = 0; j < MB_SIZE; j++)
      {
        for (i = 0; i < MB_SIZE; i++)
        { 
          pred_error->lum[j][i] = *(curr_image->lum + x + i + (y + j) * pels) - forward_pred->lum[j][i];
          pred_macroblock->lum[j][i] = forward_pred->lum[j][i];
        }
      }
      for (j = 0; j < MB_SIZE >> 1; j++)
      {
        for (i = 0; i < MB_SIZE >> 1; i++)
        {
          pred_macroblock->Cr[j][i] = forward_pred->Cr[j][i];
          pred_macroblock->Cb[j][i] = forward_pred->Cb[j][i];
        }
      }

      forward_MV->Mode = MODE_INTER;

      break;

    case EP_UPWARD_PREDICTION:    

      /* Predict B as if P. */
      DoPredChrom_P (x, y, 0, 0, curr_image, base_recon, upward_pred, pred_error);

      for (j = 0; j < MB_SIZE; j++)
      {
        for (i = 0; i < MB_SIZE; i++)
        {
          pred_error->lum[j][i] = *(curr_image->lum + x + i + (y + j) * pels) - upward_pred->lum[j][i];
          pred_macroblock->lum[j][i] = upward_pred->lum[j][i];
        }
      }

      for (j = 0; j < MB_SIZE >> 1; j++)
      {
        for (i = 0; i < MB_SIZE >> 1; i++)
        {
          pred_macroblock->Cr[j][i] = upward_pred->Cr[j][i];
          pred_macroblock->Cb[j][i] = upward_pred->Cb[j][i];
        }
      }

      forward_MV->Mode = MODE_INTER;

      /* Set forward MV data to 0 for future MV prediction. */
      forward_MV->x = forward_MV->x_half = 0;
      forward_MV->y = forward_MV->y_half = 0;

      break;

    case EP_BIDIRECTIONAL_PREDICTION:  

      /* Translate MV to chrominance-resolution (downsampled). */
      dx = 2 * forward_MV->x + forward_MV->x_half;
      dy = 2 * forward_MV->y + forward_MV->y_half;
      dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
      dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);

      DoPredChrom_P (x, y, dx, dy, curr_image, prev_recon, forward_pred, pred_error);

      /* Zero MV for upward prediction */
      DoPredChrom_P (x, y, 0, 0, curr_image, base_recon, upward_pred, pred_error);

      for (j = 0; j < MB_SIZE; j++)
      {
        for (i = 0; i < MB_SIZE; i++)
        {
          pred_error->lum[j][i] = *(curr_image->lum + x + i + (y + j) * pels) -
                                  bidir_pred->lum[j][i];
          pred_macroblock->lum[j][i] = bidir_pred->lum[j][i];
        }
      }
      for (j = 0; j < MB_SIZE >> 1; j++)
      {
        for (i = 0; i < MB_SIZE >> 1; i++)
        {
          pred_macroblock->Cr[j][i] = (forward_pred->Cr[j][i] +
                                       upward_pred->Cr[j][i]) / 2;
          pred_macroblock->Cb[j][i] = (forward_pred->Cb[j][i] +
                                       upward_pred->Cb[j][i]) / 2;
        }
      }

      forward_MV->Mode = MODE_INTER;

      break;

    case EP_INTRA_PREDICTION:
    

      /* Set forward MV data to 0 for future MV prediction. */
      forward_MV->x = forward_MV->x_half = 0;
      forward_MV->y = forward_MV->y_half = 0;

      forward_MV->Mode = MODE_INTRA;

      break;

    default:
    
      fprintf (stderr, "Illegal scalable prediction type in MB_Predict_EP (pred.c)\n");
      exit (-1);
  }


  free (forward_pred);
  free (bidir_pred);
  free (upward_pred);

  return pred_error;
}

/**********************************************************************
 *
 *	Name:           MB_Recon_EP
 *	Description:	Reconstructs EP MB
 *
 *	Input:	        pointers to decoded residual, previous recon, previous
 *                      interpolated, base recon, base interpolated,
 *                      pos. in image, MV data, and MB prediction type.
 *	Returns:        pointer to reconstructed MB data after motion compensation
 *	Side effects:   allocates memory to MB_structure
 *
 *	Date: 971101	Author: Michael Gallant --- mikeg@ee.ubc.ca
 *
 ***********************************************************************/
MB_Structure *MB_Recon_EP( PictImage *prev_recon, unsigned char *prev_ipol, 
                           MB_Structure *diff, PictImage *base_recon, 
                           unsigned char *base_ipol, int x_curr, int y_curr,
                           MotionVector *MV[7][MBR+1][MBC+2],
                           int prediction_type)
{
  MB_Structure *forward = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *upward = (MB_Structure *) malloc (sizeof (MB_Structure));
  MB_Structure *recon_data = (MB_Structure *) malloc (sizeof (MB_Structure));
  MotionVector *forward_MV;
  MotionVector ZERO = {0,0,0,0,0};
  int dxf, dyf;
  int i, j;

  switch (prediction_type)
  {

    case EP_FORWARD_PREDICTION:

      forward_MV = MV[0][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];

      if (forward_MV->Mode == MODE_INTER || forward_MV->Mode == MODE_INTER_Q)
      {
        /* Inter 16x16 */
        ReconLumBlock_P (x_curr, y_curr, forward_MV, prev_ipol, &diff->lum[0][0], 16, 0);
        dxf = 2 * forward_MV->x + forward_MV->x_half;
        dyf = 2 * forward_MV->y + forward_MV->y_half;
        dxf = (dxf % 4 == 0 ? dxf >> 1 : (dxf >> 1) | 1);
        dyf = (dyf % 4 == 0 ? dyf >> 1 : (dyf >> 1) | 1);
        ReconChromBlock_P (x_curr, y_curr, dxf, dyf, prev_recon, diff);
      }

      break;

    case EP_UPWARD_PREDICTION:

      /* Inter 16x16 */
      ReconLumBlock_P (x_curr, y_curr, &ZERO, base_ipol, &diff->lum[0][0], 16, 0);
      ReconChromBlock_P (x_curr, y_curr, 0, 0, base_recon, diff);

      break;

    case EP_BIDIRECTIONAL_PREDICTION:

      forward_MV = MV[0][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];

      if ((forward_MV->Mode == MODE_INTER || forward_MV->Mode == MODE_INTER_Q))
      {
        /* Forward prediction. */
        FindPred (x_curr, y_curr, forward_MV, prev_ipol, &forward->lum[0][0], 16, 0);

        /* Backward prediction. */
        FindPred (x_curr, y_curr, &ZERO, base_ipol, &upward->lum[0][0], 16, 0);

        for (j = 0; j < MB_SIZE; j++)
        {
          for (i = 0; i < MB_SIZE; i++)
          {
            diff->lum[j][i] += (forward->lum[j][i] +
                                upward->lum[j][i]) / 2;
          }
        }

        dxf = 2 * forward_MV->x + forward_MV->x_half;
        dyf = 2 * forward_MV->y + forward_MV->y_half;
        dxf = (dxf % 4 == 0 ? dxf >> 1 : (dxf >> 1) | 1);
        dyf = (dyf % 4 == 0 ? dyf >> 1 : (dyf >> 1) | 1);

        FindChromBlock_P (x_curr, y_curr, dxf, dyf, prev_recon, forward);

        FindChromBlock_P (x_curr, y_curr, 0, 0, base_recon, upward);

        for (j = 0; j < MB_SIZE / 2; j++)
        {
          for (i = 0; i < MB_SIZE / 2; i++)
          {
            diff->Cr[j][i] += (forward->Cr[j][i] +
                               upward->Cr[j][i]) / 2;
            diff->Cb[j][i] += (forward->Cb[j][i] +
                               upward->Cb[j][i]) / 2;
          }
        }
      }

      break;

    case EP_INTRA_PREDICTION:
    
      break;

    default:

      fprintf (stderr, "Illegal scalable prediction type in MB_Recon_EP (pred.c)\n");
      exit (-1);

      break;

  }

  memcpy (recon_data, diff, sizeof (MB_Structure));

  free (forward);
  free (upward);

  return recon_data;
}
                           
/**********************************************************************
 *
 *	Name:	       FindForwLumPredPB
 *	Description:   Finds the forward luma  prediction in PB-frame
 *                     pred.
 *
 *	Input:	       pointer to prev. recon. frame, current positon,
 *                     MV structure and pred. structure to fill
 *
 *	Date: 950114        Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void FindForwLumPredPB (unsigned char *prev_ipol, int x_curr, int y_curr,
                         MotionVector * fr, int *pred, int TRD, int TRB,
                         int bdx, int bdy, int bs, int comp)
{
  int i, j;
  int xvec, yvec, lx;

  lx = (mv_outside_frame ? pels + (long_vectors ? 64 : 32) : pels);

  /* Luma */
  xvec = (TRB) * (2 * fr->x + fr->x_half) / TRD + bdx;
  yvec = (TRB) * (2 * fr->y + fr->y_half) / TRD + bdy;

  x_curr += ((comp & 1) << 3);
  y_curr += ((comp & 2) << 2);

  for (j = 0; j < bs; j++)
  {
    for (i = 0; i < bs; i++)
    {
      *(pred + i + j * 16) = *(prev_ipol + (i + x_curr) * 2 + xvec +
                               ((j + y_curr) * 2 + yvec) * lx * 2);
    }
  }

  return;
}


/**********************************************************************
 *
 *	Name:	       FindForwLumPredDirectTrueB
 *	Description:   Finds the forward luma  prediction in true B frame
 *                     pred.
 *
 *	Input:	       pointer to prev. ipol frame, current positon,
 *                     MV structure and pred. structure to fill
 *
 *	Date: 970831   Author: Michael Gallant --- mikeg@ee.ubc.ca
 *
 ***********************************************************************/

void FindForwLumPredDirectTrueB (unsigned char *prev_ipol, int x_curr, int y_curr,
                                  MotionVector * fr, int *pred, int TRB)
{
  int i, j;
  int xvec, yvec, lx;

  lx = (mv_outside_frame ? pels + (long_vectors ? 64 : 32) : pels);

  /* Luma */
  xvec = (TRB) * (2 * fr->x + fr->x_half) / TRP;
  yvec = (TRB) * (2 * fr->y + fr->y_half) / TRP;

  for (j = 0; j < MB_SIZE; j++)
  {
    for (i = 0; i < MB_SIZE; i++)
    {
      *(pred + i + j * 16) = *(prev_ipol + (i + x_curr) * 2 + xvec +
                               ((j + y_curr) * 2 + yvec) * lx * 2);
    }
  }

  return;
}

/**********************************************************************
 *
 *	Name:	       FindBackwLumPredDirectTrueB
 *	Description:   Finds the backward luma  prediction in true B frame
 *                     pred.
 *
 *	Input:	       pointer to next ipol frame, current positon,
 *                     MV structure and pred. structure to fill
 *
 *	Date: 970831   Author: Michael Gallant --- mikeg@ee.ubc.ca
 *
 ***********************************************************************/

void FindBackwLumPredDirectTrueB (unsigned char *next_ipol, int x_curr, int y_curr,
                                   MotionVector * fr, int *pred, int TRB)
{
  int i, j;
  int xvec, yvec, mvx, mvy, lx;

  lx = (mv_outside_frame ? pels + (long_vectors ? 64 : 32) : pels);

  /* Luma */
  mvx = 2 * fr->x + fr->x_half;
  mvy = 2 * fr->y + fr->y_half;

  xvec = (TRB - TRP) * mvx / TRP;
  yvec = (TRB - TRP) * mvy / TRP;

  for (j = 0; j < MB_SIZE; j++)
  {
    for (i = 0; i < MB_SIZE; i++)
    {
      *(pred + i + j * 16) = *(next_ipol + (i + x_curr) * 2 + xvec +
                               ((j + y_curr) * 2 + yvec) * lx * 2);
    }
  }

  return;
}

/**********************************************************************
 *
 *	Name:	       FindBiDirLumPredPB
 *	Description:   Finds the bi-dir. luma prediction in PB-frame
 *                     prediction
 *
 *	Input:	       pointer to future recon. data, current positon,
 *                     MV structure and pred. structure to fill
 *
 *	Date: 950115        Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void FindBiDirLumPredPB (int *recon_P, MotionVector * fr, int *pred, int TRD,
                          int TRB, int bdx, int bdy, int nh, int nv)
{
  int xstart, xstop, ystart, ystop;
  int xvec, yvec, mvx, mvy;

  mvx = 2 * fr->x + fr->x_half;
  mvy = 2 * fr->y + fr->y_half;

  xvec = (bdx == 0 ? (TRB - TRD) * mvx / TRD : TRB * mvx / TRD + bdx - mvx);
  yvec = (bdy == 0 ? (TRB - TRD) * mvy / TRD : TRB * mvy / TRD + bdy - mvy);

  /* Luma */

  FindBiDirLimits (xvec, &xstart, &xstop, nh);
  FindBiDirLimits (yvec, &ystart, &ystop, nv);

  BiDirPredBlock (xstart, xstop, ystart, ystop, xvec, yvec, recon_P, pred, 16);

  return;
}

/**********************************************************************
 *
 *	Name:	       FindBiDirChrPredPB
 *	Description:   Finds the bi-dir. chroma prediction in PB-frame
 *                     prediction
 *
 *	Input:	       pointer to future recon. data, current positon,
 *                     MV structure and pred. structure to fill
 *
 *	Date: 950115        Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void FindBiDirChrPredPB (MB_Structure * recon_P, int dx, int dy,
                          MB_Structure * pred)
{
  int xstart, xstop, ystart, ystop;

  FindBiDirChromaLimits (dx, &xstart, &xstop);
  FindBiDirChromaLimits (dy, &ystart, &ystop);

  BiDirPredBlock (xstart, xstop, ystart, ystop, dx, dy,
                  &recon_P->Cb[0][0], &pred->Cb[0][0], 8);
  BiDirPredBlock (xstart, xstop, ystart, ystop, dx, dy,
                  &recon_P->Cr[0][0], &pred->Cr[0][0], 8);

  return;
}

void FindBiDirLimits (int vec, int *start, int *stop, int nhv)
{

  /* limits taken from C loop in section G5 in H.263 */
  *start = mmax (0, (-vec + 1) / 2 - nhv * 8);
  *stop = mmin (7, 15 - (vec + 1) / 2 - nhv * 8);

  return;

}

void FindBiDirChromaLimits (int vec, int *start, int *stop)
{

  /* limits taken from C loop in section G5 in H.263 */
  *start = mmax (0, (-vec + 1) / 2);
  *stop = mmin (7, 7 - (vec + 1) / 2);

  return;
}


void BiDirPredBlock (int xstart, int xstop, int ystart, int ystop,
                      int xvec, int yvec, int *recon, int *pred, int bl)
{
  int i, j, pel;
  int xint, yint;
  int xh, yh;

  xint = xvec >> 1;
  xh = xvec - 2 * xint;
  yint = yvec >> 1;
  yh = yvec - 2 * yint;

  if (!xh && !yh)
  {
    for (j = ystart; j <= ystop; j++)
    {
      for (i = xstart; i <= xstop; i++)
      {
        pel = *(recon + (j + yint) * bl + i + xint);
        *(pred + j * bl + i) = (mmin (255, mmax (0, pel)) + *(pred + j * bl + i)) >> 1;
      }
    }
  } else if (!xh && yh)
  {
    for (j = ystart; j <= ystop; j++)
    {
      for (i = xstart; i <= xstop; i++)
      {
        pel = (*(recon + (j + yint) * bl + i + xint) +
               *(recon + (j + yint + yh) * bl + i + xint) + 1) >> 1;
        *(pred + j * bl + i) = (pel + *(pred + j * bl + i)) >> 1;
      }
    }
  } else if (xh && !yh)
  {
    for (j = ystart; j <= ystop; j++)
    {
      for (i = xstart; i <= xstop; i++)
      {
        pel = (*(recon + (j + yint) * bl + i + xint) +
               *(recon + (j + yint) * bl + i + xint + xh) + 1) >> 1;
        *(pred + j * bl + i) = (pel + *(pred + j * bl + i)) >> 1;
      }
    }
  } else
  {                             /* xh && yh */
    for (j = ystart; j <= ystop; j++)
    {
      for (i = xstart; i <= xstop; i++)
      {
        pel = (*(recon + (j + yint) * bl + i + xint) +
               *(recon + (j + yint + yh) * bl + i + xint) +
               *(recon + (j + yint) * bl + i + xint + xh) +
               *(recon + (j + yint + yh) * bl + i + xint + xh) + 2) >> 2;
        *(pred + j * bl + i) = (pel + *(pred + j * bl + i)) >> 1;
      }
    }
  }
  return;
}

/**********************************************************************
 *
 *	Name:        DoPredChrom_P
 *	Description:	Does the chrominance prediction for P-frames
 *
 *	Input:        motionvectors for each field,
 *        current position in image,
 *        pointers to current and previos image,
 *        pointer to pred_error array,
 *        (int) field: 1 if field coding
 *
 *	Side effects:	fills chrom-array in pred_error structure
 *
 *	Date: 930211	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void DoPredChrom_P (int x_curr, int y_curr, int dx, int dy,
                    PictImage * curr, PictImage * prev, MB_Structure * prediction,
                    MB_Structure * pred_error)
{
  int m, n;

  int x, y, ofx, ofy, pel, lx;
  int xint, yint;
  int xh, yh;

  lx = (mv_outside_frame ? pels / 2 + (long_vectors ? 32 : 16) : pels / 2);

  x = x_curr >> 1;
  y = y_curr >> 1;

  xint = dx >> 1;
  xh = dx & 1;
  yint = dy >> 1;
  yh = dy & 1;

  if (!xh && !yh)
  {
    for (n = 0; n < 8; n++)
    {
      for (m = 0; m < 8; m++)
      {

        ofx = x + xint + m;
        ofy = y + yint + n;
        pel = *(prev->Cr + ofx + (ofy) * lx);
        prediction->Cr[n][m] = pel;
        pred_error->Cr[n][m] = (int) (*(curr->Cr + x + m + (y + n) * cpels) - pel);

        pel = *(prev->Cb + ofx + (ofy) * lx);
        prediction->Cb[n][m] = pel;
        pred_error->Cb[n][m] = (int) (*(curr->Cb + x + m + (y + n) * cpels) - pel);
      }
    }
  } else if (!xh && yh)
  {
    for (n = 0; n < 8; n++)
    {
      for (m = 0; m < 8; m++)
      {

        ofx = x + xint + m;
        ofy = y + yint + n;
        pel = (*(prev->Cr + ofx + (ofy) * lx) +
               *(prev->Cr + ofx + (ofy + yh) * lx) + 1) >> 1;
        prediction->Cr[n][m] = pel;
        pred_error->Cr[n][m] =
          (int) (*(curr->Cr + x + m + (y + n) * cpels) - pel);

        pel = (*(prev->Cb + ofx + (ofy) * lx) +
               *(prev->Cb + ofx + (ofy + yh) * lx) + 1) >> 1;
        prediction->Cb[n][m] = pel;
        pred_error->Cb[n][m] =
          (int) (*(curr->Cb + x + m + (y + n) * cpels) - pel);

      }
    }
  } else if (xh && !yh)
  {
    for (n = 0; n < 8; n++)
    {
      for (m = 0; m < 8; m++)
      {

        ofx = x + xint + m;
        ofy = y + yint + n;
        pel = (*(prev->Cr + ofx + (ofy) * lx) +
               *(prev->Cr + ofx + xh + (ofy) * lx) + 1) >> 1;
        prediction->Cr[n][m] = pel;
        pred_error->Cr[n][m] =
          (int) (*(curr->Cr + x + m + (y + n) * cpels) - pel);

        pel = (*(prev->Cb + ofx + (ofy) * lx) +
               *(prev->Cb + ofx + xh + (ofy) * lx) + 1) >> 1;
        prediction->Cb[n][m] = pel;
        pred_error->Cb[n][m] =
          (int) (*(curr->Cb + x + m + (y + n) * cpels) - pel);

      }
    }
  } else
  {                             /* xh && yh */
    for (n = 0; n < 8; n++)
    {
      for (m = 0; m < 8; m++)
      {
        ofx = x + xint + m;
        ofy = y + yint + n;
        pel = (*(prev->Cr + ofx + (ofy) * lx) +
               *(prev->Cr + ofx + xh + (ofy) * lx) +
               *(prev->Cr + ofx + (ofy + yh) * lx) +
               *(prev->Cr + ofx + xh + (ofy + yh) * lx) +
               2) >> 2;
        prediction->Cr[n][m] = pel;
        pred_error->Cr[n][m] =
          (int) (*(curr->Cr + x + m + (y + n) * cpels) - pel);

        pel = (*(prev->Cb + ofx + (ofy) * lx) +
               *(prev->Cb + ofx + xh + (ofy) * lx) +
               *(prev->Cb + ofx + (ofy + yh) * lx) +
               *(prev->Cb + ofx + xh + (ofy + yh) * lx) +
               2) >> 2;
        prediction->Cb[n][m] = pel;
        pred_error->Cb[n][m] =
          (int) (*(curr->Cb + x + m + (y + n) * cpels) - pel);

      }
    }
  }
  return;
}

/**********************************************************************
 *
 *	Name:        FindHalfPel
 *	Description:	Find the optimal half pel prediction
 *
 *	Input:        position, vector, array with current data
 *        pointer to previous interpolated luminance,
 *
 *	Returns:
 *
 *	Date: 930126        Author: Karl.Lillevold@nta.no
 *            950208    Mod: Karl.Lillevold@nta.no
 *
 ***********************************************************************/


void FindHalfPel (int x, int y, MotionVector * fr, unsigned char *prev,
                  int *curr, int bs, int comp)
{
  int i, m, n;
  int half_pel;
  int start_x, start_y, stop_x, stop_y, new_x, new_y, lx;
  int min_pos;
  int AE, AE_min;
  Point search[9];

  start_x = -1;
  stop_x = 1;
  start_y = -1;
  stop_y = 1;

  new_x = x + fr->x;
  new_y = y + fr->y;

  new_x += ((comp & 1) << 3);
  new_y += ((comp & 2) << 2);

  lx = (mv_outside_frame ? pels + (long_vectors ? 64 : 32) : pels);

  /* Make sure that no addressing is outside the frame */
  if (!mv_outside_frame)
  {
    if ((new_x) <= 0)
      start_x = 0;
    if ((new_y) <= 0)
      start_y = 0;
    if ((new_x) >= (pels - bs))
      stop_x = 0;
    if ((new_y) >= (lines - bs))
      stop_y = 0;
  }
  search[0].x = 0;
  search[0].y = 0;
  search[1].x = start_x;
  search[1].y = start_y;        /* 1 2 3   */
  search[2].x = 0;
  search[2].y = start_y;        /* 4 0 5   */
  search[3].x = stop_x;
  search[3].y = start_y;        /* 6 7 8   */
  search[4].x = start_x;
  search[4].y = 0;
  search[5].x = stop_x;
  search[5].y = 0;
  search[6].x = start_x;
  search[6].y = stop_y;
  search[7].x = 0;
  search[7].y = stop_y;
  search[8].x = stop_x;
  search[8].y = stop_y;

  AE_min = INT_MAX;
  min_pos = 0;
  for (i = 0; i < 9; i++)
  {
    AE = 0;
    for (n = 0; n < bs; n++)
    {
      for (m = 0; m < bs; m++)
      {
        /* Find absolute error */
        half_pel = *(prev + 2 * new_x + 2 * m + search[i].x +
                     (2 * new_y + 2 * n + search[i].y) * lx * 2);
        AE += abs (half_pel - *(curr + m + n * 16));
      }
    }
    /* if (i == 0 && fr->x == 0 && fr->y == 0 && bs == 16) AE -=
     * PREF_NULL_VEC; */
    if (AE < AE_min)
    {
      AE_min = AE;
      min_pos = i;
    }
  }

  /* Store optimal values */
  fr->min_error = AE_min;
  fr->x_half = search[min_pos].x;
  fr->y_half = search[min_pos].y;

  return;
}

/**********************************************************************
 *
 *	Name:        FindPred
 *	Description:	Find the prediction block
 *
 *	Input:        position, vector, array for prediction
 *        pointer to previous interpolated luminance,
 *
 *	Side effects:	fills array with prediction
 *
 *	Date: 930126        Author: Karl.Lillevold@nta.no
 *            950208    Mod: Karl.Lillevold@nta.no
 *
 ***********************************************************************/


void FindPred (int x, int y, MotionVector * fr, unsigned char *prev,
               int *pred, int bs, int comp)
{
  int m, n;
  int new_x, new_y;
  int lx;

  lx = (mv_outside_frame ? pels + (long_vectors ? 64 : 32) : pels);

  new_x = x + fr->x;
  new_y = y + fr->y;

  new_x += ((comp & 1) << 3);
  new_y += ((comp & 2) << 2);


  /* Fill pred. data */
  for (n = 0; n < bs; n++)
  {
    for (m = 0; m < bs; m++)
    {
      /* Find interpolated pixel-value */
      *(pred + m + n * 16) = *(prev + (new_x + m) * 2 + fr->x_half +
                               ((new_y + n) * 2 + fr->y_half) * lx * 2);
    }
  }
  return;
}

/**********************************************************************
 *
 *	Name:        FindPredOBMC
 *	Description:	Find the OBMC prediction block
 *
 *	Input:        position, vector, array for prediction
 *        pointer to previous interpolated luminance,
 *
 *	Returns:
 *	Side effects:	fills array with prediction
 *
 *	Date: 950209        Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/


void FindPredOBMC (int x, int y, MotionVector * MV[6][MBR + 1][MBC + 2],
                   unsigned char *prev, int *pred, int comp, int PB)
{
  int m, n;
  int pc, pt, pb, pr, pl;
  int nxc, nxt, nxb, nxr, nxl;
  int nyc, nyt, nyb, nyr, nyl;
  int xit, xib, xir, xil;
  int yit, yib, yir, yil;
  int vect, vecb, vecr, vecl;
  int c8, t8, l8, r8;
  int ti8, li8, ri8;
  int xmb, ymb, lx;
  MotionVector *fc, *ft, *fb, *fr, *fl;

  int Mc[8][8] = {
    {4, 5, 5, 5, 5, 5, 5, 4},
    {5, 5, 5, 5, 5, 5, 5, 5},
    {5, 5, 6, 6, 6, 6, 5, 5},
    {5, 5, 6, 6, 6, 6, 5, 5},
    {5, 5, 6, 6, 6, 6, 5, 5},
    {5, 5, 6, 6, 6, 6, 5, 5},
    {5, 5, 5, 5, 5, 5, 5, 5},
    {4, 5, 5, 5, 5, 5, 5, 4},
  };
  int Mt[8][8] = {
    {2, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 2, 2, 2, 2, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
  };
  int Mb[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 2, 2, 2, 2, 1, 1},
    {2, 2, 2, 2, 2, 2, 2, 2},
  };
  int Mr[8][8] = {
    {0, 0, 0, 0, 1, 1, 1, 2},
    {0, 0, 0, 0, 1, 1, 2, 2},
    {0, 0, 0, 0, 1, 1, 2, 2},
    {0, 0, 0, 0, 1, 1, 2, 2},
    {0, 0, 0, 0, 1, 1, 2, 2},
    {0, 0, 0, 0, 1, 1, 2, 2},
    {0, 0, 0, 0, 1, 1, 2, 2},
    {0, 0, 0, 0, 1, 1, 1, 2},
  };
  int Ml[8][8] = {
    {2, 1, 1, 1, 0, 0, 0, 0},
    {2, 2, 1, 1, 0, 0, 0, 0},
    {2, 2, 1, 1, 0, 0, 0, 0},
    {2, 2, 1, 1, 0, 0, 0, 0},
    {2, 2, 1, 1, 0, 0, 0, 0},
    {2, 2, 1, 1, 0, 0, 0, 0},
    {2, 2, 1, 1, 0, 0, 0, 0},
    {2, 1, 1, 1, 0, 0, 0, 0},
  };

  xmb = x / MB_SIZE + 1;
  ymb = y / MB_SIZE + 1;

  lx = (mv_outside_frame ? pels + (long_vectors ? 64 : 32) : pels);

  c8 = (MV[0][ymb][xmb]->Mode == MODE_INTER4V ? 1 : 0);
  c8 = (MV[0][ymb][xmb]->Mode == MODE_INTER4V_Q ? 1 : c8);

  t8 = (MV[0][ymb - 1][xmb]->Mode == MODE_INTER4V ? 1 : 0);
  t8 = (MV[0][ymb - 1][xmb]->Mode == MODE_INTER4V_Q ? 1 : t8);
  ti8 = (MV[0][ymb - 1][xmb]->Mode == MODE_INTRA ? 1 : 0);
  ti8 = (MV[0][ymb - 1][xmb]->Mode == MODE_INTRA_Q ? 1 : ti8);

  l8 = (MV[0][ymb][xmb - 1]->Mode == MODE_INTER4V ? 1 : 0);
  l8 = (MV[0][ymb][xmb - 1]->Mode == MODE_INTER4V_Q ? 1 : l8);
  li8 = (MV[0][ymb][xmb - 1]->Mode == MODE_INTRA ? 1 : 0);
  li8 = (MV[0][ymb][xmb - 1]->Mode == MODE_INTRA_Q ? 1 : li8);

  r8 = (MV[0][ymb][xmb + 1]->Mode == MODE_INTER4V ? 1 : 0);
  r8 = (MV[0][ymb][xmb + 1]->Mode == MODE_INTER4V_Q ? 1 : r8);
  ri8 = (MV[0][ymb][xmb + 1]->Mode == MODE_INTRA ? 1 : 0);
  ri8 = (MV[0][ymb][xmb + 1]->Mode == MODE_INTRA_Q ? 1 : ri8);

  if (PB)
  {
    ti8 = li8 = ri8 = 0;
  }
  switch (comp + 1)
  {

    case 1:
      vect = (ti8 ? (c8 ? 1 : 0) : (t8 ? 3 : 0));
      yit = (ti8 ? ymb : ymb - 1);
      xit = xmb;

      vecb = (c8 ? 3 : 0);
      yib = ymb;
      xib = xmb;

      vecl = (li8 ? (c8 ? 1 : 0) : (l8 ? 2 : 0));
      yil = ymb;
      xil = (li8 ? xmb : xmb - 1);

      vecr = (c8 ? 2 : 0);
      yir = ymb;
      xir = xmb;

      /* edge handling */
      if (ymb == 1)
      {
        yit = ymb;
        vect = (c8 ? 1 : 0);
      }
      if (xmb == 1)
      {
        xil = xmb;
        vecl = (c8 ? 1 : 0);
      }
      break;

    case 2:
      vect = (ti8 ? (c8 ? 2 : 0) : (t8 ? 4 : 0));
      yit = (ti8 ? ymb : ymb - 1);
      xit = xmb;

      vecb = (c8 ? 4 : 0);
      yib = ymb;
      xib = xmb;
      vecl = (c8 ? 1 : 0);
      yil = ymb;
      xil = xmb;

      vecr = (ri8 ? (c8 ? 2 : 0) : (r8 ? 1 : 0));
      yir = ymb;
      xir = (ri8 ? xmb : xmb + 1);

      /* edge handling */
      if (ymb == 1)
      {
        yit = ymb;
        vect = (c8 ? 2 : 0);
      }
      if (xmb == pels / 16)
      {
        xir = xmb;
        vecr = (c8 ? 2 : 0);
      }
      break;

    case 3:
      vect = (c8 ? 1 : 0);
      yit = ymb;
      xit = xmb;
      vecb = (c8 ? 3 : 0);
      yib = ymb;
      xib = xmb;

      vecl = (li8 ? (c8 ? 3 : 0) : (l8 ? 4 : 0));
      yil = ymb;
      xil = (li8 ? xmb : xmb - 1);

      vecr = (c8 ? 4 : 0);
      yir = ymb;
      xir = xmb;

      /* edge handling */
      if (xmb == 1)
      {
        xil = xmb;
        vecl = (c8 ? 3 : 0);
      }
      break;

    case 4:
      vect = (c8 ? 2 : 0);
      yit = ymb;
      xit = xmb;
      vecb = (c8 ? 4 : 0);
      yib = ymb;
      xib = xmb;
      vecl = (c8 ? 3 : 0);
      yil = ymb;
      xil = xmb;

      vecr = (ri8 ? (c8 ? 4 : 0) : (r8 ? 3 : 0));
      yir = ymb;
      xir = (ri8 ? xmb : xmb + 1);

      /* edge handling */
      if (xmb == pels / 16)
      {
        xir = xmb;
        vecr = (c8 ? 4 : 0);
      }
      break;

    default:
      fprintf (stderr, "Illegal block number in FindPredOBMC (pred.c)\n");
      exit (-1);
      break;
  }

  fc = MV[c8 ? comp + 1 : 0][ymb][xmb];

  ft = MV[vect][yit][xit];
  fb = MV[vecb][yib][xib];
  fr = MV[vecr][yir][xir];
  fl = MV[vecl][yil][xil];

  nxc = 2 * x + ((comp & 1) << 4);
  nyc = 2 * y + ((comp & 2) << 3);
  nxt = nxb = nxr = nxl = nxc;
  nyt = nyb = nyr = nyl = nyc;

  nxc += 2 * fc->x + fc->x_half;
  nyc += 2 * fc->y + fc->y_half;
  nxt += 2 * ft->x + ft->x_half;
  nyt += 2 * ft->y + ft->y_half;
  nxb += 2 * fb->x + fb->x_half;
  nyb += 2 * fb->y + fb->y_half;
  nxr += 2 * fr->x + fr->x_half;
  nyr += 2 * fr->y + fr->y_half;
  nxl += 2 * fl->x + fl->x_half;
  nyl += 2 * fl->y + fl->y_half;

  /* Fill pred. data */
  for (n = 0; n < 8; n++)
  {
    for (m = 0; m < 8; m++)
    {

      /* Find interpolated pixel-value */
      pc = *(prev + nxc + 2 * m + (nyc + 2 * n) * lx * 2) * Mc[n][m];
      pt = *(prev + nxt + 2 * m + (nyt + 2 * n) * lx * 2) * Mt[n][m];
      pb = *(prev + nxb + 2 * m + (nyb + 2 * n) * lx * 2) * Mb[n][m];
      pr = *(prev + nxr + 2 * m + (nyr + 2 * n) * lx * 2) * Mr[n][m];
      pl = *(prev + nxl + 2 * m + (nyl + 2 * n) * lx * 2) * Ml[n][m];

      /* pc = *(prev + nxc + 2*m + (nyc + 2*n)*lx*2) * 8; pt = *(prev +
       * nxt + 2*m + (nyt + 2*n)*lx*2) * 0;; pb = *(prev + nxb + 2*m +
       * (nyb + 2*n)*lx*2) * 0; pr = *(prev + nxr + 2*m + (nyr +
       * 2*n)*lx*2) * 0; pl = *(prev + nxl + 2*m + (nyl + 2*n)*lx*2) * 0;$ */

      *(pred + m + n * 16) = (pc + pt + pb + pr + pl + 4) >> 3;
    }
  }
  return;
}


/**********************************************************************
 *
 *	Name:        ReconMacroblock_P
 *	Description:	Reconstructs MB after quantization for P_images
 *
 *	Input:        pointers to current and previous image,
 *        current slice and mb, and which mode
 *        of prediction has been used
 *	Returns:
 *	Side effects:
 *
 *	Date: 930122	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

MB_Structure *MB_Recon_P (PictImage * prev_image, unsigned char *prev_ipol,
                          MB_Structure * diff, int x_curr, int y_curr,
                          MotionVector * MV[6][MBR + 1][MBC + 2], int PB)
{
  MB_Structure *recon_data = (MB_Structure *) malloc (sizeof (MB_Structure));
  MotionVector *fr0, *fr1, *fr2, *fr3, *fr4;
  int pred[16][16];
  int dx, dy, sum;
  int i, j;

  fr0 = MV[0][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];
  /* change advanced to overlapping_MC since * 4 MV can be used without
   * OBMC in        * deblocking filter mode                  */
  if (overlapping_MC)
  {
    if (fr0->Mode == MODE_INTER || fr0->Mode == MODE_INTER_Q)
    {
      FindPredOBMC (x_curr, y_curr, MV, prev_ipol, &pred[0][0], 0, PB);
      FindPredOBMC (x_curr, y_curr, MV, prev_ipol, &pred[0][8], 1, PB);
      FindPredOBMC (x_curr, y_curr, MV, prev_ipol, &pred[8][0], 2, PB);
      FindPredOBMC (x_curr, y_curr, MV, prev_ipol, &pred[8][8], 3, PB);
      for (j = 0; j < MB_SIZE; j++)
        for (i = 0; i < MB_SIZE; i++)
          diff->lum[j][i] += pred[j][i];
      dx = 2 * fr0->x + fr0->x_half;
      dy = 2 * fr0->y + fr0->y_half;
      dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
      dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);
      ReconChromBlock_P (x_curr, y_curr, dx, dy, prev_image, diff);
    } else if (fr0->Mode == MODE_INTER4V || fr0->Mode == MODE_INTER4V_Q)
    {                           /* Inter 8x8 */

      FindPredOBMC (x_curr, y_curr, MV, prev_ipol, &pred[0][0], 0, PB);
      FindPredOBMC (x_curr, y_curr, MV, prev_ipol, &pred[0][8], 1, PB);
      FindPredOBMC (x_curr, y_curr, MV, prev_ipol, &pred[8][0], 2, PB);
      FindPredOBMC (x_curr, y_curr, MV, prev_ipol, &pred[8][8], 3, PB);
      for (j = 0; j < MB_SIZE; j++)
        for (i = 0; i < MB_SIZE; i++)
          diff->lum[j][i] += pred[j][i];

      fr1 = MV[1][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];
      fr2 = MV[2][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];
      fr3 = MV[3][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];
      fr4 = MV[4][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];

      sum = 2 * fr1->x + fr1->x_half + 2 * fr2->x + fr2->x_half +
        2 * fr3->x + fr3->x_half + 2 * fr4->x + fr4->x_half;
      dx = sign (sum) * (roundtab[abs (sum) % 16] + (abs (sum) / 16) * 2);

      sum = 2 * fr1->y + fr1->y_half + 2 * fr2->y + fr2->y_half +
        2 * fr3->y + fr3->y_half + 2 * fr4->y + fr4->y_half;
      dy = sign (sum) * (roundtab[abs (sum) % 16] + (abs (sum) / 16) * 2);

      ReconChromBlock_P (x_curr, y_curr, dx, dy, prev_image, diff);
    }
  } else
  {
    if (fr0->Mode == MODE_INTER || fr0->Mode == MODE_INTER_Q)
    {
      /* Inter 16x16 */
      ReconLumBlock_P (x_curr, y_curr, fr0, prev_ipol, &diff->lum[0][0], 16, 0);

      dx = 2 * fr0->x + fr0->x_half;
      dy = 2 * fr0->y + fr0->y_half;
      dx = (dx % 4 == 0 ? dx >> 1 : (dx >> 1) | 1);
      dy = (dy % 4 == 0 ? dy >> 1 : (dy >> 1) | 1);
      ReconChromBlock_P (x_curr, y_curr, dx, dy, prev_image, diff);
    }
    if (fr0->Mode == MODE_INTER4V || fr0->Mode == MODE_INTER4V_Q)
    {
      fr1 = MV[1][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];
      fr2 = MV[2][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];
      fr3 = MV[3][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];
      fr4 = MV[4][y_curr / MB_SIZE + 1][x_curr / MB_SIZE + 1];
      FindPred (x_curr, y_curr, fr1, prev_ipol, &pred[0][0], 8, 0);
      FindPred (x_curr, y_curr, fr2, prev_ipol, &pred[0][8], 8, 1);
      FindPred (x_curr, y_curr, fr3, prev_ipol, &pred[8][0], 8, 2);
      FindPred (x_curr, y_curr, fr4, prev_ipol, &pred[8][8], 8, 3);
      for (j = 0; j < MB_SIZE; j++)
        for (i = 0; i < MB_SIZE; i++)
          diff->lum[j][i] += pred[j][i];

      sum = 2 * fr1->x + fr1->x_half + 2 * fr2->x + fr2->x_half +
        2 * fr3->x + fr3->x_half + 2 * fr4->x + fr4->x_half;
      dx = sign (sum) * (roundtab[abs (sum) % 16] + (abs (sum) / 16) * 2);

      sum = 2 * fr1->y + fr1->y_half + 2 * fr2->y + fr2->y_half +
        2 * fr3->y + fr3->y_half + 2 * fr4->y + fr4->y_half;
      dy = sign (sum) * (roundtab[abs (sum) % 16] + (abs (sum) / 16) * 2);

      ReconChromBlock_P (x_curr, y_curr, dx, dy, prev_image, diff);
    }
  }

  memcpy (recon_data, diff, sizeof (MB_Structure));

  return recon_data;
}

/**********************************************************************
 *
 *	Name:        ReconLumBlock_P
 *	Description:	Reconstructs one block of luminance data
 *
 *	Input:        position, vector-data, previous image, data-block
 *	Returns:
 *	Side effects:	reconstructs data-block
 *
 *	Date: 950210	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void ReconLumBlock_P (int x, int y, MotionVector * fr,
                       unsigned char *prev, int *data, int bs, int comp)
{
  int m, n;
  int x1, y1, lx;

  lx = (mv_outside_frame ? pels + (long_vectors ? 64 : 32) : pels);

  x1 = 2 * (x + fr->x) + fr->x_half;
  y1 = 2 * (y + fr->y) + fr->y_half;

  x1 += ((comp & 1) << 4);
  y1 += ((comp & 2) << 3);

  for (n = 0; n < bs; n++)
  {
    for (m = 0; m < bs; m++)
    {
      *(data + m + n * 16) += (int) (*(prev + x1 + 2 * m + (y1 + 2 * n) * 2 * lx));
    }
  }

  return;
}

/**********************************************************************
 *
 *	Name:        ReconChromBlock_P
 *	Description:        Reconstructs chrominance of one block in P frame
 *
 *	Input:        position, vector-data, previous image, data-block
 *	Returns:
 *	Side effects:	reconstructs data-block
 *
 *	Date: 930203	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void ReconChromBlock_P (int x_curr, int y_curr, int dx, int dy,
                         PictImage * prev, MB_Structure * data)

{
  int m, n;

  int x, y, ofx, ofy, pel, lx;
  int xint, yint;
  int xh, yh;

  lx = (mv_outside_frame ? pels / 2 + (long_vectors ? 32 : 16) : pels / 2);

  x = x_curr >> 1;
  y = y_curr >> 1;

  xint = dx >> 1;
  xh = dx & 1;
  yint = dy >> 1;
  yh = dy & 1;

  if (!xh && !yh)
  {
    for (n = 0; n < 8; n++)
    {
      for (m = 0; m < 8; m++)
      {

        ofx = x + xint + m;
        ofy = y + yint + n;
        pel = *(prev->Cr + ofx + (ofy) * lx);
        data->Cr[n][m] += pel;

        pel = *(prev->Cb + ofx + (ofy) * lx);
        data->Cb[n][m] += pel;
      }
    }
  } else if (!xh && yh)
  {
    for (n = 0; n < 8; n++)
    {
      for (m = 0; m < 8; m++)
      {

        ofx = x + xint + m;
        ofy = y + yint + n;
        pel = (*(prev->Cr + ofx + (ofy) * lx) +
               *(prev->Cr + ofx + (ofy + yh) * lx) + 1) >> 1;

        data->Cr[n][m] += pel;

        pel = (*(prev->Cb + ofx + (ofy) * lx) +
               *(prev->Cb + ofx + (ofy + yh) * lx) + 1) >> 1;

        data->Cb[n][m] += pel;

      }
    }
  } else if (xh && !yh)
  {
    for (n = 0; n < 8; n++)
    {
      for (m = 0; m < 8; m++)
      {

        ofx = x + xint + m;
        ofy = y + yint + n;
        pel = (*(prev->Cr + ofx + (ofy) * lx) +
               *(prev->Cr + ofx + xh + (ofy) * lx) + 1) >> 1;

        data->Cr[n][m] += pel;

        pel = (*(prev->Cb + ofx + (ofy) * lx) +
               *(prev->Cb + ofx + xh + (ofy) * lx) + 1) >> 1;

        data->Cb[n][m] += pel;

      }
    }
  } else
  {                             /* xh && yh */
    for (n = 0; n < 8; n++)
    {
      for (m = 0; m < 8; m++)
      {
        ofx = x + xint + m;
        ofy = y + yint + n;
        pel = (*(prev->Cr + ofx + (ofy) * lx) +
               *(prev->Cr + ofx + xh + (ofy) * lx) +
               *(prev->Cr + ofx + (ofy + yh) * lx) +
               *(prev->Cr + ofx + xh + (ofy + yh) * lx) +
               2) >> 2;

        data->Cr[n][m] += pel;

        pel = (*(prev->Cb + ofx + (ofy) * lx) +
               *(prev->Cb + ofx + xh + (ofy) * lx) +
               *(prev->Cb + ofx + (ofy + yh) * lx) +
               *(prev->Cb + ofx + xh + (ofy + yh) * lx) +
               2) >> 2;

        data->Cb[n][m] += pel;

      }
    }
  }
  return;
}

/**********************************************************************
 *
 *	Name:        FindChromBlock_P
 *	Description:        Finds chrominance of one block in P frame
 *
 *	Input:        position, vector-data, previous image, data-block
 *
 *	Date: 950222	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void FindChromBlock_P (int x_curr, int y_curr, int dx, int dy,
                        PictImage * prev, MB_Structure * data)

{
  int m, n;

  int x, y, ofx, ofy, pel, lx;
  int xint, yint;
  int xh, yh;

  lx = (mv_outside_frame ? pels / 2 + (long_vectors ? 32 : 16) : pels / 2);

  x = x_curr >> 1;
  y = y_curr >> 1;

  xint = dx >> 1;
  xh = dx & 1;
  yint = dy >> 1;
  yh = dy & 1;

  if (!xh && !yh)
  {
    for (n = 0; n < 8; n++)
    {
      for (m = 0; m < 8; m++)
      {

        ofx = x + xint + m;
        ofy = y + yint + n;
        pel = *(prev->Cr + ofx + (ofy) * lx);
        data->Cr[n][m] = pel;

        pel = *(prev->Cb + ofx + (ofy) * lx);
        data->Cb[n][m] = pel;
      }
    }
  } else if (!xh && yh)
  {
    for (n = 0; n < 8; n++)
    {
      for (m = 0; m < 8; m++)
      {

        ofx = x + xint + m;
        ofy = y + yint + n;
        pel = (*(prev->Cr + ofx + (ofy) * lx) +
               *(prev->Cr + ofx + (ofy + yh) * lx) + 1) >> 1;

        data->Cr[n][m] = pel;

        pel = (*(prev->Cb + ofx + (ofy) * lx) +
               *(prev->Cb + ofx + (ofy + yh) * lx) + 1) >> 1;

        data->Cb[n][m] = pel;

      }
    }
  } else if (xh && !yh)
  {
    for (n = 0; n < 8; n++)
    {
      for (m = 0; m < 8; m++)
      {

        ofx = x + xint + m;
        ofy = y + yint + n;
        pel = (*(prev->Cr + ofx + (ofy) * lx) +
               *(prev->Cr + ofx + xh + (ofy) * lx) + 1) >> 1;

        data->Cr[n][m] = pel;

        pel = (*(prev->Cb + ofx + (ofy) * lx) +
               *(prev->Cb + ofx + xh + (ofy) * lx) + 1) >> 1;

        data->Cb[n][m] = pel;

      }
    }
  } else
  {                             /* xh && yh */
    for (n = 0; n < 8; n++)
    {
      for (m = 0; m < 8; m++)
      {
        ofx = x + xint + m;
        ofy = y + yint + n;
        pel = (*(prev->Cr + ofx + (ofy) * lx) +
               *(prev->Cr + ofx + xh + (ofy) * lx) +
               *(prev->Cr + ofx + (ofy + yh) * lx) +
               *(prev->Cr + ofx + xh + (ofy + yh) * lx) +
               2) >> 2;

        data->Cr[n][m] = pel;

        pel = (*(prev->Cb + ofx + (ofy) * lx) +
               *(prev->Cb + ofx + xh + (ofy) * lx) +
               *(prev->Cb + ofx + (ofy + yh) * lx) +
               *(prev->Cb + ofx + xh + (ofy + yh) * lx) +
               2) >> 2;

        data->Cb[n][m] = pel;

      }
    }
  }
  return;
}



/**********************************************************************
 *
 *	Name:        ChooseMode
 *	Description:    chooses coding mode
 *
 *	Input:	        pointer to original fram, min_error from
 *                      integer pel search, DQUANT
 *	Returns:        1 for Inter, 0 for Intra
 *
 *	Date: 941130	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

int ChooseMode (unsigned char *curr, int x_pos, int y_pos, int min_SAD)
{
  int i, j;
  int MB_mean = 0, A = 0;
  int y_off;

  for (j = 0; j < MB_SIZE; j++)
  {
    y_off = (y_pos + j) * pels;
    for (i = 0; i < MB_SIZE; i++)
    {
      MB_mean += *(curr + x_pos + i + y_off);
    }
  }
  MB_mean /= (MB_SIZE * MB_SIZE);
  for (j = 0; j < MB_SIZE; j++)
  {
    y_off = (y_pos + j) * pels;
    for (i = 0; i < MB_SIZE; i++)
    {
      A += abs (*(curr + x_pos + i + y_off) - MB_mean);
    }
  }

  if (A < (min_SAD - 500))
    return MODE_INTRA;
  else
    return MODE_INTER;
}

int ModifyMode (int Mode, int dquant, int EPTYPE)
{

  if (Mode == MODE_INTRA)
  {
    if (dquant != 0)
      return MODE_INTRA_Q;
    else
      return MODE_INTRA;
  } else if (Mode == MODE_INTER)
  {
    if (dquant != 0)
      return MODE_INTER_Q;
    else
      return MODE_INTER;
  } else if (Mode == MODE_INTER4V)
  {
    if (dquant != 0 && EPTYPE)
      return MODE_INTER4V_Q;
    else
      return MODE_INTER4V;
  } else
    return Mode;
}
