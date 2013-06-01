#include "config.h"
#include "packetbuffer.h"
#include "databuffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


PacketBuffer::PacketBuffer(int maxPacket, int maxLength)
{
    totalPkts = 0;
    maxPkts = maxPacket;
    maxLen = maxLength;
    stream = new DataBuffer(maxPacket * maxLength);

    for (int i = 0; i < maxPkts && i < MAX_PACKETS; i++) {
	isDataRecv[i] = false;
	packets[i] = new DataBuffer(maxLen);
	//std::cout << "allocate databuffer " << i << "\n";
    }
}

PacketBuffer::~PacketBuffer()
{
    delete stream;
    for (int i = 0; i < maxPkts && i < MAX_PACKETS; i++)
	delete packets[i];
}

void PacketBuffer::writeAppend(int idx, int size, char *buffer)
{
    if (idx > MAX_PACKETS || idx < 0)
	return;
    isDataRecv[idx] = packets[idx]->writeAppend(buffer, size);
}

void PacketBuffer::write(int idx, int size, char *buffer)
{
    if (idx > MAX_PACKETS || idx < 0)
	return;
    isDataRecv[idx] = packets[idx]->write(buffer, size);
}

void PacketBuffer::setSize(int idx, int size)
{
    if (idx > MAX_PACKETS || idx < 0)
	return;
    isDataRecv[idx] = packets[idx]->setSize(size);
}

bool PacketBuffer::isComplete()
{
    if (totalPkts == 0)
	return false;
    for (int i = 0; i < totalPkts && i < MAX_PACKETS; i++){	
	if (!isDataRecv[i]){
	    //debug_msg("lost packet %d\n", i);	
	    return false;
	}
    }
    return true;
}

void PacketBuffer::setTotalPkts(int n)
{
    totalPkts = n;
}

int PacketBuffer::getTotalPkts()
{
    return totalPkts;
}

//return bitstream , corrupted packets will be ignored
char *PacketBuffer::getPacket(int n)
{
  if (isDataRecv[n]) 
    return packets[n]->getData();
  else
    debug_msg("packet %d\n", n);	
    
}

DataBuffer *PacketBuffer::getStream()
{
    if (totalPkts == 0)
	return NULL;

    //int offset = 0;
    //char* dst = stream->getData();
    for (int i = 0; i < totalPkts && i < MAX_PACKETS; i++) {
	if (isDataRecv[i]) {
	    //memcpy(dst+offset, packets[i]->getData(), packets[i]->getDataSize() );
	    //offset+=packets[i]->getDataSize();
	    stream->writeAppend(packets[i]->getData(),
				packets[i]->getDataSize());
	}
    }
    //stream->setSize(offset);
    return stream;
}

void PacketBuffer::clear()
{
    //std::cout << "Total packets " << totalPkts << "\n";
    for (int i = 0; i < maxPkts && i < MAX_PACKETS; i++) {
	packets[i]->setSize(0);
	isDataRecv[i] = false;
    }
    stream->setSize(0);
    totalPkts = 0;
}
