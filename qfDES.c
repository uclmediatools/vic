/*****************************************************************************

Saleem N. Bhatti
February 1993

Patch for Intel/Linux courtesy of Mark Handley & George Pavlou
Added 2 August 1996, Saleem
*****************************************************************************/

#if !defined(_qfDES_c_)
#define      _qfDES_c_

#if defined(__i386__)
#include <sys/types.h>

#ifndef WIN32
#include <netinet/in.h>

#else

#include "../config.h"

#endif
#endif

typedef unsigned long Word;

#include "qfDES.h"
#include "qfDES_aux.h"

#define INITIAL_PERMUTATION(t, regL, regR) \
        INITIAL_PERMUTATION_AUX(t[0], t[1], regL, regR)

#define FINAL_PERMUTATION(regR, regL, t) \
        FINAL_PERMUTATION_AUX(regR, regL, t[0], t[1])

#define PC1(k, regC, regD) \
        PC1_AUX(k[0], k[1], regC, regD)

#define PC2(regC, regD, k) \
        PC2_AUX(regC, regD, k[0], k[1])

extern char G_padChar; /* Default pad charcater */

static
Word rol[16][3] =
{ /* For rotating left */
{0x80000000, 27, 1}, {0x80000000, 27, 1},
{0xc0000000, 26, 2}, {0xc0000000, 26, 2},
{0xc0000000, 26, 2}, {0xc0000000, 26, 2},
{0xc0000000, 26, 2}, {0xc0000000, 26, 2},
{0x80000000, 27, 1}, {0xc0000000, 26, 2},
{0xc0000000, 26, 2}, {0xc0000000, 26, 2},
{0xc0000000, 26, 2}, {0xc0000000, 26, 2},
{0xc0000000, 26, 2}, {0x80000000, 27, 1}
};

#define ROTATE_LEFT(v, rot) \
v = ((v & rot[0]) >> rot[1]) | v << rot[2] 

static
Word ror[16][3] =
{ /* For rotating right */
{0x00000000,  0, 0}, {0x00000010, 27, 1},
{0x00000030, 26, 2}, {0x00000030, 26, 2},
{0x00000030, 26, 2}, {0x00000030, 26, 2},
{0x00000030, 26, 2}, {0x00000030, 26, 2},
{0x00000010, 27, 1}, {0x00000030, 26, 2},
{0x00000030, 26, 2}, {0x00000030, 26, 2},
{0x00000030, 26, 2}, {0x00000030, 26, 2},
{0x00000030, 26, 2}, {0x00000010, 27, 1}
};

#define ROTATE_RIGHT(v, rot) \
v = ((v & rot[0]) << rot[1]) | v >> rot[2]

#define K_ENCRYPT(regC, regD, ik) \
{ \
    register int z; \
    register Word *rl, *k = ik; \
    for(rl = rol[z = 0]; z < 16; rl = rol[++z], k += 2) { \
        ROTATE_LEFT(regC, rl); \
        ROTATE_LEFT(regD, rl); \
        PC2(regC, regD, k); \
    } \
}

#define K_DECRYPT(regC, regD, ik) \
{ \
    register int z; \
    register Word *rr, *k = ik; \
    for(rr = ror[z = 0]; z < 16; rr = ror[++z], k += 2) { \
        ROTATE_RIGHT(regC, rr); \
        ROTATE_RIGHT(regD, rr); \
        PC2(regC, regD, k); \
    } \
}

/*
** The S Box transformations and the permutation P are combined in the vectors
** s_p0 - s_p7. Permutation E and the MOD 2 addition with the intermediate key
** are then done "inline" on each round. The intermediate key is already in a
** a 8x6bit form because of the modified permuation PC2 used.
*/

#if defined(__i386__)

#define DES(t, ik) \
{ \
    register Word l, r, reg32, round; \
    register char *bb; \
    INITIAL_PERMUTATION(t, l, r); \
    for(bb = (char *) ik, round = 0x8000; round; bb += 8, round >>= 1) { \
        register Word w = (r << 1) | (r >> 31); \
        reg32  = s_p7[( w        & 0x3f) ^ bb[4]]; \
        reg32 |= s_p6[((w >>= 4) & 0x3f) ^ bb[5]]; \
        reg32 |= s_p5[((w >>= 4) & 0x3f) ^ bb[6]]; \
        reg32 |= s_p4[((w >>= 4) & 0x3f) ^ bb[7]]; \
        reg32 |= s_p3[((w >>= 4) & 0x3f) ^ bb[0]]; \
        reg32 |= s_p2[((w >>= 4) & 0x3f) ^ bb[1]]; \
        reg32 |= s_p1[((w >>  4) & 0x3f) ^ bb[2]]; \
        reg32 |= s_p0[(((r & 0x1) << 5) | ((r & 0xf8000000) >> 27)) ^ bb[3]]; \
        reg32 ^= l; \
        l = r; \
        r = reg32; \
    } \
    FINAL_PERMUTATION(r, l, t); \
}


