/*
 * FILE:    mbus_engine.c
 * AUTHORS: Colin Perkins, Dimitrios Miras
 * 
 * Copyright (c) 1998 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted, for non-commercial use only, provided
 * that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * Use of this software for commercial purposes is explicitly forbidden
 * unless prior written permission is obtained from the authors.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include "source.h"
#include "mbus.h"

/*
static void func_sync(char *srce, char *args, session_struct *sp)
{
	int i;

	mbus_parse_init(sp->mbus_engine, args);
	if (mbus_parse_int(sp->mbus_engine, &i)) {
		sp->sync_on = i;
	} else {
		printf("mbus: usage \"sync \"\n");
	}
	mbus_parse_done(sp->mbus_engine);
}
*/

u_int32_t cname2addr(char *cname)
{
	int i=0;

	 while (cname[i++]!='@') ;
		if (i<strlen(cname))
			 return (LookupHostAddr(cname+i));
		else 
			return (LookupHostAddr(cname));
}
/*
static u_int32_t ntptime()
{
        struct timeval t;
        gettimeofday(&t, 0);
        return (t.tv_sec << 16 | (t.tv_usec << 10) / 15625);
}
*/

static void func_source_playout(char *srce, char *args, MBusHandler *mb)
{
	char *cname, *name;
	int playout;
	u_int32_t addr;

	mb->mbus_parse_init(args);
	if (mb->mbus_parse_str(&cname) && mb->mbus_parse_int(&playout)) {
		name = mb->mbus_decode_str(cname);
		addr = cname2addr(name);
		SourceManager &sm = SourceManager::instance();
		Source *s = sm.lookup(addr);
		if (s) {
			/* if audio tool sends a playout msg
			 * then vic has to enable the sync flag. This will
			 * create the playout buffer and schedule the playout 
			 * of the video packets. 
			 */
			if (!s->sync()) { /* synchronisation not enabled yet */
				s->sync(1); /* ... enable sync */
			}
			s->apdelay(playout);
			s->pending(1);
		}
		else
			printf("Synced Source %s was not found\n", cname);
	} else {
		printf("mbus: usage \"source_playout <cname> <pp>\"\n");
	}
	mb->mbus_parse_done();
}


char *mbus_cmnd[] = {
	//"sync",
	"source_playout",
	""
};

void (*mbus_func[])(char *srce, char *args, MBusHandler *mb) = {
	//func_sync,
	func_source_playout,
};

void mbus_handler_engine(char *srce, char *cmnd, char *args, void *data)
{
	int i;

	for (i=0; strlen(mbus_cmnd[i]) != 0; i++) {
		if (strcmp(mbus_cmnd[i], cmnd) == 0) {
			mbus_func[i](srce, args, (MBusHandler *) data);
			return;
		}
	}
#ifdef DEBUG_MBUS
	printf("Unknown mbus command: %s %s\n", cmnd, args);
#endif
}

