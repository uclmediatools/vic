#ifndef deinterlace_h
#define deinterlace_h

#include <string.h>

typedef unsigned char uint8_t;
typedef  char int8_t;

#include "cpu/cpudetect.h"
#include <postproc/postprocess.h>

// only used for YUV420
class Deinterlace
{
  public:
    Deinterlace();
	~Deinterlace();
    void init(int width, int height);
	void render(void *buf, int width, int height);

  private:
    int width_, height_;
	pp_mode_t	*mode;
	pp_context_t	*context;
	int srcStride[3];
	uint8_t *src[3];

};			

#endif
