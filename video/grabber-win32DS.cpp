/*
 *  Copyright (c) 1996 John Brezak
 *  Copyright (c) 1996 Isidor Kouvelas (University College London)
 *  Portions Copyright (c) 2004 EarthLink, Inc.
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

/* grabber-win32DS.cpp

On the implementation of DirectShow frame grabbing

        Mark S. Petrovic
        EarthLink, Inc.
        Pasadena, CA  USA
        petrovic@corp.earthlink.net

I.

Timeline:  started 9/27/2004; finished 11/05/2004

II.

DirectShow video capture interface
Developed under DirectX 9, but simple enough that it
should work for DirectX 8 runtime systems. - msp 10/2004

Key references:

1.  Programming Microsoft DirectShow for Digital Video and Television, by Mark Pesce

http://www.amazon.com/exec/obidos/ASIN/0735618216/qid=1098459570/sr=2-1/ref=pd_ka_b_2_1/103-2872643-3555802

This book was very helpful in understanding DirectShow and the requisite basic COM programming.

2.  MSDN DirectShow reference

http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directshow/htm/directshowreference.asp

III.

Morning-after comments, 11/05/2004

1.  This code is good at dealing with simple USB-like (and maybe even
Firewire-like) devices.  Such devices have no crossbar filters inserted behind the scenes
by the graph builder, and which must be manipulated before grabbing begins.

2.  This code has a serious hack in dealing with devices that have "crossbar
filters".  Crossbar filters come into play with video capture cards, but, from what
I can tell, not with simple devices like USB cameras.  None of the USB cameras ship with WDM
drivers that have crossbar filters.  The Hauppauge WinTV Go Model 190 does have a crossbar
filter in its WDM driver.

The hack is this:  if a graph has a crossbar, I configure it to capture from the Composite In
port without qualification.  The user has no say in this.  Therefore, capturing from physical
s-video or antenna ports is not possible in this code as written.

Crossbar filters are added automatically by the graph builder during pBuild_->RenderStream().
Their purpose is to give
the programmer a way to programmatically choose which physical input port is "wired" to
the crossbar video decoder output port.  The crossbar video decoder output port pin is in turn
connected by the graph builder to the "true" device video capture filter that one would normally consider
to reside at the start of the capture graph.  In other words, the programmer does not, and cannot,
insert crossbars manually.

The crossbar reference:

http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directshow/htm/workingwithcrossbars.asp

3.  The issue is this:

vic, by a reading of the tcl source code in ui-ctrlmenu.tcl, considers
the objects "inputDevices" to be separate and distinct from the objects "grabbers".  A case
can be made that vic thinks devices "have" grabbers, rather than device "is" grabber.
DirectShow formally has neither concept; everything is a filter, or chains of filters.

When tcl "inputDevices" are created in the current programming model in DirectShowScanner,
the filter graph and the associated filter graph builder
do not yet exist.  The filter graph is required, however, to locate the crossbar, and the crossbar
in turn is inspected for physical input port names, such as Composite-in and s-video.  Because
the graph builder does not exist when inputDevices is created, the UI cannot be populated
with physical port names from which the user can choose to capture.

The graph builder comes into being when a DirectShowGrabber object is created when the user clicks
Transmit, and only when the grabber exists does sufficient information exist for the
crossbar to be located, its physical port names inspected, and logical wiring configured.

Two suggestions on how this model might be modified to give the user back the optoin
of choosing physical ports on capture cards:

a) create a dummy grabber of some sort during DirectShowScanner, and inspect the resulting
crossbar then for port names.  "delete dummyGrabber" at that point, as its purpose
has been fulfilled.

b) totally rework the code, and possibly vic's object model, so that grabbers exist when
input devices are created.  Probably not a good approach, but it is one possibility.

*/
#define _WIN32_DCOM
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "config.h"
#include <windows.h>
#include <set>
using namespace std;

#include "debug.h"
#include "grabber.h"
#include "device-input.h"
#include "module.h"
#include "rgb-converter.h" 
#include "yuv_convert.h"

#include "grabber-win32DS.h"

#ifndef VIDE0_FOR_WINDOWS
static DirectShowScanner findDirectShowDevices;
#endif

