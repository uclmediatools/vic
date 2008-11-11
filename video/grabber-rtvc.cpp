/*
 * Copyright (c) 1994 Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Network Research
 *      Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <ar.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "module.h"
#include "vic_tcl.h"

#include <tk.h>
#include <sys/param.h>
#include "grabber.h"
#include "crdef.h"
#include "iohandler.h"
#include "device-input.h"
#include "transmitter.h"

/*
 * rtvc driver ioctls & data structures.
 */
#define	RTVC_IOC		('r' << 8)
#define	RTVC_CMD_PROGRAM	(RTVC_IOC | 0)	/* download firmware */
#define	RTVC_CMD_VERSION	(RTVC_IOC | 1)	/* return driver version */
#define	RTVC_CMD_SET_VIDEO	(RTVC_IOC | 2)	/* set video attributes */
#define	RTVC_CMD_GET_VIDEO	(RTVC_IOC | 3)	/* get video attributes */
#define	RTVC_CMD_SEND_MESSAGE	(RTVC_IOC | 4)	/* talk to firmware */
#define	RTVC_CMD_RESET		(RTVC_IOC | 5)	/* reset board */
#define	RTVC_CMD_HALT		(RTVC_IOC | 6)	/* halt cl4000 */
#define	RTVC_CMD_GO		(RTVC_IOC | 7)	/* resume cl4000 */

struct rtvc_capture_t {
	u_int32_t byte_count;		/* buffer size on entry,
					 * image size on return */
	u_int32_t frame_number;		/* (relative to start of capture) */
	hrtime_t timestamp;		/* when image was captured */
};

struct rtvc_video_parameters_t {
	int32_t video_port;
	int32_t video_format;
	int32_t video_device;		/* 812 = Brooktree; 7191 = Philips */
	int32_t video_status;		/* video device hardware status */
	int32_t video_buffers;		/* number of video buffers */
	int32_t video_images;		/* video images available */
	int32_t buffer_alignment;	/* capture buffer alignment */
	int32_t padding;
	rtvc_capture_t capture;		/* last capture state */
	int32_t video_registers[29];
};

#define RTVC_PORT_S_VIDEO              0
#define RTVC_PORT_COMPOSITE_VIDEO_1    1
#define RTVC_PORT_COMPOSITE_VIDEO_2    2

#define RTVC_FORMAT_UNKNOWN	0
#define RTVC_FORMAT_PAL		1
#define RTVC_FORMAT_NTSC	2
#define RTVC_FORMAT_MAX		2

struct rtvc_firmware_message_t {
	u_int32_t flags;
	u_int32_t pixel_stride;
	u_int32_t return_status;
	int32_t image_skip;
	u_int32_t video_dma_interrupts;
	u_int32_t image_buffers;      
	u_int32_t dmem_image_info_end;
	u_int32_t width;
	u_int32_t height;
	u_int32_t scan_width;
	u_int32_t stride;
	u_int32_t back_porch;
	u_int32_t blanking;
	u_int32_t unused[3];
	u_int8_t data[1024];
};

#define RTVC_FIRMWARE_FLAG_GO		0x0001
#define RTVC_FIRMWARE_FLAG_FLUSH	0x0002
#define RTVC_FIRMWARE_FLAG_LOGO		0x0004
#define RTVC_FIRMWARE_FLAG_FULL_FRAMES	0x0008
#define RTVC_FIRMWARE_FLAG_DVMA		0x0010

#define	RTVC_NUM_VIDEO_SCANLINE_REGISTERS	20

struct rtvc_message_t {
	u_int32_t scanline_mask[RTVC_NUM_VIDEO_SCANLINE_REGISTERS];
	u_int32_t field1_scanline_interrupt;
	u_int32_t field2_scanline_interrupt;
	u_int32_t buffer_byte_size;
	u_int32_t max_buffers;
	rtvc_firmware_message_t firmware_message;
};

struct rtvc_program_t {
	u_int32_t *program;
	u_int32_t *buffer;
	u_int32_t *error;
	int32_t command;
};

