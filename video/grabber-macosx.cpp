/*
 * grabber-macosx.cpp --
 *
 * Video capture support for Mac OS X.
 *
 * Imported from OpenMash vic. Updated to use SGGetChannelDeviceList to list
 * all of the available SequenceGrabber video capture devices and corresponding
 * inputs, instead of using just the default capture device and input.
 * - Douglas Kosovic <doug@uq.edu.au>
 *
 * Updated to work with the iSight camera (and others) using the
 * SequenceGrabber API - Bruce Williams <netmaster_bruce@mac.com>
 *
 * It would be nicer to do the image conversion by calling DecompressImage with
 * a GWorld as that will scale and resample the image instead of cropping which
 * the YUV conversion functions do.
 *
 * Copyright (c) 2000-2002 The Regents of the University of California.
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
 */

/* QuickTime is not available to 64-bit clients */
#if !__LP64__

#include <QuickTime/QuickTime.h>
#include <Carbon/Carbon.h>
#include <stdio.h>

#if !defined(MAC_TCL) && !defined(MAC_OSX_TK)
#define Cursor XCursor
#endif

#include "grabber.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "module.h"
#include "bsd-endian.h"
#include "yuv_convert.h"

/***************************************************************************/
// FIXME: This is definitely *not* the place to define video sizes
#define PAL                  0
#define NTSC                 1

// CIF
#define CIF_WIDTH          352
#define CIF_HEIGHT         288
// QCIF
#define QCIF_WIDTH         176
#define QCIF_HEIGHT        144
// Square-pixel CCIR-601 format, digitally-encoded NTSC resolution
#define SP601_NTSC_WIDTH   640
#define SP601_NTSC_HEIGHT  480
// Square-pixel CCIR-601 format, digitally-encoded PAL resolution
#define SP601_PAL_WIDTH    768
#define SP601_PAL_HEIGHT   576
// Square-pixel SIF format, digitally-encoded NTSC resolution
#define SPSIF_NTSC_WIDTH   320
#define SPSIF_NTSC_HEIGHT  240
// Square-pixel SIF format, digitally-encoded PAL resolution
#define SPSIF_PAL_WIDTH    384
#define SPSIF_PAL_HEIGHT   288

// Color subsampling options.
#define CF_422 0
#define CF_420 1
#define CF_CIF 2

typedef struct  {
	GWorldPtr gWorld;
	ImageSequence decomSeq;
} SGDataProcRefCon;

#define BailErr(x) {err = x; if(err != noErr) goto bail;}

/***************************************************************************/
// The video capture class.
class MacOSXGrabber : public Grabber {
public:
    // Constructor.
    MacOSXGrabber(const char* format, const char* dev);

    // Destructor.
    virtual ~MacOSXGrabber();

    // Minimum necessary functions.
    virtual void start();
    virtual void stop();
    virtual int grab();

    // Overridden functions.
    virtual int command(int argc, const char*const* argv);

protected:
    // sequence grabber support requires a callback function
    static OSErr seqGrabberDataProc(SGChannel, Ptr, long, long *, long, TimeValue, short, long);

    // Supplemental functions.
    void format();
    int setport(const char *port);
    int capture();
    int setupDecom(void);

    // The frame information.
    int decimate_;
    int width_, height_;

    int imageSize;

    // True if sequence grabbing is possible.
    Boolean quicktime_;

    // sequence grabber
    SeqGrabComponent	seqGrab;
    SGChannel		sgchanVideo;

    /*
     * A graphics world would be a nicer way to convert and resample the image.
     */
    ImageDescriptionHandle desc_;
    GWorldPtr gWorld_;
    Rect frameRect_;

    SGDataProcRefCon *sgDataProcRefCon;

    // The capture format.
    int format_;

    // Name of the video capture device
    const char *dev_;

    // the capture port
    char port_[64];

    // Video input standard - ntscIn, palIn or secamIn
    short input_standard_;

    // Use SequenceGrabber config dialog box
    bool useconfig_;

    // A simple exception handler class.
    class Exception {
	public:
        Exception(char* s) {
            string = s;
        }

        char* string;
    };
};