static const GUID MEDIASUBTYPE_I420 =
{0x30323449, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

static const GUID MEDIASUBTYPE_HDYC =
{0x43594448, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

static const GUID IUNKNOWN_IID = 
{0x00000000, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

static const GUID ISAMPLEGRABBERCB_IID =
{0x0579154A, 0x2B53, 0x4994, {0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85}};

#define NAMEBUF_LEN 200

IBaseFilter  *pCaptureFilter[NUM_DEVS];

//#########################################################################
// Class Callback, for video framebuffer callback.  See DS Sample Grabber interface docs.

STDMETHODIMP Callback::SampleCB(double sampleTime, IMediaSample *pSample) {
    return E_NOTIMPL;
}

STDMETHODIMP Callback::BufferCB(double sampleTime, BYTE *pBuffer, long bufferLen) {

    ///debug_msg("Callback::BufferCB:  sampleTime= %f, pbuffer= %p, bufferLen= %ld\n", sampleTime, pBuffer, bufferLen);

    WaitForSingleObject(grabber->cb_mutex_, INFINITE);
    grabber->capture(pBuffer, bufferLen);
    ReleaseMutex(grabber->cb_mutex_);
    return S_OK;
}

//-----------------

STDMETHODIMP Callback::QueryInterface(REFIID riid, void **ppvObject) {
    if (NULL == ppvObject)
        return E_POINTER;
    if (riid == IUNKNOWN_IID)
        *ppvObject = static_cast<IUnknown*>(this);
    else if (riid == ISAMPLEGRABBERCB_IID)
        *ppvObject = static_cast<ISampleGrabberCB*>(this);
    else
        return E_NOTIMPL;
    AddRef();
    return S_OK;
}

//#########################################################################
// DirectShowGrabber definition

DirectShowGrabber::DirectShowGrabber(IBaseFilter *filt, const char * cformat, const char *nick)  {
   HRESULT         hr;
   WCHAR           nameBufW[NAMEBUF_LEN];

   /* Reference:  various, including
      - Pesce, Chapter 11
      - http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directshow/htm/samplegrabberfilter.asp
   */

   debug_msg("new DirectShowGrabber()\n");
   decimate_ = 2;  //default
   converter_=0;
   cb_mutex_=0;
   crossbar_ = crossbarCursor_ = NULL;
   pDVVideoDecoder_=0;
   pDVDecoderBaseFilter_=0;
   pNullBaseFilter_=0;
   pSampleGrabber_=0;
   pGrabberBaseFilter_=0;
   pMediaControl_=0;
   pGraph_=0;
   pBuild_=0;
   dwRegister_=0;
   pCaptureFilter_ = filt;
   pXBar_   = NULL;
   pFilter_ = NULL;
   capturing_=0;
   max_fps_ = 30;
   memset(inputPorts, 0, NUM_PORTS);
   memset(largeSizeResolutions, 0, NUM_LARGE_SIZE_RESOLUTIONS * sizeof(SIZE));
       
   numInputPorts = 0;
   initializedPorts = 0;
   svideoPortNum = compositePortNum = 1;

   if(!strcmp(cformat, "420"))
       cformat_ = CF_420;
   else if(!strcmp(cformat, "422"))
       cformat_ = CF_422;
   else if(!strcmp(cformat, "cif"))
       cformat_ = CF_CIF;

   have_I420_ = false;
   have_UYVY_ = false;
   have_YUY2_ = false;
   have_RGB24_= false;
   have_HDYC_= false;
   have_DVSD_ = false;

   setport("external-in");

   basewidth_  = NTSC_BASE_WIDTH;
   baseheight_ = NTSC_BASE_HEIGHT;
   /*if( is_pal() ) {
      basewidth_  = PAL_BASE_WIDTH;
      baseheight_ = PAL_BASE_HEIGHT;
   } else {
      basewidth_  = NTSC_BASE_WIDTH;
      baseheight_ = NTSC_BASE_HEIGHT;
   }
   mt_.majortype = MEDIATYPE_AnalogVideo;
   mt_.subtype   = MEDIASUBTYPE_AnalogVideo_NTSC_M;
   */

   // callback mutex
   cb_mutex_ = CreateMutex(NULL, FALSE, NULL);

   callback_           = new Callback();
   callback_->grabber  = this;
   debug_msg("DirectShowGrabber::DirectShowGrabber():  callback created, grabber set\n");

   // Make a graph builder object we can use for capture graph building
   // Create the capture graph builder helper object
   hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
                         IID_ICaptureGraphBuilder2, (void **)&pBuild_);
   //showErrorMessage(hr);
   if (FAILED(hr)) {
       Grabber::status_=-1;
       return;
   }
   debug_msg("DirectShowGrabber::DirectShowGrabber():  graph builder interface acquired\n");

   // Create the Filter Graph Manager
   hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                         IID_IGraphBuilder, (void **)&pGraph_);
   if (FAILED(hr)) {
       Grabber::status_=-1;
       return;
   }
   debug_msg("DirectShowGrabber::DirectShowGrabber():  graph instance acquired\n");

    // Tell the capture graph builder about the Filter Graph Manager (FGM).
   hr = pBuild_->SetFiltergraph(pGraph_);
   //showErrorMessage(hr);
   if (FAILED(hr)) {
       Grabber::status_=-1;
       return;
   }
   debug_msg("DirectShowGrabber::DirectShowGrabber():  graph associated with builder\n");

   nameBufW[0] = '\0';
   hr = MultiByteToWideChar(CP_ACP, 0, nick, -1, nameBufW, NAMEBUF_LEN);

   // Add the capture filter (obtained by the DirectShowDevice Scanner) to the filter graph
   //hr = pGraph_->AddFilter(pCaptureFilter_, L"VicCaptureFilter");
   hr = pGraph_->AddFilter(pCaptureFilter_, nameBufW);

   debug_msg("DirectShowGrabber::DirectShowGrabber():  capture filter added to graph: %d\n", hr);
   //IAMVideoCompression *pVC;
   /*pCaptureFilter_->AddRef();
   hr = pBuild_->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
                              pCaptureFilter_, NULL, NULL);

   hr = pBuild_->FindInterface(&PIN_CATEGORY_CAPTURE,
                               &MEDIATYPE_Video, pCaptureFilter_,
                               IID_IAMVideoCompression, (void **)&pVC);*/

   // Set up the Sample Grabber transform filter
   hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
                         IID_IBaseFilter, (LPVOID *)&pGrabberBaseFilter_);
   //showErrorMessage(hr);
   debug_msg("DirectShowGrabber::DirectShowGrabber():  grabber base filter instance acquired: %d\n", hr);

   hr = pGrabberBaseFilter_->QueryInterface(IID_ISampleGrabber, (void**)&pSampleGrabber_);
   //showErrorMessage(hr);
   debug_msg("DirectShowGrabber::DirectShowGrabber():  Sample Grabber interface acquired\n");

   //hr = pSampleGrabber_->SetMediaType(&mt_);                          showErrorMessage(hr);
   hr = pSampleGrabber_->SetOneShot(FALSE);                           //showErrorMessage(hr);
   hr = pSampleGrabber_->SetCallback(callback_, 1);                    //showErrorMessage(hr);
   hr = pGraph_->AddFilter(pGrabberBaseFilter_,L"VicSampleGrabber");  //showErrorMessage(hr);

   // Get the Null Renderer DS default filter
   hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
                         IID_IBaseFilter, (LPVOID *)&pNullBaseFilter_);
   //showErrorMessage(hr);
   if (FAILED(hr)) {
      Grabber::status_=-1;
      return;
   }
   debug_msg("DirectShowGrabber::DirectShowGrabber():  Null Renderer interface acquired\n");

   // Finally, add the Null Renderer "sink" to the graph
   hr = pGraph_->AddFilter(pNullBaseFilter_,L"VicNullRenderer");
    if (FAILED(hr)) {
       Grabber::status_=-1;
       return;
   }
   debug_msg("DirectShowGrabber::DirectShowGrabber():  Null Renderer added to graph\n");

   if (!getCaptureCapabilities()) {
      Grabber::status_=-1;
      return;
   }
   //Not needed as width & height aren't known yet.
   setCaptureOutputFormat();

   if (findCrossbar(pCaptureFilter_)) {
         routeCrossbar();
   }

   ZeroMemory(&mt_, sizeof(AM_MEDIA_TYPE));
   mt_.majortype = MEDIATYPE_Video;

   if (cformat_ == CF_422) {
	   if (have_HDYC_) {
		   mt_.subtype = MEDIASUBTYPE_HDYC; // Blackmagic Packed YUV 422
	   } else if (have_YUY2_) {
		   mt_.subtype = MEDIASUBTYPE_YUY2; // Packed YUV 422
	   } else if (have_UYVY_) {
		   mt_.subtype = MEDIASUBTYPE_UYVY; // Packed YUV 422
	   } else if (have_I420_) {
		   mt_.subtype = MEDIASUBTYPE_I420; // Planar YUV 420
	   } else if (have_RGB24_) {
		   mt_.subtype = MEDIASUBTYPE_RGB24; // RGB 24 bit
	   } else {
		   mt_.subtype = MEDIASUBTYPE_UYVY;
	   }
   } else {
	   if (have_HDYC_) {
		   mt_.subtype = MEDIASUBTYPE_HDYC; // Blackmagic Packed YUV 422
	   } else if (have_I420_) {
		   mt_.subtype = MEDIASUBTYPE_I420; // Planar YUV 420
	   } else if (have_YUY2_) {
		   mt_.subtype = MEDIASUBTYPE_YUY2; // Packed YUV 422
	   } else if (have_UYVY_) {
		   mt_.subtype = MEDIASUBTYPE_UYVY; // Packed YUV 422
	   } else if (have_RGB24_) {
		   mt_.subtype = MEDIASUBTYPE_RGB24; // RGB 24 bit
	   } else {
		   mt_.subtype = MEDIASUBTYPE_UYVY;
	   }
   }

   hr = pSampleGrabber_->SetMediaType(&mt_);
   //showErrorMessage(hr);

   // Obtain the interface used to run, stop, and pause the graph
   hr = pGraph_->QueryInterface(IID_IMediaControl, (void **)&pMediaControl_);
   //showErrorMessage(hr);
   if (FAILED(hr)) {
       Grabber::status_=-1;
       return;
   }
   debug_msg("DirectShowGrabber::DirectShowGrabber():  graph media control interface acquired\n");

   if (have_DVSD_) {
       // Get the DV Video Codec DS default filter
       hr = CoCreateInstance(CLSID_DVVideoCodec, NULL, CLSCTX_INPROC_SERVER,
                             IID_IBaseFilter, (LPVOID *)&pDVDecoderBaseFilter_);
       if (FAILED(hr)) {
           Grabber::status_=-1;
           return;
       }
       debug_msg("DirectShowGrabber::DirectShowGrabber():  DV Video Codec interface acquired\n");

       hr = pDVDecoderBaseFilter_->QueryInterface(IID_IIPDVDec, (void**)&pDVVideoDecoder_);
       debug_msg("DirectShowGrabber::DirectShowGrabber():  Sample Grabber interface acquired\n");

       // Add the DV Video Codec to the graph
       hr = pGraph_->AddFilter(pDVDecoderBaseFilter_, L"Vic DV Video Decoder");
       if (FAILED(hr)) {
           Grabber::status_=-1;
           return;
       }
       debug_msg("DirectShowGrabber::DirectShowGrabber():  DV Video Decoder added to graph\n");
   }

   IMoniker * pMoniker = NULL;
   IRunningObjectTable *pROT = NULL;

   // register the filter graph instance in the Running Object Table (ROT)
   // so can use GraphEdit to view graph, e.g.
   //    in GraphEdit's File menu, click "Connect to Remote Graph..."
   if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) {
     const size_t STRING_LENGTH = 256;

     WCHAR wsz[STRING_LENGTH];
     StringCbPrintfW(wsz, STRING_LENGTH, L"FilterGraph %08x pid %08x", (DWORD_PTR)pGraph_, GetCurrentProcessId());

     HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
     if (SUCCEEDED(hr)) {
         hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pGraph_, pMoniker, &dwRegister_);
         pMoniker->Release();
     }
     pROT->Release();
   }
}

