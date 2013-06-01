/*
 * Copyright (c) 1995 The Regents of the University of California.
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
 */

#include "module.h"
#include "vic_tcl.h"
#include "renderer.h"
#include "device-input.h"
#include "device-output.h"
#include "assistor-list.h"
#include "grabber.h"
#include "vw.h"
#include "crdef.h"
#include "iohandler.h"
#include "color.h"

struct deviceattributes {
	char	*name;		//Solaris device/package name
	char	*nickname;	//nickname
	char	*devname;	//device name including %d
	int	capabilities;	//XILCAP_xxxx
	int	ports;
};

#define	XILCAP_HWGRAB_H261		(1<<0)	// H261 grabbable
#define	XILCAP_HWGRAB_CELLB		(1<<1)	// CellB grabbable
#define	XILCAP_ATTR_BRIGHTNESS		(1<<2)	// xildev attribute brightness
#define	XILCAP_ATTR_HUE			(1<<3)	// xildev attribute hue
#define	XILCAP_ATTR_CHROMA_GAIN_U	(1<<4)	// xildev attribute chroma u
#define	XILCAP_ATTR_CHROMA_GAIN_V	(1<<5)	// xildev attribute chroma v
#define	XILCAP_ATTR_CONTRAST		(1<<6)	// xildev attribute contrast
#define	XILCAP_ATTR_LUMA_BRIGHTNESS	(1<<7)	// xildev attribute luma brightness
#define	XILCAP_ATTR_LUMA_CONTRAST	(1<<8)	// xildev attribute luma contrast
#define	XILCAP_ATTR_CHROMA_GAIN		(1<<9)	// xildev attribute chroma gain
#define	XILCAP_ATTR_CHROMA_SATURATION	(1<<10)	// xildev attribute chroma sat
#define XILCAP_OUTPUT			(1<<11)	// can output video
#define	XILCAP_OUT_H261			(1<<12)	// can output h261 to vidout
#define	XILCAP_OUT_JPEG			(1<<13)	// can output jpeg to vidout

#define XILPORT_IN_COMPOSITE_1		(1<<1)	// have out composite 1
#define XILPORT_IN_COMPOSITE_2		(1<<2)	// have out composite 2
#define XILPORT_IN_S_VIDEO		(1<<3)	// have in  s-video port
#define XILPORT_OUT_COMPOSITE_1		(1<<4)	// have out composite 1
// JFIF start header.
const u_char jfif_header1_[] = {
0xff,0xd8,		/* SOI */
0xff,0xc4,0x00,0x1f,	/* DHT */
	0x00,0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,
0xff,0xc4,0x00,0x1f,	/* DHT */
	0x01,0x00,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,
0xff,0xc4,0x00,0xb5,	/* DHT */
	0x10,0x00,0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,0x01,0x7d,0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,
0xff,0xc4,0x00,0xb5,	/* DHT */
	0x11,0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,0x02,0x77,0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,

0xff,0xdb,0x00,0x43,	/* DQT */
	0x00,0x1b,0x12,0x14,0x17,0x14,0x11,0x1b,0x17,0x16,0x17,0x1e,0x1c,0x1b,0x20,0x28,0x42,0x2b,0x28,0x25,0x25,0x28,0x51,0x3a,0x3d,0x30,0x42,0x60,0x55,0x65,0x64,0x5f,0x55,0x5d,0x5b,0x6a,0x78,0x99,0x81,0x6a,0x71,0x90,0x73,0x5b,0x5d,0x85,0xb5,0x86,0x90,0x9e,0xa3,0xab,0xad,0xab,0x67,0x80,0xbc,0xc9,0xba,0xa6,0xc7,0x99,0xa8,0xab,0xa4,
0xff,0xdb,0x00,0x43,	/* DQT */
	0x01,0x1c,0x1e,0x1e,0x28,0x23,0x28,0x4e,0x2b,0x2b,0x4e,0xa4,0x6e,0x5d,0x6e,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,0xa4,

0xff,0xc0,0x00,0x11,	/* SOF0 */
	0x08,		/* sample precision */
	0x01,0x20,	/* lines */
	0x01,0x80,	/* samples/line */
	0x03,		/* components in frame */
		0x00,		/* component ID */
		0x22,		/* 4 bit horizontal sampling , 4 vertical sampling*/
		0x00,		/* quantization table destination selector */

		0x01,		/* component ID */
		0x11,		/* 4 bit hor/4 bit vert */
		0x01,		/* quant table */

		0x02,		/* component ID */
		0x11,		/* 4 bit hor, 4 bit vert */
		0x01,		/* quant table */
0xff,0xda,0x00,0x0c,	/* SOS */
	0x03,0x00,0x00,0x01,0x11,0x02,0x11,0x00,0x3f,0x00
};
// JFIF continuation header.
const u_char jfif_header2_[] = {
0xff,0xd8,		/* SOI */
0xff,0xc0,0x00,0x11,	/* SOF0 */
	0x08,0x01,0x20,0x01,0x80,0x03,0x00,0x22,0x00,0x01,0x11,0x01,0x02,0x11,0x01,
0xff,0xda,0x00,0x0c,	/* SOS */
	0x03,0x00,0x00,0x01,0x11,0x02,0x11,0x00,0x3f,0x00
};
class XILWindowAssistor : public Renderer {
public:
	XILWindowAssistor(int type/*,const char *fn,struct deviceattributes *devattr*/);
	~XILWindowAssistor();
	virtual int command(int argc, const char*const* argv);
protected:
	virtual int	consume(const VideoFrame*);
	GC		gc_;
	XilSystemState	xil_;
	XilImage	displayimage_,ximage_,yimage_,imageYCC;
	XilCis		cis_;
	int		type_;
	u_int		cis_xsize,cis_ysize,cis_nbands;
	XilDataType	cis_datatype;
	XilImageType	outputtype;
	int		visual;
#define TRUECOLOR	1
#define PSEUDOCOLOR	2
	Colormap	x_cmap;
        XilLookup	colorcube;
        XilDitherMask	dmask;
        XilLookup	xil_cmap;
	int		sentfirstheader;
	u_int		decimation_,targetw_,targeth_,lastcisw,lastcish;
	u_char		jfif_header1[sizeof(jfif_header1_)];
	u_char		jfif_header2[sizeof(jfif_header2_)];

