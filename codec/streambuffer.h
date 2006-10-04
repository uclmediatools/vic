#ifndef _STREAM_BUFFER_H_
#define _STREAM_BUFFER_H_

#define BUFFER_MAX_FRAME 10

class PacketData;
class DataBuffer;
class StreamBuffer
{
  public:
    StreamBuffer(int, int, int);	//maxFrame, maxPkts, maxLength
       ~StreamBuffer();
    void write(int, int, char *, int);	//frame, pktIdx, buffer, length
    void setTotalPkts(int, int);	//# of packets of a specified frame
    bool isComplete(int);
    DataBuffer *getStream(int);
  private:
        PacketData * frameStream[BUFFER_MAX_FRAME];
    int maxFrames;
    int maxPackets;
    int maxLength;
};

#endif
