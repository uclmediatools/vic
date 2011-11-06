#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <math.h>
#include "debug.h"
#include "ffmpeg_codec.h"
/*#include "dsputil.h"
#include "avcodec.h"
#include "mpegvideo.h"
#include "h264.h"*/

#ifdef WIN32
#include <process.h>
#endif

FFMpegCodec::FFMpegCodec()
{
    state = false;
    quality = 31;
    //rtp_callback = NULL;
    enable_hq_encoding = false;
}

FFMpegCodec::~FFMpegCodec()
{
    release();
}


void FFMpegCodec::init(bool encode, CodecID id, PixelFormat fmt)
{
    encoding = encode;
    codecid = id;
    pixelfmt = fmt;
    picture_buf = NULL;
    state = false;

    avcodec_init();
    avcodec_register_all();
}

void FFMpegCodec::restart()
{
    avcodec_init();

    if (encoding) {
	init_encoder(width, height, bit_rate, frame_rate, iframe_gap);
    }
    else {
	init_decoder();
    }
}

void FFMpegCodec::init_encoder(int width_, int height_,
			       int bit_rate_, int frame_rate_,
			       int iframe_gap_)
{
    if (state) {
	release();
    }
    width = width_;
    height = height_;
    bit_rate = bit_rate_;
    frame_rate = frame_rate_;
    iframe_gap = iframe_gap_;

    c = avcodec_alloc_context();
    c->pix_fmt = pixelfmt;
    picture = avcodec_alloc_frame();

    codec = avcodec_find_encoder(codecid);
    if (!codec) {
	//printf("codec %d not found\n", codecid);
	std::cout << "codec not found\n";
	exit(1);
    }

    // assign rtp callback function
    c->rtp_callback = rtp_callback;
    //c->rtp_callback = NULL;
    // put sample parameters */
    c->bit_rate = bit_rate;
    //c->bit_rate_tolerance = 2000*1000;
    // resolution must be a multiple of two
    c->width = width;
    c->height = height;

    // frames per second */
    // OLD FFMPEG ver
    //c->frame_rate = frame_rate * FRAME_RATE_BASE;
    // New FFMPEG ver
    c->time_base.den = frame_rate;
    c->time_base.num = 1;

    // emit one intra frame every ten frames
    c->gop_size = iframe_gap;
    //c->flags |= CODEC_FLAG_EMU_EDGE;
    //c->flags |= CODEC_FLAG_LOW_DELAY;
    //c->flags |= CODEC_FLAG_PART;
    //c->flags |= CODEC_FLAG_ALT_SCAN;
    //c->flags |= CODEC_FLAG_PSNR;
    //c->flags |= CODEC_FLAG_AC_PRED;

    // in loop filter for low bitrate compression
    c->flags |= CODEC_FLAG_LOOP_FILTER;

    if (enable_hq_encoding) {
	c->flags |= CODEC_FLAG_QPEL;
	//c->flags |= CODEC_FLAG_GMC;
	c->flags |= CODEC_FLAG_AC_PRED;
	c->flags |= CODEC_FLAG_4MV;
	//c->flags |= CODEC_FLAG_QP_RD;
	//c->mb_decision = FF_MB_DECISION_RD;
    }

    c->me_method = ME_EPZS;
    c->rtp_payload_size = 1024;

    /* open it */
    if (avcodec_open(c, codec) < 0) {
	//fprintf(stderr, "could not open codec\n");
	std::cout << "could not open codec\n";
	exit(1);
    }

    /* size for YUV 420 */
    frame_size = width * height;
    picture_buf = new UCHAR[(frame_size * 3) >> 1];
    picture->data[0] = picture_buf;
    picture->data[1] = picture->data[0] + frame_size;
    picture->data[2] = picture->data[1] + (frame_size >> 2);
    picture->linesize[0] = width;
    picture->linesize[1] = width >> 1;
    picture->linesize[2] = width >> 1;

    bitstream = new UCHAR[MAX_CODED_SIZE];
    state = true;
    keyFrame = false;
}


