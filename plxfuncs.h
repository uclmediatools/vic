#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
extern "C" {
#include <X11/extensions/XShm.h>
#include <XPlxExt.h>
}

extern "C" {
	plx_IO *XPlxQueryConfig(Display *dpy, Drawable d, GC gc);
	plx_signal *XPlxQueryVideo(Display *dpy, Drawable d, GC gc);
	void XPlxVideoInputSelect(Display *dpy, Drawable d, GC gc, 
		int channel, int standard, int format, int type);
	void XPlxVideoSqueezeLive(Display *dpy, Drawable d, GC gc,
		int vx, int vy, int vw, int vh,
		int x, int y, int w, int h);
	void XPlxVideoSqueezeStill(Display *dpy, Drawable d, GC gc,
		int vx, int vy, int vw, int vh,
		int x, int y, int w, int h);
	void XPlxVideoTag(Display *dpy, Drawable, GC gc, int type);
	void XPlxVideoValueLoad(Display *dpy, Drawable d, GC gc,
		int sat, int cont, int hue, int brit);
	void XPlxVideoValueQuery(Display *dpy, Drawable d, GC gc,
		int *sat, int *cont, int *hue, int *brit);
	int MakeQTables(unsigned int factor, unsigned char **pqtable);
	XPlxCImage *XPlxCreateCImage(Display *dpy, char *data, int size,
		unsigned int width, unsigned int height);
	int XPlxDestroyCImage(XPlxCImage *ximage);
	XPlxCImage *XPlxGetCImage(Display *dpy, Drawable d, GC gc,
		int src_x, int src_y, unsigned int src_w, unsigned int src_h,
		unsigned int dst_w, unsigned int dst_h);
	void XPlxPutCImage(Display *dpy, Drawable d, GC gc, XPlxCImage *image,
		int src_x, int src_y, unsigned int src_w, unsigned int src_h,
		int dst_x, int dst_y, unsigned int dst_w, unsigned int dst_h,
		int flag);
#ifdef __hpux
	void XPlxPutTable(Display *dpy, Drawable d, GC gc,
		short *table, int size, Bool dir);
#else
	void XPlxPutTable(Display *dpy, Drawable d, GC gc,
		char *table, int size, Bool dir);
#endif
	XPlxCImage *XPlxShmCreateCImage(Display *dpy, char *data,
		XShmSegmentInfo *shminfo, int size,
		unsigned int width, unsigned int height);
	Status XPlxShmGetCImage(Display *dpy, Drawable d, GC gc,
		XPlxCImage *image, int src_x, int src_y,
		unsigned int src_w, unsigned int src_h);
	Status XPlxShmPutCImage(Display *dpy, Drawable d, GC gc,
		XPlxCImage *image, int src_x, int src_y,
		unsigned int src_w, unsigned int src_h,
		int dst_x, int dst_y, unsigned int dst_w, unsigned int dst_h,
		int flag, Bool send_event);
}
