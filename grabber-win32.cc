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
#include <windows.h>
#include <mmsystem.h>
#include <vfw.h>

#include "grabber.h"
#include "Tcl.h"
#include "device-input.h"
#include "module.h"
#include "rgb-converter.h"

extern "C" HINSTANCE TkWinGetAppInstance();

void dprintf(const char *fmt, ...)
{
    char szTemp[512];
    
    va_list ap;
    va_start (ap, fmt);
    wvsprintf(szTemp, fmt, ap);
    OutputDebugString(szTemp);
    va_end (ap);
}

static const int NTSC_BASE_WIDTH  = 640;
static const int NTSC_BASE_HEIGHT = 480;
static const int PAL_BASE_WIDTH  = 768;
static const int PAL_BASE_HEIGHT = 576;
/* CIF: 352 * 288 */

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
};

static device_type_e
get_device_type(const char *deviceName)
{
	if (!strncmp(deviceName, "QuickCam", 8)) {
		dprintf("Device=gray_QuickCam_95\n");
		return (gray_QuickCam_95);
	}
	if (!strncmp(deviceName, "quickcam.dll", 12)) {
		dprintf("Device=gray_QuickCam_NT\n");
		return (gray_QuickCam_NT);
	}
	if (!strncmp(deviceName, "miroVIDEO DC20", 14)) {
		dprintf("Device=Miro_dc20_95\n");
		return (Miro_dc20_95);
	}
	if (!strncmp(deviceName, "dc20.dll", 8)) {
		dprintf("Device=Miro_dc20_NT\n");
		return (Miro_dc20_NT);
	}
	if (!strncmp(deviceName, " MCT VMPlus", 10)) {
		dprintf("Device=MCT\n");
		return (MCT);
	}
	if (!strncmp(deviceName, "Screen Machine II", 16)) {
		dprintf("Device=SMII\n");
		return (SMII);
	}
	if (!strncmp(deviceName, "AV Master", 9)) {
		dprintf("Device=AV_Master\n");
		return (AV_Master);
	}

	dprintf("Device=Generic: %s\n", deviceName);
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
	dprintf("IC_Converter: comp=%x (%s) bpp=%d\n", comp, get_comp_name(comp), bpp);

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
		dprintf("IC: Located %s with bpp %d\n", icinfo.szName, bihOut_.biBitCount);
	}

	if (bihOut_.biBitCount <= 8)
		hIC_ = 0;

	rgb_ = new u_char[bihOut_.biSizeImage];
	rgb_bpp_ = bihOut_.biBitCount;
	dprintf("IC_Converter: rgb_bpp_ = %d\n", rgb_bpp_);

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
		dprintf("ICDecompress failed!\n");

	converter_->convert(rgb_, inw, inh, frm, outw, outh, invert);
}

class IC_Converter_411 : public IC_Converter {
public:
	IC_Converter_411(DWORD comp, int bpp, int inw, int inh);
};

IC_Converter_411::IC_Converter_411(DWORD comp, int bpp, int inw, int inh)
	: IC_Converter(comp, bpp, inw, inh)
{
	converter_ = new RGB_Converter_411(rgb_bpp_, NULL, 0);
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

class YUYV_Converter_411 : public Converter {
public:
	virtual void convert(u_int8_t *in, int inw, int inh, u_int8_t *frm, int outw, int outh, int invert = 0);
};

void YUYV_Converter_411::convert(u_int8_t *in, int inw, int inh, u_int8_t *frm, int outw, int outh, int invert)
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
 protected:
	virtual void start();
	virtual void stop();
	virtual void fps(int);
	virtual void setsize() = 0;
	virtual int grab();
	void setport(const char *port);

	device_type_e devtype_;

	int dev_;
	int connected_;
	int capturing_;
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
 protected:
	virtual void start();
	virtual void setsize();
};

class Vfw422Grabber : public VfwGrabber 
{
 public:
	Vfw422Grabber(const int dev);
 protected:
	virtual void start();
	virtual void setsize();
};

class VfwDevice : public InputDevice {
 public:
	VfwDevice(const char* name, int index);
	virtual int command(int argc, const char*const* argv);
 protected:
	DWORD vfwdev_;
};

class VfwScanner {
 public:
	VfwScanner(const int n);
};
static VfwScanner find_vfw_devices(4);

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
			new VfwDevice(strdup(deviceName), index);
                }
	}
}