//--------------------------------

DirectShowGrabber::~DirectShowGrabber() {
    HRESULT hr;

    debug_msg("~DirectShowGrabber()\n");

    //capturing_ = !capturing_;
    if (capturing_) hr  = pMediaControl_->Stop();
    //showErrorMessage(hr);

    CloseHandle(cb_mutex_);

    // unregister the filter graph instance in the Running Object Table (ROT).
    IRunningObjectTable *pROT;
    if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) {
        pROT->Revoke(dwRegister_);
        pROT->Release();
    }

    // Release COM objects in reverse order of instantiation
    callback_->Release();
    //delete callback; - done by above Release() call
    if (have_DVSD_) {
        if (pDVVideoDecoder_ != 0) {
            pDVVideoDecoder_->Release();
        }
        if (pDVDecoderBaseFilter_ != 0) {
            pDVDecoderBaseFilter_->Release();
        }
    }
    if (pNullBaseFilter_ != 0) {
        pNullBaseFilter_->Release();
    }
    if (pSampleGrabber_ != 0) {
        pSampleGrabber_->Release();
    }
    if (pGrabberBaseFilter_ != 0) {
        pGrabberBaseFilter_->Release();
    }
    if (pMediaControl_ != 0) {
        pMediaControl_->Release();
    }
    if (pGraph_ != 0) {
        pGraph_->Release();
    }
    if (pBuild_ != 0) {
        pBuild_->Release();
    }
}

//--------------------------------

bool DirectShowGrabber::findCrossbar(IBaseFilter *pCapF) {
   HRESULT     hr;

   debug_msg("DirectShowGrabber::FindCrossbar()...\n");

   hr = pBuild_->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, pCapF, IID_IAMCrossbar,
                             (void**)&pXBar_);

   if ( SUCCEEDED(hr) ) {
      addCrossbar(pXBar_);
      hr = pXBar_->QueryInterface(IID_IBaseFilter, (void**)&pFilter_);
      if ( SUCCEEDED(hr) ) {
         debug_msg("DirectShowGrabber::FindCrossbar()...Found and added\n");
         //findCrossbar(pFilter_);
         //pFilter_.Release();
         return TRUE;
      }
   }
   return FALSE;
}

//-------------------------------

void DirectShowGrabber::addCrossbar(IAMCrossbar *xbar) {
   Crossbar *pCross;

   debug_msg("DirectShowGrabber::addCrossbar()\n");

   pCross = new Crossbar(xbar);

   if( crossbar_ == NULL ) {
      crossbar_ = pCross;
   }
   else {
      crossbarCursor_->next = pCross;
   }
   crossbarCursor_ = pCross;
}

//-----------------------------

