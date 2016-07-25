#include "NetBuffer.h"
#include <memory.h>

 
NetBuffer::NetBuffer()
    :_posRead(0),
    _posWrite(0) 
{

}

NetBuffer::NetBuffer(size_t szLen)
    :_buf(szLen),
    _posRead(0),
    _posWrite(0) 
{

}

NetBuffer::NetBuffer(const char* pcBuf, size_t szLen)
    :_buf(szLen),
    _posRead(0),
    _posWrite(0)
{
    append(pcBuf, szLen);
}

NetBuffer::NetBuffer(const NetBuffer & rBuffer)
    :_buf(rBuffer._buf),
    _posRead(rBuffer._posRead),
    _posWrite(rBuffer._posWrite)
{

}

NetBuffer::NetBuffer(NetBuffer && rBuffer) noexcept
    :_posRead(0),
    _posWrite(0) 
{
    swap(rBuffer);
}

NetBuffer::~NetBuffer() 
{

}

void NetBuffer::append(const NetBuffer & rBuffer)
{
    size_t szReadable = rBuffer.readableBytes();
    if (szReadable > 0)
    {
        append(rBuffer.beginRead(), szReadable);
    }
}

void NetBuffer::append(const char * pcBuf, size_t szLen)
{
    ensureWritableBytes(szLen);
    memmove(beginWrite(), pcBuf, szLen);
    hasWritten(szLen);
}

std::string NetBuffer::retrieveAll2String()
{
    std::string kString;
    size_t szReadable = readableBytes();
    if (szReadable > 0)
    {
        kString.assign(beginRead(), readableBytes());
        hasRead(szReadable);
    }
    return kString;
}

std::string NetBuffer::retrieve2String(size_t szLen)
{
    std::string kString;
    size_t szReadable = readableBytes();
    if (szLen > 0 && szReadable >= szLen)
    {
        kString.assign(beginRead(), szLen);
        hasRead(szLen);
    }
    return kString;
}

NetBuffer NetBuffer::retrieve2Buffer(size_t szLen)
{
    NetBuffer kBuffer;
    size_t szReadable = readableBytes();
    if (szLen > 0 && szReadable >= szLen)
    {
        kBuffer.append(beginRead(), szLen);
        hasRead(szLen);
    }
    return kBuffer;
}

unsigned short NetBuffer::peekUShort()
{
    unsigned short usNum = 0;
    if (readableBytes() > sizeof(usNum))
    {
        memcpy(&usNum, beginRead(), sizeof(usNum));
    }
    return usNum;
}

void NetBuffer::reset() 
{
    _buf.clear();
    _posRead = 0;
    _posWrite = 0;
}

void NetBuffer::swap(NetBuffer & rRight) 
{
    _buf.swap(rRight._buf);
    std::swap(_posRead, rRight._posRead);
    std::swap(_posWrite, rRight._posWrite);
}

void NetBuffer::shrink_to_fit()
{
    shrink();
    if (_buf.size() > 0)
    {
        _buf.erase(_buf.begin() + _posWrite, _buf.end());
    }    
    _buf.shrink_to_fit();
}

void NetBuffer::shrink()
{
    if (_posRead > 0)
    {
        size_t szReadable = readableBytes();
        memmove(begin(), begin() + _posRead, szReadable);
        _posRead = 0;
        _posWrite = _posRead + szReadable;
    }
}

void NetBuffer::ensureWritableBytes(size_t szLen)
{
    if (writableBytes() < szLen)
    {
        makeSpace(szLen);
    }
}

void NetBuffer::makeSpace(size_t szLen)
{
    if (writableBytes() + _posRead < szLen)
    {
        _buf.resize(_posWrite + szLen);
    }
    else 
    {
        shrink();
    }
}
