/************************************************************************
 *
 *  getgob.c, gob decoding for tmndecode (H.263 decoder)
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
 *  Changed by Klaus Stuhlmueller
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
 * modified by Wayne Ellis BT Labs to run Annex E Arithmetic Decoding
 *           <ellis_w_wayne@bt-web.bt.co.uk>
 *
 * based on mpeg2decode, (C) 1994, MPEG Software Simulation Group
 * and mpeg2play, (C) 1994 Stefan Eckart
 *                         <stefan@lis.e-technik.tu-muenchen.de>
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Util.h"
#include "h263decoder.h"
#include "h263decoder.p"
#include "bitIn.h"

#include "indices.h"
#include "sactbls.h"

//SV-XXX: defined UNUSED() macro for unused variables
#ifndef UNUSED
#define UNUSED(x) (x) = (x)
#endif


/* private prototypes*/
static void _clearblock (short block[12][64], int comp);
static int _motion_decode (int long_vectors, int vec,int pmv);
static int _find_pmv (H263Global *h263Data, int x, int y, int block, int comp);
static void _addblock (H263Global *h263Data, int comp, int bx, int by,int addflag);
static void addblock_ext (short *bp, int comp, int bx, int by, int w, int h,
			  unsigned char *y, unsigned char *u, unsigned char *v,
			  int addflag);
static void _reconblock_b (H263Global *h263Data, int comp, int bx, int by,
			  int mode, int bdx, int bdy);
static void _find_bidir_limits (int vec, int *start, int*stop, int nhv);
static void _find_bidir_chroma_limits (int vec, int *start, int*stop);
void interpolate_image (unsigned char *in, unsigned char *out,
			int width, int height);

/* not included in h263+ code */
int modb_tab[3] = {0, 1, 2};
int cumf_MODB[4]={16383, 6062, 2130, 0};
int cumf_MCBPC[22]={16383, 4105, 3088, 2367, 1988, 1621, 1612, 1609, 1608, 496, 353, 195, 77, 22, 17, 12, 5, 4, 3, 2, 1, 0};

/* decode all macroblocks of the current gob */

