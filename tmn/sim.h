/************************************************************************
 *
 *  sim.h, part of (TMN encoder)
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

/**********************************************************************
 *
 * Headerfile for TMN coder
 * Type definitions and declaration of functions 
 * Date last modified: every now and then
 *
 **********************************************************************/

/* If you are not using the included Makefile, or want to override
   the Makefile, you can uncomment one or more of the defines 
   below instead */
/* #define PRINTMV */
/* to print MVs to stdout while coding. */
/* #define PRINTQ */
/* #define PRINTBLOCKS */
/* to print Blocks in advanced intra coding while coding */
/* to print the quantizer used during coding */
/* #define FASTIDCT */
/* for a fast single precision IDCT. */
/* #define QCIF */
/* to change the coding format uncommment the above line and change to
   SQCIF, QCIF, CIF, CIF4, or CIF16 */
/* #define RESTRICTED_MQ */ 
/* restrict dquant so that when the modified quantization mode is on, quant only us
es 2 bits*/
/*#define FULLSEARCH*/
/* to use the full search motion estimation */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include "config.h"
#include"macros.h"

#if 0
#define MALLOC_DEBUG_ON 1
#define malloc(x) debug_malloc(x)
#define free(x)   debug_free(x)
extern void *debug_malloc(int size);
extern void  debug_free(char *data);
extern int   malloc_count;
#endif

/* This should not be changed */
#define MB_SIZE 16
#define BLOCK_SIZE 8
#define BLOCKS_MB 6

/* If someone forgets to define a coding format, QCIF is default */
/*#if (!defined SQCIF && !defined QCIF && !defined CIF && !defined CIF4 && !defined CIF16)
#define QCIF
#endif*/

/* The coding format is decided here. */
/*$#ifdef SQCIF
#define DEF_PELS 128
#define DEF_LINES 96
#endif
#ifdef QCIF
#define DEF_PELS 176
#define DEF_LINES 144
#endif
#ifdef CIF
#define DEF_PELS 352
#define DEF_LINES 288
#endif
#ifdef CIF4
#define DEF_PELS 704
#define DEF_LINES 576
#endif
#ifdef CIF16
#define DEF_PELS 1408
#define DEF_LINES 1152
#endif$*/

/*$#define MBC   DEF_PELS/MB_SIZE
#define MBR   DEF_LINES/MB_SIZE$*/
#define MBC   88
#define MBR   72

/* Parameters from TMN */
#define PREF_NULL_VEC 100
#define PREF_16_VEC 200
#define PREF_PBDELTA_NULL_VEC 50

int headerlength; /* Global variables */
int pels, enhancement_pels, base_pels;
int cpels;
int lines, enhancement_lines, base_lines;
int trace;
int adv_pred;
int use_4mv;
int overlapping_MC;
int syntax_arith_coding;
int pb_frames;
int improved_pb_frames;
int mv_outside_frame;
int long_vectors;
float target_framerate;
FILE *tf;
/* new variables for H.263+ modes */
int advanced_intra_coding;
int deblocking_filter;
int slice_structure;
int independently_segmented_decoding;
int alternative_inter_vlc;
int modified_quantization;
int scalability_mode;
int enhancement_layer_quant;
int reference_picture_resampling;
int reduced_resolution_update;
int successive_B_frames;
int b_picture_quant;
int TRP;
int rate_control_method;
int coded_map[MBR+1][MBC+1];
/* coded_map = 0 -> not coded   *
 * coded_map = 1 -> inter coded *
 * coded_map = 2 -> intra coded */
int quant_map[MBR+1][MBC+1];

int EPTYPE;
int CommBacklog;  /* Rate control buffer */

#define PSC        1
#define PSC_LENGTH        17

#define ESCAPE                          7167

#define PCT_INTRA                       0
#define PCT_INTER                       1
#define PCT_IPB                         2
#define PCT_B                           3
#define PCT_EI                          4
#define PCT_EP                          5
#define PCT_PB							            6

#define ON                              1
#define OFF                             0

/* estimation types. */
#define P_PICTURE_ESTIMATION            0
#define PB_PICTURE_ESTIMATION           1
#define B_PICTURE_ESTIMATION            2
#define EI_EP_PICTURE_ESTIMATION        3