MacOSXGrabber::MacOSXGrabber(const char* format, const char* dev) {
    OSErr err = noErr;
    quicktime_ = true;
    decimate_ = 0;
    gWorld_ = NULL;
    seqGrab = NULL;
    dev_ = dev;
    input_standard_ = ntscIn;
    port_[0] = 0;
    useconfig_ = false;

	// time measurement
	grabber_ts_off_ = grabber_now();

     // allocate prezeroed nonrelocatable block of memory
	sgDataProcRefCon = (SGDataProcRefCon *)NewPtrClear(sizeof(SGDataProcRefCon));

    InitCursor();
    EnterMovies();

    try {
		// Remember the format.
		if (strcmp(format, "422") == 0) format_ = CF_422;
		else if (strcmp(format, "420") == 0) format_ = CF_420;
		else if (strcmp(format, "cif") == 0) format_ = CF_CIF;
		else throw Exception("Unrecognized format");
    } catch (Exception e) {
		quicktime_ = false;
		fprintf(stderr, "QuickTime error[%d]: %s\n", err, e.string);
    }
}

MacOSXGrabber::~MacOSXGrabber() {
    /*
     * Graphics worlds must be disposed of if used.
     */
    // Dipose of the GWorld.
    if (gWorld_ != NULL) DisposeGWorld(gWorld_);

    // Close the video digitizer component.
    CloseComponent(seqGrab);

	// Dispose of nonrelocatable block of memory
	if (sgDataProcRefCon != NULL) DisposePtr((char *)sgDataProcRefCon);
}

void MacOSXGrabber::start() {
    OSErr	err = noErr;

    // Don't do anything if QuickTime failed.
    if (!quicktime_) return;

	// time measurement
	target_->offset_ = grabber_ts_off_;

	format();
	
    // start the sequencer
    err = SGStartRecord(seqGrab);

    // Start capturing.
    Grabber::start();
}

void MacOSXGrabber::stop() {
    // Don't do anything if QuickTime failed.
    if (!quicktime_) return;

    // stop the sequencer
    SGStop(seqGrab);

    // Stop capturing.
    Grabber::stop();
}

int MacOSXGrabber::grab() {
	// time measurement------------------*
	start_grab_ = grabber_now() - grabber_ts_off_;
	fprintf(stderr, "start_grab\tnow: %f\n", start_grab_);
	//-----------------------------------*

    if (sgDataProcRefCon->decomSeq == 0) {
		if (setupDecom()) {
			return 0;
		}
    }
    if (capture() == 0) return 0;
    suppress(frame_);
    saveblks(frame_);
    YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_, format_);

	// time measurement------------------*
	end_grab_ = grabber_now() - grabber_ts_off_;
    fprintf(stderr, "end_grab\tnow: %f\n", end_grab_);
    fprintf(stderr, "num: %f\tgrab_time: %f\n",
        end_grab_, end_grab_ - start_grab_);
	//-----------------------------------*

    if(target_)
       return(target_->consume(&f));
    return 0;
}

int MacOSXGrabber::capture() {
    OSErr err = noErr;
    Ptr theData;
    long dataSize;

	/*
	 * Needed if decompressing to a graphics world.
	 */
    GWorldPtr currentGWorld;
    GDHandle currentGDevice;

    try {
		/*
		 * this does the work of grabbing from the camera
		 */
        err = SGIdle(seqGrab);

        // Save the current graphics port.
        GetGWorld(&currentGWorld, &currentGDevice);
        SetGWorld(gWorld_, NULL);

		theData = GetPixBaseAddr(GetGWorldPixMap(gWorld_));
		dataSize = imageSize;

        // Restore the graphics port.
        SetGWorld(currentGWorld, currentGDevice);

        // Copy the frame.
		/*
		 * i'd like to be able to use Quicktime to do the colour space
		 * conversions here, or remove the need for them totally!
		 */
        switch (format_) {
            case CF_422:
                packedYUYV422_to_planarYUYV422((char *)frame_, outw_, outh_, theData, inw_, inh_);
                break;
            case CF_420:
            case CF_CIF:
                packedYUYV422_to_planarYUYV420((char *)frame_, outw_, outh_, theData, inw_, inh_);
                break;
        }

    } catch (Exception e) {
        fprintf(stderr, "QuickTime error[%d]: %s\n", err, e.string);
        return 0;
    }

    // Return success.
    return 1;
}