void H263getGOB(H263Global *h263Data, int dec_until_sync)
{
  const int blk_cnt = 6;
  int comp;
  int MBA, MBAmax;
  int bx, by;

  int COD=0,MCBPC, CBPY, CBP=0, CBPB=0, MODB=0, Mode=0, DQUANT;
  int COD_index, CBPY_index, MODB_index, DQUANT_index, MCBPC_index;
  int INTRADC_index, YCBPB_index, UVCBPB_index, mvdbx_index, mvdby_index;
  int mvx, mvy, mvy_index, mvx_index, pmv0, pmv1, xpos, ypos, i,k;
  int mvdbx=0, mvdby=0, pmvdbx=0, pmvdby=0, YCBPB, UVCBPB, gobheader_read;
  int startmv,stopmv,xoffset,bsize,last_done=0,pCBP=0,pCBPB=0,pCOD=0;
  int DQ_tab[4] = {-1,-2,1,2};
  short *bp;

  /* number of macroblocks per picture */
  MBAmax = h263Data->mbs_in_gob;

  MBA = 0;        /* macroblock address */

  if (h263Data->gob == 0) {
    h263Data->newgob = 0;
    gobheader_read = 0;
    xpos = -1;
    ypos = 0;
  } else {
    h263Data->newgob = 1;
    gobheader_read = 1;
    xpos = 0;
    ypos = h263Data->gob * h263Data->mbs_in_gob / h263Data->mb_width;
  }

  h263Data->fault = 0;
  
  for (;;) {

#ifdef TRACE
    if (trace)
      printf("MB %d\n",MBA);
#endif
  resync:

    /* This version of the decoder does not resync on every possible
       error, and it does not do all possible error checks. It is not
       difficult to make it much more error robust, but I do not think
       it is necessary to include this in the freely available
       version. */

    if (h263Data->fault) {
      printf("Warning: A Fault Condition Has Occurred - Resyncing \n");
#if 0
      startcode(h263Data->bs);  /* sync on new startcode */
#endif
      h263Data->fault = 0;
    }

    if (MBA >= MBAmax) {    /* Check return conditions */
      h263Data->decGOBsCounter++;
      h263Data->decGOBs[h263Data->gob] = 1;
      if (dec_until_sync) {
	if (!(ShowBits(h263Data->bs,22)>>6))  /* startcode */
	  return;
	else if (h263Data->bs->BufferPtr > h263Data->bs->GOBend)
	  return;
	else {    /* No sync following, thus continue decoding */
	  h263Data->newgob = 0;
	  MBA = 0;
	  xpos = -1;
	  last_done = 0;
	  COD = 0;
	}
      } else
	return;
    }

#if 0
  finish_gob:  /* SAC specific label */
#endif

    if (!gobheader_read) {
      xpos++;
      if (xpos >= h263Data->mb_width) {
	xpos = 0;
	ypos++;
      }
      if (xpos == 0 && ypos > 0)
        h263Data->newgob = 0;
    }
    else 
      gobheader_read = 0;

    if (MBA >= MBAmax) 
      return; /* all macroblocks decoded */

  read_cod:
    if (h263Data->syntax_arith_coding) { 
      if (h263Data->pict_type == PCT_INTER) {
        COD_index = _decode_a_symbol(h263Data->bs,cumf_COD);
        COD = codtab[COD_index];
#ifdef TRACE
        if (trace) {
          printf("Arithmetic Decoding Debug \n");
          printf("COD Index: %d COD: %d \n", COD_index, COD);
        }
#endif
      }
      else
        COD = 0;  /* COD not used in I-pictures, set to zero */
    }
    else { 
      if (h263Data->pict_type == PCT_INTER) 
        COD = ShowBits(h263Data->bs,1);
      else
        COD = 0; /* Intra picture -> not skipped */
    }

    if (!COD) {  /* COD == 0 --> not skipped */    

      if (h263Data->syntax_arith_coding)  {
        if (h263Data->pict_type == PCT_INTER) {
          MCBPC_index = _decode_a_symbol(h263Data->bs,cumf_MCBPC);
          MCBPC = mcbpctab[MCBPC_index];
        }	
        else {
          MCBPC_index = _decode_a_symbol(h263Data->bs,cumf_MCBPC_intra);
          MCBPC = mcbpc_intratab[MCBPC_index];
        }
#ifdef TRACE
        if (trace) 
          printf("MCBPC Index: %d MCBPC: %d \n",MCBPC_index, MCBPC);
#endif
      }

      else {
        if (h263Data->pict_type == PCT_INTER)
          ConsumeBits(h263Data->bs, 1); /* flush COD bit */
        if (h263Data->pict_type == PCT_INTRA) 
          MCBPC = _getMCBPCintra(h263Data);
        else
          MCBPC = _getMCBPC(h263Data);
      }

      if (h263Data->fault) goto resync;
      
      if (MCBPC == 255) { /* stuffing */
        goto read_cod;   /* read next COD without advancing MB count */
      }

      else {             /* normal MB data */

        Mode = MCBPC & 7;

        /* MODB and CBPB */
        if (h263Data->pb_frame) {
          CBPB = 0;
          if (h263Data->syntax_arith_coding)  {
            MODB_index = _decode_a_symbol(h263Data->bs,cumf_MODB);
            MODB = modb_tab[MODB_index];
          }
          else 
            MODB = _getMODB(h263Data);
#ifdef TRACE
          if (trace)
            printf("MODB: %d\n", MODB);
#endif
          if (MODB == PBMODE_CBPB_MVDB) {
            if (h263Data->syntax_arith_coding)  {
              for(i=0; i<4; i++) {
		YCBPB_index = _decode_a_symbol(h263Data->bs,cumf_YCBPB);
		YCBPB = ycbpb_tab[YCBPB_index];
		CBPB |= (YCBPB << (6-1-i));
              }
 
              for(i=4; i<6; i++) {
		UVCBPB_index = _decode_a_symbol(h263Data->bs,cumf_UVCBPB);
		UVCBPB = uvcbpb_tab[UVCBPB_index];
		CBPB |= (UVCBPB << (6-1-i));
              }
            }
            else
              CBPB = GetBits(h263Data->bs,6);
#ifdef TRACE
            if (trace)
              printf("CBPB = %d\n",CBPB);
#endif
          }
        }

        if (h263Data->syntax_arith_coding) {

          if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) { /* Intra */
            CBPY_index = _decode_a_symbol(h263Data->bs,cumf_CBPY_intra);
            CBPY = cbpy_intratab[CBPY_index];
          }
          else {
            CBPY_index = _decode_a_symbol(h263Data->bs,cumf_CBPY);
            CBPY = cbpytab[CBPY_index];
        
          }
#ifdef TRACE
          if (trace)
            printf("CBPY Index: %d CBPY %d \n",CBPY_index, CBPY);
#endif

        }
        else 
          CBPY = _getCBPY(h263Data);
 
        /* Decode Mode and CBP */
        
        
        if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
          {/* Intra */
            if (!h263Data->syntax_arith_coding)	
              CBPY = CBPY^15;        /* needed in huffman coding only */
          }

        CBP = (CBPY << 2) | (MCBPC >> 4);
      }

      if (Mode == MODE_INTER4V && !h263Data->adv_pred_mode) 
        if (!h263Data->quiet)
          printf("8x8 vectors not allowed in normal prediction mode\n");
          /* Could set fault-flag and resync */


      if (Mode == MODE_INTER_Q || Mode == MODE_INTRA_Q) {
        /* Read DQUANT if necessary */

        if (h263Data->syntax_arith_coding) {
          DQUANT_index = _decode_a_symbol(h263Data->bs,cumf_DQUANT);
          DQUANT = dquanttab[DQUANT_index] - 2; 
          h263Data->quant +=DQUANT;
#ifdef TRACE
          if (trace)
            printf("DQUANT Index: %d DQUANT %d \n",DQUANT_index, DQUANT);
#endif
        }
        else {
          DQUANT = GetBits(h263Data->bs,2);
          h263Data->quant += DQ_tab[DQUANT];
#ifdef TRACE
          if (trace) {
            printf("DQUANT (");
            printbits(DQUANT,2,2);
            printf("): %d = %d\n",DQUANT,DQ_tab[DQUANT]);
          }
#endif
        }

        if (h263Data->quant > 31 || h263Data->quant < 1) {
          if (!h263Data->quiet)
            printf("Quantizer out of range: clipping\n");
          h263Data->quant = MAX(1,MIN(31,h263Data->quant));
          /* could set fault-flag and resync here */
        }
      }

      /* motion vectors */
      if (Mode == MODE_INTER || Mode == MODE_INTER_Q || 
          Mode == MODE_INTER4V || h263Data->pb_frame) {

        if (Mode == MODE_INTER4V) { startmv = 1; stopmv = 4;}
        else { startmv = 0; stopmv = 0;}

        for (k = startmv; k <= stopmv; k++) {
          if (h263Data->syntax_arith_coding) {
            mvx_index = _decode_a_symbol(h263Data->bs,cumf_MVD);
            mvx = mvdtab[mvx_index];
            mvy_index = _decode_a_symbol(h263Data->bs,cumf_MVD);
            mvy = mvdtab[mvy_index];
#ifdef TRACE
            if (trace)
              printf("mvx_index: %d mvy_index: %d \n", mvy_index, mvx_index);
#endif
          }
          else {
            mvx = _getTMNMV(h263Data);
            mvy = _getTMNMV(h263Data);
          }

          pmv0 = _find_pmv(h263Data,xpos,ypos,k,0);
          pmv1 = _find_pmv(h263Data,xpos,ypos,k,1);
          mvx = _motion_decode(h263Data->long_vectors, mvx, pmv0);
          mvy = _motion_decode(h263Data->long_vectors, mvy, pmv1);
#ifdef TRACE
          if (trace) {
            printf("mvx: %d\n", mvx);
            printf("mvy: %d\n", mvy);
          }
#endif
          /* Check mv's to prevent seg.faults when error rate is high */
          if (!h263Data->mv_outside_frame) {
            bsize = k ? 8 : 16;
            xoffset = k ? (((k-1)&1)<<3) : 0;
            /* checking only integer component */
            if ((xpos<<4) + (mvx/2) + xoffset < 0 ||
		(xpos<<4) + (mvx/2) + xoffset > (h263Data->mb_width<<4) - bsize) {
              if (!h263Data->quiet)
		printf("mvx (%d) out of range (xpos is %d): searching for sync\n",mvx,xpos);
              h263Data->fault = 1;
            }
            xoffset = k ? (((k-1)&2)<<2) : 0;
            if ((ypos<<4) + (mvy/2) + xoffset < 0 ||
		(ypos<<4) + (mvy/2) + xoffset > (h263Data->mb_height<<4) - bsize) {
              if (!h263Data->quiet)
		printf("mvy (%d) out of range (ypos is %d): searching for sync\n",mvy,ypos);
              h263Data->fault = 1;
            }
          }
          h263Data->MV[0][k][ypos+1][xpos+1] = mvx;
          h263Data->MV[1][k][ypos+1][xpos+1] = mvy;
        }

        /* PB frame delta vectors */

        if (h263Data->pb_frame) {
          if (MODB == PBMODE_MVDB || MODB == PBMODE_CBPB_MVDB) {
            if (h263Data->syntax_arith_coding) {
              mvdbx_index = _decode_a_symbol(h263Data->bs,cumf_MVD);
              mvdbx = mvdtab[mvdbx_index];
              
              mvdby_index = _decode_a_symbol(h263Data->bs,cumf_MVD);
              mvdby = mvdtab[mvdby_index];
            }
            else {
              mvdbx = _getTMNMV(h263Data);
              mvdby = _getTMNMV(h263Data);
            }


            mvdbx = _motion_decode(h263Data->long_vectors, mvdbx, 0);
            mvdby = _motion_decode(h263Data->long_vectors, mvdby, 0);
            /* This will not work if the PB deltas are so large they
               require the second colums of the motion vector VLC
               table to be used.  To fix this it is necessary to
               calculate the MV predictor for the PB delta: TRB*MV/TRD
               here, and use this as the second parameter to
               motion_decode(). The B vector itself will then be
               returned from motion_decode(). This will have to be
               changed to the PB delta again, since it is the PB delta
               which is used later in this program. I don't think PB
               deltas outside the range mentioned above is useful, but
               you never know... */

#ifdef TRACE
            if (trace) {
              printf("MVDB x: %d\n", mvdbx);
              printf("MVDB y: %d\n", mvdby);
            }
#endif
          }
          else {
            mvdbx = 0;
            mvdby = 0;
          }
        }
      }

      if (h263Data->fault) goto resync;

    }
    else { /* COD == 1 --> skipped MB */
      if (MBA >= MBAmax)
        return; /* all macroblocks decoded */
      if (!h263Data->syntax_arith_coding)
        if (h263Data->pict_type == PCT_INTER)
          ConsumeBits(h263Data->bs,1);

      Mode = MODE_UNCODED;
      
      /* Reset CBP */
      CBP = CBPB = 0;

      /* reset motion vectors */
      h263Data->MV[0][0][ypos+1][xpos+1] = 0;
      h263Data->MV[1][0][ypos+1][xpos+1] = 0;
      mvdbx = 0;
      mvdby = 0;
    }

    /* Store Mode*/
    h263Data->modemap[ypos+1][xpos+1] = Mode;

    if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) 
      if (!h263Data->pb_frame)
        h263Data->MV[0][0][ypos+1][xpos+1] = h263Data->MV[1][0][ypos+1][xpos+1] = 0;


  reconstruct_mb:

    /* pixel coordinates of top left corner of current macroblock */
    /* one delayed because of OBMC */
    if (xpos > 0) {
      bx = 16*(xpos-1);
      by = 16*ypos;
    }
    else {
      bx = h263Data->coded_picture_width-16;
      by = 16*(ypos-1);
    }

    if (MBA > 0) {

      Mode = h263Data->modemap[by/16+1][bx/16+1];

      /* forward motion compensation for B-frame */
      if (h263Data->pb_frame)
        reconstructH263(h263Data,bx,by,0,pmvdbx,pmvdby);
      
      /* motion compensation for P-frame */
      if (Mode == MODE_INTER || Mode == MODE_INTER_Q || Mode == MODE_INTER4V ||
	  Mode == MODE_UNCODED)
        reconstructH263(h263Data,bx,by,1,0,0);

      /* copy or add block data into P-picture */
      for (comp=0; comp<blk_cnt; comp++) {
        /* inverse DCT */
        if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) {
	  _idct(h263Data->block[comp]);
          _addblock(h263Data,comp,bx,by,0);
        }
        else if ( (pCBP & (1<<(blk_cnt-1-comp))) ) {
          /* No need to to do this for blocks with no coeffs */
	  _idct(h263Data->block[comp]);
          _addblock(h263Data,comp,bx,by,1);
        }
      }


      if (h263Data->pb_frame) {
        /* add block data into B-picture */
        for (comp = 6; comp<blk_cnt+6; comp++) {
          if (!pCOD || h263Data->adv_pred_mode)
            _reconblock_b(h263Data, comp-6,bx,by,Mode,pmvdbx,pmvdby);
          if ( (pCBPB & (1<<(blk_cnt-1-comp%6))) ) {
	    _idct(h263Data->block[comp]);
            _addblock(h263Data,comp,bx,by,1);
          }
        }
      }
      
    } /* end if (MBA > 0) */

    if (!COD) {

      Mode = h263Data->modemap[ypos+1][xpos+1];

      /* decode blocks */
      for (comp=0; comp<blk_cnt; comp++) {

        _clearblock(h263Data->block, comp);
        if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) { /* Intra */
          bp = h263Data->block[comp];
          if(h263Data->syntax_arith_coding) {
            INTRADC_index = _decode_a_symbol(h263Data->bs,cumf_INTRADC);
            bp[0] = intradctab[INTRADC_index];
#ifdef TRACE
            if (trace)
              printf("INTRADC Index: %d INTRADC: %d \n", INTRADC_index,bp[0]);
#endif
          }
          else {
            bp[0] = GetBits(h263Data->bs,8);
#ifdef TRACE
            if (trace) {
              printf("DC[%d]: (",comp);
              printbits((int)bp[0],8,8);
              printf("): %d\n",(int)bp[0]);
            }
#endif
          }

          if (bp[0] == 128)
            if (!h263Data->quiet)
              fprintf(stderr,"Illegal DC-coeff: 1000000\n");
          if (bp[0] == 255)  /* Spec. in H.26P, not in TMN4 */
            bp[0] = 128;
          bp[0] *= 8; /* Iquant */
          if ( (CBP & (1<<(blk_cnt-1-comp))) ) {
            if (!h263Data->syntax_arith_coding)
              _getblock(h263Data,comp,0);
            else 
              _get_sac_block(h263Data,comp,0);
          }
        }
        else { /* Inter */
          if ( (CBP & (1<<(blk_cnt-1-comp))) ) {
            if (!h263Data->syntax_arith_coding)
              _getblock(h263Data,comp,1);
            else
              _get_sac_block(h263Data,comp,1);
          }

        }
        if (h263Data->fault) goto resync;
      }

      /* Decode B blocks */
      if (h263Data->pb_frame) {
        for (comp=6; comp<blk_cnt+6; comp++) {
          _clearblock(h263Data->block, comp);
          if ( (CBPB & (1<<(blk_cnt-1-comp%6))) ) {
            if (!h263Data->syntax_arith_coding)
              _getblock(h263Data,comp,1);
            else
              _get_sac_block(h263Data,comp,1);
          }
          if (h263Data->fault) goto resync;
        }
      }
          
    }

    /* advance to next macroblock */
    MBA++;

    pCBP = CBP; pCBPB = CBPB; pCOD = COD;
    pmvdbx = mvdbx; pmvdby = mvdby;
    fflush(stdout);

    if (MBA >= MBAmax && !last_done) {
      COD = 1;
      xpos = 0;
      ypos++;
      last_done = 1;
      goto reconstruct_mb;
    }

  }
}

