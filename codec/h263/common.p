/* picture.c */
extern Picture *AllocPicture(int w, int h);
extern Picture *ClonePicture(Picture *pict);
extern void FreePicture(Picture *pict);
extern void ResetPicture(Picture *p);
extern Picture *AllocResetPicture(int w, int h);
extern Picture *CloneResetPicture(Picture *pict);
extern void SwapPictures(Picture **pict1_p, Picture **pict2_p);
extern void LinkPicture(Picture *pictIn, Picture *pictOut);
extern void CopyPicture(Picture *pictIn, Picture *pictOut);
extern int ReadPicture(char *name, int fileType, int index, Picture *pict);
extern int WritePicture(char *name, int fileType, int index, Picture *pict);
extern float PSNRPictures(Picture *pict1, Picture *pict2);
extern float PSNRSubPictures(Byte *p1, Byte *p2, int w, int h, int iw);
extern int FindFirstDifferenceBetweenPictures(Picture *pict1, Picture *pict2);
extern int FindFirstDifferenceBetweenSubPictures(Picture *pict1, Picture *pict2);
/* recon.c */
extern void recon_comp(unsigned char *src, unsigned char *dst, int lx, int lx2, int w, int h, int x, int y, int dx, int dy, int chroma);
extern void recon_comp_obmc(unsigned char *src, unsigned char *dst, int modemap[72+1][88+2], int MV[2][5][72+1][88+2], int pb_frame, int lx, int lx2, int comp, int w, int h, int x, int y);
/* block.c */
extern int subsampleSAD;
extern int MacroBlockPictureIntraSAD(int xPos, int yPos, Picture *pict, int max);
extern int MacroBlockPictureIntraVar(int xPos, int yPos, Picture *pict, int max);
extern int MacroBlockPictureSAD(int xPos, int yPos, Picture *pict1, Picture *pict2, int max);
extern int FPMotionMacroBlockPictureSAD(int xPos, int yPos, Picture *pict, Picture *refPict, MVector *mv, int max);
extern int HPMotionMacroBlockPictureSAD(int xPos, int yPos, Picture *pict, Byte *interpol[3], MVector *mv, int max);
extern int MacroBlockPictureSSE(int xPos, int yPos, Picture *pict1, Picture *pict2, int max);
extern void ResetMacroBlockPicture(int xPos, int yPos, Picture *pict);
extern void CopyMacroBlockPicture(int xPos, int yPos, Picture *pictIn, Picture *pictOut);
extern void MCMacroBlockPicture(int xPos, int yPos, Picture *pictIn, Byte *interpol[3], MVector *mv, Picture *pictOut);
extern void ExtractMacroBlock(int xPos, int yPos, Picture *pict, Byte *block);
extern void PlaceMacroBlock(int xPos, int yPos, Byte *block, Picture *pict);
extern void ExtractBlockInt(int xPos, int yPos, Byte *in, int w, int *block);
extern void ExtractDiffBlockInt(int xPos, int yPos, Byte *inP, Byte *inN, int w, int *block);
extern void PlaceBlockShortPicture(int xPos, int yPos, Byte *picComp, int w, short *block);
extern void AddBlockShortPicture(int xPos, int yPos, Byte *picComp, int w, short *block);
/* code.c */
extern Code *AllocCode(int size, int eventDim, char *title);
extern Code *InitCode(char *title, int size, int eventDim, int *event, char *cw[]);
extern int GenerateCodeTree(Code *code);
extern BinTreeNode *AllocCodeTree(void);
extern int StringToCodeword(char *str, Codeword *cw);
extern int PutIndexToBitstr(Code *code, int index, Bitstr *b);
extern int GetIndexFromEvent(Code *code, int *event, int *index);
extern int AddCodewordToCodeTree(Codeword *cw, int index, int level, BinTreeNode *btn);
extern BinTreeNode *AllocBinTreeNode(void);
/* motion.c */
extern MVector *AllocMVector(void);
extern void FreeMVector(MVector **mv_p);
extern void ResetMVector(MVector *mv);
extern void ClipMVector(int minX, int maxX, int minY, int maxY, MVector *mv);
extern int CopyMVector(MVector *mvIn, MVector *mvOut);
extern int ZeroMVector2D(MVector *mv);
extern int H263MVPrediction(MVField *mvf, int x, int y, int newgob, MVector *pv);
extern int H263subtractMVPrediction(MVector *mv, MVector *pv, int umv, MVector *dv);
extern int GetMVDRate(MVector *pv, MVector *mv);
extern int FullSearchFullPelME(int xPos, int yPos, int *minCost_p, int lambda, int searchRange, int rMin, int rMax, Picture *pict, Picture *predPict, MVector *pv, MVector *mvOut);
extern int FastFullPelMotionEstimationMB(int xPos, int yPos, Picture *pict, Picture *refPict, int cost0, int lambda, MVector *pv, MVector *mv);
extern void InitHalfPelInterpolation(Picture *pict, Byte *interpol[3]);
extern int HalfPelMotionEstimationMB(int xPos, int yPos, Picture *pict, Byte *interpol[3], int left_f, int right_f, int top_f, int bottom_f, int costMin, int lambda, MVector *pv, MVector *mv);
extern void PlotMVField(MVField *mvf, int value, float scale, Picture *pic);
extern void PrintMVField(MVField *mvf, FILE *fp);
/* pgmio.c */
extern char *ReadPGMFile(char *name, int *width, int *height);
extern int WritePGMFile(char *name, int width, int height, char *data);
