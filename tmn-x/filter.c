/************************************************************************
 *
 *  filter.c, filter routines of tmn (TMN encoder)
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
static int STRENGTH[] = {1,1,2,2,3,3,4,4,4,5,5,6,6,7,7,7,8,8,8,9,9,9,10,10,10,11,11,11,12,12,12};
static int MQ_chroma_QP_table[32] = {0,1,2,3,4,5,6,6,7,8,9,9,10,10,11,11,12,12,12,13,13,13,14,14,14,14,14,15,15,15,15,15};


/**********************************************************************
 *
 *	Name:		EdgeFilter
 *	Description:    performs in the loop edge-filtering on
 *                      reconstructed frames (Deblocking Filter)
 *
 *	Input:	        pointers to reconstructed frame and difference
 *                      image
 *	Returns:
 *	Side effects:
 *
 *      Date: 970820    Author: guyc@ee.ubc.ca
 *                              modified to implement annex J of H.263+
 *
 ***********************************************************************/


void EdgeFilter (PictImage * recon, Pict * pic)
{

  /* Luma */
  HorizEdgeFilter (recon->lum, sed_pels, sed_lines, pic, 0);
  VertEdgeFilter (recon->lum, sed_pels, sed_lines, pic, 0);

  /* Chroma */
  HorizEdgeFilter (recon->Cb, sed_pels / 2, sed_lines / 2, pic, 1);
  VertEdgeFilter (recon->Cb, sed_pels / 2, sed_lines / 2, pic, 1);
  HorizEdgeFilter (recon->Cr, sed_pels / 2, sed_lines / 2, pic, 1);
  VertEdgeFilter (recon->Cr, sed_pels / 2, sed_lines / 2, pic, 1);

  /* that's it */
  return;
}


/***********************************************************************/


void HorizEdgeFilter (unsigned char *rec, int width, int height, Pict * pic, int chr)
{
  int i, j;
  int delta, d1, d2;
  int mbc, mbr, do_filter;
  int QP;
  int bquant[] = {5, 6, 7, 8};
  int mbr_above;

  /* horizontal edges */
  for (j = 8; j < height; j += 8)
  {
    for (i = 0; i < width; i++)
    {
      if (!chr)
      {
        mbr = j >> 4;
        mbc = i >> 4;
        mbr_above = (j - 8) >> 4;
      } else
      {
        mbr = j >> 3;
        mbc = i >> 3;
        mbr_above = mbr - 1;
      }

      do_filter = coded_map[mbr + 1][mbc + 1] || coded_map[mbr_above + 1][mbc + 1];

      if (do_filter)
      {
        if (pic->PB)
        {
          QP = coded_map[mbr + 1][mbc + 1] ?
            mmin (31, bquant[pic->BQUANT] * quant_map[mbr + 1][mbc + 1] / 4) :
            mmin (31, bquant[pic->BQUANT] * quant_map[mbr_above + 1][mbc + 1] / 4);
        } else
          QP = coded_map[mbr + 1][mbc + 1] ? quant_map[mbr + 1][mbc + 1] : quant_map[mbr_above + 1][mbc + 1];

        if (modified_quantization && chr) 
        {
          QP=MQ_chroma_QP_table[QP];                   
        }
        delta = (int) (((int) (*(rec + i + (j - 2) * width)) +
                        (int) (*(rec + i + (j - 1) * width) * (-4)) +
                        (int) (*(rec + i + (j) * width) * (4)) +
                     (int) (*(rec + i + (j + 1) * width) * (-1))) / 8.0);

        d1 = sign (delta) * mmax (0, abs (delta) - mmax (0, 2 * (abs (delta) - STRENGTH[QP - 1])));

        d2 = mmin (abs (d1 / 2), mmax (-abs (d1 / 2), (int) (((*(rec + i + (j - 2) * width) -
                                   *(rec + i + (j + 1) * width))) / 4)));

        *(rec + i + (j + 1) * width) = (int) (*(rec + i + (j + 1) * width)) + d2;
        *(rec + i + (j) * width) =
          mmin (255, mmax (0, (int) (*(rec + i + (j) * width)) - d1));
        *(rec + i + (j - 1) * width) =
          mmin (255, mmax (0, (int) (*(rec + i + (j - 1) * width)) + d1));
        *(rec + i + (j - 2) * width) = (int) (*(rec + i + (j - 2) * width)) - d2;
      }
    }
  }
  return;
}

void VertEdgeFilter (unsigned char *rec, int width, int height, Pict * pic, int chr)
{
  int i, j;
  int delta, d1, d2;
  int mbc, mbr;
  int do_filter;
  int bquant[] = {5, 6, 7, 8};
  int QP;
  int mbc_left;

  /* vertical edges */
  for (i = 8; i < width; i += 8)
  {
    for (j = 0; j < height; j++)
    {
      if (!chr)
      {
        mbr = j >> 4;
        mbc = i >> 4;
        mbc_left = (i - 8) >> 4;
      } else
      {
        mbr = j >> 3;
        mbc = i >> 3;
        mbc_left = mbc - 1;
      }
      do_filter = coded_map[mbr + 1][mbc + 1] || coded_map[mbr + 1][mbc_left + 1];

      if (do_filter)
      {
        if (pic->PB)
        {
          QP = coded_map[mbr + 1][mbc + 1] ?
            mmin (31, bquant[pic->BQUANT] * quant_map[mbr + 1][mbc + 1] / 4) :
            mmin (31, bquant[pic->BQUANT] * quant_map[mbr + 1][mbc_left + 1] / 4);
        } else
          QP = coded_map[mbr + 1][mbc + 1] ? quant_map[mbr + 1][mbc + 1] : quant_map[mbr + 1][mbc_left + 1];

        if (modified_quantization && chr) 
        {
          QP=MQ_chroma_QP_table[QP];                   
        }
        delta = (int) (((int) (*(rec + i - 2 + j * width)) +
                        (int) (*(rec + i - 1 + j * width) * (-4)) +
                        (int) (*(rec + i + j * width) * (4)) +
                        (int) (*(rec + i + 1 + j * width) * (-1))) / 8.0);

        d1 = sign (delta) * mmax (0, abs (delta) - mmax (0, 2 * (abs (delta) - STRENGTH[QP - 1])));
        d2 = mmin (abs (d1 / 2), mmax (-abs (d1 / 2), (int) ((*(rec + i - 2 + j * width) -
                                      *(rec + i + 1 + j * width)) / 4)));

        *(rec + i + 1 + j * width) = (int) (*(rec + i + 1 + j * width)) + d2;
        *(rec + i + j * width) =
          mmin (255, mmax (0, (int) (*(rec + i + j * width)) - d1));
        *(rec + i - 1 + j * width) =
          mmin (255, mmax (0, (int) (*(rec + i - 1 + j * width)) + d1));
        *(rec + i - 2 + j * width) = (int) (*(rec + i - 2 + j * width)) - d2;
      }
    }
  }
  return;
}


