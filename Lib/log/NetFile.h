#pragma once
#include <stdio.h>
#include <string>
#include <memory>

#include "lock.h"
#include "NetBase.h"


class FileBase
{
public:
    typedef std::basic_string<char,
        std::char_traits<char>,
        NetAllocator<char>> NetStrings;

    FileBase(const NetStrings& filename);
    ~FileBase();
    void append(const char* logline, const size_t len);
    void flush() 
    { 
        ::fflush(_pFile); 
    }
    size_t writtenBytes() const 
    { 
        return _szWrittenBytes; 
    }

    OPERATOR_NEW_DELETE("FileBase");
private:
    FILE*	_pFile;
    size_t _szWrittenBytes;
};


class LogFile
{
public:
    typedef std::basic_string<char,
        std::char_traits<char>,
        NetAllocator<char>> NetStrings;

    LogFile(const NetStrings& basename,
        size_t rollSize,
        int flushInterval = 3);
    ~LogFile();

    void append(const char* logline, size_t len);
    void flush();

    OPERATOR_NEW_DELETE("LogFile");
private:

    void appendUnlocked(const char* logline, size_t len);

    static NetStrings getLogFileName(
        const NetStrings& basename, time_t* pNow);

    void rollFile();

    const NetStrings _ckBasename;
    const size_t _cszRollSize;
    const int _ciFlushInterval;
    CCriSec400 _kCCSecCK;
    std::shared_ptr<FileBase> _kFilePtr;

    int _iAppendCnt;
    time_t _llStartOfPeriod;
    time_t _llLastRollTime;
    time_t _llLastFlushTime;
    const static int _siCheckRollTimes = 1024;
    const static int _siSecondsPerRoll = 60 * 60 * 24;
};

