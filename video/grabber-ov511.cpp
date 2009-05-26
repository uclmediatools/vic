/*
 * grabber-ov511.cpp. Copyright (C) 2001 FUJITSU LABORATORIES LTD.
 *
 * This program is mostly based on "vid" written by Peter S. Housel
 * (http://ovtvid-bsd.sourceforge.net/).
 * The "vid" program is released under GPL so this program should 
 * also be released under GPL.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
/* Copyright information of "vid" is as follows: */
/* Simple OV511 video capture program, version 1.0.1
 *
 * Copyright 2000 Peter S. Housel.
 *
 * Portions of this program were modeled after or adapted from the
 * OV511 driver for Linux by Mark W. McClelland; see
 * http://alpha.dyndns.org/ov511/ for more information.
 *
 * This program is free software; you can redistribute it and/or modify
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#define class usbclass
#include <dev/usb/usb.h>
#undef class

#if defined(__NetBSD__) && (__NetBSD_Version__ < 106000000)
#define udi_vendorNo	vendorNo
#define udi_productNo	productNo
#define ucr_request		request
#define ucr_data		data
#define ucr_flags		flags
#define ucr_actlen		actlen
#define uai_interface_index	interface_index
#define uai_alt_no		alt_no
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <errno.h>

#include "grabber.h"
#include "vic_tcl.h"
#include "device-input.h"
#include "module.h"
#include "ov511reg.h"

/*XXX*/
#define NTSC_WIDTH 320
#define NTSC_HEIGHT 240
#define PAL_WIDTH 384
#define PAL_HEIGHT 288
#define CIF_WIDTH 352
#define CIF_HEIGHT 288

#define OV511_VENDOR		0x05a9
#define OV511_PRODUCT_511	0x0511
#define OV511_PRODUCT_511PLUS	0xa511

#define OV511_MAX_CONTRAST 255

class OV511Grabber : public Grabber {
 public:
	OV511Grabber(const char* name, int is511plus);
	virtual ~OV511Grabber();
	virtual void start();
 protected:
	virtual int command(int argc, const char*const* argv);
	int capture();
	virtual int grab();
	void format();
	void normalize();
	virtual void setsize();
	void contrast (double c);

	int format_;		/* video format: NTSC or PAL */
	int rformat_;		/* requested format, above + AUTO */
	u_int basewidth_;
	u_int baseheight_;
	int decimate_;

private:
	int initDevice();
	int regRead(int reg);
	int regWrite(int reg, int val);
	int i2cRead(int reg);
	int i2cWrite(int reg, int val);
	int init76xx();
	int init6xx0();
	void procData();
	void procSegment(u_char *buf);
	int brightness_;
	int red_;
	int blue_;
	int contrast_;
	int changed_;
	int pktsize_;

	int port_;
	int ctlfd_;
	int fd_;
	int cameraid;

	int is511plus_;
	enum {
		OV511SENSOR_7610,
		OV511SENSOR_7620,
		OV511SENSOR_7620AE,
		OV511SENSOR_6620
	} sensortype_;

	enum {
		OV511FRAME_SKIPPING,
		OV511FRAME_READING,
		OV511FRAME_DONE
	} state_;
	int segsize_;
	int iY_, jY_;
	int iUV_, jUV_;
	u_char frmbuf[NTSC_WIDTH * NTSC_HEIGHT * 3 / 2];
	u_char buf[2048];
	int loopcnt_;
};

class CIFOV511Grabber : public OV511Grabber {
public:
	CIFOV511Grabber(const char* name, int is511plus)
		: OV511Grabber(name, is511plus) {}

protected:
	virtual void setsize();
	int loff_;
	int coff_;
	int hwrap_;
	int hskip_;
};

class OV511Device : public InputDevice {
 public:
	OV511Device(const char*);
	virtual int command(int argc, const char*const* argv);

	int is511plus_;
	char* devname_;
};

static OV511Device ov511_device("ov511");

