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

#include <stdio.h>
#include <stdlib.h>

#ifdef USE_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#include "xvideo.h"

#include "color.h"
#include "rgb-converter.h"

extern "C"{
#include <tk.h>
}

IMAGE_TYPE *CreateImage    ( Display *, EXTRA_ARGS, int, int );
#ifdef USE_SHM
IMAGE_TYPE *CreateShmImage ( Display *, EXTRA_ARGS_SHM, int, int );
#endif

/* Function Definition */
#ifdef HAVE_XVIDEO
int  XVideoGetPort    ( Display *, vlc_fourcc_t , vlc_fourcc_t *);
static void XVideoReleasePort( Display *, int );
#endif

int vout_ChromaCmp( vlc_fourcc_t i_chroma, vlc_fourcc_t i_amorhc );

void DisplayVideo( Display *p_display, int i_xvport, Window video_window,
	GC gc, IMAGE_TYPE *p_image, int width, int height, int o_width, int o_height,
	bool shared_memory );

static int xv_port = -1;
// how many clients that use xv_port
static int num_xv_port = 0;

#ifdef HAVE_XVIDEO
/*****************************************************************************
 * XVideoGetPort: get YUV12 port
 *****************************************************************************/
int XVideoGetPort( Display *p_display,
                          vlc_fourcc_t i_chroma, vlc_fourcc_t *pi_newchroma )
{
    XvAdaptorInfo *p_adaptor;
    unsigned int i;
    unsigned int i_num_adaptors;
    int i_adaptor,  i_requested_adaptor;
    int i_selected_port;

    switch( XvQueryExtension( p_display, &i, &i, &i, &i, &i ) )
    {
        case Success:
            break;

        case XvBadExtension:
            printf("XvBadExtension\n");
            return -1;

        case XvBadAlloc:
            printf("XvBadAlloc\n");
            return -1;

        default:
            printf( "XvQueryExtension failed\n" );
            return -1;
    }

    switch( XvQueryAdaptors( p_display,
                             DefaultRootWindow( p_display ),
                             &i_num_adaptors, &p_adaptor ) )
    {
        case Success:
            break;

        case XvBadExtension:
            printf( "XvBadExtension for XvQueryAdaptors\n" );
            return -1;

        case XvBadAlloc:
            printf( "XvBadAlloc for XvQueryAdaptors\n" );
            return -1;

        default:
            printf( "XvQueryAdaptors failed\n" );
            return -1;
    }

    i_selected_port = -1;
    i_requested_adaptor = -1;

    for( i_adaptor = 0; i_adaptor < i_num_adaptors; ++i_adaptor )
    {
        XvImageFormatValues *p_formats;
        int i_format, i_num_formats;
        int i_port;

        /* If we requested an adaptor and it's not this one, we aren't
         * interested */
        if( i_requested_adaptor != -1 && i_adaptor != i_requested_adaptor )
        {
            continue;
        }

        /* If the adaptor doesn't have the required properties, skip it */
        if( !( p_adaptor[ i_adaptor ].type & XvInputMask ) ||
            !( p_adaptor[ i_adaptor ].type & XvImageMask ) )
        {
            continue;
        }

        /* Check that adaptor supports our requested format... */
        p_formats = XvListImageFormats( p_display,
                                        p_adaptor[i_adaptor].base_id,
                                        &i_num_formats );

        for( i_format = 0;
             i_format < i_num_formats && ( i_selected_port == -1 );
             i_format++ )
        {

/*
	    printf("  0x%x (%4.4s) %s\n",
	       p_formats[i_format].id,
	       (char *)&p_formats[i_format].id,
	       (p_formats[i_format].format == XvPacked) ? "packed" : "planar");
  */    

            /* If this is not the format we want, or at least a
             * similar one, forget it */
            if( !vout_ChromaCmp( p_formats[ i_format ].id, i_chroma ) )
            {
                continue;
            }

            // Look for the second available port supporting this format 
	    // Fist port might be used by X server
	    bool first=1;
            for( i_port = p_adaptor[i_adaptor].base_id;
                 ( i_port < (int)(p_adaptor[i_adaptor].base_id
                                   + p_adaptor[i_adaptor].num_ports) )
                   && ( i_selected_port == -1 );
                 i_port++ )
            {
                if( XvGrabPort( p_display, i_port, CurrentTime ) == Success )
                {
		    if(first){
 		      first=0;
		      continue;
		    }
                    i_selected_port = i_port;
                    *pi_newchroma = p_formats[ i_format ].id;
		    printf("using XVideo render\n");
                }
            }

            /* If no free port was found, forget it */
            if( i_selected_port == -1 )
            {
                continue;
            }
            
            // If we found a port, print information about it 
            debug_msg( "adaptor %i, port %i, format 0x%x (%4.4s) %s\n",
                     i_adaptor, i_selected_port, p_formats[ i_format ].id,
                     (char *)&p_formats[ i_format ].id,
                     ( p_formats[ i_format ].format == XvPacked ) ?
                         "packed" : "planar" );

#if 0
            XvEncodingInfo  *p_enc;
            unsigned int             i_enc, i_num_encodings;
            XvAttribute     *p_attr;
            int             i_attr, i_num_attributes;

            printf(  " encoding list:\n" );
	    
            if( XvQueryEncodings( p_display, i_selected_port,
                                  &i_num_encodings, &p_enc )
                 != Success )
            {
                printf(  "  XvQueryEncodings failed\n" );
                continue;
            }

            for( i_enc = 0; i_enc < i_num_encodings; i_enc++ )
            {
                printf( "  id=%ld, name=%s, size=%ldx%ld,"
                                      " numerator=%d, denominator=%d\n",
                             p_enc[i_enc].encoding_id, p_enc[i_enc].name,
                             p_enc[i_enc].width, p_enc[i_enc].height,
                             p_enc[i_enc].rate.numerator,
                             p_enc[i_enc].rate.denominator );
            }

            if( p_enc != NULL )
            {
                XvFreeEncodingInfo( p_enc );
            }

            printf(  " attribute list:\n" );
            p_attr = XvQueryPortAttributes( p_display,
                                            i_selected_port,
                                            &i_num_attributes );
            for( i_attr = 0; i_attr < i_num_attributes; i_attr++ )
            {
                printf( "  name=%s, flags=[%s%s ], min=%i, max=%i\n",
                      p_attr[i_attr].name,
                      (p_attr[i_attr].flags & XvGettable) ? " get" : "",
                      (p_attr[i_attr].flags & XvSettable) ? " set" : "",
                      p_attr[i_attr].min_value, p_attr[i_attr].max_value );
            }

            if( p_attr != NULL )
            {
                XFree( p_attr );
            }
#endif
        }

        if( p_formats != NULL )
        {
            XFree( p_formats );
        }

    }

    if( i_num_adaptors > 0 )
    {
        XvFreeAdaptorInfo( p_adaptor );
    }

    if( i_selected_port == -1 )
    {
        int i_chroma_tmp = X112VLC_FOURCC( i_chroma );
        if( i_requested_adaptor == -1 )
        {
            printf( "no free XVideo port found for format \n"
                      "0x%.8x (%4.4s)", i_chroma_tmp, (char*)&i_chroma_tmp );
        }
        else
        {
            printf( "XVideo adaptor %i does not have a free \n"
                      "XVideo port for format 0x%.8x (%4.4s)",
                      i_requested_adaptor, i_chroma_tmp, (char*)&i_chroma_tmp );
        }
    }

    return i_selected_port;
}
#endif 

