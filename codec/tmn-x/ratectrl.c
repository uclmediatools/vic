 /************************************************************************
 *
 *  ratectrl.c, part of tmn (TMN encoder)
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


/* IMPORTANT NOTE:
 * 
 * The H.263 standard does not specify a rate control method.  Each H.263
 * encoder has to implement a rate control suited for what the encoder is
 * going to be used for. This software now includes three rate control
 * methods: (i) the rate control from the TMN8 document (Macroblock layer), 
 * (ii) TMN-5 rate control (section 8 of tmn-8 document)
 * and (iii) the original rate control method (OFFLINE_RATE_CONTROL) 
 * The default rate control is (i)  which is suitable for
 * low-delay teleconferencing.
 * Rate control option is available on the command line with
 * -C n where n=3 for (i), n=2 for (ii) and n=1 for (iii)
 */

#include "sim.h"
/*----------- OFFLINE rate control routines --------------------*/

/* ABOUT THE OFFLINE RATE CONTROL:
 * 
 * If the OFFLINE_RATE_CONTROL is used, you will get the
 * same rate control as was used to generate the MPEG-4 anchors. This rate
 * control does not skip any extra pictures after the first frame, and it
 * uses a fixed frame rate. It is possible to start the rate control after
 * a certain percentage of the sequence has been encoded with a fixed
 * quantization parameter. Its purpose is to achieve the target bitrate as
 * a mean bitrate for the whole sequence. In other words, it is a rate
 * control method optimized for offline compression.
 * 
 * If you use the offline rate control, you will risk not achieving the
 * target rate under one or more of the following conditions :
 * 
 * (i)   too high frame rate (ii)  too low start value for the quantization
 * parameter (iii) the rate control is started too late (iv)  the sequence
 * encoded is too short
 * 
 */

#include <stdio.h>
#include <math.h>

/**********************************************************************
 *
 *	Name:	        FrameUpdateQP
 *	Description:    updates quantizer once per frame for
 *                      simplified rate control
 *
 *      Returns:        new quantizer
 *	Side effects:
 *
 *	Date: 950910	Author: <klillevo@mailbox.jf.intel.com>
 *
 ***********************************************************************/

int FrameUpdateQP (int buf, int bits, int frames_left, int QP, int B,
                    float seconds)
{
  int newQP, dQP;
  float buf_rest, buf_rest_pic;

  buf_rest = seconds * B - (float) buf;

  newQP = QP;

  if (frames_left > 0)
  {
    buf_rest_pic = buf_rest / (float) frames_left;
    
    printf ("\n");
    printf ("  Simplified rate control for %d remaining pictures:\n",
            frames_left);
    printf ("  Bits spent / left       : %8d / %d (%d per picture)\n",
            buf, mnint (buf_rest), mnint (buf_rest_pic));

    dQP = mmax (1, (int) (QP * 0.1));

    printf ("  Limits                  : %8.0f / %.0f\n",
            buf_rest_pic / 1.15, buf_rest_pic * 1.15);
    printf ("  Bits spent on last frame: %8d\n", bits);

    if (bits > buf_rest_pic * 1.15)
    {
      newQP = mmin (31, QP + dQP);
      printf ("  QP -> new QP            : %2d -> %2d\n", QP, newQP);
    } else if (bits < buf_rest_pic / 1.15)
    {
      newQP = mmax (1, QP - dQP);
      printf ("  QP -> new QP            : %2d -> %2d\n", QP, newQP);
    } else
    {
      printf ("  QP not changed\n");
    }
  }
  printf ("\n");
  return newQP;
}



/*
 * These routines are needed for the low-delay , variable frame rate, rate
 * control specified in the TMN8 document (Frame and Macroblock layer rate control)
 * 
 */

#include <math.h>

/* rate control static variables */

static float K = (float) 0.5;    /* Model Parameters */
static float C = (float) 0.0;
static float S,L;
static float K_prev, K_prev_avg = 0.0;
static float C_prev;
static float C_est, K_est, K_avg, C_avg;

