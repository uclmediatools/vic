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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include "crypt.h"
#include "inet.h"
#include "rtp.h"

extern "C" {
extern void DesQuickInit();
extern int DesMethod(u_char* m, u_char* k);
extern int DesQuickFipsEncrypt(u_char* dst, u_char* m, u_char* src);
extern int DesQuickFipsDecrypt(u_char* dst, u_char* m, u_char* src);
}

class CryptDES : public Crypt {
    public:
	CryptDES();
	~CryptDES();
	virtual int install_key(const u_int8_t* key);
	virtual u_char* Encrypt(const u_char* in, int& len);
	virtual int Decrypt(const u_char* in, int len, u_char* out);
    protected:
	int encrypt(u_int32_t* blk, const u_char* in, int len, u_char* rh);
	int decrypt(const u_char* in, int len, u_char* out, int rtcp);
	static int didinit_;
	u_char* wrkbuf_;
	u_char mkey_[128];
};

class CryptDESctrl : public CryptDES {
    public:
	virtual u_char* Encrypt(const u_char* in, int& len);
	virtual int Decrypt(const u_char* in, int len, u_char* out);
};

static class CryptDESMatcher : public Matcher {
    public:
	CryptDESMatcher() : Matcher("crypt") {}
	TclObject* match(const char* id) {
		if (strcmp(id, "DES/data") == 0 ||
		    strcmp(id, "DES1/data") == 0)
			return (new CryptDES);
		if (strcmp(id, "DES/ctrl") == 0 ||
		    strcmp(id, "DES1/ctrl") == 0)
			return (new CryptDESctrl);
		return (0);
	}
} des_matcher;

int CryptDES::didinit_;

CryptDES::CryptDES()
{
	if (didinit_ == 0) {
		DesQuickInit();
		didinit_ = 1;
	}
	/* enough extra space for padding and RTCP 4-byte random header */
	wrkbuf_ = new u_char[RTP_MTU + 8 + 4];
}

CryptDES::~CryptDES()
{
	delete wrkbuf_;
}

int CryptDES::install_key(const u_int8_t* key)
{
	/* DES key */
	u_char dk[8];
	/*
	 * Take the first 56-bits of the input key and spread
	 * it across the 64-bit DES key space inserting a bit-space
	 * of garbage (for parity) every 7 bits.  The garbage
	 * will be taken care of below.  The library we're
	 * using expects the key and parity bits in the following
	 * MSB order: K0 K1 ... K6 P0 K8 K9 ... K14 P1 ...
	 */
	dk[0] = key[0];
	dk[1] = key[0] << 7 | key[1] >> 1;
	dk[2] = key[1] << 6 | key[2] >> 2;
	dk[3] = key[2] << 5 | key[3] >> 3;
	dk[4] = key[3] << 4 | key[4] >> 4;
	dk[5] = key[4] << 3 | key[5] >> 5;
	dk[6] = key[5] << 2 | key[6] >> 6;
	dk[7] = key[6] << 1;

	/* fill in parity bits to make DES library happy */
	for (int i = 0; i < 8; ++i) {
		register int k = dk[i] & 0xfe;
		int j = k;
		j ^= j >> 4;
		j ^= j >> 2;
		j ^= j >> 1;
		j = (j & 1) ^ 1;
		dk[i] = k | j;
	}
	register int s = DesMethod(mkey_, dk);
	return (s);
}

int CryptDES::encrypt(u_int32_t* blk, const u_char* in, int len, u_char* rh)
{
	int pad = len & 7;
	if (pad != 0) {
		/* pad to an block (8 octet) boundary */
		pad = 8 - pad;
		*rh |= 0x20; // set P bit
	}

	register u_int32_t* wp = (u_int32_t*)wrkbuf_;
	register u_char* m = mkey_;
	DesQuickFipsEncrypt((u_char*)wp, m, (u_char*)blk);

	register u_int32_t* bp = (u_int32_t*)in;
	register int i;
	for (i = len >> 3; --i >= 0; ) {
		blk[0] = bp[0] ^ wp[0];
		blk[1] = bp[1] ^ wp[1];
		bp += 2;
		wp += 2;
		DesQuickFipsEncrypt((u_char*)wp, m, (u_char*)blk);
	}
	if (pad > 0) {
		len += pad;
		blk[0] = bp[0];
		blk[1] = bp[1];
		u_char* cp = (u_char*)blk + 7;
		*cp = pad;
		while (--pad > 0)
			*--cp = 0;
		blk[0] ^= wp[0];
		blk[1] ^= wp[1];
		wp += 2;
		DesQuickFipsEncrypt((u_char*)wp, m, (u_char*)blk);
	}
	return (len);
}

u_char* CryptDES::Encrypt(const u_char* in, int& len)
{
	u_int32_t blk[2];
	blk[0] = *(u_int32_t*)in;
	blk[1] = *(u_int32_t*)(in + 4);
	len = encrypt(blk, in + 8, len - 8, (u_char*)blk) + 8;
	return (wrkbuf_);
}

int CryptDES::decrypt(const u_char* in, int len, u_char* out, int rtcp)
{
	register u_int32_t* wp = (u_int32_t*)out;
	register const u_int32_t* bp = (u_int32_t*)in;
	register u_char* m = mkey_;

	/* check that packet is an integral number of blocks */
	if ((len & 7) != 0) {
		++badpktlen_;
		return (-1);
	}
	int nblk = len >> 3;
	DesQuickFipsDecrypt((u_char*)wp, m, (u_char*)bp);
	bp += 2;
	nblk -= 1;
	if (rtcp) {
		/* throw away random RTCP header */
		wp[0] = wp[1];
		wp += 1;
		len -= 4;
	} else
		wp += 2;

	while (--nblk >= 0) {
		DesQuickFipsDecrypt((u_char*)wp, m, (u_char*)bp);
		wp[0] ^= bp[-2];
		wp[1] ^= bp[-1];
		bp += 2;
		wp += 2;
	}
	if ((out[0] & 0x20) != 0) {
		/* P bit set - trim off padding */
		int pad = out[len - 1];
		if (pad > 7 || pad == 0) {
			++badpbit_;
			return (-1);
		}
		len -= pad;
	}
	return (len);
}

int CryptDES::Decrypt(const u_char* in, int len, u_char* out)
{
	return (decrypt(in, len, out, 0));
}

u_char* CryptDESctrl::Encrypt(const u_char* in, int& len)
{
	u_int32_t blk[2];
	blk[0] = random();
	blk[1] = *(u_int32_t*)in;
	len = encrypt(blk, in + 4, len - 4, (u_char*)&blk[1]) + 8;
	return (wrkbuf_);
}

int CryptDESctrl::Decrypt(const u_char* in, int len, u_char* out)
{
	return (decrypt(in, len, out, 1));
}