/* decode all macroblocks of the current gob */

int H263parseGOB(H263Global *h263Data,
		 int *mbInd, int *mbQuant, MVField *mvField, int indOffset)
{
  const int blk_cnt = 6;
  int comp;
  int MBA, MBAmax;
  int bx, by;

  int COD=0,MCBPC, CBPY, CBP=0, CBPB=0, MODB=0, Mode=0, DQUANT;
  int COD_index, CBPY_index, MODB_index, DQUANT_index, MCBPC_index;
  int INTRADC_index, YCBPB_index, UVCBPB_index, mvdbx_index, mvdby_index;
  int mvx=0, mvy=0, mvy_index, mvx_index, pmv0, pmv1, xpos, ypos, i,k;
  int mvdbx=0, mvdby=0, pmvdbx=0, pmvdby=0, YCBPB, UVCBPB, gobheader_read;
  int startmv,stopmv,xoffset,bsize,last_done=0,pCBP=0,pCBPB=0,pCOD=0;
  int DQ_tab[4] = {-1,-2,1,2};
  short *bp;
  int bsInd=0;
  int mn;
  int prev_temp_ref = h263Data->prev_temp_ref;


  if (mbInd != NULL)
    bsInd = 8 * (int)(h263Data->bs->BufferPtr - h263Data->bs->ByteBuffer) +
    h263Data->bs->ValidBits;
  
  if (!_getheader(h263Data)) {
    return 0;
  }

  h263Data->prev_temp_ref = prev_temp_ref;
  h263Data->trd = h263Data->temp_ref - prev_temp_ref;

  /* number of macroblocks per picture */
  MBAmax = h263Data->mbs_in_gob;

  MBA = 0;        /* macroblock address */

  if (h263Data->gob == 0) {
    h263Data->newgob = 0;
    gobheader_read = 0;
    xpos = -1;
    ypos = 0;
  } else {
    h263Data->newgob = 1;
    gobheader_read = 1;
    xpos = 0;
    ypos = h263Data->gob * h263Data->mbs_in_gob / h263Data->mb_width;
  }

  mn = h263Data->gob * h263Data->mbs_in_gob;

  h263Data->fault = 0;

  for (;;) {

#ifdef TRACE
    if (trace)
      printf("MB %d\n",MBA);
#endif
  resync:

    /* This version of the decoder does not resync on every possible
       error, and it does not do all possible error checks. It is not
       difficult to make it much more error robust, but I do not think
       it is necessary to include this in the freely available
       version. */

    if (h263Data->fault) {
      printf("Warning: A Fault Condition Has Occurred - Resyncing \n");
      h263Data->fault = 0;
    }

    if (MBA >= MBAmax) {    /* Check return conditions */
      h263Data->bs->BufState = GOB_EMPTY;
      return 1;
    }

    if (!gobheader_read) {
      xpos++;
      if (xpos >= h263Data->mb_width) {
	xpos = 0;
	ypos++;
      }
      if (xpos == 0 && ypos > 0)
        h263Data->newgob = 0;
    }
    else 
      gobheader_read = 0;

  read_cod:

    //SV-XXX: Added explicit braces to avoid ambiguous else
    if (mbInd != NULL){
      if (MBA > 0){
	mbInd[mn] = 8 * (int)(h263Data->bs->BufferPtr - h263Data->bs->ByteBuffer) +
	h263Data->bs->ValidBits + indOffset;
      }
      else
	mbInd[mn] = bsInd + indOffset;
    }//SV-XXX
    if (mbQuant != NULL)
      mbQuant[mn] = h263Data->quant;

    if (h263Data->syntax_arith_coding) { 
      if (h263Data->pict_type == PCT_INTER) {
        COD_index = _decode_a_symbol(h263Data->bs,cumf_COD);
        COD = codtab[COD_index];
#ifdef TRACE
        if (trace) {
          printf("Arithmetic Decoding Debug \n");
          printf("COD Index: %d COD: %d \n", COD_index, COD);
        }
#endif
      }
      else
        COD = 0;  /* COD not used in I-pictures, set to zero */
    }
    else {
      if (h263Data->pict_type == PCT_INTER) 
        COD = ShowBits(h263Data->bs,1);
      else
        COD = 0; /* Intra picture -> not skipped */
    }

    if (!COD) {  /* COD == 0 --> not skipped */    

      if (h263Data->syntax_arith_coding)  {
        if (h263Data->pict_type == PCT_INTER) {
          MCBPC_index = _decode_a_symbol(h263Data->bs,cumf_MCBPC);
          MCBPC = mcbpctab[MCBPC_index];
        }
        else {
          MCBPC_index = _decode_a_symbol(h263Data->bs,cumf_MCBPC_intra);
          MCBPC = mcbpc_intratab[MCBPC_index];
        }
#ifdef TRACE
        if (trace) 
          printf("MCBPC Index: %d MCBPC: %d \n",MCBPC_index, MCBPC);
#endif
      }

      else {
        if (h263Data->pict_type == PCT_INTER)
          ConsumeBits(h263Data->bs, 1); /* flush COD bit */
        if (h263Data->pict_type == PCT_INTRA) 
          MCBPC = _getMCBPCintra(h263Data);
        else
          MCBPC = _getMCBPC(h263Data);
      }

      if (h263Data->fault) goto resync;
      
      if (MCBPC == 255) { /* stuffing */
        goto read_cod;   /* read next COD without advancing MB count */
      }

      else {             /* normal MB data */

        Mode = MCBPC & 7;

        /* MODB and CBPB */
        if (h263Data->pb_frame) {
          CBPB = 0;
          if (h263Data->syntax_arith_coding)  {
            MODB_index = _decode_a_symbol(h263Data->bs,cumf_MODB);
            MODB = modb_tab[MODB_index];
          }
          else 
            MODB = _getMODB(h263Data);
#ifdef TRACE
          if (trace)
            printf("MODB: %d\n", MODB);
#endif
          if (MODB == PBMODE_CBPB_MVDB) {
            if (h263Data->syntax_arith_coding)  {
              for(i=0; i<4; i++) {
		YCBPB_index = _decode_a_symbol(h263Data->bs,cumf_YCBPB);
		YCBPB = ycbpb_tab[YCBPB_index];
		CBPB |= (YCBPB << (6-1-i));
              }
 
              for(i=4; i<6; i++) {
		UVCBPB_index = _decode_a_symbol(h263Data->bs,cumf_UVCBPB);
		UVCBPB = uvcbpb_tab[UVCBPB_index];
		CBPB |= (UVCBPB << (6-1-i));
              }
            }
            else
              CBPB = GetBits(h263Data->bs,6);
#ifdef TRACE
            if (trace)
              printf("CBPB = %d\n",CBPB);
#endif
          }
        }

        if (h263Data->syntax_arith_coding) {

          if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) { /* Intra */
            CBPY_index = _decode_a_symbol(h263Data->bs,cumf_CBPY_intra);
            CBPY = cbpy_intratab[CBPY_index];
          }
          else {
            CBPY_index = _decode_a_symbol(h263Data->bs,cumf_CBPY);
            CBPY = cbpytab[CBPY_index];
          }
#ifdef TRACE
          if (trace)
            printf("CBPY Index: %d CBPY %d \n",CBPY_index, CBPY);
#endif
        }
        else 
          CBPY = _getCBPY(h263Data);
 
        /* Decode Mode and CBP */

        if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
          {/* Intra */
            if (!h263Data->syntax_arith_coding)	
              CBPY = CBPY^15;        /* needed in huffman coding only */
          }

        CBP = (CBPY << 2) | (MCBPC >> 4);
      }

      if (Mode == MODE_INTER4V && !h263Data->adv_pred_mode) 
        if (!h263Data->quiet)
          printf("8x8 vectors not allowed in normal prediction mode\n");
          /* Could set fault-flag and resync */


      if (Mode == MODE_INTER_Q || Mode == MODE_INTRA_Q) {
        /* Read DQUANT if necessary */

        if (h263Data->syntax_arith_coding) {
          DQUANT_index = _decode_a_symbol(h263Data->bs,cumf_DQUANT);
          DQUANT = dquanttab[DQUANT_index] - 2; 
          h263Data->quant +=DQUANT;
#ifdef TRACE
          if (trace)
            printf("DQUANT Index: %d DQUANT %d \n",DQUANT_index, DQUANT);
#endif
        }
        else {
          DQUANT = GetBits(h263Data->bs,2);
          h263Data->quant += DQ_tab[DQUANT];
#ifdef TRACE
          if (trace) {
            printf("DQUANT (");
            printbits(DQUANT,2,2);
            printf("): %d = %d\n",DQUANT,DQ_tab[DQUANT]);
          }
#endif
        }

        if (h263Data->quant > 31 || h263Data->quant < 1) {
          if (!h263Data->quiet)
            printf("Quantizer out of range: clipping\n");
          h263Data->quant = MAX(1,MIN(31,h263Data->quant));
          /* could set fault-flag and resync here */
        }
      }

      /* motion vectors */
      if (Mode == MODE_INTER || Mode == MODE_INTER_Q || 
          Mode == MODE_INTER4V || h263Data->pb_frame) {

        if (Mode == MODE_INTER4V) { startmv = 1; stopmv = 4;}
        else { startmv = 0; stopmv = 0;}

        for (k = startmv; k <= stopmv; k++) {
          if (h263Data->syntax_arith_coding) {
            mvx_index = _decode_a_symbol(h263Data->bs,cumf_MVD);
            mvx = mvdtab[mvx_index];
            mvy_index = _decode_a_symbol(h263Data->bs,cumf_MVD);
            mvy = mvdtab[mvy_index];
#ifdef TRACE
            if (trace)
              printf("mvx_index: %d mvy_index: %d \n", mvy_index, mvx_index);
#endif
          }
          else {
            mvx = _getTMNMV(h263Data);
            mvy = _getTMNMV(h263Data);
          }

          pmv0 = _find_pmv(h263Data,xpos,ypos,k,0);
          pmv1 = _find_pmv(h263Data,xpos,ypos,k,1);
          mvx = _motion_decode(h263Data->long_vectors, mvx, pmv0);
          mvy = _motion_decode(h263Data->long_vectors, mvy, pmv1);
#ifdef TRACE
          if (trace) {
            printf("mvx: %d\n", mvx);
            printf("mvy: %d\n", mvy);
          }
#endif
	  /* Check mv's to prevent seg.faults when error rate is high */
          if (!h263Data->mv_outside_frame) {
            bsize = k ? 8 : 16;
            xoffset = k ? (((k-1)&1)<<3) : 0;
            /* checking only integer component */
            if ((xpos<<4) + (mvx/2) + xoffset < 0 ||
		(xpos<<4) + (mvx/2) + xoffset > (h263Data->mb_width<<4) - bsize) {
              if (!h263Data->quiet)
		printf("mvx out of range: searching for sync\n");
              h263Data->fault = 1;
            }
            xoffset = k ? (((k-1)&2)<<2) : 0;
            if ((ypos<<4) + (mvy/2) + xoffset < 0 ||
		(ypos<<4) + (mvy/2) + xoffset > (h263Data->mb_height<<4) - bsize) {
              if (!h263Data->quiet)
		printf("mvy out of range: searching for sync\n");
              h263Data->fault = 1;
            }
          }
          h263Data->MV[0][k][ypos+1][xpos+1] = mvx;
          h263Data->MV[1][k][ypos+1][xpos+1] = mvy;
        }
	    
        /* PB frame delta vectors */

        if (h263Data->pb_frame) {
          if (MODB == PBMODE_MVDB || MODB == PBMODE_CBPB_MVDB) {
            if (h263Data->syntax_arith_coding) {
              mvdbx_index = _decode_a_symbol(h263Data->bs,cumf_MVD);
              mvdbx = mvdtab[mvdbx_index];
              
              mvdby_index = _decode_a_symbol(h263Data->bs,cumf_MVD);
              mvdby = mvdtab[mvdby_index];
            }
            else {
              mvdbx = _getTMNMV(h263Data);
              mvdby = _getTMNMV(h263Data);
            }


            mvdbx = _motion_decode(h263Data->long_vectors, mvdbx, 0);
            mvdby = _motion_decode(h263Data->long_vectors, mvdby, 0);
            /* This will not work if the PB deltas are so large they
               require the second colums of the motion vector VLC
               table to be used.  To fix this it is necessary to
               calculate the MV predictor for the PB delta: TRB*MV/TRD
               here, and use this as the second parameter to
               motion_decode(). The B vector itself will then be
               returned from motion_decode(). This will have to be
               changed to the PB delta again, since it is the PB delta
               which is used later in this program. I don't think PB
               deltas outside the range mentioned above is useful, but
               you never know... */

#ifdef TRACE
            if (trace) {
              printf("MVDB x: %d\n", mvdbx);
              printf("MVDB y: %d\n", mvdby);
            }
#endif
          }
          else {
            mvdbx = 0;
            mvdby = 0;
          }
        }
      }

      if (h263Data->fault) goto resync;

    }
    else { /* COD == 1 --> skipped MB */
      if (MBA >= MBAmax)
        return 1; /* all macroblocks decoded */
      if (!h263Data->syntax_arith_coding)
        if (h263Data->pict_type == PCT_INTER)
          ConsumeBits(h263Data->bs,1);

      Mode = MODE_UNCODED;

      /* Reset CBP */
      CBP = CBPB = 0;

      /* reset motion vectors */
      h263Data->MV[0][0][ypos+1][xpos+1] = 0;
      h263Data->MV[1][0][ypos+1][xpos+1] = 0;
      mvdbx = 0;
      mvdby = 0;
    }

    /* Store Mode*/
    h263Data->modemap[ypos+1][xpos+1] = Mode;

    /* Store mode and motion vectors */
    if (mvField != NULL) {
      mvField->mode[mn] = Mode;
      if (Mode == MODE_INTER || Mode == MODE_INTER_Q) {
	mvField->mx[mn] = mvx;
	mvField->my[mn] = mvy;
      } else {
	mvField->mx[mn] = 0;
	mvField->my[mn] = 0;
      }
    }

    if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) 
      if (!h263Data->pb_frame)
        h263Data->MV[0][0][ypos+1][xpos+1] = h263Data->MV[1][0][ypos+1][xpos+1] = 0;


  reconstruct_mb:

    /* pixel coordinates of top left corner of current macroblock */
    /* one delayed because of OBMC */
    if (xpos > 0) {
      bx = 16*(xpos-1);
      by = 16*ypos;
    }
    else {
      bx = h263Data->coded_picture_width-16;
      by = 16*(ypos-1);
    }

    if (!COD) {

      Mode = h263Data->modemap[ypos+1][xpos+1];

      /* decode blocks */
      for (comp=0; comp<blk_cnt; comp++) {

        if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) { /* Intra */
          bp = h263Data->block[comp];
          if(h263Data->syntax_arith_coding) {
            INTRADC_index = _decode_a_symbol(h263Data->bs,cumf_INTRADC);
            bp[0] = intradctab[INTRADC_index];
#ifdef TRACE
            if (trace)
              printf("INTRADC Index: %d INTRADC: %d \n", INTRADC_index,bp[0]);
#endif
          }
          else {
            bp[0] = GetBits(h263Data->bs,8);
#ifdef TRACE
            if (trace) {
              printf("DC[%d]: (",comp);
              printbits((int)bp[0],8,8);
              printf("): %d\n",(int)bp[0]);
            }
#endif
          }

          if (bp[0] == 128)
            if (!h263Data->quiet)
              fprintf(stderr,"Illegal DC-coeff: 1000000\n");
          if (bp[0] == 255)  /* Spec. in H.26P, not in TMN4 */
            bp[0] = 128;
          bp[0] *= 8; /* Iquant */
          if ( (CBP & (1<<(blk_cnt-1-comp))) ) {
            if (!h263Data->syntax_arith_coding)
              _getblock(h263Data,comp,0);
            else 
              _get_sac_block(h263Data,comp,0);
          }
        }
        else { /* Inter */
          if ( (CBP & (1<<(blk_cnt-1-comp))) ) {
            if (!h263Data->syntax_arith_coding)
              _getblock(h263Data,comp,1);
            else
              _get_sac_block(h263Data,comp,1);
          }

        }
        if (h263Data->fault) goto resync;
      }

      /* Decode B blocks */
      if (h263Data->pb_frame) {
        for (comp=6; comp<blk_cnt+6; comp++) {
          if ( (CBPB & (1<<(blk_cnt-1-comp%6))) ) {
            if (!h263Data->syntax_arith_coding)
              _getblock(h263Data,comp,1);
            else
              _get_sac_block(h263Data,comp,1);
          }
          if (h263Data->fault) goto resync;
        }
      }
          
    }

    /* advance to next macroblock */
    MBA++;
    mn++;

    pCBP = CBP; pCBPB = CBPB; pCOD = COD;
    pmvdbx = mvdbx; pmvdby = mvdby;
    fflush(stdout);

    if (MBA >= MBAmax && !last_done) {
      COD = 1;
      xpos = 0;
      ypos++;
      last_done = 1;
      goto reconstruct_mb;
    }

  }
}

