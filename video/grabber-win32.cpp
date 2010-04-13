/*
 *  Copyright (c) 1996 John Brezak
 *  Copyright (c) 1996 Isidor Kouvelas (University College London)
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "config.h"
#include <windows.h>
//#include <mmsystem.h>
#include <vfw.h>
#include <ctype.h>

#include "grabber.h"
#include "device-input.h"
#include "module.h"
#include "rgb-converter.h"


extern "C" HINSTANCE TkWinGetAppInstance();

int capture_;

static const int NTSC_BASE_WIDTH  = 640;
static const int NTSC_BASE_HEIGHT = 480;
static const int PAL_BASE_WIDTH  = 768;
static const int PAL_BASE_HEIGHT = 568;
static const int CIF_BASE_WIDTH  = 704;
static const int CIF_BASE_HEIGHT = 576;

static int bit_options[] = { 16, 24, 32, 8, 4, 1, 0 };

enum device_type_e {
	Generic,
	gray_QuickCam_95,
	gray_QuickCam_NT,
	MCT,
	SMII,
	Miro_dc20_95,
	Miro_dc20_NT,
	AV_Master,
	Intel_SVR3,
};

static device_type_e
get_device_type(const char *deviceName)
{
	if (!strncmp(deviceName, "QuickCam", 8)) {
		debug_msg("Device=gray_QuickCam_95\n");
		return (gray_QuickCam_95);
	}
	if (!strncmp(deviceName, "quickcam.dll", 12)) {
		debug_msg("Device=gray_QuickCam_NT\n");
		return (gray_QuickCam_NT);
	}
	if (!strncmp(deviceName, "miroVIDEO DC20", 14)) {
		debug_msg("Device=Miro_dc20_95\n");
		return (Miro_dc20_95);
	}
	if (!strncmp(deviceName, "dc20.dll", 8)) {
		debug_msg("Device=Miro_dc20_NT\n");
		return (Miro_dc20_NT);
	}
	if (!strncmp(deviceName, " MCT VMPlus", 10)) {
		debug_msg("Device=MCT\n");
		return (MCT);
	}
	if (!strncmp(deviceName, "Screen Machine II", 16)) {
		debug_msg("Device=SMII\n");
		return (SMII);
	}
	if (!strncmp(deviceName, "AV Master", 9)) {
		debug_msg("Device=AV_Master\n");
		return (AV_Master);
	}
	if (!strncmp(deviceName, "ISVR III", 8)) {
		debug_msg("Device=ISVR III\n");
		return (Intel_SVR3);
	}

	debug_msg("Device=Generic: %s\n", deviceName);
	return (Generic);
}

static char *
get_comp_name(DWORD fourcc)
{
	static char name[5];

	switch (fourcc) {
	case 0:
		return ("BI_RGB");
	default:
		char *p = (char *)&fourcc;
		for (int i = 0; i < 4; i++)
			if (isprint(p[i]))
				name[i] = p[i];
			else
				name[i] = ' ';
		name[4] = 0;
		return (name);
	}
}

class IC_Converter : public Converter {
public:
	IC_Converter(DWORD comp, int bpp, int inw, int inh);
	~IC_Converter();
	virtual void convert(u_int8_t *in, int inw, int inh, u_int8_t *frm, int outw, int outh, int invert);
protected:
	BITMAPINFOHEADER	bihIn_, bihOut_;
	HIC			hIC_;
	RGB_Converter		*converter_;
	u_char			*rgb_;
	int			rgb_bpp_;
};

IC_Converter::IC_Converter(DWORD comp, int bpp, int inw, int inh)
	: converter_(0), rgb_(0), rgb_bpp_(0), hIC_(0)
{
	debug_msg("IC_Converter: comp=%x (%s) bpp=%d\n", comp, get_comp_name(comp), bpp);

	bihIn_.biSize = bihOut_.biSize = sizeof(BITMAPINFOHEADER);
	bihIn_.biXPelsPerMeter = bihIn_.biYPelsPerMeter = bihOut_.biXPelsPerMeter = bihOut_.biYPelsPerMeter = 0;
	bihIn_.biPlanes = bihOut_.biPlanes= 1;

	bihIn_.biCompression = comp;
	bihIn_.biClrUsed = bihOut_.biClrImportant = 0;
	bihIn_.biWidth = inw;
	bihIn_.biHeight = inh;
	bihIn_.biBitCount = bpp;
	bihIn_.biSizeImage = bihIn_.biWidth * bihIn_.biHeight * bihIn_.biPlanes * bihIn_.biBitCount / 8;

	bihOut_.biCompression = BI_RGB;
	bihOut_.biClrUsed = bihOut_.biClrImportant = 0;
	bihOut_.biWidth = inw;
	bihOut_.biHeight = inh;
	int i = 0;
	do {
		bihOut_.biBitCount = bit_options[i++];
		bihOut_.biSizeImage = bihOut_.biWidth * bihOut_.biHeight * bihOut_.biPlanes * bihOut_.biBitCount / 8;
		if (hIC_ = ICLocate(ICTYPE_VIDEO, 0L, (LPBITMAPINFOHEADER)&bihIn_, (LPBITMAPINFOHEADER)&bihOut_, ICMODE_DECOMPRESS))
			break;
	} while (bihOut_.biBitCount > 0);

	if (hIC_) {
		ICINFO icinfo;
		ICGetInfo(hIC_, &icinfo, sizeof(icinfo));
		debug_msg("IC: Located %s with bpp %d\n", icinfo.szName, bihOut_.biBitCount);
	}

	if (bihOut_.biBitCount <= 8)
		hIC_ = 0;

	rgb_ = new u_char[bihOut_.biSizeImage];
	rgb_bpp_ = bihOut_.biBitCount;
	debug_msg("IC_Converter: rgb_bpp_ = %d\n", rgb_bpp_);

	if (hIC_ == 0)
		fprintf(stderr, "ICLocate: Unable to find supported bpp for format %x!\n", comp);
	else if (ICDecompressBegin(hIC_, &bihIn_, &bihOut_) != ICERR_OK) {
		fprintf(stderr, "ICDecompressBegin failed!\n");
		hIC_ = 0;
	}
}

IC_Converter::~IC_Converter()
{
	if (rgb_) {
		delete [] rgb_;
		rgb_ = 0;
	}
	if (hIC_) {
		ICDecompressEnd(hIC_);
		hIC_ = 0;
	}
	if (converter_) {
		delete converter_;
		converter_ = 0;
	}
}

void
IC_Converter::convert(u_int8_t *in, int inw, int inh, u_int8_t *frm, int outw, int outh, int invert)
{
	if (hIC_ == 0)
		return;

	if (ICDecompress(hIC_, 0, &bihIn_, in, &bihOut_, rgb_) != ICERR_OK)
		debug_msg("ICDecompress failed!\n");

	converter_->convert(rgb_, inw, inh, frm, outw, outh, invert);
}

class IC_Converter_420 : public IC_Converter {
public:
	IC_Converter_420(DWORD comp, int bpp, int inw, int inh);
};

IC_Converter_420::IC_Converter_420(DWORD comp, int bpp, int inw, int inh)
	: IC_Converter(comp, bpp, inw, inh)
{
	converter_ = new RGB_Converter_420(rgb_bpp_, NULL, 0);
}

class IC_Converter_422 : public IC_Converter {
public:
	IC_Converter_422(DWORD comp, int bpp, int inw, int inh);
};

IC_Converter_422::IC_Converter_422(DWORD comp, int bpp, int inw, int inh)
	: IC_Converter(comp, bpp, inw, inh)
{
	converter_ = new RGB_Converter_422(rgb_bpp_, NULL, 0);
}

class YUYV_Converter_420 : public Converter {
public:
	virtual void convert(u_int8_t *in, int inw, int inh, u_int8_t *frm, int outw, int outh, int invert = 0);
};

void YUYV_Converter_420::convert(u_int8_t *in, int inw, int inh, u_int8_t *frm, int outw, int outh, int invert)
{
	u_int8_t *yp = (u_int8_t*)frm;
	int off = outw * outh;
	u_int8_t *up = (u_int8_t*)(frm + off);
	off += off >> 2;
	u_int8_t *vp = (u_int8_t*)(frm + off);

	unsigned short *p = (unsigned short *)in;

	int h = min(inh, outh);
	int w = min(inw, outw);

	int next_line = inw * 2;
	if (invert) {
		in += 2 * inw * (inh - 1 - (inh - h) / 2);
		next_line = -next_line;
	} else
		in += 2 * inw * (inh - h) / 2;
	int inpad = (inw - w) * 2;
	in += inpad / 2;
	int outpad = outw - w;
	int outvpad = ((outh - h) / 2) & (outw > 176? ~0xf: ~0x3);

	yp += outw * outvpad + outpad / 2;
	up += outw / 4 * outvpad + outpad / 4;
	vp += outw / 4 * outvpad + outpad / 4;

	for (h >>= 1; --h > 0;) {
		for (int x = w; x > 0; x -= 2) {
			yp[outw] = in[next_line];
			*yp++ = *in++;
			*up++ = *in++ ^ 0x80;

			yp[outw] = in[next_line];
			*yp++ = *in++;
			*vp++ = *in++ ^ 0x80;
		}
		in += inpad;
		yp += outw + outpad;
		up += outpad >> 1;
		vp += outpad >> 1;
		if (invert)
			in -= 6 * inw;
		else
			in += inw << 1;
	}
}

class YUYV_Converter_422 : public Converter {
public:
	virtual void convert(u_int8_t *in, int inw, int inh, u_int8_t *frm, int outw, int outh, int invert = 0);
};

void YUYV_Converter_422::convert(u_int8_t *in, int inw, int inh, u_int8_t *frm, int outw, int outh, int invert)
{
	u_int8_t *yp = (u_int8_t*)frm;
	int off = outw * outh;
	u_int8_t *up = (u_int8_t*)(frm + off);
	off += off >> 1;
	u_int8_t *vp = (u_int8_t*)(frm + off);

	assert(inw == outw);
	
	if (invert)
		in += 2 * inw * (inh - 1 - (inh - outh) / 2);
	else
		in += 2 * inw * (inh - outh) / 2;

	for (int h = outh; h > 0; h--) {
		for (int w = outw; w > 0; w -= 2) {
			*yp++ = *in++;
			*up++ = *in++ ^ 0x80;
			*yp++ = *in++;
			*vp++ = *in++ ^ 0x80;
		}
		if (invert)
			in -= inw << 2;
	}
}

class VfwGrabber;
class VfwGrabber : public Grabber {
 public:
	VfwGrabber(const int dev);
	virtual ~VfwGrabber();
	virtual int command(int argc, const char*const* argv);
	inline void converter(Converter* v) { converter_ = v; }
	void capture(VfwGrabber *gw, LPBYTE);
	inline int is_pal() const { return (max_fps_ == 25); }
	int capturing_;

 protected:
	virtual void start();
	virtual void stop();
	virtual void fps(int);
	virtual void setsize() = 0;
	virtual int grab();
	void setport(const char *port);

	device_type_e devtype_;
	int useconfig_;
	int dev_;
	int connected_;
	u_int max_fps_;
	int basewidth_;
	int baseheight_;
	u_int decimate_;	/* division of base sizes */
	
	HWND			capwin_;
	CAPDRIVERCAPS		caps_;
	CAPSTATUS		status_;
	CAPTUREPARMS		parms_;
	LPBITMAPINFOHEADER	fmt_;
	u_int			fmtsize_;

	HANDLE			frame_sem_;
	HANDLE			cb_mutex_;
	LPBYTE			last_frame_;
	Converter		*converter_;
	
 private:
	static LRESULT CALLBACK VideoHandler(HWND, LPVIDEOHDR);
	static LRESULT CALLBACK ErrorHandler(HWND, int, LPCSTR);
};

