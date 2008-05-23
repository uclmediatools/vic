/************************************************************************
 *
 *  global.h, global variables for tmndecode (H.263 decoder)
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


/* choose between declaration (GLOBAL undefined) and definition (GLOBAL
 * defined) GLOBAL is defined in exactly one file (tmndec.c) */

#include <stdio.h>
#include <memory.h>

#ifndef GLOBAL
#define EXTERN extern
#else
#define EXTERN
#endif

/* Some macros */
#define mmax(a, b)        ((a) > (b) ? (a) : (b))
#define mmin(a, b)        ((a) < (b) ? (a) : (b))
#define mnint(a)        ((a) < 0 ? (int)(a - 0.5) : (int)(a + 0.5))
#define sign(a)         ((a) < 0 ? -1 : 1)

/* prototypes of global functions */

/* getbits.c */
void initbits _ANSI_ARGS_ ((void));
void fillbfr _ANSI_ARGS_ ((void));
unsigned int showbits _ANSI_ARGS_ ((int n));
unsigned int getbits1 _ANSI_ARGS_ ((void));
void flushbits _ANSI_ARGS_ ((int n));
unsigned int vic_getbits _ANSI_ARGS_ ((int n));

/* getblk.c */
void getblock _ANSI_ARGS_ ((int comp, int mode, int INTRA_AC_DC, int Mode));
void get_sac_block _ANSI_ARGS_ ((int comp, int mode, int INTRA_AC_DC, int Mode));

/* gethdr.c */
int getheader _ANSI_ARGS_ ((void));
void startcode _ANSI_ARGS_ ((void));
void initoptions _ANSI_ARGS_ ((void));

/* getpic.c */
void getpicture _ANSI_ARGS_ ((int *framenum));
void putlast _ANSI_ARGS_ ((int framenum));
void PictureDisplay _ANSI_ARGS_((int *framenum));

/* scalability funcitons */
void UpsampleReferenceLayerPicture _ANSI_ARGS_((void));
void UpsampleComponent _ANSI_ARGS_((unsigned char *enhanced, unsigned char *base,
                        int horiz, int vert));
/* advanced intra coding functions */
void Intra_AC_DC_Decode _ANSI_ARGS_ ((short *store_qcoeff, int INTRA_AC_DC, int MBA, int xpos, int ypos, int comp, int newgob));
void fill_null _ANSI_ARGS_ ((int pred[]));
void fill_A _ANSI_ARGS_ ((int pred[], short *store_qcoeff, int xpos, int ypos, int block));
void fill_B _ANSI_ARGS_ ((int pred[], short *store_qcoeff, int xpos, int ypos, int block));
int oddifyclipDC _ANSI_ARGS_ ((int x));
int clipAC _ANSI_ARGS_ ((int x));
int clipDC _ANSI_ARGS_ ((int x));
void interpolate_image _ANSI_ARGS_ ((unsigned char *in, unsigned char
                                     *out, int width, int height));

/* getvlc.c */
int getTMNMV _ANSI_ARGS_ ((void));
int getRVLC  _ANSI_ARGS_ ((void));
int getMCBPC _ANSI_ARGS_ ((void));
int getMODB _ANSI_ARGS_ ((void));
int getMCBPCintra _ANSI_ARGS_ ((void));
int getCBPY _ANSI_ARGS_ ((void));
int getMBTYPE _ANSI_ARGS_ ((int *true_B_cbp, int *true_B_quant));
int getscalabilityCBPC ();

/* idct.c */
void idct _ANSI_ARGS_ ((short *block));
void init_idct _ANSI_ARGS_ ((void));

/* idctref.c */
void init_idctref _ANSI_ARGS_ ((void));
void idctref _ANSI_ARGS_ ((short *block));

/* tmndec.c */
void error _ANSI_ARGS_ ((char *text));
void printbits _ANSI_ARGS_ ((int code, int bits, int len));
#ifdef USE_TIME
void doframerate (int pb);
#endif

/* recon.c */
void reconstruct _ANSI_ARGS_ ((int bx, int by, int P, int bdx, int bdy, int MODB));
void reconstruct_true_B _ANSI_ARGS_ ((int bx, int by, int true_B_prediction_type));
void reconstruct_ei_ep _ANSI_ARGS_ ((int bx, int by, int ei_ep_prediction_type));

