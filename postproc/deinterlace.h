#ifndef deinterlace_h
#define deinterlace_h

#include <string.h>

/*****************************************************************************
 * CPU capabilities
 *****************************************************************************/
#define CPU_CAPABILITY_NONE    0
#define CPU_CAPABILITY_486     (1<<0)
#define CPU_CAPABILITY_586     (1<<1)
#define CPU_CAPABILITY_PPRO    (1<<2)
#define CPU_CAPABILITY_MMX     (1<<3)
#define CPU_CAPABILITY_3DNOW   (1<<4)
#define CPU_CAPABILITY_MMXEXT  (1<<5)
#define CPU_CAPABILITY_SSE     (1<<6)
#define CPU_CAPABILITY_SSE2    (1<<7)
#define CPU_CAPABILITY_ALTIVEC (1<<16)
#define CPU_CAPABILITY_FPU     (1<<31)
//****************************************************************************
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
//****************************************************************************

#define DEINTERLACE_DISCARD 1
#define DEINTERLACE_MEAN    2
#define DEINTERLACE_BLEND   3
#define DEINTERLACE_BOB     4
#define DEINTERLACE_LINEAR  5

#ifndef WINDOWS
#define CAN_COMPILE_SSE
#define CAN_COMPILE_MMX
#endif
//****************************************************************************


static class Deinterlace{
  public:
    Deinterlace();
    static void Render(uint8_t  *source, int w, int h);
   
  protected:
    void init();
    void cpu_check();
    static void RenderBlend( uint8_t  *target, uint8_t  *source, int w, int h);  
    
}deinterlace;  // for gobal initialization

#endif
