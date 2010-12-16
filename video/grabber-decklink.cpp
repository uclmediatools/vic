/*
 * grabber-declink.cpp
 *
 * Copyright (c) 2010 The University of Queensland.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
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
 *
 * Written by Douglas Kosovic <d.kosovic@uq.edu.au>
 *
 * VIC Grabber for the Blackmagic Design DeckLink SDK
 * Supports Blackmagic Design's DeckLink, Multibridge and Intensity products.
 *
 */

#include <stdio.h>
#include <sys/types.h>

#include "grabber.h"
#include "module.h"
#include "device-input.h"

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <objbase.h>
#include "DeckLinkAPI_h.h"
#include "inttypes.h"
#else
#include <unistd.h>
#include "DeckLinkAPIDispatch.cpp"
#endif

#include "yuv_convert.h"

#ifdef HAVE_SWSCALE
extern "C" {
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"

#include "config_arch.h"
#include "cpu/cpudetect.h"

static int available_cpu_flags = cpu_check();
}
#endif

#if !defined(_WIN32) && !defined(_WIN64)
#undef debug_msg
#define debug_msg(args...) fprintf(stderr, args)
#endif

#define NUM_DEVS 5   // max number of DeckLink capture devices we'll support

// Color subsampling options.
#define CF_422 0
#define CF_420 1
#define CF_CIF 2

// CIF resolution
#define CIF_WIDTH          352
#define CIF_HEIGHT         288

// QCIF resolution
#define QCIF_WIDTH         176
#define QCIF_HEIGHT        144

class DeckLinkCaptureDelegate : public IDeckLinkInputCallback {
public:
    DeckLinkCaptureDelegate(int32_t width, int32_t height, int format) {
        mRefCount = 1;
        mReadIndex = 0;
        mWriteIndex = 0;
        mLastReadFrameNum = 0;
        mLastWriteFrameNum = 0;
#ifdef HAVE_SWSCALE
        sws_context = NULL;
#endif
        outw = width;
        outh = height;
        cformat = format;
        for (int i = 0; i < mBufferSize; i++) {
            if (cformat == CF_422) {
                    mBuffer[i] = new uint8_t[width * height * 2];
            } else {
                    mBuffer[i] = new uint8_t[width * height * 3 / 2];
            }
            mBufferFrameNum[i] = 0;
        }
    }

    ~DeckLinkCaptureDelegate() {
        for (int i = 0; i < mBufferSize; i++) {
            delete [] mBuffer[i];
        }
    }