static float B_prev;            /* number of bits spent for the previous
                                 * frame */
static float B_target;          /* target number of bits/picture               */
static float global_adj;        /* due to bits spent for the previous
                                 * frame    */
static int N, N_frame;          /* number of macroblocks in a frame */
static int K_count;
static int MBN;
static float *var,*dev;
static float *alpha;
static int parameter_count;

/* tmn-7 rate control initialization */
void InitializeRateControl ()
{
  B_prev = (float) 0.0;
}

/* tmn-7 rate control update */
void UpdateRateControl(int bits)
{
    B_prev = (float) bits;
}
/**********************************************************************
 *
 *	Name:	        InitializeRateControlMB
 *	Description:    Initialize macroblock layer Rate Control 
 *                      and calculate the Variance of a frame
 *                      tmn-8 rate control 
 *
 *      Returns:        
 *	Side effects:
 *
 *	Date: 970922	Author: Guy Cote  <guyc@ee.ubc.ca>
 *
 ***********************************************************************/

void InitializeRateControlMB(PictImage *curr,float bit_rate, float target_frame_rate, MotionVector * MV[7][MBR + 1][MBC + 2])
{
  int             i, j, k, l;
  float    ep, s, avg, tmp_var;
  float  delta;
  float  A = (float) 0.1;   /* delay requirements */
  static int first_frame = 0;
  
  /* First set the target bit rate for the picture   *
   * taking into account the current buffer fullness *
   * and the delay requirement (see tmn 8)           */
  delta = (CommBacklog > A * bit_rate / target_frame_rate) ? 
          (float) CommBacklog / target_frame_rate : CommBacklog - A * bit_rate / target_frame_rate;
  B_target = (float) ((bit_rate / target_frame_rate) - delta);
 
  MBN = 1;
 
  (first_frame == 0) ? (K_prev = 0.5) : (K_prev = K_prev_avg);
  first_frame = 1;
  
  C_prev=C;
  K_avg = C_avg = (float) 0.0;
  K_count = 0;
  N = N_frame = (sed_lines>>4) * (sed_pels>>4);
  dev = (float *)calloc((sed_pels>>4)*(sed_lines>>4), sizeof(float));
  alpha = (float *)calloc((sed_pels>>4)*(sed_lines>>4), sizeof(float));
  var = (float *)calloc((sed_pels>>4)*(sed_lines>>4), sizeof(float));
  parameter_count = 0;
  



  /* Calculate the variance of the frame */
  for (k=0;k<(sed_lines>>4);k++)
  { 
    for (l=0;l<(sed_pels>>4);l++)
    {
      s = ep = tmp_var = avg = 0;
      /* first pass to get the mean */
      for (j=0;j<16;j++)   
        for (i = 0; i < 16; i++) 
          avg += curr->lum[sed_pels*j+l*16 + i+k*sed_pels*16]; 
      for (j = 0;j < 8; j++) 
        for (i = 0; i < 8; i++)
        {
          avg += curr->Cr[(sed_pels>>1)*j+l*8 + i+k*(sed_pels>>1)*8];  
          avg += curr->Cb[(sed_pels>>1)*j+l*8 + i+k*(sed_pels>>1)*8]; 
        }
      avg /= 384;
      /* get the second moment */
      for (j = 0; j < 16; j++) 
        for (i = 0; i < 16; i++)
        {
          /* lum */
          s = (float) abs( (int) (curr->lum[sed_pels*j+l*16 + i+k*sed_pels*16] - avg));
          ep += s;
          tmp_var += s*s;
        }
      for (j = 0;j < 8; j++) 
        for (i = 0; i < 8; i++)
        {
          /* Cr */
          s = (float) abs( (int) (curr->Cr[(sed_pels>>1)*j+l*8 + i+k*(sed_pels>>1)*8] - avg));
          ep += s;
          tmp_var += s*s;
          /* Cb */
          s = (float) abs( (int) (curr->Cb[(sed_pels>>1)*j+l*8 + i+k*(sed_pels>>1)*8] - avg));
          ep += s;
          tmp_var += s*s; 
        }
 
        /* get the variance (according to the corrected two pass *
         * algorithm) and divide variance by 3 if Intra coded    */
         var[l+k*(sed_pels>>4)] = (MV[0][k][l]->Mode == MODE_INTRA) ? ((tmp_var-ep*ep/384)/(383))/3 :
                              (tmp_var-ep*ep/384)/(383);
      
        /* Initialize parameter S */
        dev[l+k*(sed_pels>>4)] = (float) sqrt(var[l+k*(sed_pels>>4)]);
        alpha[l+k*(sed_pels>>4)] = ((B_target/256)/N_frame < 0.5) ? 
          B_target/(128*N_frame) * (1-dev[l+k*(sed_pels>>4)]) + dev[l+k*(sed_pels>>4)] : 1; 
        S += alpha[l+k*(sed_pels>>4)] * dev[l+k*(sed_pels>>4)];
    }
  }

}