#define SF_SQCIF                        1  /* 001 */
#define SF_QCIF                         2  /* 010 */
#define SF_CIF                          3  /* 011 */
#define SF_4CIF                         4  /* 100 */
#define SF_16CIF                        5  /* 101 */
#define SF_CUSTOM                       6  /* 110 */
#define SF_EPTYPE                       7  /* 111 Extended PTYPE */

/* PB frames modes */
#define PB_FRAMES                       1
#define IM_PB_FRAMES                    2

/* Prediction modes for PB-frames */
#define FORWARD_PREDICTION              0
#define BACKWARD_PREDICTION             1
#define BIDIRECTIONAL_PREDICTION        2
#define DIRECT_PREDICTION               3
#define UPWARD_PREDICTION               4
#define INTRA_PREDICTION                5

/* Prediction modes for True B-pictures. */
#define B_DIRECT_PREDICTION               0
#define B_FORWARD_PREDICTION              1
#define B_BACKWARD_PREDICTION             2
#define B_BIDIRECTIONAL_PREDICTION        3
#define B_INTRA_PREDICTION                4

#define EP_FORWARD_PREDICTION             0
#define EP_UPWARD_PREDICTION              1
#define EP_BIDIRECTIONAL_PREDICTION       2
#define EP_INTRA_PREDICTION               3

#define EI_UPWARD_PREDICTION              0
#define EI_INTRA_PREDICTION               1

/* Pixel aspect ration for custom source format */
#define PAR_0                           0  /* 0000 */
#define PAR_SQUARE                      1  /* 0001 */
#define PAR_CIF                         2  /* 0010 */
#define PAR_525                         3  /* 0011 */
#define PAR_CIF_STRETCHED               4  /* 0100 */
#define PAR_525_STRETCHED               5  /* 0101 */
#define PAR_EXTENDED                    15 /* 1111 */

#define CCC_1000                        0  /* clock conversion code */
#define CCC_1001                        1

/* Mode flags of reference picture selection */

#define MF_NO_ACK_NACK                  0 /* neither ACK nor NACK needed */
#define MF_ACK                          1 /* ACK signals need returning */
#define MF_NACK                         2 /* NACK signals need returning */
#define MF_ACK_NACK                     3 /* ACK and NACK */

#define MODE_INTER                      0
#define MODE_INTER_Q                    1
#define MODE_INTER4V                    2
#define MODE_INTRA                      3
#define MODE_INTRA_Q                    4
#define MODE_INTER4V_Q                  5


#define PBMODE_NORMAL                   0
#define PBMODE_MVDB                     1
#define PBMODE_CBPB_MVDB                2

#define PBMODE_BIDIR_PRED               0
#define PBMODE_CBPB_BIDIR_PRED          2
#define PBMODE_FRW_PRED                 6
#define PBMODE_CBPB_FRW_PRED            14
#define PBMODE_BCKW_PRED                30
#define PBMODE_CBPB_BCKW_PRED           31

#define SNR_SCALABILITY	                1
#define SPATIAL_SCALABILITY_H	        3
#define SPATIAL_SCALABILITY_V           5
#define SPATIAL_SCALABILITY_HV          7

/* Intra Mode for Advanced Intra Coding mode */
#define INTRA_MODE_DC                   0
#define INTRA_MODE_VERT_AC              2
#define INTRA_MODE_HORI_AC              3

/* Scalability default layer number. */
#define BASE_LAYER_NUM                  1
#define ENHANCEMENT_LAYER_NUM           2

/* Rate control methods */
#define OFFLINE_RC                      1
#define TMN5_RC                         2
#define TMN8_RC                         3 

#define NO_VEC                          999

/* Motionvector structure */
typedef struct motionvector {
  int x;         /* Horizontal comp. of mv            */
  int y;         /* Vertical comp. of mv              */
  int x_half;    /* Horizontal half-pel acc.          */
  int y_half;    /* Vertical half-pel acc.            */
  int min_error; /* Min error for this vector         */
  int Mode;      /* Necessary for adv. pred. mode     */
} MotionVector;

/* Point structure */

typedef struct point {
  int x;
  int y;
} Point;

/* Structure with image data */

