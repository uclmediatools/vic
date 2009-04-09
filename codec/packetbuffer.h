#ifndef _PACKET_DATA_H_
#define _PACKET_DATA_H_

#define MAX_PACKETS 2048

#include "databuffer.h"


//class DataBuffer;
class PacketBuffer
{
  public:
    PacketBuffer(int, int);	//maxPackets, maxLength
       ~PacketBuffer();
    void write(int, int, char *);	//pktIdx, length, buffer
    void writeAppend(int, int, char *);	//pktIdx, length, buffer
    void setSize(int, int);	//pktIdx, length
    void setTotalPkts(int);
    int getTotalPkts();
    bool isComplete();
    DataBuffer *getStream();
    char *getPacket(int);
    void clear();
    DataBuffer *packets[MAX_PACKETS];
  private:
    int totalPkts;
    int maxPkts;
    int maxLen;
    DataBuffer *stream;
    bool isDataRecv[MAX_PACKETS];
};

#endif