/**********************************************************************
 *
 *	Name:	        Compute_QP
 *	Description:    compute quantizer of a macroblock
 *
 *      Returns:        Optimized quantizer for the macroblock 
 *	Side effects:
 *
 *	Date: 970922	Author: Guy Cote  <guyc@ee.ubc.ca>
 *
 ***********************************************************************/

int Compute_QP(int xpos,int ypos)
{
   int newQP;

   if ((L=(B_target - 256 * N * C))<=0) newQP = 31;
   else 
   {
     newQP = (int) (sqrt( (256 * K * (float) dev[xpos+ypos * (sed_pels>>4)] * S) / (L * alpha[xpos+ypos * (sed_pels>>4)]) ) + 1)/2;
     newQP = mmax (1, mmin (31, newQP));
   }

#if 0
  fprintf(stdout,"\nQP(%3d): %3d K: %5.4f B_target: %6.0f dev: %5.2f alpha: %4.3f  L: %5.0f C: %5.4f, S: %5.0f",xpos+ypos * (sed_pels>>4), newQP, K, B_target, dev[xpos+ypos * (sed_pels>>4)], alpha[xpos+ypos * (sed_pels>>4)], L, C, S );
#endif

   return newQP;
}

/**********************************************************************
 *
 *	Name:	          UpdateRateControlMB
 *	Description:    update counters and rate control parameters for the
 *                  macroblock
 *
 *      Returns:     
 *	Side effects:   
 *
 *	Date: 970922	Author: Guy Cote  <guyc@ee.ubc.ca>
 *
 ***********************************************************************/

void UpdateRateControlMB(int bitsMB, int bitsCoeff, int xpos, int ypos, int QP)
{

    /* Update counters */
    B_target -= bitsMB;
    S -= alpha[xpos+ypos*(sed_pels>>4)] * (float) dev[xpos+ypos*(sed_pels>>4)];
    N--;
    parameter_count++;
    
    /* Update model parameters */
    K_est = ((float) bitsCoeff * (2*QP)*(2*QP)) / (var[xpos+ypos*(sed_pels>>4)] * 256);
    C_est = ((float) bitsMB - (float) bitsCoeff) / 256;
    
    if (K_est>=0 && K_est <= 4.53236014) 
    {
      K_count++;
      K_avg = K_avg*((float) K_count-1)/K_count + K_est/K_count; 
    } 
    C_avg = C_avg*((float) MBN-1)/(MBN) + C_est/(MBN);
    
    /* The updates are a weighted average of the initial estimates *
     * and the current averages */ 
    K = K_avg*((float) MBN/N_frame) + K_prev*(N_frame - MBN)/N_frame;
    C = C_avg*((float) MBN/N_frame) + C_prev*(N_frame - MBN)/N_frame;
    
    /* keep an average of K to be used at the start of the next frame */
    K_prev_avg =  K_prev_avg*((float) (MBN-1)/N_frame) + K*(N_frame - (MBN-1))/N_frame;   
    MBN++;

}

