#include <stdlib.h>
#include <string.h>
#include "h263decoder.h"
#include "h263.h"
#include "bitIn.h"


H263Global *NewH263Decoder(void) {
  H263Global *h263Data;

  h263Data = NewH263DecoderNoBuffer();
  InitStreamBuffer(h263Data->bs, 1);

  return(h263Data);
}

H263Global *NewH263DecoderNoBuffer(void) {
  H263Global *h263Data = (H263Global *) malloc(sizeof(H263Global));

  h263Data->bs = (Bitstream *) malloc(sizeof(Bitstream));
  InitStreamBuffer(h263Data->bs, 0);

  h263Data->first = 1;
  h263Data->expand = 0;

  h263Data->ptype = 0;

  h263Data->decGOBsCounter = 0;

  h263Data->gobs_in_pict = 100000; /* Just an utopic high number */
  memset(&(h263Data->decGOBs),0,sizeof(h263Data->decGOBs));

  return(h263Data);
}

void FreeH263Decoder(H263Global *h263Data)
{
  FreeStreamBuffer(h263Data->bs);
  free(h263Data->bs);
  free(h263Data);
}



/* Decode one GOB (or a picture) including the header */
/* Input Arguments:  h263Data->...
   Bitstream *bs
   int       first     set to 1 if the function is called the first time.
                       In this case memory is allocated.
		       Otherwise set to 0.
   int       quiet     set to 1 to suppress warnings.

   int dec_until_sync  1: decode until a sync is found.
                       0: decode exactly one GOB.

   Output Arguments:  h263Data->...
   unsigned char *bframe[3]           decoded B-frame if 'h263Data->pb_frame == 1'.
   unsigned char *newframe[3]         decoded frame.
   int           temp_ref             temporal reference (of I- or P-frame).
   int           coded_picture_width  width of 'bframe[0]' and 'newframe[0]'.
   int           coded_picture_height height of 'bframe[0]' and 'newframe[0]'.
   int           dec_gobs             number of decoded gobs in this picture.
   unsigned char gob_dec[]            flag if the GOB is already decoded.
*/
int H263DecodeGOB(H263Global *h263Data, int dec_until_sync)
{

	if (!_getheader(h263Data)) {
    H263ConcealGOB(h263Data);
    h263Data->decGOBsCounter++;
    h263Data->decGOBs[h263Data->gob] = 1;
    h263Data->bs->BufState = GOB_EMPTY;
    return 0;
  }

  H263getGOB(h263Data, dec_until_sync);
  h263Data->bs->BufState = GOB_COMPLETE;
  return 1;
}


void newpictureinit(H263Global *h263Data)
{
  int i;
  unsigned char *tmp;


  h263Data->decGOBsCounter = 0;
  memset(h263Data->decGOBs, 0, MAX_GOBS * sizeof(unsigned char));

  for (i=0; i<3; i++) {
    tmp = h263Data->oldrefframe[i];
    h263Data->oldrefframe[i] = h263Data->refframe[i];
    h263Data->refframe[i] = tmp;
    h263Data->newframe[i] = h263Data->refframe[i];
  }

  if (h263Data->mv_outside_frame && h263Data->pict_type == PCT_INTER) {
    make_edge_image(h263Data->oldrefframe[0], h263Data->edgeframe[0],
		    h263Data->coded_picture_width,
		    h263Data->coded_picture_height,32);
    make_edge_image(h263Data->oldrefframe[1], h263Data->edgeframe[1],
		    h263Data->chrom_width, h263Data->chrom_height,16);
    make_edge_image(h263Data->oldrefframe[2], h263Data->edgeframe[2],
		    h263Data->chrom_width, h263Data->chrom_height,16);
  }

}

