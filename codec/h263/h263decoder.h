#ifndef _H263_DECODER_H
#define _H263_DECODER_H

#include <stdio.h>

#include "defs.h"
#include "structs.h"
#include "bitIn.h"
#include "Util.h"

/* For every H.263 decoder one 'H263Global' is needed */
typedef struct H263GlobalStruct {
  Bitstream *bs;

  /* Images */
  unsigned char *refframe[3], *oldrefframe[3], *bframe[3], *newframe[3];
  unsigned char *edgeframe[3], *edgeframeorig[3], *exnewframe[3];

  int MV[2][5][MBR+1][MBC+2];
  int modemap[MBR+1][MBC+2];

  /* block data */
  short block[12][64];

  int first;
  int quiet;
  int expand;

  int fault;
  int gfid;
  int ptype;

  int temp_ref, prev_temp_ref, next_temp_ref;
  int trd;

  int quant;

  int source_format;
  int horizontal_size, vertical_size;
  int coded_picture_width, coded_picture_height;
  int chrom_width, chrom_height;
  int mb_width, mb_height;
  int mbs_in_gob, gobs_in_pict;

  int pict_type;

  int newgob, gob;
  int decGOBsCounter;
  unsigned char decGOBs[MAX_GOBS];

  /* Options */
  int mv_outside_frame, long_vectors;
  int adv_pred_mode;
  int pb_frame;
  int syntax_arith_coding;

  int ufep;

  /* For PB frames */
  int trb, bquant;

} H263Global;

#ifdef notextern
int vic_roundtab[16]
#ifdef DEFINE_GLOBALS
= {0,0,0,1,1,1,1,1,1,1,1,1,1,1,2,2}
#endif
;

int bquant_tab[4]
#ifdef DEFINE_GLOBALS
= {5,6,7,8}
#endif
;
#else
extern int vic_roundtab[];
extern int bquant_tab[];
#endif

typedef struct {
  int val, len;
} VLCtab;

typedef struct {
  char run, level, len;
} DCTtab;


/* Include prototype file */
#include "h263decoder.p"

#endif /* _H263_DECODER_H */
