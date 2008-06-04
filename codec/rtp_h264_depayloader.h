/*
 * RTP H264 Protocol (RFC3984)
 * Copyright (c) 2006 Ryan Martell.
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef RTP_H264DEPAYLOADER_H
#define RTP_H264DEPAYLOADER_H

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/base64.h"
#include "packetbuffer.h"
//#include "rtp_internal.h"
//#include "mpegts.h"
//#include "bitstream.h"
}

int strstart(const char *str, const char *val, const char **ptr);
int rtsp_next_attr_and_value(const char **p, char *attr, int attr_size, char *value, int value_size);



#if !defined(BE_16) || !defined(BE_32) || !defined(LE_16) || !defined(LE_32)
#define BE_16(x)  ((((uint8_t*)(x))[0] << 8) | ((uint8_t*)(x))[1])
#define BE_32(x)  ((((uint8_t*)(x))[0] << 24) | \
                   (((uint8_t*)(x))[1] << 16) | \
                   (((uint8_t*)(x))[2] << 8) | \
                    ((uint8_t*)(x))[3])
#define LE_16(x)  ((((uint8_t*)(x))[1] << 8) | ((uint8_t*)(x))[0])
#define LE_32(x)  ((((uint8_t*)(x))[3] << 24) | \
                   (((uint8_t*)(x))[2] << 16) | \
                   (((uint8_t*)(x))[1] << 8) | \
                    ((uint8_t*)(x))[0])
#endif



#define MAGIC_COOKIE (0xdeadbeef)       ///< Cookie for the extradata; to verify we are what we think we are, and that we haven't been freed.
#define DEAD_COOKIE (0xdeaddead)        ///< Cookie for the extradata; once it is freed.


typedef struct h264_rtp_extra_data {
    unsigned long cookie;       ///< sanity check, to make sure we get the pointer we're expecting.

    //sdp setup parameters
    uint8_t profile_idc;        ///< from the sdp setup parameters.
    uint8_t profile_iop;        ///< from the sdp setup parameters.
    uint8_t level_idc;          ///< from the sdp setup parameters.
    int packetization_mode;     ///< from the sdp setup parameters.
#ifdef DEBUG
    int packet_types_received[32];
#endif
} h264_rtp_extra_data;


class H264Depayloader
{
  public:
    H264Depayloader();
    ~H264Depayloader();
    
    h264_rtp_extra_data *h264_extradata;

    int parse_h264_sdp_line(AVCodecContext *codec, /*AVStream * stream,*/ void *data, const char *line);
    int h264_handle_packet(h264_rtp_extra_data *data, /*AVPacket * pkt*/ int pktIdx, PacketBuffer *pb, /*uint32_t * timestamp,*/ const uint8_t * buf, int len, int mbit);

/**
    RTP/H264 specific private data.
*/
  private:
    int aggregate_pkt; // Count of mbits for decoding IOCOM H.264 

    void h264_free_extradata(void *d);
    void *h264_new_extradata();
    void sdp_parse_fmtp_config_h264(AVCodecContext *codec, /*AVStream * stream,*/
                                       h264_rtp_extra_data * h264_data,
                                       char *attr, char *value);
};


//extern RTPDynamicProtocolHandler ff_h264_dynamic_handler;
#endif /* RTP_H264DEPAYLOADER_H */
