/************************************************************************
 *
 *  coder.c, main coding engine of tmn (TMN encoder)
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

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any
 * license fee or royalty on an "as is" basis. The University of
 * British Columbia disclaims any and all warranties, whether
 * express, implied, or statuary, including any implied warranties
 * or merchantability or of fitness for a particular purpose.  In no
 * event shall the copyright-holder be liable for any incidental,
 * punitive, or consequential damages of any kind whatsoever arising
 * from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs
 * and user's customers, employees, agents, transferees, successors,
 * and assigns.
 *
 * The University of British Columbia does not represent or warrant
 * that the programs furnished hereunder are free of infringement of
 * any third-party patents.
 *
 * Commercial implementations of H.263, including shareware, are
 * subject to royalty fees to patent holders.  Many of these patents
 * are general enough such that they are unavoidable regardless of
 * implementation design.
 *
*/


/*****************************************************************
 *
 * Modified by Pat Mulroy, BT Labs to run syntax based arithmetic
 * coding.  SAC option, H.263 (Annex E).
 *
 *****************************************************************/


#include"sim.h"

//SV-XXX: defined UNUSED_() macro for unused variables
#ifndef UNUSED_
#define UNUSED_(x) (x) = (x)
#endif

static MotionVector *True_B_Direct_Mode_MV[MBR][MBC];

/**********************************************************************
 *
 *	Name:           CodeOneOrTwo
 *	Description:	code one image normally or two images
 *                      as a PB-frame (CodeTwoPB and CodeOnePred merged)
 *	
 *	Input:          pointer to image, prev_image, prev_recon, Q
 *        
 *	Returns:	pointer to reconstructed image
 *	Side effects:	memory is allocated to recon image
 *
 *	Date: 950221	Author:	<klillevo@mailbox.jf.intel.com>
 *            970831    modified by mikeg@ee.ubc.ca
 *            970815    modified by guyc@ee.ubc.ca
 *
 ***********************************************************************/

