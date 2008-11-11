/* =========================================================================

     Copyright (c) 2003 University of Sydney Vislab
     All rights reserved.

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

#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>

#include <stdexcept>

#include "grabber-linux1394.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "module.h"


//#define DEBUG(x)
#define DEBUG(x) (x)

// Get library to dump details where available
#define LIBDUMP 1

#define CIF_WIDTH   352
#define CIF_HEIGHT  288

// Possible DMA devices
static char *devs[] = {
    "/dev/video1394",
    "/dev/video1394/0",
    "/devfs/video1394/0",
    NULL
};

/* ----------------------------------------------------------------- */


Linux1394Grabber::Linux1394Grabber(const Camera *cam, const mode_t _mode)
  : camera(cam), byteorder(BYTE_ORDER_FIRST), fps(FRAMERATE_7_5), mode(_mode)
{
    DEBUG(fprintf(stderr, "Creating 1394 Grabber\n"));

    fprintf(stderr, "Linux1394: Probing DMA devices:\n");
    dev = getenv("VIC_DEVICE");
    if (dev) {
        fprintf(stderr, "   Testing user-defined device %s: ", dev);
        if (access(dev, R_OK) != 0) {
            fprintf(stderr, "\nLinux1394: Device test failed: %s\n",
                    strerror(errno));
            abort();
        }
        fprintf(stderr, "Passed\n");

    } else {
        for (char **i = devs; *i; i++) {

            fprintf(stderr, "   Testing device %s: ", *i);
            if (access(*i, R_OK) == 0) {
                fprintf(stderr, "Pass\n");
                dev = *i;
                break;
            }
            fprintf(stderr, "Fail (%s)\n", strerror(errno));
        }

        if (!dev) {
            fprintf(stderr, "Linux1394: Failed to find a valid DMA device.\n"
                    "  If the device is configured in non-standard place set\n"
                    "  the \"VIC_DEVICE\" environment variable.\n");
            abort();
        }

    }
}


Linux1394Grabber::~Linux1394Grabber()
{
    DEBUG(fprintf(stderr,"Linux1394: destructor\n"));
}


void Linux1394Grabber::start()
{
    DEBUG(fprintf(stderr,"Linux1394: start\n"));

    if (dc1394_dma_setup_capture(camera->handle, camera->id, 0, /* channel */
                                 FORMAT_VGA_NONCOMPRESSED,
                                 camera->best422,
                                 SPEED_400, fps,
                                 8, 1, dev, &capture)
        != DC1394_SUCCESS)
    {
        fprintf(stderr,"Linux1394: failed to setup capture\n");
        dc1394_dma_release_camera(camera->handle, &capture);
        abort();
    }

    switch (mode) {
      case MODE420:
        width = capture.frame_width;
        height = capture.frame_height;
        set_size_420(width, height);
        break;
      case MODE422:
        width = capture.frame_width;
        height = capture.frame_height;
        set_size_422(width, height);
        break;
      case MODECIF:
        width = capture.frame_width;
        height = capture.frame_height;
        set_size_cif(width, height);
        break;
    }

    allocref(); /* allocate reference frame */

    fprintf(stderr,"Linux1394: Started with resolution of %dx%d\n",
            width, height);

    if (dc1394_start_iso_transmission(camera->handle, capture.node)
        != DC1394_SUCCESS)
    {
        fprintf( stderr, "Linux1394: Unable to start camera capture\n");
        dc1394_dma_release_camera(camera->handle, &capture);
        abort();
    }

    Tcl &tcl = Tcl::instance();
    if ( tcl.attr("yuv_byteOrder") != NULL ) {
        DEBUG(fprintf(stderr,"Linux1394: Byteorder = %s\n", tcl.attr("yuv_byteOrder")));
        byteorder = (byteorder_t)atoi( tcl.attr("yuv_byteOrder") );
        if ((byteorder < BYTE_ORDER_FIRST) && (byteorder > BYTE_ORDER_LAST))
            byteorder = BYTE_ORDER_FIRST;
    }

    Grabber::start();
}

