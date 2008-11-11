/* =========================================================================
 
     Copyright (c) 1997 Regents of Koji OKAMURA, oka@kobe-u.ac.jp
     All rights reserved.
 
     largely rewritten for new bttv/video4linux interface
     by Gerd Knorr <kraxel@cs.tu-berlin.de>
 
     Added brightness, contrast, hue and saturation controls.
     by Jean-Marc Orliaguet <jmo@medialab.chalmers.se>
 
     Added support for various YUV byte orders.
     by Jean-Marc Orliaguet <jmo@medialab.chalmers.se>
 
     Added support for NTSC/PAL/SECAM video norm selection. (14/10/99)
     by Jean-Marc Orliaguet <jmo@medialab.chalmers.se>
 
     Early stage V4L2 implementation. (v0.92a)
     by Jean-Marc Orliaguet <jmo@medialab.chalmers.se>
     Capture code taken from xcaptest.c V4L2 demo app (Bill Dirks)
     Some code contributed by Malcolm Caldwell <malcolm@it.ntu.edu.au>
 
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

extern "C"
{
#include <asm/types.h>
/*#include <linux/videodev.h>*/
#include "videodev2-old.h"
}

#include "grabber.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "module.h"

/* here you can tune the device names *
static const char *devlist[] = { "/dev/video0", "/dev/video1", "/dev/video2", "/dev/video3",NULL };*/
static const char *devlist[] = { "/dev/v4l2/capture0", "/dev/v4l2/capture1", "/dev/v4l2/capture2", "/dev/v4l2/capture3",NULL };

//#define DEBUG(x)
#define DEBUG(x) (x)

#define NTSC_WIDTH  640
#define NTSC_HEIGHT 480
#define PAL_WIDTH   768
#define PAL_HEIGHT  576
#define CIF_WIDTH   352
#define CIF_HEIGHT  288

/* pass 0/1 by reference */
static const int  one = 1, zero = 0;

#define CF_422 0
#define CF_420 1
#define CF_CIF 2

typedef struct tag_vimage
{
        struct v4l2_buffer      vidbuf;
        char                    *data;
}       VIMAGE;


#define STREAMBUFS 2

class V4l2Grabber : public Grabber
{
public:
        V4l2Grabber(const char * cformat, const char *dev);
        virtual ~V4l2Grabber();

        virtual int  command(int argc, const char*const* argv);
        virtual void start();
        virtual void stop();
        virtual int  grab();


protected:
        void format();
        void setsize();

        void packed422_to_planar422(char *, char*);
        void packed422_to_planar420(char *, char*);

        void setctrl(int, int, char *, int);

        struct v4l2_capability   capability;
        struct v4l2_input        *inputs;
        struct v4l2_pix_format   pict;
        struct v4l2_input	 input;
        struct v4l2_queryctrl	 qctrl;
        struct v4l2_format 	fmt;
        struct v4l2_requestbuffers req;


        /* mmap */
        int                      have_mmap;
        VIMAGE		vimage[STREAMBUFS];
        struct v4l2_buffer      tempbuf;
        int 		buf_sync;