typedef struct pict_image {
  unsigned char *lum;        /* Luminance plane        */
  unsigned char *Cr;        /* Cr plane        */
  unsigned char *Cb;        /* Cb plane        */
} PictImage;

typedef struct diff_image {
  int *lum;
  int *Cr;
  int *Cb;
} DiffImage;

/* Group of pictures structure. */

/* Picture structure */

typedef struct pict {
  int prev; 
  int curr;
  int TR;             /* Time reference */
  int bit_rate;
  int src_frame_rate;
  float target_frame_rate;
  int source_format;
  int picture_coding_type;
  int spare;
  int unrestricted_mv_mode;
  int PB;
  int QUANT;
  int DQUANT;
  int dquant_size;       /* dquant bit number when using modified quantization*/
  int MB;
  int seek_dist;        /* Motion vector search window */
  int use_gobsync;      /* flag for gob_sync */
  int MODB;             /* B-frame mode */
  int BQUANT;           /* which quantizer to use for B-MBs in PB-frame */
  int TRB;              /* Time reference for B-picture */
  float QP_mean;        /* mean quantizer */
  int EPTYPE;           /* use extended P-Type */
  int UFEP;             /* Update Full Extended PTYPE */
  int PCF;              /* Custom picture clock frequency */
  int PAR;              /* Pixel aspect ratio for custom format */
  int PWI;              /* Picture width indication for custom format */
  int PHI;              /* Picture height indication for custom format */
  int PAR_width;        /* Extended PAR width for custom format */
  int PAR_height;       /* Extended PAR height for custom format */
  int CCC;              /* Clock conversion code for custom picture clock*/
  int CD;               /* Clock divisor for custom picture clock */
  int rectangular_slice;    /* Slice structure submode */
  int arbitrary_slice_ordering;
  int ELNUM;            /* Enhancement layer number */
  int RLNUM;            /* Reference layer number */
  int MF;               /* Mode flags of reference picture selection */
  int reference_picture_selection;
  int TRPI;             /* Temporal reference for prediction indication */
  int TRP;              /* Temporal reference for prediction */
  int BCI;              /* Back channel message indication */
  int BCM;              /* Back channel message */
  int BCM_LENGTH;       /* Back channel message lenght */
  int RPRP;             /* Reference picture resampling parameters */     
  int RPRP_LENGTH;      /* Reference picture resampling parameters lenght */
  int PEI;              /* Extra insertion information, signals PSUPP */
  int PSUPP;            /* Supplemental Information (Annex L) */  
  int Intra_Mode;       /* Intra Mode for advanced Intra Coding */
  int RTYPE;            /* Rounding type for P-pictures */
} Pict;

/* Slice structure */

typedef struct slice {
  unsigned int vert_pos;	/* Vertical position of slice        */
  unsigned int quant_scale;	/* Quantization scale        */
} Slice;

/* Macroblock structure */

typedef struct macroblock {
  int mb_address;        /* Macroblock address */
  int macroblock_type;   /* Macroblock type    */
  int skipped;           /* 1 if skipped       */
  MotionVector motion;	 /* Motion Vector      */
} Macroblock;

/* Structure for macroblock data */

typedef struct mb_structure {
  int lum[16][16];
  int Cr[8][8];
  int Cb[8][8];
} MB_Structure;


/* Structure for counted bits */

typedef struct bits_counted {
  int Y;
  int C;
  int vec;
  int CBPY;
  int CBPCM;
  int INTRA_MODE;       /* used in advanced intra coding mode */
  int MODB;
  int CBPB;
  int COD;
  int header;
  int DQUANT;
  int total;
  int no_inter;
  int no_inter4v;
  int no_intra;
/* NB: Remember to change AddBits(), ZeroBits() and AddBitsPicture() 
   when entries are added here */
} Bits;


/* Structure for data for data from previous macroblock */

/* Structure for average results and virtal buffer data */

typedef struct results {
  float SNR_l;        /* SNR for luminance */
  float SNR_Cr;        /* SNR for chrominance */
  float SNR_Cb;
  float QP_mean;                /* Mean quantizer */
} Results;