void Linux1394Grabber::stop()
{
    DEBUG(fprintf(stderr,"Linux1394: stop\n"));

    if (dc1394_stop_iso_transmission(camera->handle, capture.node)
        != DC1394_SUCCESS)
    {
        fprintf(stderr, "Linux1394: Couldn't stop the camera?\n");
    }

    dc1394_dma_release_camera(camera->handle, &capture);

    Grabber::stop();
}

int Linux1394Grabber::grab()
{
    // DEBUG(fprintf(stderr,"Linux1394: Grab\n"));

    if (dc1394_dma_single_capture(&capture) != DC1394_SUCCESS)
    {
        fprintf( stderr, "Linux1394: Unable to capture a frame\n");
        abort();
    }

    firewire422_to_planar422((char*)frame_, (char*)capture.capture_buffer);

    dc1394_dma_done_with_buffer(&capture);

    suppress(frame_);
    saveblks(frame_);
    YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);

    return (target_->consume(&f));
}


int Linux1394Grabber::command(int argc, const char*const* argv)
{
    DEBUG(fprintf(stderr,"Linux1394: Command %s\n", argv[1]));

    if (argc != 3)
        return (Grabber::command(argc, argv));

    if (strcmp(argv[1], "decimate") == 0) {
        DEBUG(fprintf(stderr,"Linux1394: Doing decimate %s\n",argv[2]));
        decimate = atoi(argv[2]);

    }

    if (strcmp(argv[1], "brightness") == 0) {
        u_char val = atoi(argv[2]);
        DEBUG(fprintf(stderr, "Linux1394: Doing Brightness = %d\n", val));

        // pict.brightness=val*256;

        return (TCL_OK);
    }

    if (strcmp(argv[1], "contrast") == 0) {
        u_char val = atoi(argv[2]);
        DEBUG(fprintf(stderr, "Linux1394: Doing Contrast = %d\n", val));

        // pict.contrast=val*256;

        return (TCL_OK);
    }

    if (strcmp(argv[1], "hue") == 0) {
        u_char val = atoi(argv[2]);
        DEBUG(fprintf(stderr, "Linux1394: Doing Hue = %d\n", val));

        // pict.hue=val*256;

        return (TCL_OK);
    }

    if (strcmp(argv[1], "saturation") == 0) {
        u_char val = atoi(argv[2]);
        DEBUG(fprintf(stderr, "Linux1394: Doing Saturation = %d\n", val));

        // pict.colour=val*256;

        return (TCL_OK);
    }

    if (strcmp(argv[1], "controls") == 0) {
        if (strcmp(argv[2], "reset") == 0) {
            DEBUG(fprintf(stderr, "Linux1394: Doing Factory Reset\n"));

            dc1394_init_camera(camera->handle, camera->id);
            return (TCL_OK);
        }
    }

    if (strcmp(argv[1], "yuv_byteorder") == 0) {
        DEBUG(fprintf(stderr,"Linux1394: Doing yuv_byteorder %s\n",argv[2]));
        byteorder = (byteorder_t)atoi(argv[2]);
        return (TCL_OK);
    }

    if (strcmp(argv[1], "fps") == 0) {
        int val = atoi(argv[2]);
        DEBUG(fprintf(stderr,"Linux1394: Doing DC1394 fps = %d\n", fps));

        // Pick closest supported FPS
        // FIXME: This could be better
        if (val < 3)
            fps = FRAMERATE_1_875;
        else if (val <= 4)
            fps = FRAMERATE_3_75;
        else if (val <= 10)
            fps = FRAMERATE_7_5;
        else if (val <=28)
            fps = FRAMERATE_15;
        else if (val <= 30)
            fps = FRAMERATE_30;
        else if (val <= 60)
            fps = FRAMERATE_60;
        else
            fps = FRAMERATE_7_5;  // Should be supported by most

        if (dc1394_set_video_framerate(camera->handle, camera->id,
                                       fps) != DC1394_SUCCESS)
        {
            fprintf(stderr,"Linux1394: Failed to set fps to %d\n", fps);
            return (TCL_OK);
        }
    }

    return (Grabber::command(argc, argv));

}


void Linux1394Grabber::firewire422_to_planar422(char *dest, char *src)
{
    int i;
    char *s, *y,*u,*v;
    unsigned int a, *srca;

    srca = (unsigned int *)src;

    i = (width * height)/2;
    y = dest;
    u = y + width * height;
    v = u + width * height / 2;

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
}

