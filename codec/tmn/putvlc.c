/************************************************************************
 *
 *  tmn (TMN encoder) 
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



/************************************************************************
 *
 * Author:        Robert Danielsen <Robert.Danielsen@nta.no>
 * Date:	06.05.96
 *
 * Comment:	Based on ideas from MPEG-2 Software Simulation Group
 *
 ************************************************************************/

#include <assert.h>
#include "sim.h"
#include "vlc.h"



int
 put_mv (int mvint)
{
  int sign = 0;
  int absmv;

  if (mvint >= 32)
  {
    absmv = -mvint + 64;
    sign = 1;
  } else
    absmv = mvint;

  putbits (mvtab[absmv].len, mvtab[absmv].code);

  if (mvint != 0)
  {
    putbits (1, sign);
    return mvtab[absmv].len + 1;
  } else
    return mvtab[absmv].len;
}

int
 put_cbpcm_intra (int cbpc, int mode)
{
  int index;

  index = ((mode & 3) >> 1) | ((cbpc & 3) << 2);

  putbits (cbpcm_intra_tab[index].len, cbpcm_intra_tab[index].code);

  return cbpcm_intra_tab[index].len;
}

int
 put_cbpcm_inter (int cbpc, int mode)
{
  int index;

  index = (mode & 7) | ((cbpc & 3) << 3);

  putbits (cbpcm_inter_tab[index].len, cbpcm_inter_tab[index].code);

  return cbpcm_inter_tab[index].len;
}


int
 put_cbpy (int cbp, int mode)
{
  int index;

  index = cbp >> 2;

  /* if cbpcm is "11", use CBPY intra with annex S (S.3) */ 
  if ((mode < 3 || mode == 5) && !(alternative_inter_vlc && ((cbp & 3) == 3)))    
    index ^= 15;

  putbits (cbpy_tab[index].len, cbpy_tab[index].code);

  return cbpy_tab[index].len;
}


int
 put_coeff (int run, int level, int last)
{
  int length = 0;

  assert (last >= 0 && last < 2);
  assert (run >= 0 && run < 64);
  assert (level > 0 && level < 128);

  if (last == 0)
  {
    if (run < 2 && level < 13)
    {
      putbits (coeff_tab0[run][level - 1].len,
               coeff_tab0[run][level - 1].code);

      length = coeff_tab0[run][level - 1].len;
    } else if (run > 1 && run < 27 && level < 5)
    {
      putbits (coeff_tab1[run - 2][level - 1].len,
               coeff_tab1[run - 2][level - 1].code);

      length = coeff_tab1[run - 2][level - 1].len;
    }
  } else if (last == 1)
  {
    if (run < 2 && level < 4)
    {
      putbits (coeff_tab2[run][level - 1].len,
               coeff_tab2[run][level - 1].code);

      length = coeff_tab2[run][level - 1].len;
    } else if (run > 1 && run < 42 && level == 1)
    {
      putbits (coeff_tab3[run - 2].len,
               coeff_tab3[run - 2].code);

      length = coeff_tab3[run - 2].len;
    }
  }
  return length;
}


int
 put_intra_coeff (int run, int level, int last)
{
  int length = 0;

  assert (last >= 0 && last < 2);
  assert (run >= 0 && run < 64);
  assert (level > 0 && level < 128);

  if (last == 0)
  {
    if (run < 2 && level < 26)
    {
      putbits (intra_coeff_tab0[run][level - 1].len,
               intra_coeff_tab0[run][level - 1].code);

      length = intra_coeff_tab0[run][level - 1].len;
    } else if (run > 1 && run < 14 && level < 5)
    {
      putbits (intra_coeff_tab1[run - 2][level - 1].len,
               intra_coeff_tab1[run - 2][level - 1].code);

      length = intra_coeff_tab1[run - 2][level - 1].len;
    }
  } else if (last == 1)
  {
    if (run < 2 && level < 11)
    {
      putbits (intra_coeff_tab2[run][level - 1].len,
               intra_coeff_tab2[run][level - 1].code);

      length = intra_coeff_tab2[run][level - 1].len;
    } else if (run > 1 && run < 24 && level < 4)
    {
      putbits (intra_coeff_tab3[run - 2][level - 1].len,
               intra_coeff_tab3[run - 2][level - 1].code);

      length = intra_coeff_tab3[run - 2][level - 1].len;
    }
  }
  return length;
}

