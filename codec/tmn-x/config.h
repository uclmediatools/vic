/************************************************************************
 *
 *  config.h, part of tmn (TMN encoder)
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


#define YES 1
#define NO 0

/*************************************************************************/

/* Default modes */

/* use Unrestricted Motion Vector mode as default (also option "-D") */
#define DEF_UMV_MODE   NO

/* use Syntax-based Arithmetic Coding mode as default (also option "-E") */
#define DEF_SAC_MODE   NO

/* use Advanced Prediction mode as default (also option "-F") */
#define DEF_ADV_MODE   NO

/* use PB-frames mode as default (also option "-G") */
#define DEF_PBF_MODE   NO

/* New H.263+ options */

/* use Advanced Intra Coding Mode (also optioon "-I") */
#define DEF_AIC_MODE   NO

/* use Deblocking Filter Mode (also option "-J") */
#define DEF_DF_MODE   NO

/* use Slice Structure Mode (also option "-K") */
#define DEF_SS_MODE   NO

/* use improved PB-frames mode as default (also option "-M") */
#define DEF_IM_PBF_MODE   NO


/* use Reference Picture Selection Mode (also option "-N") */
#define DEF_RPS_MODE   NO

/* use SNR/Spacial Scalability Type (also option "-O") */
#define DEF_SNR_SPAT_SCAL_MODE    NO

/* use Temporal Scalability Type (also option "-c") */
#define DEF_TEMP_SCAL    NO

/* use Reference Picture Resampling Mode (also option "-P") */
#define DEF_RPR_MODE   NO

/*use Reduced Resolution Update (also option "-Q") */
#define DEF_RRU_MODE   NO

/* use Independently Segmented Decoding Mode (also option "-R") */
#define DEF_ISD_MODE   NO

/* use Alternative Inter VLC Mode (also option "-S") */
#define DEF_AIV_MODE   NO

/* use Modified Quantization Mode (also option "-T") */
#define DEF_MQ_MODE   NO

/*************************************************************************/

/* default coding format
   choose one of SF_SQCIF, SF_QCIF, SF_CIF, SF_4CIF, SF_16CIF */
#define DEF_CODING_FORMAT   SF_QCIF

/*************************************************************************/


/* Default filenames */

/* default output bitstream filename (also option "-B <filename>") */
#define DEF_STREAMNAME   "./stream.263"

/* default filename for reconstructed sequence 
   (also option "-o <filename>") */
#define DEF_OUTFILENAME   "./out.raw"

/* default filename for enhancemennt layer reconstructed sequence */
#define DEF_OUTFILENAME_ENHANCE   "./enhanced.raw"

/* write difference image to file (also option "-w") */
#define DEF_WRITE_DIFF   NO

/* default filename for difference images if "-w" is used */
#define DEF_DIFFILENAME   "./diff.raw"

/*************************************************************************/


/* Frame rate parameters */

/* default reference frame rate, 25 or 30 Hz 
 * (also option "-Z <n>") */
#define DEF_REF_FRAME_RATE   30.0

/* default number of skipped frames in original sequence compared to */
/* the reference picture rate ( also option "-O <n>" ) */
/* 3 means that the original sequence is grabbed at 6.25/7.5 Hz */
/* 0 means that the original sequence is grabbed at 25.0/30.0 Hz */
#define DEF_ORIG_SKIP      0

/* default skipped frames between encoded frames (P or B) */
/* reference is original sequence */
/* 2 means 8.33/10.0 fps encoded frame rate with 25.0/30.0 fps original */
/* 0 means 8.33/10.0 fps encoded frame rate with 8.33/10.0 fps original */
#define DEF_FRAMESKIP      2   

/* default rate control method (tmn-8 rate control) */
#define DEF_RC_METHOD TMN8_RC

/*************************************************************************/

/* Search windows */


/* default integer pel search seek distance ( also option "-s <n> " ) */
#define DEF_SEEK_DIST   15   

/* default integer search window for 8x8 search centered 
   around 16x16 vector. When it is zero only half pel estimation
   around the integer 16x16 vector will be performed */
/* for best performance, keep this small, preferably zero,
   but do your own simulations if you want to try something else */
#define DEF_8X8_WIN     0

/* default search window for PB delta vectors */
/* keep this small also */
#define DEF_PBDELTA_WIN   2

/*************************************************************************/


/* Frame numbers to start and stop encoding at */

/* default frame number to start at (also option "-a <n>") */
#define DEF_START_FRAME   0

/* default frame number to stop at (also option "-b <n>") */
#define DEF_STOP_FRAME    0

/*************************************************************************/


/* Quantization parameters */

/* default inter quantization parameter (also option "-q <n>") */
#define DEF_INTER_QUANT   13

/* default intra quantization parameter (also option "-I <n>") */
#define DEF_INTRA_QUANT   13

/* default true B quantization parameter (also option "-d <n>") */
#define DEF_TRUE_B_QUANT   13

/* default EI/EP quantization parameter (also option "-v <n>") */
#define DEF_EI_EP_QUANT   13

/* BQUANT parameter for PB-frame coding 
 *   (n * QP / 4 ) 
 *
 *  BQUANT  n 
 *   0      5 
 *   1      6 
 *   2      7 
 *   3      8 
 * ( also option "-Q <BQUANT>" ) */
#define DEF_BQUANT   2


/*************************************************************************/

/* Miscellaneous */

/* write repeated reconstructed frames to disk (useful for variable
 * framerate, since sequence will be saved at 25 Hz) 
 * Can be changed at run-time with option "-m" */
#define DEF_WRITE_REPEATED   NO

/* write bitstream trace to files trace.intra / trace 
 * (also option "-t") */
#define DEF_WRITE_TRACE   NO

/* start rate control after DEF_START_RATE_CONTROL % of sequence
 * has been encoded. Can be changed at run-time with option "-R <n>" */
#define DEF_START_RATE_CONTROL   0
/* default target frame rate when rate control is used */
/* DEF_FRAMESKIP is now used to determine the target frame rate */
/* headerlength on concatenated 4:1:1 YUV input file 
 * Can be changed at run-time with option -e <headerlength> */
#define DEF_HEADERLENGTH   0

/* insert sync after each DEF_INSERT_SYNC for increased error robustness
 * 0 means do not insert extra syncs */
#define DEF_INSERT_SYNC   0

/*************************************************************************/