#define	RTVC_MMAP_VIDEO_OFFSET		((off_t)0x00800000)
#define	RTVC_READ_CAPTURED_OFFSET	((off_t)0x01000000)


class RTVCGrabber : public Grabber, public IOHandler {
    protected:
	RTVCGrabber(int devno);
    public:
	virtual ~RTVCGrabber();
	virtual void start();
	virtual void stop();
	virtual int command(int argc, const char*const* argv);
	virtual void fps(int);
	inline int is_pal() const { return (max_fps_ == 25); }
    protected:
	void dispatch(int mask);
	void set_size_nachos(int w, int h);
	virtual void setsize() = 0;
	virtual int capture();
	void setInputPort(int newport);
	void updateParameters(int both_fields = 0);
	void loadFirmware(int fid);
	void sendMessage();
	void halt();
	void run();
	void findFirmware();
	virtual int firmwareFID() const;
	u_int32_t media_ts();
	u_int32_t ref_ts();

	double nextframetime_;
	int fd_;		/* rtvc data fd */
	u_int decimate_;
	u_int basewidth_;
	u_int baseheight_;
	u_int max_fps_;		/* 25 (PAL) or 30 (NTSC) */
	int pagesize_;

	volatile rtvc_video_parameters_t* state_;
	rtvc_message_t fpb_;
	int cur_video_format_;
	int capsize_;
	u_char* capbuf_;	/* rtvc capture buffer */
	u_char* capbase_;	/* buffer base address (not page aligned) */
	const char* arpath_;
	int fid_;		/* current loaded capture firmware id */
	int ardir_[8];		/* offsets to firmware files in archive */

#define PAL_WIDTH 768
#define PAL_HEIGHT 576
#define PAL_FIRST_SCANLINE  23
#define NTSC_WIDTH 640
#define NTSC_HEIGHT 480
#define NTSC_FIRST_SCANLINE 23

#define CAPTURE_FID 0
#define SMALL_CAPTURE_FID 1
#define LARGE_CAPTURE_FID 2
#define JPEG_FID 3
#define JPEG_PAL_FID 4
};

class RTVCYuvGrabber : public RTVCGrabber {
    protected:
	inline RTVCYuvGrabber(int devno) : RTVCGrabber(devno) {}
	virtual int grab();
	void suppress(const u_char* in, int istride);
	virtual void saveblks(const u_char* in, int istride) = 0;
};


class RTVC422Grabber : public RTVCYuvGrabber {
    public:
	inline RTVC422Grabber(int devno) : RTVCYuvGrabber(devno) {}
    protected:
	virtual void setsize();
	void saveblk(const u_char* in, u_char* yp, u_char* up, u_char* vp,
		     int stride, int istride);
	void saveblks(const u_char* in, int istride);
};


class RTVCCIFGrabber : public RTVCYuvGrabber {
    public:
	inline RTVCCIFGrabber(int devno) : RTVCYuvGrabber(devno) {}
    protected:
	virtual void setsize();
	void saveblk(const u_char* in, u_char* yp, u_char* up, u_char* vp,
		     int stride, int istride);
	void saveblks(const u_char* in, int istride);
};

class RTVCJpegGrabber : public RTVCGrabber {
    public:
	RTVCJpegGrabber(int devno);
    protected:
	virtual int grab();
	virtual void setsize();
	virtual int command(int argc, const char*const* argv);
	virtual int firmwareFID() const;
	void setq(int q);

	int q_;
};

class RTVCDevice : public InputDevice {
    public:
	RTVCDevice(const char* s, int devno);
	virtual int command(int argc, const char*const* argv);
    protected:
	int devno_;

};

static class RTVCBuilder {
public:
	RTVCBuilder();
	static int checkdev(int n);
	static void build_device(int devno, int no_suffix);
} rtvc_builder;

int RTVCBuilder::checkdev(int n)
{
	char wrk[30];
	sprintf(wrk, "/dev/rtvc%d", n);
	return (access(wrk, F_OK));
}