	void reset(int type,int q, int w, int h);
	int inq_,lastjpegtype_;
	int need_refresh_;
	static void handle_exposure_callback(ClientData, XEvent *);
	static void handle_exposure_idle(ClientData);
	void handle_exposure() { need_refresh_ = 1; }
	XilLookup create_cmap(Colormap *, int , XilIndexList *, XilLookup , XilLookup );
	void cell_install_cmap( Colormap x_cmap, XilLookup cmap, XilIndexList *ilist);
	VideoWindow* window_;
	/*
	char		*filename_;
	struct deviceattributes deviceattributes_;
	 */
};

static class XILWindowAssistorMatcher : public Matcher {
public:
	XILWindowAssistorMatcher() : Matcher("assistor") {
		extern class AssistorList assistorlist;

		assistorlist.register_assistor("xil","jpeg/420 jpeg/422 h261 cellb");
	}
	TclObject* match(const char* id) {
		if (strncmp(id,"xil/",4)) // direct XIL assistor request
			return (0);
		id=id+4; // skip over "xil/"
		//fprintf(stderr,"XILWindowAssistorMatcher::"__FUNCTION__"(%s)\n",id);
		if (!strcmp(id, "jpeg/422") || !strcmp(id, "jpeg/420"))
			return (new XILWindowAssistor(FT_HW|FT_JPEG/*,filename_,&deviceattributes_*/));
		if (!strcmp(id, "h.261") || !strcmp(id, "h261"))
			return (new XILWindowAssistor(FT_H261/*,filename_,&deviceattributes_*/));
		if (!strcmp(id, "cellb"))
			return (new XILWindowAssistor(FT_CELLB/*,filename_,&deviceattributes_*/));
		return (0);
	}
} assistor_xil;

class XILOutputAssistor : public Renderer {
public:
	XILOutputAssistor(int type,const char *fn,struct deviceattributes *devattr); 
	~XILOutputAssistor(); 
protected:
	virtual int command(int argc,const char*const* argv);
	void reset(int inq,int inw,int inh);
	void setq(int q);
	virtual int consume(const VideoFrame* vf);

	XilSystemState	xil_;
	XilImage	o1kimage,imageYCC;
	XilCis		cis_;
	XilDevice	device_;
	int		type_;
	int		inq_;		//jpeg.
	u_int		cis_xsize,cis_ysize,cis_nbands;
	XilDataType	cis_datatype;
	XilImageType	outputtype;
	int		visual;
	int		sentfirstheader;
	u_int		targetw_,targeth_,lastcisw,lastcish;
	u_char		jfif_header1[sizeof(jfif_header1_)];
	u_char		jfif_header2[sizeof(jfif_header2_)];

	char		*filename_;
	struct deviceattributes deviceattributes_;
};

/* output device for XIL */
class XILOutputDevice : public OutputDevice { 
 public:
 	XILOutputDevice(const char *name,const char *fn,struct deviceattributes *devattr);
	virtual int command(int argc, const char*const* argv);
 protected:
 	struct deviceattributes deviceattributes_;
	char	*filename_;
	char	*nickname_;
};


class XILGrabber : public Grabber, public IOHandler {
 protected:
	XILGrabber(char *filename,struct deviceattributes *devattr);
 public:
	virtual ~XILGrabber();
	virtual void start();
	virtual void stop();
	virtual int command(int argc, const char*const* argv);
	virtual void fps(int);
	inline int is_pal() { return (max_fps_ == 25); }
 protected:
	void dispatch(int mask);
	int attr(const char* name, int value);
	int attr(const char* name);
	const char* cattr(const char* name);
	virtual void setsize() {}
	void suppress(const u_char* devbuf, int istride);
	void update_grabber_panel();