void DirectShowGrabber::routeCrossbar() {
    HRESULT     hr;
    long        output           = -1;
    long        input            = -1;
    int         videoDecoderPort = -1;
    int         port;
    long        related;
    long        pinType;
    IAMCrossbar *xb;

    if( crossbar_ == NULL ) return;

    xb = crossbar_->getXBar();


    hr = xb->get_PinCounts(&output, &input);
    debug_msg("num input %d num output %d", input, output);


    long selectedPort = -1;
    long inport = -1;
    for( int i = 0; i < output; ++i ) {
        xb->get_IsRoutedTo(i, &inport);
        debug_msg("DirectShowGrabber::routeCrossbar():  input pin %d is mapped to output pin %d\n", inport, i);
        if( inport >= 0 )
            selectedPort = inport;
    }


    if(initializedPorts == 0) {
        initializedPorts=1;
        svideoPortNum = 1;
        compositePortNum = 1;
        numInputPorts = 0;
        for( int i = 0; i < input; ++i ) {
            xb->get_CrossbarPinInfo(TRUE, i, &related, &pinType);
            if( pinType == PhysConn_Video_SVideo ) {
                debug_msg("Found svideo port %d\n", svideoPortNum);
                inputPorts[numInputPorts] = new Port();
                inputPorts[numInputPorts]->id = i;
                if( svideoPortNum > 1 )
                    StringCbPrintf(inputPorts[numInputPorts]->name, 64, "S-Video%d", svideoPortNum );
                else
                    StringCbCopy(inputPorts[numInputPorts]->name, 64, "S-Video");
                if( selectedPort == i )
                    StringCbCopy(input_port_, 64, inputPorts[numInputPorts]->name);
                numInputPorts++;
                svideoPortNum++;

            }
            if( pinType == PhysConn_Video_Composite ) {
                debug_msg("Found composite port %d\n", compositePortNum);
                inputPorts[numInputPorts] = new Port();
                inputPorts[numInputPorts]->id = i;
                if(compositePortNum > 1 )
                    StringCbPrintf(inputPorts[numInputPorts]->name, 64, "Composite%d", compositePortNum);
                else
                    StringCbCopy(inputPorts[numInputPorts]->name, 64, "Composite");
                if( selectedPort == i )
                    StringCbCopy(input_port_, 64, inputPorts[numInputPorts]->name);
                numInputPorts++;
                compositePortNum++;
            }
        }
    }

    for( int i = 0; i < output; ++i ) {
        xb->get_CrossbarPinInfo(FALSE, i, &related, &pinType);
        if( pinType == PhysConn_Video_VideoDecoder ) {
            videoDecoderPort = i;
            break;
        }
    }

    port = -1;
    for( int i=0; i<numInputPorts; i++) {
        if (strcmp(input_port_,inputPorts[i]->name) == 0) {
            port = inputPorts[i]->id;
            break;
        }
    }
    if( port == -1 ) { 
        debug_msg("**** Failed to find port for %s\n", input_port_);
        return;
    }

    if( xb->CanRoute(videoDecoderPort, port) == S_FALSE )
        debug_msg("DirectShowGrabber::routeCrossbar():  cannot route input pin %d to output pin %d\n", port, videoDecoderPort);
    else {
        debug_msg("DirectShowGrabber::routeCrossbar() routing pin %d to pin %d\n", port, videoDecoderPort);
        hr = xb->Route(videoDecoderPort, port);
        //showErrorMessage(hr);
    }

    xb->get_IsRoutedTo(videoDecoderPort, &input);
    debug_msg("DirectShowGrabber::routeCrossbar():  pin %d is now routed to output pin %d\n", input, videoDecoderPort);
}

//-----------------------------

void DirectShowGrabber::start() {
   HRESULT hr;

   setsize();
   setCaptureOutputFormat();
   hr = pBuild_->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
   pCaptureFilter_, pGrabberBaseFilter_, pNullBaseFilter_);
   if (SUCCEEDED(hr) )
       debug_msg("DirectShowGrabber::DirectShowGrabber():  builder render stream\n");
   else {
       debug_msg("DirectShowGrabber::DirectShowGrabber():  FAILED to builder render stream: %x\n", hr);
       // This _usually_ fails because RendersStream has already been called - we should really
       // diassemble the filterGraph and rebuild it when we change stuff.
       //stop();
       //return;
   }

   WaitForSingleObject(cb_mutex_, INFINITE);

   capturing_  = 1;
   last_frame_ = NULL;

   debug_msg("DirectShowGrabber::start():  starting capture graph...\n");

   // Run the graph...
   hr = pMediaControl_->Run();
   if (SUCCEEDED(hr) )
       debug_msg("DirectShowGrabber::DirectShowGrabber():  Graph set to Run\n");

   //showErrorMessage(hr);

   switch (cformat_) {
   case CF_420:
   case CF_CIF:
       converter(new RGB_Converter_420(24, (u_int8_t *)NULL, 0));
       break;
   case CF_422:
       converter(new RGB_Converter_422(24, (u_int8_t *)NULL, 0));
       break;
   }

   Grabber::start();
   ReleaseMutex(cb_mutex_);
   Grabber::timeout();
}

//--------------------------------

void DirectShowGrabber::stop() {
   HRESULT hr;

   debug_msg("DirectShowGrabber::stop() thread=%x\n", GetCurrentThreadId());

   if (capturing_) {
      hr = pMediaControl_->Stop();
   }
   //showErrorMessage(hr);
   ReleaseMutex(cb_mutex_);

   if (converter_) {
       delete converter_;
       converter_ = 0;
   }
   capturing_  = 0;
   last_frame_ = 0;

   Grabber::stop();
}

//--------------------------------

void DirectShowGrabber::fps(int f) {
   if (f <= 0)
      f = 1;
   else if (u_int(f) > max_fps_)
      f = max_fps_;

   Grabber::fps(f);
}

void DirectShowGrabber::setsize() {

   if (decimate_ == 1 && !have_DVSD_){  //i.e. Large
	   int flags = TCL_GLOBAL_ONLY;
	   Tcl& tcl = Tcl::instance();
	   const char* largeSizeResolution = Tcl_GetVar(tcl.interp(), "largeSizeResolution", flags);
	   if (largeSizeResolution) {
		   sscanf(largeSizeResolution, "%ix%i", &width_, &height_);
	   } else {
		   width_  = 640;
		   height_ = 480;
	   }
   } else {
       width_ = basewidth_  / decimate_;
       height_ = baseheight_ / decimate_;
   }

   if (have_DVSD_) {
       switch(decimate_) {
           case 1:
             pDVVideoDecoder_->put_IPDisplay(DVRESOLUTION_FULL);
             break;
           case 2:
             pDVVideoDecoder_->put_IPDisplay(DVRESOLUTION_HALF);
             break;
           case 4:
             pDVVideoDecoder_->put_IPDisplay(DVRESOLUTION_QUARTER);
             break;
       }
   }

   debug_msg("DirectShowGrabber::setsize: %dx%d\n", width_, height_);

   switch (cformat_) {
       case CF_CIF:
           set_size_cif(width_, height_);
           break;
       case CF_420:
           set_size_420(width_, height_);
           break;
       case CF_422:
           set_size_422(width_, height_);
           break;
   }

   allocref();
}

//--------------------------------

void DirectShowGrabber::capture(BYTE *frameBuf, long bufLen) {
   last_frame_ = frameBuf;
   // debug_msg("DirectShowGrabber::capture: frameBuf=%p, last_frame_=%p, bufLen=%ld\n", frameBuf, last_frame_, bufLen);
}

//--------------------------------

