#include "config_unix.h"
#include "config_win32.h"
#include "debug.h"
#include "net_udp.h"
#include "crypt.h"
#include "md5.h"
#include "qfDES.h"
#include "crypt_random.h"

/* Global variables accessible inside this module only */
static uint  empty_key = TRUE;
static u_char des_key[8];
u_char        crypt_buffer[16532];
u_char       *wrkbuf_ = crypt_buffer;
static uint  badpktlen_ = 0;
static uint  badpbit_ = 0;

int Null_Key(void)
{
	return(empty_key);
}

static int install_key( unsigned char* key)
/************************************************************************
* DESCRIPTION                                                           *
*                                                                       *
* This function expands the 56-bit key into 64-bits by inserting parity *
* bits.  The MSB of each byte is used as the parity bit.                *
*                                                                       *
* INPUT PARAMETERS                                                      *
*                                                                       *
*  unsigned char* key: a 56-bit key                                	*
*                                                                       *
* RETURNS                                                               *
*                                                                       *
* This function always returns 0 (Success)                              *
************************************************************************/
{
	int i;
	int j;
	register int k;

	/* DES key */
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
	for (i = 0; i < 8; ++i) 
	{
		k = des_key[i] & 0xfe;
		j = k;
		j ^= j >> 4;
		j ^= j >> 2;
		j ^= j >> 1;
		j = (j & 1) ^ 1;
		des_key[i] = k | j;
	}

	return (0);
}

int Set_Key(char* key)
/************************************************************************
* DESCRIPTION                                                           *
*                                                                       *
* This function creates an MD5 digest of a plain text key to produce a  *
* 56-bit encryption key.  The MD5 digest has (more) uniform entropy     *
* distribution.                                                         *
*                                                                       *
* INPUT PARAMETERS                                                      *
*                                                                       *
*  char* key: the plain text key entered by the operator           	*
*                                                                       *
* RETURNS                                                               *
*                                                                       *
* This function will always return 0 (Success)                          *
************************************************************************/
{
        u_char hash[16];
	int i;
	
	if ( key[0] != 0 )
	{
          MD5_CTX context;
          MD5Init(&context);
          MD5Update(&context, (u_char*)key, strlen(key));
          MD5Final((u_char *)hash, &context);
	  empty_key = FALSE;
          return (install_key(hash));
	}
	else
	{
	  empty_key = TRUE;
	  for (i=0; i<8; i++)
	    des_key[i]=0;
	  return 1;
	}
}

u_char* Encrypt( u_char* in, int* len)
/************************************************************************
* DESCRIPTION                                                           *
*                                                                       *
* This function encrypts data packets.  The data is padded with zeros if*
* the buffer size does not lie on an 8-octet boundary.                  *
*                                                                       *
* INPUT PARAMETERS                                                      *
*                                                                       *
* u_char* in:  the input buffer to be encrypted                   	*
* int* len:          buffer length                                      *
*                                                                       *
* OUTPUT PARAMETERS                                                     *
*                                                                       *
* int* len:  this is to account for padding.                            *
*                                                                       *
* RETURNS                                                               *
*                                                                       *
* A pointer to the encrypted data is returned.                          *
************************************************************************/
{

	/* We are not using the IV */
	 u_char initVec[8] = {0,0,0,0,0,0,0,0};
	int pad;
	u_char* rh = wrkbuf_;
	int i;
	u_char* padding;
 
        memcpy(wrkbuf_, in, *len);

	/* Pad with zeros to the nearest 8 octet boundary */	
	pad = *len & 7;
        if (pad != 0) 
	{
                /* pad to an block (8 octet) boundary */
                pad = 8 - pad;
                rh[0] = rh[0] | 0x20; /* set P bit */
		padding = (wrkbuf_ + *len);
		for (i=1; i<pad; i++)
		  *(padding++) = 0;
		*(padding++) = (char)pad;
		*len += pad;
        }

	/* Carry out the encryption */
        qfDES_CBC_e(des_key, wrkbuf_, *len, initVec);
	return(wrkbuf_);	
}

