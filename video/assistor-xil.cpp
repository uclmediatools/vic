/*
 * Copyright (c) 1995 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
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
/*
static char rcsid[] =
    "@(#) $Header$ (LBL)";
 */
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <xil/xil.h>
#include <sys/types.h>
#include <sys/ipc.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>


#include "vic_tcl.h"
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "vw.h"
#include "renderer.h"
#include "device-output.h"
#include "xil.h"

#define M_SOI 0xd8
#define M_APP0 0xe0
#define M_DQT 0xdb
#define M_SOF0 0xc0	/* marks BASELINE DCT */
#define M_DHT 0xc4
#define M_SOS 0xda

static int
mapkoord2jfifentry[] = {
	0, 1,8, 16,9,2, 3,10,17,24, 32,25,18,11,4, 5,12,19,26,33,40,
	48,41,34,27,20,13,6, 7,14,21,28,35,42,49,56, 57,50,43,36,29,22,15,
	23,30,37,44,51,58, 59,52,45,38,31, 39,46,53,60, 61,54,47, 55,62, 63

};

int XILWindowAssistor::consume(const VideoFrame* vf)
{
	int	len=0,dofree=0;
	u_char	*bp,*p=NULL;
        float scale[3], offset[3];
        XilColorspace ycc, rgb;

//	fprintf(stderr,"XILWindowAssistor::"__FUNCTION__"\n");
	switch (type_) {
	case FT_JPEG|FT_HW: {
		JpegFrame* jf = (JpegFrame*)vf;
		if (	(jf->type_ != lastjpegtype_) ||
			(jf->q_ != inq_) ||
			(jf->width_ != width_) ||
			(jf->height_ != height_)
		) {
			//fprintf(stderr,"w=%d,h=%d,q=%d\n",jf->width_,jf->height_,jf->q_);
			reset(jf->type_,jf->q_, jf->width_, jf->height_);
		}
		len=jf->len_;
		bp=jf->bp_;
		dofree=1;
		if (sentfirstheader) {
			p = new u_char[sizeof(jfif_header2)+len];
			memcpy(p,jfif_header2,sizeof(jfif_header2));
			memcpy(p+sizeof(jfif_header2),bp,len);
			len+=sizeof(jfif_header2);
		} else {
			p = new u_char[sizeof(jfif_header1)+len];
			memcpy(p,jfif_header1,sizeof(jfif_header1));
			memcpy(p+sizeof(jfif_header1),bp,len);
			len+=sizeof(jfif_header1);
			sentfirstheader=1;
		}
		break;
	}
	case FT_CELLB: {
		CellBFrame* hf = (CellBFrame*) vf;
		len=hf->len_;
		p=hf->bp_;
		xil_cis_set_attribute(cis_, "WIDTH", (void *)hf->width_);
		xil_cis_set_attribute(cis_, "HEIGHT", (void *)hf->height_);
		break;
	}
	case FT_H261: {
		H261Frame* hf = (H261Frame*) vf;
		len=hf->len_;
		p=hf->bp_;
		//Don't accept double zero bytes. xil chokes on that.
		while ((p[0]==0)&&(p[1]==0)&&len) {
			p++;len--;
		}
		break;
	}
	}
	xil_cis_put_bits(cis_,len,1/*framecount*/,p);
	if (dofree) delete[] p; //move this to the end, if you use put_bits_ptr //SV-XXX: Debian
	outputtype = xil_cis_get_output_type(cis_);
	xil_imagetype_get_info(outputtype, &cis_xsize, &cis_ysize,&cis_nbands, &cis_datatype);

	if (!imageYCC || (lastcisw!=cis_xsize) || (lastcish!=cis_ysize)) {
		lastcish = cis_ysize;
		lastcisw = cis_xsize;
		//fprintf(stderr,"	w=%d,h=%d,b=%d,t=%d\n",cis_xsize,cis_ysize,cis_nbands,cis_datatype);
		if (imageYCC) xil_destroy(imageYCC);
		if (ximage_) xil_destroy(ximage_);
		ximage_ = xil_create(xil_,cis_xsize,cis_ysize,cis_nbands,cis_datatype);
		imageYCC = xil_create(xil_,cis_xsize,cis_ysize,cis_nbands,cis_datatype);
	}
        if (visual == PSEUDOCOLOR) {
            scale[0] = 255.0 / (235.0 - 16.0);
            scale[1] = 255.0 / (240.0 - 16.0);
            scale[2] = 255.0 / (240.0 - 16.0);
            offset[0] = -16.0 * scale[0];
            offset[1] = -16.0 * scale[1];
            offset[2] = -16.0 * scale[2];

	    if (!x_cmap) {
		    colorcube = xil_lookup_get_by_name(xil_, "cc855");
		    dmask = xil_dithermask_get_by_name(xil_, "dm443");
		    xil_cmap = create_cmap(&x_cmap, FT_JPEG, NULL, 
			xil_lookup_get_by_name(xil_, "yuv_to_rgb"),
			colorcube
		    );
	    }
            /* Begin XIL Molecule... */
            xil_decompress( cis_, imageYCC );
            xil_rescale(imageYCC, imageYCC, scale, offset);
	    xil_scale(imageYCC,ximage_,"nearest",(1.0*targetw_)/lastcisw,(1.0*targeth_)/lastcish);
            xil_ordered_dither( ximage_, displayimage_, colorcube, dmask);
            /* End of XIL Molecule */
        }

        if (visual == TRUECOLOR) {
            ycc = xil_colorspace_get_by_name(xil_, "ycc601");
            rgb = xil_colorspace_get_by_name(xil_, "rgb709");
            /* for H261, XIL may drop core on the last frame 
             * to avoid this, only continue as long as 2 or
             * more frames in the CIS */
            /* Begin XIL molecule */
	    xil_set_attribute(displayimage_,"HW_DECOMPRESS_CIS",(void *)-1);
            xil_decompress(cis_, imageYCC);
	    if ((targetw_ == lastcisw) && (targeth_ == lastcish)) {
		    xil_set_colorspace(imageYCC, ycc);
		    xil_set_colorspace(displayimage_, rgb);
            	    xil_color_convert(imageYCC, displayimage_);
	    } else {
		    xil_set_colorspace(imageYCC, ycc);
		    xil_set_colorspace(ximage_, rgb);
		    xil_color_convert(imageYCC, ximage_);
		    xil_scale(ximage_,displayimage_,"nearest",(1.0*targetw_)/lastcisw,(1.0*targeth_)/lastcish);
	    }
            /* End of XIL molecule */
        }
	xil_cis_sync(cis_);

	xil_toss(imageYCC);
	xil_toss(ximage_);

	xil_cis_reset(cis_);

	return (0);
}

