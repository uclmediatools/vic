/*
	Netvideo version 3.1
	Written by Ron Frederick <frederick@parc.xerox.com>

	Simple hack to translate a Tcl/Tk init file into a C string constant
*/

/*
 * Copyright (c) Xerox Corporation 1992. All rights reserved.
 *  
 * License is granted to copy, to use, and to make and to use derivative
 * works for research and evaluation purposes, provided that Xerox is
 * acknowledged in all documentation pertaining to any such copy or derivative
 * work. Xerox grants no other licenses expressed or implied. The Xerox trade
 * name should not be used in any advertising without its written permission.
 *  
 * XEROX CORPORATION MAKES NO REPRESENTATIONS CONCERNING EITHER THE
 * MERCHANTABILITY OF THIS SOFTWARE OR THE SUITABILITY OF THIS SOFTWARE
 * FOR ANY PARTICULAR PURPOSE.  The software is provided "as is" without
 * express or implied warranty of any kind.
 *  
 * These notices must be retained in any copies of any part of this software.
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

static int pass;

void put(int c)
{
	static int n;

	if (pass > 10) {
		putchar(c);
	} else
		printf("%u,%c", c, ((++n & 0xf) == 0) ? '\n' : ' ');
}

main(int argc, char **argv)
{
	int c;
	FILE* f;
	
	if (argc < 3) {
		fprintf(stderr, "Usage: tcl2c++ pass file ...\n");
		exit(1);
	}
	printf("static const char code[] = {\n");
	pass = atoi(argv[1]);
	for ( ; --argc >= 2; ++argv) {
		int nl = 1;
		int skipping = 0;

		f = fopen(argv[2], "r");
		if (f == 0) {
			perror(argv[2]);
			exit(1);
		}
		while ((c = getc(f)) != EOF) {
			switch (c) {
			case ' ':
			case '\t':
			case '\f':
				if (nl || skipping)
					continue;
				break;
			case '#':
				if (skipping)
					continue;
				if (nl) {
					skipping = 1;
					continue;
				}
				break;
			case '\n':
				if (skipping) {
					skipping = 0;
					nl = 1;
					continue;
				}
				nl = 1;
				break;
			case 's':
				if (skipping)
					continue;
				if (nl && (pass == 1 || pass > 10)) {
					/* skip 'source' lines */
					const char* targ = "source";
					const char* cp = targ + 1;
					while ((c = getc(f)) == *cp)
						++cp;
					if (*cp == 0) {
						if (c == ' ' || c == '\t') {
							skipping = 1;
							continue;
						}
					}
					for ( ; targ < cp; ++targ)
						put(*targ);
				}
				nl = 0;
				break;
			default:
				nl = 0;
				if (skipping)
					continue;
				break;
			}
			put(c);
		}
		fclose(f);
	}
	printf("0 };\n");
	printf("#include \"vic_tcl.h\"\nstatic EmbeddedTcl etcl(%d, code);\n",
	       pass);
	exit(0);
	/*NOTREACHED*/
}