int Decrypt( const u_char* in, u_char* out, int* len)
/************************************************************************
* DESCRIPTION                                                           *
*                                                                       *
* This function provides the external interface for decrypting data     *
* packets                                                               *
*                                                                       *
* INPUT PARAMETERS                                                      *
*                                                                       *
*  u_char* in: the buffer to be decrypted                               *
* int len:          the size of the buffer                              *
*                                                                       *
* OUTPUT PARAMETERS                                                     *
*                                                                       *
* u_char* out: the decrypted data                                       *
*                                                                       *
* RETURNS                                                               *
*                                                                       *
* int : the number of padding bytes removed.		                *
************************************************************************/
{
	int pad=0;

	/* We are not using the IV */
	u_char initVec[8] = {0,0,0,0,0,0,0,0};

	/* check that packet is an integral number of blocks */
	if ((*len & 7) != 0) {
		++badpktlen_;
		return (0);
	}

	/* Carry out decryption */
	memcpy(wrkbuf_, in, *len);
	qfDES_CBC_d(des_key, wrkbuf_, *len, initVec);

	memcpy(out, wrkbuf_, *len);

	/* Strip off the padding, where necessary */
	if ((out[0] & 0x20) != 0) {
		/* P bit set - trim off padding */
		out[0] = out[0] ^ 0x20;
		pad = out[*len - 1];
		if (pad > 7 || pad == 0) {
			++badpbit_;
			return (0);
		}
		*len -= pad;
	}

	return pad;
}

int Decrypt_Ctrl( const u_char* in, u_char* out, int* len)
/*
 * This function provides the external interface for decrypting control packets
 *
 * INPUT PARAMETERS
 * 	u_char* in: the buffer to be decrypted
 * 	int* len:   the size of the buffer
 * OUTPUT PARAMETERS
 * 	u_char* out: the decrypted control information
 * RETURNS
 * 	int : the number of bytes removed
 */
{
	int pad=0;
	rtcp_t *rtcp_p;
	rtcp_t *current_p;

	/* We are not using the IV */
	u_char initVec[8] = {0,0,0,0,0,0,0,0};

	/* check that packet is an integral number of blocks */
	if ((*len & 7) != 0) {
		++badpktlen_;
		return 0;
	}

	/* Carry out decryption */
	memcpy(wrkbuf_, in, *len);
	qfDES_CBC_d(des_key, wrkbuf_, *len, initVec);

	/* Strip off the first 4 bytes of the RTCP packet (random data) */
	*len -= 4;
	memcpy(out, (wrkbuf_+4), *len);
	rtcp_p = (rtcp_t *) out;

	if (rtcp_p->common.p == 1) {
		/* Padding bit is set on the first packet in the compound. 
		 * This is, of course, illegal, but vat insists on doing it, 
		 * despite numerous people pointing out to Steve McCanne that
		 * it's JUST PLAIN WRONG! Try to sort out the mess...
		 */
		debug_msg("Decrypted RTCP packet has bogus padding bit. Assuming vat bug, and trying workaround...\n");
		rtcp_p->common.p = 0;
		do {
			current_p = rtcp_p;
			rtcp_p    = (rtcp_t *) ((uint32 *) rtcp_p + ntohs(rtcp_p->common.length) + 1);
			if (current_p->common.pt == RTCP_SDES) {
				/* Sigh. Yet another vat bug... the length field in SDES packets is wrong, if encrypted */
				current_p->common.length = (uint16)htons((uint16)(ntohs(current_p->common.length) + 1));
			}
		} while (rtcp_p < (rtcp_t *) (out + *len) && rtcp_p->common.type == 2);
		pad = out[*len - 1];
		if (pad > 7 || pad == 0) {
			++badpbit_;
			return 0;
		}
		current_p->common.length = (uint16)htons((uint16)(((((ntohs(current_p->common.length)+1)*4)-pad)/4)-1));
		*len -= pad;
		return pad+4;
	} else {
		/* Find the last packet, in this compound, and strip off the padding... */
		do {
			current_p = rtcp_p;
			rtcp_p    = (rtcp_t *) ((uint32 *) rtcp_p + ntohs(rtcp_p->common.length) + 1);
		} while (rtcp_p < (rtcp_t *) (out + *len) && rtcp_p->common.type == 2);
		if (current_p->common.p == 1) {
			current_p->common.p = 0;	/* Clear the padding bit. */
			pad = out[*len - 1];
			if (pad > 7 || pad == 0) {
				++badpbit_;
				return 0;
			}
			current_p->common.length = (uint16)htons((uint16)(((((ntohs(current_p->common.length)+1)*4)-pad)/4)-1));
			*len -= pad;
		}
		return pad+4;
	}
}