void XILWindowAssistor::handle_exposure_idle(ClientData cd) {
	//fprintf(stderr,"XILWindowAssistor::"__FUNCTION__"\n");
	((XILWindowAssistor *)cd)->handle_exposure();
}

void XILWindowAssistor::handle_exposure_callback(ClientData cd, XEvent *) {
	//fprintf(stderr,"XILWindowAssistor::"__FUNCTION__"\n");
	Tk_DoWhenIdle(handle_exposure_idle, cd);
}

XILWindowAssistor::XILWindowAssistor(int type)
	:  Renderer(type), inq_(20)
{
	char	*cisname;
	XilDevice device;

	window_ = NULL;
	//fprintf(stderr,"XILWindowAssistor::"__FUNCTION__"\n");
	cisname=NULL;
	switch (type) {
	case FT_JPEG|FT_HW:
		cisname = "Jpeg";
		memcpy(jfif_header1,jfif_header1_,sizeof(jfif_header1_));
		memcpy(jfif_header2,jfif_header2_,sizeof(jfif_header2_));
		decimation_ = 420;
		lastjpegtype_ = 0xff; // invalid, so we reset first..
		break;
	case FT_CELLB:
		cisname = "CellB";
		break;
	case FT_H261:
		cisname = "H261";
		break;
	default:
		fprintf(stderr,"	unknown cis %d???\n",type&~FT_HW);
		break;
	}
	type_ = type;
	need_refresh_ = 1;
	imageYCC = NULL;
	ximage_ = NULL;
	xil_ = xil_open();
	if (xil_ == 0) {
		fprintf(stderr,"couldn't open xil.\n");
		return;//FIXME
	}
/*
	device = xil_device_create(xil_,"MMACo1k");
	if (device == 0) {
		fprintf(stderr,"couldn't open MMACo1k \n");
		return; //FIXME
	}
	xil_device_set_value(device,"DEVICE_NAME",(void*)"/dev/o1k0");
	//fprintf(stderr,"VIDEO_OUT\n");
	xil_device_set_value(device,"VIDEO_OUT",(void*)FALSE);
	xil_device_destroy(device);
*/
	cis_ = xil_cis_create(xil_ , cisname);
	if (cis_ == 0) {
		fprintf(stderr,"couldn't create CIS for %s \n",cisname);
		return;
	}
//	xil_cis_set_attribute(cis_,"DECOMPRESSION_QUALITY", (void*)100);
}