    uint8_t *GetVideoFrame() {

        if(mReadIndex == mWriteIndex) {
            return NULL;
        }

        // ignore older frames
        if (mLastReadFrameNum >= mBufferFrameNum[mReadIndex] && mLastReadFrameNum <= 0xFFFFFFFF - mBufferSize) {
            return NULL;
        }
        mLastReadFrameNum = mBufferFrameNum[mReadIndex];

        uint8_t *retval = (uint8_t *)(mBuffer[mReadIndex]);
        int nextElement = (mReadIndex + 1) % mBufferSize;
        mReadIndex = nextElement;

        return retval;
    }

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) {
        HRESULT result = E_NOINTERFACE;

#if defined(_WIN32) || defined(_WIN64) 
        // Initialise the return result
        *ppv = NULL;

        // Obtain the IUnknown interface and compare it the provided REFIID
        if (iid == IID_IUnknown) {
            *ppv = this;
            AddRef();
            result = S_OK;
        } else if (iid == IID_IDeckLinkInputCallback) {
            *ppv = (IDeckLinkInputCallback*)this;
            AddRef();
            result = S_OK;
        }
#endif
        return result;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef() {
#if defined(_WIN32) || defined(_WIN64) 
       return InterlockedIncrement((LONG*)&mRefCount);
#else
       return __sync_fetch_and_add(&mRefCount, 1);
#endif
    }

    virtual ULONG STDMETHODCALLTYPE Release() {
        int32_t newRefValue;

#if defined(_WIN32) || defined(_WIN64) 
        newRefValue = InterlockedDecrement((LONG*)&mRefCount);
#else
        newRefValue = __sync_fetch_and_sub(&mRefCount, 1);
#endif
        if (newRefValue == 0) {
#ifdef HAVE_SWSCALE
            if (sws_context != NULL){
                sws_freeContext(sws_context);
            }
#endif
            delete this;
            return 0;
        }
        return newRefValue;
    }

    virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame* arrivedFrame, IDeckLinkAudioInputPacket*) {

        void *videoFrame;
        arrivedFrame->GetBytes(&videoFrame);

        int nextElementIndex = (mWriteIndex + 1) % mBufferSize;

        if(nextElementIndex != mReadIndex) {
#ifdef HAVE_SWSCALE
            // don't use SWSCALE to scale 1920x1080 to 1920x1072
            if (outh == 1072) {
                if (cformat == CF_422) {
                    packedUYVY422_to_planarYUYV422((char *)mBuffer[mWriteIndex], outw, outh,
                                                   (char *)videoFrame, arrivedFrame->GetWidth(), arrivedFrame->GetHeight());
                } else {
                    packedUYVY422_to_planarYUYV420((char *)mBuffer[mWriteIndex], outw, outh,
                                                   (char *)videoFrame, arrivedFrame->GetWidth(), arrivedFrame->GetHeight());
                }
                mBufferFrameNum[mWriteIndex] = ++mLastWriteFrameNum;
                mWriteIndex = nextElementIndex;

                return S_OK;
            }

            int flags = SWS_BILINEAR;

            if (sws_context == NULL){

#ifdef RUNTIME_CPUDETECT
                flags |= (available_cpu_flags & FF_CPU_MMX ? SWS_CPU_CAPS_MMX : 0);
                flags |= (available_cpu_flags & FF_CPU_MMXEXT ? SWS_CPU_CAPS_MMX2 : 0);
                flags |= (available_cpu_flags & FF_CPU_3DNOW ? SWS_CPU_CAPS_3DNOW : 0);
                flags |= (available_cpu_flags & FF_CPU_ALTIVEC ? SWS_CPU_CAPS_ALTIVEC : 0);
#elif defined(HAVE_MMX)
                flags |= SWS_CPU_CAPS_MMX;
#if defined(HAVE_MMX2)
                flags |= SWS_CPU_CAPS_MMX2;
#endif
#elif defined(HAVE_3DNOW)
                flags |= SWS_CPU_CAPS_3DNOW;
#endif
                PixelFormat in_format = PIX_FMT_UYVY422;

                PixelFormat out_format = PIX_FMT_YUV420P;
                if (cformat == CF_422) {
                    out_format = PIX_FMT_YUV422P;
                }

                // Accelerated Colour conversion routines
                sws_context = sws_getContext(arrivedFrame->GetWidth(), arrivedFrame->GetHeight(), in_format,
                                             outw, outh, out_format, flags, NULL, NULL, NULL);
                if(sws_context == NULL){
                    debug_msg("DeckLinkCaptureDelegate: error! cannot allocate memory for swscontext!\n");
                    return S_FALSE;
                } 
            }

            sws_src[0] = (uint8_t*)videoFrame;
            sws_src[1] = sws_src[2] = NULL;
            sws_src_stride[0] = arrivedFrame->GetRowBytes();
            sws_src_stride[1] = sws_src_stride[2] = 0;

            sws_tar[0] = (uint8_t*)mBuffer[mWriteIndex];
            sws_tar[1] = sws_tar[0] + outw * outh;
            if (cformat == CF_422) {
                sws_tar[2] = sws_tar[1] + outw * outh / 2;
            } else {
                sws_tar[2] = sws_tar[1] + outw * outh / 4;
            }
            sws_tar_stride[0] = outw;
            sws_tar_stride[1] = sws_tar_stride[2] = outw/2;

            sws_scale(sws_context, sws_src, sws_src_stride, 0, arrivedFrame->GetHeight(), sws_tar, sws_tar_stride);
#else
            if (cformat == CF_422) {
                packedUYVY422_to_planarYUYV422((char *)mBuffer[mWriteIndex], outw, outh,
                                               (char *)videoFrame, arrivedFrame->GetWidth(), arrivedFrame->GetHeight());
            } else {
                packedUYVY422_to_planarYUYV420((char *)mBuffer[mWriteIndex], outw, outh,
                                               (char *)videoFrame, arrivedFrame->GetWidth(), arrivedFrame->GetHeight());
            }
#endif
            mBufferFrameNum[mWriteIndex] = ++mLastWriteFrameNum;
            mWriteIndex = nextElementIndex;
        }

        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags) {
        return S_OK;
    }