static const char disabled_str[] = "disabled";
#if defined(__FreeBSD__)
static const char ugen0_00[] = "/dev/ugen0";
static const char ugen0_01[] = "/dev/ugen0.1";
#elif defined(__NetBSD__)
static const char ugen0_00[] = "/dev/ugen0.00";
static const char ugen0_01[] = "/dev/ugen0.01";
#endif

OV511Device::OV511Device(const char* name) : InputDevice(name)
{
	int fd;
	struct usb_device_info udi;

	fd = open(ugen0_00, O_RDWR);
	if (fd < 0) {
		perror(ugen0_00);
		attributes_ = disabled_str;
		return;
	}

	if (ioctl(fd, USB_GET_DEVICEINFO, &udi) < 0) {
		perror(ugen0_00);
		close(fd);
		attributes_ = disabled_str;
		return;
	}

	is511plus_ = 0;
	if (udi.udi_vendorNo == OV511_VENDOR) {
		switch (udi.udi_productNo) {
		case OV511_PRODUCT_511:
			break;
		case OV511_PRODUCT_511PLUS:
			is511plus_ = 1;
			break;
		default:
			close(fd);
			return;
		}
	}

	close(fd);
	attributes_ = "\
format {422 420 cif} \
size {small normal cif} \
port {default}";
}

int OV511Device::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();
	if (argc == 3) {
		if (strcmp(argv[1], "open") == 0) {
			TclObject* o = 0;
			if (strcmp(argv[2], "422") == 0)
				o = new OV511Grabber(ugen0_00, is511plus_);
			else if (strcmp(argv[2], "420") == 0)
				o = new CIFOV511Grabber(ugen0_00, is511plus_);
			else if (strcmp(argv[2], "cif") == 0)
				o = new CIFOV511Grabber(ugen0_00, is511plus_);
			if (o != 0)
				tcl.result(o->name());
			return (TCL_OK);
		}
	}
	return (InputDevice::command(argc, argv));
}

OV511Grabber::OV511Grabber(const char* name, int is511plus) :
	is511plus_(is511plus)
{
	ctlfd_ = open(name, O_RDWR);
	if (ctlfd_ < 0) {
		perror("open");
		exit(1);
	}

	if (initDevice() < 0) {
		exit(1);
	}

	red_ = 0xb0;
	blue_ = 0xd0;
	contrast_ = 0xb0;
	brightness_ = 0x70;
	changed_ = 0;

	(Tcl::instance()).evalf
		("if [info exists ov511window] {\n"
		 "    eval \"$ov511window(setred) %d\"\n"
		 "    eval \"$ov511window(setblue) %d\"\n"
		 "}\n",
		 red_, blue_);

	format_ = 0;
	decimate_ = 2;

	basewidth_ = NTSC_WIDTH * decimate_;
	baseheight_ = NTSC_HEIGHT * decimate_;
}

OV511Grabber::~OV511Grabber()
{
	close(ctlfd_);
	close(fd_);

}

void OV511Grabber::setsize()
{
	set_size_422(basewidth_ / decimate_, baseheight_ / decimate_);
	allocref();
}

void OV511Grabber::format()
{
	setsize();
}

/* XXX */
void OV511Grabber::normalize()
{
	fprintf(stderr, "normalize \n");
}

void OV511Grabber::start()
{
	format();
	Grabber::start();
}