void RTVCBuilder::build_device(int devno, int no_suffix)
{
	if (no_suffix)
		new RTVCDevice("sunvideo", devno);
	else {
		char wrk[32];
		sprintf(wrk, "sunvideo-%d", devno);
		char* device = new char[strlen(wrk) + 1];
		strcpy(device, wrk);
		new RTVCDevice(device, devno);
	}
}

RTVCBuilder::RTVCBuilder()
{
	/*
	 * rtvc devices might not start at 0 (they get their
	 * sbus slot number) so check all possible slots &
	 * count how many we have.
	 */
	int i, ndev = 0, dev = 0;
	for (i = 0; i < 8; ++i) {
		if (checkdev(i) == 0) {
			dev = i;
			++ndev;
		}
	}
	if (ndev == 1)
		/* don't number them if there's only one. */
		build_device(dev, 1);
	else
		for (i = 0; i < 8; ++i)
			if (checkdev(i) == 0)
				build_device(i, 0);
}

RTVCDevice::RTVCDevice(const char* s, int devno)
	: InputDevice(s), devno_(devno)
{
	attributes_ = "\
format { 420 422 jpeg } \
size { small large cif } \
port { Composite-1 Composite-2 S-Video }";
}

int RTVCDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			TclObject* o = 0;
			if (strcmp(argv[2], "422") == 0)
				o = new RTVC422Grabber(devno_);
			else if (strcmp(argv[2], "cif") == 0)
				o = new RTVCCIFGrabber(devno_);
			else if (strcmp(argv[2], "jpeg") == 0)
				o = new RTVCJpegGrabber(devno_);
			if (o != 0)
				tcl.result(o->name());
			return (TCL_OK);
		}
	}
	return (InputDevice::command(argc, argv));
}

RTVCGrabber::RTVCGrabber(int devno)
{
	fid_ = -1;
	cur_video_format_ = -1;
	basewidth_ = 1;
	baseheight_ = 1;
	decimate_ = 2;
	capbase_ = 0;
	capsize_ = 0;
	state_ = 0;

	char device[30];
	sprintf(device, "/dev/rtvc%d", devno);
	fd_ = open(device, O_RDWR);
	if (fd_ < 0) {
		perror(device);
		status_ = -1;
		return;
	}
	sprintf(device, "/dev/rtvcctl%d", devno);
	int cd = open(device, O_RDONLY);
	if (cd < 0) {
		perror(device);
		close(fd_);
		status_ = -1;
		return;
	}
	int ver = ioctl(cd, RTVC_CMD_VERSION, 0);
	if (ver < 8) {
		fprintf(stderr,
			"-vic: warning: expected rtvc driver ver.14, have ver.%d\n",
			ver);
	}
	state_ = (rtvc_video_parameters_t*)
		    mmap(0, sizeof(rtvc_video_parameters_t),
			 PROT_READ, MAP_SHARED, cd,
			 RTVC_MMAP_VIDEO_OFFSET);
	if ((int)state_ == -1) {
		perror("mmap");
		close(fd_);
		close(cd);
		status_ = -1;
		return;
	}
	close(cd);
#ifdef __svr4__
	pagesize_ = sysconf(_SC_PAGESIZE);
#else
	pagesize_ = NBPG;
#endif
	arpath_ = "xilSUNWrtvc_ucode.a";
	if (access(arpath_, R_OK) == -1) {
	        arpath_ = "/usr/openwin/lib/xil/devhandlers/xilIO_SUNWrtvc_ucode.a";
	        if (access(arpath_, R_OK) == -1) {
	                arpath_ = "/opt/SUNWits/Graphics-sw/xil/lib/pipelines/xilSUNWrtvc_ucode.a";
	                if (access(arpath_, R_OK) == -1) {
	                        printf("-vic: using rtvc ucode in current directory\n");
	                }
	        }
	}
	findFirmware();

	fpb_.firmware_message.image_skip = 1;
	fpb_.firmware_message.flags = 0;
	fpb_.max_buffers = 1;
	setInputPort(RTVC_PORT_COMPOSITE_VIDEO_1);
}

RTVCGrabber::~RTVCGrabber()
{
	close(fd_);
	if (capbase_ != 0)
		free(capbase_);
	if (state_)
		(void)munmap((caddr_t)state_, sizeof(rtvc_video_parameters_t));
}