class VfwCIFGrabber : public VfwGrabber 
{
 public:
	VfwCIFGrabber(const int dev);
	~VfwCIFGrabber();
 protected:
	virtual void start();
	virtual void setsize();
};

class Vfw420Grabber : public VfwGrabber 
{
 public:
	Vfw420Grabber(const int dev);
	~Vfw420Grabber();
 protected:
	virtual void start();
	virtual void setsize();
};

class Vfw422Grabber : public VfwGrabber 
{
 public:
	Vfw422Grabber(const int dev);
	~Vfw422Grabber();
 protected:
	virtual void start();
	virtual void setsize();
};

class VfwDevice : public InputDevice {
 public:
	VfwDevice(const char* name, int index);
	~VfwDevice();
	virtual int command(int argc, const char*const* argv);
 protected:
	DWORD vfwdev_;
	VfwGrabber *grabber_;
};

#define NUM_DEVS 12

class VfwScanner {
 public:
	VfwScanner(const int n);
	~VfwScanner();
 protected:
	VfwDevice *devs_[NUM_DEVS];
};

static VfwScanner find_vfw_devices(NUM_DEVS);

VfwScanner::VfwScanner(const int n)
{
	char deviceName[80] ;
	char deviceVersion[100] ;
	
	for (int index = 0 ; index < n; index++) {
		if (capGetDriverDescription(index,
					    (LPSTR)deviceName,
					    sizeof(deviceName),
					    (LPSTR)deviceVersion,
					    sizeof(deviceVersion))) {
			debug_msg("Adding device %d\n", index);
			devs_[index] = new VfwDevice(strdup(deviceName), index);
		} else
			devs_[index] = NULL;
	}
}

