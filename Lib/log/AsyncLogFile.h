#pragma once

#include <string>
#include <memory>
#include <vector>

#include "lock.h"
#include "NetAllocator.h"
#include "NetConf.h"
#include "NetBase.h"

template<int iSize>
class FixedBuffer
{
public:
    FixedBuffer() :mCur(mData) 
    { 
        memset(mData, 0, sizeof(mData)); 
    }

    ~FixedBuffer() 
    {

    }

    int append(const char* buf, size_t szLen)
    {
        if (free() <= szLen) return 0;
        memcpy(mCur, buf, szLen);
        mCur += szLen;

        return 1;
    }

    char* data() 
    { 
        return mData; 
    }

    size_t len() 
    { 
        return mCur - mData; 
    }

    size_t free()
    { 
        return sizeof(mData) - len(); 
    }

    char* cur() 
    { 
        return mCur; 
    }

    void add(size_t szLen)
    { 
        mCur += szLen;
    }

    void reset()
    {
        memset(mData, 0, sizeof(mData));
        mCur = mData;
    }

    OPERATOR_NEW_DELETE("FixedBuffer");
private:
    char mData[iSize];
    char* mCur;
};



class AsyncLogFile
{
public:
    typedef std::basic_string<char,
        std::char_traits<char>,
        NetAllocator<char>> NetStrings;

	AsyncLogFile(const NetStrings& basename,
		size_t rollSize = 1024 * 1024 * 200, 
		int flushInterval = 3);

	~AsyncLogFile()
    {

    }
	
	void append(const char* logline, size_t len);
	void flush();

	void				start			( void );
	static DWORD WINAPI	Run				( LPVOID lpParameter );	
	DWORD				WorkerThreadProc( void );
    
private:
	const NetStrings mBasename;
	const size_t mRollSize;
	const int mFlushInterval;
	bool mRun;
    CCriSec400 mCCSecCK;
	HANDLE mEvent;

	typedef FixedBuffer<4096*1024> Buffer;
    typedef std::shared_ptr<Buffer> BufferPtr;
	typedef std::vector<BufferPtr, NetAllocator<BufferPtr>> BufferVector;
	
    BufferPtr _kBufferCurrent;
    BufferPtr _kBufferNext;
	BufferVector _vectorbuffers;
};