private:
    volatile int32_t mRefCount;
    volatile int32_t mReadIndex;
    volatile int32_t mWriteIndex;
    static const int32_t mBufferSize = 4;
    volatile uint8_t *mBuffer[mBufferSize];
    volatile uint32_t mLastReadFrameNum;
    volatile int32_t mLastWriteFrameNum;
    volatile uint32_t mBufferFrameNum[mBufferSize];
#ifdef HAVE_SWSCALE
    SwsContext *sws_context;
    uint8_t *sws_src[3];
    uint8_t *sws_tar[3];
    int sws_src_stride[3];
    int sws_tar_stride[3];
#endif
    int cformat, outw, outh;
};


class DeckLinkGrabber : public Grabber {
public:
    DeckLinkGrabber(const char *cformat, IDeckLink* deckLink);
    virtual ~DeckLinkGrabber();

    virtual int command(int argc, const char*const* argv);
    void start();
    void stop();
    virtual int grab();

protected:
    IDeckLink* deckLink_;
    IDeckLinkInput *deckLinkInput_;

    int cformat_;
    int decimate_;
    int width_, height_;

    BMDDisplayMode displayMode_;
    long displayModeWidth_;
    long displayModeHeight_;

    DeckLinkCaptureDelegate *delegate_;
};

class DeckLinkDevice : public InputDevice {
public:
    DeckLinkDevice(const char* name, IDeckLink* deckLink);
    ~DeckLinkDevice();
    virtual int command(int argc, const char*const* argv);
protected:
    DeckLinkGrabber *grabber_;
    IDeckLink* deckLink_;
};

class DeckLinkScanner {
public:
    DeckLinkScanner();
    ~DeckLinkScanner();
protected:
    DeckLinkDevice *devs_[NUM_DEVS];

};

static DeckLinkScanner find_decklink_devices;

DeckLinkScanner::DeckLinkScanner()
{
    IDeckLink* deckLink;
    int n = 0;
    HRESULT result;
    char *nick_name[NUM_DEVS];

#if defined(_WIN32) || defined(_WIN64) 
    // Initialize COM on this thread
    result = CoInitializeEx(NULL,COINIT_MULTITHREADED);
    if (FAILED(result)) {
        debug_msg("DeckLinkScanner: Failed COM subsystem initialisation.\n");
        return;
    }

    IDeckLinkIterator* deckLinkIterator;
    result = CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_INPROC_SERVER, IID_IDeckLinkIterator, (void**)&deckLinkIterator);
    if (FAILED(result)) {
        debug_msg("DeckLinkScanner: DeckLink iterator instance could not be created\n");
        CoUninitialize();
        return;
    }
#else
    IDeckLinkIterator* deckLinkIterator;
    deckLinkIterator = CreateDeckLinkIteratorInstance();
    if (deckLinkIterator == NULL) {
        debug_msg("DeckLinkScanner: DeckLink iterator instance could not be created\n");
        return;
    }