/* Decode an intra block of an intra frame */

void H263getIFrameMB(Bitstream *bs, int bx, int by, int w, int h,
		     unsigned char *y, unsigned char *u, unsigned char *v,
		     int *quant_p, int quiet, int *fault_p)
{
  const int blk_cnt = 6, bquant = 0;
  short block[6][64];
  short *bp;
  int mcbpc, cbp, cbpy, mode;
  int comp;
  int dquant;
  int DQ_tab[4] = {-1,-2,1,2};


  do {
    mcbpc = getMCBPCintra_ext(bs, quiet, fault_p);
  } while (mcbpc == 255);

  mode = mcbpc & 7;

  cbpy = getCBPY_ext(bs, quiet, fault_p);

  /* Decode Mode and CBP */
  if (mode == MODE_INTRA || mode == MODE_INTRA_Q)
    {/* Intra */
      cbpy = cbpy^15;        /* needed in huffman coding only */
    }

  cbp = (cbpy << 2) | (mcbpc >> 4);

  if (mode == MODE_INTRA_Q) {
    /* Read DQUANT if necessary */
    dquant = GetBits(bs,2);
    *quant_p += DQ_tab[dquant];
#ifdef TRACE
    if (trace) {
      printf("DQUANT (");
      printbits(dquant,2,2);
      printf("): %d = %d\n",dquant,DQ_tab[dquant]);
    }
#endif

    if (*quant_p > 31 || *quant_p < 1) {
      if (!quiet)
	printf("Quantizer out of range: clipping\n");
      *quant_p = MAX(1,MIN(31,*quant_p));
      /* could set fault-flag and resync here */
    }
  }

  /*if (*fault_p) goto resync;*/

  /* decode blocks */
  for (comp = 0; comp < blk_cnt; comp++) {

    _clearblock(block, comp);
    bp = block[comp];
    bp[0] = GetBits(bs,8);
#ifdef TRACE
    if (trace) {
      printf("DC[%d]: (",comp);
      printbits((int)bp[0],8,8);
      printf("): %d\n",(int)bp[0]);
    }
#endif

    if (bp[0] == 128)
      if (!quiet)
	fprintf(stderr,"Illegal DC-coeff: 1000000\n");
    if (bp[0] == 255)  /* Spec. in H.26P, not in TMN4 */
      bp[0] = 128;
    bp[0] *= 8; /* Iquant */
    if ( (cbp & (1<<(blk_cnt-1-comp))) )
      getblock_ext(bs, bp, comp, 0, *quant_p, bquant, quiet, fault_p);
        
    /* reconstruct MB */
    /* copy or add block data into picture */
    /* inverse DCT */
    _idct(bp);
    addblock_ext(bp, comp, bx, by, w, h, y, u, v, 0);
  }

}


