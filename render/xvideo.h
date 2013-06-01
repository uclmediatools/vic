/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,Boston MA
 * 02110-1301, USA.
 */

#ifndef _XVIDEO_H_
#define _XVIDEO_H_

#include "config_arch.h"

#include "vic_tcl.h"
extern "C"{
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <tk.h>
}

// XShm.h should be included before Xvlib.h
#ifdef USE_SHM
#   include <X11/extensions/XShm.h>
#endif

#ifdef HAVE_XVIDEO
#   include <X11/extensions/Xv.h>
#   include <X11/extensions/Xvlib.h>
#endif

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef uint32_t vlc_fourcc_t;

#ifdef HAVE_XVIDEO
#   define IMAGE_TYPE     XvImage
#   define EXTRA_ARGS     int i_xvport, vlc_fourcc_t i_chroma, int i_bits_per_pixel
#   define EXTRA_ARGS_SHM int i_xvport, vlc_fourcc_t i_chroma, XShmSegmentInfo *p_shm
#   define DATA_SIZE(p)   (p)->data_size
#   define IMAGE_FREE     XFree      /* There is nothing like XvDestroyImage */
#else
#   define IMAGE_TYPE     XImage
#   define EXTRA_ARGS     Visual *p_visual, int i_depth, int i_bytes_per_pixel
#   define EXTRA_ARGS_SHM Visual *p_visual, int i_depth, XShmSegmentInfo *p_shm
#   define DATA_SIZE(p)   ((p)->bytes_per_line * (p)->height)
#   define IMAGE_FREE     XDestroyImage
#endif

class XRender{
  public:
    XRender();
    ~XRender();
    int init(Display *_dpy, vlc_fourcc_t _chroma, Visual *_p_visual=NULL, int _depth=0, int _bytes_per_rgb=3);
    IMAGE_TYPE* createImage(int _width, int _height);
    void displayImage(Window video_window, GC gc, int _o_width, int _o_height) const;    
    void release();
    bool enable_shm;

        
  protected:
    IMAGE_TYPE *yuv_image;  
    Display *display;
    int i_width, i_height;
    Visual *p_visual;
    int depth, bytes_per_rgb;
        
    // for Xvideo extension
    vlc_fourcc_t i_chroma;
    vlc_fourcc_t o_chroma;
    
    // for shared memory
    #ifdef USE_SHM
    XShmSegmentInfo  yuv_shminfo;
    #endif 

};


/*****************************************************************************
 * Fourcc definitions that we can handle internally
 *****************************************************************************/
 #define VLC_FOURCC( a, b, c, d ) \
        ( ((uint32_t)a) | ( ((uint32_t)b) << 8 ) \
           | ( ((uint32_t)c) << 16 ) | ( ((uint32_t)d) << 24 ) )     
#define X11_FOURCC( a, b, c, d ) \
        ( ((uint32_t)a) | ( ((uint32_t)b) << 8 ) \
           | ( ((uint32_t)c) << 16 ) | ( ((uint32_t)d) << 24 ) )
#define VLC2X11_FOURCC( i ) \
        X11_FOURCC( ((char *)&i)[0], ((char *)&i)[1], ((char *)&i)[2], \
                    ((char *)&i)[3] )
#define X112VLC_FOURCC( i ) \
        VLC_FOURCC( i & 0xff, (i >> 8) & 0xff, (i >> 16) & 0xff, \
                    (i >> 24) & 0xff )  	
                   
/* Packed RGB 15bpp, usually 0x7c00, 0x03e0, 0x001f */
#define FOURCC_RV15         VLC_FOURCC('R','V','1','5')

/* Packed RGB 16bpp, usually 0xf800, 0x07e0, 0x001f */
#define FOURCC_RV16         VLC_FOURCC('R','V','1','6')

/* Packed RGB 24bpp, usually 0x00ff0000, 0x0000ff00, 0x000000ff */
#define FOURCC_RV24         VLC_FOURCC('R','V','2','4')

/* Packed RGB 32bpp, usually 0x00ff0000, 0x0000ff00, 0x000000ff */
#define FOURCC_RV32         VLC_FOURCC('R','V','3','2')

/* Planar YUV 4:2:0, Y:U:V */
#define FOURCC_I420         VLC_FOURCC('I','4','2','0')
#define FOURCC_IYUV         VLC_FOURCC('I','Y','U','V')

/* Planar YUV 4:2:0, Y:V:U */
#define FOURCC_YV12         VLC_FOURCC('Y','V','1','2')

/* Packed YUV 4:2:2, U:Y:V:Y, interlaced */
#define FOURCC_IUYV         VLC_FOURCC('I','U','Y','V')

/* Packed YUV 4:2:2, U:Y:V:Y */
#define FOURCC_UYVY         VLC_FOURCC('U','Y','V','Y')
#define FOURCC_UYNV         VLC_FOURCC('U','Y','N','V')
#define FOURCC_Y422         VLC_FOURCC('Y','4','2','2')

/* Packed YUV 4:2:2, U:Y:V:Y, reverted */
#define FOURCC_cyuv         VLC_FOURCC('c','y','u','v')

/* Packed YUV 4:2:2, Y:U:Y:V */
#define FOURCC_YUY2         VLC_FOURCC('Y','U','Y','2')
#define FOURCC_YUNV         VLC_FOURCC('Y','U','N','V')

/* Packed YUV 4:2:2, Y:V:Y:U */
#define FOURCC_YVYU         VLC_FOURCC('Y','V','Y','U')

/* Packed YUV 2:1:1, Y:U:Y:V */
#define FOURCC_Y211         VLC_FOURCC('Y','2','1','1')

/* Planar YUV 4:1:1, Y:U:V */
#define FOURCC_I411         VLC_FOURCC('I','4','1','1')

/* Planar YUV 4:1:0, Y:U:V */
#define FOURCC_I410         VLC_FOURCC('I','4','1','0')

/* Planar Y, packed UV, from Matrox */
#define FOURCC_YMGA         VLC_FOURCC('Y','M','G','A')

/* Planar 4:2:2, Y:U:V */
#define FOURCC_I422         VLC_FOURCC('I','4','2','2')

/* Planar 4:4:4, Y:U:V */
#define FOURCC_I444         VLC_FOURCC('I','4','4','4')


#endif
