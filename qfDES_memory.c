/*****************************************************************************
Saleem N. Bhatti
February 1993
*****************************************************************************/

#if !defined(_qfDES_memory_c_)
#define      _qfDES_memory_c_

#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <ctype.h>
#include "config.h"
#include "qfDES.h"

#ifndef DEFINED_ERRLIST
 extern int sys_nerr;
 extern char *sys_errlist[];
#endif
extern int errno;


#if defined(DEBUG)
#define DES_ERROR(s_) \
    fprintf(stderr, "%s: errno %d - %s [%s line %d]\n", s_, errno, errno < sys_nerr ? sys_errlist[errno] : "???", __FILE__, __LINE__)
#else
#define DES_ERROR(s_) \
    fprintf(stderr, "%s: errno %d - %s\n", s_, errno, errno < sys_nerr ? sys_errlist[errno] : "???")
#endif

char G_padChar; /* in qfDES.c */

char
#if defined(__STDC__) || defined(__cplusplus)
qfDES_setPad(char pad)
#else
qfDES_setPad(pad)
char pad;
#endif
{ char b = G_padChar; G_padChar = pad; return b; }

unsigned int
#if defined(__STDC__) || defined(__cplusplus)
qfDES_malloc(char **ptr, unsigned int size)
#else
qfDES_malloc(ptr, size)
char **ptr;
unsigned int size;
#endif
{
    register
    unsigned int pad, s;

    register
    char *bp;

    pad = 8 - (size % 8);
    s = size + pad;

    if ((bp = (char *) malloc(s)) != (char *) 0) {

        /* pad with spaces */
        qfDES_insertPadding(bp, size);
        *ptr = bp;
    }
    else {
        DES_ERROR("qfDES_malloc");
        s = 0;
        *ptr = (char *) 0;
    }

    return s;
}

unsigned int
#if defined(__STDC__) || defined(__cplusplus)
qfDES_realloc(char **ptr, unsigned int size)
#else
qfDES_realloc(ptr, size)
char **ptr;
unsigned int size;
#endif
{
    register
    unsigned int pad, s;

    char *bp;

    pad = 8 - (size % 8);
    s = size + pad;

    if ((bp = (char *) realloc((char *) *ptr, s)) != (char *) 0) {

        /* pad it */
        qfDES_insertPadding(bp, size);
        *ptr = bp;
    }
    else {
        DES_ERROR("qfDES_realloc()");
        s = 0;
    }

    return s;
}

unsigned int
#if defined(__STDC__) || defined(__cplusplus)
qfDES_insertPadding(char *ptr, unsigned int size)
#else
qfDES_insertPadding(ptr, size)
char         *ptr;
unsigned int size;
#endif
{
    register
    unsigned int pad, s;

    pad = 8 - (size % 8);
    s = size + pad;

    /* pad the buffer */
    memset((void *) &(ptr[size]), G_padChar, (pad - 1));
    ptr[s - 1] = (char) pad;

    return s;
}

void
#if defined(__STDC__) || defined(__cplusplus)
qfDES_free(char *ptr)
#else
qfDES_free(ptr)
char *ptr;
#endif
{ if (ptr) free((char *) ptr); }

unsigned int
#if defined(__STDC__) || defined(__cplusplus)
qfDES_bin2hex(char *binText, char *hexText, unsigned int size)
#else
qfDES_bin2hex(binText, hexText, size)
char *binText;
char *hexText;
unsigned int size;
#endif
{
    register
    unsigned int i, s;
    register
    char *bp, *hp;

    s = (size * 2) + 1;

    for(i = 0, bp = binText, hp = hexText; i < size; ++i, ++bp, hp += 2)
        sprintf(hp, "%02x", (unsigned int) ((unsigned char) *bp));

    return s;
}

unsigned int
#if defined(__STDC__) || defined(__cplusplus)
qfDES_hex2bin(char *hexText, char *binText, unsigned int size)
#else
qfDES_hex2bin(hexText, binText, size)
char *hexText;
char *binText;
unsigned int size;
#endif
{
    register
    unsigned int i, s;
    register
    char *bp, *hp;

    unsigned long v;

    s = size/2;

    for(i = 0, hp = hexText, bp = binText; i < s; ++i, hp += 2, ++bp) {

        /* Check that digits in string are OK */
        if (!isxdigit((unsigned int) ((unsigned char) hp[0])) ||
            !isxdigit((unsigned int) ((unsigned char) hp[1])))
            return i;

        sscanf(hp, "%02lx", &v);
        *bp = (char) v;
    }

    return s;
}

char *
#if defined(__STDC__) || defined(__cplusplus)
qfDES_copy(char *ptr, unsigned int size)
#else
qfDES_copy(ptr, size)
char *ptr;
unsigned int size;
#endif
{
    register
    char *bp = (char *) malloc(size);

    if (bp != (char *) 0)
        memcpy((void *) bp, (void *) ptr, size);
    else
        DES_ERROR("qfDES_copy()");

    return bp;
}

#endif /* !_qfDES_memory_c_ */
