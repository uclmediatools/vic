/* bitOut.c */
extern Bitstr *AllocBitstr(unsigned int n); //SV-XXX: changed arg to uint
extern void FreeBitstr(Bitstr **b_p);
extern int CheckOutBuffer(Bitstr *b);
extern Bitstr *FopenBitstr(int n, char *name, char *mode);
extern void FcloseBitstr(Bitstr **b_p);
extern int BitsToByteAlignment(Bitstr *bs);
extern int ByteAlign0X(Bitstr *b);
extern void PutBitX(int bit, Bitstr *b);
extern void PutCodewordX(char *cw, Bitstr *b);
extern void PutNumberX(int num, int n, Bitstr *b);
