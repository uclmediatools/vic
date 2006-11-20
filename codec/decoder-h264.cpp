#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "renderer.h"
#include "packetbuffer.h"
#include "databuffer.h"
#include "ffmpeg_codec.h"

//#define DIRECT_DISPLAY 1

using namespace std;

extern "C" UCHAR * video_frame;

class H264Decoder:public Decoder
{
  public:
    H264Decoder();
    ~H264Decoder();

    virtual void recv(pktbuf *);
    int colorhist(u_int * hist) const;
  protected:
    void decode(const u_char * vh, const u_char * bp, int cc);
    virtual void redraw();

    /* packet statistics */
    u_int16_t last_seq;		/* sequence number */

    UCHAR bitstream[MAX_CODED_SIZE];	/* bitstream data */

    /* collecting data for a frame */
    int idx;
    int last_mbit;
    int last_iframe;
    bool startPkt;

    /* image */
    UCHAR xxx_frame[MAX_FRAME_SIZE];
    FFMpegCodec h264;
    PacketBuffer *stream;


    //For DEBUG
    FILE *fptr;
};

static class H264DecoderMatcher:public Matcher
{
  public:
    H264DecoderMatcher():Matcher("decoder")
    {
    }
    TclObject *match(const char *id)
    {
	if (strcasecmp(id, "h264") == 0)
	    return (new H264Decoder());
	return (0);
    }
}

dm_h264;


H264Decoder::H264Decoder():Decoder(2)
{				/* , codec_(0), */

    decimation_ = 411;
    /*
     * Assume CIF.  Picture header will trigger a resize if
     * we encounter QCIF instead.
     */
    inw_ = 0;
    inh_ = 0;

     /*XXX*/ resize(inw_, inh_);

    // libavcodec
    h264.init(false, CODEC_ID_H264, PIX_FMT_YUV420P);
    h264.init_decoder();

    idx = 0;
    last_mbit = 0;
    last_iframe = 0;
    last_seq = 0;

    //256 packets, each 1024 byte (default will not exceed 1000 byte)
    //cout << "new PacketBuffer..\n";
    stream = new PacketBuffer(1024, 1024);
    startPkt = false;

    //fptr = fopen("out.m4v", "w");
}

H264Decoder::~H264Decoder()
{
    delete stream;
    //fclose(fptr);
}

int H264Decoder::colorhist(u_int * hist)  const
{
    return (1);
}

void H264Decoder::recv(pktbuf * pb)
{
    rtphdr *rh = (rtphdr *) pb->dp;
    int hdrsize = sizeof(rtphdr) + hdrlen();
    u_char *bp = pb->dp + hdrsize;
    int cc = pb->len - hdrsize;
    static int iframe_c = 0, pframe_c = 0;

    int mbit = ntohs(rh->rh_flags) >> 7 & 1;
    int seq = ntohs(rh->rh_seqno);
    int ts = ntohl(rh->rh_ts);

    if (!startPkt) {
       stream->clear();
       startPkt = true;
       idx = seq;
    }
	  
    int pktIdx = seq - idx;
    if (pktIdx < 0) {
        pktIdx = (0xFFFF - idx) + seq;
    }

    if (pktIdx - last_seq > 5) {
       debug_msg("mp4dec: sequece interrupt!\n");
       idx = seq;
       pktIdx = 0;
       stream->clear();
    }
    
    //copy packet
    stream->write(pktIdx, cc, (char *) bp);
    // printf("pktIdx=%d, cc=%d, seq=%d\n", pktIdx, cc, seq);
    
    if (last_seq + 1 != seq) {
       /* oops - missing packet */
       debug_msg("h264dec: missing packet\n");
    }
	
    last_seq = seq;
    int len=0;
	
    if (mbit) {
	    stream->setTotalPkts(pktIdx + 1);

	    DataBuffer *f;	    
	    if (stream->isComplete()) {
		f = stream->getStream();
		len =  h264.decode((UCHAR *) f->getData(), f->getDataSize(),
				xxx_frame);
	    }
	    
            if(len == -2){
               debug_msg("h264dec: resize\n");
    	       h264.release();
               h264.init_decoder();
	       len = h264.decode((UCHAR *) f->getData(), f->getDataSize(), xxx_frame);
	    }
	   if (len <= 0) {
	       debug_msg("h264dec: frame error\n");
	    }
	   
	    if (inw_ != h264.width || inh_ != h264.height) {
		inw_ = h264.width;
		inh_ = h264.height;
		resize(inw_, inh_);
	    }
	    else {
		Decoder::redraw(xxx_frame);
	    }
            stream->clear();
	    idx = seq+1;
		     
    }
    pb->release();
}

void H264Decoder::redraw()
{
    Decoder::redraw(xxx_frame);
}
