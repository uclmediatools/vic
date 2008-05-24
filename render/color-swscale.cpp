#include <stdlib.h>
#include <stdio.h>
#include "vic_tcl.h"
#include "renderer-window.h"
#include "vw.h"
#include "renderer.h"

#include "config_arch.h"
extern "C" {
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
}
#include "cpu/cpudetect.h"

int available_cpu_flags = cpu_check();

#ifdef HAVE_SWSCALE

class X11WindowRenderer : public WindowDitherer {
public:
	X11WindowRenderer(VideoWindow* vw, int decimation, int depth_) :
	WindowDitherer(vw, decimation) { 
	i_width = i_height = o_width = o_height = -1;
	sws_context = NULL;
	  
	  // 15, 16, 24, or 32 bits
	  switch (depth_) {
	    case 15:
	    case 16:
		// printf("16\n");
	        //out_format = PIX_FMT_RGB565;
		out_format = PIX_FMT_BGR565;
	        bytes_per_pixel	= 2;
	    	break;
	    	
	    case 24:
		//	printf("24\n");
		out_format = PIX_FMT_RGB24;
		bytes_per_pixel = 3;
		break;
	    case 32:
			// printf("32\n");
	        out_format = PIX_FMT_RGB32;
	        bytes_per_pixel	= 4;
  	  }
	}

	~X11WindowRenderer() { 
	  if(sws_context) sws_freeContext(sws_context);
	}
	
	inline bool resized(){
	  return (i_width != width_ || i_height != height_ || o_width != outw_ || o_height != outh_);
	}
	
	void render(const u_char* frm, int off, int x, int w, int h) {
				
	  if(enable_xv){
	    memcpy(pixbuf_, frm, framesize_*3/2);	  
	  }else{ 
	    if(!outw_ || !outh_	|| !width_ || !height_)
	        return;

	    if(resized()){
	      if(sws_context){
	        sws_freeContext(sws_context);	
			sws_context = NULL;
	      }
	      int flags = SWS_FAST_BILINEAR;

#ifdef RUNTIME_CPUDETECT	    
	      flags |= (available_cpu_flags & FF_CPU_MMX ? SWS_CPU_CAPS_MMX : 0);
	      flags |= (available_cpu_flags & FF_CPU_MMXEXT ? SWS_CPU_CAPS_MMX2 : 0);
	      flags |= (available_cpu_flags & FF_CPU_3DNOW ? SWS_CPU_CAPS_3DNOW : 0);
	      flags |= (available_cpu_flags & FF_CPU_ALTIVEC ? SWS_CPU_CAPS_ALTIVEC : 0);
#elif defined(HAVE_MMX)
		  flags |= SWS_CPU_CAPS_MMX;
	#if defined(HAVE_MMX2)
		  flags |= SWS_CPU_CAPS_MMX2;
	#endif 
#elif defined(HAVE_3DNOW)
		  flags |= SWS_CPU_CAPS_3DNOW;
#elif defined(HAVE_ALTIVEC)
		  flags |= SWS_CPU_CAPS_ALTIVEC;
#endif

	      if(!outw_ || !outh_ || !width_ || !height_) return;

		  // Accelerated Colour conversion routines
	      sws_context = sws_getContext(width_, height_, PIX_FMT_YUV420P,
	             outw_, outh_, out_format, flags, NULL, NULL, NULL); 	   
	      // printf("X11WindowRenderer: %dx%d ==> %dx%d\n", width_, height_, outw_, outh_);
	      
	      if(sws_context == NULL){
	          debug_msg("X11WindowRenderer: error! cannot allocate memory for swscontext!\n");
		  return;
	      }
		      
			    
	      sws_src_stride[0] = width_;
	      sws_src_stride[1] = sws_src_stride[2] = width_/2;
	    
  	      sws_tar[0] = pixbuf_;
  	      sws_tar[1] = sws_tar[2] = NULL;
  	      sws_tar_stride[0] = outw_*bytes_per_pixel;
  	      sws_tar_stride[1] = sws_tar_stride[2] = 0;	  	      
        }

	    i_width = width_; i_height = height_; o_width = outw_; o_height = outh_;	  		 	
	    sws_src[0] = (uint8_t*)frm;
	    sws_src[1] = sws_src[0] + framesize_;
	    sws_src[2] = sws_src[1] + framesize_/4;

#if LIBSWSCALE_VERSION_INT  >= ((0<<16)+(5<<8)+0)
     	sws_scale(sws_context, sws_src, sws_src_stride, 0, height_, sws_tar, sws_tar_stride);
#else
     	sws_scale_ordered(sws_context, sws_src, sws_src_stride, 0, height_, sws_tar, sws_tar_stride);
#endif
	  }	
	}
protected:
	virtual void update() { }
	virtual void disable() { }

	// libswscale: color conversion and interpolation
	int i_width, i_height, o_width, o_height;
	SwsContext *sws_context;
	uint8_t *sws_src[3];
  	uint8_t *sws_tar[3];
  	int sws_src_stride[3];
  	int sws_tar_stride[3];	
  	int out_format;
  	int bytes_per_pixel;
};


class X11ColorModel : public TclObject {
public:
	virtual int command(int argc, const char*const* argv){
	  Tcl& tcl = Tcl::instance();	
	  
	  if (argc == 4 && strcmp(argv[1], "renderer") == 0) {

		VideoWindow* vw = VideoWindow::lookup(argv[2]);
		int decimation = atoi(argv[3]);
		
		Renderer* r = new X11WindowRenderer(vw, decimation, vw->bpp());
		tcl.result(r->name());
		return (TCL_OK);
	  }
	  if (argc == 2) {
		if (strcmp(argv[1], "alloc-colors") == 0) {
			tcl.result("1");
			return (TCL_OK);
		}
		if (strcmp(argv[1], "free-colors") == 0) {
			return (TCL_OK);
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "visual") == 0) {
			return (TCL_OK);
		}
		if (strcmp(argv[1], "gamma") == 0) {
			tcl.result("1");
			return (TCL_OK);
		}
	}	  
	  return (TclObject::command(argc, argv));
	}
};


class X11ColorMatcher : public Matcher {
public:
	X11ColorMatcher() : Matcher("colormodel") {}
	virtual TclObject* match(const char* id) {
		if ( strcasecmp(id, "truecolor/15") == 0 ||
		     strcasecmp(id, "truecolor/16") == 0 ||
		     strcasecmp(id, "truecolor/24") == 0 ||
		     strcasecmp(id, "truecolor/32") == 0 ){
			return (new X11ColorModel());
		}		
		return (0);
	}
} matcher_x11;

					       

#endif // HAVE_SWSCALE