int DirectShowGrabber::grab() {
    int rval;


    //debug_msg("DirectShowGrabber::grab: thread=%x w=%d h=%d bw=%d bh=%d frame_=%p fsize_=%d in=%dx%d out=%dx%d\n",
   //          GetCurrentThreadId(),
   //          width_, height_, basewidth_, baseheight_, frame_, framesize_,
   //          inw_, inh_, outw_, outh_);

    WaitForSingleObject(cb_mutex_, INFINITE);

    if( last_frame_ == NULL || capturing_ == 0 ) {
        ReleaseMutex(cb_mutex_);
        return FALSE;
    }
    switch (cformat_) {
    case CF_420:
    case CF_CIF:
        if (have_HDYC_)
            packedUYVY422_to_planarYUYV420((char *)frame_, outw_, outh_, (char *)last_frame_, inw_, inh_);
        else if (have_I420_)
            planarYUYV420_to_planarYUYV420((char *)frame_, outw_, outh_, (char *)last_frame_, inw_, inh_);
        else if (have_YUY2_)
            packedYUYV422_to_planarYUYV420((char *)frame_, outw_, outh_, (char *)last_frame_, inw_, inh_);
        else if (have_UYVY_)
            packedUYVY422_to_planarYUYV420((char *)frame_, outw_, outh_, (char *)last_frame_, inw_, inh_);
        else if (have_RGB24_)
            converter_->convert((u_int8_t*)last_frame_, width_, height_, frame_, outw_, outh_, TRUE);
        else
            packedUYVY422_to_planarYUYV420((char *)frame_, outw_, outh_, (char *)last_frame_, inw_, inh_);
        break;

    case CF_422:
        if (have_HDYC_)
            packedUYVY422_to_planarYUYV422((char *)frame_, outw_, outh_, (char *)last_frame_, inw_, inh_);
        if (have_YUY2_)
            packedYUYV422_to_planarYUYV422((char *)frame_, outw_, outh_, (char *)last_frame_, inw_, inh_);
        else if (have_UYVY_)
            packedUYVY422_to_planarYUYV422((char *)frame_, outw_, outh_, (char *)last_frame_, inw_, inh_);
        else if (have_I420_)
            planarYUYV420_to_planarYUYV422((char *)frame_, outw_, outh_, (char *)last_frame_, inw_, inh_);
        else if (have_RGB24_)
            converter_->convert((u_int8_t*)last_frame_, width_, height_, frame_, outw_, outh_, TRUE);
        else
            packedUYVY422_to_planarYUYV422((char *)frame_, outw_, outh_, (char *)last_frame_, inw_, inh_);
        break;
    }

    last_frame_ = NULL;

    suppress(frame_);
    saveblks(frame_);
    YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);

    rval = (target_->consume(&f));

    // release block so that callback can get new frame
    ReleaseMutex(cb_mutex_);

    return rval;
}

//--------------------------------

void DirectShowGrabber::setport(const char *port) {

   debug_msg("DirectShowGrabber::setport: %s thread=%x\n", port, GetCurrentThreadId());
   StringCbCopy(input_port_, 64, port);
   routeCrossbar();

}
//  Free an existing media type (ie free resources it holds)

void FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0) {
        CoTaskMemFree((PVOID)mt.pbFormat);

        // Strictly unnecessary but tidier
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL) {
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}
// general purpose function to delete a heap allocated AM_MEDIA_TYPE structure
// which is useful when calling IEnumMediaTypes::Next as the interface
// implementation allocates the structures which you must later delete
// the format block may also be a pointer to an interface to release

void  DeleteMediaType( AM_MEDIA_TYPE *pmt)
{
    // allow NULL pointers for coding simplicity

    if (pmt == NULL) {
        return;
    }

    FreeMediaType(*pmt);
    CoTaskMemFree((PVOID)pmt);
}
//--------------------------------

static bool sizeComp(SIZE lhs, SIZE rhs) {return ((lhs.cy << 16) + lhs.cx) < ((rhs.cy << 16) + rhs.cx);}

int DirectShowGrabber::getCaptureCapabilities() {
   IAMStreamConfig          *pConfig;
   AM_MEDIA_TYPE            *pmtConfig;
   int                      iCount;
   int                      iSize;
   VIDEO_STREAM_CONFIG_CAPS scc;
   HRESULT                  hr;
   VIDEOINFOHEADER          *pVih;

   bool(*fn_pt)(SIZE, SIZE) = sizeComp;
   set<SIZE,bool(*)(SIZE, SIZE)> resolutionSet(fn_pt);

   pConfig   = NULL;
   hr        = pBuild_->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
                                     pCaptureFilter_, IID_IAMStreamConfig, (void**)&pConfig);
   if (FAILED(hr)) {
       return FALSE;
   }

   max_width_ = 0;
   max_height_ = 0;
   min_width_ = 0xFFFF;
   min_height_ = 0xFFFF;
   iCount = iSize = 0;
   hr     = pConfig->GetNumberOfCapabilities(&iCount, &iSize);
   // Check the size to make sure we pass in the correct structure.
   // The alternative output of iSize is AUDIO_STREAM_CONFIG_CAPS, btw.
   if ( iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS) ) {

       for (int iFormat = 0; iFormat < iCount; iFormat++) {
           hr = pConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE *)&scc);
           //showErrorMessage(hr);
           if( SUCCEEDED(hr) ) {
               if ((pmtConfig->majortype  == MEDIATYPE_Video)          &&
                   (pmtConfig->formattype == FORMAT_VideoInfo)         &&
                   (pmtConfig->cbFormat   >= sizeof (VIDEOINFOHEADER)) &&
                   (pmtConfig->pbFormat   != NULL)) {
					   resolutionSet.insert(scc.InputSize);
                       if(scc.MaxOutputSize.cy > max_height_){
                           max_width_  = scc.MaxOutputSize.cx;
                           max_height_ =  scc.MaxOutputSize.cy;
                       }
                       if(scc.MinOutputSize.cx < min_width_){
                           min_width_  = scc.MinOutputSize.cx;
                           min_height_ =  scc.MinOutputSize.cy;
                       }
                       if (pmtConfig->subtype == MEDIASUBTYPE_I420) {
                           have_I420_ = true; // Planar YUV 420
                       } else if (pmtConfig->subtype == MEDIASUBTYPE_UYVY) {
                           have_UYVY_ = true; // Packed YUV 422
                       } else if (pmtConfig->subtype == MEDIASUBTYPE_YUY2) {
                           have_YUY2_ = true; // Packed YUV 422
                       } else if (pmtConfig->subtype == MEDIASUBTYPE_HDYC) {
                           have_HDYC_ = true; // Blackmagic Packed YUV 422
                       } else if (pmtConfig->subtype == MEDIASUBTYPE_RGB24) {
                           have_RGB24_ = true; // RGB 24 bit
                       } else if (pmtConfig->subtype == MEDIASUBTYPE_dvsd) {
                           have_DVSD_ = true; // DV Standard definition
                       }

                       debug_msg("Windows GDI BITMAPINFOHEADER follows:\n");
                       pVih                        = (VIDEOINFOHEADER *)pmtConfig->pbFormat;
                       debug_msg("biWidth=        %d\n", pVih->bmiHeader.biWidth);
                       debug_msg("biHeight=       %d\n", pVih->bmiHeader.biHeight);
                       debug_msg("biSize=         %d\n", pVih->bmiHeader.biSize);
                       debug_msg("biPlanes=       %d\n", pVih->bmiHeader.biPlanes);
                       debug_msg("biBitCount=     %d\n", pVih->bmiHeader.biBitCount);
                       debug_msg("biCompression=  %d\n", pVih->bmiHeader.biCompression);
                       debug_msg("biSizeImage=    %d\n", pVih->bmiHeader.biSizeImage);
                       debug_msg("biXPelsPerMeter=%d\n", pVih->bmiHeader.biXPelsPerMeter);
                       debug_msg("biYPelsPerMeter=%d\n", pVih->bmiHeader.biYPelsPerMeter);
                   }
                   DeleteMediaType(pmtConfig);
           }
       }
   }
   int i=0;
   set<SIZE, bool(*)(SIZE, SIZE)>::iterator it;
   for (it=resolutionSet.begin() ; it != resolutionSet.end() && i < NUM_LARGE_SIZE_RESOLUTIONS; it++) {
	  if (it->cy < NTSC_BASE_HEIGHT) continue; // ignore resolutions < 640p for Large capture
	  largeSizeResolutions[i].cx = it->cx;
	  largeSizeResolutions[i].cy = it->cy;
	  i++;
   }
   pConfig->Release();

   if (max_width_>0)
       return TRUE;

   return FALSE;
}

