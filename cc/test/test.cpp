#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "buffer.h"
#include "cc_output.h"

#define UNUSED(x) (x) = (x)

int main (int argc, char *argv[]) {

	UNUSED(argc);
	UNUSED(argv);

	int val = cc_output();

	return val;
}
