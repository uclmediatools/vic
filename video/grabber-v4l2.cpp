/* =========================================================================

     Copyright (c) 1997 Regents of Koji OKAMURA, oka@kobe-u.ac.jp
     All rights reserved.

     largely rewritten for new bttv/video4linux interface
     by Gerd Knorr <kraxel@cs.tu-berlin.de>

     Added brightness, contrast, hue and saturation controls.
     by Jean-Marc Orliaguet <jmo@medialab.chalmers.se>

     Added support for NTSC/PAL/SECAM video norm selection. (14/10/99)
     by Jean-Marc Orliaguet <jmo@medialab.chalmers.se>

     Early stage V4L2 implementation. (v0.92a)
     by Jean-Marc Orliaguet <jmo@medialab.chalmers.se>
     Capture code taken from xcaptest.c V4L2 demo app (Bill Dirks)
     Some code contributed by Malcolm Caldwell <malcolm@it.ntu.edu.au>

     Added support for MJPEG/JPEG.
     Added gamma and gain controls.
     by Douglas Kosovic <douglask@itee.uq.edu.au>
     MJPEG/JPEG support uses Tiny Jpeg Decoder from :
     http://www.saillard.org/programs_and_patches/tinyjpegdecoder/

   ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <endian.h>

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
#include <linux/videodev2.h>
}

#ifdef HAVE_LIBV4L
#include <libv4l2.h>
#else
#define v4l2_open open
#define v4l2_close close
#define v4l2_ioctl ioctl
#define v4l2_read read
#define v4l2_mmap mmap
#define v4l2_munmap munmap
#include "tinyjpeg.h"
#endif

#include "grabber.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "module.h"
#include "yuv_convert.h"

/* here you can tune the device names */
static const char *devlist[] = {
    "/dev/video0", "/dev/video1", "/dev/video2", "/dev/video3",
    "/dev/video4", "/dev/video5", "/dev/video6", "/dev/video7",
    "/dev/video8", "/dev/video9", "/dev/video10", "/dev/video11",
    NULL
};

#define PAL_BT601_WIDTH   720
#define PAL_BT601_HEIGHT  576
#define NTSC_BT601_WIDTH  720
#define NTSC_BT601_HEIGHT 480
#define VGA_WIDTH   640
#define VGA_HEIGHT  480
#define CIF_WIDTH   352
#define CIF_HEIGHT  288

#define CF_422 0
#define CF_420 1
#define CF_CIF 2

#define CS_VC         0 /* 4CIF (704x576), CIF (352x288), QCIF (176x144) */
#define CS_VGA        1 /* VGA (640x480), 1/4 VGA (320x240), 1/16 VGA (160x120) */
#define CS_BT601_NTSC 2 /* ITU-R Recommendation BT.601 720x480 (plus 360x240 & 180x120) */
#define CS_BT601_PAL  3 /* ITU-R Recommendation BT.601 720x576 (plus 360x288 & 180x144) */

/* YUV Byte order */
#define BYTE_ORDER_YUYV 0
#define BYTE_ORDER_YVYU 1
#define BYTE_ORDER_UYVY 2
#define BYTE_ORDER_VYUY 3

/* V4L2 driver specific controls */
#ifndef V4L2_CID_POWER_LINE_FREQUENCY
#define V4L2_CID_POWER_LINE_FREQUENCY (V4L2_CID_PRIVATE_BASE + 1)
#endif
#ifndef V4L2_CID_FOCUS_AUTO
#define V4L2_CID_FOCUS_AUTO           (V4L2_CID_PRIVATE_BASE + 4)
#endif
#ifndef V4L2_CID_FOCUS_ABSOLUTE
#define V4L2_CID_FOCUS_ABSOLUTE       (V4L2_CID_PRIVATE_BASE + 5)
#endif
#ifndef V4L2_CID_FOCUS_RELATIVE
#define V4L2_CID_FOCUS_RELATIVE       (V4L2_CID_PRIVATE_BASE + 6)
#endif

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

#ifndef HAVE_LIBV4L
        void jpeg_to_planar420(char *, const char*);
#endif

        void setctrl(int, int, const char *, int);

        struct v4l2_capability   capability;
        struct v4l2_input        *inputs;
        struct v4l2_input        input;
        struct v4l2_queryctrl    qctrl;
        struct v4l2_format       fmt;
        struct v4l2_requestbuffers req;

        /* mmap */
        int                      have_mmap;
        VIMAGE                   vimage[STREAMBUFS];
        struct v4l2_buffer       tempbuf;
        int                      buf_sync;

        __u32   pixelformat;
        int fd_;
        int format_;
        int have_YUV422;
        int have_YUV422P;
        int have_YUV420P;
        int have_MJPEG;
        int have_JPEG;

        int cformat_;
        int port_;
        int norm_;

        unsigned char *tm_;
        int width_;
        int height_;
        int max_width_;
        int max_height_;
        int decimate_;
        int bytesused_;

