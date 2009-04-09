/*-
 * Copyright (c) 1993-1994 The Regents of the University of California.
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

static const char rcsid[] =
    "@(#) $Header$ (LBL)";

#include "pktbuf.h"

pktbuf* BufferPool::freebufs_=0;
int BufferPool::nbufs_=0;

/*static class BufferPoolClass : public TclClass {
public:
	BufferPoolClass() : TclClass("BufferPool") {}
	virtual TclObject* create(int argc, const char*const* argv) {
		return (new BufferPool);
	}
} buffer_pool_class;
*/

BufferPool::BufferPool()
{
}

pktbuf* BufferPool::alloc(int layer)
{
	pktbuf* pb = freebufs_;
	if (pb != 0)
		freebufs_ = pb->next;
	else {
		/*XXX grow exponentially*/
		pb = new pktbuf;
		pb->manager = this;
		++nbufs_;
	}
	pb->len = 0;
	pb->ref = 1;
	pb->layer = layer;
	pb->dp = pb->data;
	return (pb);
}

void BufferPool::release(pktbuf* pb)
{
	pb->next = freebufs_;
	freebufs_ = pb;
}

Buffer* pktbuf::copy()
{
	pktbuf* cp = manager->alloc(layer);
	memcpy(cp->dp, dp, len);
	cp->len = len;
	return (cp);
}

void Buffer::release()
{
}