#endif

    memset(devs_, 0, sizeof(devs_));
    // Enumerate all DeckLink cards on this system
    while (deckLinkIterator->Next(&deckLink) == S_OK && n < NUM_DEVS) {

#if defined(_WIN32) || defined(_WIN64) 
        char deviceNameString[64] = {};
        BSTR cardNameBSTR;

        result = deckLink->GetModelName(&cardNameBSTR);
        if (result == S_OK) {
            wcstombs(deviceNameString, cardNameBSTR, 63);
        }
#elif __APPLE__
        char deviceNameString[64] = {};
        CFStringRef modelName;

        result = deckLink->GetModelName(&modelName);

        if (result == S_OK) {
            CFStringGetCString(modelName, deviceNameString, sizeof(deviceNameString), kCFStringEncodingASCII);
        }
#else
        const char * deviceNameString = NULL;

        result = deckLink->GetModelName(&deviceNameString);
#endif

        if (result == S_OK) {
            debug_msg("Adding device %s\n", deviceNameString);

            int dev_count_suffix = 1;
            for (int i = 0; i < n; i++) {
                if (strncmp(deviceNameString, (char *)(nick_name[i] + 11), strlen(deviceNameString)) == 0) {
                    dev_count_suffix++;
                }
            }
            char *nick;
            if (dev_count_suffix == 1) {
                nick = new char[strlen(deviceNameString) + 12];
                sprintf(nick,"Blackmagic-%s", deviceNameString);
            } else {
                nick = new char[strlen(deviceNameString) + 16];
                sprintf(nick,"Blackmagic-%s #%i", deviceNameString, dev_count_suffix);
            }
            nick_name[n] = nick;

            devs_[n] = new DeckLinkDevice(nick, deckLink);
            n++;
        }
    }
}

DeckLinkScanner::~DeckLinkScanner() {
    debug_msg("~DeckLinkScanner\n");

    for (int i = 0; i < NUM_DEVS; i++) {
        if (devs_[i]) {
            debug_msg("Deleting DeckLink device %d\n", i);
            delete devs_[i];
        }
    }
#if defined(_WIN32) || defined(_WIN64) 
    CoUninitialize();
#endif
}

DeckLinkDevice::DeckLinkDevice(const char* name, IDeckLink* deckLink) : InputDevice(name), grabber_(0), deckLink_(deckLink)
{
    char *attr = new char[512];
    IDeckLinkAttributes *deckLinkAttributes = NULL;
    IDeckLinkInput *deckLinkInput = NULL;
    IDeckLinkDisplayModeIterator *displayModeIterator = NULL;
    IDeckLinkDisplayMode *displayMode = NULL;
    int64_t ports;

    HRESULT result;

    result = deckLink->QueryInterface(IID_IDeckLinkAttributes, (void**)&deckLinkAttributes);
    if (result != S_OK) {
        debug_msg("DeckLinkDevice: Could not obtain the IID_IDeckLinkAttributes interface - %08x\n", result);
        strcpy(attr,"disabled");
        return;
    }

    result = deckLinkAttributes->GetInt(BMDDeckLinkVideoInputConnections, &ports);
    if (result != S_OK) {
        debug_msg("DeckLinkDevice: Could not obtain the video input connection attributes\n");
        strcpy(attr,"disabled");
        return;
    }

    strcpy(attr,"format { 420 422 cif } ");
    strcat(attr,"size { small cif large } ");

    strcat(attr,"port { ");

    if (ports & bmdVideoConnectionSDI) {
        strcat(attr,"SDI ");
    }
    if (ports & bmdVideoConnectionHDMI) {
        strcat(attr,"HDMI ");
    }
    if (ports & bmdVideoConnectionComponent) {
        strcat(attr,"Component ");
    }
    if (ports & bmdVideoConnectionComposite) {
        strcat(attr,"Composite ");
    }
    if (ports & bmdVideoConnectionSVideo) {
        strcat(attr,"S-Video ");
    }
    if (ports & bmdVideoConnectionOpticalSDI) {
        strcat(attr,"Optical-SDI ");
    }
    strcat(attr,"} ");

    strcat(attr,"large_size_resolution { none 960p 720p 576p 480p } ");

    result = deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput);
    if (result != S_OK) {
        debug_msg("DeckLinkDevice: Could not obtain the IDeckLinkInput interface\n");
        strcpy(attr, "disabled");
        return;
    }

    result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
    if (result != S_OK) {
        debug_msg("DeckLinkDevice: Could not obtain the video input display mode iterator\n");
        strcpy(attr, "disabled");
        return;
    }

    strcat(attr,"type { ");
    while (displayModeIterator->Next(&displayMode) == S_OK) {
        char typeString[128];

#if defined(_WIN32) || defined(_WIN64) 
        char displayModeString[64] = {};
        BSTR displayModeNameBSTR;

        result = displayMode->GetName(&displayModeNameBSTR);
        if (result == S_OK) {
            wcstombs(displayModeString, displayModeNameBSTR, 63);
        }
#elif __APPLE__
        char displayModeString[64] = {};

        CFStringRef displayModeName;
        result = displayMode->GetName(&displayModeName);

        if (result == S_OK) {
            CFStringGetCString(displayModeName, displayModeString, sizeof(displayModeString), kCFStringEncodingASCII);
        }
#else
        const char *displayModeString = NULL;

        result = displayMode->GetName(&displayModeString);
#endif

        if (result == S_OK) {
            strncpy(typeString, displayModeString, sizeof(typeString));
            typeString[sizeof(typeString) - 1] = 0;
            for (unsigned int i=0 ; i < strlen(typeString) ; i++) {
                if (typeString[i]==' ') typeString[i]='-';
            }

            strcat(attr, typeString);
            strcat(attr, " ");
        }
        displayMode->Release();
    }
    strcat(attr,"} ");

    attributes_ = attr;
    debug_msg("DeckLinkDevice:  ==> %s\n",attr);


    if (displayModeIterator != NULL) {
        displayModeIterator->Release();
    }

    if (deckLinkInput != NULL) {
        deckLinkInput->Release();
    }

    if (deckLinkAttributes != NULL) {
        deckLinkAttributes->Release();
    }

}

