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

    decimation_ = 420;
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
    stream = new PacketBuffer(1024, 1600);

    last_iframe = 0;
    last_seq = 0;
    debug_msg("mp4dec: initialized\n");
}

MPEG4Decoder::~MPEG4Decoder()
{
    debug_msg("mp4dec: released\n");
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

    int mbit = ntohs(rh->rh_flags) >> 7 & 1;
    int seq = ntohs(rh->rh_seqno);
    int ts = ntohl(rh->rh_ts);

	// debug_msg("seq=%d, idx=%d, size=%d\n", seq, idx, cc);

    if (!startPkt) {
		stream->clear();
	    startPkt = true;
	    idx = seq;
		last_seq = seq - 1;
    }

    int pktIdx = seq - idx;
    if (pktIdx < 0) {
	    pktIdx = (0xFFFF - idx) + seq;
    }
 
    if (abs(seq - last_seq) > 5) {
	    debug_msg("mp4dec: sequece interrupt!\n");
	    idx = seq;
	    pktIdx = 0;
	    stream->clear();
    }else if (last_seq + 1 != seq) {
	    /* oops - missing packet */
	    debug_msg("mp4dec: missing packet\n");
    }
    
    stream->write(pktIdx, cc, (char *) bp);

    last_seq = seq;
    int len = 0;

    if (mbit) {
	    DataBuffer *f;
	    UCHAR *encData;

	    stream->setTotalPkts(pktIdx + 1);

	    if (stream->isComplete()) {
	        f = stream->getStream();
	        encData = (UCHAR *) f->getData();

	        if (encData[0] == 0x00 && encData[1] == 0x00 && encData[2] == 0x01
		       && encData[3] == 0xb0) {
		       startFrame = true;
	        }

	        if (!startFrame) {
				debug_msg("mp4dec: waiting for the first key frame...\n");
			    pb->release();
			    stream->clear();
			    idx = seq + 1;
			    return;
	        }

	        len = mpeg4.decode(encData, f->getDataSize(), xxx_frame);
	    }

		if (len < 0) {
			pb->release();
			debug_msg("mp4dec: frame error\n");
			stream->clear();
			idx = seq + 1;
			return;
		}
	    
		if (inw_ != mpeg4.width || inh_ != mpeg4.height) {
			inw_ = mpeg4.width;
			inh_ = mpeg4.height;
			resize(inw_, inh_);
		}
  		else {
			Decoder::redraw(xxx_frame);
		}
		stream->clear();
		idx = seq + 1;
    }
    pb->release();
}

void MPEG4Decoder::redraw()
{
    Decoder::redraw(xxx_frame);
}