VfwScanner::~VfwScanner()
{
	debug_msg("~VfwScanner\n");
	for (int i = 0; i < NUM_DEVS; i++)
		if (devs_[i]) {
			debug_msg("Deleteing device %d\n", i);
			delete devs_[i];
	}
}

VfwDevice::VfwDevice(const char* name, int index) :
	InputDevice(name), vfwdev_(DWORD(-1)), grabber_(0)
{
	debug_msg("VfwDevice: [%d] \"%s\"\n", index, name);

	if (index >= 0) {
		vfwdev_ = index;
		switch (get_device_type(name)) {
		case gray_QuickCam_95:
			attributes_ = "format { 422 420 cif } size { small cif } port { QuickCam } type { ntsc pal }";
			break;
		case Generic:
		default:
			attributes_ = "format { 422 420 cif } size { large small cif } port { external-in } type { ntsc pal }";
			break;
		}
	} else
		attributes_ = "disabled";
}

VfwDevice::~VfwDevice()
{
	/* The following should not be necessary but there is no call to
	 * delete the grabber when vic exits while capturing. */
	if (grabber_) {
		if (capture_==1) {
			debug_msg("VfwDevice::~VfwDevice deleting grabber!\n");
			delete grabber_;
		}
	}
}

int VfwDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if ((argc == 3) && (strcmp(argv[1], "open") == 0)) {
		TclObject* o = 0;
		if (strcmp(argv[2], "422") == 0)
			o = grabber_ = new Vfw422Grabber(vfwdev_);
		if (strcmp(argv[2], "420") == 0)
			o = grabber_ = new Vfw420Grabber(vfwdev_);
		else if (strcmp(argv[2], "cif") == 0)
			o = grabber_ = new VfwCIFGrabber(vfwdev_);
		if (o != 0)
			Tcl::instance().result(o->name());
		return (TCL_OK);
	}
	return (InputDevice::command(argc, argv));
}