XILWindowAssistor::~XILWindowAssistor()
{
	//fprintf(stderr,"XILWindowAssistor::~XILWindowAssistor\n");
	if (cis_) {
		xil_cis_sync(cis_);
		xil_cis_destroy(cis_);
		cis_ = 0;
	}
	/*
	 * Clear out the image so that the Video class does
	 * not try to use it before another window gets attached
	 * and replaces the image.
	 */ 
	if (window_) {
		window_->setimage(0);
	// Tk_FreeGC(Tk_Display(window_->tkwin()), gc_);
		XFreeGC(Tk_Display(window_->tkwin()), gc_);
	}
	if (ximage_) xil_destroy(ximage_);
	if (displayimage_) {
		xil_set_attribute(displayimage_,"HW_DECOMPRESS_CIS",NULL);
		xil_destroy(displayimage_);
	}
	if (imageYCC)
		xil_destroy(imageYCC);
	/*
	if (xil_) {
		xil_close(xil_);xil_=0;
	}
	*/
}

int XILWindowAssistor::command(int argc, const char*const* argv)
{
	u_int		nbands;
	XilDataType	datatype;

//	fprintf(stderr,"XILWindowAssistor::"__FUNCTION__"\n");
	if (argc == 3 && strcmp(argv[1], "window") == 0) {
		sentfirstheader=0;
		imageYCC = NULL;
		ximage_ = NULL;
		window_ = VideoWindow::lookup(argv[2]);
		Tk_MakeWindowExist(window_->tkwin());
		// gc_ = Tk_GetGC(window_->tkwin(), 0, 0);
		gc_ = XCreateGC(Tk_Display(window_->tkwin()),
				Tk_WindowId(window_->tkwin()), 0, NULL);
		Tk_CreateEventHandler(window_->tkwin(), ExposureMask,
				      handle_exposure_callback,
				      (ClientData)this);
		//fprintf(stderr,"xil_create_from_window(%p,%p,%08lx)\n", xil_,Tk_Display(window_->tkwin()),Tk_WindowId(window_->tkwin()));
		displayimage_=xil_create_from_window(xil_,Tk_Display(window_->tkwin()),Tk_WindowId(window_->tkwin()));
		//fprintf(stderr,"HW_DECOMPRESS_CIS\n");
		// disable (osprey) hardware decompression for Windowed Display.
		// the software decoder is fast enough
		xil_set_attribute(displayimage_,"HW_DECOMPRESS_CIS",(void *)-1);
/*
		int arg;
		xil_get_device_attribute(displayimage_,"VIDEO_OUT",(void**)&arg);
		//fprintf(stderr,"VIDEO_OUT-> %d\n",arg);

		xil_set_device_attribute(displayimage_,"VIDEO_OUT",(void*)FALSE);

 */
//		xil_set_device_attribute(displayimage_,"H261_PIP",(void*)TRUE);

		xil_get_info(displayimage_, &targetw_, &targeth_, &nbands, &datatype);
		//fprintf(stderr,"w=%d,h=%d,bands=%d,dt=%d\n", targetw_,targeth_,nbands,datatype);
		switch (Tk_Depth(window_->tkwin())) {
		case 8:
			visual = PSEUDOCOLOR;
			break;
		case 24:
			visual = TRUECOLOR;
			break;
		default:
			fprintf(stderr,"XILWindowAssistor:command(window), unknown depth %d!\n",Tk_Depth(window_->tkwin()));
			visual = PSEUDOCOLOR;
			break;
		}
		x_cmap = 0;
		return (TCL_OK);
	}
	return (Renderer::command(argc, argv));
}

