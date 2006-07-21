/* defs.h - a combined defs.h from h263 encoder and decoder tree 
 *
 * 15.12.97, jw
 */

#ifndef _DEFS_H_
#define _DEFS_H_

#define NUM_LAYERS 4

#ifndef _WIN32
#ifndef RAND_MAX
# define RAND_MAX 32767		/* sos4 has it not */
#endif
#endif

#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif
#define FALSE                        0
#define TRUE                  (!FALSE)

#define ON                           1
#define OFF                          0

#define NO_INDEX                    -1

/* File types */
#define SEP_RAW_TYPE                 0
#define CAT_SEQ_TYPE                 1

/* Coding scheme */
#define NONE                        0
#define H263                        1
#define PYRA                        2

/* Picture coding types */
#define PICTURE_CODING_TYPE_UNKNOWN -1
#define PICTURE_CODING_TYPE_INTRA    0
#define PICTURE_CODING_TYPE_INTER    1


#define SQCIF_WIDTH                128
#define SQCIF_HEIGHT                96
#define QCIF_WIDTH                 176
#define QCIF_HEIGHT                144
#define CIF_WIDTH                  352
#define CIF_HEIGHT                 288
#define CIF4_WIDTH                 704
#define CIF4_HEIGHT                576
#define CIF16_WIDTH               1408
#define CIF16_HEIGHT              1152

#define QCIF2_WIDTH                 96    /* CEILING_16(QCIF/2) */
#define QCIF2_HEIGHT                80
#define QCIF4_WIDTH                 48    /* CEILING_16(QCIF/4) */
#define QCIF4_HEIGHT                48
#define QCIF8_WIDTH                 32    /* CEILING_16(QCIF/8) */
#define QCIF8_HEIGHT                32

#define MAX_GOBS                        18



/* H.263 coding modes */
#define BASELINE_MODE                0
#define UMV_MODE                     1
#define AP_MODE                      2
#define UMV_AP_MODE                  3
#define PB_MODE                      4
#define SAC_MODE                     8


/* If PYRA_DCT_BASELAYER is defined, the intra blocks of the pyra layers
   are coded with DCT (as in H.263) in the baselayer */
/* #define PYRA_DCT_BASELAYER */


/* MB coding modes for H.263 and other coders */
#define MODE_UNSET                  -2
#define MODE_UNCODED                -1
#define MODE_INTER                   0
#define MODE_INTER_Q                 1
#define MODE_INTER4V                 2
#define MODE_INTRA                   3
#define MODE_INTRA_Q                 4
#define MODE_STUFFING                5

/* Define here what the parameter 'codingTime' shall switch at what time */
#define TIME_SUBSAMPLE_SAD_4    1
#define TIME_TH_UNCODED         1
#define TIME_HALFPEL_MC         2
#define TIME_FASTFULLPEL_MC     3
#define TIME_3COEF_DCT          4
#define TIME_FULLSEARCH_ME      5
#define TIME_SUBSAMPLE_SAD      6
#define TIME_RD_MODE_DECISION   7
/*#define TIME_RD_ME*/


/* Pyramid scales */
#define SCALE_STEP            2
#define SCALE_MIN            16
#define SCALE_MAX            78

/* Scaling scale for chroma */
#define CSCALE          0.6

typedef unsigned char        Byte;

/* For H.263 */
#define PSC                             1
#define PSC_LENGTH                      17
#define SE_CODE                         31

#define PBMODE_NORMAL                   0
#define PBMODE_MVDB                     1
#define PBMODE_CBPB_MVDB                2

#define ESCAPE                          7167
#define ESCAPE_INDEX                    102

#define PCT_INTER                       1
#define PCT_INTRA                       0

#define SQCIF                      	1  /* 001 */
#define QCIF                       	2  /* 010 */
#define CIF                        	3  /* 011 */
#define CIF4                       	4  /* 100 */
#define CIF16                      	5  /* 101 */
#define CUSTOM_SOURCE_FORMAT            6  /* 110 */

#define QCIF2                       	8    /* CEILING_16(QCIF/2) */
#define QCIF4                       	9    /* CEILING_16(QCIF/4) */
#define QCIF8                          10    /* CEILING_16(QCIF/8) */

#define MACROBLOCK_SIZE                 16
#define BLOCK_SIZE                      8 
#define MAX_SEARCH_RANGE                32


/* this is necessary for the max resolution 16CIF */
#define MBC                             88
#define MBR                             72

#define NO_VEC                          999

#define SyntaxError(str) \
{ fprintf(stderr,"Syntax error while parsing table %s\n", str); exit(1); }

#endif  /* #ifndef _DEFS_H_ */
