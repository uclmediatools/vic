/************************************************************************
 *
 *  sac.c, part of tmndecode (H.263 decoder)
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


/*********************************************************************
 *        SAC Decoder Module
 *        Algorithm as Specified in H26P Annex -E
 *              (c) 1995 BT Labs
 *
 *	Author:	Wayne Ellis <ellis_w_wayne@bt-web.bt.co.uk>
 *
 *********************************************************************/

#include <stdio.h>
#include <string.h>

#include "h263decoder.h"
#include "bitIn.h"

#define   q1    16384
#define   q2    32768
#define   q3    49152
#define   top   65535

/* local prototypes */
static void _bit_out_psc_layer(Bitstream *bs);

/*********************************************************************
 *        SAC Decoder Algorithm as Specified in H26P Annex -E
 *
 *        Name:        decode_a_symbol
 *
 *	Description:	Decodes an Aritmetically Encoded Symbol
 *
 *	Input:        array holding cumulative freq. data
 *        also uses static data for decoding endpoints
 *        and code_value variable
 *
 *	Returns:	Index to relevant symbol model
 *
 *	Side Effects:	Modifies low, high, length, cum and code_value
 *
 *	Author:        Wayne Ellis <ellis_w_wayne@bt-web.bt.co.uk>
 *
 *********************************************************************/
 
static long low, high, code_value, bit, length, sacindex, cum, zerorun=0;

int _decode_a_symbol(Bitstream *bs, int cumul_freq[ ])
{

  length = high - low + 1;
  cum = (-1 + (code_value - low + 1) * cumul_freq[0]) / length;
  for (sacindex = 1; cumul_freq[sacindex] > cum; sacindex++);
  high = low - 1 + (length * cumul_freq[sacindex-1]) / cumul_freq[0];
  low += (length * cumul_freq[sacindex]) / cumul_freq[0];

  for ( ; ; ) {  
    if (high < q2) ;
    else if (low >= q2) {
      code_value -= q2; 
      low -= q2; 
      high -= q2;
    }
    else if (low >= q1 && high < q3) {
      code_value -= q1; 
      low -= q1; 
      high -= q1;
    }
    else {
      break;
    }
 
    low *= 2; 
    high = 2*high + 1;
    _bit_out_psc_layer(bs);
    code_value = 2*code_value + bit;
  }

  return (sacindex-1);
}
 
/*********************************************************************
 *
 *        Name:        decoder_reset
 *
 *	Description:	Fills Decoder FIFO after a fixed word length
 *        string has been detected.
 *
 *	Input:        None
 *
 *	Returns:	Nothing
 *
 *	Side Effects:	Fills Arithmetic Decoder FIFO
 *
 *	Author:        Wayne Ellis <ellis_w_wayne@bt-web.bt.co.uk>
 *
 *********************************************************************/

void _decoder_reset(Bitstream *bs)
{
  int i;
  zerorun = 0;        /* clear consecutive zero's counter */
  code_value = 0;
  low = 0;
  high = top;
  for (i = 1;   i <= 16;   i++) {
    _bit_out_psc_layer(bs); 
    code_value = 2 * code_value + bit;
  }
#ifdef TRACE
  if (trace)
    printf("Arithmetic Decoder Reset \n");
#endif
}

/*********************************************************************
 *
 *        Name:        bit_out_psc_layer
 *
 *	Description:	Gets a bit from the Encoded Stream, Checks for
 *        and removes any PSC emulation prevention bits
 *        inserted at the decoder, provides 'zeros' to the
 *        Arithmetic Decoder FIFO to allow it to finish 
 *        data prior to the next PSC. (Garbage bits)
 *
 *	Input:        None
 *
 *	Returns:	Nothing
 *
 *	Side Effects:	Gets a bit from the Input Data Stream
 *
 *	Author:        Wayne Ellis <ellis_w_wayne@bt-web.bt.co.uk>
 *
 *********************************************************************/

static void _bit_out_psc_layer(Bitstream *bs)
{
  if (ShowBits(bs,17)!=1) { /* check for startcode in Arithmetic Decoder FIFO */

    bit = GetOneBit(bs);

    if(zerorun > 13) {	/* if number of consecutive zeros = 14 */	 
      if (!bit) {
#ifdef TRACE
        if (trace)
          printf("PSC/GBSC, Header Data, or Encoded Stream Error \n");
#endif
        zerorun = 1;        
      }
      else { /* if there is a 'stuffing bit present */
#ifdef TRACE
        if (trace)
          printf("Removing Startcode Emulation Prevention bit \n");
#endif
        bit = GetOneBit(bs);        /* overwrite the last bit */	
        zerorun = !bit;        /* zerorun=1 if bit is a '0' */
      }
    }

    else { /* if consecutive zero's not exceeded 14 */

      if (!bit)
        zerorun++;
      else
        zerorun = 0;
    }

  } /* end of if !(ShowBits(bs,17)) */

  else {
    bit = 0;
#ifdef TRACE
    if (trace)
      printf("Startcode Found:Finishing Arithmetic Decoding using "
	     "'Garbage bits'\n");
#endif
  }

   /*	
   printf("lastbit = %ld bit = %ld zerorun = %ld \n", lastbit, bit, zerorun); 
   lastbit = bit;
   */
        /* latent diagnostics */
        
}

