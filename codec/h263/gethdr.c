/************************************************************************
 *
 *  gethdr.c, header decoding for tmndecode (H.263 decoder)
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
 */


#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "h263decoder.h"
#include "debug.h"

/* static prototypes */
static void getpicturehdr(H263Global *h263Data);
static int getgobhdr(H263Global *h263Data);


/*
 * decode headers from one input stream
 * until an End of Sequence or picture start code
 * is found
 */

int _getheader(H263Global *h263Data)
{
  unsigned int code;

#if 0
  /* look for startcode */
  startcode(h263Data->bs);
#endif
  code = GetBits(h263Data->bs, PSC_LENGTH);

  h263Data->gob = GetBits(h263Data->bs, 5);
  if (h263Data->gob == SE_CODE)
    return 0;

  if (h263Data->gob == 0) {
    getpicturehdr(h263Data);
    if (h263Data->first)
      InitH263Decoder(h263Data);
    if (h263Data->syntax_arith_coding)      /* reset decoder after receiving */
      _decoder_reset(h263Data->bs);	        /* fixed length PSC string */
  } else if (!h263Data->first) {
    code = getgobhdr(h263Data);
    if (h263Data->syntax_arith_coding) 
      _decoder_reset(h263Data->bs);  /* init. arithmetic decoder buffer after gob */
  } else {   /* We have not seen any picture header so far ! */
    return 0;
  }

  if (h263Data->decGOBsCounter == 0)
    newpictureinit(h263Data);

  if (!code)
    return 0;
  return h263Data->gob + 1;
}


#if 0
/* align to start of next startcode */

void startcode(Bitstream *bs)
{
  /* search for new picture start code */
  while (showbits(PSC_LENGTH)!=1l) 
         flushbits(1);
}
#endif


/* decode picture header */

static void getpicturehdr(H263Global *h263Data)
{
  int pei, tmp;
  int extendedPTYPE;
  Bitstream *bs = h263Data->bs;


  h263Data->prev_temp_ref = h263Data->temp_ref;
  h263Data->temp_ref = GetBits(h263Data->bs, 8);
  h263Data->trd = h263Data->temp_ref - h263Data->prev_temp_ref;

  if (h263Data->trd < 0)
    h263Data->trd += 256;

  /* Check ptype for change */
  tmp = ShowBits(bs, 13);
  if (1/*tmp != h263Data->ptype || h263Data->gfid > 3*/) {
    h263Data->gfid = -1;
    h263Data->ptype = tmp;
  }

  tmp = GetOneBit(bs); /* always "1" */
  if (!tmp)
    if (!h263Data->quiet)
      printf("warning: spare in picture header should be \"1\"\n");
  tmp = GetOneBit(bs); /* always "0" */
  if (tmp)
    if (!h263Data->quiet)
      printf("warning: H.261 distinction bit should be \"0\"\n");
  tmp = GetOneBit(bs); /* split_screen_indicator */
  if (tmp) {
    if (!h263Data->quiet)
      printf("error: split-screen not supported in this version\n");
    exit (-1);
  }
  tmp = GetOneBit(bs); /* document_camera_indicator */
  if (tmp)
    if (!h263Data->quiet)
      printf("warning: document camera indicator not supported in this "
	     "version\n");

  tmp = GetOneBit(bs); /* freeze_picture_release */
  if (tmp)
    if (!h263Data->quiet)
      printf("warning: frozen picture not supported in this version\n");

  if ((h263Data->source_format = GetBits(bs, 3)) == 7) {
    extendedPTYPE = 1;
    GetPLUSHEADER(bs,h263Data);
  } else {
    extendedPTYPE = 0;
    h263Data->pict_type = GetOneBit(bs);
    h263Data->mv_outside_frame = GetOneBit(bs);
    h263Data->long_vectors = (h263Data->mv_outside_frame ? 1 : 0);
    h263Data->syntax_arith_coding = GetOneBit(bs);
    h263Data->adv_pred_mode = GetOneBit(bs);
    assert(h263Data->adv_pred_mode==0);
/*debug_msg("gethdr:adv_pred_mode==0\n");*/

    h263Data->mv_outside_frame = (h263Data->adv_pred_mode ? 1 : h263Data->mv_outside_frame);
    h263Data->pb_frame = GetOneBit(bs);
  }

  h263Data->quant = GetBits(bs, 5);

  if (!extendedPTYPE) {
    tmp = GetOneBit(bs);
    if (tmp) {
      if (!h263Data->quiet)
	printf("error: CPM not supported in this version\n");
      exit(-1);
    }
  }

  if (h263Data->pb_frame) {
    h263Data->trb = GetBits(bs, 3);
    h263Data->bquant = GetBits(bs, 2);
  }
  else {
    h263Data->trb = 0;
  }

  pei = GetOneBit(bs);
pspare:
  if (pei) {
     /* extra info for possible future backward compatible additions */
    GetBits(bs, 8);  /* not used */
    pei = GetOneBit(bs);
    if (pei) goto pspare; /* keep on reading pspare until pei=0 */
  }
}


