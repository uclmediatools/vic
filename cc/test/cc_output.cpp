/*
 * test code for cc_output() method in rtp/session.cpp
 */

/* $Id$ */

#include "buffer.h"

// pre-define functions
Buffer gen_packets(int num);

int main() {

	Buffer pktbuf;
	const int NUMPACK = 10;

	pktbuf = gen_packets (NUMPACK);
	pktbuf.display();

	return 0;
}

// generate packets
Buffer gen_packets (int num) {
	Buffer buf;
	int i;

	for (i = 1; i <= num; i++)
		buf.insert_front(i);

	return buf;
}