void RTVCGrabber::findFirmware()
{
#ifdef stat
#undef stat
#endif
	struct stat fs;

	int arfd = open(arpath_, O_RDONLY);
	if (arfd < 0) {
		perror(arpath_);
		exit(1);
	}
	fstat(arfd, &fs);
	char* bp = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, arfd, 0);
	close(arfd);
	if (bp != (char*) -1) {
		if (memcmp(bp, ARMAG, SARMAG) != 0) {
			printf("rtvc: bad magic number in %s\n", arpath_);
			exit(1);
		}
		char* ep = bp + fs.st_size;
		for (char* cp = bp + SARMAG; cp < ep; ) {
			ar_hdr* ar = (ar_hdr*)cp;
			cp += sizeof(*ar);
			if (!memcmp(ar->ar_name,      "scale_cif.i/    ", 16))
				ardir_[CAPTURE_FID] = cp - bp;
			else if (!memcmp(ar->ar_name, "scale_even.i/   ", 16))
				ardir_[SMALL_CAPTURE_FID] = cp - bp;
			else if (!memcmp(ar->ar_name, "scale_full.i/   ", 16))
				ardir_[LARGE_CAPTURE_FID] = cp - bp;
			else if (!memcmp(ar->ar_name, "jpeg.i/         ", 16))
				ardir_[JPEG_FID] = cp - bp;
			else if (!memcmp(ar->ar_name, "jpeg_pal.i/     ", 16))
				ardir_[JPEG_PAL_FID] = cp - bp;
			int s = atoi(ar->ar_size);
			if (s & 1)
				++s;
			cp += s;
		}
		(void)munmap((caddr_t)bp, fs.st_size);
	}
}

int RTVCGrabber::firmwareFID() const
{
	switch (decimate_) {
	case 1:
		return (LARGE_CAPTURE_FID);
	case 4:
		return (SMALL_CAPTURE_FID);
	}
	return (CAPTURE_FID);
}

void RTVCGrabber::sendMessage()
{
	if (ioctl(fd_, RTVC_CMD_SEND_MESSAGE, &fpb_) < 0) {
		perror("RTVC_CMD_SEND_MESSAGE");
		exit(1);
	}
}

void RTVCGrabber::halt()
{
	u_int flags = fpb_.firmware_message.flags;
	fpb_.firmware_message.flags &=~ RTVC_FIRMWARE_FLAG_GO;
	sendMessage();
	fpb_.firmware_message.flags = flags;
}

void RTVCGrabber::run()
{
	if (ioctl(fd_, RTVC_CMD_GO, 0) < 0) {
		perror("RTVC_CMD_GO");
		exit(1);
	}
}

void RTVCGrabber::updateParameters(int both_fields)
{
	rtvc_message_t& fpb = fpb_;
	rtvc_firmware_message_t& msg = fpb.firmware_message;

	msg.flags = RTVC_FIRMWARE_FLAG_FLUSH | RTVC_FIRMWARE_FLAG_GO;
	int fullframe = decimate_ & 1;
	if (fullframe)
		msg.flags |= RTVC_FIRMWARE_FLAG_FULL_FRAMES;
	msg.return_status = 0;
	msg.pixel_stride = decimate_;
	msg.width = basewidth_;
	msg.stride = basewidth_;
	msg.height = baseheight_ / decimate_;
	msg.blanking = msg.back_porch = 0;
	msg.video_dma_interrupts = fullframe?
					(msg.height + 1) >> 1 : msg.height;
	int slintr, firstline;
	switch (cur_video_format_) {
	case RTVC_FORMAT_PAL:
		msg.scan_width = (384 << 2);
		firstline = (PAL_FIRST_SCANLINE - 7) << 1;
		slintr = PAL_FIRST_SCANLINE - 7 + PAL_HEIGHT + 2;
		break;

	default:
	case RTVC_FORMAT_NTSC:
		msg.scan_width = (320 << 2);
		firstline = (NTSC_FIRST_SCANLINE - 10) << 1;
		slintr = NTSC_FIRST_SCANLINE - 10 + NTSC_HEIGHT + 2;
		break;
	}
	fpb.field1_scanline_interrupt = slintr;
	fpb.field2_scanline_interrupt = slintr;
	memset(fpb.scanline_mask, 0, sizeof(fpb.scanline_mask));
	int ystop = baseheight_ + firstline;
	int ystride = both_fields? 1 : decimate_;
	for (int line = firstline; line < ystop; line += ystride) {
		int i, field;
		if (line & 1)
			field = RTVC_NUM_VIDEO_SCANLINE_REGISTERS / 2;
		else
			field = 0;
		i = (line + 1) >> 1;
		fpb.scanline_mask[field + (i >> 5)] |= 1 << (i & 0x1f);
	}
}


