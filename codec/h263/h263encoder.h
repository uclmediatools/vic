/***********************************************************HeaderBegin*******
 *                                                                         
 * File:        h263.h
 *
 * Author:      K.S.
 *
 * Created:	5-Nov-97
 *                                                                         
 * Description: Header file for h263 encoder
 *
 * Notes: 	
 *
 * Modified:	
 *              
 *
 ***********************************************************HeaderEnd*********/


#ifndef   _H263_H_
#define   _H263_H_

#include "Motion.h"
#include "defs.h"
#include "structs.h"
#include "bitIn.h"
#include "Util.h"

typedef struct {
  int intradc;     /* DC coefficient for I-macroblocks */
  int *level;      /* (DCT) transform coefficients */
} H263blkStruct;


typedef struct {
  int type;        /* MB type: MODE_UNCODED, MODE_INTER, MODE_INTER_Q, ... */
  int quant;       /* MB quantizer */
  int *cbp;        /* Coded block pattern for all blocks in MB */
  MVector **pv;    /* Predicted motion vector */
  MVector **mv;    /* Motion vector */
  H263blkStruct **blk;    /* Blocks */
  /* Additional, useful variables */
  int rate;        /* Rate */
  int dist;        /* Distortion */
} H263mbStruct;


#endif /* _H263CODEC_H_ */