void Linux1394Grabber::firewire411_to_planar420(char *dest, char *src)
{
    int i;
    char *s, *y,*u,*v;
    unsigned int a, *srca;

    srca = (unsigned int *)src;

    i = (width * height)/2;
    y = dest;
    u = y + width * height;
    v = u + width * height / 2;

    while (--i) {
        a = *(srca++);
        *(u++) = a & 0xff;
        a >>= 8;
        *(y++) = a & 0xff;
        a >>= 8;
        *(y++) = a & 0xff;
        a >>= 8;
        *(v++) = a & 0xff;
        a >>= 8;
        *(y++) = a & 0xff;
        a >>= 8;
        *(y++) = a & 0xff;
    }
}

void Linux1394Grabber::firewire422_to_planar420(char *dest, char *src)
{
    int  a1,b;
    char *s, *y,*u,*v;
    unsigned int a, *srca;

    srca = (unsigned int *)src;

    y = dest;
    u = y + width * height;
    v = u + width * height / 4;

    for (a1 = height; a1 > 0; a1 -= 2) {
        for (b = width; b > 0; b -= 2) {
            a = *(srca++);
            *(u++) = a & 0xff; a >>= 8;
            *(y++) = a & 0xff; a >>= 8;
            *(v++) = a & 0xff; a >>= 8;
            *(y++) = a & 0xff;
        }
        for (b = width; b > 0; b -= 2) {
            a = *(srca++);
            *(y++) = a & 0xff; a >>= 16;
            *(y++) = a & 0xff;
        }
    }
}


/* ----------------------------------------------------------------- */


#define ERR(x,msg) { if (x == DC1394_FAILURE) { throw debug_msg(msg); } }


inline bool Camera::has_format(unsigned int fmt)
{
    return (dc1394_query_supported_modes(handle, id, fmt, &modes)
            == DC1394_SUCCESS);
};

inline bool Camera::has_mode(unsigned int mode, unsigned int min)
{
    return (modes & (0x1<<(31-(mode - min))));
};


Camera::Camera(const raw1394handle_t h, const nodeid_t i)
  : handle(h), id(i), best420(0), best422(0),
    mode420(false), mode422(false),
    small(false), medium(false), large(false)
{
    int err;

    if (dc1394_get_camera_info(handle, id, &info) == DC1394_FAILURE)
    	debug_msg("dc1394: Failed to get camera info");

    if (dc1394_get_camera_misc_info(handle, id, &misc)  == DC1394_FAILURE)
        debug_msg("dc1394: Failed to get camera misc info");

    if (dc1394_get_camera_feature_set(handle, id, &features) == DC1394_FAILURE)
        debug_msg("dc1394: Failed to get camera features");

    if (has_format(FORMAT_VGA_NONCOMPRESSED)) {
        DEBUG(fprintf(stderr, "Got VGA mode\n"));
        if (has_mode(MODE_320x240_YUV422, MODE_FORMAT0_MIN)) {
            small = mode422 = true;
            best422 = MODE_320x240_YUV422;
        }
        if (has_mode(MODE_640x480_YUV422, MODE_FORMAT0_MIN)) {
            small = medium = mode422 = true;
            //best422 = MODE_640x480_YUV422;  // FIXME: Framerate problems
        }
        if (has_mode(MODE_640x480_YUV411, MODE_FORMAT0_MIN)) {
            small = medium = mode420 = true;
            best420 = MODE_640x480_YUV411;
        }
    }

    if (has_format(FORMAT_SVGA_NONCOMPRESSED_1)) {
        DEBUG(fprintf(stderr, "Got SVGA1 mode\n"));
        if (has_mode(MODE_800x600_YUV422, MODE_FORMAT1_MIN)) {
            small = medium = large = mode422 = true;
            best422 = MODE_640x480_YUV422;
        }
        if (has_mode(MODE_1024x768_YUV422, MODE_FORMAT1_MIN)) {
            small = medium = large = mode422 = true;
            best422 = MODE_1024x768_YUV422;
        }
    }

    if (has_format(FORMAT_SVGA_NONCOMPRESSED_2)) {
        DEBUG(fprintf(stderr, "Got SVGA2 mode\n"));
        if (has_mode(MODE_1280x960_YUV422, MODE_FORMAT2_MIN)) {
            small = medium = large = mode422 = true;
            best422 = MODE_1280x960_YUV422;
        }
        if (has_mode(MODE_1600x1200_YUV422, MODE_FORMAT2_MIN)) {
            small = medium = large = mode422 = true;
            best422 = MODE_1600x1200_YUV422;
        }
    }

    if (has_format(FORMAT_STILL_IMAGE)) {
        DEBUG(fprintf(stderr, "Got Still mode\n"));
        // Not really any use to us.
    }
    if (has_format(FORMAT_SCALABLE_IMAGE_SIZE)) {
        DEBUG(fprintf(stderr, "Got Scalable mode\n"));
        // FIXME: Do we need to do this?  It's overkill for vic, and if we
        // have this we'll surely have the others?
    }

};


