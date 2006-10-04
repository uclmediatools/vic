#ifndef CONFIG_ARCH_H
#define CONFIG_ARCH_H 
#define __CPU__ 586
//#define USE_FASTMEMCPY 1
#define HAVE_SSE2 1
#define HAVE_SSE 1
#define HAVE_MMX2 1
#define HAVE_MMX 1

#ifdef HAVE_MMX
#define USE_MMX_IDCT 1
#endif

#ifndef WINDOWS
#define HAVE_XVIDEO
#define HAVE_SHM
#endif

#undef HAVE_DIRECTX

#endif
