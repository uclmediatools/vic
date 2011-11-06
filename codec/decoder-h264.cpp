#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <errno.h>
#include <fcntl.h>
#ifdef WIN32
#include <io.h> //close()
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "renderer.h"
#include "packetbuffer.h"
#include "databuffer.h"
#include "ffmpeg_codec.h"
#include "rtp_h264_depayloader.h"


#define SDP_LINE_LEN 10000

//#define DIRECT_DISPLAY 1
//using namespace std;
//extern "C" UCHAR * video_frame;


class H264Decoder:public Decoder
{
  public:
    H264Decoder();
    ~H264Decoder();

    void handleSDP();
    virtual void recv(pktbuf *);
    int colorhist(u_int * hist) const;
  protected:
    void decode(const u_char * vh, const u_char * bp, int cc);
    virtual void redraw();

    /* packet statistics */
    uint16_t last_seq;		/* sequence number */

    UCHAR bitstream[MAX_CODED_SIZE];	/* bitstream data */

    /* collecting data for a frame */
    uint16_t idx;
    int last_mbit;
    int last_iframe;
    bool startPkt;

    int aggregate_pkt; // Count of mbits for decoding IOCOM H.264

    /* image */
    UCHAR xxx_frame[MAX_FRAME_SIZE];
    FFMpegCodec h264;
    PacketBuffer *stream; //SV: probably this is going to be substituted by the AVPacket->data when we call decode()...
    H264Depayloader *h264depayloader;

    //For DEBUG
    //FILE *fptr;
    //FILE *sdp_fptr;
};

static class H264DecoderMatcher:public Matcher
{
  public:
    H264DecoderMatcher():Matcher("decoder")
    {
    }
    TclObject *match(const char *id)
    {
	if ((strcasecmp(id, "h264") == 0 ) ||
	    (strcasecmp(id, "h264_IOCOM") == 0))
	    return (new H264Decoder());
	return (0);
    }
}

dm_h264;


H264Decoder::H264Decoder():Decoder(0 /* 0 byte extra header */)
{				/* , codec_(0), */


    //Barz: =============================================
    decimation_ = 420;
    /*
     * Assume CIF.  Picture header will trigger a resize if
     * we encounter another size instead.
     */
    inw_ = 352;
    inh_ = 288;

     /*XXX*/
    resize(inw_, inh_);

    // libavcodec
    h264.init(false, CODEC_ID_H264, PIX_FMT_YUV420P);
    h264.init_decoder();

    idx = 0;
    last_mbit = 0;
    last_iframe = 0;
    last_seq = 0;
    //===================================================


    //SV: ===============================================
    //Create payloader
    h264depayloader = new H264Depayloader;

    handleSDP();
    //make sure all codec params are set up properly before decoding

    //check whether AVPacket struct can be used in stead of Barz's PacketBuffer
    //===================================================


    //256 packets, each 1600 byte (default will not exceed 1600 byte)
    //cout << "new PacketBuffer..\n";
    stream = new PacketBuffer(1024, 1600); //SV: 1024 = ???
    startPkt = false;
}

