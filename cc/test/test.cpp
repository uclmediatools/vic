/*
 * main.cpp
 */

/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "buffer.h"
#include "cc_output.h"
#include "bitvec.h"
#include "hdr.h"

#define UNUSED(x) (x) = (x)

int main (int argc, char *argv[]) {

	UNUSED(argc);
	UNUSED(argv);

	int val;

	//val = cc_output();
	//val = bitvec();
	val = build_hdr_test();

	return val;
}