void CodeOneOrTwo(PictImage *curr, PictImage *B_image, PictImage *prev, 
          PictImage *pr, int QP, int frameskip, Bits *bits,
          Pict *pic, PictImage *B_recon, PictImage *recon)
{
  unsigned char *prev_ipol, *pi;
  PictImage *prev_recon=NULL, *pr_edge = NULL;
  MotionVector *MV[7][MBR+1][MBC+2];
  MotionVector *B_f_MV[7][MBR+1][MBC+2];
  MotionVector ZERO = {0,0,0,0,0,0}; //SV-XXX: was only 5 zeros in set, but MotionVect struct has 6 elements
  MB_Structure *recon_data_P; 
  MB_Structure *recon_data_B=NULL; 
  MB_Structure *diff; 
  int *qcoeff_P, *rcoeff, *coeff;
  int *qcoeff_B=NULL;
  int Mode=MODE_INTER; //SV-XXX: initialised to MODE_INTER to shut up gcc4
  int CBP, CBPB=0;
  int bquant[] = {5,6,7,8};
  int QP_B;
  int newgob, advanced_temporarily_off = DEF_AIC_MODE;
  int i,j,k;

  /* buffer control vars */
  float QP_cumulative = (float)0.0;
  int abs_mb_num = 0, QuantChangePostponed = 0;
  int QP_new=0, QP_prev=0, dquant, QP_xmitted=QP; //SV-XXX: inited to 0

  /* Rate control variables */
  Bits *bits_prev = (Bits*)calloc(1,sizeof(Bits));

  MB_Structure *pred_P = (MB_Structure *)malloc(sizeof(MB_Structure));
  MB_Structure *pred_B = (MB_Structure *) malloc (sizeof (MB_Structure));
  
  int PB_pred_type;

  /* advanced intra coding variables */
  int *store_rcoeff=NULL, *store_coeff=NULL, *pcoeff=NULL; //SV-XXX: inited to NULL /* used to do prediction in advanced intra coding */
  int store_pb;
  
  //SV-XXX
  UNUSED_(prev);

  ZeroBits(bits);

  /* Currently, the MV info is stored in 7 MV structs per MB. MV[0]
   * stores the 16x16 MV. MV[1] through MV[4] store the 8x8 MVs. MV[6]
   * stores the PB delta MV or the forward predicted B MV. MV[5]
   * stores the backward predicted MV for true-B pictures. */ 

  InitializeMV(MV);
  InitializeMV(B_f_MV);

  /* Mark PMV's outside the frame. */
  for (i = 1; i < (sed_pels>>4)+1; i++) 
  {
    for (j = 0; j < 7; j++)
    {
      MarkVec(MV[j][0][i]); 
      MarkVec(B_f_MV[j][0][i]); 
    }
    MV[0][0][i]->Mode = MODE_INTRA;
    B_f_MV[0][0][i]->Mode = MODE_INTRA;
  }

  /* Zero out PMV's outside the frame */
  for (i = 0; i < (sed_lines>>4)+1; i++) 
  {
    for (j = 0; j < 7; j++) 
    {
      ZeroVec(MV[j][i][0]);
      ZeroVec(MV[j][i][(sed_pels>>4)+1]);
      ZeroVec(B_f_MV[j][i][0]);
      ZeroVec(B_f_MV[j][i][(sed_pels>>4)+1]);
    }

    MV[0][i][0]->Mode = MODE_INTRA;
    MV[0][i][(sed_pels>>4)+1]->Mode = MODE_INTRA;

    B_f_MV[0][i][0]->Mode = MODE_INTRA;
    B_f_MV[0][i][(sed_pels>>4)+1]->Mode = MODE_INTRA;
  }

  GenerateFrameAndInterpolatedImages(pr, pic, &prev_ipol, &prev_recon, &pi, &pr_edge);

  /* Integer and half pel motion estimation */
  MotionEstimatePicture( curr->lum,prev_recon->lum, NULL, prev_ipol,
                         NULL, pic->seek_dist, MV, pic->use_gobsync,
                         P_PICTURE_ESTIMATION);

  if (successive_B_frames)
  {
    StoreDirectModeVectors(MV, True_B_Direct_Mode_MV);
  }

  if (PCT_IPB == pic->picture_coding_type)
  {
    if (adv_pred)
    {
      /* Turn off advanced coding since there can be only 1 
       * motion vector for B frames of IPB. */
      advanced_temporarily_off = YES;
      overlapping_MC = OFF;
      adv_pred = OFF;
      use_4mv = OFF;
    }

    /* Forward motion Integer and half pel estimation for improved PB frames.
     * Motion estimation type is set to PB_PICTURE so that long vectors will 
     * not be used. */
    MotionEstimatePicture( B_image->lum,prev_recon->lum, NULL, prev_ipol,
                           NULL, pic->seek_dist, B_f_MV, pic->use_gobsync, 
                           PB_PICTURE_ESTIMATION);                          

    if (advanced_temporarily_off)
    {
      advanced_temporarily_off = NO;
      overlapping_MC = ON;
      adv_pred = ON;
      use_4mv = ON;
    }
  }

  switch (rate_control_method) 
  {
    case NO:
    case OFFLINE_RC:

      QP_new = QP_xmitted = QP_prev = QP; /* Copy the passed value of QP */
      break;

    case  TMN8_RC:
      
      /* Initialization routine for Frame Layer Rate Control */
      InitializeRateControlMB(curr, (float)pic->bit_rate, (pic->PB ? pic->target_frame_rate/2 : pic->target_frame_rate), MV);

      /* compute the first QP to be included in the picture header */
      QP_new = Compute_QP(0,0);
      QP_xmitted = QP_prev = QP_new; 
      break;

    case  TMN5_RC:

      /* Initialization routine for MB Layer Rate Control */
      QP_new = InitializeQuantizer(PCT_INTER, (float)pic->bit_rate, 
               (pic->PB ? pic->target_frame_rate/2 : pic->target_frame_rate),
               pic->QP_mean);
      QP_xmitted = QP_prev = QP_new;
      break;

    default:
      break;
  }

  dquant = 0; 

  for ( j = 0; j < sed_lines/MB_SIZE; j++) 
  {

    if (rate_control_method == TMN5_RC) 
    {
      /* QP updated at the beginning of each row for Frame layer rate control */
      AddBitsPicture(bits);
      QP_new =  UpdateQuantizer(abs_mb_num, pic->QP_mean, PCT_INTER, 
                (float)pic->bit_rate, sed_pels/MB_SIZE, sed_lines/MB_SIZE, 
                bits->total);
    }  

    newgob = 0;

    if (j == 0) 
    {
      if (advanced_intra_coding)
      {
         /* store the coeff for the frame */
         if ((store_coeff=(int *)calloc(384*(sed_pels/MB_SIZE)*(sed_lines/MB_SIZE), sizeof(int))) == 0) 
         {
            fprintf(stderr,"coder(): Couldn't allocate store_coeff.\n");
            exit(-1);
         }
         if ((store_rcoeff=(int *)calloc(384*(sed_pels/MB_SIZE)*(sed_lines/MB_SIZE), sizeof(int))) == 0) 
         {
            fprintf(stderr,"coder(): Couldn't allocate store_rcoeff.\n");
            exit(-1);
         }
      }

      pic->QUANT = QP_new;

      bits->header += CountBitsPicture(pic);

      QP_xmitted = QP_prev = QP_new;
    }
    else if (pic->use_gobsync && j%pic->use_gobsync == 0) 
    {
      bits->header += CountBitsSlice(j,QP_new); /* insert gob sync */
      QP_xmitted = QP_prev = QP_new;
      newgob = 1;
    }
 
    for ( i = 0; i < sed_pels/MB_SIZE; i++) 
    {     
      if (rate_control_method == TMN8_RC) 
      {
        /* Compute optimized quantizer for the current macroblock */
        QP_new = Compute_QP(i,j);
      }
      /* Update of dquant, check and correct its limit */
      if (PCT_B != pic->picture_coding_type)
      {
        
        dquant = QP_new - QP_prev;
        if ( dquant != 0 && MV[0][j+1][i+1]->Mode == MODE_INTER4V
	     && !EPTYPE) 
        {
          /* It is not possible to change the quantizer and at the same
           * time use 8x8 vectors, unless H.263+ PLUS PTYPE is used. 
           * Turning off 8x8 vectors is not
           * possible at this stage because the previous macroblock
           * encoded assumed this one should use 8x8 vectors. Therefore
           * the change of quantizer is postponed until the first MB
           * without 8x8 vectors */
      	   dquant = 0;
           QP_xmitted = QP_prev;
           QuantChangePostponed = 1;
        }
        else 
        {
          QP_xmitted = QP_new;
          QuantChangePostponed = 0;
        }

        /* Unless modified quantization mode is in use restrict the range of
         * dquant to [-2,+2]*/
        if (!modified_quantization) 
	{
      	  if (dquant > 2)  { dquant =  2; QP_xmitted = QP_prev + dquant;}
   	  if (dquant < -2) { dquant = -2; QP_xmitted = QP_prev + dquant;}
	}
        else 
        {
          /* Modified quantization mode is in use. */
#ifdef RESTRICTED_MQ
          /* restrict dquant so that when the modified quantization mode is on quant only uses 2 bits*/
          if (dquant != 0)
          {
            dquant= Get_restricted_MQ(dquant,QP_prev);
            QP_xmitted = QP_prev + dquant;
          }
#endif
          if (dquant!=0) 	
	  {
            if ((QP_prev >= 21 && dquant == -3) ||
                (QP_prev >= 11 && QP_prev <= 20 && dquant == -2) ||
                (QP_prev >= 2  && QP_prev <= 10 && dquant == -1) ||
                (QP_prev == 1  && dquant == 2)) 
	    {
   	      /* dquant will be coded into 2 bits*/
       	      pic->dquant_size = 2;
	      dquant = 2;
            }
            else 
            {
              if ((QP_prev == 31 && dquant == -5) ||
                  (QP_prev == 30 && dquant == 1) ||
                  (QP_prev == 29 && dquant == 2) ||
                  (QP_prev >= 21 && QP_prev <= 28 && dquant == 3) ||
                  (QP_prev >= 11 && QP_prev <= 20 && dquant == 2) ||
                  (QP_prev >= 1 && QP_prev <= 10 && dquant == 1))
	      {
		 /* dquant will be coded into 2 bits*/
	      	 pic->dquant_size = 2;
		 dquant = 3;
	      } 
	      else 
	      {
	        /* dquant will be coded intou 6 bits. */
		pic->dquant_size=6;
		/* instead of the difference between current quantizer and
		 * the previous quantizer,current quantizer is coded into dquant parameter*/
                dquant = QP_new;
              }
            }
          }
        }
	pic->DQUANT = dquant;

        /* modify mode if dquant != 0 (e.g. MODE_INTER -> MODE_INTER_Q  
         * or MODE_INTER4V -> MODE_INTER4V_Q) */
        Mode = ModifyMode(MV[0][j+1][i+1]->Mode,pic->DQUANT, EPTYPE);
        MV[0][j+1][i+1]->Mode = Mode;
      }

      pic->MB = i + j * (sed_pels/MB_SIZE);
      
      /* store the QP for every macroblock */
      quant_map[j+1][i+1] = QP_xmitted;

      if ((rcoeff = (int *)malloc(sizeof(int)*384)) == NULL) 
      {
        fprintf(stderr,"MB_Coder: Could not allocate space for rcoeff\n");
        exit(-1);
      }  

      if ( Mode == MODE_INTER || Mode == MODE_INTER_Q || 
          Mode == MODE_INTER4V || Mode==MODE_INTER4V_Q)
      {
        /* Predict P-MB */
	diff = Predict_P( curr,prev_recon,prev_ipol, pred_P,
                          i*MB_SIZE,j*MB_SIZE,MV,pic->PB);
      }
      else 
      {
        /* fill prediction array with 0s */
        memset (pred_P, 0, sizeof (MB_Structure));
        /* fill the difference array with the current macroblock pixels */
        diff = (MB_Structure *)malloc(sizeof(MB_Structure));
	FillLumBlock(i*MB_SIZE, j*MB_SIZE, curr, diff);
        FillChromBlock(i*MB_SIZE, j*MB_SIZE, curr, diff);
      }

      /* P or INTRA Macroblock */
      if ((qcoeff_P=(int *)malloc(sizeof(int)*384)) == 0) 
      {
        fprintf(stderr,"coder(): Couldn't allocate qcoeff_P.\n");
        exit(-1);
      }
      coeff = MB_Encode(diff);
      if (advanced_intra_coding) 
      {
        if (!(Mode == MODE_INTRA || Mode == MODE_INTRA_Q)) 
        {
          for (k=0;k<6;k++) 
          {    
            /* store default coeff if non-intra macroblock */
            store_coeff[(i + j * sed_pels/MB_SIZE) * 384 + k * 64] = 1024;
            store_rcoeff[(i + j * sed_pels/MB_SIZE) * 384 + k * 64] = 1024;
          }
          for (k=0;k<6;k++)
          {
            Quant_blk(coeff,qcoeff_P,QP_xmitted, Mode,k);
          }
          CBP = FindCBP(qcoeff_P, Mode, 64);
          if (CBP == 0 && (Mode == MODE_INTER || Mode == MODE_INTER_Q)) 
          {
            ZeroMBlock(diff);
          }
          else
          {  
            for (k=0;k<6;k++)
            {
              Quant_blk(coeff,qcoeff_P,QP_xmitted, Mode,k);
              Dequant(qcoeff_P, rcoeff, QP_xmitted, Mode,k);
            }
            MB_Decode(rcoeff, diff);
          }
        }
        else 
        {
          if ((pcoeff=(int *)malloc(sizeof(int)*384)) == 0) 
          {
            fprintf(stderr,"coder(): Couldn't allocate pcoeff.\n");
            exit(-1);
          }

          /* store the quantized DCT coefficients */
          memcpy( (void *) (store_coeff + (i + j*sed_pels/MB_SIZE)*384), (void *) coeff, sizeof(int) * 384);

          /* Do Intra mode prediction */
          pic->Intra_Mode = Choose_Intra_Mode(pcoeff, store_coeff, i, j, newgob);
          for (k=0;k<6;k++) 
          { 
            Intra_AC_DC_Encode(coeff, store_rcoeff, pic->Intra_Mode, i, j, newgob,k);
            Quant_blk(coeff,pcoeff,QP_xmitted,Mode,k);
            Dequant(pcoeff, rcoeff, QP_xmitted, Mode,k);
            sed_Intra_AC_DC_Decode(rcoeff, store_rcoeff, pic->Intra_Mode, i, j, newgob,k);
          }
          MB_Decode(rcoeff, diff);
          CBP = FindCBP(pcoeff,Mode,64);
        }    
      }
      else
      {  
        for (k=0;k<6;k++)
          Quant_blk(coeff,qcoeff_P,QP_xmitted, Mode,k);
        CBP = FindCBP(qcoeff_P, Mode, 64);
        if (CBP == 0 && (Mode == MODE_INTER || Mode == MODE_INTER_Q)) 
          ZeroMBlock(diff);
        else
        {
          for (k=0;k<6;k++)  
            Dequant(qcoeff_P, rcoeff, QP_xmitted, Mode,k); 
          MB_Decode(rcoeff, diff);
        }
      }

      recon_data_P = MB_Reconstruct (diff, pred_P);

      Clip(recon_data_P);
      free(diff);
      free(coeff);
        
      /* Predict B-MB using reconstructed P-MB and prev. recon. image */
      if (pic->PB) 
      {
        diff = Predict_B( B_image, prev_recon, prev_ipol,pred_B,i*MB_SIZE, j*MB_SIZE,
                          MV,B_f_MV, recon_data_P, frameskip, pic->TRB, pic->PB, 
                          &PB_pred_type);
        if (QP_xmitted == 0)
        {
          /* (QP = 0 means no quantization) */
          QP_B = 0;  
        }
        else 
        {
          QP_B = mmax(1,mmin(31,bquant[pic->BQUANT]*QP_xmitted/4));
        }
        if ((qcoeff_B=(int *)malloc(sizeof(int)*384)) == 0) 
        {
		      fprintf(stderr,"coder(): Couldn't allocate qcoeff_B.\n");
          exit(-1);
        }
        coeff = MB_Encode(diff);
        for (k=0;k<6;k++)
          Quant_blk(coeff,qcoeff_B,QP_B, MODE_INTER,k);
        CBPB = FindCBP(qcoeff_B, MODE_INTER, 64);
         
        if (CBPB)
        { 
          for (k=0;k<6;k++)
            Dequant(qcoeff_B, rcoeff, QP_B, MODE_INTER,k);
          MB_Decode(rcoeff, diff);
        }
        else
          ZeroMBlock(diff);

        recon_data_B = MB_Reconstruct(diff,pred_B);
        Clip(recon_data_B);

        /* decide MODB */ 
        if (pic->PB == IM_PB_FRAMES) 
        {
          if (CBPB)
          {
            if (PB_pred_type == FORWARD_PREDICTION)
              pic->MODB = PBMODE_CBPB_FRW_PRED;
            else if (PB_pred_type == BACKWARD_PREDICTION)
            pic->MODB = PBMODE_CBPB_BCKW_PRED;
            else
              pic->MODB = PBMODE_CBPB_BIDIR_PRED;
          } else
          {
            if (PB_pred_type == FORWARD_PREDICTION)
              pic->MODB = PBMODE_FRW_PRED;
             else if (PB_pred_type == BACKWARD_PREDICTION)
              pic->MODB = PBMODE_BCKW_PRED;
            else
              pic->MODB = PBMODE_BIDIR_PRED;
          }
          if (PB_pred_type == FORWARD_PREDICTION)
          {
            /* Store the forward predicted MV instead of PB delta. */
            MV[6][j + 1][i + 1]->x = B_f_MV[0][j + 1][i + 1]->x;
            MV[6][j + 1][i + 1]->y = B_f_MV[0][j + 1][i + 1]->y;
            MV[6][j + 1][i + 1]->x_half = B_f_MV[0][j + 1][i + 1]->x_half;
            MV[6][j + 1][i + 1]->y_half = B_f_MV[0][j + 1][i + 1]->y_half;
          }
          if (PB_pred_type == BACKWARD_PREDICTION)
          {
            MV[6][j + 1][i + 1]->x = 0;
            MV[6][j + 1][i + 1]->y = 0;
            MV[6][j + 1][i + 1]->x_half = 0;
            MV[6][j + 1][i + 1]->y_half = 0;
          }
        }
        else 
        {
          if (CBPB) 
          {
            pic->MODB = PBMODE_CBPB_MVDB;
          }
          else  
          {
            if (MV[6][j+1][i+1]->x == 0 && MV[6][j+1][i+1]->y == 0)
              pic->MODB = PBMODE_NORMAL;
            else
              pic->MODB = PBMODE_MVDB;
          }
        }
        free(diff);
        free(coeff);
        
      }
      else
        ZeroVec(MV[6][j+1][i+1]); /* Zero out PB deltas */

      if ((CBP==0) && (CBPB==0) && (EqualVec(MV[0][j+1][i+1],&ZERO)) && 
          (EqualVec(MV[6][j+1][i+1],&ZERO)) &&
          (Mode == MODE_INTER || Mode == MODE_INTER_Q) &&
          (pic->MODB==0)) 
      { 
        /* Skipped MB : both CBP and CBPB are zero, 16x16 vector is zero,
         * PB delta vector is zero, Mode = MODE_INTER and B picture is
         * predicted bidirectionally. */
        coded_map[j+1][i+1] = 0;
        quant_map[j+1][i+1] = 0;

        if (Mode == MODE_INTER_Q || Mode == MODE_INTER4V_Q) 
        {
          /* DQUANT != 0 but not coded anyway */
          QP_xmitted = QP_prev;
          pic->DQUANT = 0;
          Mode = MODE_INTER;
        }
        if (!syntax_arith_coding)
          CountBitsMB(Mode,1,CBP,CBPB,pic,bits,0);
        else
          Count_sac_BitsMB(Mode,1,CBP,CBPB,pic,bits);
      }
      else 
      {
        /* Normal MB */
        if (!syntax_arith_coding) 
        { 
          /* VLC */
          CountBitsMB(Mode,0,CBP,CBPB,pic,bits,0);

          if (Mode == MODE_INTER  || Mode == MODE_INTER_Q) 
          {
            coded_map[j+1][i+1] = 1;
            quant_map[j+1][i+1] = QP_xmitted;

            bits->no_inter++;
            CountBitsVectors(MV, bits, i, j, Mode, newgob, pic, 0);
          }
          else if (Mode == MODE_INTER4V || Mode == MODE_INTER4V_Q) 
          {
            coded_map[j+1][i+1] = 1;
            quant_map[j+1][i+1] = QP_xmitted;

            bits->no_inter4v++;
            CountBitsVectors(MV, bits, i, j, Mode, newgob, pic, 0);
          }
          else 
          {
            /* MODE_INTRA or MODE_INTRA_Q */
            coded_map[j+1][i+1] = 1;
            quant_map[j+1][i+1] = QP_xmitted;
           
            bits->no_intra++;
            if (pic->PB)
            {
              CountBitsVectors(MV, bits, i, j, Mode, newgob, pic, 0);
            }
          }

          if ( (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) &&
                advanced_intra_coding )
          {
            Scan(pcoeff,pic->Intra_Mode);
            CountBitsCoeff(pcoeff, Mode, CBP, bits, 64);
          }
          else if (CBP || Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
          {
            Scan(qcoeff_P,0);
            CountBitsCoeff(qcoeff_P, Mode, CBP, bits, 64);  
          }        

          if (CBPB)
          {
            Scan(qcoeff_B,0);
            CountBitsCoeff(qcoeff_B, MODE_INTER, CBPB, bits, 64);
          }
        } 
        /* end VLC */
        else 
        { 
          /* SAC */
          Count_sac_BitsMB(Mode,0,CBP,CBPB,pic,bits);

          if (Mode == MODE_INTER  || Mode == MODE_INTER_Q) 
          {
            coded_map[j+1][i+1] = 1;
            quant_map[j+1][i+1] = QP_xmitted;
      
            bits->no_inter++;
            Count_sac_BitsVectors(MV, bits, i, j, Mode, newgob, pic);
          }
          else if (Mode == MODE_INTER4V || Mode == MODE_INTER4V_Q) 
          {
            coded_map[j+1][i+1] = 1;
            quant_map[j+1][i+1] = QP_xmitted;

            bits->no_inter4v++;
            Count_sac_BitsVectors(MV, bits, i, j, Mode, newgob, pic);
          }
          else 
          {
            /* MODE_INTRA or MODE_INTRA_Q */
            coded_map[j+1][i+1] = 1;
            quant_map[j+1][i+1] = QP_xmitted;

            bits->no_intra++;
            if (pic->PB)
	            Count_sac_BitsVectors(MV, bits, i, j, Mode, newgob, pic);
          }

          if ((Mode == MODE_INTRA || Mode == MODE_INTRA_Q) &&
               advanced_intra_coding)
          {
            Scan(pcoeff, pic->Intra_Mode);
            Count_sac_BitsCoeff(pcoeff, Mode, CBP, bits, 64);
          }
          else if ( CBP || Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
          {
            Scan(qcoeff_P,0);           
            Count_sac_BitsCoeff(qcoeff_P, Mode, CBP, bits, 64);
          }

          if (CBPB)
          {
            Scan(qcoeff_B,0);
            Count_sac_BitsCoeff(qcoeff_B, MODE_INTER, CBPB, bits, 64);
          } 
        }
        /* end SAC */
        QP_prev = QP_xmitted;
      }

      /* update the rate control parameters for TMN-8 rate control */
      if (rate_control_method == TMN8_RC) 
      { 
        AddBitsPicture (bits);  
        UpdateRateControlMB((bits->total - bits_prev->total),
                            ((bits->Y + bits->C)-(bits_prev->Y + bits_prev->C)), i, j, QP_xmitted);  
        bits_prev->total = bits->total;
        bits_prev->Y = bits->Y;
        bits_prev->C = bits->C;
      } 

      abs_mb_num++;
      QP_cumulative += QP_xmitted;     
#ifdef PRINTQ 
      /* most useful when quantizer changes within a picture */
      if (QuantChangePostponed)
        fprintf(stdout,"@%2d",QP_xmitted);
      else
        fprintf(stdout," %2d",QP_xmitted);
#endif

      if (pic->PB) 
      {
        ReconImage(i,j,recon_data_B,B_recon);
      }

      ReconImage(i,j,recon_data_P,recon);

      if ( (PCT_IPB == pic->picture_coding_type) || PCT_PB == pic->picture_coding_type )
      {
        free(qcoeff_B);
        free(recon_data_B);
      }

      free(recon_data_P);
      free(qcoeff_P);
      free(rcoeff);

      if (advanced_intra_coding && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))  
        free(pcoeff);
    } 
#ifdef PRINTQ
    fprintf(stdout,"\n");
#endif
  }

  /* Do the deblocking filtering, not used in true B pictures. */
  if (deblocking_filter) 
  {
    store_pb = pic->PB;
    pic->PB = 0;
    EdgeFilter(recon, pic);
    pic->PB = store_pb;
    if (pic->PB)
      EdgeFilter(B_recon,pic);
  }

  pic->QP_mean = QP_cumulative/(float)abs_mb_num;

  /* Free memory */
  free (pred_P);
  free (pred_B);

  if (mv_outside_frame)
  {
    free(prev_recon);
    FreeImage(pr_edge);
  }

  free(pi);
 
  for (j = 0; j < (sed_lines>>4)+1; j++)
    for (i = 0; i < (sed_pels>>4)+2; i++) 
      for (k = 0; k < 7; k++)
        free(MV[k][j][i]),free(B_f_MV[k][j][i]);

  if (advanced_intra_coding)
  {
    free(store_coeff);
    free(store_rcoeff);
  }

  if (advanced_temporarily_off)
  {
    overlapping_MC = ON;
    adv_pred = ON;
    use_4mv = ON;
    advanced_temporarily_off = NO;
  }
  return;
}


/**********************************************************************
 *
 *	Name:        CodeOneIntra
 *	Description:	codes one image intra
 *	
 *	Input:        pointer to image, QP
 *        
 *	Returns:	pointer to reconstructed image
 *	Side effects:	memory is allocated to recon image
 *
 *	Date: 940110	Author:	Karl.Lillevold@nta.no
 *
 ***********************************************************************/


PictImage *CodeOneIntra(PictImage *curr, int QP, Bits *bits, Pict *pic)
{
  PictImage *recon;
  MB_Structure *data = (MB_Structure *)malloc(sizeof(MB_Structure));
  int *qcoeff, *pcoeff = NULL, *rcoeff, *coeff; //SV-XXX inited to NULL
  int Mode = MODE_INTRA;
  int CBP,COD;
  int i,j,k;
  int *store_coeff=NULL, *store_rcoeff=NULL; //SV-XXX ininted to NULL /* used to do prediction in advanced intra coding */
  int newgob = 0;

  pic ->Intra_Mode = 0;
  recon = InitImage(sed_pels*sed_lines);
  ZeroBits(bits);
  
  pic->QUANT = QP;
  pic->RTYPE = 0;
  
  if (EPTYPE)
     pic->UFEP = ON;   /* Update full extended PTYPE is set for Intra pictures */
  if (advanced_intra_coding)
  {
     pic->UFEP = ON;   /* Update full extended PTYPE is set for Intra pictures */
     /* store the qcoeff for the frame */
     if ((store_rcoeff=(int *)malloc(sizeof(int)*384*(sed_pels/MB_SIZE)*
          (sed_lines/MB_SIZE))) == 0) 
     {
        fprintf(stderr,"coder(): Couldn't allocate store_rcoeff.\n");
        exit(-1);
     }
     if ((store_coeff=(int *)malloc(sizeof(int)*384*(sed_pels/MB_SIZE)*
          (sed_lines/MB_SIZE))) == 0) 
     {
        fprintf(stderr,"coder(): Couldn't allocate store_coeff.\n");
        exit(-1);
     }
     if ((pcoeff=(int *)malloc(sizeof(int)*384)) == 0) 
     {
        fprintf(stderr,"coder(): Couldn't allocate pcoeff.\n");
        exit(-1);
     }
  } 
  if ((qcoeff=(int *)malloc(sizeof(int)*384)) == 0) {
    fprintf(stderr,"coder(): Couldn't allocate qcoeff.\n");
    exit(-1);
  }
  if ((rcoeff = (int *)malloc(sizeof(int)*384)) == NULL) {
    fprintf(stderr,"coder: Could not allocate space for rcoeff\n");
    exit(-1);
  } 

  bits->header += CountBitsPicture(pic);

  COD = 0; /* Every block is coded in Intra frame */
  for ( j = 0; j < sed_lines/MB_SIZE; j++) {

    /* insert sync in *every* slice if use_gobsync is chosen */
    if (pic->use_gobsync && j != 0)
    {
      bits->header += CountBitsSlice(j,QP);
      newgob = 1;
    }
    for ( i = 0; i < sed_pels/MB_SIZE; i++) {

      pic->MB = i + j * (sed_pels/MB_SIZE);
      bits->no_intra++;
      FillLumBlock(i*MB_SIZE, j*MB_SIZE, curr, data);
      FillChromBlock(i*MB_SIZE, j*MB_SIZE, curr, data);

      coeff = MB_Encode(data);
      /* store the QP for every macroblock */
      quant_map[j+1][i+1] = QP;
      
      if (advanced_intra_coding) 
      {
          /* store the DCT coefficients for encoding */
          memcpy( (void *) (store_coeff + (i + j*sed_pels/MB_SIZE)*384), (void *) coeff, sizeof(int) * 384);
          /* Do Intra mode prediction */
          pic->Intra_Mode = Choose_Intra_Mode(coeff, store_coeff, i, j, newgob);
          for (k=0;k<6;k++) 
          {
          
            Intra_AC_DC_Encode(coeff, store_rcoeff, pic->Intra_Mode, i, j, newgob,k);
            Quant_blk(coeff,pcoeff,QP,Mode,k);
            Dequant(pcoeff, rcoeff, QP, Mode,k);
            sed_Intra_AC_DC_Decode(rcoeff, store_rcoeff, pic->Intra_Mode, i, j, newgob,k);
          }
          MB_Decode(rcoeff, data);
          Scan(pcoeff, pic->Intra_Mode); 
          CBP = FindCBP(pcoeff,Mode,64);
          if (!syntax_arith_coding) 
          {
            CountBitsMB(Mode,COD,CBP,0,pic,bits,0);
            CountBitsCoeff(pcoeff, Mode, CBP,bits,64);
          } 
          else 
          {
            Count_sac_BitsMB(Mode,COD,CBP,0,pic,bits);
            Count_sac_BitsCoeff(pcoeff, Mode, CBP,bits,64);
          }
      }
      else
      {
          for (k=0;k<6;k++)
            Quant_blk(coeff,qcoeff,QP,Mode,k);
          Scan(qcoeff,0);
          CBP = FindCBP(qcoeff,Mode,64);
          if (!syntax_arith_coding) 
          {
            CountBitsMB(Mode,COD,CBP,0,pic,bits,0);
            CountBitsCoeff(qcoeff, Mode, CBP,bits,64);
          } 
          else 
          {
            Count_sac_BitsMB(Mode,COD,CBP,0,pic,bits);
            Count_sac_BitsCoeff(qcoeff, Mode, CBP,bits,64);
          }
          DeScan(qcoeff,0);
          for (k=0;k<6;k++)
            Dequant(qcoeff, rcoeff, QP, Mode,k);
          MB_Decode(rcoeff, data);
      }
      Clip(data);
      ReconImage(i,j,data,recon);

      /* every block is coded */     
      coded_map[j+1][i+1] = 2;
      free(coeff);
      
    }
  }
  pic->QP_mean = (float)QP;
  
  if (deblocking_filter) {
    EdgeFilter(recon,pic);
  }

  if (advanced_intra_coding)
  {
    free(store_coeff);
    free(store_rcoeff);
    free(pcoeff);
  }
  free(qcoeff);
  free(data);
  free(rcoeff);
  return recon;
}

/**********************************************************************
 *
 *	Name:         CodeOneTrueB
 *	Description:  code one true B image 
 *	
 *	Input:        pointer to image, prev_image, prev_recon, Q
 *        
 *	Returns:      pointer to reconstructed image
 *	Side effects: memory is allocated to recon image
 *
 *	Date: 950930  Author: Michael Gallant <mikeg@ee.ubc.ca>
 *
 ***********************************************************************/

void CodeOneTrueB(PictImage *curr, PictImage *B_image, PictImage *prev, 
          PictImage *pr, int QP, int frameskip, Bits *bits,
          Pict *pic, PictImage *B_recon, PictImage *recon)
{
  unsigned char *prev_ipol, *next_ipol, *pi, *ni;
  PictImage *prev_recon=NULL, *next_recon=NULL, *pr_edge = NULL, *nr_edge = NULL;
  MotionVector *MV[7][MBR+1][MBC+2];
  //MotionVector ZERO = {0,0,0,0,0,0}; //SV-XXX: was only 5 zeros in set, but MotionVect struct has 6 elements
  MB_Structure *recon_data_true_B = NULL;
  MB_Structure *diff; 
  int *rcoeff, *coeff;
  int *qcoeff_true_B = NULL;
  int Mode;
  int CBP;
  int newgob, advanced_temporarily_off = DEF_AIC_MODE;
  int i,j,k;
  int true_B_prediction_type;

  /* buffer control vars */
  float QP_cumulative = (float)0.0;
  int abs_mb_num = 0; //SV-XXX , QuantChangePostponed = 0; moved down in #ifdef
  int QP_new, QP_prev, QP_xmitted=QP;

  MB_Structure *pred = (MB_Structure *)malloc(sizeof(MB_Structure));

  /* advanced intra coding variables */
  int *store_rcoeff=NULL, *store_coeff=NULL, *pcoeff=NULL; //SV-XXX: inited to NULL 

  //SV-XXX: 
  UNUSED_(curr);
  UNUSED_(prev);
  UNUSED_(frameskip);

  ZeroBits(bits);

  /* Currently, the MV info is stored in 7 MV structs per MB. MV[0]
   * stores the 16x16 MV. MV[1] through MV[4] store the 8x8 MVs. MV[6]
   * stores the PB delta MV or the forward predicted B MV. MV[5]
   * stores the backward predicted MV for true-B pictures. */ 

  InitializeMV(MV);

  /* Mark PMV's outside the frame. */
  for (i = 1; i < (sed_pels>>4)+1; i++) 
  {
    for (j = 0; j < 7; j++)
    {
      MarkVec(MV[j][0][i]);  
    }
    MV[0][0][i]->Mode = MODE_INTRA;
  }

  /* Zero out PMV's outside the frame */
  for (i = 0; i < (sed_lines>>4)+1; i++) 
  {
    for (j = 0; j < 7; j++) 
    {
      ZeroVec(MV[j][i][0]);
      ZeroVec(MV[j][i][(sed_pels>>4)+1]);
    }

    MV[0][i][0]->Mode = MODE_INTRA;
    MV[0][i][(sed_pels>>4)+1]->Mode = MODE_INTRA;
  }

  GenerateFrameAndInterpolatedImages(pr, pic, &prev_ipol, &prev_recon, &pi, &pr_edge);
  GenerateFrameAndInterpolatedImages(recon, pic, &next_ipol, &next_recon, &ni, &nr_edge);

  if (adv_pred)
  {
    /* Turn off advanced coding since there can be only 1 
     * motion vector for B frames of IPB. */
    advanced_temporarily_off = YES;
    overlapping_MC = OFF;
    adv_pred = OFF;
    use_4mv = OFF;
  }

  /* If coding a B-picture, need to pass pictures for forward AND
   * backward prediction, as well as interpolated forward and backward
   * pictures for half-pel prediction. */
  MotionEstimatePicture( B_image->lum,prev_recon->lum, next_recon->lum,         
                         prev_ipol, next_ipol, pic->seek_dist, MV,
                         pic->use_gobsync, B_PICTURE_ESTIMATION);

  QP_new = QP_xmitted = QP_prev = QP; /* Copy the passed value of QP */

  for ( j = 0; j < sed_lines/MB_SIZE; j++) 
  {
    newgob = 0;

    if (j == 0) 
    {
      if (advanced_intra_coding)
      {
         /* store the coeff for the frame */
         if ((store_coeff=(int *)calloc(384*(sed_pels/MB_SIZE)*(sed_lines/MB_SIZE), sizeof(int))) == 0) 
         {
            fprintf(stderr,"coder(): Couldn't allocate store_coeff.\n");
            exit(-1);
         }
         if ((store_rcoeff=(int *)calloc(384*(sed_pels/MB_SIZE)*(sed_lines/MB_SIZE), sizeof(int))) == 0) 
         {
            fprintf(stderr,"coder(): Couldn't allocate store_rcoeff.\n");
            exit(-1);
         }
      }

      pic->QUANT = QP_new;

      bits->header += CountBitsPicture(pic);

      QP_xmitted = QP_prev = QP_new;
    }
    else if (pic->use_gobsync && j%pic->use_gobsync == 0) 
    {
      bits->header += CountBitsSlice(j,QP_new); /* insert gob sync */
      QP_xmitted = QP_prev = QP_new;
      newgob = 1;
    }
 
    for ( i = 0; i < sed_pels/MB_SIZE; i++) 
    {
      pic->MB = i + j * (sed_pels/MB_SIZE);
      
      /* store the QP for every macroblock */
      quant_map[j+1][i+1] = QP_xmitted;

      if ((rcoeff = (int *)malloc(sizeof(int)*384)) == NULL) 
      {
        fprintf(stderr,"MB_Coder: Could not allocate space for rcoeff\n");
        exit(-1);
      }  

      /* Predict true B-MB */
      diff = Predict_True_B( B_image, prev_recon, prev_ipol,next_recon, 
                             next_ipol, pred,i*MB_SIZE, j*MB_SIZE,
                             MV, True_B_Direct_Mode_MV, pic->TRB, 
                             &true_B_prediction_type, &Mode);

      if (B_INTRA_PREDICTION == true_B_prediction_type)
      {
        FillLumBlock(i*MB_SIZE, j*MB_SIZE, B_image, diff);
        FillChromBlock(i*MB_SIZE, j*MB_SIZE, B_image, diff);
      }

      if ((qcoeff_true_B=(int *)malloc(sizeof(int)*384)) == 0) 
      {
        fprintf(stderr,"coder(): Couldn't allocate qcoeff_true_B.\n");
        exit(-1);
      }
      coeff = MB_Encode(diff);

      if (advanced_intra_coding) 
      {
        if (!(Mode == MODE_INTRA || Mode == MODE_INTRA_Q)) 
       	{
          for (k=0;k<6;k++) 
          {    
       	     /* store default coeff if non-intra macroblock */
       	     store_coeff[(i + j * sed_pels/MB_SIZE) * 384 + k * 64] = 1024;
             store_rcoeff[(i + j * sed_pels/MB_SIZE) * 384 + k * 64] = 1024;
          }
          for (k=0;k<6;k++)
            Quant_blk(coeff,qcoeff_true_B,QP_xmitted, Mode,k);
          CBP = FindCBP(qcoeff_true_B, Mode, 64);
          if (CBP == 0 && (Mode == MODE_INTER || Mode == MODE_INTER_Q)) 
            ZeroMBlock(diff);
          else
          {  
            for (k=0;k<6;k++)
            {
              Quant_blk(coeff,qcoeff_true_B,QP_xmitted, Mode,k);
              Dequant(qcoeff_true_B, rcoeff, QP_xmitted, Mode,k);
            }
            MB_Decode(rcoeff, diff);
          }
       	}
       	else 
        {
       	  if ((pcoeff=(int *)malloc(sizeof(int)*384)) == 0) 
          {
	    fprintf(stderr,"coder(): Couldn't allocate pcoeff.\n");
	    exit(-1);
          }

          /* store the quantized DCT coefficients */
          memcpy( (void *) (store_coeff + (i + j*sed_pels/MB_SIZE)*384), (void *) coeff, sizeof(int) * 384);

          /* Do Intra mode prediction */
          pic->Intra_Mode = Choose_Intra_Mode(pcoeff, store_coeff, i, j, newgob);

          for (k=0;k<6;k++) 
          { 
            Intra_AC_DC_Encode(coeff, store_rcoeff, pic->Intra_Mode, i, j, newgob,k);
            Quant_blk(coeff,pcoeff,QP_xmitted,Mode,k);
            Dequant(pcoeff, rcoeff, QP_xmitted, Mode,k);
            sed_Intra_AC_DC_Decode(rcoeff, store_rcoeff, pic->Intra_Mode, i, j, newgob,k);
          }
          MB_Decode(rcoeff, diff);
          CBP = FindCBP(pcoeff,Mode,64);
	}    
      }
      else
      {  
        for (k=0;k<6;k++)
          Quant_blk(coeff,qcoeff_true_B,QP_xmitted, Mode,k);
        CBP = FindCBP(qcoeff_true_B, Mode, 64);
        if (CBP == 0 && (Mode == MODE_INTER || Mode == MODE_INTER_Q)) 
          ZeroMBlock(diff);
        else
        {
          for (k=0;k<6;k++)  
            Dequant(qcoeff_true_B, rcoeff, QP_xmitted, Mode,k); 
          MB_Decode(rcoeff, diff);
        }
      }

      recon_data_true_B = MB_Recon_True_B( prev_recon, prev_ipol, diff, 
                                           next_recon, next_ipol, 
                                           i*MB_SIZE,j*MB_SIZE,MV,
                                           True_B_Direct_Mode_MV, pic->TRB,
                                           true_B_prediction_type);
      Clip(recon_data_true_B);
      free(diff);
      free(coeff);

      if ((CBP==0) && (B_DIRECT_PREDICTION == true_B_prediction_type) &&
          (Mode == MODE_INTER) && (0 == pic->DQUANT) ) 
      {
        coded_map[j+1][i+1] = 0;
        quant_map[j+1][i+1] = 0;

        CountBitsScalMB(Mode, 1, CBP, 0, pic, bits, true_B_prediction_type, 0);
      }
      else 
      {
        CountBitsScalMB(Mode,0,CBP,0,pic,bits,true_B_prediction_type, 0);

        if (MODE_INTER == Mode || MODE_INTER_Q == Mode)
        {
	        coded_map[j+1][i+1] = 1;
	        quant_map[j+1][i+1] = QP_xmitted;

          bits->no_inter++;
          CountBitsVectors( MV, bits, i, j, Mode, newgob, pic, 
                            true_B_prediction_type);
        }
        else 
        {
          /* MODE_INTRA or MODE_INTRA_Q */
          coded_map[j+1][i+1] = 1;
          quant_map[j+1][i+1] = QP_xmitted;

          bits->no_intra++;
        }
         
        if ( (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) && advanced_intra_coding )
        {
          Scan(pcoeff,pic->Intra_Mode);
          CountBitsCoeff(pcoeff, Mode, CBP, bits, 64);
        }
        else if (CBP || Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
        {
          Scan(qcoeff_true_B,0);
          CountBitsCoeff(qcoeff_true_B, Mode, CBP, bits, 64);
        }
      }
      QP_prev = QP_xmitted;
    
      abs_mb_num++;
      QP_cumulative += QP_xmitted;     
#ifdef PRINTQ 
      int QuantChangePostponed = 0; //SV-XXX: moved here to avoid gcc4 warning

      /* most useful when quantizer changes within a picture */
      if (QuantChangePostponed)
        fprintf(stdout,"@%2d",QP_xmitted);
      else
        fprintf(stdout," %2d",QP_xmitted);
#endif

      ReconImage(i,j,recon_data_true_B,B_recon);
 
      free(qcoeff_true_B);
      free(recon_data_true_B);

      if (advanced_intra_coding && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))  
        free(pcoeff);
    }
#ifdef PRINTQ
    fprintf(stdout,"\n");
#endif
  }

  pic->QP_mean = QP_cumulative/(float)abs_mb_num;

  /* Free memory */
  free(pred);
  free(prev_recon);
  free(next_recon);
  FreeImage(pr_edge);
  FreeImage(nr_edge);
  free(pi);
  free(ni);
  
  for (j = 0; j < (sed_lines>>4)+1; j++)
    for (i = 0; i < (sed_pels>>4)+2; i++) 
      for (k = 0; k < 7; k++)
        free(MV[k][j][i]);

  if (advanced_intra_coding)
  {
    free(store_coeff);
    free(store_rcoeff);
  }

  if (advanced_temporarily_off)
  {
    overlapping_MC = ON;
    adv_pred = ON;
    use_4mv = ON;
    advanced_temporarily_off = NO;
  }

  return;
}

/**********************************************************************
 *
 *	Name:         CodeOneEI
 *	Description:  codes one image intra
 *	
 *	Input:        pointer to image, QP
 *        
 *	Returns:      pointer to reconstructed image
 *	Side effects: memory is allocated to recon image
 *
 *	Date: 940110  Author: Michael Gallant	mikeg@ee.ubc.ca
 *
 ***********************************************************************/


void CodeOneEI(PictImage *curr_image, PictImage *pr, 
                     int QP, Bits *bits, Pict *pic, PictImage *recon)
{
  MB_Structure *diff = (MB_Structure *)malloc(sizeof(MB_Structure));
  MB_Structure *recon_data_ei = (MB_Structure *)malloc(sizeof(MB_Structure));
  PictImage *base_recon=NULL, *base_recon_edge=NULL;
  int *qcoeff, *pcoeff=NULL, *rcoeff, *coeff; //SV-XXX: inited to NULL
  int Mode = MODE_INTER;
  int CBP;
  int i,j,k;
  int *store_coeff=NULL, *store_rcoeff=NULL; //SV-XXX: inited to NULL /* used to do prediction in advanced intra coding */
  int newgob = 0;
  int ei_prediction_type;
  unsigned char *base_ipol, *bi;
  int advanced_temporarily_off = DEF_AIC_MODE;

  /* buffer control vars */
  float QP_cumulative = (float)0.0;
  int abs_mb_num = 0; //SV-XXX , QuantChangePostponed = 0; moved down in #ifdef
  int QP_new, QP_prev, QP_xmitted=QP;
  
  ZeroBits(bits);

  GenerateFrameAndInterpolatedImages(pr, pic, &base_ipol, &base_recon, &bi, &base_recon_edge);

  if (adv_pred)
  {
    /* Turn off advanced coding since there can be only 1 
     * motion vector for B frames of IPB. */
    advanced_temporarily_off = YES;
    overlapping_MC = OFF;
    adv_pred = OFF;
    use_4mv = OFF;
  }

  QP_new = QP_xmitted = QP_prev = QP; /* Copy the passed value of QP */

  for ( j = 0; j < sed_lines/MB_SIZE; j++) 
  {
    newgob = 0;

    if (j == 0) 
    {
      if (advanced_intra_coding)
      {
         /* store the coeff for the frame */
         if ((store_coeff=(int *)calloc(384*(sed_pels/MB_SIZE)*(sed_lines/MB_SIZE), sizeof(int))) == 0) 
         {
            fprintf(stderr,"coder(): Couldn't allocate store_coeff.\n");
            exit(-1);
         }
         if ((store_rcoeff=(int *)calloc(384*(sed_pels/MB_SIZE)*(sed_lines/MB_SIZE), sizeof(int))) == 0) 
         {
            fprintf(stderr,"coder(): Couldn't allocate store_rcoeff.\n");
            exit(-1);
         }
      }

      pic->QUANT = QP_new;

      bits->header += CountBitsPicture(pic);

      QP_xmitted = QP_prev = QP_new;
    }
    else if (pic->use_gobsync && j%pic->use_gobsync == 0) 
    {
      bits->header += CountBitsSlice(j,QP_new); /* insert gob sync */
      QP_xmitted = QP_prev = QP_new;
      newgob = 1;
    }
 
    for ( i = 0; i < sed_pels/MB_SIZE; i++) 
    {
      pic->MB = i + j * (sed_pels/MB_SIZE);
      
      /* store the QP for every macroblock */
      quant_map[j+1][i+1] = QP_xmitted;

      if ((rcoeff = (int *)malloc(sizeof(int)*384)) == NULL) 
      {
        fprintf(stderr,"MB_Coder: Could not allocate space for rcoeff\n");
        exit(-1);
      }  

      /* Predict true B-MB */
      diff = Predict_EI( curr_image, base_recon, base_ipol, 
                         i*MB_SIZE, j*MB_SIZE,
                         &ei_prediction_type, &Mode);
      
      if (EI_INTRA_PREDICTION == ei_prediction_type)
      {
        FillLumBlock(i*MB_SIZE, j*MB_SIZE, curr_image, diff);
        FillChromBlock(i*MB_SIZE, j*MB_SIZE, curr_image, diff);
      }

      if ((qcoeff=(int *)malloc(sizeof(int)*384)) == 0) 
      {
        fprintf(stderr,"coder(): Couldn't allocate qcoeff.\n");
        exit(-1);
      }
      coeff = MB_Encode(diff);

      if (advanced_intra_coding) 
      {	
        if (!(Mode == MODE_INTRA || Mode == MODE_INTRA_Q)) 
        {
          for (k=0;k<6;k++) 
          {    
            /* store default coeff if non-intra macroblock */
            store_coeff[(i + j * sed_pels/MB_SIZE) * 384 + k * 64] = 1024;
            store_rcoeff[(i + j * sed_pels/MB_SIZE) * 384 + k * 64] = 1024;
          }
          for (k=0;k<6;k++)
            Quant_blk(coeff,qcoeff,QP_xmitted, Mode,k);
          CBP = FindCBP(qcoeff, Mode, 64);
          if (CBP == 0 && (Mode == MODE_INTER || Mode == MODE_INTER_Q)) 
            ZeroMBlock(diff);
          else
          {  
            for (k=0;k<6;k++)
            {
              Quant_blk(coeff,qcoeff,QP_xmitted, Mode,k);
              Dequant(qcoeff, rcoeff, QP_xmitted, Mode,k);
            }
            MB_Decode(rcoeff, diff);
          }
        }
        else 
        {
          if ((pcoeff=(int *)malloc(sizeof(int)*384)) == 0) 
          {
            fprintf(stderr,"coder(): Couldn't allocate pcoeff.\n");
            exit(-1);
          }

          /* store the quantized DCT coefficients */
          memcpy( (void *) (store_coeff + (i + j*sed_pels/MB_SIZE)*384), (void *) coeff, sizeof(int) * 384);

          /* Do Intra mode prediction */
          pic->Intra_Mode = Choose_Intra_Mode(pcoeff, store_coeff, i, j, newgob);

          for (k=0;k<6;k++) 
          { 
            Intra_AC_DC_Encode(coeff, store_rcoeff, pic->Intra_Mode, i, j, newgob,k);
            Quant_blk(coeff,pcoeff,QP_xmitted,Mode,k);
            Dequant(pcoeff, rcoeff, QP_xmitted, Mode,k);
            sed_Intra_AC_DC_Decode(rcoeff, store_rcoeff, pic->Intra_Mode, i, j, newgob,k);
          }
          MB_Decode(rcoeff, diff);
          CBP = FindCBP(pcoeff,Mode,64);
        }    
      }
      else
      {  
        for (k=0;k<6;k++)
          Quant_blk(coeff,qcoeff,QP_xmitted, Mode,k);
        CBP = FindCBP(qcoeff, Mode, 64);
        if (CBP == 0 && (Mode == MODE_INTER || Mode == MODE_INTER_Q)) 
          ZeroMBlock(diff);
        else
        {
          for (k=0;k<6;k++)  
            Dequant(qcoeff, rcoeff, QP_xmitted, Mode,k); 
          MB_Decode(rcoeff, diff);
        }
      }

      recon_data_ei = MB_Recon_EI( base_recon, base_ipol, diff, 
                                   i*MB_SIZE,j*MB_SIZE,
                                   ei_prediction_type);

      Clip(recon_data_ei);
      free(diff);
      free(coeff);

      if ((CBP==0) && (EI_UPWARD_PREDICTION == ei_prediction_type) &&
          (Mode == MODE_INTER) && (0 == pic->DQUANT) ) 
      {
        coded_map[j+1][i+1] = 0;
        quant_map[j+1][i+1] = 0;

        CountBitsScalMB(Mode, 1, CBP, 0, pic, bits, ei_prediction_type, 0);
      }
      else 
      {
        CountBitsScalMB(Mode,0,CBP,0,pic,bits,ei_prediction_type, 0);

        if (MODE_INTER == Mode || MODE_INTER_Q == Mode)
        {
          coded_map[j+1][i+1] = 1;
          quant_map[j+1][i+1] = QP_xmitted;
          bits->no_inter++;
        }
        else 
        {
          /* MODE_INTRA or MODE_INTRA_Q */
          coded_map[j+1][i+1] = 1;
          quant_map[j+1][i+1] = QP_xmitted;
          bits->no_intra++;
        }
         
        if ( (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) && advanced_intra_coding )
        {
          Scan(pcoeff,pic->Intra_Mode);
	  CountBitsCoeff(pcoeff, Mode, CBP, bits, 64);
        }
        else if (CBP || Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
        {
          Scan(qcoeff,0);
          CountBitsCoeff(qcoeff, Mode, CBP, bits, 64);
        }
      }
      QP_prev = QP_xmitted;
    
      abs_mb_num++;
      QP_cumulative += QP_xmitted;     
#ifdef PRINTQ 
      int QuantChangePostponed = 0; //SV-XXX: moved here to shut up gcc4

      /* most useful when quantizer changes within a picture */
      if (QuantChangePostponed)
        fprintf(stdout,"@%2d",QP_xmitted);
      else
        fprintf(stdout," %2d",QP_xmitted);
#endif

      ReconImage(i,j,recon_data_ei,recon);
 
      free(qcoeff);
      free(recon_data_ei);

      if (advanced_intra_coding && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))  
        free(pcoeff);
    }
#ifdef PRINTQ
    fprintf(stdout,"\n");
#endif
  }

  pic->QP_mean = QP_cumulative/(float)abs_mb_num;

  /* Free memory */
  free(base_recon);
  FreeImage(base_recon_edge);
  free(bi);

  if (advanced_intra_coding)
  {
    free(store_coeff);
    free(store_rcoeff);
  }

  if (advanced_temporarily_off)
  {
    overlapping_MC = ON;
    adv_pred = ON;
    use_4mv = ON;
    advanced_temporarily_off = NO;
  }

  return;
}

/**********************************************************************
 *
 *	Name:         CodeOneEP
 *	Description:  code one EP image 
 *	
 *	Input:        pointer to image, prev_image, prev_recon, Q
 *        
 *	Returns:      pointer to reconstructed image
 *	Side effects: memory is allocated to recon image
 *
 *	Date: 950930  Author: Michael Gallant <mikeg@ee.ubc.ca>
 *
 ***********************************************************************/
 void CodeOneEP(PictImage *curr, PictImage *prev_enhance_image, 
               PictImage *prev_enhance_recon, int QP, int frameskip, Bits *bits,
               Pict *pic, PictImage *curr_ref_recon, PictImage *curr_enhance_recon)
{
  unsigned char *prev_ipol, *base_ipol, *pi, *bi;
  PictImage *prev_recon=NULL, *base_recon=NULL, *pr_edge = NULL, *nr_edge = NULL;
  MotionVector *MV[7][MBR+1][MBC+2];
  MotionVector ZERO = {0,0,0,0,0,0}; //SV-XXX: was only 5 zeros in set, but MotionVect struct has 6 elements
  MB_Structure *recon_data_ep = NULL;
  MB_Structure *diff; 
  int *rcoeff, *coeff;
  int *qcoeff_ep = NULL;
  int Mode;
  int CBP;
  int newgob, advanced_temporarily_off = DEF_AIC_MODE;
  int i,j,k;
  int ep_prediction_type, MV_present = OFF;

  /* buffer control vars */
  float QP_cumulative = (float)0.0;
  int abs_mb_num = 0; //SV-XXX , QuantChangePostponed = 0; moved down in #ifdef
  int QP_new, QP_prev, QP_xmitted=QP;

  MB_Structure *pred = (MB_Structure *)malloc(sizeof(MB_Structure)); 

  /* advanced intra coding variables */
  int *store_rcoeff=NULL, *store_coeff=NULL, *pcoeff=NULL; //SV-XXX: inited to NULL

  //SV-XXX:
  UNUSED_(frameskip);
  UNUSED_(prev_enhance_image);

  ZeroBits(bits);

  /* Currently, the MV info is stored in 7 MV structs per MB. MV[0]
   * stores the 16x16 MV. MV[1] through MV[4] store the 8x8 MVs. MV[6]
   * stores the PB delta MV or the forward predicted B MV. MV[5]
   * stores the backward predicted MV for true-B pictures. */ 

  InitializeMV(MV);

  /* Mark PMV's outside the frame. */
  for (i = 1; i < (sed_pels>>4)+1; i++) 
  {
    for (j = 0; j < 7; j++)
    {
      MarkVec(MV[j][0][i]);  
    }
    MV[0][0][i]->Mode = MODE_INTRA;
  }

  /* Zero out PMV's outside the frame */
  for (i = 0; i < (sed_lines>>4)+1; i++) 
  {
    for (j = 0; j < 7; j++) 
    {
      ZeroVec(MV[j][i][0]);
      ZeroVec(MV[j][i][(sed_pels>>4)+1]);
    }
    MV[0][i][0]->Mode = MODE_INTRA;
    MV[0][i][(sed_pels>>4)+1]->Mode = MODE_INTRA;
  }

  GenerateFrameAndInterpolatedImages(prev_enhance_recon, pic, &prev_ipol, &prev_recon, &pi, &pr_edge);
  GenerateFrameAndInterpolatedImages(curr_ref_recon, pic, &base_ipol, &base_recon, &bi, &nr_edge);

  if (adv_pred)
  {
    /* Turn off advanced coding since there can be only 1 
     * motion vector for B frames of IPB. */
    advanced_temporarily_off = YES;
    overlapping_MC = OFF;
    adv_pred = OFF;
    use_4mv = OFF;
  }

  MotionEstimatePicture( curr->lum,prev_recon->lum, base_recon->lum,         
                         prev_ipol, base_ipol, pic->seek_dist, MV,
                         pic->use_gobsync, EI_EP_PICTURE_ESTIMATION);

  QP_new = QP_xmitted = QP_prev = QP;

  /* Copy the passed value of QP */

  for ( j = 0; j < sed_lines/MB_SIZE; j++) 
  {
    newgob = 0;

    if (j == 0) 
    {
      if (advanced_intra_coding)
      {
        /* store the coeff for the frame */
        if ((store_coeff=(int *)calloc(384*(sed_pels/MB_SIZE)*(sed_lines/MB_SIZE), sizeof(int))) == 0) 
        {
          fprintf(stderr,"coder(): Couldn't allocate store_coeff.\n");
          exit(-1);
        }
        if ((store_rcoeff=(int *)calloc(384*(sed_pels/MB_SIZE)*(sed_lines/MB_SIZE), sizeof(int))) == 0) 
        {
          fprintf(stderr,"coder(): Couldn't allocate store_rcoeff.\n");
          exit(-1);
        }
      }

      pic->QUANT = QP_new;

      bits->header += CountBitsPicture(pic);

      QP_xmitted = QP_prev = QP_new;
    }
    else if (pic->use_gobsync && j%pic->use_gobsync == 0) 
    {
      bits->header += CountBitsSlice(j,QP_new);

      /* insert gob sync */
      QP_xmitted = QP_prev = QP_new;
      newgob = 1;
    }
 
    for ( i = 0; i < sed_pels/MB_SIZE; i++) 
    {
      pic->MB = i + j * (sed_pels/MB_SIZE);
      
      /* store the QP for every macroblock */
      quant_map[j+1][i+1] = QP_xmitted;

      if ((rcoeff = (int *)malloc(sizeof(int)*384)) == NULL) 
      {
        fprintf(stderr,"MB_Coder: Could not allocate space for rcoeff\n");
        exit(-1);
      }  

      /* Predict true B-MB */
      diff = Predict_EP( curr, prev_recon, prev_ipol, base_recon, 
                         base_ipol, pred, i*MB_SIZE, j*MB_SIZE,
                         MV, &ep_prediction_type, &Mode);

      if (EP_INTRA_PREDICTION == ep_prediction_type)
      {
        FillLumBlock(i*MB_SIZE, j*MB_SIZE, curr, diff);
        FillChromBlock(i*MB_SIZE, j*MB_SIZE, curr, diff);
      }

      if ((qcoeff_ep=(int *)malloc(sizeof(int)*384)) == 0) 
      {
        fprintf(stderr,"coder(): Couldn't allocate qcoeff_ep.\n");
        exit(-1);
      }
      coeff = MB_Encode(diff);
      if (advanced_intra_coding) 
      {
        if (!(Mode == MODE_INTRA || Mode == MODE_INTRA_Q)) 
        {
          for (k=0;k<6;k++) 
          {    
            /* store default coeff if non-intra macroblock */
            store_coeff[(i + j * sed_pels/MB_SIZE) * 384 + k * 64] = 1024;
            store_rcoeff[(i + j * sed_pels/MB_SIZE) * 384 + k * 64] = 1024;
          }
          for (k=0;k<6;k++)
            Quant_blk(coeff,qcoeff_ep,QP_xmitted, Mode,k);
          CBP = FindCBP(qcoeff_ep, Mode, 64);
          if (CBP == 0 && (Mode == MODE_INTER || Mode == MODE_INTER_Q)) 
            ZeroMBlock(diff);
          else
          {  
            for (k=0;k<6;k++)
            {
              Quant_blk(coeff,qcoeff_ep,QP_xmitted, Mode,k);
              Dequant(qcoeff_ep, rcoeff, QP_xmitted, Mode,k);
            }
            MB_Decode(rcoeff, diff);
          }
        }
        else 
        {
          if ((pcoeff=(int *)malloc(sizeof(int)*384)) == 0) 
          {
            fprintf(stderr,"coder(): Couldn't allocate pcoeff.\n");
            exit(-1);
          }

          /* store the quantized DCT coefficients */
          memcpy( (void *) (store_coeff + (i + j*sed_pels/MB_SIZE)*384), (void *) coeff, sizeof(int) * 384);

          /* Do Intra mode prediction */
          pic->Intra_Mode = Choose_Intra_Mode(pcoeff, store_coeff, i, j, newgob);

          for (k=0;k<6;k++) 
          { 
            Intra_AC_DC_Encode(coeff, store_rcoeff, pic->Intra_Mode, i, j, newgob,k);
            Quant_blk(coeff,pcoeff,QP_xmitted,Mode,k);
            Dequant(pcoeff, rcoeff, QP_xmitted, Mode,k);
            sed_Intra_AC_DC_Decode(rcoeff, store_rcoeff, pic->Intra_Mode, i, j, newgob,k);
          }
          MB_Decode(rcoeff, diff);
          CBP = FindCBP(pcoeff,Mode,64);
        }    
      }
      else
      {  
        for (k=0;k<6;k++)
          Quant_blk(coeff,qcoeff_ep,QP_xmitted, Mode,k);
        CBP = FindCBP(qcoeff_ep, Mode, 64);
        if (CBP == 0 && (Mode == MODE_INTER || Mode == MODE_INTER_Q)) 
          ZeroMBlock(diff);
        else
        {
          for (k=0;k<6;k++)  
            Dequant(qcoeff_ep, rcoeff, QP_xmitted, Mode,k); 
          MB_Decode(rcoeff, diff);
        }
      }

      recon_data_ep = MB_Recon_EP( prev_recon, prev_ipol, diff, 
                                   base_recon, base_ipol, 
                                   i*MB_SIZE,j*MB_SIZE,MV,
                                   ep_prediction_type);
      Clip(recon_data_ep);
      free(diff);
      free(coeff);

      if ((CBP==0) && (EP_FORWARD_PREDICTION == ep_prediction_type) &&
          (Mode == MODE_INTER) && (0 == pic->DQUANT) &&
          (EqualVec(MV[0][j+1][i+1],&ZERO)) ) 
      {
        coded_map[j+1][i+1] = 0;
        quant_map[j+1][i+1] = 0;

        CountBitsScalMB(Mode, 1, CBP, 0, pic, bits, ep_prediction_type, 0);
      }
      else 
      {
        if ( (EP_FORWARD_PREDICTION == ep_prediction_type) ||
             ( (EP_BIDIRECTIONAL_PREDICTION == ep_prediction_type) &&
               ( !(EqualVec(MV[0][j+1][i+1],&ZERO)) || (0 != CBP) ) ) )
        {
          MV_present = ON;
        }
        else
        {
          MV_present = OFF;
        }

        CountBitsScalMB(Mode,0,CBP,0,pic,bits,ep_prediction_type, MV_present);

        if (MODE_INTER == Mode || MODE_INTER_Q == Mode)
        {
          coded_map[j+1][i+1] = 1;
          quant_map[j+1][i+1] = QP_xmitted;
          bits->no_inter++;

          if (MV_present)
          {
            CountBitsVectors( MV, bits, i, j, Mode, newgob, pic, 
                              ep_prediction_type);
          }
        }
        else 
        {
          /* MODE_INTRA or MODE_INTRA_Q */
          coded_map[j+1][i+1] = 1;
          quant_map[j+1][i+1] = QP_xmitted;
          bits->no_intra++;
        }
         
        if ( (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) && advanced_intra_coding )
        {
          Scan(pcoeff,pic->Intra_Mode);
          CountBitsCoeff(pcoeff, Mode, CBP, bits, 64);
        }
        else if (CBP || Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
        {
          Scan(qcoeff_ep,0);
          CountBitsCoeff(qcoeff_ep, Mode, CBP, bits, 64);
        }
      }
      QP_prev = QP_xmitted;
    
      abs_mb_num++;
      QP_cumulative += QP_xmitted;     
#ifdef PRINTQ 
      int QuantChangePostponed = 0; //SV-XXX: moved here to shut up gcc4

      /* most useful when quantizer changes within a picture */
      if (QuantChangePostponed)
        fprintf(stdout,"@%2d",QP_xmitted);
      else
        fprintf(stdout," %2d",QP_xmitted);
#endif

      ReconImage(i,j,recon_data_ep,curr_enhance_recon);
 
      free(qcoeff_ep);
      free(recon_data_ep);

      if (advanced_intra_coding && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q))  
        free(pcoeff);
    }
#ifdef PRINTQ
    fprintf(stdout,"\n");
#endif
  }

  pic->QP_mean = QP_cumulative/(float)abs_mb_num;

  /* Free memory */
  free(pred);
  free(prev_recon);
  free(base_recon);
  FreeImage(pr_edge);
  FreeImage(nr_edge);
  free(pi);
  free(bi);

  for (j = 0; j < (sed_lines>>4)+1; j++)
    for (i = 0; i < (sed_pels>>4)+2; i++) 
      for (k = 0; k < 7; k++)
        free(MV[k][j][i]);

  if (advanced_intra_coding)
  {
    free(store_coeff);
    free(store_rcoeff);
    free(pcoeff);
  }

  if (advanced_temporarily_off)
  {
    overlapping_MC = ON;
    adv_pred = ON;
    use_4mv = ON;
    advanced_temporarily_off = NO;
  }

  return;
}

/**********************************************************************
 *
 *	Name:        MB_Encode
 *	Description:	DCT  of Macroblocks
 *
 *	Input:        MB data struct
 *	Returns:	Pointer to coefficients 
 *	Side effects:	
 *
 *	Date: 930128	Author: Robert.Danielsen@nta.no
 *      Date: 970915    Author: guyc@ee.ubc.ca
 *                              Quantization done separatly
 *
 **********************************************************************/
int *MB_Encode(MB_Structure *mb_orig)
{
  int        i, j, k, l, row, col;
  int        fblock[64];
  int        *coeff;
  int        *coeff_ind;

  if ((coeff = (int *)malloc(sizeof(int)*384)) == NULL) {
    fprintf(stderr,"MB_Encode: Could not allocate space for coeff\n");
    exit(-1);
  } 
  coeff_ind = coeff;
  for (k=0;k<16;k+=8) {
    for (l=0;l<16;l+=8) {
      for (i=k,row=0;row<64;i++,row+=8) {
        for (j=l,col=0;col<8;j++,col++) {
          *(fblock+row+col) = mb_orig->lum[i][j];
        }
      }
      Dct(fblock,coeff_ind);
      coeff_ind += 64;
    }
  }

  for (i=0;i<8;i++) {
    for (j=0;j<8;j++) {
      *(fblock+i*8+j) = mb_orig->Cb[i][j];
    }
  }
  Dct(fblock,coeff_ind);
  coeff_ind += 64;
  for (i=0;i<8;i++) {
    for (j=0;j<8;j++) {
      *(fblock+i*8+j) = mb_orig->Cr[i][j];
    }
  }
  Dct(fblock,coeff_ind);
  
  return coeff;
}

/**********************************************************************
 *
 *	Name:        MB_Decode
 *	Description:	Reconstruction of quantized DCT-coded Macroblocks
 *
 *	Input:        Quantized coefficients, MB data
 *        QP (1-31, 0 = no quant), MB info block
 *	Returns:	int (just 0)
 *	Side effects:	
 *
 *	Date: 930128	Author: Robert.Danielsen@nta.no
 *      Date: 970915    Author: guyc@ee.ubc.ca
 *                              Deqantization done separatly 
 **********************************************************************/
int MB_Decode(int *rcoeff, MB_Structure *mb_recon)
{
  int	i, j, k, l, row, col;
  int	*iblock;
  int	*rcoeff_ind;

  if ((iblock = (int *)malloc(sizeof(int)*64)) == NULL) {
    fprintf(stderr,"MB_Coder: Could not allocate space for iblock\n");
    exit(-1);
  }

  /* For control purposes */
  /* Zero data */
  for (i = 0; i < 16; i++)
    for (j = 0; j < 16; j++)
      mb_recon->lum[j][i] = 0;
  for (i = 0; i < 8; i++) 
    for (j = 0; j < 8; j++) {
      mb_recon->Cb[j][i] = 0;
      mb_recon->Cr[j][i] = 0;
    }

  rcoeff_ind = rcoeff;

  for (k=0;k<16;k+=8) {
    for (l=0;l<16;l+=8) {
#ifndef FASTIDCT
      sed_idctref(rcoeff_ind,iblock); 
#else
      idct(rcoeff_ind,iblock); 
#endif
      rcoeff_ind += 64;
      for (i=k,row=0;row<64;i++,row+=8) {
        for (j=l,col=0;col<8;j++,col++) {
          mb_recon->lum[i][j] = *(iblock+row+col);
        }
      }
    }
  }

#ifndef FASTIDCT
  sed_idctref(rcoeff_ind,iblock); 
#else
  idct(rcoeff_ind,iblock); 
#endif
  rcoeff_ind += 64;
  for (i=0;i<8;i++) {
    for (j=0;j<8;j++) {
      mb_recon->Cb[i][j] = *(iblock+i*8+j);
    }
  }

/*  printf("Cb.\n");
  for (i=0;i<8;i++) 
  {
    printf("%d %d %d %d %d %d %d %d\n", mb_recon->Cb[i][0],
           mb_recon->Cb[i][1], mb_recon->Cb[i][2], mb_recon->Cb[i][3],
           mb_recon->Cb[i][4], mb_recon->Cb[i][5], mb_recon->Cb[i][6],
           mb_recon->Cb[i][7]);
  }
  printf("\n");
*/
#ifndef FASTIDCT
  sed_idctref(rcoeff_ind,iblock); 
#else
  idct(rcoeff_ind,iblock); 
#endif
  for (i=0;i<8;i++) {
    for (j=0;j<8;j++) {
      mb_recon->Cr[i][j] = *(iblock+i*8+j);
    }
  }
  free(iblock);
  return 0;
}


/**********************************************************************
 *
 *	Name:        FillLumBlock
 *	Description:        Fills the luminance of one block of PictImage
 *	
 *	Input:        Position, pointer to PictImage, array to fill
 *	Returns:        
 *	Side effects:	fills array
 *
 *	Date: 930129	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void FillLumBlock( int x, int y, PictImage *image, MB_Structure *data)
{
  int n;
  register int m;

  for (n = 0; n < MB_SIZE; n++)
    for (m = 0; m < MB_SIZE; m++)
      data->lum[n][m] = 
        (int)(*(image->lum + x+m + (y+n)*sed_pels));
  return;
}

/**********************************************************************
 *
 *	Name:        FillChromBlock
 *	Description:        Fills the chrominance of one block of PictImage
 *	
 *	Input:        Position, pointer to PictImage, array to fill
 *	Returns:        
 *	Side effects:	fills array
 *                      128 subtracted from each
 *
 *	Date: 930129	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void FillChromBlock(int x_curr, int y_curr, PictImage *image,
            MB_Structure *data)
{
  int n;
  register int m;

  int x, y;

  x = x_curr>>1;
  y = y_curr>>1;

  for (n = 0; n < (MB_SIZE>>1); n++)
    for (m = 0; m < (MB_SIZE>>1); m++) {
      data->Cr[n][m] = 
        (int)(*(image->Cr +x+m + (y+n)*csed_pels));
      data->Cb[n][m] = 
        (int)(*(image->Cb +x+m + (y+n)*csed_pels));
    }
  return;
}


/**********************************************************************
 *
 *	Name:        FillDiffLumBlock
 *	Description:        Fills the luminance of one block of DiffImage
 *	
 *	Input:        Position, pointer to DiffImage, array to fill
 *	Returns:        
 *	Side effects:	fills array
 *
 *	Date: 930129	Author: mikeg@ee.ubc.ca
 *
 ***********************************************************************/

void FillDiffLumBlock( int x, int y, DiffImage *image, MB_Structure *data)
{
  int n;
  register int m;

  for (n = 0; n < MB_SIZE; n++)
    for (m = 0; m < MB_SIZE; m++)
      data->lum[n][m] = 
        (int)(*(image->lum + x+m + (y+n)*sed_pels));
  return;
}

/**********************************************************************
 *
 *	Name:        FillDiffChromBlock
 *	Description:        Fills the chrominance of one block of DiffImage
 *	
 *	Input:        Position, pointer to DiffImage, array to fill
 *	Returns:        
 *	Side effects:	fills array
 *                      128 subtracted from each
 *
 *	Date: 930129	Author: mikeg@ee.ubc.ca
 *
 ***********************************************************************/

void FillDiffChromBlock(int x_curr, int y_curr, DiffImage *image,
            MB_Structure *data)
{
  int n;
  register int m;

  int x, y;

  x = x_curr>>1;
  y = y_curr>>1;

  for (n = 0; n < (MB_SIZE>>1); n++)
    for (m = 0; m < (MB_SIZE>>1); m++) {
      data->Cr[n][m] = 
        (int)(*(image->Cr +x+m + (y+n)*csed_pels));
      data->Cb[n][m] = 
        (int)(*(image->Cb +x+m + (y+n)*csed_pels));
    }
  return;
}



/**********************************************************************
 *
 *	Name:        ZeroMBlock
 *	Description:        Fills one MB with Zeros
 *	
 *	Input:        MB_Structure to zero out
 *	Returns:        
 *	Side effects:	
 *
 *	Date: 940829	Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void ZeroMBlock(MB_Structure *data)
{
  int n;
  register int m;

  for (n = 0; n < MB_SIZE; n++)
    for (m = 0; m < MB_SIZE; m++)
      data->lum[n][m] = 0;
  for (n = 0; n < (MB_SIZE>>1); n++)
    for (m = 0; m < (MB_SIZE>>1); m++) {
      data->Cr[n][m] = 0;
      data->Cb[n][m] = 0;
    }
  return;
}

/**********************************************************************
 *
 *	Name:        ReconImage
 *	Description:	Puts together reconstructed image
 *	
 *	Input:        position of curr block, reconstructed
 *        macroblock, pointer to recontructed image
 *	Returns:
 *	Side effects:
 *
 *	Date: 930123        Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void ReconImage (int i, int j, MB_Structure *data, PictImage *recon)
{
  int n;
  register int m;

  int x_curr, y_curr;

  x_curr = i * MB_SIZE;
  y_curr = j * MB_SIZE;

  /* Fill in luminance data */
  for (n = 0; n < MB_SIZE; n++)
    for (m= 0; m < MB_SIZE; m++) {
      *(recon->lum + x_curr+m + (y_curr+n)*sed_pels) = data->lum[n][m];
    }

  /* Fill in chrominance data */
  for (n = 0; n < MB_SIZE>>1; n++)
    for (m = 0; m < MB_SIZE>>1; m++) {
      *(recon->Cr + (x_curr>>1)+m + ((y_curr>>1)+n)*csed_pels) = data->Cr[n][m];
      *(recon->Cb + (x_curr>>1)+m + ((y_curr>>1)+n)*csed_pels) = data->Cb[n][m];
    }
  return;
}