void DirectShowGrabber::setCaptureOutputFormat() {
   IAMStreamConfig          *pConfig;
   int                      iCount;
   int                      iSize;
   int                      curr_w=0;
   int                      curr_h=0;
   int                      temp_w, temp_h;
   VIDEOINFOHEADER          *pVih;
   VIDEO_STREAM_CONFIG_CAPS scc;
   AM_MEDIA_TYPE            *pmtConfig;
   int                      formatSet;
   HRESULT                  hr;

   // Reference http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directshow/htm/configurethevideooutputformat.asp

   debug_msg("DirectShowGrabber::setCaptureOutputFormat(): enter...\n");

   formatSet = 0;
   pConfig   = NULL;
   hr        = pBuild_->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
                                     pCaptureFilter_, IID_IAMStreamConfig, (void**)&pConfig);
   if (FAILED(hr)) {
        debug_msg("Failed to FindInterface\n");
        Grabber::status_=-1;
        return;
   }

   debug_msg("DirectShowGrabber::setCaptureOutputFormat(): IAMStreamConfig interface acquired\n");

   iCount = iSize = 0;
   hr     = pConfig->GetNumberOfCapabilities(&iCount, &iSize);
   // Check the size to make sure we pass in the correct structure.
   // The alternative output of iSize is AUDIO_STREAM_CONFIG_CAPS, btw.
   if ( iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS) ) {
      GUID mediasubtype = MEDIASUBTYPE_NULL;

      switch (cformat_) {
      case CF_420:
      case CF_CIF:
          if (have_HDYC_)
              mediasubtype = MEDIASUBTYPE_HDYC; // Blackmagic Packed YUV 422
          else if (have_I420_)
              mediasubtype = MEDIASUBTYPE_I420; // Planar YUV 420
          else if (have_YUY2_)
              mediasubtype = MEDIASUBTYPE_YUY2; // Packed YUV 422
          else if (have_UYVY_)
              mediasubtype = MEDIASUBTYPE_UYVY; // Packed YUV 422
          else if (have_RGB24_)
              mediasubtype = MEDIASUBTYPE_RGB24; // RGB 24 bit
        break;

      case CF_422:
          if (have_HDYC_)
              mediasubtype = MEDIASUBTYPE_HDYC; // Blackmagic Packed YUV 422
          else if (have_YUY2_)
              mediasubtype = MEDIASUBTYPE_YUY2; // Packed YUV 422
          else if (have_UYVY_)
              mediasubtype = MEDIASUBTYPE_UYVY; // Packed YUV 422
          else if (have_I420_)
              mediasubtype = MEDIASUBTYPE_I420; // Planar YUV 420
          else if (have_RGB24_)
              mediasubtype = MEDIASUBTYPE_RGB24; // RGB 24 bit
          break;
      }

      for (int iFormat = 0; iFormat < iCount; iFormat++) {
         hr = pConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE *)&scc);
         //showErrorMessage(hr);

         if( SUCCEEDED(hr) ) {
            if ((pmtConfig->majortype  == MEDIATYPE_Video)            &&
                  (pmtConfig->subtype == mediasubtype || mediasubtype == MEDIASUBTYPE_NULL) &&
                  (pmtConfig->formattype == FORMAT_VideoInfo)         &&
                  (pmtConfig->cbFormat   >= sizeof (VIDEOINFOHEADER)) &&
                  (pmtConfig->pbFormat   != NULL) /*                  &&
                  (scc.MaxOutputSize.cx <= width_)                    &&
                  (scc.MaxOutputSize.cy <= height_)*/){

               if ( (width_ <= scc.MaxOutputSize.cx) && (width_ >= scc.MinOutputSize.cx) &&
                    (height_ <= scc.MaxOutputSize.cx) && (height_ >= scc.MinOutputSize.cy)) {

                    pVih = (VIDEOINFOHEADER *)pmtConfig->pbFormat;
                    //pVih->bmiHeader.biWidth     = width_;
                    //pVih->bmiHeader.biHeight    = height_;
                    //pVih->bmiHeader.biSizeImage = DIBSIZE(pVih->bmiHeader);
                    // AvgTimePerFrame value that specifies the video frame'
                    // average display time, in 100-nanosecond units.
                    if (fps_)
                        pVih->AvgTimePerFrame = 10000000/fps_;

                    debug_msg("fps_= %d, AvgTimePerFrame: %d\n", fps_, pVih->AvgTimePerFrame);

                    debug_msg("Windows GDI BITMAPINFOHEADER follows:\n");
                    debug_msg("biWidth=        %d\n", pVih->bmiHeader.biWidth);
                    debug_msg("biHeight=       %d\n", pVih->bmiHeader.biHeight);
                    debug_msg("biSize=         %d\n", pVih->bmiHeader.biSize);
                    debug_msg("biPlanes=       %d\n", pVih->bmiHeader.biPlanes);
                    debug_msg("biBitCount=     %d\n", pVih->bmiHeader.biBitCount);
                    debug_msg("biCompression=  %d\n", pVih->bmiHeader.biCompression);
                    debug_msg("biSizeImage=    %d\n", pVih->bmiHeader.biSizeImage);
                    debug_msg("biXPelsPerMeter=%d\n", pVih->bmiHeader.biXPelsPerMeter);
                    debug_msg("biYPelsPerMeter=%d\n", pVih->bmiHeader.biYPelsPerMeter);
                    debug_msg("biClrUsed=      %d\n", pVih->bmiHeader.biClrUsed);
                    debug_msg("biClrImportant= %d\n", pVih->bmiHeader.biClrImportant);

                    temp_w = pVih->bmiHeader.biWidth;
                    temp_h = pVih->bmiHeader.biHeight;
                    pVih->bmiHeader.biWidth     = width_;
                    pVih->bmiHeader.biHeight    = height_;
                    hr = pConfig->SetFormat(pmtConfig);
                    if (SUCCEEDED(hr)) {
                        curr_w = width_;
                        curr_h = height_;
                        formatSet = 1;
                        debug_msg("Set(wxh): %dx%d, and Got: %dx%d\n",width_,height_, pVih->bmiHeader.biWidth, pVih->bmiHeader.biHeight);
                        break;
                    } else {
                        if ((temp_w < width_) && (temp_h < height_)) {
                            pVih->bmiHeader.biWidth = temp_w;
                            pVih->bmiHeader.biHeight = temp_h;
                            hr = pConfig->SetFormat(pmtConfig);
                            if (SUCCEEDED(hr)) {
                                curr_w = temp_w;
                                curr_h = temp_h;
                                formatSet = 1;
                                debug_msg("Set(wxh): %dx%d, and Got: %dx%d\n",width_,height_, pVih->bmiHeader.biWidth, pVih->bmiHeader.biHeight);
                            }
                        } else {
                            debug_msg("Failed to Set format this time - trying again\n");
                        }
                    }
               }
            }
            DeleteMediaType(pmtConfig);
         }
      }
   }
   pConfig->Release();

   if ( formatSet ) {
      if ( (curr_w != width_) || (curr_h != height_ )) {
           width_  = curr_w;
           height_ = curr_h;
		   switch (cformat_) {
		   case CF_CIF:
			   set_size_cif(width_, height_);
			   break;
		   case CF_420:
			   set_size_420(width_, height_);
			   break;
		   case CF_422:
			   set_size_422(width_, height_);
			   break;
		   }
           debug_msg("DirectShowGrabber::setCaptureOutputFormat:  format set to near res: %dx%d\n",width_,height_);
      } else
           debug_msg("DirectShowGrabber::setCaptureOutputFormat:  format set\n");
   }
   else
      debug_msg("DirectShowGrabber::setCaptureOutputFormat:  format not set\n");
}

