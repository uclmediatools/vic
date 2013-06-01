/*
 * Marcus Meissner, 1997
 */

#ifndef lint
/*
static char rcsid[] =
    "@(#) $Header$ (LBL)";
 */
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <xil/xil.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>


#include "vic_tcl.h"
#include "inet.h"
#include "rtp.h"
#include "decoder.h"
#include "vw.h"
#include "renderer.h"
#include "device-output.h"
#include "xil.h"

#define M_SOI 0xd8
#define M_APP0 0xe0
#define M_DQT 0xdb
#define M_SOF0 0xc0	/* marks BASELINE DCT */
#define M_DHT 0xc4
#define M_SOS 0xda
#define M_EOI 0xd9

class FILEOutputDevice : public OutputDevice { 
 public:
        FILEOutputDevice();
        virtual int command(int argc, const char*const* argv);
};

class FILEOutputAssistor : public Renderer {
public:
        FILEOutputAssistor(int type); 
        ~FILEOutputAssistor(); 
protected:
        virtual int command(int argc,const char*const* argv);
        void reset(int type, int inq,int inw,int inh);
        void setq(int q);
        virtual int consume(const VideoFrame* vf);
 
 	    int decimation_;
 	    int lastjpegtype_;
        int             type_;
        int             inq_;           //jpeg.
	int		xfd_;
        int             visual;
        int             sentfirstheader;
        u_int           targetw_,targeth_,lastcisw,lastcish;
        u_char          jfif_header1[sizeof(jfif_header1_)];
        u_char          jfif_header2[sizeof(jfif_header2_)];
 
        char            *filename_;
	int		sigalarmpid_,xtime_,lasttime_;
        struct deviceattributes deviceattributes_;
};

extern jpeg_luma_qt(int,int[]);
extern jpeg_chroma_qt(int,int[]);

static int
mapkoord2jfifentry[] = {
	0, 1,8, 16,9,2, 3,10,17,24, 32,25,18,11,4, 5,12,19,26,33,40,
	48,41,34,27,20,13,6, 7,14,21,28,35,42,49,56, 57,50,43,36,29,22,15,
	23,30,37,44,51,58, 59,52,45,38,31, 39,46,53,60, 61,54,47, 55,62, 63

};

FILEOutputAssistor::FILEOutputAssistor(int type) : Renderer(type) {
	char	*cisname;
	XilDevice	device;
	char	*interval,*sigalarm;

	interval = getenv("VIC_DROPINTERVAL");
	sigalarm = getenv("VIC_SIGALARM");
	if (!interval || !sscanf(interval,"%d",&xtime_))
		xtime_ = 15; // 15 seconds
	if (sigalarm)
		sigalarmpid_=atoi(sigalarm);
	lasttime_=0;
	cisname=NULL;
	switch (type) {
	case FT_YUV_420:
		break;
	case FT_JPEG|FT_HW:
		memcpy(jfif_header1,jfif_header1_,sizeof(jfif_header1_));
		memcpy(jfif_header2,jfif_header2_,sizeof(jfif_header2_));
		break;
	case FT_H261:
		cisname = "H261";
		break;
	case FT_CELLB:
		cisname = "CellB";
		break;
	default:
		fprintf(stderr,"	unknown type %d???\n",type&~FT_HW);
		break;
	}
	type_ = type;
	xfd_ = open("/tmp/vic.out",O_WRONLY|O_CREAT,0644);
	if (xfd_==-1)
		perror("open /tmp/vic.out");
}

FILEOutputAssistor::~FILEOutputAssistor() {
	close(xfd_);
}

void FILEOutputAssistor::setq(int q)
{
        int	qt[8][8];
	XilJpegQTable	xq;

	inq_ = q;
	sentfirstheader=0;

	jpeg_luma_qt(q, (int*)qt);
	xq.table = 0;
	xq.value = qt;
	//(void)xil_cis_set_attribute(cis_, "QUANTIZATION_TABLE", (void*)&xq);
	jpeg_chroma_qt(q, (int*)qt);
	xq.table = 1;
	xq.value = qt;
	//(void)xil_cis_set_attribute(cis_, "QUANTIZATION_TABLE", (void*)&xq);
}
 