int OV511Grabber::command(int argc, const char *const *argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 4) {
		if (strcmp (argv[1], "set") == 0) {
			changed_ = 1;
			if (strcmp (argv[2], "BRIGHT") == 0) {
				brightness_ = atoi(argv[3]);
				return (TCL_OK);
			} else if (strcmp (argv[2], "RED") == 0) {
				red_ = atoi(argv[3]);
				return (TCL_OK);
			} else if (strcmp (argv[2], "BLUE") == 0) {
				blue_ = atoi(argv[3]);
				return (TCL_OK);
			} else if (strcmp (argv[2], "CONT") == 0) {
				contrast_ = atoi(argv[3]);
				return (TCL_OK);
			} else {
				tcl.resultf("%s: unknown set command: %s",
					    argv[0], argv[2]);
				return (TCL_ERROR);
			}
		}
	} else if (argc == 3) {
		if (strcmp(argv[1], "get") == 0) {
			if (strcmp (argv[2], "BRIGHT") == 0) {
				tcl.resultf("%d", brightness_);
				return (TCL_OK);
			} else if (strcmp (argv[2], "CONT") == 0) {
				tcl.resultf ("%f", (double) contrast_ /
					     (double) OV511_MAX_CONTRAST);
				return (TCL_OK);
			}
		} else if (strcmp(argv[1], "decimate") == 0) {
			int dec = atoi(argv[2]);
			if (dec <= 0) {
				tcl.resultf("%s: divide by zero", argv[0]);
				return (TCL_ERROR);
			}
			if (dec != decimate_) {
				decimate_ = dec;
				setsize();
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "port") == 0) {
			int p = atoi(argv[2]);
			if (p != port_) {
				port_ = p;
			}
			return (TCL_OK);
		} else if (strcmp(argv[1], "format") == 0) {
			if (running_)
				format();
			return (TCL_OK);
		} else if (strcmp(argv[1], "contrast") == 0) {
			contrast(atof(argv[2]));
			return (TCL_OK);
		}
	} else if (argc == 2) {
		if (strcmp(argv[1], "normalize") == 0) {
			normalize();
			return (TCL_OK);
		} else if (strcmp(argv[1], "format") == 0) {
			Tcl& tcl = Tcl::instance();
			switch (format_) {

			default:
				tcl.result("");
				break;
			}
			return (TCL_OK);

		}
	}

	return (Grabber::command(argc, argv));
}

int OV511Grabber::grab()
{
	if (capture() == 0)
		return (0);

	suppress(frame_);
	saveblks(frame_);
	YuvFrame f(media_ts(), frame_, crvec_, outw_, outh_);
	return (target_->consume(&f));
}

void OV511Grabber::contrast (double c)
{
	contrast_ = (int) (OV511_MAX_CONTRAST * c);
}

int OV511Grabber::regRead(int reg)
{
	struct usb_ctl_request ur;
	unsigned char data[1024];

	ur.ucr_request.bmRequestType = UT_READ_VENDOR_INTERFACE;
	ur.ucr_request.bRequest = 2;

	USETW(ur.ucr_request.wValue, 0);
	USETW(ur.ucr_request.wIndex, reg);
	USETW(ur.ucr_request.wLength, 1);
	ur.ucr_data = data;
	ur.ucr_flags = 0;
	ur.ucr_actlen = 0;

	if (ioctl(ctlfd_, USB_DO_REQUEST, &ur) < 0) {
		return -1;
	}

	return data[0] & 0xFF;
}

int OV511Grabber::regWrite(int reg, int val)
{
	struct usb_ctl_request ur;
	unsigned char data[1024];

	data[0] = val;

	ur.ucr_request.bmRequestType = UT_WRITE_VENDOR_INTERFACE;
	ur.ucr_request.bRequest = 2;

	USETW(ur.ucr_request.wValue, 0);
	USETW(ur.ucr_request.wIndex, reg);
	USETW(ur.ucr_request.wLength, 1);
	ur.ucr_data = data;
	ur.ucr_flags = 0;
	ur.ucr_actlen = 0;

	if (ioctl(ctlfd_, USB_DO_REQUEST, &ur) < 0) {
		return -1;
	}

	return 0;
}

