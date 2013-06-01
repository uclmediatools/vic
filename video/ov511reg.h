/*
 * Most portions of this file are taken from vid.h in "vid" written 
 * by Peter S. Housel (http://ovtvid-bsd.sourceforge.net/).
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

#ifndef _VIDEO_GRABBER_OV511_H_
#define _VIDEO_GRABBER_OV511_H_

#define OV511_REG_DLYM		0x10
#define OV511_REG_PEM		0x11
#define OV511_REG_PXCNT		0x12
#define OV511_REG_LNCNT		0x13
#define OV511_REG_PXDV		0x14
#define OV511_REG_LNDV		0x15
#define OV511_REG_M400		0x16
#define OV511_REG_LSTR		0x17
#define OV511_REG_M420_YFIR	0x18
#define OV511_REG_SPDLY		0x19
#define OV511_REG_SNPX		0x1A
#define OV511_REG_SNLN		0x1B
#define OV511_REG_SNPD		0x1C
#define OV511_REG_SNLD		0x1D
#define OV511_REG_SN400		0x1E
#define OV511_REG_SNAF		0x1F
#define OV511_REG_ENFC		0x20
#define OV511_REG_ARCP		0x21
#define OV511_REG_MRC		0x22
#define OV511_REG_RFC		0x23
#define OV511_REG_PKSZ		0x30
#define OV511_REG_PKFMT		0x31
#define OV511_REG_PIO		0x38
#define OV511_REG_PDATA		0x39
#define OV511_REG_ENTP		0x3E
#define OV511_REG_I2C_CONTROL	0x40
#define OV511_REG_SID		0x41
#define OV511_REG_SWA		0x42
#define OV511_REG_SMA		0x43
#define OV511_REG_SRA		0x44
#define OV511_REG_SDA		0x45
#define OV511_REG_PSC		0x46
#define OV511_REG_TMO		0x47
#define OV511_REG_SPA		0x48
#define OV511_REG_SPD		0x49
#define OV511_REG_RST		0x50
#define OV511_REG_CLKDIV	0x51
#define OV511_REG_SNAP		0x52
#define OV511_REG_EN_SYS	0x53
#define OV511_REG_USR		0x5E
#define OV511_REG_CID		0x5F
#define OV511_REG_PRH_Y		0x70
#define OV511_REG_PRH_UV	0x71
#define OV511_REG_PRV_Y		0x72
#define OV511_REG_PRV_UV	0x73
#define OV511_REG_QTH_Y		0x74
#define OV511_REG_QTH_UV	0x75
#define OV511_REG_QTV_Y		0x76
#define OV511_REG_QTV_UV	0x77
#define OV511_REG_CE_EN		0x78
#define OV511_REG_LT_EN		0x79

#define OV7610_REG_GC		0x00
#define OV7610_REG_BLU		0x01
#define OV7610_REG_RED		0x02
#define OV7610_REG_SAT		0x03
#define OV7610_REG_CTR		0x05
#define OV7610_REG_BRT		0x06
#define OV7610_REG_BBS		0x0C
#define OV7610_REG_RBS		0x0D
#define OV7610_REG_GAM		0x0E
#define OV7610_REG_RWB		0x0F
#define OV7610_REG_EC		0x10
#define OV7610_REG_SYN_CLK	0x11
#define OV7610_REG_COMA		0x12
#define OV7610_REG_COMB		0x13
#define OV7610_REG_COMC		0x14
#define OV7610_REG_COMD		0x15
#define OV7610_REG_FD		0x16
#define OV7610_REG_HS		0x17
#define OV7610_REG_HE		0x18
#define OV7610_REG_VS		0x19
#define OV7610_REG_VE		0x1A
#define OV7610_REG_PS		0x1B
#define OV7610_REG_MIDH		0x1C
#define OV7610_REG_MIDL		0x1D
#define OV7610_REG_COME		0x20
#define OV7610_REG_YOF		0x21
#define OV7610_REG_UOF		0x22
#define OV7610_REG_ECW		0x24
#define OV7610_REG_ECB		0x25
#define OV7610_REG_COMF		0x26
#define OV7610_REG_COMG		0x27
#define OV7610_REG_COMH		0x28
#define OV7610_REG_COMI		0x29
#define OV7610_REG_EHSH		0x2A
#define OV7610_REG_EHSL		0x2B
#define OV7610_REG_EXBK		0x2C
#define OV7610_REG_COMJ		0x2D
#define OV7610_REG_VOF		0x2E
#define OV7610_REG_ABS		0x2F
#define OV7610_REG_YGAM		0x33
#define OV7610_REG_BADJ		0x34
#define OV7610_REG_COML		0x35
#define OV7610_REG_COMK		0x38

#define OV7610_I2C_WRITE_ID	0x42
#define OV7610_I2C_READ_ID	0x43
#define OV6xx0_I2C_WRITE_ID	0xC0
#define OV6xx0_I2C_READ_ID	0xC1

#define OV7610_I2C_RETRIES	3
#define OV7610_I2C_CLOCK_DIV	4

#endif /* _VIDEO_GRABBER_OV511_H_ */
