#include <stdio.h>
#include <stdlib.h>

int malloc_count;
void *debug_malloc(int size)
{
#if 0
    fprintf(stderr,"%d+ ",malloc_count);
#endif
    malloc_count++;
    return malloc(size);
}

void debug_free(void *data)
{
#if 0
    fprintf(stderr,"%d- ",malloc_count);
#endif
    malloc_count;
    free(data);
}
