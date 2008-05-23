/************************************************************************
 *
 *  getpic.c, picture decoding for tmndecode (H.263 decoder)
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
 *  Modified by: Mike Gallant <mikeg@ee.ubc.ca>
 *               Guy Cote <guyc@ee.ubc.ca>
 *               Berna Erol <bernae@ee.ubc.ca>
 *
 *  Contacts:
 *  Micheal Gallant                   <mikeg@ee.ubc.ca>
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


/* modified to support annex O true B frames, mikeg@ee.ubc.ca
 * 
 * modified by Wayne Ellis BT Labs to run Annex E Arithmetic Decoding
 * <ellis_w_wayne@bt-web.bt.co.uk>
 * 
 * based on mpeg2decode, (C) 1994, MPEG Software Simulation Group and
 * mpeg2play, (C) 1994 Stefan Eckart <stefan@lis.e-technik.tu-muenchen.de> */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "tmndec.h"
#include "global.h"

#include "indices.h"
#include "sactbls.h"
#ifdef WIN32
#include "win.h" //SV-XXX: was warning
#endif

static int coded_map[MBR + 1][MBC + 1];
static int quant_map[MBR + 1][MBC + 1];
static int STRENGTH[] = {1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12};
static int STRENGTH1[] = {1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4};
static int STRENGTH2[] = {1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};


/* private prototypes */
static void get_I_P_MBs _ANSI_ARGS_ ((int framenum));
static void get_B_MBs _ANSI_ARGS_ ((int framenum));
static void get_EI_EP_MBs _ANSI_ARGS_ ((int framenum));
static void clearblock _ANSI_ARGS_ ((int comp));
static int motion_decode _ANSI_ARGS_ ((int vec, int pmv));
static int find_pmv _ANSI_ARGS_ ((int x, int y, int block, int comp));
static void addblock _ANSI_ARGS_ ((int comp, int bx, int by, int addflag));
static void reconblock_b _ANSI_ARGS_ ((int comp, int bx, int by, int mode, int bdx, int bdy));
static void find_bidir_limits _ANSI_ARGS_ ((int vec, int *start, int *stop, int nhv));
static void find_bidir_chroma_limits _ANSI_ARGS_ ((int vec, int *start, int *stop));
static void make_edge_image _ANSI_ARGS_ ((unsigned char *src, unsigned char *dst, int width, int height, int edge));
static void init_enhancement_layer _ANSI_ARGS_((int layer));
void edge_filter _ANSI_ARGS_ ((unsigned char *lum, unsigned char *Cb, unsigned char *Cr, int width, int height));
void horiz_edge_filter _ANSI_ARGS_ ((unsigned char *rec, int width, int height, int chr));
void vert_edge_filter _ANSI_ARGS_ ((unsigned char *rec, int width, int height, int chr));
void vert_post_filter (unsigned char *rec, int width, int height, int chr);
void horiz_post_filter (unsigned char *rec, int width, int height, int chr);
void PostFilter (unsigned char *lum, unsigned char *Cb, unsigned char *Cr, int width, int height);

/* reference picture selection */
int  get_reference_picture(void);
void store_picture(int);

/**********************************************************************
 *
 *	Name:	      getpicture
 *	Description:  decode and display one picture 
 *	Input:	      frame number
 *	Returns:      
 *	Side effects: 
 *
 *  Date: 971102  Author: mikeg@ee.ubc.ca
 *
 *
 ***********************************************************************/
void getpicture (int *framenum)
{
  unsigned char *tmp;
  int            i, store_pb, quality=0;

  switch (pict_type)
  {

    case PCT_INTRA:
    case PCT_INTER:
    case PCT_PB:
    case PCT_IPB:

      enhance_pict = NO;

      if (reference_picture_selection_mode)
	  if (-1 == (quality = get_reference_picture()))
	      break;
      if (quality > 256) {
	  fprintf(stderr,"completely out of sync -- waiting for I-frame\n");
	  stop_decoder = 1;
	  break;
      }
      
      for (i = 0; i < 3; i++)
      {
        tmp = prev_frame[i];
        prev_I_P_frame[i] = prev_frame[i] = next_I_P_frame[i];
        next_I_P_frame[i] = current_frame[i] = tmp;
      }
      
      if ((mv_outside_frame) && (*framenum > 0))
      {
        make_edge_image (prev_I_P_frame[0], edgeframe[0], coded_picture_width,
                         coded_picture_height, 32);
        make_edge_image (prev_I_P_frame[1], edgeframe[1], chrom_width, chrom_height, 16);
        make_edge_image (prev_I_P_frame[2], edgeframe[2], chrom_width, chrom_height, 16);
      }

      get_I_P_MBs (*framenum);

      store_pb = pb_frame;
      pb_frame = 0;

      if (deblocking_filter_mode)
        edge_filter (current_frame[0], current_frame[1], current_frame[2],
                     coded_picture_width, coded_picture_height);
     
      pb_frame = store_pb;
 
      if (pb_frame) 
      { 
        if (deblocking_filter_mode)
          edge_filter(bframe[0],bframe[1],bframe[2],
                      coded_picture_width,coded_picture_height); 
      }

      PictureDisplay(framenum);      

      if (reference_picture_selection_mode)
	  store_picture((pict_type == PCT_INTRA) ? 0 : quality);
      
      break;

    case PCT_B:

      if (enhancement_layer_num > 1)
        enhance_pict = YES;
      else
        enhance_pict = NO;

      for (i = 0; i < 3; i++)
      {
        current_frame[i] = bframe[i];
      }

      /* Forward prediction */
      make_edge_image (prev_I_P_frame[0], edgeframe[0], coded_picture_width,
                       coded_picture_height, 32);
      make_edge_image (prev_I_P_frame[1], edgeframe[1], chrom_width, chrom_height, 16);
      make_edge_image (prev_I_P_frame[2], edgeframe[2], chrom_width, chrom_height, 16);
      
      /* Backward prediction */
      make_edge_image (next_I_P_frame[0], nextedgeframe[0], coded_picture_width,
                       coded_picture_height, 32);
      make_edge_image (next_I_P_frame[1], nextedgeframe[1], chrom_width, chrom_height, 16);
      make_edge_image (next_I_P_frame[2], nextedgeframe[2], chrom_width, chrom_height, 16);
  
      get_B_MBs (*framenum);

      PictureDisplay(framenum);

      break;

    case PCT_EI:
    case PCT_EP:

      enhance_pict = YES;

      if (!enhancement_layer_init[enhancement_layer_num-2])
      {
        init_enhancement_layer(enhancement_layer_num-2);
        enhancement_layer_init[enhancement_layer_num-2] = ON;
      }

      for (i = 0; i < 3; i++)
      {
        tmp = prev_enhancement_frame[enhancement_layer_num-2][i];
        prev_enhancement_frame[enhancement_layer_num-2][i] = 
          current_enhancement_frame[enhancement_layer_num-2][i];
        current_enhancement_frame[enhancement_layer_num-2][i] = tmp;
      }

      if (scalability_mode >= 3)
      {
        UpsampleReferenceLayerPicture();
        curr_reference_frame[0] = upsampled_reference_frame[0];
        curr_reference_frame[1] = upsampled_reference_frame[1];
        curr_reference_frame[2] = upsampled_reference_frame[2];
      }
      else
      {
        curr_reference_frame[0] = current_frame[0];
        curr_reference_frame[1] = current_frame[1];
        curr_reference_frame[2] = current_frame[2];
      }

      make_edge_image (prev_enhancement_frame[enhancement_layer_num-2][0], 
                       enhance_edgeframe[enhancement_layer_num-2][0], 
                       coded_picture_width, coded_picture_height, 32);
      make_edge_image (prev_enhancement_frame[enhancement_layer_num-2][1], 
                       enhance_edgeframe[enhancement_layer_num-2][1], 
                       chrom_width, chrom_height, 16);
      make_edge_image (prev_enhancement_frame[enhancement_layer_num-2][2], 
                       enhance_edgeframe[enhancement_layer_num-2][2], 
                       chrom_width, chrom_height, 16);
  
      get_EI_EP_MBs (*framenum);

      if (deblocking_filter_mode)
        edge_filter (current_enhancement_frame[enhancement_layer_num-2][0], 
                     current_enhancement_frame[enhancement_layer_num-2][1], 
                     current_enhancement_frame[enhancement_layer_num-2][2], 
                     coded_picture_width, coded_picture_height);

      store_one (enhance_recon_file_name[enhancement_layer_num-2], 
                 current_enhancement_frame[enhancement_layer_num-2], 
                 0, coded_picture_width, vertical_size);
  
      break;

    default:

      break;

  }
}

/* decode all macroblocks of the current picture */
static int change_of_quant_tab_10[32] = {0, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3};
static int change_of_quant_tab_11[32] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, -5};

