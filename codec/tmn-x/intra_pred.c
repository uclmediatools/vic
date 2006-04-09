/************************************************************************
 *
 *  intra_pred.c, Intra Prediction routines of tmn (TMN encoder)
 *  for Advanced Intra Coding Mode
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

//SV-XXX: defined UNUSED() macro for unused variables
#ifndef UNUSED
#define UNUSED(x) (x) = (x)
#endif


/**********************************************************************
 *
 *	Name:           Choose_AC_DC_Pred
 *	Description:    Intra Prediction in Advanced Intra Coding
 *
 *	Input:	        qcoeff, store_coeff, position of MB
 *
 *	Side effects:   change qcoeff to predicted qcoeff
 *
 *      Return:         Intra_Mode (used for the scanning order)
 *
 *	Date:970717     Guy Cote <guyc@ee.ubc.ca>
 *
 ***********************************************************************/

int Choose_Intra_Mode (int *pcoeff, int *store_coeff, int xpos, int ypos, int newgob)
{
  int E0[6], E1[8][6], E2[8][6], A[8][6], B[8][6];
  int i, j;
  int SAD0 = 0, SAD1 = 0, SAD2 = 0;
  int Intra_Mode;

  for (i = 0; i < 6; i++)
  {

    if (xpos == 0 && ypos == 0)
    {                           
      /* top left corner */
      ((i == 2 || i == 3)) ? sed_fill_A (A, store_coeff, xpos, ypos, i - 2, i) : sed_fill_null (A, i);
      ((i == 1 || i == 3)) ? sed_fill_B (B, store_coeff, xpos, ypos, i - 1, i) : sed_fill_null (B, i);

    } else
    {
      if (xpos == 0)
      {
        /* left edge of the picture */
        (i == 2 || i == 3) ? sed_fill_A (A, store_coeff, xpos, ypos, i - 2, i) :
        ((i == 0 || i == 1) && !(newgob)) ? sed_fill_A (A, store_coeff, xpos, ypos - 1, i + 2, i) :
        ((i == 4 || i == 5) && !(newgob)) ? sed_fill_A (A, store_coeff, xpos, ypos - 1, i, i) :
        sed_fill_null (A, i);

        (i == 1 || i == 3) ? sed_fill_B (B, store_coeff, xpos, ypos, i - 1, i) : sed_fill_null (B, i);


      } else
      {
        if (ypos == 0)
        { 
          /* top border of picture */
          (i == 2 || i == 3) ? sed_fill_A (A, store_coeff, xpos, ypos, i - 2, i) : sed_fill_null (A, i);
          ((i == 4 || i == 5) && coded_map[ypos + 1][xpos] == 2) ?
          sed_fill_B (B, store_coeff, xpos - 1, ypos, i, i) :
          (i == 1 || i == 3) ? sed_fill_B (B, store_coeff, xpos, ypos, i - 1, i) :
          ((i == 0 || i == 2) && coded_map[ypos + 1][xpos] == 2) ?
          sed_fill_B (B, store_coeff, xpos - 1, ypos, i + 1, i) : sed_fill_null (B, i);


        } else
        { 
          /* anywhere else in the picture, do not
           * cross GOB boundary */
          (i == 2 || i == 3) ? sed_fill_A (A, store_coeff, xpos, ypos, i - 2, i) :
          ((i == 0 || i == 1) && !(newgob) && coded_map[ypos][xpos + 1] == 2) ?
          sed_fill_A (A, store_coeff, xpos, ypos - 1, i + 2, i) :
          ((i == 4 || i == 5) && !(newgob) && coded_map[ypos][xpos + 1] == 2) ?
          sed_fill_A (A, store_coeff, xpos, ypos - 1, i, i) :
          sed_fill_null (A, i);
          ((i == 4 || i == 5) && coded_map[ypos + 1][xpos] == 2) ?
          sed_fill_B (B, store_coeff, xpos - 1, ypos, i, i) :
          (i == 1 || i == 3) ? sed_fill_B (B, store_coeff, xpos, ypos, i - 1, i) :
          ((i == 0 || i == 2) && coded_map[ypos + 1][xpos] == 2) ?
          sed_fill_B (B, store_coeff, xpos - 1, ypos, i + 1, i) : sed_fill_null (B, i);


        }
      }
    }

    /* DC prediction */

    E0[i] = store_coeff[(xpos + ypos * sed_pels / MB_SIZE) * 384 + i * 64]
      - (A[0][i] + B[0][i] + 1) / 2;

    for (j = 0; j < 8; j++)
    {
      /* DC/AC prediction from above (block A) */
      E1[j][i] = store_coeff[(xpos + ypos * sed_pels / MB_SIZE) * 384 + j + i * 64]
        - A[j][i];
      /* DC/AC prediction from left (block B) */
      E2[j][i] = store_coeff[(xpos + ypos * sed_pels / MB_SIZE) * 384 + j * 8 + i * 64]
        - B[j][i];
    }
    /* Choose Intra Mode based on SAD of the prediction error */
    SAD0 += E0[i];
    SAD1 += E1[0][i];
    SAD2 += E2[0][i];
    for (j = 1; j < 8; j++)
    {
      SAD0 += 32 * abs (store_coeff[(xpos + ypos * sed_pels / MB_SIZE) * 384 + j + i * 64])
        + 32 * abs (store_coeff[(xpos + ypos * sed_pels / MB_SIZE) * 384 + j * 8 + i * 64]);
      SAD1 += 32 * abs (E1[j][i]) + 32 * abs (store_coeff[(xpos + ypos * sed_pels / MB_SIZE) * 384 + j * 8 + i * 64]);
      SAD2 += 32 * abs (store_coeff[(xpos + ypos * sed_pels / MB_SIZE) * 384 + j + i * 64]) + 32 * abs (E2[j][i]);
    }
  }

  if (SAD0 <= SAD1)
    if (SAD0 <= SAD2)
      Intra_Mode = INTRA_MODE_DC;
    else
      Intra_Mode = INTRA_MODE_HORI_AC;
  else if (SAD1 <= SAD2)
    Intra_Mode = INTRA_MODE_VERT_AC;
  else
    Intra_Mode = INTRA_MODE_HORI_AC;

#ifdef PRINTBLOCKS
  for (i = 0; i < 6; i++)
  {
    printf ("\nMB: %2i  Block: %1i\n", xpos + ypos * sed_pels / MB_SIZE, i);

    printf ("qcoeff: \n");
    for (j = 0; j < 8; j++)
    {
      printf ("%4i%4i%4i%4i%4i%4i%4i%4i\n", pcoeff[j * 8 + i * 64], pcoeff[j * 8 + 1 + i * 64], pcoeff[j * 8 + 2 + i * 64], pcoeff[j * 8 + 3 + i * 64], pcoeff[j * 8 + 4 + i * 64], pcoeff[j * 8 + 5 + i * 64], pcoeff[j * 8 + 6 + i * 64], pcoeff[j * 8 + 7 + i * 64]);
    }
    printf ("\nA[%1i]: ", i);
    for (j = 0; j < 8; j++)
    {
      printf ("%4i", A[j][i]);
    }
    printf ("\nB[%1i]: ", i);
    for (j = 0; j < 8; j++)
    {
      printf ("%4i", B[j][i]);
    }
    printf ("\nE0[%1i]: %3i", i, E0[i]);
    printf ("\nE1[%1i]: ", i);
    for (j = 0; j < 8; j++)
    {
      printf ("%4i", E1[j][i]);
    }
    printf ("\nE2[%1i]: ", i);
    for (j = 0; j < 8; j++)
    {
      printf ("%4i", E2[j][i]);
    }
  }
  printf ("\nSAD0: %4i, SAD1: %4i, SAD2: %4i\n", SAD0, SAD1, SAD2);

#else
  UNUSED(pcoeff);
#endif
  return Intra_Mode;
}

