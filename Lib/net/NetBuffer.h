#pragma once
#include <vector>
#include "NetAllocator.h"

class NetBuffer
{
public:
    NetBuffer();
    NetBuffer(size_t szLen);
    NetBuffer(const char* pcBuf, size_t szLen);
    NetBuffer(const NetBuffer& rBuffer);
    NetBuffer(NetBuffer&& rBuffer) noexcept;
    ~NetBuffer();

    void append(const NetBuffer& rBuffer);
    void append(const char* pcBuf, size_t szLen);
    std::string retrieveAll2String();

    std::string retrieve2String(size_t szLen);
    NetBuffer retrieve2Buffer(size_t szLen);

    unsigned short peekUShort();

    void reset();
    void swap(NetBuffer& rRight);

    //************************************
    // Method:    beginWrite
    // FullName:  NetBuffer::beginWrite
    // Access:    public 
    // Returns:   char*
    // Qualifier: 调用前保证 writableBytes() > 0
    //************************************
    char* beginWrite()
    {
        return begin() + _posWrite;
    }

    //************************************
    // Method:    beginRead
    // FullName:  NetBuffer::beginRead
    // Access:    public 
    // Returns:   const char*
    // Qualifier: 调用前保证 readableBytes() > 0
    //************************************
    const char* beginRead() const
    {
        return cbegin() + _posRead;
    }

    char* beginRead()
    {
        return begin() + _posRead;
    }

    void hasWritten(size_t szWrite)
    {
        _posWrite += szWrite;
    }

    void hasRead(size_t szRead)
    {
        _posRead += szRead;
    }

    size_t writableBytes() const
    {
        return (int)_buf.size() - _posWrite;
    }

    size_t readableBytes() const
    {
        return _posWrite - _posRead;
    }

    void shrink_to_fit();
    void shrink();

    size_t size() const
    {
        return _buf.size();
    }

    size_t capacity() const
    {
        return _buf.capacity();
    }
    
    NetBuffer& operator= (NetBuffer&& kRight) noexcept
    {
        if (this != &kRight)
        {
            swap(kRight);
        }
        return *this;
    }

    NetBuffer& operator= (const NetBuffer& kRight)
    {
        if (this != &kRight)
        {
            _posRead = kRight._posRead;
            _posWrite = kRight._posWrite;
            _buf = kRight._buf;
        }
        return *this;
    }
    
protected:
    char* begin() 
    { 
        return &*_buf.begin(); 
    }

    const char* cbegin() const 
    {
        return &*_buf.begin();
    }

    void ensureWritableBytes(size_t szLen);

    void makeSpace(size_t szLen);

protected:
    std::vector<char, NetAllocator<char>> _buf;
    size_t _posRead;
    size_t _posWrite;
};














