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

#include "crypt.h"
#include "inet.h"
#include "rtp.h"

/*
 * A simple XOR data scrambler.  This is module is provided for test
 * purposes and to illustrate the extensibility of the encryption interface.
 *
 * This algorithm is trivial to crack using an obvious plaintext
 * attack.  It will only protect against accidental eavesdropping.
 */
class CryptDull : public Crypt {
    public:
	CryptDull();
	~CryptDull();
	virtual int install_key(const u_int8_t* key);
	virtual u_char* Encrypt(const u_char* in, int& len);
	virtual int Decrypt(const u_char* in, int len, u_char* out);
    protected:
	void crypt(const u_int32_t* in, u_int32_t* out, int nw);
	u_int32_t key_;
	u_char* wrkbuf_;
};

static class CryptDullMatcher : public Matcher {
    public:
	CryptDullMatcher() : Matcher("crypt") {}
	TclObject* match(const char* id) {
		if (strcmp(id, "DULL/data") == 0 ||
		    strcmp(id, "DULL/ctrl") == 0)
			return (new CryptDull);
		else
			return (0);
	}
} crypt_dull_matcher;

CryptDull::CryptDull()
{

	/* enough extra space for padding and RTCP 4-byte random header */
	wrkbuf_ = new u_char[RTP_MTU + 8 + 4];
}

CryptDull::~CryptDull()
{
	delete[] wrkbuf_; //SV-XXX: Debian
}

int CryptDull::install_key(const u_int8_t* key)
{
	/* net order */
	key_ = *(u_int32_t*)key;
	return (0);
}

void CryptDull::crypt(const u_int32_t* in, u_int32_t* out, int nw)
{
	u_int32_t k = key_;
	for (int i = 0; i < nw; ++i)
		out[i] = in[i] ^ k;
}

u_char* CryptDull::Encrypt(const u_char* in, int& len)
{
	/*
	 * assume input is word-aligned and we can read
	 * past end to next longword boundary
	 */
	crypt((const u_int32_t*)in, (u_int32_t*)wrkbuf_, (len + 3) >> 2);
	return (wrkbuf_);
}

int CryptDull::Decrypt(const u_char* in, int len, u_char* out)
{
	/*
	 * assume input/output is word-aligned and we can read/write
	 * past end to next longword boundary
	 */
	crypt((const u_int32_t*)in, (u_int32_t*)out, (len + 3) >> 2);
	return (len);
}