VfwGrabber::VfwGrabber(const int dev) : dev_(dev), connected_(0),
	last_frame_(0), devtype_(Generic), useconfig_(0), converter_(0), fmt_(0), frame_sem_(0), cb_mutex_(0)
{
	char deviceName[80] ;
	char deviceVersion[100] ;
	
	if (!capGetDriverDescription(dev, (LPSTR)deviceName, sizeof(deviceName), (LPSTR)deviceVersion, sizeof(deviceVersion))) {
		fprintf(stderr, "VfwGrabber: Cannot get driver info!\n");
		Grabber::status_=-1;
		return;
	}

	devtype_ = get_device_type(deviceName);
	setport("external");
	if (is_pal()) {
		basewidth_ = PAL_BASE_WIDTH;
		baseheight_ = PAL_BASE_HEIGHT;
	} else {
		basewidth_ = NTSC_BASE_WIDTH;
		baseheight_ = NTSC_BASE_HEIGHT;
	}
}

VfwGrabber::~VfwGrabber()
{
	debug_msg("~VfwGrabber\n");
	if (capwin_) {
		if (capturing_) {
			debug_msg("stopping...\n");
			capCaptureStop(capwin_);
			ReleaseSemaphore(frame_sem_, 1, NULL);
			CloseHandle(frame_sem_);
			capturing_ = 0;
			capture_=0;
		}
		if (connected_) {
			debug_msg("disconnecting...\n");
			capDriverDisconnect(capwin_);
			connected_ = 0;
		}
		capSetCallbackOnVideoStream(capwin_, NULL);
		DestroyWindow(capwin_);
	}
}

VfwCIFGrabber::VfwCIFGrabber(const int dev) : VfwGrabber(dev)
{
	debug_msg("VfwCIFGrabber\n");
}

VfwCIFGrabber::~VfwCIFGrabber()
{
	debug_msg("~VfwCIFGrabber\n");
}

void VfwCIFGrabber::setsize()
{
	int w = basewidth_ / decimate_;
	int h = baseheight_ / decimate_;
	debug_msg("VfwCIFGrabber::setsize: %dx%d\n", w, h);
	set_size_cif(w, h);
	allocref();
}

Vfw420Grabber::Vfw420Grabber(const int dev) : VfwGrabber(dev)
{
	debug_msg("Vfw420Grabber\n");
}

Vfw420Grabber::~Vfw420Grabber()
{
	debug_msg("~Vfw420Grabber\n");
}

void Vfw420Grabber::setsize()
{
	int w = basewidth_ / decimate_;
	int h = baseheight_ / decimate_;
	debug_msg("Vfw420Grabber::setsize: %dx%d\n", w, h);
	set_size_420(w, h);
	allocref();
}

Vfw422Grabber::Vfw422Grabber(const int dev) : VfwGrabber(dev)
{
	debug_msg("Vfw422Grabber\n");
}

Vfw422Grabber::~Vfw422Grabber()
{
	debug_msg("~Vfw422Grabber\n");
}
void Vfw422Grabber::setsize()
{
	int w = basewidth_ / decimate_;
	int h = baseheight_ / decimate_;
	debug_msg("Vfw422Grabber::setsize: %dx%d\n", w, h);
	set_size_422(w, h);
	allocref();
}

void VfwGrabber::fps(int f)
{
	if (f <= 0)
		f = 1;
	else if (u_int(f) > max_fps_)
		f = max_fps_;
	Grabber::fps(f);

#ifdef NDEF
	if (capturing_) {
		stop();
		start();
	}
#endif
}

