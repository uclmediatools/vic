#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "defs.h"
#include "structs.h"
#include "Util.h"

#include "bitOut.h"

#include "h263encoder.h"

#include "code.h"
#include "h263encoder.p"
#include "common.h"
#include "common.p" //SV-XXX: to avoid implicit declarations of several functions

/* Subsampling factor for SAD measurement */
extern int subsampleSAD;

/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- EncodeH263 -- Encode entire picture 
 *
 * Author:           K.S.
 *
 * Created:          05-Nov-97
 *
 * Purpose:          Encode picture
 * 
 * Arguments in:     int     rate             target bitrate for this picture.
 *                   int     codingTime       target coding time (0...15).
 *                   int     type             PICTURE_CODING_TYPE_INTRA or
 *                                            PICTURE_CODING_TYPE_INTER.
 *                   int     t                temporal reference.
 *                   int     percentIMBs      Percentage how many macroblocks
 *                                            are forced to be coded INTRA.
 *                   Picture *pict            used for ME,
 *                   Picture *prevPict        reference picture for ME.
 *                   Picture *prevDecPict     used as reference picture for
 *                                            HP-ME, MC.
 *                   unsigned char *crvec     conditional replenishment vector.
 *                                            array of unsigned char of which
 *                                            every element stands for a 16x16
 *                                            block. If the 7th bit is set
 *                                            (& 0x80), this block is
 *                                            updated/existing.
 *                                            [(row/16)*(width/16)+(column/16)]
 *
 * Arguments in/out: -
 *
 * Arguments out:    Picture *decPict         decoded picture.
 *                   Bitstr  *bs              Bitstream.
 *                   int     *mbInd           start of macroblocks in bitstr.
 *                                            (disregarded if NULL).
 *                   int     *mbQuant         macroblock quantizers.
 *                                            (disregarded if NULL).
 *                   MVField *mvField         motion vector field.
 *                                            (disregarded if NULL).
 *
 * Return values:    -
 *
 * Example:          EncodeH263(rate, codingTime, type, t, pict, prevPict,
 *                              prevDecPict, decPict, bs, mbInd, mbQuant,
 *                              mvField)
 *
 * Side effects:     -
 *
 * Description:      Switches depending on 'type' between
 *                   PICTURE_CODING_TYPE_INTRA and PICTURE_CODING_TYPE_INTER.
 *
 * See also:         H263intraPictureEncode, H263interPictureEncode
 *
 * Modified:         -
 *
 *****************************************************************************/
int EncodeH263(int rate, int codingTime, int type, int t, int percentIMBs,
	       Picture *pict, Picture *prevPict, Picture *prevDecPict,
	       unsigned char *crvec, Picture *decPict, Bitstr *bs,
	       int *mbInd, int *mbQuant, MVField *mvField)
