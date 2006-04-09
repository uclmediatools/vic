/************************************************************************
 *
 *  countbit.c, bitstream generation for tmn (TMN encoder)
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
#include"sactbls.h"
#include"indices.h"
#include "putvlc.h"

//SV-XXX: defined UNUSED() macro for unused variables
#ifndef UNUSED
#define UNUSED(x) (x) = (x)
#endif

int arith_used = 0;

int true_b_length[5][3] = { {0,2,4}, {3,3,5}, {3,3,5}, {5,5,5}, {6,7,9} };
int true_b_code[5][3] = { {1,3,1}, {4,5,6}, {2,3,7}, {4,5,1}, {1,1,1} };
int ei_length[4][4] = { {1,3,3,3}, {4,7,7,7}, {8,8,8,8}, {8,8,8,8} };
int ei_code[4][4] = { {1,1,2,3}, {1,1,2,3}, {1,9,10,11}, {12,13,14,15} };
int ep_length[4][3] = { {0,1,3}, {3,3,5}, {5,5,6}, {7,8,9} };
int ep_code[4][3] = { {1,1,1}, {2,3,1}, {2,3,1}, {1,1,1} };

/**********************************************************************
 *
 *	Name:        CountBitsMB
 *	Description:    counts bits used for MB info
 *
 *	Input:	        Mode, COD, CBP, Picture and Bits structures
 *	Returns:
 *	Side effects:
 *
 *	Date: 941129	Author: <klillevo@mailbox.jf.intel.com>
 *
 ***********************************************************************/
