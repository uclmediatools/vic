/************************************************************************
 *
 *  getvlc.c, variable length decoding for tmndecode (H.263 decoder)
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

#include "config.h"
#include "tmndec.h"
#include "global.h"
#include "getvlc.h"

int getTMNMV ()
{
  int code;

  if (trace)
    fprintf (trace_file, "motion_code (");

  if (getbits1 ())
  {
    if (trace)
      fprintf (trace_file, "1): 0\n");
    return 0;
  }
  if ((code = showbits (12)) >= 512)
  {
    code = (code >> 8) - 2;
    flushbits (TMNMVtab0[code].len);

    if (trace)
    {
      fprintf (trace_file, "0");
      printbits (code + 2, 4, TMNMVtab0[code].len);
      fprintf (trace_file, "): %d\n", TMNMVtab0[code].val);
    }
    return TMNMVtab0[code].val;
  }
  if (code >= 128)
  {
    code = (code >> 2) - 32;
    flushbits (TMNMVtab1[code].len);

    if (trace)
    {
      fprintf (trace_file, "0");
      printbits (code + 32, 10, TMNMVtab1[code].len);
      fprintf (trace_file, "): %d\n", TMNMVtab1[code].val);
    }
    return TMNMVtab1[code].val;
  }
  if ((code -= 5) < 0)
  {
    if (!quiet)
      fprintf (stderr, "Invalid motion_vector code\n");
    fault = 1;
    return 0;
  }
  flushbits (TMNMVtab2[code].len);

  if (trace)
  {
    fprintf (trace_file, "0");
    printbits (code + 5, 12, TMNMVtab2[code].len);
    fprintf (trace_file, "): %d\n", TMNMVtab2[code].val);
  }
  return TMNMVtab2[code].val;
}

/**********************************************************************
 *
 *      Name:        getRVLC
 *      Description: extracts RVLC for motion vectors in Annex D
 *
 *      Input:
 *
 *      Returns:     motion vector component
 *      Side effects:
 *
 *      Date: 971026 Author: Guy Cote -- guyc@ee.ubc.ca
 *
 ***********************************************************************/
 
int getRVLC ()
{
 
  int code = 0, sign;
 
  if (trace)
    fprintf (trace_file, "motion_code (");
 
  if (getbits1 ())
  {
    if (trace)
      fprintf (trace_file, "1): 0\n");
    return 0;
  }
  code = 2 + vic_getbits(1);
  while (vic_getbits(1))
  {
    code <<= 1;
    code += vic_getbits(1);
  }
  sign = code & 1;
  code >>= 1;
  if (trace)
  {
    fprintf (trace_file, "RVLC");
    fprintf (trace_file, "): %d\n", (sign) ? -code : code);
  }
  return (sign) ? -code : code;
}

int getMCBPC ()
{
  int code;

  if (trace)
    fprintf (trace_file, "MCBPC (");

  code = showbits (13);

  if (code >> 4 == 1)
  {
    /* macroblock stuffing */
    if (trace)
      fprintf (trace_file, "000000001): stuffing\n");
    flushbits (9);
    return 255;
  }
  if (code == 0)
  {
    if (!quiet)
      fprintf (stderr, "Invalid MCBPC code\n");
    fault = 1;
    return 0;
  }
  if (code >= 4096)
  {
    flushbits (1);
    if (trace)
      fprintf (trace_file, "1): %d\n", 0);
    return 0;
  }
  if (code >= 16)
  {
    flushbits (MCBPCtab0[code >> 4].len);
    if (trace)
    {
      printbits (code >> 4, 9, MCBPCtab0[code >> 4].len);
      fprintf (trace_file, "): %d\n", MCBPCtab0[code >> 4].val);
    }
    return MCBPCtab0[code >> 4].val;
  } else
  {
    flushbits (MCBPCtab1[code - 8].len);
    if (trace)
    {
      printbits (code, 13, MCBPCtab1[code - 8].len);
      fprintf (trace_file, "): %d\n", MCBPCtab1[code - 8].val);
    }
    return MCBPCtab1[code - 8].val;
  }
}

/**********************************************************************
 *
 *	Name:        getMBTYPE
 *	Description: extracts annex O MBTYPE information from bitstream
 *               and whether or not CBP or Quant fields 
 *               follow
 *
 *	Input:       pointers for cbp and dquant (to be filled in from
 *               MBTYPE table in annex O).
 *
 *	Returns:
 *	Side effects:
 *
 *	Date: 971102 Author: Michael Gallant --- mikeg@ee.ubc.ca
 *
 ***********************************************************************/