void MacOSXGrabber::format() {
    OSErr err = noErr;

    // Set the image size.
    switch (decimate_) {
		case 1: // full-sized
			width_ = SP601_NTSC_WIDTH;
			height_ = SP601_NTSC_HEIGHT;
			break;
		case 2: // CIF-sized
			width_ = CIF_WIDTH;
			height_ = CIF_HEIGHT;
			break;
		case 4: // QCIF - may not work
			width_ = QCIF_WIDTH;
			height_ = QCIF_HEIGHT;
			break;
    }
    switch (format_) {
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

    try {
        // Set the frame rect.
        SetRect(&frameRect_, 0, 0, width_, height_);

        if (gWorld_ == NULL) {
            // Create or update an offscreen GWorld for the frame pixel map.
            err = QTNewGWorld(&gWorld_, kYUVSPixelFormat, &frameRect_, 0, NULL, 0);
            if (err != noErr) throw Exception("QTNewGWorld");
			sgDataProcRefCon->gWorld = gWorld_;
            if (!LockPixels(GetGWorldPixMap(gWorld_))) throw Exception("LockPixels");

            /*
             * ref: MakeSequenceGrabber()
			 * need better error checking here
			 */
			seqGrab = OpenDefaultComponent(SeqGrabComponentType, 0);
			if (seqGrab != NULL)
				err = SGInitialize(seqGrab);
            if (err == noErr)
				err = SGSetGWorld(seqGrab, NULL, NULL);
			if (err == noErr)
				err = SGSetDataRef(seqGrab, 0, 0, seqGrabDontMakeMovie);

			/* ref: MakeSequenceGrabChannel */
			err = SGNewChannel(seqGrab, VideoMediaType, &sgchanVideo);
			if (err == noErr) {
				Str63 devPStr;
				c2pstrcpy(devPStr, dev_);

				SGSetChannelDevice(sgchanVideo, (StringPtr)&devPStr);

				setport(port_);

				VideoDigitizerComponent vdigComponent = SGGetVideoDigitizerComponent(sgchanVideo);
				DigitizerInfo digitizerInfo;
				err = VDGetDigitizerInfo(vdigComponent, &digitizerInfo);
				if (err == noErr) {
					if ((digitizerInfo.inputCapabilityFlags & digiInDoesNTSC) && (input_standard_ == ntscIn)) {
						err = VDSetInputStandard(vdigComponent, ntscIn);
					} else if ((digitizerInfo.inputCapabilityFlags & digiInDoesPAL) && (input_standard_ == palIn)) {
						err = VDSetInputStandard(vdigComponent, palIn);
					} else if ((digitizerInfo.inputCapabilityFlags & digiInDoesSECAM) && (input_standard_ == secamIn)) {
						err = VDSetInputStandard(vdigComponent, secamIn);
					}
				}

                /* set sg to gworld */
                err = SGSetGWorld(seqGrab, gWorld_, GetMainDevice());

				err = SGSetChannelBounds(sgchanVideo, (const Rect *)&frameRect_);

				if (err == noErr)
					err = SGSetChannelUsage(sgchanVideo, seqGrabRecord);
				if (err != noErr) {
					SGDisposeChannel(seqGrab, sgchanVideo);
					sgchanVideo = NULL;
				}
			}
			err = SGSetDataProc(seqGrab, NewSGDataUPP(&seqGrabberDataProc), (long)sgDataProcRefCon);
			err = SGPrepare(seqGrab, false, true);

        } else {
			fprintf(stderr, "      format() - reformat not implemented!\n");
			UnlockPixels(GetGWorldPixMap(gWorld_));
			UpdateGWorld(&gWorld_, kYUVUPixelFormat, &frameRect_, 0, NULL, 0);
			if (!LockPixels(GetGWorldPixMap(gWorld_))) throw Exception("LockPixels");
        }
    } catch (Exception e) {
        fprintf(stderr, "QuickTime error[%d]: %s\n", err, e.string);
    }
    // Allocate the reference buffer.
    allocref();
}

int MacOSXGrabber::setport(const char *port) {
	OSErr err = noErr;
	int inputNumber = 0;

	if (strcmp(port_, port) == 0) {
		return 0;
	}

	SGDeviceList sgdeviceList;
	err = SGGetChannelDeviceList(sgchanVideo, sgDeviceListIncludeInputs, &sgdeviceList);
	if (err != noErr) {
		return TCL_ERROR;
	}
	char port_str[64];
	int selectedIndex = (*sgdeviceList)->selectedIndex;
	for (int i = 0; i < (*(((*sgdeviceList)->entry[selectedIndex]).inputs))->count; i++) {
		p2cstrcpy(port_str, (*(((*sgdeviceList)->entry[selectedIndex]).inputs))->entry[i].name);
		if (strcmp(port, port_str) == 0) {
			inputNumber = i;
		}
	}
	SGDisposeDeviceList(seqGrab, sgdeviceList);

	if (running_) {
		stop();
	}

	err = SGSetChannelDeviceInput(sgchanVideo, inputNumber);
	if (err != noErr) {
		strcpy((char*)port, port_);
		return TCL_ERROR;
	}
}

int MacOSXGrabber::command(int argc, const char*const* argv) {
    //fprintf(stderr,"### MacOSXGrabber::command : argv[1] = %s\n", argv[1]);
    if (argc == 3) {
		if (strcmp(argv[1], "decimate") == 0) {
			decimate_ = atoi(argv[2]);
			if (running_) {
				stop(); start();
			}
			return(TCL_OK);

		} else if (strcmp(argv[1], "port") == 0) {
			if (seqGrab == NULL) {
				strcpy((char *)argv[2], port_);
			} else {
				setport(argv[2]);
				start();
				return(TCL_OK);
			}
        } else if (strcmp(argv[1], "type") == 0) {
			if (strcmp(argv[2], "pal") == 0)
				input_standard_ = palIn;
			else if (strcmp(argv[2], "secam") == 0)
				input_standard_ = secamIn;
			else
				input_standard_ = ntscIn;

			if(running_) {
				stop(); start();
			}

			return(TCL_OK);

        } else if (strcmp(argv[1], "useconfig") == 0) {
			if (strcmp(argv[2], "1") == 0) useconfig_ = true;
			if (strcmp(argv[2], "0") == 0) useconfig_ = false;

        }  else if (strcmp(argv[1], "send") == 0) {
			if (useconfig_ && running_ == 0) {
				Component *panelListPtr = NULL;
				UInt8 numberOfPanels = 0;

				ComponentDescription cd = { SeqGrabPanelType, VideoMediaType, 0, 0, 0 };
				Component	 c = 0;
				Component *cPtr = NULL;

				// set up the settings panel list removing the "Compression" and "Source" panels
				numberOfPanels = CountComponents(&cd);
				if (numberOfPanels == 0)
					return TCL_ERROR;

				panelListPtr = (Component *)NewPtr(sizeof(Component) * (numberOfPanels + 1));
				if (MemError() || panelListPtr == NULL)
					return TCL_ERROR;

				cPtr = panelListPtr;
				numberOfPanels = 0;
				char panel_name[255];
				do {
					ComponentDescription compInfo;
					c = FindNextComponent(c, &cd);
					if (c) {
						Handle hName = NewHandle(0);
						if (MemError() || hName == NULL)
							return TCL_ERROR;
						GetComponentInfo(c, &compInfo, hName, NULL, NULL);
						p2cstrcpy(panel_name, (unsigned char*)(*hName));
						if (strcmp(panel_name, "Compression") != 0 && strcmp(panel_name, "Source") != 0) {
							*cPtr++ = c;
							numberOfPanels++;
						}
						DisposeHandle(hName);
					}
				} while (c);

				SGSettingsDialog(seqGrab, sgchanVideo, numberOfPanels, panelListPtr, 0, NULL, 0);
			}
        }

    } else if (argc == 2) {
		if (strcmp(argv[1], "decimate") == 0) {
			Tcl& tcl = Tcl::instance();
			tcl.resultf("%d", decimate_);
			return (TCL_OK);
		}
    }
    return Grabber::command(argc, argv);
}

int MacOSXGrabber::setupDecom(void) {
	ComponentResult	err = noErr;
	Rect sourceRect = { 0, 0 };
	MatrixRecord scaleMatrix;
	ImageDescriptionHandle imageDesc = (ImageDescriptionHandle)NewHandle(0);

	err = SGGetChannelSampleDescription(sgchanVideo, (Handle)imageDesc);
	BailErr(err);

	sourceRect.right = (**imageDesc).width;
	sourceRect.bottom = (**imageDesc).height;
	RectMatrix(&scaleMatrix, &sourceRect, &frameRect_);

	err = DecompressSequenceBegin(&(sgDataProcRefCon->decomSeq),
								  imageDesc,
								  sgDataProcRefCon->gWorld,
								  NULL,
								  NULL,
								  &scaleMatrix,
								  srcCopy,
								  NULL,
								  0,
								  codecNormalQuality,
								  bestSpeedCodec);
	BailErr(err);

	imageSize = ((*imageDesc)->width * (*imageDesc)->height);

	DisposeHandle ((Handle)imageDesc);
	imageDesc = NULL;

bail:
		if (imageDesc)
			DisposeHandle((Handle)imageDesc);
	return(err);
}

/*
 * video sequence data proc
 */
OSErr MacOSXGrabber::seqGrabberDataProc(SGChannel c, Ptr p, long len, long *offset, long chRefCon, TimeValue time, short writeType, long refCon) {

#pragma unused (c, offset, chRefCon, time, writeType)

	ComponentResult	err = noErr;
	CodecFlags ignore;

	SGDataProcRefCon *sgDataProcRefCon = (SGDataProcRefCon *)refCon;

	if (!sgDataProcRefCon->gWorld) return(err);
	err = DecompressSequenceFrameS(sgDataProcRefCon->decomSeq,
								   p,
								   len,
								   0,
								   &ignore,
								   NULL);
	return(err);
}

/***************************************************************************/
// The hybrid class providing Tcl with access to the video capture device.
class MacOSXDevice : public InputDevice {
public:
    MacOSXDevice(const char* device, char *attr);
    virtual int command(int argc, const char*const* argv);
};

MacOSXDevice::MacOSXDevice(const char* device, char *attr) : InputDevice(device) {
	attributes_ = attr;
    fprintf(stderr,"OSX Capture:  ==> %s\n", attr);
}

int MacOSXDevice::command(int argc, const char*const* argv)
{
	//fprintf(stderr,"### MacOSXDevice::command : argv[1] = %s\n", argv[1]);
    Tcl& tcl = Tcl::instance();
    if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			TclObject* o = 0;
			o = new MacOSXGrabber(argv[2], nickname_);
			if (o != 0)
				tcl.result(o->name());
			return(TCL_OK);
		}
    }
    return(InputDevice::command(argc, argv));
}


