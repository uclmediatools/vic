#ifndef _BUFFER_H_
#define _BUFFER_H_

//includes frame basic operations and keeps frame data
class DataBuffer
{
  public:
    DataBuffer(int);
    ~DataBuffer();
    char *getData();
    int getCapacity();
    int getDataSize();
    bool setSize(int);
    bool write(char *, int);
    bool writeAppend(char *, int);
    char *data;

  private:
    int length;
    int size;

};

#endif