void Help();
void AdvancedHelp();
void DeterminePictureType(int *frame_no, Pict *pic, int P_skip, int B_skip, int i);
void CalculateStatistics(PictImage *curr_image, PictImage *curr_recon, 
                         PictImage *pb_b_image, PictImage *pb_b_recon, 
                         Bits *bits, int QP, Pict *pic);
void StoreDirectModeVectors(MotionVector *ppMV[7][MBR+1][MBC+2], MotionVector *directMV[MBR][MBC]);
void InitializeMV(MotionVector *MV[7][MBR+1][MBC+2]);
void CodingLossImage(DiffImage *diff_image, PictImage *curr_image, 
                     PictImage *curr_reference_recon);
PictImage *AddEnhancementAndReferenceLayers(DiffImage *enhancement, 
                                            PictImage *reference);
int NextTwoPB(PictImage *p2, PictImage *bim, PictImage *p1, 
              int bskip, int pskip, int seek_dist);
void PrintSNR(Results *res, int num);
void PrintResult(Bits *bits, int num_units, int num);
unsigned char *ReadImage(char *filename, int frame_no, int headerlength);
PictImage *FillImage(unsigned char *in);
void WriteImage(PictImage *image, char *filename);
PictImage *InitImage(int size);
DiffImage *InitDiffImage (int size);
void FreeImage(PictImage *image);
void FreeDiffImage (DiffImage * image);
char *StripName(char *s);
int *MB_Encode(MB_Structure *mb_orig);
int MB_Decode(int *qcoeff, MB_Structure *mb_recon);
int Dct( int *block, int *coeff);
void Scan(int *qcoeff, int Intra_Mode);
void DeScan(int *qcoeff, int Intra_Mode);
int idct(int *coeff,int *block);
void FillLumBlock( int x, int y, PictImage *image, MB_Structure *data);
void FillChromBlock(int x_curr, int y_curr, PictImage *image,
            MB_Structure *data);
void FillDiffLumBlock( int x, int y, DiffImage *image, MB_Structure *data);
void FillDiffChromBlock(int x_curr, int y_curr, DiffImage *image,
            MB_Structure *data);
void ReconImage (int i, int j, MB_Structure *data, PictImage *recon);
void ReconDiffImage (int i, int j, MB_Structure *data, DiffImage *recon);
void CodeOneOrTwo(PictImage *curr, PictImage *B_image, PictImage *prev, 
          PictImage *prev_recon, int QP, int frameskip, Bits *bits,
          Pict *pic, PictImage *B_recon, PictImage *recon);
void CodeOneEP(PictImage *curr, PictImage *prev_enhance_image, 
                   PictImage *prev_enhance_recon, int QP, int frameskip, Bits *bits,
                   Pict *pic, PictImage *curr_ref_recon, PictImage *curr_enhance_recon);
PictImage *CodeOneIntra(PictImage *curr, int QP, Bits *bits, Pict *pic);
void CodeOneEI(PictImage *curr_image, PictImage *pr, 
                     int QP, Bits *bits, Pict *pic,
                     PictImage *recon);
void CodeOneTrueB(PictImage *curr, PictImage *B_image, PictImage *prev, 
          PictImage *pr, int QP, int frameskip, Bits *bits,
          Pict *pic, PictImage *B_recon, PictImage *recon);
void Dequant(int *qcoeff, int *rcoeff, int QP, int I, int block);
void Quant_blk(int *coeff, int *qcoeff, int QP, int Mode, int block);
void CountBitsCoeff(int *qcoeff, int I, int CBP, Bits *bits, int ncoeffs);
int CodeCoeff(int Mode, int *qcoeff, int block, int ncoeffs);
int FindCBP(int *qcoeff, int Mode, int ncoeffs);
void CountBitsVectors(MotionVector *MV[7][MBR+1][MBC+2], Bits *bits, 
              int i, int j, int Mode, int newgob, Pict *pic, 
              int scalability_prediction_type);
void FindPMV(MotionVector *MV[7][MBR+1][MBC+2], int x, int y, 
             int *p0, int *p1, int block, int newgob, int half_pel);
