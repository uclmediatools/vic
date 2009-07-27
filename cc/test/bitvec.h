/*
 * bitvec.h
 */

/* $Id$ */

#ifndef vic_cc_test_bitvec_h
#define vic_cc_test_bitvec_h

#include <stdbool.h>		// boolean type and values
#include "buffer.h"

#define DUPACKS 3		// dupacks
#define CHB 0x80000000  // ackvec check bit (head search)
#define CTB 0x01        // ackvec check bit (tail search)

// set AckVec bitmap from LSB
#define SET_BIT_VEC(map, val) (map = ((map << 1) | val))

// AckVec bitmap at i-th location
#define GET_BIT_VEC(map, i, val) ((1 << (val - i)) & map)

// AckVec head search
#define GET_HEAD_VEC(map, i) ( map & (CHB >> i) )

// AckVec tail search
#define GET_TAIL_VEC(map, i) ( map & (CTB << i) )

// check bit at i-th location
#define CHECK_BIT_AT(map, i) ( map & (1 << (i-1)) )

// set bit at i-th location
#define SET_BIT_AT(map, i) ( map |= 1 << (i-1) )

// clear bit at i-th location
#define CLR_BIT_AT(map, i) ( map &= ~(1 << (i-1)) )

// functions
int		bitvec();
int		get_head_pos (int vec);
int		get_tail_pos (int vec);
void	marginvec (int vec);
void	send_buf (Buffer, int);
void	trimvec (int vec, int);
int		ackofack ();

#endif /* vic_cc_test_bitvec_h */