	int port_;		/* XIL input port */
	XilSystemState xil_;
	XilImage source_;
	XilCis cis_;
	XilImage image_;
	double scale_;
	double nextframetime_;
	int fd_;		/* rtvc data fd */
	u_int decimate_;
	u_int basewidth_;
	u_int baseheight_;
	u_int max_fps_;		/* 25 (PAL) or 30 (NTSC) */
	u_int vformat_; 	/* Video Format ie PAL or NTSC */
	struct deviceattributes deviceattributes_;
	char *filename_;
	XilDataType datatype_;
	u_int nbands_;
};

class XILYuvGrabber : public XILGrabber {
 public:
	XILYuvGrabber(char *filename,struct deviceattributes *devattr);
	virtual ~XILYuvGrabber();
 protected:
	virtual int grab();
	virtual void setsize();
	void saveblk(const u_char* in, u_char* yp, u_char* up, u_char* vp,
		     int stride, int istride);
	void saveblks(const u_char* in, int istride);
};


class XILCIFGrabber : public XILYuvGrabber {
 public:
	XILCIFGrabber(char *devname,struct deviceattributes *devattr);
 protected:
	virtual int grab();
	virtual void setsize();
	void saveblk(const u_char* in, u_char* yp, u_char* up, u_char* vp,
		     int stride, int istride);
	void saveblks(const u_char* in, int istride);
};

class XIL420Grabber : public XILCIFGrabber {
    public:
	XIL420Grabber(char *filename,struct deviceattributes *devattr);
    protected:
	virtual void setsize(int xsize, int ysize);
};

class XILCodecGrabber : public XILGrabber {
 public:
	XILCodecGrabber(const char *type, char *filename,struct deviceattributes *devattr);
	virtual ~XILCodecGrabber();
 protected:
	void update_grabber_panel();
	int cattr(const char* name, int value);
	u_char* capture(int& length);
};

class XILCellBGrabber : public XILCodecGrabber {
 public:
	XILCellBGrabber(char *filename,struct deviceattributes *devattr);
	int grab();
	int command(int argc, const char*const* argv);
 protected:
};

class XILJpegGrabber : public XILCodecGrabber {
 public:
	XILJpegGrabber(char *filename,struct deviceattributes *devattr);
 protected:
	virtual int command(int argc, const char*const* argv);
	void setq(int q);
	int grab();
	int q_;
};

class XILp64Grabber : public XILCodecGrabber {
 public:
	XILp64Grabber(char *filename,struct deviceattributes *devattr);
 protected:
	virtual	int command(int argc, const char*const* argv);
	u_char*	capture(int& length);
	int	grab();
	void	setq(int arg);
	int	targetwidth_,targetheight_;
};

class XILDevice : public InputDevice {
 public:
	XILDevice(const char* s,const char*fn,const struct deviceattributes *devattr);
	virtual int command(int argc, const char*const* argv);
 protected:
 	struct deviceattributes deviceattributes_;
	char	*filename_;
	char	*nickname_;
};


class XILDevices {
 public:
	XILDevices();
 protected:
	XILDevice **xildevices_;
	int	nrofxildevices_;
};

#define XIL_ATTRIBUTES {						\
	{ "SUNWrtvc","SunVideo","/dev/rtvc%d",0,			\
		XILPORT_IN_COMPOSITE_1|XILPORT_IN_COMPOSITE_2|		\
		XILPORT_IN_S_VIDEO},					\
	{ "MMACo1k","Osprey 1k","/dev/o1k%d",				\
		XILCAP_HWGRAB_H261|XILCAP_ATTR_BRIGHTNESS|XILCAP_ATTR_HUE|\
		XILCAP_ATTR_CHROMA_GAIN_U|XILCAP_ATTR_CHROMA_GAIN_V|	\
		XILCAP_ATTR_CONTRAST|XILCAP_OUT_H261|XILCAP_OUTPUT,	\
		XILPORT_IN_COMPOSITE_1|XILPORT_IN_COMPOSITE_2|		\
		XILPORT_IN_S_VIDEO|XILPORT_OUT_COMPOSITE_1},		\
	{ "MMACslv","SlicVideo","/dev/slv%d",				\
		XILCAP_ATTR_HUE|XILCAP_ATTR_LUMA_BRIGHTNESS|		\
		XILCAP_ATTR_LUMA_CONTRAST|XILCAP_ATTR_CHROMA_GAIN|	\
		XILCAP_ATTR_CHROMA_SATURATION,				\
		XILPORT_IN_COMPOSITE_1|XILPORT_IN_COMPOSITE_2|		\
		XILPORT_IN_S_VIDEO},					\
	{ "SUNWvfc","VideoPix","/dev/vfc%d",0,				\
		XILPORT_IN_COMPOSITE_1|XILPORT_IN_COMPOSITE_2|		\
		XILPORT_IN_S_VIDEO},					\
	}
#define XIL_NROFDEVICES 4

extern void osprey_open();