void ZeroBits(Bits *bits);
void ZeroRes(Results *res);
void ZeroVec(MotionVector *MV);
void MarkVec(MotionVector *MV);
void CopyVec(MotionVector *MV1, MotionVector *MV2);
int EqualVec(MotionVector *MV2, MotionVector *MV1);
void AddBits(Bits *total, Bits *bits);
void AddRes(Results *total, Results *res, Pict *pic);
void AddBitsPicture(Bits *bits);
void FindMB(int x, int y, unsigned char *image, int MB[16][16]);
MB_Structure *MB_Recon(PictImage *prev_recon, MB_Structure *diff,
               int x_curr, int y_curr, MotionVector *MV);
MB_Structure *Predict(PictImage *curr, PictImage *prev_recon,
              int x_curr, int y_curr, MotionVector *MV);
unsigned char *InterpolateImage(unsigned char *image, int w, int h, int RTYPE);
void GenerateFrameAndInterpolatedImages(PictImage *in_recon,  
                                   Pict *pic, unsigned char **out_recon_ipol, 
                                   PictImage **out_recon,
                                   unsigned char **recon_interpolated,
                                   PictImage **recon_frame);
void MotionEstimatePicture(unsigned char *curr, unsigned char *prev, 
           unsigned char *next, unsigned char *prev_ipol, unsigned
           char *next_ipol, int seek_dist, MotionVector *MV[7][MBR+1][MBC+2], 
           int gobsync, int estimation_type);
void MotionEstimation(unsigned char *curr, unsigned char *prev, int x_curr,
              int y_curr, int xoff, int yoff, int seek_dist, 
              MotionVector *MV[7][MBR+1][MBC+2], int *sad_0, 
              int estimation_type);
void MotionEstimationBackward( unsigned char *curr, unsigned char *reference, 
                               int x_curr, int y_curr, int xoff, int yoff,
                               int seek_dist, MotionVector *MV[7][MBR+1][MBC+2], 
                               int *SAD_0);
unsigned char *LoadArea(unsigned char *im, int x, int y, 
        int x_size, int y_size, int lx);
int SAD_Macroblock(unsigned char *ii, unsigned char *act_block,
           int h_length, int Min_FRAME);
int SAD_MB_Bidir(unsigned char *ii, unsigned char *aa, unsigned char *bb, 
         int width, int min_sofar);
int SAD_Block(unsigned char *ii, unsigned char *act_block,
              int h_length, int min_sofar);
int SAD_MB_integer(int *ii, int *act_block, int h_length, int min_sofar);
MB_Structure *Predict_P(PictImage *curr_image, PictImage *prev_image,
        unsigned char *prev_ipol,MB_Structure * pred_macroblock,int x_curr, int y_curr, 
        MotionVector *fr[7][MBR+1][MBC+2], int PB);
MB_Structure *Predict_B(PictImage *curr_image, PictImage *prev_image,
        unsigned char *prev_ipol,MB_Structure *pred_macroblock,int x_curr, int y_curr,
        MotionVector *MV[7][MBR+1][MBC+2],MotionVector *B_f_MV[7][MBR+1][MBC+2],
        MB_Structure *recon_P, int TR,int TRB,int PB, int *im_PB_pred_type);
MB_Structure *Predict_True_B(PictImage *curr_image, PictImage *prev_image,
        unsigned char *prev_ipol, PictImage *next_image, unsigned char *next_ipol,
        MB_Structure *pred_macroblock, int x, int y,
        MotionVector *MV[7][MBR+1][MBC+2], MotionVector *Direct_MV[MBR][MBC], 
      	int TRB, int *prediction_type, int *mode);
MB_Structure *Predict_EI( PictImage *curr_image, PictImage *base_recon, 
                          unsigned char *base_ipol, int x, int y,
                          int *prediction_type , int *mode);
MB_Structure *MB_Recon_EI( PictImage *base_recon, unsigned char *base_ipol, 
                           MB_Structure *diff, int x, int y,
                           int prediction_type);
MB_Structure *Predict_EP( PictImage *curr, PictImage *prev_recon, unsigned char *prev_ipol, 
                         PictImage *next_recon, unsigned char *next_ipol, MB_Structure *pred, 
                         int x, int y, MotionVector *MV[7][MBR+1][MBC+2], 
                         int *prediction_type, int *mode);
