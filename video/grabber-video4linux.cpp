/* =========================================================================

     Copyright (c) 1997 Regents of Koji OKAMURA, oka@kobe-u.ac.jp
     All rights reserved.

     largely rewritten for new bttv/video4linux interface
     by Gerd Knorr <kraxel@cs.tu-berlin.de>

     Added brightness, contrast, hue and saturation controls.
     by Jean-Marc Orliaguet <jmo@medialab.chalmers.se>

     Added support for various YUV byte orders.
     by Jean-Marc Orliaguet <jmo@medialab.chalmers.se>

     Added support for NTSC/PAL/SECAM norm selection. (14/10/99)
     by Jean-Marc Orliaguet <jmo@medialab.chalmers.se>

   ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/fcntl.h>  
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>


extern "C" {
#include <asm/types.h>
#include <linux/videodev.h>
}

#include "grabber.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "module.h"

/* here you can tune the device names */
static const char *devlist[] = {
    "/dev/video0", "/dev/video1", "/dev/video2", "/dev/video3",
    NULL
};

#define DEBUG(x)
//#define DEBUG(x) (x)

#define NTSC_WIDTH  640
#define NTSC_HEIGHT 480
#define PAL_WIDTH   768
#define PAL_HEIGHT  576
#define CIF_WIDTH   352
#define CIF_HEIGHT  288

/* pass 0/1 by reference */
static const int  one = 1, zero = 0;

#define CF_422 0
#define CF_411 1
#define CF_CIF 2

/* YUV Byte order */
#define BYTE_ORDER_YUYV 0
#define BYTE_ORDER_YVYU 1
#define BYTE_ORDER_UYVY 2
#define BYTE_ORDER_VYUY 3


class V4lGrabber : public Grabber {
public:
    V4lGrabber(const char * cformat, const char *dev);
    virtual ~V4lGrabber();

    virtual int  command(int argc, const char*const* argv);
    virtual void start();
    virtual void stop();
    virtual int  grab();


protected:
    void format();
    void setsize();

    void packed422_to_planar422(char *, char*);
    void packed422_to_planar411(char *, char*);

    struct video_capability  capability;
    struct video_channel     *channels;
    struct video_picture     pict;
    struct video_window      win;

    /* mmap */
    int                      have_mmap;
    int                      grab_count;
    int                      sync_count;
    struct video_mmap        gb_even;
    struct video_mmap        gb_odd;
    struct video_mbuf        gb_buffers;
    char                     *mem;

    int fd_;
    int format_;
    int have_422P;
    int v4lformat_;
    int byteorder_;
    int cformat_;
    int port_;
    int norm_;
    
    unsigned char *tm_;
    int width_;
    int height_;
    int max_width_;
    int max_height_;
    int decimate_;
};

/* ----------------------------------------------------------------- */

class V4lDevice : public InputDevice {
public:
    V4lDevice(const char *dev, const char*, char *attr);
    virtual int command(int argc, const char*const* argv);

private:
    const char *dev_;
};


V4lDevice::V4lDevice(const char *dev, const char *name, char *attr) : InputDevice(name)
{
    dev_ = dev;
    attributes_ = attr;
    fprintf(stderr,"V4l:  ==> %s\n",attr);
}

int V4lDevice::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();


    if (argc == 3) {
	if (strcmp(argv[1], "open") == 0) {
	    TclObject* o = 0;
	    o = new V4lGrabber(argv[2],dev_);
	    if (o != 0)
		tcl.result(o->name());
	    return (TCL_OK);
	}


    }
    return (InputDevice::command(argc, argv));
}

/* ----------------------------------------------------------------- */

class V4lScanner {
public:
    V4lScanner(const char **dev);
};

static V4lScanner find_video4linux_devices(devlist);