/**********************************************************************
 *
 *	Name:        ReconDiffImage
 *	Description:	Puts together reconstructed image
 *	
 *	Input:        position of curr block, reconstructed
 *        macroblock, pointer to recontructed image
 *	Returns:
 *	Side effects:
 *
 *	Date: 930123        Author: mikeg@ee.ubc.ca
 *
 ***********************************************************************/

void ReconDiffImage (int i, int j, MB_Structure *data, DiffImage *recon)
{
  int n;
  register int m;

  int x_curr, y_curr;

  x_curr = i * MB_SIZE;
  y_curr = j * MB_SIZE;

  /* Fill in luminance data */
  for (n = 0; n < MB_SIZE; n++)
    for (m= 0; m < MB_SIZE; m++) {
      *(recon->lum + x_curr+m + (y_curr+n)*sed_pels) = data->lum[n][m];
    }

  /* Fill in chrominance data */
  for (n = 0; n < MB_SIZE>>1; n++)
    for (m = 0; m < MB_SIZE>>1; m++) {
      *(recon->Cr + (x_curr>>1)+m + ((y_curr>>1)+n)*csed_pels) = data->Cr[n][m];
      *(recon->Cb + (x_curr>>1)+m + ((y_curr>>1)+n)*csed_pels) = data->Cb[n][m];
    }
  return;
}