int InitializeQuantizer (int pict_type, float bit_rate,
                          float target_frame_rate, float QP_mean)

/* QP_mean = mean quantizer parameter for the previous picture */
/* bitcount = current total bit count                          */
/* To calculate bitcount in coder.c, do something like this :  */
/* int bitcount;                                               */
/* AddBitsPicture(bits);                                       */
/* bitcount = bits->total;                                     */

{
  int newQP;

  if (pict_type == PCT_INTER || pict_type == PCT_IPB)
  {

    B_target = bit_rate / target_frame_rate;

    /* compute picture buffer descrepency as of the previous picture */

    if (B_prev != 0.0)
    {
      global_adj = (B_prev - B_target) / (2 * B_target);
    } else
    {
      global_adj = (float) 0.0;
    }
    newQP = (int) (QP_mean * (1 + global_adj) + (float) 0.5);
    newQP = mmax (1, mmin (31, newQP));
  } else if (pict_type == PCT_INTRA)
  {
    fprintf (stderr, "No need to call InititializeQuantizer() for Intra picture\n");
    exit (-1);
  } else
  {
    fprintf (stderr, "Error (InitializePictureRate): picture type unkown.\n");
    exit (-1);
  }
#if 1
  printf ("Global adj = %.2f\n", global_adj);
  printf ("meanQP = %.2f   newQP = %d\n", QP_mean, newQP);
#endif
  fprintf (stdout, "Target no. of bits: %.2f\n", B_target);

  return newQP;
}


/*********************************************************************
*   Name:          UpdateQuantizer
*
*
* Description: This function generates a new quantizer step size based
*                  on bits spent up until current macroblock and bits
*                  spent from the previous picture.  Note: this
*                  routine should be called at the beginning of each
*                  macroblock line as specified by TMN4. However, this
*                  can be done at any macroblock if so desired.
*
*  Input: current macroblock number (raster scan), mean quantizer
*  paramter for previous picture, bit rate, source frame rate,
*  hor. number of macroblocks, vertical number of macroblocks, total #
*  of bits used until now in the current picture.
*
*  Returns: Returns a new quantizer step size for the use of current
*  macroblock Note: adjustment to fit with 2-bit DQUANT should be done
*  in the calling program (unless Annex T is used)
*
*  Side Effects:
*
*  Date: 1/5/95    Author: Anurag Bist
*  
*
**********************************************************************/


int UpdateQuantizer (int mb, float QP_mean, int pict_type, float bit_rate,
                      int mb_width, int mb_height, int bitcount)

/* mb = macroblock index number */
/* QP_mean = mean quantizer parameter for the previous picture */
/* bitcount = total # of bits used until now in the current picture */

{
  int newQP = 16;
  float local_adj, descrepency, projection;

  if (pict_type == PCT_INTRA)
  {
    newQP = 16;
  } else if (pict_type == PCT_INTER || pict_type == PCT_IPB)
  {
    /* compute expected buffer fullness */

    projection = mb * (B_target / (mb_width * mb_height));

    /* measure descrepency between current fullness and projection */
    descrepency = (bitcount - projection);

    /* scale */

    local_adj = 12 * descrepency / bit_rate;

#if 0
    printf ("mb = %d\n", mb);
    printf ("bit_count = %d projection = %.2f \n", bitcount, projection);
    printf ("B_target = %.2f local_adj = %.2f \n", B_target, local_adj);
#endif

    newQP = (int) (QP_mean * (1 + global_adj + local_adj) + 0.5);

    /* the update equation for newQP in TMN4 document section 3.7 */

  } else
  {
    fprintf (stderr, "Error (UpdateQuantizer): picture type unkown.\n");
  }

#if 0
  printf ("mb = %d  newQP = %d \n", mb, newQP);
#endif

  newQP = mmax (1, mmin (31, newQP));
  return newQP;
}