void Intra_AC_DC_Encode (int *pcoeff, int *store_rcoeff, int Intra_Mode, int xpos, int ypos, int newgob, int i)
{

  int A[8][6], B[8][6];
  int j;


  if (xpos == 0 && ypos == 0)
  { 
    /* top left corner */
    (i == 2 || i == 3) ? sed_fill_A (A, store_rcoeff, xpos, ypos, i - 2, i) : sed_fill_null (A, i);
    (i == 1 || i == 3) ? sed_fill_B (B, store_rcoeff, xpos, ypos, i - 1, i) : sed_fill_null (B, i);
  } else
  { 
    /* left border of picture */
    if (xpos == 0)
    {                           /* left edge of the picture */
      (i == 2 || i == 3) ? sed_fill_A (A, store_rcoeff, xpos, ypos, i - 2, i) :
      ((i == 0 || i == 1) && !(newgob)) ? sed_fill_A (A, store_rcoeff, xpos, ypos - 1, i + 2, i) :
      ((i == 4 || i == 5) && !(newgob)) ? sed_fill_A (A, store_rcoeff, xpos, ypos - 1, i, i) : sed_fill_null (A, i);
      (i == 1 || i == 3) ? sed_fill_B (B, store_rcoeff, xpos, ypos, i - 1, i) : sed_fill_null (B, i);

    } else
    {
      if (ypos == 0)
      { 
        /* top border of picture */
        (i == 2 || i == 3) ? sed_fill_A (A, store_rcoeff, xpos, ypos, i - 2, i) : sed_fill_null (A, i);
        (i == 4 || i == 5) ? sed_fill_B (B, store_rcoeff, xpos - 1, ypos, i, i) :
          (i == 1 || i == 3) ? sed_fill_B (B, store_rcoeff, xpos, ypos, i - 1, i) :
          sed_fill_B (B, store_rcoeff, xpos - 1, ypos, i + 1, i);
      } else
      { 
        /* anywhere else in the picture, do not
         * cross GOB boundary */
        (i == 2 || i == 3) ? sed_fill_A (A, store_rcoeff, xpos, ypos, i - 2, i) :
        ((i == 0 || i == 1) && !(newgob)) ? sed_fill_A (A, store_rcoeff, xpos, ypos - 1, i + 2, i) :
        ((i == 4 || i == 5) && !(newgob)) ? sed_fill_A (A, store_rcoeff, xpos, ypos - 1, i, i) : sed_fill_null (A, i);

        (i == 4 || i == 5) ? sed_fill_B (B, store_rcoeff, xpos - 1, ypos, i, i) :
        (i == 1 || i == 3) ? sed_fill_B (B, store_rcoeff, xpos, ypos, i - 1, i) :
        sed_fill_B (B, store_rcoeff, xpos - 1, ypos, i + 1, i);

      }
    }
  }

#ifdef PRINTBLOCKS
  printf ("Intra_Mode: %i", Intra_Mode);

  printf ("\nA[%1i]: ", i);
  for (j = 0; j < 8; j++)
  {
    printf ("%4i", A[j][i]);
  }
  printf ("\nB[%1i]: ", i);
  for (j = 0; j < 8; j++)
  {
    printf ("%4i", B[j][i]);
  }
#endif

  /* replace the qcoeff with the predicted values pcoeff */

  switch (Intra_Mode)
  {
    case INTRA_MODE_DC:
      /* It is OK to use 1024 to check the DC direction since the DC *
       * reconstructed coefficient will always be odd                */
      pcoeff[i * 64] -=
        (A[0][i] == 1024 && B[0][i] == 1024) ? 1024 :
        (A[0][i] == 1024) ? B[0][i] :
        (B[0][i] == 1024) ? A[0][i] : ((A[0][i] + B[0][i]) / 2);

      break;
    case INTRA_MODE_VERT_AC:
      for (j = 0; j < 8; j++)
      {
        pcoeff[j + i * 64] -= A[j][i];
      }
      break;
    case INTRA_MODE_HORI_AC:
      for (j = 0; j < 8; j++)
      {
        pcoeff[j * 8 + i * 64] -= B[j][i];
      }
      break;
    default:
      printf ("Error in Prediction in Advanced Intra Coding\n");
      exit (-1);
      break;
  }

#ifdef PRINTBLOCKS
  printf ("MB: %2i  Block: %1i\n", xpos + ypos * sed_pels / MB_SIZE, i);

  printf ("pcoeff: \n");
  for (j = 0; j < 8; j++)
  {
    printf ("%4i%4i%4i%4i%4i%4i%4i%4i\n", pcoeff[j * 8 + i * 64], pcoeff[j * 8 + 1 + i * 64], pcoeff[j * 8 + 2 + i * 64], pcoeff[j * 8 + 3 + i * 64], pcoeff[j * 8 + 4 + i * 64], pcoeff[j * 8 + 5 + i * 64], pcoeff[j * 8 + 6 + i * 64], pcoeff[j * 8 + 7 + i * 64]);
  }

#endif

  return;
}