/* This function needs only 'h263Data->source_format' as input */
void InitH263Decoder(H263Global *h263Data)
{
  int i, k;
  int ysize, csize, size;


  h263Data->first = 0;

  switch (h263Data->source_format) {
  case SQCIF:
    h263Data->horizontal_size = 128;
    h263Data->vertical_size = 96;
    h263Data->mbs_in_gob = 128 / 16;
    h263Data->gobs_in_pict = 96 / 16;
    break;
  case QCIF:
    h263Data->horizontal_size = 176;
    h263Data->vertical_size = 144;
    h263Data->mbs_in_gob = 176 / 16;
    h263Data->gobs_in_pict = 144 / 16;
    break;
  case CIF:
    h263Data->horizontal_size = 352;
    h263Data->vertical_size = 288;
    h263Data->mbs_in_gob = 352 / 16;
    h263Data->gobs_in_pict = 288 / 16;
    break;
  case CIF4:
    h263Data->horizontal_size = 704;
    h263Data->vertical_size = 576;
    h263Data->mbs_in_gob = 2 * 704 / 16;
    h263Data->gobs_in_pict = 576 / 16 / 2;
    break;
  case CIF16:
    h263Data->horizontal_size = 1408;
    h263Data->vertical_size = 1152;
    h263Data->mbs_in_gob = 4 * 1408 / 16;
    h263Data->gobs_in_pict = 1152 / 16 / 4;
    break;
  case CUSTOM_SOURCE_FORMAT:
    h263Data->mbs_in_gob = h263Data->horizontal_size / 16;
    h263Data->gobs_in_pict = h263Data->vertical_size / 16;
    if (h263Data->vertical_size > 800) {
      h263Data->mbs_in_gob *= 4;
      h263Data->gobs_in_pict /= 4;
    } else if (h263Data->vertical_size > 400) {
      h263Data->mbs_in_gob *= 2;
      h263Data->gobs_in_pict /= 2;
    }
    break;
  default:
    printf("ERROR: Illegal input format\n");
    exit(-1);
    break;
  }


  h263Data->mb_width = h263Data->horizontal_size/16;
  h263Data->mb_height = h263Data->vertical_size/16;
  h263Data->coded_picture_width = h263Data->horizontal_size;
  h263Data->coded_picture_height = h263Data->vertical_size;
  h263Data->chrom_width =  h263Data->coded_picture_width>>1;
  h263Data->chrom_height = h263Data->coded_picture_height>>1;

  ysize = h263Data->coded_picture_width*h263Data->coded_picture_height;
  csize = h263Data->chrom_width*h263Data->chrom_height;
  size = ysize + 2 * csize;

  /* refframe (= newframe) */
  if (!(h263Data->refframe[0] = (unsigned char *)malloc(size)))
    EXIT_WITH_ERR("malloc failed\n");
  h263Data->refframe[1] = h263Data->refframe[0] + ysize;
  h263Data->refframe[2] = h263Data->refframe[1] + csize;
  /* Init with 128 */
  memset(h263Data->refframe[0], 128, size);

  /* oldrefframe */
  if (!(h263Data->oldrefframe[0] = (unsigned char *)malloc(size)))
    EXIT_WITH_ERR("malloc failed\n");
  h263Data->oldrefframe[1] = h263Data->oldrefframe[0] + ysize;
  h263Data->oldrefframe[2] = h263Data->oldrefframe[1] + csize;

  /* bframe */
  if (!(h263Data->bframe[0] = (unsigned char *)malloc(size)))
    EXIT_WITH_ERR("malloc failed\n");
  h263Data->bframe[1] = h263Data->bframe[0] + ysize;
  h263Data->bframe[2] = h263Data->bframe[1] + csize;

  /* edgeframes */
  ysize = (h263Data->coded_picture_width+64) *
    (h263Data->coded_picture_height+64);
  csize = (h263Data->chrom_width+32)*(h263Data->chrom_height+32);
  size = ysize + 2 * csize;
  if (!(h263Data->edgeframeorig[0] = (unsigned char *)malloc(size)))
    EXIT_WITH_ERR("malloc failed\n");
  h263Data->edgeframe[0] = h263Data->edgeframeorig[0] +
    (h263Data->coded_picture_width+64) * 32 + 32;
  h263Data->edgeframeorig[1] = h263Data->edgeframeorig[0] + ysize;
  h263Data->edgeframe[1] = h263Data->edgeframeorig[1] +
    (h263Data->chrom_width+32) * 16 + 16;
  h263Data->edgeframeorig[2] = h263Data->edgeframeorig[1] + csize;
  h263Data->edgeframe[2] = h263Data->edgeframeorig[2] +
    (h263Data->chrom_width+32) * 16 + 16;

  if (h263Data->expand) {
    ysize = h263Data->coded_picture_width*h263Data->coded_picture_height*4;
    csize = h263Data->chrom_width*h263Data->chrom_height*4;
    size = ysize + 2 * csize;
    if (!(h263Data->exnewframe[0] = (unsigned char *)malloc(size)))
      EXIT_WITH_ERR("malloc failed\n");
    h263Data->exnewframe[1] = h263Data->exnewframe[1] + ysize;
    h263Data->exnewframe[2] = h263Data->exnewframe[2] + csize;
  } else
    h263Data->exnewframe[0] = NULL;

  /* mark MV's above the picture */
  for (i = 1; i < h263Data->mb_width+1; i++) {
    for (k = 0; k < 5; k++) {
      h263Data->MV[0][k][0][i] = NO_VEC;
      h263Data->MV[1][k][0][i] = NO_VEC;
    }
    h263Data->modemap[0][i] = MODE_INTRA;
  }
  /* zero MV's on the sides of the picture */
  for (i = 0; i < h263Data->mb_height+1; i++) {
    for (k = 0; k < 5; k++) {
      h263Data->MV[0][k][i][0] = 0;
      h263Data->MV[1][k][i][0] = 0;
      h263Data->MV[0][k][i][h263Data->mb_width+1] = 0;
      h263Data->MV[1][k][i][h263Data->mb_width+1] = 0;
    }
    h263Data->modemap[i][0] = MODE_INTRA;
    h263Data->modemap[i][h263Data->mb_width+1] = MODE_INTRA;
  }
}


void DisinitH263Decoder(H263Global *h263Data)
{
    if (!h263Data->first) {
        h263Data->first = 1;

        free(h263Data->refframe[0]);
        free(h263Data->oldrefframe[0]);
        free(h263Data->bframe[0]);

        free(h263Data->edgeframeorig[0]);

        if (h263Data->expand) {
            free(h263Data->exnewframe[0]);
            h263Data->exnewframe[0] = NULL;
        }
    }
}


void H263FinishPicture(H263Global *h263Data)
{
  int gn;
  int n = 0;


  if (!h263Data->first) {
    for (gn = 0; gn < h263Data->gobs_in_pict; gn++)
      if (!h263Data->decGOBs[gn]) {
	h263Data->gob = gn;
	H263ConcealGOB(h263Data);
	n++;
      }

#if 0
    if (n == 1 && h263Data->decGOBs[0]) {
      /* We have only decoded the first GOB;
	 Thus we have to set GFID to an unreachable number.
	 In that way decoding can only continue with a new picture header. */
      h263Data->gfid = 4;
    }
#endif

    memset(h263Data->decGOBs, 0, MAX_GOBS * sizeof(unsigned char));
    h263Data->decGOBsCounter = 0;
  }
}
