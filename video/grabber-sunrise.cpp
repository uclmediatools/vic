/*
 * Copyright (c) 1993-1995 The Regents of the University of California.
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
 *
 * Contributed by Bob Olson (olson@mcs.anl.gov) September 1995.
 */

#ifndef lint
static char rcsid[] =
    "@(#) $Header$ (LBL)";
#endif

#include <stdio.h>
#include <errno.h>
#include <osfcn.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "inet.h"
#include "vic_tcl.h"
#include "rtp.h"
#include "grabber.h"
#include "crdef.h"
#include "device-input.h"
#include "transmitter.h"
#include "module.h"

#include <UMSVideoIO.xh>
#include <UMSStrings.h>

#define UMS_Success(rc) (rc == UMSVideoIO_Success)

#define NTSC_FULL_WIDTH 640
#define NTSC_FULL_HEIGHT 480

#define N_RING_BUFFERS 4

class SunriseGrabber : public Grabber {
protected:
	SunriseGrabber();
public:
	virtual ~SunriseGrabber();
	virtual int command(int argc, const char* const * argv);
	virtual void fps(int);
protected:
	_IDL_SEQUENCE_UMSVideoIO_RingBufferElement ring_buffer;
	void allocate_ring_buffers(long n_buffers, long buffer_size);
	int decimate_;
	long width_;
	long height_;
	string port_;
	virtual void setsize() = 0;
	virtual void set_port();
	Environment *ev;
};

class SunriseJpegGrabber : public SunriseGrabber {
public:
	SunriseJpegGrabber();
	virtual ~SunriseJpegGrabber();
	virtual int command(int argc, const char* const * argv);
	void setsize();
protected:
	void start();
	void stop();
private:
	int q_;
	int grab();
};

class SunriseYuvCore : public SunriseGrabber {
public:
	SunriseYuvCore();
	virtual ~SunriseYuvCore();
protected:
	virtual void start();
	virtual void stop();
	virtual int command(int argc, const char * const * argv);
	u_char* frame_;
	_IDL_SEQUENCE_octet colormap;
};

class SunriseYuvGrabber : public SunriseYuvCore {
public:
	SunriseYuvGrabber();
	virtual int command(int argc, const char * const * argv);
	virtual int grab();
	void setsize();
protected:
};

class SunriseCIFGrabber : public SunriseYuvCore {
public:
	SunriseCIFGrabber();
	virtual int command(int argc, const char * const * argv);
	virtual int grab();
	void setsize();
protected:
};

class SunriseDevice : public InputDevice {
public:
//    SunriseDevice(const char* s);
    SunriseDevice(const char* s, const char *which);
    virtual int command(int argc, const char * const * argv);
    virtual Grabber* yuv_grabber();
    virtual Grabber* jpeg_grabber();
    virtual Grabber* cif_grabber();
    UMSVideoIO *video;
    SunriseDevice *next;
    const char *myname;
    char *device;
    int open_video();
    void close_video();
protected:
    Environment *ev;
};

class SunriseDeviceMaker
{
 public:
    SunriseDeviceMaker();
    ~SunriseDeviceMaker();
 protected:
    SunriseDevice *devices;
    void trydev(string dev);
    Environment *ev;
};

static SunriseDeviceMaker maker;
static SunriseDevice *sunrise_device = NULL;

SunriseDeviceMaker::SunriseDeviceMaker()
{
    string devnames[] = { "/dev/sr1", "/dev/sr0", NULL };
    int i;
    string devname;
    struct stat statbuf;
    SunriseDevice *dev;

    ev = somGetGlobalEnvironment();
    
    devices = NULL;

    for (i = 0; devnames[i] != NULL; i++)
    {
	devname = devnames[i];
	if (stat(devname, &statbuf) == 0)
	{
	    if (statbuf.st_mode & (S_IWUSR | S_IRUSR))
	    {
		char *s;
		
		if (s = rindex(devname, '/'))
		    s++;
		else
		    s = devname;
		char *name = new char[strlen(s) + 10];
		sprintf(name, "Sunrise_%s", s);
		dev = new SunriseDevice(name, devname);
#ifdef DEBUG
		printf("Creating %s %x\n", s, dev);
#endif
		dev->next = devices;
		devices = dev;
	    }
	}
    }
}


