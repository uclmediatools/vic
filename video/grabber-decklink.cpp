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
#include <atlbase.h>
#include "DeckLinkAPI_h.h"
#include "inttypes.h"
#else
#include <unistd.h>
#include "DeckLinkAPIDispatch.cpp"
#endif

#include "yuv_convert.h"

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
    DeckLinkCaptureDelegate(int32_t width, int32_t height) {
        mRefCount = 1;
        mReadIndex = 0;
        mWriteIndex = 0;
        for (int i = 0; i < mBufferSize; i++) {
            mBuffer[i] = new uint8_t[width * height * 2];
            memset((void *)mBuffer[i], width * height, sizeof(uint8_t));
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

        uint8_t *retval = (uint8_t *)(mBuffer[mReadIndex]);
        int nextElement = (mReadIndex + 1) % mBufferSize;
        mReadIndex = nextElement;

// fprintf(stderr, "*pop * mBuffer[%i] = 0x%lx\n", mReadIndex, mBuffer[mReadIndex]);

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
            memcpy((void *)(mBuffer[mWriteIndex]), videoFrame, arrivedFrame->GetRowBytes() * arrivedFrame->GetHeight());

// fprintf(stderr, "*push* mBuffer[%i] = 0x%lx\n", mWriteIndex, mBuffer[mWriteIndex]);
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
    BMDTimeValue displayModeFrameDuration_;
    BMDTimeScale displayModeTimeScale_;

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
    DeckLinkScanner(int maxNumDevices);
    ~DeckLinkScanner();
protected:
    DeckLinkDevice *devs_[NUM_DEVS];

};

static DeckLinkScanner find_decklink_devices(NUM_DEVS);

DeckLinkScanner::DeckLinkScanner(int maxNumDevices)
{
    IDeckLink* deckLink;
    int n = 0;
    HRESULT result;

#if defined(_WIN32) || defined(_WIN64) 
	// Initialize COM on this thread
	result = CoInitializeEx(NULL,COINIT_MULTITHREADED);
	if (FAILED(result)) {
		debug_msg("DeckLinkScanner: Failed COM subsystem initialisation.\n");
		return;
	}

    CComPtr<IDeckLinkIterator> deckLinkIterator = NULL;
	if (CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&deckLinkIterator) != S_OK || deckLinkIterator == NULL) {
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
    while (deckLinkIterator->Next(&deckLink) == S_OK && n < maxNumDevices) {

#if defined(_WIN32) || defined(_WIN64) 
        char deviceNameString[64] = {};
        CComBSTR cardNameBSTR;

        result = deckLink->GetModelName(&cardNameBSTR);
	    if (result == S_OK) {
            CW2A tmpstr1(cardNameBSTR);
            strncpy_s(deviceNameString, sizeof(deviceNameString), tmpstr1, _TRUNCATE);
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

            char *nick = new char[strlen(deviceNameString) + 10];
            sprintf(nick,"DeckLink-%s", deviceNameString);

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
    IDeckLinkConfiguration *deckLinkConfiguration = NULL;
    IDeckLinkConfiguration *deckLinkValidator = NULL;
    IDeckLinkInput *deckLinkInput = NULL;
    IDeckLinkDisplayModeIterator *displayModeIterator = NULL;
    IDeckLinkDisplayMode *displayMode = NULL;

    HRESULT result;

    result = deckLink->QueryInterface(IID_IDeckLinkConfiguration, (void**)&deckLinkConfiguration);
    if (result != S_OK) {
        debug_msg("DecLinkDevice: Could not obtain the IDeckLinkConfiguration interface - %08x\n", result);
        strcpy(attr,"disabled");
        return;
    }

    result = deckLinkConfiguration->GetConfigurationValidator(&deckLinkValidator);
    if (result != S_OK) {
        debug_msg("DecLinkDevice: Could not obtain the configuration validator interface\n");
        strcpy(attr,"disabled");
        return;
    }

    strcpy(attr,"format { 420 422 cif } ");
    strcat(attr,"size { small large cif } ");

    strcat(attr,"port { ");

    if (deckLinkValidator->SetVideoInputFormat(bmdVideoConnectionSDI) == S_OK) {
        strcat(attr,"SDI ");
    }
    if (deckLinkValidator->SetVideoInputFormat(bmdVideoConnectionHDMI) == S_OK) {
        strcat(attr,"HDMI ");
    }
    if (deckLinkValidator->SetVideoInputFormat(bmdVideoConnectionComponent) == S_OK) {
        strcat(attr,"Component ");
    }
    if (deckLinkValidator->SetVideoInputFormat(bmdVideoConnectionComposite) == S_OK) {
        strcat(attr,"Composite ");
    }
    if (deckLinkValidator->SetVideoInputFormat(bmdVideoConnectionSVideo) == S_OK) {
        strcat(attr,"S-Video ");
    }
    if (deckLinkValidator->SetVideoInputFormat(bmdVideoConnectionOpticalSDI) == S_OK) {
        strcat(attr,"Optical-SDI ");
    }
    strcat(attr,"} ");

    result = deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput);
    if (result != S_OK) {
        debug_msg("DecLinkDevice: Could not obtain the IDeckLinkInput interface\n");
        strcpy(attr, "disabled");
        return;
    }

    result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
    if (result != S_OK) {
        debug_msg("DecLinkDevice: Could not obtain the video input display mode iterator\n");
        strcpy(attr, "disabled");
        return;
    }

    strcat(attr,"type { ");
    while (displayModeIterator->Next(&displayMode) == S_OK) {
        char typeString[128];

#if defined(_WIN32) || defined(_WIN64) 
        char displayModeString[64] = {};
        CComBSTR displayModeNameBSTR;

        result = displayMode->GetName(&displayModeNameBSTR);
	    if (result == S_OK) {
            CW2A tmpstr1(displayModeNameBSTR);
            strncpy_s(displayModeString, sizeof(displayModeString), tmpstr1, _TRUNCATE);
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

    if (deckLinkValidator != NULL) {
        deckLinkValidator->Release();
    }

    if (deckLinkConfiguration != NULL) {
        deckLinkConfiguration->Release();
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
        debug_msg("DecLinkDevice: Could not obtain the IDeckLinkInput interface\n");
    }
    running_  = 0;
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
        }

        if (strcmp(argv[1], "port") == 0) {
            IDeckLinkConfiguration *deckLinkConfiguration = NULL;
            BMDVideoConnection bmdVideoConnection = bmdVideoConnectionHDMI;

            result = deckLink_->QueryInterface(IID_IDeckLinkConfiguration, (void**)&deckLinkConfiguration);

            if (result != S_OK) {
                debug_msg("DecLinkGrabber: Could not obtain the IDeckLinkConfiguration interface\n");
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

            result = deckLinkConfiguration->SetVideoInputFormat(bmdVideoConnection);

            if (result != S_OK) {
                debug_msg("DecLinkGrabber: Could not set input video connection\n");
                return TCL_ERROR;
            }

            if (deckLinkConfiguration != NULL) {
                deckLinkConfiguration->Release();
            }

            if (running_) {
                stop(); start();
            }
        
            return (TCL_OK);
        }

        if (strcmp(argv[1], "fps") == 0) {
            debug_msg("DecLinkGrabber: fps %s\n",argv[2]);
        }

        if (strcmp(argv[1], "type") == 0 || strcmp(argv[1], "format") == 0) {

            IDeckLinkDisplayModeIterator *displayModeIterator = NULL;
            IDeckLinkDisplayMode *displayMode = NULL;

            result = deckLinkInput_->GetDisplayModeIterator(&displayModeIterator);
            if (result != S_OK) {
                debug_msg("DecLinkDevice: Could not obtain the video input display mode iterator\n");
                return TCL_ERROR;
            }

            while (displayModeIterator->Next(&displayMode) == S_OK) {
                char typeString[128];

#if defined(_WIN32) || defined(_WIN64) 
                char displayModeString[64] = {};
                CComBSTR displayModeNameBSTR;

                result = displayMode->GetName(&displayModeNameBSTR);
	            if (result == S_OK) {
                    CW2A tmpstr1(displayModeNameBSTR);
                    strncpy_s(displayModeString, sizeof(displayModeString), tmpstr1, _TRUNCATE);
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
                        displayMode->GetFrameRate(&displayModeFrameDuration_, &displayModeTimeScale_);
fprintf(stderr, "DisplayMode width=%li height=%li frame duration=%li time scale=%li\n", displayModeWidth_, displayModeHeight_,displayModeFrameDuration_,  displayModeTimeScale_);
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
}

void DeckLinkGrabber::start()
{
    HRESULT result;

    // Set the image size.
    switch (decimate_) {
    case 1: // full-sized
        width_ = displayModeWidth_;
        height_ = displayModeHeight_;
        break;
    case 2: // CIF-sized
        width_ = CIF_WIDTH;
        height_ = CIF_HEIGHT;
        break;
    case 4: // QCIF-sized
        width_ = QCIF_WIDTH;
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
        debug_msg("DecLinkGrabber: Could not enable video input\n");
        return;
    }

    delegate_ = new DeckLinkCaptureDelegate(width_, height_);

    result = deckLinkInput_->SetCallback(delegate_);

    if (result != S_OK) {
        debug_msg("DecStartStreams();LinkGrabber: Could not set callback\n");
        return;
    }

    result = deckLinkInput_->StartStreams();

    if (result != S_OK) {
        debug_msg("DecStartStreams();LinkGrabber: Could not start streams\n");
        return;
    }

    // Allocate the reference buffer.
    allocref();

    fprintf(stderr, "Grabber::start()\n");
    Grabber::start();
    running_ = 1;
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

    uint8_t *fr = delegate_->GetVideoFrame();
    if (fr == NULL) {
        return 0;
    }

    // Need to fix, should do HDYC (a packed UYVY variation) to YUYV instead
    switch (cformat_) {
    case CF_420:
    case CF_CIF:
        packedUYVY422_to_planarYUYV420((char *)frame_, outw_, outh_, (char *)fr, inw_, inh_);
      break;

    case CF_422:
        packedUYVY422_to_planarYUYV422((char *)frame_, outw_, outh_, (char *)fr, inw_, inh_);
      break;
    }

    suppress(frame_);
    saveblks(frame_);
    YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
    return (target_->consume(&f));

}
