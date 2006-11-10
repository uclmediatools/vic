#include "x264encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "ffmpeg/avcodec.h"
extern "C"
{
// #include "x264/common/common.h"
#include "x264.h"
}

#include "databuffer.h"

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

    x264_param_t *param = &(enc->param);

    x264_param_default(param);
    //param->analyse.inter = X264_ANALYSE_PSUB16x16;
    //DISALBE PARTITION MODE
    param->analyse.inter = 0;
    //param->analyse.i_direct_mv_pred = X264_DIRECT_PRED_NONE;
    //DISABLE CABAC for more frame rate
    param->b_cabac = 0;
    //DONOT ENABLE PSNR ANALYSE
    // param->analyse.b_psnr = 0;
    param->i_keyint_max = 60;
    param->i_keyint_min = 20;
 
    enc->h = NULL;

    encoder = (void *) enc;

    isFrameEncoded = false;
}

x264Encoder::~x264Encoder()
{
    x264 *enc = (x264 *) encoder;
    if (enc->h != NULL) {
	x264_encoder_close(enc->h);
	x264_picture_clean(&(enc->pic));
    }
    free(enc);
}

bool x264Encoder::init(int w, int h, int bps, int fps)
{
    x264 *enc = (x264 *) encoder;
    x264_param_t *param = &(enc->param);

    param->rc.i_bitrate = bps;
    // param->rc.i_rc_method = X264_RC_ABR;
    param->rc.i_rc_method = X264_RC_CRF;
    // param->b_cabac = FF_CODER_TYPE_AC;
    // param->b_deblocking_filter = 1;
    param->i_fps_num = fps * 1000;
    param->i_fps_den = 1000;
    param->rc.f_qcompress = 0;  /* 0.0 => cbr, 1.0 => constant qp */
    param->analyse.i_me_method = X264_ME_UMH;

    //Currently X264 only handle (16*n)x(16*m)
    if (w % 16 != 0 && h % 16 != 0) {
	return 0;
    }

    param->i_width = w;
    param->i_height = h;

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

bool x264Encoder::encodeFrame(DataBuffer * in)
{
    x264 *enc = (x264 *) encoder;
    x264_param_t *param = &(enc->param);
    x264_picture_t *pic = &(enc->pic);

    char *f = in->getData();
    int frame_size = param->i_width * param->i_height;

    //refresh 
    enc->i_nal = 0;

    memcpy(pic->img.plane[0], f, frame_size);
    memcpy(pic->img.plane[1], (f + frame_size), frame_size / 4);
    memcpy(pic->img.plane[2], (f + frame_size * 5 / 4), frame_size / 4);

    int result = x264_encoder_encode(enc->h, &(enc->nal), &(enc->i_nal), pic,
				     &(enc->pic_out));

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

    packetSize = x264_nal_encode(pkt, &data, 1, &(enc->nal[idx]));

    f->setSize(packetSize);

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
