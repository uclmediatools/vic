/************************************************************************
 *
 *  getvlc.c, variable length decoding for tmndecode (H.263 decoder)
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
#include "h263decoder.h"
#include "getvlc.h"

#include "defs.h"
#include "structs.h"
#include "bitIn.h"


int _getTMNMV(H263Global *h263Data)
{
  int code;

#ifdef TRACE
  if (trace)
    printf("motion_code (");
#endif

  if (GetOneBit(h263Data->bs))
  {
#ifdef TRACE
    if (trace)
      printf("1): 0\n");
#endif
    return 0;
  }

  if ((code = ShowBits(h263Data->bs,12))>=512)
  {
    code = (code>>8) - 2;
    ConsumeBits(h263Data->bs,TMNMVtab0[code].len);

#ifdef TRACE
    if (trace)
    {
      printf("0");
      printbits(code+2,4,TMNMVtab0[code].len);
      printf("): %d\n", TMNMVtab0[code].val);
    }
#endif

    return TMNMVtab0[code].val;
  }

  if (code>=128)
  {
    code = (code>>2) -32;
    ConsumeBits(h263Data->bs,TMNMVtab1[code].len);

#ifdef TRACE
    if (trace)
    {
      printf("0");
      printbits(code+32,10,TMNMVtab1[code].len);
      printf("): %d\n",TMNMVtab1[code].val);
    }
#endif

    return TMNMVtab1[code].val;
  }

  if ((code-=5)<0)
  {
    if (!h263Data->quiet)
      fprintf(stderr,"Invalid motion_vector code\n");
    h263Data->fault=1;
    return 0;
  }

  ConsumeBits(h263Data->bs,TMNMVtab2[code].len);

#ifdef TRACE
  if (trace)
  {
    printf("0");
    printbits(code+5,12,TMNMVtab2[code].len);
    printf("): %d\n",TMNMVtab2[code].val);
  }
#endif

  return TMNMVtab2[code].val;
}


int _getMCBPC(H263Global *h263Data)
{
  int code;

#ifdef TRACE
  if (trace)
    printf("MCBPC (");
#endif

  code = ShowBits(h263Data->bs,9);

  if (code == 1) {
    /* macroblock stuffing */
#ifdef TRACE
    if (trace)
      printf("000000001): stuffing\n");
#endif
    ConsumeBits(h263Data->bs,9);
    return 255;
  }

  if (code == 0) {
    if (!h263Data->quiet) 
      fprintf(stderr,"Invalid MCBPC code\n");
    h263Data->fault = 1;
    return 0;
  }
    
  if (code>=256)
  {
    ConsumeBits(h263Data->bs,1);
#ifdef TRACE
    if (trace)
      printf("1): %d\n",0);
#endif
    return 0;
  }
    
  ConsumeBits(h263Data->bs,MCBPCtab[code].len);

#ifdef TRACE
  if (trace)
  {
    printbits(code,9,MCBPCtab[code].len);
    printf("): %d\n",MCBPCtab[code].val);
  }
#endif

  return MCBPCtab[code].val;
}

int _getMODB(H263Global *h263Data)
{
  int code;
  int MODB;

#ifdef TRACE
  if (trace)
    printf("MODB (");
#endif

  code = ShowBits(h263Data->bs,2);

  if (code < 2) {
#ifdef TRACE
    if (trace)
      printf("0): MODB = 0\n");
#endif
    MODB = 0;
    ConsumeBits(h263Data->bs,1);
  }
  else if (code == 2) {
#ifdef TRACE
    if (trace)
      printf("10): MODB = 1\n");
#endif
    MODB = 1;
    ConsumeBits(h263Data->bs,2);
  }
  else { /* code == 3 */
#ifdef TRACE
    if (trace)
      printf("11): MODB = 2\n");
#endif
    MODB = 2;
    ConsumeBits(h263Data->bs,2);
  }
  return MODB;
}


