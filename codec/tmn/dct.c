/************************************************************************
 *
 *  dct.c, part of tmn (TMN encoder)
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


/*****************************************************************
 *
 * These routines are translated from Gisle Bjøntegaards's FORTRAN
 * routines by Robert.Danielsen@nta.no
 *
 * 970715       Modified by Guy Cote <guyc@ee.ubc.ca> to include new
 *              scanning for (advanced intra coding mode of H.263+
 *
 *****************************************************************/

#include "macros.h"
#include "sim.h"

#include <math.h>

#ifndef PI
# ifdef M_PI
#  define PI M_PI
# else
#  define PI 3.14159265358979323846
# endif
#endif

int zigzag[8][8] = {
  {0, 1, 5, 6, 14, 15, 27, 28},
  {2, 4, 7, 13, 16, 26, 29, 42},
  {3, 8, 12, 17, 25, 30, 41, 43},
  {9, 11, 18, 24, 31, 40, 44, 53},
  {10, 19, 23, 32, 39, 45, 52, 54},
  {20, 22, 33, 38, 46, 51, 55, 60},
  {21, 34, 37, 47, 50, 56, 59, 61},
  {35, 36, 48, 49, 57, 58, 62, 63},
};
int alternate_horizontal[8][8] = {
  {0, 1, 2, 3, 10, 11, 12, 13},
  {4, 5, 8, 9, 17, 16, 15, 14},
  {6, 7, 19, 18, 26, 27, 28, 29},
  {20, 21, 24, 25, 30, 31, 32, 33},
  {22, 23, 34, 35, 42, 43, 44, 45},
  {36, 37, 40, 41, 46, 47, 48, 49},
  {38, 39, 50, 51, 56, 57, 58, 59},
  {52, 53, 54, 55, 60, 61, 62, 63},
};
int alternate_vertical[8][8] = {
  {0, 4, 6, 20, 22, 36, 38, 52},
  {1, 5, 7, 21, 23, 37, 39, 53},
  {2, 8, 19, 24, 34, 40, 50, 54},
  {3, 9, 18, 25, 35, 41, 51, 55},
  {10, 17, 26, 30, 42, 46, 56, 60},
  {11, 16, 27, 31, 43, 47, 57, 61},
  {12, 15, 28, 32, 44, 48, 58, 62},
  {13, 14, 29, 33, 45, 49, 59, 63},
};

/**********************************************************************
 *
 *	Name:        Dct
 *	Description:	Does dct on an 8x8 block
 *
 *	Input:        64 pixels in a 1D array
 *	Returns:	64 coefficients in a 1D array
 *	Side effects:
 *
 *	Date: 930128	Author: Robert.Danielsen@nta.no
 *
 **********************************************************************/