        int fd_;
        int format_;
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

class V4l2Device : public InputDevice
{
public:
        V4l2Device(const char *dev, const char*, char *attr);
        virtual int command(int argc, const char*const* argv);

private:
        const char *dev_;
};


V4l2Device::V4l2Device(const char *dev, const char *name, char *attr) : InputDevice(name)
{
        dev_ = dev;
        attributes_ = attr;
        fprintf(stderr,"V4l2:  ==> %s\n",attr);
}

int V4l2Device::command(int argc, const char*const* argv)
{
        Tcl& tcl = Tcl::instance();


        if (argc == 3) {
                if (strcmp(argv[1], "open") == 0) {
                        TclObject* o = 0;
                        o = new V4l2Grabber(argv[2],dev_);
                        if (o != 0)
                                tcl.result(o->name());
                        return (TCL_OK);
                }

        }
        return (InputDevice::command(argc, argv));
}

/* ----------------------------------------------------------------- */

class V4l2Scanner
{
public:
        V4l2Scanner(const char **dev);
};

static V4l2Scanner find_video4linux_devices(devlist);

V4l2Scanner::V4l2Scanner(const char **dev)
{
        struct v4l2_capability   capability;
        struct v4l2_input	 input;
        struct v4l2_fmtdesc 	 pixfmt;

        int  k,j,i,err,fd;
        char *nick, *attr;

        for (i = 0; dev[i] != NULL; i++) {
                fprintf(stderr,"V4l2: trying %s... ",dev[i]);
                if (-1 == (fd = open(dev[i],O_NONCAP))) {
                        perror("open");
                        continue;
                }
                if (-1 == ioctl(fd,VIDIOC_QUERYCAP,&capability)) {
                        perror("ioctl VIDIOC_QUERYCAP");
                        close(fd);
                        continue;
                }

                if (capability.type != V4L2_TYPE_CAPTURE) {
                        fprintf(stderr,"device %d can't capture\n",capability.type);
                        close(fd);
                        continue;
                }

                fprintf(stderr,"ok, %s\nV4l2:   %s; size: %dx%d => %dx%d\n",
                        capability.name,
                        capability.flags & V4L2_FLAG_MONOCHROME ? "mono" : "color",
                        capability.minwidth,capability.minheight,
                        capability.maxwidth,capability.maxheight);

                attr = new char[512];
                strcpy(attr,"format { 420 422 cif } ");

                if (capability.maxwidth  > PAL_WIDTH/2 &&
                                        capability.maxheight > PAL_HEIGHT/2) {
                        strcat(attr,"size { small large cif } ");
                } else {
                        strcat(attr,"size { small cif } ");
                }

                fprintf(stderr,"V4l2:   ports:");
                strcat(attr,"port { ");
                for (j = 0; j < capability.inputs; j++) {
                        if (-1 == ioctl(fd,VIDIOC_S_INPUT,&j)) {
                                perror("ioctl VIDIOC_S_INPUT");
                        } else {
                                input.index=j;
                                if (-1 == ioctl(fd,VIDIOC_ENUMINPUT,&input)) {
                                        perror("ioctl VIDIOC_ENUMINPUT");
                                } else {
                                        fprintf(stderr," %s: ",input.name);
                                        for (unsigned int s=0 ; s<strlen(input.name) ; s++)
                                                if (input.name[s]==' ') input.name[s]='-';
                                        strcat(attr,input.name);
                                        strcat(attr," ");
                                }

                                pixfmt.index=j;
                                if (-1 == ioctl(fd,VIDIOC_ENUM_PIXFMT,&pixfmt)) {
                                        perror("ioctl VIDIOC_ENUM_PIXFMT");
                                } else fprintf(stderr, "%s ",pixfmt.description);
                        }
                }
                fprintf(stderr,"\n");
                strcat(attr,"} ");

		fprintf(stderr,"V4l2:   norms: ");
                strcat(attr,"type { ");
		
		for (k = 0, err = 0; err == 0; ++k)
        	{
                	struct v4l2_enumstd     estd;
                	estd.index = k;
                	err = ioctl(fd, VIDIOC_ENUMSTD, &estd);
                	if (!err) {
				strcat(attr, (const char*)estd.std.name);
				strcat(attr," ");           
				fprintf(stderr,"%s ", estd.std.name);
			}     	
        	}
                fprintf(stderr,"\n");

		strcat(attr,"} ");

                nick = new char[strlen(capability.name)+6];
                sprintf(nick,"v4l- %s",capability.name);
                new V4l2Device(dev[i],nick,attr);

                close(fd);
        }
}

/* ----------------------------------------------------------------- */

V4l2Grabber::V4l2Grabber(const char *cformat, const char *dev)
{

        fd_ = open(dev, O_RDWR);
        if (fd_ < 0) {
                perror("open");
                return;
        }

        have_mmap = 0;

        struct v4l2_capability cap;
        if (-1 == ioctl(fd_,VIDIOC_QUERYCAP,&cap)) {
                perror("ioctl VIDIOC_QUERYCAP");
        } else {
                if ( !(cap.flags & V4L2_FLAG_READ) && !(cap.flags & V4L2_FLAG_STREAMING )) {
                        fprintf(stderr,"v4l2: fatal: device does not support read() call or streaming capture.\n");
                        return;
                }

                if ( (cap.flags & V4L2_FLAG_STREAMING) && (cap.flags & V4L2_FLAG_SELECT))
                        have_mmap = 1;

        }

        /* fill in defaults */
        if(!strcmp(cformat, "420"))
                cformat_ = CF_420;
        if(!strcmp(cformat, "422"))
                cformat_ = CF_422;
        if(!strcmp(cformat, "cif"))
                cformat_ = CF_CIF;

        port_      = 0;
        norm_      =  0; 
        decimate_  = 2;
        running_ = 0;

}


V4l2Grabber::~V4l2Grabber()
{
        int i;
        DEBUG(fprintf(stderr,"V4l2: destructor\n"));

        if (have_mmap) {
                for (i = 0; i < STREAMBUFS; ++i) {
                        if (vimage[i].data)
                                munmap(vimage[i].data,
                                       vimage[i].vidbuf.length);
                        vimage[i].data = NULL;
                }
        } else {
                if (vimage[0].data)
                        free(vimage[0].data);
                vimage[0].data = NULL;
        }
        close(fd_);

}

int V4l2Grabber::command(int argc, const char*const* argv)
{
        int i, err;

        Tcl &tcl = Tcl::instance();

        byteorder_ = 0;

        if ( tcl.attr("yuv_byteOrder") != NULL )
                byteorder_ = atoi( tcl.attr("yuv_byteOrder") );

        if ( ! ((byteorder_ >= 0) && (byteorder_ <= 3)) ) byteorder_=0;

        if (argc == 3) {
                if (strcmp(argv[1], "decimate") == 0) {
                        decimate_ = atoi(argv[2]);

                        if (running_) {
                                stop(); start();
                        }
                }

                if (strcmp(argv[1], "port") == 0) {

                        for (i = 0, err = 0; err == 0; ++i) {
                                struct v4l2_input inp;
                                inp.index = i;
                                err = ioctl(fd_, VIDIOC_ENUMINPUT, &inp);
                                if (!err) {

                                        char input[32];
                                        unsigned int s;
                                        for ( s=0 ; s<= strlen(inp.name) ; s++ )
                                                        if (inp.name[s]==' ') input[s]='-'; else input[s]=inp.name[s];

                                        if ( !strcmp(input,argv[2])) {
                                                port_ = i;
                                                break;
                                        }
                                }
                        }
                        if (running_) {
                                stop(); start();
                        }

                        return (TCL_OK);
                }


                if (strcmp(argv[1], "brightness") == 0) {
                        setctrl(atoi(argv[2]), V4L2_CID_BRIGHTNESS, "Brightness", 0);
                        return (TCL_OK);
                }

                if (strcmp(argv[1], "hue") == 0) {
                        setctrl(atoi(argv[2]), V4L2_CID_HUE, "Hue", 0);
                        return (TCL_OK);
                }

                if (strcmp(argv[1], "contrast") == 0) {
                        setctrl(atoi(argv[2]), V4L2_CID_CONTRAST, "Contrast", 0);
                        return (TCL_OK);
                }

                if (strcmp(argv[1], "saturation") == 0) {
                        setctrl(atoi(argv[2]), V4L2_CID_SATURATION, "Saturation", 0);
                        return (TCL_OK);
                }


                if (strcmp(argv[1], "controls") == 0) {
                        if (strcmp(argv[2], "reset") == 0) {
                                DEBUG(fprintf(stderr, "V4l2: Resetting controls\n"));

                                setctrl(atoi(argv[2]), V4L2_CID_BRIGHTNESS, "Brightness", 1);
                                setctrl(atoi(argv[2]), V4L2_CID_HUE, "Hue", 1);
                                setctrl(atoi(argv[2]), V4L2_CID_CONTRAST, "Contrast", 1);
                                setctrl(atoi(argv[2]), V4L2_CID_SATURATION, "Saturation", 1);
                                return (TCL_OK);
                        }
                }

                if (strcmp(argv[1], "yuv_byteorder") == 0) {
                        DEBUG(fprintf(stderr,"V4l2: asked for yuv_byteorder\n"));
                        return (TCL_OK);
                }

                if (strcmp(argv[1], "fps") == 0) {
                        DEBUG(fprintf(stderr,"V4l2: fps %s\n",argv[2]));
                }


                if (strcmp(argv[1], "type") == 0 || strcmp(argv[1], "format") == 0) {
                        
                        for (int k = 0, err = 0; err == 0; ++k)
        		{
                		struct v4l2_enumstd     estd;
                		estd.index = k;
                		if ( !(err = ioctl(fd_, VIDIOC_ENUMSTD, &estd)) )
					if ( strcmp(argv[2], (const char *)estd.std.name) == 0)
						norm_ = k;          
			}     	     

                        if (running_) {
                                stop(); start();
                        }

                        return (TCL_OK);
                }

	} else if (argc == 2) {
        	if (strcmp(argv[1], "format") == 0 ||
               		strcmp(argv[1], "type") == 0) {
            		return (TCL_OK);
        	}
    	}

        return (Grabber::command(argc, argv));
}

void V4l2Grabber::start()
{
        int     err;
        int     i;

        if (fd_ > 0 ) {
                DEBUG(fprintf(stderr,"\nv4l2: start\n"));

                format();

                if (have_mmap) {
                        buf_sync = 1;

                        req.count = STREAMBUFS;
                        req.type = V4L2_BUF_TYPE_CAPTURE;
                        err = ioctl(fd_, VIDIOC_REQBUFS, &req);
                        if (err < 0 || req.count < 1) {
                                DEBUG(fprintf(stderr,"REQBUFS returned error %d, count %d\n", errno,req.count));
                                return;
                        }

                        for (i = 0; i < req.count; ++i) {
                                vimage[i].vidbuf.index = i;
                                vimage[i].vidbuf.type = V4L2_BUF_TYPE_CAPTURE;
                                err = ioctl(fd_, VIDIOC_QUERYBUF, &vimage[i].vidbuf);
                                if (err < 0) {
                                        DEBUG(fprintf(stderr,"QUERYBUF returned error %d\n",errno));
                                        return;
                                }
                                vimage[i].data = (typeof(vimage[0].data)) mmap(0,  vimage[i].vidbuf.length, PROT_READ, MAP_SHARED, fd_, vimage[i].vidbuf.offset);

                                if ((int)vimage[i].data == -1) {
                                        DEBUG(fprintf(stderr,"v4l2: mmap() returned error %d\n", errno));
                                        return;
                                } else DEBUG(fprintf(stderr,"v4l2: mmap()'ed buffer at 0x%x (%d bytes)\n", (int)vimage[i].data, vimage[i].vidbuf.length));
                        }

                        for (i = 0; i < req.count; ++i)
                                if ((err = ioctl(fd_, VIDIOC_QBUF, &vimage[i].vidbuf))) {
                                        DEBUG(fprintf(stderr,"QBUF returned error %d\n",errno));
                                        return;
                                }

                        err = ioctl(fd_, VIDIOC_STREAMON, &vimage[0].vidbuf.type);
                        if (err)
                                DEBUG(fprintf(stderr,"STREAMON returned error %d\n",errno));

                } else {
                        vimage[0].data = (typeof(vimage[0].data)) malloc(fmt.fmt.pix.sizeimage);

                        if (vimage[0].data == NULL) {
                                DEBUG(fprintf(stderr,"malloc(%d) failed\n", fmt.fmt.pix.sizeimage));
                                return;
                        } else DEBUG(fprintf(stderr,"v4l2: malloc()'ed buffer (%d bytes)\n",  fmt.fmt.pix.sizeimage));
                }

                Grabber::start();
                running_ = 1;
        }
}

void V4l2Grabber::stop()
{
        DEBUG(fprintf(stderr,"V4l2: stop\n"));
        int i, err;

        if (have_mmap) {
                if ( fd_ > 0 ) {
                        i = V4L2_BUF_TYPE_CAPTURE;
                        if ( (err = ioctl(fd_, VIDIOC_STREAMOFF, &i) ) )
                                DEBUG(fprintf(stderr,"v4l2: VIDIOC_STREAMOFF failed\n"));
                }

                tempbuf.type = vimage[0].vidbuf.type;
                while (  !(err = ioctl(fd_, VIDIOC_DQBUF, &tempbuf)) )
                        DEBUG(fprintf(stderr,"v4l2: dequeued old buffer\n"));

                for (i = 0; i < STREAMBUFS; ++i) {
                        if (vimage[i].data)
                                err = munmap(vimage[i].data, vimage[i].vidbuf.length);
                        vimage[i].data = NULL;
                }


        } else {
                if (vimage[0].data)
                        free(vimage[0].data);
                vimage[0].data = NULL;
        }

        Grabber::stop();
        running_ = 0;
}

int V4l2Grabber::grab()
{
        char  *fr=NULL;
        fd_set                  rdset;
        struct timeval          timeout;
        int                     n;

        if (have_mmap) {
                FD_ZERO(&rdset);
                FD_SET(fd_, &rdset);
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
                n = select(fd_+1, &rdset, NULL, NULL, &timeout);

                if (n == 0)
                        return (0);
                else if (FD_ISSET(fd_, &rdset)) {
                        tempbuf.type = vimage[0].vidbuf.type;
                        fr = vimage[buf_sync%2].data;
                        ioctl(fd_, VIDIOC_DQBUF, &tempbuf);

                        if (  (req.count > 1) ) buf_sync++;
                        fr = vimage[buf_sync%2].data;

                }

        } else {
                fr = vimage[0].data;
                read(fd_, vimage[0].data, fmt.fmt.pix.sizeimage);
        }

        switch (cformat_) {
        case CF_420:

        case CF_CIF:
                packed422_to_planar420((char*)frame_,fr);
                break;

        case CF_422:
                packed422_to_planar422((char*)frame_,fr);
                break;
        }

        if (have_mmap)
                ioctl(fd_, VIDIOC_QBUF, &vimage[buf_sync%2].vidbuf);


        suppress(frame_);
        saveblks(frame_);
        YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);