static void get_I_P_MBs (int framenum)
{
  int comp;
  int MBA, MBAmax;
  int bx, by;

  int COD = 0, MCBPC, CBPY, CBP = 0, CBPB = 0, MODB = 0, Mode = 0, DQUANT;
  int COD_index, CBPY_index, MODB_index, DQUANT_index, MCBPC_index;
  int INTRADC_index, YCBPB_index, UVCBPB_index, mvdbx_index, mvdby_index;
  int mvx = 0, mvy = 0, mvy_index, mvx_index, pmv0, pmv1, xpos, ypos, gob, i, k;
  int mvdbx = 0, mvdby = 0, pmvdbx, pmvdby, gfid, YCBPB, UVCBPB, gobheader_read;
  int startmv, stopmv, offset, bsize, last_done = 0, pCBP = 0, pCBPB = 0, pCOD = 0, pMODB = 0;
  int x_avg, y_avg;
  int DQ_tab[4] = {-1, -2, 1, 2};
  short *bp;
  int long_vectors_bak;
  int tmp = 0;

  /* variables used in advanced intra coding mode */
  int INTRA_AC_DC = 0, INTRA_AC_DC_index;
  short *store_qcoeff;

  /* number of macroblocks per picture */
  MBAmax = mb_width * mb_height;

  MBA = 0;                      /* macroblock address */
  newgob = 0;

  /* mark MV's above the picture */
  for (i = 1; i < mb_width + 1; i++)
  {
    for (k = 0; k < 5; k++)
    {
      MV[0][k][0][i] = NO_VEC;
      MV[1][k][0][i] = NO_VEC;
    }
    modemap[0][i] = MODE_INTRA;
  }
  /* zero MV's on the sides of the picture */
  for (i = 0; i < mb_height + 1; i++)
  {
    for (k = 0; k < 5; k++)
    {
      MV[0][k][i][0] = 0;
      MV[1][k][i][0] = 0;
      MV[0][k][i][mb_width + 1] = 0;
      MV[1][k][i][mb_width + 1] = 0;
    }
    modemap[i][0] = MODE_INTRA;
    modemap[i][mb_width + 1] = MODE_INTRA;
  }
  /* initialize the qcoeff used in advanced intra coding */
  if (advanced_intra_coding)
  {
    /* store the qcoeff for the frame */
    if ((store_qcoeff = (short *) calloc (64 * MBAmax * blk_cnt, sizeof (short))) == 0)
    {
      fprintf (stderr, "getMB(): Couldn't allocate store_qcoeff.\n");
      exit (-1);
    }
  }
  fault = 0;
  gobheader_read = 0;

  for (;;)
  {
    if (trace)
      fprintf (trace_file, "frame %d, MB %d\n", framenum, MBA);

resync:
    /* This version of the decoder does not resync on every possible
     * error, and it does not do all possible error checks. It is not
     * difficult to make it much more error robust, but I do not think it
     * is necessary to include this in the freely available version. */
   
    if (fault)
    {
      printf ("Warning: A Fault Condition Has Occurred - Resyncing \n");
      startcode ();             /* sync on new startcode */
      fault = 0;
    }
    if (!(showbits (22) >> 6))
    {
      /* startcode */
      startcode ();

      /* in case of byte aligned start code, ie. PSTUF, GSTUF or ESTUF is
       * used */
      if (showbits (22) == (32 | SE_CODE))
      {
        /* end of sequence */
        if (!(syntax_arith_coding && MBA < MBAmax))
        {
          return;
        }
      } 
      else if ((showbits (22) == PSC << 5))
      {
        /* new picture */
        if (!(syntax_arith_coding && MBA < MBAmax))
        {
          return;
        }
      } 
      else
      {
        if (!(syntax_arith_coding && MBA % mb_width))
        {
          if (syntax_arith_coding)
          {
            /* SAC hack to finish GOBs which  end with MBs coded with no
             * bits. */
            gob = (showbits (22) & 31);
            if (gob * mb_width != MBA)
              goto finish_gob;
          }
          gob = getheader () - 1;
          if (gob > mb_height)
          {
            if (!quiet)
              printf ("GN out of range\n");
            return;
          }
          /* GFID is not allowed to change unless PTYPE in picture header
           * changes */
          gfid = vic_getbits (2);
          if (trace)
          {
            fprintf (trace_file, "gfid: ");
            printbits (gfid, 2, 2);
          }
          /* NB: in error-prone environments the decoder can use this
           * value to determine whether a picture header where the PTYPE
           * has changed, has been lost */

          quant = vic_getbits (5);
          if (trace)
          {
            fprintf (trace_file, "quant: ");
            printbits (quant, 5, 5);
          }
          xpos = 0;
          ypos = gob;
          MBA = ypos * mb_width;

          newgob = 1;
          gobheader_read = 1;
          if (syntax_arith_coding)
            decoder_reset ();   /* init. arithmetic decoder buffer after
                                 * gob */
        }
      }
    }
finish_gob:

    /* SAC specific label */
    if (!gobheader_read)
    {
      xpos = MBA % mb_width;
      ypos = MBA / mb_width;
      if (xpos == 0 && ypos > 0)
        newgob = 0;
    } else
      gobheader_read = 0;

    if (MBA >= MBAmax)
    {
      /* all macroblocks decoded */
      return;
    }
read_cod:

    if (syntax_arith_coding)
    {
      if (pict_type == PCT_INTER || pict_type == PCT_IPB)
      {
        COD_index = decode_a_symbol (cumf_COD);
        COD = codtab[COD_index];
        if (trace)
        {
          fprintf (trace_file, "Arithmetic Decoding Debug \n");
          fprintf (trace_file, "COD Index: %d COD: %d \n", COD_index, COD);
        }
      } 
      else
      {
        COD = 0;                /* COD not used in I-pictures, set to zero */
        coded_map[ypos + 1][xpos + 1] = 1;
      }
    } 
    else
    {
      if (PCT_INTER == pict_type || PCT_IPB == pict_type)
      {
        COD = showbits (1);
        if (trace)
        {
          fprintf (trace_file, "COD : %d \n", COD);
        }
      } 
      else
      {
        COD = 0;                /* Intra picture -> not skipped */
        coded_map[ypos + 1][xpos + 1] = 1;
      }
    }

    if (!COD)
    {
      /* COD == 0 --> not skipped */
      if (syntax_arith_coding)
      {
        if (pict_type == PCT_INTER || pict_type == PCT_IPB)
        {
          if (plus_type) 
          {
            MCBPC_index = decode_a_symbol (cumf_MCBPC_4MVQ);
            MCBPC = mcbpctab_4mvq[MCBPC_index];
          } 
          else 
          {
            MCBPC_index = decode_a_symbol (cumf_MCBPC_no4MVQ);
            MCBPC = mcbpctab[MCBPC_index]; 
          }
        } 
        else
        {
          MCBPC_index = decode_a_symbol (cumf_MCBPC_intra);
          MCBPC = mcbpc_intratab[MCBPC_index];
        }
        if (trace)
          fprintf (trace_file, "MCBPC Index: %d MCBPC: %d \n", MCBPC_index, MCBPC);
      } 
      else
      {
        if (PCT_INTER == pict_type || PCT_IPB == pict_type)
        {
          /* flush COD bit */
          flushbits (1);
        }
        if (PCT_INTRA == pict_type)
        {
          MCBPC = getMCBPCintra ();
        } 
        else
        {
          MCBPC = getMCBPC ();
        }
      }

      if (fault)
        goto resync;

      if (MCBPC == 255)
      {      
        /* stuffing - read next COD without advancing MB count. */        
        goto read_cod;        
      } 
      else      
      {      
        /* normal MB data */        
        Mode = MCBPC & 7;        
        if (advanced_intra_coding && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))        
        {        
          /* get INTRA_AC_DC mode for annex I */          
          if (syntax_arith_coding)          
          {          
            INTRA_AC_DC_index = decode_a_symbol (cumf_INTRA_AC_DC);            
            INTRA_AC_DC = intra_ac_dctab[INTRA_AC_DC_index];            
          } 
          else          
          {          
            /* using VLC */            
            if (!showbits (1))            
              INTRA_AC_DC = vic_getbits (1);              
            else            
              INTRA_AC_DC = vic_getbits (2);            
          }          
          if (trace)          
            fprintf (trace_file, "INTRA_AC_DC: %d\n", INTRA_AC_DC);          
        }
        
        /* MODB and CBPB */        
        if (pb_frame)        
        {        
          CBPB = 0;          
          if (syntax_arith_coding)          
          {          
            if (pb_frame== PB_FRAMES)            
            {            
              MODB_index = decode_a_symbol (cumf_MODB_G);              
              MODB = modb_tab_G[MODB_index];              
            }            
            else            
            {            
              MODB_index = decode_a_symbol (cumf_MODB_M);              
              MODB = modb_tab_M[MODB_index];              
            }            
          } 
          else          
            MODB = getMODB ();
          
          if ( (MODB == PBMODE_CBPB_MVDB && pb_frame == PB_FRAMES) ||          
               (pb_frame == IM_PB_FRAMES && 
               (MODB == PBMODE_CBPB_FRW_PRED ||            
                MODB == PBMODE_CBPB_BIDIR_PRED || MODB == PBMODE_CBPB_BCKW_PRED) ) )            
          {          
            if (syntax_arith_coding)            
            {            
              for (i = 0; i < 4; i++)              
              {              
                YCBPB_index = decode_a_symbol (cumf_YCBPB);                
                YCBPB = ycbpb_tab[YCBPB_index];                
                CBPB |= (YCBPB << (6 - 1 - i));                
              }              
              for (i = 4; i < 6; i++)              
              {              
                UVCBPB_index = decode_a_symbol (cumf_UVCBPB);                
                UVCBPB = uvcbpb_tab[UVCBPB_index];                
                CBPB |= (UVCBPB << (6 - 1 - i));                
              }              
            } 
            else            
              CBPB = vic_getbits (6); 
            
            if (trace)              
              fprintf (trace_file, "CBPB = %d\n", CBPB);            
          }          
        }
        
        if (syntax_arith_coding)        
        {          
          if ((Mode == MODE_INTRA || Mode == MODE_INTRA_Q))          
          {          
            /* Intra */            
            CBPY_index = decode_a_symbol (cumf_CBPY_intra);            
            CBPY = cbpy_intratab[CBPY_index];            
          } 
          else          
          {          
            CBPY_index = decode_a_symbol (cumf_CBPY);            
            CBPY = cbpytab[CBPY_index];            
          }
          
          if (trace)          
            fprintf (trace_file, "CBPY Index: %d CBPY %d \n", CBPY_index, CBPY);          
        } 
        else        
        {        
          CBPY = getCBPY ();          
        }        
      }

      /* Decode Mode and CBP */      
      if ((Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
      {
        /* Intra */
        coded_map[ypos + 1][xpos + 1] = 1;

        if (!syntax_arith_coding)
          CBPY = CBPY ^ 15;   /* needed in huffman coding only */
      } 
      else if (!syntax_arith_coding && alternative_inter_VLC_mode && ((MCBPC >> 4) == 3))
        CBPY = CBPY ^ 15;     /* Annex S.3 change */

      CBP = (CBPY << 2) | (MCBPC >> 4);
        
      if ((Mode == MODE_INTER4V || Mode == MODE_INTER4V_Q) && !use_4mv)
      {
        if (!quiet)
        {
          /* Could set fault-flag and resync */
          printf ("8x8 vectors not allowed in normal prediction mode\n");
        }
        if (trace)
          fprintf (trace_file, "8x8 vectors not allowed in normal prediction mode\n");
      }

      if (Mode == MODE_INTER_Q || Mode == MODE_INTRA_Q || Mode == MODE_INTER4V_Q)
      {
        /* Read DQUANT if necessary */
        if (syntax_arith_coding)
        {
          DQUANT_index = decode_a_symbol (cumf_DQUANT);
          DQUANT = dquanttab[DQUANT_index] - 2;
          quant += DQUANT;
          if (trace)
            fprintf (trace_file, "DQUANT Index: %d DQUANT %d \n", DQUANT_index, DQUANT);
        } 
        else 
        {
          if (!modified_quantization_mode)
          {
            DQUANT = vic_getbits (2);
            quant += DQ_tab[DQUANT];
            if (trace)
            {
              fprintf (trace_file, "DQUANT (");
              printbits (DQUANT, 2, 2);
              fprintf (trace_file, "): %d = %d\n", DQUANT, DQ_tab[DQUANT]);
            }
          } 
          else
          {
            tmp = vic_getbits (1);
            if (tmp)
            {
              /* only one more additional bit was sent */
              tmp = vic_getbits (1);
              if (tmp)
              {
                /* second bit of quant is 1 */
                DQUANT = change_of_quant_tab_11[quant];
              } 
              else
              {
                /* second bit of quant is 0 */
                DQUANT = change_of_quant_tab_10[quant];
              }
              quant += DQUANT;
              if (trace)
              {
                fprintf (trace_file, "DQUANT (1");
                printbits (tmp, 1, 1);
                fprintf (trace_file, "): %d \n", DQUANT);
              }
            } 
            else
            {
              /* five additional bits were sent as            
               * DQUANT */
              DQUANT = vic_getbits (5);
              quant = DQUANT;
              if (trace)
              {
                fprintf (trace_file, "DQUANT (");
                printbits (DQUANT, 5, 5);
                fprintf (trace_file, "): %d \n", DQUANT);
              }
            }
          }
        }
      
        if (quant > 31 || quant < 1)
        {
          if (!quiet)
            printf ("Quantizer out of range: clipping\n");
          quant = mmax (1, mmin (31, quant));
          /* could set fault-flag and resync here */
        }
      }
    
      /* motion vectors */    
      if (Mode == MODE_INTER || Mode == MODE_INTER_Q ||
          Mode == MODE_INTER4V || Mode == MODE_INTER4V_Q || pb_frame)
      {     
        if (Mode == MODE_INTER4V || Mode == MODE_INTER4V_Q)
        {
          startmv = 1;
          stopmv = 4;
        } 
        else
        { 
          startmv = 0;
          stopmv = 0;
        }

        for (k = startmv; k <= stopmv; k++)
        {
          if (syntax_arith_coding)
          {
            mvx_index = decode_a_symbol (cumf_MVD);
            mvx = mvdtab[mvx_index];
            mvy_index = decode_a_symbol (cumf_MVD);
            mvy = mvdtab[mvy_index];
            if (trace)
              fprintf (trace_file, "mvx_index: %d mvy_index: %d \n", mvy_index, mvx_index);
          } 
          else
          {
            if (plus_type && long_vectors)
            {
              mvx = getRVLC ();
              mvy = getRVLC ();
 
              /* flush start code emulation bit */
              if (mvx == 1 && mvy == 1)
                flushbits(1);
            }
            else
            {
              mvx = getTMNMV ();
              mvy = getTMNMV ();
            }
          }

          pmv0 = find_pmv (xpos, ypos, k, 0);
          pmv1 = find_pmv (xpos, ypos, k, 1);

          if (plus_type && long_vectors && !syntax_arith_coding)
          {
            mvx += pmv0;
            mvy += pmv1;
          }
          else
          {
            mvx = motion_decode (mvx, pmv0);
            mvy = motion_decode (mvy, pmv1);
          }

          if (trace)
          {
            fprintf (trace_file, "mvx: %d\n", mvx);
            fprintf (trace_file, "mvy: %d\n", mvy);
          }
          /* store coded or not-coded */
          coded_map[ypos + 1][xpos + 1] = 1;

          /* Check mv's to prevent seg.faults when error rate is high */
          if (!mv_outside_frame)
          {
            bsize = k ? 8 : 16;
            offset = k ? (((k - 1) & 1) << 3) : 0;
            /* checking only integer component */
            if ((xpos << 4) + (mvx / 2) + offset < 0 ||
                (xpos << 4) + (mvx / 2) + offset > (mb_width << 4) - bsize)
            {
              if (!quiet)
                printf ("mvx out of range: searching for sync\n");
              fault = 1;
            }
            offset = k ? (((k - 1) & 2) << 2) : 0;
            if ((ypos << 4) + (mvy / 2) + offset < 0 ||
                (ypos << 4) + (mvy / 2) + offset > (mb_height << 4) - bsize)
            {
              if (!quiet)
                printf ("mvy out of range: searching for sync\n");
              fault = 1;
            }
          }
          MV[0][k][ypos + 1][xpos + 1] = mvx;
          MV[1][k][ypos + 1][xpos + 1] = mvy;
        }

        /* Store P MVs for use with true B direct mode. */
        if (PCT_INTER == pict_type)
        {
          if (Mode == MODE_INTER4V || Mode == MODE_INTER4V_Q)
          {
            x_avg = (MV[0][1][ypos + 1][xpos + 1] + MV[0][2][ypos + 1][xpos + 1] +
                     MV[0][3][ypos + 1][xpos + 1] + MV[0][4][ypos + 1][xpos + 1] + 2) >> 2;
            true_B_direct_mode_MV[0][ypos + 1][xpos + 1] = x_avg;
            y_avg = (MV[1][1][ypos + 1][xpos + 1] + MV[1][2][ypos + 1][xpos + 1] +
                     MV[1][3][ypos + 1][xpos + 1] + MV[1][4][ypos + 1][xpos + 1] + 2) >> 2;
            true_B_direct_mode_MV[1][ypos + 1][xpos + 1] = y_avg;
          } 
          else
          {
            true_B_direct_mode_MV[0][ypos + 1][xpos + 1] = MV[0][0][ypos + 1][xpos + 1];
            true_B_direct_mode_MV[1][ypos + 1][xpos + 1] = MV[1][0][ypos + 1][xpos + 1];
          }
        }

        /* PB frame delta vectors */
        if (pb_frame)
        {
          if (((MODB == PBMODE_MVDB || MODB == PBMODE_CBPB_MVDB) && pb_frame == PB_FRAMES) ||
              (pb_frame == IM_PB_FRAMES && (MODB == PBMODE_CBPB_FRW_PRED || MODB == PBMODE_FRW_PRED)))
          {
            if (syntax_arith_coding)
            {
              mvdbx_index = decode_a_symbol (cumf_MVD);
              mvdbx = mvdtab[mvdbx_index];
              mvdby_index = decode_a_symbol (cumf_MVD);
              mvdby = mvdtab[mvdby_index];
            } 
            else
            {
              if (plus_type && long_vectors)
              {
                mvdbx = getRVLC ();
                mvdby = getRVLC ();

                /* flush start code emulation bit */
                if (mvdbx == 1 && mvdby == 1)
                  flushbits(1); 
              }
              else
              {
                mvdbx = getTMNMV ();
                mvdby = getTMNMV ();
              }
            }

            long_vectors_bak = long_vectors;

            /* turn off long vectors when decoding forward motion vector
             * of im.pb frames */
            pmv0 = 0;
            pmv1 = 0;
            if (pb_frame == IM_PB_FRAMES && 
                (MODB == PBMODE_CBPB_FRW_PRED || MODB == PBMODE_FRW_PRED))
            {
              long_vectors = 0;
              if (MBA % mb_width && (pMODB == PBMODE_CBPB_FRW_PRED ||
                                     pMODB == PBMODE_FRW_PRED))
              {
                /* MBA%mb_width : if not new gob */
                pmv0 = pmvdbx;
                pmv1 = pmvdby;
              }
            }
            if (plus_type && long_vectors && !syntax_arith_coding)
            {
              mvdbx += pmv0;
              mvdby += pmv1;
            }
            else
            {
              mvdbx = motion_decode (mvdbx, pmv0);
              mvdby = motion_decode (mvdby, pmv1);
            }

            long_vectors = long_vectors_bak;

            /* This will not work if the PB deltas are so large they
             * require the second colums of the motion vector VLC table to
             * be used.  To fix this it is necessary to calculate the MV
             * predictor for the PB delta: TRB*MV/TRD here, and use this
             * as the second parameter to motion_decode(). The B vector
             * itself will then be returned from motion_decode(). This
             * will have to be changed to the PB delta again, since it is
             * the PB delta which is used later */
            if (trace)
            {
              fprintf (trace_file, "MVDB x: %d\n", mvdbx);
              fprintf (trace_file, "MVDB y: %d\n", mvdby);
            }
          } 
          else
          {
            mvdbx = 0;
            mvdby = 0;
          }
        }
      }
      /* Intra. */
      else
      {
        /* Set MVs to 0 for potential future use in true B direct   mode
         * prediction. */
        if (PCT_INTER == pict_type)
        {
          true_B_direct_mode_MV[0][ypos + 1][xpos + 1] = 0;
          true_B_direct_mode_MV[1][ypos + 1][xpos + 1] = 0;
        }
      }
      if (fault)
        goto resync;
    } 
    else
    {
      /* COD == 1 --> skipped MB */
      if (MBA >= MBAmax)
      {
        /* all macroblocks decoded */
        return;
      }
      if (!syntax_arith_coding)
        if (PCT_INTER == pict_type || PCT_IPB == pict_type)
          flushbits (1);

      Mode = MODE_INTER;
      /* Reset CBP */
      CBP = CBPB = 0;
      coded_map[ypos + 1][xpos + 1] = 0;
     
      /* reset motion vectors */
      MV[0][0][ypos + 1][xpos + 1] = 0;
      MV[1][0][ypos + 1][xpos + 1] = 0;

      if (PCT_INTER == pict_type)
      {
        true_B_direct_mode_MV[0][ypos + 1][xpos + 1] = 0;
        true_B_direct_mode_MV[1][ypos + 1][xpos + 1] = 0;
      }
      mvdbx = 0;
      mvdby = 0;    
    }

    /* Store mode and prediction type */
    modemap[ypos + 1][xpos + 1] = Mode;
    
    /* store defaults for advanced intra coding mode */
    if (advanced_intra_coding)
    {
      for (i = 0; i < blk_cnt; i++)
        store_qcoeff[MBA * blk_cnt * 64 + i * 64] = 1024;
    }
    if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
    {
      if (!pb_frame)
      {
        MV[0][0][ypos + 1][xpos + 1] = MV[1][0][ypos + 1][xpos + 1] = 0;
      }
    }

reconstruct_mb:

    /* pixel coordinates of top left corner of current macroblock */
    /* one delayed because of OBMC */
    if (xpos > 0)
    {
      bx = 16 * (xpos - 1);
      by = 16 * ypos;
    } 
    else
    {
      bx = coded_picture_width - 16;
      by = 16 * (ypos - 1);
    }

    if (MBA > 0)
    {
      Mode = modemap[by / 16 + 1][bx / 16 + 1];

      /* forward motion compensation for B-frame */
      if (pb_frame)
      {
        if (pCOD)
        {
          /* if the macroblock is not coded then it is bidir predicted */
          pMODB = PBMODE_BIDIR_PRED;
          reconstruct (bx, by, 0, pmvdbx, pmvdby, PBMODE_BIDIR_PRED);
        } 
        else
        {
          if (!(pb_frame == IM_PB_FRAMES && (pMODB == PBMODE_CBPB_BCKW_PRED || pMODB == PBMODE_BCKW_PRED)))
            reconstruct (bx, by, 0, pmvdbx, pmvdby, pMODB);
        }
      }
    
      /* motion compensation for P-frame */
      if (Mode == MODE_INTER || Mode == MODE_INTER_Q ||
          Mode == MODE_INTER4V || Mode == MODE_INTER4V_Q)
        reconstruct (bx, by, 1, 0, 0, pMODB);

      /* copy or add block data into P-picture */
      for (comp = 0; comp < blk_cnt; comp++)
      {
        /* inverse DCT */
        if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
        {
          if (refidct)
            idctref (ld->block[comp]);
          else
            idct (ld->block[comp]);
          addblock (comp, bx, by, 0);
        } 
        else if ((pCBP & (1 << (blk_cnt - 1 - comp))))
        {
          /* No need to to do this for blocks with no coeffs */
          if (refidct)
            idctref (ld->block[comp]);
          else
            idct (ld->block[comp]);
          addblock (comp, bx, by, 1);
        }
      }

      if (pb_frame)
      {

        if (pMODB == PBMODE_CBPB_BCKW_PRED || pMODB == PBMODE_BCKW_PRED)
        {
          reconstruct (bx, by, 0, 0, 0, pMODB);
        }
        /* add block data into B-picture */
        for (comp = 6; comp < blk_cnt + 6; comp++)
        {
          if (!pCOD || adv_pred_mode)
          {
            if (pb_frame == IM_PB_FRAMES)
            {
              if (pMODB == PBMODE_CBPB_BIDIR_PRED || pMODB == PBMODE_BIDIR_PRED || pCOD)
              {
                reconblock_b (comp - 6, bx, by, Mode, 0, 0);
              }
            } 
            else
            {
              reconblock_b (comp - 6, bx, by, Mode, pmvdbx, pmvdby);
            }
          }
          if ((pCBPB & (1 << (blk_cnt - 1 - comp % 6))))
          {
            if (refidct)
              idctref (ld->block[comp]);
            else
              idct (ld->block[comp]);
            addblock (comp, bx, by, 1);
          }
        }
      }
    }
    /* end if (MBA > 0) */
    if (!COD)
    {
      Mode = modemap[ypos + 1][xpos + 1];
    
      /* decode blocks */
      for (comp = 0; comp < blk_cnt; comp++)
      {
        clearblock (comp);
        if ((Mode == MODE_INTRA || Mode == MODE_INTRA_Q) && !(advanced_intra_coding))
        {
          /* Intra (except in advanced intra coding mode) */
          bp = ld->block[comp];
          if (syntax_arith_coding)
          {
            INTRADC_index = decode_a_symbol (cumf_INTRADC);
            bp[0] = intradctab[INTRADC_index];
            if (trace)
              fprintf (trace_file, "INTRADC Index: %d INTRADC: %d \n", INTRADC_index, bp[0]);
          } 
          else
          {
            bp[0] = vic_getbits (8);
            if (trace)
            {
              fprintf (trace_file, "DC[%d]: (", comp);
              printbits ((int) bp[0], 8, 8);
              fprintf (trace_file, "): %d\n", (int) bp[0]);
            }
          }

          if (bp[0] == 128)
            if (!quiet)
              fprintf (stderr, "Illegal DC-coeff: 1000000\n");
          if (bp[0] == 255)   /* Spec. in H.26P, not in TMN4 */
            bp[0] = 128;
          bp[0] *= 8;         /* Iquant */
          if ((CBP & (1 << (blk_cnt - 1 - comp))))
          {
            if (!syntax_arith_coding)
              getblock (comp, 0, 0, Mode);
            else
              get_sac_block (comp, 0, 0, Mode);
          }
        } 
        else
        {
          /* Inter (or Intra in advanced intra coding mode) */
          if ((CBP & (1 << (blk_cnt - 1 - comp))))
          {
            if (!syntax_arith_coding)
              getblock (comp, 1, INTRA_AC_DC, Mode);
            else
              get_sac_block (comp, 1, INTRA_AC_DC, Mode);
          }
        }
        if ((advanced_intra_coding) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
        {
          Intra_AC_DC_Decode (store_qcoeff, INTRA_AC_DC, MBA, xpos, ypos, comp, newgob);
          memcpy ((void *) (store_qcoeff + MBA * blk_cnt * 64 + comp * 64),
                  (void *) ld->block[comp], sizeof (short) * 64);
        }
        if (fault)
          goto resync;
      }

      /* Decode B blocks */
      if (pb_frame)
      {
        for (comp = 6; comp < blk_cnt + 6; comp++)
        {
          clearblock (comp);
          if ((CBPB & (1 << (blk_cnt - 1 - comp % 6))))
          {
            if (!syntax_arith_coding)
              getblock (comp, 1, 0, MODE_INTER);
            else
              get_sac_block (comp, 1, 0, MODE_INTER);
          } 
          if (fault)
            goto resync;        
        }
      }
    }
    
    /* advance to next macroblock */
    MBA++;
    pCBP = CBP;
    pCBPB = CBPB;
    pCOD = COD;
    pMODB = MODB;
    quant_map[ypos + 1][xpos + 1] = quant;

    pmvdbx = mvdbx;
    pmvdby = mvdby;
    fflush (stdout);

    if (MBA >= MBAmax && !last_done)
    {
      COD = 1;
      xpos = 0;
      ypos++;
      last_done = 1;
      goto reconstruct_mb;
    }
  }
}


/**********************************************************************
 *
 *	Name:		      get_B_MBs
 *	Description:  decode MBs for one B picture 
 *	Input:	      frame number
 *	Returns:      
 *	Side effects: 
 *
 *  Date: 971102  Author: mikeg@ee.ubc.ca
 *
 *
 ***********************************************************************/
 static void get_B_MBs (int framenum)
 {
  int comp;
  int MBA, MBAmax;
  int bx, by;
  int COD = 0, CBP = 0, Mode = 0, DQUANT;
  int mvx = 0, mvy = 0, xpos, ypos, gob, i;
  int mvfx = 0, mvfy = 0, mvbx = 0, mvby = 0, pmvf0, pmvf1, pmvb0, pmvb1;
  int gfid, gobheader_read;
  int last_done = 0, pCBP = 0, pCOD = 0;
  int DQ_tab[4] = {-1, -2, 1, 2};
  short *bp;
  int true_B_cbp = 0, true_B_quant = 0, true_B_prediction_type;
  int CBPC = 0, CBPY = 0, tmp = 0;

  /* variables used in advanced intra coding mode */
  int INTRA_AC_DC = 0;
  short *store_qcoeff;

  /* number of macroblocks per picture */
  MBAmax = mb_width * mb_height;

  MBA = 0;                      /* macroblock address */
  newgob = 0;

  /* mark MV's above the picture */
  for (i = 1; i < mb_width + 1; i++)
  {
    MV[0][0][0][i] = NO_VEC;
    MV[1][0][0][i] = NO_VEC;
    MV[0][5][0][i] = NO_VEC;
    MV[1][5][0][i] = NO_VEC;
    modemap[0][i] = MODE_INTRA;
    predictionmap[0][i] = B_INTRA_PREDICTION;
  }
  /* zero MV's on the sides of the picture */
  for (i = 0; i < mb_height + 1; i++)
  {
    MV[0][0][i][0] = 0;
    MV[1][0][i][0] = 0;
    MV[0][0][i][mb_width + 1] = 0;
    MV[1][0][i][mb_width + 1] = 0;

    MV[0][5][i][0] = 0;
    MV[1][5][i][0] = 0;
    MV[0][5][i][mb_width + 1] = 0;
    MV[1][5][i][mb_width + 1] = 0;

    modemap[i][0] = MODE_INTRA;
    modemap[i][mb_width + 1] = MODE_INTRA;
    predictionmap[i][0] = B_INTRA_PREDICTION;
    predictionmap[i][mb_width + 1] = B_INTRA_PREDICTION;
  }
  /* initialize the qcoeff used in advanced intra coding */
  if (advanced_intra_coding)
  {
    /* store the qcoeff for the frame */
    if ((store_qcoeff = (short *) calloc (64 * MBAmax * blk_cnt, sizeof (short))) == 0)
    {
      fprintf (stderr, "getMB(): Couldn't allocate store_qcoeff.\n");
      exit (-1);
    }
  }
  fault = 0;
  gobheader_read = 0;

  for (;;)
  {

    if (trace)
      fprintf (trace_file, "frame %d, MB %d\n", framenum, MBA);

resync:
    /* This version of the decoder does not resync on every possible
     * error, and it does not do all possible error checks. It is not
     * difficult to make it much more error robust, but I do not think it
     * is necessary to include this in the freely available version. */
   
    if (fault)
    {
      printf ("Warning: A Fault Condition Has Occurred - Resyncing \n");
      startcode ();             /* sync on new startcode */
      fault = 0;
    }
    if (!(showbits (22) >> 6))
    {
      /* startcode */
      startcode ();

      /* in case of byte aligned start code, ie. PSTUF, GSTUF or ESTUF is
       * used */
      if (showbits (22) == (32 | SE_CODE))
      {
        /* end of sequence */
        if (!(MBA < MBAmax))
        {
          return;
        }
      } 
      else if ((showbits (22) == PSC << 5))
      {
        /* new picture */
        if (!(MBA < MBAmax))
        {
          return;
        }
      } 
      else
      {
        if (!(MBA % mb_width))
        {
          gob = getheader () - 1;
          if (gob > mb_height)
          {
            if (!quiet)
              printf ("GN out of range\n");
            return;
          }
          /* GFID is not allowed to change unless PTYPE in picture header
           * changes */
          gfid = vic_getbits (2);
          if (trace)
          {
            fprintf (trace_file, "gfid: ");
            printbits (gfid, 2, 2);
          }
          /* NB: in error-prone environments the decoder can use this
           * value to determine whether a picture header where the PTYPE
           * has changed, has been lost */

          quant = vic_getbits (5);
          if (trace)
          {
            fprintf (trace_file, "quant: ");
            printbits (quant, 5, 5);
          }
          xpos = 0;
          ypos = gob;
          MBA = ypos * mb_width;

          newgob = 1;
          gobheader_read = 1;
        }
      }
    }

    /* SAC specific label */
    if (!gobheader_read)
    {
      xpos = MBA % mb_width;
      ypos = MBA / mb_width;
      if (xpos == 0 && ypos > 0)
        newgob = 0;
    } 
    else
      gobheader_read = 0;

    if (MBA >= MBAmax)
    {
      /* all macroblocks decoded */
      return;
    }
read_cod:

    COD = showbits (1);
    if (trace)
    {
      fprintf (trace_file, "COD : %d \n", COD);
    }

    if (!COD)
    {
      /* COD == 0 --> not skipped */

      /* flush COD bit */
      flushbits (1);
     
      true_B_prediction_type = getMBTYPE (&true_B_cbp, &true_B_quant);

      if (fault)
        goto resync;
      
      if (B_EI_EP_STUFFING == true_B_prediction_type)
      {
        /* stuffing - read next COD without advancing MB count. */        
        goto read_cod;        
      } 
      
      if (advanced_intra_coding && (B_INTRA_PREDICTION == true_B_prediction_type))
      {
        /* get INTRA_AC_DC mode for annex I */
        if (!showbits (1))
          INTRA_AC_DC = vic_getbits (1);
        else
          INTRA_AC_DC = vic_getbits (2);

        if (trace)
        {
          fprintf (trace_file, "INTRA_AC_DC: %d\n", INTRA_AC_DC);
        }
      }
      if (1 == true_B_cbp)
      {
        CBPC = getscalabilityCBPC ();
        CBPY = getCBPY ();          
      
        /* Decode Mode and CBP */     
        if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
        {
          /* Intra */

          /* needed in huffman coding only */
          CBPY = CBPY ^ 15;   
        } 
        /* Annex S.3 change */
        else if (alternative_inter_VLC_mode && (CBPC == 3) )
          CBPY = CBPY ^ 15;     
      
        CBP = (CBPY << 2) | CBPC;
      }
      else
      {
        CBP = 0;
      }

      if (B_INTRA_PREDICTION != true_B_prediction_type)
      {
        if (1 == true_B_quant)
        {
          Mode = MODE_INTER_Q;
        } 
        else
        {
          Mode = MODE_INTER;
        }
      }  
      else
      {
        coded_map[ypos + 1][xpos + 1] = 1;

        if (1 == true_B_quant)
        {
          Mode = MODE_INTRA_Q;
        } 
        else
        {
          Mode = MODE_INTRA;
        }
      }
    
      if (fault)
        goto resync;

      if (Mode == MODE_INTER_Q || Mode == MODE_INTRA_Q )
      {
        /* Read DQUANT if necessary */
        if (true_B_frame && true_B_quant)
        {
          if (!modified_quantization_mode)
          {
            DQUANT = vic_getbits (2);
            quant += DQ_tab[DQUANT];
            if (trace)
            {
              fprintf (trace_file, "DQUANT (");
              printbits (DQUANT, 2, 2);
              fprintf (trace_file, "): %d = %d\n", DQUANT, DQ_tab[DQUANT]);
            }
          } 
          else
          {
            tmp = vic_getbits (1);
            if (tmp)
            {                   /* only one more additional bit was sent */
              tmp = vic_getbits (1);
              if (tmp)
              {                 /* second bit of quant is 1 */
                DQUANT = change_of_quant_tab_11[quant];
              } else
              {                 /* second bit of quant is 0 */
                DQUANT = change_of_quant_tab_10[quant];
              }
              quant += DQUANT;
              if (trace)
              {
                fprintf (trace_file, "DQUANT (1");
                printbits (tmp, 1, 1);
                fprintf (trace_file, "): %d \n", DQUANT);
              }
            } 
            else
            {                   /* five additional bits were sent as
                                 * DQUANT */
              DQUANT = vic_getbits (5);
              quant = DQUANT;
              if (trace)
              {
                fprintf (trace_file, "DQUANT (");
                printbits (DQUANT, 5, 5);
                fprintf (trace_file, "): %d \n", DQUANT);
              }
            }
          }
        }
        if (quant > 31 || quant < 1)
        {
          if (!quiet)
            printf ("Quantizer out of range: clipping\n");
          quant = mmax (1, mmin (31, quant));
          /* could set fault-flag and resync here */
        }
      }
      
      /* motion vectors */
      if (Mode == MODE_INTER || Mode == MODE_INTER_Q )
      {
        switch (true_B_prediction_type)
        {
          case B_FORWARD_PREDICTION:
           
            if (plus_type && long_vectors)
            {
              mvfx = getRVLC ();
              mvfy = getRVLC ();

              /* flush start code emulation bit */
              if (mvfx == 1 && mvfy == 1)
                flushbits(1);
            }
            else
            {
              mvfx = getTMNMV ();
              mvfy = getTMNMV ();
            }

            pmvf0 = find_pmv (xpos, ypos, 0, 0);
            pmvf1 = find_pmv (xpos, ypos, 0, 1);

            if (plus_type && long_vectors)
            {
              mvfx += pmvf0;
              mvfy += pmvf1;
            }
            else
            {
              mvfx = motion_decode (mvfx, pmvf0);
              mvfy = motion_decode (mvfy, pmvf1);
            }
            
            if (trace)
            {
              fprintf (trace_file, "mvfx: %d\n", mvfx);
              fprintf (trace_file, "mvfy: %d\n", mvfy);
            }
            
            MV[0][0][ypos + 1][xpos + 1] = mvfx;
            MV[1][0][ypos + 1][xpos + 1] = mvfy;

            MV[0][5][ypos + 1][xpos + 1] = 0;
            MV[1][5][ypos + 1][xpos + 1] = 0;

            break;

          case B_BACKWARD_PREDICTION:

            if (plus_type && long_vectors)
            {
              mvbx = getRVLC ();
              mvby = getRVLC ();

              /* flush start code emulation bit */
              if (mvbx == 1 && mvby == 1)
                flushbits(1);
            }
            else
            {
              mvbx = getTMNMV ();
              mvby = getTMNMV ();               
            }

            pmvb0 = find_pmv (xpos, ypos, 5, 0);
            pmvb1 = find_pmv (xpos, ypos, 5, 1);

            if (plus_type && long_vectors)
            {
              mvbx += pmvb0;
              mvby += pmvb1;
            }
            else
            {
              mvbx = motion_decode (mvbx, pmvb0);
              mvby = motion_decode (mvby, pmvb1);
            }
            
            if (trace)
            {
              fprintf (trace_file, "mvbx: %d\n", mvbx);
              fprintf (trace_file, "mvby: %d\n", mvby);
            }
           
            MV[0][5][ypos + 1][xpos + 1] = mvbx;
            MV[1][5][ypos + 1][xpos + 1] = mvby;

            MV[0][0][ypos + 1][xpos + 1] = 0;
            MV[1][0][ypos + 1][xpos + 1] = 0;

            break;

          case B_BIDIRECTIONAL_PREDICTION:

            if (plus_type && long_vectors)
            {
              mvfx = getRVLC ();
              mvfy = getRVLC ();

              /* flush start code emulation bit */
              if (mvfx == 1 && mvfy == 1)
                flushbits(1);
            }
            else
            {
              mvfx = getTMNMV ();
              mvfy = getTMNMV ();
            }

            pmvf0 = find_pmv (xpos, ypos, 0, 0);
            pmvf1 = find_pmv (xpos, ypos, 0, 1);

            if (plus_type && long_vectors)
            {
              mvfx += pmvf0;
              mvfy += pmvf1;
            }
            else
            {
              mvfx = motion_decode (mvfx, pmvf0);
              mvfy = motion_decode (mvfy, pmvf1);
            }
            
            if (trace)
            {
              fprintf (trace_file, "mvfx: %d\n", mvfx);
              fprintf (trace_file, "mvfy: %d\n", mvfy);
            }

            if (plus_type && long_vectors)
            {
              mvbx = getRVLC ();
              mvby = getRVLC ();

              /* flush start code emulation bit */
              if (mvbx == 1 && mvby == 1)
                flushbits(1);
            }
            else
            {
              mvbx = getTMNMV ();
              mvby = getTMNMV ();
            }
 
            pmvb0 = find_pmv (xpos, ypos, 5, 0);
            pmvb1 = find_pmv (xpos, ypos, 5, 1);

            if (plus_type && long_vectors)
            {
              mvbx += pmvb0;
              mvby += pmvb1;
            }
            else
            {
              mvbx = motion_decode (mvbx, pmvb0);
              mvby = motion_decode (mvby, pmvb1);
            }
            
            if (trace)
            {
              fprintf (trace_file, "mvbx: %d\n", mvbx);
              fprintf (trace_file, "mvby: %d\n", mvby);
            }
              
            MV[0][0][ypos + 1][xpos + 1] = mvfx;
            MV[1][0][ypos + 1][xpos + 1] = mvfy;

            MV[0][5][ypos + 1][xpos + 1] = mvbx;
            MV[1][5][ypos + 1][xpos + 1] = mvby;

            break;

          default:

            /* No MV data for other modes. */
            MV[0][0][ypos + 1][xpos + 1] = 0;
            MV[1][0][ypos + 1][xpos + 1] = 0;

            MV[0][5][ypos + 1][xpos + 1] = 0;
            MV[1][5][ypos + 1][xpos + 1] = 0;

            break;
        }
      }

      if (fault)
        goto resync;
    } 
    else
    {
      /* COD == 1 --> skipped MB */
      if (MBA >= MBAmax)
      {
        /* all macroblocks decoded */
        return;
      }
      flushbits (1);

      Mode = MODE_INTER;

      /* Reset CBP */
      CBP = 0;

      coded_map[ypos + 1][xpos + 1] = 0;

      true_B_prediction_type = B_DIRECT_PREDICTION;
      
      /* reset motion vectors */
      MV[0][0][ypos + 1][xpos + 1] = 0;
      MV[1][0][ypos + 1][xpos + 1] = 0;
      MV[0][5][ypos + 1][xpos + 1] = 0;
      MV[1][5][ypos + 1][xpos + 1] = 0;
    } 

    /* Store mode and prediction type */
    modemap[ypos + 1][xpos + 1] = Mode;
    predictionmap[ypos + 1][xpos + 1] = true_B_prediction_type;

    /* store defaults for advanced intra coding mode */
    if (advanced_intra_coding)
    {
      for (i = 0; i < blk_cnt; i++)
        store_qcoeff[MBA * blk_cnt * 64 + i * 64] = 1024;
    }
    if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
    {
      MV[0][0][ypos + 1][xpos + 1] = MV[1][0][ypos + 1][xpos + 1] = 0;
      MV[0][5][ypos + 1][xpos + 1] = MV[1][5][ypos + 1][xpos + 1] = 0;  
    }

reconstruct_mb:

    /* pixel coordinates of top left corner of current macroblock */
    /* one delayed because of OBMC */
    if (xpos > 0)
    {
      bx = 16 * (xpos - 1);
      by = 16 * ypos;
    } else
    {
      bx = coded_picture_width - 16;
      by = 16 * (ypos - 1);
    }

    if (MBA > 0)
    {
      Mode = modemap[by / 16 + 1][bx / 16 + 1];
      true_B_prediction_type = predictionmap[by / 16 + 1][bx / 16 + 1];

      /* motion compensation for true B frame. */
      if (Mode == MODE_INTER || Mode == MODE_INTER_Q)
        reconstruct_true_B (bx, by, true_B_prediction_type);

      /* copy or add block data into true B picture */
      for (comp = 0; comp < blk_cnt; comp++)
      {
        /* inverse DCT */
        if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
        {
          if (refidct)
            idctref (ld->block[comp]);
          else
            idct (ld->block[comp]);
          addblock (comp, bx, by, 0);
        } 
        else if ((pCBP & (1 << (blk_cnt - 1 - comp))))
        {
          /* No need to to do this for blocks with no coeffs */
          if (refidct)
            idctref (ld->block[comp]);
          else
            idct (ld->block[comp]);
          addblock (comp, bx, by, 1);
        }
      }  
    }    
    /* end if (MBA > 0) */
    if (!COD)
    {
      Mode = modemap[ypos + 1][xpos + 1];
      true_B_prediction_type = predictionmap[ypos + 1][xpos + 1];

      for (comp = 0; comp < blk_cnt; comp++)
      {
        clearblock (comp);
        if ((Mode == MODE_INTRA || Mode == MODE_INTRA_Q) && !(advanced_intra_coding))
        {
          /* Intra (except in advanced intra coding mode) */
          bp = ld->block[comp];
          bp[0] = vic_getbits (8);
          if (trace)
          {
            fprintf (trace_file, "DC[%d]: (", comp);
            printbits ((int) bp[0], 8, 8);
            fprintf (trace_file, "): %d\n", (int) bp[0]);
          }
          if (bp[0] == 128)
            if (!quiet)
              fprintf (stderr, "Illegal DC-coeff: 1000000\n");
          if (bp[0] == 255)   /* Spec. in H.26P, not in TMN4 */
            bp[0] = 128;
          bp[0] *= 8;         /* Iquant */

          if ((CBP & (1 << (blk_cnt - 1 - comp))))
          {
            getblock (comp, 0, 0, Mode);
          }
        } 
        else
        {
          /* Inter (or Intra in advanced intra coding mode) */
          if ((CBP & (1 << (blk_cnt - 1 - comp))))
          {
            getblock (comp, 1, INTRA_AC_DC, Mode);
          }
        }
        if ((advanced_intra_coding) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
        {
          Intra_AC_DC_Decode (store_qcoeff, INTRA_AC_DC, MBA, xpos, ypos, comp, newgob);
          memcpy ((void *) (store_qcoeff + MBA * blk_cnt * 64 + comp * 64),
                  (void *) ld->block[comp], sizeof (short) * 64);

        }
        if (fault)
          goto resync;
      }
    }

    /* advance to next macroblock */
    MBA++;
    pCBP = CBP;
    pCOD = COD;
    quant_map[ypos + 1][xpos + 1] = quant;

    fflush (stdout);

    if (MBA >= MBAmax && !last_done)
    {
      COD = 1;
      xpos = 0;
      ypos++;
      last_done = 1;
      goto reconstruct_mb;
    }
  }
}


/**********************************************************************
 *
 *	Name:		      get_EI_EP_MBs
 *	Description:  decode MBs for one EI or EP picture 
 *	Input:	      frame number
 *	Returns:      
 *	Side effects: 
 *
 *  Date: 971102  Author: mikeg@ee.ubc.ca
 *
 ***********************************************************************/
 static void get_EI_EP_MBs (int framenum)
{
  int comp;
  int MBA, MBAmax;
  int bx, by;
  int COD = 0, CBP = 0, Mode = 0, DQUANT;
  int mvx = 0, mvy = 0, xpos, ypos, gob, i;
  int mvfx = 0, mvfy = 0, mvbx = 0, mvby = 0, pmvf0, pmvf1;
  int gfid, gobheader_read;
  int last_done = 0, pCBP = 0, pCOD = 0;
  int DQ_tab[4] = {-1, -2, 1, 2};
  short *bp;
  int ei_ep_cbp = 0, ei_ep_quant = 0, ei_ep_prediction_type;
  int CBPC = 0, CBPY = 0, tmp = 0;

  /* variables used in advanced intra coding mode */
  int INTRA_AC_DC = 0;
  short *store_qcoeff;

  MBAmax = mb_width * mb_height;

  MBA = 0;                      /* macroblock address */
  newgob = 0;

  /* mark MV's above the picture */
  for (i = 1; i < mb_width + 1; i++)
  {
    MV[0][0][0][i] = NO_VEC;
    MV[1][0][0][i] = NO_VEC;
    modemap[0][i] = MODE_INTRA;
    predictionmap[0][i] = EI_EP_INTRA_PREDICTION;
  }
  /* zero MV's on the sides of the picture */
  for (i = 0; i < mb_height + 1; i++)
  {
    MV[0][0][i][0] = 0;
    MV[1][0][i][0] = 0;    
    MV[0][0][i][mb_width + 1] = 0;    
    MV[1][0][i][mb_width + 1] = 0;
    modemap[i][0] = MODE_INTRA;
    modemap[i][mb_width + 1] = MODE_INTRA;
    predictionmap[i][0] = EI_EP_INTRA_PREDICTION;
    predictionmap[i][mb_width + 1] = EI_EP_INTRA_PREDICTION;
  }
  /* initialize the qcoeff used in advanced intra coding */
  if (advanced_intra_coding)
  {
    /* store the qcoeff for the frame */
    if ((store_qcoeff = (short *) calloc (64 * MBAmax * blk_cnt, sizeof (short))) == 0)
    {
      fprintf (stderr, "getMB(): Couldn't allocate store_qcoeff.\n");
      exit (-1);
    }
  }
  fault = 0;
  gobheader_read = 0;

  for (;;)
  {

    if (trace)
      fprintf (trace_file, "frame %d, MB %d\n", framenum, MBA);

resync:
    /* This version of the decoder does not resync on every possible
     * error, and it does not do all possible error checks. It is not
     * difficult to make it much more error robust, but I do not think it
     * is necessary to include this in the freely available version. */
   
    if (fault)
    {
      printf ("Warning: A Fault Condition Has Occurred - Resyncing \n");
      startcode ();             /* sync on new startcode */
      fault = 0;
    }
    if (!(showbits (22) >> 6))
    {
      /* startcode */
      startcode ();

      /* in case of byte aligned start code, ie. PSTUF, GSTUF or ESTUF is
       * used */
      if (showbits (22) == (32 | SE_CODE))
      {
        /* end of sequence */
        if (!(MBA < MBAmax))
        {
          return;
        }
      } 
      else if ((showbits (22) == PSC << 5))
      {
        /* new picture */
        if (!(MBA < MBAmax))
        {
          return;
        }
      } 
      else
      {
        if (!(MBA % mb_width))
        {
          gob = getheader () - 1;
          if (gob > mb_height)
          {
            if (!quiet)
              printf ("GN out of range\n");
            return;
          }
          /* GFID is not allowed to change unless PTYPE in picture header
           * changes */
          gfid = vic_getbits (2);
          if (trace)
          {
            fprintf (trace_file, "gfid: ");
            printbits (gfid, 2, 2);
          }
          /* NB: in error-prone environments the decoder can use this
           * value to determine whether a picture header where the PTYPE
           * has changed, has been lost */

          quant = vic_getbits (5);
          if (trace)
          {
            fprintf (trace_file, "quant: ");
            printbits (quant, 5, 5);
          }
          xpos = 0;
          ypos = gob;
          MBA = ypos * mb_width;

          newgob = 1;
          gobheader_read = 1;
        }
      }
    }

    /* SAC specific label */
    if (!gobheader_read)
    {
      xpos = MBA % mb_width;
      ypos = MBA / mb_width;
      if (xpos == 0 && ypos > 0)
        newgob = 0;
    } 
    else
      gobheader_read = 0;

    if (MBA >= MBAmax)
    {
      /* all macroblocks decoded */
      return;
    }
read_cod:

    COD = showbits (1);
    if (trace)
    {
      fprintf (trace_file, "COD : %d \n", COD);
    }

    if (!COD)
    {
      /* COD == 0 --> not skipped */

      /* flush COD bit */
      flushbits (1);
     
      ei_ep_prediction_type = getMBTYPE (&ei_ep_cbp, &ei_ep_quant);

      if (fault)
        goto resync;
      
      if (B_EI_EP_STUFFING == ei_ep_prediction_type)
      {
        /* stuffing - read next COD without advancing MB count. */        
        goto read_cod;        
      } 
      
      if (advanced_intra_coding && (EI_EP_INTRA_PREDICTION == ei_ep_prediction_type))
      {
        /* get INTRA_AC_DC mode for annex I */
        if (!showbits (1))
          INTRA_AC_DC = vic_getbits (1);
        else
          INTRA_AC_DC = vic_getbits (2);

        if (trace)
        {
          fprintf (trace_file, "INTRA_AC_DC: %d\n", INTRA_AC_DC);
        }
      }
      if (1 == ei_ep_cbp || PCT_EI == pict_type )
      {
        if (PCT_EP == pict_type)
          CBPC = getscalabilityCBPC ();
        else
          CBPC = ei_ep_cbp;

        CBPY = getCBPY ();          
      
        /* Decode Mode and CBP */     
        if ( (MODE_INTRA == Mode || MODE_INTRA_Q == Mode) ||
             (EI_EP_UPWARD_PREDICTION == ei_ep_prediction_type) ||
             (PCT_EP == pict_type &&  
              EP_BIDIRECTIONAL_PREDICTION == ei_ep_prediction_type) ) 
        {
          /* needed in huffman coding only */
          CBPY = CBPY ^ 15;   
        } 
        /* Annex S.3 change */
        else if (alternative_inter_VLC_mode && (CBPC == 3) )
          CBPY = CBPY ^ 15;     
      
        CBP = (CBPY << 2) | CBPC;
      }
      else
      {
        CBP = 0;
      }

      if (EI_EP_INTRA_PREDICTION != ei_ep_prediction_type)
      {
        if (1 == ei_ep_quant)
        {
          Mode = MODE_INTER_Q;
        } 
        else
        {
          Mode = MODE_INTER;
        }
      }  
      else
      {
        coded_map[ypos + 1][xpos + 1] = 1;

        if (1 == ei_ep_quant)
        {
          Mode = MODE_INTRA_Q;
        } 
        else
        {
          Mode = MODE_INTRA;
        }
      }
    
      if (fault)
        goto resync;

      if (Mode == MODE_INTER_Q || Mode == MODE_INTRA_Q )
      {
        /* Read DQUANT if necessary */
        if (ei_ep_quant)
        {
          if (!modified_quantization_mode)
          {
            DQUANT = vic_getbits (2);
            quant += DQ_tab[DQUANT];
            if (trace)
            {
              fprintf (trace_file, "DQUANT (");
              printbits (DQUANT, 2, 2);
              fprintf (trace_file, "): %d = %d\n", DQUANT, DQ_tab[DQUANT]);
            }
          } 
          else
          {
            tmp = vic_getbits (1);
            if (tmp)
            {                   /* only one more additional bit was sent */
              tmp = vic_getbits (1);
              if (tmp)
              {                 /* second bit of quant is 1 */
                DQUANT = change_of_quant_tab_11[quant];
              } else
              {                 /* second bit of quant is 0 */
                DQUANT = change_of_quant_tab_10[quant];
              }
              quant += DQUANT;
              if (trace)
              {
                fprintf (trace_file, "DQUANT (1");
                printbits (tmp, 1, 1);
                fprintf (trace_file, "): %d \n", DQUANT);
              }
            } 
            else
            {                   /* five additional bits were sent as
                                 * DQUANT */
              DQUANT = vic_getbits (5);
              quant = DQUANT;
              if (trace)
              {
                fprintf (trace_file, "DQUANT (");
                printbits (DQUANT, 5, 5);
                fprintf (trace_file, "): %d \n", DQUANT);
              }
            }
          }
        }
        if (quant > 31 || quant < 1)
        {
          if (!quiet)
            printf ("Quantizer out of range: clipping\n");
          quant = mmax (1, mmin (31, quant));
          /* could set fault-flag and resync here */
        }
      }
      
      /* motion vectors */
      if (Mode == MODE_INTER || Mode == MODE_INTER_Q )
      {
        switch (ei_ep_prediction_type)
        {
          case EP_FORWARD_PREDICTION:
             
            if (plus_type && long_vectors)
            {
              mvfx = getRVLC ();
              mvfy = getRVLC ();

              /* flush start code emulation bit */
              if (mvfx == 1 && mvfy == 1)
                flushbits(1);
            }
            else
            {
              mvfx = getTMNMV ();
              mvfy = getTMNMV ();
            }

            pmvf0 = find_pmv (xpos, ypos, 0, 0);
            pmvf1 = find_pmv (xpos, ypos, 0, 1);

            if (plus_type && long_vectors)
            {
              mvfx += pmvf0;
              mvfy += pmvf1;
            }
            else
            {
              mvfx = motion_decode (mvfx, pmvf0);
              mvfy = motion_decode (mvfy, pmvf1);
            }
            
            if (trace)
            {
              fprintf (trace_file, "mvfx: %d\n", mvfx);
              fprintf (trace_file, "mvfy: %d\n", mvfy);
            }
            
            MV[0][0][ypos + 1][xpos + 1] = mvfx;
            MV[1][0][ypos + 1][xpos + 1] = mvfy;

            break;

          case EP_BIDIRECTIONAL_PREDICTION:

            /* No MVs for Bi-Dir (no texture) MBTYPE */
            if ( (0 == ei_ep_cbp) && (0 == ei_ep_quant) )
            {
              MV[0][0][ypos + 1][xpos + 1] = 0;
              MV[1][0][ypos + 1][xpos + 1] = 0;
              break;
            }

            if (plus_type && long_vectors)
            {
              mvfx = getRVLC ();
              mvfy = getRVLC ();

              /* flush start code emulation bit */
              if (mvfx == 1 && mvfy == 1)
                flushbits(1);
            }
            else
            {
              mvfx = getTMNMV ();
              mvfy = getTMNMV ();
            }

            pmvf0 = find_pmv (xpos, ypos, 0, 0);
            pmvf1 = find_pmv (xpos, ypos, 0, 1);

            if (plus_type && long_vectors)
            {
              mvfx += pmvf0;
              mvfy += pmvf1;
            }
            else
            {
              mvfx = motion_decode (mvfx, pmvf0);
              mvfy = motion_decode (mvfy, pmvf1);
            }
            
            if (trace)
            {
              fprintf (trace_file, "mvfx: %d\n", mvfx);
              fprintf (trace_file, "mvfy: %d\n", mvfy);
            }
              
            MV[0][0][ypos + 1][xpos + 1] = mvfx;
            MV[1][0][ypos + 1][xpos + 1] = mvfy;

            break;

          case EI_EP_UPWARD_PREDICTION:
          default:

            /* No MV data for other modes. */
            MV[0][0][ypos + 1][xpos + 1] = 0;
            MV[1][0][ypos + 1][xpos + 1] = 0;

            break;
        }
      }

      if (fault)
        goto resync;
    } 
    else
    {
      /* COD == 1 --> skipped MB */
      if (MBA >= MBAmax)
      {
        /* all macroblocks decoded */
        return;
      }
      flushbits (1);

      Mode = MODE_INTER;

      /* Reset CBP */
      CBP = 0;

      coded_map[ypos + 1][xpos + 1] = 0;

      if (PCT_EI == pict_type)
      {
        ei_ep_prediction_type = EI_EP_UPWARD_PREDICTION;
      }
      else
      {
        ei_ep_prediction_type = EP_FORWARD_PREDICTION;
      }
      
      /* reset motion vectors */
      MV[0][0][ypos + 1][xpos + 1] = 0;
      MV[1][0][ypos + 1][xpos + 1] = 0;
    } 

    /* Store mode and prediction type */
    modemap[ypos + 1][xpos + 1] = Mode;
    predictionmap[ypos + 1][xpos + 1] = ei_ep_prediction_type;

    /* store defaults for advanced intra coding mode */
    if (advanced_intra_coding)
    {
      for (i = 0; i < blk_cnt; i++)
        store_qcoeff[MBA * blk_cnt * 64 + i * 64] = 1024;
    }
    if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
    {
      MV[0][0][ypos + 1][xpos + 1] = MV[1][0][ypos + 1][xpos + 1] = 0;
    }

reconstruct_mb:

    /* pixel coordinates of top left corner of current macroblock */
    /* one delayed because of OBMC */
    if (xpos > 0)
    {
      bx = 16 * (xpos - 1);
      by = 16 * ypos;
    } else
    {
      bx = coded_picture_width - 16;
      by = 16 * (ypos - 1);
    }

    if (MBA > 0)
    {
      Mode = modemap[by / 16 + 1][bx / 16 + 1];
      ei_ep_prediction_type = predictionmap[by / 16 + 1][bx / 16 + 1];

      /* motion compensation for true B frame. */
      if (Mode == MODE_INTER || Mode == MODE_INTER_Q)
        reconstruct_ei_ep (bx, by, ei_ep_prediction_type);

      /* copy or add block data into true B picture */
      for (comp = 0; comp < blk_cnt; comp++)
      {
        /* inverse DCT */
        if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
        {
          if (refidct)
            idctref (ld->block[comp]);
          else
            idct (ld->block[comp]);
          addblock (comp, bx, by, 0);
        } 
        else if ((pCBP & (1 << (blk_cnt - 1 - comp))))
        {
          /* No need to to do this for blocks with no coeffs */
          if (refidct)
            idctref (ld->block[comp]);
          else
            idct (ld->block[comp]);
          addblock (comp, bx, by, 1);
        }
      }  
    }    
    /* end if (MBA > 0) */
    if (!COD)
    {
      Mode = modemap[ypos + 1][xpos + 1];
      ei_ep_prediction_type = predictionmap[ypos + 1][xpos + 1];

      for (comp = 0; comp < blk_cnt; comp++)
      {
        clearblock (comp);
        if ((Mode == MODE_INTRA || Mode == MODE_INTRA_Q) && !(advanced_intra_coding))
        {
          /* Intra (except in advanced intra coding mode) */
          bp = ld->block[comp];
          bp[0] = vic_getbits (8);
          if (trace)
          {
            fprintf (trace_file, "DC[%d]: (", comp);
            printbits ((int) bp[0], 8, 8);
            fprintf (trace_file, "): %d\n", (int) bp[0]);
          }
          if (bp[0] == 128)
            if (!quiet)
              fprintf (stderr, "Illegal DC-coeff: 1000000\n");
          if (bp[0] == 255)   /* Spec. in H.26P, not in TMN4 */
            bp[0] = 128;
          bp[0] *= 8;         /* Iquant */

          if ((CBP & (1 << (blk_cnt - 1 - comp))))
          {
            getblock (comp, 0, 0, Mode);
          }
        } 
        else
        {
          /* Inter (or Intra in advanced intra coding mode) */
          if ((CBP & (1 << (blk_cnt - 1 - comp))))
          {
            getblock (comp, 1, INTRA_AC_DC, Mode);
          }
        }
        if ((advanced_intra_coding) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))
        {
          Intra_AC_DC_Decode (store_qcoeff, INTRA_AC_DC, MBA, xpos, ypos, comp, newgob);
          memcpy ((void *) (store_qcoeff + MBA * blk_cnt * 64 + comp * 64),
                  (void *) ld->block[comp], sizeof (short) * 64);

        }
        if (fault)
          goto resync;
      }
    }

    /* advance to next macroblock */
    MBA++;
    pCBP = CBP;
    pCOD = COD;
    quant_map[ypos + 1][xpos + 1] = quant;

    fflush (stdout);

    if (MBA >= MBAmax && !last_done)
    {
      COD = 1;
      xpos = 0;
      ypos++;
      last_done = 1;
      goto reconstruct_mb;
    }
  }
}
/* set block to zero */

static void clearblock (int comp)
{
  int *bp;
  int i;

  bp = (int *) ld->block[comp];

  for (i = 0; i < 8; i++)
  {
    bp[0] = bp[1] = bp[2] = bp[3] = 0;
    bp += 4;
  }
}


/* move/add 8x8-Block from block[comp] to refframe or bframe */

static void addblock (int comp, int bx, int by, int addflag)
{
  int cc, i, iincr, P = 1;
  unsigned char *rfp;
  short *bp;
  unsigned char *curr[3];

  if (enhancement_layer_num > 1)
  {
    curr[0] = current_enhancement_frame[enhancement_layer_num-2][0];
    curr[1] = current_enhancement_frame[enhancement_layer_num-2][1];
    curr[2] = current_enhancement_frame[enhancement_layer_num-2][2];
  }
  else
  {
    curr[0] = current_frame[0];
    curr[1] = current_frame[1];
    curr[2] = current_frame[2];
  }


  bp = ld->block[comp];

  if (comp >= 6)
  {
    /* This is a component for B-frame forward prediction */
    P = 0;
    addflag = 1;
    comp -= 6;
  }
  cc = (comp < 4) ? 0 : (comp & 1) + 1; /* color component index */

  if (cc == 0)
  {
    /* luminance */

    /* frame DCT coding */
    if (P)
      rfp = curr[0]
        + coded_picture_width * (by + ((comp & 2) << 2)) + bx + ((comp & 1) << 3);
    else  
      rfp = bframe[0]
        + coded_picture_width * (by + ((comp & 2) << 2)) + bx + ((comp & 1) << 3);
    iincr = coded_picture_width;
  } 
  else
  {
    /* chrominance */

    /* scale coordinates */
    bx >>= 1;
    by >>= 1;
    /* frame DCT coding */
    if (P)
      rfp = curr[cc] + chrom_width * by + bx;
    else
      rfp = bframe[cc] + chrom_width * by + bx;
    iincr = chrom_width;
  }


  if (addflag)
  {
    for (i = 0; i < 8; i++)
    {
      rfp[0] = clp[bp[0] + rfp[0]];
      rfp[1] = clp[bp[1] + rfp[1]];
      rfp[2] = clp[bp[2] + rfp[2]];
      rfp[3] = clp[bp[3] + rfp[3]];
      rfp[4] = clp[bp[4] + rfp[4]];
      rfp[5] = clp[bp[5] + rfp[5]];
      rfp[6] = clp[bp[6] + rfp[6]];
      rfp[7] = clp[bp[7] + rfp[7]];
      bp += 8;
      rfp += iincr;
    }
  } else
  {
    for (i = 0; i < 8; i++)
    {
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

  /* printf("%s", (cc ? "Chrominance\n" : "Luminance\n"));  for (i=0; i<8;
   * ++i) { printf("%d %d %d %d %d %d %d %d\n", rfp[0], rfp[1], rfp[2],
   * rfp[3],  rfp[4], rfp[5], rfp[6], rfp[7]); rfp+= iincr; }
   * printf("\n");  */
}

/* bidirectionally reconstruct 8x8-Block from block[comp] to bframe */

static void reconblock_b (int comp, int bx, int by, int mode, int bdx, int bdy)
{
  int cc, i, j, k, ii;
  unsigned char *bfr, *ffr;
  int BMVx, BMVy;
  int xa, xb, ya, yb, x, y, xvec, yvec, mvx, mvy;
  int xint, xhalf, yint, yhalf, pel;

  x = bx / 16 + 1;
  y = by / 16 + 1;

  if (mode == MODE_INTER4V || mode == MODE_INTER4V_Q)
  {
    if (comp < 4)
    {
      /* luma */
      mvx = MV[0][comp + 1][y][x];
      mvy = MV[1][comp + 1][y][x];
      BMVx = (bdx == 0 ? (trb - trd) * mvx / trd : trb * mvx / trd + bdx - mvx);
      BMVy = (bdy == 0 ? (trb - trd) * mvy / trd : trb * mvy / trd + bdy - mvy);
    } else
    {
      /* chroma */
      xvec = yvec = 0;
      for (k = 1; k <= 4; k++)
      {
        mvx = MV[0][k][y][x];
        mvy = MV[1][k][y][x];
        xvec += (bdx == 0 ? (trb - trd) * mvx / trd : trb * mvx / trd + bdx - mvx);
        yvec += (bdy == 0 ? (trb - trd) * mvy / trd : trb * mvy / trd + bdy - mvy);
      }

      /* chroma rounding (table 16/H.263) */
      BMVx = sign (xvec) * (vic_roundtab[abs (xvec) % 16] + (abs (xvec) / 16) * 2);
      BMVy = sign (yvec) * (vic_roundtab[abs (yvec) % 16] + (abs (yvec) / 16) * 2);
    }
  } else
  {
    if (comp < 4)
    {
      /* luma */
      mvx = MV[0][0][y][x];
      mvy = MV[1][0][y][x];
      BMVx = (bdx == 0 ? (trb - trd) * mvx / trd : trb * mvx / trd + bdx - mvx);
      BMVy = (bdy == 0 ? (trb - trd) * mvy / trd : trb * mvy / trd + bdy - mvy);
    } else
    {
      /* chroma */
      mvx = MV[0][0][y][x];
      mvy = MV[1][0][y][x];
      xvec = (bdx == 0 ? (trb - trd) * mvx / trd : trb * mvx / trd + bdx - mvx);
      yvec = (bdy == 0 ? (trb - trd) * mvy / trd : trb * mvy / trd + bdy - mvy);
      xvec *= 4;
      yvec *= 4;

      /* chroma rounding (table 16/H.263) */
      BMVx = sign (xvec) * (vic_roundtab[abs (xvec) % 16] + (abs (xvec) / 16) * 2);
      BMVy = sign (yvec) * (vic_roundtab[abs (yvec) % 16] + (abs (yvec) / 16) * 2);
    }
  }

  cc = (comp < 4) ? 0 : (comp & 1) + 1; /* color component index */

  if (cc == 0)
  {
    /* luminance */
    find_bidir_limits (BMVx, &xa, &xb, comp & 1);
    find_bidir_limits (BMVy, &ya, &yb, (comp & 2) >> 1);
    bfr = bframe[0] +
      coded_picture_width * (by + ((comp & 2) << 2)) + bx + ((comp & 1) << 3);
    ffr = current_frame[0] +
      coded_picture_width * (by + ((comp & 2) << 2)) + bx + ((comp & 1) << 3);
    ii = coded_picture_width;
  } else
  {
    /* chrominance */
    /* scale coordinates and vectors */
    bx >>= 1;
    by >>= 1;

    find_bidir_chroma_limits (BMVx, &xa, &xb);
    find_bidir_chroma_limits (BMVy, &ya, &yb);

    bfr = bframe[cc] + chrom_width * (by + ((comp & 2) << 2)) + bx + (comp & 8);
    ffr = current_frame[cc] + chrom_width * (by + ((comp & 2) << 2)) + bx + (comp & 8);
    ii = chrom_width;
  }

  xint = BMVx >> 1;
  xhalf = BMVx - 2 * xint;
  yint = BMVy >> 1;
  yhalf = BMVy - 2 * yint;

  ffr += xint + (yint + ya) * ii;
  bfr += ya * ii;

  if (!xhalf && !yhalf)
  {
    for (j = ya; j < yb; j++)
    {
      for (i = xa; i < xb; i++)
      {
        pel = ffr[i];
        bfr[i] = ((unsigned int) (pel + bfr[i])) >> 1;
      }
      bfr += ii;
      ffr += ii;
    }
  } else if (xhalf && !yhalf)
  {
    for (j = ya; j < yb; j++)
    {
      for (i = xa; i < xb; i++)
      {
        pel = ((unsigned int) (ffr[i] + ffr[i + 1] + 1)) >> 1;
        bfr[i] = ((unsigned int) (pel + bfr[i])) >> 1;
      }
      bfr += ii;
      ffr += ii;
    }
  } else if (!xhalf && yhalf)
  {
    for (j = ya; j < yb; j++)
    {
      for (i = xa; i < xb; i++)
      {
        pel = ((unsigned int) (ffr[i] + ffr[ii + i] + 1)) >> 1;
        bfr[i] = ((unsigned int) (pel + bfr[i])) >> 1;
      }
      bfr += ii;
      ffr += ii;
    }
  } else
  {                             /* if (xhalf && yhalf) */
    for (j = ya; j < yb; j++)
    {
      for (i = xa; i < xb; i++)
      {
        pel = ((unsigned int) (ffr[i] + ffr[i + 1] + ffr[ii + i] + ffr[ii + i + 1] + 2)) >> 2;
        bfr[i] = ((unsigned int) (pel + bfr[i])) >> 1;
      }
      bfr += ii;
      ffr += ii;
    }
  }
  return;
}

int motion_decode (int vec, int pmv)
{
  if (vec > 31)
    vec -= 64;
  vec += pmv;
  if (!long_vectors)
  {
    if (vec > 31)
      vec -= 64;
    if (vec < -32)
      vec += 64;
  } else
  {
    if (pmv < -31 && vec < -63)
      vec += 64;
    if (pmv > 32 && vec > 63)
      vec -= 64;
  }
  return vec;
}


int find_pmv (int x, int y, int block, int comp)
{
  int p1, p2, p3;
  int xin1, xin2, xin3;
  int yin1, yin2, yin3;
  int vec1, vec2, vec3;
  int l8, o8, or8;

  x++;
  y++;

  l8 = (modemap[y][x - 1] == MODE_INTER4V ? 1 : 0);
  l8 = (modemap[y][x - 1] == MODE_INTER4V_Q ? 1 : l8);

  o8 = (modemap[y - 1][x] == MODE_INTER4V ? 1 : 0);
  o8 = (modemap[y - 1][x] == MODE_INTER4V_Q ? 1 : o8);

  or8 = (modemap[y - 1][x + 1] == MODE_INTER4V ? 1 : 0);
  or8 = (modemap[y - 1][x + 1] == MODE_INTER4V_Q ? 1 : or8);


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
      fprintf (stderr, "Illegal block number in find_pmv (getpic.c)\n");
      exit (1);
      break;
  }
  p1 = MV[comp][vec1][yin1][xin1];
  p2 = MV[comp][vec2][yin2][xin2];
  p3 = MV[comp][vec3][yin3][xin3];

  if (newgob && (block == 0 || block == 1 || block == 2))
    p2 = NO_VEC;

  if (p2 == NO_VEC)
  {
    p2 = p3 = p1;
  }
  return p1 + p2 + p3 - mmax (p1, mmax (p2, p3)) - mmin (p1, mmin (p2, p3));
}



void find_bidir_limits (int vec, int *start, int *stop, int nhv)
{
  /* limits taken from C loop in section G5 in H.263 */
  *start = mmax (0, (-vec + 1) / 2 - nhv * 8);
  *stop = mmin (7, 15 - (vec + 1) / 2 - nhv * 8);

  (*stop)++;                    /* I use < and not <= in the loop */
}

void find_bidir_chroma_limits (int vec, int *start, int *stop)
{

  /* limits taken from C loop in section G5 in H.263 */
  *start = mmax (0, (-vec + 1) / 2);
  *stop = mmin (7, 7 - (vec + 1) / 2);

  (*stop)++;                    /* I use < and not <= in the loop */
  return;
}

void make_edge_image (unsigned char *src, unsigned char *dst, 
                      int width, int height, int edge)
{
  int i, j;
  unsigned char *p1, *p2, *p3, *p4;
  unsigned char *o1, *o2, *o3, *o4;

  /* center image */
  p1 = dst;
  o1 = src;
  for (j = 0; j < height; j++)
  {
    for (i = 0; i < width; i++)
    {
      *(p1 + i) = *(o1 + i);
    }
    p1 += width + (edge << 1);
    o1 += width;
  }

  /* left and right edges */
  p1 = dst - 1;
  o1 = src;
  for (j = 0; j < height; j++)
  {
    for (i = 0; i < edge; i++)
    {
      *(p1 - i) = *o1;
      *(p1 + width + i + 1) = *(o1 + width - 1);
    }
    p1 += width + (edge << 1);
    o1 += width;
  }

  /* top and bottom edges */
  p1 = dst;
  p2 = dst + (width + (edge << 1)) * (height - 1);
  o1 = src;
  o2 = src + width * (height - 1);
  for (j = 0; j < edge; j++)
  {
    p1 = p1 - (width + (edge << 1));
    p2 = p2 + (width + (edge << 1));
    for (i = 0; i < width; i++)
    {
      *(p1 + i) = *(o1 + i);
      *(p2 + i) = *(o2 + i);
    }
  }

  /* corners */
  p1 = dst - (width + (edge << 1)) - 1;
  p2 = p1 + width + 1;
  p3 = dst + (width + (edge << 1)) * (height) - 1;
  p4 = p3 + width + 1;

  o1 = src;
  o2 = o1 + width - 1;
  o3 = src + width * (height - 1);
  o4 = o3 + width - 1;
  for (j = 0; j < edge; j++)
  {
    for (i = 0; i < edge; i++)
    {
      *(p1 - i) = *o1;
      *(p2 + i) = *o2;
      *(p3 - i) = *o3;
      *(p4 + i) = *o4;
    }
    p1 = p1 - (width + (edge << 1));
    p2 = p2 - (width + (edge << 1));
    p3 = p3 + width + (edge << 1);
    p4 = p4 + width + (edge << 1);
  }

}


void interpolate_image (unsigned char *in, unsigned char *out, int width, int height)
/* only used for displayed interpolated frames, not reconstructed ones */
{

  int x, xx, y, w2;

  unsigned char *pp, *ii;

  w2 = 2 * width;

  /* Horizontally */
  pp = out;
  ii = in;
  for (y = 0; y < height - 1; y++)
  {
    for (x = 0, xx = 0; x < width - 1; x++, xx += 2)
    {
      *(pp + xx) = *(ii + x);
      *(pp + xx + 1) = ((unsigned int) (*(ii + x) + *(ii + x + 1)) ) >> 1;
      *(pp + w2 + xx) = ((unsigned int) (*(ii + x) + *(ii + x + width))) >> 1;
      *(pp + w2 + xx + 1) = ((unsigned int) (*(ii + x) + *(ii + x + 1) +
                *(ii + x + width) + *(ii + x + width + 1))) >> 2;

    }
    *(pp + w2 - 2) = *(ii + width - 1);
    *(pp + w2 - 1) = *(ii + width - 1);
    *(pp + w2 + w2 - 2) = *(ii + width + width - 1);
    *(pp + w2 + w2 - 1) = *(ii + width + width - 1);
    pp += w2 << 1;
    ii += width;
  }

  /* last lines */
  for (x = 0, xx = 0; x < width - 1; x++, xx += 2)
  {
    *(pp + xx) = *(ii + x);
    *(pp + xx + 1) = ((unsigned int) (*(ii + x) + *(ii + x + 1) + 1 )) >> 1;
    *(pp + w2 + xx) = *(ii + x);
    *(pp + w2 + xx + 1) = ((unsigned int) (*(ii + x) + *(ii + x + 1) + 1 )) >> 1;
  }

  /* bottom right corner pels */
  *(pp + (width << 1) - 2) = *(ii + width - 1);
  *(pp + (width << 1) - 1) = *(ii + width - 1);
  *(pp + (width << 2) - 2) = *(ii + width - 1);
  *(pp + (width << 2) - 1) = *(ii + width - 1);

  return;
}




/**********************************************************************
 *
 *	Name:           Intra_AC_DC_Decode
 *	Description:    Intra Prediction in Advanced Intra Coding
 *
 *	Input:	        store_qcoeff, Intra_AC_DC, position of MB, store_QP
 *
 *	Side effects:   change qcoeff to predicted qcoeff
 *
 *      Return:
 *
 *	Date:970717     Guy Cote <guyc@ee.ubc.ca>
 *
 ***********************************************************************/


void Intra_AC_DC_Decode (short *store_qcoeff, int INTRA_AC_DC, int MBA, int xpos, int ypos, int comp, int newgob)
{

  int A[8], B[8];
  int i, j, tempDC;
  short *Rec_C;
  short *rcoeff;


  Rec_C = ld->block[comp];

  if (xpos == 0 && ypos == 0)
  {                             /* top left corner */
    (comp == 2 || comp == 3) ? fill_A (A, store_qcoeff, xpos, ypos, comp - 2) : fill_null (A);
    (comp == 1 || comp == 3) ? fill_B (B, store_qcoeff, xpos, ypos, comp - 1) : fill_null (B);
  } else
  {                             /* left border of picture */
    if (xpos == 0)
    {                           /* left edge of the picture */
      (comp == 2 || comp == 3) ? fill_A (A, store_qcoeff, xpos, ypos, comp - 2) :
        ((comp == 0 || comp == 1) && !(newgob)) ? fill_A (A, store_qcoeff, xpos, ypos - 1, comp + 2) :
        ((comp == 4 || comp == 5) && !(newgob)) ? fill_A (A, store_qcoeff, xpos, ypos - 1, comp) : fill_null (A);
      (comp == 1 || comp == 3) ? fill_B (B, store_qcoeff, xpos, ypos, comp - 1) : fill_null (B);
    } else
    {
      if (ypos == 0)
      {                         /* top border of picture */
        (comp == 2 || comp == 3) ? fill_A (A, store_qcoeff, xpos, ypos, comp - 2) : fill_null (A);
        (comp == 4 || comp == 5) ? fill_B (B, store_qcoeff, xpos - 1, ypos, comp) :
          (comp == 1 || comp == 3) ? fill_B (B, store_qcoeff, xpos, ypos, comp - 1) :
          fill_B (B, store_qcoeff, xpos - 1, ypos, comp + 1);
      } else
      {                         /* anywhere else in the picture, do not
                                 * cross GOB boundary */
        (comp == 2 || comp == 3) ? fill_A (A, store_qcoeff, xpos, ypos, comp - 2) :
          ((comp == 0 || comp == 1) && !(newgob)) ? fill_A (A, store_qcoeff, xpos, ypos - 1, comp + 2) :
          ((comp == 4 || comp == 5) && !(newgob)) ? fill_A (A, store_qcoeff, xpos, ypos - 1, comp) : fill_null (A);

        (comp == 4 || comp == 5) ? fill_B (B, store_qcoeff, xpos - 1, ypos, comp) :
          (comp == 1 || comp == 3) ? fill_B (B, store_qcoeff, xpos, ypos, comp - 1) :
          fill_B (B, store_qcoeff, xpos - 1, ypos, comp + 1);
      }
    }
  }


  /* replace the qcoeff with the predicted values pcoeff */
  switch (INTRA_AC_DC)
  {
    case INTRA_MODE_DC:

      tempDC = Rec_C[0] + ((A[0] == 1024 && B[0] == 1024) ? 1024 :
                           (A[0] == 1024) ? B[0] :
                           (B[0] == 1024) ? A[0] : (A[0] + B[0]) / 2);
      for (i = 0; i < 8; i++)
        for (j = 0; j < 8; j++)
          Rec_C[i * 8 + j] = clipAC (Rec_C[i * 8 + j]);
      Rec_C[0] = oddifyclipDC (tempDC);
      break;
    case INTRA_MODE_VERT_AC:
      tempDC = Rec_C[0] + A[0];
      for (i = 1; i < 8; i++)
      {
        rcoeff = &Rec_C[i];
        *rcoeff = clipAC (Rec_C[i] + A[i]);
      }
      for (i = 1; i < 8; i++)
        for (j = 0; j < 8; j++)
          Rec_C[i * 8 + j] = clipAC (Rec_C[i * 8 + j]);
      Rec_C[0] = oddifyclipDC (tempDC);
      break;
    case INTRA_MODE_HORI_AC:
      tempDC = Rec_C[0] + B[0];
      for (i = 1; i < 8; i++)
        Rec_C[i * 8] = clipAC (Rec_C[i * 8] + B[i]);
      for (i = 0; i < 8; i++)
        for (j = 1; j < 8; j++)
          Rec_C[i * 8 + j] = clipAC (Rec_C[i * 8 + j]);
      Rec_C[0] = oddifyclipDC (tempDC);
      break;
    default:
      printf ("Error in Prediction in Advanced Intra Coding\n");
      exit (-1);
      break;
  }


  return;
}

/**********************************************************************
 *
 *	Name:           fill_null, fill_A, fill_B, oddifyclipDC, clipAC
 *                      and clipDC
 *	Description:    Fill values in predictor coefficients
 *                      Functions used in advanced intra coding mode
 *
 *	Input:	        predictor qcoefficients
 *	Side effects:
 *
 *      Return:
 *
 *	Date:970717     Guy Cote <guyc@ee.ubc.ca>
 *
 ***********************************************************************/


void fill_null (int pred[])
{
  int j;

  pred[0] = 1024;
  for (j = 1; j < 8; j++)
  {
    pred[j] = 0;
  }
}

void fill_A (int pred[], short *store_qcoeff, int xpos, int ypos, int block)
{
  /* Fill first row of block at MB xpos, ypos, in pred[] */
  int j;
  for (j = 0; j < 8; j++)
  {
    pred[j] = *(store_qcoeff + (ypos * mb_width + xpos) * 384 + block * 64 + j);
  }
}

void fill_B (int pred[], short *store_qcoeff, int xpos, int ypos, int block)
{
  /* Fill first column of block at MB xpos, ypos, in pred[][i] */
  int j;
  for (j = 0; j < 8; j++)
  {
    pred[j] = *(store_qcoeff + (ypos * mb_width + xpos) * 384 + block * 64 + j * 8);
  }
}

int oddifyclipDC (int x)
{

  int result;


  (x % 2) ? (result = clipDC (x)) : (result = clipDC (x + 1));
  return result;
}
int clipAC (int x)
{
  int clipped;

  if (x > 2047)
    clipped = 2047;
  else if (x < -2048)
    clipped = -2048;
  else
    clipped = x;
  return clipped;

}
int clipDC (int x)
{
  int clipped;
  if (x > 2047)
    clipped = 2047;
  else if (x < 0)
    clipped = 0;
  else
    clipped = x;
  return clipped;

}


/**********************************************************************
 *
 *	Name:		EdgeFilter
 *	Description:    performs in the loop edge-filtering on
 *                      reconstructed frames
 *
 *	Input:	        pointers to reconstructed frame and difference
 *                      image
 *	Returns:
 *	Side effects:   since neither the algorithm nor the routines
 *                      have been optimized for speed, the use of the
 *                      edge-filter slows down decoding speed
 *
 *	Date: 951129 	Author: Gisle.Bjontegaard@fou.telenor.no
 *                        Karl.Lillevold@nta.no
 *  Date: 970820  Author: guyc@ee.ubc.ca
 *                        modified to implement annex J of H.263+
 *
 ***********************************************************************/


void edge_filter (unsigned char *lum, unsigned char *Cb, unsigned char *Cr,
                   int width, int height)
{

  /* Luma */
  horiz_edge_filter (lum, width, height, 0);
  vert_edge_filter (lum, width, height, 0);

  /* Chroma */
  horiz_edge_filter (Cb, width / 2, height / 2, 1);
  vert_edge_filter (Cb, width / 2, height / 2, 1);
  horiz_edge_filter (Cr, width / 2, height / 2, 1);
  vert_edge_filter (Cr, width / 2, height / 2, 1);

  /* that's it */
  return;
}


/***********************************************************************/


void horiz_edge_filter (unsigned char *rec, int width, int height, int chr)
{
  int i, j;
  int delta, d1, d2;
  int mbc, mbr, do_filter;
  int QP;
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
        if (pb_frame)
        {
          QP = coded_map[mbr + 1][mbc + 1] ?
            mmax (1, mmin (31, bquant_tab[bquant] * quant_map[mbr + 1][mbc + 1] / 4)) :
            mmax (1, mmin (31, bquant_tab[bquant] * quant_map[mbr_above + 1][mbc + 1] / 4));
        } else
          QP = coded_map[mbr + 1][mbc + 1] ? quant_map[mbr + 1][mbc + 1] : quant_map[mbr_above + 1][mbc + 1];
        if (chr && modified_quantization_mode) 
        {
          QP = MQ_chroma_QP_table[QP];
        } 

        delta = (int) (((int) (*(rec + i + (j - 2) * width)) +
                        (int) (*(rec + i + (j - 1) * width) * (-4)) +
                        (int) (*(rec + i + (j) * width) * (4)) +
                        (int) (*(rec + i + (j + 1) * width) * (-1))) / 8.0);

        d1 = sign (delta) * mmax (0, abs (delta) - mmax (0, 2 * (abs (delta) - STRENGTH[QP - 1])));

        d2 = mmin (abs (d1 / 2), mmax (-abs (d1 / 2), (int) (((*(rec + i + (j - 2) * width) -
                                   *(rec + i + (j + 1) * width))) / 4)));

        *(rec + i + (j + 1) * width) += d2; /* D */
        *(rec + i + (j) * width) = mmin (255, mmax (0, (int) (*(rec + i + (j) * width)) - d1)); /* C */
        *(rec + i + (j - 1) * width) = mmin (255, mmax (0, (int) (*(rec + i + (j - 1) * width)) + d1)); /* B */
        *(rec + i + (j - 2) * width) -= d2; /* A */
      }
    }
  }
  return;
}