int OV511Grabber::i2cRead(int reg)
{
	int status = 0;
	int val = 0;
	int retries = OV7610_I2C_RETRIES;

	while (--retries >= 0) {
		/* wait until bus idle */
		do {
			if ((status = regRead(OV511_REG_I2C_CONTROL)) < 0)
				return -1;
		} while ((status & 0x01) == 0);

		/* perform a dummy write cycle to set the register */
		if (regWrite(OV511_REG_SMA, reg) < 0)
			return -1;

		/* initiate the dummy write */
		if(regWrite(OV511_REG_I2C_CONTROL, 0x03) < 0)
			return -1;

		/* wait until bus idle */
		do {
			if((status = regRead(OV511_REG_I2C_CONTROL)) < 0)
				return -1;
		} while ((status & 0x01) == 0);

		if ((status & 0x2) == 0)
			break;
	}

	if (retries < 0)
		return -1;

	retries = OV7610_I2C_RETRIES;
	while (--retries >= 0) {
		/* initiate read */
		if(regWrite(OV511_REG_I2C_CONTROL, 0x05) < 0)
			return -1;

		/* wait until bus idle */
		do {
			if((status = regRead(OV511_REG_I2C_CONTROL)) < 0)
				return -1;
		} while ((status & 0x01) == 0);

		if ((status & 0x2) == 0)
			break;

		/* abort I2C bus before retrying */
		if (regWrite(OV511_REG_I2C_CONTROL, 0x10) < 0)
			return -1;
	}
	if (retries < 0)
		return -1;

	/* retrieve data */
	val = regRead(OV511_REG_SDA);

	/* issue another read for some weird reason */
	if (regWrite(OV511_REG_I2C_CONTROL, 0x05) < 0)
		return -1;

	return val;
}

int OV511Grabber::i2cWrite(int reg, int val)
{
	int status = 0;
	int retries = OV7610_I2C_RETRIES;

	while (--retries >= 0) {
		if (regWrite(OV511_REG_SWA, reg) < 0)
			return -1;

		if (regWrite(OV511_REG_SDA, val) < 0)
			return -1;

		if (regWrite(OV511_REG_I2C_CONTROL, 0x1) < 0)
			return -1;

		/* wait until bus idle */
		do {
			if ((status = regRead(OV511_REG_I2C_CONTROL)) < 0)
				return -1;
		} while ((status & 0x01) == 0);

		/* OK if ACK */
		if ((status & 0x02) == 0)
			return 0;
	}

	return -1;
}

int OV511Grabber::init6xx0()
{
	/* XXX  Not implemented yet */
	return -1;
}

