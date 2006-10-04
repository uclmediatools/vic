#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "ffmpeg_codec.h"


static Transmitter *tx;
static RTP_BufferPool *pool;
static u_int32_t ts;
static int vicEncodedMB;
static int vicNumMB;

class MPEG4Encoder:public TransmitterModule
{
  public:
    MPEG4Encoder();
    ~MPEG4Encoder();

    void setq(int q);
    int command(int argc, const char *const *argv);

    void size(int w, int h);
    int consume(const VideoFrame *);
    static void rtp_callback(AVCodecContext * c, void *data, int size,
			     int num_mb);

  protected:
    int fps, bps, gop;
    bool state;

    FFMpegCodec mpeg4;
    UCHAR *bitstream;

};

static class MPEG4EncoderMatcher:public Matcher
{
  public:
    MPEG4EncoderMatcher():Matcher("module")
    {
    }
    TclObject *match(const char *fmt)
    {
	if (strcasecmp(fmt, "mpeg4") == 0)
	    return (new MPEG4Encoder);
	return (0);
    }
}

encoder_matcher_mpeg4;

MPEG4Encoder::MPEG4Encoder():TransmitterModule(FT_YUV_CIF)
{
    state = false;
    mpeg4.init(true, CODEC_ID_MPEG4, PIX_FMT_YUV420P);
    mpeg4.rtp_callback = rtp_callback;
    bps = 1000 * 1024;
    fps = 20;
    gop = 20;
    vicNumMB = 0;
    vicEncodedMB = 0;
}

MPEG4Encoder::~MPEG4Encoder()
{

}

void MPEG4Encoder::size(int w, int h)
{
    Module::size(w, h);
    vicNumMB = (w / 16 * h / 16);
    vicEncodedMB = 0;
}

int MPEG4Encoder::command(int argc, const char *const *argv)
{
    if (argc == 3) {
	if (strcmp(argv[1], "q") == 0) {
	    // mpeg4.quality = atoi(argv[2]);
	    // mpeg4.set_max_quantizer(mpeg4.quality);
	    gop = atoi(argv[2]);
	    mpeg4.set_gop(gop);
	    return (TCL_OK);
	}
	else if (strcmp(argv[1], "fps") == 0) {
	    fps = atoi(argv[2]);
	    return (TCL_OK);
	}
	else if (strcmp(argv[1], "bps") == 0) {
	    bps = atoi(argv[2]);
	    return (TCL_OK);
	}
	else if (strcmp(argv[1], "hq") == 0) {
	    int enable_hq = atoi(argv[2]);
	    //printf("enable mpeg4 high quality encoding\n", enable_hq);
	    mpeg4.enable_hq_encoding = bool(enable_hq);
	    return (TCL_OK);
	}
    }
    return (TransmitterModule::command(argc, argv));
}

void MPEG4Encoder::rtp_callback(AVCodecContext * c, void *data, int size,
				int num_mb)
{
    //printf("packet size = %d, packet num= %d \n", size, packet_number);

    pktbuf *pb;
    rtphdr *rh;

    pb = pool->alloc(ts, RTP_PT_MPEG4);
    rh = (rtphdr *) pb->data;

    vicEncodedMB += num_mb;
    if (vicEncodedMB == vicNumMB) {
	//std::cout << "mpeg4enc: set RTP_M\n";
	rh->rh_flags |= htons(RTP_M);
	vicEncodedMB = 0;
    }

    UCHAR *pd = (UCHAR *) data;

    // sizeof(rtphdr) is 12
    memcpy(&pb->data[12], data, size);

    pb->len = size + 12;
    tx->send(pb);
}


int MPEG4Encoder::consume(const VideoFrame * vf)
{
    pktbuf *pb;
    rtphdr *rh;
    int len;
    ts = vf->ts_;
    tx = tx_;
    pool = pool_;

    static int pframe_no = 0;

    double psnr = 0;

    if (!state) {
	state = true;
	size(vf->width_, vf->height_);
	std::cout << "MPEG4 ENC: WxH:" << vf->width_ << "x" << vf->height_;
	std::cout << "BPS:" << bps * 1024 << " FPS:" << fps << "\n";
	mpeg4.init_encoder(vf->width_, vf->height_, bps * 1024, fps, gop);
    }
    tx_->flush();
    bitstream = mpeg4.encode(vf->bp_, len);

    psnr = mpeg4.get_PSNR();
    char buf[20];
    //sprintf(buf,"%.1f dB", psnr);    
    Tcl tcl = Tcl::instance();
    Tcl_SetVar(tcl.interp(), "psnr", buf, TCL_GLOBAL_ONLY);

    /*
       if(mpeg4.pict_type == FF_I_TYPE){ 
       pframe_no=0;
       return len/2;  // for lag reduction
       }
       else 
       return len; */
    /*
     * champ:
     * VIC will grab a frame with a time interval according to this return value.
     * Since this module adopts rate-control by bps value, it returns a constant value for a reason of smoothness.
     */
    return (bps * 100) / fps;
}