void H263parseIFrameMB(Bitstream *bs, int w, int h,
		       int *quant_p, int quiet, int *fault_p)
{
  const int blk_cnt = 6, bquant = 0;
  short block[6][64];
  short *bp;
  int mcbpc, cbp, cbpy, mode;
  int comp;
  int dquant;
  int DQ_tab[4] = {-1,-2,1,2};

  //SV-XXX: unused w, h
  UNUSED(w);
  UNUSED(h);

  do {
    mcbpc = getMCBPCintra_ext(bs, quiet, fault_p);
  } while (mcbpc == 255);

  mode = mcbpc & 7;

  cbpy = getCBPY_ext(bs, quiet, fault_p);

  /* Decode Mode and CBP */
  if (mode == MODE_INTRA || mode == MODE_INTRA_Q)
    {/* Intra */
      cbpy = cbpy^15;        /* needed in huffman coding only */
    }

  cbp = (cbpy << 2) | (mcbpc >> 4);

  if (mode == MODE_INTRA_Q) {
    /* Read DQUANT if necessary */
    dquant = GetBits(bs,2);
    *quant_p += DQ_tab[dquant];
#ifdef TRACE
    if (trace) {
      printf("DQUANT (");
      printbits(dquant,2,2);
      printf("): %d = %d\n",dquant,DQ_tab[dquant]);
    }
#endif

    if (*quant_p > 31 || *quant_p < 1) {
      if (!quiet)
	printf("Quantizer out of range: clipping\n");
      *quant_p = MAX(1,MIN(31,*quant_p));
      /* could set fault-flag and resync here */
    }
  }

  /*if (*fault_p) goto resync;*/

  /* decode blocks */
  for (comp = 0; comp < blk_cnt; comp++) {

    bp = block[comp];
    bp[0] = GetBits(bs,8);
#ifdef TRACE
    if (trace) {
      printf("DC[%d]: (",comp);
      printbits((int)bp[0],8,8);
      printf("): %d\n",(int)bp[0]);
    }
#endif

    if (bp[0] == 128)
      if (!quiet)
	fprintf(stderr,"Illegal DC-coeff: 1000000\n");
    if (bp[0] == 255)  /* Spec. in H.26P, not in TMN4 */
      bp[0] = 128;
    bp[0] *= 8; /* Iquant */
    if ( (cbp & (1<<(blk_cnt-1-comp))) )
      getblock_ext(bs, bp, comp, 0, *quant_p, bquant, quiet, fault_p);
        
  }

}


