/************************************************************************
 *
 *  getblk.c, DCT block decoding for tmndecode (H.263 decoder)
 *  Copyright (C) 1995, 1996  Telenor R&D, Norway
 *        Karl Olav Lillevold <Karl.Lillevold@nta.no>
 *  
 *  Contacts: 
 *  Karl Olav Lillevold               <Karl.Lillevold@nta.no>, or
 *  Robert Danielsen                  <Robert.Danielsen@nta.no>
 *
 *  Telenor Research and Development  http://www.nta.no/brukere/DVC/
 *  P.O.Box 83                        tel.:   +47 63 84 84 00
 *  N-2007 Kjeller, Norway            fax.:   +47 63 81 00 76
 *  
 ************************************************************************/

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any
 * license fee or royalty on an "as is" basis.  Telenor Research and
 * Development disclaims any and all warranties, whether express,
 * implied, or statuary, including any implied warranties or
 * merchantability or of fitness for a particular purpose.  In no
 * event shall the copyright-holder be liable for any incidental,
 * punitive, or consequential damages of any kind whatsoever arising
 * from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs
 * and user's customers, employees, agents, transferees, successors,
 * and assigns.
 *
 * Telenor Research and Development does not represent or warrant that
 * the programs furnished hereunder are free of infringement of any
 * third-party patents.
 *
 * Commercial implementations of H.263, including shareware, are
 * subject to royalty fees to patent holders.  Many of these patents
 * are general enough such that they are unavoidable regardless of
 * implementation design.
 * */


/*
 * based on mpeg2decode, (C) 1994, MPEG Software Simulation Group
 * and mpeg2play, (C) 1994 Stefan Eckart
 *                         <stefan@lis.e-technik.tu-muenchen.de>
 *
 */


#include <stdio.h>
#include <stdlib.h>

#include "Util.h"
#include "h263decoder.h"
#include "bitIn.h"

#include "indices.h"
#include "sactbls.h"

/* zig-zag scan */
static unsigned char zig_zag_scan[64] = {
  0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
  12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
  35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
  58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
};

#if 0
typedef struct {
  char run, level, len;
} DCTtab;
#endif
typedef struct {
  int val, len;
} VLCtabI;
typedef struct {
  int val, run, sign;
} RunCoef;

/* local prototypes */
static RunCoef vlc_word_decode (Bitstream *bs, int symbol_word, int *last); 
static RunCoef Decode_Escape_Char (Bitstream *bs, int intra, int *last);
static int DecodeTCoef (Bitstream *bs, int position, int intra);



#if 0
extern VLCtabI DCT3Dtab0[],DCT3Dtab1[],DCT3Dtab2[];
#endif



void _getblock(H263Global *h263Data, int comp, int mode)
{
  int val, i, j, sign;
  unsigned int code;
  VLCtab *tab;
  short *bp;
  int run, last, level, QP;
  short *qval;

  bp = h263Data->block[comp];   

  /* decode AC coefficients */
  for (i=(mode==0); ; i++) {
    code = ShowBits(h263Data->bs,12);
    if (code>=512)
      tab = &DCT3Dtab0[(code>>5)-16];
    else if (code>=128)
      tab = &DCT3Dtab1[(code>>2)-32];
    else if (code>=8)
      tab = &DCT3Dtab2[(code>>0)-8];
    else {
      if (!h263Data->quiet)
        fprintf(stderr,"invalid Huffman code in getblock()\n");
      h263Data->fault = 1;
      return;
    }

    ConsumeBits(h263Data->bs,tab->len);

    run = (tab->val >> 4) & 255;
    level = tab->val & 15;
    last = (tab->val >> 12) & 1;

#ifdef TRACE
    if (trace) {
      printf(" (");
      printbits(code,12,tab->len);
    }
#endif

   if (tab->val==ESCAPE) { /* escape */
#ifdef TRACE
      if (trace) {
        putchar(' ');
        printbits(ShowBits(h263Data->bs,1),1,1);
      }
#endif
      last = GetOneBit(h263Data->bs);
#ifdef TRACE
      if (trace) {
        putchar(' ');
        printbits(ShowBits(h263Data->bs,6),6,6);
      }
#endif
      i += run = GetBits(h263Data->bs,6);
#ifdef TRACE
      if (trace) {
        putchar(' ');
        printbits(ShowBits(h263Data->bs,8),8,8);
      }
#endif
      level = GetBits(h263Data->bs,8);

      if ((sign = (level>=128)))
        val = 256 - level;
      else 
        val = level;
    }
    else {
      i+= run;
      val = level;
      sign = GetBits(h263Data->bs,1);
#ifdef TRACE
      if (trace)
        printf("%d",sign);
#endif
    }

    if (i >= 64)
    {
      if (!h263Data->quiet)
        fprintf(stderr,"DCT coeff index (i) out of bounds\n");
      h263Data->fault = 1;
      return;
    }

#ifdef TRACE
    if (trace)
      printf("): %d/%d\n",run,sign ? -val : val);
#endif


    j = zig_zag_scan[i];
    qval = &bp[j];
    if (comp >= 6)
      QP = MAX (1, MIN( 31, ( bquant_tab[h263Data->bquant] * h263Data->quant) >> 2 ));
    else
      QP = h263Data->quant;
      
    /* TMN3 dequantization */
    if ((QP % 2) == 1)
      *qval = ( sign ? -(QP * (2* val+1))  : QP * (2* val+1) );
    else
      *qval = ( sign ? -(QP * (2* val+1)-1): QP * (2* val+1)-1 );

    if (last) { /* That's it */
#ifdef TRACE
      if (trace)
        printf("last\n");
#endif
      return;
    }
  }
}