void Clip(MB_Structure *data);
void FindForwLumPredPB(unsigned char *prev_ipol, int x_curr, int y_curr, 
               MotionVector *fr, int *pred, int TRD, int TRB, 
               int bdx, int bdy, int bs, int comp);
void FindForwLumPredDirectTrueB(unsigned char *prev_ipol, int x_curr, int y_curr, 
               MotionVector *fr, int *pred, int TRB);
void FindBackwLumPredDirectTrueB(unsigned char *next_ipol, int x_curr, int y_curr, 
               MotionVector *fr, int *pred, int TRB);
void FindBiDirLumPredPB(int *recon_P, MotionVector *fr, int *pred, int TRD, 
        int TRB, int bdx, int bdy, int nh, int nv);
void FindBiDirChrPredPB(MB_Structure *recon_P, int dx, int dy, 
        MB_Structure *pred);
void FindBiDirLimits(int vec, int *start, int *stop, int nhv);
void FindBiDirChromaLimits(int vec, int *start, int *stop);
void BiDirPredBlock(int xstart, int xstop, int ystart, int ystop,
            int xvec, int yvec, int *recon, int *pred, int bl);
void DoPredChrom_P(int x_curr, int y_curr, int dx, int dy,
           PictImage *curr, PictImage *prev, MB_Structure *prediction,
           MB_Structure *pred_error);
void FindHalfPel(int x, int y, MotionVector *MV, unsigned char *prev, 
         int *curr, int bs, int comp);
void FindPred(int x, int y, MotionVector *fr, unsigned char *prev, 
              int *pred, int bs, int comp); 
void FindPredOBMC(int x, int y, MotionVector *MV[5][MBR+1][MBC+2], 
          unsigned char *prev, int *pred, int comp, int PB);
MB_Structure *MB_Recon_P(PictImage *prev_image, unsigned char *prev_ipol,
         MB_Structure *diff, int x_curr, int y_curr, 
         MotionVector *MV[7][MBR+1][MBC+2], int PB);
MB_Structure *MB_Recon_B(PictImage *prev_image, MB_Structure *diff,
         unsigned char *prev_ipol,int x_curr, int y_curr,
         MotionVector *MV[7][MBR+1][MBC+2],
         MB_Structure *recon_P,int TR, int TRB);
MB_Structure *MB_Recon_True_B( PictImage *prev_image, unsigned char *prev_ipol,
                               MB_Structure *diff, PictImage *next_image, 
                               unsigned char *next_ipol, int x_curr, int y_curr, 
                               MotionVector *MV[7][MBR+1][MBC+2], 
                               MotionVector *Direct_MV[MBR][MBC], int TRB,
                               int prediction_type);
MB_Structure *MB_Recon_EP( PictImage *prev_recon, unsigned char *prev_ipol, 
                           MB_Structure *diff, PictImage *next_recon, 
                           unsigned char *next_ipol, int x_curr, int y_curr,
                           MotionVector *MV[7][MBR+1][MBC+2],
                           int prediction_type);
MB_Structure *MB_Reconstruct (MB_Structure *diff,MB_Structure *pred);
void ReconLumBlock_P(int x, int y, MotionVector *fr,
             unsigned char *prev, int *data,int bs,int comp);
void ReconChromBlock_P(int x_curr, int y_curr, int dx, int dy,
               PictImage *prev, MB_Structure *data);
void FindChromBlock_P(int x_curr, int y_curr, int dx, int dy,
               PictImage *prev, MB_Structure *data);
void ComputeSNR(PictImage *im1, PictImage *im2, Results *res, int picture_type, int write);     
void ZeroMBlock(MB_Structure *data);
int CountBitsPicture(Pict *pic);
void CountBitsMB( int Mode, int COD, int CBP, int CBPB, Pict *pic, 
                  Bits *bits, int scalability_prediction_type);
void CountBitsScalMB (int Mode, int COD, int CBP, int CBPB, Pict * pic,
                      Bits * bits, int scalability_prediction_type,
                      int MV_present);
int CountBitsSlice(int slice, int quant);
int ChooseMode(unsigned char *curr, int x_pos, int y_pos, int min_SAD);
int ModifyMode(int Mode, int dquant, int EPTYPE);
int *InterleaveCoeff(int *qp, int *qb, int bs);
void MakeEdgeImage(unsigned char *src, unsigned char *dst, int width,
           int height, int edge);
