/*
 * Copyright (c) 1998 The Regents of the University of California.
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

#ifndef lint
static const char rcsid[] =
    "@(#) $Header$";
#endif

#include <stdio.h>
#include "pkttbl.h"
#include "config.h"

//could put these in config.h
#define bzero(dest,count) memset(dest,0,count)
#define bcopy(src,dest,size) memcpy(dest,src,size)

#define PKT_TABLE_SIZE	1024

PacketTable::PacketTable()
	:size_(PKT_TABLE_SIZE), pool_(0), last_(-1)
{
	table_ = new pktbuf*[size_];
	bzero((char*)table_, size_*sizeof(pktbuf*));
}

void PacketTable::grow()
{
	int newsize = size_ * 2;
	pktbuf** newtable = new pktbuf*[newsize];
	bzero((char*)newtable, newsize * sizeof(pktbuf*));
	bcopy((char*)table_, (char*)newtable, size_*sizeof(pktbuf*));
	size_ = newsize;
	delete[] table_; //SV-XXX: Debian
	table_ = newtable;
}

PacketTable::~PacketTable()
{
	for (int i = 0; i < size_; i++)
		if (table_[i] != 0)
			pool_->release(table_[i]);
	delete[] table_; //SV-XXX: Debian
}

int PacketTable::command(int argc, const char*const* argv)
{
	Tcl& tcl = Tcl::instance();

	if (argc == 3) {
		if (strcmp(argv[1], "pool") == 0) {
			pool_ = (BufferPool*)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
		if (strcmp(argv[1], "delete-packet") == 0) {
			delpkt(atoi(argv[2]));
			return (TCL_OK);
		}
	} else if (argc == 4) {
		if (strcmp(argv[1], "get-field") == 0) {
			int pktid = atoi(argv[3]);
			const u_char* p = getpkt(pktid);
			
			if (p == 0) {
				fprintf(stderr, 
					"get-field: error: empty packet\n");
				return (TCL_ERROR);
			}
			/* 
			 * This is not the cleanest way, but it keeps us 
			 * from making the table elements more heavyweight
			 * classes.
			 */
			u_char* f;
			int fsize = get_field(p, argv[2], &f);
			switch(fsize) {
			case -1:
				/* error */
				fprintf(stderr, "error: bad field name %s.\n",
					argv[2]);
				exit(1);
			case 0:
				/* string */
				tcl.resultf("%s", f);
				break;
			case 1:
				/* char (8 bit) */
				tcl.resultf("%d", *f);
				break;
			case 2:
			{
				/* 16 bit unsigned */
				u_int16_t s = ntohs(*(u_int16_t*)f);
				tcl.resultf("%u", s);
				break;
			}
			case 3:
			{
				/* 16 bit signed */
				int16_t s = ntohs(*(int16_t*)f);
				tcl.resultf("%d", s);
				break;
			}
			case 4:
			{
				/* 32 bit unsigned */
				u_int32_t l = ntohl(*(u_int32_t*)f);
				tcl.resultf("%u", l);
				break;
			}
			case 5:
			{
				/* 32 bit signed */
				int32_t l = ntohl(*(int32_t*)f);
				tcl.resultf("%d", l);
				break;
			}
			}
			return (TCL_OK);
		}
	}

	return (TclObject::command(argc, argv));
}

int PacketTable::addpkt(const u_char* p, int len)
{
	pktbuf* pb = pool_->alloc();
	bcopy(p, pb->dp, len);
	pb->len = len;
	
	/* 
	 * Linearly search for open hole.  We could hash this, but the 
	 * assumption is that tcl effectively does the has for us in the 
	 * storing of the id and that is the only way we will access the
	 * packet.  Change this if needed.
	 */
	int i = (last_ + 1) % size_;
	while(i != last_) {
		if (table_[i] == 0) {
			last_ = i;
			table_[i] = pb;
			return (i);
		}
		i = (i + 1) % size_;
	}

	/* 
	 * We wrapped around - the table is full.  Grow the table and 
	 * place the new packet in the first slot of the extension.
	 */
	int s = size_;
	grow();
	table_[s] = pb;
	last_ = s;

	return (s);
}

void PacketTable::delpkt(int i)
{
	/* sanity check */
	if (i >= size_) {
		fprintf(stderr, "error: tried to delete packet %d/%d.\n", 
			i, size_);
		return;
	}		
	
	pktbuf** pb = &table_[i];
	if (*pb != 0) {
		pool_->release(*pb);
		*pb = 0;
	}
}

/* XXX Do we want to return the pktbuf here? */
u_char* PacketTable::getpkt(int i)
{
	/* sanity check */
	if (i >= size_) {
		fprintf(stderr, "error: tried to get packet %d/%d.\n", 
			i, size_);
		return (0);
	}		
	
	pktbuf* pb = table_[i];
	if (pb != 0)
		return (pb->dp);
	else 
		return (0);
}
