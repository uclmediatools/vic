/*
 * Copyright (c) 1993-1994 The Regents of the University of California.
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
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/errno.h>
extern int errno;

#include "module.h"
#include "vic_tcl.h"

#include <tk.h>
#include <sys/param.h>
#include <xil/xil.h>

#include "xil.h"

XILDevices xil_devices;

XILDevices::XILDevices() {
	struct deviceattributes deviceattributes_[] = XIL_ATTRIBUTES;
	xildevices_=NULL;nrofxildevices_=0;
	int	found = 0;

#if 0
	extern void osprey_open();

	osprey_open();
#endif

	for (int i=0;i<XIL_NROFDEVICES;i++) {
		for (int j=0;j<10;j++) {
			char	buf[200],fn[200];
			struct	stat stbuf;

			sprintf(fn,deviceattributes_[i].devname,j);
			if (-1==stat(fn,&stbuf)) {
				/*fprintf(stderr,"stat %s:%s\n",fn,strerror(errno));*/
				break;
			}
			if (-1==access(fn,R_OK)) {
				fprintf(stderr,"access %s:%s\n",fn,strerror(errno));
				if (errno!=EBUSY)
					break;
			}
			/* We check that the device name from /dev
			 * points directly to ../devices/ otherwise
			 * it's an emulated device (like sunvideo emulation
			 * for slicvideo or osprey-1k and we ignore it
			 */
			{
				char link[BUFSIZ];
				if(readlink(fn, link, sizeof link) > 0) {
					if(! strstr(link, "/devices/")) {
						break; 
					}
				}
			}
			/* good, we seem to have one of those devices */
			found = 1;
			xildevices_ = (XILDevice**)realloc(xildevices_,sizeof(XILDevice*)*(++nrofxildevices_));
			sprintf(buf,"xil %s-%d",deviceattributes_[i].nickname,j+1);
			xildevices_[nrofxildevices_-1] = new XILDevice(strdup(buf),fn,&(deviceattributes_[i]));
			if (deviceattributes_[i].capabilities & XILCAP_OUTPUT) {
				new XILOutputDevice(strdup(buf),fn,&(deviceattributes_[i]));
			}
		}
	}
	if (!found)
		new XILDevice("xil (no device)","none",NULL);
}

/*
 * XXX gcc 2.5.8 needs this useless stub.
 */
XILDevice::XILDevice(
	const char* s,const char*fn,const struct deviceattributes *devattr
) : InputDevice(s)
{
//	fprintf(stderr,"XILDevice::XILDevice(fn=%s)\n",fn);
	char	*attributes;
	if (!strcmp(fn,"none")) {
		attributes_ = "disabled";
		return;
	}
	memcpy(&deviceattributes_,devattr,sizeof(*devattr));
	filename_ = strdup(fn);
	nickname_ = strdup(s);
	/*XXX ports should be queried from xil */
	attributes = (char*)malloc(2000);
	attributes[0]='\0';
	strcat(attributes,"format { 420 422 jpeg cellb ");
	if (deviceattributes_.capabilities & XILCAP_HWGRAB_H261)
		strcat(attributes,"h261 ");
	strcat(attributes,"}\n");
	strcat(attributes,"size { small large cif }\n");
	strcat(attributes,"port { ");
	if (deviceattributes_.ports & XILPORT_IN_COMPOSITE_1)
		strcat(attributes,"Composite-1 ");
	if (deviceattributes_.ports & XILPORT_IN_COMPOSITE_2)
		strcat(attributes,"Composite-2 ");
	if (deviceattributes_.ports & XILPORT_IN_S_VIDEO)
		strcat(attributes,"S-Video ");
	strcat(attributes,"}\n");

	attributes_ = (const char*)attributes;//EAT THIS!!!!
}

int XILDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
/*	fprintf(stderr,"XILDevice(%s)::command(%d,",filename_,argc);
	for (int i=1;i<argc;i++)
		fprintf(stderr,"%s,",argv[i]);
	fprintf(stderr,")\n");
 */
	if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			TclObject* o = 0;
			if (strcmp(argv[2], "422") == 0)
				o = new XILYuvGrabber(filename_,&deviceattributes_);
			else if (strcmp(argv[2], "cif") == 0)
				o = new XILCIFGrabber(filename_,&deviceattributes_);
			else if (strcmp(argv[2], "jpeg") == 0)
				o = new XILJpegGrabber(filename_,&deviceattributes_);
			else if (strcmp(argv[2], "cellb") == 0)
				o = new XILCellBGrabber(filename_,&deviceattributes_);
			else if ((strcmp(argv[2], "h261") == 0) && (deviceattributes_.capabilities & XILCAP_HWGRAB_H261))
				o = new XILp64Grabber(filename_,&deviceattributes_);
			if (o != 0)
				tcl.result(o->name());
			return (TCL_OK);
		}
	}
	return (InputDevice::command(argc, argv));
}


