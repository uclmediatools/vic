/* fdct.c */
extern void FdctFast(int *block, int *coeff);
extern void FdctFastByte(Byte *block, int w, int *coeff);
extern void Fdct3coef(int *block, int *coeff);
extern void Fdct3coefByte(Byte *block, int w, int *coeff);
/* h263enc.c */
extern int EncodeH263(int rate, int codingTime, int type, int t, int percentIMBs, Picture *pict, Picture *prevPict, Picture *prevDecPict, unsigned char *crvec, Picture *decPict, Bitstr *bs, int *mbInd, int *mbQuant, MVField *mvField);
extern int EncodeH263Q(int q, int codingTime, int type, int t, int percentIMBs, Picture *pict, Picture *prevPict, Picture *prevDecPict, unsigned char *crvec, Picture *decPict, Bitstr *bs, int *mbInd, int *mbQuant, MVField *mvField);
extern int H263intraPictureEncode(int rate, int codingTime, int t, int gfid, Picture *pict, Picture *decPict, Bitstr *bs, int *mbInd, int *mbQuant, MVField *mvField);
extern int H263intraPictureEncodeQ(int q, int codingTime, int t, int gfid, Picture *pict, Picture *decPict, Bitstr *bs, int *mbInd, int *mbQuant, MVField *mvField);
extern int H263interPictureEncode(int rate, int codingTime, int t, int gfid, int percentIMBs, Picture *pict, Picture *prevPict, Picture *prevDecPict, unsigned char *crvec, Picture *decPict, Bitstr *bs, int *mbInd, int *mbQuant, MVField *mvField);
extern int H263interPictureEncodeQ(int q, int codingTime, int t, int gfid, int percentIMBs, Picture *pict, Picture *prevPict, Picture *prevDecPict, unsigned char *crvec, Picture *decPict, Bitstr *bs, int *mbInd, int *mbQuant, MVField *mvField);
extern int H263dctMbEncodeIFrame(int mx, int my, int q, Picture *pict, Picture *decPict, Bitstr *bs);
extern int H263dctMbEncodePFrame(int mx, int my, H263mbStruct *mb, int codingTime, Picture *pict, Picture *decPict, Bitstr *bs);
extern int H263MbEncodePFrame(int mx, int my, int q, int codingTime, int forceIntra, Picture *pict, Picture *prevPict, Picture *prevDecPict, Byte *interpol[3], unsigned char *crvec, Picture *decPict, MVField *mvField, Bitstr *bs);
extern void H263quant(int *coeff, int mbQuant, int mode, int *qcoeff, int *intradc, int *cbp);
extern void H263dequant(int *qcoeff, int intradc, int mbQuant, int mode, int cbp, int *coeff);
extern void H263initQuantTables(int q);
extern void H263quantTable(int *coeff, int mbQuant, int mode, int *qcoeff, int *intradc, int *cbp);
extern void H263dequantTable(int *qcoeff, int intradc, int mbQuant, int mode, int cbp, int *coeff);
extern H263mbStruct *H263allocMbStruct(int numberOfMVs, int numberOfBLKs, int numberOfLevels);
extern H263blkStruct *H263allocBlkStruct(int numberOfLevels);
/* h263mux.c */
extern int H263putPictureHeader(int tr, int sourceFormat, int pictureCodingType, int q, int w, int h, Bitstr *bs);
extern int PutPLUSHEADER(int sourceFormat, int w, int h, int umv, int sac, int ap, int pictureCodingType, int cpm, int psbi, Bitstr *bs);
extern int PutPLUSPTYPE(int ufep, int sourceFormat, int w, int h, int pcf, int umv, int sac, int ap, int aic, int df, int ss, int rps, int isd, int aiv, int mq, int pictureCodingType, int rpr, int rru, int rtype, Bitstr *bs);
extern int H263putGOBHeader(int GOBnumber, int gfid, int q, Bitstr *bitstream);
extern int H263putIFrameMB(H263mbStruct *mb, int dq, Bitstr *bs);
extern int H263putPFrameMB(H263mbStruct *mb, int dq, Bitstr *bs);
extern int H263putIFrameMCBPC(int type, int *cbpc, Bitstr *bs);
extern int H263putPFrameMCBPC(int type, int *cbpc, Bitstr *bs);
extern int H263putCBPY(int type, int *cbpy, Bitstr *bs);
extern int H263putDQUANT(int dq, Bitstr *bs);
extern int H263putINTRADC(int intradc, Bitstr *bs);
extern int H263putDCTBitsZigzag(H263blkStruct *blk, Bitstr *bs, int intra_f);
extern int H263putMVD(H263mbStruct *mb, int mva, int umv, Bitstr *bs);
/* idct.c */
extern void IdctFastRow(short *blk);
extern void IdctFastColClip(short *blk);
extern void IdctFast(int *coeff, short *block);
extern void IdctFastFloat(int *coeff, int *block);
extern void IdctRef(double *coeff, int *block);
extern void Idct3coef(int *coeff, short *block);