#ifndef HAVE_LIBV4L
        struct jdec_private *jpegdec_;
#endif
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
        debug_msg("V4L2:  ==> %s\n",attr);
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
        struct v4l2_input        input;

        int  k,i,err,fd;
        char *nick, *attr;

        // VIC_DEVICE env variable selects V4L device with AGTk 3.02 and earlier
        // but doesn't work if V4L2 devices are listed before the V4L device
        // so don't list V4L2 devices if VIC_DEVICE env variable is set.
        const char *myDev = getenv("VIC_DEVICE");
        if (myDev != 0)
        {
                 return;
        }

        for (i = 0; dev[i] != NULL; i++) {
                debug_msg("V4L2: trying %s... ",dev[i]);
                if (-1 == (fd = v4l2_open(dev[i],O_RDWR))) {
                        debug_msg("Error opening: %s : %s\n", dev[i], strerror(errno));
                        continue;
                }
#ifdef HAVE_LIBV4L
                if (-1 == v4l2_fd_open(fd,V4L2_ENABLE_ENUM_FMT_EMULATION)) {
                        perror("V4L2: v4l2_fd_open V4L2_ENABLE_ENUM_FMT_EMULATION");
                        v4l2_close(fd);
                        continue;
                }
#endif
                memset(&capability,0,sizeof(capability));
                if (-1 == v4l2_ioctl(fd,VIDIOC_QUERYCAP,&capability)) {
                        perror("V4L2: ioctl VIDIOC_QUERYCAP");
                        v4l2_close(fd);
                        continue;
                }

                if ((capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
                        debug_msg("%s, %s can't capture\n",capability.card,capability.bus_info);
                        v4l2_close(fd);
                        continue;
                }

                attr = new char[512];
                strcpy(attr,"format { 420 422 cif } ");
                strcat(attr,"size { small large cif } ");

                debug_msg("V4L2:   ports:");
                strcat(attr,"port { ");
                //input.index = 0;
                memset(&input, 0, sizeof(input));
                while (-1 != v4l2_ioctl(fd, VIDIOC_ENUMINPUT, &input)) {
                        if (-1 == v4l2_ioctl(fd,VIDIOC_S_INPUT,&input.index)) {
                                debug_msg("ioctl VIDIOC_S_INPUT: %s", strerror(errno));
                        } else {

                                debug_msg(" %s: ", (char*)input.name);
                                for (unsigned int s=0 ; s<strlen((char*)input.name) ; s++)
                                        if (input.name[s]==' ') input.name[s]='-';
                                strcat(attr,(const char*)input.name);
                                strcat(attr," ");
                        }
                        input.index++;
                }
                debug_msg("\n");
                strcat(attr,"} ");

                debug_msg("V4L2:   norms: ");
                strcat(attr,"type { ");

                for (k = 0, err = 0; err == 0; ++k)
                {
                        struct v4l2_standard     estd;
                        memset(&estd,0,sizeof(estd));
                        estd.index = k;
                        err = v4l2_ioctl(fd, VIDIOC_ENUMSTD, &estd);
                        if (!err) {
                                strcat(attr, (const char*)estd.name);
                                strcat(attr," ");
                                debug_msg("%s ", estd.name);
                        }
                }
                debug_msg("\n");

                strcat(attr,"} ");

                nick = new char[strlen((char*)capability.card)+strlen(dev[i])+7];
                sprintf(nick,"V4L2-%s %s",capability.card, dev[i]);
                new V4l2Device(dev[i],nick,attr);
                fprintf(stderr, "Attached to V4L2 device: %s\n", nick);
                v4l2_close(fd);
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
        have_YUV422 = 0;
        have_YUV422P = 0;
        have_YUV420P = 0;
        have_MJPEG = 0;
        have_JPEG = 0;

#ifndef HAVE_LIBV4L
        jpegdec_ = NULL;
#endif

        struct v4l2_capability cap;
        memset(&cap,0,sizeof(cap));
        if (-1 == v4l2_ioctl(fd_,VIDIOC_QUERYCAP,&cap)) {
                perror("ioctl VIDIOC_QUERYCAP");
        } else {
                if ( !(cap.capabilities & V4L2_CAP_READWRITE) && !(cap.capabilities & V4L2_CAP_STREAMING)) {
                        debug_msg("V4L2: fatal: device does not support read()/write() calls or streaming capture.\n");
                        status_=-1;
                        return;
                }
        }

        memset(&fmt,0,sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_ioctl(fd_, VIDIOC_G_FMT, &fmt);

        unsigned int test_width[] =  {CIF_WIDTH,  PAL_BT601_WIDTH/2,  NTSC_BT601_WIDTH/2,  VGA_WIDTH/2,  0};
        unsigned int test_height[] = {CIF_HEIGHT, PAL_BT601_HEIGHT/2, NTSC_BT601_HEIGHT/2, VGA_HEIGHT/2, 0};
        for (unsigned int i = 0; test_width[i] != 0; i++) {
                fmt.fmt.pix.width = test_width[i];
                fmt.fmt.pix.height = test_height[i];
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
                if (-1 != v4l2_ioctl(fd_, VIDIOC_S_FMT, &fmt) ) {
                        if (fmt.fmt.pix.height == test_height[i] && fmt.fmt.pix.width >= test_width[i] && fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420) {
                                have_YUV420P = 1;
                                debug_msg("Device supports V4L2_PIX_FMT_YUV420 capture at %dx%d\n",test_width[i],test_height[i]);
                        } else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV420) {
                                debug_msg("V4L2_PIX_FMT_YUV420 capture at %dx%d not supported, returned %dx%d\n",test_width[i],test_height[i],fmt.fmt.pix.width,fmt.fmt.pix.height);
                        }
                }

                fmt.fmt.pix.width = test_width[i];
                fmt.fmt.pix.height = test_height[i];
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV422P;
                if (-1 != v4l2_ioctl(fd_, VIDIOC_S_FMT, &fmt) ) {
                        if (fmt.fmt.pix.height == test_height[i] && fmt.fmt.pix.width >= test_width[i] && fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUV422P) {
                                have_YUV422P = 1;
                                debug_msg("Device supports V4L2_PIX_FMT_YUV422 capture at %dx%d\n",test_width[i],test_height[i]);
                        } else {
                                debug_msg("V4L2_PIX_FMT_YUV422 capture at %dx%d not supported, returned %dx%d\n",test_width[i],test_height[i],fmt.fmt.pix.width,fmt.fmt.pix.height);
                        }
                }

                fmt.fmt.pix.width = test_width[i];
                fmt.fmt.pix.height = test_height[i];
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
                if (-1 != v4l2_ioctl(fd_, VIDIOC_S_FMT, &fmt) ) {
                        if (fmt.fmt.pix.height == test_height[i] && fmt.fmt.pix.width >= test_width[i]) {
                                have_YUV422 = 1;
                                debug_msg("Device supports V4L2_PIX_FMT_YUYV (YUV 4:2:2) capture at %dx%d\n",test_width[i],test_height[i]);
                        } else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) {
                                debug_msg("V4L2_PIX_FMT_YUYV (YUV 4:2:2) capture at %dx%d not supported, returned %dx%d\n",test_width[i],test_height[i],fmt.fmt.pix.width,fmt.fmt.pix.height);
                        }
                }

#ifdef HAVE_LIBV4L
                fmt.fmt.pix.width = test_width[i];
                fmt.fmt.pix.height = test_height[i];
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
                if (-1 != v4l2_ioctl(fd_, VIDIOC_S_FMT, &fmt) ) {
                        if (fmt.fmt.pix.height == test_height[i] && fmt.fmt.pix.width >= test_width[i] && fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG) {
                                have_MJPEG = 1;
                                debug_msg("Device supports V4L2_PIX_FMT_MJPEG capture at %dx%d\n",test_width[i],test_height[i]);
                        } else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG) {
                                debug_msg("V4L2_PIX_FMT_MJPEG capture at %dx%d not supported, returned %dx%d\n",test_width[i],test_height[i],fmt.fmt.pix.width,fmt.fmt.pix.height);
                        }
                }

                fmt.fmt.pix.width = test_width[i];
                fmt.fmt.pix.height = test_height[i];
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
                if (-1 != v4l2_ioctl(fd_, VIDIOC_S_FMT, &fmt) ) {
                        if (fmt.fmt.pix.height == test_height[i] && fmt.fmt.pix.width >= test_width[i] && fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_JPEG) {
                                have_MJPEG = 1;
                                debug_msg("Device supports V4L2_PIX_FMT_JPEG capture at %dx%d\n",test_width[i],test_height[i]);
                        } else if (fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_JPEG) {
                                debug_msg("V4L2_PIX_FMT_JPEG capture at %dx%d not supported, returned %dx%d\n",test_width[i],test_height[i],fmt.fmt.pix.width,fmt.fmt.pix.height);
                        }
                }
#endif
        }

        if( !( have_YUV422P || have_YUV422 || have_YUV420P || have_MJPEG || have_JPEG)){
                debug_msg("No suitable pixelformat found\n");
                v4l2_close(fd_);
                status_=-1;
                return;
        }

        struct v4l2_requestbuffers reqbuf;
        memset(&reqbuf,0,sizeof(reqbuf));
        reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        reqbuf.memory = V4L2_MEMORY_MMAP;
        reqbuf.count = 20;
        have_mmap = 1;
        if (-1 == v4l2_ioctl(fd_, VIDIOC_REQBUFS, &reqbuf)) {
                if (errno == EINVAL) {
                        printf("Video capturing or mmap-streaming is not supported\n");
                        have_mmap = 0;
                        status_=-1;
                } else {
                        perror ("VIDIOC_REQBUFS");
                        have_mmap = 0;
                }
        }

        /* fill in defaults */
        if(!strcmp(cformat, "420"))
                cformat_ = CF_420;
        if(!strcmp(cformat, "422"))
                cformat_ = CF_422;
        if(!strcmp(cformat, "cif"))
                cformat_ = CF_CIF;

        port_      = 0;
        norm_      = 0;
        decimate_  = 2;
        running_   = 0;

}


