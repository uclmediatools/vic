#include "defs.h"

#include "Util.h"

#include "bitOut.h"
#include "code.h"

#include "h263encoder.h"

#include "structs.h"
#include "h263encoder.p"

#include "H263_mcbpcP.h"
#include "H263_mcbpcI.h"
#include "H263_modb.h"
#include "H263_cbpb.h"
#include "H263_cbpyP.h"
#include "H263_cbpyI.h"
#include "H263_mvd.h"
#include "H263_intradc.h"
#include "H263_tcoef.h"
#include "H263_escrun.h"
#include "H263_esclevel.h"

#include "common.h"
#include "common.p" //SV-XXX: avoids implicit declarations


static int izigzag[64] = {
   0, 1, 8,16, 9, 2, 3,10,
  17,24,32,25,18,11, 4, 5,
  12,19,26,33,40,48,41,34,
  27,20,13, 6, 7,14,21,28,
  35,42,49,56,57,50,43,36,
  29,22,15,23,30,37,44,51,
  58,59,52,45,38,31,39,46,
  53,60,61,54,47,55,62,63
};


int H263putPictureHeader(int tr, int sourceFormat, int pictureCodingType,
			 int q, int w, int h, Bitstr *bs)
{
  int umv = 0, sac = 0, ap = 0, pb = 0;
  int cpm = 0, psbi = 0;
  int trb = 0, dbquant = 0;

  int bits = 0;
  int ptype;
  int extendedPTYPE = (sourceFormat == CUSTOM_SOURCE_FORMAT);

  /* PSC */
  PutCodewordX("0000000000000000100000", bs);
  bits += 22;

  /* TR */
  PutNumberX(tr, 8, bs);
  bits += 8;  

  /* PTYPE */
  ptype = 0x1000;
  if (!extendedPTYPE) {
    ptype |= ((sourceFormat << 5) & 0xe0);
    if (pictureCodingType == PICTURE_CODING_TYPE_INTER)
      ptype |= 0x10;
    if (umv)
      ptype |= 0x8;
    if (sac)
      ptype |= 0x4;
    if (ap)
      ptype |= 0x2;
    if (pb)
      ptype |= 0x1;
    PutNumberX(ptype, 13, bs);
    bits += 13;
  } else {       /* Extended PTYPE */
    ptype |= ((7 << 5) & 0xe0);
    PutNumberX(ptype>>5, 8, bs);
    bits += 8;
    /* PLUSHEADER */
    bits += PutPLUSHEADER(sourceFormat, w, h, umv, sac, ap,
			  pictureCodingType, cpm, psbi, bs);
  }

  /* PQUANT */
  PutNumberX(q, 5, bs);
  bits += 5;

  if (!extendedPTYPE) {  /* Only if not Extended PTYPE */
    /* CPM */
    PutBitX(cpm,bs);
    bits += 1;

    /* PSBI */
    if(cpm) {
      PutNumberX(psbi, 2, bs);
      bits += 2;
    }
  }

  /* TRB, DBQUANT */
  if(pb) {
    PutNumberX(trb, 3, bs);
    bits += 3;
    PutNumberX(dbquant, 2, bs);
    bits += 2;
  }

  /* PEI */
  PutBitX(0,bs);      
  bits += 1;

  return(bits);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- PutPLUSHEADER -- Put PLUSHEADER bits to bit-stream
 *
 * Author:           Niko Faerber
 * Created:          26-Feb-98
 *****************************************************************************/
int PutPLUSHEADER(int sourceFormat, int w, int h, int umv, int sac, int ap,
		  int pictureCodingType, int cpm, int psbi, Bitstr *bs)
/***********************************************************CommentEnd********/
{
  int pcf = 0, aic = 0, df = 0, ss = 0, rps = 0, isd = 0, aiv = 0, mq = 0,
      rpr = 0, rru = 0, rtype = 0;
  int ufep = 0, rpsmf = 0;
  int trpi = 0, trp = 0;
  int bits = 0;


  /* UFEP */
  if (pictureCodingType == PICTURE_CODING_TYPE_INTRA)
    ufep = 1;

  /* PLUSPTYPE */
  bits = PutPLUSPTYPE(ufep, sourceFormat, w, h, pcf, umv, sac, ap, aic,
		      df, ss, rps, isd, aiv, mq,
		      pictureCodingType, rpr, rru, rtype, bs);
  /* CPM */
  PutBitX(cpm, bs);
  bits++;
  /* PSBI */
  if(cpm) {
    PutNumberX(psbi, 2, bs);
    bits += 2;
  }

  /* Note: For modes not jet supported, no bits are written.
   * Please extend when adding a mode.
   */

  /* CPFMT */
  if (ufep && sourceFormat == CUSTOM_SOURCE_FORMAT) {
    /* Pixel Aspect Ratio Code */
    PutNumberX(1, 4, bs);
    /* Picture width */
    PutNumberX(w/4 - 1, 9, bs);
    PutBitX(1, bs);
    /* Picture height */
    PutNumberX(h/4, 9, bs);
    bits += 23;
  }
  /* EPAR */
  /* CPCFC */
  /* ETR */
  /* UUI */
  /* SS */
  /* ELNUM */
  /* RLNUM */

  if (rps) {
    /* RPSMF */
    if (ufep) {
      PutNumberX(rpsmf, 3, bs);
      bits += 3;
    }
    /* TRPI, TRP */
    PutBitX(trpi,bs);
    bits++;
    if (trpi) {
      PutNumberX(trp, 10, bs);
      bits += 10;
    }
    /* BCI */
    PutBitX(0,bs);
    bits++;
  }

  /* RPRP LAYER */

  return(bits);
}


/***********************************************************CommentBegin******
 *****************************************************************************
 *
 * -- PutPLUSPTYPE -- Put PLUSPTYPE bits to bit-stream
 *
 * Author:           Niko Faerber
 * Created:          26-Feb-98
 *****************************************************************************/
int PutPLUSPTYPE(int ufep, int sourceFormat, int w, int h,
		 int pcf, int umv, int sac, int ap, int aic,
		 int df, int ss, int rps, int isd, int aiv, int mq,
		 int pictureCodingType, int rpr, int rru, int rtype,
		 Bitstr *bs)
/***********************************************************CommentEnd********/
{
  int bits = 0;

  //SV-XXX: unused params
  UNUSED(w);
  UNUSED(h);

  /* UFEP */
  PutNumberX(ufep, 3, bs);
  bits += 3;
  /* OPPTYPE (Optional Part) */
  if(ufep) {
    PutNumberX(sourceFormat, 3, bs);
    PutBitX(pcf,bs);
    PutBitX(umv,bs);
    PutBitX(sac,bs);
    PutBitX(ap,bs);
    PutBitX(aic,bs);
    PutBitX(df,bs);
    PutBitX(ss,bs);
    PutBitX(rps,bs);
    PutBitX(isd,bs);
    PutBitX(aiv,bs);
    PutBitX(mq,bs);
    PutBitX(1,bs);
    PutBitX(0,bs);
    PutBitX(0,bs);
    PutBitX(0,bs);
    bits += 18;
  }
  /* MPPTYPE (Mandatory Part) */
  PutNumberX(pictureCodingType, 3, bs);
  PutBitX(rpr,bs);
  PutBitX(rru,bs);
  PutBitX(rtype,bs);
  PutBitX(0,bs);
  PutBitX(0,bs);
  PutBitX(1,bs);
  bits += 9;

  return(bits);
}


int H263putGOBHeader(int GOBnumber, int gfid, int q, Bitstr *bitstream)
{
  int cpm = 0, gsbi = 0;

  int bits = 0;


  /* GSTUF to byte align GOB Sync */
  bits += ByteAlign0X(bitstream);

  /* GOB start code (GBSC) */
  PutCodewordX("00000000000000001", bitstream);
  bits += 17;

  /* GOB number (GN) */
  PutNumberX(GOBnumber, 5, bitstream);
  bits += 5;

  /* GOB sub bitstream indicator (GSBI) */
  if (cpm) {   /* Only present in continuous presence multipoint mode */
    PutNumberX(gsbi, 2, bitstream);
    bits += 2;
  }

  /* GOB frame ID (GFID) */
  PutNumberX(gfid, 2, bitstream);
  bits += 2;

  /* GQuant */
  PutNumberX(q, 5, bitstream);
  bits += 5;

  return(bits);
}



int H263putIFrameMB(H263mbStruct *mb, int dq, Bitstr *bs) 
{
  int bits = 0;
  int k;


  /* MCBPC */
  bits = H263putIFrameMCBPC(mb->type, mb->cbp, bs); 
  
  /* CBPY I-Mbs */
  bits += H263putCBPY(MODE_INTRA, mb->cbp, bs);
  
  /* DQUANT */
  if (mb->type == MODE_INTRA_Q || mb->type == MODE_INTER_Q)
    bits += H263putDQUANT(dq, bs);
  
  /* BLKs */
  for (k = 0; k < 6; k++) {
    bits += H263putINTRADC(mb->blk[k]->intradc, bs);
    if (mb->cbp[k]==1) {
      bits += H263putDCTBitsZigzag(mb->blk[k], bs, 1);
    }
  }

  return(bits);
}


int H263putPFrameMB(H263mbStruct *mb, int dq, Bitstr *bs) 
{
  int umv = 0;
  int bits = 0;
  int k;
  int intra_f;


  if (mb->type == MODE_UNCODED) {
    PutBitX(1, bs);
    bits += 1;
  } else {
    PutBitX(0, bs);
    bits += 1;

    /* MCBPC */
    bits += H263putPFrameMCBPC(mb->type, mb->cbp, bs); 
    
    /* CBPY */
    bits += H263putCBPY(mb->type, mb->cbp, bs);

    /* DQUANT */
    if (mb->type == MODE_INTRA_Q || mb->type == MODE_INTER_Q)
      bits += H263putDQUANT(dq, bs);
  
    /* MVs */
    if (mb->type == MODE_INTER || mb->type == MODE_INTER_Q ||
	mb->type == MODE_INTER4V) {
      bits += H263putMVD(mb, 0, umv, bs);
      if (mb->type == MODE_INTER4V) {
	bits += H263putMVD(mb, 1, umv, bs);
	bits += H263putMVD(mb, 2, umv, bs);
	bits += H263putMVD(mb, 3, umv, bs);
      }
    }

    /* BLKs */
    for (k = 0; k < 6; k++) {        
      if (mb->type==MODE_INTRA || mb->type==MODE_INTRA_Q) {
	bits += H263putINTRADC(mb->blk[k]->intradc, bs);
	intra_f = 1;
      } else
	intra_f = 0;
      if (mb->cbp[k] == 1) {
	bits += H263putDCTBitsZigzag(mb->blk[k], bs, intra_f);
      }
    }
  }  

  return(bits);
}


int H263putIFrameMCBPC(int type, int *cbpc, Bitstr *bs) 
{
  static Code *code = NULL;
  int index;


  if (code == NULL) {
    int  mcbpcI_event[H263_MCBPC_I_SIZE * H263_MCBPC_I_EVENT_DIM] =
                         H263_MCBPC_I_EVENT;
    char *mcbpcI_bits[H263_MCBPC_I_SIZE] = H263_MCBPC_I_BITS;

    code = InitCode("H263-MCBPC-Intra", H263_MCBPC_I_SIZE,
		    H263_MCBPC_I_EVENT_DIM, mcbpcI_event, mcbpcI_bits);
  }

  index = (type - 3) << 2;
  index |= (cbpc[4] << 1);
  index |= cbpc[5];

  PutIndexToBitstr(code, index, bs);

  return(code->cwTable[index].size);
}


int H263putPFrameMCBPC(int type, int *cbpc, Bitstr *bs) 
{
  static Code *code = NULL;
  int index;


  if (code == NULL) {
    int  mcbpcP_event[H263_MCBPC_P_SIZE * H263_MCBPC_P_EVENT_DIM] =
                         H263_MCBPC_P_EVENT;
    char *mcbpcP_bits[H263_MCBPC_P_SIZE] = H263_MCBPC_P_BITS;

    code = InitCode("H263-MCBPC-Inter", H263_MCBPC_P_SIZE,
		    H263_MCBPC_P_EVENT_DIM, mcbpcP_event, mcbpcP_bits);
  }

  index = type << 2;
  index |= (cbpc[4] << 1);
  index |= cbpc[5];

  PutIndexToBitstr(code, index, bs);

  return(code->cwTable[index].size);
}


int H263putCBPY(int type, int *cbpy, Bitstr *bs) 
{
  static Code *codeI = NULL, *codeP = NULL;
  Code *code = NULL;
  int  index;


  if (codeI == NULL) {
    int  cbpyI_event[H263_CBPY_I_SIZE * H263_CBPY_I_EVENT_DIM] =
                     H263_CBPY_I_EVENT;
    char *cbpyI_bits[H263_CBPY_I_SIZE] = H263_CBPY_I_BITS;
    int  cbpyP_event[H263_CBPY_P_SIZE * H263_CBPY_P_EVENT_DIM] =
                     H263_CBPY_P_EVENT;
    char *cbpyP_bits[H263_CBPY_P_SIZE] = H263_CBPY_P_BITS;

    codeI = InitCode("H263-CBPY-I", H263_CBPY_I_SIZE,
		    H263_CBPY_I_EVENT_DIM, cbpyI_event, cbpyI_bits);

    codeP = InitCode("H263-CBPY-P", H263_CBPY_P_SIZE,
		    H263_CBPY_P_EVENT_DIM, cbpyP_event, cbpyP_bits);
  }

  index = cbpy[0] << 3;
  index |= (cbpy[1] << 2);
  index |= (cbpy[2] << 1);
  index |= cbpy[3];

  if (type==MODE_INTER || type==MODE_INTER_Q || type==MODE_INTER4V) {
    /* CBPY P-Mbs */
    code = codeP;
  } else if (type==MODE_INTRA || type==MODE_INTRA_Q) {
    /* CBPY I-Mbs */
    code = codeI;
  }

  PutIndexToBitstr(code, index, bs);

  return(code->cwTable[index].size);
}


int H263putDQUANT(int dq, Bitstr *bs) 
{
  int n;


  switch(dq) {
  case -1:
    n = 0;
    break;
  case -2:
    n = 1;
    break;
  case 1:
    n = 2;
    break;
  case 2:
    n = 0;
    break;
  default:
    return(0);
  }

  PutNumberX(n, 2, bs);

  return(2);
}


int H263putINTRADC(int intradc, Bitstr *bs) 
{
  static Code *code = NULL;
  int index;


  if (code == NULL) {
    int  intradc_event[H263_INTRA_DC_SIZE * H263_INTRA_DC_EVENT_DIM] =
                       H263_INTRA_DC_EVENT;
    char *intradc_bits[H263_INTRA_DC_SIZE] = H263_INTRA_DC_BITS;

    code = InitCode("H263-INTRA-DC", H263_INTRA_DC_SIZE,
		    H263_INTRA_DC_EVENT_DIM, intradc_event, intradc_bits);
  }

  index = (intradc >> 3) - 1;
  PutIndexToBitstr(code, index, bs);

  return(code->cwTable[index].size);
}


int H263putDCTBitsZigzag(H263blkStruct *blk, Bitstr *bs, int intra_f)
{
  int bits = 0;

  int i,j;
  int index;
  int lev, level;
  int run = 0;


  static int  virgin = TRUE;
  static Code *tcoefCode, *escrunCode, *esclevelCode;


  if (virgin) {
    int  tcoef_event[H263_TCOEF_SIZE * H263_TCOEF_EVENT_DIM] =
                     H263_TCOEF_EVENT;
    char *tcoef_bits[H263_TCOEF_SIZE] = H263_TCOEF_BITS;
    int  escrun_event[H263_ESCRUN_SIZE * H263_ESCRUN_EVENT_DIM] =
                      H263_ESCRUN_EVENT;
    char *escrun_bits[H263_ESCRUN_SIZE] = H263_ESCRUN_BITS;
    int  esclevel_event[H263_ESCLEVEL_SIZE * H263_ESCLEVEL_EVENT_DIM] =
                        H263_ESCLEVEL_EVENT;
    char *esclevel_bits[H263_ESCLEVEL_SIZE] = H263_ESCLEVEL_BITS;

    tcoefCode = InitCode("H263-TCOEF", H263_TCOEF_SIZE,
			 H263_TCOEF_EVENT_DIM, tcoef_event, tcoef_bits);
    escrunCode = InitCode("H263-ESC-RUN", H263_ESCRUN_SIZE,
			  H263_ESCRUN_EVENT_DIM, escrun_event, escrun_bits);
    esclevelCode = InitCode("H263-ESC-LEVEL", H263_ESCLEVEL_SIZE,
			    H263_ESCLEVEL_EVENT_DIM, esclevel_event,
			    esclevel_bits);
    virgin = FALSE;
  }

  for (j = 63; j >= (intra_f ? 1 : 0); --j) {
    if (blk->level[izigzag[j]]!=0) {
      break;
    }
  }

  for (i = (intra_f ? 1 : 0); i < j; ++i) {
    if((lev = blk->level[izigzag[i]]) == 0) {
      ++run;
    } else {
      level = ABS(lev);

      /* last == 0 */
      switch (run) {
      case 0:
	index = (level <= 12) ? (level - 1) : -1;
	break;
      case 1:
	index = (level <= 6) ? (level + 11) : -1;
	break;
      case 2:
	index = (level <= 4) ? (level + 17) : -1;
	break;
      case 3:
	index = (level <= 3) ? (level + 21) : -1;
	break;
      case 4:
	index = (level <= 3) ? (level + 24) : -1;
	break;
      case 5:
	index = (level <= 3) ? (level + 27) : -1;
	break;
      case 6:
	index = (level <= 3) ? (level + 30) : -1;
	break;
      case 7:
	index = (level <= 2) ? (level + 33) : -1;
	break;
      case 8:
	index = (level <= 2) ? (level + 35) : -1;
	break;
      case 9:
	index = (level <= 2) ? (level + 37) : -1;
	break;
      case 10:
	index = (level <= 2) ? (level + 39) : -1;
	break;
      default:
	if (level == 1) {
	  index = (run <= 26) ? (run + 31) : -1;
	} else
	  index = -1;
	break;
      }

      if (index < 0) {
	/* ESCAPE code */

	/* escape TCOEFF */
	PutIndexToBitstr(tcoefCode, 102, bs); /* ESCAPE code is at pos. 102 */
	bits += tcoefCode->cwTable[102].size;
	/* last TCOEFF */
	PutBitX(0, bs);
	bits += 1; 
	/* run ESCAPE */
	PutIndexToBitstr(escrunCode, run, bs);	
	bits += escrunCode->cwTable[run].size;
	/* level ESCAPE*/
	index = lev + ((lev < 0) ? 127 : 126);
	PutIndexToBitstr(esclevelCode, index, bs);	
	bits += esclevelCode->cwTable[index].size; 
      } else {
	/* TCOEFF */
	PutIndexToBitstr(tcoefCode, index, bs);
	bits += tcoefCode->cwTable[index].size;
	/* sign TCOEFF */
	PutBitX(lev < 0, bs);
	bits += 1;
      }

      run = 0;
    }
  }

  lev = blk->level[izigzag[i]];
  level = ABS(lev);

  switch (run) {
  case 0:
    index = (level <= 3) ? (level + 57) : -1;
    break;
  case 1:
    index = (level <= 2) ? (level + 60) : -1;
    break;
  default:
    if (level == 1)
      index = (run <= 40) ? (run + 61) : -1;
    else
      index = -1;
    break;
  }

  if (index < 0) {
    /* ESCAPE code */
    /* escape TCOEFF */
    PutIndexToBitstr(tcoefCode, 102, bs);
    bits += tcoefCode->cwTable[102].size;
    /* last TCOEFF */
    PutBitX(1, bs);
    bits += 1; 
    /* run ESCAPE */
    PutIndexToBitstr(escrunCode, run, bs);	
    bits += escrunCode->cwTable[run].size;
    /* level ESCAPE*/
    index = lev + ((lev < 0) ? 127 : 126);
    PutIndexToBitstr(esclevelCode, index, bs);	
    bits += esclevelCode->cwTable[index].size; 
  } else {
    /* TCOEFF */
    PutIndexToBitstr(tcoefCode, index, bs);
    bits += tcoefCode->cwTable[index].size;
    /* sign TCOEFF */
    PutBitX(lev < 0, bs);
    bits += 1; 
  }

  return(bits);
}


int H263putMVD(H263mbStruct *mb, int mva, int umv, Bitstr *bs) 
{
  int bits = 0;
  int index;
  static int     virgin = TRUE;
  static MVector *mvd;
  static Code    *code;


  if (virgin) {
    int   mvd_event[H263_MVD_SIZE * H263_MVD_EVENT_DIM] =
                    H263_MVD_EVENT;
    char  *mvd_bits[H263_MVD_SIZE] = H263_MVD_BITS;

    code = InitCode("H263-MVD", H263_MVD_SIZE, H263_MVD_EVENT_DIM,
		    mvd_event, mvd_bits);
    mvd = AllocMVector();
    mvd->sF = 2;
    virgin = FALSE;
  }

  H263subtractMVPrediction(mb->mv[mva], mb->pv[mva], umv, mvd);

  index = 2 * mvd->x + mvd->sx + 32;

  PutIndexToBitstr(code, index, bs);
  bits += code->cwTable[index].size;

  index = 2 * mvd->y + mvd->sy + 32;

  PutIndexToBitstr(code, index, bs);
  bits += code->cwTable[index].size;

  return(bits);
}