        return (target_->consume(&f));
}


void V4l2Grabber::packed422_to_planar422(char *dest, char *src)
{
        int i;
        int w, h;
        char *s, *y,*u,*v;
        unsigned int a, *srca;

        srca = (unsigned int *)src;

        w = fmt.fmt.pix.width;
        h = fmt.fmt.pix.height;

        i = (w * h)/2;
        s = src;
        y = dest;
        u = y + w * h;
        v = u + w * h /2 ;

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

}

void V4l2Grabber::packed422_to_planar420(char *dest, char *src)
{
        int  a1,b;
        int w,h;
        char *s, *y,*u,*v;
        unsigned int a, *srca;

        srca = (unsigned int *)src;

        w = fmt.fmt.pix.width;
        h = fmt.fmt.pix.height;
        s = src;
        y = dest;
        u = y + w * h;
        v = u + w * h  / 4;

        for (a1 = h; a1 > 0; a1 -= 2) {
                for (b = w; b > 0; b -= 2) {
                        a = *(srca++);
                        *(y++) = a & 0xff; a >>= 8;
                        *(u++) = a & 0xff; a >>= 8;
                        *(y++) = a & 0xff; a >>= 8;
                        *(v++) = a & 0xff;
                }
                for (b = w; b > 0; b -= 2) {
                        a = *(srca++);
                        *(y++) = a & 0xff; a >>= 16;
                        *(y++) = a & 0xff;
                }
        }

}