/* store.c */
void storeframe _ANSI_ARGS_ ((unsigned char *src[], int frame));
void store_enhancement_frame _ANSI_ARGS_ ((unsigned char *src[], int frame));
void save_frame _ANSI_ARGS_ ((unsigned char *src[], int framenum, FILE * file));
void store_one _ANSI_ARGS_ ((char *outname, unsigned char *src[], int offset, int incr, 
                             int height));

/* sac.c */
int decode_a_symbol _ANSI_ARGS_ ((int cumul_freq[]));
void decoder_reset ();

#ifdef DISPLAY
/* display.c */
void init_display _ANSI_ARGS_ ((char *name));
void exit_display _ANSI_ARGS_ ((void));
void dither _ANSI_ARGS_ ((unsigned char *src[]));
void init_dither _ANSI_ARGS_ ((void));

/* dither.c */
void ord4x4_dither_init _ANSI_ARGS_ ((void));
void ord4x4_dither_frame _ANSI_ARGS_ ((unsigned char *[], unsigned char *));

/* yuv2rgb.c */
void Color16DitherImage _ANSI_ARGS_ ((unsigned char *[], unsigned char *));
void Color32DitherImage _ANSI_ARGS_ ((unsigned char *[], unsigned char *));
void InitColorDither _ANSI_ARGS_ ((int));
void init_dither_tab _ANSI_ARGS_ ((void));

/* yuvrgb24.c */
void ConvertYUVtoRGB (
                       unsigned char *src0,
                       unsigned char *src1,
                       unsigned char *src2,
                       unsigned char *dst_ori,
                       int width,
                       int height
);
#endif



/* global variables */

#ifndef VIC
EXTERN char version[]
#ifdef GLOBAL
= "tmndecode v3.0\n(C) 1997, University of British Columbia, Canada\n(C) 1995, 1996 Telenor R&D, Norway\n"
#endif
;
#endif

/* zig-zag scan */
EXTERN unsigned char zig_zag_scan[64]
#ifdef GLOBAL
=
{
  0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
  12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
}
#endif
;

EXTERN int lines[]
#ifdef GLOBAL
= {-1,128,176,352,704,1408,-1}
#endif
;

EXTERN int pels[7]
#ifdef GLOBAL
= {-1,96,144,288,576,1152,-1}
#endif
;

/* other scan orders needed in advanced intra coding mode */
EXTERN unsigned char alternate_horizontal_scan[64]
#ifdef GLOBAL
=
{
  0, 1, 2, 3, 8, 9, 16, 17, 10, 11, 4, 5, 6, 7, 15, 14,
  13, 12, 19, 18, 24, 25, 32, 33, 26, 27, 20, 21, 22, 23, 28, 29,
  30, 31, 34, 35, 40, 41, 48, 49, 42, 43, 36, 37, 38, 39, 44, 45,
  46, 47, 50, 51, 56, 57, 58, 59, 52, 53, 54, 55, 60, 61, 62, 63
}
#endif
;
EXTERN unsigned char alternate_vertical_scan[64]
#ifdef GLOBAL
=
{
  0, 8, 16, 24, 1, 9, 2, 10, 17, 25, 32, 40, 48, 56, 57, 49,
  41, 33, 26, 18, 3, 11, 4, 12, 19, 27, 34, 42, 50, 58, 35, 43,
  51, 59, 20, 28, 5, 13, 6, 14, 21, 29, 36, 44, 52, 60, 37, 45,
  53, 61, 22, 30, 7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
}
#endif
;

/* color space conversion coefficients
 * 
 * entries are {crv,cbu,cgu,cgv}
 * 
 * crv=(255/224)*65536*(1-cr)/0.5 cbu=(255/224)*65536*(1-cb)/0.5
 * cgu=(255/224)*65536*(cb/cg)*(1-cb)/0.5
 * cgv=(255/224)*65536*(cr/cg)*(1-cr)/0.5
 * 
 * where Y=cr*R+cg*G+cb*B (cr+cg+cb=1) */

EXTERN int convmat[8][4]
#ifdef GLOBAL
=
{
  {117504, 138453, 13954, 34903}, /* no sequence_display_extension */
  {117504, 138453, 13954, 34903}, /* ITU-R Rec. 709 (1990) */
  {104597, 132201, 25675, 53279}, /* unspecified */
  {104597, 132201, 25675, 53279}, /* reserved */
  {104448, 132798, 24759, 53109}, /* FCC */
  {104597, 132201, 25675, 53279}, /* ITU-R Rec. 624-4 System B, G */
  {104597, 132201, 25675, 53279}, /* SMPTE 170M */
  {117579, 136230, 16907, 35559}/* SMPTE 240M (1987) */
}
#endif
;