void vert_edge_filter (unsigned char *rec, int width, int height, int chr)
{
  int i, j;
  int delta, d1, d2;
  int mbc, mbr;
  int do_filter;
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
      } 
      else
      {
        mbr = j >> 3;
        mbc = i >> 3;
        mbc_left = mbc - 1;
      }
      do_filter = coded_map[mbr + 1][mbc + 1] || coded_map[mbr + 1][mbc_left + 1];

      if (do_filter)
      {
        if (pb_frame)
        {
          QP = coded_map[mbr + 1][mbc + 1] ?
            mmax (1, mmin (31, bquant_tab[bquant] * quant_map[mbr + 1][mbc + 1] / 4)) :
            mmax (1, mmin (31, bquant_tab[bquant] * quant_map[mbr + 1][mbc_left + 1] / 4));
        } 
        else
          QP = coded_map[mbr + 1][mbc + 1] ? 
               quant_map[mbr + 1][mbc + 1] : quant_map[mbr + 1][mbc_left + 1];
        if (chr && modified_quantization_mode) 
        {
          QP = MQ_chroma_QP_table[QP];
        } 

        delta = (int) (((int) (*(rec + i - 2 + j * width)) +
                        (int) (*(rec + i - 1 + j * width) * (-4)) +
                        (int) (*(rec + i + j * width) * (4)) +
                        (int) (*(rec + i + 1 + j * width) * (-1))) / 8.0);

        d1 = sign (delta) * mmax (0, abs (delta) - 
                            mmax (0, 2 * (abs (delta) - STRENGTH[QP - 1])));

        d2 = mmin (abs (d1 / 2), mmax (-abs (d1 / 2), 
                   (int) ((*(rec + i - 2 + j * width) -
                           *(rec + i + 1 + j * width)) / 4)));

        *(rec + i + 1 + j * width) += d2; /* D */
        *(rec + i + j * width) = mmin (255, mmax (0, (int) (*(rec + i + j * width)) - d1)); /* C */
        *(rec + i - 1 + j * width) = mmin (255, mmax (0, (int) (*(rec + i - 1 + j * width)) + d1)); /* B */
        *(rec + i - 2 + j * width) -= d2; /* A */
      }
    }
  }
  return;
}