class Linux1394Device : public InputDevice {
public:
    Linux1394Device(const Camera *cam)
      : InputDevice(cam->info.model), camera(cam)
    {
        // Turn the probed modes into tcl statements.
        // This is sick, sick, sick.

        char *attr = new char[512];
        strcpy(attr, "format {");
        if (camera->mode420) strcat(attr, " 420 ");
        if (camera->mode422) strcat(attr, " 422 ");

        strcat(attr, "} size {");
        if (camera->small) strcat(attr, " small ");
        if (camera->medium) strcat(attr, " medium normal ");
        if (camera->large) strcat(attr, " large ");
        strcat(attr, "}");

        attributes_ = attr;
    };

    virtual int command(int argc, const char*const* argv)
    {
        fprintf(stderr, "Linux1394.device: Got command %s\n", argv[1]);

        Tcl& tcl = Tcl::instance();

        if ((argc == 3) && (strcmp(argv[1], "open") == 0)) {
            fprintf(stderr, "Linux1394: creating grabber with %s\n", argv[2]);

            mode_t mode;
            if (strcmp(argv[2], "420") == 0) {
                mode = MODE420;
            } else if (strcmp(argv[2], "422") == 0) {
                mode = MODE422;
            } else if (strcmp(argv[2], "cif") == 0) {
                mode = MODECIF;
            }

            TclObject* o = new Linux1394Grabber(camera, mode);
            if (o != 0) {
                fprintf(stderr, "Linux1394.device: Created\n");
                tcl.result(o->name());
            } else {
                fprintf(stderr, "Linux1394.device: Failed\n");
                abort();
            }
            return (TCL_OK);
        }
        return (InputDevice::command(argc, argv));
    };

  private:

    const Camera *camera;
};



static Linux1394Scanner scanner;

Linux1394Scanner::Linux1394Scanner()
{
    int  j,i,fd, err;
    char *nick, *attr;

    // Find how many OHCI devices we have
    raw1394handle_t handle = raw1394_new_handle();

    if (handle == (raw1394handle_t)NULL) {
      fprintf(stderr, "Failed to get raw1394 handle - No 1394 devices\n");
      return;
    }
      
    int numbus = raw1394_get_port_info(handle, NULL, 0);
    fprintf(stderr, "Found %d 1394 busses\n", numbus);
    raw1394_destroy_handle(handle);

    // Traverse the busses looking for cameras.  First a quick total count...
    for (int i = 0; i<numbus; i++) {
        raw1394handle_t handle = dc1394_create_handle(i);
        if (!handle) {
            fprintf(stderr, "Failed to get camera bus handle\n");
            continue;
        }

        int num = 0;
        nodeid_t *nodes = dc1394_get_camera_nodes(handle, &num, LIBDUMP);
        if (!nodes) {
            fprintf(stderr, "Failed to get camera nodes\n");
            continue;
        }

        for (int i=0; i<num; i++) {

            try {

                Camera *camera = new Camera(handle, nodes[i]);

                // This feels sooooo wrong ...
                new Linux1394Device(camera);

            } catch (const std::runtime_error &e) {
                fprintf(stderr, "Error getting camera info: %s", e.what());
            }
        }
        free(nodes);
    }
}