SunriseDeviceMaker::~SunriseDeviceMaker()
{
}

SunriseDevice::SunriseDevice(const char* s, const char *dev = "/dev/sr0")
    : InputDevice(s), myname(s)
{
    attributes_ = "format { 422 jpeg } size { small large cif } port { composite svideo }";
    
#ifdef DEBUG
    printf("SunriseDevice::SunriseDevice name=%s\n", s);
#endif /* DEBUG */

    ev = somGetGlobalEnvironment();

    video = new UMSVideoIO;

    device = new char[strlen(dev) + 1];
    strcpy(device, dev);
}

int SunriseDevice::command(int argc, const char*const* argv)
{
    if (argc == 3 && strcmp(argv[1], "open") == 0) {
	const char* fmt = argv[2];
	TclObject* o = 0;
	if (strcmp(fmt, "jpeg") == 0)
	    o = jpeg_grabber();
	else if (strcmp(fmt, "422") == 0)
	    o = yuv_grabber();
	else if (strcmp(fmt, "cif") == 0)
	    o = cif_grabber();
/*
	else if (strcmp(fmt, "420") == 0)
	    o = new Jv420Grabber;
	*/
	if (o != 0)
	    Tcl::instance().result(o->name());
	return (TCL_OK);
    }
    return (InputDevice::command(argc, argv));
}

int SunriseDevice::open_video()
{
    UMSVideoIO_ReturnCode rc;

#ifdef DEBUG
    printf("%x %s open_video\n", this, device);
#endif
    if ((rc = video->open(ev, (string) device)) != UMSVideoIO_Success)
    {
#ifdef DEBUG
	printf("error %d opening video device %s\n", rc, device);
#endif
	return 0;
    }

#ifdef DEBUG
    printf("opened device %s\n", device);
#endif /* DEBUG */

    return 1;
}

void SunriseDevice::close_video()
{
    UMSVideoIO_ReturnCode rc;

#ifdef DEBUG
    printf("%x %s close_video\n", this, device);
#endif
    if ((rc = video->close(ev)) != UMSVideoIO_Success)
    {
#ifdef DEBUG
	printf("error %d closing video device %s\n", rc, device);
#endif
    }
    else
    {
#ifdef DEBUG
	printf("closed device %s\n", device);
#endif
    }
    
}

Grabber* SunriseDevice::jpeg_grabber()
{
#ifdef DEBUG
    printf("Asking device %s for new jpeg grabber\n", myname);
#endif
    if (open_video())
    {
#ifdef DEBUG
	printf("Open succeeded\n");
#endif
	
	sunrise_device = this;
	return (new SunriseJpegGrabber());
    }
    else
    {
#ifdef DEBUG
	printf("open failed\n");
#endif
	return NULL;
    }
}

Grabber* SunriseDevice::yuv_grabber()
{
#ifdef DEBUG
    printf("Asking device %s for new jpeg grabber\n", myname);
#endif
    if (open_video())
    {
#ifdef DEBUG
	printf("Open succeeded\n");
#endif
	
	sunrise_device = this;
	return (new SunriseYuvGrabber());
    }
    else
    {
#ifdef DEBUG
	printf("open failed\n");
#endif
	return NULL;
    }
}

Grabber* SunriseDevice::cif_grabber()
{
#ifdef DEBUG
    printf("Asking device %s for new jpeg grabber\n", myname);
#endif
    if (open_video())
    {
#ifdef DEBUG
	printf("Open succeeded\n");
#endif
	
	sunrise_device = this;
	return (new SunriseCIFGrabber());
    }
    else
    {
#ifdef DEBUG
	printf("open failed\n");
#endif
	return NULL;
    }
}

SunriseGrabber::SunriseGrabber() : port_(INPUT_COMPOSITE)
{
    ev = somGetGlobalEnvironment();

    ring_buffer._buffer = (UMSVideoIO_RingBufferElement *) NULL;
}