void RTVCGrabber::setInputPort(int newport)
{
	if (running_)
		halt();

	rtvc_video_parameters_t param;
	memset(&param, -1, sizeof(param));
	param.video_port = newport;
	if (ioctl(fd_, RTVC_CMD_SET_VIDEO, &param) < 0) {
		perror("RTVC_CMD_SET_VIDEO");
		fprintf(stderr, "couldn't set port\n");
		exit(1);
	}
	int new_video_format = state_->video_format;

	if ((new_video_format < 0) || (new_video_format > RTVC_FORMAT_MAX))
		new_video_format = RTVC_FORMAT_UNKNOWN;

	if (state_->video_format == 1) {
		max_fps_ = 25;
		basewidth_ = PAL_WIDTH;
		baseheight_ = PAL_HEIGHT;
	} else {
		max_fps_ = 30;
		basewidth_ = NTSC_WIDTH;
		baseheight_ = NTSC_HEIGHT;
	}
	capsize_ = basewidth_ * baseheight_ * 3;
	if (capbase_ == 0)
		capbase_ = (u_char*)malloc(capsize_ + pagesize_);
	else
		capbase_ = (u_char*)realloc(capbase_, capsize_ + pagesize_);
	int off = (int)capbase_ & (pagesize_ - 1);
	capbuf_ = capbase_ + (pagesize_ - off);
	cur_video_format_ = new_video_format;
}

void RTVCGrabber::loadFirmware(int fid)
{
	rtvc_program_t program;
	struct stat fs;

	fid_ = -1;
	if (ioctl(fd_, RTVC_CMD_RESET, 0) < 0) {
		perror("RTVC_CMD_RESET");
		exit(1);
	}
	int arfd = open(arpath_, O_RDONLY);
	if (arfd < 0) {
		perror(arpath_);
		exit(1);
	}
	fstat(arfd, &fs);
	char* cp = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, arfd, 0);
	close(arfd);
	if (cp != (char*) -1) {
		program.program = (u_int32_t*)(cp + ardir_[fid]);
		program.buffer = 0;
		program.error = 0;

		int rc = ioctl(fd_, RTVC_CMD_PROGRAM, &program);
		if (rc < 0) {
			perror("RTVC_CMD_PROGRAM");
			exit(1);
		}
		(void)munmap((caddr_t)cp, fs.st_size);
		fid_ = fid;
	}
}

struct scale_fpb {
	u_int num_lines;
	u_int scanline_dmas;
	u_int even_word_stride;
	u_int buffer_size;
};

void RTVCGrabber::set_size_nachos(int w, int h)
{
	int fid = firmwareFID();
	if (fid != fid_) {
		loadFirmware(fid);
		run();
	}
	updateParameters();
	u_int bytes_per_image = w * h * 3;
	fpb_.buffer_byte_size = (bytes_per_image + 3) &~ 3;
	scale_fpb* spb = (scale_fpb*)fpb_.firmware_message.data;
	spb->num_lines = h * 2;
	spb->scanline_dmas = (basewidth_ >> 7) - 1;
	spb->even_word_stride = (decimate_ >> 1) << 2;
	spb->buffer_size = bytes_per_image;
	sendMessage();
}