// ======================================================================================
int vout_ChromaCmp( vlc_fourcc_t i_chroma, vlc_fourcc_t i_amorhc )
{
     /* If they are the same, they are the same ! */
     if( i_chroma == i_amorhc )
     {
         return 1;
     }
 
     /* Check for equivalence classes */
     switch( i_chroma )
     {
         case FOURCC_I420:
         case FOURCC_IYUV:
         case FOURCC_YV12:
             switch( i_amorhc )
             {
                 case FOURCC_I420:
                 case FOURCC_IYUV:
                 case FOURCC_YV12:
                     return 1;
 
                 default:
                     return 0;
             }
 
         case FOURCC_UYVY:
         case FOURCC_UYNV:
         case FOURCC_Y422:
             switch( i_amorhc )
             {
                 case FOURCC_UYVY:
                 case FOURCC_UYNV:
                 case FOURCC_Y422:
                     return 1;
 
                 default:
                     return 0;
             }
 
         case FOURCC_YUY2:
         case FOURCC_YUNV:
             switch( i_amorhc )
             {
                 case FOURCC_YUY2:
                 case FOURCC_YUNV:
                     return 1;
 
                 default:
                     return 0;
             }
 
         default:
             return 0;
     }
}

#ifdef HAVE_XVIDEO
/*****************************************************************************
 * XVideoReleasePort: release YUV12 port
 *****************************************************************************/
static void XVideoReleasePort( Display *p_display, int i_port )
{
    XvUngrabPort( p_display, i_port, CurrentTime );
}
#endif



#ifdef USE_SHM
/*****************************************************************************
 * CreateShmImage: create an XImage or XvImage using shared memory extension
 *****************************************************************************
 * Prepare an XImage or XvImage for display function.
 * The order of the operations respects the recommandations of the mit-shm
 * document by J.Corbet and K.Packard. Most of the parameters were copied from
 * there. See http://ftp.xfree86.org/pub/XFree86/4.0/doc/mit-shm.TXT
 *****************************************************************************/
