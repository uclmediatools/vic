
#ifdef USE_DVDECODER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "renderer.h"

#include <libdv/dv.h>

/*
 * libdv does either yv12 or yuyv and this is determined at
 * libdv's compile time. There is no way programatically to 
 * check this, so we use the following #define depending on
 * how libdv is compiled. A segfault is likely if libdv 
 * produces yuy2 and this code assumes yv12 and visa-versa.
 */

#define ASSUME_YUY2 1
#if ASSUME_YUY2 == 1
static void yuy2_to_i420(const u_char *in,
			 u_int width,
			 u_int height,
			 u_char *out);
#endif // ASSUME_YUY2

class DVDecoder : public Decoder {
    public:
	DVDecoder();
	virtual ~DVDecoder();
	virtual void info(char* wrk) const;
	virtual void recv(pktbuf*);
	int colorhist(u_int* hist) const;
	virtual void stats(char* wrk);
    protected:
	void decode(const u_char* vh, const u_char* bp, int cc);
	virtual void redraw();

        dv_decoder_t *dv_decoder;
        u_char *dv_buffer;

        u_char *dv_frame;

#if ASSUME_YUY2 == 1
	u_char *dv_yuy2_frame;
#endif 

        bool header_received;
        bool headers_received[12];
        bool vaux_received[12];

        unsigned int difblocks;
};

static class DVDecoderMatcher : public Matcher {
    public:
	DVDecoderMatcher() : Matcher("decoder") {}
	TclObject* match(const char* id) {
		if (strcasecmp(id, "dv") == 0)
			return (new DVDecoder());
		return (0);
	}
} dm_dvdecoder;

#define STAT_BAD_PSC	0
#define STAT_BAD_GOB	1
#define STAT_BAD_SYNTAX	2
#define STAT_BAD_FMT	3
#define STAT_FMT_CHANGE 4	/* # times fmt changed */
#define STAT_BAD_HEADER 5

/* From libdv's encode.h */
#define DV_WIDTH       720
#define DV_PAL_HEIGHT  576
#define DV_NTSC_HEIGHT 480

DVDecoder::DVDecoder() : Decoder(0), 
			 dv_decoder(0), 
			 dv_buffer(new u_char[144000]), 
			 dv_frame(new u_char[(DV_WIDTH * DV_PAL_HEIGHT * 3)/2]), 
#if ASSUME_YUY2 == 1
			 dv_yuy2_frame(new u_char[DV_WIDTH * DV_PAL_HEIGHT * 2]),
#endif 
			 header_received(false),
			 difblocks(0)
{
	stat_[STAT_BAD_PSC].name = "H261-Bad-PSC";
	stat_[STAT_BAD_GOB].name = "H261-Bad-GOB";
	stat_[STAT_BAD_SYNTAX].name = "H261-Bad-Syntax";
	stat_[STAT_BAD_FMT].name = "H261-Bad-fmt";
	stat_[STAT_FMT_CHANGE].name = "H261-Fmt-change";
	stat_[STAT_BAD_HEADER].name = "H261-Bad-Header";
	nstat_ = 6;

	decimation_ = 420;

	/*
	 * Assume PAL. 
	 */
	inw_ = DV_WIDTH;
	inh_ = DV_PAL_HEIGHT;

	/*XXX*/
	resize(inw_, inh_);

	for (int i = 0; i < 12; i ++) {
	  headers_received[i] = false;
	  vaux_received[i] = false;
	}

	dv_decoder = dv_decoder_new(0, TRUE, TRUE);
	//dv_decoder->quality = DV_QUALITY_FASTEST;
	dv_decoder->quality = DV_QUALITY_BEST;

	memset(dv_buffer, 0, 144000);
	memset(dv_frame, 127, (DV_WIDTH * DV_PAL_HEIGHT * 3)/2);
#if ASSUME_YUY2 == 1
	memset(dv_yuy2_frame, 127, DV_WIDTH * DV_PAL_HEIGHT * 2);
#endif 
}

DVDecoder::~DVDecoder()
{

}

void DVDecoder::info(char* wrk) const
{
        *wrk = 0;
}

void DVDecoder::stats(char* wrk)
{
        Decoder::stats(wrk);
}

int DVDecoder::colorhist(u_int* hist) const
{
        UNUSED(hist);
	return (0);
}