int RTVCGrabber::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "port") == 0) {
			int newport;
			if (strcasecmp(argv[2], "composite-1") == 0)
				newport = RTVC_PORT_COMPOSITE_VIDEO_1;
			else if (strcasecmp(argv[2], "composite-2") == 0)
				newport = RTVC_PORT_COMPOSITE_VIDEO_2;
			else
				newport = RTVC_PORT_S_VIDEO;

			setInputPort(newport);
			/* video format may have changed when port changed */
			setsize();
			return (TCL_OK);
		} else if (strcmp(argv[1], "decimate") == 0) {
			int d = atoi(argv[2]);
			if (d <= 0) {
				Tcl& tcl = Tcl::instance();
				tcl.resultf("%s: divide by zero", argv[0]);
				return (TCL_ERROR);
			}
			decimate_ = d;
			setsize();
			return (TCL_OK);
		}
	}
	return (Grabber::command(argc, argv));
}

void RTVCGrabber::fps(int f)
{
	if (f <= 0)
		f = 1;
	else if (u_int(f) > max_fps_)
		f = max_fps_;
	/* convert to skip count then back */
	int sc = max_fps_ / f;
	fpb_.firmware_message.image_skip = sc - 1;
	if (running_)
		sendMessage();
	Grabber::fps(max_fps_ / sc);
}

void RTVCGrabber::start()
{
	setsize();
	link(fd_, TK_READABLE);
	double now = gettimeofday_usecs();
	frameclock_ = now;
	nextframetime_ = now + tick(grab());
}

void RTVCGrabber::stop()
{
	unlink();
}

int RTVCGrabber::capture()
{
	int cc = pread(fd_, (void*)capbuf_, capsize_,
		       RTVC_READ_CAPTURED_OFFSET);
	return (cc);
}

void RTVCGrabber::dispatch(int)
{
	double now = gettimeofday_usecs();
	if (nextframetime_ > now) {
		/*
		 * the frame is too early & we want to flush it.
		 * unfortunately, the sunvideo driver doesn't provide
		 * any mechanism for flushing frames.  So we do an inefficient
		 * 2MB read to get rid of the sucker.
		 */
		u_char buf[768*576*3 + 8192];
		pread(fd_, (void*)((u_int)(buf + pagesize_) &~ (pagesize_ - 1)),
		      768*576*3, RTVC_READ_CAPTURED_OFFSET);
	} else
		nextframetime_ = tick(grab()) + now;
}

/*
 * convert timestamp of most recent frame to 32 bit rtp timestamp format.
 * this must be called after 'capture' and before the next read.
 */
u_int32_t RTVCGrabber::media_ts()
{
	/*
	 * the conversion constant is 90000 / 10^9 but this is
	 * multiplied by 2^32 so we can multiply rather than
	 * divide to get the scaled ts.
	 */
	unsigned long long ts = state_->capture.timestamp * 386547;
	u_int32_t t = ts >> 32;
	return (t + offset_);
}

/*
 * return the media timestamp corresponding to the current unix time.
 * we use that fact that the media ts is an hrtime.
 */
u_int32_t RTVCGrabber::ref_ts()
{
	unsigned long long ts = (unsigned long long)gethrtime() * 386547;
	u_int32_t t = ts >> 32;
	return (t + offset_);
}

int RTVCYuvGrabber::grab()
{
	capture();
	int istride = inw_ * 3;
	suppress(capbuf_, istride);
	saveblks(capbuf_, istride);
	u_int32_t ts = media_ts();
	YuvFrame f(ts, frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
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

void RTVCYuvGrabber::suppress(const u_char* devbuf, int is)
{
	const u_char* start = frame_ + 16 * vstart_ * outw_ + 16 * hstart_;
	REPLENISH(devbuf, start, is, 3,
		  hstart_, hstop_, vstart_, vstop_);
}

void RTVC422Grabber::setsize()
{
	int w = basewidth_ / decimate_;
	int h = baseheight_ / decimate_;
	set_size_nachos(w, h);
	set_size_422(w, h);
}

inline void 
RTVC422Grabber::saveblk(const u_char* in,
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

void RTVC422Grabber::saveblks(const u_char* devbuf, int is)
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
				saveblk(devbuf, lum, chm, chm + off, outw_, is);

			devbuf += 48;
			lum += 16;
			chm += 8;
		}
		lum += stride;
		chm += stride >> 1;
		devbuf += istride;
	}
}

