#include "config.h"

#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#endif

extern "C"{
#include "cpu/cpudetect.h"
}
#include <stdio.h>

static CpuCaps aCpuCaps;
static int available_cpu_flags;

#ifndef WIN64
extern "C"
{
 // cpu_flag detection helper functions 
 extern int check_cpu_features(void);
 extern void sse_os_trigger(void);
 extern void sse2_os_trigger(void);
}


 #ifdef __GNUC__
  #include <signal.h>
  #include <setjmp.h>
  static jmp_buf mark;
  static void sigill_handler(int signal)
   {
    longjmp(mark, 1);
   }
  static int sigill_check(void (*func)())
   {
    void (*old_handler)(int);
    int jmpret;
    old_handler=signal(SIGILL,sigill_handler);
    if (old_handler==SIG_ERR)
     return -1;
  
    jmpret=setjmp(mark);
    if (jmpret==0)
     func();
    signal(SIGILL,old_handler);
    return jmpret;
   }
 #else
  static int sigill_check(void (*func)())
   {
    __try 
     {
      func();
     }
    __except(EXCEPTION_EXECUTE_HANDLER) 
     {
      if (_exception_code()==STATUS_ILLEGAL_INSTRUCTION)
       return 1;
     }
    return 0;
   }
 #endif
#endif 


int cpu_check(){

#ifdef __GNUC__
	/* CPU capabilities detection */
	GetCpuCaps(&aCpuCaps);
    available_cpu_flags=(aCpuCaps.hasMMX  ? FF_CPU_MMX|FF_CPU_MMXEXT:0)|
                       (aCpuCaps.has3DNow ? FF_CPU_3DNOW|FF_CPU_3DNOWEXT:0)|
                       (aCpuCaps.hasSSE   ? FF_CPU_SSE:0)|
                       (aCpuCaps.hasSSE2  ? FF_CPU_SSE2:0);

#elif defined(WIN32) 
   available_cpu_flags=check_cpu_features();
   if ((available_cpu_flags&FF_CPU_SSE) && sigill_check(sse_os_trigger))
    available_cpu_flags&=~FF_CPU_SSE;
   if ((available_cpu_flags&FF_CPU_SSE2) && sigill_check(sse2_os_trigger))
    available_cpu_flags&=~FF_CPU_SSE2;

#elif defined(WIN64)
   available_cpu_flags=(IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE)?FF_CPU_MMX|FF_CPU_MMXEXT:0)|
                       (IsProcessorFeaturePresent(PF_3DNOW_INSTRUCTIONS_AVAILABLE)?FF_CPU_3DNOW|FF_CPU_3DNOWEXT:0)|
                       (IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE)?FF_CPU_SSE:0)|
                       (IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE)?FF_CPU_SSE2:0);
   #ifdef __INTEL_COMPILER
   available_cpu_flags|=FF_CPU_MMX|FF_CPU_MMXEXT;
   #endif    

#endif

   aCpuCaps.hasMMX		= (available_cpu_flags & FF_CPU_MMX ? 1:0);
   aCpuCaps.hasMMX2		= (available_cpu_flags & FF_CPU_MMXEXT ? 1:0);
   aCpuCaps.hasSSE		= (available_cpu_flags & FF_CPU_SSE ? 1:0);
   aCpuCaps.hasSSE2		= (available_cpu_flags & FF_CPU_SSE2 ? 1:0);
   aCpuCaps.has3DNow	= (available_cpu_flags & FF_CPU_3DNOW ? 1:0);
   aCpuCaps.has3DNowExt	= (available_cpu_flags & FF_CPU_3DNOWEXT ? 1:0);

   debug_msg("cpudetect: MMX=%d MMX2=%d SSE=%d SSE2=%d 3DNow=%d 3DNowExt=%d\n",  \
	        aCpuCaps.hasMMX, aCpuCaps.hasMMX2, aCpuCaps.hasSSE, aCpuCaps.hasSSE2, \
	       	aCpuCaps.has3DNow, aCpuCaps.has3DNowExt );
   return available_cpu_flags;
}