int OV511Grabber::init76xx()
{
	int val;

	if (regWrite(OV511_REG_SID, OV7610_I2C_WRITE_ID) < 0)
		return -1;

	if (regWrite(OV511_REG_SRA, OV7610_I2C_READ_ID) < 0)
		return -1;

	if (regWrite(OV511_REG_RST, 0x0) < 0)
		return -1;

	if (i2cWrite(OV7610_REG_COMA, 0x80) < 0)
		return -1;

	i2cRead(0x00);
	while (1) {
		if (i2cRead(OV7610_REG_MIDH) == 0x7f &&
		    i2cRead(OV7610_REG_MIDL) == 0xa2)
			break;
	}

	val = i2cRead(OV7610_REG_COMI);
	if (val < 0) {
		return -1;
	} else if ((val & 3) == 3) {
		sensortype_ = OV511SENSOR_7610;
	} else if ((val & 3) == 1) {
		/*
		 * XXX  Workaround for 511+/7620AE
		 * (from ov511 driver for Linux).
		 */
		if (is511plus_)
			sensortype_ = OV511SENSOR_7620;
		else
			sensortype_ = OV511SENSOR_7620AE;
	} else if ((val & 3) == 0) {
		sensortype_ = OV511SENSOR_7620;
	}

	if (sensortype_ == OV511SENSOR_7620) {
		i2cWrite(OV7610_REG_EC,		0xff);
		i2cWrite(OV7610_REG_FD,		0x06);
		i2cWrite(OV7610_REG_COMH,	0x24);
		i2cWrite(OV7610_REG_EHSL,	0xac);
		i2cWrite(OV7610_REG_COMA,	0x00);
		i2cWrite(OV7610_REG_COMH,	0x24);
		i2cWrite(OV7610_REG_RWB,	0x85);
		i2cWrite(OV7610_REG_COMD,	0x01);
		i2cWrite(0x23,			0x00);
		i2cWrite(OV7610_REG_ECW,	0x10);
		i2cWrite(OV7610_REG_ECB,	0x8a);
		i2cWrite(OV7610_REG_COMG,	0xe2);
		i2cWrite(OV7610_REG_EHSH,	0x00);
		i2cWrite(OV7610_REG_EXBK,	0xfe);
		i2cWrite(0x30,			0x71);
		i2cWrite(0x31,			0x60);
		i2cWrite(0x32,			0x26);
		i2cWrite(OV7610_REG_YGAM,	0x20);
		i2cWrite(OV7610_REG_BADJ,	0x48);
		i2cWrite(OV7610_REG_COMA,	0x24);
		i2cWrite(OV7610_REG_SYN_CLK,	0x01);
		i2cWrite(OV7610_REG_BBS,	0x24);
		i2cWrite(OV7610_REG_RBS,	0x24);

	} else {
		i2cWrite(OV7610_REG_EC,		0xff);
		i2cWrite(OV7610_REG_FD,		0x06);
		i2cWrite(OV7610_REG_COMH,	0x24);
		i2cWrite(OV7610_REG_EHSL,	0xac);
		i2cWrite(OV7610_REG_COMA,	0x00);
		i2cWrite(OV7610_REG_COMK,	0x81);
		i2cWrite(OV7610_REG_COMH,	0x24);	/* 0c */
		i2cWrite(OV7610_REG_RWB,	0x85);	/* lg's setting */
		i2cWrite(OV7610_REG_COMD,	0x01);
		i2cWrite(OV7610_REG_COME,	0x1c);
		i2cWrite(0x23,			0x2a);
		i2cWrite(OV7610_REG_ECW,	0x10);
		i2cWrite(OV7610_REG_ECB,	0x8a);
		i2cWrite(OV7610_REG_COMG,	0xc2);
		i2cWrite(OV7610_REG_EHSH,	0x04);
		i2cWrite(OV7610_REG_EXBK,	0xfe);
		i2cWrite(0x30,			0x71);
		i2cWrite(0x31,			0x60);
		i2cWrite(0x32,			0x26);
		i2cWrite(OV7610_REG_YGAM,	0x20);
		i2cWrite(OV7610_REG_BADJ,	0x48);
		i2cWrite(OV7610_REG_COMA,	0x24);
		i2cWrite(OV7610_REG_SYN_CLK,	0x01);
		i2cWrite(OV7610_REG_BBS,	0x24);
		i2cWrite(OV7610_REG_RBS,	0x24);
	}

	i2cWrite(OV7610_REG_COMF, 0xa2);
	i2cWrite(OV7610_REG_COMB, 0x01);
	if (sensortype_ == OV511SENSOR_7620)
		i2cWrite(OV7610_REG_COMJ, 0x91);
	else
		i2cWrite(OV7610_REG_COMJ, 0x93);

	return 0;
}

