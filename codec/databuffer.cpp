#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "databuffer.h"

DataBuffer::DataBuffer(int allocSize)
{
    data = NULL;
    size = 0;
    data = new char[allocSize];
    length = allocSize;
}

DataBuffer::~DataBuffer()
{
    if (data)
	delete[]data;
}

char *DataBuffer::getData()
{
    if (data != NULL)
	return data;
    else
	return NULL;
}

bool DataBuffer::setSize(int s)
{
    if (!data)
	return false;
    if (s > length)
	return false;

    size = s;
    return true;
}

int DataBuffer::getDataSize()
{
    return size;
}

int DataBuffer::getCapacity()
{
    return length;
}

bool DataBuffer::write(char *src, int dataSize)
{
    bool result = setSize(dataSize);
    if (result && src != NULL)
	memcpy(data, src, dataSize);
    else
	return false;
    return result;
}

bool DataBuffer::writeAppend(char *src, int dataSize)
{
    int orgSize = size;
    bool result = setSize(size + dataSize);
    if (result && src != NULL)
	memcpy(data + orgSize, src, dataSize);
    else
	return false;
    return result;
}