IMAGE_TYPE * CreateShmImage( Display* p_display, EXTRA_ARGS_SHM,
                                    int i_width, int i_height )
{
    IMAGE_TYPE *p_image;

    /* Create XImage / XvImage */
#ifdef HAVE_XVIDEO
    p_image = XvShmCreateImage( p_display, i_xvport, VLC2X11_FOURCC(i_chroma), 0,
                                i_width, i_height, p_shm );
#else
    p_image = XShmCreateImage( p_display, p_visual, i_depth, ZPixmap, 0,
                               p_shm, i_width, i_height );
#endif
    if( p_image == NULL )
    {
        printf("image creation failed\n");
        return NULL;
    }

    /* Allocate shared memory segment - 0776 set the access permission
     * rights (like umask), they are not yet supported by all X servers */
    p_shm->shmid = shmget( IPC_PRIVATE, DATA_SIZE(p_image), IPC_CREAT | 0776 );
    if( p_shm->shmid < 0 )
    {
        printf("cannot allocate shared image data\n");
        IMAGE_FREE( p_image );
        return NULL;
    }

    /* Attach shared memory segment to process (read/write) */
    p_shm->shmaddr = p_image->data = (char*)shmat( p_shm->shmid, 0, 0 );
    if( !p_shm->shmaddr )
    {
        printf("cannot attach shared memory\n");
        IMAGE_FREE( p_image );
        shmctl( p_shm->shmid, IPC_RMID, 0 );
        return NULL;
    }

    /* Read-only data. We won't be using XShmGetImage */
    p_shm->readOnly = True;

    /* Attach shared memory segment to X server */
    if( XShmAttach( p_display, p_shm ) == False )
    {
        printf("cannot attach shared memory to X server\n" );
        IMAGE_FREE( p_image );
        shmctl( p_shm->shmid, IPC_RMID, 0 );
        shmdt( p_shm->shmaddr );
        return NULL;
    }

    /* Send image to X server. This instruction is required, since having
     * built a Shm XImage and not using it causes an error on XCloseDisplay,
     * and remember NOT to use XFlush ! */
    XSync( p_display, False );

#if 0
    /* Mark the shm segment to be removed when there are no more
     * attachements, so it is automatic on process exit or after shmdt */
    shmctl( p_shm->shmid, IPC_RMID, 0 );
#endif

    return p_image;
}
#endif

/*****************************************************************************
 * CreateImage: create an XImage or XvImage
 *****************************************************************************
 * Create a simple image used as a buffer.
 *****************************************************************************/
IMAGE_TYPE * CreateImage( Display *p_display, EXTRA_ARGS,
                                 int i_width, int i_height )
{
    uint8_t *    p_data;                           /* image data storage zone */
    IMAGE_TYPE *p_image;
#ifndef HAVE_XVIDEO
    int         i_quantum;                     /* XImage quantum (see below) */
    int         i_bytes_per_line;
#endif

    /* Allocate memory for image */
#ifdef HAVE_XVIDEO
    p_data = (uint8_t *) malloc( i_width * i_height * i_bits_per_pixel / 8 );
#else
    i_bytes_per_line = i_width * i_bytes_per_pixel;
    p_data = (uint8_t *) malloc( i_bytes_per_line * i_height );
#endif
    if( !p_data )
    {
        printf("out of memory\n" );
        return NULL;
    }

#ifndef HAVE_XVIDEO
    /* Optimize the quantum of a scanline regarding its size - the quantum is
       a diviser of the number of bits between the start of two scanlines. */
    /* Specifies the quantum of a scanline (8, 16, or 32). In other words, 
       the start of one scanline is separated in client memory from the start 
       of the next scanline by an integer multiple of this many bits.      */
       
    if( i_bytes_per_line & 0xf )  // 8
    {
        i_quantum = 0x8;
    }
    else if( i_bytes_per_line & 0x10 ) // 16
    {
        i_quantum = 0x10;
    }
    else  // 32
    {
        i_quantum = 0x20;
    }
#endif

    /* Create XImage. p_data will be automatically freed */
#ifdef HAVE_XVIDEO
    p_image = XvCreateImage( p_display, i_xvport, VLC2X11_FOURCC(i_chroma),
                             (char*)p_data, i_width, i_height );
#else
    p_image = XCreateImage( p_display, p_visual, i_depth, ZPixmap, 0,
                            (char*)p_data, i_width, i_height, i_quantum, 0 );
#endif
    if( p_image == NULL )
    {
        printf("XCreateImage() failed\n");
        free( p_data );
        return NULL;
    }

    return p_image;
 }