extern void jpeg_luma_qt(int,int[]);
extern void jpeg_chroma_qt(int,int[]);

void XILWindowAssistor::reset(int type,int q, int w, int h)
{
        int		qt[64];
	unsigned int		i,j;

	//fprintf(stderr,"XILWindowAssistor::"__FUNCTION__"\n");
	Module::size(w, h);

	inq_ = q;
	sentfirstheader=0;
	lastjpegtype_ = type;
	switch (type) {
	case 1: /* 420 */
		decimation_ = 420;
		break;
	default:
		decimation_ = 422;
		break;
	}
	for (i=0;i<sizeof(jfif_header1_);i++) {
		if (jfif_header1[i]!=0xff)
			continue;
		i++;if (i==sizeof(jfif_header1)) break;
		switch (jfif_header1[i]) {
		case M_DQT:
			//well the size is correct. we have defined the table
			i+=3;
			switch (jfif_header1[i]) {
			case 0:/* LUMA */
				i++;
				jpeg_luma_qt(q,qt);
				for (j=0;j<64;j++)
					jfif_header1[i+j]=qt[mapkoord2jfifentry[j]];
				break;
			case 1:/* CHROMA */
				i++;
				jpeg_chroma_qt(q,qt);
				for (j=0;j<64;j++)
					jfif_header1[i+j]=qt[mapkoord2jfifentry[j]];
				break;
			}
			break;
		case M_SOF0:
			i+=3;
			i++;	/* sample precision */
			jfif_header1[i++] = h>>8;
			jfif_header1[i++] = h&0xff;
			jfif_header1[i++] = w>>8;
			jfif_header1[i++] = w&0xff;
			i++; 	/* components (3) */
				i++;	/* comp id 0 */
				if (decimation_ == 420)
					jfif_header1[i++] = 0x22;/* 0x22 for 2 hor/2 vert */
				else
					jfif_header1[i++] = 0x21;/* 0x21 for 2 hor/1 vert */
				i++;	/* quant dest */

				i++;	/* comp id 1 */
				i++;	/* hor/ver quant 1:1 */
				i++;	/* quant dest */

				i++;	/* comp id 2 */
				i++;	/* hor/ver quant 1:1 */
				i++;	/* quant dest */
			break;
		default:
			//fprintf(stderr,"blocktype %02x\n",jfif_header1[i]);
			break;
		}
	}
	for (i=0;i<sizeof(jfif_header2);i++) {
		if (jfif_header2[i]!=0xff)
			continue;
		i++;if (i==sizeof(jfif_header2)) break;
		switch (jfif_header2[i]) {
		case M_SOF0:
			i+=3;
			i++;	/* sample precision */
			jfif_header2[i++] = h>>8;
			jfif_header2[i++] = h&0xff;
			jfif_header2[i++] = w>>8;
			jfif_header2[i++] = w&0xff;
			i++; 	/* components (3) */
				i++;	/* comp id 0 */
				if (decimation_ == 420)
					jfif_header2[i++] = 0x22;/* 0x22 for 2 hor/2 vert */
				else
					jfif_header2[i++] = 0x21;/* 0x21 for 2 hor/1 vert */
				i++;	/* quant dest */

				i++;	/* comp id 1 */
				i++;	/* hor/ver quant 1:1 */
				i++;	/* quant dest */

				i++;	/* comp id 2 */
				i++;	/* hor/ver quant 1:1 */
				i++;	/* quant dest */
			break;
		default:
			break;
		}
	}
	/*
	 * Make sure all outstanding putimage's finish.
	 * Syncing with the first window, syncs the server,
	 * and hence all of them.
	 */
	window_->sync();
}