int OV511Grabber::initDevice()
{
	struct usb_alt_interface alt;

	/* reset the OV511 */
	if (regWrite(OV511_REG_RST, 0x7f) < 0)
		return -1;

	if (regWrite(OV511_REG_RST, 0) < 0)
		return -1;

	/* initialize system */
	if (regWrite(OV511_REG_EN_SYS, 0x1) < 0)
		return -1;

	/* determine the camera model */
	if ((cameraid = regRead(OV511_REG_CID)) < 0)
		return -1;

	/* set I2C write slave ID for OV7610 */
	if (regWrite(OV511_REG_SID, OV7610_I2C_WRITE_ID) < 0)
		return -1;

	/* set I2C read slave ID for OV7610 */
	if (regWrite(OV511_REG_SRA, OV7610_I2C_READ_ID) < 0)
		return -1;

	if (regWrite(OV511_REG_PKSZ, 0x1) < 0)
		return -1;

	if (regWrite(OV511_REG_PKFMT, 0x0) < 0)
		return -1;

	if (regWrite(OV511_REG_RST, 0x3d) < 0)
		return -1;

	if (regWrite(OV511_REG_RST, 0x0) < 0)
		return -1;

	if (i2cWrite(OV7610_REG_COMA, 0x80) < 0) {
		if (regWrite(OV511_REG_SID, OV6xx0_I2C_WRITE_ID) < 0)
			return -1;

		if (regWrite(OV511_REG_SRA, OV6xx0_I2C_READ_ID) < 0)
			return -1;

		if (regWrite(OV511_REG_RST, 0) < 0)
			return -1;

		if (i2cWrite(OV7610_REG_COMA, 0x80) < 0)
			return -1;

		if (init6xx0() < 0)
			return -1;
	} else {
		if (init76xx() < 0)
			return -1;
	}

	if (i2cRead(0x00) < 0)
		return -1;

	/* set YUV 4:2:0 format, Y channel LPF */
	if (regWrite(OV511_REG_M400, 0x01) < 0)
		return -1;
	if (regWrite(OV511_REG_M420_YFIR, 0x03) < 0)
		return -1;

	/* disable snapshot */
	if (regWrite(OV511_REG_SNAP, 0x0) < 0)
		return -1;
	/* disable compression */
	if (regWrite(OV511_REG_CE_EN, 0x0) < 0)
		return -1;

	regWrite(OV511_REG_PXCNT, 0x27);
	regWrite(OV511_REG_LNCNT, 0x1D);

	regWrite(OV511_REG_PXDV, 0x01);
	regWrite(OV511_REG_LNDV, 0x01);

	if (is511plus_) {
		/* set FIFO format (961-byte packets) */
		pktsize_ = 961;
		/* select the 961-byte alternative */
		alt.uai_interface_index = 0;
		alt.uai_alt_no = 7;
	} else {
		/* set FIFO format (993-byte packets) */
		pktsize_ = 993;
		/* select the 993-byte alternative */
		alt.uai_interface_index = 0;
		alt.uai_alt_no = 1;
	}

	if (regWrite(OV511_REG_PKSZ, (pktsize_ - 1) / 32) < 0)
		return -1;
	if (regWrite(OV511_REG_PKFMT, 0x03) < 0)
		return -1;

	if (ioctl(ctlfd_, USB_SET_ALTINTERFACE, &alt) < 0) {
		perror("USB_SET_ALTINTERFACE");
		return -1;
	}

	/* reset the device again */

	if (regWrite(OV511_REG_RST, 0x3F) < 0)
		return -1;

	if (regWrite(OV511_REG_RST, 0x00) < 0)
		return -1;

	state_ = OV511FRAME_SKIPPING;
	segsize_ = 384;

	iY_ = jY_ = iUV_ = jUV_ = 0;

	if ((fd_ = open(ugen0_01, O_RDONLY)) < 0) {
		perror(ugen0_01);
		return -1;
	}

	return 0;
}

int OV511Grabber::capture()
{
	int len;
	int srcoff, dstoff, cplen;
	int frmnm = 0;
	int ret;
	struct timeval tv;
	fd_set readfds;

	if (changed_) {
		i2cWrite(OV7610_REG_RED, red_);
		i2cWrite(OV7610_REG_BLU, blue_);
		i2cWrite(OV7610_REG_CTR, contrast_);
		i2cWrite(OV7610_REG_BRT, brightness_);
		changed_ = 0;
	}


	loopcnt_ = 0;
#if 0
	state_ = OV511FRAME_SKIPPING;
#endif
	iY_ = jY_ = iUV_ = jUV_ = 0;
	while (1) {
		tv.tv_sec = tv.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_SET(fd_, &readfds);
		ret = select(fd_ + 1, &readfds, NULL, NULL, &tv);
		if (ret <= 0)
			break;
		if ((len = read(fd_, buf, pktsize_)) != pktsize_)
			break;

		if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 0
		   && buf[4] == 0 && buf[5] == 0 && buf[6] == 0 && buf[7] == 0
		   && (buf[8] & 0x80) == 0 && buf[pktsize_ - 1] == 0) {
			state_ = OV511FRAME_READING;
			loopcnt_ = 0;
			iY_ = jY_ = iUV_ = jUV_ = 0;
			srcoff = 9;
			dstoff = 0;
			cplen = pktsize_ - 10;
		} else if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0
			   && buf[3] == 0 && buf[4] == 0 && buf[5] == 0
			   && buf[6] == 0 && buf[7] == 0
			   && (buf[8] & 0x80) == 0x80
			   && state_ == OV511FRAME_READING) {
			// fprintf(stderr, "done\n");
			state_ = OV511FRAME_DONE;
			break;
		} else {
			frmnm = buf[pktsize_ - 1];
			if (frmnm == 0)
				continue;
			state_ = OV511FRAME_READING;
			srcoff = 0;
			dstoff = (pktsize_ - 10) +
				(pktsize_ - 1) *
				(256 * loopcnt_ + frmnm - 1);
			cplen = pktsize_ - 1;
			if (frmnm == 255)
				loopcnt_++;
		}

		if (dstoff + cplen < sizeof(frmbuf))
			memcpy(&frmbuf[dstoff], &buf[srcoff], cplen);
	}

	if (state_ != OV511FRAME_DONE) {
		return 0;
	} else {
		procData();
		state_ = OV511FRAME_SKIPPING;
		return 1;
	}
}