SunriseGrabber::~SunriseGrabber()
{
#ifdef DEBUG
    printf("grabber destroy\n");
#endif
    sunrise_device->close_video();
    sunrise_device = NULL;
}

void SunriseGrabber::fps(int val)
{
    UMSVideoIO_ReturnCode rc;
    double capture_rate = val;
    int was_running;

    was_running = running_;
    if (was_running)
	stop();
    
    rc = sunrise_device->video->set_capture_rate(ev, &capture_rate);
#ifdef DEBUG
    if (!UMS_Success(rc))
    {
	printf("set_capture_rate failed with rc=%d\n", rc);
    }
    else
	printf("set capture rate to %lf\n", capture_rate);
#endif /* DEBUG */

    if (was_running)
	start();

    Grabber::fps(val);
}

int SunriseGrabber::command(int argc, const char * const * argv)
{
    int i;
    Tcl& tcl = Tcl::instance();
    
#ifdef DEBUG
    printf("SunriseGrabber::command argc=%d ", argc);
    for (i = 0; i < argc; i++)
	printf("\"%s\" ", argv[i]);
    printf("\n");
#endif /* DEBUG */
    if (argc == 3)
    {
	if (strcmp(argv[1], "port") == 0) {
	    string new_port = "";

	    if (strcmp(argv[2], "composite") == 0)
		new_port = INPUT_COMPOSITE;
	    else
		new_port = INPUT_SVIDEO;

	    if (strcmp(port_, new_port) != 0)
	    {
		port_ = new_port;
		set_port();
	    }
	}
    }

    return (Grabber::command(argc, argv));
}

void SunriseGrabber::set_port()
{
    int was_running = running_;
    UMSVideoIO_ReturnCode rc;

    if (was_running)
	stop();

    rc = sunrise_device->video->set_video_input_connector(ev, port_);
    if (!UMS_Success(rc) && rc != UMSVideoIO_ValueChanged)
    {
#ifdef DEBUG
	printf("set_video_input_connector %s failed with rc=%d\n",
	       port_, rc);
#endif /* DEBUG */
    }
    if (was_running)
	start();
}

SunriseJpegGrabber::SunriseJpegGrabber() : q_(50)
{
    decimate_ = 2;
//    setsize();
}

void SunriseJpegGrabber::setsize()
{
    UMSVideoIO_ReturnCode rc;
    long w, h;
    int was_running;
    
#ifdef DEBUG
    printf("SunriseJpegGrabber::setsize()\n");
#endif /* DEBUG */

    was_running = running_;
    if (was_running)
	stop();
    
    w = 640 / decimate_;
    if (decimate_ == 4 || decimate_ == 2)
	h = 240;
    else
	h = 120;

#ifdef DEBUG
    printf("set_input_image_size %dx%d\n", w, h);
#endif /* DEBUG */
    
    rc = sunrise_device->video->set_input_image_size(ev, &w, &h);
    if (!UMS_Success(rc) && rc != UMSVideoIO_ValueChanged)
    {
#ifdef DEBUG
	printf("set_input_image_size %dx%d failed with rc=%d\n",
	       w, h, rc);
#endif /* DEBUG */
    }

#ifdef DEBUG
    printf("after set_input_image_size w=%d h=%d rc=%d\n", w, h, rc);
#endif /* DEBUG */

    width_ = w;
    height_ = h;

    double capture_rate = fps_;
    rc = sunrise_device->video->set_capture_rate(ev, &capture_rate);
#ifdef DEBUG
    if (!UMS_Success(rc))
	printf("set_capture_rate failed with rc=%d\n", rc);
#endif /* DEBUG */

    allocate_ring_buffers(N_RING_BUFFERS, w * h);

    long q = q_;
    rc = sunrise_device->video->set_quality_factor(ev, &q);
    if (!UMS_Success(rc))
    {
#ifdef DEBUG
	printf("set_quality_factor failed with rc=%d\n", rc);
#endif /* DEBUG */
	return;
    }
    q_ = q;

    rc = sunrise_device->video->setup_compressed_capture_buffers(ev, &ring_buffer);
    if (!UMS_Success(rc))
    {
#ifdef DEBUG
	printf("setup_compressed_capture failed with rc=%d\n", rc);
#endif /* DEBUG */
	return;
    }

    if (was_running)
	start();
}