/**********************************************************************
 *
 *	Name:        InterpolateImage
 *	Description:    Interpolates a complete image for easier half
 *                      pel prediction
 *	
 *	Input:	        pointer to image structure
 *	Returns:        pointer to interpolated image
 *	Side effects:   allocates memory to interpolated image
 *
 *	Date: 950207        Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/


unsigned char *InterpolateImage(unsigned char *image, int width, int height, int RTYPE)
{
  unsigned char *ipol_image, *ii, *oo;
  int i,j;

  ipol_image = (unsigned char *)malloc(sizeof(char)*width*height*4);
  ii = ipol_image;
  oo = image;

  /* main image */
  for (j = 0; j < height-1; j++) {
    for (i = 0; i  < width-1; i++) {
      *(ii + (i<<1)) = *(oo + i);
      *(ii + (i<<1)+1) = (*(oo + i) + *(oo + i + 1) + 1 - RTYPE)>>1;
      *(ii + (i<<1)+(width<<1)) = (*(oo + i) + *(oo + i + width) + 1 - RTYPE)>>1;
      *(ii + (i<<1)+1+(width<<1)) = (*(oo+i) + *(oo+i+1) + 
         *(oo+i+width) + *(oo+i+1+width) + 2 - RTYPE)>>2;
    }
    /* last sed_pels on each line */
    *(ii+ (width<<1) - 2) = *(oo + width - 1);
    *(ii+ (width<<1) - 1) = *(oo + width - 1);
    *(ii+ (width<<1)+ (width<<1)-2) = (*(oo+width-1)+*(oo+width+width-1)+1-RTYPE)>>1;
    *(ii+ (width<<1)+ (width<<1)-1) = (*(oo+width-1)+*(oo+width+width-1)+1-RTYPE)>>1;
    ii += (width<<2);
    oo += width;
  }

  /* last sed_lines */
  for (i = 0; i < width-1; i++) {
    *(ii+ (i<<1)) = *(oo + i);    
    *(ii+ (i<<1)+1) = (*(oo + i) + *(oo + i + 1) + 1 - RTYPE)>>1;
    *(ii+ (width<<1)+ (i<<1)) = *(oo + i);    
    *(ii+ (width<<1)+ (i<<1)+1) = (*(oo + i) + *(oo + i + 1) + 1 - RTYPE)>>1;
          
  }

  /* bottom right corner sed_pels */
  *(ii + (width<<1) - 2) = *(oo + width -1);
  *(ii + (width<<1) - 1) = *(oo + width -1);
  *(ii + (width<<2) - 2) = *(oo + width -1);
  *(ii + (width<<2) - 1) = *(oo + width -1);

  return ipol_image;
}


