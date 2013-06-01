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
    // the speed preset here should probably be an option
    x264_param_default_preset(param, "ultrafast", "zerolatency");

    // necessary stuff
    // * seting rate control
    param->rc.i_bitrate = bps;
    param->rc.i_rc_method = X264_RC_ABR;
    param->rc.b_mb_tree = 0;

    param->i_width = w;
    param->i_height = h;

    param->i_keyint_max = 50;
    param->i_keyint_min = 20;

    param->i_fps_num = fps * 1000;
    param->i_fps_den = 1000;

    // if annexb is 1, x264 uses startcodes (0 -> plain size)
    // starting Feb 2010, x264 uses short startcodes in some cases which throws
    // off the packetizer since it assumes the NAL payload is at a constant byte
    // offset from the header
    param->b_annexb = 0;

    // single-frame vbv, helps with latency
    param->rc.i_vbv_max_bitrate = bps;
    param->rc.i_vbv_buffer_size = bps / fps;
    printf( "vbv buffer size set to %i\n", bps / fps );

    // intra refresh - maybe this should be an option?
    // should help with packet loss resiliency at low bitrates
    param->b_intra_refresh = 1;

    // may want to try slice_max_size=packet size (mtu=1450? need to check rtp
    // header length), but might be higher than the normal max amount of slices
    // supported by decoder (most builds of FFmpeg) (16) depending on bitrate

    // just to be safe, make sure there are no b-frames (just in case it may be
    // set by a slower speed preset)
    param->i_bframe = 0;

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

    //debug_msg("i_nal=%d, idx=%d, size=%d\n", enc->i_nal, idx, packetSize);
    //debug_msg("nal type is %i\n", enc->nal[idx].i_type);

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
