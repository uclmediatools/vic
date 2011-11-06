#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <iostream>

#include "inet.h"
#include "net.h"
#include "rtp.h"
#include "vic_tcl.h"
#include "transmitter.h"
#include "pktbuf-rtp.h"
#include "module.h"

#include "databuffer.h"
#include "x264encoder.h"
#include "deinterlace.h"
/*extern "C"
{
#include "base64.h"
}*/

static Transmitter *tx;
static RTP_BufferPool *pool;
static u_int32_t ts;
static unsigned char frame_seq;

class H264Encoder:public TransmitterModule
{
  public:
    H264Encoder();
    ~H264Encoder();

    void setq(int q);
    int command(int argc, const char *const *argv);

    void size(int w, int h);
    int consume(const VideoFrame *);

  protected:
    int fps, kbps, gop;
    bool state;
    //UCHAR* bitstream;

    x264Encoder *enc;
    DataBuffer *fOut;
    Deinterlace deinterlacer;
    bool use_deinterlacer;

    FILE *fptr;
};

static class H264EncoderMatcher:public Matcher
{
  public:
    H264EncoderMatcher():Matcher("module")
    {
    }
    TclObject *match(const char *fmt)
    {
	if (strcasecmp(fmt, "h264") == 0)
	    return (new H264Encoder);
	return (0);
    }
}

encoder_matcher_h264;

H264Encoder::H264Encoder():TransmitterModule(FT_YUV_420)
{
    enc = new x264Encoder();
    state = false;
    fptr = NULL;
    frame_seq = 0;
    fps = 20;
    kbps = 512;
    gop = 20;
    fOut=NULL;
    use_deinterlacer=false; 
}

H264Encoder::~H264Encoder()
{
    delete enc;
    if (fOut) delete fOut;
}

void H264Encoder::size(int w, int h)
{
    debug_msg("H264: WxH %dx%d\n", w, h);
    Module::size(w, h);
    fOut = new DataBuffer(w * h * 3 >> 2);
}

int H264Encoder::command(int argc, const char *const *argv)
{
    if (argc == 3) {
	if (strcmp(argv[1], "q") == 0) {
	    gop = atoi(argv[2]);
	    return (TCL_OK);
	}
	else if (strcmp(argv[1], "fps") == 0) {
	    fps = atoi(argv[2]);
	    //std::cout << "H264: fps " << fps << "\n";
	    return (TCL_OK);
	}
	else if (strcmp(argv[1], "kbps") == 0) {
	    kbps = atoi(argv[2]);
	    if (kbps < 64)
		kbps = 64;
	    //std::cout << "H264: kbps " << kbps << "\n";
	    return (TCL_OK);
	}
	else if (strcmp(argv[1], "useDeinterlacer") == 0) {
	    use_deinterlacer = atoi(argv[2]);
	    return (TCL_OK);
	}
	else if (strcmp(argv[1], "hq") == 0) {
	    int enable_hq = atoi(argv[2]);
	    return (TCL_OK);
	}
    }
    return (TransmitterModule::command(argc, argv));
    return 0;
}

