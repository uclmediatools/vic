/*
 * FILE:    mbus_engine.h
 * AUTHORS: Colin Perkins
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

#ifndef _MBUS_ENGINE_H
#define _MBUS_ENGINE_H

#include "source.h"

void mbus_handler_engine(char *srce, char *cmnd, char *args, void *data);

class MBusEngine {
private:
	
	struct cname_src {
		char	*cname;
		Source	*source;
	} cname_hash_table[SOURCE_HASH];
	
	struct mbus_cmnd_to_func {
		const char *mbus_cmnd;
		void (MBusEngine::*mbus_func)(char *srce, char *args, MBusHandler *mb);
	} *mbus_cmnd_to_func_table_;

public:
	MBusEngine();
	~MBusEngine(){;};
	
	void rx_mbus_hello(char *srce, char *args, MBusHandler *mb);
	void rx_source_cname(char *srce, char *args, MBusHandler *mb);
	void rx_source_playout(char *srce, char *args, MBusHandler *mb);
	void rx_powermeter(char *srce, char *args, MBusHandler *mb);
	void rx_source_active(char *srce, char *args, MBusHandler *mb);
	
	void mbus_handler_engine(char *srce, char *cmnd, char *args, void *data);
	const char* cname2addr(char *cname);
	//void (MBusEngine::)(char *srce, char *args, MBusHandler *mb) mbus_handler_engine_func() 
	//{ return (MBusEngine::mbus_handler_engine);}
};
#endif