/**********************************************************************
 *
 *	Name:		PostFilter
 *	Description:    performs in the loop edge-filtering on
 *                      reconstructed frames
 *
 *	Input:	        pointers to reconstructed frame and difference
 *                      image
 *	Returns:
 *	Side effects:   since neither the algorithm nor the routines
 *                      have been optimized for speed, the use of the
 *                      edge-filter slows down decoding speed
 *
 *
 *      Date: 971004    Author: guyc@ee.ubc.ca
 *
 *
 ***********************************************************************/


void PostFilter (unsigned char *lum, unsigned char *Cb, unsigned char *Cr,
                  int width, int height)
{

  /* Luma */
  horiz_post_filter (lum, width, height, 0);
  vert_post_filter (lum, width, height, 0);

  /* Chroma */
  horiz_post_filter (Cb, width / 2, height / 2, 1);
  vert_post_filter (Cb, width / 2, height / 2, 1);
  horiz_post_filter (Cr, width / 2, height / 2, 1);
  vert_post_filter (Cr, width / 2, height / 2, 1);

  /* that's it */
  return;
}


/***********************************************************************/


void horiz_post_filter (unsigned char *rec, int width, int height, int chr)
{
  int i, j;
  int delta, d1;
  int mbc, mbr;
  int QP;
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


      if (pb_frame)
      {
        QP = coded_map[mbr + 1][mbc + 1] ?
          mmax (1, mmin (31, bquant_tab[bquant] * quant_map[mbr + 1][mbc + 1] / 4)) :
          mmax (1, mmin (31, bquant_tab[bquant] * quant_map[mbr_above + 1][mbc + 1] / 4));
      } else
        QP = coded_map[mbr + 1][mbc + 1] ? 
             quant_map[mbr + 1][mbc + 1] : quant_map[mbr_above + 1][mbc + 1];

      delta = (int) (((int) (*(rec + i + (j - 3) * width)) +
                      (int) (*(rec + i + (j - 2) * width)) +
                      (int) (*(rec + i + (j - 1) * width)) +
                      (int) (*(rec + i + (j) * width) * (-6)) +
                      (int) (*(rec + i + (j + 1) * width)) +
                      (int) (*(rec + i + (j + 2) * width)) +
                      (int) (*(rec + i + (j + 3) * width))) / 8.0);

      d1 = sign (delta) * mmax (0, abs (delta) - mmax (0, 2 * (abs (delta) - STRENGTH1[QP - 1])));

      /* Filter D */
      *(rec + i + (j) * width) += d1;
    }
  }
  return;
}