void FILEOutputAssistor::reset(int type, int q, int w, int h)
{
        int		qt[64];
	unsigned int		i,j;

	// blblbl.
	//fprintf(stderr,"XILWindowAssistor::"__FUNCTION__"\n");
	Module::size(w, h);

	inq_ = q;
	sentfirstheader=0;
	lastjpegtype_ = type;
	switch (type) {
	case 1: /* 420 */
		decimation_ = 420;
		break;
	default:
		decimation_ = 422;
		break;
	}
	for (i=0;i<sizeof(jfif_header1_);i++) {
		if (jfif_header1[i]!=0xff)
			continue;
		i++;if (i==sizeof(jfif_header1)) break;
		switch (jfif_header1[i]) {
		case M_DQT:
			//well the size is correct. we have defined the table
			i+=3;
			switch (jfif_header1[i]) {
			case 0:/* LUMA */
				i++;
				jpeg_luma_qt(q,qt);
				for (j=0;j<64;j++)
					jfif_header1[i+j]=qt[mapkoord2jfifentry[j]];
				break;
			case 1:/* CHROMA */
				i++;
				jpeg_chroma_qt(q,qt);
				for (j=0;j<64;j++)
					jfif_header1[i+j]=qt[mapkoord2jfifentry[j]];
				break;
			}
			break;
		case M_SOF0:
			i+=3;
			i++;	/* sample precision */
			jfif_header1[i++] = h>>8;
			jfif_header1[i++] = h&0xff;
			jfif_header1[i++] = w>>8;
			jfif_header1[i++] = w&0xff;
			i++; 	/* components (3) */
				i++;	/* comp id 0 */
				if (decimation_ == 420)
					jfif_header1[i++] = 0x22;/* 0x22 for 2 hor/2 vert */
				else
					jfif_header1[i++] = 0x21;/* 0x21 for 2 hor/1 vert */
				i++;	/* quant dest */

				i++;	/* comp id 1 */
				i++;	/* hor/ver quant 1:1 */
				i++;	/* quant dest */

				i++;	/* comp id 2 */
				i++;	/* hor/ver quant 1:1 */
				i++;	/* quant dest */
			break;
		default:
			//fprintf(stderr,"blocktype %02x\n",jfif_header1[i]);
			break;
		}
	}
	for (i=0;i<sizeof(jfif_header2);i++) {
		if (jfif_header2[i]!=0xff)
			continue;
		i++;if (i==sizeof(jfif_header2)) break;
		switch (jfif_header2[i]) {
		case M_SOF0:
			i+=3;
			i++;	/* sample precision */
			jfif_header2[i++] = h>>8;
			jfif_header2[i++] = h&0xff;
			jfif_header2[i++] = w>>8;
			jfif_header2[i++] = w&0xff;
			i++; 	/* components (3) */
				i++;	/* comp id 0 */
				if (decimation_ == 420)
					jfif_header2[i++] = 0x22;/* 0x22 for 2 hor/2 vert */
				else
					jfif_header2[i++] = 0x21;/* 0x21 for 2 hor/1 vert */
				i++;	/* quant dest */

				i++;	/* comp id 1 */
				i++;	/* hor/ver quant 1:1 */
				i++;	/* quant dest */

				i++;	/* comp id 2 */
				i++;	/* hor/ver quant 1:1 */
				i++;	/* quant dest */
			break;
		default:
			break;
		}
	}
}

int FILEOutputAssistor::command(int argc, const char*const* argv) {
/*
	fprintf(stderr,"FILEOutputAsssitor()::"__FUNCTION__"(");
	for (int i=1;i<argc;i++)
		fprintf(stderr,"%s,",argv[i]);
	fprintf(stderr,")\n");
*/
	if (argc == 3 ) {
		if (	strcmp(argv[1],"scale") == 0) {
			/* ignore !?! */
			return (TCL_OK);
		}
	}
	return Renderer::command(argc,argv);
}