/* functions used in alternative inter vlc mode (Annex S) */

void
 show_inter_coeff (int run, int level, int last, int *length, int *code)
{

  assert (last >= 0 && last < 2);
  assert (run >= 0 && run < 64);
  assert (level > 0 && level < 128);
  *length = 22;

  if (last == 0)
  {
    if (run < 2 && level < 13)
    {
      *code = coeff_tab0[run][level - 1].code;
      *length = coeff_tab0[run][level - 1].len;
    } else if (run > 1 && run < 27 && level < 5)
    {
      *code = coeff_tab1[run - 2][level - 1].code;
      *length = coeff_tab1[run - 2][level - 1].len;
    }
  } else if (last == 1)
  {
    if (run < 2 && level < 4)
    {
      *code = coeff_tab2[run][level - 1].code;
      *length = coeff_tab2[run][level - 1].len;
    } else if (run > 1 && run < 42 && level == 1)
    {
      *code = coeff_tab3[run - 2].code;
      *length = coeff_tab3[run - 2].len;
    }
  }
  return;
}

void
 show_intra_coeff (int run, int level, int last, int *length, int *code)
{

  assert (last >= 0 && last < 2);
  assert (run >= 0 && run < 64);
  assert (level > 0 && level < 128);

  *length = 22;

  if (last == 0)
  {
    if (run < 2 && level < 26)
    {
      *code = intra_coeff_tab0[run][level - 1].code;
      *length = intra_coeff_tab0[run][level - 1].len;
    } else if (run > 1 && run < 14 && level < 5)
    {
      *code = intra_coeff_tab1[run - 2][level - 1].code;
      *length = intra_coeff_tab1[run - 2][level - 1].len;
    }
  } else if (last == 1)
  {
    if (run < 2 && level < 11)
    {
      *code = intra_coeff_tab2[run][level - 1].code;
      *length = intra_coeff_tab2[run][level - 1].len;
    } else if (run > 1 && run < 24 && level < 4)
    {
      *code = intra_coeff_tab3[run - 2][level - 1].code;
      *length = intra_coeff_tab3[run - 2][level - 1].len;
    }
  }
  return;
}

int show_inter_run (int length, int code)
{
  unsigned int tmp;
  int run;
  VLCtab *tab;

  tmp = (code << (12 - length));
  if (tmp >= 512)
    tab = &DCT3Dtab0[(tmp >> 5) - 16];
  else if (tmp >= 128)
    tab = &DCT3Dtab1[(tmp >> 2) - 32];
  else if (tmp >= 8)
    tab = &DCT3Dtab2[(tmp >> 0) - 8];
  else
  {
    fprintf (stderr, "invalid Huffman code in show_inter_run()\n");
    exit (-1);
    return 0;
  }

  run = (tab->val >> 4) & 255;
  return run;

}

/**********************************************************************
 *
 *	Name:           put_rvlc
 *	Description:    Write the rvlc code to the bitstream 
 *                      for difference motion vectors
 *
 *	Input:          difference motion vector
 *
 *	Returns:        length of motion vector code
 *	Side effects:
 *
 *	Date: 971026    Author:	Guy Cote <guyc@ee.ubc.ca>
 *
 ***********************************************************************/

int put_rvlc(int data)
{
  int sign, length = 0, code = 0;
  

  if (data == 0) 
  {
    putbits (1,1);
    length = 1;
  }
  else 
  { 
    sign = (data < 0) ? 1 : 0;
    data = abs(data);

    while (data > 1)
    {
      code += (((data & 1)<<1) + 1) << length;
      data >>= 1;
      length += 2;
    }
    code = (code<<2) + (sign<<1);
    length += 3; 
    putbits ( length, code );
  }
  return length;
}