V4lScanner::V4lScanner(const char **dev)
{
    static const char *palette_name[] = {
	"", "grey", "hi240", "rgb16", "rgb24", "rgb32", "rgb15", 
	"yuv422", "yuyv422", "uyvy422", "yuv420", "yuv411", "RAW", "yuv422P", "yuv411P", "yuv420P", "yuv410P" };
    
    struct video_capability  capability;
    struct video_channel     channel;
    struct video_picture     pict;
    int  j,i,fd;
    char *nick, *attr;

    for (i = 0; dev[i] != NULL; i++) {
	fprintf(stderr,"V4l: trying %s... ",dev[i]);
	if (-1 == (fd = open(dev[i],O_RDONLY))) {
	    perror("open");
	    continue;
	}
	if (-1 == ioctl(fd,VIDIOCGCAP,&capability)) {
	    perror("ioctl VIDIOCGCAP");
	    close(fd);
	    continue;
	}

	if (!(capability.type & VID_TYPE_CAPTURE)) {
	    fprintf(stderr,"device can't capture\n");
	    close(fd);
	    continue;
	}

	fprintf(stderr,"ok, %s\nV4l:   %s; size: %dx%d => %dx%d%s\n",
		capability.name,
		capability.type & VID_TYPE_MONOCHROME ? "mono" : "color",
		capability.minwidth,capability.minheight,
		capability.maxwidth,capability.maxheight,
		capability.type & VID_TYPE_SCALES ? " (scales)" : "");

	attr = new char[512];
	strcpy(attr,"format { 411 422 cif } ");

        if (capability.maxwidth  > PAL_WIDTH/2 &&
	    capability.maxheight > PAL_HEIGHT/2) {
	    strcat(attr,"size { small large cif } ");
	} else {
	    strcat(attr,"size { small cif } ");
	}
	
	fprintf(stderr,"V4l:   ports:");
	strcat(attr,"port { ");
	for (j = 0; j < capability.channels; j++) {
	    channel.channel = j;
	    if (-1 == ioctl(fd,VIDIOCGCHAN,&channel)) {
		perror("ioctl VIDIOCGCHAN");
	    } else {
		fprintf(stderr," %s",channel.name);
		strcat(attr,channel.name);
		strcat(attr," ");
	    }
	}
	fprintf(stderr,"\n");
	strcat(attr,"} ");

	if (-1 == ioctl(fd,VIDIOCGPICT,&pict)) {
	    perror("ioctl VIDIOCGPICT");
	}
	fprintf(stderr,"V4l:   depth=%d, palette=%s\n",
		pict.depth,(pict.palette<sizeof(palette_name)/sizeof(char*))?
		palette_name[pict.palette]:"??");


	nick = new char[strlen(capability.name)+6];
	sprintf(nick,"v4l- %s",capability.name);
	new V4lDevice(dev[i],nick,attr);


	close(fd);
    }
}

/* ----------------------------------------------------------------- */

V4lGrabber::V4lGrabber(const char *cformat, const char *dev)
{
    int i;
    struct video_mmap 	     vid_mmap;

    have_mmap = 0;
    have_422P = 0;

    fd_ = open(dev, O_RDWR);
    if (fd_ < 0) {
	perror("open");
	exit(1);
    }
    vid_mmap.format = VIDEO_PALETTE_YUV422P;
    vid_mmap.frame = 0;
    vid_mmap.width = PAL_WIDTH;
    vid_mmap.height = PAL_HEIGHT;

    if (-1 != ioctl(fd_, VIDIOCMCAPTURE, &vid_mmap)) {
           have_422P = 1;
    }
    /* Release device */
    close(fd_);

    fd_ = open(dev, O_RDWR);
    if (fd_ < 0) {
	perror("open");
	exit(1);
    }
    /* ask for capabilities */
    if (-1 == ioctl(fd_,VIDIOCGCAP,&capability)) {
	perror("ioctl VIDIOCGCAP");
	exit(1);
    }
    channels = (struct video_channel*)
	calloc(capability.channels,sizeof(struct video_channel));
    for (i = 0; i < capability.channels; i++) {
	channels[i].channel = i;
	if (-1 == ioctl(fd_,VIDIOCGCHAN,&channels[i])) {
	    perror("ioctl VIDIOCGCHAN");
	}
    }
    if (-1 == ioctl(fd_,VIDIOCGPICT,&pict)) {
	perror("ioctl VIDIOCGPICT");
    }

    /* map grab buffer */
    gb_buffers.size = 2*0x151000;
    gb_buffers.offsets[0] = 0;
    gb_buffers.offsets[1] = 0x151000;
    if (-1 == ioctl(fd_,VIDIOCGMBUF,&gb_buffers)) {
	perror("ioctl VIDIOCGMBUF");
    }
    mem = (char *)mmap(0,gb_buffers.size,PROT_READ|PROT_WRITE,MAP_SHARED,fd_,0);
    if ((char*)-1 == mem) {
	perror("mmap");
	fprintf(stderr,"V4l: device has no mmap support\n");
    } else {
	fprintf(stderr,"v4l: mmap()'ed buffer size = 0x%x\n",
		gb_buffers.size);
	have_mmap = 1;
    }
  
 
 
    /* fill in defaults */
    if(!strcmp(cformat, "411"))
	cformat_ = CF_411;
    if(!strcmp(cformat, "422"))
	cformat_ = CF_422;
    if(!strcmp(cformat, "cif"))
	cformat_ = CF_CIF;
    
    port_      = 0;
    decimate_  = 2;
}