void vert_post_filter (unsigned char *rec, int width, int height, int chr)
{
  int i, j;
  int delta, d1;
  int mbc, mbr;
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

      if (pb_frame)
      {
        QP = coded_map[mbr + 1][mbc + 1] ?
          mmax (1, mmin (31, bquant_tab[bquant] * quant_map[mbr + 1][mbc + 1] / 4)) :
          mmax (1, mmin (31, bquant_tab[bquant] * quant_map[mbr + 1][mbc_left + 1] / 4));
      } else
        QP = coded_map[mbr + 1][mbc + 1] ? 
             quant_map[mbr + 1][mbc + 1] : quant_map[mbr + 1][mbc_left + 1];

      delta = (int) (((int) (*(rec + i - 3 + j * width)) +
                      (int) (*(rec + i - 2 + j * width)) +
                      (int) (*(rec + i - 1 + j * width)) +
                      (int) (*(rec + i + j * width) * (-6)) +
                      (int) (*(rec + i + 1 + j * width)) +
                      (int) (*(rec + i + 2 + j * width)) +
                      (int) (*(rec + i + 3 + j * width))) / 8.0);

      d1 = sign (delta) * mmax (0, abs (delta) - mmax (0, 2 * (abs (delta) - STRENGTH2[QP - 1])));

      /* Post Filter D */
      *(rec + i + j * width) += d1;
    }
  }
  return;
}

