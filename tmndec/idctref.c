/************************************************************************
 *
 *  idctref.c, inverse DCT, double precision, for tmndecode (H.263 decoder)
 *  Copyright (C) 1995, 1996  Telenor R&D, Norway
 *
 *  Contacts:
 *  Robert Danielsen                  <Robert.Danielsen@nta.no>
 *
 *  Telenor Research and Development  http://www.nta.no/brukere/DVC/
 *  P.O.Box 83                        tel.:   +47 63 84 84 00
 *  N-2007 Kjeller, Norway            fax.:   +47 63 81 00 76
 *
 *  Copyright (C) 1997  University of BC, Canada
 *  Modified by: Michael Gallant <mikeg@ee.ubc.ca>
 *               Guy Cote <guyc@ee.ubc.ca>
 *               Berna Erol <bernae@ee.ubc.ca>
 *
 *  Contacts:
 *  Michael Gallant                   <mikeg@ee.ubc.ca>
 *
 *  UBC Image Processing Laboratory   http://www.ee.ubc.ca/image
 *  2356 Main Mall                    tel.: +1 604 822 4051
 *  Vancouver BC Canada V6T1Z4        fax.: +1 604 822 5949
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



/* based on mpeg2decode, (C) 1994, MPEG Software Simulation Group and
 * mpeg2play, (C) 1994 Stefan Eckart <stefan@lis.e-technik.tu-muenchen.de>
 * 
 */


/* Perform IEEE 1180 reference (64-bit floating point, separable 8x1
 * direct matrix multiply) Inverse Discrete Cosine Transform */


/* Here we use math.h to generate constants.  Compiler results may vary a
 * little */

#include <math.h>

#include "config.h"

#ifndef PI
# ifdef M_PI
#  define PI M_PI
# else
#  define PI 3.14159265358979323846
# endif
#endif

/* global declarations */
void init_idctref _ANSI_ARGS_ ((void));
void idctref _ANSI_ARGS_ ((short *block));

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

void idctref (short *block)
{
  int i, j, k, v;
  double partial_product;
  double tmp[64];

  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
    {
      partial_product = 0.0;

      for (k = 0; k < 8; k++)
        partial_product += c[k][j] * block[8 * i + k];

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