EXTERN int quiet;
EXTERN int trace;
EXTERN int save_frames;
EXTERN char errortext[256];
EXTERN unsigned char *prev_frame[3], *current_frame[3], *bframe[3];
EXTERN unsigned char *tmp_f[3], *tmp_b[3];
EXTERN unsigned char *edgeframe[3], *edgeframeorig[3], *exnewframe[3];
EXTERN unsigned char *nextedgeframe[3], *nextedgeframeorig[3];
EXTERN unsigned char *enhancementedgeframe[3], *enhacnementedgeframeorig[3];
EXTERN unsigned char *next_I_P_frame[3], *prev_I_P_frame[3], *buffered_frame[3];
EXTERN int MV[2][6][MBR + 1][MBC + 2];
EXTERN int true_B_direct_mode_MV[2][MBR + 1][MBC + 2];
EXTERN int modemap[MBR + 1][MBC + 2];
EXTERN int predictionmap[MBR + 1][MBC + 2];
EXTERN unsigned char *clp;
EXTERN int horizontal_size, vertical_size, mb_width, mb_height;
EXTERN int coded_picture_width, coded_picture_height;
EXTERN int ref_coded_picture_width, ref_coded_picture_height;
EXTERN int chrom_width, chrom_height, blk_cnt;
EXTERN int ref_chrom_width, ref_chrom_height;
EXTERN int pict_type, newgob;
EXTERN int mv_outside_frame;
EXTERN int syntax_arith_coding;
EXTERN int adv_pred_mode;
EXTERN int overlapping_MC;
EXTERN int use_4mv;
EXTERN int pb_frame;
EXTERN int true_B_frame;
EXTERN int scalability_mode;
EXTERN int buffered_framenum;
EXTERN int long_vectors;
EXTERN int fault, expand;
EXTERN int verbose;
EXTERN int refidct;
EXTERN int matrix_coefficients;
EXTERN int temp_ref, prev_non_disposable_temp_ref, next_non_disposable_temp_ref;
EXTERN int pic_quant, quant, source_format, source_format_old, base_source_format;

/* Following variables are used in H.263+ bitstream decoding */
EXTERN int plus_type;           /* indicates if extended PTYPE exists or
                                 * not. */
EXTERN int optional_custom_PCF;
EXTERN int advanced_intra_coding;
EXTERN int deblocking_filter_mode;
EXTERN int slice_structured_mode;
EXTERN int reference_picture_selection_mode;
EXTERN int independently_segmented_decoding_mode;
EXTERN int alternative_inter_VLC_mode;
EXTERN int modified_quantization_mode;
EXTERN int reduced_resolution_update_mode;
EXTERN int reference_picture_resampling_mode;
EXTERN int rtype;
EXTERN int post_filter;

/* Scalability globals */
EXTERN unsigned char *prev_enhancement_frame[MAX_LAYERS][3], *current_enhancement_frame[MAX_LAYERS][3];
EXTERN unsigned char *curr_reference_frame[3], *upsampled_reference_frame[3];
EXTERN unsigned char *enhance_edgeframeorig[MAX_LAYERS][3], *enhance_edgeframe[MAX_LAYERS][3];
EXTERN unsigned char *tmp_enhance_fwd[MAX_LAYERS][3], *tmp_enhance_up[MAX_LAYERS][3];
EXTERN int enhancement_layer_init[MAX_LAYERS];
EXTERN int enhance_pict;
 
/* custom format variables */
EXTERN int CP_PAR_code;
EXTERN int CP_picture_width_indication;
EXTERN int CP_picture_width;
EXTERN int CP_picture_height_indication;
EXTERN int CP_picture_height;
EXTERN int PAR_width;
EXTERN int PAR_height;

EXTERN int CP_clock_frequency;
EXTERN int SSS_rectangular_slices;
EXTERN int SSS_arbitary_slice_ordering;
EXTERN int enhancement_layer_num;
EXTERN int reference_layer_number;
EXTERN int MF_of_reference_picture_selection;
EXTERN int TRPI;
EXTERN int temporal_reference_for_prediction;