int getMBTYPE (int *cbp_present, int *quant_present)
{ 
  int code;

  if (trace)
    fprintf (trace_file, "MBTYPE (");

  code = showbits (9);
  
  if (code == 1)
  {
    /* macroblock stuffing */
    if (trace)
      fprintf (trace_file, "000000001): stuffing\n");
    flushbits (9);
    *cbp_present = *quant_present = NO;
    return B_EI_EP_STUFFING;

  }

  switch (pict_type)
  {
    case PCT_B:

      if (code < 4)
      {
        if (!quiet)
          fprintf (stderr, "Invalid MBTYPE code\n");
        fault = 1;
        *cbp_present = *quant_present = -1;
        return INVALID_MBTYPE;
      }

      code >>= 2;

      if (code >= 96)
      {
        flushbits (2);
        if (trace)
          fprintf (trace_file, "11): %d\n", 0);
        *cbp_present = YES;
        *quant_present = NO;
        return B_DIRECT_PREDICTION;
      }
      else
      {
        flushbits (MBTYPEtabB[code].len);
        if (trace)
        {
          printbits (code, 7, MBTYPEtabB[code].len);
          fprintf (trace_file, "): %d\n", MBTYPEtabB[code].val);
        }
        *cbp_present = CBP_present_B[MBTYPEtabB[code].val];
        *quant_present = QUANT_present_B[MBTYPEtabB[code].val];
        return PRED_type_B[MBTYPEtabB[code].val];
      }

      break;

    case PCT_EP:

      if (code < 2)
      {
        if (!quiet)
          fprintf (stderr, "Invalid MBTYPE code\n");
        fault = 1;
        *cbp_present = *quant_present = -1;
        return INVALID_MBTYPE;
      }

      code >>= 1;

      if (code >= 128)
      {
        flushbits (1);
        if (trace)
          fprintf (trace_file, "1): %d\n", 0);
        *cbp_present = YES;
        *quant_present = NO;
        return EP_FORWARD_PREDICTION;
      }
      else
      {
        flushbits (MBTYPEtabEP[code].len);
        if (trace)
        {
          printbits (code, 8, MBTYPEtabEP[code].len);
          fprintf (trace_file, "): %d\n", MBTYPEtabEP[code].val);
        }
        *cbp_present = CBP_present_EP[MBTYPEtabEP[code].val];
        *quant_present = QUANT_present_EP[MBTYPEtabEP[code].val];
        return PRED_type_EP[MBTYPEtabEP[code].val];
      }

      break;

    case PCT_EI:

      /* In the case of EI pictures, we use cbp_present to return the chrominance 
       * coded block pattern */
      if (code < 2)
      {
        if (!quiet)
          fprintf (stderr, "Invalid MBTYPE code\n");
        fault = 1;
        *cbp_present = *quant_present = -1;
        return INVALID_MBTYPE;
      }

      code >>= 1;
  
      if (8 == code)
      {
        if (!quiet)
          fprintf (stderr, "Invalid MBTYPE code\n");
        fault = 1;
        *cbp_present = *quant_present = -1;
        return INVALID_MBTYPE;
      }
  
      if (code >= 128)
      {
        flushbits (1);
        if (trace)
          fprintf (trace_file, "1): %d\n", 0);
        *cbp_present = 0;
        *quant_present = NO;
        return EI_EP_UPWARD_PREDICTION;
      }
      else
      {
        flushbits (MBTYPEtabEI[code].len);
        if (trace)
        {
          printbits (code, 8, MBTYPEtabEI[code].len);
          fprintf (trace_file, "): %d\n", MBTYPEtabEI[code].val);
        }
        *cbp_present = CBPC_pattern_EI[MBTYPEtabEI[code].val];
        *quant_present = QUANT_present_EI[MBTYPEtabEI[code].val];
        return PRED_type_EI[MBTYPEtabEI[code].val];
      }

      break;
      
    default:

      break;

  }    

  //should not reach here
  return INVALID_MBTYPE; //XXX
}