void DisplayVideo( Display *p_display, int i_xvport, Window video_window,
	GC gc, IMAGE_TYPE *p_image, int width, int height, int o_width, int o_height,
	bool shared_memory )
{


#ifdef USE_SHM
    if( shared_memory )
    {
        /* Display rendered image using shared memory extension */
#   ifdef HAVE_XVIDEO
        XvShmPutImage( p_display, i_xvport,
                       video_window,
                       gc, p_image,
                       0 /*src_x*/, 0 /*src_y*/,
                       width, height,
                       0 /*dest_x*/, 0 /*dest_y*/, o_width, o_height,
                       False /* Don't put True here or you'll waste your CPU */ );
#   else
        XShmPutImage( p_display,
                      video_window,
                      gc, p_image,
                      0 /*src_x*/, 0 /*src_y*/, 0 /*dest_x*/, 0 /*dest_y*/,
                      width, height,
                      False /* Don't put True here ! */ );
#   endif
    }
    else
#endif /* USE_SHM */
    {
        /* Use standard XPutImage -- this is gonna be slow ! */
#ifdef HAVE_XVIDEO
        XvPutImage( p_display, i_xvport,
                    video_window,
                    gc, p_image,
                    0 /*src_x*/, 0 /*src_y*/,
                    width, height,
                    0 /*dest_x*/, 0 /*dest_y*/, o_width, o_height );
#else
        XPutImage( p_display,
                   video_window,
                   gc, p_image,
                   0 /*src_x*/, 0 /*src_y*/, 0 /*dest_x*/, 0 /*dest_y*/,
                   width, height );
#endif
    }

    /* Make sure the command is sent now - do NOT use XFlush !*/
#ifndef WINDOWS
    XSync( p_display, False );
#else
	p_display->request++;
#endif
}

// ****************


XRender::XRender()
  :yuv_image(NULL), display(NULL), enable_shm(false){

}

// XRender::init
// return value:
// -1: indicate the X server doesn't support xvideo extension
//  0: successfult allocate xv_port
//  1: exist xv_port allocated 
int XRender::init(Display *_dpy, vlc_fourcc_t _chroma, Visual *_p_visual, int _depth, int _bytes_per_rgb){
  display = _dpy;
  i_chroma = _chroma;
  p_visual = _p_visual;
  depth = _depth;
  bytes_per_rgb = _bytes_per_rgb;
  
  
#ifdef USE_SHM 
  if (XShmQueryExtension(display)){
    enable_shm = true;  	
    // printf("==> using shared memory.\n");
  }else{
    enable_shm = false;
    // printf("==> no shmem available.\n");    
  }
#endif
  
#ifdef HAVE_XVIDEO  
  if(xv_port != -1){
    num_xv_port++;
    return 1;
  }
  
  xv_port = XVideoGetPort( display, i_chroma, &o_chroma );
  if(xv_port != -1){
    num_xv_port = 1; 
    return 0;
  }else{ // doesn't supoort Xvideo extension
    enable_shm = false;
    return -1;	
  }
#endif
  return -1;
}

IMAGE_TYPE* XRender::createImage(int _width, int _height){
  i_width  = _width;
  i_height = _height;
  
  if(yuv_image) IMAGE_FREE(yuv_image);
  
#ifdef USE_SHM    
  if(enable_shm){
    yuv_image = CreateShmImage(display, 
    #ifdef HAVE_XVIDEO   
      xv_port, i_chroma, &yuv_shminfo,
    #else
      p_visual, depth, &yuv_shminfo, 
    #endif
      i_width, i_height );  
  }else
#endif  // USE_SHM
  {
    yuv_image = CreateImage(display,
    #ifdef HAVE_XVIDEO  
      xv_port, i_chroma, 12,
    #else
      p_visual, depth, bytes_per_rgb,
    #endif
      i_width, i_height );
  }
  return yuv_image; 
}

void XRender::displayImage(Window video_window, GC gc, int o_width, int o_height) const{
  DisplayVideo( display, xv_port, video_window, gc, yuv_image, i_width, i_height, 
    o_width, o_height, enable_shm );	
	
}

void XRender::release(){
  if(yuv_image) IMAGE_FREE(yuv_image);
  
  #ifdef HAVE_XVIDEO
  if(--num_xv_port == 0)
    XVideoReleasePort(display, xv_port);
  #endif  
  
  #ifdef USE_SHM
  if(enable_shm){
    XShmDetach(display, &yuv_shminfo);
    if(shmdt( yuv_shminfo.shmaddr) <0 )
      printf("vic: shmdt\n");
  }
  #endif
}

XRender::~XRender(){
  release();	
}