void VfwGrabber::start()
{
	debug_msg("VfwGrabber::start() thread=%x\n", GetCurrentThreadId());
	debug_msg("basewidth_=%d, baseheight_=%d, decimate_=%d\n", basewidth_, baseheight_, decimate_);

	/*
	 * The quickcam driver seems to completely ignore the information
	 * given in the capSetVideoFormat call. To get around this we have
	 * to set the desired values in the quickcam.ini file before we
	 * connect the driver.
	 */
	if (devtype_ == gray_QuickCam_95) {
		/* Should really use binary name in calls below instad of Vic */
		debug_msg("argv %s\n", *__argv);
		switch (decimate_) {
		case 4:
			WritePrivateProfileString("Vic", "Size40", "4", "quickcam.ini");
			WritePrivateProfileString("Vic", "Xfermode", "4", "quickcam.ini");
			break;
		case 2:
			WritePrivateProfileString("Vic", "Size40", "8", "quickcam.ini");
			WritePrivateProfileString("Vic", "Xfermode", "0", "quickcam.ini");
			break;
		default:
			debug_msg("Quickcam cannot do this decimation!\n");
			break;
		}
	}

	/* lock out VideoHandler until everything is set up - cmg */
	cb_mutex_ = CreateMutex(NULL,FALSE,NULL);
	WaitForSingleObject(cb_mutex_,INFINITE);

	if ((capwin_ = capCreateCaptureWindow((LPSTR)"Capture Window", WS_POPUP | WS_CAPTION,
		CW_USEDEFAULT, CW_USEDEFAULT, (basewidth_ / decimate_ + GetSystemMetrics(SM_CXFIXEDFRAME)), 
		(baseheight_ / decimate_ + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME)), 
		(HWND) 0, (int) 0)) == NULL) {
		  debug_msg("capCreateCaptureWindow: failed - %lu\n", capwin_);
		  abort();
	}
	capSetCallbackOnError(capwin_, ErrorHandler);
	if (!capSetCallbackOnVideoStream(capwin_, VideoHandler)) {
		debug_msg("capSetCallbackOnVideoStream: failed - %lu\n", GetLastError());
		/*abort();*/
	}
	if (!capDriverConnect(capwin_, dev_)) {
		debug_msg( "capDriverConnect: dev=%d failed - %lu\n", dev_, GetLastError());
		stop();
		return;
		/*abort();*/
	}

	if (useconfig_) {
		capDlgVideoFormat(capwin_);
		capDlgVideoSource(capwin_);
		capDlgVideoDisplay(capwin_);
	}

	capSetUserData(capwin_, this);
	debug_msg("SetUserData=%x\n", this);
	connected_ = 1;

	if (!capDriverGetCaps(capwin_, &caps_, sizeof(caps_))) {
		debug_msg( "capGetDriverCaps: failed - %lu\n", GetLastError());
		/*abort();*/
	}

	debug_msg("capdrivercaps: overlay=%d dlgSource=%d dlgFmt=%d dlgDis=%d init=%d pal=%d\n",
		caps_.fHasOverlay, caps_.fHasDlgVideoSource,
		caps_.fHasDlgVideoFormat, caps_.fHasDlgVideoDisplay,
		caps_.fCaptureInitialized, caps_.fDriverSuppliesPalettes);
	
	fmtsize_ = capGetVideoFormatSize(capwin_);
	fmt_ = (LPBITMAPINFOHEADER)new u_char[fmtsize_];
	if (!capGetVideoFormat(capwin_, fmt_, fmtsize_)) {
		debug_msg("capGetVideoFormat: failed - %lu\n", GetLastError());
		/*abort();*/
	}
	int orig_comp = fmt_->biCompression;
	int orig_bpp = fmt_->biBitCount;
	debug_msg("Original comp=%x (%s) bpp=%d\n", orig_comp, get_comp_name(orig_comp), orig_bpp);

	fmt_->biPlanes = 1;

	/* if the driver isn't set to either pal or ntsc then use default ntsc values */
	if ((fmt_->biWidth != 192 && fmt_->biWidth != 176 && fmt_->biWidth != 384 && fmt_->biWidth != 352 && fmt_->biWidth != 768 && fmt_->biWidth != 704) ||
		(fmt_->biHeight!= 142 && fmt_->biHeight!= 144 && fmt_->biHeight!= 284 && fmt_->biHeight!= 288 && fmt_->biHeight!= 568 && fmt_->biHeight!= 576)) {
		fmt_->biWidth = basewidth_ / decimate_;
		fmt_->biHeight = baseheight_ / decimate_;
	}

	switch (devtype_) {
	case gray_QuickCam_NT:
	case Miro_dc20_95:
	case Miro_dc20_NT:
	case AV_Master:
	case Generic:
		if (useconfig_) {
			if (caps_.fHasDlgVideoFormat) capDlgVideoFormat(capwin_);
			if (caps_.fHasDlgVideoSource) capDlgVideoSource(capwin_);
		}
		delete [] fmt_;
		fmtsize_ = capGetVideoFormatSize(capwin_);
		fmt_ = (LPBITMAPINFOHEADER) new u_char [fmtsize_];
		capGetVideoFormat(capwin_, fmt_, fmtsize_);
		break;
	case Intel_SVR3:
		/* the driver does not like to generic query much
		 * just use yuv9 */
		fmt_->biCompression = mmioFOURCC('Y','V','U','9');
		fmt_->biBitCount = 9;
		fmt_->biSizeImage = fmt_->biWidth * fmt_->biHeight * fmt_->biPlanes * fmt_->biBitCount / 8;
		if (!capSetVideoFormat(capwin_, fmt_, fmtsize_)) {
			debug_msg("Intel SVR3 format failed!\n");
			fprintf(stderr, "Unable to set SVR3 to YUV9 format!\n");
			stop();
			abort();
		}
		break;
	case gray_QuickCam_95:
		/* We cannot use Generic as QuickCam says YES to anything
		 * but does not do it! */
		fmt_->biCompression = BI_RGB;
		fmt_->biBitCount = 4;
		fmt_->biSizeImage = fmt_->biWidth * fmt_->biHeight * fmt_->biPlanes * fmt_->biBitCount / 8;
		capSetVideoFormat(capwin_, fmt_, fmtsize_);
		break;
	case MCT:
#ifdef NDEF
		fmt_->biCompression = mmioFOURCC('M','Y','4','2');
		fmt_->biBitCount = 16;
		fmt_->biSizeImage = fmt_->biWidth * fmt_->biHeight * fmt_->biPlanes * fmt_->biBitCount / 8;
		if (capSetVideoFormat(capwin_, fmt_, fmtsize_))
			break;
		debug_msg("MCT MY42 format failed!\n");
#endif
		goto Generic;
	case SMII:
		fmt_->biCompression = mmioFOURCC('Y','U','Y','V');
		fmt_->biBitCount = 16;
		fmt_->biSizeImage = fmt_->biWidth * fmt_->biHeight * fmt_->biPlanes * fmt_->biBitCount / 8;
		if (capSetVideoFormat(capwin_, fmt_, fmtsize_))
			break;
		debug_msg("SMII YUYV format failed!\n");
		goto Generic;
	Generic:
	default:
		/* Try to figure out what compression formats are supported. */
		fmt_->biCompression = BI_RGB;
		int i = 0;
		do {
			fmt_->biBitCount = bit_options[i++];
			fmt_->biSizeImage = fmt_->biWidth * fmt_->biHeight * fmt_->biPlanes * fmt_->biBitCount / 8;
			if (capSetVideoFormat(capwin_, fmt_, fmtsize_))
				break;
		} while (fmt_->biBitCount > 0);
		if (fmt_->biBitCount == 0)
			debug_msg("Unable to find supported RGB format!\n");
		else
			break;

		/* RGB failed. Try using a decompressor... */
		fmt_->biCompression = orig_comp;
		fmt_->biBitCount = orig_bpp;
		fmt_->biSizeImage = fmt_->biWidth * fmt_->biHeight * fmt_->biPlanes * fmt_->biBitCount / 8;
		if (!capSetVideoFormat(capwin_, fmt_, fmtsize_)) {
			fprintf(stderr, "Unable to set size in native fmt!\n");
			stop();
			abort();
		}
		break;
	}

	/* OK now lets see what the driver really thinks about the format! */
	delete [] fmt_;
	fmtsize_ = capGetVideoFormatSize(capwin_);
	fmt_ = (LPBITMAPINFOHEADER) new u_char [fmtsize_];
	capGetVideoFormat(capwin_, fmt_, fmtsize_);
	debug_msg("bitmapinfo: comp= %x (%s) w=%d h=%d planes=%d bitcnt=%d szImage=%d ClrUsed=%d ClrImp=%d\n",
		fmt_->biCompression, get_comp_name(fmt_->biCompression),
		fmt_->biWidth, fmt_->biHeight,
		fmt_->biPlanes, fmt_->biBitCount, fmt_->biSizeImage,
		fmt_->biClrUsed, fmt_->biClrImportant);

	assert(fmt_->biPlanes == 1);
	debug_msg("Using biBitCount = %d\n", fmt_->biBitCount);
	if (fmt_->biWidth != (LONG) (basewidth_ / decimate_)) {
		/* The driver is totally stupid so accept it's settings! */
		debug_msg("Stupid driver. Accepting %x %d*%d*%d\n", fmt_->biCompression, fmt_->biWidth, fmt_->biHeight, fmt_->biBitCount);
		switch (fmt_->biWidth) {
			case 640:
			case 320:
			case 160:
				max_fps_ = 30;
				basewidth_ = NTSC_BASE_WIDTH;
				baseheight_ = NTSC_BASE_HEIGHT;
				break;
			case 704:
			case 352:
			case 176:
				max_fps_ = 25;
				basewidth_ = CIF_BASE_WIDTH;
				baseheight_ = CIF_BASE_HEIGHT;
				break;
			case 768:
			case 384:
			case 192:
				max_fps_ = 25;
				basewidth_ = PAL_BASE_WIDTH;
				baseheight_ = PAL_BASE_HEIGHT;
				break;

			default:
				fprintf(stderr, "Image dimensions not suitable.\n");
				max_fps_ = 25;
				basewidth_ = PAL_BASE_WIDTH;
				baseheight_ = PAL_BASE_HEIGHT;
				return;
		}
		decimate_ = basewidth_ / fmt_->biWidth;
		setsize();
	}
	
	if (!capCaptureGetSetup(capwin_, &parms_, sizeof(parms_))) {
		fprintf(stderr, "capCaptureGetSetup: failed - %lu\n", GetLastError());
		/*abort();*/
	}

	debug_msg("GetSetup: uSec=%d drop=%d DOS=%d nVid=%d yield=%d\n",
		parms_.dwRequestMicroSecPerFrame,
		parms_.wPercentDropForError,
		parms_.fUsingDOSMemory,
		parms_.wNumVideoRequested,
		parms_.fYield);

	/*1e6 / double(max_fps_);*/
	/*(DWORD)frametime_*/
	parms_.dwRequestMicroSecPerFrame = (unsigned long) (1e6 / double(max_fps_));
	parms_.wPercentDropForError = 100;
	parms_.fUsingDOSMemory = FALSE;
	parms_.wNumVideoRequested = 3;
	parms_.fYield = TRUE;
	parms_.fMakeUserHitOKToCapture = FALSE;
	parms_.fCaptureAudio = FALSE;
	parms_.vKeyAbort = 0;
	parms_.fAbortLeftMouse = FALSE;
	parms_.fAbortRightMouse = FALSE;
	parms_.fLimitEnabled = FALSE;
	parms_.fMCIControl = FALSE;
	parms_.wStepCaptureAverageFrames=1;

	if (!capCaptureSetSetup(capwin_, &parms_, sizeof(parms_))) {
		fprintf(stderr, "capCaptureSetSetup: failed - %lu\n", GetLastError());
		/*abort();*/
	}