/**********************************************************************
 *
 *	Name:          MotionEstimatePicture
 *	Description:   Finds integer and half pel motion estimation
 *                     and chooses 8x8 or 16x16 
 *	
 *	Input:	       current image, previous image, interpolated
 *                     reconstructed previous image, seek_dist,
 *                     motion vector array
 *	Returns:       
 *	Side effects:  Allocates memory for MV structure
 *
 *	Date: 950209   Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void MotionEstimatePicture( unsigned char *curr, unsigned char *prev,
                            unsigned char *next, unsigned char *prev_ipol, 
                            unsigned char *next_ipol, int seek_dist, 
                            MotionVector *MV[7][MBR+1][MBC+2], 
                            int gobsync, int estimation_type)                      
{
  int i,j,k;
  int pmv0,pmv0b,pmv1,pmv1b,xoff,yoff, xoffb, yoffb;
  int curr_mb[16][16];
  int sad8 = INT_MAX, sad16, sad16b, sad0, sad0b;
  int newgob;
  MotionVector *f0,*f1,*f2,*f3,*f4,*fBack;

  /* Do motion estimation and store result in array */
  for ( j = 0; j < sed_lines/MB_SIZE; j++) 
  {
    newgob = 0;
    if (gobsync && j%gobsync == 0) 
    {
      newgob = 1;
    }

    for ( i = 0; i < sed_pels/MB_SIZE; i++) 
    {
      /* Integer pel search */
      f0 = MV[0][j+1][i+1];
      f1 = MV[1][j+1][i+1];
      f2 = MV[2][j+1][i+1];
      f3 = MV[3][j+1][i+1];
      f4 = MV[4][j+1][i+1];
      fBack = MV[5][j+1][i+1];

      /* P-picture or PB-picture prediction. */
      if (B_PICTURE_ESTIMATION != estimation_type)
      {
        /* Here the PMV's are found using integer motion vectors for
         * the forward prediction only. */
        FindPMV(MV,i+1,j+1,&pmv0,&pmv1,0,newgob,0);

        if (long_vectors && (estimation_type != PB_PICTURE_ESTIMATION) )
        {
          /* Always divisable by two. */
          xoff = pmv0/2;
          yoff = pmv1/2;
        }
        else 
        {
          xoff = yoff = 0;
        }
      
        MotionEstimation( curr, prev, i*MB_SIZE, j*MB_SIZE, 
                          xoff, yoff, seek_dist, MV,
                          &sad0,estimation_type);

        sad16 = f0->min_error;

        if (use_4mv)
        {
          sad8 = f1->min_error + f2->min_error + f3->min_error + f4->min_error;
        }

        f0->Mode = ChooseMode(curr,i*MB_SIZE,j*MB_SIZE, mmin(sad8,sad16));

        /* Half pel search. Forward */
        if (f0->Mode != MODE_INTRA) 
        {
          FindMB(i*MB_SIZE,j*MB_SIZE ,curr, curr_mb);
          FindHalfPel(i*MB_SIZE,j*MB_SIZE,f0, prev_ipol, &curr_mb[0][0],16,0);
          sad16 = f0->min_error;

          if (use_4mv) 
          {
            FindHalfPel(i*MB_SIZE,j*MB_SIZE,f1, prev_ipol, &curr_mb[0][0],8,0);
            FindHalfPel(i*MB_SIZE,j*MB_SIZE,f2, prev_ipol, &curr_mb[0][8],8,1);
            FindHalfPel(i*MB_SIZE,j*MB_SIZE,f3, prev_ipol, &curr_mb[8][0],8,2);
            FindHalfPel(i*MB_SIZE,j*MB_SIZE,f4, prev_ipol, &curr_mb[8][8],8,3);

            sad8 = f1->min_error +f2->min_error +f3->min_error +f4->min_error;
            sad8 += PREF_16_VEC;
          
            /* Choose Zero Vector, 8x8 or 16x16 vectors */
            if (sad0 < sad8 && sad0 < sad16) 
            {
              f0->x = f0->y = 0;
              f0->x_half = f0->y_half = 0;
            }
            else 
            {
              if (sad8 < sad16) 
                 f0->Mode = MODE_INTER4V;
            }
          }
          else 
          {
            /* Choose Zero Vector or 16x16 vectors */
            if (sad0 < sad16) 
            {
              f0->x = f0->y = 0;
              f0->x_half = f0->y_half = 0;
            }
          }
        }
        else
        { 
          for (k = 0; k < 5; k++)
          {
            ZeroVec(MV[k][j+1][i+1]);
          }
        }
      }
      /* B-Picture. */
      else
      {
        /* Here the PMV's are found using integer motion vectors 
         * for both backward and forward prediction. */
        FindPMV(MV,i+1,j+1,&pmv0,&pmv1,0,newgob,0);
        FindPMV(MV,i+1,j+1,&pmv0b,&pmv1b,5,newgob,0);
      
        if (long_vectors && (estimation_type != PB_PICTURE_ESTIMATION) )
        {
          /* Always divisable by two. */
          xoff = pmv0/2;
          yoff = pmv1/2;
          xoffb = pmv0b/2;
          yoffb = pmv1b/2;
        }
        else 
        {
          xoff = yoff = xoffb = yoffb = 0;
        }

        /* Forward prediction. */
        MotionEstimation( curr, prev, i*MB_SIZE, j*MB_SIZE, 
                          xoff, yoff, seek_dist, MV,
                          &sad0, estimation_type);

        sad16 = f0->min_error;

        /* Backward prediction. */
        MotionEstimationBackward( curr, next, i*MB_SIZE, j*MB_SIZE, 
                                  xoffb, yoffb, seek_dist, MV,
                                  &sad0b);

        sad16b = fBack->min_error;

        FindMB(i*MB_SIZE, j*MB_SIZE, curr, curr_mb);
        FindHalfPel(i*MB_SIZE,j*MB_SIZE, f0, prev_ipol, &curr_mb[0][0],16,0);
        sad16 = f0->min_error;

        /* Choose Zero Vector or 16x16 vectors */
        if (sad0 < sad16) 
        {
          f0->x = f0->y = 0;
          f0->x_half = f0->y_half = 0;
        }
       
        FindHalfPel(i*MB_SIZE,j*MB_SIZE, fBack, next_ipol, &curr_mb[0][0],16,0);
        sad16b = fBack->min_error;

        /* Choose Zero Vector or 16x16 vectors */
        if (sad0b < sad16b) 
        {
          fBack->x = fBack->y = 0;
          fBack->x_half = fBack->y_half = 0;
        }
      }
    }
  }

