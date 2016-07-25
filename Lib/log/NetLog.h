#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <sstream>

#include "NetConf.h"
#include "lock.h"
#include "NetAllocator.h"

class LogTime
{
public:
    LogTime();
    void getStrTime();
    char* time() { return mTime; }
private:
    char mTime[30];
};

class NetLogger
{
public:
    enum LogLevel
    {
        E_LOG_SYSERR = 0,
        E_LOG_ERROR,
        E_LOG_WARN,
        E_LOG_INFO,
        E_LOG_DEBUG,
        E_LOG_NUM
    };
    class SourceFile
    {
    public:
        template<int N>
        inline SourceFile(const char(&arr)[N])
            : data_(arr),
            size_(N - 1)
        {
            const char* slash = strrchr(data_, '\\');
            if (slash)
            {
                data_ = slash + 1;
                size_ -= static_cast<int>(data_ - arr);
            }
        }

        explicit SourceFile(const char* filename)
            : data_(filename)
        {
            const char* slash = strrchr(filename, '\\');
            if (slash)
            {
                data_ = slash + 1;
            }
            size_ = static_cast<int>(strlen(data_));
        }
        const char* Data() { return data_; }
    private:
        const char* data_;
        int size_;
    };

    NetLogger(SourceFile file, int line, LogLevel level);
    NetLogger(SourceFile file, int line, LogLevel level, const char* func);
    NetLogger(SourceFile file, int line);
    ~NetLogger();

    typedef std::basic_stringstream<char,
        std::char_traits<char>,
        NetAllocator<char>> NetStream;

    typedef std::basic_string<char,
        std::char_traits<char>,
        NetAllocator<char>> NetStrings;

    NetStream& stream() { return _kNetStream; }
    void finish();

    static LogLevel logLevel();
    static void setLogLevel(LogLevel level);

    typedef void(*OutputFunc)(LogLevel iLevel, const char* msg, size_t len);
    typedef void(*FlushFunc)();
    static void setOutput(OutputFunc out);
    static void setFlush(FlushFunc flush);
    
private:
    static LogLevel		_seLoglevel;
    static OutputFunc	_spOutput;
    static FlushFunc	_spFlush;

    bool		_bToAbort;
    LogLevel	_eCurlevel;
    NetStream	_kNetStream;
    int			_iLine;
    SourceFile	_kFile;
    LogTime		_kLogTime;
};

#define LOG_SYSERR	NetLogger(__FILE__, __LINE__).stream()

#define LOG_ERROR	if (NetLogger::logLevel() >= NetLogger::E_LOG_ERROR) \
	NetLogger(__FILE__, __LINE__, NetLogger::E_LOG_ERROR).stream()

#define LOG_WARN	if (NetLogger::logLevel() >= NetLogger::E_LOG_WARN) \
	NetLogger(__FILE__, __LINE__, NetLogger::E_LOG_WARN).stream()

#define LOG_INFO	if (NetLogger::logLevel() >= NetLogger::E_LOG_INFO) \
	NetLogger(__FILE__, __LINE__, NetLogger::E_LOG_INFO).stream()

#define LOG_DEBUG	if (NetLogger::logLevel() >= NetLogger::E_LOG_DEBUG) \
	NetLogger(__FILE__, __LINE__, NetLogger::E_LOG_DEBUG, __FUNCSIG__).stream()




// 获取当前应用程序名
int getAppName(char* fname, int iSize);

template<int iSize>
int getAppName(char(&fname)[iSize])
{
    return getAppName(fname, iSize);
}