#ifdef NDEF
	MoveWindow(capwin_, 0, 0,
		   (basewidth_ / decimate_ + GetSystemMetrics(SM_CXFIXEDFRAME)),
		   (baseheight_ / decimate_ + GetSystemMetrics(SM_CYCAPTION)
		    + GetSystemMetrics(SM_CYFIXEDFRAME)), TRUE);

	ShowWindow(capwin_, SW_SHOW);
	
	if (caps_.fHasOverlay) {
		capOverlay(capwin_, TRUE);
	} else {
		capPreviewRate(capwin_, 66);
		capPreview(capwin_, TRUE);
	}
#endif
	frame_sem_ = CreateSemaphore(NULL, 0, 1, NULL);
	if (!capCaptureSequenceNoFile(capwin_)) {
		fprintf(stderr, "capCaptureSequenceNoFile: failed - %lu\n", GetLastError());
		/*abort();*/
	} else {
		capturing_ = 1;
		capture_=1;
	}
	last_frame_ = 0;

	Grabber::start();
}

void Vfw422Grabber::start()
{
	VfwGrabber::start();
	switch (fmt_->biCompression) {
	case BI_RGB:
		converter(new RGB_Converter_422(fmt_->biBitCount, (u_int8_t *)(fmt_ + 1), fmt_->biClrUsed));
		break;
	case mmioFOURCC('Y','U','Y','V'):
		converter(new YUYV_Converter_422());
		break;
	default:
		converter(new IC_Converter_422(fmt_->biCompression, fmt_->biBitCount, fmt_->biWidth, fmt_->biHeight));
		break;
	}
	/* allow video handler callback to progress */
	ReleaseMutex(cb_mutex_);
	Grabber::timeout();
}