/* set block to zero */

static void _clearblock(short block[12][64], int comp)
{
  int *bp;
  int i;

  bp = (int *)block[comp];

  for (i=0; i<8; i++)
  {
    bp[0] = bp[1] = bp[2] = bp[3] = 0;
    bp += 4;
  }
}


/* move/add 8x8-Block from block[comp] to refframe or bframe */

static void _addblock(H263Global *h263Data, int comp, int bx, int by, int addflag)
{
  int cc,i, iincr, P = 1;
  unsigned char *rfp;
  short *bp;

  bp = h263Data->block[comp];

  if (comp >= 6) {
    /* This is a component for B-frame forward prediction */
    P = 0;
    addflag = 1;
    comp -= 6;
  }

  cc = (comp<4) ? 0 : (comp&1)+1; /* color component index */

  if (cc==0) {
    /* luminance */
    
    /* frame DCT coding */
    if (P)
      rfp = h263Data->newframe[0]
        + h263Data->coded_picture_width*(by+((comp&2)<<2)) + bx + ((comp&1)<<3);
    else
      rfp = h263Data->bframe[0]
        + h263Data->coded_picture_width*(by+((comp&2)<<2)) + bx + ((comp&1)<<3);
    iincr = h263Data->coded_picture_width;
  }
  else {
    /* chrominance */

    /* scale coordinates */
    bx >>= 1;
    by >>= 1;
    /* frame DCT coding */
    if (P)
      rfp = h263Data->newframe[cc] + h263Data->chrom_width*by + bx;
    else
      rfp = h263Data->bframe[cc] + h263Data->chrom_width*by + bx;
    iincr = h263Data->chrom_width;
  }


  if (addflag) {
    for (i=0; i<8; i++) {
      rfp[0] = clp[bp[0]+rfp[0]];
      rfp[1] = clp[bp[1]+rfp[1]];
      rfp[2] = clp[bp[2]+rfp[2]];
      rfp[3] = clp[bp[3]+rfp[3]];
      rfp[4] = clp[bp[4]+rfp[4]];
      rfp[5] = clp[bp[5]+rfp[5]];
      rfp[6] = clp[bp[6]+rfp[6]];
      rfp[7] = clp[bp[7]+rfp[7]];
      bp += 8;
      rfp+= iincr;
    }
  }
  else  {
    for (i=0; i<8; i++) {
      rfp[0] = clp[bp[0]];
      rfp[1] = clp[bp[1]];
      rfp[2] = clp[bp[2]];
      rfp[3] = clp[bp[3]];
      rfp[4] = clp[bp[4]];
      rfp[5] = clp[bp[5]];
      rfp[6] = clp[bp[6]];
      rfp[7] = clp[bp[7]];
      bp += 8;
      rfp += iincr;
    }
  }
}