#ifdef USE_TIME
EXTERN int framerate;
#ifndef WIN32
EXTERN struct timeval tftarget;
#else
EXTERN unsigned int targetTime;
#endif
#endif


EXTERN int trd, trb, bscan, bquant, true_b_trb;
EXTERN int bscan_tab[]
#ifdef GLOBAL
= {2, 4, 6, 8}
#endif
;
EXTERN int bquant_tab[]
#ifdef GLOBAL
= {5, 6, 7, 8}
#endif
;

EXTERN int OM[5][8][8]
#ifdef GLOBAL
= {
  {
    {4, 5, 5, 5, 5, 5, 5, 4},
    {5, 5, 5, 5, 5, 5, 5, 5},
    {5, 5, 6, 6, 6, 6, 5, 5},
    {5, 5, 6, 6, 6, 6, 5, 5},
    {5, 5, 6, 6, 6, 6, 5, 5},
    {5, 5, 6, 6, 6, 6, 5, 5},
    {5, 5, 5, 5, 5, 5, 5, 5},
    {4, 5, 5, 5, 5, 5, 5, 4},
  }, {
    {2, 2, 2, 2, 2, 2, 2, 2},
    {1, 1, 2, 2, 2, 2, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
  }, {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 2, 2, 2, 2, 1, 1},
    {2, 2, 2, 2, 2, 2, 2, 2},
  }, {
    {0, 0, 0, 0, 1, 1, 1, 2},
    {0, 0, 0, 0, 1, 1, 2, 2},
    {0, 0, 0, 0, 1, 1, 2, 2},
    {0, 0, 0, 0, 1, 1, 2, 2},
    {0, 0, 0, 0, 1, 1, 2, 2},
    {0, 0, 0, 0, 1, 1, 2, 2},
    {0, 0, 0, 0, 1, 1, 2, 2},
    {0, 0, 0, 0, 1, 1, 1, 2},
  }, {
    {2, 1, 1, 1, 0, 0, 0, 0},
    {2, 2, 1, 1, 0, 0, 0, 0},
    {2, 2, 1, 1, 0, 0, 0, 0},
    {2, 2, 1, 1, 0, 0, 0, 0},
    {2, 2, 1, 1, 0, 0, 0, 0},
    {2, 2, 1, 1, 0, 0, 0, 0},
    {2, 2, 1, 1, 0, 0, 0, 0},
    {2, 1, 1, 1, 0, 0, 0, 0},
}}
#endif
;

EXTERN int vic_roundtab[16]
#ifdef GLOBAL
= {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2}
#endif
;

EXTERN int MQ_chroma_QP_table[32]
#ifdef GLOBAL
 = {0, 1, 2, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10, 10, 11, 11, 12, 12, 12, 13, 13, 13, 
14, 14, 14, 14, 14, 15, 15, 15, 15, 15}
#endif
;

/* output */
EXTERN char *outputname;
EXTERN char trace_file_name[100];
EXTERN FILE *trace_file;
EXTERN char recon_file_name[100];
EXTERN FILE *recon_file_ptr;
EXTERN char enhance_recon_file_name[MAX_LAYERS][100];
EXTERN FILE *enhance_recon_file_ptr[MAX_LAYERS];

EXTERN int outtype;
#define T_YUV      0
#define T_SIF      1
#define T_TGA      2
#define T_PPM      3
#define T_X11      4
#define T_YUV_CONC 5
#define T_WIN      6

EXTERN struct ld
{
  /* bit input */
  int infile;
  unsigned char rdbfr[2051];
  unsigned char *rdptr;
  unsigned char inbfr[16];
  int incnt;
  int bitcnt;
  /* block data */
  short block[12][64];
} base, *ld;


/* for reference picture selection */

#define RING_SIZE 8

EXTERN int   ring_ptr;               /* pointer - to last picture */
EXTERN void *ring_lum[RING_SIZE];    /* luminance */
EXTERN void *ring_c1[RING_SIZE];     /* chominance */
EXTERN void *ring_c2[RING_SIZE];
EXTERN int   ring_tr[RING_SIZE];     /* TR values for the pictures */
EXTERN int   ring_qu[RING_SIZE];     /* picture quality */
EXTERN int   stop_decoder;           /* flag if we completely out of sync */