void
H264Decoder::handleSDP()
{
    char SdpFilename[SDP_LINE_LEN];
    char *sdp_string;
    char *SdpLine=NULL;
    int n_char;
    int sdp_fptr;
    struct stat      s;
    size_t nBytes = 0;
    char defaultSDP[]="a=rtpmap:96 H264/90000\na=fmtp:96 profile-level-id=00000d; packetization-mode=1\n";

    sprintf(SdpFilename, "%s/default.sdp", getenv("HOME"));

    if ((sdp_fptr = open(SdpFilename, O_RDONLY)) != -1 ) {
      debug_msg("Open SDP file: %s\n",SdpFilename);

      if (fstat(sdp_fptr, &s) != 0) {
        debug_msg("Unable to stat config file\n");
        close(sdp_fptr);
        sdp_string=defaultSDP;
      }
      sdp_string = (char *)malloc(s.st_size+1);
      memset(sdp_string, '\0', s.st_size+1);
      if (read(sdp_fptr, sdp_string, s.st_size) != s.st_size) {
        debug_msg("Unable to read config file\n");
	close(sdp_fptr);
        sdp_string=defaultSDP;
      }
      close(sdp_fptr);
    } else {
      debug_msg( "H264_RTP: Couldn't open SDP file %s to read. Errno = %d\n", SdpFilename, errno);
      debug_msg("H264_RTP: Using default SDP: %s \n", defaultSDP);
      sdp_string=defaultSDP;
    }

    while ((n_char = strcspn(sdp_string, "\n"))!=0) {
      SdpLine=(char*)realloc((void*)SdpLine, n_char+1);
      memset(SdpLine, '\0', n_char+1);
      strncpy(SdpLine, sdp_string, n_char);
      sdp_string += n_char + 1;
      h264depayloader->parse_h264_sdp_line(h264.c, h264depayloader->h264_extradata, SdpLine);
    }
    free(SdpLine);
}

H264Decoder::~H264Decoder()
{
    delete stream;
    delete h264depayloader;
}

int H264Decoder::colorhist(u_int * hist)  const
{
    UNUSED(hist);

    return (1);
}

void H264Decoder::recv(pktbuf * pb)
{
    rtphdr *rh = (rtphdr *) pb->dp;
    int hdrsize = sizeof(rtphdr) + hdrlen();
    u_char *buf = pb->dp + hdrsize;
    int len = pb->len - hdrsize;
    //static int iframe_c = 0, pframe_c = 0;
    int decodeLen=0;
    int flags = ntohs(rh->rh_flags);
    int mbit = flags >> 7 & 1;
    int fmt = flags & 0x7f;
    uint16_t seq = ntohs(rh->rh_seqno);
    int packetStatus;
    //int ts = ntohl(rh->rh_ts);


    //Barz: =============================================
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
	    debug_msg("H264_RTP: sequece interrupt!\n");
	    idx = seq;
	    pktIdx = 0;
	    stream->clear();
    }else if (last_seq + 1 != seq) {
	    // oops - missing packet
	    debug_msg("H264_RTP: missing packet\n");
    }

    //===================================================

    //fprintf(stderr, "H264_RTP: packet idx:%d, seq: %d\n", pktIdx,seq);


    if (fmt == 107 ){
      // IOCOM's non-standard H.264 packet format
      //fprintf(stderr,"IOCOM pkt\n");
      if (mbit) {
          if (aggregate_pkt)
            stream->writeAppend(pktIdx, len-20, (char *)buf+20);
          else
            stream->writeAppend(pktIdx, len-21, (char *)buf+21);
          aggregate_pkt = 0;
       }  else {
          if (aggregate_pkt)
            stream->writeAppend(pktIdx, len-20, (char *)buf+20);
          else
            stream->writeAppend(pktIdx, len-21, (char *)buf+21);
          aggregate_pkt = 1;
       }
    } else {
       packetStatus = h264depayloader->h264_handle_packet(h264depayloader->h264_extradata, pktIdx, stream, buf, len);
    }


    //Barz: =============================================

    last_seq = seq;
    if (mbit) {
	    stream->setTotalPkts(pktIdx + 1);

	    DataBuffer *f;
	    if (stream->isComplete()) {
		    f = stream->getStream();
		    decodeLen =  h264.decode((UCHAR *) f->getData(), f->getDataSize(), xxx_frame);
	    }

	    if (decodeLen < 0) {
		  debug_msg("H264_RTP: frame error\n");
	    }

	    if (inw_ != h264.width || inh_ != h264.height) {
			inw_ = h264.width;
			inh_ = h264.height;
			resize(inw_, inh_);
	    } else {
			Decoder::redraw(xxx_frame);
                //render_frame(xxx_frame);
	    }
	    stream->clear();
	    idx = seq+1;
    }

    //===================================================

    pb->release();
}

void H264Decoder::redraw()
{
    Decoder::redraw(xxx_frame);
}
