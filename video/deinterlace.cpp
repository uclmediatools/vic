#include "deinterlace.h"


extern "C"{
#include "cpu/cpudetect.h"
}

int cpu_flags = cpu_check();

Deinterlace::Deinterlace()
{
    context = NULL;
    width_ = height_ = 0;
    mode = 0;
}

Deinterlace::~Deinterlace()
{
    if(context){
 	pp_free_context(context);
	context = NULL;
    }
}


void Deinterlace::init(int width, int height)
{
    if(context){
	pp_free_context(context);
	context = NULL;
    }

    int flags = 0;
    flags |= (cpu_flags & FF_CPU_MMX ? PP_CPU_CAPS_MMX : 0);
    flags |= (cpu_flags & FF_CPU_MMXEXT ? PP_CPU_CAPS_MMX2 : 0);
    flags |= (cpu_flags & FF_CPU_3DNOW ? PP_CPU_CAPS_3DNOW : 0);
    flags |= (cpu_flags & FF_CPU_ALTIVEC ? PP_CPU_CAPS_ALTIVEC : 0);

    context = pp_get_context(width, height, PP_FORMAT_420 | flags);
    mode = pp_get_mode_by_name_and_quality("lb", 4);
}


void Deinterlace::render(void *buf, int width, int height)
{
	if(width != width_ || height != height_){
		init(width, height);
	}

	srcStride[0] = width;
	srcStride[1] = srcStride[2] = width/2;


	int framesize = width*height;
	src[0] = (uint8_t*)buf;
	src[1] = src[0] + framesize;
	src[2] = src[1] + framesize/4;


        pp_postprocess((const uint8_t**)src, srcStride,
                   src, srcStride,
                   width, height,
                   NULL,  0,
                   mode, context, 1);
}