XILGrabber::XILGrabber(char *devname,struct deviceattributes *devattr)
{
	XilDevice device_;

//	fprintf(stderr,"%s\n",__FUNCTION__);
	memcpy(&deviceattributes_,devattr,sizeof(*devattr));
	filename_ = strdup(devname);
#ifdef OSPREY_RAT_HACK
	osprey_open();
#endif
	xil_ = xil_open();
	if (xil_ == 0) {
		status_ = -1;
		return;
	}

	device_ = xil_device_create(xil_, deviceattributes_.name);
	xil_device_set_value(device_,"DEVICE_NAME",(void*)filename_);

	source_ = xil_create_from_device(xil_, deviceattributes_.name, device_);
	xil_device_destroy(device_);
	if (source_ == 0) {
		xil_close(xil_);xil_ = 0;
		status_ = -1;
		return;
	}


	Tcl &tcl = Tcl::instance();
	char	buf[200];

	tcl.evalf("proc build.\"%s\" w { build.xil $w \"%s\"}\n",
		deviceattributes_.name,
		deviceattributes_.name
	);

	update_grabber_panel();

	vformat_ = attr("FORMAT_V");
	sprintf(buf,"%d",vformat_);		\
	Tcl_SetVar(tcl.interp(),"vformat",buf,TCL_GLOBAL_ONLY);\

	attr("H261_PIP", 1);
	attr("PORT_V", 1);
	attr("FLUSH_BUFFERS", 0);
	attr("IMAGE_SKIP", 0);
	attr("MAX_BUFFERS", 1);

	xil_get_info(source_, &basewidth_, &baseheight_, &nbands_, &datatype_);

	image_ = xil_create(xil_, basewidth_ >> 1, baseheight_ >> 1,
			    nbands_, datatype_);
	decimate_ = 2;
	scale_ = .5;
	frame_ = 0;

	fd_ = attr("FILE_DESCRIPTOR");
//	fd_ = 0;
	max_fps_ = vformat_==1 ?25:30;
}

XILGrabber::~XILGrabber()
{
	Tcl &tcl = Tcl::instance();
//	fprintf(stderr,"XILGrabber::~XILGrabber\n");
	if (status_ >= 0) {
	    if (image_)
	    	xil_destroy(image_);
	    image_ = NULL;
	    if (source_)
		xil_destroy(source_);
	    source_ = NULL;
	    if (xil_)
	      xil_close(xil_);        
	    xil_ = NULL;
//      fprintf(stderr,"XILGrabber::~XILGrabber:xil_close\n");
	}
	tcl.evalf("HACK_detach_xil");
}