void SunriseJpegGrabber::start()
{
    UMSVideoIO_ReturnCode rc;

    rc = sunrise_device->video->set_compression(ev, UMSVideoIO_On);
    if (!UMS_Success(rc))
    {
#ifdef DEBUG
	printf("set_compression failed with rc=%d\n", rc);
#endif /* DEBUG */
	return;
    }

    frameclock_ = gettimeofday_usecs();

    /*
     * This sleep is VERY IMPORTANT. It lets the capture card
     * get ahead of vic, so that frames will be available when
     * vic tries to grab them. Otherwise, the grab will block
     * and vic will be permanently behind, locking out X events.
     *
     * Another fix might be to use nonblocking frame grabs.
     */
//    sleep(1);
    timeout();
}

void SunriseJpegGrabber::stop()
{
    UMSVideoIO_ReturnCode rc;

    rc = sunrise_device->video->set_compression(ev, UMSVideoIO_Off);
    if (!UMS_Success(rc))
    {
#ifdef DEBUG
	printf("set_uncompression failed with rc=%d\n", rc);
#endif /* DEBUG */
    }

    cancel();
}

int SunriseJpegGrabber::grab()
{
    UMSVideoIO_RingBufferElement *frame;
    static UMSVideoIO_RingBufferElement *last_frame = NULL;
    long index;
    UMSVideoIO_ReturnCode rc;
    int frc;
    unsigned char *data;

    tx_->flush();
    if (last_frame)
	last_frame->InUseByCaller = 0;

    rc = sunrise_device->video->get_compressed_frame(ev, &index, FALSE);
    if (!UMS_Success(rc))
    {
#ifdef DEBUG
	printf("get_compressed_frame failed with rc=%d\n", rc);
#endif
	return 0;
    }
    
    frame = &(ring_buffer._buffer[index]);

    data = (unsigned char *) frame->Address;

    JpegFrame f(media_ts(), data, frame->SizeOfDataInBuffer,
		q_, 0, width_, height_);
    frc = target_->consume(&f);

    last_frame = frame;
    
    return frc;
}

int SunriseJpegGrabber::command(int argc, const char * const * argv)
{
    int i;
    Tcl& tcl = Tcl::instance();
    
#ifdef DEBUG
    printf("SunriseJpegGrabber::command argc=%d ", argc);
    for (i = 0; i < argc; i++)
	printf("\"%s\" ", argv[i]);
    printf("\n");
#endif /* DEBUG */

    if (argc == 3)
    {
	if (strcmp(argv[1], "q") == 0) {
	    /* assume value is in range */
	    q_ = atoi(argv[2]);
	    setsize();
	    return (TCL_OK);
	}
	else if (strcmp(argv[1], "decimate") == 0) {
	    int d = atoi(argv[2]);
	    Tcl& tcl = Tcl::instance();
	    if (d <= 0) {
		Tcl_AppendResult(tcl.interp(),
				 "%s: divide by zero", argv[0], 0);
		return (TCL_ERROR);
	    }
	    decimate_ = d;
	    setsize();
	    return(TCL_OK);
	}
    }
    return SunriseGrabber::command(argc, argv);
}

SunriseJpegGrabber::~SunriseJpegGrabber()
{
}


SunriseYuvCore::SunriseYuvCore() : frame_(0)
{
}

int SunriseYuvCore::command(int argc, const char * const * argv)
{
    int i;
    Tcl& tcl = Tcl::instance();
    
#ifdef DEBUG
    printf("SunriseYuvCore::command argc=%d ", argc);
    for (i = 0; i < argc; i++)
	printf("\"%s\" ", argv[i]);
    printf("\n");
#endif /* DEBUG */

    if (argc == 3)
    {
	if (strcmp(argv[1], "decimate") == 0)
	{
	    int d = atoi(argv[2]);
	    Tcl& tcl = Tcl::instance();
	    if (d <= 0) {
		Tcl_AppendResult(tcl.interp(),
				 "%s: divide by zero", argv[0], 0);
		return (TCL_ERROR);
	    }
	    decimate_ = d;
	    setsize();
	    return(TCL_OK);
	}
    }
    return SunriseGrabber::command(argc, argv);
}

 SunriseYuvCore::~SunriseYuvCore()
{
}

 SunriseYuvGrabber::SunriseYuvGrabber()
{
    decimate_ = 2;
//    setsize();
}