void RTVCCIFGrabber::setsize()
{
	int w = basewidth_ / decimate_;
	int h = baseheight_ / decimate_;
	set_size_nachos(w, h);
	set_size_cif(w, h);
}

inline void 
RTVCCIFGrabber::saveblk(const u_char* in,
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

void RTVCCIFGrabber::saveblks(const u_char* in, int is)
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

RTVCJpegGrabber::RTVCJpegGrabber(int devno) : RTVCGrabber(devno), q_(40)
{
}

void RTVCJpegGrabber::setq(int q)
{
	/* XXX
	 * the cl4000 jpeg microcode (deliberately!?!) halts the
	 * processor if any dct coef. saturates.
	 * So to keep this from happening we (silently)
	 * clamp q at 80.
	 */
	if (q > 80)
		q = 80;
	q_ = q;
	setsize();
}

int RTVCJpegGrabber::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "q") == 0) {
			int q = atoi(argv[2]);
			setq(q);
			return (TCL_OK);
		}
	}
	return (RTVCGrabber::command(argc, argv));
}

int RTVCJpegGrabber::firmwareFID() const
{
	return is_pal()? JPEG_PAL_FID : JPEG_FID;
}

#include <math.h>

static void
build_qtable(u_int* buf, const int* qt0, const int* qt1)
{
	double cr[8], cc[8];
	cc[0] = cr[0] = cos(M_PI / 4.);
	cc[1] = (cr[1] = cos(M_PI / 16.)) * 2.;
	cc[2] = cr[2] = cos(M_PI / 8.);
	cc[3] = cr[3] = cos(3 * M_PI / 16.);
	cc[4] = cr[4] = cos(M_PI / 4.);
	cc[5] = (cr[5] = cos(5 * M_PI / 16.) * 2.) / 2.;
	cc[6] = (cr[6] = cos(3 * M_PI / 8.) * 2.) / 2.;
	cc[7] = (cr[7] = cos(7 * M_PI / 16.) * 4.) / 2.;

	for (int r = 0; r < 8; r += 2) {
		for (int c = 0; c < 8; ++c) {
			double cs = cr[r] * cc[c] * 65536.;
			int qle = int(cs / double(qt0[r << 3 | c]) + .5);
			int qce = int(cs / double(qt1[r << 3 | c]) + .5);

			cs = cr[r + 1] * cc[c] * 65536.;
			int qlo = int(cs / double(qt0[(r+1) << 3 | c]) + .5);
			int qco = int(cs / double(qt1[(r+1) << 3 | c]) + .5);

			buf[0]  = (qle << 16) | (qlo & 0xffff);
			buf[64] = (buf[32] = (qce << 16) | (qco & 0xffff));
			++buf;
		}
	}
}

extern void jpeg_chroma_qt(int q, int* qt);
extern void jpeg_luma_qt(int q, int* qt);

void RTVCJpegGrabber::setsize()
{
	int qt0[64];
	int qt1[64];
	jpeg_luma_qt(q_, qt0);
	jpeg_chroma_qt(q_, qt1);

	int fid = firmwareFID();
	if (fid != fid_) {
		loadFirmware(fid);
		run();
	}
	updateParameters(1);
	inw_ = basewidth_ / decimate_;
	inh_ = baseheight_ / decimate_;
	fpb_.buffer_byte_size = inw_ * inh_ * 2;
	u_int* jpeg_fpb = (u_int*)fpb_.firmware_message.data;
	*jpeg_fpb++ = 1;
	build_qtable(jpeg_fpb, qt0, qt1);
	sendMessage();
}

int RTVCJpegGrabber::grab()
{
	tx_->flush();

	int cc = capture();
	if (cc <= 0)
		return (0);
	u_int32_t ts = media_ts();
	JpegFrame f(ts, capbuf_, cc, q_, 1, inw_, inh_);
	return (target_->consume(&f));
}