void XILGrabber::update_grabber_panel() {
	Tcl &tcl = Tcl::instance();
	char	buf[200];

	
	return ;
#define CHECKATTR(attrname,varname) 					\
	{								\
		static int last##attrname = -1;				\
		int	attrval;					\
		if (	(deviceattributes_.capabilities & XILCAP_ATTR_##attrname) &&\
			attrval!=last##attrname) {			\
			sprintf(buf,"%d",(int)attr(#attrname));		\
			Tcl_SetVar(tcl.interp(),#varname,buf,TCL_GLOBAL_ONLY);\
		}							\
	}
#define CHECKATTR2(attrname,varname) 					\
	{								\
		static int last##attrname = -1;				\
		int	attrval;					\
		if (	(deviceattributes_.capabilities & XILCAP_ATTR_##attrname) &&\
			attrval!=last##attrname				\
		) {							\
			sprintf(buf,"%d",(int)attr(#attrname)-256);	\
			Tcl_SetVar(tcl.interp(),#varname,buf,TCL_GLOBAL_ONLY);\
		}							\
	}

	CHECKATTR(HUE,xilHue);
	CHECKATTR(BRIGHTNESS,xilBrightness);
	CHECKATTR2(CONTRAST,xilContrast);
	CHECKATTR2(CHROMA_GAIN_U,xilChromaGainU);
	CHECKATTR2(CHROMA_GAIN_V,xilChromaGainV);

	CHECKATTR(LUMA_BRIGHTNESS,xilLumaBrightness);
	CHECKATTR(LUMA_CONTRAST,xilLumaContrast);
	CHECKATTR(CHROMA_GAIN,xilChromaGain);
	CHECKATTR(CHROMA_SATURATION,xilChromaSaturation);
}

int XILGrabber::attr(const char* name, int value)
{
	int s = xil_set_device_attribute(source_, (char*)name, (void*)value);
	if (s != XIL_SUCCESS) {
		fprintf(stderr, "vic: can't set XIL attribute: %s\n", name);
		return (-1);
	}
	return (0);
}

int XILGrabber::attr(const char* name)
{
	int value;
	int s = xil_get_device_attribute(source_, (char*)name, (void**)&value);
	if (s != XIL_SUCCESS) {
		fprintf(stderr, "vic: can't get XIL attribute: %s\n", name);
		return (-1);
	}
	return (value);
}

const char* XILGrabber::cattr(const char* name)
{
	const char* value;
	int s = xil_get_device_attribute(source_, (char*)name, (void**)&value);
	if (s != XIL_SUCCESS) {
		fprintf(stderr, "vic: can't get XIL attribute: %s\n", name);
		return (0);
	}
	return (value);
}

int XILGrabber::command(int argc, const char*const* argv)
{
/*	fprintf(stderr,"XILGrabber(%s)::command(%d,",filename_,argc);
	for (int i=1;i<argc;i++)
		fprintf(stderr,"%s,",argv[i]);
	fprintf(stderr,")\n");
*/
	if (argc == 3) {
		if (strcmp(argv[1], "port") == 0) {
			int newport;
			if (strcasecmp(argv[2], "composite-1") == 0)
				newport = 1;
			else if (strcasecmp(argv[2], "composite-2") == 0)
				newport = 2;
			else 
				newport = 0;

			attr("PORT_V", newport);
			/* video format may have changed when port changed */
			xil_get_info(source_, &basewidth_, &baseheight_,
				     &nbands_, &datatype_);
			setsize();
			/* need to kick-start frame grabbing process */
			if (running_) {
				stop();
				start();
			}
			return (TCL_OK);
		}
		if (strcmp(argv[1], "decimate") == 0) {
			int d = atoi(argv[2]);
			if (d <= 0) {
				Tcl& tcl = Tcl::instance();
				tcl.resultf("%s: divide by zero", argv[0]);
				return (TCL_ERROR);
			}
			scale_ = 1. / double(d);
			decimate_ = d;
			setsize();
			return (TCL_OK);
		}
		if (strcmp(argv[1], "q") == 0) {
			// just ignore it
			return (TCL_OK);
		}
		if (strcmp(argv[1],"get")==0) {
			if (strcmp(argv[1], "vformat") == 0) {	
				Tcl& tcl = Tcl::instance();
//				fprintf (stderr,"%s: vformat: %d",__FUNCTION__,vformat_);
				switch (vformat_) {
					case 1:	tcl.resultf("%s","pal");
						break;

					case 2: tcl.resultf("%s", "ntsc");
                                       		break;

					case 0:	tcl.resultf("%s", "auto");
				}
				return (TCL_OK);	
			}
		}
	}
	if (argc==4) {
		if (strcmp(argv[1],"set")==0) {
			if (strcmp(argv[2],"HUE")==0) {
				attr("HUE",atoi(argv[3]));
				return (TCL_OK);
			} else if (strcmp(argv[2],"SATURATION")==0) {
				attr("CHROMA_GAIN_U",atoi(argv[3])+256);
				attr("CHROMA_GAIN_V",atoi(argv[3])+256);
				return (TCL_OK);
			} else if (strcmp(argv[2],"CHROMA_GAIN_U")==0) {
				attr("CHROMA_GAIN_U",atoi(argv[3])+256);
				return (TCL_OK);
			} else if (strcmp(argv[2],"CHROMA_GAIN")==0) {
				attr("CHROMA_GAIN",atoi(argv[3]));
				return (TCL_OK);
			} else if (strcmp(argv[2],"CHROMA_SATURATION")==0) {
				attr("CHROMA_SATURATION",atoi(argv[3]));
				return (TCL_OK);
			} else if (strcmp(argv[2],"CHROMA_GAIN_V")==0) {
				attr("CHROMA_GAIN_V",atoi(argv[3])+256);
				return (TCL_OK);
			} else if (strcmp(argv[2],"CONTRAST")==0) {
				attr("CONTRAST",atoi(argv[3])+256);
				return (TCL_OK);
			} else if (strcmp(argv[2],"LUMA_BRIGHTNESS")==0) {
				attr("LUMA_BRIGHTNESS",atoi(argv[3]));
				return (TCL_OK);
			} else if (strcmp(argv[2],"LUMA_CONTRAST")==0) {
				attr("LUMA_CONTRAST",atoi(argv[3]));
				return (TCL_OK);
			} else if (strcmp(argv[2],"BRIGHTNESS")==0) {
				attr("BRIGHTNESS",atoi(argv[3]));
				return (TCL_OK);
			} else if (strcmp(argv[2], "vformat") == 0) {

				if (strcasecmp(argv[3], "pal") == 0)
					vformat_ = 1;
				else if (strcasecmp(argv[3], "ntsc") == 0)
					vformat_ = 2;
				else if (strcasecmp(argv[3], "auto") == 0)
					vformat_ = 0;
				if (running_) {
					stop();
				}
				attr("FORMAT_V", vformat_);
				/* video format may have changed when port changed */
				xil_get_info(source_, &basewidth_, &baseheight_,
					     &nbands_, &datatype_);
				setsize();
				max_fps_ = vformat_==1 ?25:30;
				/* need to kick-start frame grabbing process */
				if (running_) {
					start();
				}
				return (TCL_OK);
			}
		} 
				
	}
	return (Grabber::command(argc, argv));
}

void XILGrabber::fps(int f)
{
	if (f <= 0)
		f = 1;
	else if (f > max_fps_)
		f = max_fps_;
	/* convert to skip count then back */
	int sc = max_fps_ / f;
	attr("IMAGE_SKIP", sc - 1);
	Grabber::fps(max_fps_ / sc);
}

void XILGrabber::start()
{
	Grabber::start();
//	return ;

	//link(fd_, TK_READABLE);
	double now = gettimeofday_usecs();
	frameclock_ = now;
	nextframetime_ = now + tick(grab());
}

void XILGrabber::stop()
{
	Grabber::stop();
	unlink();
}

void XILGrabber::dispatch(int mask)
{
	UNUSED(mask);
	double now = gettimeofday_usecs();
	if (nextframetime_ > now) {
#if 0
		if (fd_) {
			/*
			 * the frame is too early & we want to flush it.
			 * unfortunately, the sunvideo driver doesn't provide
			 * a hook for flushing a frame.  So, we have to do
			 * a 2MB read to get rid of the sucker.
			 */
			char* buf = new char[768 * 576 * 3];
			(void)pread(fd_, (void*)buf, sizeof(buf), (off_t)0x01000000);
			delete[] buf; //SV-XXX: Debian
		}
		/* else do nothing */
#endif
	} else
		nextframetime_ = tick(grab()) + now;
}

/*
 * define these for REPLENISH macro used below
 */
#define DIFF4(in, frm, v) \
	v += (in)[0] - (frm)[0]; \
	v += (in)[3] - (frm)[1]; \
	v += (in)[6] - (frm)[2]; \
	v += (in)[9] - (frm)[3];

#define DIFFLINE(in, frm, left, center, right) \
	DIFF4(in, frm, left); \
	DIFF4(in + 1*12, frm + 1*4, center); \
	DIFF4(in + 2*12, frm + 2*4, center); \
	DIFF4(in + 3*12, frm + 3*4, right); \
	if (right < 0) \
		right = -right; \
	if (left < 0) \
		left = -left; \
	if (center < 0) \
		center = -center;

void XILGrabber::suppress(const u_char* devbuf, int is)
{
	const u_char* start = frame_ + 16 * vstart_ * outw_ + 16 * hstart_;
	REPLENISH(devbuf, start, is, 3,
		  hstart_, hstop_, vstart_, vstop_);
}

XILYuvGrabber::XILYuvGrabber(char *devname,struct deviceattributes *devattr)
	:XILGrabber(devname,devattr)
{
	setsize();
}

XILYuvGrabber::~XILYuvGrabber()
{
	xil_destroy(image_);image_=0;
	delete[] frame_; //SV-XXX: Debian
}

void XILYuvGrabber::setsize()
{
	xil_destroy(image_);
	set_size_422(basewidth_ / decimate_, baseheight_ / decimate_);
	image_ = xil_create(xil_, basewidth_/decimate_, baseheight_/decimate_, nbands_, datatype_);
}

inline void 
XILYuvGrabber::saveblk(const u_char* in,
	u_char* yp, u_char* up, u_char* vp, int stride, int istride)
{
	for (int i = 16; --i >= 0; ) {
		/*
		 * Each iteration of this loop grabs 16 Ys & 8 U/Vs.
		 */
		register u_int y0, y1, u, v;

		u  = in[1]  << 24 | in[7]  << 16 | in[13] << 8 | in[19];
		v  = in[2]  << 24 | in[8] << 16  | in[14] << 8 | in[20];
		y0 = in[0]  << 24 | in[3]  << 16 | in[6]  << 8 | in[9];
		y1 = in[12] << 24 | in[15] << 16 | in[18] << 8 | in[21];

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;
		*(u_int*)up = u;
		*(u_int*)vp = v;

		u  = in[24+1]  << 24 | in[24+7]  << 16 | in[24+13] << 8 | in[24+19];
		v  = in[24+2]  << 24 | in[24+8] << 16  | in[24+14] << 8 | in[24+20];
		y0 = in[24+0]  << 24 | in[24+3]  << 16 | in[24+6]  << 8 | in[24+9];
		y1 = in[24+12] << 24 | in[24+15] << 16 | in[24+18] << 8 | in[24+21];

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;
		((u_int*)up)[1] = u;
		((u_int*)vp)[1] = v;

		in += istride;
		yp += stride;
		up += stride >> 1;
		vp += stride >> 1;
	}
}

void XILYuvGrabber::saveblks(const u_char* in, int is)
{
	u_char* crv = crvec_;
	int off = framesize_;
	u_char* lum = frame_;
	u_char* chm = lum + off;
	off >>= 1;
	int stride = 15 * outw_;
	int istride = is * 15;
	for (int y = 0; y < blkh_; ++y) {
		for (int x = 0; x < blkw_; ++x) {
			int s = *crv++;
			if ((s & CR_SEND) != 0)
				saveblk(in, lum, chm, chm + off, outw_, is);

			in += 48;
			lum += 16;
			chm += 8;
		}
		lum += stride;
		chm += stride >> 1;
		in += istride;
	}
}

int XILYuvGrabber::grab()
{
	/*XXX*/
	xil_scale(source_, image_, "nearest", scale_, scale_);
	if (xil_export(image_) != XIL_SUCCESS) {
		fprintf(stderr, "vic: xil_export failed\n");
		abort();
	}
	XilMemoryStorage layout;
	unsigned int xw,xh,xb;
	XilDataType xd;
	xil_get_info(image_, &xw, &xh, &xb, &xd);
	if (xil_get_memory_storage(image_, &layout) == 0) {
		fprintf(stderr, "vic: xil_get_memory_storage failed\n");
		abort();
	}
	int istride = layout.byte.scanline_stride;
	if (layout.byte.pixel_stride != 3) {
		fprintf(stderr, "vic: xil: bad pixel stride (%d)\n",layout.byte.pixel_stride);
		abort();
	}
	register u_char* data = layout.byte.data;
	suppress(data, istride);
	saveblks(data, istride);
	xil_import(image_, 0);
	xil_toss(image_);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

XILCIFGrabber::XILCIFGrabber(char *devname,struct deviceattributes *devattr)
	:XILYuvGrabber(devname,devattr)
{
	setsize();
}

void XILCIFGrabber::setsize()
{
	xil_destroy(image_);
	set_size_cif(basewidth_ / decimate_, baseheight_ / decimate_);
	image_ = xil_create(xil_, basewidth_/decimate_, baseheight_/decimate_, nbands_, datatype_);
}

/* 420 */
inline void 
XILCIFGrabber::saveblk(const u_char* in,
	u_char* yp, u_char* up, u_char* vp, int stride, int istride)
{
	for (int i = 8; --i >= 0; ) {
		/*
		 * Each iteration of this loop grabs 16 Ys & 8 U/Vs.
		 */
		register u_int y0, y1, u, v;

		u  = in[1]  << 24 | in[7]  << 16 | in[13] << 8 | in[19];
		v  = in[2]  << 24 | in[8] << 16  | in[14] << 8 | in[20];
		y0 = in[0]  << 24 | in[3]  << 16 | in[6]  << 8 | in[9];
		y1 = in[12] << 24 | in[15] << 16 | in[18] << 8 | in[21];

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;
		*(u_int*)up = u;
		*(u_int*)vp = v;

		u  = in[24+1]  << 24 | in[24+7]  << 16 | in[24+13] << 8 | in[24+19];
		v  = in[24+2]  << 24 | in[24+8] << 16  | in[24+14] << 8 | in[24+20];
		y0 = in[24+0]  << 24 | in[24+3]  << 16 | in[24+6]  << 8 | in[24+9];
		y1 = in[24+12] << 24 | in[24+15] << 16 | in[24+18] << 8 | in[24+21];

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;
		((u_int*)up)[1] = u;
		((u_int*)vp)[1] = v;

		in += istride;
		yp += stride;
		up += stride >> 1;
		vp += stride >> 1;

		/* do the 2nd (y only instead of yuv) line */
		y0 = in[0]  << 24 | in[3]  << 16 | in[6]  << 8 | in[9];
		y1 = in[12] << 24 | in[15] << 16 | in[18] << 8 | in[21];

		((u_int*)yp)[0] = y0;
		((u_int*)yp)[1] = y1;

		y0 = in[24+0]  << 24 | in[24+3]  << 16 | in[24+6]  << 8 | in[24+9];
		y1 = in[24+12] << 24 | in[24+15] << 16 | in[24+18] << 8 | in[24+21];

		((u_int*)yp)[2] = y0;
		((u_int*)yp)[3] = y1;

		in += istride;
		yp += stride;
	}
}

void XILCIFGrabber::saveblks(const u_char* in, int is)
{
	u_char* crv = crvec_;
	int off = framesize_;
	u_char* lum = frame_;
	u_char* chm = lum + off;
	off >>= 2;

	crv += vstart_ * blkw_ + hstart_;
	lum += vstart_ * outw_ * 16 + hstart_ * 16;
	chm += vstart_ * (outw_ >> 1) * 8 + hstart_ * 8;

	int skip = hstart_ + (blkw_ - hstop_);

	for (int y = vstart_; y < vstop_; ++y) {
		const u_char* nin = in;
		for (int x = hstart_; x < hstop_; ++x) {
			int s = *crv++;
			if ((s & CR_SEND) != 0)
				saveblk(in, lum, chm, chm + off, outw_, is);

			in += 48;
			lum += 16;
			chm += 8;
		}
		crv += skip;
		lum += 15 * outw_ + skip * 16;
		chm += 7 * (outw_ >> 1) + skip * 8;
		in = nin + 16 * is;
	}
}

int XILCIFGrabber::grab()
{
	/*
	 * This is similar to XILGrabber:grab() except it converts
	 * an PAL image to CIF dimensions (by dropping the last
	 * 32 pixels of each line) and it decimates the
	 * uv info by 2 vertically to convert 4:2:2 to 4:1:1.
	 */
	xil_scale(source_, image_, "nearest", scale_, scale_);
	if (xil_export(image_) != XIL_SUCCESS) {
		fprintf(stderr, "vic: xil_export failed\n");
		abort();
	}
	XilMemoryStorage layout;
	if (xil_get_memory_storage(image_, &layout) == 0) {
		fprintf(stderr, "vic: xil_get_memory_storage failed\n");
		abort();
	}
	register u_int istride = layout.byte.scanline_stride;
	register u_char* p = layout.byte.data;
	suppress(p, istride);
	saveblks(p, istride);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

XILCodecGrabber::XILCodecGrabber(const char* type,char *devname,struct deviceattributes *devattr)
	: XILGrabber(devname,devattr)
{
	//fprintf(stderr,"XILCodecGrabber::"__FUNCTION__"\n");
	cis_ = xil_cis_create(xil_, (char*)type);
	xil_cis_set_keep_frames(cis_, 3);
	xil_cis_set_max_frames(cis_, 3);
}

XILCodecGrabber::~XILCodecGrabber()
{
//	fprintf(stderr,"XILCodecGrabber::~XILCodecGrabber\n");
	xil_cis_sync(cis_);
	xil_destroy(image_);
	xil_cis_destroy(cis_);
	cis_ = NULL;
}

void XILCodecGrabber::update_grabber_panel() {

	XILGrabber::update_grabber_panel();
}

int XILCodecGrabber::cattr(const char* name, int value)
{
//	fprintf(stderr,__FUNCTION__"(%s,%d)\n",name,value);
	(void)xil_cis_set_attribute(cis_, (char*)name, (void*)value);
	return (0);
}

u_char* XILCodecGrabber::capture(int& length)
{
//	fprintf(stderr,__FUNCTION__":scale=%g\n",scale_);

	xil_scale(source_, image_, "nearest", scale_, scale_);
	xil_compress(image_, cis_);
	update_grabber_panel();
	xil_cis_sync(cis_);
	xil_toss(image_);
	if (!xil_cis_has_frame(cis_))
		return (0);

	int cc, nf;
	u_char* p = (u_char*)xil_cis_get_bits_ptr(cis_, &cc, &nf);
	length = cc;
	return (p);
}

XILCellBGrabber::XILCellBGrabber(char*devname,struct deviceattributes *devattr)
	: XILCodecGrabber("CellB",devname,devattr)
{
}

int XILCellBGrabber::grab()
{
	/*XXX can get timestamp from xil */
	int cc;

	u_char* p = capture(cc);

	if (!p) return cc;
	CellBFrame f(media_ts(),(short*)p,cc,basewidth_/decimate_,baseheight_/decimate_); 
	return target_->consume(&f);
}

int XILCellBGrabber::command(int argc, const char*const* argv)
{
/*	fprintf(stderr,"XILCellBGrabber(%s)::command(%d,",filename_,argc);
	for (int i=1;i<argc;i++)
		fprintf(stderr,"%s,",argv[i]);
	fprintf(stderr,")\n");
*/
	if (argc == 3) {
		if (strcmp(argv[1], "decimate") == 0) {
			int		d = atoi(argv[2]);
			u_int		nbands;
			XilDataType	datatype;
			
			if ((d!=2) && (d!=4) && (d!=1))
				return (TCL_ERROR);
			decimate_ = d;
			scale_ = 1. / double(d);
			int targetwidth_ = basewidth_/decimate_;
			int targetheight_ = baseheight_/decimate_;
			//fprintf(stderr,"new w is %d,h is %d\n",targetwidth_,targetheight_);
			xil_cis_sync(cis_);
			xil_destroy(image_);
			xil_cis_destroy(cis_);
			cis_ = xil_cis_create(xil_, "CellB");
			xil_cis_set_keep_frames(cis_, 3);
			xil_cis_set_max_frames(cis_, 3);
			xil_get_info(source_, &basewidth_, &baseheight_, &nbands, &datatype);
			//fprintf(stderr,"new grabbing cis, w=%d,h=%d\n",targetwidth_,targetheight_);
			image_ = xil_create(xil_, targetwidth_, targetheight_, nbands, datatype);
			return (TCL_OK);
		}
	}
	return (XILCodecGrabber::command(argc, argv));
}


XILJpegGrabber::XILJpegGrabber(char*devname,struct deviceattributes *devattr)
	: XILCodecGrabber("Jpeg",devname,devattr), q_(50)
{
	cattr("ENCODE_411_INTERLEAVED", 1);
}

int XILJpegGrabber::command(int argc, const char*const* argv)
{
/*	fprintf(stderr,"XILJpegGrabber(%s)::command(%d,",filename_,argc);
	for (int i=1;i<argc;i++)
		fprintf(stderr,"%s,",argv[i]);
	fprintf(stderr,")\n");
 */
	if (argc == 3) {
		if (strcmp(argv[1], "q") == 0) {
			int q = atoi(argv[2]);
			setq(q);
			return (TCL_OK);
		}
		if (!strcmp(argv[1], "decimate")) {
			int		d = atoi(argv[2]);
			u_int		nbands;
			XilDataType	datatype;
			
			if ((d!=2) && (d!=4) && (d!=1))
				return (TCL_ERROR);
			//fprintf(stderr,"new decimate is %d\n",d);
			decimate_ = d;
			scale_ = 1. / double(d);
			if (cis_) {
				xil_cis_sync(cis_);
				xil_cis_destroy(cis_);
				cis_= 0;
			}

			if (image_) xil_destroy(image_);image_=0;
			cis_ = xil_cis_create(xil_, "Jpeg");
			xil_cis_set_keep_frames(cis_, 3);
			xil_cis_set_max_frames(cis_, 3);
			cattr("ENCODE_411_INTERLEAVED", 1);
			xil_get_info(source_, &basewidth_, &baseheight_, &nbands, &datatype);
			image_ = xil_create(xil_, basewidth_/decimate_, baseheight_/decimate_, nbands, datatype);
			return (TCL_OK);
		}
	}
	return (XILGrabber::command(argc, argv));
}
 
extern void jpeg_chroma_qt(int q, int* qt);
extern void jpeg_luma_qt(int q, int* qt);

void XILJpegGrabber::setq(int q)
{
	Tcl &tcl = Tcl::instance();
	char	buf[20];

	sprintf(buf,"%d",q);
	Tcl_SetVar(tcl.interp(),"QUALITY",buf,TCL_GLOBAL_ONLY);
	/*
	 * NB - if any entry of any quantization table is < 8, XIL
	 * will refuse to use the CL4000 for compression & instead
	 * do a raw capture followed by a software jpeg conversion.
	 * This will flush performance straight down the toilet.
	 */
	q_ = q;
	int qt[8][8];
	XilJpegQTable xq;

	jpeg_luma_qt(q, (int*)qt);
	xq.table = 0;
	xq.value = qt;
	(void)xil_cis_set_attribute(cis_, "QUANTIZATION_TABLE", (void*)&xq);

	jpeg_chroma_qt(q, (int*)qt);
	xq.table = 1;
	xq.value = qt;
	(void)xil_cis_set_attribute(cis_, "QUANTIZATION_TABLE", (void*)&xq);
}

int XILJpegGrabber::grab()
{
	/*XXX can get timestamp from xil */
	int cc;
	u_char* p = capture(cc);
	if (p == 0)
		return (0);

	/* get rid of the jfif header that xil prepends to each frame */
	u_char *ep,*bp;
	bp = p;
	for (ep = p + cc; p < ep; ++p) {
		if (*p == 0xff) {
			++p;
			if  (*p == 0xda) {
				/* found start-of-scan marker */
				++p;
				if (p + 2 <= ep) {
					/* skip over SOS */
					u_int t = (p[0] << 8) | p[1];
					p += t;
					break;
				}
			}
		}
	}
#undef DEBUG_JFIF
#ifdef DEBUG_JFIF
	int wrapc = 0, injfifhdr = 1;
	for (int i = 0;i<cc;i++) {
		if (i==(p-bp)) {
			fprintf(stderr,"\n	begin of stream \n");
			injfifhdr = 0;
			wrapc=0;
		}
		if (injfifhdr && (bp[i]==0xff)) {
			fprintf(stderr,"\n");
			wrapc=0;
		}
		fprintf(stderr,"%02x ",bp[i]);
		if (((wrapc++)% 20) == 19) {
			fprintf(stderr,"\n");
			wrapc=0;
		}
	}
	fprintf(stderr,"\n	end of stream \n");
#endif

	if (p >= ep)
		return (0);
	// the 1 is the type and means 420 encoding.
	JpegFrame f(media_ts(), p, ep - p, q_, 1,
		    basewidth_ / decimate_, baseheight_ / decimate_);
	return (target_->consume(&f));
}

XILp64Grabber::XILp64Grabber(char *devname,struct deviceattributes *devattr)
	 :XILCodecGrabber("H261",devname,devattr)
{
        XilDataType datatype;
        u_int nbands;
 
	attr("H261_BIT_RATE",500);
	attr("H261_MAX_QUANT",10);

        xil_get_info(source_, &basewidth_, &baseheight_, &nbands, &datatype);
        targetwidth_ = 352*2/decimate_;
        targetheight_ = 288*2/decimate_;
        /* XXX: free old image */
        image_ = xil_create(xil_, targetwidth_, targetheight_, nbands, datatype);
	xil_cis_set_keep_frames(cis_, 3);
	xil_cis_set_max_frames(cis_, 3);

}

u_char* XILp64Grabber::capture(int& length)
{

	u_char *p = NULL;
	int cc, nf;

	update_grabber_panel();

	xil_scale(source_, image_, "nearest", 1.0*targetwidth_/basewidth_, 1.0*targetheight_/baseheight_);
	xil_compress(image_, cis_);
	xil_cis_sync(cis_);
	xil_toss(image_);
	if (xil_cis_has_frame(cis_)) {
		p = (u_char*)xil_cis_get_bits_ptr(cis_, &cc, &nf);
		//fprintf(stderr,"capture, cc=%d?\n",cc);
		length = cc;
		return p;
	}
	return NULL;
}
void XILp64Grabber::setq(int q)
{
	attr("H261_MAX_QUANT",q);
}

int XILp64Grabber::grab()
{
	/*XXX can get timestamp from xil */
	int cc;

	u_char* p = capture(cc);

	if (!p) return cc;
	H261Frame f(media_ts(),(short*)p,cc,targetwidth_,targetheight_); 
	return target_->consume(&f);
}

int XILp64Grabber::command(int argc, const char*const* argv)
{
/*	fprintf(stderr,"XILp64Grabber(%s)::command(%d,",filename_,argc);
	for (int i=1;i<argc;i++)
		fprintf(stderr,"%s,",argv[i]);
	fprintf(stderr,")\n");
*/
	if (argc == 3) {
		if (!strcmp(argv[1],"bps")) {
			int	xbps;

			//fprintf(stderr,"bitrate set to %d\n",atoi(argv[2]));
			xbps = atoi(argv[2]);
			if (xbps < 30)
				xbps = 30;
			attr("H261_BIT_RATE",xbps);
			// fall through to ::command()... 
		}
		if (strcmp(argv[1], "q") == 0) {
			int q = atoi(argv[2]);
			setq(q);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "decimate") == 0) {
			int		d = atoi(argv[2]);
			u_int		nbands;
			XilDataType	datatype;
			
			if ((d!=2) && (d!=4))
				return (TCL_ERROR);
			//fprintf(stderr,"new decimate is %d\n",d);
			decimate_ = d;
			scale_ = 1. / double(d);
			targetwidth_ = 352*2/decimate_;
			targetheight_ = 288*2/decimate_;
			//fprintf(stderr,"new w is %d,h is %d\n",targetwidth_,targetheight_);
			xil_cis_sync(cis_);
			xil_destroy(image_);
			xil_cis_destroy(cis_);
			cis_ = xil_cis_create(xil_, "H261");
			xil_cis_set_keep_frames(cis_, 3);
			xil_cis_set_max_frames(cis_, 3);
			xil_get_info(source_, &basewidth_, &baseheight_, &nbands, &datatype);
			//fprintf(stderr,"new grabbing cis, w=%d,h=%d\n",targetwidth_,targetheight_);
			image_ = xil_create(xil_, targetwidth_, targetheight_, nbands, datatype);
			return (TCL_OK);
		}
	}
	return (XILCodecGrabber::command(argc, argv));
}
