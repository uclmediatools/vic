/************************************************************************
 *
 *  getblk.c, DCT block decoding for tmndecode (H.263 decoder)
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


#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "tmndec.h"
#include "global.h"

#define INDICES
#include "indices.h"

#define SACTABLES
#include "sactbls.h"


typedef struct
{
  char run, level, len;
} DCTtab;
typedef struct
{
  int val, len;
} VLCtabI;
typedef struct
{
  int val, run, sign;
} RunCoef;

typedef struct
{
  int code, level, last, run, sign, extended_level;
} CodeCoeff;



/* local prototypes */
RunCoef vlc_word_decode _ANSI_ARGS_ ((int symbol_word, int *last));
RunCoef Decode_Escape_Char _ANSI_ARGS_ ((int intra, int *last));
int DecodeTCoef _ANSI_ARGS_ ((int position, int intra));



extern VLCtabI DCT3Dtab0[], DCT3Dtab1[], DCT3Dtab2[];

/* Tables used in advanced intra coding mode */
extern VLCtabI INTRA_DCT3Dtab0[], INTRA_DCT3Dtab1[], INTRA_DCT3Dtab2[];

extern MQ_chroma_QP_table[];

void getblock (int comp, int mode, int INTRA_AC_DC, int Mode)
{
  int val, i, j, sign;
  unsigned int code;
  VLCtabI *tab;
  short *bp;
  int run, last, level, QP;
  short *qval;


  /* store_code used in alternative inter VLC mode */
  int use_intra = 0;
  CodeCoeff store_code[64];
  int coeff_ind;

  bp = ld->block[comp];

  /* decode AC coefficients (or all coefficients in advanced intra coding
   * mode) */
  if (trace)
    fprintf (trace_file, "block: %i\n", comp);

  if (alternative_inter_VLC_mode && !(Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
  {
    coeff_ind = 0;
    use_intra = 0;
    for (i = (mode == 0);; i++)
    {
      code = showbits (12);
      store_code[coeff_ind].code = code;
      if (code >= 512)
        tab = &DCT3Dtab0[(code >> 5) - 16];
      else if (code >= 128)
        tab = &DCT3Dtab1[(code >> 2) - 32];
      else if (code >= 8)
        tab = &DCT3Dtab2[(code >> 0) - 8];
      else
      {
        if (!quiet)
          fprintf (stderr, "invalid Huffman code in getblock()\n");
        fault = 1;
        return;
      }

      run = (tab->val >> 4) & 255;
      last = (tab->val >> 12) & 1;
      flushbits (tab->len);
      if (tab->val == ESCAPE)
      {
        last = getbits1 ();
        store_code[coeff_ind].last = last;

        i += run = vic_getbits (6);
        store_code[coeff_ind].run = run;

        level = vic_getbits (8);
        store_code[coeff_ind].level = level;


        if (level == 128 && modified_quantization_mode)
        {
          level = vic_getbits (11);
          store_code[coeff_ind].extended_level = level;
        }
      } else
      {
        store_code[coeff_ind].sign = vic_getbits (1);
        i += run;
      }
      coeff_ind += 1;

      if (i >= 64)
        use_intra = 1;
      if (last)
        break;
    }
    if (trace)
      fprintf (trace_file, "inter VLC Table: %s\n", (use_intra ? "Intra" : "Inter"));
  }
  coeff_ind = 0;

  for (i = (mode == 0);; i++)
  {
    if ((alternative_inter_VLC_mode) && !(Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
    {
      if (use_intra)
      {
        if ((store_code[coeff_ind].code) >= 512)
          tab = &INTRA_DCT3Dtab0[((store_code[coeff_ind].code) >> 5) - 16];
        else if ((store_code[coeff_ind].code) >= 128)
          tab = &INTRA_DCT3Dtab1[((store_code[coeff_ind].code) >> 2) - 32];
        else if ((store_code[coeff_ind].code) >= 8)
          tab = &INTRA_DCT3Dtab2[((store_code[coeff_ind].code) >> 0) - 8];
        else
        {
          if (!quiet)
            fprintf (stderr, "invalid Huffman code in getblock()\n");
          fault = 1;
          return;
        }
        run = (tab->val >> 6) & 63;
        level = tab->val & 63;
        last = (tab->val >> 12) & 1;
      } else
      {
        if (store_code[coeff_ind].code >= 512)
          tab = &DCT3Dtab0[(store_code[coeff_ind].code >> 5) - 16];
        else if (store_code[coeff_ind].code >= 128)
          tab = &DCT3Dtab1[(store_code[coeff_ind].code >> 2) - 32];
        else if (store_code[coeff_ind].code >= 8)
          tab = &DCT3Dtab2[(store_code[coeff_ind].code >> 0) - 8];
        else
        {
          if (!quiet)
            fprintf (stderr, "invalid Huffman code in getblock()\n");
          fault = 1;
          return;
        }

        run = (tab->val >> 4) & 255;
        level = tab->val & 15;
        last = (tab->val >> 12) & 1;
      }

      if (trace)
      {
        fprintf (trace_file, " (");
        printbits (store_code[coeff_ind].code, 12, tab->len);
      }
      if (tab->val == ESCAPE)
      {                         /* escape */
        if (trace)
        {
          fprintf (trace_file, " ");
          printbits (store_code[coeff_ind].last, 1, 1);
        }
        last = store_code[coeff_ind].last;
        if (trace)
        {
          fprintf (trace_file, " ");
          printbits (store_code[coeff_ind].run, 6, 6);
        }
        i += run = store_code[coeff_ind].run;
        if (trace)
        {
          fprintf (trace_file, " ");
          printbits (store_code[coeff_ind].level, 8, 8);
        }
        level = store_code[coeff_ind].level;

        if ((sign = (level >= 128)))
          val = 256 - level;
        else
          val = level;

        if (level == 128 && modified_quantization_mode)
        {
          level = store_code[coeff_ind].extended_level;
          level = (level >> 6 & 0x001F) | level << 5;
          /* correctly set the sign */
          level = (level << (sizeof (int) * 8 - 11)) >> (sizeof (int) * 8 - 11);
          if (level >= 0)
            sign = 0;
          else
            sign = 1;
          val = abs (level);
        }
      } else
      {
        sign = store_code[coeff_ind].sign;
        i += run;
        val = level;

        if (trace)
          fprintf (trace_file, "%d", sign);
      }
      coeff_ind += 1;

    } else
    {
      code = showbits (12);
      if ((advanced_intra_coding) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
      {
        if (code >= 512)
          tab = &INTRA_DCT3Dtab0[(code >> 5) - 16];
        else if (code >= 128)
          tab = &INTRA_DCT3Dtab1[(code >> 2) - 32];
        else if (code >= 8)
          tab = &INTRA_DCT3Dtab2[(code >> 0) - 8];
        else
        {
          if (!quiet)
            fprintf (stderr, "invalid Huffman code in getblock()\n");
          fault = 1;
          return;
        }
        run = (tab->val >> 6) & 63;
        level = tab->val & 63;
        last = (tab->val >> 12) & 1;
      } else
      {

        if (code >= 512)
          tab = &DCT3Dtab0[(code >> 5) - 16];
        else if (code >= 128)
          tab = &DCT3Dtab1[(code >> 2) - 32];
        else if (code >= 8)
          tab = &DCT3Dtab2[(code >> 0) - 8];
        else
        {
          if (!quiet)
            fprintf (stderr, "invalid Huffman code in getblock()\n");
          fault = 1;
          return;
        }

        run = (tab->val >> 4) & 255;
        level = tab->val & 15;
        last = (tab->val >> 12) & 1;
      }
      flushbits (tab->len);
      if (trace)
      {
        fprintf (trace_file, " (");
        printbits (code, 12, tab->len);
      }
      if (tab->val == ESCAPE)
      {                         /* escape */
        if (trace)
        {
          fprintf (trace_file, " ");
          printbits (showbits (1), 1, 1);
        }
        last = getbits1 ();
        if (trace)
        {
          fprintf (trace_file, " ");
          printbits (showbits (6), 6, 6);
        }
        i += run = vic_getbits (6);
        if (trace)
        {
          fprintf (trace_file, " ");
          printbits (showbits (8), 8, 8);
        }
        level = vic_getbits (8);

        if ((sign = (level >= 128)))
          val = 256 - level;
        else
          val = level;

        if (level == 128 && modified_quantization_mode)
        {
          /* extended escape code was received */
          level = vic_getbits (11);
          level = (level >> 6 & 0x001F) | level << 5;
          /* correctly set the sign */
          level = (level << (sizeof (int) * 8 - 11)) >> (sizeof (int) * 8 - 11);
          if (level >= 0)
            sign = 0;
          else
            sign = 1;
          val = abs (level);
        }
      } else
      {
        i += run;
        val = level;
        sign = vic_getbits (1);
        if (trace)
          fprintf (trace_file, "%d", sign);
      }
    }

    if (i >= 64)
    {
      if (!quiet)
        fprintf (stderr, "DCT coeff index (i) out of bounds\n");
      fault = 1;
      return;
    }
    if (trace)
      fprintf (trace_file, "): %d/%d\n", run, sign ? -val : val);

    /* Descan in the proper order in advanced intra coding mode */

    if (advanced_intra_coding && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
    {
      switch (INTRA_AC_DC)
      {
        case INTRA_MODE_DC:
          j = zig_zag_scan[i];
          break;
        case INTRA_MODE_VERT_AC:
          j = alternate_horizontal_scan[i];
          break;
        case INTRA_MODE_HORI_AC:
          j = alternate_vertical_scan[i];
          break;
        default:
          printf ("Invalid Intra_Mode in Advanced Intra Coding\n");
          exit (-1);
          break;
      }
    } else
    {
      j = zig_zag_scan[i];
    }
    qval = &bp[j];
    if (comp >= 6)
      QP = mmax (1, mmin (31, (bquant_tab[bquant] * quant) >> 2));
    else
      QP = quant;

    if (modified_quantization_mode && (comp == 4 || comp == 5 || comp == 10 || comp == 11))
    {
      /* when modified quantization mode is on, use modified quantizer for
       * chorominance coefficients */

      QP = MQ_chroma_QP_table[QP];
    }
    if ((advanced_intra_coding) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
    {
      /* Advanced intra coding dequantization */
      *qval = (sign ? -(QP * (2 * val)) : QP * (2 * val));
    } else
    {
      /* TMN3 dequantization */
      if ((QP % 2) == 1)
        *qval = (sign ? -(QP * (2 * val + 1)) : QP * (2 * val + 1));
      else
        *qval = (sign ? -(QP * (2 * val + 1) - 1) : QP * (2 * val + 1) - 1);
    }
    if (last)
    {                           /* That's it */
      if (trace)
        fprintf (trace_file, "last\n------------------------------------------------------------------\n");
      return;
    }
  }
}



/*********************************************************************
 *
 *        Name:        get_sac_block
 *
 *	Description:	Decodes blocks of Arithmetic Encoded DCT Coeffs.
 *        and performs Run Length Decoding and Coefficient
 *        Dequantisation.
 *
 *	Input:        Picture block type and number.
 *
 *	Returns:	Nothing.
 *
 *	Side Effects:
 *
 *	Author:        Wayne Ellis <ellis_w_wayne@bt-web.bt.co.uk>
 *
 *********************************************************************/


void get_sac_block (int comp, int ptype, int INTRA_AC_DC, int Mode)
{
  int position = 0;
  int TCOEF_index, symbol_word;
  int last = 0, QP, i, j;
  short *qval;
  short *bp;
  RunCoef DCTcoef;
  int intra;


  bp = ld->block[comp];

  i = (ptype == 0);
  intra = (Mode == MODE_INTRA || Mode == MODE_INTRA_Q);

  while (!last)
  {                             /* while there are DCT coefficients
                                 * remaining */
    position++;                 /* coefficient counter relates to Coeff.
                                 * model */
    TCOEF_index = DecodeTCoef (position, intra);

    if (TCOEF_index == ESCAPE_INDEX)
    {                           /* ESCAPE code encountered */
      DCTcoef = Decode_Escape_Char (intra, &last);
      if (trace)
        fprintf (trace_file, "ESC: ");
    } else
    {
      symbol_word = tcoeftab[TCOEF_index];

      DCTcoef = vlc_word_decode (symbol_word, &last);
    }

    if (trace)
    {
      fprintf (trace_file, "val: %d, run: %d, sign: %d, last: %d\n",
               DCTcoef.val, DCTcoef.run, DCTcoef.sign, last);
    }
    i += DCTcoef.run;
    if (advanced_intra_coding && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
    {
      switch (INTRA_AC_DC)
      {
        case INTRA_MODE_DC:
          j = zig_zag_scan[i];
          break;
        case INTRA_MODE_VERT_AC:
          j = alternate_horizontal_scan[i];
          break;
        case INTRA_MODE_HORI_AC:
          j = alternate_vertical_scan[i];
          break;
        default:
          printf ("Invalid Intra_Mode in Advanced Intra Coding");
          exit (-1);
          break;
      }
    } else
    {
      j = zig_zag_scan[i];
    }

    qval = &bp[j];

    i++;

    if (comp >= 6)
      QP = mmax (1, mmin (31, (bquant_tab[bquant] * quant) >> 2));
    else
      QP = quant;
    if ((advanced_intra_coding) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
    {
      /* Advanced intra coding dequantization */
      *qval = (DCTcoef.sign ? -(QP * (2 * DCTcoef.val)) : QP * (2 * DCTcoef.val));
    } else
    {

      if ((QP % 2) == 1)
        *qval = ((DCTcoef.sign) ? -(QP * (2 * (DCTcoef.val) + 1)) :
                 QP * (2 * (DCTcoef.val) + 1));
      else
        *qval = ((DCTcoef.sign) ? -(QP * (2 * (DCTcoef.val) + 1) - 1) :
                 QP * (2 * (DCTcoef.val) + 1) - 1);
    }
  }
  return;
}

/*********************************************************************
 *
 *        Name:        vlc_word_decode
 *
 *	Description:	Fills Decoder FIFO after a fixed word length
 *        string has been detected.
 *
 *	Input:        Symbol to be decoded, last data flag.
 *
 *	Returns:	Decoded Symbol via the structure DCTcoeff.
 *
 *	Side Effects:	Updates last flag.
 *
 *	Author:        Wayne Ellis <ellis_w_wayne@bt-web.bt.co.uk>
 *
 *********************************************************************/

RunCoef vlc_word_decode (int symbol_word, int *last)
{
  int sign_index;
  RunCoef DCTcoef;

  *last = (symbol_word >> 12) & 01;

  DCTcoef.run = (symbol_word >> 4) & 255;

  DCTcoef.val = (symbol_word) & 15;

  sign_index = decode_a_symbol (cumf_SIGN);

  DCTcoef.sign = signtab[sign_index];

  return (DCTcoef);
}

/*********************************************************************
 *
 *        Name:        Decode_Escape_Char
 *
 *	Description:	Decodes all components for a Symbol when an
 *        ESCAPE character has been detected.
 *
 *	Input:        Picture Type and last data flag.
 *
 *	Returns:	Decoded Symbol via the structure DCTcoeff.
 *
 *	Side Effects:	Modifies last data flag.
 *
 *	Author:        Wayne Ellis <ellis_w_wayne@bt-web.bt.co.uk>
 *
 *********************************************************************/

RunCoef Decode_Escape_Char (int intra, int *last)
{
  int last_index, run, run_index, level, level_index;
  RunCoef DCTcoef;

  if (intra)
  {
    last_index = decode_a_symbol (cumf_LAST_intra);
    *last = last_intratab[last_index];
  } else
  {
    last_index = decode_a_symbol (cumf_LAST);
    *last = lasttab[last_index];
  }

  if (intra)
    run_index = decode_a_symbol (cumf_RUN_intra);
  else
    run_index = decode_a_symbol (cumf_RUN);

  run = runtab[run_index];

  /* $if (mrun) run|=64;$ */

  DCTcoef.run = run;

  if (intra)
    level_index = decode_a_symbol (cumf_LEVEL_intra);
  else
    level_index = decode_a_symbol (cumf_LEVEL);

  if (trace)
    fprintf (trace_file, "level_idx: %d ", level_index);

  level = leveltab[level_index];

  if (level > 128)
    level -= 256;

  if (level < 0)
  {
    DCTcoef.sign = 1;
    DCTcoef.val = abs (level);
  } else
  {
    DCTcoef.sign = 0;
    DCTcoef.val = level;
  }

  return (DCTcoef);

}
/*********************************************************************
 *
 *        Name:        DecodeTCoef
 *
 *	Description:	Decodes a.c DCT Coefficients using the
 *        relevant arithmetic decoding model.
 *
 *	Input:        DCT Coeff count and Picture Type.
 *
 *	Returns:	Index to LUT
 *
 *	Side Effects:	None
 *
 *	Author:        Wayne Ellis <ellis_w_wayne@bt-web.bt.co.uk>
 *
 *********************************************************************/

int DecodeTCoef (int position, int intra)
{
  int index;

  switch (position)
  {
    case 1:
      {
        if (intra)
          index = decode_a_symbol (cumf_TCOEF1_intra);
        else
          index = decode_a_symbol (cumf_TCOEF1);
        break;
      }
    case 2:
      {
        if (intra)
          index = decode_a_symbol (cumf_TCOEF2_intra);
        else
          index = decode_a_symbol (cumf_TCOEF2);
        break;
      }
    case 3:
      {
        if (intra)
          index = decode_a_symbol (cumf_TCOEF3_intra);
        else
          index = decode_a_symbol (cumf_TCOEF3);
        break;
      }
    default:
      {
        if (intra)
          index = decode_a_symbol (cumf_TCOEFr_intra);
        else
          index = decode_a_symbol (cumf_TCOEFr);
        break;
      }
  }

  return (index);
}