void UpsampleComponent (unsigned char *base, unsigned char *enhanced,
                        int horiz, int vert);
PictImage *UpsampleReferenceLayerPicture(PictImage *base_image);
void BitPrint(int length, int val, char *bit);

/* Rate Control Variables */
int FrameUpdateQP( int buf, int bits, int frames_left, int QP, int B, 
                   float seconds);
void InitializeRateControlMB(PictImage *curr, float bit_rate, float target_frame_rate
, MotionVector * MV[7][MBR + 1][MBC + 2]);
int Compute_QP(int xpos,int ypos);
void UpdateRateControlMB(int bitsMB, int bitsCoeff, int xpos, int ypos, int QP);

int UpdateQuantizer( int mb, float QP_mean, int pict_type, float bit_rate, 
                     int mb_width, int mb_height, int bitcount);

int InitializeQuantizer( int pict_type, float bit_rate, 
                         float target_frame_rate, float QP_mean);
void InitializeRateControl();
void UpdateRateControl(int bits);
int FrameLayerRC(Pict *pic);

#ifndef FASTIDCT
/* global declarations for idctref */
void init_idctref (void);
void idctref (int *coeff, int *block);
#endif

/* Syntax based arithmetic coding routines */

void Count_sac_BitsCoeff(int *qcoeff, int I, int CBP, Bits *bits, int ncoeffs);
int Code_sac_Coeff(int Mode, int *qcoeff, int block, int ncoeffs);
int CodeTCoef(int mod_index, int position, int intra);
void Count_sac_BitsVectors( MotionVector *MV[7][MBR+1][MBC+2], Bits *bits,
                            int i, int j, int Mode, int newgob, Pict *pic);
void Count_sac_BitsMB(int Mode,int COD,int CBP,int CBPB,Pict *pic,Bits *bits);
int AR_Encode(int index, int cumul_freq[]);
int indexfn(int value, int table[], int max);
int bit_opp_bits(int);
int bit_in_psc_layer(int);
int encoder_flush();

/* bitstream-functions */

void initbits();
void putbits (int, int);
int alignbits ();
int bitcount();
void sacPutbits (int n, int val);

/* funtions for advanced intra coding mode */

int Choose_Intra_Mode(int *pcoeff, int *store_qcoeff, int xpos, int ypos, int newgob);
void fill_null(int pred[][6], int i);
void fill_A(int pred[][6],int *store_qcoeff, int xpos,int ypos, int block, int i);
void fill_B(int  pred[][6],int *store_qcoeff, int xpos,int ypos, int block, int i);
void Intra_AC_DC_Encode(int *pcoeff, int *store_rcoeff, int INTRA_AC_DC, int xpos, int ypos, int newgob, int i);
void Intra_AC_DC_Decode(int *rcoeff, int *store_qcoeff, int INTRA_AC_DC, int xpos, int ypos, int newgob, int i);
int oddifyclipDC(int x);
int clipAC(int x);
int clipDC(int x);
int put_intra_coeff (int run, int level, int last);

/* functions for deblocking filter */
void VertEdgeFilter(unsigned char *rec, int width, int height, Pict *pic, int chr);
void HorizEdgeFilter(unsigned char *rec, int width, int height, Pict *pic, int chr);
void EdgeFilter(PictImage *recon, Pict *pic);

/* functions for alternative Inter VLC mode */
void show_intra_coeff (int run, int level, int last, int *length, int *code);
void show_inter_coeff (int run, int level, int last, int *length, int *code);
int show_inter_run(int length, int code);

/* Reversible VLC coding function */
int put_rvlc(int data);

/* Fix broken header-files on suns to avoid compiler warnings */
/* #define BROKEN_SUN_HEADERS here or in Makefile */
#ifdef BROKEN_SUN_HEADERS
extern int printf();
extern int fprintf();
extern int time();
extern int fclose();
extern int rewind();
extern int fseek();
extern int fread();
extern int fwrite();
extern int fflush();
extern int fscanf();
extern int _flsbuf();
extern int _filbuf();
#endif

