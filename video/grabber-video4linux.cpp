/* =========================================================================

     Copyright (c) 1997 Regents of Koji OKAMURA, oka@kobe-u.ac.jp
     All rights reserved.

     largely rewritten for new bttv/video4linux interface
     by Gerd Knorr <kraxel@cs.tu-berlin.de>

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
    int cformat_;
    int port_;
    
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
	"", "grey", "hi240", "rgb16", "rgb24", "rgb32", "rgb15" };
    
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
		pict.depth,palette_name[pict.palette]);

	nick = new char[strlen(capability.name)+6];
	sprintf(nick,"v4l: %s",capability.name);
	new V4lDevice(dev[i],nick,attr);

	close(fd);
    }
}

/* ----------------------------------------------------------------- */

V4lGrabber::V4lGrabber(const char *cformat, const char *dev)
{
    int i,zero=0;
    
    DEBUG(fprintf(stderr,"V4l: constructor %s\n",cformat));

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
    mem = mmap(0,gb_buffers.size,PROT_READ|PROT_WRITE,MAP_SHARED,fd_,0);
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
    char  *fr;

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

    i = (width_ * height_)/2;
    s = src;
    y = dest;
    u = y + width_ * height_;
    v = u + width_ * height_ / 2;
    
    while (--i) {
	*(y++) = *(src++);
	*(u++) = *(src++);
	*(y++) = *(src++);
        *(v++) = *(src++);
    }
}

void V4lGrabber::packed422_to_planar411(char *dest, char *src)
{
    int  a,b;
    char *s, *y,*u,*v;

    s = src;
    y = dest;
    u = y + width_ * height_;
    v = u + width_ * height_ / 4;

    for (a = height_; a > 0; a -= 2) {
	for (b = width_; b > 0; b -= 2) {
	    
	    *(y++) = *(src++);
	    *(u++) = *(src++);
	    *(y++) = *(src++);
	    *(v++) = *(src++);
	}
	for (b = width_; b > 0; b -= 2) {
	    *(y++) = *(src++);
	             *(src++);
	    *(y++) = *(src++);
	             *(src++);
	}
    }
}

void V4lGrabber::format()
{
    DEBUG(fprintf(stderr,"V4l: format"));

    width_  = CIF_WIDTH  *2  / decimate_;
    height_ = CIF_HEIGHT *2  / decimate_;

    if (have_mmap) {
	gb_even.frame  = 0;
	gb_even.format = VIDEO_PALETTE_YUV422;
	gb_even.width  = width_;
	gb_even.height = height_;
	gb_odd.frame   = 1;
	gb_odd.format  = VIDEO_PALETTE_YUV422;
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
    }

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
	DEBUG(fprintf(stderr," 422"));
	break;
    }
    
    DEBUG(fprintf(stderr," size=%dx%d",width_,height_));

    if (-1 == ioctl(fd_, VIDIOCSCHAN, &port_))
	perror("ioctl VIDIOCSCHAN");
    DEBUG(fprintf(stderr," port=%d\n",port_));

    allocref();
}