int DeckLinkDevice::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();
    if (argc == 3) {
        if (strcmp(argv[1], "open") == 0) {
            TclObject* o = 0;
            o = new DeckLinkGrabber(argv[2], deckLink_);
            if (o != 0)
                tcl.result(o->name());
            return (TCL_OK);
        }
    } else if (argc == 2) {
        tcl.evalc("set_scaler_buttons_state");
    }
    return (InputDevice::command(argc, argv));
}

DeckLinkDevice::~DeckLinkDevice()
{
}

DeckLinkGrabber::DeckLinkGrabber(const char *cformat, IDeckLink* deckLink) :
    deckLink_(deckLink)
{
    HRESULT result;

    if (strcmp(cformat, "422") == 0) cformat_ = CF_422;
    else if (strcmp(cformat, "420") == 0) cformat_ = CF_420;
    else if (strcmp(cformat, "cif") == 0) cformat_ = CF_CIF;
    else cformat_ = CF_420;

    result = deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput_);
    if (result != S_OK) {
        debug_msg("DeckLinkDevice: Could not obtain the IDeckLinkInput interface\n");
    }
    running_  = 0;
    delegate_ = NULL;
}

int DeckLinkGrabber::command(int argc, const char*const* argv)
{
    HRESULT result;

    if (argc == 3) {
        if (strcmp(argv[1], "decimate") == 0) {
            decimate_ = atoi(argv[2]);

            if (running_) {
                stop(); start();
            }
            return (TCL_OK);
        } else if (strcmp(argv[1], "port") == 0) {
            IDeckLinkConfiguration *deckLinkConfiguration = NULL;
            BMDVideoConnection bmdVideoConnection = bmdVideoConnectionHDMI;

            result = deckLink_->QueryInterface(IID_IDeckLinkConfiguration, (void**)&deckLinkConfiguration);

            if (result != S_OK) {
                debug_msg("DeckLinkGrabber: Could not obtain the IDeckLinkConfiguration interface\n");
                return TCL_ERROR;
            }

            if (strcasecmp(argv[2], "SDI") == 0) {
                bmdVideoConnection = bmdVideoConnectionSDI;

            } else if(strcasecmp(argv[2], "HDMI") == 0) {
                bmdVideoConnection = bmdVideoConnectionHDMI;

            } else if(strcasecmp(argv[2], "Component") == 0) {
                bmdVideoConnection = bmdVideoConnectionComponent;

            } else if(strcasecmp(argv[2], "Composite") == 0) {
                bmdVideoConnection = bmdVideoConnectionComposite;

            } else if(strcasecmp(argv[2], "S-Video") == 0) {
                bmdVideoConnection = bmdVideoConnectionSVideo;

            } else if(strcasecmp(argv[2], "Optical-SDI") == 0) {
                bmdVideoConnection = bmdVideoConnectionOpticalSDI;
            }

            result = deckLinkConfiguration->SetInt(bmdDeckLinkConfigVideoInputConnection, bmdVideoConnection);

            if (result != S_OK) {
                debug_msg("DeckLinkGrabber: Could not set video input connection\n");
                return TCL_ERROR;
            }

            if (deckLinkConfiguration != NULL) {
                deckLinkConfiguration->Release();
            }

            if (running_) {
                stop(); start();
            }
        
            return (TCL_OK);

        } else if (strcmp(argv[1], "fps") == 0) {
            debug_msg("DeckLinkGrabber: fps %s\n",argv[2]);

        } else if (strcmp(argv[1], "type") == 0 || strcmp(argv[1], "format") == 0) {

            IDeckLinkDisplayModeIterator *displayModeIterator = NULL;
            IDeckLinkDisplayMode *displayMode = NULL;

            result = deckLinkInput_->GetDisplayModeIterator(&displayModeIterator);
            if (result != S_OK) {
                debug_msg("DeckLinkDevice: Could not obtain the video input display mode iterator\n");
                return TCL_ERROR;
            }

            while (displayModeIterator->Next(&displayMode) == S_OK) {
                char typeString[128];

#if defined(_WIN32) || defined(_WIN64) 
                char displayModeString[64] = {};
                BSTR displayModeNameBSTR;

                result = displayMode->GetName(&displayModeNameBSTR);
                if (result == S_OK) {
                    wcstombs(displayModeString, displayModeNameBSTR, 63);
                }
#elif __APPLE__
                char displayModeString[64] = {};

                CFStringRef displayModeName;
                result = displayMode->GetName(&displayModeName);

                if (result == S_OK) {
                    CFStringGetCString(displayModeName, displayModeString, sizeof(displayModeString), kCFStringEncodingASCII);
                }
#else
                const char *displayModeString = NULL;

                result = displayMode->GetName(&displayModeString);
#endif

                if (result == S_OK) {
                    strncpy(typeString, displayModeString, sizeof(typeString));
                    typeString[sizeof(typeString) - 1] = 0;
                    for (unsigned int i=0 ; i < strlen(typeString) ; i++) {
                        if (typeString[i]==' ') typeString[i] = '-';
                    }
                    if (strcasecmp(argv[2], typeString) == 0) {
                        displayMode_ = displayMode->GetDisplayMode();
                        displayModeWidth_ = displayMode->GetWidth();
                        displayModeHeight_ = displayMode->GetHeight();
                        break;
                    }
                }
            }

            if (displayMode != NULL) {
                displayMode->Release();
            }

            if (displayModeIterator != NULL) {
                displayModeIterator->Release();
            }

            if (running_) {
                stop(); start();
            }

            return TCL_OK;
        }

    } else if (argc == 2) {
        if (strcmp(argv[1], "format") == 0 ||
            strcmp(argv[1], "type") == 0) {
            return TCL_OK;
        }
    }

    return (Grabber::command(argc, argv));

}