/* move/add 8x8-Block from block[comp] to refframe or bframe */

static void addblock_ext(short *bp, int comp, int bx, int by,
			 int w, int h,
			 unsigned char *y, unsigned char *u, unsigned char *v,
			 int addflag)
{
  int cc, i, iincr;
  unsigned char *rfp;

  //SV-XXX:
  UNUSED(h);

  if (comp >= 6) {
    /* This is a component for B-frame forward prediction */
    addflag = 1;
    comp -= 6;
  }

  cc = (comp<4) ? 0 : (comp&1)+1; /* color component index */

  if (cc==0) {
    /* luminance */
    
    /* frame DCT coding */
    rfp = y + w * (by+((comp&2)<<2)) + bx + ((comp&1)<<3);

    iincr = w;
  }
  else {
    /* chrominance */

    /* scale coordinates */
    bx >>= 1;
    by >>= 1;
    iincr = w>>1;
    /* frame DCT coding */
    if (cc == 1)
      rfp = u + iincr*by + bx;
    else
      rfp = v + iincr*by + bx;
  }


  if (addflag) {
    for (i=0; i<8; i++) {
      rfp[0] = clp[bp[0]+rfp[0]];
      rfp[1] = clp[bp[1]+rfp[1]];
      rfp[2] = clp[bp[2]+rfp[2]];
      rfp[3] = clp[bp[3]+rfp[3]];
      rfp[4] = clp[bp[4]+rfp[4]];
      rfp[5] = clp[bp[5]+rfp[5]];
      rfp[6] = clp[bp[6]+rfp[6]];
      rfp[7] = clp[bp[7]+rfp[7]];
      bp += 8;
      rfp+= iincr;
    }
  }
  else  {
    for (i=0; i<8; i++) {
      rfp[0] = clp[bp[0]];
      rfp[1] = clp[bp[1]];
      rfp[2] = clp[bp[2]];
      rfp[3] = clp[bp[3]];
      rfp[4] = clp[bp[4]];
      rfp[5] = clp[bp[5]];
      rfp[6] = clp[bp[6]];
      rfp[7] = clp[bp[7]];
      bp += 8;
      rfp += iincr;
    }
  }
}

/* bidirectionally reconstruct 8x8-Block from block[comp] to bframe */

static void _reconblock_b(H263Global *h263Data, int comp, int bx, int by,
			 int mode, int bdx, int bdy)
{
  int cc,i,j,k, ii;
  unsigned char *bfr, *ffr;
  int BMVx, BMVy;
  int xa,xb,ya,yb,x,y,xvec,yvec,mvx,mvy;
  int xint,xhalf,yint,yhalf,pel;
  int trd = h263Data->trd, trb = h263Data->trb;


  x = bx/16+1;y=by/16+1;

  if (mode == MODE_INTER4V) {
    if (comp < 4) {
      /* luma */
      mvx = h263Data->MV[0][comp+1][y][x];
      mvy = h263Data->MV[1][comp+1][y][x];
      BMVx = (bdx == 0 ? (trb-trd)* mvx/trd : trb * mvx/trd + bdx - mvx);
      BMVy = (bdy == 0 ? (trb-trd)* mvy/trd : trb * mvy/trd + bdy - mvy);
    }
    else {
      /* chroma */
      xvec = yvec = 0;
      for (k = 1; k <= 4; k++) {
        mvx = h263Data->MV[0][k][y][x];
        mvy = h263Data->MV[1][k][y][x];
        xvec += (bdx == 0 ? (trb-trd)* mvx/trd : trb * mvx/trd + bdx - mvx);
        yvec += (bdy == 0 ? (trb-trd)* mvy/trd : trb * mvy/trd + bdy - mvy);
      }
      
      /* chroma rounding (table 16/H.263) */
      BMVx = SGN(xvec)*(vic_roundtab[abs(xvec)%16]+(abs(xvec)/16)*2);
      BMVy = SGN(yvec)*(vic_roundtab[abs(yvec)%16]+(abs(yvec)/16)*2);
    }
  }
  else {
    if (comp < 4) {
      /* luma */
      mvx = h263Data->MV[0][0][y][x];
      mvy = h263Data->MV[1][0][y][x];
      BMVx = (bdx == 0 ? (trb-trd)* mvx/trd : trb * mvx/trd + bdx - mvx);
      BMVy = (bdy == 0 ? (trb-trd)* mvy/trd : trb * mvy/trd + bdy - mvy);
    }
    else {
      /* chroma */
      mvx = h263Data->MV[0][0][y][x];
      mvy = h263Data->MV[1][0][y][x];
      xvec = (bdx == 0 ? (trb-trd)* mvx/trd : trb * mvx/trd + bdx - mvx);
      yvec = (bdy == 0 ? (trb-trd)* mvy/trd : trb * mvy/trd + bdy - mvy);
      xvec *= 4;
      yvec *= 4;
      
      /* chroma rounding (table 16/H.263) */
      BMVx = SGN(xvec)*(vic_roundtab[abs(xvec)%16] +(abs(xvec)/16)*2);
      BMVy = SGN(yvec)*(vic_roundtab[abs(yvec)%16] +(abs(yvec)/16)*2);
    }
  }

  cc = (comp<4) ? 0 : (comp&1)+1; /* color component index */

  if (cc==0) {
    /* luminance */
    _find_bidir_limits(BMVx,&xa,&xb,comp&1);
    _find_bidir_limits(BMVy,&ya,&yb,(comp&2)>>1);
    bfr = h263Data->bframe[0] +
      h263Data->coded_picture_width*(by+((comp&2)<<2)) + bx + ((comp&1)<<3);
    ffr = h263Data->newframe[0] +
      h263Data->coded_picture_width*(by+((comp&2)<<2)) + bx + ((comp&1)<<3);
    ii = h263Data->coded_picture_width;
  }
  else {
    /* chrominance */
    /* scale coordinates and vectors*/
    bx >>= 1;
    by >>= 1;

    _find_bidir_chroma_limits(BMVx,&xa,&xb);
    _find_bidir_chroma_limits(BMVy,&ya,&yb);

    bfr = h263Data->bframe[cc]+h263Data->chrom_width*(by+((comp&2)<<2)) +bx+(comp&8);
    ffr = h263Data->newframe[cc]+h263Data->chrom_width*(by+((comp&2)<<2))+bx+(comp&8);
    ii = h263Data->chrom_width;
  }

  xint = BMVx>>1;
  xhalf = BMVx - 2*xint;
  yint = BMVy>>1;
  yhalf = BMVy - 2*yint;

  ffr += xint + (yint+ya)*ii;
  bfr += ya*ii;
  
  if (!xhalf && !yhalf) {
    for (j = ya; j < yb; j++) {
      for (i = xa; i < xb; i++) {
        pel = ffr[i];
        bfr[i] = ((unsigned int)(pel + bfr[i]))>>1;
      }
      bfr += ii;
      ffr += ii;
    }
  }
  else if (xhalf && !yhalf) {
    for (j = ya; j < yb; j++) {
      for (i = xa; i < xb; i++) {
        pel = ((unsigned int)(ffr[i]+ffr[i+1]+1))>>1;
        bfr[i] = ((unsigned int)(pel + bfr[i]))>>1;
      }
      bfr += ii;
      ffr += ii;
    }
  }
  else if (!xhalf && yhalf) {
    for (j = ya; j < yb; j++) {
      for (i = xa; i < xb; i++) {
        pel = ((unsigned int)(ffr[i]+ffr[ii+i]+1))>>1;
        bfr[i] = ((unsigned int)(pel + bfr[i]))>>1;
      }
      bfr += ii;
      ffr += ii;
    }
  }
  else { /* if (xhalf && yhalf) */
    for (j = ya; j < yb; j++) {
      for (i = xa; i < xb; i++) {
        pel = ((unsigned int)(ffr[i]+ffr[i+1]+ffr[ii+i]+ffr[ii+i+1]+2))>>2;
        bfr[i] = ((unsigned int)(pel + bfr[i]))>>1;
      }
      bfr += ii;
      ffr += ii;
    }
  }
  return;
}

