/*
 * Copyright (c) 2004 Regents of National Center of High Performance Computing.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and the Network Research Group at
 *      Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header: /forge/cvsroot/nchc-vic/vic/render/color-x11.cpp,v 1.1.1.1 2005/08/11 15:28:33 barz Exp $ (LBL)";
#endif

#include <stdlib.h>
#include <stdio.h>
#include "vic_tcl.h"
#include "renderer-window.h"
#include "vw.h"
#include "renderer.h"
#include "postproc/swscale.h"

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
	        out_format = IMGFMT_BGR16;
	        bytes_per_pixel	= 2;
	    	break;
	    	
	    case 24:
	    case 32:
	        out_format = IMGFMT_BGR32;
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
	    if(resized()){
	      if(sws_context)
	        sws_freeContext(sws_context);	
	      sws_context = sws_getContext(width_, height_, IMGFMT_I420,
	                 outw_, outh_, out_format, SWS_FAST_BILINEAR, NULL, NULL, NULL); 	   
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

     	    sws_scale_ordered(sws_context, sws_src, sws_src_stride, 0, height_, sws_tar, sws_tar_stride);
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

					       