DeckLinkGrabber::~DeckLinkGrabber()
{
    if (deckLinkInput_ != NULL) {
        deckLinkInput_->Release();
    }
    if (delegate_ != NULL) {
        delegate_->Release();
    }
}

void DeckLinkGrabber::start()
{
    HRESULT result;
    int flags = TCL_GLOBAL_ONLY;
    Tcl& tcl = Tcl::instance();
    const char* largeSizeResolution = Tcl_GetVar(tcl.interp(), "largeSizeResolution", flags);

    // Set the image size.
    switch (decimate_) {
    case 1: // large-size
        if (strcmp(largeSizeResolution, "960p") == 0) {
            width_ = int(960 * displayModeWidth_ / displayModeHeight_);
            height_ = 960;
        } else if (strcmp(largeSizeResolution, "720p") == 0) {
            width_ = int(720 * displayModeWidth_ / displayModeHeight_);
            height_ = 720;
        } else if (strcmp(largeSizeResolution, "576p") == 0) {
          width_ = int(576 * displayModeWidth_ / displayModeHeight_);
          height_ = 576;
        } else if (strcmp(largeSizeResolution, "480p") == 0) {
          width_ = int(480 * displayModeWidth_ / displayModeHeight_);
          height_ = 480;
        } else {
          width_ = displayModeWidth_;
          height_ = displayModeHeight_;
        }
        break;
    case 2: // CIF-size
        width_ = int(CIF_HEIGHT * displayModeWidth_ / displayModeHeight_);
        height_ = CIF_HEIGHT;
        break;
    case 4: // QCIF-size
        width_ = int(QCIF_HEIGHT * displayModeWidth_ / displayModeHeight_);
        height_ = QCIF_HEIGHT;
        break;
    }

    switch (cformat_) {
    case CF_422:
        set_size_422(width_, height_);
        break;
    case CF_420:
        set_size_420(width_, height_);
        break;
    case CF_CIF:
        set_size_cif(width_, height_);
        break;
    }

    result = deckLinkInput_->EnableVideoInput(displayMode_, bmdFormat8BitYUV, 0);
    if (result != S_OK) {
        debug_msg("DeckLinkGrabber: Could not enable video input\n");
        return;
    }

    if (delegate_) {
        delegate_->Release();
    }
    if (decimate_ >= 2) {
        delegate_ = new DeckLinkCaptureDelegate(width_, height_, cformat_);
    } else {
        delegate_ = new DeckLinkCaptureDelegate(outw_, outh_, cformat_);
    }

    result = deckLinkInput_->SetCallback(delegate_);

    if (result != S_OK) {
        debug_msg("DeckLinkGrabber::start();LinkGrabber: Could not set callback\n");
        return;
    }

    result = deckLinkInput_->StartStreams();

    if (result != S_OK) {
        debug_msg("DeckLinkGrabber::start();LinkGrabber: Could not start streams\n");
        return;
    }

    // Allocate the reference buffer.
    allocref();

    fprintf(stderr, "Grabber::start()\n");
    Grabber::start();
    running_ = 1;
#if defined(_WIN32) || defined(_WIN64)
    Grabber::timeout();
#endif
}

void DeckLinkGrabber::stop()
{
    deckLinkInput_->StopStreams();

    Grabber::stop();
    running_ = 0;
}

int DeckLinkGrabber::grab()
{
    if (!delegate_) {
        return 0;
    }

    if (!running_) {
        return 0;
    }

    uint8_t *fr = delegate_->GetVideoFrame();
    if (fr == NULL) {
        return 0;
    }


    switch (decimate_) {
    case 1: // large-size
        if (cformat_ == CF_422) {
            memcpy((char *)frame_, (char *)fr, outw_ * outh_ * 2);
        } else {
            memcpy((char *)frame_, (char *)fr, outw_ * outh_ * 3 / 2);
        }
        break;

    case 2: // CIF-size
    case 4: // QCIF-size
        if (cformat_ == CF_422) {
            planarYUYV422_to_planarYUYV422((char *)frame_, outw_, outh_, (char *)fr, width_, height_);
        } else {
            planarYUYV420_to_planarYUYV420((char *)frame_, outw_, outh_, (char *)fr, width_, height_);
        }
        break;
    }

    suppress(frame_);
    saveblks(frame_);
    YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
    return (target_->consume(&f));

}