XilLookup XILWindowAssistor::create_cmap(
    Colormap *cmap, int type, XilIndexList *ilist, 
    XilLookup yuvtorgb, XilLookup colorcube
) {
    unsigned long tmp_colors[256], pixels[256], mask;
    XColor	cdefs[256];
    int		top_colors = 2;	/* colormap indices 255 and 254 */
    int		i, t;
    Xil_unsigned8 *data;
    XilLookup	lut;
    int		cmapsize;
    Display	*display = Tk_Display(window_->tkwin());
    Window	window = Tk_WindowId(window_->tkwin());
    int		screen = Tk_ScreenNumber(window_->tkwin());

    //fprintf(stderr,"XILWindowAssistor::"__FUNCTION__"\n");

    /* Get colormap size */
    switch(type)
    {
    case FT_JPEG:
    case FT_H261:
        cmapsize = xil_lookup_get_num_entries(yuvtorgb);
        break;

    case FT_CELLB:
    default: /* hmm */
        xil_cis_get_attribute(cis_,"DECOMPRESSOR_MAX_CMAP_SIZE", (void**)&cmapsize);
        break;
    } 

    /* Create an X colormap for the cis.*/
    *cmap = XCreateColormap(display, window, DefaultVisual(display,screen), AllocNone);

    /* Allocate X Colormap cells
     *
     * If we do not need the entire colormap, allocate `cmapsize'
     * entries just below the top of the colormap.  Here's how:
     *    Temporarily allocate some entries at the front of the cmap.  
     *    Don't allocate the top_colors (the top two color indices 
     *	  are often used by other applications). 
     *    Allocate the needed entries in the next cmap section
     *    Free the temporary entries.
     * This allows the X-Window manager to use them and reduces the
     * chances of of your window colormap flashing.
     */

    if (cmapsize < 256-top_colors) {
        if (!XAllocColorCells(display, *cmap, 0, &mask, 0,
	        tmp_colors, 256 - cmapsize - top_colors)) {
            fprintf(stderr, " XAllocColorCells for cmap_create failed(1)\n");
        }
    }
  
    if (!XAllocColorCells(display, *cmap, 0, &mask, 0, pixels, cmapsize)) {
        fprintf(stderr, " XAllocColorCells for cmap_create failed(2)\n");
    }
    /* The remaining code assumes that the values returned in pixels[0] through
     * pixels[cmapsize-1] are a contiguous range.
     */

    /* Free the unused colors in the front */
    if (cmapsize < 256-top_colors)
        XFreeColors(display, *cmap, tmp_colors, 256 - cmapsize - top_colors, 0);

#if 0
    if (type == CELLB) {

        /* Initialize the XilIndexList to use when setting the RDWR_INDICES
         * attribute.  In this example, we make all of the indices writable.
         */
        if ( (ilist->pixels = (Xil_unsigned32 *)
                malloc(sizeof(Xil_unsigned32) * cmapsize) ) == NULL ) {
            fprintf(stderr, " out of memory for ilist->pixels create\n");
        }
        ilist->ncolors = cmapsize;

        /* Copy the color cells returned by XAllocColorCells into the ilist */
        for (i = 0; i < cmapsize; i++)
            ilist->pixels[i] = (Xil_unsigned32) pixels[i];
    }
#else 
UNUSED(ilist);
#endif

    /* Allocate memory to hold colormap data. */
    if ( (data = (Xil_unsigned8 *)
            malloc(sizeof(Xil_unsigned8) * cmapsize * 3) ) == NULL ) {
        fprintf(stderr, "xilcis_color: out of memory for cmap data create\n");
    }  

    /* Get the entries for the colormap. The method depends on the compression
     * type.  For CELL, get the entries in the current default colormap.
     * For JPEG, get the entries from the standard lookup table
     * yuv_to_rgb.
     */
    switch(type_) {
#if 0
/*    case FT_CELLB:*/

        /* Get the current values in the colormap */
        for (i = 0; i < cmapsize; i++)
            cdefs[i].pixel = i + pixels[0];

        XQueryColors(display, DefaultColormap(display, screen), cdefs,
                     cmapsize);

        /* Convert the values read from the colormap to an array that can
         * be read by xil_lookup_create.  Note that the colormap values are
         * are stored in the XilLookup in BGR order.
         */
        for (i = 0, j = 0; i < cmapsize; i++, j += 3) {
            data[j] = cdefs[i].blue >> 8;
            data[j + 1] = cdefs[i].green >> 8;
            data[j + 2] = cdefs[i].red >> 8;
        }

        lut = xil_lookup_create(xil_, XIL_BYTE, XIL_BYTE, 3, cmapsize,
				(int)pixels[0], data);
        break;
#endif
    case FT_JPEG:
    case FT_H261:
    case FT_CELLB:
        xil_lookup_get_values(yuvtorgb, xil_lookup_get_offset(yuvtorgb),
                              cmapsize, data);
        xil_lookup_set_offset(colorcube, (unsigned int)pixels[0]);
        for (i = 0, t = 0; i < cmapsize; i++, t += 3) {
            cdefs[i].pixel = pixels[i];
            cdefs[i].flags = DoRed | DoGreen | DoBlue;
            cdefs[i].blue = data[t] << 8;  
            cdefs[i].green = data[t+1] << 8; 
            cdefs[i].red = data[t+2] << 8;  
        }
        XStoreColors(display, *cmap, cdefs, cmapsize);
        lut = yuvtorgb;
        break;
     }

    Tk_SetWindowColormap(window_->tkwin(),*cmap);
    free(data);
    return(lut);
}