V4lGrabber::~V4lGrabber()
{
    DEBUG(fprintf(stderr,"V4l: destructor\n"));

    if (have_mmap)
	munmap(mem,gb_buffers.size);
    close(fd_);
}

int V4lGrabber::command(int argc, const char*const* argv)
{
    int i;
    struct video_channel     channel;

    Tcl &tcl = Tcl::instance();

    byteorder_ = 0;

    if ( tcl.attr("yuv_byteOrder") != NULL ) 
	byteorder_ = atoi( tcl.attr("yuv_byteOrder") );
 
    if ( ! ((byteorder_ >= 0) && (byteorder_ <= 3)) ) byteorder_=0;


    if (argc == 3) {
	if (strcmp(argv[1], "decimate") == 0) {
	    decimate_ = atoi(argv[2]);
	    if (running_)
		format();
	}

	if (strcmp(argv[1], "port") == 0) {
	    for (i = 0; i < capability.channels; i++)
		if(!strcmp(argv[2], channels[i].name))
		    port_ = i;
	    if (running_)
		format();
    	    return (TCL_OK);
	}


        if (strcmp(argv[1], "brightness") == 0) {
                        u_char val = atoi(argv[2]);
		
			pict.brightness=val*256;
			
    			if (-1 == ioctl(fd_,VIDIOCSPICT,&pict))
        		perror("ioctl VIDIOCSPICT");

                        DEBUG(fprintf(stderr, "V4l: Brightness = %d\n", val));
                        return (TCL_OK);
        }

        if (strcmp(argv[1], "contrast") == 0) {
                        u_char val = atoi(argv[2]);
			
			pict.contrast=val*256;

    			if (-1 == ioctl(fd_,VIDIOCSPICT,&pict))
        		perror("ioctl VIDIOCSPICT");

                        DEBUG(fprintf(stderr, "V4l: Contrast = %d\n", val));
                        return (TCL_OK);
        }

        if (strcmp(argv[1], "hue") == 0) {
                        u_char val = atoi(argv[2]);
			
			pict.hue=val*256;

    			if (-1 == ioctl(fd_,VIDIOCSPICT,&pict))
        		perror("ioctl VIDIOCSPICT");

                        DEBUG(fprintf(stderr, "V4l: Hue = %d\n", val));
                        return (TCL_OK);
        }

        if (strcmp(argv[1], "saturation") == 0) {
                        u_char val = atoi(argv[2]);

                        pict.colour=val*256;

    			if (-1 == ioctl(fd_,VIDIOCSPICT,&pict))
        		perror("ioctl VIDIOCSPICT");

                        DEBUG(fprintf(stderr, "V4l: Saturation = %d\n", val));
                        return (TCL_OK);
        }

	if (strcmp(argv[1], "controls") == 0) {
		if (strcmp(argv[2], "reset") == 0) {

			pict.brightness=32768;
			pict.contrast=32768;
			pict.colour=32768;
			pict.hue=32768;

                        if (-1 == ioctl(fd_,VIDIOCSPICT,&pict))
                        perror("ioctl VIDIOCSPICT");

                        DEBUG(fprintf(stderr, "V4l: Resetting controls\n"));
                        return (TCL_OK);
		}
        }



        if (strcmp(argv[1], "yuv_byteorder") == 0) {
                        byteorder_ = atoi(argv[2]);
                        return (TCL_OK);
        }

        if (strcmp(argv[1], "norm") == 0) {
                        norm_ = atoi(argv[2]);

                        channel.channel=port_;
                        channel.norm=norm_;

    			if (-1 == ioctl(fd_, VIDIOCSCHAN, &channel))
			perror("ioctl VIDIOCSCHAN");

                        DEBUG(fprintf(stderr, "V4l: Norm = %d\n", norm_));
                        return (TCL_OK);
	}

	if (strcmp(argv[1], "fps") == 0) {
	    DEBUG(fprintf(stderr,"V4l: fps %s\n",argv[2]));
	}
    }

    return (Grabber::command(argc, argv));
}

void V4lGrabber::start()
{
    DEBUG(fprintf(stderr,"V4l: start\n"));

    format();

    if (have_mmap) {
	grab_count = 0;
	sync_count = 0;

	if (-1 == ioctl(fd_, VIDIOCMCAPTURE, &gb_even))
	    perror("ioctl VIDIOCMCAPTURE even");
	else
	    grab_count++;

	if (-1 == ioctl(fd_, VIDIOCMCAPTURE, &gb_odd))
	    perror("ioctl VIDIOCMCAPTURE odd");
	else
	    grab_count++;
    }
    
    Grabber::start();
}