void CountBitsMB (int Mode, int COD, int CBP, int CBPB, Pict * pic,
                   Bits * bits, int scalability_prediction_type)
{
  int cbpy, cbpcm, length;
  //SV-XXX unused variable: int true_B_cbp_present = 0;

  UNUSED(scalability_prediction_type); //SV-XXX: unused param

  /* COD */
  if (trace)
  {
    fprintf (tf, "MB-nr: %d\n", pic->MB);
    if (pic->picture_coding_type == PCT_INTER ||
        pic->picture_coding_type == PCT_IPB   ||
        pic->picture_coding_type == PCT_PB)
      fprintf (tf, "  COD: %d\n", COD);
  }
  if (pic->picture_coding_type == PCT_INTER ||
      pic->picture_coding_type == PCT_IPB   ||
      pic->picture_coding_type == PCT_PB )
  {
    putbits (1, COD);
    bits->COD++;
  }
  if (COD)
    return;                     /* not coded */

  /* CBPCM */
  cbpcm = Mode | ((CBP & 3) << 4);
  if (trace)
  {
    fprintf (tf, "CBPCM (CBP=%d) (cbpcm=%d): ", CBP, cbpcm);
  }
  if (pic->picture_coding_type == PCT_INTRA)
    length = put_cbpcm_intra (CBP, Mode);
  else 
    length = put_cbpcm_inter (CBP, Mode);
  bits->CBPCM += length;

  /* INTRA_MODE when advanced intra coding mode is used */
  if ((advanced_intra_coding) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
  {
    if (trace)
      fprintf (tf, "INTRA_MODE: ");
    if (pic->Intra_Mode)
    {
      /* length is two bits */
      putbits (2, pic->Intra_Mode);
      bits->INTRA_MODE += 2;
    } else
    {
      /* length is one bit */
      putbits (1, pic->Intra_Mode);
      bits->INTRA_MODE++;
    }
  }

  /* MODB & CBPB */
  if (pic->PB == PB_FRAMES)
  {
    switch (pic->MODB)
    {
      case PBMODE_NORMAL:
        putbits (1, 0);
        bits->MODB += 1;
        break;
      case PBMODE_MVDB:
        putbits (2, 2);
        bits->MODB += 2;
        break;
      case PBMODE_CBPB_MVDB:
        putbits (2, 3);
        bits->MODB += 2;
        /* CBPB */
        putbits (6, CBPB);
        bits->CBPB += 6;
        break;
    }
    if (trace)
    {
      fprintf (tf, "MODB: %d, CBPB: %d\n", pic->MODB, CBPB);
    }
  }

  if (pic->PB == IM_PB_FRAMES)
  {
    switch (pic->MODB)
    {
      case PBMODE_BIDIR_PRED:
        putbits (1, 0);
        bits->MODB += 1;
        break;
      case PBMODE_CBPB_BIDIR_PRED:
        putbits (2, 2);
        bits->MODB += 2;
        /* CBPB */
        putbits (6, CBPB);
        bits->CBPB += 6;
        break;
      case PBMODE_FRW_PRED:
        putbits (3, 6);
        bits->MODB += 3;
        break;
      case PBMODE_CBPB_FRW_PRED:
        putbits (4, 14);
        bits->MODB += 4;
        /* CBPB */
        putbits (6, CBPB);
        bits->CBPB += 6;
        break;
      case PBMODE_BCKW_PRED:
        putbits (5, 30);
        bits->MODB += 5;
        break;
      case PBMODE_CBPB_BCKW_PRED:
        putbits (5, 31);
        bits->MODB += 5;
        /* CBPB */
        putbits (6, CBPB);
        bits->CBPB += 6;
        break;
    }
    if (trace)
    {
      fprintf (tf, "MODB: %d, CBPB: %d\n", pic->MODB, CBPB);
    }
  }

  /* CBPY */
  cbpy = CBP >> 2;
  /* Intra. */
  if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
    cbpy = cbpy ^ 15;
  else if (alternative_inter_vlc && ((CBP & 3) == 3)) 
    cbpy = cbpy ^ 15;
  if (trace)
  {
    fprintf (tf, "CBPY (CBP=%d) (cbpy=%d): ", CBP, cbpy);
  }
  length = put_cbpy (CBP, Mode);
  bits->CBPY += length;

  /* DQUANT */
  if ((Mode == MODE_INTER_Q) || (Mode == MODE_INTRA_Q) 
    || (Mode == MODE_INTER4V_Q))
  {
    if (trace)
    {
      fprintf (tf, "DQUANT: %d\n ",pic->DQUANT);
    }
    if (!modified_quantization)
    {
      switch (pic->DQUANT)
      {
        case -1:
          putbits (2, 0);
          break;
        case -2:
          putbits (2, 1);
          break;
        case 1:
          putbits (2, 2);
          break;
        case 2:
          putbits (2, 3);
          break;
        default:
          fprintf (stderr, "Invalid DQUANT\n");
          exit (-1);
      }
      bits->DQUANT += 2;
    }
    else
    {
      /* modified quantization mode, dquant take any value between 0 and
       * 31 */
      if (pic->dquant_size == 2)
      {
        putbits (2, pic->DQUANT);
        bits->DQUANT += 2;
      } else
      {
        /* DQUANT will be coded into 6 bits */
        putbits (1, 0);
        putbits (5, pic->DQUANT);
        bits->DQUANT += 6;
      }
    }
  }
  return;
}

/**********************************************************************
 *
 *	Name:         CountBitsScalMB
 *	Description:  counts bits used for EI, EP, and B MB info
 *
 *	Input:	      Mode, COD, CBP, Picture and Bits structures
 *	Returns:
 *	Side effects:
 *
 *	Date: 970831  Author: Michael Gallant <mikeg@ee.ubc.ca>
 *
 ***********************************************************************/
void CountBitsScalMB (int Mode, int COD, int CBP, int CBPB, Pict * pic,
                      Bits * bits, int scalability_prediction_type,
                      int MV_present)
{
  int cbpy, cbpcm=0, length=0, code=0; //SV-XXX: init'ed to 0 to shut up gcc4
  int cbp_present = 0;
  int param = 0;

  UNUSED(CBPB); //SV-XXX: unused param

  /* COD */
  if (trace)
  {
    fprintf (tf, "MB-nr: %d", pic->MB);
    fprintf (tf, "  COD: %d\n", COD);
  }
  putbits (1, COD);
  bits->COD++;

  if (COD)
    return;                     /* not coded */

  /* MBTYPE */
  if (trace)
  {
    fprintf (tf, "MBTYPE: ");
  }

  switch (pic->picture_coding_type)
  {
    case PCT_B:

      param += (CBP) ? 1 : 0;
      param += (pic->DQUANT) ? 1 : 0;

      if (B_INTRA_PREDICTION == scalability_prediction_type)
      {
        param -=1;
      }

      length = true_b_length[scalability_prediction_type][param];
      code = true_b_code[scalability_prediction_type][param];

      break;

    case PCT_EI:

      //SV-XXX: suggest parentheses around + or - inside shift
      length = ei_length[scalability_prediction_type<<(1+((pic->DQUANT)) ? 1:0)][CBP&3]; 
      code = ei_code[scalability_prediction_type<<(1+((pic->DQUANT)) ? 1:0)][CBP&3];

      break;

    case PCT_EP:

      param += (CBP || MV_present) ? 1 : 0;
      param += (pic->DQUANT) ? 1 : 0;

      if (EP_INTRA_PREDICTION == scalability_prediction_type)
      {
        param -=1;
      }

      if (MV_present)
      {
        cbp_present = 1;
      }

      length = ep_length[scalability_prediction_type][param];
      code = ep_code[scalability_prediction_type][param];

      break;

    default:

      break;

  }

  putbits(length, code);

  if ( (PCT_EI != pic->picture_coding_type) && 
       (MODE_INTRA == Mode || MODE_INTRA_Q == Mode || 0 != CBP) )
  {
    cbp_present = 1;
  } 

  /* INTRA_MODE when advanced intra coding mode is used */
  if ((advanced_intra_coding) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
  {
    if (trace)
      fprintf (tf, "INTRA_MODE: ");
    if (pic->Intra_Mode)
    {
      /* length is two bits */
      putbits (2, pic->Intra_Mode);
      bits->INTRA_MODE += 2;
    } 
    else
    {
      /* length is one bit */
      putbits (1, pic->Intra_Mode);
      bits->INTRA_MODE++;
    }
  }

  /* Coded Block Patterns. */
  if (cbp_present)
  {
    cbpcm = (CBP & 3);

    if (trace)
    {
      fprintf (tf, "CBPC: ");
    }
    if (0 == cbpcm)
    {
      putbits (1, 0);
      bits->CBPCM += 1;
    } 
    else if (1 == cbpcm)
    {
      putbits (2, 2);
      bits->CBPCM += 2;
    } 
    else if (2 == cbpcm)
    {
      putbits (3, 7);
      bits->CBPCM += 3;
    } 
    else
    {
      putbits (3, 6);
      bits->CBPCM += 3;
    }
  }

  if (cbp_present || (PCT_EI == pic->picture_coding_type) )
  {
    cbpy = CBP >> 2;

    if (trace)
    {
      fprintf (tf, "CBPY: ");
    }

    if ( (MODE_INTRA == Mode || MODE_INTRA_Q == Mode) ||
         (PCT_EI == pic->picture_coding_type && EI_UPWARD_PREDICTION == scalability_prediction_type) ||
         (PCT_EP == pic->picture_coding_type && 
          (EP_UPWARD_PREDICTION == scalability_prediction_type || 
           EP_BIDIRECTIONAL_PREDICTION == scalability_prediction_type) ) )
    {
      length = put_cbpy (CBP, MODE_INTRA);
      bits->CBPY += length;
    }
    else if (alternative_inter_vlc && (3 == cbpcm) )
    {
      length = put_cbpy (CBP, MODE_INTRA);
      bits->CBPY += length;
    }
    else
    {
      length = put_cbpy (CBP, MODE_INTER);
      bits->CBPY += length;
    }

  }

  /* DQUANT */
  if ((Mode == MODE_INTER_Q) || (Mode == MODE_INTRA_Q) || (Mode == MODE_INTER4V_Q))
  {
    if (trace)
    {
      fprintf (tf, "DQUANT: %d\n ",pic->DQUANT);
    }
    if (!modified_quantization)
    {
      switch (pic->DQUANT)
      {
        case -1:
          putbits (2, 0);
          break;
        case -2:
          putbits (2, 1);
          break;
        case 1:
          putbits (2, 2);
          break;
        case 2:
          putbits (2, 3);
          break;
        default:
          fprintf (stderr, "Invalid DQUANT\n");
          exit (-1);
      }
      bits->DQUANT += 2;
    } 
    else
    {
      /* modified quantization mode, dquant take any value between 0 and
       * 31 */
      if (pic->dquant_size == 2)
      {
        putbits (2, pic->DQUANT);
        bits->DQUANT += 2;
      } 
      else
      {
        /* DQUANT will be coded into 6 bits */
        putbits (1, 0);
        putbits (5, pic->DQUANT);
        bits->DQUANT += 6;
      }
    }
  }
  return;
}

/**********************************************************************
 *
 *      Name:           Count_sac_BitsMB
 *      Description:    counts bits used for MB info using SAC models
 *                      modified from CountBitsMB
 *
 *      Input:          Mode, COD, CBP, Picture and Bits structures
 *      Returns:	none
 *      Side effects:	Updates Bits structure.
 *
 *      Author:        pmulroy@visual.bt.co.uk
 *
 ***********************************************************************/

void Count_sac_BitsMB (int Mode, int COD, int CBP, int CBPB, Pict * pic, Bits * bits)
{
  int cbpy, cbpcm, length, i;

  arith_used = 1;

  /* COD */

  if (trace)
  {
    fprintf (tf, "MB-nr: %d\n", pic->MB);
    if (pic->picture_coding_type == PCT_INTER || pic->picture_coding_type == PCT_IPB)
      fprintf (tf, "  COD: %d ", COD);
  }
  if (pic->picture_coding_type == PCT_INTER || pic->picture_coding_type == PCT_IPB)
    bits->COD += AR_Encode (COD, sed_cumf_COD);

  if (COD)
    return;                     /* not coded */

  /* CBPCM */

  cbpcm = Mode | ((CBP & 3) << 4);
  if (trace)
  {
    fprintf (tf, "CBPCM (CBP=%d) (cbpcm=%d): ", CBP, cbpcm);
  }
  if (pic->picture_coding_type == PCT_INTRA)
    length = AR_Encode (indexfn (cbpcm, sed_mcbpc_intratab, 9), sed_cumf_MCBPC_intra);
  else if (EPTYPE)
    length = AR_Encode (indexfn (cbpcm, sed_sed_mcbpctab_4mvq, 25), sed_cumf_MCBPC_4MVQ);
  else
    length = AR_Encode (indexfn (cbpcm, sed_mcbpctab, 21), sed_cumf_MCBPC_no4MVQ);
  bits->CBPCM += length;

  /* INTRA_MODE when advanced intra coding mode is used */
  if (advanced_intra_coding && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
  {
    if (trace)
      fprintf (tf, "INTRA_MODE: ");
    switch (pic->Intra_Mode)
    {
      case INTRA_MODE_DC:
        bits->INTRA_MODE += AR_Encode (0, sed_cumf_INTRA_AC_DC);
        break;
      case INTRA_MODE_VERT_AC:
        bits->INTRA_MODE += AR_Encode (1, sed_cumf_INTRA_AC_DC);
        break;
      case INTRA_MODE_HORI_AC:
        bits->INTRA_MODE += AR_Encode (2, sed_cumf_INTRA_AC_DC);
    }
  }
  /* MODB & CBPB */
  if (pic->PB == PB_FRAMES)
  {
    switch (pic->MODB)
    {
      case PBMODE_NORMAL:
        bits->MODB += AR_Encode (0, sed_cumf_MODB_G);
        break;
      case PBMODE_MVDB:
        bits->MODB += AR_Encode (1, sed_cumf_MODB_G);
        break;
      case PBMODE_CBPB_MVDB:
        bits->MODB += AR_Encode (2, sed_cumf_MODB_G);
        /* CBPB */
        for (i = 5; i > 1; i--)
          bits->CBPB += AR_Encode (((CBPB & 1 << i) >> i), sed_cumf_YCBPB);
        for (i = 1; i > -1; i--)
          bits->CBPB += AR_Encode (((CBPB & 1 << i) >> i), sed_cumf_UVCBPB);
        break;
    }
    if (trace)
      fprintf (tf, "MODB: %d, CBPB: %d\n", pic->MODB, CBPB);
  }
  if (pic->PB == IM_PB_FRAMES)
  {
    switch (pic->MODB)
    {
      case PBMODE_BIDIR_PRED:
        bits->MODB += AR_Encode (0, sed_cumf_MODB_M);
        break;
      case PBMODE_CBPB_BIDIR_PRED:
        bits->MODB += AR_Encode (1, sed_cumf_MODB_M);
        break;
      case PBMODE_FRW_PRED:
        bits->MODB += AR_Encode (2, sed_cumf_MODB_M);
        break;
      case PBMODE_CBPB_FRW_PRED:
        bits->MODB += AR_Encode (3, sed_cumf_MODB_M);
        break;
      case PBMODE_BCKW_PRED:
        bits->MODB += AR_Encode (4, sed_cumf_MODB_M);
        break;
      case PBMODE_CBPB_BCKW_PRED:
        bits->MODB += AR_Encode (5, sed_cumf_MODB_M);
        break;
    }

    if (pic->MODB==PBMODE_CBPB_BIDIR_PRED || pic->MODB==PBMODE_CBPB_FRW_PRED ||
pic->MODB==PBMODE_CBPB_BCKW_PRED)
    {
        /* CBPB */
        for (i = 5; i > 1; i--)
          bits->CBPB += AR_Encode (((CBPB & 1 << i) >> i), sed_cumf_YCBPB);
        for (i = 1; i > -1; i--)
          bits->CBPB += AR_Encode (((CBPB & 1 << i) >> i), sed_cumf_UVCBPB);
    }

    if (trace)
    {
      fprintf (tf, "MODB: %d, CBPB: %d\n", pic->MODB, CBPB);
    }
  }

  /* CBPY */

  cbpy = CBP >> 2;
  if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
  {                             /* Intra */
    length = AR_Encode (indexfn (cbpy, sed_cbpy_intratab, 16), sed_sed_cumf_CBPY_intra);
  } else
  {
    length = AR_Encode (indexfn (cbpy, sed_cbpytab, 16), sed_cumf_CBPY);
  }
  if (trace)
  {
    fprintf (tf, "CBPY (CBP=%d) (cbpy=%d): ", CBP, cbpy);
  }
  bits->CBPY += length;

  /* DQUANT */
  if ((Mode == MODE_INTER_Q) || (Mode == MODE_INTRA_Q)
      || (Mode == MODE_INTER4V_Q))
  {
    if (trace)
    {
      fprintf (tf, "DQUANT: %d\n ",pic->DQUANT);
    }
    bits->DQUANT += AR_Encode (indexfn (pic->DQUANT + 2, sed_dquanttab, 4), sed_cumf_DQUANT);
  }
  return;
}


/**********************************************************************
 *
 *	Name:        CountBitsSlice
 *	Description:    counts bits used for slice (GOB) info
 *
 *	Input:	        slice no., quantizer
 *
 *	Date: 94????	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

int CountBitsSlice (int slice, int quant)
{
  int bits = 0;

  if (arith_used)
  {
    bits += encoder_flush ();   /* Need to call before fixed length string
                                 * output */
    arith_used = 0;
  }
  /* Picture Start Code */
  if (trace)
    fprintf (tf, "GOB sync (GBSC): ");
  putbits (PSC_LENGTH, PSC);    /* PSC */
  bits += PSC_LENGTH;

  /* Group Number */
  if (trace)
    fprintf (tf, "GN: ");
  putbits (5, slice);
  bits += 5;

  /* GOB Sub Bitstream Indicator */
  /* if CPM == 1: read 2 bits GSBI */
  /* not supported in this version */

  /* GOB Frame ID */
  if (trace)
    fprintf (tf, "GFID: ");
  putbits (2, 0);
  /* NB: in error-prone environments this value should change if PTYPE in
   * picture header changes. In this version of the encoder PTYPE only
   * changes when PB-frames are used in the following cases: (i) after the
   * first intra frame (ii) if the distance between two P-frames is very
   * large Therefore I haven't implemented this GFID change */
  /* GFID is not allowed to change unless PTYPE changes */
  bits += 2;

  /* Gquant */
  if (trace)
    fprintf (tf, "GQUANT: ");
  putbits(5,quant);

  bits += 5;

  return bits;
}


/**********************************************************************
 *
 *	Name:        CountBitsCoeff
 *	Description:	counts bits used for coeffs
 *
 *	Input:        qcoeff, coding mode CBP, bits structure, no. of
 *                      coeffs
 *
 *	Returns:	struct with no. of bits used
 *	Side effects:
 *
 *	Date: 940111	Author:	Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void CountBitsCoeff (int *qcoeff, int Mode, int CBP, Bits * bits, int ncoeffs)
{

  int i;

  if ((Mode == MODE_INTRA || Mode == MODE_INTRA_Q) && !(advanced_intra_coding))
  {
    for (i = 0; i < 4; i++)
    {
      bits->Y += CodeCoeff (Mode, qcoeff, i, ncoeffs);
    }
    for (i = 4; i < 6; i++)
    {
      bits->C += CodeCoeff (Mode, qcoeff, i, ncoeffs);
    }
  } else
  {
    for (i = 0; i < 4; i++)
    {
      if ((i == 0 && CBP & 32) ||
          (i == 1 && CBP & 16) ||
          (i == 2 && CBP & 8) ||
          (i == 3 && CBP & 4) ||
          (i == 4 && CBP & 2) ||
          (i == 5 && CBP & 1))
      {
        bits->Y += CodeCoeff (Mode, qcoeff, i, ncoeffs);
      }
    }
    for (i = 4; i < 6; i++)
    {
      if ((i == 0 && CBP & 32) ||
          (i == 1 && CBP & 16) ||
          (i == 2 && CBP & 8) ||
          (i == 3 && CBP & 4) ||
          (i == 4 && CBP & 2) ||
          (i == 5 && CBP & 1))
      {
        bits->C += CodeCoeff (Mode, qcoeff, i, ncoeffs);
      }
    }
  }
  return;
}

int CodeCoeff (int Mode, int *qcoeff, int block, int ncoeffs)
{
  int j, bits;
  int prev_run, run, prev_level, level, first;
  int prev_s, s, length, use_extended_escape;

  /* Annex S variables */
  int code_intra, code_inter, length_intra = 0, length_inter = 0, run_inter;
  int total_length_inter, total_length_intra;
  int use_intra = 0;

  if (trace)
  {
    fprintf (tf, "Coeffs block %d:\n", block);
  }
  /* Annex S */
  if (alternative_inter_vlc && !(Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
  {                             /* choose the table to use in alternative
                                 * inter VLC mode */
    run = bits = 0;
    first = 1;
    prev_run = prev_level = level = s = prev_s = 0;
    use_intra = 0;
    run_inter = 0;
    total_length_inter = total_length_intra = 0;

    for (j = block * ncoeffs; j < (block + 1) * ncoeffs; j++)
    {

      /* Increment run if coeff is zero */
      if ((level = qcoeff[j]) == 0)
      {
        run++;
      } else
      {
        if (-127 <= level && level < 0)
          level = -level;
        if (!first)
        {

          if (prev_level >= 0 && prev_level < 26 && prev_run < 64)
          {
            if (prev_level < 13)
            {
              show_inter_coeff (prev_run, prev_level, 0, &length_inter, &code_inter);
              if (!(length_inter))
                length_inter = 22;
            } else
              length_inter = 22;
            show_intra_coeff (prev_run, prev_level, 0, &length_intra, &code_intra);
            if (!(length_intra))
              length_intra = 22;
          } else
            length_intra = 22;
          if (modified_quantization && (prev_level < -127 || prev_level > 127))
          {
            length_inter += 11;
            length_intra += 11;
          }
          if (length_intra < 22)
          {
            run_inter += show_inter_run (length_intra, code_intra) + 1;
          } else
          {
            run_inter += prev_run + 1;
          }
          total_length_inter += length_inter;
          total_length_intra += length_intra;

        }
        prev_run = run;
        prev_level = level;
        run = first = 0;
      }
    }
    /* Check the last coeff */
    if (!first)
    {
      if (prev_level >= 0 && prev_level < 26 && prev_run < 64)
      {
        if (prev_level < 13)
        {
          show_inter_coeff (prev_run, prev_level, 1, &length_inter, &code_inter);
          if (!(length_inter))
            length_inter = 22;
        } else
          length_inter = 22;
        show_intra_coeff (prev_run, prev_level, 1, &length_intra, &code_intra);
        if (!(length_intra))
          length_intra = 22;
      } else
        length_intra = 22;
      if (modified_quantization && (prev_level < -127 || prev_level > 127))
      {
        length_inter += 11;
        length_intra += 11;
      }
      if (length_intra < 22)
      {
        run_inter += show_inter_run (length_intra, code_intra) + 1;
      } else
      {
        run_inter += prev_run + 1;
      }
      total_length_inter += length_inter;
      total_length_intra += length_intra;
      if (total_length_intra < total_length_inter && run_inter > 64)
      {
        /* conditions are satisfied to use intra table for inter coeff */
        use_intra = 1;
      }
    }
    if (trace)
    {
      fprintf (tf, "Alternative Inter VLC Table: %s\n", use_intra ? "Intra" : "Inter");
    }
  }
  /* Encode the coefficients */
  run = bits = 0;
  first = 1;
  prev_run = prev_level = level = s = prev_s = 0;

  for (j = block * ncoeffs; j < (block + 1) * ncoeffs; j++)
  {
    /* Do this block's DC-coefficient first (unless advanced intra coding
     * is used) */
    if ((!(j % ncoeffs) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)) && (!advanced_intra_coding))
    {
      /* DC coeff */
      if (trace)
      {
        fprintf (tf, "DC: ");
      }
      if (qcoeff[block * ncoeffs] != 128)
        putbits (8, qcoeff[block * ncoeffs]);
      else
        putbits (8, 255);
      bits += 8;
    } else
    {
      /* AC coeff (or DC/AC in advanced intra coding mode) */
      s = 0;
      /* Increment run if coeff is zero */
      if ((level = qcoeff[j]) == 0)
      {
        run++;
      } else
      {
        /* code run & level and count bits */
        if (-127 <= level && level < 0)
        {
          s = 1;
          level = -level;
        }
        if (!first)
        {
          /* Encode the previous coefficient */
          //SV-XXX: suggest parentheses around && within ||
          if ((advanced_intra_coding && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
              || (alternative_inter_vlc && !(Mode == MODE_INTRA || Mode == MODE_INTRA_Q) && use_intra))
          {
            if (prev_level >= 0 && prev_level < 26 && prev_run < 64)
            {
              if (trace)
              {
                fprintf (tf, "run: %2i  level: %3i  code: ", prev_run, prev_s ? -(prev_level) : prev_level);
              }
              length = put_intra_coeff (prev_run, prev_level, 0);
            } else
            {
              length = 0;
            }
          } else
          {
            if (prev_level >= 0 && prev_level < 13 && prev_run < 64)
            {
              if (trace)
              {
                fprintf (tf, "run: %2i  level: %3i  code: ", prev_run, prev_s ? -(prev_level) : prev_level);
              }
              length = put_coeff (prev_run, prev_level, 0);
            } else
              length = 0;
          }
          if (length == 0)
          {                     /* Escape coding */
            if (trace)
            {
              fprintf (tf, "Escape code: ");
            }
            if (!modified_quantization || (prev_level >= -127 && prev_level <= 127))
            {
              use_extended_escape = 0;
              if (prev_s == 1)
              {
                prev_level = (prev_level ^ 0xffff) + 1;
              }
            } else
              use_extended_escape = 1;
            putbits (7, 3);     /* Escape code */
            if (trace)
              fprintf (tf, "last(0): ");
            putbits (1, 0);
            if (trace)
              fprintf (tf, "run(%4d): ", prev_run);
            putbits (6, prev_run);

            if (!use_extended_escape)
            {
              if (trace)
                fprintf (tf, "level(%4d): ", prev_level);
              putbits (8, prev_level);
              bits += 22;
            } else
            {                   /* use extended escape code to code
                                 * coefficient that are outside the range
                                 * of -127, +127 */
              if (trace)
              {
                fprintf (tf, "Extended Escape code: ");
              }
              putbits (8, 0x80);/* extended escape code */
              if (trace)
                fprintf (tf, "extended level: ");
              putbits (11, (prev_level >> 5 & 0x003F) | (prev_level << 6));
              bits += 33;
            }
          } else
          {
            putbits (1, prev_s);
            bits += length + 1;
          }
        }
        prev_run = run;
        prev_s = s;
        prev_level = level;
        run = first = 0;
      }
    }
  }

  /* Encode the last coeff */
  if (!first)
  {
    if (trace)
    {
      fprintf (tf, "Last coeff: ");
    }
    //SV-XXX: suggest parentheses around && within ||
    if ((advanced_intra_coding && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
        || (alternative_inter_vlc && !(Mode == MODE_INTRA || Mode == MODE_INTRA_Q) && use_intra))
    {
      if (prev_level >= 0 && prev_level < 26 && prev_run < 64)
      {
        if (trace)
        {
          fprintf (tf, "run: %2i  level: %3i  code: ", prev_run, s ? -(prev_level) : prev_level);
        }
        length = put_intra_coeff (prev_run, prev_level, 1);
      } else
        length = 0;
    } else
    {
      if (prev_level >= 0 && prev_level < 13 && prev_run < 64)
      {
        if (trace)
        {
          fprintf (tf, "run: %2i  level: %3i  code: ", prev_run, s ? -(prev_level) : prev_level);
        }
        length = put_coeff (prev_run, prev_level, 1);
      } else
        length = 0;
    }
    if (length == 0)
    {                           /* Escape coding */
      if (trace)
      {
        fprintf (tf, "Escape code: ");
      }
      if (!modified_quantization || (prev_level >= -127 && prev_level <= 127))
      {
        use_extended_escape = 0;
        if (prev_s == 1)
        {
          prev_level = (prev_level ^ 0xffff) + 1;
        }
      } else
        use_extended_escape = 1;

      putbits (7, 3);           /* Escape code */
      if (trace)
        fprintf (tf, "last(1): ");
      putbits (1, 1);
      if (trace)
        fprintf (tf, "run(%4d): ", prev_run);
      putbits (6, prev_run);

      if (!use_extended_escape)
      {
        if (trace)
          fprintf (tf, "level(%4d): ", prev_level);
        putbits (8, prev_level);
        bits += 22;
      } else
      {                         /* use extended escape code to code
                                 * coefficient that are outside the range
                                 * of -127, +127 */
        if (trace)
        {
          fprintf (tf, "Extended Escape code: ");
        }
        putbits (8, 0x80);      /* extended escape code */
        if (trace)
          fprintf (tf, "extended level: ");
        putbits (11, (prev_level >> 5 & 0x003F) || (prev_level << 6));
        bits += 33;
      }
    } else
    {
      putbits (1, prev_s);
      bits += length + 1;
    }
  }
  return bits;
}


/**********************************************************************
 *
 *      Name:           Count_sac_BitsCoeff
 *                      counts bits using SAC models
 *
 *      Input:          qcoeff, coding mode CBP, bits structure, no. of
 *                      coeffs
 *
 *      Returns:        struct with no. of bits used
 *      Side effects:
 *
 *      Author:        pmulroy@visual.bt.co.uk
 *
 ***********************************************************************/

void Count_sac_BitsCoeff (int *qcoeff, int Mode, int CBP, Bits * bits, int ncoeffs)
{

  int i;

  arith_used = 1;

  if ((Mode == MODE_INTRA || Mode == MODE_INTRA_Q) && !advanced_intra_coding)
  {
    for (i = 0; i < 4; i++)
    {
      bits->Y += Code_sac_Coeff (Mode, qcoeff, i, ncoeffs);
    }
    for (i = 4; i < 6; i++)
    {
      bits->C += Code_sac_Coeff (Mode, qcoeff, i, ncoeffs);
    }
  } else
  {
    for (i = 0; i < 4; i++)
    {
      if ((i == 0 && CBP & 32) ||
          (i == 1 && CBP & 16) ||
          (i == 2 && CBP & 8) ||
          (i == 3 && CBP & 4) ||
          (i == 4 && CBP & 2) ||
          (i == 5 && CBP & 1))
      {
        bits->Y += Code_sac_Coeff (Mode, qcoeff, i, ncoeffs);
      }
    }
    for (i = 4; i < 6; i++)
    {
      if ((i == 0 && CBP & 32) ||
          (i == 1 && CBP & 16) ||
          (i == 2 && CBP & 8) ||
          (i == 3 && CBP & 4) ||
          (i == 4 && CBP & 2) ||
          (i == 5 && CBP & 1))
      {
        bits->C += Code_sac_Coeff (Mode, qcoeff, i, ncoeffs);
      }
    }
  }
  return;
}

int Code_sac_Coeff (int Mode, int *qcoeff, int block, int ncoeffs)
{
  int j, bits, mod_index, intra;
  int prev_run, run, prev_level, level, first, prev_position, position;
  int prev_ind, ind, prev_s, s, length;

  run = bits = 0;
  first = 1;
  position = 0;
  intra = 0;

  level = s = ind = 0;
  prev_run = prev_level = prev_ind = prev_s = prev_position = 0;

  /* SAC table for Coeff in advanced intra coding not specified */
  /* use same tables and encode DC as AC                        */

  intra = (Mode == MODE_INTRA || Mode == MODE_INTRA_Q);

  for (j = block * ncoeffs; j < (block + 1) * ncoeffs; j++)
  {

    if ((!(j % ncoeffs) && (intra && !(advanced_intra_coding))))
    {
      if (trace)
      {
        fprintf (tf, "DC: ");
      }
      if (qcoeff[block * ncoeffs] != 128)
        mod_index = indexfn (qcoeff[block * ncoeffs], sed_intradctab, 254);
      else
        mod_index = indexfn (255, sed_intradctab, 254);
      bits += AR_Encode (mod_index, sed_cumf_INTRADC);
    } else
    {

      s = 0;
      /* Increment run if coeff is zero */
      if ((level = qcoeff[j]) == 0)
      {
        run++;
      } else
      {
        /* code run & level and count bits */
        if (level < 0)
        {
          s = 1;
          level = -level;
        }
        ind = level | run << 4;
        ind = ind | 0 << 12;    /* Not last coeff */
        position++;

        if (!first)
        {
          mod_index = indexfn (prev_ind, sed_tcoeftab, 103);
          if (prev_level < 13 && prev_run < 64)
            length = CodeTCoef (mod_index, prev_position, intra);
          else
            length = -1;

          if (length == -1)
          {                     /* Escape coding */
            if (trace)
            {
              fprintf (tf, "Escape coding:\n");
            }
            if (prev_s == 1)
            {
              prev_level = (prev_level ^ 0xff) + 1;
            }
            mod_index = indexfn (ESCAPE, sed_tcoeftab, 103);
            bits += CodeTCoef (mod_index, prev_position, intra);

            if (intra)
              bits += AR_Encode (indexfn (0, sed_lasttab, 2), sed_sed_cumf_LAST_intra);
            else
              bits += AR_Encode (indexfn (0, sed_lasttab, 2), sed_cumf_LAST);

            if (intra)
              bits += AR_Encode (indexfn (prev_run, sed_runtab, 64), sed_sed_cumf_RUN_intra);
            else
              bits += AR_Encode (indexfn (prev_run, sed_runtab, 64), sed_cumf_RUN);

            if (intra)
              bits += AR_Encode (indexfn (prev_level, sed_leveltab, 254),
                                 sed_sed_cumf_LEVEL_intra);
            else
              bits += AR_Encode (indexfn (prev_level, sed_leveltab, 254),
                                 sed_cumf_LEVEL);

          } else
          {
            bits += AR_Encode (indexfn (prev_s, sed_signtab, 2), sed_cumf_SIGN);
            bits += length;
          }
        }
        prev_run = run;
        prev_s = s;
        prev_level = level;
        prev_ind = ind;
        prev_position = position;

        run = first = 0;

      }
    }
  }

  /* Encode Last Coefficient */

  if (!first)
  {
    if (trace)
    {
      fprintf (tf, "Last coeff: ");
    }
    prev_ind = prev_ind | 1 << 12;  /* last coeff */
    mod_index = indexfn (prev_ind, sed_tcoeftab, 103);

    if (prev_level < 13 && prev_run < 64)
      length = CodeTCoef (mod_index, prev_position, intra);
    else
      length = -1;

    if (length == -1)
    {                           /* Escape coding */
      if (trace)
      {
        fprintf (tf, "Escape coding:\n");
      }
      if (prev_s == 1)
      {
        prev_level = (prev_level ^ 0xff) + 1;
      }
      mod_index = indexfn (ESCAPE, sed_tcoeftab, 103);
      bits += CodeTCoef (mod_index, prev_position, intra);

      if (intra)
        bits += AR_Encode (indexfn (1, sed_lasttab, 2), sed_sed_cumf_LAST_intra);
      else
        bits += AR_Encode (indexfn (1, sed_lasttab, 2), sed_cumf_LAST);

      if (intra)
        bits += AR_Encode (indexfn (prev_run, sed_runtab, 64), sed_sed_cumf_RUN_intra);
      else
        bits += AR_Encode (indexfn (prev_run, sed_runtab, 64), sed_cumf_RUN);

      if (intra)
        bits += AR_Encode (indexfn (prev_level, sed_leveltab, 254), sed_sed_cumf_LEVEL_intra);
      else
        bits += AR_Encode (indexfn (prev_level, sed_leveltab, 254), sed_cumf_LEVEL);
    } else
    {
      bits += AR_Encode (indexfn (prev_s, sed_signtab, 2), sed_cumf_SIGN);
      bits += length;
    }
  }                             /* last coeff */
  return bits;
}

/*********************************************************************
 *
 *      Name:           CodeTCoef
 *
 *      Description:    Encodes an AC Coefficient using the
 *                      relevant SAC model.
 *
 *      Input:          Model index, position in DCT block and intra/
 *        inter flag.
 *
 *      Returns:        Number of bits used.
 *
 *      Side Effects:   None
 *
 *      Author:         pmulroy@visual.bt.co.uk
 *
 *********************************************************************/

int CodeTCoef (int mod_index, int position, int intra)
{
  int length;

  switch (position)
  {
    case 1:
      {
        if (intra)
          length = AR_Encode (mod_index, sed_sed_cumf_TCOEF1_intra);
        else
          length = AR_Encode (mod_index, sed_cumf_TCOEF1);
        break;
      }
    case 2:
      {
        if (intra)
          length = AR_Encode (mod_index, sed_sed_cumf_TCOEF2_intra);
        else
          length = AR_Encode (mod_index, sed_cumf_TCOEF2);
        break;
      }
    case 3:
      {
        if (intra)
          length = AR_Encode (mod_index, sed_sed_cumf_TCOEF3_intra);
        else
          length = AR_Encode (mod_index, sed_cumf_TCOEF3);
        break;
      }
    default:
      {
        if (intra)
          length = AR_Encode (mod_index, sed_sed_cumf_TCOEFr_intra);
        else
          length = AR_Encode (mod_index, sed_cumf_TCOEFr);
        break;
      }
  }

  return length;
}

/**********************************************************************
 *
 *	Name:        FindCBP
 *	Description:	Finds the CBP for a macroblock
 *
 *	Input:        qcoeff and mode
 *
 *	Returns:	CBP
 *	Side effects:
 *
 *	Date: 940829	Author:	Karl.Lillevold@nta.no
 *
 ***********************************************************************/


int FindCBP (int *qcoeff, int Mode, int ncoeffs)
{

  int i, j;
  int CBP = 0;
  int intra = ((Mode == MODE_INTRA || Mode == MODE_INTRA_Q) && (!advanced_intra_coding));

  /* Set CBP for this Macroblock */
  for (i = 0; i < 6; i++)
  {
    for (j = i * ncoeffs + intra; j < (i + 1) * ncoeffs; j++)
    {
      if (qcoeff[j])
      {
        if (i == 0)
        {
          CBP |= 32;
        } else if (i == 1)
        {
          CBP |= 16;
        } else if (i == 2)
        {
          CBP |= 8;
        } else if (i == 3)
        {
          CBP |= 4;
        } else if (i == 4)
        {
          CBP |= 2;
        } else if (i == 5)
        {
          CBP |= 1;
        } else
        {
          fprintf (stderr, "Error in CBP assignment\n");
          exit (-1);
        }
        break;
      }
    }
  }

  return CBP;
}


/**********************************************************************
 *
 *	Name:           CountBitsVectors
 *	Description:	counts bits used for MVs and writes to bitstream
 *
 *	Input:          MV struct, bits struct, picture struct,
 *                      position, mode, annex O prediction type,
 *
 *	Returns:
 *	Side effects:
 *
 *	Date: 940111	Author:	???
 *            970831    Added support for annex O, true b pictures,
 *                      modified by mikeg@ee.ubc.ca
 *
 ***********************************************************************/

void CountBitsVectors (MotionVector * MV[7][MBR + 1][MBC + 2], Bits * bits,
                        int x, int y, int Mode, int newgob, Pict * pic,
                        int scalability_prediction_type)
{
  int y_vec, x_vec;
  int pmv0, pmv1;
  int start, stop, block=0, blockb;//SV-XXX: inited to 0
  int y_vecb, x_vecb;
  int pmv0b, pmv1b;

  x++;
  y++;

  if (PCT_B != pic->picture_coding_type)
  {
    if (Mode == MODE_INTER4V || Mode == MODE_INTER4V_Q)
    {
      start = 1;
      stop = 4;
    } else
    {
      start = 0;
      stop = 0;
    }

    for (block = start; block <= stop; block++)
    {
      FindPMV (MV, x, y, &pmv0, &pmv1, block, newgob, 1);

      x_vec = (2 * MV[block][y][x]->x + MV[block][y][x]->x_half) - pmv0;
      y_vec = (2 * MV[block][y][x]->y + MV[block][y][x]->y_half) - pmv1;

      /* When PLUSPTYPE is signalled and Annex D is used, *
       * Reversible motion vector codes are used          */
      if (EPTYPE && long_vectors)
      {
        if (trace)
        {
          fprintf (tf, "Vectors:\n");
        }

        bits->vec += put_rvlc(x_vec);
        bits->vec += put_rvlc(y_vec);


        if (trace)
        {
          fprintf (tf, "(x,y) = (%d,%d) - ",
                   (2 * MV[block][y][x]->x + MV[block][y][x]->x_half),
                   (2 * MV[block][y][x]->y + MV[block][y][x]->y_half));
          fprintf (tf, "(Px,Py) = (%d,%d)\n", pmv0, pmv1);
          fprintf (tf, "(x_diff,y_diff) = (%d,%d)\n", x_vec, y_vec);
        }
        /* prevent start code emulation */
        if (x_vec == 1 && y_vec == 1)
        {
          if (trace)
          {
            fprintf (tf, "Prevent Start Code Emulation bit:\n");
          }
          putbits (1,1);
        }
      }
      else
      {
        if (!long_vectors)
        {
          if (x_vec < -32)
            x_vec += 64;
          else if (x_vec > 31)
            x_vec -= 64;

          if (y_vec < -32)
            y_vec += 64;
          else if (y_vec > 31)
            y_vec -= 64;
        } else
        {
          if (pmv0 < -31 && x_vec < -63)
            x_vec += 64;
          else if (pmv0 > 32 && x_vec > 63)
            x_vec -= 64;
  
          if (pmv1 < -31 && y_vec < -63)
            y_vec += 64;
          else if (pmv1 > 32 && y_vec > 63)
            y_vec -= 64;
        }

        if (trace)
        {
          fprintf (tf, "Vectors:\n");
        }
        if (x_vec < 0)
          x_vec += 64;
        if (y_vec < 0)
          y_vec += 64;

        bits->vec += put_mv (x_vec);
        bits->vec += put_mv (y_vec);

        if (trace)
        {
          if (x_vec > 31)
            x_vec -= 64;
          if (y_vec > 31)
            y_vec -= 64;
          fprintf (tf, "(x,y) = (%d,%d) - ",
                   (2 * MV[block][y][x]->x + MV[block][y][x]->x_half),
                   (2 * MV[block][y][x]->y + MV[block][y][x]->y_half));
          fprintf (tf, "(Px,Py) = (%d,%d)\n", pmv0, pmv1);
          fprintf (tf, "(x_diff,y_diff) = (%d,%d)\n", x_vec, y_vec);
        }
      }
    }
  } 
  else if ((PCT_B == pic->picture_coding_type) && (MODE_INTER == Mode))
  {
    switch (scalability_prediction_type)
    {

      case B_DIRECT_PREDICTION:

        break;

      case B_FORWARD_PREDICTION:

        block = 0;
        FindPMV (MV, x, y, &pmv0, &pmv1, block, newgob, 1);

        x_vec = (2 * MV[block][y][x]->x + MV[block][y][x]->x_half) - pmv0;
        y_vec = (2 * MV[block][y][x]->y + MV[block][y][x]->y_half) - pmv1;

       /* When PLUSPTYPE is signalled and Annex D is used, *
         * Reversible motion vector codes are used          */
        if (EPTYPE && long_vectors)
        {
          if (trace)
          {
            fprintf (tf, "Vectors:\n");
          }

          bits->vec += put_rvlc(x_vec);
          bits->vec += put_rvlc(y_vec);


          if (trace)
          {
             fprintf (tf, "(x,y) = (%d,%d) - ",
                     (2 * MV[block][y][x]->x + MV[block][y][x]->x_half),
                     (2 * MV[block][y][x]->y + MV[block][y][x]->y_half));
            fprintf (tf, "(Px,Py) = (%d,%d)\n", pmv0, pmv1);
            fprintf (tf, "(x_diff,y_diff) = (%d,%d)\n", x_vec, y_vec);
          }

          /* prevent start code emulation */
          if (x_vec == 1 && y_vec == 1)
          {
            if (trace)
            {
              fprintf (tf, "Prevent Start Code Emulation bit:\n");
            }
            putbits (1,1);
          }
        }
        else
        {        
          if (!long_vectors)
          {
            if (x_vec < -32)
              x_vec += 64;
            else if (x_vec > 31)
              x_vec -= 64;

            if (y_vec < -32)
              y_vec += 64;
            else if (y_vec > 31)
              y_vec -= 64;
          } else
          {
            if (pmv0 < -31 && x_vec < -63)
              x_vec += 64;
            else if (pmv0 > 32 && x_vec > 63)
              x_vec -= 64;
 
            if (pmv1 < -31 && y_vec < -63)
              y_vec += 64;
            else if (pmv1 > 32 && y_vec > 63)
              y_vec -= 64;
          }

          if (trace)
          {
            fprintf (tf, "Vectors:\n");
          }
          if (x_vec < 0)
            x_vec += 64;
          if (y_vec < 0)
            y_vec += 64;

          bits->vec += put_mv (x_vec);
          bits->vec += put_mv (y_vec);

          if (trace)
          {
            if (x_vec > 31)
              x_vec -= 64;
            if (y_vec > 31)
              y_vec -= 64;
            fprintf (tf, "Forward (x,y) = (%d,%d) - ",
                     (2 * MV[block][y][x]->x + MV[block][y][x]->x_half),
                     (2 * MV[block][y][x]->y + MV[block][y][x]->y_half));
            fprintf (tf, "(Px,Py) = (%d,%d)\n", pmv0, pmv1);
            fprintf (tf, "(x_diff,y_diff) = (%d,%d)\n", x_vec, y_vec);
          }
        }
        break;

      case B_BACKWARD_PREDICTION:

        blockb = 5;
        FindPMV (MV, x, y, &pmv0b, &pmv1b, blockb, newgob, 1);

        x_vecb = (2 * MV[blockb][y][x]->x + MV[blockb][y][x]->x_half) - pmv0b;
        y_vecb = (2 * MV[blockb][y][x]->y + MV[blockb][y][x]->y_half) - pmv1b;

        /* When PLUSPTYPE is signalled and Annex D is used, *
         * Reversible motion vector codes are used          */
        if (EPTYPE && long_vectors)
        {
          if (trace)
          {
            fprintf (tf, "Vectors:\n");
          }

          bits->vec += put_rvlc(x_vecb);
          bits->vec += put_rvlc(y_vecb);


          if (trace)
          {
            fprintf (tf, "(x,y) = (%d,%d) - ",
                     (2 * MV[blockb][y][x]->x + MV[blockb][y][x]->x_half),
                     (2 * MV[blockb][y][x]->y + MV[blockb][y][x]->y_half));
            fprintf (tf, "(Px,Py) = (%d,%d)\n", pmv0b, pmv1b);
            fprintf (tf, "(x_diff,y_diff) = (%d,%d)\n", x_vecb, y_vecb);
          }
          /* prevent start code emulation */
          if (x_vecb == 1 && y_vecb == 1)
          {
            if (trace)
            {
              fprintf (tf, "Prevent Start Code Emulation bit:\n");
            }
            putbits (1,1);
          }
        }
        else
        {
          if (!long_vectors)
          {
            if (x_vecb < -32)
              x_vecb += 64;
            else if (x_vecb > 31)
              x_vecb -= 64;

            if (y_vecb < -32)
              y_vecb += 64;
            else if (y_vecb > 31)
              y_vecb -= 64;
          } else
          {
            if (pmv0b < -31 && x_vecb < -63)
              x_vecb += 64;
            else if (pmv0b > 32 && x_vecb > 63)
              x_vecb -= 64;

            if (pmv1b < -31 && y_vecb < -63)
              y_vecb += 64;
            else if (pmv1b > 32 && y_vecb > 63)
              y_vecb -= 64;
          }
 
          if (trace)
          {
            fprintf (tf, "Vectors:\n");
          }
          if (x_vecb < 0)
            x_vecb += 64;
          if (y_vecb < 0)
            y_vecb += 64;

          bits->vec += put_mv (x_vecb);
          bits->vec += put_mv (y_vecb);
  
          if (trace)
          {
            if (x_vecb > 31)
              x_vecb -= 64;
            if (y_vecb > 31)
              y_vecb -= 64;
            fprintf (tf, "Backward (x,y) = (%d,%d) - ",
                     (2 * MV[blockb][y][x]->x + MV[blockb][y][x]->x_half),
                     (2 * MV[blockb][y][x]->y + MV[blockb][y][x]->y_half));
            fprintf (tf, "(Px,Py) = (%d,%d)\n", pmv0b, pmv1b);
            fprintf (tf, "(x_diff,y_diff) = (%d,%d)\n", x_vecb, y_vecb);
          }
        }
        break;

      case B_BIDIRECTIONAL_PREDICTION:

        block = 0;
        FindPMV (MV, x, y, &pmv0, &pmv1, block, newgob, 1);
        x_vec = (2 * MV[block][y][x]->x + MV[block][y][x]->x_half) - pmv0;
        y_vec = (2 * MV[block][y][x]->y + MV[block][y][x]->y_half) - pmv1;

        blockb = 5;
        FindPMV (MV, x, y, &pmv0b, &pmv1b, blockb, newgob, 1);
        x_vecb = (2 * MV[blockb][y][x]->x + MV[blockb][y][x]->x_half) - pmv0b;
        y_vecb = (2 * MV[blockb][y][x]->y + MV[blockb][y][x]->y_half) - pmv1b;

        /* When PLUSPTYPE is signalled and Annex D is used, *
         * Reversible motion vector codes are used          */
        if (EPTYPE && long_vectors)
        {
          if (trace)
          {
            fprintf (tf, "Vectors:\n");
          }

          bits->vec += put_rvlc(x_vec);
          bits->vec += put_rvlc(y_vec);

          /* prevent start code emulation */
          if (x_vec == 1 && y_vec == 1)
          {
            if (trace)
            {
              fprintf (tf, "Prevent Start Code Emulation bit:\n");
            }
            putbits (1,1);
          }
          /* Backward Vectors */
          bits->vec += put_rvlc(x_vecb);
          bits->vec += put_rvlc(y_vecb);

          /* prevent start code emulation */
          if (x_vecb == 1 && y_vecb == 1)
          {
            if (trace)
            {
              fprintf (tf, "Prevent Start Code Emulation bit:\n");
            }
            putbits (1,1);
          }
          if (trace)
          {
            fprintf (tf, "Forward (x,y) = (%d,%d) - ",
                     (2 * MV[block][y][x]->x + MV[block][y][x]->x_half),
                     (2 * MV[block][y][x]->y + MV[block][y][x]->y_half));
            fprintf (tf, "(Px,Py) = (%d,%d)\n", pmv0, pmv1);
            fprintf (tf, "(x_diff,y_diff) = (%d,%d)\n", x_vec, y_vec);
            fprintf (tf, "Backward (x,y) = (%d,%d) - ",
                     (2 * MV[blockb][y][x]->x + MV[blockb][y][x]->x_half),
                     (2 * MV[blockb][y][x]->y + MV[blockb][y][x]->y_half));
            fprintf (tf, "(Px,Py) = (%d,%d)\n", pmv0b, pmv1b);
            fprintf (tf, "(x_diff,y_diff) = (%d,%d)\n", x_vecb, y_vecb);
          }


        }
        else
        {
          if (!long_vectors)
          {
            if (x_vec < -32)
              x_vec += 64;
            else if (x_vec > 31)
              x_vec -= 64;

            if (y_vec < -32)
              y_vec += 64;
            else if (y_vec > 31)
              y_vec -= 64;
 
            if (x_vecb < -32)
              x_vecb += 64;
            else if (x_vecb > 31)
              x_vecb -= 64;

            if (y_vecb < -32)
              y_vecb += 64;
            else if (y_vecb > 31)
              y_vecb -= 64;
          } else
          {
            if (pmv0 < -31 && x_vec < -63)
              x_vec += 64;
            else if (pmv0 > 32 && x_vec > 63)
              x_vec -= 64;

            if (pmv1 < -31 && y_vec < -63)
              y_vec += 64;
            else if (pmv1 > 32 && y_vec > 63)
              y_vec -= 64;
 
            if (pmv0b < -31 && x_vecb < -63)
              x_vecb += 64;
            else if (pmv0b > 32 && x_vecb > 63)
              x_vecb -= 64;
  
            if (pmv1b < -31 && y_vecb < -63)
              y_vecb += 64;
            else if (pmv1b > 32 && y_vecb > 63)
              y_vecb -= 64;
          }

          if (trace)
          {
            fprintf (tf, "Vectors:\n");
          }
          if (x_vec < 0)
            x_vec += 64;
          if (y_vec < 0)
            y_vec += 64;
          if (x_vecb < 0)
            x_vecb += 64;
          if (y_vecb < 0)
            y_vecb += 64;

          bits->vec += put_mv (x_vec);
          bits->vec += put_mv (y_vec);
          bits->vec += put_mv (x_vecb);
          bits->vec += put_mv (y_vecb);
 
          if (trace)
          {
            if (x_vec > 31)
              x_vec -= 64;
            if (y_vec > 31)
              y_vec -= 64;
            if (x_vecb > 31)
              x_vecb -= 64;
            if (y_vecb > 31)
              y_vecb -= 64;
            fprintf (tf, "Forward (x,y) = (%d,%d) - ",
                     (2 * MV[block][y][x]->x + MV[block][y][x]->x_half),
                     (2 * MV[block][y][x]->y + MV[block][y][x]->y_half));
            fprintf (tf, "(Px,Py) = (%d,%d)\n", pmv0, pmv1);
            fprintf (tf, "(x_diff,y_diff) = (%d,%d)\n", x_vec, y_vec);
            fprintf (tf, "Backward (x,y) = (%d,%d) - ",
                     (2 * MV[blockb][y][x]->x + MV[blockb][y][x]->x_half),
                     (2 * MV[blockb][y][x]->y + MV[blockb][y][x]->y_half));
            fprintf (tf, "(Px,Py) = (%d,%d)\n", pmv0b, pmv1b);
            fprintf (tf, "(x_diff,y_diff) = (%d,%d)\n", x_vecb, y_vecb);
          }
        }
        break;

      default:

        fprintf (stderr, "Illegal scalable prediction type in CountBitsVectors (countbit.c)\n");
        exit (-1);
        break;
    }
  }
  /* PB-frames delta vectors */
  if ((pic->PB == PB_FRAMES &&
      (pic->MODB == PBMODE_MVDB || pic->MODB == PBMODE_CBPB_MVDB)) ||
      (pic->PB == IM_PB_FRAMES &&
      (pic->MODB == PBMODE_FRW_PRED || pic->MODB == PBMODE_CBPB_FRW_PRED)))
  {
    if (pic->PB == PB_FRAMES && (pic->MODB == PBMODE_MVDB ||
                                 pic->MODB == PBMODE_CBPB_MVDB))
    {
      /* x_vec and y_vec are the PB-delta vectors */
      x_vec = MV[6][y][x]->x;
      y_vec = MV[6][y][x]->y;
    } else
    {
      /* x_vec and y_vec are the forward motion vectors for B picture in
       * Impreved PB frames mode. */

      if (newgob)
      {
        /* no predictors */
        x_vec = 2 * MV[6][y][x]->x + MV[6][y][x]->x_half;
        y_vec = 2 * MV[6][y][x]->y + MV[6][y][x]->y_half;

      } else
      {

        /* find prediction vectors */
        /* this would work even though the previous macroblock is
         * predicted bidirectionally. In that case the motion vector will
         * be ZERO vector */

        pmv0 = (2 * MV[6][y][x - 1]->x + MV[6][y][x - 1]->x_half);
        pmv1 = (2 * MV[6][y][x - 1]->y + MV[6][y][x - 1]->y_half);

        x_vec = (2 * MV[6][y][x]->x + MV[6][y][x]->x_half) - pmv0;
        y_vec = (2 * MV[6][y][x]->y + MV[6][y][x]->y_half) - pmv1;
      }
    }
    /* When PLUSPTYPE is signalled and Annex D is used, *
     * Reversible motion vector codes are used          */
    if (EPTYPE && long_vectors)
    {
      if (trace)
      {
        if (pic->PB == PB_FRAMES)
          fprintf (tf, "PB delta vectors:\n");
        if (pic->PB == IM_PB_FRAMES)
          fprintf (tf, "PB ,B forward vectors:\n");
      }

      bits->vec += put_rvlc(x_vec);
      bits->vec += put_rvlc(y_vec);

      if (trace)
      {
        fprintf (tf, "(x,y) = (%d,%d) - ",
                 (2 * MV[block][y][x]->x + MV[block][y][x]->x_half),
                 (2 * MV[block][y][x]->y + MV[block][y][x]->y_half));
        fprintf (tf, "(Px,Py) = (%d,%d)\n", pmv0, pmv1);
        fprintf (tf, "(x_diff,y_diff) = (%d,%d)\n", x_vec, y_vec);
      }

      /* prevent start code emulation */
      if (x_vec == 1 && y_vec == 1)
      {
        if (trace)
        {
          fprintf (tf, "Prevent Start Code Emulation bit:\n");
        }
        putbits (1,1);
      }
    }
    else
    {
      if (x_vec < -32)
        x_vec += 64;
      else if (x_vec > 31)
        x_vec -= 64;
      if (y_vec < -32)
        y_vec += 64;
      else if (y_vec > 31)
        y_vec -= 64;

      if (x_vec < 0)
        x_vec += 64;
      if (y_vec < 0)
        y_vec += 64;

      if (trace)
      {
        if (pic->PB == PB_FRAMES)
          fprintf (tf, "PB delta vectors:\n");
        if (pic->PB == IM_PB_FRAMES)
          fprintf (tf, "PB ,B forward vectors:\n");
      }
      bits->vec += put_mv (x_vec);
      bits->vec += put_mv (y_vec);

      if (trace)
      {
        if (x_vec > 31)
          x_vec -= 64;
        if (y_vec > 31)
          y_vec -= 64;
        fprintf (tf, "MVDB (x,y) = (%d,%d)\n", x_vec, y_vec);
      }
    }
  }
  return;
}

void Count_sac_BitsVectors (MotionVector * MV[7][MBR + 1][MBC + 2], Bits * bits,
                          int x, int y, int Mode, int newgob, Pict * pic)
{
  int y_vec, x_vec;
  int pmv0, pmv1;
  int start, stop, block;

  arith_used = 1;
  x++;
  y++;

  if (Mode == MODE_INTER4V || Mode == MODE_INTER4V_Q)
  {
    start = 1;
    stop = 4;
  } else
  {
    start = 0;
    stop = 0;
  }

  for (block = start; block <= stop; block++)
  {

    FindPMV (MV, x, y, &pmv0, &pmv1, block, newgob, 1);

    x_vec = (2 * MV[block][y][x]->x + MV[block][y][x]->x_half) - pmv0;
    y_vec = (2 * MV[block][y][x]->y + MV[block][y][x]->y_half) - pmv1;

    if (!long_vectors)
    {
      if (x_vec < -32)
        x_vec += 64;
      else if (x_vec > 31)
        x_vec -= 64;

      if (y_vec < -32)
        y_vec += 64;
      else if (y_vec > 31)
        y_vec -= 64;
    } else
    {
      if (pmv0 < -31 && x_vec < -63)
        x_vec += 64;
      else if (pmv0 > 32 && x_vec > 63)
        x_vec -= 64;

      if (pmv1 < -31 && y_vec < -63)
        y_vec += 64;
      else if (pmv1 > 32 && y_vec > 63)
        y_vec -= 64;
    }

    if (x_vec < 0)
      x_vec += 64;
    if (y_vec < 0)
      y_vec += 64;

    if (trace)
    {
      fprintf (tf, "Vectors:\n");
    }
    bits->vec += AR_Encode (indexfn (x_vec, sed_mvdtab, 64), sed_cumf_MVD);
    bits->vec += AR_Encode (indexfn (y_vec, sed_mvdtab, 64), sed_cumf_MVD);

    if (trace)
    {
      if (x_vec > 31)
        x_vec -= 64;
      if (y_vec > 31)
        y_vec -= 64;
      fprintf (tf, "(x,y) = (%d,%d) - ",
               (2 * MV[block][y][x]->x + MV[block][y][x]->x_half),
               (2 * MV[block][y][x]->y + MV[block][y][x]->y_half));
      fprintf (tf, "(Px,Py) = (%d,%d)\n", pmv0, pmv1);
      fprintf (tf, "(x_diff,y_diff) = (%d,%d)\n", x_vec, y_vec);
    }
  }

  /* PB-frames delta vectors */
  if ((pic->PB == PB_FRAMES &&
       (pic->MODB == PBMODE_MVDB || pic->MODB == PBMODE_CBPB_MVDB)) ||
      (pic->PB == IM_PB_FRAMES &&
    (pic->MODB == PBMODE_FRW_PRED || pic->MODB == PBMODE_CBPB_FRW_PRED)))
  {
    if (pic->PB == PB_FRAMES && (pic->MODB == PBMODE_MVDB ||
                                 pic->MODB == PBMODE_CBPB_MVDB))
    {
      /* x_vec and y_vec are the PB-delta vectors */
      x_vec = MV[6][y][x]->x;
      y_vec = MV[6][y][x]->y;
    }
    else
    {
      /* x_vec and y_vec are the forward motion vectors for B picture in
       * Improved PB frames mode. */

      if (newgob)
      {
        /* no predictors */
        x_vec = 2 * MV[6][y][x]->x + MV[6][y][x]->x_half;
        y_vec = 2 * MV[6][y][x]->y + MV[6][y][x]->y_half;

      } else
      {
        /* find prediction vectors */
        /* this would work even though the previous macroblock is
         * predicted bidirectionally. In that case the motion vector will
         * be ZERO vector */

        pmv0 = (2 * MV[6][y][x - 1]->x + MV[6][y][x - 1]->x_half);
        pmv1 = (2 * MV[6][y][x - 1]->y + MV[6][y][x - 1]->y_half);

        x_vec = (2 * MV[6][y][x]->x + MV[6][y][x]->x_half) - pmv0;
        y_vec = (2 * MV[6][y][x]->y + MV[6][y][x]->y_half) - pmv1;
      }
    }

    if (x_vec < -32)
      x_vec += 64;
    else if (x_vec > 31)
      x_vec -= 64;
    if (y_vec < -32)
      y_vec += 64;
    else if (y_vec > 31)
      y_vec -= 64;

    if (x_vec < 0)
      x_vec += 64;
    if (y_vec < 0)
      y_vec += 64;

    if (trace)
    {
      if (pic->PB == PB_FRAMES)
        fprintf (tf, "PB delta vectors:\n");
      if (pic->PB == IM_PB_FRAMES)
        fprintf (tf, "PB ,B forward vectors:\n");
    }
    bits->vec += AR_Encode (indexfn (x_vec, sed_mvdtab, 64), sed_cumf_MVD);
    bits->vec += AR_Encode (indexfn (y_vec, sed_mvdtab, 64), sed_cumf_MVD);

    if (trace)
    {
      if (x_vec > 31)
        x_vec -= 64;
      if (y_vec > 31)
        y_vec -= 64;
      fprintf (tf, "MVDB (x,y) = (%d,%d)\n", x_vec, y_vec);
    }
  }
  return;
}

/**********************************************************************
 *
 *	Name:           FindPMV
 *	Description:	counts bits used for MVs and writes to bitstream
 *
 *	Input:          MV struct, position, pointers to MV predictors,
 *                      block number, half pel flag.
 *
 *	Returns:        median predicted MV
 *	Side effects:
 *
 *	Date: 940111	Author:	???
 *            970831    Added support for backwards MV prediction,
 *                      modified by mikeg@ee.ubc.ca
 *
 ***********************************************************************/

void FindPMV (MotionVector * MV[7][MBR + 1][MBC + 2], int x, int y,
               int *pmv0, int *pmv1, int block,
               int newgob, int half_pel)
{
  int p1, p2, p3;
  int xin1, xin2, xin3;
  int yin1, yin2, yin3;
  int vec1, vec2, vec3;
  int l8, o8, or8;


  l8 = o8 = or8 = 0;
  if (MV[0][y][x - 1]->Mode == MODE_INTER4V || MV[0][y][x - 1]->Mode == MODE_INTER4V_Q)
    l8 = 1;
  if (MV[0][y - 1][x]->Mode == MODE_INTER4V || MV[0][y - 1][x]->Mode == MODE_INTER4V_Q)
    o8 = 1;
  if (MV[0][y - 1][x + 1]->Mode == MODE_INTER4V || MV[0][y - 1][x + 1]->Mode == MODE_INTER4V_Q)
    or8 = 1;

  switch (block)
  {
    case 0:
      vec1 = (l8 ? 2 : 0);
      yin1 = y;
      xin1 = x - 1;
      vec2 = (o8 ? 3 : 0);
      yin2 = y - 1;
      xin2 = x;
      vec3 = (or8 ? 3 : 0);
      yin3 = y - 1;
      xin3 = x + 1;
      break;
    case 1:
      vec1 = (l8 ? 2 : 0);
      yin1 = y;
      xin1 = x - 1;
      vec2 = (o8 ? 3 : 0);
      yin2 = y - 1;
      xin2 = x;
      vec3 = (or8 ? 3 : 0);
      yin3 = y - 1;
      xin3 = x + 1;
      break;
    case 2:
      vec1 = 1;
      yin1 = y;
      xin1 = x;
      vec2 = (o8 ? 4 : 0);
      yin2 = y - 1;
      xin2 = x;
      vec3 = (or8 ? 3 : 0);
      yin3 = y - 1;
      xin3 = x + 1;
      break;
    case 3:
      vec1 = (l8 ? 4 : 0);
      yin1 = y;
      xin1 = x - 1;
      vec2 = 1;
      yin2 = y;
      xin2 = x;
      vec3 = 2;
      yin3 = y;
      xin3 = x;
      break;
    case 4:
      vec1 = 3;
      yin1 = y;
      xin1 = x;
      vec2 = 1;
      yin2 = y;
      xin2 = x;
      vec3 = 2;
      yin3 = y;
      xin3 = x;
      break;
    case 5:
      vec1 = 5;
      yin1 = y;
      xin1 = x - 1;
      vec2 = 5;
      yin2 = y - 1;
      xin2 = x;
      vec3 = 5;
      yin3 = y - 1;
      xin3 = x + 1;
      break;
    default:
      fprintf (stderr, "Illegal block number in FindPMV (countbit.c)\n");
      exit (-1);
      break;
  }

  if (half_pel)
  {
    p1 = 2 * MV[vec1][yin1][xin1]->x + MV[vec1][yin1][xin1]->x_half;
    p2 = 2 * MV[vec2][yin2][xin2]->x + MV[vec2][yin2][xin2]->x_half;
    p3 = 2 * MV[vec3][yin3][xin3]->x + MV[vec3][yin3][xin3]->x_half;
  } else
  {
    p1 = 2 * MV[vec1][yin1][xin1]->x;
    p2 = 2 * MV[vec2][yin2][xin2]->x;
    p3 = 2 * MV[vec3][yin3][xin3]->x;
  }

  if (newgob && (block == 0 || block == 1 || block == 2))
    p2 = 2 * NO_VEC;

  if (p2 == 2 * NO_VEC)
  {
    p2 = p3 = p1;
  }
  *pmv0 = p1 + p2 + p3 - mmax (p1, mmax (p2, p3)) - mmin (p1, mmin (p2, p3));

  if (half_pel)
  {
    p1 = 2 * MV[vec1][yin1][xin1]->y + MV[vec1][yin1][xin1]->y_half;
    p2 = 2 * MV[vec2][yin2][xin2]->y + MV[vec2][yin2][xin2]->y_half;
    p3 = 2 * MV[vec3][yin3][xin3]->y + MV[vec3][yin3][xin3]->y_half;
  } else
  {
    p1 = 2 * MV[vec1][yin1][xin1]->y;
    p2 = 2 * MV[vec2][yin2][xin2]->y;
    p3 = 2 * MV[vec3][yin3][xin3]->y;
  }

  if (newgob && (block == 0 || block == 1 || block == 2))
    p2 = 2 * NO_VEC;

  if (p2 == 2 * NO_VEC)
  {
    p2 = p3 = p1;
  }
  *pmv1 = p1 + p2 + p3 - mmax (p1, mmax (p2, p3)) - mmin (p1, mmin (p2, p3));

  return;
}

void ZeroBits (Bits * bits)
{
  bits->Y = 0;
  bits->C = 0;
  bits->vec = 0;
  bits->CBPY = 0;
  bits->CBPCM = 0;
  bits->MODB = 0;
  bits->CBPB = 0;
  bits->COD = 0;
  bits->DQUANT = 0;
  bits->header = 0;
  bits->total = 0;
  bits->no_inter = 0;
  bits->no_inter4v = 0;
  bits->no_intra = 0;
  bits->INTRA_MODE = 0;
  return;
}
void ZeroRes (Results * res)
{
  res->SNR_l = (float) 0;
  res->SNR_Cr = (float) 0;
  res->SNR_Cb = (float) 0;
  res->QP_mean = (float) 0;
}
void AddBits (Bits * total, Bits * bits)
{
  total->Y += bits->Y;
  total->C += bits->C;
  total->vec += bits->vec;
  total->CBPY += bits->CBPY;
  total->CBPCM += bits->CBPCM;
  total->MODB += bits->MODB;
  total->CBPB += bits->CBPB;
  total->COD += bits->COD;
  total->DQUANT += bits->DQUANT;
  total->header += bits->header;
  total->total += bits->total;
  total->no_inter += bits->no_inter;
  total->no_inter4v += bits->no_inter4v;
  total->no_intra += bits->no_intra;
  total->INTRA_MODE += bits->INTRA_MODE;
  return;
}
void AddRes (Results * total, Results * res, Pict * pic)
{
  total->SNR_l += res->SNR_l;
  total->SNR_Cr += res->SNR_Cr;
  total->SNR_Cb += res->SNR_Cb;
  total->QP_mean += pic->QP_mean;
  return;
}

void AddBitsPicture (Bits * bits)
{
  bits->total =
  bits->Y +
  bits->C +
  bits->vec +
  bits->CBPY +
  bits->CBPCM +
  bits->MODB +
  bits->CBPB +
  bits->COD +
  bits->DQUANT +
  bits->header +
  bits->INTRA_MODE;
}
void ZeroVec (MotionVector * MV)
{
  MV->x = 0;
  MV->y = 0;
  MV->x_half = 0;
  MV->y_half = 0;
  return;
}
void MarkVec (MotionVector * MV)
{
  MV->x = NO_VEC;
  MV->y = NO_VEC;
  MV->x_half = 0;
  MV->y_half = 0;
  return;
}

void CopyVec (MotionVector * MV2, MotionVector * MV1)
{
  MV2->x = MV1->x;
  MV2->x_half = MV1->x_half;
  MV2->y = MV1->y;
  MV2->y_half = MV1->y_half;
  return;
}

int EqualVec (MotionVector * MV2, MotionVector * MV1)
{
  if (MV1->x != MV2->x)
    return 0;
  if (MV1->y != MV2->y)
    return 0;
  if (MV1->x_half != MV2->x_half)
    return 0;
  if (MV1->y_half != MV2->y_half)
    return 0;
  return 1;
}

/**********************************************************************
 *
 *	Name:        CountBitsPicture(Pict *pic)
 *	Description:    counts the number of bits needed for picture
 *                      header
 *
 *	Input:	        pointer to picture structure
 *	Returns:        number of bits
 *	Side effects:
 *
 *	Date: 941128	Author:Karl.Lillevold@nta.no
 *            970831    Added support for H.263+ syntax,
 *                      modified by bernae@ee.ubc.ca, guyc@ee.ubc.ca,
 *                      and mikeg@ee.ubc.ca
 *
 ***********************************************************************/

int CountBitsPicture (Pict * pic)
{
  int bits = 0;

  /* in case of arithmetic coding, encoder_flush() has been called before
   * zeroflush() in main.c */

  /* Picture start code */
  if (trace)
  {
    fprintf (tf, "picture_start_code: ");
  }
  putbits (PSC_LENGTH, PSC);
  bits += PSC_LENGTH;

  /* Group number */
  if (trace)
  {
    fprintf (tf, "Group number in picture header: ");
  }
  putbits (5, 0);
  bits += 5;

  /* Time reference */
  if (trace)
  {
    fprintf (tf, "Time reference: ");
  }
  putbits (8, pic->TR);         /* 8 LSB of TR (10 bits) */
  bits += 8;

  /* bit 1 */
  if (trace)
  {
    fprintf (tf, "spare: ");
  }
  pic->spare = 1;               /* always 1 to avoid start code emulation */
  putbits (1, pic->spare);
  bits += 1;

  /* bit 2 */
  if (trace)
  {
    fprintf (tf, "always zero for distinction with H.261\n");
  }
  putbits (1, 0);
  bits += 1;

  /* bit 3 */
  if (trace)
  {
    fprintf (tf, "split_screen_indicator: ");
  }
  putbits (1, 0);               /* no support for split-screen in this
                                 * software */
  bits += 1;

  /* bit 4 */
  if (trace)
  {
    fprintf (tf, "document_camera_indicator: ");
  }
  putbits (1, 0);
  bits += 1;

  /* bit 5 */
  if (trace)
  {
    fprintf (tf, "freeze_picture_release: ");
  }
  putbits (1, 0);
  bits += 1;

  if (EPTYPE == 1)
  {
    /* extented PTYPE (PLUSPTYPE) is used, bits 6-8 of PTYPE. */
    putbits (3, SF_EPTYPE);
    bits += 3;

    if (trace)
    {
      fprintf (tf, "UFEP: ");
    }
    putbits (3, pic->UFEP);
    bits += 3;

    if (pic->UFEP == 1)
    {                           /* optional part of PLUSPTYPE (OPPTYPE)
                                 * fields */
      /* bits 1-3 */
      if (trace)
      {
        fprintf (tf, "source_format: ");
      }
      putbits (3, pic->source_format);
      bits += 3;
      /* bit 4 */
      if (trace)
      {
        fprintf (tf, "custom_picture_clock_frequency: ");
      }
      putbits (1, pic->PCF);
      bits += 1;
      /* bit 5 */
      if (trace)
      {
        fprintf (tf, "mv_outside_frame: ");
      }
      putbits (1, pic->unrestricted_mv_mode); /* Unrestricted Motion
                                               * Vector mode */
      bits += 1;

      /* bit 6  */
      if (trace)
      {
        fprintf (tf, "sac_coding: ");
      }
      putbits (1, syntax_arith_coding); /* Syntax-based Arithmetic Coding
                                         * mode */
      bits += 1;

      /* bit 7  */
      if (trace)
      {
        fprintf (tf, "adv_pred_mode: ");
      }
      putbits (1, adv_pred);    /* Advanced Prediction mode */
      bits += 1;

      /* bit 8  */
      if (trace)
      {
        fprintf (tf, "adv_intra_coding: ");
      }
      putbits (1, advanced_intra_coding); /* Advanced Intra Coding mode */
      bits += 1;

      /* bit 9  */
      if (trace)
      {
        fprintf (tf, "deblocking_filter: ");
      }
      putbits (1, deblocking_filter); /* Deblocking Filter  mode */
      bits += 1;

      /* bit 10 */
      if (trace)
      {
        fprintf (tf, "slice_structure  : ");
      }
      putbits (1, slice_structure); /* Slice Structured mode */
      bits += 1;

      /* bit 11 */
      if (trace)
      {
        fprintf (tf, "reference_picture_selection  : ");
      }
      putbits (1, pic->reference_picture_selection);  /* reference picture
                                                       * selection mode */
      bits += 1;

      /* bit 12 */
      if (trace)
      {
        fprintf (tf, "independently_segmented_decoding  : ");
      }
      putbits (1, independently_segmented_decoding);  /* independently
                                                       * segmented decoding
                                                       * mode */
      bits += 1;

      /* bit 13 */
      if (trace)
      {
        fprintf (tf, "alternative_inter_vlc  : ");
      }
      putbits (1, alternative_inter_vlc); /* alternative inter vlc mode */
      bits += 1;

      /* bit 14 */
      if (trace)
      {
        fprintf (tf, "modified_quantization  : ");
      }
      putbits (1, modified_quantization); /* modified quantization mode */
      bits += 1;

      /* bit 15 */
      if (trace)
      {
        fprintf (tf, "spare  : ");
      }
      putbits (1, 1);           /* always 1 to avoid start code emulation */
      bits += 1;

      /* bit 16-18 */
      if (trace)
      {
        fprintf (tf, "reserved  : ");
      }
      putbits (3, 0);           /* bit 16-18 are reserved and set to zero */
      bits += 3;
    }
    /* mandatory part of PLUSPTYPE (MPPTYPE) */

    /* bit 1-3 */
    if (trace)
    {
      fprintf (tf, "picture_coding_type: ");
    }
    putbits (3, pic->picture_coding_type);
    bits += 3;

    /* bit 4 */
    if (trace)
    {
      fprintf (tf, "reference_picture_resampling: ");
    }
    putbits (1, reference_picture_resampling);  /* reference picture
                                                 * resampling mode */
    bits += 1;

    /* bit 5 */
    if (trace)
    {
      fprintf (tf, "reduced_resolution_update: ");
    }
    putbits (1, reduced_resolution_update); /* reduced resolution update */
    bits += 1;

    /* bit 6 */
    if (trace)
    {
      fprintf (tf, "rounding_type: ");
    }
    putbits (1, pic->RTYPE);    /* rounding type */
    bits += 1;

    /* bit 7 */
    if (trace)
    {
      fprintf (tf, "reserved: ");
    }
    putbits (1, 0);             /* bit 7 is reserved and set to zero */
    bits += 1;

    /* bit 8 */
    if (trace)
    {
      fprintf (tf, "reserved: ");
    }
    putbits (1, 0);             /* bit 8 is reserved and set to zero */
    bits += 1;


    /* bit 9 */
    if (trace)
    {
      fprintf (tf, "spare  : ");
    }
    putbits (1, 1);             /* always 1 to avoid start code emulation */
    bits += 1;

    /* Continuous Presence Multipoint (CPM) */
    if (trace)
    {
      fprintf (tf, "CPM: ");
    }
    putbits (1, 0);               /* CPM is not supported in this software */
    bits += 1;

    /* Picture Sub Bitstream Indicator (PSBI) */
    /* if CPM == 1: 2 bits PSBI */
    /* not supported */

    /* PLUSPTYPE related fields */
    if (pic->UFEP == 1 && pic->source_format == SF_CUSTOM)
    {
      /* custom picture format (CPSMT) is present */
      /* bit 1-4 */
      if (trace)
      {
        fprintf (tf, "pixel_aspect_ratio: ");
      }

      /* only supports CCIR 601 pixel aspect ratio */
      pic->PAR = PAR_CIF;      
      putbits (4, pic->PAR);
      bits += 4;

      /* bit 5-13 */
      if (trace)
      {
        fprintf (tf, "picture_width_indication: ");

      }
      pic->PWI = sed_pels / 4 - 1;
      putbits (9, pic->PWI);    /* pixels per line = (PWI+1)*4 */
      bits += 9;                /* PWI range: [0,...,511] */

      /* bit 14 */
      if (trace)
      {
        fprintf (tf, "reserved: ");
      }
      putbits (1, 1);           /* always 1 to avoid start code emulation */
      bits += 1;

      /* bit 15 - 23 */
      if (trace)
      {
        fprintf (tf, "picture_height_indication: ");

      }
      pic->PHI = sed_lines / 4;
      putbits (8, pic->PHI);    /* number of sed_lines = (PHI+1)*4 */
      bits += 8;                /* PWI range: [0,...,287] */
      if (pic->PAR == PAR_EXTENDED)
      {
        /* extended pixel aspect ratio is used */
        /* bit 1-8 */
        if (trace)
        {
          fprintf (tf, "PAR_width: ");
        }
        putbits (8, pic->PAR_width);  /* natural binary width */
        bits += 8;
        /* bits 9-16 */
        putbits (8, pic->PAR_height);
        bits += 8;

      }
    }
    if (pic->UFEP == 1 && pic->PCF == 1)
    {
      /* Custom picture clock frequency (CPCFC) is present */
      /* bit 1 */
      if (trace)
      {
        fprintf (tf, "clock_conversion_code: ");
      }
      putbits (1, pic->CCC);
      bits += 1;
      /* bit 2-8 */
      if (trace)
      {
        fprintf (tf, "clock_divisor: ");
      }
      putbits (7, pic->CD);
      bits += 7;
    }
    if (pic->PCF)
    {
      /* extended temporal reference (ETR) */
      if (trace)
      {
        fprintf (tf, "extended_temporal_reference: ");

      }
      putbits (2, (pic->TR & 0x0030));  /* 2 MSB of TR (10 bits) */
      bits += 2;

    }

    if (pic->UFEP == 1 && slice_structure)
    {
      if (trace)
      {
        fprintf (tf, "slice_structure_submode: ");
      }
      /* bit 1 */
      putbits (1, pic->rectangular_slice);
      bits += 1;

      /* bit 2 */
      putbits (1, pic->arbitrary_slice_ordering);
      bits += 1;
    }

    if (PCT_B  == pic->picture_coding_type ||
        PCT_EI == pic->picture_coding_type ||
        PCT_EP == pic->picture_coding_type)
    {
      /* enhancement layer number (ELNUM) */
      /* bit 1-4 */
      if (trace)
      {
        fprintf (tf, "enhancement_layer_number: ");
      }
      putbits (4, pic->ELNUM);
      bits += 4;
    }
    if ((PCT_B  == pic->picture_coding_type || 
         PCT_EI == pic->picture_coding_type || 
         PCT_EP == pic->picture_coding_type) 
         && (pic->UFEP == 1))
    {
      /* reference layer number (RLNUM) */
      /* bit 1-4 */
      if (trace)
      {
        fprintf (tf, "reference_layer_number: ");
      }
      putbits (4, pic->RLNUM);
      bits += 4;
    }

    if (pic->reference_picture_selection && pic->UFEP)
    {
      if (trace)
      {
        fprintf (tf, "mode_flags: ");
      }
      putbits (3, pic->MF);
      bits += 3;
    }
    if (pic->reference_picture_selection)
    {
      /* Temporal reference for prediction indication(1 bit) */
      if (trace)
      {
        fprintf (tf, "reference_picture_selection: ");
      }
      if (pic->picture_coding_type == PCT_INTRA ||
          pic->picture_coding_type == PCT_EI)
      {
        pic->TRPI = 0;
      }
      putbits (1, pic->TRPI);
      bits += 1;

      if (pic->TRPI)
      {
        /* Temporal reference for prediction */
        if (trace)
        {
          fprintf (tf, "temporal_reference_for_prediction");
        }
        if (pic->PCF)
        {
          putbits (10, pic->TR);
          bits += 10;
        } else
        {
          putbits (10, pic->TR);
          bits += 10;
        }
      }
    }
    if (pic->reference_picture_selection)
    {
      /* Back Channel message Indication (BCI) */
      if (trace)
      {
        fprintf (tf, "back_channel_message_indication: ");
      }
      putbits (1, pic->BCI);
      bits += 1;
      if (pic->BCI)
      {
        /* Back Channel Message (BCM) */
        if (trace)
        {
          fprintf (tf, "back_channel_message: ");
        }
        putbits (pic->BCM_LENGTH, pic->BCM);
        bits += pic->BCM_LENGTH;
      }
    }
    if (reference_picture_resampling)
    {
      /* Reference picture resampling parameters */
      if (trace)
      {
        fprintf (tf, "reference_picture_resampling_parameters: ");
      }
      putbits (pic->RPRP_LENGTH, pic->RPRP);
      bits += pic->RPRP_LENGTH;

    }

    /* PQUANT */
    if (trace)
    {
      fprintf (tf, "PQUANT: ");
    }
    putbits (5, pic->QUANT);
    bits += 5;
  } 
  else
  {                             /* remainder bits of PTYPE when EPTYPE not
                                 * used */
    /* bit 6-8 */
    if (trace)
    {
      fprintf (tf, "source_format: ");
    }
    putbits (3, pic->source_format);
    bits += 3;

    /* bit 9 */
    if (trace)
    {
      fprintf (tf, "picture_coding_type: ");
    }
    if (pic->picture_coding_type == PCT_INTRA)
      putbits (1, 0);
    else
      putbits (1, 1);
    bits += 1;

    /* bit 10 */
    if (trace)
    {
      fprintf (tf, "mv_outside_frame: ");
    }
    putbits (1, pic->unrestricted_mv_mode); /* Unrestricted Motion Vector
                                             * mode */
    bits += 1;

    /* bit 11 */
    if (trace)
    {
      fprintf (tf, "sac_coding: ");
    }
    putbits (1, syntax_arith_coding); /* Syntax-based Arithmetic Coding
                                       * mode */
    bits += 1;

    /* bit 12 */
    if (trace)
    {
      fprintf (tf, "adv_pred_mode: ");
    }
    putbits (1, adv_pred);      /* Advanced Prediction mode */
    bits += 1;

    /* bit 13 */
    if (trace)
    {
      fprintf (tf, "PB-coded: "); /* PB-frames mode */
    }
    putbits (1, pic->PB);
    bits += 1;

    /* QUANT */
    if (trace)
    {
      fprintf (tf, "QUANT: ");
    }
    putbits (5, pic->QUANT);
    bits += 5;

    /* Continuous Presence Multipoint (CPM) */
    if (trace)
    {
      fprintf (tf, "CPM: ");
    }
    putbits (1, 0);               /* CPM is not supported in this software */
    bits += 1;

    /* Picture Sub Bitstream Indicator (PSBI) */
    /* if CPM == 1: 2 bits PSBI */
    /* not supported */
  }

  /* extra information for PB-frames */
  if (pic->PB)
  {
    if (trace)
    {
      fprintf (tf, "TRB: ");
    }
    if (pic->PCF)
    {
      putbits (5, pic->TRB);
      bits += 5;
    } else
    {
      putbits (3, pic->TRB);
      bits += 3;
    }


    if (trace)
    {
      fprintf (tf, "BQUANT: ");
    }
    putbits (2, pic->BQUANT);
    bits += 2;

  }
  /* PEI (extra information) */
  for (;;)
  {
    if (trace)
    {
      fprintf (tf, "PEI: ");
    }
    /* signal the presence of PSUPP when set to "1" */
    putbits (1, pic->PEI);
    bits += 1;

    /* PSUPP */
    /* if PEI == 1: 8 bits PSUPP + another PEI bit */
    /* not supported */
    if (pic->PEI)
    {
      if (trace)
      {
        fprintf (tf, "PSUPP: ");
      }
      putbits (8, pic->PSUPP);
      bits += 8;
    } else
      break;
  }

  return bits;
}