void XILWindowAssistor::cell_install_cmap(
	Colormap x_cmap, XilLookup cmap, XilIndexList *ilist
) {
    int			i, j, t;
    XColor		cdefs[256];
    unsigned char	cmap_data[3 * 256];
    unsigned short	cmapsize;
    int			cmapoffset;
    XilColorList	clist;
 
    //fprintf(stderr,"XILWindowAssistor::"__FUNCTION__"\n");
    cmapsize = xil_lookup_get_num_entries(cmap);
    cmapoffset = xil_lookup_get_offset(cmap);
    xil_lookup_get_values(cmap, cmapoffset, cmapsize, cmap_data);

    for (i = 0, j = 0; i < (int) ilist->ncolors; i++, j += 3) {
        t = (ilist->pixels[i] - cmapoffset) * 3;

        cdefs[i].pixel = ilist->pixels[i];
        cdefs[i].flags = DoRed | DoGreen | DoBlue;
        cdefs[i].blue = cmap_data[t] << 8;
        cdefs[i].green = cmap_data[t + 1] << 8;
        cdefs[i].red = cmap_data[t + 2] << 8;
    }

    clist.ncolors = ilist->ncolors; 
    clist.colors  = cdefs; 
    clist.cmap    = x_cmap; 

    /* Setting XCOLORMAP accomplishes the same thing as: 
     *   XStoreColors(display, x_cmap, cdefs, ilist->ncolors);
     *   XFlush( display );
     * but goes through DGA (Direct Graphics Access). This is 
     * much faster.
     */
    xil_set_device_attribute(displayimage_, "XCOLORMAP", &clist );
}


/***************************************************************
 *  XILOutputAssistor                                          *
 *                                                             *
 ***************************************************************/