static int getgobhdr(H263Global *h263Data)
{
  int gfid;

  /*!@ GSBI fehlt @!*/
  /* GFID is not allowed to change unless PTYPE in picture header changes */
  gfid = GetBits(h263Data->bs,2);
  /* NB: in error-prone environments the decoder can use this
     value to determine whether a picture header where the PTYPE
     has changed, has been lost */
  if (h263Data->gfid >= 0) {
    if (h263Data->gfid != gfid) {
      h263Data->gfid = 4;
      /* Error concealment */
      return(0);
    }
  } else
    h263Data->gfid = gfid;  /* Store gfid */

  h263Data->quant = GetBits(h263Data->bs,5);
#ifdef TRACE
  if (trace)
    printf("GQUANT: %d\n", h263Data->quant);
#endif

  return(1);
}



/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- GetPLUSHEADER -- Read PLUS HEADER from a bitsream
 *
 * Author:           Niko Faerber
 * Created:          29-Jan-98
 *****************************************************************************/
int GetPLUSHEADER(Bitstream *bs, H263Global *h263Data)
/***********************************************************CommentEnd********/
{
  int tmp;

  /* PLUSPTYPE */
  if(!GetPLUSPTYPE(bs, h263Data)) return 0;
  /* CPM */
  tmp = GetOneBit(bs);
  if (tmp) {
    if (!h263Data->quiet)
      printf("error: CPM not supported in this version\n");
    exit(-1);
  }
  /* PSBI */
  /*if(pic->cpm) {
    if(!GetPSBI(bs,pic,cInfo)) return 0;
  }*/

  /* Note: For modes not jet supported, the demux should exits
   * after GetPLUSPTYPE(). Therefore, the coresponding
   * codewords are not read in the following. Blease extend when
   * adding a mode.
   */

  /* CPFMT */
  if (h263Data->ufep && h263Data->source_format == CUSTOM_SOURCE_FORMAT) {
    /* Pixel Aspect Ratio Code */
    GetBits(bs, 4);
    /* Picture width */
    h263Data->horizontal_size = (GetBits(bs, 9) + 1) * 4;
    if (GetOneBit(bs) != 1) {
      if (!h263Data->quiet)
	printf("error: This bit shall be 1\n");
      return 0;
    }
    /* Picture height */
    h263Data->vertical_size = GetBits(bs, 9) * 4;
  }
  /* EPAR */
  /* CPCFC */
  /* ETR */
  /* UUI */
  /* SS */
  /* ELNUM */
  /* RLNUM */

  /* RPS LAYER: RPSMF, TRPI, TRP, BCI, BCM */
  /*if(rps) {
    if(!GetRPSLayer(bs, pic, cInfo, 0)) return 0;
  }*/

  /* RPRP LAYER */
  
  return 1;
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- GetPLUSPTYPE -- Read PLUSPTYPE from a bitsream
 *
 * Author:           Niko Faerber
 * Created:          29-Jan-98
 *****************************************************************************/
int GetPLUSPTYPE(Bitstream *bs, H263Global *h263Data)
/***********************************************************CommentEnd********/
{
  int bits;
  int pcf, umv, aic, df, ss, rps, isd, aiv, mq, rpr, rru, rtype;


  /* UFEP (Update Full Extended PTYPE) */
  h263Data->ufep = GetBits(bs, 3);
  if (h263Data->ufep!=0 && h263Data->ufep!=1) {
    if (!h263Data->quiet)
      printf("error: ERROR_ILLEGAL_UFEP\n");
    return 0;
  }
  /* OPPTYPE (Optional Part) */
  if (h263Data->ufep) {
    h263Data->source_format = GetBits(bs,3);
    if(h263Data->source_format == 0) {
      if (!h263Data->quiet)
	printf("error: ERROR_ILLEGAL_SOURCE_FORMAT\n");
      return 0;
    }
    pcf = GetOneBit(bs);
    umv = GetOneBit(bs);
    h263Data->syntax_arith_coding = GetOneBit(bs);
    h263Data->adv_pred_mode = GetOneBit(bs);

    aic = GetOneBit(bs);
    df = GetOneBit(bs);
    ss = GetOneBit(bs);
    rps = GetOneBit(bs);
    isd = GetOneBit(bs);
    aiv = GetOneBit(bs);
    mq = GetOneBit(bs);
    if ((bits = GetBits(bs,4)) != 8) {
      if (!h263Data->quiet)
	printf("error: ERROR_ILLEGAL_OPPTYPE\n");
      return 0;
    }
  }
  /* MPPTYPE (Mandatory Part) */
  h263Data->pict_type = GetBits(bs,3);
  if (h263Data->pict_type == 6 || h263Data->pict_type == 7) {
    if (!h263Data->quiet)
      printf("error: ERROR_ILLEGAL_PICTURE_TYPE\n");
    return 0;
  }
  rpr = GetOneBit(bs);
  rru = GetOneBit(bs);
  rtype = GetOneBit(bs);
  if ((bits = GetBits(bs,3))!=1) {
    if (!h263Data->quiet)
      printf("error: ERROR_ILLEGAL_MPPTYPE\n");
    return 0;
  }

  return 1;
}