class MacOSXScanner {
public:
    MacOSXScanner();
};

// Instantiate the video capture device.
static MacOSXScanner find_capture_devices;

MacOSXScanner::MacOSXScanner()
{
	char *attr;
	SeqGrabComponent seqGrab;
	SGChannel sgchanVideo;
	SGDeviceList sgdeviceList;
	OSErr err = noErr;

	char *myDev = getenv("VIC_DEVICE");
	if (myDev != 0) {

	}
	seqGrab = OpenDefaultComponent(SeqGrabComponentType, 0);
	if (seqGrab != NULL)
		err = SGInitialize(seqGrab);
	if (err == noErr)
		err = SGSetGWorld(seqGrab, NULL, NULL);
	if (err == noErr)
		err = SGSetDataRef(seqGrab, 0, 0, seqGrabDontMakeMovie);
	err = SGNewChannel(seqGrab, VideoMediaType, &sgchanVideo);
	if (err != noErr) {
		// clean up on failure
		SGDisposeChannel(seqGrab, sgchanVideo);
		return;
	}
	if (err == noErr) {
		SGGetChannelDeviceList(sgchanVideo, sgDeviceListIncludeInputs, &sgdeviceList);
		char dev_str[64];
		char port_str[64];
		for (int i = 0; i < (*sgdeviceList)->count; i++) {
			// ignore devices that are currently unavailable
			p2cstrcpy(dev_str, (*sgdeviceList)->entry[i].name);
			if ((sgDeviceNameFlagDeviceUnavailable & (*sgdeviceList)->entry[i].flags) == 0) {
				attr = new char[512];
				strcpy(attr, "size {large small cif} format {420 422 cif} type {ntsc pal secam} port {");
				if ((*sgdeviceList)->entry[i].inputs != NULL) {
					fprintf(stderr,"OSX Capture: \"%s\" has %i input(s)\n", dev_str, (*(((*sgdeviceList)->entry[i]).inputs))->count);
					for (int j = 0; j < (*(((*sgdeviceList)->entry[i]).inputs))->count; j++) {
						p2cstrcpy(port_str, (*(((*sgdeviceList)->entry[i]).inputs))->entry[j].name);
						if (strchr(port_str, ' ')) {
							strcat(attr, "\"");
							strcat(attr, port_str);
							strcat(attr, "\" ");
						} else {
							strcat(attr, port_str);
							strcat(attr, " ");
						}
					}
				} else {
					fprintf(stderr,"OSX Capture: \"%s\" has no inputs\n", dev_str);
				}
				strcat(attr, "}");

				char *device = new char[strlen(dev_str) + 1];
				strcpy(device, dev_str);
				new MacOSXDevice(device, attr);
				//delete [] device;
				//delete [] attr;
			} else {
				fprintf(stderr,"OSX Capture: \"%s\" unavailable, ignoring\n", dev_str);
			}
		}

	}

	SGDisposeDeviceList(seqGrab, sgdeviceList);
	SGDisposeChannel(seqGrab, sgchanVideo);
	CloseComponent(seqGrab);
}


#endif // !__LP64__

#endif // !__LP64__

