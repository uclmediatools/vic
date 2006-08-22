
#ifndef lib_p64as_h
#define lib_p64as_h

#include <sys/types.h>
#include <stdio.h>

struct huffcode;

#define MBPERGOB 33

class P64ASDecoder {
public:
  P64ASDecoder();
  virtual ~P64ASDecoder();

  virtual const u_char* frame() const { return (front_); }

  inline int ndblk() const { return (ndblk_); }
  inline void resetndblk() { ndblk_ = 0; }
  inline int width() const { return (width_); }
  inline int height() const { return (height_); }

  virtual void set_size(int w, int h);
   
  virtual int decode(const u_char* bp, 
		     int cc, 
		     int ebit,
		     int quant);

  virtual void sync();
  inline void bb(int& x, int& y, int& w, int& h) {
    x = bbx_; y = bby_; w = bbw_; h = bbh_;
  };
  inline u_int bad_psc() const { return (bad_psc_); }
  inline u_int bad_bits() const { return (bad_bits_); }
  inline u_int bad_GOBno() const { return (bad_GOBno_); }
  inline u_int bad_fmt() const { return (bad_fmt_); }
  inline int seenMaxGob() const { return (maxgob_ >= ngob_ - 1); }
  inline u_int gobquant() const { return (gobquant_); }
  
  inline void marks(u_char* p) { marks_ = p; }
  inline void mark(int v) { mark_ = v; }
  
  inline void setPreviewLevel( u_short level ) { set_preview_level = level; }
  inline u_short getPreviewLevel() const { return preview_level; }
  
  inline u_short getPreviewLevelMax() const { return 3; }
  void getPreviewLevelSize( u_short level, u_int &width, u_int &height );
  
  void mvblk(u_char* in, u_char* out, u_int stride);
  void mvblka(u_char*, u_char*, u_int stride);

protected:
  void init();
  void inithuff();
  void initquant();
  virtual void err(const char* msg ...) const;
  int quantize(int v, int q);
#ifdef INT_64
  int parse_block(short* blk, INT_64* mask);
#else
  int parse_block(short* blk, u_int* mask);
#endif
  void decode_block(u_int tc, u_int x, u_int y, u_int stride,
		    u_char* front, u_char* back, int sf);
  
  int parse_gob_hdr(int);
  int parse_mb_hdr(u_int& cbp);
  int decode_gob(u_int gob);
  int decode_mb();
  
  u_int size_;
  u_char* fs_;
  u_char* front_;
  u_char* back_;

  struct hufftab {
    int maxlen;
    const short* prefix;
  };
  hufftab ht_mba_;
  hufftab ht_mvd_;
  hufftab ht_cbp_;
  hufftab ht_tcoeff_;
  hufftab ht_mtype_;
  
  u_int bb_;		/* 32-bit bit buffer */
  int nbb_;		/* number bits in bit buffer */
  const u_short* bs_;	/* input bit stream (less bits in bb_) */
  const u_short* es_;	/* pointer to end if input stream */
  const u_short* ps_;	/* packet start (for error reporting) */
  int pebit_;		/* packet end bit (for error reporting) */
  
#define MBST_FRESH	0
#define MBST_OLD	1
#define MBST_NEW	2
  u_char* mbst_;
  short* qt_;
  
  u_int width_;		/* width of Y component */
  u_int height_;		/* height of Y component */
  
  u_int ngob_;		/* number of gobs (12 for CIF, 3 for QCIF) */
  u_int maxgob_;		/* max gob seen this frame */
  int ndblk_;		/* # of decoded macroblocks */
  u_int gobquant_;	/* last gob quantizer (for info funtion) */
  
  u_int mt_;		/* macroblock type (flags in p64-huff.h) */
  u_int gob_;		/* current gob index */
  int mba_;		/* macroblock address (predictor) */
  int mvdh_;		/* motion vector (predictor) */
  int mvdv_;		/* motion vector (predictor) */
  
  /* bounding box */
  u_int minx_;
  u_int miny_;
  u_int maxx_;
  u_int maxy_;
  u_int bbx_;
  u_int bby_;
  u_int bbw_;
  u_int bbh_;
  
  /*
   * Table to indicate which blocks have changed.
   */
  u_char* marks_;
  int mark_;
  
  /* error counters */
  int bad_psc_;
  int bad_bits_;
  int bad_GOBno_;
  int bad_fmt_;		/* # times RTP fmt != H.261 fmt */
  
  /* inverse quantization via table lookup */
  short quant_[32 * 256];
  
  /* gob/mba to coordinate mappings */
  u_int* coord_;
  u_char* mb_state_;
  
  
  u_short preview_level;
  u_short set_preview_level;
};

#endif