int FILEOutputAssistor::consume(const VideoFrame* vf) {
	int	res,len=0,dofree=0;
	u_char	*bp,*p=NULL;

	//fprintf(stderr,"FILEOutputAssistor()::"__FUNCTION__"\n");
	if (time(NULL)<lasttime_+xtime_)
		return 0;
	lasttime_ = time(NULL);
	switch (type_) {
	case FT_JPEG|FT_HW: {
		JpegFrame* jf = (JpegFrame*)vf;
		if (	(jf->q_ != inq_) ||
			(jf->width_ != width_) ||
			(jf->height_ != height_)
		)
			reset(jf->type_, jf->q_, jf->width_, jf->height_);
		len=jf->len_;
		bp=jf->bp_;
		dofree=1;
		// kill marcus
		if (sentfirstheader) {
			p = new u_char[sizeof(jfif_header2)+len+2];
			memcpy(p,jfif_header2,sizeof(jfif_header2));
			memcpy(p+sizeof(jfif_header2),bp,len);
			// kill me
			memcpy(p+sizeof(jfif_header2)+len,"\0xff\0xd9",2);
			len+=sizeof(jfif_header2)+2;
		} else {
			p = new u_char [sizeof(jfif_header1)+len+2];
			memcpy(p,jfif_header1,sizeof(jfif_header1));
			memcpy(p+sizeof(jfif_header1),bp,len);
			// kill me
			memcpy(p+sizeof(jfif_header1)+len,"\0xff\0xd9",2);
			len+=sizeof(jfif_header1)+2;
			sentfirstheader=1;
		}
		break;
	}
	case FT_CELLB: {
		CellBFrame* hf = (CellBFrame*) vf;
		len=hf->len_;
		p=hf->bp_;
//		xil_cis_set_attribute(cis_, "WIDTH", (void *)hf->width_);
//		xil_cis_set_attribute(cis_, "HEIGHT", (void *)hf->height_);
		break;
	}
	case FT_YUV_420:
		p=vf->bp_;
		len=vf->width_*vf->height_*6/4;
		break;
	case FT_H261: {
		H261Frame* hf = (H261Frame*) vf;
		len=hf->len_;
		p=hf->bp_;
		while ((p[0]==0)&&(p[1]==0)) {
			p++;len--;
		}
		break;
	}
	}
	res=write(xfd_,p,len);
	if (res==-1)
		perror("FILEOutputAssistor::write");
	if (res<=0) {
		//hmmm...
	}
	if (sigalarmpid_)
		kill(sigalarmpid_,SIGALRM);
	return (0);
} 

FILEOutputDevice fileoutputdevice;

FILEOutputDevice::FILEOutputDevice() : OutputDevice("file")
{
}

int FILEOutputDevice::command(int argc, const char*const* argv) {
	Tcl& tcl = Tcl::instance();
/*
	fprintf(stderr,"FILEOutputDevice::command(");
	for (int i=1;i<argc;i++)
		fprintf(stderr,"%s,",argv[i]);
	fprintf(stderr,")\n");
*/
	if (argc == 3 ) {
		if (!strcmp(argv[1],"renderer")) {
			if (!strcmp(argv[2],"420")) {
				FILEOutputAssistor* p = new FILEOutputAssistor(FT_YUV_420);
				tcl.result(p->name());
				return (TCL_OK);
			}
		}
		if ( !strcmp(argv[1],"assistor")) {
#if 0
			if (!strcmp(argv[2],"cellb")) {
				FILEOutputAssistor* p = new FILEOutputAssistor(FT_CELLB,filename_,&deviceattributes_);
				tcl.result(p->name());
				return (TCL_OK);
			}
#endif
			if (!strcmp(argv[2],"h261")) {
				FILEOutputAssistor* p = new FILEOutputAssistor(FT_H261);
				tcl.result(p->name());
				return (TCL_OK);
			}
			if (!strcmp(argv[2],"jpeg/420") || !strcmp(argv[2],"jpeg/422")) {
				//FILEOutputAssistor* p = new FILEOutputAssistor(FT_JPEG|FT_HW,filename_,&deviceattributes_);
				FILEOutputAssistor* p = new FILEOutputAssistor(FT_JPEG|FT_HW);
				tcl.result(p->name());
				return (TCL_OK);
			}
		}
	}
	return OutputDevice::command(argc,argv);
}
