#include "deinterlace.h"


/*****************************************************************************
 * Local protypes
 *****************************************************************************/
static void MergeGeneric ( void *, const void *, const void *, size_t );
#if defined(CAN_COMPILE_C_ALTIVEC)
static void MergeAltivec ( void *, const void *, const void *, size_t );
#endif
#if defined(CAN_COMPILE_MMX)
static void MergeMMX     ( void *, const void *, const void *, size_t );
#endif
#if defined(CAN_COMPILE_SSE)
static void MergeSSE2    ( void *, const void *, const void *, size_t );
#endif
#if defined(CAN_COMPILE_MMX) || defined(CAN_COMPILE_SSE)
static void EndMMX       ( void );
#endif

// ***************************************************************
static void (*Merge) ( void *, const void *, const void *, size_t );
static void (*EndMerge) ( void );          
static uint8_t target[1280*1024*3/2];  
static int cpu_info;   
 
Deinterlace::Deinterlace(){
  cpu_check();
  init();
}

void Deinterlace::cpu_check() { 
  cpu_info =  CPU_CAPABILITY_SSE2 ; 
}

void Deinterlace::init(){

#if defined(CAN_COMPILE_C_ALTIVEC)
    if( cpu_info & CPU_CAPABILITY_ALTIVEC )
    {
        Merge = MergeAltivec;
        EndMerge = NULL;
    }
#endif
#if defined(CAN_COMPILE_SSE)
    if( cpu_info & CPU_CAPABILITY_SSE2 )
    {
        Merge = MergeSSE2;
        EndMerge = EndMMX;
    }
    else
#endif
#if defined(CAN_COMPILE_MMX)
    if( cpu_info & CPU_CAPABILITY_MMX )
    {
        Merge = MergeMMX;
        EndMerge = EndMMX;
    }
    else
#endif
    {
        Merge = MergeGeneric;
        EndMerge = NULL;
    }	
}


static void MergeGeneric( void *_p_dest, const void *_p_s1,
                          const void *_p_s2, size_t i_bytes )
{
    uint8_t* p_dest = (uint8_t*)_p_dest;
    const uint8_t *p_s1 = (const uint8_t *)_p_s1;
    const uint8_t *p_s2 = (const uint8_t *)_p_s2;
    uint8_t* p_end = p_dest + i_bytes - 8;

    while( p_dest < p_end )
    {
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
    }

    p_end += 8;

    while( p_dest < p_end )
    {
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
    }
}

#if defined(CAN_COMPILE_MMX)
static void MergeMMX( void *_p_dest, const void *_p_s1, const void *_p_s2,
                      size_t i_bytes )
{
    uint8_t* p_dest = (uint8_t*)_p_dest;
    const uint8_t *p_s1 = (const uint8_t *)_p_s1;
    const uint8_t *p_s2 = (const uint8_t *)_p_s2;
    uint8_t* p_end = p_dest + i_bytes - 8;
    while( p_dest < p_end )
    {
        __asm__  __volatile__( "movq %2,%%mm1;"
                               "pavgb %1, %%mm1;"
                               "movq %%mm1, %0" :"=m" (*p_dest):
                                                 "m" (*p_s1),
                                                 "m" (*p_s2) );
        p_dest += 8;
        p_s1 += 8;
        p_s2 += 8;
    }

    p_end += 8;

    while( p_dest < p_end )
    {
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
    }
}
#endif

#if defined(CAN_COMPILE_SSE)
static void MergeSSE2( void *_p_dest, const void *_p_s1, const void *_p_s2,
                       size_t i_bytes )
{
    uint8_t* p_dest = (uint8_t*)_p_dest;
    const uint8_t *p_s1 = (const uint8_t *)_p_s1;
    const uint8_t *p_s2 = (const uint8_t *)_p_s2;
    while( (int)p_s1 % 16 )
    {
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
    }
    uint8_t* p_end = p_dest + i_bytes - 16;
    while( p_dest < p_end )
    {
        __asm__  __volatile__( "movdqu %2,%%xmm1;"
                               "pavgb %1, %%xmm1;"
                               "movdqu %%xmm1, %0" :"=m" (*p_dest):
                                                 "m" (*p_s1),
                                                 "m" (*p_s2) );
        p_dest += 16;
        p_s1 += 16;
        p_s2 += 16;
    }

    p_end += 16;

    while( p_dest < p_end )
    {
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
    }
}
#endif

#if defined(CAN_COMPILE_MMX) || defined(CAN_COMPILE_SSE)
static void EndMMX( void )
{
    __asm__ __volatile__( "emms" );
}
#endif

#ifdef CAN_COMPILE_C_ALTIVEC
static void MergeAltivec( void *_p_dest, const void *_p_s1,
                          const void *_p_s2, size_t i_bytes )
{
    uint8_t *p_dest = (uint8_t*)_p_dest;
    const uint8_t *p_s1 = (const uint8_t *)_p_s1;
    const uint8_t *p_s2 = (const uint8_t *)_p_s2;
    uint8_t *p_end = p_dest + i_bytes - 16;

    if( ( (int)p_s1 & 0xF ) | ( (int)p_s2 & 0xF ) |
        ( (int)p_dest & 0xF ) )
    {
        /* TODO Handle non 16-bytes aligned planes */
        MergeGeneric( _p_dest, _p_s1, _p_s2, i_bytes );
        return;
    }

    while( p_dest < p_end )
    {
        vec_st( vec_avg( vec_ld( 0, p_s1 ), vec_ld( 0, p_s2 ) ),
                0, p_dest );
        p_s1   += 16;
        p_s2   += 16;
        p_dest += 16;
    }

    p_end += 16;

    while( p_dest < p_end )
    {
        *p_dest++ = ( (uint16_t)(*p_s1++) + (uint16_t)(*p_s2++) ) >> 1;
    }
}
#endif

//*******************************************************************
void Deinterlace::RenderBlend( uint8_t  *target, uint8_t  *source, int w, int h)
{
    int i_plane;

    /* Copy image and skip lines */
    for( i_plane = 0 ; i_plane < 3 ; i_plane++ )
    {
       if(i_plane == 1){
         w /= 2;
         h /= 2;
       }
       int framesize = w*h;
 
       /* First line: simple copy */
       memcpy( target, source, w );
       uint8_t *p_in   = source;
       uint8_t *p_out = target+w;
       uint8_t *p_out_end = target + framesize;
       
       /* Remaining lines: mean value */
       for( ; p_out < p_out_end ; ){
              Merge( p_out, p_in, p_in + w, w);
              p_out += w;
              p_in += w;
       }
       source += framesize;
       target  += framesize;
    }
    if(EndMerge) EndMerge();    
}

void Deinterlace::Render(uint8_t  *source, int w, int h){       
  RenderBlend(target, source, w, h);
  memcpy(source, target, w*h*3/2);		
}