void SunriseYuvGrabber::setsize()
{
    long w, h, v;
    UMSVideoIO_ReturnCode rc;
    int was_running;
    
    was_running = running_;
    if (was_running)
	stop();

    w = NTSC_FULL_WIDTH / decimate_;
    h = NTSC_FULL_HEIGHT / decimate_;

    rc = sunrise_device->video->set_output_image_format(ev, YUV422);
    
    if (!UMS_Success(rc))
    {
#ifdef DEBUG
	printf("set_output_image_format failed with rc=%d\n", rc);
#endif /* DEBUG */
	return;
    }

    long iw, ih;

    if (w > 320)
	iw = 640;
    else
	iw = 320;

    if (h > 240)
	ih = 480;
    else
	ih = 240;
    
    rc = sunrise_device->video->set_input_image_size(ev, &iw, &ih);
    if (!UMS_Success(rc) && rc != UMSVideoIO_ValueChanged)
    {
#ifdef DEBUG
	printf("set_input_image_size failed with rc=%d\n", rc);
#endif /* DEBUG */
	return;
    }

#ifdef DEBUG
    printf("after set_input iw=%d ih=%d rc=%d\n", iw, ih, rc);
#endif /* DEBUG */
    
    rc = sunrise_device->video->set_uncompressed_image_size(ev, &w, &h);
    if (!UMS_Success(rc) && rc != UMSVideoIO_ValueChanged)
    {
#ifdef DEBUG
	printf("set_uncompressed_image_size failed with rc=%d\n", rc);
#endif /* DEBUG */
	return;
    }

#ifdef DEBUG
    printf("after set_uncompressed_image_size w=%d h=%d rc=%d\n", w, h, rc);
#endif /* DEBUG */


    width_ = w;
    height_ = h;
    framesize_ = w * h;
    frame_ = new u_char[2 * framesize_];
    crinit(w, h);
    allocref();
    
    double capture_rate = fps_;
    rc = sunrise_device->video->set_capture_rate(ev, &capture_rate);
#ifdef DEBUG
    if (!UMS_Success(rc))
    {
	printf("set_capture_rate failed with rc=%d\n", rc);
    }
    else
	printf("set capture rate to %lf\n", capture_rate);
#endif /* DEBUG */

    allocate_ring_buffers(N_RING_BUFFERS, w * h * 2);
    
    rc = sunrise_device->video->setup_uncompressed_capture_buffers(ev, &ring_buffer);

#ifdef DEBUG
    if (!UMS_Success(rc))
	printf("setup_uncompressed_capture failed with rc=%d\n", rc);
#endif /* DEBUG */

    if (was_running)
	start();
}

int SunriseYuvGrabber::command(int argc, const char * const * argv)
{
    return (SunriseYuvCore::command(argc, argv));
}

int SunriseYuvGrabber::grab()
{
    UMSVideoIO_RingBufferElement *frame;
    long index;
    UMSVideoIO_ReturnCode rc;
    int grc;
    unsigned char *yptr, *uptr, *vptr;
    int i;

    rc = sunrise_device->video->get_uncompressed_frame(ev, &index, TRUE);
    if (!UMS_Success(rc))
    {
	printf("get_uncompressed_frame failed with rc=%d\n", rc);
	return 0;
    }
    
    frame = &(ring_buffer._buffer[index]);


    /*
     * Pixel format: YYYYYYYY UUUUUUUU YYYYYYYY VVVVVVVV
     * U & V are shared for two pixels.
     */

    struct pixel
    {
	unsigned char y1;
	unsigned char u;
	unsigned char y2;
	unsigned char v;
    } *p, *p_end;

    yptr = frame_;
    uptr = frame_ + framesize_;
    vptr = uptr + framesize_ / 2;

    p = (struct pixel *) frame->Address;
    p_end = (struct pixel *) (frame->Address + frame->SizeOfDataInBuffer);

    while (p < p_end)
    {
	yptr[0] = p->y1;
	yptr[1] = p->y2;
	uptr[0] = p->u;
	vptr[0] = p->v;

	yptr += 2;
	uptr++;
	vptr++;
	p++;
    }

    int overruns = frame->NumberOfOverruns;
    frame->InUseByCaller = 0;

#if 0
    if (overruns > 0)
    {
	printf("Got %d overruns, flushing\n", frame->NumberOfOverruns);
	
	rc = sunrise_device->video->flush_uncompressed_capture_buffers(ev);
	if (!UMS_Success(rc))
	{
	    printf("flush_uncompressed_capture_buffers failed with rc=%d\n", rc);
	}
    }
#endif

    suppress(frame_);
    saveblks(frame_);
    YuvFrame f(media_ts(), frame_, crvec_, width_, height_);
    grc = target_->consume(&f);
    return grc;
}