#define LITTLE_ENDIAN(t, s) \
{ \
    register unsigned int z, l = s/4; \
    register Word *tp = (Word *) t; \
    for(z = 0; z < l; ++z) tp[z] = htonl(tp[z]); \
}

#else /* __i386__ */

#define DES(t, ik) \
{ \
    register Word l, r, reg32, round; \
    register char *bb; \
    INITIAL_PERMUTATION(t, l, r); \
    for(bb = (char *) ik, round = 0x8000; round; bb += 8, round >>= 1) { \
        register Word w = (r << 1) | (r >> 31); \
        reg32  = s_p7[( w        & 0x3f) ^ bb[7]]; \
        reg32 |= s_p6[((w >>= 4) & 0x3f) ^ bb[6]]; \
        reg32 |= s_p5[((w >>= 4) & 0x3f) ^ bb[5]]; \
        reg32 |= s_p4[((w >>= 4) & 0x3f) ^ bb[4]]; \
        reg32 |= s_p3[((w >>= 4) & 0x3f) ^ bb[3]]; \
        reg32 |= s_p2[((w >>= 4) & 0x3f) ^ bb[2]]; \
        reg32 |= s_p1[((w >>  4) & 0x3f) ^ bb[1]]; \
        reg32 |= s_p0[(((r & 0x1) << 5) | ((r & 0xf8000000) >> 27)) ^ bb[0]]; \
        reg32 ^= l; \
        l = r; \
        r = reg32; \
    } \
    FINAL_PERMUTATION(r, l, t); \
}

#endif /* __i386__ */

int
#if defined(__STDC__) || defined(__cplusplus)
qfDES(char         *key,
      char         *data,
      unsigned int size,
const QFDES_what   what,
const QFDES_mode   mode,
      char         *initVec)
