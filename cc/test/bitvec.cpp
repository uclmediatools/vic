/*
 * bitvec.cpp
 */

/* $Id$ */

#include <stdio.h>		// standard input/output
#include <stdint.h>		// integer with certain length
#include <stdbool.h>	// boolean type and values
#include <sys/types.h>	// data types
#include "bitvec.h"

// variables
static u_int32_t map = 0;	// bit vector
static int seqno	 = 0;	// received seqno
static int ackofack	 = 0;	// ack of ack
static int lastseq	 = 0;	// last seqno
static int mvec[DUPACKS];	// margin vec

int bitvec () {

	return 0;
}

int get_head_pos (int vec) {
	int l;
	for (l = 0; l < 32; l++) {
		if (GET_HEAD_VEC(vec, l))
			break;
	}
	return (32 - l);
}

int get_tail_pos (int vec) {
	int l;
	for (l = 0; l < 32; l++) {
		if (GET_TAIL_VEC(vec, l))
			break;
	}
	return (l + 1);
}

void marginvec (int vec) {
	int hseq = get_head_pos(vec) + ackofack;

	for (int i = 0; i < DUPACKS; i++) 
		mvec[i] = ((hseq - i) < 0) ? 0 : (hseq - i);
}
