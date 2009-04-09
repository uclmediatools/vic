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
"@(#) $Header: /cvs/fl/ag-media/vic/net/crypt-rijndael.cpp,v 1.2 2006/04/03 19:33:37 turam Exp $ (LBL)";
 
extern "C" {
#include "rijndael-api-fst.h"
}

#include "crypt.h"
#include "inet.h"
#include "rtp.h"

class CryptRijndael : public Crypt {
public:
    CryptRijndael();
    ~CryptRijndael();
    virtual int install_key(const u_int8_t* key);
    virtual u_char* Encrypt(const u_char* in, int& len);
    virtual int Decrypt(const u_char* in, int len, u_char* out);

protected:
    int decrypt(const u_char* in, int len, u_char* out, int rtcp);

    keyInstance keyInstEncrypt_;
    keyInstance keyInstDecrypt_;
    cipherInstance cipherInst_;
    static int didinit_;
    u_char* wrkbuf_;
};

class CryptRijndaelCtrl : public CryptRijndael {
public:
    virtual u_char* Encrypt(const u_char* in, int& len);
    virtual int Decrypt(const u_char* in, int len, u_char* out);
};

static class CryptRijndaelMatcher : public Matcher {
public:
    CryptRijndaelMatcher() : Matcher("crypt") {}
    TclObject* match(const char* id) {
	if (strcmp(id, "Rijndael/data") == 0 ||
	    strcmp(id, "Rijndael1/data") == 0 ||
        strcmp(id, "AES/data") == 0)
	    return (new CryptRijndael);
	if (strcmp(id, "Rijndael/ctrl") == 0 ||
	    strcmp(id, "Rijndael1/ctrl") == 0 ||
        strcmp(id, "AES/ctrl") == 0)
	    return (new CryptRijndaelCtrl);
	return (0);
    }
} rijndael_matcher;

CryptRijndael::CryptRijndael() 
{
    /* enough extra space for padding and RTCP 4-byte random header */
    wrkbuf_ = new u_char[2*RTP_MTU + 4 ];
}

CryptRijndael::~CryptRijndael()
{
    delete wrkbuf_;
}

int CryptRijndael::install_key(const u_int8_t* key)
{
    debug_msg("Install rijndael key '%s'\n", key);

    int rc;
//    int keylen = strlen((const char *) key);

    //
    // The vic framework gives us an md5 hash which is 16 bytes long.
    //
    int keylen = 16;
    rc = makeKey(&keyInstEncrypt_, DIR_ENCRYPT, keylen * 8, (BYTE *) key);
    if (rc < 0)
	debug_msg("makeKey failed: %d\n", rc);

    rc = makeKey(&keyInstDecrypt_, DIR_DECRYPT, keylen * 8, (BYTE *) key);
    if (rc < 0)
	debug_msg("makeKey failed: %d\n", rc);

    rc = cipherInit(&cipherInst_, MODE_ECB, NULL);
    if (rc < 0)
	debug_msg("ciperInit failed: %d\n", rc);

    return (0);
}

u_char* CryptRijndael::Encrypt(const u_char* in, int& len)
{
    // Pad with zeros to the nearest 8 octet boundary       
    int pad = len & 15;
    if (pad != 0) {
	/* pad to an block (16 octet) boundary */
	pad = 16 - pad;
	u_char* rh = (u_char*)in;
	*rh |= 0x20; // set P bit
	u_char *padding = ((u_char*)in + len);
	for (int i=1; i<pad; i++)
	    *padding++ = 0;
	*padding++ = (char)pad;
	len += pad;
    }

    int rc = blockEncrypt(&cipherInst_, &keyInstEncrypt_, (unsigned char *) in, len * 8, wrkbuf_);
    if (rc < 0)
	debug_msg("blockEncrypt failed: %d\n", rc);
    
    return(wrkbuf_);        
}

int CryptRijndael::decrypt(const u_char* in, int len, u_char* out, int rtcp)
{
    /* check that packet is an integral number of blocks */
    if ((len & 15) != 0) {
	++badpktlen_;
	return (-1);
    }

    blockDecrypt(&cipherInst_, &keyInstDecrypt_, (unsigned char *) in, len * 8, out);

    // Strip the header of the first 4 bytes if it is an RTCP packet
    if (rtcp)
    {
	memmove(out, (u_char *)(out+4), (unsigned long)(len-4));
	len -= 4;
    }

    if ((out[0] & 0x20) != 0) {
	/* P bit set - trim off padding */
	int pad = out[len - 1];
	if (pad > 15 || pad == 0) {
	    ++badpbit_;
	    return (-1);
	}
	len -= pad;
    }
    return (len);
}

int CryptRijndael::Decrypt(const u_char* in, int len, u_char* out)
{
    return decrypt(in, len, out, 0);
}

u_char* CryptRijndaelCtrl::Encrypt(const u_char* in, int& len)
{
    // Attach 4 random bytes to the top of the header to reduce chances of a
    // plaintext attack on the otherwise fixed header.
    u_int32_t* new_random = (u_int32_t*)wrkbuf_;
    new_random[0] = random();
 
    // Copy into the working buffer
    memcpy((u_char *)(wrkbuf_+4),(u_char *)(in),(unsigned long)(len));
    len +=4;
 
    // Pad with zeros to the nearest 8 octet boundary	
    int pad = len & 15;
    if (pad != 0) {
	/* pad to an block (8 octet) boundary */
	pad = 16 - pad;
	u_char* rh = (wrkbuf_+4);
	*rh |= 0x20; // set P bit
	u_char *padding = (wrkbuf_ + len);
	for (int i=1; i<pad; i++)
	    *padding++ = 0;
	*padding++ = (char)pad;
	len += pad;
    }
 
    int rc = blockEncrypt(&cipherInst_, &keyInstEncrypt_, wrkbuf_, len * 8, wrkbuf_);
    if (rc < 0)
	debug_msg("blockEncrypt failed: %d\n", rc);

    return (wrkbuf_);
}

int CryptRijndaelCtrl::Decrypt(const u_char* in, int len, u_char* out)
{
    return (decrypt(in, len, out, 1));
}