/**********************************************************************
 *
 *	Name:		      init_enhancement_layer
 *	Description:  intializes an enhancement layer when the first picture header
 *                in that layer is decoded
 *	Input:	      layer number
 *	Returns:      
 *	Side effects: allocates memory for previous and current enhancement 
 *                layer picture as well as framed enhancement layer picture, 
 *                and tmp forward and upward pictures (for bi-dir prediction),
 *                Y, Cb, and Cr components.
 *                
 *  Date: 971102  Author: mikeg@ee.ubc.ca
 *
 *
 ***********************************************************************/
static void init_enhancement_layer (int layer)
{
  int cc, size;

  blk_cnt = 6;

  for (cc = 0; cc < 3; cc++)
  {
    if (cc == 0)
      size = coded_picture_width * coded_picture_height;
    else
      size = chrom_width * chrom_height;

    /* Used for bidirectional and direct prediction mode for true B
     * pictures, one for forward and one for backward. */
    if (!(prev_enhancement_frame[layer][cc] = (unsigned char *) malloc (size)))
      error ("malloc failed\n");
    
    if (!(current_enhancement_frame[layer][cc] = (unsigned char *) malloc (size)))
      error ("malloc failed\n");

    if (!(tmp_enhance_fwd[layer][cc] = (unsigned char *) malloc (size)))
      error ("malloc failed\n");

    if (!(tmp_enhance_up[layer][cc] = (unsigned char *) malloc (size)))
      error ("malloc failed\n");

  }

  for (cc = 0; cc < 3; cc++)
  {
    if (cc == 0)
    {
      size = (coded_picture_width + 64) * (coded_picture_height + 64);

      /* Stores framed version of previous inter-picture, luminance. */
      if (!(enhance_edgeframeorig[layer][cc] = (unsigned char *) malloc (size)))
        error ("malloc failed\n");
      enhance_edgeframe[layer][cc] = enhance_edgeframeorig[layer][cc] + (coded_picture_width + 64) * 32 + 32;
    } 
    else
    {
      size = (chrom_width + 32) * (chrom_height + 32);

      /* Stores framed version of previous inter-picture, chrominance. */
      if (!(enhance_edgeframeorig[layer][cc] = (unsigned char *) malloc (size)))
        error ("malloc failed\n");
      enhance_edgeframe[layer][cc] = enhance_edgeframeorig[layer][cc] + (chrom_width + 32) * 16 + 16;
    }
  }

#ifdef WINDOWS
  if (outtype == T_WIN)
  {
    initDisplay (coded_picture_width, coded_picture_height);
  }
#endif

  sprintf (enhance_recon_file_name[enhancement_layer_num-2], "enhanced_%d.raw", 
           (enhancement_layer_num-1));
  if ((enhance_recon_file_ptr[enhancement_layer_num-2] = fopen (
       enhance_recon_file_name[enhancement_layer_num-2], "wb")) == NULL)
  {
    printf ("Unable to open enhancement layer reconstruction file");
    exit (-1);
  }
}