int getMODB ()
{
  int code;
  int MODB;

  if (trace)
    fprintf (trace_file, "MODB (");

  if (pict_type == PCT_IPB)
  {
    code = vic_getbits (1);
    if (code == 0)
    {
      MODB = PBMODE_BIDIR_PRED;
      if (trace)
        fprintf (trace_file, "0): 0\n");
    }
    else
    {
      code=vic_getbits (1);
      if (code==0)
      {
        MODB = PBMODE_CBPB_BIDIR_PRED;
        if (trace)
          fprintf (trace_file, "10): 2\n");
      }
      else
      {
        code=vic_getbits (1);
        if (code==0) 
        {
          MODB = PBMODE_FRW_PRED;
          if (trace)
            fprintf (trace_file, "110): 6\n");
        }
        else
        {
          code=vic_getbits (1);
          if (code==0) 
          {
            MODB = PBMODE_CBPB_FRW_PRED;
            if (trace)
              fprintf (trace_file, "1110): 14\n");
          }
          else
          {
            code=vic_getbits (1);
            if (code==0) 
            {
              MODB = PBMODE_BCKW_PRED;
              if (trace)
                fprintf (trace_file, "11110): 30\n");
            }
            else
            {
              MODB = PBMODE_CBPB_BCKW_PRED;
              if (trace)
                fprintf (trace_file, "11111): 31\n");
            }
          }
        }
      }
    }
  } else
  {
    code = showbits (2);

    if (code < 2)
    {
      if (trace)
        fprintf (trace_file, "0): 0\n");
      MODB = 0;
      flushbits (1);
    } else if (code == 2)
    {
      if (trace)
        fprintf (trace_file, "10): 1\n");
      MODB = 1;
      flushbits (2);
    } else
    {
      /* code == 3 */
      if (trace)
        fprintf (trace_file, "11): 2\n");
      MODB = 2;
      flushbits (2);
    }
  }

  return MODB;

}


int getMCBPCintra ()
{
  int code;

  if (trace)
    fprintf (trace_file, "MCBPCintra (");

  code = showbits (9);

  if (code == 1)
  {
    /* macroblock stuffing */
    if (trace)
      fprintf (trace_file, "000000001): stuffing\n");
    flushbits (9);
    return 255;

  }
  if (code < 8)
  {
    if (!quiet)
      fprintf (stderr, "Invalid MCBPCintra code\n");
    fault = 1;
    return 0;
  }
  code >>= 3;

  if (code >= 32)
  {
    flushbits (1);
    if (trace)
      fprintf (trace_file, "1): %d\n", 3);
    return 3;
  }
  flushbits (MCBPCtabintra[code].len);

  if (trace)
  {
    printbits (code, 6, MCBPCtabintra[code].len);
    fprintf (trace_file, "): %d\n", MCBPCtabintra[code].val);
  }
  return MCBPCtabintra[code].val;
}

/* extract vlc representing true B CBPC --- mikeg@ee.ubc.ca */
int getscalabilityCBPC ()
{
  int code;

  if (trace)
    fprintf (trace_file, "CBPC: ");

  code = showbits (1);

  if (0 == code)
  {
    if (trace)
      fprintf (trace_file, "0 - Blocks 56: 00\n");
    flushbits (1);
    return 0;
  } else
  {
    code = showbits (2);

    if (2 == code)
    {
      if (trace)
        fprintf (trace_file, "10 - Blocks 56: 01\n");
      flushbits (2);
      return 1;
    } else
    {
      code = showbits (3);

      if (6 == code)
      {
        if (trace)
          fprintf (trace_file, "110 - Blocks 56: 11\n");
        flushbits (3);
        return 3;
      } else if (7 == code)
      {
        if (trace)
          fprintf (trace_file, "111 - Blocks 56: 10\n");
        flushbits (3);
        return 2;
      } else
      {
        if (!quiet)
          printf ("Invalid chromiance CBP in getscalabilityCBPC (getvlc.c).\n");
        fault = 1;
        return 0;
      }
    }
  }
}

int getCBPY ()
{
  int code;

  if (trace)
    fprintf (trace_file, "CBPY (");

  code = showbits (6);
  if (code < 2)
  {
    if (!quiet)
      fprintf (stderr, "Invalid CBPY code\n");
    fault = 1;
    return -1;
  }
  if (code >= 48)
  {
    flushbits (2);
    if (trace)
      fprintf (trace_file, "11): %d\n", 0);
    return 0;
  }
  flushbits (CBPYtab[code].len);

  if (trace)
  {
    printbits (code, 6, CBPYtab[code].len);
    fprintf (trace_file, "): %d\n", CBPYtab[code].val);
  }
  return CBPYtab[code].val;
}