void FFMpegCodec::init_decoder()
{
    if (state) {
	release();
    }
    width = height = 0;
	frame_size = 0;

    picture = avcodec_alloc_frame();
    c = avcodec_alloc_context();
    c->flags |= CODEC_FLAG_EMU_EDGE | CODEC_FLAG_PART;
    // c->flags |= CODEC_FLAG_ALT_SCAN;

    codec = avcodec_find_decoder(codecid);
    if (!codec) {
	//fprintf(stderr, "codec not found\n");
	exit(1);
    }

    /* open it */
    if (avcodec_open(c, codec) < 0) {
	//fprintf(stderr, "could not open codec\n");
	exit(1);
    }

    state = true;

    //fptr = (void*)fopen("ffmpeg.yuv", "w");
}

void FFMpegCodec::release()
{
    if (state) {
	avcodec_close(c);
	av_free(c);
	av_free(picture);
	c = NULL;
	picture = NULL;
	codec = NULL;

	if (encoding) {
	    delete picture_buf;
	    delete bitstream;
	    picture_buf = bitstream = NULL;
	}
	state = false;
    }
}

// return: the coding length
UCHAR *FFMpegCodec::encode(const UCHAR * vf, int &len)
{
    memcpy(picture->data[0], vf, frame_size);
    memcpy(picture->data[1], (vf + frame_size), frame_size / 4);
    memcpy(picture->data[2], (vf + frame_size * 5 / 4), frame_size / 4);

    len = avcodec_encode_video(c, bitstream, MAX_CODED_SIZE, picture);
    if (c->coded_frame && c->coded_frame->key_frame) {
	keyFrame = true;
    }
    else {
	keyFrame = false;
    }
    assert(len < MAX_CODED_SIZE);
    pict_type = c->coded_frame->pict_type;

    return bitstream;
}

bool FFMpegCodec::isKeyFrame()
{
    return keyFrame;
}

double FFMpegCodec::get_PSNR()
{
#ifndef WIN32
    double mse1 = c->error[0] / float (frame_size);
    c->error[0] = 0;

    return 10 * log10(47961 / mse1);
#else
    c->error[0] = 0;
    return 0.0;
#endif
}

// return:  videoframe size
//         -1 indicates decoding failure
int FFMpegCodec::decode(UCHAR * codedstream, int size, UCHAR * vf)
{
    int got_picture;
    int len;

    len = avcodec_decode_video(c, picture, &got_picture, codedstream, size);

    if (!got_picture || len < 0) {
		return -1;
    }

    if (state) {
      //H264Context *h = c->priv_data;
      //printf("h->sps.ref_frame_count: %d",h->sps.ref_frame_count);
    }
	if (c->width != width || c->height != height) {
		debug_msg("ffmpegcodec: resize from %dx%d (framesize:%d) to %dx%d, (size: %d) got_picture: %d, len: %d\n", width, height,
			frame_size, c->width, c->height, size, got_picture, len);
		resize(c->width, c->height);
    }
/*
    if(avpicture_deinterlace((AVPicture *)picture, (AVPicture *)picture,
                                    c->pix_fmt, c->width, c->height) < 0) {
          printf("deinterlace error\n");
    }
*/
    pict_type = picture->pict_type;
    memcpy(vf, picture->data[0], frame_size);
    memcpy(vf + frame_size, picture->data[1], frame_size / 4);
    memcpy(vf + frame_size * 5 / 4, picture->data[2], frame_size / 4);

    //fwrite(picture->data[0], frame_size, 1, (FILE*)fptr);
    //fwrite(picture->data[1], frame_size/4, 1, (FILE*)fptr);
    //fwrite(picture->data[2], frame_size/4, 1, (FILE*)fptr);

    return len;
}

void FFMpegCodec::resize(int w, int h)
{
    width = w;
    height = h;
    frame_size = width * height;
    assert(frame_size * 3 / 2 <= MAX_FRAME_SIZE);
}

void FFMpegCodec::set_gop(int gop_)
{
/*
    iframe_gap = gop_;

    if (state) {
	MpegEncContext *s = (MpegEncContext *) c->priv_data;
	s->gop_size = gop_;
	if (s->gop_size <= 1) {
	    s->intra_only = 1;
	    s->gop_size = 12;
	}
	else {
	    s->intra_only = 0;
	}
    }
*/
}

void FFMpegCodec::set_max_quantizer(int q)
{
    quality = q;
    if (state) {
//    MpegEncContext *s = (MpegEncContext*)c->priv_data;
//    s->qmax = c->qmax = c->mb_qmax = q;
    }
}
