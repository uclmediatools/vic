/*****************************************************************************




Saleem N. Bhatti
February 1993
*****************************************************************************/

#if !defined(_qfDES_h_)
#define      _qfDES_h_

#if defined(__STDC__) || defined(__cplusplus)
#define _a_(_args_) _args_
#else
#define _a_(_args_) ()
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/* what */
typedef enum QFDES_what_e {qfDES_encrypt, qfDES_decrypt} QFDES_what;

/* mode */
typedef enum QFDES_mode_e {qfDES_ecb, qfDES_cbc, qfDES_cfb, qfDES_ofb} QFDES_mode;

/* parity */
typedef enum QFDES_parity_e {qfDES_even, qfDES_odd} QFDES_parity;

/* key/IV generation */
typedef enum QFDES_generate_e {qfDES_key, qfDES_iv} QFDES_generate;

/* This does it all */
int qfDES _a_((char *key, char *data, unsigned int size, const QFDES_what what,
          const QFDES_mode mode, char *initVec));

/* Handy macros */
#define qfDES_ECB_e(_key, _data, _size) \
    qfDES(_key, _data, _size, qfDES_encrypt, qfDES_ecb, (char *) 0)

#define qfDES_ECB_d(_key, _data, _size) \
    qfDES(_key, _data, _size, qfDES_decrypt, qfDES_ecb, (char *) 0)

#define qfDES_CBC_e(_key, _data, _size, _initVec) \
    qfDES(_key, _data, _size, qfDES_encrypt, qfDES_cbc, _initVec)

#define qfDES_CBC_d(_key, _data, _size, _initVec) \
    qfDES(_key, _data, _size, qfDES_decrypt, qfDES_cbc, _initVec)

#define qfDES_CFB_e(_key, _data, _size, _initVec) \
    qfDES(_key, _data, _size, qfDES_encrypt, qfDES_cfb, _initVec)

#define qfDES_CFB_d(_key, _data, _size, _initVec) \
    qfDES(_key, _data, _size, qfDES_decrypt, qfDES_cfb, _initVec)

#define qfDES_OFB_e(_key, _data, _size, _initVec) \
    qfDES(_key, _data, _size, qfDES_encrypt, qfDES_ofb, _initVec)

#define qfDES_OFB_d(_key, _data, _size, _initVec) \
    qfDES(_key, _data, _size, qfDES_decrypt, qfDES_ofb, _initVec)

/* Padded [m|re]alloc() */
char    qfDES_setPad _a_((char pad));
#define qfDES_padSpace() qfDES_setPad((char) ' ')
#define qfDES_padZero() qfDES_setPad((char) '\0')

unsigned int qfDES_insertPadding _a_((char *ptr, unsigned int size));
unsigned int qfDES_malloc _a_((char **ptr, unsigned int size));
unsigned int qfDES_realloc _a_((char **ptr, unsigned int size));
void         qfDES_free _a_((char *ptr)); /* Just a wrap-up of free(3) */
char         *qfDES_copy _a_((char *ptr, unsigned int size));

/* The size of text in a qfDES_malloc()ed block */
#define qfDES_plainTextSize(_ptr, _size) \
    (unsigned int) ((_size) - (unsigned int) (_ptr)[(_size) - 1])

/* Converting between binary and hexdump */
unsigned int qfDES_bin2hex _a_((char *binText, char *hexText, unsigned int size));
unsigned int qfDES_hex2bin _a_((char *hexText, char *binText, unsigned int size));

/* Keys */
void qfDES_setParity _a_((char *ptr, unsigned int size, const QFDES_parity parity));
unsigned int qfDES_checkParity _a_((char *ptr, unsigned int size, const QFDES_parity parity));

char *qfDES_str2key _a_((char *str)); /* returns ptr to static memory */
char *qfDES_generate _a_((const QFDES_generate what)); /* returns a pointer to
                                                          static memory */
#define qfDES_generateKey() qfDES_generate(qfDES_key)
#define qfDES_generateIV() qfDES_generate(qfDES_iv)

int qfDES_checkWeakKeys _a_((char *key));

#if defined(__cplusplus)
}
#endif

#endif /* !_qfDES_h_ */