void OV511Grabber::procData()
{
	int res;
	u_char *p;

	res = sizeof(frmbuf);
	p = frmbuf;
	while (res > 0) {
		procSegment(p);
		p += segsize_;
		res -= segsize_;
	}
}

#define OV511_BLKWIDTH		8
#define OV511_BLKHEIGHT		8
#define OV511_UVWIDTH		8
#define OV511_UVHEIGHT		8
#define OV511_YWIDTH		32
#define OV511_YHEIGHT		8

void OV511Grabber::procSegment(u_char *p)
{
	int i, j;
	u_char *udata, *vdata, *ydata;
	u_char *uout, *vout, *yout;

	udata = p;
	vdata = udata + (OV511_UVWIDTH * OV511_UVHEIGHT);
	ydata = vdata + (OV511_UVWIDTH * OV511_UVHEIGHT);

	yout = frame_ + (jY_ + (outh_ - inh_) / 2) * outw_
		+ iY_ + (outw_ - inw_) / 2;
	uout = (frame_ + framesize_)
		+ (jUV_ + (outh_ - inh_) / 4) * outw_ / 2
		+ iUV_ + (outw_ - inw_) / 4;
	vout = uout + framesize_ / 4;

	/* process U/V data */
	for (j = 0; j < OV511_BLKHEIGHT; j++) {
		memcpy(uout + j * outw_ / 2, udata, OV511_BLKWIDTH);
		udata += OV511_BLKWIDTH;
		memcpy(vout + j * outw_ / 2, vdata, OV511_BLKWIDTH);
		vdata += OV511_BLKWIDTH;
	}

	iUV_ += OV511_UVWIDTH;
	if (iUV_ >= inw_ / 2) {
		iUV_ = 0;
		jUV_ += OV511_UVHEIGHT;
		if (jUV_ >= inh_ / 2)
			jUV_ = 0;
	}

	/* process Y data */
	for (i = 0; i < OV511_YHEIGHT * (OV511_YWIDTH / OV511_BLKWIDTH); i++) {
		int k;

		j = i / OV511_BLKHEIGHT;
		k = i - j * OV511_BLKHEIGHT;
		memcpy(yout + k * outw_ + j * OV511_BLKWIDTH, ydata,
		       OV511_BLKWIDTH);
		ydata += OV511_BLKWIDTH;
	}

	iY_ += OV511_YWIDTH;
	if (iY_ >= inw_) {
		iY_ = 0;
		jY_ += OV511_YHEIGHT;
		if (jY_ >= inh_)
			jY_ = 0;
	}

	if (iY_ == 0 && jY_ == 0)
		state_ = OV511FRAME_DONE;
}


void CIFOV511Grabber::setsize()
{
	if (format_ < 0)
		return;

	set_size_cif(basewidth_ / decimate_, baseheight_ / decimate_);
	allocref();
}