//--------------------------------

int DirectShowGrabber::command(int argc, const char* const* argv) {

    if (argc == 3) {
        if (strcmp(argv[1], "decimate") == 0) {
            u_int dec = (u_int)atoi(argv[2]);
            Tcl& tcl = Tcl::instance();
            if (dec <= 0) {
                tcl.resultf("%s: divide by zero", argv[0]);
                return (TCL_ERROR);
            }
            debug_msg("DirectShowGrabber::command: decimate=%d (dec)=%d\n", dec, decimate_);
            if (dec != decimate_) {
                decimate_ = dec;
                if (running_) {
                    stop();
                    setsize();
                    setCaptureOutputFormat();
                    start();
                } else{
                    setsize();
                    setCaptureOutputFormat();
                }
            }
            return (TCL_OK);
        } else if (strcmp(argv[1], "port") == 0) {
            setport(argv[2]);
            return (TCL_OK);
        } else if (strcmp(argv[1], "type") == 0) {
            if (strcmp(argv[2], "auto") == 0)
                ;
            else if (strcmp(argv[2], "pal") == 0) {
                if (have_DVSD_) { // DV Standard definition
                    basewidth_  = 720;
                    baseheight_ = 576;
                } else {
                    basewidth_  = CIF_BASE_WIDTH;
                    baseheight_ = CIF_BASE_HEIGHT;
                }
            } else if (strcmp(argv[2], "ntsc") == 0) {
                if (have_DVSD_) { // DV Standard definition
                    basewidth_  = 720;
                    baseheight_ = 480;
                } else {
                    basewidth_  = NTSC_BASE_WIDTH;
                    baseheight_ = NTSC_BASE_HEIGHT;
                }
            }
            if (running_) {
                stop();
                setsize();
                setCaptureOutputFormat();
                start();
            } else {
                setsize();
                setCaptureOutputFormat();
            }
            return (TCL_OK);
        } else if (strcmp(argv[1], "useconfig") ==0) {
            if (strcmp(argv[2], "1") == 0)
                useconfig_=1;
            if (strcmp(argv[2], "0") == 0)
                useconfig_=0;
        }
    }
    return (Grabber::command(argc, argv));

}


//#########################################################################
// DirectShowDevice class

DirectShowDevice::DirectShowDevice(char *friendlyName, IBaseFilter *pCapFilt) : InputDevice(friendlyName, "directshow") {

    attri_ = new char[512];
    attri_[0] = 0;

    debug_msg("new DirectShowDevice():  friendlyName=%s\n", friendlyName);
    pDirectShowFilter_  = pCapFilt;
    DirectShowGrabber o(pDirectShowFilter_, "420", friendlyName);

    StringCbCatA(attri_, 511, "format { 420 422 cif } size { ");

    if ((o.minHeight() > (CIF_BASE_HEIGHT / 2)) && !o.hasDV_SD()) {
        StringCbCatA(attri_, 511, "large");
    } else if (o.maxWidth() < NTSC_BASE_WIDTH) {
        StringCbCatA(attri_, 511, "small cif");
    } else {
        StringCbCatA(attri_, 511, "small cif large");
    }

    StringCbCatA(attri_, 511, " } type { pal ntsc } port { ");

    Port **inputPorts = o.getInputPorts();
    if(inputPorts[0] != NULL) {
        int i=0;
        while( i <  NUM_PORTS && inputPorts[i] != NULL ) {
            StringCbCatA(attri_, 511, inputPorts[i]->name);
            StringCbCatA(attri_, 511, " ");
            i++;
        }
    }else{
        StringCbCatA(attri_, 511, "external-in ");
    }
	StringCbCatA(attri_, 511, "} ");
    debug_msg("new DirectShowDevice():  after appending ports\n");

	SIZE *largeSizeResolutions = o.getLargeSizeResolutions();
    if(largeSizeResolutions[0].cx != 0) {
		StringCbCatA(attri_, 511, "large_size_resolution {");
        int i=0;
        while( i <  NUM_LARGE_SIZE_RESOLUTIONS && largeSizeResolutions[i].cx != 0 ) {
			StringCbPrintfA(attri_, 511, "%s %ix%i", attri_, largeSizeResolutions[i].cx, largeSizeResolutions[i].cy);
            i++;
        }
		StringCbCatA(attri_, 511, "} ");
    }

    char *inport = o.getInputPort();
    StringCbPrintfA(attri_, 511, "%s selected_port { %s }", attri_, inport );
    free(inport);

    attributes_ = attri_;
    debug_msg("attributes: %s", attributes_);
}