u_char* Encrypt_Ctrl( u_char* in, int* len)
/************************************************************************
* DESCRIPTION                                                           *
*                                                                       *
* This function encrypts control packets.  Four random bytes are added  *
* to the start of control packets to strengthen the encryption scheme.  *
* The control information is padded with zeros if the buffer size does  *
* not lie on an 8-octet boundary.                                       *
*                                                                       *
* INPUT PARAMETERS                                                      *
*                                                                       *
* u_char* in:  the input buffer to be encrypted                   	*
* int* len:          buffer length                                      *
*                                                                       *
* OUTPUT PARAMETERS                                                     *
*                                                                       *
* int* len:  this is to account for padding and the 4 random bytes      *
*                                                                       *
* RETURNS                                                               *
*                                                                       *
* A pointer to the encrypted control information is returned.           *
************************************************************************/
{
	int     pad;
	int     i;
	rtcp_t *rtcp_p;
	rtcp_t *current_p;
	u_char *padding;

	/* We are not using the IV */
	 u_char initVec[8] = {0,0,0,0,0,0,0,0};
       
	/* Attach 4 random bytes to the top of the header to reduce chances of a */
	/* plaintext attack on the otherwise fixed header.                       */
	unsigned int* new_random = (unsigned int*)wrkbuf_;
	new_random[0] = lbl_random();

	/* Copy into the working buffer */
	memcpy((wrkbuf_+4),in, *len);
	*len +=4;

	/* Pad with zeros to the nearest 8 octet boundary */	
	pad = *len & 7;
        if (pad != 0) {
                /* Pad to a block (8 octet) boundary */
                pad = 8 - pad;
		/* The following is a prime example why compound RTCP packets are a  */
		/* really dumb idea: We have to pad to a multiple of 8 bytes so that */
		/* the DES stuff works. Since compound RTCP packets are encrytped as */
		/* a whole, only the last packet in the compound gets the padding    */
		/* added to it (RFC1889). We must therefore parse the entire RTCP    */
		/* packet to find the start of the final packet, and then set the P  */
		/* bit and adjust the length field of that. If we only sent out RTCP */
		/* packets one-at-a-time, then the P bit and length fields would be  */
		/* a fixed distance into the packet, and this would be trivial. [csp]*/
		rtcp_p = (rtcp_t *) (wrkbuf_ + 4);
		do {
			current_p = rtcp_p;
			rtcp_p = (rtcp_t *) ((uint32 *) rtcp_p +
				 ntohs(rtcp_p->common.length) + 1);
		} while (rtcp_p < (rtcp_t *) (wrkbuf_ + *len) && 
			 rtcp_p->common.type == 2);
		current_p->common.p      = 1;	/* Set the padding bit. */
		current_p->common.length = 
				(uint16)htons((uint16)(((((ntohs(current_p->common.length)+1)*4)+
				pad)/4)-1));

		padding = (wrkbuf_ + *len);
		for (i=1; i<pad; i++) {
			*padding++ = 0;
		}
		*padding++ = (char)pad;
		*len += pad;
        }

	/* Carry out the encryption */
	qfDES_CBC_e(des_key, wrkbuf_, *len, initVec);
	return wrkbuf_;
}