void SunriseYuvCore::start()
{
    UMSVideoIO_ReturnCode rc;

    if (running_)
	return;

    rc = sunrise_device->video->set_uncompression(ev, UMSVideoIO_On);
    if (!UMS_Success(rc))
    {
	printf("set_uncompression failed with rc=%d\n", rc);
	return;
    }

    running_ = 1;
    frameclock_ = gettimeofday_usecs();
    timeout();
}

void SunriseYuvCore::stop()
{
    UMSVideoIO_ReturnCode rc;

    if (!running_)
	return;

    rc = sunrise_device->video->set_uncompression(ev, UMSVideoIO_Off);
    if (!UMS_Success(rc))
    {
	printf("set_uncompression failed with rc=%d\n", rc);
    }

    running_ = 0;
    cancel();
}

void SunriseGrabber::allocate_ring_buffers(long n_buffers, long buffer_size)
{
    unsigned char **Address;
    UMSVideoIO_RingBufferElement *rbuffer1;
    int i;
    
    Address = new unsigned char * [n_buffers];
    
    ring_buffer._length = n_buffers;
    ring_buffer._maximum  = n_buffers;

#ifdef DEBUG
    printf("Allocating ring buffers: n_buffers=%d buffer_size=%d\n",
	   n_buffers, buffer_size);
#endif /* DEBUG */

    if (ring_buffer._buffer != (struct UMSVideoIO_RingBufferElement *) NULL)
	delete[] ring_buffer._buffer; //SV-XXX: Debian

    ring_buffer._buffer = new UMSVideoIO_RingBufferElement[n_buffers];

    if (ring_buffer._buffer <= 0)
    {
	fprintf(stderr, "Cannot malloc somThis->ring_buffer._buffer\n");
	exit(1);
    }

    /* Set up each ring buffer */

    rbuffer1 = ring_buffer._buffer;

    for (i = 0; i < n_buffers; i++)
    {
	Address[i] = (unsigned char *)
	    malloc(buffer_size + 4096);
	if (Address[i] <= 0)
	{
	    fprintf(stderr, "Cannot malloc ring_buffer %d\n", i);
	    free(ring_buffer._buffer);
	    exit(1);
	}
	rbuffer1->Address = ( ((long) Address[i]) + 4096
					- ( ((long) Address[i]) % 4096));
	rbuffer1->AfterHeader = rbuffer1->Address;
	rbuffer1->SizeOfBuffer = buffer_size;
	rbuffer1->SizeOfDataInBuffer = 0;
	rbuffer1->InUseByCaller	= 0;
	rbuffer1->NumberOfOverruns = 0;
	rbuffer1++;
    }
}


 SunriseCIFGrabber::SunriseCIFGrabber()
{
#ifdef DEBUG
    printf("SunriseCIFGrabber::SunriseCIFGrabber\n");
#endif

    decimate_ = 2;
//    setsize();
}

