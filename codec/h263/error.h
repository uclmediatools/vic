#ifndef _ERROR_
#define _ERROR_

#include <stdio.h>

void error (char *name, int line, char *comment);
#define FATAL(why) error(__FILE__,__LINE__,why)
#define WARING(why) warn(__FILE__,__LINE__,why)

#endif