/**********************************************************************
 *
 *  Name:         UpsampleReferenceLayerPicture
 *  Description:  calls UpsampleComponent for each picture component 
 *                (Y,Cb, or Cr)
 *  Input:	      
 *  Returns:      
 *  Side effects: allocates memory for spatially scaled enhancement 
 *                layer picture Y, Cb, and Cr components.
 *
 *  Date: 971102  Author: mikeg@ee.ubc.ca
 *
 ***********************************************************************/
void UpsampleReferenceLayerPicture(void)
{

  int cc, size, x, y;
  
  for (cc = 0; cc < 3; cc++)
  {
    if (cc == 0)
    {
      size = coded_picture_width * coded_picture_height;
      x = ref_coded_picture_width;
      y = ref_coded_picture_height;
    }
    else
    {
      size = chrom_width * chrom_height;
      x = ref_chrom_width;
      y = ref_chrom_height;
    }

    if (!(upsampled_reference_frame[cc] = (unsigned char *) malloc (size)))
      error ("malloc failed\n");

    UpsampleComponent(upsampled_reference_frame[cc], current_frame[cc], x, y);
  }
}

/**********************************************************************
 *
 *  Name:         UpsampleComponent
 *  Description:  interpolates in horiz only, vert only or both 
 *                vert and horiz depending on spatial scalability
 *                option in use
 *  Input:        pointers to  reference layer and enhancement layer
 *                picture components (Y,Cb,or Cr) and dimensions
 *                of reference layer picture
 *  Returns:      
 *  Side effects: 
 *
 *  Date: 971102  Author: mikeg@ee.ubc.ca
 *
 ***********************************************************************/
