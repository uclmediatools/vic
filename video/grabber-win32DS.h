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

// grabber-win32DS.h

//warning C4996: 'strcpy': This function or variable may be unsafe. Consider using strcpy_s instead. 
// To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include <dshow.h>   // DirectShow
#include <amstream.h>   // DirectShow
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__
// Had to comment out #include  <dxtrans.h> from qedit.h - broken M$ setup?!
// the above #define's make sure it's contents is not needed
#include <qedit.h>   // DirectShow

#include "crossbar.h"

#define NUM_DEVS 20   // max number of capture devices we'll support
//#define showErrorMessage(x)   ShowErrorMessage(x, __LINE__, __FILE__)

//extern void ShowErrorMessage(HRESULT, int, char* );


static const int D1_BASE_WIDTH  = 720;
static const int D1_BASE_HEIGHT = 480;
static const int NTSC_BASE_WIDTH  = 640;
static const int NTSC_BASE_HEIGHT = 480;
static const int PAL_BASE_WIDTH   = 768;
static const int PAL_BASE_HEIGHT  = 568;
static const int CIF_BASE_WIDTH   = 704;
static const int CIF_BASE_HEIGHT  = 576;

//#########################################################################

/*
class Crossbar {
   public:
      Crossbar(IAMCrossbar *);      
      IAMCrossbar *getXBar();

      Crossbar *next;
   private:
      IAMCrossbar  *xbar;      
};
*/

//#########################################################################

class Callback;

class DirectShowGrabber : public Grabber {
   public:
      DirectShowGrabber(IBaseFilter *, const char * nick);
      ~DirectShowGrabber();
      virtual int  command(int argc, const char*const* argv);
      inline void  converter(Converter* v) {
         converter_ = v;
      }
      void         capture(BYTE *, long);

	  bool		   hasComposite(){
	      return (compositePort >= 0);
	  }

	  bool		   hasSVideo(){
		  return (svideoPort >= 0);
	  }

      int          capturing_;
      HANDLE       cb_mutex_;
   protected:
      virtual void start();
      virtual void stop();
      virtual void fps(int);
      virtual void setsize() = 0;
      virtual int  grab();
      void         setport(const char *port);
      int	   getCaptureCapabilities();
      virtual void setCaptureOutputFormat();

      int          useconfig_;
      int          basewidth_;
      int          baseheight_;
      u_int        max_fps_;
	  int		   max_width_;
	  int		   max_height_;
	  int		   width_;
      int		   height_;
      int          compositePort;
	  int		   svideoPort;

      u_int        decimate_;    // set in this::command via small/normal/large in vic UI; msp
      BYTE         *last_frame_;
      Converter    *converter_;

   private:
      IBaseFilter*			 pFilter_;
      IBaseFilter*           pCaptureFilter_;
      ISampleGrabber*        pSampleGrabber_;
      IBaseFilter*           pGrabberBaseFilter_;
      IBaseFilter*           pNullRenderer_;
      IBaseFilter*           pNullBaseFilter_;
      IGraphBuilder*         pGraph_;
      ICaptureGraphBuilder2* pBuild_;
      IMediaControl*         pMediaControl_;
      AM_MEDIA_TYPE          mt_;
      Callback               *callback_;

      IAMCrossbar 			 *pXBar_;
      Crossbar                       *crossbar_;
      Crossbar                       *crossbarCursor_;
      char			     input_port_[20];
      bool                           findCrossbar(IBaseFilter *);
      void                           addCrossbar(IAMCrossbar *);
      void                           routeCrossbar();

};

//#########################################################################

class DirectShowCIFGrabber : public DirectShowGrabber {
   public:
      DirectShowCIFGrabber(IBaseFilter *, const char *nick = 0 );
      ~DirectShowCIFGrabber();
   protected:
      virtual void start();
      virtual void setsize();
};

//#########################################################################

class Callback : public ISampleGrabberCB {
   private:
      volatile long m_cRef;
   public:
      Callback() {}      

      DirectShowGrabber *grabber;

      // IUnknown methods
      STDMETHODIMP_(ULONG) AddRef() {
         return InterlockedIncrement(&m_cRef);
      }

      STDMETHODIMP_(ULONG) Release() {
         long lCount = InterlockedDecrement(&m_cRef);
         if (lCount == 0) {
            delete this;
         }
         // Return the temporary variable, not the member
         // variable, for thread safety.
         return (ULONG)lCount;
      }

      STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);

      // our callbacks
      STDMETHODIMP SampleCB(double sampleTime, IMediaSample *pSample);
      STDMETHODIMP BufferCB(double sampleTime, BYTE *pBuffer, long bufferLen);
};

//#########################################################################

class DirectShowDevice : public InputDevice {
   public:
      DirectShowDevice(char *, IBaseFilter *);
	  ~DirectShowDevice();
      virtual int command(int argc, const char* const* argv);     

   protected:
      IBaseFilter*           pDirectShowFilter_;

      //IBaseFilter       *directShowFilter_;
      DirectShowGrabber *directShowGrabber_;   
	  char *attri_;
};

//#########################################################################

class DirectShowScanner {
   public:      
      DirectShowScanner();
      ~DirectShowScanner();
   protected:
      DirectShowDevice *devs_[NUM_DEVS];
      IMoniker*     pMoniker_;

};