void Vfw420Grabber::start()
{
	VfwGrabber::start();
	switch (fmt_->biCompression) {
	case BI_RGB:
		converter(new RGB_Converter_420(fmt_->biBitCount, (u_int8_t *)(fmt_ + 1), fmt_->biClrUsed));
		break;
	case mmioFOURCC('Y','U','Y','V'):
		converter(new YUYV_Converter_420());
		break;
	default:
		converter(new IC_Converter_420(fmt_->biCompression, fmt_->biBitCount, fmt_->biWidth, fmt_->biHeight));
		break;
	}
	/* allow video handler callback to progress */
	ReleaseMutex(cb_mutex_);
	Grabber::timeout();
}

void VfwCIFGrabber::start()
{
	VfwGrabber::start();
	if (fmt_!=NULL) {
	    switch (fmt_->biCompression) {
		case BI_RGB:
		    converter(new RGB_Converter_420(fmt_->biBitCount, (u_int8_t *)(fmt_ + 1), fmt_->biClrUsed));
		    break;
		case mmioFOURCC('Y','U','Y','V'):
		    converter(new YUYV_Converter_420());
		    break;
		default:
		    converter(new IC_Converter_420(fmt_->biCompression, fmt_->biBitCount, fmt_->biWidth, fmt_->biHeight));
		    break;
	    }
	}
	/* allow video handler callback to progress */
	ReleaseMutex(cb_mutex_);
	Grabber::timeout();
}

void VfwGrabber::stop()
{
	debug_msg("VfwWindow::stop() thread=%x\n", GetCurrentThreadId());

	if (cb_mutex_!=NULL) {
		CloseHandle(cb_mutex_);
		cb_mutex_=0;
	} else 
		return;

	if (capturing_)
		capCaptureStop(capwin_);
	/* ensure this won't be called */
	capSetCallbackOnVideoStream(capwin_, NULL);
	capturing_ = 0;
	capture_=0;
	if (frame_sem_!=0 ) {
		ReleaseSemaphore(frame_sem_, 1, NULL);
		CloseHandle(frame_sem_);
	}
#ifdef NDEF
	if (caps_.fHasOverlay)
		capOverlay(capwin_, FALSE);
	else
		capPreview(capwin_, FALSE);
#endif

	if (capwin_!=NULL) 
	    capDriverDisconnect(capwin_);
	connected_ = 0;

	if (converter_!=NULL) //if (fmt_->biCompression == BI_RGB)
		delete converter_;
	converter_ = 0;

	delete [] fmt_;

	capSetCallbackOnError(capwin_, NULL);
	DestroyWindow(capwin_);
	capwin_ = NULL;
	last_frame_ = 0;

	Grabber::stop();
}