XILOutputAssistor::XILOutputAssistor(int type,const char*fn,struct deviceattributes *devattr) : Renderer(type) {
	char	*cisname;
	XilDevice	device;

	filename_ = strdup(fn);
	memcpy(&deviceattributes_,devattr,sizeof(deviceattributes_));
	cisname=NULL;
	switch (type) {
	case FT_JPEG|FT_HW:
		cisname = "Jpeg";
		memcpy(jfif_header1,jfif_header1_,sizeof(jfif_header1_));
		memcpy(jfif_header2,jfif_header2_,sizeof(jfif_header2_));
		break;
	case FT_H261:
		cisname = "H261";
		break;
	case FT_CELLB:
		cisname = "CellB";
		break;
	default:
		fprintf(stderr,"	unknown cis %d???\n",type&~FT_HW);
		break;
	}
	type_ = type;
	imageYCC = NULL;
	xil_ = xil_open();
	if (xil_ == 0) {
		fprintf(stderr,"couldn't open xil.\n");
		return;//FIXME
	}
	device = xil_device_create(xil_,deviceattributes_.name);
	if (device == 0) {
		fprintf(stderr,"couldn't open %s\n",deviceattributes_.name);
		return;
	}
	xil_device_set_value(device,"DEVICE_NAME",(void*)filename_);
	//fprintf(stderr,"VIDEO_OUT\n");
	xil_device_set_value(device,"VIDEO_OUT",(void*)TRUE);
	o1kimage = xil_create_from_device(xil_,deviceattributes_.name,device);

	int arg;
	xil_get_device_attribute(o1kimage,"VIDEO_OUT",(void**)&arg);
	//fprintf(stderr,"VIDEO_OUT-> %d\n",arg);
	cis_ = xil_cis_create(xil_ , cisname);
	if (cis_ == 0) {
		fprintf(stderr,"couldn't create CIS for %s\n",cisname);
		return;
	}
	xil_cis_set_keep_frames(cis_, 3);
	xil_cis_set_max_frames(cis_, 3);

	//fprintf(stderr,"HW_DECOMPRESS_CIS\n");
	// use _THIS_ cis_ for HW decompression
	xil_set_attribute(o1kimage,"HW_DECOMPRESS_CIS",cis_);
}

XILOutputAssistor::~XILOutputAssistor() {
	xil_set_attribute(o1kimage,"HW_DECOMPRESS_CIS",NULL);
	if (o1kimage) xil_destroy(o1kimage);
	if (cis_) xil_cis_destroy(cis_);
	/*
	if (xil_) {xil_close(xil_);xil_=0;}
	*/
}

void XILOutputAssistor::setq(int q)
{
        int	qt[8][8];
	XilJpegQTable	xq;

	inq_ = q;
	sentfirstheader=0;

	jpeg_luma_qt(q, (int*)qt);
	xq.table = 0;
	xq.value = qt;
	(void)xil_cis_set_attribute(cis_, "QUANTIZATION_TABLE", (void*)&xq);
	jpeg_chroma_qt(q, (int*)qt);
	xq.table = 1;
	xq.value = qt;
	(void)xil_cis_set_attribute(cis_, "QUANTIZATION_TABLE", (void*)&xq);
}
 
void XILOutputAssistor::reset(int inq, int w, int h)
{
	//fprintf(stderr,"XILOutputAssistor(%s)::"__FUNCTION__"\n",filename_);
	Module::size(w, h);
	setq(inq);
}

int XILOutputAssistor::command(int argc, const char*const* argv) {
/*	fprintf(stderr,"XILOutputAsssitor(%s)::"__FUNCTION__"(",filename_);
	for (int i=1;i<argc;i++)
		fprintf(stderr,"%s,",argv[i]);
	fprintf(stderr,")\n");
*/
	if (argc == 3 ) {
		if (	strcmp(argv[1],"scale") == 0) {
			/* ignore !?! */
			return (TCL_OK);
		}
	}
	return Renderer::command(argc,argv);
}

