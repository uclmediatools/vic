/*
 * FILE:    mbus_engine.c 
 * AUTHORS: Colin Perkins, Dimitrios Miras, Piers O'Hanlon
 * 
 * Copyright (c) 1998 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

#include "mbus_parser.h"

#include "mbus_handler.h"
#include "mbus_engine.h"

MBusEngine  me; // MBus Engine that deals with mbus messages

MBusEngine::MBusEngine() {
	int i;
	
	for (i=0; i<SOURCE_HASH; i++) {
		cname_hash_table[i].cname=(char *)NULL;
		cname_hash_table[i].source=(Source *)NULL;
	}
	
	static struct mbus_cmnd_to_func  mbus_cmnd_to_func_table[] = {
		{"rtp.source.playout",		&MBusEngine::rx_source_playout},
		{"rtp.source.cname",		&MBusEngine::rx_source_cname},
		{"rtp.source.active",		&MBusEngine::rx_source_active},
		{"audio.output.powermeter",	&MBusEngine::rx_powermeter},
		{"audio.input.powermeter",	&MBusEngine::rx_powermeter},
		{"mbus.hello",				&MBusEngine::rx_mbus_hello},
		{NULL,NULL}
	};
	mbus_cmnd_to_func_table_=mbus_cmnd_to_func_table;
}


void MBusEngine::rx_mbus_hello(char *srce, char *args, MBusHandler *mb)
{
	//SV-XXX: all unused, empty function
	UNUSED(srce);
        UNUSED(args);
        UNUSED(mb);
}

void MBusEngine::rx_source_cname(char *srce, char *args, MBusHandler *mb)
{
	struct mbus_parser *mp;
        char *cname, *ssrc_str;
	u_int32_t	ssrc;

        //SV-XXX: unused
        UNUSED(srce);
        UNUSED(mb);
	
	mp = mbus_parse_init(args);
	
	if (mbus_parse_str(mp, &ssrc_str)) {
		ssrc = strtoul(mbus_decode_str(ssrc_str), NULL, 16);
		int h = SHASH(ssrc);
		if (cname_hash_table[h].cname==NULL) {
			// If ssrc doesn't exist - Add new entry to Hash Table
			mbus_parse_str(mp, &cname);
			cname_hash_table[h].cname=strdup(mbus_decode_str(cname));
			debug_msg("MBus: New ssrc: %s(%u), Cname: %s\n",ssrc_str,ssrc,cname_hash_table[h].cname);
		} 
	}
	mbus_parse_done(mp);
}

void MBusEngine::rx_source_playout(char *srce, char *args, MBusHandler *mb)
{
	struct  mbus_parser *mp;
        int	playout;
	char	*ssrc_str;
	unsigned long	ssrc;

 	//SV-XXX: unused
        UNUSED(srce);
        UNUSED(mb);
	
	mp = mbus_parse_init(args);
	if (mbus_parse_str(mp, &ssrc_str) && mbus_parse_int(mp, &playout)) {
		ssrc = strtoul(mbus_decode_str(ssrc_str), NULL, 16);
		int h = SHASH(ssrc);
		if (cname_hash_table[h].source!=NULL) {
			cname_hash_table[h].source->apdelay(playout);
			cname_hash_table[h].source->pending(1);
		} else if (cname_hash_table[h].cname!=NULL) {
			SourceManager &sm = SourceManager::instance();
			Source *s = sm.lookup(cname_hash_table[h].cname);
			if (s) {
			/* if audio tool sends a playout msg
			* then vic has to enable the sync flag. This will
			* create the playout buffer and schedule the playout 
			* of the video packets. 
				*/
				//cname_hash_table[h].source=s;
				if (!s->sync() ) { /* synchronisation not enabled yet */
					s->sync(1); /* ... enable sync */
				}
				s->apdelay(playout);
				s->pending(1);
			} else
				debug_msg("Synced Source %s was not found (playout: %d)\n", ssrc_str, playout);
		}
	} else {
		debug_msg("mbus: usage \"source_playout <ssrc> <playout_delay>\"\n");
	}
	mbus_parse_done(mp);
}

void MBusEngine::rx_powermeter(char *srce, char *args, MBusHandler *mb)
{
	int value;
	struct mbus_parser *mp;

	//SV-XXX: unused
        UNUSED(srce);

	mp = mbus_parse_init(args);
	if (mbus_parse_int(mp, &value)) {
		if (strcmp(mb->name, "") == 1) {
			Tcl::instance().evalf("catch {relate_power {%s %i}}", mb->name, value);
		}
	} else {
		debug_msg("mbus: usage \"powermeter <value>\"\n");
	}
	mbus_parse_done(mp);
}

void MBusEngine::rx_source_active(char *srce, char *args, MBusHandler *mb)
{
	char *cname;
	struct mbus_parser *mp;
	
	//SV-XXX: unused
        UNUSED(srce);
	
        mp = mbus_parse_init(args);
	if (mbus_parse_str(mp, &cname)) {
		strcpy(mb->name, mbus_decode_str(cname));
	} else {
		debug_msg("mbus: usage \"source_active_now <cname>\"\n");
	}
	mbus_parse_done(mp);
}

void MBusEngine::mbus_handler_engine(char *srce, char *cmnd, char *args, void *data)
{
	int i;
	for (i=0; mbus_cmnd_to_func_table_[i].mbus_cmnd!=NULL; i++) {
//		debug_msg(" vic mbus recv'd: %s(from %s)\n",cmnd, srce);
		if (strcmp(mbus_cmnd_to_func_table_[i].mbus_cmnd, cmnd) == 0) {
			//debug_msg(" vic mbus recon'd: %s\n",mbus_cmnd_to_func_table_[i].mbus_cmnd);
			(this->*(mbus_cmnd_to_func_table_[i].mbus_func))(srce, args, (MBusHandler *) data);
			return;
		}
	}
	debug_msg("Unknown mbus command: %s %s\n", cmnd, args);
}

void mbus_handler_engine(char *srce, char *cmnd, char *args, void *data) {
	me.mbus_handler_engine(srce, cmnd, args, data);
}

const char* MBusEngine::cname2addr(char *cname)
{
	unsigned int i=0;
	
	while (cname[i++]!='@') ;
	
	if (i<strlen(cname))
		return (cname+i);
	else 
		return (cname);
}

/*
static void rx_sync(char *srce, char *args, session_struct *sp)
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

/*
static u_int32_t ntptime()
{
struct timeval t;
gettimeofday(&t, 0);
return (t.tv_sec << 16 | (t.tv_usec << 10) / 15625);
}
*/