V4l2Grabber::~V4l2Grabber()
{
        int i;
        debug_msg("V4L2: destructor\n");

        if (have_mmap) {
                for (i = 0; i < STREAMBUFS; ++i) {
                        if (vimage[i].data)
                                v4l2_munmap(vimage[i].data,
                                            vimage[i].vidbuf.length);
                        vimage[i].data = NULL;
                }
        } else {
                if (vimage[0].data)
                        free(vimage[0].data);
                vimage[0].data = NULL;
        }
        v4l2_close(fd_);

#ifndef HAVE_LIBV4L
        if (jpegdec_) {
                tinyjpeg_free(jpegdec_);
        }
#endif

}

int V4l2Grabber::command(int argc, const char*const* argv)
{
        int i, err;

        if (argc == 3) {
                if (strcmp(argv[1], "decimate") == 0) {
                        decimate_ = atoi(argv[2]);

                        if (running_) {
                                stop(); start();
                        }
                        return (TCL_OK);
                }

                if (strcmp(argv[1], "port") == 0) {

                        for (i = 0, err = 0; err == 0; ++i) {
                                struct v4l2_input inp;
                                memset(&inp,0,sizeof(inp));
                                inp.index = i;
                                err = v4l2_ioctl(fd_, VIDIOC_ENUMINPUT, &inp);
                                if (!err) {

                                        char input[32];
                                        unsigned int s;
                                        for ( s=0 ; s<= strlen((const char*)inp.name) ; s++ )
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

                if (strcmp(argv[1], "gamma") == 0) {
                        setctrl(atoi(argv[2]), V4L2_CID_GAMMA, "Gamma", 0);
                        return (TCL_OK);
                }

                if (strcmp(argv[1], "gain") == 0) {
                        setctrl(atoi(argv[2]), V4L2_CID_GAIN, "Gain", 0);
                        return (TCL_OK);
                }

                if (strcmp(argv[1], "antiflicker") == 0) {
                        struct v4l2_queryctrl qctrl;
                        struct v4l2_control ctrl;

                        memset (&qctrl, 0, sizeof(qctrl));
                        qctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
                        if (-1 != v4l2_ioctl(fd_, VIDIOC_QUERYCTRL, &qctrl)) {
                                if (strcmp((char *)qctrl.name, "Power Line Frequency") == 0) {
                                        ctrl.id = qctrl.id;
                                        ctrl.value = atoi(argv[2]);
                                        if (-1 == v4l2_ioctl(fd_, VIDIOC_S_CTRL, &ctrl))
                                                perror("ioctl  VIDIOC_S_CTRL");
                                        else
                                                debug_msg( "V4L2: V4L2_CID_POWER_LINE_FREQUENCY = %d\n", ctrl.value);
                                 }
                        }
                        return (TCL_OK);
                }

                if (strcmp(argv[1], "controls") == 0) {
                        if (strcmp(argv[2], "reset") == 0) {
                                debug_msg( "V4L2: Resetting controls\n");

                                setctrl(atoi(argv[2]), V4L2_CID_BRIGHTNESS, "Brightness", 1);
                                setctrl(atoi(argv[2]), V4L2_CID_HUE, "Hue", 1);
                                setctrl(atoi(argv[2]), V4L2_CID_CONTRAST, "Contrast", 1);
                                setctrl(atoi(argv[2]), V4L2_CID_SATURATION, "Saturation", 1);
                                setctrl(atoi(argv[2]), V4L2_CID_GAMMA, "Gamma", 1);
                                setctrl(atoi(argv[2]), V4L2_CID_GAIN, "Gain", 1);
                                return (TCL_OK);
                        }
                }

                if (strcmp(argv[1], "fps") == 0) {
                        debug_msg("V4L2: fps %s\n",argv[2]);
                }


                if (strcmp(argv[1], "type") == 0 || strcmp(argv[1], "format") == 0) {

                        for (int k = 0, err = 0; err == 0; ++k)
                        {
                                struct v4l2_standard     estd;
                                memset(&estd,0,sizeof(estd));
                                estd.index = k;
                                if ( !(err = v4l2_ioctl(fd_, VIDIOC_ENUMSTD, &estd)) )
                                        if ( strcasecmp(argv[2], (const char *)estd.name) == 0)
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
                debug_msg("\nV4L2: start\n");

                format();

                if (have_mmap) {
                        buf_sync = 1;

                        memset(&req, 0, sizeof(req));
                        req.count = STREAMBUFS;
                        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        req.memory = V4L2_MEMORY_MMAP;
                        err = v4l2_ioctl(fd_, VIDIOC_REQBUFS, &req);
                        if (err < 0 || req.count < 1) {
                                debug_msg("REQBUFS returned error %d, count %d\n", errno,req.count);
                                return;
                        }

                        for (i = 0; i < (int)req.count; ++i) {
                                vimage[i].vidbuf.index = i;
                                vimage[i].vidbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                                vimage[i].vidbuf.memory = V4L2_MEMORY_MMAP;
                                err = v4l2_ioctl(fd_, VIDIOC_QUERYBUF, &vimage[i].vidbuf);
                                if (err < 0) {
                                        debug_msg("QUERYBUF returned error %d\n", errno);
                                        return;
                                }
                                vimage[i].data = (typeof(vimage[0].data)) v4l2_mmap(0,  vimage[i].vidbuf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd_, vimage[i].vidbuf.m.offset);

                                if ((long)vimage[i].data == -1) {
                                        debug_msg("V4L2: mmap() returned error %d\n", errno);
                                        return;
                                } else debug_msg("V4L2: mmap()'ed buffer at 0x%lx (%u bytes)\n", (unsigned long)vimage[i].data, vimage[i].vidbuf.length);
                        }

                        for (i = 0; i < (int)req.count; ++i)
                                if ((err = v4l2_ioctl(fd_, VIDIOC_QBUF, &vimage[i].vidbuf))) {
                                        debug_msg("QBUF returned error %d\n",errno);
                                        return;
                                }

                        err = v4l2_ioctl(fd_, VIDIOC_STREAMON, &vimage[0].vidbuf.type);
                        if (err)
                                debug_msg("STREAMON returned error %d\n",errno);

                } else {
                        vimage[0].data = (typeof(vimage[0].data)) malloc(fmt.fmt.pix.sizeimage);

                        if (vimage[0].data == NULL) {
                                debug_msg("malloc(%d) failed\n", fmt.fmt.pix.sizeimage);
                                return;
                        } else debug_msg("V4L2: malloc()'ed buffer (%d bytes)\n", fmt.fmt.pix.sizeimage);
                }

                Grabber::start();
                running_ = 1;
        }
}

void V4l2Grabber::stop()
{
        debug_msg("V4L2: stop\n");
        int i, err;

        if (have_mmap) {
                if ( fd_ > 0 ) {
                        i = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        if ( (err = v4l2_ioctl(fd_, VIDIOC_STREAMOFF, &i) ) )
                                debug_msg("V4L2: VIDIOC_STREAMOFF failed\n");
                }

                tempbuf.type = vimage[0].vidbuf.type;
                while ((err = v4l2_ioctl(fd_, VIDIOC_DQBUF, &tempbuf)) < 0 &&
                       (errno == EINTR)) ;

                if (err < 0) {
                        debug_msg("V4L2: VIDIOC_DQBUF failed: %s\n", strerror(errno));
                }

                for (i = 0; i < STREAMBUFS; ++i) {
                        if (vimage[i].data)
                                err = v4l2_munmap(vimage[i].data, vimage[i].vidbuf.length);
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

        bytesused_ = 0;
        if (have_mmap) {
                FD_ZERO(&rdset);
                FD_SET(fd_, &rdset);
                timeout.tv_sec = 0;
                timeout.tv_usec = 0;
                n = select(fd_+1, &rdset, NULL, NULL, &timeout);

                if (n == 0)
                        return (0);
                else if (FD_ISSET(fd_, &rdset)) {
                        memset(&tempbuf, 0, sizeof(struct v4l2_buffer));
                        tempbuf.type = vimage[0].vidbuf.type;
                        tempbuf.memory = vimage[0].vidbuf.memory;
                        if (-1 == v4l2_ioctl(fd_, VIDIOC_DQBUF, &tempbuf))
                                perror("ioctl  VIDIOC_DQBUF");

                        if (  (req.count > 1) ) buf_sync++;
                        fr = vimage[buf_sync%2].data;
                        bytesused_ = tempbuf.bytesused;

                }

        } else {
                fr = vimage[0].data;
                v4l2_read(fd_, vimage[0].data, fmt.fmt.pix.sizeimage);
                bytesused_ = fmt.fmt.pix.sizeimage;
        }

        switch (cformat_) {
        case CF_420:
        case CF_CIF:
                if (have_YUV420P)
                       planarYUYV420_to_planarYUYV420((char *)frame_, outw_, outh_, fr, inw_, inh_);
                else if (have_YUV422)
                       packedYUYV422_to_planarYUYV420((char *)frame_, outw_, outh_, fr, inw_, inh_);
                else if (have_YUV422P)
                       planarYUYV422_to_planarYUYV420((char *)frame_, outw_, outh_, fr, inw_, inh_);
#ifndef HAVE_LIBV4L
                else if (have_MJPEG || have_JPEG) {
                       jpeg_to_planar420((char*)frame_,fr);
                }
#endif
                break;

        case CF_422:
                if (have_YUV422P)
                       planarYUYV422_to_planarYUYV422((char *)frame_, outw_, outh_, fr, inw_, inh_);
                else if (have_YUV422)
                       packedYUYV422_to_planarYUYV422((char *)frame_, outw_, outh_, fr, inw_, inh_);
                else if (have_YUV420P)
                       planarYUYV420_to_planarYUYV422((char *)frame_, outw_, outh_, fr, inw_, inh_);

#ifndef HAVE_LIBV4L
                else if (have_MJPEG  || have_JPEG)
                       // jpeg_to_planar422((char*)frame_,fr);
#endif
                break;
        }

        if (have_mmap)
                v4l2_ioctl(fd_, VIDIOC_QBUF, &vimage[buf_sync%2].vidbuf);

        suppress(frame_);
        saveblks(frame_);
        YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
        return (target_->consume(&f));
}

#ifndef HAVE_LIBV4L
void V4l2Grabber::jpeg_to_planar420(char *dest, const char *src)
{
        char *y,*u,*v;
        unsigned char *components[4];

        y = dest;
        u = y + width_ * height_;
        v = u + width_ * height_ /4;

        components[0] = (unsigned char *) y;
        components[1] = (unsigned char *) u;
        components[2] = (unsigned char *) v;

        if (jpegdec_ == NULL) {
                jpegdec_ = tinyjpeg_init();
                if (jpegdec_ == NULL) {
                        return;
                }
                tinyjpeg_set_flags(jpegdec_, TINYJPEG_FLAGS_MJPEG_TABLE);
        }
        tinyjpeg_set_components(jpegdec_, components, 4);
        if (tinyjpeg_parse_header(jpegdec_, (unsigned char *)src, bytesused_) < 0) {
               debug_msg("V4L2 Jpeg error: %s", tinyjpeg_get_errorstring(jpegdec_));
               return;
        }
        if (tinyjpeg_decode(jpegdec_, TINYJPEG_FMT_YUV420P) < 0) {
               debug_msg("V4L2 Jpeg error: %s", tinyjpeg_get_errorstring(jpegdec_));
               return;
        }
}
#endif


void V4l2Grabber::format()
{

        struct v4l2_standard    standard;

        int i, err;
        int input;
        int format_ok = 0;
        int capture_standard = CS_VC; // initially try video conferencing resolutions

        switch (cformat_) {
        case CF_420:
        case CF_CIF:
                if( have_YUV420P )
                       pixelformat = V4L2_PIX_FMT_YUV420;
                else if( have_YUV422 )
                       pixelformat = V4L2_PIX_FMT_YUYV;
#ifndef HAVE_LIBV4L
                else if( have_MJPEG )
                       pixelformat = V4L2_PIX_FMT_MJPEG;
                else if( have_JPEG )
                       pixelformat = V4L2_PIX_FMT_JPEG;
#endif
                break;

        case CF_422:
                if (have_YUV422P) 
                       pixelformat = V4L2_PIX_FMT_YUV422P;
                else if( have_YUV422 )
                       pixelformat = V4L2_PIX_FMT_YUYV;
                else if( have_YUV420P )
                       pixelformat = V4L2_PIX_FMT_YUV420;
#ifndef HAVE_LIBV4L
                else if( have_MJPEG )
                       pixelformat = V4L2_PIX_FMT_MJPEG;
                else if( have_JPEG )
                       pixelformat = V4L2_PIX_FMT_JPEG;
#endif
                break;
        }

        while ( !format_ok ) {
                if (capture_standard == CS_VC) {
                        width_  = CIF_WIDTH  *2  / decimate_;
                        height_ = CIF_HEIGHT *2  / decimate_;
                }

                debug_msg("V4L2: format");
                switch (cformat_) {
                case CF_CIF:
                        set_size_cif(width_, height_);
                        debug_msg(" cif\n");
                        break;
                case CF_420:
                        set_size_420(width_, height_);
                        debug_msg(" 420\n");
                        break;
                case CF_422:
                        set_size_422(width_, height_);
                        debug_msg(" 422\n");
                        break;
                }
                debug_msg("decimate: %d\n",decimate_);

                memset(&standard,0,sizeof(standard));
                standard.index = norm_;
                err = v4l2_ioctl(fd_, VIDIOC_ENUMSTD, &standard);
                if (!err) {
                        if (-1 == v4l2_ioctl(fd_, VIDIOC_S_STD, &standard.id))
                                perror("ioctl VIDIOC_S_STD");
                        else debug_msg("V4L2: setting norm to %s\n",standard.name);
                }

                input = port_;
                if ((err = v4l2_ioctl(fd_, VIDIOC_S_INPUT, &input)) )
                        debug_msg("S_INPUT returned error %d\n",errno);
                else debug_msg("V4L2: setting input port to %d\n", port_);

                for (i = 0, err = 0; err == 0; ++i) {
                        struct v4l2_fmtdesc     fmtd;
                        memset(&fmtd,0,sizeof(fmtd));
                        fmtd.index = i;
                        fmtd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        err = v4l2_ioctl(fd_, VIDIOC_ENUM_FMT, &fmtd);
                        if (err) {
                                debug_msg("V4L2: VIDIOC_ENUM_FMT returned unexpected EINVAL error code\n");
                                debug_msg("V4L2: giving up ...\n");
                                format_ok = 1;

                        } else {
                                if (fmtd.pixelformat == pixelformat) {
                                        memset(&fmt,0,sizeof(fmt));
                                        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                                        v4l2_ioctl(fd_, VIDIOC_G_FMT, &fmt);
                                        fmt.fmt.pix.width = width_;
                                        fmt.fmt.pix.height = height_;
                                        fmt.fmt.pix.field = V4L2_FIELD_ANY;
                                        fmt.fmt.pix.pixelformat = pixelformat;

                                        if ( (err = v4l2_ioctl(fd_, VIDIOC_S_FMT, &fmt) ) )
                                                debug_msg("\nV4L2: Failed to set format\n");

                                        /* Clip horizontally (if necessary) capture resolutions that satisfy the
                                         * vertical resolution, but ignore resolutions that are very wide and most
                                         * likely not square pixels
                                         */
                                        if ( (fmt.fmt.pix.width > (unsigned int)width_ ) &&
                                                               (fmt.fmt.pix.height == (unsigned int)height_) &&
                                                               (fmt.fmt.pix.width < (2 * fmt.fmt.pix.height)) ) {
                                                inw_ = width_ = fmt.fmt.pix.width;
                                                debug_msg("V4L2: will clip input width=%d to %d\n", inw_, outw_);
                                        }

                                        if ( ( fmt.fmt.pix.width == (unsigned int)width_ ) &&
                                                                ( fmt.fmt.pix.height == (unsigned int)height_ ) )  {
                                                debug_msg("V4L2: setting format: width=%d height=%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
                                                format_ok = 1;
                                                break;
                                        } else {

                                                debug_msg("V4L2: failed to set format! requested %dx%d, got %dx%d\n", width_, height_, fmt.fmt.pix.width, fmt.fmt.pix.height);

                                                switch(decimate_) {
                                                case 1:
                                                        switch (capture_standard) {
                                                        case CS_VC :
                                                                debug_msg("V4L2: trying VGA resolution ...\n");
                                                                width_ = VGA_WIDTH;
                                                                height_ = VGA_HEIGHT;
                                                                capture_standard = CS_VGA;
                                                                break;
                                                        case CS_VGA :
                                                                debug_msg("V4L2: trying ITU-R BT.601 NTSC resolution ...\n");
                                                                width_ = NTSC_BT601_WIDTH;
                                                                height_ = NTSC_BT601_HEIGHT;
                                                                capture_standard = CS_BT601_NTSC;
                                                                break;
                                                        case CS_BT601_NTSC :
                                                                debug_msg("V4L2: trying ITU-R BT.601 PAL resolution ...\n");
                                                                width_ = PAL_BT601_WIDTH;
                                                                height_ = PAL_BT601_HEIGHT;
                                                                capture_standard = CS_BT601_PAL;
                                                                break;
                                                        default :
                                                                debug_msg("V4L2: trying resolution under ...\n");
                                                                decimate_ = 2;
                                                                capture_standard = CS_VC;
                                                        }
                                                        break;
                                                case 2:
                                                        switch (capture_standard) {
                                                        case CS_VC :
                                                                debug_msg("V4L2: trying 1/4 VGA resolution ...\n");
                                                                width_ = VGA_WIDTH / 2;
                                                                height_ = VGA_HEIGHT / 2;
                                                                capture_standard = CS_VGA;
                                                                break;
                                                        case CS_VGA :
                                                                debug_msg("V4L2: trying 1/4 ITU-R BT.601 NTSC resolution ...\n");
                                                                width_ = NTSC_BT601_WIDTH / 2;
                                                                height_ = NTSC_BT601_HEIGHT / 2;
                                                                capture_standard = CS_BT601_NTSC;
                                                                break;
                                                        case CS_BT601_NTSC :
                                                                debug_msg("V4L2: trying 1/4 ITU-R BT.601 PAL resolution ...\n");
                                                                width_ = PAL_BT601_WIDTH / 2;
                                                                height_ = PAL_BT601_HEIGHT / 2;
                                                                capture_standard = CS_BT601_PAL;
                                                                break;
                                                        default :
                                                                debug_msg("V4L2: trying resolution under ...\n");
                                                                decimate_ = 4;
                                                                capture_standard = CS_VC;
                                                        }
                                                        break;
                                                default:
                                                        switch (capture_standard) {
                                                        case CS_VC :
                                                                debug_msg("V4L2: trying 1/16 VGA resolution ...\n");
                                                                width_ = VGA_WIDTH / 4;
                                                                height_ = VGA_HEIGHT / 4;
                                                                capture_standard = CS_VGA;
                                                                break;
                                                        case CS_VGA :
                                                                debug_msg("V4L2: trying 1/16 ITU-R BT.601 NTSC resolution ...\n");
                                                                width_ = NTSC_BT601_WIDTH / 4;
                                                                height_ = NTSC_BT601_HEIGHT / 4;
                                                                capture_standard = CS_BT601_NTSC;
                                                                break;
                                                        case CS_BT601_NTSC :
                                                                debug_msg("V4L2: trying 1/16 ITU-R BT.601 PAL resolution ...\n");
                                                                width_ = PAL_BT601_WIDTH / 4;
                                                                height_ = PAL_BT601_HEIGHT / 4;
                                                                capture_standard = CS_BT601_PAL;
                                                                break;
                                                        default :
                                                                debug_msg("V4L2: giving up ...\n");
                                                                format_ok = 1;
                                                        }
                                                }
                                                break; // inner for loop
                                        }
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
        if (-1 != v4l2_ioctl(fd_, VIDIOC_QUERYCTRL, &qctrl)) {
                if ( !(qctrl.flags & V4L2_CTRL_FLAG_DISABLED) ) {

                        ctrl.id = cid;
                        if ( reset )
                                ctrl.value = qctrl.default_value;
                        else
                                ctrl.value = qctrl.minimum + (qctrl.maximum - qctrl.minimum) * val/256;
                        if (-1 == v4l2_ioctl(fd_, VIDIOC_S_CTRL,&ctrl))
                                perror("ioctl  VIDIOC_S_CTRL");
                        else debug_msg( "V4L2: %s = %d\n", controlname, val);
                }
        }
        return;
}