int H264Encoder::consume(const VideoFrame * vf)
{
    pktbuf *pb;
    rtphdr *rh;
    ts = vf->ts_;
    tx = tx_;
    pool = pool_;

    int numNAL, i, sent_size = 0;
    int frame_size = 0;
    unsigned char f_seq = 0;
    //unsigned char f_total_pkt = 0;
    int RTP_HDR_LEN = sizeof(rtphdr);
    int NAL_FRAG_THRESH = tx->mtu() - RTP_HDR_LEN; /* payload max in one packet */
#ifdef H264DEBUG
    debug_msg( "MTU=%d, RTP_HDR_LEN=%d\n", NAL_FRAG_THRESH, RTP_HDR_LEN);
#endif

    tx->flush();

    if (!state) {
	    state = true;
	    size(vf->width_, vf->height_);
	    debug_msg("init x264 encoder with kbps:%d, fps:%d\n", kbps, fps);
	    enc->setGOP(gop);
	    enc->init(vf->width_, vf->height_, kbps, fps);
	    frame_size = vf->width_ * vf->height_;
    }

    frame_size = vf->width_ * vf->height_;
    if (use_deinterlacer) {
	    deinterlacer.render(vf->bp_, vf->width_, vf->height_);
    }
    enc->encodeFrame(vf->bp_);
    numNAL = enc->numNAL();

    //Send out numNAL packets framed according to RFC3984
    for (i = 0; i < numNAL; i++) {

	bool firstFragment = true;
	int FU_HDR_LEN = 1;
	int offset = 0;

	enc->getNALPacket(i, fOut);
	int nalSize1 = fOut->getDataSize();
	int nalSize = nalSize1-5;
	char *data1 = fOut->getData();
	char *data = fOut->getData();
 	uint8_t NALhdr = data1[4]; //SV-XXX why does our x.264 provide 4-byte StartSync in the NALU?
	uint8_t NALtype = NALhdr & 0x1f;
#ifdef H264DEBUG
	debug_msg( "Got NALhdr=0x%02x, NALtype=0x%02x, nal size %i from encoded frame.\n", NALhdr, NALtype, nalSize );
#endif
	memcpy(data, &data1[5], nalSize);

	sent_size += nalSize;


	while (nalSize > 0) {
	    pb = pool->alloc(vf->ts_, RTP_PT_H264);
	    rh = (rtphdr *) pb->data;

	    if (nalSize <= NAL_FRAG_THRESH) {
		//==============================================
		//Single NAL or last fragment of FU-A
		//==============================================

		if ( i == numNAL - 1 )
			rh->rh_flags |= htons(RTP_M);	// set M bit - ONLY if last NAL of frame

		pb->len = nalSize + RTP_HDR_LEN + FU_HDR_LEN;

#ifdef H264DEBUG
		debug_msg( "NAL : ");
#endif
		if (FU_HDR_LEN==2) {
			//==============================================
			//Last fragment of FU-A
			//==============================================
	       		pb->data[12] = 0x00 | (NALhdr & 0x60) | 28; 	//FU indicator
       			pb->data[13] = 0x40  | NALtype; 		//FU header
			
#ifdef H264DEBUG
			debug_msg( "FU_Indicator=0x%02x, FU_Header=0x%02x, ", pb->data[12], pb->data[13]);
#endif
		} 
		else {
	       		pb->data[12] = NALhdr; 				//NAL Header
#ifdef H264DEBUG
			debug_msg( "-----------------, --------------, ");
#endif
		}

		memcpy(&pb->data[RTP_HDR_LEN + FU_HDR_LEN], data + offset, nalSize);

#ifdef H264DEBUG
		debug_msg( "i=%d/%d, nalSize=%4d len=%4d firstFrag=%d offset=%4d\n", i, numNAL, nalSize, pb->len, firstFragment, offset);
#endif

		nalSize = 0;
		offset = 0;

	    } else {
		//==============================================
		//FU-A (not the last fragment though)
		//==============================================

		FU_HDR_LEN = 2;
		pb->len = (NAL_FRAG_THRESH - FU_HDR_LEN) + RTP_HDR_LEN + FU_HDR_LEN;

       		pb->data[12] = 0x00 | (NALhdr & 0x60) | 28; 			//FU indicator
       		pb->data[13] = ( (firstFragment) ? 0x80 : 0x00 ) | NALtype;	//FU header

		memcpy(&pb->data[RTP_HDR_LEN + FU_HDR_LEN], data + offset, NAL_FRAG_THRESH - FU_HDR_LEN);

#ifdef H264DEBUG
		debug_msg( "FU-A: FU_Indicator=0x%02x, FU_Header=0x%02x, i=%d/%d, nalSize=%4d len=%4d firstFrag=%d offset=%4d\n",  pb->data[12], pb->data[13], i, numNAL, nalSize, pb->len, firstFragment, offset);
#endif

		nalSize -= (NAL_FRAG_THRESH-FU_HDR_LEN);
		offset += (NAL_FRAG_THRESH-FU_HDR_LEN);
		firstFragment = false;
	    }

	    tx->send(pb);
	    f_seq++;
	}

    }

    frame_seq++;


    return (kbps*1024) / (fps*8);
}
