/* getblk.c */
extern int codtab[2];
extern int mcbpctab[21];
extern int mcbpc_intratab[9];
extern int modb_tab[3];
extern int ycbpb_tab[2];
extern int uvcbpb_tab[2];
extern int cbpytab[16];
extern int cbpy_intratab[16];
extern int dquanttab[4];
extern int mvdtab[64];
extern int intradctab[254];
extern int tcoeftab[103];
extern int signtab[2];
extern int lasttab[2];
extern int last_intratab[2];
extern int runtab[64];
extern int leveltab[254];
extern int cumf_COD[3];
extern int cumf_MCBPC[22];
extern int cumf_MCBPC_intra[10];
extern int cumf_MODB[4];
extern int cumf_YCBPB[3];
extern int cumf_UVCBPB[3];
extern int cumf_CBPY[17];
extern int cumf_CBPY_intra[17];
extern int cumf_DQUANT[5];
extern int cumf_MVD[65];
extern int cumf_INTRADC[255];
extern int cumf_TCOEF1[104];
extern int cumf_TCOEF2[104];
extern int cumf_TCOEF3[104];
extern int cumf_TCOEFr[104];
extern int cumf_TCOEF1_intra[104];
extern int cumf_TCOEF2_intra[104];
extern int cumf_TCOEF3_intra[104];
extern int cumf_TCOEFr_intra[104];
extern int cumf_SIGN[3];
extern int cumf_LAST[3];
extern int cumf_LAST_intra[3];
extern int cumf_RUN[65];
extern int cumf_RUN_intra[65];
extern int cumf_LEVEL[255];
extern int cumf_LEVEL_intra[255];
extern void _getblock(H263Global *h263Data, int comp, int mode);
extern void getblock_ext(Bitstream *bs, short *bp, int comp, int mode, int quant, int bquant, int quiet, int *fault_p);
extern void _get_sac_block(H263Global *h263Data, int comp, int ptype);
/* getgob.c */
extern void H263getGOB(H263Global *h263Data, int dec_until_sync);
extern int H263parseGOB(H263Global *h263Data, int *mbInd, int *mbQuant, MVField *mvField, int indOffset);
extern void H263getIFrameMB(Bitstream *bs, int bx, int by, int w, int h, unsigned char *y, unsigned char *u, unsigned char *v, int *quant_p, int quiet, int *fault_p);
extern void H263parseIFrameMB(Bitstream *bs, int w, int h, int *quant_p, int quiet, int *fault_p);
extern void make_edge_image(unsigned char *src, unsigned char *dst, int width, int height, int edge);
extern void interpolate_image(unsigned char *in, unsigned char *out, int width, int height);
extern void H263ConcealGOB(H263Global *h263Data);
/* gethdr.c */
extern int _getheader(H263Global *h263Data);
extern int GetPLUSHEADER(Bitstream *bs, H263Global *h263Data);
extern int GetPLUSPTYPE(Bitstream *bs, H263Global *h263Data);
/* getvlc.c */
extern VLCtab DCT3Dtab0[];
extern VLCtab DCT3Dtab1[];
extern VLCtab DCT3Dtab2[];
extern int _getTMNMV(H263Global *h263Data);
extern int _getMCBPC(H263Global *h263Data);
extern int _getMODB(H263Global *h263Data);
extern int _getMCBPCintra(H263Global *h263Data);
extern int getMCBPCintra_ext(Bitstream *bs, int quiet, int *fault_p);
extern int _getCBPY(H263Global *h263Data);
extern int getCBPY_ext(Bitstream *bs, int quiet, int *fault_p);
/* h263dec.c */
extern H263Global *NewH263Decoder(void);
extern H263Global *NewH263DecoderNoBuffer(void);
extern void FreeH263Decoder(H263Global *h263Data);
extern int H263DecodeGOB(H263Global *h263Data, int dec_until_sync);
extern void newpictureinit(H263Global *h263Data);
extern void InitH263Decoder(H263Global *h263Data);
extern void DisinitH263Decoder(H263Global *h263Data);
extern void H263FinishPicture(H263Global *h263Data);
/* idct.c */
extern void _idct(short *block);
/* input.c */
extern int ParseH263JunkForIntra(H263Global *h263Data, Byte **next_p, Byte *end, int lastjunk);
extern int HandleH263DataJunk(H263Global *h263Data, Byte **next_p, Byte *end, int lastjunk);
extern int HandleH263CompleteGOBs(H263Global *h263Data, Byte *next, Byte *end);
/* reconh263.c */
extern void reconstructH263(H263Global *h263Data, int bx, int by, int P, int bdx, int bdy);
/* sac.c */
extern int _decode_a_symbol(Bitstream *bs, int cumul_freq[ ]);
extern void _decoder_reset(Bitstream *bs);
