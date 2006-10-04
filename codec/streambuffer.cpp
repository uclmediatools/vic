#include "streambuffer.h"
#include "packetdata.h"
#include "databuffer.h"

StreamBuffer::StreamBuffer(int frame, int pkts, int len)
{
    if (frame > BUFFER_MAX_FRAME)
	maxFrames = BUFFER_MAX_FRAME;
    else
	maxFrames = frame;
    maxPackets = pkts;
    maxLength = len;

    for (int i = 0; i < maxFrames; i++) {
	frameStream[i] = new PacketData(pkts, len);
    }
}

StreamBuffer::~StreamBuffer()
{
}

void StreamBuffer::setTotalPkts(int idx, int n)
{
    frameStream[idx]->setTotalPkts(n);
}

bool StreamBuffer::isComplete(int idx)
{
    return frameStream[idx]->isComplete();
}

DataBuffer *StreamBuffer::getStream(int idx)
{
    return frameStream[idx]->getStream();
}

void StreamBuffer::write(int fIdx, int pIdx, char *buffer, int len)
{
    frameStream[fIdx]->write(pIdx, len, buffer);
}
