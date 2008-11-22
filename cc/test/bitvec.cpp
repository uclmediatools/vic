/*
 * bitvec.cpp
 */

/* $Id$ */

#include <stdio.h>		// standard input/output
#include <stdint.h>		// integer with certain length
#include <stdbool.h>	// boolean type and values
#include <sys/types.h>	// data types
#include "bitvec.h"
#include "cc_output.h"

//#define EMUL_LOSS

// variables
static int aoa		 = 0;	// ack of ack
static int mvec[DUPACKS];	// margin vec
static int currseq	 = 0;	// current seqno
static int prevseq	 = 0;	// previous seqno
static int ackVec	 = 0;	// ack vector

int bitvec () {

	const int NUMPKTS = 10;		// # of generated packet seqno
	int i, j;					// used for indexing
	int cnt, offset;

	// initializing mvec[]
	for (i = 0; i < DUPACKS; i++)
		mvec[i] = 0;

	// declare packet buffer
	Buffer sendbuf, recvbuf;
	Data *ptr;

	// simulating packet transmission
	send_buf(sendbuf, NUMPKTS);

#ifdef EMUL_LOSS
	printf("** EMULATING PACKET LOSS...\n");
#endif
	// simulating packet reception
	for (i = 1; i <= NUMPKTS; i++) {
		// fake packet loss
#ifdef EMUL_LOSS
		if (i != 2 && i != 3) {
#endif
			recvbuf.insert_front(i);

			// set current packet seqno
			ptr = recvbuf.get_head_ptr();
			currseq = ptr->get_val();

			// generate ackVec
			if (currseq == prevseq + 1) {
				// set next bit to 1
				SET_BIT_VEC (ackVec, 1);
			} else {
				cnt = currseq - prevseq - 1;
				printf("number of lost packets: %d\n", cnt);

				// set next bit to 0 equal to the # of lost packets
				for (j = 0; j < cnt; j++)
					SET_BIT_VEC (ackVec, 0);

				// and then, set this packet as received
				SET_BIT_VEC (ackVec, 1);
			}
			prevseq = currseq;
#ifdef EMUL_LOSS
		}
#endif
		printf("ackVec: %d\n", ackVec);

		// margin vec
		marginvec(ackVec);
		printf("\tmvec[0][1][2] = (%d %d %d)\n", \
				mvec[0], mvec[1], mvec[2]);

		// ackofack
		aoa = ackofack();
		printf("\tackofack: %d\n", aoa);

		// trim ackVec
		offset = mvec[2] - aoa;
		//offset = currseq - aoa + 1 - DUPACKS - cnt;
		printf("\toffset (mvec[2] - aoa): %d - %d = %d\n", mvec[2], aoa, offset);
		if (aoa)
			trimvec(ackVec, offset);
	} // end for(;;)

	// display packet seqno
	printf("\nreceived packets...");
	recvbuf.display();

	// display ackVec
	printf("ackVec: %d\n", ackVec);

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

// margin vec
void marginvec (int vec) {
	int hseq = get_head_pos(vec) + aoa;
	//printf("\thead of vec: %d\n", get_head_pos(vec));
	printf("\thighest seqno: %d\n", hseq);

	for (int i = 0; i < DUPACKS; i++) 
		mvec[i] = ((hseq - i) < 0) ? 0 : (hseq - i);
}

// simulating packet transmission
void send_buf (Buffer buf, const int num) {
	int i;
	// simulating packet generation
	for (i = 1; i <= num; i++)
		buf.insert_front(i);

	// display packet seqno
	printf("\ngenerated packets...");
	buf.display();
}

// trim ackVec
void trimvec (int vec, int offset) {
	ackVec = vec >> offset;
}

// ackofack
int ackofack () {
	return (mvec[DUPACKS - 1] - 1) < 0 
		? 0 : (mvec[DUPACKS - 1] - 1);
}