#ifdef PRINTMV
  fprintf(stdout,"Motion estimation\n");
  fprintf(stdout,"16x16 vectors:\n");

  for ( j = 0; j < sed_lines/MB_SIZE; j++) 
  {
    for ( i = 0; i < sed_pels/MB_SIZE; i++) 
    {
      /* B picture. */
      if (B_PICTURE_ESTIMATION == estimation_type)
      {
        /* Forward MV. */
        fprintf(stdout," forward (B): %3d%3d",
        2*MV[0][j+1][i+1]->x + MV[0][j+1][i+1]->x_half,
        2*MV[0][j+1][i+1]->y + MV[0][j+1][i+1]->y_half);
        fprintf(stdout," backward (B): %3d%3d",
        2*MV[5][j+1][i+1]->x + MV[5][j+1][i+1]->x_half,
        2*MV[5][j+1][i+1]->y + MV[5][j+1][i+1]->y_half);
      }
      /* P picture. */
      else if (MV[0][j+1][i+1]->Mode != MODE_INTRA) 
      {
        fprintf(stdout," %3d%3d",
        2*MV[0][j+1][i+1]->x + MV[0][j+1][i+1]->x_half,
        2*MV[0][j+1][i+1]->y + MV[0][j+1][i+1]->y_half);
      }
      else
      {
        fprintf(stdout,"  .  . ");
      }
    }
    fprintf(stdout,"\n");
  }

  if (use_4mv) 
  {
    fprintf(stdout,"8x8 vectors:\n");
    for (k = 1; k < 5; k++) 
    {
      fprintf(stdout,"Block: %d\n", k-1);
      for ( j = 0; j < sed_lines/MB_SIZE; j++) 
      {
        for ( i = 0; i < sed_pels/MB_SIZE; i++) 
	      {
          if (MV[0][j+1][i+1]->Mode != MODE_INTRA)
            fprintf(stdout," %3d%3d",
            2*MV[k][j+1][i+1]->x + MV[k][j+1][i+1]->x_half,
            2*MV[k][j+1][i+1]->y + MV[k][j+1][i+1]->y_half);
          else
            fprintf(stdout,"  .  . ");
        }
        fprintf(stdout,"\n");
      }
    }
  }