void DVDecoder::recv(pktbuf* pb)
{	
	rtphdr* rh = (rtphdr*)pb->dp;
	u_char* vh = (u_char*)(rh + 1);
	u_int cc = pb->len - sizeof(*rh);

	while (cc >= 80) {
	  u_int sct = (u_int)(vh[0] & 0xe0) >> 5;
	  u_int dseq = (u_int)(vh[1] & 0xf0) >> 4;
	  u_int dbn = (u_int)(vh[2]);
	  u_int offset;

	  switch(sct) {
	  case 0: // Header
	    offset = dseq * 12000;
	    headers_received[dseq] = true;
	    break;

	  case 1: // Subcode
	    offset = dseq * 12000 + (dbn + 1) * 80;
	    break;

	  case 2: // VAUX
	    offset = dseq * 12000 + (dbn + 3) * 80;
	    vaux_received[dseq] = true;
	    break;

	  case 3: // Audio
	    offset = dseq * 12000 + 480 + (dbn * 16 * 80);
	    break;
	    
	  case 4: // Video
	    offset = (dseq * 150 + 7 + dbn + dbn/15) * 80;
	    break;

	  default:
	    // TODO: Log error here.
	    continue;
	  }

	  if (offset + 80 > 144000) {
	    // TODO: Log error here.
	    continue;
	  }

	  memcpy(dv_buffer + offset, vh, 80);
	  difblocks ++;

	  vh += 80;
	  cc -= 80;
	}
	   
	/*XXX*/
	if (ntohs(rh->rh_flags) & RTP_M) {
	        /*
	         * Don't try and decode until we're sure what system of dv
		 * we have and the best way to tell is to make sure we have
		 * the headers and vaux headers.
	         */
	        if (!header_received) {
  		        header_received = true;
		        for (int i = 0; i < 10; i ++) {
		                if (!headers_received[i] || 
				    !vaux_received[i]) {
		                        header_received = false;
		                        break;
		                }
		        }
		}

		if (header_received) {
		        if (dv_parse_header(dv_decoder, 
					    (const uint8_t*)dv_buffer) >= 0 &&
			    dv_decoder->system != e_dv_system_none) {

		                switch(dv_decoder->system) {
		                case e_dv_system_525_60: // NTSC
		                        if (inh_ != DV_NTSC_HEIGHT) {
			                        inh_ = DV_NTSC_HEIGHT;
			                        resize(inw_, inh_);
		                        }
		                        break;

		                case e_dv_system_625_50: // PAL
		                        if (inh_ != DV_PAL_HEIGHT) {
			                        inh_ = DV_PAL_HEIGHT;
			                        resize(inw_, inh_);
		                        }
		                        break;
				case e_dv_system_none:
				        // Handled in if statement above.
				        break;
				}

#if ASSUME_YUY2 == 1
				unsigned char *pixels[3];
				int pitches[3];

				pixels[0] = dv_yuy2_frame;
				pixels[1] = 0;
				pixels[2] = 0;

				pitches[0] = inw_*2;
				pitches[1] = 0;
				pitches[2] = 0;

 				dv_decode_full_frame(dv_decoder,
 						     (const uint8_t*)dv_buffer,
 						     e_dv_color_yuv,
 						     pixels,
 						     pitches);

				yuy2_to_i420(dv_yuy2_frame,
					     inw_,
					     inh_,
					     dv_frame);
#else				
				unsigned char *pixels[3];
				int pitches[3];

				// U and V planes deliberately swapped here.
				pixels[0] = dv_frame;
				pixels[1] = dv_frame + (inh_ * inw_ * 5)/4;
				pixels[2] = dv_frame + (inh_ * inw_);

				pitches[0] = inw_;
				pitches[1] = inw_/2;
				pitches[2] = inw_/2;

 				dv_decode_full_frame(dv_decoder,
 						     (const uint8_t*)dv_buffer,
 						     e_dv_color_yuv,
 						     pixels,
 						     pitches);
#endif // ASSUME_YUY2

				// Indicate that the entire frame has updated
				// as there isn't an easy way to do partial
				// frame updates with DV.
				memset(rvts_, now_, nblk_);
				ndblk_ = nblk_;
				
				render_frame(dv_frame);
			}
		}
		difblocks = 0;
	}
	pb->release();
}

void DVDecoder::redraw()
{
        Decoder::redraw(dv_frame);
}

static void yuy2_to_i420(const u_char *in,
			 u_int width,
			 u_int height,
			 u_char *out)
{
  const u_char *_in[2];
  u_int in_rowstride = width * 2;
    

  u_char *out_y[2];
  u_char *out_u;
  u_char *out_v;

  u_int half_width = width/2;
  u_int half_height = height/2;

  _in[0] = in;
  _in[1] = in + in_rowstride;

  out_y[0] = out;
  out_y[1] = out + width;
  out_u = out + width * height;
  out_v = out + (width * height * 5)/4;

  for (u_int j = 0; j < half_height; j ++) {
    for (u_int i = 0; i < half_width; i ++, _in[0] += 4, _in[1] += 4, out_y[0] += 2, out_y[1] += 2, out_u ++, out_v ++) {

      out_y[0][0] = _in[0][0];
      out_y[0][1] = _in[0][2];

      out_y[1][0] = _in[1][0];
      out_y[1][1] = _in[1][2];

      *out_u = _in[0][1];
      *out_v = _in[0][3];
    }
     _in[0] += in_rowstride;
     _in[1] += in_rowstride;

     out_y[0] += width;
     out_y[1] += width;
  }
}

#endif // USE_DVDECODER

