#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "renderer.h"
#include "databuffer.h"
#include "packetbuffer.h"
#include "ffmpeg_codec.h"


//#define DIRECT_DISPLAY 1

extern "C" UCHAR * video_frame;

class MPEG4Decoder:public Decoder
{
  public:
    MPEG4Decoder();
    ~MPEG4Decoder();

    virtual void recv(pktbuf *);
    int colorhist(u_int * hist) const;
  protected:
    void decode(const u_char * vh, const u_char * bp, int cc);
    virtual void redraw();

    /* packet statistics */
    u_int16_t last_seq;		/* sequence number */

    bool startPkt, startFrame;
    PacketBuffer *stream;

    /* collecting data for a frame */
    int b_off;			/* how much bitstream data we have */
    int b_all;			/* got all packets for so far? */
    int last_mbit;
    int last_iframe;
    int idx;

    /* image */
    UCHAR xxx_frame[MAX_FRAME_SIZE];
    FFMpegCodec mpeg4;
};

static class MPEG4DecoderMatcher:public Matcher
{
  public:
    MPEG4DecoderMatcher():Matcher("decoder")
    {
    }
    TclObject *match(const char *id)
    {
	if (strcasecmp(id, "mpeg4") == 0)
	    return (new MPEG4Decoder());
	return (0);
    }
}

dm_mpeg4;


MPEG4Decoder::MPEG4Decoder():Decoder(2)
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
    mpeg4.init(false, CODEC_ID_MPEG4, PIX_FMT_YUV420P);
    mpeg4.init_decoder();
    startPkt = false;
    startFrame = false;
    // std::cout << "allocate packet buffer\n";
    stream = new PacketBuffer(1024, 1280);

    b_all = 1;
    b_off = 0;
    last_mbit = 0;
    last_iframe = 0;

}

MPEG4Decoder::~MPEG4Decoder()
{
    std::cout << "destroy mpeg4 decoder\n";
    delete stream;
}

int MPEG4Decoder::colorhist(u_int * hist) const
{
    return (1);
}

void MPEG4Decoder::recv(pktbuf * pb)
{
    rtphdr *rh = (rtphdr *) pb->dp;
    int hdrsize = sizeof(rtphdr);	// sizeof(rtphdr) is 12 bytes
    u_char *bp = pb->dp + hdrsize;
    int cc = pb->len - hdrsize;

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

    if (!startPkt) {
	startPkt = true;
	idx = seq;
    }

    int pktIdx = seq - idx;
    if (pktIdx < 0) {
	pktIdx = (0xFFFF - idx) + seq;
    }

    if (pktIdx - last_seq > 5) {
	//printf("sequece interrupt...\n");
	idx = seq;
	pktIdx = 0;
    }

    last_seq = pktIdx;
    //printf("%2d%2d%2d%2d\n", pb->dp[12], pb->dp[13], pb->dp[14], pb->dp[15]);

    //printf("%d %d %d\n", pktIdx, cc, idx);
    stream->write(pktIdx, cc, (char *) bp);
    /* copy packet */

    if (last_seq + 1 != seq) {
	/* oops - missing packet */
	debug_msg("mpeg4dec: missing packet\n");
    }

    last_seq = seq;
    int len = 0;
    if (mbit) {
	stream->setTotalPkts(pktIdx + 1);
	debug_msg("receive %d\n", b_off);


	if (stream->isComplete()) {
	    int ti;
	    DataBuffer *f = stream->getStream();
	    UCHAR *encData = (UCHAR *) f->getData();
	    /*
	       for(ti = 0; ti < 12; ti++)
	       printf("%2d ", encData[ti]);
	       printf("\n");
	     */
	    if (encData[0] == 0x00 && encData[1] == 0x00 && encData[2] == 0x01
		&& encData[3] == 0xb0) {
		startFrame = true;
	    }
	    if (!startFrame) {
		pb->release();
		stream->clear();
		idx = seq + 1;
		return;
	    }
	    len = mpeg4.decode(encData, f->getDataSize(), xxx_frame);
	}

	if (len < 0) {
	
	    pb->release();
	    debug_msg("mpeg4dec: frame error\n");
	    std::cout << "mpeg4dec: frame error\n";
	    stream->clear();
	    idx = seq + 1;
	    return;
	}			/*else if(len == -2){
				   debug_msg("mpeg4dec: resize\n");
				   mpeg4.release();
				   mpeg4.init_decoder();
				   len= mpeg4.decode(bitstream, b_off, xxx_frame);
				   } */

	if (inw_ != mpeg4.width || inh_ != mpeg4.height) {
	    inw_ = mpeg4.width;
	    inh_ = mpeg4.height;
	    resize(inw_, inh_);
	}
	else {
	    Decoder::redraw(xxx_frame);
	}
	b_off = 0;
	stream->clear();
	idx = seq + 1;
    }
    pb->release();
}

void MPEG4Decoder::redraw()
{
    Decoder::redraw(xxx_frame);
}