void V4lGrabber::stop()
{
    DEBUG(fprintf(stderr,"V4l: stop\n"));

    if (have_mmap) {
	while (grab_count > sync_count) {
	    if (-1 == ioctl(fd_, VIDIOCSYNC, (sync_count%2) ? &one:&zero)) {
		perror("ioctl VIDIOCSYNC");
		break;
	    } else
		sync_count++;
	}
    }
    
    Grabber::stop();
}

int V4lGrabber::grab()
{
    char  *fr=NULL;

    DEBUG(fprintf(stderr,(sync_count % 2) ? "o" : "e"));

    if (have_mmap) {
	fr = mem + (gb_buffers.offsets[ (sync_count % 2) ? 1: 0]);
	if (-1 == ioctl(fd_, VIDIOCSYNC, (sync_count%2) ? &one:&zero))
	    perror("ioctl VIDIOCSYNC");
	else
	    sync_count++;
    } else {
	/* FIXME: read() */
    }


    switch (cformat_) {
    case CF_411:

    case CF_CIF:
	packed422_to_planar411((char*)frame_,fr);
	break;

    case CF_422:
    if (have_422P) 
	memcpy((void *)frame_, (const void *)fr, (size_t)height_*width_*2);
    else
	packed422_to_planar422((char*)frame_,fr);
	break;
    }

    if (have_mmap) {
	if (-1 == ioctl(fd_, VIDIOCMCAPTURE,
			(grab_count % 2) ? &gb_odd : &gb_even))
	    perror("ioctl VIDIOMCAPTURE");
	else
	    grab_count++;
    }

    suppress(frame_);
    saveblks(frame_);
    YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
    return (target_->consume(&f));
}

void V4lGrabber::packed422_to_planar422(char *dest, char *src)
{
    int i;
    char *s, *y,*u,*v;
    unsigned int a, *srca;

    srca = (unsigned int *)src;

    i = (width_ * height_)/2;
    s = src;
    y = dest;
    u = y + width_ * height_;
    v = u + width_ * height_ / 2;


    switch (byteorder_) {
    case BYTE_ORDER_YUYV:
	while (--i) {
        	a = *(srca++);
        	*(y++) = a & 0xff;
        	a >>= 8;
        	*(u++) = a & 0xff;
        	a >>= 8;
        	*(y++) = a & 0xff;
        	a >>= 8;
        	*(v++) = a & 0xff;
    	}
        break;

    case BYTE_ORDER_YVYU:
	while (--i) {
        	a = *(srca++);
        	*(y++) = a & 0xff;
        	a >>= 8;
        	*(v++) = a & 0xff;
        	a >>= 8;
        	*(y++) = a & 0xff;
        	a >>= 8;
        	*(u++) = a & 0xff;
    	}
        break;

    case BYTE_ORDER_UYVY:
	while (--i) {
        	a = *(srca++);
        	*(u++) = a & 0xff;
        	a >>= 8;
        	*(y++) = a & 0xff;
        	a >>= 8;
        	*(v++) = a & 0xff;
        	a >>= 8;
        	*(y++) = a & 0xff;
    	}
        break;

    case BYTE_ORDER_VYUY:
	while (--i) {
        	a = *(srca++);
        	*(v++) = a & 0xff;
        	a >>= 8;
        	*(y++) = a & 0xff;
        	a >>= 8;
        	*(u++) = a & 0xff;
        	a >>= 8;
        	*(y++) = a & 0xff;
    	}
        break;
    }

}