/**********************************************************************
 *
 *	Name:           sed_fill_null, sed_fill_A and sed_fill_B
 *	Description:    Fill values in predictor coefficients
 *                      Functions used in advanced intra coding mode
 *
 *	Input:	        predictor qcoefficients
 *	Side effects:
 *
 *      Return:
 *
 *	Date:970717     Guy Cote <guyc@ee.ubc.ca>
 *
 ***********************************************************************/


void sed_fill_null (int pred[][6], int i)
{
  int j;

  pred[0][i] = 1024;

  for (j = 1; j < 8; j++)
  {
    pred[j][i] = 0;
  }
}

void sed_fill_A (int pred[][6], int *store_rcoeff, int xpos, int ypos, int block, int i)
{
  /* Fill first row of block at MB xpos, ypos, in pred[][i] */
  int j;

  for (j = 0; j < 8; j++)
  {
    pred[j][i] = *(store_rcoeff + (ypos * sed_pels / MB_SIZE + xpos) * 384 + block * 64 + j);
  }
}

void sed_fill_B (int pred[][6], int *store_rcoeff, int xpos, int ypos, int block, int i)
{
  /* Fill first column of block at MB xpos, ypos, in pred[][i] */
  int j;

  for (j = 0; j < 8; j++)
  {
    pred[j][i] = *(store_rcoeff + (ypos * sed_pels / MB_SIZE + xpos) * 384 + block * 64 + j * 8);
  }
}


