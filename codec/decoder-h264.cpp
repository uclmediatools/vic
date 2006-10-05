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
    int b_off;			/* how much bitstream data we have */
    int b_all;			/* got all packets for so far? */
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

    b_all = 1;
    b_off = 0;
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
/*    
    // simulate the pack loss
    if(rand() % 10 == 0){
      printf("drop the packet\n");
      pb->release();
      return;      
    }
    if(rand() % 50 == 0){
      printf("packet error\n");	
      int l = rand() % pb->len;
      for(int i=0; i < l; i++)
        pb->dp[rand()% pb->len] = u_char(rand() % 256);
    }    
*/
    rtphdr *rh = (rtphdr *) pb->dp;
    int hdrsize = sizeof(rtphdr) + hdrlen() + 2;
    u_char *bp = pb->dp + hdrsize;
    int cc = pb->len - hdrsize;
    static int iframe_c = 0, pframe_c = 0;

    //(sizeof(*rh) + 4);
    /* RTP header  */
    /* Basic RTP header 
       struct rtphdr {
       u_int16_t rh_flags;      // T:2 P:1 X:1 CC:4 M:1 PT:7 
       u_int16_t rh_seqno;      // sequence number 
       u_int32_t rh_ts; // media-specific time stamp 
       u_int32_t rh_ssrc;       // synchronization src id 
       // data sources follow per cc 
       }; */

    int mbit = ntohs(rh->rh_flags) >> 7 & 1;
    int seq = ntohs(rh->rh_seqno);
    int ts = ntohl(rh->rh_ts);

    /* H.263 payload */
    u_char *pl = (u_char *) (rh + 1);
    /* 5 bits reserved */
    int pbit = (pl[0] >> 2) & 1;	/* 1 bit */
    int ibit = (pl[0] >> 1) & 1;	/* 1 bit */

    int ms = 0x0000FFFF & ntohl(*((u_int *) pl));
    //std::cout << "dec: "<< ms << "\n";

    if (pbit) {
	/* start of a new frame */
	b_all = 1;
	b_off = 0;
	stream->clear();
	idx = seq;
	startPkt = true;
    }


    if (startPkt) {
	int pktIdx = seq - idx;
	if (pktIdx < 0) {
	    pktIdx = (0xFFFF - idx) + seq;
	}
	//copy packet
	stream->write(pktIdx, cc, (char *) bp);
	if (last_seq + 1 != seq) {
	    /* oops - missing packet */
	    debug_msg("h264 dec: missing packet\n");
	}
	last_seq = seq;

	int len;
	if (mbit) {
	    stream->setTotalPkts(pktIdx + 1);
	    /* yeah! have a complete frame */
	    if (ibit) {
		last_iframe = 1;
	    }
	    else if (!last_iframe) {
		debug_msg("h264dec: no I-frame yet\n");
		pb->release();
		return;
	    }

	    assert(b_off < MAX_CODED_SIZE);
	    //printf("receive %d\n", b_off);
	    //fwrite(bitstream, b_off, 1, fptr);    //File decoding problem occurs

	    if (stream->isComplete()) {
		DataBuffer *f = stream->getStream();
		len =
		    h264.decode((UCHAR *) f->getData(), f->getDataSize(),
				xxx_frame);
	    }
	    else {
		cout << "packet loss...\n";
		len = 0;
	    }

	    if (len < 0) {
		cout << "decoder : frame error\n";
		pb->release();
		debug_msg("h264dec: frame error\n");
		return;
	    }

	    if (inw_ != h264.width || inh_ != h264.height) {
		inw_ = h264.width;
		inh_ = h264.height;
		resize(inw_, inh_);
	    }
	    else {
		Decoder::redraw(xxx_frame);
	    }

	    //NEXT Should Be P bit
	    b_all = 1;
	    stream->clear();
	    //b_off = 0;
	}
    }
    pb->release();
}

void H264Decoder::redraw()
{
    Decoder::redraw(xxx_frame);
}