#else
qfDES(key, data, size, what, mode, initVec)
char         *key;
char         *data;
unsigned int size;
QFDES_what   what;
QFDES_mode   mode;
char         *initVec;
#endif
{
    /* Store some info to optimise for multiple calls ... */
    static
    char desKey[8],
         desKeys[128];

    static
    Word *oldKey = (Word *) desKey,
         *keys   = (Word *) desKeys;

    static
    QFDES_what oldWhat;

    static
    QFDES_mode oldMode;

    char b0[8], b1[8]; /* feedback blocks */

    register
    Word *newKey = (Word *) key, /* key from user */
         *text,                  /* text to be [en|de]crypted */
         *cb     = (Word *) b0,  /* the chained block in CBC mode */
         *cb1    = (Word *) b1;  /* a copy for use when decrypting */

#if defined(__i386__)
    unsigned int origSize = size;
    LITTLE_ENDIAN(key, 8);
    LITTLE_ENDIAN(data, origSize);
#endif

    /*
    ** Check new key against old key
    ** and set up intermediate keys.
    */
    if (newKey[0] != oldKey[0] || newKey[1] != oldKey[1]) {

        /* all new */
        register
        Word c, d;  /* C and D registers */

        oldKey[0] = newKey[0]; oldKey[1] = newKey[1];
        oldWhat = what;
        oldMode = mode;

        PC1(newKey, c, d);

        if ((what == qfDES_encrypt) ||
            (mode == qfDES_cfb) || (mode == qfDES_ofb)) /* Always encrypt */
            { K_ENCRYPT(c, d, keys); }
        else
            { K_DECRYPT(c, d, keys); }
    }

    else if ((what != oldWhat) &&
             ((mode == qfDES_ecb) || (mode == qfDES_cbc))) {

        /*
        ** Same key but different direction.
        ** Reverse intermediate key sequence (ECB and CBC).
        */
        Word *ik1, *ik2, ik3[2];

        for(ik1 = keys, ik2 = &(keys[30]); ik1 < ik2; ik1 += 2, ik2 -= 2) {
            ik3[0] = ik1[0]; ik3[1] = ik1[1];
            ik1[0] = ik2[0]; ik1[1] = ik2[1];
            ik2[0] = ik3[0]; ik2[1] = ik3[1];
        }

        oldWhat = what;
    }

    /* Set up initilaisation vector */
    if (mode != qfDES_ecb) {

        if (initVec)
            { cb[0] = ((Word *) initVec)[0]; cb[1] = ((Word *) initVec)[1]; }
        else
            cb[0] = cb[1] = 0;

#if defined(__i386__)
        LITTLE_ENDIAN(cb, 8);
#endif
    }

    /*
    ** Lots of gotos and code duplication follow (arrgh) but it speeds
    ** it up a wee bit!
    ** What would be more useful is looking more carefully at the DES
    ** permutations to produce more efficient versions of the macros
    ** of the "auto-generated" versions used in qfDES-aux.c.
    */

    size >>= 3; /* this is always a multpile of 8 */

    if (what == qfDES_encrypt) {
        switch ((int) mode) {
        case ((int) qfDES_ecb): goto _ECB_;
        case ((int) qfDES_cbc): goto _CBC_encrypt_;
        case ((int) qfDES_cfb): goto _CFB_encrypt_;
        case ((int) qfDES_ofb): goto _OFB_;
        }
    }
    else {
        switch ((int) mode) {
        case ((int) qfDES_ecb): goto _ECB_;
        case ((int) qfDES_cbc): goto _CBC_decrypt_;
        case ((int) qfDES_cfb): goto _CFB_decrypt_;
        case ((int) qfDES_ofb): goto _OFB_;
        }
    }

_ECB_:

    /* ECB */
    for(text = (Word *) data; size; --size, text += 2)
        { DES(text, keys); }

    goto _exit_qfDES_;

_CBC_encrypt_:

    /* CBC Encryption */
    for(text = (Word *) data; size; --size, text += 2) {

        /* chaining block */
        text[0] ^= cb[0]; text[1] ^= cb[1];

        DES(text, keys);

        /* set up chaining block for next round */
        cb[0] = text[0]; cb[1] = text[1];
    }

    goto _initVec_;

_CBC_decrypt_:

    /* CBC Decryption */
    for(text = (Word *) data; size; --size, text += 2) {

        /* set up chaining block */
        /*
        ** The decryption is done in place so I need
        ** to copy this text block for the next round.
        */
        cb1[0] = text[0]; cb1[1] = text[1];

        DES(text, keys);

        /* chaining block for next round */
        text[0] ^= cb[0]; text[1] ^= cb[1];

        /*
        ** Copy back the saved encrypted text - this makes
        ** CBC decryption slower than CBC encryption.
        */
        cb[0] = cb1[0]; cb[1] = cb1[1];
    }

    goto _initVec_;

_CFB_encrypt_:

    /* CFB Encryption */
    for(text = (Word *) data; size; --size, text += 2) {

        /* use cb as the feedback block */
        DES(cb, keys);

        text[0] ^= cb[0]; text[1] ^= cb[1];

        /* set up feedback block for next round */
        cb[0] = text[0]; cb[1] = text[1];
    }

    goto _initVec_;

_CFB_decrypt_:

    /* CFB Decryption */
    for(text = (Word *) data; size; --size, text += 2) {

        /* set up feedback block */
        /*
        ** The decryption is done in place so I need
        ** to copy this text block for the next round.
        */
        cb1[0] = text[0]; cb1[1] = text[1];

        /* use cb as the feedback block */
        DES(cb, keys);

        text[0] ^= cb[0]; text[1] ^= cb[1];

        /* set up feedback block for next round */
        cb[0] = cb1[0]; cb[1] = cb1[1];
    }

    goto _initVec_;

_OFB_:

    /* OFB */
    for(text = (Word *) data; size; --size, text += 2) {

        /* use cb as the feed back block */
        DES(cb, keys);

        text[0] ^= cb[0]; text[1] ^= cb[1];
    }

_initVec_:

    /*
    ** Copy the final chained block back to initVec (CBC, CFB and OFB).
    ** This allows the [en|de]cryption of large amounts of data in small
    ** chunks.
    */
    if (initVec) {
        ((Word *) initVec)[0] = cb[0];
        ((Word *) initVec)[1] = cb[1];

#if defined(__i386__)
        LITTLE_ENDIAN(initVec, 8);
#endif
    }

_exit_qfDES_:

#if defined(__i386__)
    LITTLE_ENDIAN(key, 8);
    LITTLE_ENDIAN(data, origSize);
#endif

    return 0;
}

#endif /* !_qfDES_c_ */
