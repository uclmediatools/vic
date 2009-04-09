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

static const char rcsid[] =
    "@(#) $Header$ (LBL)";
 

#include "qfDES.h"
#include "crypt.h"
#include "inet.h"
#include "rtp.h"

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
	u_char des_key[8];
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

	/* enough extra space for padding and RTCP 4-byte random header */
	wrkbuf_ = new u_char[2*RTP_MTU + 4];
}

CryptDES::~CryptDES()
{
	delete[] wrkbuf_; //SV-XXX: Debian
}

int CryptDES::install_key(const u_int8_t* key)
{

	/*
	 * Take the first 56-bits of the input key and spread
	 * it across the 64-bit DES key space inserting a bit-space
	 * of garbage (for parity) every 7 bits.  The garbage
	 * will be taken care of below.  The library we're
	 * using expects the key and parity bits in the following
	 * MSB order: K0 K1 ... K6 P0 K8 K9 ... K14 P1 ...
	 */
 	des_key[0] = key[0];
 	des_key[1] = key[0] << 7 | key[1] >> 1;
 	des_key[2] = key[1] << 6 | key[2] >> 2;
 	des_key[3] = key[2] << 5 | key[3] >> 3;
 	des_key[4] = key[3] << 4 | key[4] >> 4;
 	des_key[5] = key[4] << 3 | key[5] >> 5;
 	des_key[6] = key[5] << 2 | key[6] >> 6;
 	des_key[7] = key[6] << 1;

	/* fill in parity bits to make DES library happy */
	for (int i = 0; i < 8; ++i) {
		register int k = des_key[i] & 0xfe;
		int j = k;
		j ^= j >> 4;
		j ^= j >> 2;
		j ^= j >> 1;
		j = (j & 1) ^ 1;
		des_key[i] = k | j;
	}

	return (0);
}

int CryptDES::encrypt(u_int32_t* blk, const u_char* in, int len, u_char* rh)
{
	//SV-XXX
	UNUSED(blk);
	UNUSED(in);
	UNUSED(rh);

 	// This function is not used in this implementation
  	return (len);
}

u_char* CryptDES::Encrypt(const u_char* in, int& len)
{
       // We are not using the IV
       char initVec[8] = {0,0,0,0,0,0,0,0};
        
       // Pad with zeros to the nearest 8 octet boundary       
       int pad = len & 7;
         if (pad != 0) {
                 /* pad to an block (8 octet) boundary */
                 pad = 8 - pad;
               u_char* rh = (u_char*)in;
                 *rh |= 0x20; // set P bit
               u_char *padding = ((u_char*)in + len);
               for (int i=1; i<pad; i++)
                 *padding++ = 0;
               *padding++ = (char)pad;
               len += pad;
         }
 
       // Carry out the encryption
         memcpy(wrkbuf_, (u_char *)(in), (unsigned long)(len));
         qfDES_CBC_e((unsigned char *)des_key, (unsigned char *)(wrkbuf_), len, (unsigned char *)initVec);
       return(wrkbuf_);        
}

int CryptDES::decrypt(const u_char* in, int len, u_char* out, int rtcp)
{

	/* check that packet is an integral number of blocks */
	if ((len & 7) != 0) {
		++badpktlen_;
		return (-1);
	}

 	// We are not using the IV
 	char initVec[8] = {0,0,0,0,0,0,0,0};
 	u_char* inpk = (u_char *)in;
	u_char* padbit;
 
 	// Carry out decryption
 	qfDES_CBC_d((unsigned char *)des_key, (unsigned char *)(inpk), len, (unsigned char *)initVec);
 
 	// Strip the header of the first 4 bytes if it is an RTCP packet
 	if (rtcp)
 	{
 		memcpy(out, (u_char *)(in+4), (unsigned long)(len-4));
 		len -= 4;
		// Find padding bit at end
		u_char* rhp = out;
		padbit = out;
		while (((rhp+=((ntohs(((rtcphdr*)rhp)->rh_len)+1)<<2))-out) < (len)) {
				padbit=rhp;
		}
 	}
 	else
 	{
 		memcpy(out, in, len);
		padbit = out;
  	}
	/* Check pad bit - officially supposed to be in last packet
	 * Retain compat with old vic which put it in first packet
	 */
	if (((padbit[0] & 0x20) != 0) || ((out[0] & 0x20) != 0)) {
		/* P bit set - trim off padding */
		int pad = out[len - 1];
		if (pad > 7 || pad == 0) {
			++badpbit_;
			return (-1);
		}
		//correct header length after pad removal
		((rtcphdr*)padbit)->rh_len=htons(ntohs(((rtcphdr*)padbit)->rh_len)-(pad>>2));
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
 
 	// We are not using the IV
 	char initVec[8] = {0,0,0,0,0,0,0,0};
        
 	// Attach 4 random bytes to the top of the header to reduce chances of a
 	// plaintext attack on the otherwise fixed header.
 	u_int32_t* new_random = (u_int32_t*)wrkbuf_;
 	new_random[0] = random();
 
 	// Copy into the working buffer
 	memcpy((u_char *)(wrkbuf_+4),(u_char *)(in),(unsigned long)(len));
 	len +=4;
 
 	// Pad with zeros to the nearest 8 octet boundary	
 	int pad = len & 7;
    if (pad != 0) {
 		u_char* rh = (wrkbuf_+4);
		u_char* rhp = rh;
		u_char* rhlastp = rh;
 		u_char *padding = (wrkbuf_ + len);

		while (((rhp+=((ntohs(((rtcphdr*)rhp)->rh_len)+1)<<2))-rh) < (len-4))
			rhlastp=rhp;
		*rhlastp |= 0x20; // set Padding bit on LAST rtcp packet

		/* pad to an block (8 octet) boundary */
        pad = 8 - pad;
 		for (int i=1; i<pad; i++)
 			*padding++ = 0;
 		*padding++ = (char)pad;
 		len += pad;
		//correct header length after pad removal
		((rtcphdr*)rhlastp)->rh_len=htons(ntohs(((rtcphdr*)rhlastp)->rh_len)+(pad>>2));

     }
 
 
 	// Carry out the encryption
 	qfDES_CBC_e((unsigned char *)des_key, (unsigned char *)(wrkbuf_), len, (unsigned char *)initVec);
	return (wrkbuf_);
}

int CryptDESctrl::Decrypt(const u_char* in, int len, u_char* out)
{
	return (decrypt(in, len, out, 1));
}