void V4lGrabber::packed422_to_planar411(char *dest, char *src)
{
    int  a1,b;
    char *s, *y,*u,*v;
    unsigned int a, *srca;

    srca = (unsigned int *)src;

    s = src;
    y = dest;
    u = y + width_ * height_;
    v = u + width_ * height_ / 4;

    switch (byteorder_) {
    case BYTE_ORDER_YUYV:
	for (a1 = height_; a1 > 0; a1 -= 2) {
        	for (b = width_; b > 0; b -= 2) {
                	a = *(srca++);
                	*(y++) = a & 0xff; a >>= 8;
                	*(u++) = a & 0xff; a >>= 8;
                	*(y++) = a & 0xff; a >>= 8;
                	*(v++) = a & 0xff;
        	}
        	for (b = width_; b > 0; b -= 2) {
                	a = *(srca++); 
                	*(y++) = a & 0xff; a >>= 16;
                	*(y++) = a & 0xff;
        	}
    	}
        break;

    case BYTE_ORDER_YVYU:
	for (a1 = height_; a1 > 0; a1 -= 2) {
        	for (b = width_; b > 0; b -= 2) {
                	a = *(srca++);
                	*(y++) = a & 0xff; a >>= 8;
                	*(v++) = a & 0xff; a >>= 8;
                	*(y++) = a & 0xff; a >>= 8;
                	*(u++) = a & 0xff;
        	}
        	for (b = width_; b > 0; b -= 2) {
                	a = *(srca++); 
                	*(y++) = a & 0xff; a >>= 16;
                	*(y++) = a & 0xff;
        	}
    	}
        break;

    case BYTE_ORDER_UYVY:
	for (a1 = height_; a1 > 0; a1 -= 2) {
        	for (b = width_; b > 0; b -= 2) {
                	a = *(srca++);
                	*(u++) = a & 0xff; a >>= 8;
                	*(y++) = a & 0xff; a >>= 8;
                	*(v++) = a & 0xff; a >>= 8;
                	*(y++) = a & 0xff;
        	}
        	for (b = width_; b > 0; b -= 2) {
                	a = *(srca++); a >>= 8;
                	*(y++) = a & 0xff; a >>= 16;
                	*(y++) = a & 0xff;
        	}
    	}
        break;

    case BYTE_ORDER_VYUY:
	for (a1 = height_; a1 > 0; a1 -= 2) {
        	for (b = width_; b > 0; b -= 2) {
                	a = *(srca++);
                	*(v++) = a & 0xff; a >>= 8;
                	*(y++) = a & 0xff; a >>= 8;
                	*(u++) = a & 0xff; a >>= 8;
                	*(y++) = a & 0xff;
        	}
        	for (b = width_; b > 0; b -= 2) {
                	a = *(srca++); a >>= 8;
                	*(y++) = a & 0xff; a >>= 16;
                	*(y++) = a & 0xff;
        	}
    	}
        break;
    }


}



void V4lGrabber::format()
{
    struct video_channel     channel;
    DEBUG(fprintf(stderr,"V4l: format"));

    width_  = CIF_WIDTH  *2  / decimate_;
    height_ = CIF_HEIGHT *2  / decimate_;

    v4lformat_=VIDEO_PALETTE_YUV422;

    switch (cformat_) {
    case CF_CIF:
        set_size_411(width_, height_);
        DEBUG(fprintf(stderr," cif"));
        break;
    case CF_411:
        set_size_411(width_, height_);
        DEBUG(fprintf(stderr," 411"));
        break;
    case CF_422:
        set_size_422(width_, height_);
    	if (have_422P ) {
		v4lformat_=VIDEO_PALETTE_YUV422P;
        	fprintf(stderr,"Capturing directly in 422 Planar\n");
	}
        DEBUG(fprintf(stderr," 422"));
        break;
    }
    if (have_mmap) {
	gb_even.frame  = 0;
	gb_even.format = v4lformat_;
	gb_even.width  = width_;
	gb_even.height = height_;
	gb_odd.frame   = 1;
	gb_odd.format  = v4lformat_;
	gb_odd.width   = width_;
	gb_odd.height  = height_;
    } else {
	memset(&win,0,sizeof(win));
	win.width  = width_;
	win.height = height_;
	if (-1 == ioctl(fd_,VIDIOCSWIN,&win))
	    perror("ioctl VIDIOCSWIN");
	if (-1 == ioctl(fd_,VIDIOCGWIN,&win))
	    perror("ioctl VIDIOCGWIN");
	width_  = win.width;
	height_ = win.height;
	pict.palette = v4lformat_;
	if (-1 == ioctl(fd_,VIDIOCSPICT,&pict)) {
	  perror("ioctl VIDIOCSPICT");
	}
    }
    if (-1 == ioctl(fd_,VIDIOCGPICT,&pict)) {
	perror("ioctl VIDIOCGPICT");
    }

    
    DEBUG(fprintf(stderr," size=%dx%d",width_,height_));

    channel.channel = port_;
    channel.norm = norm_;
    if (-1 == ioctl(fd_, VIDIOCSCHAN, &channel))
	perror("ioctl VIDIOCSCHAN");
    DEBUG(fprintf(stderr," port=%d\n",port_));
    DEBUG(fprintf(stderr," norm=%d\n",norm_));

    allocref();
}