int Dct (int *block, int *coeff)
{
  int j1, i, j, k;
  float b[8];
  float b1[8];
  float d[8][8];
  float f0 = (float) .7071068;
  float f1 = (float) .4903926;
  float f2 = (float) .4619398;
  float f3 = (float) .4157348;
  float f4 = (float) .3535534;
  float f5 = (float) .2777851;
  float f6 = (float) .1913417;
  float f7 = (float) .0975452;

  for (i = 0, k = 0; i < 8; i++, k += 8)
  {
    for (j = 0; j < 8; j++)
    {
      b[j] = (float) block[k + j];
    }
    /* Horizontal transform */
    for (j = 0; j < 4; j++)
    {
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
  for (i = 0; i < 8; i++)
  {
    for (j = 0; j < 4; j++)
    {
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
    d[4][i] = (b[0] - b[1]) * f4;
    d[2][i] = b[2] * f6 + b[3] * f2;
    d[6][i] = b[3] * f6 - b[2] * f2;
    b1[4] = b[4] + b[5];
    b1[7] = b[7] + b[6];
    b1[5] = b[4] - b[5];
    b1[6] = b[7] - b[6];
    d[1][i] = b1[4] * f7 + b1[7] * f1;
    d[5][i] = b1[5] * f3 + b1[6] * f5;
    d[7][i] = b1[7] * f7 - b1[4] * f1;
    d[3][i] = b1[6] * f3 - b1[5] * f5;
  }
  for (i = 0; i < 8; i++)
  {
    for (j = 0; j < 8; j++)
    {
      *(coeff + j + i * 8) = (int) (d[i][j]);
    }
  }
  return 0;
}

/**********************************************************************
 *
 *	Name:           Scan and Descan
 *	Description:	Does scanning of the an 8x8 block quantized dct
 *                      coefficients.
 *                      (zigzag, or alernate horizontal, alternate
 *                      vertical when advanced intra coding is used)
 *
 *	Input:          64 quantized coefficients in a 1D array
 *	Returns:	0
 *	Side effects:	scans the dct qcoeff in the desired order
 *                      (zigzag if advanced intra coding is not used)
 *
 *	Date: 970716
 *      Author: Guy Cote <guyc@ee.ubc.ca>
 *
 **********************************************************************/

void Scan (int *qcoeff, int Intra_Mode)
{
  int i, j, k;
  int store_qcoeff[384];
  /* Zigzag - scanning of a Macroblock */
  /* Choose Scanning if Advanced Intra Coding Mode is used */

  for (i = 0; i < 384; i++)
  {
    store_qcoeff[i] = *(qcoeff + i);
  }

  for (k = 0; k < 6; k++)
  {                             /* Do for all blocks */
    for (i = 0; i < 8; i++)
    {
      for (j = 0; j < 8; j++)
      {
        if (advanced_intra_coding)
        {
          switch (Intra_Mode)
          {
            case INTRA_MODE_DC:
              *(qcoeff + zigzag[i][j] + k * 64) = (int) (store_qcoeff[i * 8 + j + k * 64]);
              break;
            case INTRA_MODE_VERT_AC:
              *(qcoeff + alternate_horizontal[i][j] + k * 64) = (int) (store_qcoeff[i * 8 + j + k * 64]);
              break;
            case INTRA_MODE_HORI_AC:
              *(qcoeff + alternate_vertical[i][j] + k * 64) = (int) (store_qcoeff[i * 8 + j + k * 64]);
              break;
            default:
              printf ("Invalid Intra_Mode in Advanced Intra Coding");
              exit (-1);
              break;
          }
        } else
        {
          *(qcoeff + zigzag[i][j] + k * 64) = (int) (store_qcoeff[i * 8 + j + k * 64]);
        }
      }
    }
  }
  return;
}
void DeScan (int *qcoeff, int Intra_Mode)
{
  /* Descan coefficients first */
  /* Choose Scanning if Advanced Intra Coding Mode is used */
  int store_qcoeff[384];
  int i, j, k;

  for (i = 0; i < 384; i++)
  {
    store_qcoeff[i] = *(qcoeff + i);
  }
  for (k = 0; k < 6; k++)
  {                             /* Do for all blocks */
    for (i = 0; i < 8; i++)
    {
      for (j = 0; j < 8; j++)
      {
        if (advanced_intra_coding)
        {                       /* Descan with proper scanning matrix */
          switch (Intra_Mode)
          {
            case INTRA_MODE_DC:
              *(qcoeff + k * 64 + i * 8 + j) = *(store_qcoeff + k * 64 + zigzag[i][j]);
              break;
            case INTRA_MODE_VERT_AC:
              *(qcoeff + k * 64 + i * 8 + j) = *(store_qcoeff + k * 64 + alternate_horizontal[i][j]);
              break;
            case INTRA_MODE_HORI_AC:
              *(qcoeff + k * 64 + i * 8 + j) = *(store_qcoeff + k * 64 + alternate_vertical[i][j]);
              break;
            default:
              printf ("Invalid Intra_Mode in Advanced Intra Coding\n");
              exit (-1);
              break;
          }
        } else
        {
          *(qcoeff + k * 64 + i * 8 + j) = *(store_qcoeff + k * 64 + zigzag[i][j]);
        }
      }
    }
  }
  return;
}

#ifdef FASTIDCT

/**********************************************************************
 *
 *	Name:        idct
 *	Description:    inverse dct on 64 coefficients
 *                      single precision floats
 *
 *	Input:        64 coefficients, block for 64 pixels
 *	Returns:        0
 *	Side effects:
 *
 *	Date: 930128	Author: Robert.Danielsen@nta.no
 *
 **********************************************************************/

int idct (int *coeff, int *block)
{
  int j1, i, j;
  double b[8], b1[8], d[8][8];
  double f0 = .7071068;
  double f1 = .4903926;
  double f2 = .4619398;
  double f3 = .4157348;
  double f4 = .3535534;
  double f5 = .2777851;
  double f6 = .1913417;
  double f7 = .0975452;
  double e, f, g, h;

  /* Horizontal */

  for (i = 0; i < 8; i++)
  {

    for (j = 0; j < 8; j++)
      b[j] = coeff[j + i * 8];

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

    for (j = 0; j < 4; j++)
    {
      j1 = 7 - j;
      d[i][j] = b[j] + b[j1];
      d[i][j1] = b[j] - b[j1];
    }
  }


  /* Vertical */

  for (i = 0; i < 8; i++)
  {
    for (j = 0; j < 8; j++)
    {
      b[j] = d[j][i];
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

    for (j = 0; j < 4; j++)
    {
      j1 = 7 - j;
      d[j][i] = b[j] + b[j1];
      d[j1][i] = b[j] - b[j1];
    }
  }

  for (i = 0; i < 8; i++)
  {
    for (j = 0; j < 8; j++)
    {
      *(block + i * 8 + j) = mnint (d[i][j]);
    }
  }
  return 0;
}

#else
/* Perform IEEE 1180 reference (64-bit floating point, separable 8x1
 * direct matrix multiply) Inverse Discrete Cosine Transform */


/* Here we use math.h to generate constants.  Compiler results may vary a
 * little */


/* private data */

/* cosine transform matrix for 8x1 IDCT */
static double c[8][8];

/* initialize DCT coefficient matrix */

void init_idctref ()
{
  int freq, time;
  double scale;

  for (freq = 0; freq < 8; freq++)
  {
    scale = (freq == 0) ? sqrt (0.125) : 0.5;
    for (time = 0; time < 8; time++)
      c[freq][time] = scale * cos ((PI / 8.0) * freq * (time + 0.5));
  }
}

/* perform IDCT matrix multiply for 8x8 coefficient block */

void idctref (int *coeff, int *block)
{
  int i, j, k, v;
  double partial_product;
  double tmp[64];
  int tmp2[64];

  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
      tmp2[j + i * 8] = coeff[j + i * 8];

  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
    {
      partial_product = 0.0;

      for (k = 0; k < 8; k++)
        partial_product += c[k][j] * tmp2[8 * i + k];

      tmp[8 * i + j] = partial_product;
    }

  /* Transpose operation is integrated into address mapping by switching
   * loop order of i and j */

  for (j = 0; j < 8; j++)
    for (i = 0; i < 8; i++)
    {
      partial_product = 0.0;

      for (k = 0; k < 8; k++)
        partial_product += c[k][i] * tmp[8 * k + j];

      v = (int) floor (partial_product + 0.5);
      block[8 * i + j] = (v < -256) ? -256 : ((v > 255) ? 255 : v);
    }


}
#endif
