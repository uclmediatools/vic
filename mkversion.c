/*
 * mkversion.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

main(int argc, char **argv)
{
	char s[256];
	int l;
	const char* fmt = "const char version[] = \"%s\";";

	if (argc > 1)
		fmt = argv[1];

	if (fgets(s, sizeof(s), stdin) == NULL) {
		perror("fgets");
		exit(1);
	}

	for (l = strlen(s) - 1; s[l] == '\n' || s[l] == '\r'; --l)
	    s[l] = '\0';
	
	printf(fmt, s);
	printf("\n");
	exit(0);
}
