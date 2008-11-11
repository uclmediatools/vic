/* =========================================================================

     Copyright (c) 2003 University of Sydney Vislab
     All rights reserved.

   ========================================================================= */

#ifndef _GRABBER_LINUX1394_H
#define _GRABBER_LINUX1394_H

#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>

#include "grabber.h"

//#define DEBUG(x)
#define DEBUG(x) (x)

// Get library to dump details where available
#define LIBDUMP 1

typedef enum {
    BYTE_ORDER_YUYV,
    BYTE_ORDER_YVYU,
    BYTE_ORDER_UYVY,
    BYTE_ORDER_VYUY
} byteorder_t;
#define BYTE_ORDER_FIRST BYTE_ORDER_YUYV
#define BYTE_ORDER_LAST BYTE_ORDER_VYUY

typedef enum {
    MODE420,
    MODE422,
    MODECIF
} modes_t;

// Intelligent struct for camera info
class Camera {
  public:
    Camera(const raw1394handle_t h, const nodeid_t i);

    raw1394handle_t handle;
    nodeid_t id;
    dc1394_camerainfo info;
    dc1394_feature_set features;
    dc1394_miscinfo misc;

    unsigned int best420;
    unsigned int best422;
    unsigned int bestcif;

    bool mode420, mode422,
         small, medium, large;

  private:

    bool has_format(unsigned int fmt);
    bool has_mode(unsigned int mode, unsigned int min);

    quadlet_t modes;

};


class Linux1394Grabber : public Grabber {
public:
    Linux1394Grabber(const Camera *cam, const mode_t _mode);
    virtual ~Linux1394Grabber();

    virtual int  command(int argc, const char*const* argv);
    virtual void start();
    virtual void stop();
    virtual int  grab();

 protected:

    void firewire411_to_planar420(char *dest, char *src);
    void firewire422_to_planar420(char *dest, char *src);
    void firewire422_to_planar422(char *dest, char *src);

    const Camera *camera;
    dc1394_cameracapture capture;

    char *dev;

    byteorder_t byteorder;

    unsigned int fps;
    mode_t mode;

    int width, height;
    int winwidth, winheight;
    bool scale;

    int decimate;

};


class Linux1394Scanner {
  public:
    Linux1394Scanner();

};

#endif // _GRABBER_LINUX1394_H

