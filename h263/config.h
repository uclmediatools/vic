
/* 
 * QCIF decodes only the lower resolution, 
 * yielding a smaller image, unless EXPANDed 
 */
#ifndef SIZE_QCIF
# define SIZE_QCIF 0
#endif

#ifndef LOOP
# define LOOP 0
#endif

#ifdef WITHOUT_AUDIO
# undef AUDIO
# define AUDIO 0
#endif
#ifndef AUDIO
# define AUDIO 1
#endif
