#ifndef _PACKET_DATA_H_
#define _PACKET_DATA_H_

#define MAX_PACKETS 2048


class DataBuffer;
class PacketBuffer
{
  public:
    PacketBuffer(int, int);	//maxPackets, maxLength
       ~PacketBuffer();
    void write(int, int, char *);	//pktIdx, length, buffer
    void setTotalPkts(int);
    int getTotalPkts();
    bool isComplete();
    DataBuffer *getStream();
    void clear();
  private:
    int totalPkts;
    int maxPkts;
    int maxLen;
    DataBuffer *stream;
    DataBuffer *packets[MAX_PACKETS];
    bool isDataRecv[MAX_PACKETS];
};

#endif