int _getMCBPCintra(H263Global *h263Data)
{
  int code;

#ifdef TRACE
  if (trace)
    printf("MCBPCintra (");
#endif

  code = ShowBits(h263Data->bs,9);

  if (code == 1) {
    /* macroblock stuffing */
#ifdef TRACE
    if (trace)
      printf("000000001): stuffing\n");
#endif
    ConsumeBits(h263Data->bs,9);
    return 255;
  }

  if (code < 8) {
    if (!h263Data->quiet) 
      fprintf(stderr,"Invalid MCBPCintra code\n");
    h263Data->fault = 1;
    return 0;
  }

  code >>= 3;
    
  if (code>=32)
  {
    ConsumeBits(h263Data->bs,1);
#ifdef TRACE
    if (trace)
      printf("1): %d\n",3);
#endif
    return 3;
  }

  ConsumeBits(h263Data->bs,MCBPCtabintra[code].len);

#ifdef TRACE
  if (trace)
  {
    printbits(code,6,MCBPCtabintra[code].len);
    printf("): %d\n",MCBPCtabintra[code].val);
  }
#endif

  return MCBPCtabintra[code].val;
}

int getMCBPCintra_ext(Bitstream *bs, int quiet, int *fault_p)
{
  int code;

#ifdef TRACE
  if (trace)
    printf("MCBPCintra (");
#endif

  code = ShowBits(bs,9);

  if (code == 1) {
    /* macroblock stuffing */
#ifdef TRACE
    if (trace)
      printf("000000001): stuffing\n");
#endif
    ConsumeBits(bs,9);
    return 255;
  }

  if (code < 8) {
    if (!quiet) 
      fprintf(stderr,"Invalid MCBPCintra code\n");
    *fault_p = 1;
    return 0;
  }

  code >>= 3;
    
  if (code>=32)
  {
    ConsumeBits(bs,1);
#ifdef TRACE
    if (trace)
      printf("1): %d\n",3);
#endif
    return 3;
  }

  ConsumeBits(bs,MCBPCtabintra[code].len);

#ifdef TRACE
  if (trace)
  {
    printbits(code,6,MCBPCtabintra[code].len);
    printf("): %d\n",MCBPCtabintra[code].val);
  }
#endif

  return MCBPCtabintra[code].val;
}

int _getCBPY(H263Global *h263Data)
{
  int code;

#ifdef TRACE
  if (trace)
    printf("CBPY (");
#endif

  code = ShowBits(h263Data->bs,6);
  if (code < 2) {
    if (!h263Data->quiet) 
      fprintf(stderr,"Invalid CBPY code\n");
    h263Data->fault = 1;
    return -1;
  }
    
  if (code>=48)
  {
    ConsumeBits(h263Data->bs,2);
#ifdef TRACE
    if (trace)
      printf("11): %d\n",0);
#endif
    return 0;
  }

  ConsumeBits(h263Data->bs,CBPYtab[code].len);

#ifdef TRACE
  if (trace)
  {
    printbits(code,6,CBPYtab[code].len);
    printf("): %d\n",CBPYtab[code].val);
  }
#endif

  return CBPYtab[code].val;
}


int getCBPY_ext(Bitstream *bs, int quiet, int *fault_p)
{
  int code;

#ifdef TRACE
  if (trace)
    printf("CBPY (");
#endif

  code = ShowBits(bs,6);
  if (code < 2) {
    if (!quiet) 
      fprintf(stderr,"Invalid CBPY code\n");
    *fault_p = 1;
    return -1;
  }
    
  if (code>=48)
  {
    ConsumeBits(bs,2);
#ifdef TRACE
    if (trace)
      printf("11): %d\n",0);
#endif
    return 0;
  }

  ConsumeBits(bs,CBPYtab[code].len);

#ifdef TRACE
  if (trace)
  {
    printbits(code,6,CBPYtab[code].len);
    printf("): %d\n",CBPYtab[code].val);
  }
#endif

  return CBPYtab[code].val;
}