/**********************************************************************
 *
 *	Name:           sed_Intra_AC_DC_Decode
 *	Description:    Intra Prediction in Advanced Intra Coding
 *
 *	Input:	        store_rcoeff, Intra_AC_DC, position of MB, store_QP
 *
 *	Side effects:   change qcoeff to predicted qcoeff
 *
 *      Return:
 *
 *	Date:970717     Guy Cote <guyc@ee.ubc.ca>
 *
 ***********************************************************************/


void sed_Intra_AC_DC_Decode (int *rcoeff, int *store_rcoeff, int INTRA_AC_DC, int xpos, int ypos, int newgob, int i)
{

  int A[8][6], B[8][6];
  int j, k, tempDC;


  if (xpos == 0 && ypos == 0)
  {                             
    /* top left corner */
    (i == 2 || i == 3) ? sed_fill_A (A, store_rcoeff, xpos, ypos, i - 2, i) : sed_fill_null (A, i);
    (i == 1 || i == 3) ? sed_fill_B (B, store_rcoeff, xpos, ypos, i - 1, i) : sed_fill_null (B, i);
  } else
  {                             /* left border of picture */
    if (xpos == 0)
    {                           /* left edge of the picture */
      (i == 2 || i == 3) ? sed_fill_A (A, store_rcoeff, xpos, ypos, i - 2, i) :
      ((i == 0 || i == 1) && !(newgob)) ? sed_fill_A (A, store_rcoeff, xpos, ypos - 1, i + 2, i) :
      ((i == 4 || i == 5) && !(newgob)) ? sed_fill_A (A, store_rcoeff, xpos, ypos - 1, i, i) : sed_fill_null (A, i);
      (i == 1 || i == 3) ? sed_fill_B (B, store_rcoeff, xpos, ypos, i - 1, i) : sed_fill_null (B, i);

    } else
    {
      if (ypos == 0)
      {                         /* top border of picture */
        (i == 2 || i == 3) ? sed_fill_A (A, store_rcoeff, xpos, ypos, i - 2, i) : sed_fill_null (A, i);
        (i == 4 || i == 5) ? sed_fill_B (B, store_rcoeff, xpos - 1, ypos, i, i) :
        (i == 1 || i == 3) ? sed_fill_B (B, store_rcoeff, xpos, ypos, i - 1, i) :
        sed_fill_B (B, store_rcoeff, xpos - 1, ypos, i + 1, i);
      } else
      {                         
        /* anywhere else in the picture, do not
         * cross GOB boundary */
        (i == 2 || i == 3) ? sed_fill_A (A, store_rcoeff, xpos, ypos, i - 2, i) :
        ((i == 0 || i == 1) && !(newgob)) ? sed_fill_A (A, store_rcoeff, xpos, ypos - 1, i + 2, i) :
        ((i == 4 || i == 5) && !(newgob)) ? sed_fill_A (A, store_rcoeff, xpos, ypos - 1, i, i) : sed_fill_null (A, i);

        (i == 4 || i == 5) ? sed_fill_B (B, store_rcoeff, xpos - 1, ypos, i, i) :
        (i == 1 || i == 3) ? sed_fill_B (B, store_rcoeff, xpos, ypos, i - 1, i) :
        sed_fill_B (B, store_rcoeff, xpos - 1, ypos, i + 1, i);
      }
    }
  }

  switch (INTRA_AC_DC)
  {
    case INTRA_MODE_DC:
      /* It is OK to use 1024 to check the DC direction since the DC *
       * reconstructed coefficient will always be odd                */
      tempDC = rcoeff[i * 64]
        + ((A[0][i] == 1024 && B[0][i] == 1024) ? 1024 :
           (A[0][i] == 1024) ? B[0][i] :
           (B[0][i] == 1024) ? A[0][i] : (A[0][i] + B[0][i]) / 2);
      for (j = 0; j < 8; j++)
        for (k = 0; k < 8; k++)
          rcoeff[i * 64 + j * 8 + k] = sed_clipAC (rcoeff[i * 64 + j * 8 + k]);
      rcoeff[i * 64] = sed_oddifysed_clipDC (tempDC);
      break;

    case INTRA_MODE_VERT_AC:
      tempDC = rcoeff[i * 64] + A[0][i];
      for (j = 1; j < 8; j++)
      {
        rcoeff[i * 64 + j] = sed_clipAC (rcoeff[i * 64 + j] + A[j][i]);
      }
      for (j = 1; j < 8; j++)
        for (k = 0; k < 8; k++)
          rcoeff[i * 64 + j * 8 + k] = sed_clipAC (rcoeff[i * 64 + j * 8 + k]);
      rcoeff[i * 64] = sed_oddifysed_clipDC (tempDC);
      break;
    case INTRA_MODE_HORI_AC:
      tempDC = rcoeff[i * 64] + B[0][i];
      for (j = 1; j < 8; j++)
        rcoeff[i * 64 + j * 8] = sed_clipAC (rcoeff[i * 64 + j * 8] + B[j][i]);
      for (j = 0; j < 8; j++)
        for (k = 1; k < 8; k++)
          rcoeff[i * 64 + j * 8 + k] = sed_clipAC (rcoeff[i * 64 + j * 8 + k]);
      rcoeff[i * 64] = sed_oddifysed_clipDC (tempDC);
      break;
    default:
      printf ("Error in Prediction in Advanced Intra Coding\n");
      exit (-1);
      break;
  }
  
  /* store the  reconstructed DCT coefficients */
  memcpy ((void *) (store_rcoeff + i * 64 + (xpos + ypos * sed_pels / MB_SIZE) * 384), (void *) (rcoeff + i * 64), sizeof (int) * 64);



#ifdef PRINTBLOCKS
  printf ("MB: %2i  Block: %1i\n", xpos + ypos * sed_pels / MB_SIZE, i);

  printf ("rcoeff: \n");
  for (j = 0; j < 8; j++)
  {
    printf ("%4i%4i%4i%4i%4i%4i%4i%4i\n", rcoeff[j * 8 + i * 64], rcoeff[j * 8 + 1 + i * 64], rcoeff[j * 8 + 2 + i * 64], rcoeff[j * 8 + 3 + i * 64], rcoeff[j * 8 + 4 + i * 64], rcoeff[j * 8 + 5 + i * 64], rcoeff[j * 8 + 6 + i * 64], rcoeff[j * 8 + 7 + i * 64]);
  }
#endif
  return;
}

/**********************************************************************
 *
 *	Name:           sed_oddifysed_clipDC, sed_clipAC and sed_clipDC
 *	Description:    Fill values in predictor coefficients
 *                      Functions used in advanced intra coding mode
 *
 *	Input:	        predictor qcoefficients
 *	Side effects:
 *
 *      Return:
 *
 *	Date:970717     Guy Cote <guyc@ee.ubc.ca>
 *
 ***********************************************************************/


int sed_oddifysed_clipDC (int x)
{

  int result;

  (x % 2) ? (result = sed_clipDC (x)) : (result = sed_clipDC (x + 1));
  return result;
}
int sed_clipAC (int x)
{
  int clipped;

  if (x > 2047)
    clipped = 2047;
  else if (x < -2048)
    clipped = -2048;
  else
    clipped = x;
  return clipped;

}
int sed_clipDC (int x)
{
  int clipped;
  if (x > 2047)
    clipped = 2047;
  else if (x < 0)
    clipped = 0;
  else
    clipped = x;
  return clipped;
}