void VfwGrabber::setport(const char *port)
{
	debug_msg("setport: %s thread=%x\n", port, GetCurrentThreadId());

	/* Decision about PAL / NTSC has to be made at this point */
	max_fps_ = 30;
}

int VfwGrabber::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if (strcmp(argv[1], "decimate") == 0) {
			u_int dec = (u_int)atoi(argv[2]);
			Tcl& tcl = Tcl::instance();
			if (dec <= 0) {
				tcl.resultf("%s: divide by zero", argv[0]);
				return (TCL_ERROR);
			}
			debug_msg("VfwGrabber::command: decimate=%d (dec)=%d\n", dec, decimate_);
			if (dec != decimate_) {
				decimate_ = dec;
				if (running_) {
					stop();
					setsize();
					start();
				} else
					setsize();
			}
			return (TCL_OK);	
		} else if (strcmp(argv[1], "port") == 0) {
			setport(argv[2]);
			return (TCL_OK);
		} else if (strcmp(argv[1], "useconfig") ==0) {
			if (strcmp(argv[2], "1") == 0) useconfig_=1;
			if (strcmp(argv[2], "0") == 0) useconfig_=0;
		} else if (strcmp(argv[1], "type") == 0) {
			if (strcmp(argv[2], "auto") == 0) {
			} else if (strcmp(argv[2], "pal") == 0) {
				basewidth_  = CIF_BASE_WIDTH;
				baseheight_ = CIF_BASE_HEIGHT;
			} else if (strcmp(argv[2], "ntsc") == 0) {
				basewidth_  = NTSC_BASE_WIDTH;
				baseheight_ = NTSC_BASE_HEIGHT;
			}
		}
	}
	return (Grabber::command(argc, argv));
}

LRESULT CALLBACK
VfwGrabber::ErrorHandler(HWND hwnd, int id, LPCSTR err)
{
	if (hwnd == NULL)
		return ((LRESULT)FALSE);
	if (id == 0)
		return ((LRESULT)TRUE);

	debug_msg("ErrorHandler: thread=%x [id=%d] %s\n",
		GetCurrentThreadId(), id, err);

	return ((LRESULT)TRUE);
}

LRESULT CALLBACK
VfwGrabber::VideoHandler(HWND hwnd, LPVIDEOHDR vh)
{
	static int not_done = 0;

	VfwGrabber *gp = (VfwGrabber*)capGetUserData(hwnd);
	/* in case we are not fast enough */
	if (gp==NULL) return ((LRESULT)TRUE);

	/* Block grab code until frame has been copied into last_frame_ (in capture function) */
	WaitForSingleObject(gp->cb_mutex_,INFINITE);

#ifdef DEBUG__	
	debug_msg("VfwGrabber::VideoHandler: thread=%x data=%x flags=%x len=%d time=%d\n",
		GetCurrentThreadId(),
		vh->lpData, vh->dwFlags, vh->dwBytesUsed, vh->dwTimeCaptured);
#endif

//	if (vh->dwFlags & VHDR_DONE)
	if (vh->dwFlags & 0x00000001)
		(gp->capture)(gp, vh->lpData);
	else if (not_done++ % 10 == 0)
		debug_msg("Frames not ready! %d\n", not_done);

	/* Release to process frame in grab */
	ReleaseMutex(gp->cb_mutex_);
	return ((LRESULT)TRUE);
}

void
VfwGrabber::capture(VfwGrabber *gw, LPBYTE frame)
{
#ifdef DEBUG
	if (gw->last_frame_ != NULL)
		debug_msg("Last frame not grabbed!\n");
#endif
	if (capturing_) gw->last_frame_ = frame;
}

int
VfwGrabber::grab()
{
#ifdef DEBUG_GRAB
	debug_msg("VfwGrabber::grab: thread=%x w=%d h=%d frame_=%d fsize_=%d in=%dx%d out=%dx%d\n",
		GetCurrentThreadId(),
		basewidth_, baseheight_, frame_, framesize_,
		inw_, inh_, outw_, outh_);
#endif

	/* block the VideoHandler callback code until we've processed frame */
	WaitForSingleObject(cb_mutex_,INFINITE);

	if (last_frame_ == NULL || capturing_ == 0) {
		ReleaseMutex(cb_mutex_);
		return (FALSE);
	}

	converter_->convert((u_int8_t*)last_frame_, basewidth_ / decimate_, baseheight_ / decimate_, frame_, outw_, outh_, TRUE);
	last_frame_ = NULL;
	suppress(frame_);
	saveblks(frame_);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	int rval = (target_->consume(&f));

	/* release block so that VideoHandler can get new frame */
	ReleaseMutex(cb_mutex_);
	return rval;
}