void UpsampleComponent (unsigned char *enhanced, unsigned char *base,
                        int horiz, int vert)
{
  int i,j;
  unsigned char *base_next, *enhanced_next, *enhanced_origin;

  enhanced_origin = enhanced;

  switch (scalability_mode)
  {
    case SPATIAL_SCALABILITY_H:

      /* Rows */
      for( j=0; j<vert; j++ )
      {
        /* First column of rows */
        *enhanced++ = *base;  
        for( i=1; i<horiz; i++ )
        {
          *enhanced++ = (3* *base +    *(base+1) + 2) >> 2;
          *enhanced++ = (   *base + 3* *(base+1) + 2) >> 2;
          base++;
        }
        /* Last column of rows */
        *enhanced++ = *base++;
      }

      break;

    case SPATIAL_SCALABILITY_V:
      
      /* First row */
      for( i=0 ; i<horiz; i++ )      
      {
        *enhanced++ = *base++;
      }
      
      enhanced_next = enhanced + horiz;
      base          = base - horiz;
      base_next     = base + horiz;

      /* Rows */
      for( j=0; j<vert-1; j++ )
      {
        
        for( i=0; i<horiz; i++ )
        {
          *enhanced++      = (3* *base +    *(base_next) + 2) >> 2;  
          *enhanced_next++ = (   *base + 3* *(base_next) + 2) >> 2;  
          base++;
          base_next++;
        }
        enhanced      = enhanced + horiz;
        enhanced_next = enhanced + horiz;
      }

      /* Last row */
      for( i=0 ; i<horiz; i++ )      
      {
        *enhanced++ = *base++;
      }
      
      break;

    case SPATIAL_SCALABILITY_HV:

      /* Top left corner pel */
      *enhanced++  = *base;   
      /* First row */
      for( i=1 ; i<horiz; i++ )      
      {
        *enhanced++ = (3* *base +    *(base+1) + 2) >> 2;
        *enhanced++ = (   *base + 3* *(base+1) + 2) >> 2;
        base++ ;
      }
      /* Top right corner pel */
      *enhanced++ = *base++;   

      enhanced_next = enhanced + (horiz<<1);
      base          = base - horiz;
      base_next     = base + horiz;
      
      /* Rows */
      for( j=0; j<vert-1; j++ )
      {
        /* First column of rows */
        *enhanced++       = (3* *base +    *(base_next) + 2) >> 2;  
        *enhanced_next++  = (   *base + 3* *(base_next) + 2) >> 2;
        for( i=1; i<horiz; i++ )
        {
          *enhanced++      = (9* *base + 3* *(base+1) + 3* *base_next +    
                                 *(base_next+1) + 8) >> 4;
          *enhanced++      = (3* *base + 9* *(base+1) +    *base_next + 
                              3* *(base_next+1) + 8) >> 4;
          *enhanced_next++ = (3* *base +    *(base+1) + 9* *base_next + 
                              3* *(base_next+1) + 8) >> 4;
          *enhanced_next++ = (   *base + 3* *(base+1) + 3* *base_next + 
                              9* *(base_next+1) + 8) >> 4;
          base++;
          base_next++;
        }
        /* Last column of rows */
        *enhanced++      = (3* *base +    *(base_next) + 2) >> 2;  
        *enhanced_next++ = (   *base + 3* *(base_next) + 2) >> 2;  

        enhanced      = enhanced + (horiz<<1);
        enhanced_next = enhanced + (horiz<<1);
        base++;
        base_next++;
      }
      
      /* Bottom left corner pel */
      *enhanced++  = *base;   
      /* Last row */
      for( i=1; i<horiz; i++ )                                
      {
        *enhanced++ = (3* *base +    *(base+1) + 2) >> 2 ;
        *enhanced++ = (   *base + 3* *(base+1) + 2) >> 2 ;
        base++ ;
      }
      /* Bottom right corner pel */
      *enhanced = *base;

      break;

    default:

      break;
  }
}


/**********************************************************************
 *
 *  Name:	      DisplayPicture
 *  Description:  manages a one frame buffer for re-ordering frames prior 
 *                to displaying or writing to a file.
 *  Input:	      frame number
 *  Returns:      
 *  Side effects: 
 *
 *  Date: 971102  Author: mikeg@ee.ubc.ca
 *
 ***********************************************************************/
void PictureDisplay(int *framenum)
{ 
  unsigned char *frame_to_display[3];
  static int     last_frame_disposable = 0, display_buffer_empty = 1;
  static int     buffered_frame_temp_ref = 0, display_framenum = 0;
  int            cc, size, update_buffered_frame;

  if (display_buffer_empty)
  {
    /* Store current (initial) frame. */
    update_buffered_frame = ON;
  } 
  else
  {
    if ( (temp_ref < buffered_frame_temp_ref) && 
         ( (buffered_frame_temp_ref - temp_ref) < 100) )
    {
      /* Do not store current frame. */
      update_buffered_frame = OFF;

      /* Display current frame. */
      display_framenum = *framenum;

      frame_to_display[0] = current_frame[0];
      frame_to_display[1] = current_frame[1];
      frame_to_display[2] = current_frame[2];
    } 
    else
    {
      /* Diplay frame in storage. */
      display_framenum = buffered_framenum;

      frame_to_display[0] = buffered_frame[0];
      frame_to_display[1] = buffered_frame[1];
      frame_to_display[2] = buffered_frame[2];

      /* Store current frame. */
      update_buffered_frame = ON;
    }
  }

  if (!display_buffer_empty)
  {
    /* Post Filter */
    if (post_filter)
      PostFilter (frame_to_display[0], frame_to_display[1], frame_to_display[2],
              coded_picture_width, coded_picture_height);

    if (expand && outtype == T_X11)
    {
      /* display this image and keep frame already in storage where it is. */
      interpolate_image (frame_to_display[0], exnewframe[0],
                         coded_picture_width, coded_picture_height);
      interpolate_image (frame_to_display[1], exnewframe[1], chrom_width, chrom_height);
      interpolate_image (frame_to_display[2], exnewframe[2], chrom_width, chrom_height);

      storeframe (exnewframe, *framenum);
      if (save_frames)
      {
        save_frame (exnewframe, *framenum, recon_file_ptr);
      }
      if (pb_frame)
      {
#ifdef USE_TIME
        if (framerate > 0)
          doframerate (1);
#endif

        (*framenum)++;
        interpolate_image (bframe[0], exnewframe[0], coded_picture_width, coded_picture_height);
        interpolate_image (bframe[1], exnewframe[1], chrom_width, chrom_height);
        interpolate_image (bframe[2], exnewframe[2], chrom_width, chrom_height);
        storeframe (exnewframe, *framenum);
        if (save_frames)
        {
          save_frame (exnewframe, *framenum, recon_file_ptr);
        }
      }
    }  
    else
    {
      storeframe (frame_to_display, *framenum);
      if (save_frames)
      {
        save_frame (frame_to_display, *framenum, recon_file_ptr);
      }
      if (pb_frame)
      {
#ifdef USE_TIME
        if (framerate > 0)
          doframerate (1);
#endif

        (*framenum)++;
        storeframe (bframe, *framenum);

        if (save_frames)
        {
          save_frame (bframe, *framenum, recon_file_ptr);
        }
      }
    }
  }

  /* After first decoded frame, the buffer is never again empty, until
   * flushed. */
  if (display_buffer_empty)
  {
    display_buffer_empty = 0;
  }
  if (update_buffered_frame)
  {
    buffered_frame_temp_ref = temp_ref;
    buffered_framenum = *framenum;

    for (cc = 0; cc < 3; cc++)
    {
      if (cc == 0)
        size = coded_picture_width * coded_picture_height;
      else
        size = chrom_width * chrom_height;

      memcpy (buffered_frame[cc], current_frame[cc], size);
    }
  }
}


/* ------------------------------------------------------------------------ */
/* reference picture selection                                              */

/*
 * I'm using a ring buffer to hold the last RING_SIZE pictures.
 *
 * get_reference_picture() gets the requested prediction picture from
 * the ring buffer.
 * 
 * store_picture() stores the picture into the ring buffer.
 *
 */

#define RING_SIZE 8

int   ring_ptr;               /* pointer - to last picture */
void *ring_lum[RING_SIZE];    /* luminance */
void *ring_c1[RING_SIZE];     /* chominance */
void *ring_c2[RING_SIZE];
int   ring_tr[RING_SIZE];     /* TR values for the pictures */
int   ring_qu[RING_SIZE];     /* picture quality */

int get_reference_picture(void)
{
    int i,d;
    
    if (!TRPI) {
	fprintf(stderr,"getrefpic: TR=%d - I-frame\n",temp_ref);
	return 0;
    }
    for (i = ring_ptr; i != (ring_ptr+1)%RING_SIZE;
	 i = (i+RING_SIZE-1)%RING_SIZE) {
	d =  temporal_reference_for_prediction - ring_tr[i];
	if (d > 128)  d -= 256;
	if (d < -128) d += 256;
	if (d >= 0)  break;
    }
    if (ring_tr[ring_ptr] == temp_ref &&
	ring_qu[i]+d >= ring_qu[ring_ptr]) {
	return -1; /* don't decode this - second frame for a sync point */
    }
    if (ring_lum[i]) {
	memcpy(next_I_P_frame[0],ring_lum[i],
	       coded_picture_height*coded_picture_width);
	memcpy(next_I_P_frame[1],ring_c1[i],
	       coded_picture_height*coded_picture_width/4);
	memcpy(next_I_P_frame[2],ring_c2[i],
	       coded_picture_height*coded_picture_width/4);
    }
    if (d)
	fprintf(stderr,"getrefpic: TR=%d, TRP=%d, have %d, q=%d+%d\n",
		temp_ref,temporal_reference_for_prediction,
		ring_tr[i],ring_qu[i],d);
    return ring_qu[i]+d;
}

void store_picture(int quality)
{
    ring_ptr = (ring_ptr+1)%RING_SIZE;
    if (!ring_lum[ring_ptr]) {
	ring_lum[ring_ptr] = malloc(coded_picture_height*coded_picture_width);
	ring_c1[ring_ptr] = malloc(coded_picture_height*coded_picture_width/4);
	ring_c2[ring_ptr] = malloc(coded_picture_height*coded_picture_width/4);
    }
    memcpy(ring_lum[ring_ptr],current_frame[0],
	   coded_picture_height*coded_picture_width);
    memcpy(ring_c1[ring_ptr],current_frame[1],
	   coded_picture_height*coded_picture_width/4);
    memcpy(ring_c2[ring_ptr],current_frame[2],
	   coded_picture_height*coded_picture_width/4);
    ring_tr[ring_ptr] = temp_ref;
    ring_qu[ring_ptr] = quality;
}