int XILOutputAssistor::consume(const VideoFrame* vf) {
	int	len=0,dofree=0;
	u_char	*bp,*p=NULL;

	//fprintf(stderr,"XILOutputAssistor(%s)::"__FUNCTION__"\n",filename_);
	switch (type_) {
	case FT_JPEG|FT_HW: {
		JpegFrame* jf = (JpegFrame*)vf;
		if (	(jf->q_ != inq_) ||
			(jf->width_ != width_) ||
			(jf->height_ != height_)
		)
			reset(jf->q_, jf->width_, jf->height_);
		len=jf->len_;
		bp=jf->bp_;
		dofree=1;
		if (sentfirstheader) {
			p = new u_char[sizeof(jfif_header2)+len];
			memcpy(p,jfif_header2,sizeof(jfif_header2));
			memcpy(p+sizeof(jfif_header2),bp,len);
			len+=sizeof(jfif_header2);
		} else {
			p = new u_char [sizeof(jfif_header1)+len];
			memcpy(p,jfif_header1,sizeof(jfif_header1));
			memcpy(p+sizeof(jfif_header1),bp,len);
			len+=sizeof(jfif_header1);
			sentfirstheader=1;
		}
		break;
	}
	case FT_CELLB: {
		CellBFrame* hf = (CellBFrame*) vf;
		len=hf->len_;
		p=hf->bp_;
		xil_cis_set_attribute(cis_, "WIDTH", (void *)hf->width_);
		xil_cis_set_attribute(cis_, "HEIGHT", (void *)hf->height_);
		break;
	}
	case FT_H261: {
		H261Frame* hf = (H261Frame*) vf;
		len=hf->len_;
		p=hf->bp_;
		while ((p[0]==0)&&(p[1]==0)) {
			p++;len--;
		}
		break;
	}
	}
	xil_cis_put_bits(cis_,len,1/*framecount*/,p);
	if (dofree) delete[] p; //SV-XXX: Debian
	outputtype = xil_cis_get_output_type(cis_);
	xil_imagetype_get_info(outputtype, &cis_xsize, &cis_ysize,&cis_nbands, &cis_datatype);

	if (!imageYCC || lastcisw!=cis_xsize || lastcish!=cis_ysize) {
		lastcish = cis_ysize;
		lastcisw = cis_xsize;
		//fprintf(stderr,"	w=%d,h=%d\n",cis_xsize,cis_ysize);
		if (imageYCC) xil_destroy(imageYCC);
		imageYCC = xil_create(xil_,cis_xsize,cis_ysize,cis_nbands,cis_datatype);
		return 0;
	}
        xil_decompress(cis_, imageYCC);
	xil_sync(imageYCC);
	//xil_cis_reset(cis_);
	return (0);
} 

XILOutputDevice::XILOutputDevice(const char *name,const char *fn,struct deviceattributes *devattr)
	: OutputDevice(name)
{
	//fprintf(stderr,"XILOutputDevice::XILOutputDevice(%s,%s)\n",name,fn);
	nickname_ = strdup(name);
	filename_ = strdup(fn);
	memcpy(&deviceattributes_,devattr,sizeof(deviceattributes_));
}

int XILOutputDevice::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();

/*
	fprintf(stderr,"XILOutputDevice::"__FUNCTION__"(");
	for (int i=1;i<argc;i++)
		fprintf(stderr,"%s,",argv[i]);
	fprintf(stderr,")\n");
 */

	if (argc == 3 ) {
		if (!strcmp(argv[1],"assistor")) {
			if (!strcmp(argv[2],"cellb")) {
				XILOutputAssistor* p = new XILOutputAssistor(FT_CELLB,filename_,&deviceattributes_);
				tcl.result(p->name());
				return (TCL_OK);
			}
			if (!strcmp(argv[2],"h261")) {
				XILOutputAssistor* p = new XILOutputAssistor(FT_H261,filename_,&deviceattributes_);
				tcl.result(p->name());
				return (TCL_OK);
			}
			/*
			if (!strcmp(argv[2],"jpeg/420") || !strcmp(argv[2],"jpeg/422")) {
				XILOutputAssistor* p = new XILOutputAssistor(FT_JPEG|FT_HW,filename_,&deviceattributes_);
				tcl.result(p->name());
				return (TCL_OK);
			}
			*/
		}
	}
	return OutputDevice::command(argc,argv);
}
