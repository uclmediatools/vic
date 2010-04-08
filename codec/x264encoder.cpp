#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "x264encoder.h"
#include "libavcodec/avcodec.h"
#include "databuffer.h"
extern "C"{
#include "inttypes.h"
#include "x264.h"
}

typedef struct
{
    x264_t *h;
    x264_param_t param;
    x264_picture_t pic;
    x264_picture_t pic_out;

    x264_nal_t *nal;
    int i_nal;
} x264;

x264Encoder::x264Encoder()
{
    x264 *enc;
    enc = (x264 *) malloc(sizeof(x264));
    enc->h = NULL;
    encoder = (void *) enc;
    isFrameEncoded = false;
}

x264Encoder::~x264Encoder()
{
    x264 *enc = (x264 *) encoder;
    if (enc->h != NULL) {
	  x264_encoder_close(enc->h);
//	  x264_picture_clean(&(enc->pic));
    }
    free(enc);
}

bool x264Encoder::init(int w, int h, int bps, int fps)
{
    x264 *enc = (x264 *) encoder;
    x264_param_t *param = &(enc->param);
    x264_param_default(param);

    // * seting rate control
    param->rc.i_bitrate = bps;
    param->rc.i_rc_method = X264_RC_ABR;
    
    //param->analyse.inter = X264_ANALYSE_PSUB16x16;
    //DISALBE PARTITION MODE
    param->analyse.inter = 0;
    param->analyse.intra = 0; // try this for intra too
    //param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_NONE;

    //DISABLE CABAC for more frame rate
    param->b_cabac = 0;
    
    //DONOT ENABLE PSNR ANALYSE
    param->analyse.b_psnr = 0;
    param->analyse.b_ssim = 0;
    
    param->i_keyint_max = 50;
    param->i_keyint_min = 20;
    param->i_bframe = 0;
    
    // deblocking filter
    // i_deblocking_filter_alphac0, [-6, 6] -6 light filter, 6 strong    
    param->b_deblocking_filter = 1;
    param->i_deblocking_filter_alphac0 = 3;
    
    param->i_fps_num = fps * 1000;
    param->i_fps_den = 1000;

    // set frame reference to 1 to reduce encoding latency
    param->i_frame_reference = 1;
    param->i_scenecut_threshold = 0;
    
    // motion estimation method, using umh if higher quality is essential.
    //param->analyse.i_me_method = X264_ME_HEX;
    param->analyse.i_me_method = X264_ME_DIA; //supposedly DIA is faster

    //other optimization stuff
    //these seem to add a couple fps in non-threaded mode
    param->analyse.b_transform_8x8 = 0;
    param->analyse.i_subpel_refine = 0;
    param->rc.i_aq_mode = 0;
    param->analyse.b_mixed_references = 0;
    param->analyse.i_trellis = 0;

    param->i_width = w;
    param->i_height = h;

    // attempt to make threads
    param->i_threads = 0;
    param->b_deterministic = 1;
    // sliced threads might be nice for threading on the decoding side but just
    // seems to be broken
    //param->b_sliced_threads = 1;

    // make it completely 1-pass only
    param->rc.b_stat_read = 0;
    param->rc.b_stat_write = 0;

    // removing lookahead might reduce latency?
    #if X264_BUILD > 69
    param->rc.i_lookahead = 0;
    #endif
    #if X264_BUILD > 74
    param->i_sync_lookahead = 0;
    #endif

    x264_picture_alloc(&(enc->pic), X264_CSP_I420, param->i_width,
                       param->i_height);

    x264_t *handle = x264_encoder_open(param);

    if (handle != NULL) {
        enc->h = handle;
        return true;
    }
    else {
        return false;
    }
}

bool x264Encoder::encodeFrame(uint8 *buf)
{
    x264 *enc = (x264 *) encoder;
    x264_param_t *param = &(enc->param);


    int frame_size = param->i_width * param->i_height;

    //refresh 
    enc->i_nal = 0;
    enc->pic.img.plane[0] = buf;
    enc->pic.img.plane[1] = buf + frame_size;
    enc->pic.img.plane[2] = buf + frame_size*5/4;

    int result = x264_encoder_encode(enc->h, &(enc->nal), &(enc->i_nal), &(enc->pic), &(enc->pic_out));

    if (result < 0) {
	  isFrameEncoded = false;
	  return false;
    }
    else {
	  isFrameEncoded = true;
	  return true;
    }
}

int x264Encoder::numNAL()
{
    if (isFrameEncoded) {
	x264 *enc = (x264 *) encoder;
	return enc->i_nal;
    }
    else
	return 0;
}

bool x264Encoder::getNALPacket(int idx, DataBuffer * f)
{
    x264 *enc = (x264 *) encoder;
    if (!isFrameEncoded || idx >= enc->i_nal)
	return false;

    char *pkt = f->getData();
    int data = f->getCapacity();
    int packetSize;

    #if X264_BUILD < 76
    packetSize = x264_nal_encode(pkt, &data, 1, &(enc->nal[idx]));
    f->setSize( packetSize );
    #else
    // to conform to new x264 API: data within the payload is already
    // nal-encoded, so we should just be able to grab
    packetSize = enc->nal[idx].i_payload;
    f->write( (char*)enc->nal[idx].p_payload, packetSize );
    #endif

    //printf("i_nal=%d, idx=%d, size=%d\n", enc->i_nal, idx, packetSize);
    
    return isFrameEncoded;
}

void x264Encoder::setGOP(int gop)
{
    x264 *enc = (x264 *) encoder;
    x264_param_t *param = &(enc->param);
    param->i_keyint_max = 2*gop;
    param->i_keyint_min = gop;
}

void x264Encoder::setBitRate(int br)
{
    x264 *enc = (x264 *) encoder;
    x264_param_t *param = &(enc->param);
    param->rc.i_bitrate = br;
}

void x264Encoder::setFPS(int fps)
{
    x264 *enc = (x264 *) encoder;
    x264_param_t *param = &(enc->param);
    param->i_fps_num = fps * 1000;
    param->i_fps_den = 1000;
}

bool x264Encoder::isInitialized()
{
    x264 *enc = (x264 *) encoder;
    if (enc->h == NULL)
	return false;
    else
	return true;
}