#endif
  return;
}


/**********************************************************************
 *
 *	Name:        MakeEdgeImage
 *	Description:    Copies edge sed_pels for use with unrestricted
 *                      motion vector mode
 *	
 *	Input:	        pointer to source image, destination image
 *                      width, height, edge
 *	Returns:       
 *	Side effects:
 *
 *	Date: 950219        Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void MakeEdgeImage(unsigned char *src, unsigned char *dst, int width,
           int height, int edge)
{
  int i,j;
  unsigned char *p1,*p2,*p3,*p4;
  unsigned char *o1,*o2,*o3,*o4;

  /* center image */
  p1 = dst;
  o1 = src;
  for (j = 0; j < height;j++) {
    memcpy(p1,o1,width);
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


/**********************************************************************
 *
 *	Name:        Clip
 *	Description:    clips recontructed data 0-255
 *	
 *	Input:	        pointer to recon. data structure
 *	Side effects:   data structure clipped
 *
 *	Date: 950718        Author: Karl.Lillevold@nta.no
 *
 ***********************************************************************/

void Clip(MB_Structure *data)
{
  int m,n;

  for (n = 0; n < 16; n++) {
    for (m = 0; m < 16; m++) {
      data->lum[n][m] = mmin(255,mmax(0,data->lum[n][m]));
    }
  }
  for (n = 0; n < 8; n++) {
    for (m = 0; m < 8; m++) {
      data->Cr[n][m] = mmin(255,mmax(0,data->Cr[n][m]));
      data->Cb[n][m] = mmin(255,mmax(0,data->Cb[n][m]));
    }
  }
}

/**********************************************************************
 *
 *	Name:           StoreDirectModeVectors
 *	Description:    Stores info for P-P MVs, to be used with direct 
 *			mode prediction in true B pictures.
 *	
 *	Input:		MV data
 *	Side effects:   allocates memory for 1-D MV structs.
 *
 *      Return:         
 *
 *	Date:970926	Michael Gallant <mikeg@ee.ubc.ca>
 *
 ***********************************************************************/

void StoreDirectModeVectors(MotionVector *ppMV[7][MBR+1][MBC+2], MotionVector *directMV[MBR][MBC])
{
  int x_avg, x_half_avg, y_avg, y_half_avg, min_error;
  int i, j;

  for (j=0; j<(sed_lines>>4)+1; ++j)
  {
    for (i=0; i<(sed_pels>>4)+2; ++i)
    {
      directMV[j][i] = (MotionVector *)malloc(sizeof(MotionVector) );
    }
  }

  for ( j = 0; j < sed_lines/MB_SIZE; j++) 
  {
    for ( i = 0; i < sed_pels/MB_SIZE; i++) 
    {
      /* Must store all forward MVs found for the P-picture, to use with true B 
       * Direct Mode prediction. */
 
      if ( MODE_INTER4V == ppMV[0][j+1][i+1]->Mode ||   
	   MODE_INTER4V_Q == ppMV[0][j+1][i+1]->Mode)
      {
        x_avg =  ( ppMV[1][j+1][i+1]->x + ppMV[2][j+1][i+1]->x +
                   ppMV[3][j+1][i+1]->x + ppMV[4][j+1][i+1]->x + 2) >> 2;
        directMV[j][i]->x = x_avg;
        x_half_avg = ( ppMV[1][j+1][i+1]->x_half + ppMV[2][j+1][i+1]->x_half + 
                       ppMV[3][j+1][i+1]->x_half + ppMV[4][j+1][i+1]->x_half + 
                       2) >> 2;
        directMV[j][i]->x_half = x_half_avg;
        y_avg =  ( ppMV[1][j+1][i+1]->y + ppMV[2][j+1][i+1]->y +
                   ppMV[3][j+1][i+1]->y + ppMV[4][j+1][i+1]->y + 2) >> 2;
        directMV[j][i]->y = y_avg;
        y_half_avg = ( ppMV[1][j+1][i+1]->y_half + ppMV[2][j+1][i+1]->y_half + 
                       ppMV[3][j+1][i+1]->y_half + ppMV[4][j+1][i+1]->y_half + 
                       2) >> 2;
        directMV[j][i]->y_half = y_half_avg;
        min_error =  ppMV[1][j+1][i+1]->min_error + ppMV[2][j+1][i+1]->min_error +
                     ppMV[3][j+1][i+1]->min_error + ppMV[4][j+1][i+1]->min_error;
        directMV[j][i]->min_error = min_error;
      }
      else
      {
        directMV[j][i]->x =         ppMV[0][j+1][i+1]->x;
        directMV[j][i]->x_half =    ppMV[0][j+1][i+1]->x_half;
        directMV[j][i]->y =         ppMV[0][j+1][i+1]->y;
        directMV[j][i]->y_half =    ppMV[0][j+1][i+1]->y_half;
        directMV[j][i]->min_error = ppMV[0][j+1][i+1]->min_error;
      }           
    }
  }
  return;
}

void InitializeMV(MotionVector *MV[7][MBR+1][MBC+2])
{
  int i,j,k;

  for (j=0; j<(sed_lines>>4)+1; ++j)
  {
    for (i=0; i<(sed_pels>>4)+2; ++i)
    {
      for (k=0; k<7; ++k)
      {
        MV[k][j][i] = (MotionVector *)malloc(sizeof(MotionVector));
      }
    }
  }
}

/**********************************************************************
 *
 *	Name:           GenerateFrameAndInterpolatedImages
 *	Description:    
 *
 *	Input:		reconstructed image
 *	Side effects:   sets out_recon_ipol, out_recon via 
 *                      recon_interpolated and recon_frame
 *      Return:         
 *
 *	Date:970926     Michael Gallant <mikeg@ee.ubc.ca>
 *
 ***********************************************************************/
void GenerateFrameAndInterpolatedImages(PictImage *in_recon, Pict *pic, 
                                        unsigned char **out_recon_ipol, 
                                        PictImage **out_recon, 
                                        unsigned char **recon_interpolated,
                                        PictImage **recon_frame)
{

  int B;
  unsigned char *recon_lum_frame = NULL;

  /* interpolate image */
  if (mv_outside_frame) 
  {
    if (long_vectors) 
    {
      /* If the Extended Motion Vector range is used, motion vectors
         may point further out of the picture than in the normal range,
         and the edge images will have to be made larger */
      B = 16;
    }
    else 
    {
      /* normal range */
      B = 8;
    }

    /* pi_edge stores an edge_image, luminance component only. */
    recon_lum_frame = (unsigned char *)malloc(sizeof(char)*(sed_pels+4*B)*(sed_lines+4*B));
    if (recon_lum_frame == NULL) 
    {
      fprintf(stderr,"Couldn't allocate memory for recon_lum_frame\n");
      exit(-1);
    }

    /* Make previous reconstructed picture's luminance into an edge
     * image. */
    MakeEdgeImage(in_recon->lum,recon_lum_frame + (sed_pels + 4*B)*2*B+2*B,sed_pels,sed_lines,2*B);

    /* pi stores the interpolated previous reconstructed edge
     * luminance image. */
    *(recon_interpolated) = InterpolateImage(recon_lum_frame, sed_pels+4*B, sed_lines+4*B, 
                                          pic->RTYPE);
    free(recon_lum_frame);

    /* prev_ipol points to position in pi where (0,0) of
     * original resolution would be. */
    *(out_recon_ipol) = *(recon_interpolated) + (2*sed_pels + 8*B) * 4*B + 4*B; 
    
    /* Prepare to store an edge image, including luminance AND 
     * chrominance components. */
    *(recon_frame) = InitImage((sed_pels+4*B)*(sed_lines+4*B));
    
    /* Make edge image of non-interpolated, reconstructed,
     * previous image, luminace and chrominance components. */
    MakeEdgeImage(in_recon->lum,(*recon_frame)->lum + (sed_pels+4*B)*2*B + 2*B,sed_pels,sed_lines,2*B);
    MakeEdgeImage(in_recon->Cr,(*recon_frame)->Cr + (sed_pels/2 + 2*B)*B + B,sed_pels/2,sed_lines/2,B);
    MakeEdgeImage(in_recon->Cb,(*recon_frame)->Cb + (sed_pels/2 + 2*B)*B + B,sed_pels/2,sed_lines/2,B);
    
    (*out_recon) = (PictImage *)malloc(sizeof(PictImage));
    (*out_recon)->lum = (*recon_frame)->lum + (sed_pels + 4*B)*2*B + 2*B;
    (*out_recon)->Cr = (*recon_frame)->Cr + (sed_pels/2 + 2*B)*B + B;
    (*out_recon)->Cb = (*recon_frame)->Cb + (sed_pels/2 + 2*B)*B + B;
  }
  else 
  {
    /* P-pictures. */
    *(recon_interpolated) = InterpolateImage(in_recon->lum,sed_pels,sed_lines,pic->RTYPE);
    *(out_recon_ipol) = *(recon_interpolated);
    *(out_recon) = in_recon;
  }
}

int Get_restricted_MQ(int dquant,int QP_prev)
{
  int new_dquant;

  new_dquant=dquant;

  /*recalculate dquant*/

  if (QP_prev == 31)
  {
    if (dquant <= -5)
      new_dquant=-5;
    else
      new_dquant=-3;
  }
  else
  if (QP_prev == 30)
  {
    if (dquant <= 0)
      new_dquant=-3;
    else
      new_dquant=1;
  }
  else
  if (QP_prev == 29)
  {
    if (dquant <= 0)
      new_dquant=-3;
    else
      new_dquant=2;
  }
  else
  if (QP_prev >= 21)
  {
    if (dquant <= 0)
      new_dquant=-3;
     else
      new_dquant=3;
   }
   else
   if (QP_prev >= 11)
   {
     if (dquant <= 0)
       new_dquant=-2;
     else
       new_dquant=2;
   }
   else
   if (QP_prev >= 2)
   {
     if (dquant <= 0)
       new_dquant=-1;
     else
       new_dquant=1;
   }
   else
   {
     if (dquant <= 1)
       new_dquant=1;
     else
       new_dquant=2;
   }

   return new_dquant;
}