static int _motion_decode(int long_vectors, int vec, int pmv)
{
  if (vec > 31) vec -= 64;
  vec += pmv;
  if (!long_vectors) {
    if (vec > 31)
      vec -= 64;
    if (vec < -32)
      vec += 64;
  }
  else {
    if (pmv < -31 && vec < -63)
      vec += 64;
    if (pmv > 32 && vec > 63)
      vec -= 64;
  }
  return vec;
}


static int _find_pmv(H263Global *h263Data, int x, int y, int block, int comp)
{
  int p1,p2,p3;
  int xin1,xin2,xin3;
  int yin1,yin2,yin3;
  int vec1,vec2,vec3;
  int l8,o8,or8;

  x++;y++;

  l8 = (h263Data->modemap[y][x-1] == MODE_INTER4V ? 1 : 0);
  o8 =  (h263Data->modemap[y-1][x] == MODE_INTER4V ? 1 : 0);
  or8 = (h263Data->modemap[y-1][x+1] == MODE_INTER4V ? 1 : 0);

  switch (block) {
  case 0: 
    vec1 = (l8 ? 2 : 0) ; yin1 = y  ; xin1 = x-1;
    vec2 = (o8 ? 3 : 0) ; yin2 = y-1; xin2 = x;
    vec3 = (or8? 3 : 0) ; yin3 = y-1; xin3 = x+1;
    break;
  case 1:
    vec1 = (l8 ? 2 : 0) ; yin1 = y  ; xin1 = x-1;
    vec2 = (o8 ? 3 : 0) ; yin2 = y-1; xin2 = x;
    vec3 = (or8? 3 : 0) ; yin3 = y-1; xin3 = x+1;
    break;
  case 2:
    vec1 = 1            ; yin1 = y  ; xin1 = x;
    vec2 = (o8 ? 4 : 0) ; yin2 = y-1; xin2 = x;
    vec3 = (or8? 3 : 0) ; yin3 = y-1; xin3 = x+1;
    break;
  case 3:
    vec1 = (l8 ? 4 : 0) ; yin1 = y  ; xin1 = x-1;
    vec2 = 1            ; yin2 = y  ; xin2 = x;
    vec3 = 2            ; yin3 = y  ; xin3 = x;
    break;
  case 4:
    vec1 = 3            ; yin1 = y  ; xin1 = x;
    vec2 = 1            ; yin2 = y  ; xin2 = x;
    vec3 = 2            ; yin3 = y  ; xin3 = x;
    break;
  default:
    fprintf(stderr,"Illegal block number in find_pmv (getpic.c)\n");
    exit(1);
    break;
  }
  p1 = h263Data->MV[comp][vec1][yin1][xin1];
  p2 = h263Data->MV[comp][vec2][yin2][xin2];
  p3 = h263Data->MV[comp][vec3][yin3][xin3];

  if (h263Data->newgob && (block == 0 || block == 1 || block == 2))
    p2 = NO_VEC;

  if (p2 == NO_VEC) { p2 = p3 = p1; }

  return p1+p2+p3 - MAX(p1,MAX(p2,p3)) - MIN(p1,MIN(p2,p3));
}



static void _find_bidir_limits(vec, start, stop, nhv)
int vec;
int *start, *stop, nhv;
{
  /* limits taken from C loop in section G5 in H.263 */
  *start = MAX(0,(-vec+1)/2 - nhv*8);
  *stop = MIN(7,15-(vec+1)/2 - nhv*8);

  (*stop)++; /* I use < and not <= in the loop */
}

static void _find_bidir_chroma_limits(vec, start, stop)
int vec;
int *start, *stop;
{

  /* limits taken from C loop in section G5 in H.263 */
  *start = MAX(0,(-vec+1)/2);
  *stop = MIN(7,7-(vec+1)/2);

  (*stop)++; /* I use < and not <= in the loop */
  return;
}

void make_edge_image(src,dst,width,height,edge)
unsigned char *src, *dst;
int width,height,edge;
{
  int i,j;
  unsigned char *p1,*p2,*p3,*p4;
  unsigned char *o1,*o2,*o3,*o4;

  /* center image */
  p1 = dst;
  o1 = src;
  for (j = 0; j < height;j++) {
    for (i = 0; i < width; i++) {
      *(p1 + i) = *(o1 + i);
    }
    p1 += width + (edge<<1);
    o1 += width;
  }

  /* left and right edges */
  p1 = dst-1;
  o1 = src;
  for (j = 0; j < height;j++) {
    for (i = 0; i < edge; i++) {
      *(p1 - i) = *o1;
      *(p1 + width + i + 1) = *(o1 + width - 1);
    }
    p1 += width + (edge<<1);
    o1 += width;
  }    
    
  /* top and bottom edges */
  p1 = dst;
  p2 = dst + (width + (edge<<1))*(height-1);
  o1 = src;
  o2 = src + width*(height-1);
  for (j = 0; j < edge;j++) {
    p1 = p1 - (width + (edge<<1));
    p2 = p2 + (width + (edge<<1));
    for (i = 0; i < width; i++) {
      *(p1 + i) = *(o1 + i);
      *(p2 + i) = *(o2 + i);
    }
  }    

  /* corners */
  p1 = dst - (width+(edge<<1)) - 1;
  p2 = p1 + width + 1;
  p3 = dst + (width+(edge<<1))*(height)-1;
  p4 = p3 + width + 1;

  o1 = src;
  o2 = o1 + width - 1;
  o3 = src + width*(height-1);
  o4 = o3 + width - 1;
  for (j = 0; j < edge; j++) {
    for (i = 0; i < edge; i++) {
      *(p1 - i) = *o1;
      *(p2 + i) = *o2;
      *(p3 - i) = *o3;
      *(p4 + i) = *o4; 
    }
    p1 = p1 - (width + (edge<<1));
    p2 = p2 - (width + (edge<<1));
    p3 = p3 + width + (edge<<1);
    p4 = p4 + width + (edge<<1);
  }

}    

void H263ConcealGOB(H263Global *h263Data)
{
  int gobsize = h263Data->mbs_in_gob * 16 * 16 * sizeof(unsigned char);


  memcpy(h263Data->newframe[0] + h263Data->gob * gobsize,
	 h263Data->oldrefframe[0] + h263Data->gob * gobsize, gobsize);
  gobsize /= 4;
  memcpy(h263Data->newframe[1] + h263Data->gob * gobsize,
	 h263Data->oldrefframe[1] + h263Data->gob * gobsize, gobsize);
  memcpy(h263Data->newframe[2] + h263Data->gob * gobsize,
	 h263Data->oldrefframe[2] + h263Data->gob * gobsize, gobsize);
}