void V4l2Grabber::format()
{

        struct v4l2_standard 	standard;

        int i, err;
        int input;
        int format_ok = 0;

        while ( !format_ok ) {
                width_  = CIF_WIDTH  *2  / decimate_;
                height_ = CIF_HEIGHT *2  / decimate_;

                DEBUG(fprintf(stderr,"v4l2: format"));
                switch (cformat_) {
                case CF_CIF:
                        set_size_420(width_, height_);
                        DEBUG(fprintf(stderr," cif"));
                        break;
                case CF_420:
                        set_size_420(width_, height_);
                        DEBUG(fprintf(stderr," 420"));
                        break;
                case CF_422:
                        set_size_422(width_, height_);
                        DEBUG(fprintf(stderr," 422"));
                        break;
                }
                DEBUG(fprintf(stderr,"\n"));

                ioctl(fd_, VIDIOC_G_STD, &standard);

                
                        struct v4l2_enumstd     estd;
                        estd.index = norm_;
                        err = ioctl(fd_, VIDIOC_ENUMSTD, &estd);
                        if (!err) {         
                                        standard = estd.std;
                                        if (-1 == ioctl(fd_, VIDIOC_S_STD, &standard))
                                                perror("ioctl VIDIOC_S_STD");
                                        else DEBUG(fprintf(stderr,"v4l2: setting norm to %s\n",estd.std.name));                                     }
                

                input = port_;
                if ((err = ioctl(fd_, VIDIOC_S_INPUT, &input)) )
                        DEBUG(fprintf(stderr,"S_INPUT returned error %d\n",errno));
                else DEBUG(fprintf(stderr,"v4l2: setting input port to %d\n", port_));

                for (i = 0, err = 0; err == 0; ++i) {
                        struct v4l2_fmtdesc     fmtd;
                        fmtd.index = i;
                        err = ioctl(fd_, VIDIOC_ENUM_PIXFMT, &fmtd);
                        if (!err) {

                                if (fmtd.pixelformat == V4L2_PIX_FMT_YUYV) {

                                        fmt.type = V4L2_BUF_TYPE_CAPTURE;
                                        ioctl(fd_, VIDIOC_G_FMT, &fmt);
                                        fmt.fmt.pix.width = width_;
                                        fmt.fmt.pix.height = height_;
                                        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
                                        fmt.fmt.pix.flags |= V4L2_FMT_FLAG_INTERLACED;

                                        if ( (err = ioctl(fd_, VIDIOC_S_FMT, &fmt) ) )
                                                DEBUG(fprintf(stderr,"\nv4l2: Failed to set format\n"));

                                        if ( ( fmt.fmt.pix.width != (unsigned int)width_ ) ||
                                                                ( fmt.fmt.pix.height !=  (unsigned int)height_ ) ) {

                                                DEBUG(fprintf(stderr,"v4l2: failed to set format! requested %dx%d, got %dx%d\n", width_, height_, fmt.fmt.pix.width, fmt.fmt.pix.height));


                                                switch(decimate_) {
                                                case 2:
                                                        fprintf(stderr,"v4l2: trying resolution under ...\n");
                                                        decimate_ = 4;
                                                        break;
                                                case 1:
                                                        fprintf(stderr,"v4l2: trying resolution under ...\n");
                                                        decimate_ = 2;
                                                        break;
                                                default:
                                                        fprintf(stderr,"v4l2: giving up ...\n");
                                                        format_ok = 1;
                                                }

                                        } else {
                                                DEBUG(fprintf(stderr,"v4l2: setting format: width=%d height=%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height));
                                                format_ok = 1;
                                        }
                                        break;
                                }
                        }
                }
        }


        allocref();
}

void V4l2Grabber::setctrl(int val, int cid, const char *controlname, int reset)
{
        struct v4l2_queryctrl qctrl;
        struct v4l2_control ctrl;

        qctrl.id = cid;
        if (-1 != ioctl(fd_, VIDIOC_QUERYCTRL, &qctrl)) {
                if ( !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED) ) {

                        ctrl.id = cid;
                        if ( reset )
                                ctrl.value = qctrl.default_value;
                        else
                                ctrl.value = qctrl.minimum + (qctrl.maximum - qctrl.minimum) * val/256;
                        if (-1 == ioctl(fd_, VIDIOC_S_CTRL,&ctrl))
                                perror("ioctl  VIDIOC_S_CTRL");
                        else DEBUG(fprintf(stderr, "V4l2: %s = %d\n", controlname, val));
                }
        }
        return;
}
