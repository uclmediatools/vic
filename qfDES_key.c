/*****************************************************************************
Saleem N. Bhatti
February 1993
*****************************************************************************/

#if !defined(_qfDES_key_h_)
#define      _qfDES_key_c_

#include <stdlib.h>
#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#else
#include "config.h"
#endif
#include <string.h>
#include "config.h"
#include "qfDES.h"

extern char G_padChar; /* in qfDES.c */

/*
** This function sets bit 8 of each byte to odd or even parity as requested.
** It is assumed that the right-most bit of each byte is the parity bit.
** Althoug this is really only used by the two key generation functions below,
** it may be useful to someone.
*/

void
#if defined(__STDC__) || defined(__cplusplus)
qfDES_setParity(char *ptr, unsigned int size, const QFDES_parity parity)
#else
qfDES_setParity(ptr, size, parity)
char         *ptr;
unsigned int size;
QFDES_parity parity;
#endif
{
    unsigned int i, mask, bits;

    for(i = 0; i < size; ++i, ++ptr) {
        for(mask = 0x80, bits = 0; mask > 0x01; mask >>= 1)
            if (((unsigned int) *ptr) & mask) ++bits;

        *ptr |= bits % 2 == (unsigned int) parity ? 0x00 : 0x01;
    }
}

unsigned int
#if defined(__STDC__) || defined(__cplusplus)
qfDES_checkParity(char *ptr, unsigned int size, const QFDES_parity parity)
#else
qfDES_checkParity(ptr, size, parity)
char         *ptr;
unsigned int size;
QFDES_parity parity;
#endif
{
    unsigned int i, mask, bits, parityBit, parityErrors = 0;

    for(i = 0; i < size; ++i, ++ptr) {
        for(mask = 0x80, bits = 0; mask > 0x01; mask >>= 1)
            if (((unsigned int) *ptr) & mask) ++bits;

        parityBit = bits % 2 == (unsigned int) parity ? 0 : 1;

        if ((((unsigned int) *ptr) & 0x1) != parityBit)
            ++parityErrors;
    }

    return parityErrors;
}

/*
** For now, I simply take a string and set odd parity. Later, I will add the
** "proper" functions to allow the generation of DES keys from abritary text
** or byte string using MD5 or something equally flash like that.
*/

char *
#if defined(__STDC__) || defined(__cplusplus)
qfDES_str2key(char *str)
#else
qfDES_str2key(str)
char *str;
#endif
{
    static char key[8];
    char *kp = key;
    unsigned int i, l;

    l = strlen(str);
    if (l <= 8) {
        memcpy((void *) kp, (void *) str, l);
        for(i = l; i < 8; kp[i++] = G_padChar);
    }
    else
        memcpy((void *) kp, (void *) str, 8);

    for(i = 0; i < 8; kp[i++] <<= 1); /* shift bits up by one */
    qfDES_setParity(kp, 8, qfDES_odd);

    return (qfDES_checkWeakKeys(kp) ? (char *) 0 : kp);
}

static
char weakKeys[18][8] =
{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
 {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11},
 {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
 {0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe},
 {0x1f, 0x1f, 0x1f, 0x1f, 0x0e, 0x0e, 0x0e, 0x0e},
 {0xe0, 0xe0, 0xe0, 0xe0, 0xf1, 0xf1, 0xf1, 0xf1},
 {0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe},
 {0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01, 0xfe, 0x01},
 {0x1f, 0xe0, 0x1f, 0xe0, 0x0e, 0xf1, 0x0e, 0xf1},
 {0xe0, 0x1f, 0xe0, 0x1f, 0xf1, 0x0e, 0xf1, 0x0e},
 {0x01, 0xe0, 0x01, 0xe0, 0x01, 0xf1, 0x01, 0xf1},
 {0xe0, 0x01, 0xe0, 0x01, 0xf1, 0x01, 0xf1, 0x01},
 {0x1f, 0xfe, 0x1f, 0xfe, 0x0e, 0xfe, 0x0e, 0xfe},
 {0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x0e, 0xfe, 0x0e},
 {0x01, 0x1f, 0x01, 0x1f, 0x01, 0x0e, 0x01, 0x0e},
 {0x1f, 0x01, 0x1f, 0x01, 0x0e, 0x01, 0x0e, 0x01},
 {0xe0, 0xfe, 0xe0, 0xfe, 0xf1, 0xfe, 0xf1, 0xfe},
 {0xfe, 0xe0, 0xfe, 0xe0, 0xfe, 0xf1, 0xfe, 0xf1}};

/*
** Although this is really only used by the key generation function below,
** it may be handy to someone.
*/

int
#if defined(__STDC__) || defined(__cplusplus)
qfDES_checkWeakKeys(char *key)
#else
qfDES_checkWeakKeys(key)
char *key;
#endif
{
    char *bp;
    int i;

    for(bp = weakKeys[i = 0]; i < 18; bp = weakKeys[++i])
        if (memcmp((void *) key, (void *) bp, 8) == 0) return -1;

    return 0;
}

/*
** The following function attempts to genreate a random key or IV.
** It relies on the randomness of the  of the random(3) function. Although
** it is probably not particularly fast, keys and IV will most probably be
** generated off-line so it does not matter too much.
*/

char *
#if defined(__STDC__) || defined(__cplusplus)
qfDES_generate(const QFDES_generate what)
#else
qfDES_generate(what)
QFDES_generate what;
#endif
{
    static
    char buffer[8];
    static
    int flag = 0;

    char *bp;
    long mask = what == qfDES_key ? 0xfe : 0xff;

    /* Set up a seed - 42 is the answer ... */
#if 1
    if (!flag) {
        srandom((int) (getpid() * 42) ^ (int) time((time_t *) 0));
        flag = 1;
    }
#else
    flag = (int) (getpid() * 42) ^ (int) time((time_t *) 0) ^ flag;
    srandom(flag);
#endif
    do {

        for(bp = buffer; bp <= &(buffer[7]); *bp++ = (char) (random() & mask));

        if (what ==  qfDES_key)
            qfDES_setParity(buffer, 8, qfDES_odd);

    } while(what == qfDES_key ? qfDES_checkWeakKeys(buffer) : 0);

    return buffer;
}

#endif /* !_qfDES_key_c_ */