void SunriseCIFGrabber::setsize()
{
    long w, h, v;
    UMSVideoIO_ReturnCode rc;
    
#ifdef DEBUG
    printf("SunriseCIFGrabber::setsize()\n");
#endif

    w = 2 * 352 / decimate_;
    h = 2 * 288 / decimate_;

    rc = sunrise_device->video->set_output_image_format(ev, YUV422);
    
    if (!UMS_Success(rc))
    {
#ifdef DEBUG
	printf("set_output_image_format failed with rc=%d\n", rc);
#endif
	return;
    }


    long iw, ih;

    if (w > 320)
	iw = 640;
    else
	iw = 320;

    if (h > 240)
	ih = 480;
    else
	ih = 240;
    
    rc = sunrise_device->video->set_input_image_size(ev, &iw, &ih);
    if (!UMS_Success(rc) && rc != UMSVideoIO_ValueChanged)
    {
#ifdef DEBUG
	printf("set_input_image_size failed with rc=%d\n", rc);
#endif
	return;
    }

#ifdef DEBUG
    printf("after set_input iw=%d ih=%d rc=%d\n", iw, ih, rc);
#endif
    
    rc = sunrise_device->video->set_uncompressed_image_size(ev, &w, &h);
    if (!UMS_Success(rc) && rc != UMSVideoIO_ValueChanged)
    {
#ifdef DEBUG
	printf("set_uncompressed_image_size failed with rc=%d\n", rc);
#endif
	return;
    }

#ifdef DEBUG
    printf("after set_uncompressed_image_size w=%d h=%d rc=%d\n", w, h, rc);
#endif


    width_ = w;
    height_ = h;
    framesize_ = w * h;
    frame_ = new u_char[framesize_ + framesize_ / 2];
    crinit((int) w, (int) h);
    allocref();

    double capture_rate = fps_;
    rc = sunrise_device->video->set_capture_rate(ev, &capture_rate);
    if (!UMS_Success(rc))
    {
#ifdef DEBUG
	printf("set_capture_rate failed with rc=%d\n", rc);
#endif
    }
    else
    {
#ifdef DEBUG
	printf("set capture rate to %lf\n", capture_rate);
#endif
    }

    allocate_ring_buffers(N_RING_BUFFERS, w * h * 2);
    
    rc = sunrise_device->video->setup_uncompressed_capture_buffers(ev, &ring_buffer);
    if (!UMS_Success(rc))
    {
#ifdef DEBUG
	printf("setup_uncompressed_capture failed with rc=%d\n", rc);
#endif
	return;
    }

}

int SunriseCIFGrabber::grab()
{
    UMSVideoIO_RingBufferElement *frame;
    long index;
    UMSVideoIO_ReturnCode rc;
    int grc;
    unsigned char *yptr, *uptr, *vptr;
    int i, x, y;

    rc = sunrise_device->video->get_uncompressed_frame(ev, &index, TRUE);
    if (!UMS_Success(rc))
    {
	printf("get_uncompressed_frame failed with rc=%d\n", rc);
	return 0;
    }
    
    frame = &(ring_buffer._buffer[index]);

    /*
     * Pixel format: YYYYYYYY UUUUUUUU YYYYYYYY VVVVVVVV
     * U & V are shared for two pixels.
     */

    struct pixel
    {
	unsigned char y1;
	unsigned char u;
	unsigned char y2;
	unsigned char v;
    } *p, *p_end;

    yptr = frame_;
    uptr = frame_ + framesize_;
    vptr = uptr + framesize_ / 4;

    p = (struct pixel *) frame->Address;
    p_end = (struct pixel *) (frame->Address + frame->SizeOfDataInBuffer);

    for (y = 0; y < height_; y++)
    {
	for (x = 0; x < width_; x += 2)
	{
	    yptr[0] = p->y1;
	    yptr[1] = p->y2;
	    yptr += 2;
	    p++;

	    if ((y & 1) == 0)
	    {
		*uptr++ = p->u;
		*vptr++ = p->v;
	    }
	}
    }

    frame->InUseByCaller = 0;

    suppress(frame_);
    saveblks(frame_);
    YuvFrame f(media_ts(), frame_, crvec_, width_, height_);
    grc = target_->consume(&f);
    return grc;
}

int SunriseCIFGrabber::command(int argc, const char * const * argv)
{
    return (SunriseYuvCore::command(argc, argv));
}

