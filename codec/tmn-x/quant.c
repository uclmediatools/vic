
/************************************************************************
 *
 *  quant.c, part of tmn (TMN encoder)
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

static int MQ_chroma_QP_table[32] = {0,1,2,3,4,5,6,6,7,8,9,9,10,10,11,11,12,12,12,13,13,13,14,14,14,14,14,15,15,15,15,15};

/**********************************************************************
 *
 *	Name:        Quant_blk
 *	Description:	quantizer
 *
 *	Input:        pointers to coeff and qcoeff
 *
 *	Returns:
 *	Side effects:
 *
 *	Date: 940111	Author:	<klillevo@mailbox.jf.intel.com>
 *
 ***********************************************************************/


void Quant_blk (int *coeff, int *qcoeff, int QP, int Mode, int block)
{
  int i;
  int level;
  

  /*Modify the chroma quantizer if modified quantization mode is in use*/
  if (modified_quantization && (block == 4 || block == 5)) 
  {
    QP=MQ_chroma_QP_table[QP];                   
  }
  if (QP)
  {
    if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
    {                           /* Intra */
      /* advanced intra coding quantization */
      if (advanced_intra_coding)
      {
        for (i = 0; i < 64; i++)
        {
          qcoeff[block*64 + i] = (coeff[block*64 + i] + sign(coeff[block*64 + i]) * QP/2) / (2 * QP);
        }

      } else
      {
        qcoeff[block*64] = mmax (1, mmin (254, (coeff[block*64]+4) / 8));

        for (i = 1; i < 64; i++)
        {
          level = (abs (coeff[block*64 + i])) / (2 * QP);

          /* if QP is larger than 8, don't use extended quantization */
          if (!modified_quantization || QP >= 8)
          {
            /* clipping to [-127,+127] */
            qcoeff[block*64 + i] = mmin (127, mmax (-127, sign (coeff[block*64 + i]) * level));
          } else
          {
            /* no clipping, quantized coefficients  */
            qcoeff[block*64 + i] = sign (coeff[block*64 + i]) * level;
          }
        }
      }
    }
      
    else
    {                           /* non Intra */
      for (i = 0; i < 64; i++)
      {
        level = (abs (coeff[block*64 + i]) - QP / 2) / (2 * QP);
        /* if QP is larger than 8, don't use extended quantization */
        if (!modified_quantization || QP >= 8)
        {
          /* clipping to [-127,+127] */
          qcoeff[block*64 + i] = mmin (127, mmax (-127, sign (coeff[block*64 + i]) * level));
        } else
        {
          /* no clipping, quantized coefficients  */
          qcoeff[block*64 + i] = sign (coeff[block*64 + i]) * level;
        }
      }
    }
  }
    
  else
  {
    /* No quantizing. Used only for testing. Bitstream will not be
     * decodable whether clipping is performed or not */
    for (i = 0; i < 64; i++)
    {
      qcoeff[block*64 + i] = coeff[block*64 + i];
    }
  }  
  return;
}

/**********************************************************************
 *
 *	Name:        Dequant
 *	Description:	dequantizer
 *
 *	Input:        pointers to coeff and qcoeff
 *
 *	Returns:
 *	Side effects:
 *
 *	Date: 940111	Author:	Karl.Lillevold@nta.no
 *
 ***********************************************************************/


void Dequant (int *qcoeff,int *rcoeff, int QP, int Mode, int block)
{
  int i;

  /* Modify the chorama quantizer if modified quantization mode is in use */
  if (modified_quantization && (block == 4 || block == 5))
  {
    QP=MQ_chroma_QP_table[QP];
  }
  if (QP)
  {
    for (i = 0; i < 64; i++)
    {
      if (qcoeff[block * 64 + i])
      {
        if ((advanced_intra_coding) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
        {
          /* Advanced intra coding dequantization */
          rcoeff[block * 64 + i] = (QP * (2 * qcoeff[block * 64 + i]));

        } else
        {
          if ((QP % 2) == 1)
            rcoeff[block * 64 + i] = QP * (2 * abs (qcoeff[block * 64 + i]) + 1);
          else
            rcoeff[block * 64 + i] = QP * (2 * abs (qcoeff[block * 64 + i]) + 1) - 1;
          rcoeff[block * 64 + i] = sign (qcoeff[block * 64 + i]) * rcoeff[block * 64 + i];
        }

      } else
        rcoeff[block * 64 + i] = 0;
    }

    if ((Mode == MODE_INTRA || Mode == MODE_INTRA_Q) && !advanced_intra_coding)
    {
      /* Intra (except when advanced intra coding is used) */
      rcoeff[block * 64] = qcoeff[block * 64] * 8;
    }
  } else
  {
    /* No quantizing at all */
    for (i = 0; i < 64; i++)
    {
      rcoeff[block * 64 + i] = qcoeff[block * 64 + i];
    }
  }
  return;
}