/***********************************************************CommentEnd********/
{
  static int gfid = -1;
  static int prevType = -1;
  int bits;


  if (type != prevType)
    gfid++;
  prevType = type;

  switch(type) {
  case PICTURE_CODING_TYPE_INTRA:
    bits = H263intraPictureEncode(rate, codingTime, t, gfid, pict,
				  decPict, bs, mbInd, mbQuant, mvField);
    break;
  case PICTURE_CODING_TYPE_INTER:
    bits = H263interPictureEncode(rate, codingTime, t, gfid, percentIMBs,
				  pict, prevPict, prevDecPict, crvec, decPict,
				  bs, mbInd, mbQuant, mvField);
    break;
  default:
    fprintf(stderr,
	    "H263pictureEncode: pictureCodingType >>%d<< not supported\n", 
	    type);
    return(0);
  }

  return(bits);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- EncodeH263Q -- Encode entire picture with fixed quantizer
 *
 * Author:           K.S.
 *
 * Created:          05-Nov-97
 *
 * Purpose:          Encode picture with fixed quantizer 'q'.
 * 
 * Arguments in:     int     q                quantizer
 *                   int     codingTime       target coding time (0...15).
 *                                            At the moment only conditional
 *                                            replenishment (0) and half pel
 *                                            MC (1) is implemented.
 *                   int     type             PICTURE_CODING_TYPE_INTRA or
 *                                            PICTURE_CODING_TYPE_INTER.
 *                   int     t                temporal reference.
 *                   int     percentIMBs      Percentage how many macroblocks
 *                                            are forced to be coded INTRA.
 *                   Picture *pict            used for ME,
 *                   Picture *prevPict        reference picture for ME.
 *                   Picture *prevDecPict     used as reference picture for
 *                                            HP-ME, MC.
 *                   unsigned char *crvec     conditional replenishment vector.
 *                                            array of unsigned char of which
 *                                            every element stands for a 16x16
 *                                            block. If the 7th bit is set
 *                                            (& 0x80), this block is
 *                                            updated/existing.
 *                                            [(row/16)*(width/16)+(column/16)]
 *
 * Arguments in/out: -
 *
 * Arguments out:    Picture *decPict         decoded picture.
 *                   Bitstr  *bs              Bitstream.
 *                   int     *mbInd           start of macroblocks in bitstr.
 *                                            (disregarded if NULL).
 *                   int     *mbQuant         macroblock quantizers.
 *                                            (disregarded if NULL).
 *                   MVField *mvField         motion vector field.
 *                                            (disregarded if NULL).
 *
 * Return values:    -
 *
 * Example:          EncodeH263Q(q, codingTime, type, t, pict, prevPict,
 *                               prevDecPict, decPict, bs, mbInd, mbQuant,
 *                               mvField)
 *
 * Side effects:     -
 *
 * Description:      Switches depending on 'type' between
 *                   PICTURE_CODING_TYPE_INTRA and PICTURE_CODING_TYPE_INTER.
 *
 * See also:         EncodeH263, H263intraPictureEncode, H263interPictureEncode
 *
 * Modified:         -
 *
 *****************************************************************************/
int EncodeH263Q(int q, int codingTime, int type, int t, int percentIMBs,
		Picture *pict, Picture *prevPict, Picture *prevDecPict,
		unsigned char *crvec, Picture *decPict, Bitstr *bs,
		int *mbInd, int *mbQuant, MVField *mvField)
/***********************************************************CommentEnd********/
{
  static int gfid = -1;
  static int prevType = -1;
  int bits;


  if (type != prevType)
    gfid++;
  prevType = type;

  switch(type) {
  case PICTURE_CODING_TYPE_INTRA:
    bits = H263intraPictureEncodeQ(q, codingTime, t, gfid, pict,
				   decPict, bs, mbInd, mbQuant, mvField);
    break;
  case PICTURE_CODING_TYPE_INTER:
    bits = H263interPictureEncodeQ(q, codingTime, t, gfid, percentIMBs,
				   pict, prevPict, prevDecPict,
				   crvec, decPict, bs, mbInd, mbQuant,
				   mvField);
    break;
  default:
    fprintf(stderr,
	    "H263pictureEncodeQ: pictureCodingType >>%d<< not supported\n", 
	    type);
    return(0);
  }

  return(bits);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263intraPictureEncode -- Encode intra picture 
 *
 * Author:           K.S.
 *
 * Created:          05-Nov-97
 *
 * Purpose:          Encode intra picture 
 *
 * Arguments in:     int       rate       target bitrate for picture.
 *                   int     codingTime   target coding time (0...15).
 *                   int       t          time reference.
 *                   int       gfid       Group Frame ID.
 *                   Picture   *pict      picture to be coded using H.263
 *                                        DCT transform architecture.
 *
 * Arguments in/out: -
 *
 * Arguments out:    Picture   *decPict   decoded picture.
 *                   Bitstr    *bs        bitstream.
 *                   int       *mbInd     start of macroblocks in bitstr.
 *                                        (disregarded if NULL).
 *                   int       *mbQuant   macroblock quantizers.
 *                                        (disregarded if NULL).
 *                   MVField   *mvField   motion vector field.
 *                                        (disregarded if NULL).
 *
 * Return values:    int   number of bits used for coding the picture.
 *
 * Example:          H263intraPictureEncode(rate, codingTime, t, gfid,
 *                                          pict, decPict, bs, mbInd, mbQuant,
 *                                          mvField);
 *
 * Side effects:     -
 *
 * Description:      -
 *
 * See also:         H263dctMbEncode
 *
 * Modified:         -
 *
 *****************************************************************************/
int H263intraPictureEncode(int rate, int codingTime, int t, int gfid,
			   Picture *pict, Picture *decPict, Bitstr *bs,
			   int *mbInd, int *mbQuant, MVField *mvField)
/***********************************************************CommentEnd********/
{
  static int q = 20;
  static int bits = 8000;
  int        partRate, tolerance;
  int        sourceFormat;
  int        j;
  int        mx, my, mn;
  int        nMbVert = pict->h / MACROBLOCK_SIZE;/* wrong for 4CIF and 16CIF */

  //SV-XXX: unused
  UNUSED(codingTime);

  if (pict->w == SQCIF_WIDTH && pict->h == SQCIF_HEIGHT) {
    sourceFormat = SQCIF;
  } else if (pict->w==QCIF_WIDTH && pict->h==QCIF_HEIGHT){
    sourceFormat = QCIF;
  } else if (pict->w==CIF_WIDTH && pict->h==CIF_HEIGHT){
    sourceFormat = CIF;
  } else if (pict->w==CIF4_WIDTH && pict->h==CIF4_HEIGHT){
    sourceFormat = CIF4;
  } else if (pict->w==CIF16_WIDTH && pict->h==CIF16_HEIGHT){
    sourceFormat = CIF16;
  } else if (!(pict->w % MACROBLOCK_SIZE || pict->h % MACROBLOCK_SIZE)) {
    sourceFormat = CUSTOM_SOURCE_FORMAT;
  } else {
    fprintf(stderr,"Picture format %dx%d is not allowed in this H.263 coder\n",
	    pict->w, pict->h);
    return(0);
  }

  if (bits > rate)
    q++;
  else if (bits < rate)
    q--;

  bits = 0;
  for (j = 0, my = 0, mn = 0; j < nMbVert; ++j, my += MACROBLOCK_SIZE) {
    /* wrong for 4CIF and 16CIF */
    partRate = j * rate / nMbVert;
    tolerance = partRate / 5;
    if (bits > partRate)
      q++;
    else if (bits < partRate - tolerance)
      q--;
    q = CLIP(q, 1, 31);

    /* Init quant tables */
    if (q != quantInit)
      H263initQuantTables(q);

    if (mbInd != NULL)
      mbInd[mn] = bs->ind;
    /* Send Header */
    if (j == 0)
      bits += H263putPictureHeader(t, sourceFormat, PICTURE_CODING_TYPE_INTRA,
				   q, pict->w, pict->h, bs);
    else
      bits += H263putGOBHeader(j, gfid, q, bs);
#ifdef PROTOCOL
    if (fp_prot_g != NULL && protFormat_g >= 3)
      fprintf(fp_prot_g, "\n");
#endif
    /* Encode and send data */
    for (mx = 0; mx < pict->w; mx += MACROBLOCK_SIZE, mn++) {
#ifdef PROTOCOL
      if (fp_prot_g != NULL && protFormat_g >= 3)
	fprintf(fp_prot_g, "I");
#endif
      if (mx != 0 && mbInd != NULL)  /* wrong for 4CIF and 16CIF */
	mbInd[mn] = bs->ind;
      if (mbQuant != NULL)
	mbQuant[mn] = q;
      if (mvField != NULL) {
	mvField->mode[mn] = MODE_INTRA;
	mvField->mx[mn] = 0;
	mvField->my[mn] = 0;
      }

      bits += H263dctMbEncodeIFrame(mx, my, q, pict, decPict, bs);
    }

    /* PSTUF or GSTUF to byte align Syncs */
    bits += ByteAlign0X(bs);
  }

  return(bits);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263intraPictureEncodeQ -- Encode intra picture with fixed quantizer
 *
 * Author:           K.S.
 *
 * Created:          05-Nov-97
 *
 * Purpose:          Encode intra picture with fixed quantizer 'q'.
 *
 * Arguments in:     int       q          quantizer
 *                   int     codingTime   target coding time (0...15).
 *                   int       t          time reference.
 *                   int       gfid       Group Frame ID.
 *                   Picture   *pict      picture to be coded using H.263
 *                                        DCT transform architecture.
 *
 * Arguments in/out: -
 *
 * Arguments out:    Picture   *decPict   decoded picture.
 *                   Bitstr    *bs        bitstream.
 *                   int       *mbInd     start of macroblocks in bitstr.
 *                                        (disregarded if NULL).
 *                   int       *mbQuant   macroblock quantizers.
 *                                        (disregarded if NULL).
 *                   MVField   *mvField   motion vector field.
 *                                        (disregarded if NULL).
 *
 * Return values:    int   number of bits used for coding the picture.
 *
 * Example:          H263intraPictureEncodeQ(q, codingTime, t, gfid, pict,
 *                                           decPict, bs, mbInd, mbQuant,
 *                                           mvField);
 *
 * Side effects:     -
 *
 * Description:      -
 *
 * See also:         H263intraPictureEncode, H263dctMbEncode
 *
 * Modified:         -
 *
 *****************************************************************************/
int H263intraPictureEncodeQ(int q, int codingTime, int t, int gfid,
			    Picture *pict, Picture *decPict, Bitstr *bs,
			    int *mbInd, int *mbQuant, MVField *mvField)
/***********************************************************CommentEnd********/
{
  int        bits = 0;
  int        sourceFormat;
  int        j;
  int        mx, my, mn;
  int        nMbVert = pict->h / MACROBLOCK_SIZE;/* wrong for 4CIF and 16CIF */

  //SV-XXX: unused
  UNUSED(codingTime);

  if (pict->w == SQCIF_WIDTH && pict->h == SQCIF_HEIGHT) {
    sourceFormat = SQCIF;
  } else if (pict->w==QCIF_WIDTH && pict->h==QCIF_HEIGHT) {
    sourceFormat = QCIF;
  } else if (pict->w==CIF_WIDTH && pict->h==CIF_HEIGHT) {
    sourceFormat = CIF;
  } else if (pict->w==CIF4_WIDTH && pict->h==CIF4_HEIGHT){
    sourceFormat = CIF4;
  } else if (pict->w==CIF16_WIDTH && pict->h==CIF16_HEIGHT){
    sourceFormat = CIF16;
  } else if (!(pict->w % MACROBLOCK_SIZE || pict->h % MACROBLOCK_SIZE)) {
    sourceFormat = CUSTOM_SOURCE_FORMAT;
  } else {
    fprintf(stderr,"Picture format %dx%d is not allowed in this H.263 coder\n",
	    pict->w, pict->h);
    return(0);
  }

  /* Init quant tables */
  if (q != quantInit)
    H263initQuantTables(q);

  for (j = 0, my = 0, mn = 0; j < nMbVert; ++j, my += MACROBLOCK_SIZE) {
    /* wrong for 4CIF and 16CIF */
    if (mbInd != NULL)
      mbInd[mn] = bs->ind;
    /* Send Header */
    if (j == 0)
      bits += H263putPictureHeader(t, sourceFormat, PICTURE_CODING_TYPE_INTRA,
				   q, pict->w, pict->h, bs);
    else
      bits += H263putGOBHeader(j, gfid, q, bs);
#ifdef PROTOCOL
    if (fp_prot_g != NULL && protFormat_g >= 3)
      fprintf(fp_prot_g, "\n");
#endif
    /* Encode and send data */
    for (mx = 0; mx < pict->w; mx += MACROBLOCK_SIZE, mn++) {
#ifdef PROTOCOL
      if (fp_prot_g != NULL && protFormat_g >= 3)
	fprintf(fp_prot_g, "I");
#endif
      if (mx != 0 && mbInd != NULL)   /* wrong for 4CIF and 16CIF */
	mbInd[mn] = bs->ind;
      if (mbQuant != NULL)
	mbQuant[mn] = q;
      if (mvField != NULL) {
	mvField->mode[mn] = MODE_INTRA;
	mvField->mx[mn] = 0;
	mvField->my[mn] = 0;
      }

      bits += H263dctMbEncodeIFrame(mx, my, q, pict, decPict, bs);
    }

    /* PSTUF or GSTUF to byte align Syncs */
    bits += ByteAlign0X(bs);
  }

  return(bits);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263interPictureEncode -- Encode inter picture
 *
 * Author:           K.S.
 *
 * Created:          05-Nov-97
 *
 * Purpose:          Encode inter picture using slight RD optimzation
 *
 * Arguments in:     int       rate          target bitrate for picture
 *                   int       codingTime    target coding time (0...15).
 *                   int       t             time reference.
 *                   int       gfid          Group Frame ID.
 *                   int       percentIMBs   Percentage how many macroblocks
 *                                           are forced to be coded INTRA.
 *                   Picture   *pict         picture to encode.
 *                   Picture   *prevPict     reference picture for ME.
 *                   Picture   *prevDecPict  used as reference picture for
 *                                           HP-ME, MC.
 *
 * Arguments in/out: -
 *
 * Arguments out:    Picture   *decPict      decoded picture.
 *                   Bitstr    *bs           bitstream.
 *                   int       *mbInd        start of macroblocks in bitstr.
 *                                           (disregarded if NULL).
 *                   int       *mbQuant      macroblock quantizers.
 *                                           (disregarded if NULL).
 *                   MVField   *mvField      motion vector field.
 *                                           (disregarded if NULL).
 *
 * Return values:    -
 *
 * Example:          H263interPictureEncode(rate, codingTime, t, gfid,
 *                                          pict, prevPict, prevDecPict,
 *                                          decPict, bs, mbInd, mbQuant,
 *                                          mvField);
 *
 * Side effects:     -
 *
 * Description:      -
 *
 * See also:         H263pictureEncode.
 *
 * Modified:         -
 *
 *****************************************************************************/
int H263interPictureEncode(int rate, int codingTime, int t, int gfid,
			   int percentIMBs, Picture *pict, Picture *prevPict,
			   Picture *prevDecPict,
                           unsigned char *crvec, Picture *decPict, Bitstr *bs,
			   int *mbInd, int *mbQuant, MVField *mvField)
/***********************************************************CommentEnd********/
{
  static int q = 20;
  static int bits = 8000;
  static Byte *interpol[3] = {NULL, NULL, NULL};
  static int w;
  int        partRate, tolerance;
  int        sourceFormat;
  int        j;
  int        mx, my, mn;
  int        nMbVert = pict->h / MACROBLOCK_SIZE;/* wrong for 4CIF and 16CIF */
  int        forceIntra = 0;


  if (codingTime < TIME_SUBSAMPLE_SAD_4)
    subsampleSAD = 4;
  else if (codingTime < TIME_SUBSAMPLE_SAD)
    subsampleSAD = 2;
  else
    subsampleSAD = 1;

  if (codingTime >= TIME_HALFPEL_MC) {
    /* Allocate memory for interpolation images */
    if (interpol[0] == NULL) {
      w = pict->w;
      interpol[0] = (Byte *)malloc(pict->w * pict->h * sizeof(Byte));
      interpol[1] = (Byte *)malloc(pict->w * pict->h * sizeof(Byte));
      interpol[2] = (Byte *)malloc(pict->w * pict->h * sizeof(Byte));
    } else if (w != pict->w) {
      w = pict->w;
      free(interpol[0]);
      free(interpol[1]);
      free(interpol[2]);
      interpol[0] = (Byte *)malloc(pict->w * pict->h * sizeof(Byte));
      interpol[1] = (Byte *)malloc(pict->w * pict->h * sizeof(Byte));
      interpol[2] = (Byte *)malloc(pict->w * pict->h * sizeof(Byte));
    }
    /* Init interpolation images */
    InitHalfPelInterpolation(prevDecPict, interpol);
  }

  if (pict->w == SQCIF_WIDTH && pict->h == SQCIF_HEIGHT) {
    sourceFormat = SQCIF;
  } else if (pict->w==QCIF_WIDTH && pict->h==QCIF_HEIGHT) {
    sourceFormat = QCIF;
  } else if (pict->w==CIF_WIDTH && pict->h==CIF_HEIGHT) {
    sourceFormat = CIF;
  } else if (pict->w==CIF4_WIDTH && pict->h==CIF4_HEIGHT){
    sourceFormat = CIF4;
  } else if (pict->w==CIF16_WIDTH && pict->h==CIF16_HEIGHT){
    sourceFormat = CIF16;
  } else if (!(pict->w % MACROBLOCK_SIZE || pict->h % MACROBLOCK_SIZE)) {
    sourceFormat = CUSTOM_SOURCE_FORMAT;
  } else {
    fprintf(stderr,"Picture format %dx%d is not allowed in this H.263 coder\n",
	    pict->w, pict->h);
    return(0);
  }

  if (bits > rate)
    q++;
  else if (bits < rate)
    q--;

  bits = 0;
  for (j = 0, my = 0, mn = 0; j < nMbVert; ++j, my += MACROBLOCK_SIZE) {
    /* wrong for 4CIF and 16CIF */
    partRate = j * rate / nMbVert;
    tolerance = partRate / 5;
    if (bits > partRate)
      q++;
    else if (bits < partRate - tolerance)
      q--;
    q = CLIP(q, 2, 31);

    /* Init quant tables */
    if (q != quantInit)
      H263initQuantTables(q);

    if (mbInd != NULL)
      mbInd[mn] = bs->ind;
    /* Send Header */
    if (j == 0)
      bits += H263putPictureHeader(t, sourceFormat, PICTURE_CODING_TYPE_INTER,
				   q, pict->w, pict->h, bs);
    else
      bits += H263putGOBHeader(j, gfid, q, bs);
#ifdef PROTOCOL
    if (fp_prot_g != NULL && protFormat_g >= 3)
      fprintf(fp_prot_g, "\n");
#endif
    /* Encode and send data */
    for (mx = 0; mx < pict->w; mx += MACROBLOCK_SIZE, mn++) {
      if (mx != 0 && mbInd != NULL)   /* wrong for 4CIF and 16CIF */
	mbInd[mn] = bs->ind;
      if (mbQuant != NULL)
	mbQuant[mn] = q;

      /* Decide randomly which MBs are forced to be coded in INTRA mode */
      if (percentIMBs > 0)
	forceIntra = (rand() <= (percentIMBs * RAND_MAX) / 100);

      bits += H263MbEncodePFrame(mx, my, q, codingTime, forceIntra, pict,
				 prevPict, prevDecPict, interpol, crvec,
				 decPict, mvField, bs);
    }
    /* PSTUF or GSTUF to byte align Syncs */
    bits += ByteAlign0X(bs);
  }

  return(bits);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263interPictureEncodeQ -- Encode inter picture with fixed quantizer
 *
 * Author:           K.S.
 *
 * Created:          05-Nov-97
 *
 * Purpose:          Encode inter picture with fixed quantizer
 *
 * Arguments in:     int       q             quantizer.
 *                   int       codingTime    target coding time (0...15).
 *                   int       t             time reference.
 *                   int       gfid          Group Frame ID.
 *                   Picture   *pict         picture to encode.
 *                   int       percentIMBs   Percentage how many macroblocks
 *                                           are forced to be coded INTRA.
 *                   Picture   *prevPict     reference picture for ME.
 *                   Picture   *prevDecPict  used as reference picture for
 *                                           HP-ME, MC.
 *
 * Arguments in/out: -
 *
 * Arguments out:    Picture   *decPict      decoded picture.
 *                   Bitstr    *bs           bitstream.
 *                   int       *mbInd        start of macroblocks in bitstr.
 *                                           (disregarded if NULL).
 *                   int       *mbQuant      macroblock quantizers.
 *                                           (disregarded if NULL).
 *                   MVField   *mvField      motion vector field.
 *                                           (disregarded if NULL).
 *
 * Return values:    -
 *
 * Example:          H263interPictureEncodeQ(q, codingTime, t, gfid,
 *                                           pict, prevPict, prevDecPict,
 *                                           decPict, bs, mbInd, mbQuant,
 *                                           mvField);
 *
 * Side effects:     -
 *
 * Description:      -
 *
 * See also:         H263interPictureEncoder, H263pictureEncode.
 *
 * Modified:         -
 *
 *****************************************************************************/
int H263interPictureEncodeQ(int q, int codingTime, int t, int gfid,
			    int percentIMBs, Picture *pict, Picture *prevPict,
			    Picture *prevDecPict,
			    unsigned char *crvec, Picture *decPict, Bitstr *bs,
			    int *mbInd, int *mbQuant, MVField *mvField)
/***********************************************************CommentEnd********/
{
  int bits = 0;
  static Byte *interpol[3] = {NULL, NULL, NULL};
  static int w;
  int        sourceFormat;
  int        j;
  int        mx, my, mn;
  int        nMbVert = pict->h / MACROBLOCK_SIZE;/* wrong for 4CIF and 16CIF */
  int        forceIntra = 0;


  if (codingTime < TIME_SUBSAMPLE_SAD_4)
    subsampleSAD = 4;
  else if (codingTime < TIME_SUBSAMPLE_SAD)
    subsampleSAD = 2;
  else
    subsampleSAD = 1;

  if (codingTime >= TIME_HALFPEL_MC) {
    /* Allocate memory for interpolation images */
    if (interpol[0] == NULL) {
      w = pict->w;
      interpol[0] = (Byte *)malloc(pict->w * pict->h * sizeof(Byte));
      interpol[1] = (Byte *)malloc(pict->w * pict->h * sizeof(Byte));
      interpol[2] = (Byte *)malloc(pict->w * pict->h * sizeof(Byte));
    } else if (w != pict->w) {
      w = pict->w;
      free(interpol[0]);
      free(interpol[1]);
      free(interpol[2]);
      interpol[0] = (Byte *)malloc(pict->w * pict->h * sizeof(Byte));
      interpol[1] = (Byte *)malloc(pict->w * pict->h * sizeof(Byte));
      interpol[2] = (Byte *)malloc(pict->w * pict->h * sizeof(Byte));
    }
    /* Init interpolation images */
    InitHalfPelInterpolation(prevDecPict, interpol);
  }

  if (pict->w == SQCIF_WIDTH && pict->h == SQCIF_HEIGHT) {
    sourceFormat = SQCIF;
  } else if (pict->w==QCIF_WIDTH && pict->h==QCIF_HEIGHT){
    sourceFormat = QCIF;
  } else if (pict->w==CIF_WIDTH && pict->h==CIF_HEIGHT){
    sourceFormat = CIF;
  } else if (pict->w==CIF4_WIDTH && pict->h==CIF4_HEIGHT){
    sourceFormat = CIF4;
  } else if (pict->w==CIF16_WIDTH && pict->h==CIF16_HEIGHT){
    sourceFormat = CIF16;
  } else if (!(pict->w % MACROBLOCK_SIZE || pict->h % MACROBLOCK_SIZE)) {
    sourceFormat = CUSTOM_SOURCE_FORMAT;
  } else {
    fprintf(stderr,"Picture format %dx%d is not allowed in this H.263 coder\n",
	    pict->w, pict->h);
    return(0);
  }

  /* Init quant tables */
  if (q != quantInit)
    H263initQuantTables(q);

  for (j = 0, my = 0, mn = 0; j < nMbVert; ++j, my += MACROBLOCK_SIZE) {
    /* wrong for 4CIF and 16CIF */
    if (mbInd != NULL)
      mbInd[mn] = bs->ind;
    /* Send Header */
    if (j == 0)
      bits += H263putPictureHeader(t, sourceFormat, PICTURE_CODING_TYPE_INTER,
				   q, pict->w, pict->h, bs);
    else
      bits += H263putGOBHeader(j, gfid, q, bs);
#ifdef PROTOCOL
    if (fp_prot_g != NULL && protFormat_g >= 3)
      fprintf(fp_prot_g, "\n");
#endif
    /* Encode and send data */
    for (mx = 0; mx < pict->w; mx += MACROBLOCK_SIZE, mn++) {
      if (mx != 0 && mbInd != NULL)   /* wrong for 4CIF and 16CIF */
	mbInd[mn] = bs->ind;
      if (mbQuant != NULL)
	mbQuant[mn] = q;
      /* Decide randomly which MBs are forced to be coded in INTRA mode */
      if (percentIMBs > 0)
	forceIntra = (rand() <= (percentIMBs * RAND_MAX) / 100);
      bits += H263MbEncodePFrame(mx, my, q, codingTime, forceIntra, pict,
				 prevPict, prevDecPict, interpol, crvec,
				 decPict, mvField, bs);
    }
    /* PSTUF or GSTUF to byte align Syncs */
    bits += ByteAlign0X(bs);
  }

#ifdef PROTOCOL
  if (fp_prot_g != NULL && protFormat_g >= 4) {
    if (mvField != NULL) {
      fprintf(fp_prot_g,"\n");
      PrintMVField(mvField, fp_prot_g);
      fprintf(fp_prot_g,"\n\n");
    }
  }
#endif
  return(bits);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263dctMbEncodeIFrame -- H.263 DCT transfrom quantization of a macroblock
 *
 * Author:           K.S.
 *
 * Created:          05-Nov-97
 *
 * Purpose:          H.263 DCT transfrom quantization of a macroblock
 * 
 * Arguments in:     int     mx         x position of macroblock
 *                   int     my         y position of macroblock
 *                   int     q          quantizer
 *                   Picture *pict      transform coded picture using H.263
 *                                      DCT transform architecture,
 *
 * Arguments in/out: -
 *
 * Arguments out:    Picture *decPict   decoded picture
 *                   Bitstr  *bs        bitstream
 *
 * Return values:    int     Number of bits used for DCT encoding.
 *
 * Example:          H263dctMbEncodeIFrame(i*16, j*16, q, pict, decPict, bs);
 *
 * Side effects:     -
 *
 * Description:      Extracts six blocks from 'pict' according to mx, my and
 *                   does H.263 DCT transfrom quantization according to 'q'.
 *
 * See also:         -
 *	
 * Modified:         -
 *	
 *****************************************************************************/
int H263dctMbEncodeIFrame(int mx, int my, int q, Picture *pict,
			  Picture *decPict, Bitstr *bs)
/***********************************************************CommentEnd********/
{
  int                  w = pict->w;
  int                  wc = w >> 1;
  int                  wt;
  int                  k;
  int                  xpos, ypos;
  int                  dq = 0;
  Byte                 *bp;
  int                  coeff[64];
  short                res[64];
  static H263mbStruct  *mb = NULL;


  if (mb == NULL) {
    mb = H263allocMbStruct(1,6,64);
    mb->type = MODE_INTRA;
  }

  /* DCT, quant, zigzag */
  for (k=0;k<6;++k) {
    if (k<4) {
      bp = decPict->y;
      wt = w;
      xpos = mx + (k%2)*BLOCK_SIZE;
      ypos = my + (k>1)*BLOCK_SIZE;
      FdctFastByte(pict->y + xpos + ypos * w, w, coeff);
    } else {
      wt = wc;
      xpos = mx / 2;
      ypos = my / 2;
      if (k==4) {
	FdctFastByte(pict->u + xpos + ypos * wc, wc, coeff);
	bp = decPict->u;
      } else {
	FdctFastByte(pict->v + xpos + ypos * wc, wc, coeff);
	bp = decPict->v;
      }
    }
    H263quantTable(coeff, q, MODE_INTRA, mb->blk[k]->level, &(mb->blk[k]->intradc),
		   &(mb->cbp[k]));
    H263dequantTable(mb->blk[k]->level, mb->blk[k]->intradc, q, MODE_INTRA,
		     mb->cbp[k], coeff);
    IdctFast(coeff, res);
    PlaceBlockShortPicture(xpos, ypos, bp, wt, res);
  }

  /* Coding */
  return(H263putIFrameMB(mb, dq, bs));
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263dctMbEncodePFrame -- H.263 DCT transfrom quantization of a macroblock
 *
 * Author:           K.S.
 *
 * Created:          20-Jan-98
 *
 * Purpose:          H.263 DCT transfrom quantization of a macroblock
 * 
 * Arguments in:     int     mx         x position of macroblock
 *                   int     my         y position of macroblock
 *                   int     codingTime < TIME_3COEF_DCT: Use adaptive
 *                                                        3 coeff. DCT
 *                   Picture *pict      transform coded picture using H.263
 *                                      DCT transform architecture,
 *
 * Arguments in/out: H263mbStruct *mb   macroblock struct containing
 *                                      the mode, the quantizer,
 *                                      the motion vector and the predictor.
 *                                      mb->level and mb->intradc are updated.
 *                   Picture *decPict   decoded picture.
 *                                      contains the motion compensated pred.
 *
 * Arguments out:    Bitstr  *bs        bitstream
 *
 * Return values:    int     Number of bits used for DCT encoding.
 *
 * Example:          bits = H263dctMbEncodePFrame(i*16, j*16, mb, q,
 *                                                pict, decPict, bs);
 *
 * Side effects:     -
 *
 * Description:      Extracts six blocks from 'pict' according to mx, my and
 *                   does H.263 DCT transfrom quantization according to 'q'.
 *
 * See also:         -
 *
 * Modified:         -
 *
 *****************************************************************************/
int H263dctMbEncodePFrame(int mx, int my, H263mbStruct *mb, int codingTime,
			  Picture *pict, Picture *decPict, Bitstr *bs)
/***********************************************************CommentEnd********/
{
  const int     w = pict->w;   /* width of luminance */
  const int     wc = w >> 1;   /* width of color components */
  int           wt;            /* tmp width variable */
  int           dq = 0;
  int           k, l;
  int           cost = 0;
  int           tmp;
  int           xpos, ypos;
  Byte          *bp1, *bp2;   /* Pointer to picture components */
  int           coeff[64];
  short         res[64];
  int           block[64];


  /* Reset decoded picture for an intra macroblock */
  if (mb->type == MODE_INTRA || mb->type == MODE_INTRA_Q)
    ResetMacroBlockPicture(mx, my, decPict);

  /* DCT, quant, zigzag */
  if (mb->type != MODE_UNCODED) {
    for (k=0;k<6;++k) {
      if (k<4) {
	xpos = mx + (k%2)*BLOCK_SIZE;
	ypos = my + (k>1)*BLOCK_SIZE;
	wt = w;
	bp1 = pict->y;
	bp2 = decPict->y;
      } else {
	xpos = mx / 2;
	ypos = my / 2;
	wt = wc;
	if (k==4) {
	  bp1 = pict->u;
	  bp2 = decPict->u;
	} else {
	  bp1 = pict->v;
	  bp2 = decPict->v;
	}
      }
      if (mb->type == MODE_INTRA || mb->type == MODE_INTRA_Q)
	ExtractBlockInt(xpos, ypos, bp1, wt, block);
      else
	ExtractDiffBlockInt(xpos, ypos, bp1, bp2, wt, block);
      if (codingTime < TIME_3COEF_DCT) {
	Fdct3coef(block, coeff);
	H263quantTable(coeff, mb->quant, mb->type, mb->blk[k]->level,
		       &(mb->blk[k]->intradc), &(mb->cbp[k]));
	H263dequantTable(mb->blk[k]->level, mb->blk[k]->intradc, mb->quant,
			 mb->type, mb->cbp[k], coeff);
	Idct3coef(coeff, res);
	cost = 0;
	for (l = 0; l < BLOCK_SIZE*BLOCK_SIZE; l += 2) { /* Subsampling! */
	  tmp = res[l] - block[l];
	  cost += ABS(tmp);
	}
	cost *= 2;   /* Because of subsampling */
	if (cost > mb->quant * 40) {   /* Threshold! */
	  /* Nothing beats the real thing! */
	  FdctFast(block, coeff);
	  H263quantTable(coeff, mb->quant, mb->type, mb->blk[k]->level,
			 &(mb->blk[k]->intradc), &(mb->cbp[k]));
	  H263dequantTable(mb->blk[k]->level, mb->blk[k]->intradc, mb->quant,
			   mb->type, mb->cbp[k], coeff);
	  IdctFast(coeff, res);
	}
      } else {
	FdctFast(block, coeff);
	H263quantTable(coeff, mb->quant, mb->type, mb->blk[k]->level,
		       &(mb->blk[k]->intradc), &(mb->cbp[k]));
	H263dequantTable(mb->blk[k]->level, mb->blk[k]->intradc, mb->quant,
			 mb->type, mb->cbp[k], coeff);
	IdctFast(coeff, res);
      }
      /* Place decoded back in picture */
      AddBlockShortPicture(xpos, ypos, bp2, wt, res);
    }

    /* Detect UNCODED mode */
    //SV-XXX: added explicit braces to avoid ambiguous else
    if (mb->cbp[0] == 0 && mb->cbp[1] == 0 && mb->cbp[2] == 0 &&
	mb->cbp[3] == 0	&& mb->cbp[4] == 0 && mb->cbp[5] == 0) {
      if (mb->type == MODE_INTER && mb->type == MODE_INTER_Q &&
	  ZeroMVector2D(mb->mv[0])) {
	mb->type = MODE_UNCODED;
      }
      else if (mb->type == MODE_INTER4V &&
	       ZeroMVector2D(mb->mv[0]) && ZeroMVector2D(mb->mv[1]) &&
	       ZeroMVector2D(mb->mv[2]) && ZeroMVector2D(mb->mv[3])) {
		mb->type = MODE_UNCODED;
           }
    }
  }

  /* Coding */
  return(H263putPFrameMB(mb, dq, bs));
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263MbEncodePFrame -- H.263 encoding of a P-Frame macroblock
 *
 * Author:           K.S.
 *
 * Created:          06-Nov-97
 *
 * Purpose:          -
 * 
 * Arguments in:     int     mx         x position of macroblock
 *                   int     my         y position of macroblock
 *                   int     q          quantizer.
 *                   int     codingTime target coding time (0...15).
 *                   Picture *pict      transform coded picture using H.263
 *                                      DCT transform architecture.
 *                   Picture *prevPict  reference original picture
 *                   Picture *prevDecPict  reference picture, previously decoded.
 *                   Byte    *interpol[3] halfpel shifted versions of prevDecPict.
 *
 * Arguments in/out: -
 *
 * Arguments out:    Picture *decPict   decoded picture
 *                   MVField *mvField   motion vector field
 *                                      (disregarded if NULL).
 *                   Bitstr  *bs        bitstream
 *
 * Return values:    int     Number of bits used for encoding the macroblock.
 *
 * Example:          H263MbEncodePFrame(mx, my, q, codingTime, pict, prevPict,
 *                                      prevDecPict, interpol, decPict,
 *                                      mvField, bs);
 *
 * Side effects:     -
 *
 * Description:      Does motion estimation or conditional replenishment.
 *                   Extracts six blocks from 'pict' according to mx, my and
 *                   does H.263 DCT transfrom quantization according to 'q'.
 *                   Does not work for CIF4 of CIF16.
 *
 * See also:         -
 *
 * Modified:         -
 *
 *****************************************************************************/
int H263MbEncodePFrame(int mx, int my, int q, int codingTime, int forceIntra,
		       Picture *pict, Picture *prevPict, Picture *prevDecPict,
		       Byte *interpol[3], unsigned char *crvec,
		       Picture *decPict, MVField *mvField, Bitstr *bs)
/***********************************************************CommentEnd********/
{
  int                  bsIndStart = bs->ind;
  int                  mn = mx / MACROBLOCK_SIZE +
                           (my / MACROBLOCK_SIZE) * (pict->w/MACROBLOCK_SIZE);
  int                  cost = 0, sadI, costMin;
  int                  costU = 0, costP, costI;
  int                  modeMin = 0;
  int                  dq = 0;
  int                  rate = 0;
  int                  lambda = 0;     /* for RD mode decision */
  int                  me_lambda = q;  /* for RD motion estimation */
  int                  bits = 0;       /* For counting bits */
  Byte                 mblk[6*64];   /* for storing the decoded I macroblock */
  static H263mbStruct  *mb = NULL, *mbI = NULL;


  if (mb == NULL) {
    mb = H263allocMbStruct(1,6,64);
    mbI = H263allocMbStruct(1,6,64);
    mbI->type = MODE_INTRA;
  }

  if (mx == 0) {   /* First MB in GOB */
    ResetMVector(mb->pv[0]);
    ResetMVector(mb->mv[0]);
  }
  mb->quant = q;

  if (codingTime >= TIME_RD_MODE_DECISION) {
    lambda = (17 * SQR(q)) / 20;
  }

  if (forceIntra) {
    mb->type = MODE_INTRA;
    bits = H263dctMbEncodePFrame(mx, my, mb, codingTime, pict, decPict, bs);
  } else {
    mb->type = MODE_UNSET;
    /* Conditional replenishment */
    if (crvec != NULL)
      if (!(crvec[mn] & 0x80))
	mb->type = MODE_UNCODED;

    if (mb->type == MODE_UNCODED) {

      /***********/
      /* UNCODED */
      /***********/
      CopyMacroBlockPicture(mx, my, prevPict, decPict);
      bits = H263putPFrameMB(mb, dq, bs);

    } else {

      /* Compute lagrangian cost for uncoded mode */
      if (lambda != 0)
	costU = MacroBlockPictureSSE(mx, my, pict, prevDecPict, INT_MAX)
	+ lambda;

      /************************/
      /* ME and mode decision */
      /************************/

      cost = FPMotionMacroBlockPictureSAD(mx, my, pict, prevDecPict,
					      mb->pv[0], INT_MAX);
      if (me_lambda != 0)
	cost += me_lambda * GetMVDRate(mb->pv[0], NULL);
      else
	cost -= 100;	/* We prefer the zero vector */
      if (codingTime < TIME_FASTFULLPEL_MC)
	ResetMVector(mb->mv[0]);
      else if (codingTime < TIME_FULLSEARCH_ME) {
	mb->mv[0]->sx = 0;
	mb->mv[0]->sy = 0;
	cost = FastFullPelMotionEstimationMB(mx, my, pict, prevPict, cost,
					     me_lambda, mb->pv[0], mb->mv[0]);
      } else {
	ResetMVector(mb->mv[0]);
	FullSearchFullPelME(mx, my, &cost, me_lambda, 16, -15, 15,
			    pict, prevPict, mb->pv[0], mb->mv[0]);
      }

      /* Subtract rate for comparison with intra sad */
      if (me_lambda != 0)
	rate = me_lambda * GetMVDRate(mb->pv[0], mb->mv[0]);
      else
	rate = 0;
      cost -= rate;
      /* We prefer INTER mode quite a lot */
      costMin = cost - 500;

      /* Evaluate intra (I) mode */
      if (lambda == 0) {
	if ((sadI=MacroBlockPictureIntraSAD(mx, my, pict, costMin)) < costMin){
	  /* INTRA */
	  mb->type = MODE_INTRA;
	  bits = H263dctMbEncodePFrame(mx, my, mb, codingTime, pict, decPict,
				       bs);
	}
      } else {
	mbI->quant = q;
	bits = H263dctMbEncodePFrame(mx, my, mbI, codingTime, pict,decPict,bs);
	costI = lambda * bits;
	bs->ind = bsIndStart;
	costI += MacroBlockPictureSSE(mx, my, pict, decPict, costU - costI);
	if (costI < costU) {
	  costMin = costI;
	  modeMin = MODE_INTRA;
	  ExtractMacroBlock(mx, my, decPict, mblk);
	} else {
	  costMin = costU;
	  modeMin = MODE_UNCODED;
	}
      }

      /* Halfpel ME */
      if (mb->type == MODE_UNSET) {
	/* INTER */
	mb->type = MODE_INTER;
	if (codingTime >= TIME_HALFPEL_MC) {
	  cost = FPMotionMacroBlockPictureSAD(mx, my, pict, prevDecPict,
					      mb->mv[0], INT_MAX);
	  cost += rate;
	  /* Half pel motion estimation and compensation */
	  cost = HalfPelMotionEstimationMB(mx, my, pict, interpol,
					   mb->mv[0]->x > -16, 1,
					   mb->mv[0]->y > -16, 1,
					   cost, me_lambda,
					   mb->pv[0], mb->mv[0]);
	  MCMacroBlockPicture(mx, my, prevDecPict, interpol, mb->mv[0],
			      decPict);
	} else
	  CopyMacroBlockPicture(mx, my, prevDecPict, decPict);
	/* DCT coding of residuum */
	bits = H263dctMbEncodePFrame(mx, my, mb, codingTime, pict, decPict,bs);
      }

      /* Compute lagrangian cost for inter (P) mode and
	 decide which mode we take */
      if (lambda != 0) {
	costP = bits * lambda;
	costP += MacroBlockPictureSSE(mx, my, pict, decPict, costMin - costP);
	if (costP >= costMin) {   /* P is not the best mode */
	  mb->type = modeMin;
	  bs->ind = bsIndStart;    /* Reset bitstream */
	  if (modeMin == MODE_UNCODED) {
	    CopyMacroBlockPicture(mx, my, prevDecPict, decPict);
	    bits = H263putPFrameMB(mb, dq, bs);
	  } else if (modeMin == MODE_INTRA || modeMin == MODE_INTRA_Q) {
	    PlaceMacroBlock(mx, my, mblk, decPict);
	    bits = H263putPFrameMB(mbI, dq, bs);
	  }
	}
      }
    }
  }

  if (mvField != NULL) {
    mvField->mode[mn] = mb->type;
    switch (mb->type) {
    case MODE_INTER:
    case MODE_INTER_Q:
      mvField->mx[mn] = 2 * mb->mv[0]->x + mb->mv[0]->sx;
      mvField->my[mn] = 2 * mb->mv[0]->y + mb->mv[0]->sy;
      break;
    case MODE_UNCODED:
    case MODE_INTRA:
    case MODE_INTRA_Q:
      ResetMVector(mb->mv[0]);
      mvField->mx[mn] = 0;
      mvField->my[mn] = 0;
      break;
    }
  }

#ifdef PROTOCOL
  if (fp_prot_g != NULL && protFormat_g >= 3) {
    if (mb->type == MODE_UNCODED)
      fprintf(fp_prot_g, "U");
    else if (mb->type == MODE_INTRA || mb->type == MODE_INTRA_Q)
      fprintf(fp_prot_g, "I");
    else
      fprintf(fp_prot_g, "P");
    if (protFormat_g >= 4 && mvField == NULL) {
      fprintf(fp_prot_g, "\n");
      fprintf(fp_prot_g,"(%d;%d) ", 2 * mb->mv[0]->x + mb->mv[0]->sx,
	      2 * mb->mv[0]->y + mb->mv[0]->sy);
      fprintf(fp_prot_g, "\n\n");
    }
  }
#endif

  /* Store motion vector predictor for next macroblock */
  CopyMVector(mb->mv[0], mb->pv[0]);
  if (mb->type == MODE_INTER4V) {
    CopyMVector(mb->mv[1], mb->pv[1]);
    CopyMVector(mb->mv[2], mb->pv[2]);
    CopyMVector(mb->mv[3], mb->pv[3]);
  }

  /* Coding */
  return(bits);

}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263quant -- H.263-based quantization
 *
 * Author :            Karl Olav Lillevold
 *
 * Purpose :           Quantization of DCT-transform coefficients
 *
 * Arguments in :      int *coeff    pointer to (zigzag-scanned)
 *                                   transform coefficients
 *                     int mbQuant   Quantization parameter of macroblock
 *                     int mode      macroblock mode
 *
 * Arguments in/out :  -
 *
 * Arguments out :     int *qcoeff   quantized coefficients.
 *                     int *intradc  intra DC coefficient.
 *                     int *cbp      coded block pattern
 *                                   0 if all 'qcoeff's and the 'intradc' are 0
 *                                   1 if any 'qcoeff' or the 'intradc' isnot 0
 *
 * Return values :     none
 *
 * Side effects :      none
 *
 * Description :       Quantizes 'coeff' based on H.263 rules given the
 *                     macroblock 'mode' and the quantization parameter
 *	               'mbQuant'.
 *
 * See also :          H263dequant
 *
 * Modified :          T.W., 12-Nov-96, adapted from TMN-2.0
 *                     (Cross-checked to TMN-version: same output, overflow
 *                      observed in both versions--clipping necessary )
 *
 *****************************************************************************/
void H263quant(int *coeff, int mbQuant, int mode, int *qcoeff, int *intradc,
	       int *cbp)
/***********************************************************CommentEnd********/
{
  int i;
  int level;
  int mbQh = mbQuant >> 1, mbQd = mbQuant << 1;
  int *qc = qcoeff;
  int *c = coeff;


  *cbp = 0;

  if (mbQuant>0) {
    if (mode == MODE_INTRA || mode == MODE_INTRA_Q) { /* Intra */
      *intradc = CLIP(*c/8, 1, 254)*8;
      *qc = *intradc;
      qc++;
      c++;
      for (i = 1; i < 64; i++, c++) {
        level = (ABS(*c)) / mbQd;
	level = clp[level << 1] >> 1;
        *qc = (*c < 0) ? -level : level;
        /**qc =  CLIP(SGN(*c) * level, -127, 127);*/
	if (*qc++!=0) *cbp = 1;
      }
    } else { /* non Intra */
      for (i = 0; i < 64; i++, c++) {
        level = *c;
        level = ABS(level);
        level = (int)((level-mbQh) / mbQd);
	level = clp[level << 1] >> 1;
        *qc = (*c < 0) ? -level : level;
	/*        *qc =  CLIP(level, -127, 127);*/
	if (*qc++!=0) *cbp = 1;
      }
    }
  } else {
    /* No quantizing.
       Used only for testing. Bitstream will not be decodable 
       whether clipping is performed or not */
    for (i = 0; i < 64; i++) {
      qcoeff[i] = coeff[i];
      if (qcoeff[i]!=0) *cbp = 1;
    }
  }
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- H263dequant -- H.263-based de-quantization
 *
 * Author :            Karl Olav Lillevold		
 *	
 * Purpose :           Quantization of DCT-transform coefficients
 *
 * Arguments in :      int *qcoeff :   pointer to quantized coefficients.
 *                     int intradc :   intra DC coefficient.
 *                     int mbQuant:    Quantization parameter of macroblock
 *                     int mode   :    
 *                     int cbp     :   coded block pattern
 *
 * Arguments in/out :  none
 *
 * Arguments out :     int *coeff:     dequantized coefficients.
 *
 * Return values :     none
 *	
 * Side effects :      none
 *	
 * Description :       De-Quantizes 'qcoeff' based on H.263 rules given the 
 *                     macroblock 'mode' and the quantization parameter 
 *	               'mbQuant'.
 *	
 * See also :          H263quant
 *	
 * Modified :        - T.W., 12-Nov-96, adapted from TMN-2.0
 *                     (Cross-checked to TMN-version: same output, overflow
 *                      observed in both versions--clipping necessary )
 *                   - K.S., 15-May-97, introduced 'cbp'-check.
 *
 ****************************************************************************/
void H263dequant(int *qcoeff, int intradc, int mbQuant, int mode, int cbp,
		 int *coeff)
/***********************************************************CommentEnd********/
{
  int i;
  int *c = coeff, *qc = qcoeff;


  if (cbp) {
    if (mbQuant>0) {
      if (mode == MODE_INTRA || mode == MODE_INTRA_Q) { /* Intra */
	*c = intradc;
	qc++;
	c++;
	for (i = 1; i < 64; i++, c++, qc++) {
	  if (*qc != 0) {
	    if ((mbQuant % 2) == 1) {
	      *c = mbQuant * (2*ABS(*qc) + 1);
	    } else {
	      *c = mbQuant * (2*ABS(*qc) + 1) - 1;
	    }
	    *c = SGN(*qc) * *c;
	  } else {
	    *c = 0;
	  }
	}
      } else { /* Inter */
	for (i = 0; i < 64; i++, c++, qc++) {
	  if (*qc != 0) {
	    if ((mbQuant % 2) == 1) {
	      *c = mbQuant * (2*ABS(*qc) + 1);
	    } else {
	      *c = mbQuant * (2*ABS(*qc) + 1) - 1;
	    }
	    *c = SGN(*qc) * *c;
	  } else { 
	    *c = 0;
	  }
	}
      }
    } else {
      /* No quantization */
      for (i = 0; i < 64; i++) {
	coeff[i] = qcoeff[i];
      }
    }
  } else {     /* cbp == 0 */
    memset(coeff, 0, 64 * sizeof(int));
    if (mode == MODE_INTRA || mode == MODE_INTRA_Q)  /* Intra */
      coeff[0] = intradc;
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263initQuantTables -- Initialization of H.263 quant and dequant tables
 *
 * Author :            Klaus Stuhlmueller
 *
 * Purpose :           Initialization tables for fast quant-/dequantization
 *
 * Arguments in :      int q   Quantization parameter
 *
 * Arguments in/out :  -
 *
 * Arguments out :     -
 *
 * Return values :     none
 *
 * Side effects :      modifies 'quant_table' and 'dequant_table'.
 *
 * Description :       -
 *
 * See also :          H263quant, H263dequant
 *
 * Modified :          -
 *
 *****************************************************************************/
void H263initQuantTables(int q)
/***********************************************************CommentEnd********/
{
  int i;
  int Qd = q << 1;
  int oddQ = (q % 2);

  /* Quant table */
  quant_table[0] = 0;
  for (i = 2040; i; i--)
    quant_table[i] = clp[(i / Qd) << 1] >> 1;
  for (i = 2040; i; i--)
    quant_table[-i] = - quant_table[i];

  /* Dequant table */
  dequant_table[0] = 0;
  for (i = 127; i; i--)
    dequant_table[i] = q * (2 * i + 1) - (oddQ ? 0 : 1);
  for (i = 127; i; i--)
    dequant_table[-i] = - dequant_table[i];
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263quantTable -- H.263-based quantization using quant table
 *
 * Author :            Klaus Stuhlmueller
 *
 * Purpose :           Quantization of DCT-transform coefficients
 *
 * Arguments in :      int *coeff    pointer to (zigzag-scanned)
 *                                   transform coefficients
 *                     int mbQuant   Quantization parameter of macroblock
 *                     int mode      macroblock mode
 *
 * Arguments in/out :  -
 *
 * Arguments out :     int *qcoeff   quantized coefficients.
 *                     int *intradc  intra DC coefficient.
 *                     int *cbp      coded block pattern
 *                                   0 if all 'qcoeff's and the 'intradc' are 0
 *                                   1 if any 'qcoeff' or the 'intradc' isnot 0
 *
 * Return values :     none
 *
 * Side effects :      none
 *
 * Description :       Quantizes 'coeff' based on H.263 rules given the
 *                     macroblock 'mode' and the quantization parameter
 *	               'mbQuant'.
 *
 * See also :          H263quant, H263initQuantTables, H263dequantTable
 *
 * Modified :          -
 *
 *****************************************************************************/
void H263quantTable(int *coeff, int mbQuant, int mode, int *qcoeff, int *intradc,
		    int *cbp)
/***********************************************************CommentEnd********/
{
  int i;
  int mbQh = mbQuant >> 1;
  int *qc = qcoeff;
  int *c = coeff;


  *cbp = 0;

  if (mode == MODE_INTRA || mode == MODE_INTRA_Q) { /* Intra */
    *intradc = CLIP(*c/8, 1, 254)*8;
    *qc = *intradc;
    qc++;
    c++;
    for (i = 1; i < 64; i++, c++) {
      *qc = quant_table[*c];
      if (*qc++!=0) *cbp = 1;
    }
  } else { /* non Intra */
    for (i = 0; i < 64; i++, c++) {
      *qc = quant_table[*c - mbQh];
      if (*qc++!=0) *cbp = 1;
    }
  }
}


/***********************************************************CommentBegin******
 ****************************************************************************
 *
 * -- H263dequantTable -- H.263-based de-quantization using dequant table
 *
 * Author :            Klaus Stuhlmueller
 *	
 * Purpose :           Quantization of DCT-transform coefficients
 *
 * Arguments in :      int *qcoeff :   pointer to quantized coefficients.
 *                     int intradc :   intra DC coefficient.
 *                     int mbQuant:    Quantization parameter of macroblock
 *                     int mode   :    
 *                     int cbp     :   coded block pattern
 *
 * Arguments in/out :  none
 *
 * Arguments out :     int *coeff:     dequantized coefficients.
 *
 * Return values :     none
 *	
 * Side effects :      none
 *	
 * Description :       De-Quantizes 'qcoeff' based on H.263 rules given the 
 *                     macroblock 'mode' and the quantization parameter 
 *	               'mbQuant'.
 *	
 * See also :          H263dequant, H263initQuantTables, H263quantTable
 *	
 * Modified :          -
 *
 ****************************************************************************/
void H263dequantTable(int *qcoeff, int intradc, int mbQuant, int mode, int cbp,
		      int *coeff)
/***********************************************************CommentEnd********/
{
  int i;
  int *c = coeff, *qc = qcoeff;

  //SV-XXX: unused
  UNUSED(mbQuant);

  if (cbp) {
    if (mode == MODE_INTRA || mode == MODE_INTRA_Q) { /* Intra */
      *c = intradc;
      qc++;
      c++;
      for (i = 1; i < 64; i++, c++, qc++)
	*c = dequant_table[*qc];
    } else { /* Inter */
      for (i = 0; i < 64; i++, c++, qc++)
	*c = dequant_table[*qc];
    }
  } else {     /* cbp == 0 */
    memset(coeff, 0, 64 * sizeof(int));
    if (mode == MODE_INTRA || mode == MODE_INTRA_Q)  /* Intra */
      coeff[0] = intradc;
  }
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263allocMbStruct -- Allocate a macroblock struct
 *
 * Author:           Thomas Wiegand
 *
 * Created:          07-March-97	
 *	
 * Purpose:          Allocate a macroblockblock struct
 *
 * Arguments in:     numberOfMVs : number of motion vectors (usually 0,1,4 or 5)
 *                   numberOfBLKs : number of blocks (usually 6 or 12)
 *                   numberOfLevels : number of levels (usually 64)
 *
 * Arguments in/out: -
 *
 * Arguments out:    -
 *
 * Return values:    Pointer to the new macrblock struct on success
 *                   NULL on error (failed calloc)
 *
 * Example:          mb = H263allocMbStruct(1,6,64);
 *
 * Side effects:     -
 *
 * Description:      Allocates a structure of type H263mbStruct with
 *                   the specified number of motion vectors, blocks and levels.
 *
 * See also:         H263allocBlkStruct(), H263allocPicStruct()
 *	
 * Modified:         -
 *	
 *****************************************************************************/
H263mbStruct *H263allocMbStruct(int numberOfMVs, int numberOfBLKs, 
				int numberOfLevels)
/***********************************************************CommentEnd********/
{
  int i;
  H263mbStruct *mb = (H263mbStruct *)malloc(1 * sizeof(H263mbStruct));
  if(mb==NULL) return 0;

  /* allocate CBP */
  mb->cbp = (int *)malloc(numberOfBLKs * sizeof(int));
  if(mb->cbp==NULL) return 0;

  /* allocate MVs */
  mb->mv = (MVector **)malloc(numberOfMVs*sizeof(MVector *));    
  if(mb->mv==NULL) return 0;
  mb->pv = (MVector **)malloc(numberOfMVs*sizeof(MVector *));    
  if(mb->pv==NULL) return 0;
  for(i=0;i<numberOfMVs;i++) {
    mb->mv[i] = (MVector *)malloc(1 * sizeof(MVector));
    mb->mv[i]->sF = 2;
    mb->pv[i] = (MVector *)calloc(1, sizeof(MVector));
    mb->pv[i]->sF = 2;
  }  

  /* allocate BLKs */
  mb->blk = (H263blkStruct **)malloc(numberOfBLKs * sizeof(H263blkStruct *));
  if(mb->blk==NULL) return 0;
  for(i=0;i<numberOfBLKs;i++) {
    mb->blk[i] = H263allocBlkStruct(numberOfLevels);
    if(mb->blk[i]==NULL) return 0;
  }  

  return(mb);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- H263allocBlkStruct -- Allocate a block struct
 *
 * Author:           Thomas Wiegand
 *
 * Created:          07-March-97	
 *	
 * Purpose:          Allocate a block struct
 *
 * Arguments in:     numberOfLevels : number of levels (usually 64)
 *
 * Arguments in/out: -
 *
 * Arguments out:    -
 *
 * Return values:    Pointer to the new block struct on success
 *                   NULL on error (failed calloc)
 *
 * Example:          blk = H263allocBlkStruct(64);
 *
 * Side effects:     -
 *
 * Description:      Allocates a structure of type H263blkStruct with
 *                   the specified number of levels (usually 64).
 *
 * See also:         H263allocMbStruct(), H263allocPicStruct()
 *	
 * Modified:         -
 *	
 *****************************************************************************/
H263blkStruct *H263allocBlkStruct(int numberOfLevels)
/***********************************************************CommentEnd********/
{
  H263blkStruct *blk = (H263blkStruct *)malloc(1 * sizeof(H263blkStruct));
  
  /* allocate LEVELs */
  blk->level = (int *)calloc(numberOfLevels,sizeof(int));
  if(blk->level==NULL) return 0;      

  return(blk);
}