DirectShowDevice::~DirectShowDevice(){
    // Release necessary as ATL smart pointers are NOT used.
    pDirectShowFilter_->Release();
    delete attri_;
}
//--------------------------------

int DirectShowDevice::command(int argc, const char* const* argv) {
    Tcl& tcl = Tcl::instance();
    if ((argc == 3) && (strcmp(argv[1], "open") == 0)) {
        TclObject* o = 0;

        o = directShowGrabber_ = new DirectShowGrabber(pDirectShowFilter_, argv[2]);
        if (o != 0)
            Tcl::instance().result(o->name());
        return (TCL_OK);
    } else if (argc == 2) {
		if (strcmp(argv[1], "api") == 0) {
			tcl.result(api_);
			return (TCL_OK);
		} else if (strcmp(argv[1], "properties") == 0) {
			DisplayPropertyPage();
            return TCL_OK;
		}
		tcl.evalc("set_capture_resolution_button_state");
	}
    return (InputDevice::command(argc, argv));
}

// This fn displays the custom Property page to set something
bool DirectShowDevice::DisplayPropertyPage() {
	bool Done = false;

	ISpecifyPropertyPages *pProp;
	HRESULT hr = pDirectShowFilter_->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);
	if (SUCCEEDED(hr))  {
		IUnknown *pFilterUnk;
		pDirectShowFilter_->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);

		// Show the page.
		CAUUID caGUID;
		pProp->GetPages(&caGUID);
		pProp->Release();
		OleCreatePropertyFrame(
			NULL,                   // Parent window
			0, 0,                   // Reserved
			NULL,                   // Caption for the dialog box
			1,                      // Number of objects (just the filter)
			&pFilterUnk,            // Array of object pointers.
			caGUID.cElems,          // Number of property pages
			caGUID.pElems,          // Array of property page CLSIDs
			0,                      // Locale identifier
			0, NULL                 // Reserved
		);

		// Clean up.
		pFilterUnk->Release();
		CoTaskMemFree(caGUID.pElems);
		Done = true;
	}
	return Done;
}

//#########################################################################
// DirectShowScanner class

DirectShowScanner::DirectShowScanner():pMoniker_(0)
{
	ICreateDevEnum *pDevEnum      = 0;
	int             hr;
	int             devNum;
	char            nameBuf[NAMEBUF_LEN];

	// Reference:  Pesce, pp 54-56.

	debug_msg("new DirectShowScanner()\n");

	// Initialize the COM subsystem - it seems that CoInitializeEx is reccommended with COINIT_MULTITHREADED as the driver will spawn threads.
	hr=CoInitializeEx(NULL,COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		debug_msg("Failed COM subsystem initialisation.\n");
		return;
	}

	// Create a helper object to find the capture devices.
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (LPVOID*)&pDevEnum);
	if (FAILED(hr)) {
		debug_msg("Failed to Create a helper object to find the DS capture devices.\n");
		CoUninitialize();
		return;
	}

	IEnumMoniker *pEnum    = 0;
	IPropertyBag *pPropBag = 0;
	VARIANT      varName;
	CLSID        clsid;

	// Get an enumerator over video capture filters
	hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
	//showErrorMessage(hr);
	if (FAILED(hr) || pEnum == 0) {
		debug_msg("Failed to Get an enumerator over DS video capture filters.\n");
		CoUninitialize();
		return;
	}

	// Get the capture filter for each device installed, up to NUM_DEVS devices
	for( devNum=0; devNum < NUM_DEVS; ++devNum) {
		if ( pEnum->Next(1, &pMoniker_, NULL) == S_OK ) {

			hr = pMoniker_->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
			if (FAILED(hr)) {
				debug_msg("Failed to Get propbag bound to storage on DS dev: %d\n", devNum);
				continue;
			}
			//showErrorMessage(hr);
			debug_msg("propbag bound to storage ok= %d\n", hr);

			VariantInit(&varName);
			hr = pPropBag->Read(L"FriendlyName", &varName, 0);
			if (FAILED(hr)) {
				debug_msg("Failed to Get friendly name read on DS dev: %d\n", devNum);
				VariantClear(&varName);
				pPropBag->Release();
				continue;
			}
			//showErrorMessage(hr);
			debug_msg("friendly name read ok= %d\n", hr);

			// Need this macro in atlconv.h to go from bStr to char* - msp
			//USES_CONVERSION;

			nameBuf[0] = '\0';
			WideCharToMultiByte(CP_ACP, 0, varName.bstrVal, -1, nameBuf, NAMEBUF_LEN,  NULL, NULL);
			//strcpy(nameBuf, W2A(varName.bstrVal));

			debug_msg("DirectShowScanner::DirectShowScanner():  found nameBuf/FriendlyName=%s\n", nameBuf);

			// needs work, but don't add drivers that look like VFW drivers - msp
			//if( (strstr(nameBuf, "VFW") == NULL) ) {
			hr = pMoniker_->BindToObject(0, 0, IID_IBaseFilter, (void **)&pCaptureFilter[devNum]);
			if (FAILED(hr)) {
				debug_msg("Failed to Get friendly name read on DS dev: %d\n", devNum);
				VariantClear(&varName);
				pPropBag->Release();
				continue;
			}
			debug_msg("capture filter bound ok= [%d} %s\n", hr, nameBuf);
			pCaptureFilter[devNum]->GetClassID(&clsid);
			VariantClear(&varName);
			if (IsEqualGUID(clsid,CLSID_VfwCapture)) {
				debug_msg("discarding an apparent VFW device= %s\n", nameBuf);
				devs_[devNum] = NULL;
				pMoniker_->Release();
			} else if (strcmp(nameBuf, "Decklink Video Capture") == 0) {
				debug_msg("discarding an DirectShow based Decklink Video Capture\n");
				devs_[devNum] = NULL;
				pMoniker_->Release();
			} else {
				pMoniker_->AddRef();
				debug_msg("Adding capture filter %d\n", hr);
				devs_[devNum] = new DirectShowDevice(strdup(nameBuf), pCaptureFilter[devNum]);
			}
			pPropBag->Release();
		}
	}

	// Release these objects so COM can release their memory
	pEnum->Release();
	pDevEnum->Release();
}

//--------------------------------

DirectShowScanner::~DirectShowScanner() {
   // This should be printed only when the app exits - msp
   CoUninitialize();
   debug_msg("~DirectShowScanner()\n");
}