VfwDevice::VfwDevice(const char* name, int index) :
	InputDevice(name), vfwdev_(DWORD(-1))
{
	dprintf("VfwDevice: [%d] \"%s\"\n", index, name);

	if (index >= 0) {
		vfwdev_ = index;
		switch (get_device_type(name)) {
		case gray_QuickCam_95:
			attributes_ = "format { 422 } size { small cif } port { QuickCam } ";
			break;
		case Generic:
		default:
			attributes_ = "format { 422 } size { small cif } port { external-in } ";
			break;
		}
	} else
		attributes_ = "disabled";
}

int VfwDevice::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if ((argc == 3) && (strcmp(argv[1], "open") == 0)) {
		TclObject* o = 0;
		if (strcmp(argv[2], "422") == 0)
			o = new Vfw422Grabber(vfwdev_);
		else if (strcmp(argv[2], "cif") == 0)
			o = new VfwCIFGrabber(vfwdev_);
		if (o != 0)
			Tcl::instance().result(o->name());
		return (TCL_OK);
	}
	return (InputDevice::command(argc, argv));
}

VfwGrabber::VfwGrabber(const int dev) : dev_(dev), connected_(0),
	last_frame_(0), devtype_(Generic)
{
	char deviceName[80] ;
	char deviceVersion[100] ;
	
	if (!capGetDriverDescription(dev, (LPSTR)deviceName, sizeof(deviceName), (LPSTR)deviceVersion, sizeof(deviceVersion))) {
		fprintf(stderr, "VfwGrabber: Cannot get driver info!\n");
		abort();
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
	if (capwin_) {
		if (capturing_) {
			capCaptureStop(capwin_);
			capturing_ = 0;
		}
		if (connected_) {
			capDriverDisconnect(capwin_);
			connected_ = 0;
		}
		capSetCallbackOnVideoStream(capwin_, NULL);
		DestroyWindow(capwin_);
	}
}

VfwCIFGrabber::VfwCIFGrabber(const int dev) : VfwGrabber(dev)
{
	dprintf("VfwCIFGrabber\n");
}

Vfw422Grabber::Vfw422Grabber(const int dev) : VfwGrabber(dev)
{
	dprintf("Vfw422Grabber\n");
}

void Vfw422Grabber::setsize()
{
	int w = basewidth_ / decimate_;
	int h = baseheight_ / decimate_;
	dprintf("Vfw422Grabber::setsize: %dx%d\n", w, h);
	set_size_422(w, h);
	allocref();
}

void VfwCIFGrabber::setsize()
{
	int w = basewidth_ / decimate_;
	int h = baseheight_ / decimate_;
	dprintf("VfwCIFGrabber::setsize: %dx%d\n", w, h);
	set_size_cif(w, h);
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

extern "C" {
extern char **__argv;
}

void VfwGrabber::start()
{
	dprintf("VfwGrabber::start() thread=%x\n", GetCurrentThreadId());
	dprintf("basewidth_=%d, baseheight_=%d, decimate_=%d\n", basewidth_, baseheight_, decimate_);

	/*
	 * The quickcam driver seems to completely ignore the information
	 * given in the capSetVideoFormat call. To get around this we have
	 * to set the desired values in the quickcam.ini file before we
	 * connect the driver.
	 */
	if (devtype_ == gray_QuickCam_95) {
		/* Should really use binary name in calls below instad of Vic */
		dprintf("argv %s\n", *__argv);
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
			dprintf("Quickcam cannot do this decimation!\n");
			break;
		}
	}

	if ((capwin_ = capCreateCaptureWindow((LPSTR)"Capture Window", WS_POPUP | WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT, (basewidth_ / decimate_ + GetSystemMetrics(SM_CXFIXEDFRAME)), (baseheight_ / decimate_ + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME)), (HWND) 0, (int) 0)) == NULL) {
		fprintf(stderr, "capCreateCaptureWindow: failed - %lu\n", capwin_);
		abort();
	}
	capSetCallbackOnError(capwin_, ErrorHandler);
	if (!capSetCallbackOnVideoStream(capwin_, VideoHandler)) {
		fprintf(stderr, "capSetCallbackOnVideoStream: failed - %lu\n", GetLastError());
		/*abort();*/
	}
	if (!capDriverConnect(capwin_, dev_)) {
		fprintf(stderr, "capDriverConnect: dev=%d failed - %lu\n", dev_, GetLastError());
		/*abort();*/
	}
	capSetUserData(capwin_, this);
	dprintf("SetUserData=%x\n", this);
	connected_ = 1;

	if (!capDriverGetCaps(capwin_, &caps_, sizeof(caps_))) {
		fprintf(stderr, "capGetDriverCaps: failed - %lu\n", GetLastError());
		/*abort();*/
	}

	dprintf("capdrivercaps: overlay=%d dlgSource=%d dlgFmt=%d dlgDis=%d init=%d pal=%d\n",
		caps_.fHasOverlay, caps_.fHasDlgVideoSource,
		caps_.fHasDlgVideoFormat, caps_.fHasDlgVideoDisplay,
		caps_.fCaptureInitialized, caps_.fDriverSuppliesPalettes);
	
	fmtsize_ = capGetVideoFormatSize(capwin_);
	fmt_ = (LPBITMAPINFOHEADER)new u_char[fmtsize_];
	if (!capGetVideoFormat(capwin_, fmt_, fmtsize_)) {
		fprintf(stderr, "capGetVideoFormat: failed - %lu\n",
			GetLastError());
		/*abort();*/
	}
	int orig_comp = fmt_->biCompression;
	int orig_bpp = fmt_->biBitCount;
	dprintf("Original comp=%x (%s) bpp=%d\n", orig_comp, get_comp_name(orig_comp), orig_bpp);

	fmt_->biPlanes = 1;
	fmt_->biWidth = basewidth_ / decimate_;
	fmt_->biHeight = baseheight_ / decimate_;
	switch (devtype_) {
	case gray_QuickCam_NT:
	case Miro_dc20_95:
	case Miro_dc20_NT:
	case AV_Master:
		if (caps_.fHasDlgVideoFormat)
			if (caps_.fHasDlgVideoSource)
				capDlgVideoSource(capwin_);
			for (;;) {
				capDlgVideoFormat(capwin_);
				delete [] fmt_;
				fmtsize_ = capGetVideoFormatSize(capwin_);
				fmt_ = (LPBITMAPINFOHEADER) new u_char [fmtsize_];
				capGetVideoFormat(capwin_, fmt_, fmtsize_);
				if (fmt_->biWidth == 320 || fmt_->biWidth == 160 || fmt_->biWidth == 384 || fmt_->biWidth == 192)
					break;
				else
					fprintf(stderr, "Selected image size not supported! Use 320x240 or 160x120.\n");
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
		dprintf("MCT MY42 format failed!\n");
#endif
		goto Generic;
	case SMII:
		fmt_->biCompression = mmioFOURCC('Y','U','Y','V');
		fmt_->biBitCount = 16;
		fmt_->biSizeImage = fmt_->biWidth * fmt_->biHeight * fmt_->biPlanes * fmt_->biBitCount / 8;
		if (capSetVideoFormat(capwin_, fmt_, fmtsize_))
			break;
		dprintf("SMII YUYV format failed!\n");
		goto Generic;
	case Generic:
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
			dprintf("Unable to find supported RGB format!\n");
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
	dprintf("bitmapinfo: comp= %x (%s) w=%d h=%d planes=%d bitcnt=%d szImage=%d ClrUsed=%d ClrImp=%d\n",
		fmt_->biCompression, get_comp_name(fmt_->biCompression),
		fmt_->biWidth, fmt_->biHeight,
		fmt_->biPlanes, fmt_->biBitCount, fmt_->biSizeImage,
		fmt_->biClrUsed, fmt_->biClrImportant);

	assert(fmt_->biPlanes == 1);
	dprintf("Using biBitCount = %d\n", fmt_->biBitCount);
	if (fmt_->biWidth != basewidth_ / decimate_) {
		/* The driver is totally stupid so accept it's settings! */
		dprintf("Stupid driver. Accepting %x %d*%d*%d\n", fmt_->biCompression, fmt_->biWidth, fmt_->biHeight, fmt_->biBitCount);
		switch (fmt_->biWidth) {
			case 640:
			case 320:
			case 160:
				max_fps_ = 30;
				basewidth_ = NTSC_BASE_WIDTH;
				baseheight_ = NTSC_BASE_HEIGHT;
				break;
			case 768:
			case 384:
			case 192:
				max_fps_ = 25;
				basewidth_ = PAL_BASE_WIDTH;
				baseheight_ = PAL_BASE_HEIGHT;
				break;
			default:
				fprintf(stderr, "Cannot make any sence out of driver!\n");
				stop();
				abort();
		}
		decimate_ = basewidth_ / fmt_->biWidth;
		setsize();
	}
	
	if (!capCaptureGetSetup(capwin_, &parms_, sizeof(parms_))) {
		fprintf(stderr, "capCaptureGetSetup: failed - %lu\n", GetLastError());
		/*abort();*/
	}

	dprintf("GetSetup: uSec=%d drop=%d DOS=%d nVid=%d yield=%d\n",
		parms_.dwRequestMicroSecPerFrame,
		parms_.wPercentDropForError,
		parms_.fUsingDOSMemory,
		parms_.wNumVideoRequested,
		parms_.fYield);

	/*1e6 / double(max_fps_);*/
	/*(DWORD)frametime_*/
	parms_.dwRequestMicroSecPerFrame = 1e6 / double(50);
	parms_.wPercentDropForError = 90;
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
	} else
		capturing_ = 1;
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
}

void VfwCIFGrabber::start()
{
	VfwGrabber::start();
	switch (fmt_->biCompression) {
	case BI_RGB:
		converter(new RGB_Converter_411(fmt_->biBitCount, (u_int8_t *)(fmt_ + 1), fmt_->biClrUsed));
		break;
	case mmioFOURCC('Y','U','Y','V'):
		converter(new YUYV_Converter_411());
		break;
	default:
		converter(new IC_Converter_411(fmt_->biCompression, fmt_->biBitCount, fmt_->biWidth, fmt_->biHeight));
		break;
	}
}

void VfwGrabber::stop()
{
	dprintf("VfwWindow::stop() thread=%x\n", GetCurrentThreadId());

	if (capturing_)
		capCaptureStop(capwin_);
	capturing_ = 0;
	ReleaseSemaphore(frame_sem_, 1, NULL);
	CloseHandle(frame_sem_);
#ifdef NDEF
	if (caps_.fHasOverlay)
		capOverlay(capwin_, FALSE);
	else
		capPreview(capwin_, FALSE);
#endif

	capDriverDisconnect(capwin_);
	connected_ = 0;

	if (fmt_->biCompression == BI_RGB)
		delete converter_;
	converter_ = 0;

	delete [] fmt_;

	capSetCallbackOnVideoStream(capwin_, NULL);
	capSetCallbackOnError(capwin_, NULL);
	DestroyWindow(capwin_);
	capwin_ = NULL;
	last_frame_ = 0;

	Grabber::stop();
}

void VfwGrabber::setport(const char *port)
{
	dprintf("setport: %s thread=%x\n", port, GetCurrentThreadId());

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
			dprintf("VfwGrabber::command: decimate=%d (dec)=%d\n", dec, decimate_);
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

	dprintf("ErrorHandler: thread=%x [id=%d] %s\n",
		GetCurrentThreadId(), id, err);

	return ((LRESULT)TRUE);
}

LRESULT CALLBACK
VfwGrabber::VideoHandler(HWND hwnd, LPVIDEOHDR vh)
{
	static int set = 0;
	static int not_done = 0;

	if (set == 0) {
		//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
		//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
		//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
		set = 1;
	}

	VfwGrabber *gp = (VfwGrabber*)capGetUserData(hwnd);
#ifdef DEBUG	
	dprintf("VfwGrabber::VideoHandler: thread=%x data=%x flags=%x len=%d time=%d\n",
		GetCurrentThreadId(),
		vh->lpData, vh->dwFlags, vh->dwBytesUsed, vh->dwTimeCaptured);
#endif

	if (vh->dwFlags & VHDR_DONE)
		(gp->capture)(gp, vh->lpData);
	else if (not_done++ % 10 == 0)
		dprintf("Frames not ready! %d\n", not_done);
	
	return ((LRESULT)TRUE);
}

void
VfwGrabber::capture(VfwGrabber *gw, LPBYTE frame)
{
#ifdef DEBUG
	if (last_frame_ != NULL)
		dprintf("Last frame not grabbed!\n");
#endif
	if (capturing_) {
		gw->last_frame_ = frame;
		WaitForSingleObject(frame_sem_, INFINITE);
	}
}

int
VfwGrabber::grab()
{
#ifdef DEBUG
	dprintf("VfwGrabber::grab: thread=%x w=%d h=%d frame_=%d fsize_=%d in=%dx%d out=%dx%d\n",
		GetCurrentThreadId(),
		basewidth_, baseheight_, frame_, framesize_,
		inw_, inh_, outw_, outh_);
#endif

	if (last_frame_ == NULL || capturing_ == 0)
		return (FALSE);

	converter_->convert((u_int8_t*)last_frame_, basewidth_ / decimate_, baseheight_ / decimate_, frame_, outw_, outh_, TRUE);
	ReleaseSemaphore(frame_sem_, 1, NULL);
	last_frame_ = NULL;
	suppress(frame_);
	saveblks(frame_);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}
