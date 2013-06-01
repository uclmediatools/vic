#ifndef _FFMPEG_CODEC_H_
#define _FFMPEG_CODEC_H_

extern "C"
{
#include "libavcodec/avcodec.h"
//#include "ffmpeg/libavcodec/dsputil.h"
//#include "ffmpeg/libavcodec/mpegvideo.h"
}


// libavcode codec id
// Type: CodecID
// CODEC_ID_MPEG4;
// CODEC_ID_H263;

// Pixel Format
// Type: PixelFormat
// PIX_FMT_YUV420P,
// PIX_FMT_YUV422,
// PIX_FMT_YUV422P,
// PIX_FMT_YUV444P,
// PIX_FMT_YUV410P,
// PIX_FMT_YUV411P,

/* Motion Estimation Algorithms
enum Motion_Est_ID {
    ME_ZERO = 1,
    ME_FULL,
    ME_LOG,
    ME_PHODS,
    ME_EPZS,
    ME_X1
};
*/
/* AVFrame->pict_type
#define FF_I_TYPE 1 // Intra
#define FF_P_TYPE 2 // Predicted
#define FF_B_TYPE 3 // Bi-dir predicted
#define FF_S_TYPE 4 // S(GMC)-VOP MPEG4
*/

typedef unsigned char UCHAR;
const int MAX_FRAME_SIZE = 1920*1080*3/2;
const int MAX_CODED_SIZE = 400000;


class FFMpegCodec
{
  public:
    FFMpegCodec();
    ~FFMpegCodec();

    void init(bool encode, CodecID id, PixelFormat fmt);
    void init_encoder(int width_, int height_,
		      int bit_rate_, int frame_rate_, int iframe_gap_);
    void init_decoder();
    void release();
    void restart();

    // encode frame, return the code point
    UCHAR *encode(const UCHAR * vf, int &len);
    bool isKeyFrame();

    // return: the decoding length
    //     -1 indicates failure decoding
    //     -2  indicates resizing    
    int decode(UCHAR * codedstream, int size, UCHAR * vf);


    void set_gop(int gop);
    void set_max_quantizer(int q);
    double get_PSNR();

    //New interface
    void (*rtp_callback) (AVCodecContext * c, void *data, int size,
			  int packet_number);
    int width;
    int height;
    int bit_rate;
    int frame_rate;
    int iframe_gap;
    int quality;
    int pict_type;
    bool enable_hq_encoding;
    AVCodecContext *c;

  private:
    void resize(int w, int h);

    CodecID codecid;
    PixelFormat pixelfmt;
    bool encoding;
    bool keyFrame;

    AVCodec *codec;

    AVFrame *picture;

    unsigned char *picture_buf;	// image pointer  
    int frame_size;		// video frame size
    UCHAR *bitstream;		// encoder output 
    bool state;			// initialized state  

    void *fptr;
};

#endif /* _FFMPEG_CODEC_H_ */