void getblock_ext(Bitstream *bs, short *bp, int comp, int mode,
		  int quant, int bquant, int quiet, int *fault_p)
{
  int val, i, j, sign;
  unsigned int code;
  VLCtab *tab;
  int run, last, level, QP;
  short *qval;

  /* decode AC coefficients */
  for (i=(mode==0); ; i++) {
    code = ShowBits(bs,12);
    if (code>=512)
      tab = &DCT3Dtab0[(code>>5)-16];
    else if (code>=128)
      tab = &DCT3Dtab1[(code>>2)-32];
    else if (code>=8)
      tab = &DCT3Dtab2[(code>>0)-8];
    else {
      if (!quiet)
        fprintf(stderr,"invalid Huffman code in getblock()\n");
      *fault_p = 1;
      return;
    }

    ConsumeBits(bs,tab->len);

    run = (tab->val >> 4) & 255;
    level = tab->val & 15;
    last = (tab->val >> 12) & 1;

#ifdef TRACE
    if (trace) {
      printf(" (");
      printbits(code,12,tab->len);
    }
#endif

   if (tab->val==ESCAPE) { /* escape */
#ifdef TRACE
      if (trace) {
        putchar(' ');
        printbits(ShowBits(bs,1),1,1);
      }
#endif
      last = GetOneBit(bs);
#ifdef TRACE
      if (trace) {
        putchar(' ');
        printbits(ShowBits(bs,6),6,6);
      }
#endif
      i += run = GetBits(bs,6);
#ifdef TRACE
      if (trace) {
        putchar(' ');
        printbits(ShowBits(bs,8),8,8);
      }
#endif
      level = GetBits(bs,8);

      if ((sign = (level>=128)))
        val = 256 - level;
      else 
        val = level;
    }
    else {
      i+= run;
      val = level;
      sign = GetBits(bs,1);
#ifdef TRACE
      if (trace)
        printf("%d",sign);
#endif
    }

    if (i >= 64)
    {
      if (!quiet)
        fprintf(stderr,"DCT coeff index (i) out of bounds\n");
      *fault_p = 1;
      return;
    }

#ifdef TRACE
    if (trace)
      printf("): %d/%d\n",run,sign ? -val : val);
#endif


    j = zig_zag_scan[i];
    qval = &bp[j];
    if (comp >= 6)
      QP = MAX (1, MIN( 31, ( bquant_tab[bquant] * quant) >> 2 ));
    else
      QP = quant;
      
    /* TMN3 dequantization */
    if ((QP % 2) == 1)
      *qval = ( sign ? -(QP * (2* val+1))  : QP * (2* val+1) );
    else
      *qval = ( sign ? -(QP * (2* val+1)-1): QP * (2* val+1)-1 );

    if (last) { /* That's it */
#ifdef TRACE
      if (trace)
        printf("last\n");
#endif
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


void _get_sac_block(H263Global *h263Data, int comp, int ptype)
{
  int position=0;
  int TCOEF_index, symbol_word;
  int last=0, QP, i, j;
  short *qval, *bp;
  RunCoef DCTcoef;

  bp = h263Data->block[comp];

  i = (ptype==0);

  while (!last) {	/* while there are DCT coefficients remaining */
    position++;	/* coefficient counter relates to Coeff. model */
    TCOEF_index = DecodeTCoef(h263Data->bs, position, !ptype);

    if (TCOEF_index == ESCAPE_INDEX) {        /* ESCAPE code encountered */
      DCTcoef = Decode_Escape_Char(h263Data->bs, !ptype, &last);
#ifdef TRACE
      if (trace)
        printf("ESC: ");
#endif
    }
    else {
      symbol_word = tcoeftab[TCOEF_index];

      DCTcoef = vlc_word_decode(h263Data->bs,symbol_word,&last);
    }

#ifdef TRACE
    if (trace) {
      printf("val: %d, run: %d, sign: %d, last: %d\n", 
             DCTcoef.val, DCTcoef.run, DCTcoef.sign, last);
    }
#endif

    i += DCTcoef.run;

    j = zig_zag_scan[i];

    qval = &bp[j];

    i++;

    if (comp >= 6)
      QP = MAX (1, MIN( 31, ( bquant_tab[h263Data->bquant] * h263Data->quant) >> 2 ));
    else 
      QP = h263Data->quant;

    if ((QP % 2) == 1)
      *qval = ( (DCTcoef.sign) ? -(QP * (2* (DCTcoef.val)+1))  : 
        QP * (2* (DCTcoef.val)+1) );
    else
      *qval = ( (DCTcoef.sign) ? -(QP * (2* (DCTcoef.val)+1)-1): 
        QP * (2* (DCTcoef.val)+1)-1 );
        
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

static RunCoef vlc_word_decode(Bitstream *bs, int symbol_word, int *last)
{
  int sign_index;
  RunCoef DCTcoef;

  *last = (symbol_word >> 12) & 01;
 
  DCTcoef.run = (symbol_word >> 4) & 255; 

  DCTcoef.val = (symbol_word) & 15;

  sign_index = _decode_a_symbol(bs,cumf_SIGN);	

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

static RunCoef Decode_Escape_Char(Bitstream *bs, int intra, int *last)
{
  int last_index, run, run_index, level, level_index;
  RunCoef DCTcoef;

  if (intra) {
    last_index = _decode_a_symbol(bs,cumf_LAST_intra);
    *last = last_intratab[last_index];
  }
  else {
    last_index = _decode_a_symbol(bs,cumf_LAST);
    *last = lasttab[last_index];
  }

  if (intra) 
    run_index = _decode_a_symbol(bs,cumf_RUN_intra);
  else
    run_index = _decode_a_symbol(bs,cumf_RUN);

  run = runtab[run_index];

  /*$if (mrun) run|=64;$*/

  DCTcoef.run = run;

  if (intra)
    level_index = _decode_a_symbol(bs,cumf_LEVEL_intra);
  else
    level_index = _decode_a_symbol(bs,cumf_LEVEL);

#ifdef TRACE
  if (trace)
    printf("level_idx: %d ",level_index);
#endif

  level = leveltab[level_index];

  if (level >128) 
    level -=256;

  if (level < 0) {
    DCTcoef.sign = 1;
    DCTcoef.val = abs(level);
  }

  else {	
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

static int DecodeTCoef(Bitstream *bs, int position, int intra)
{
  int index;

  switch (position) {
  case 1:
    {
      if (intra) 
        index = _decode_a_symbol(bs,cumf_TCOEF1_intra);
      else 
        index = _decode_a_symbol(bs,cumf_TCOEF1); 
      break; 
    }
  case 2:
    {
      if (intra) 
        index = _decode_a_symbol(bs,cumf_TCOEF2_intra);
      else
        index = _decode_a_symbol(bs,cumf_TCOEF2);
      break; 
    }
  case 3:
    {
      if (intra) 
        index = _decode_a_symbol(bs,cumf_TCOEF3_intra);
      else
        index = _decode_a_symbol(bs,cumf_TCOEF3);
      break; 
    }
  default: 
    {
      if (intra) 
        index = _decode_a_symbol(bs,cumf_TCOEFr_intra);
      else
        index = _decode_a_symbol(bs,cumf_TCOEFr);
      break; 
    }
  }

  return (index);
}
