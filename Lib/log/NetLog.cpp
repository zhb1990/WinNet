#include "NetLog.h"
#include <windows.h>


LogTime::LogTime()
{
    memset(mTime, 0, sizeof(mTime));
    getStrTime();
}
void LogTime::getStrTime()
{
    SYSTEMTIME CurLocalTime = { 0 };
    GetLocalTime(&CurLocalTime);

    _snprintf_s(mTime, sizeof(mTime), sizeof(mTime), "%.4d-%.2d-%.2d %.2d:%.2d:%.2d.%.3d | ", CurLocalTime.wYear,
        CurLocalTime.wMonth, CurLocalTime.wDay, CurLocalTime.wHour, CurLocalTime.wMinute,
        CurLocalTime.wSecond, CurLocalTime.wMilliseconds);
}

void defaultOutput(NetLogger::LogLevel iLevel, const char* msg, size_t len)
{
    fwrite(msg, 1, len, stdout);
}

void defaultFlush()
{
    fflush(stdout);
}

NetLogger::LogLevel NetLogger::_seLoglevel = NetLogger::E_LOG_NUM;
NetLogger::OutputFunc NetLogger::_spOutput = defaultOutput;
NetLogger::FlushFunc NetLogger::_spFlush = defaultFlush;

NetLogger::NetLogger(SourceFile file,
    int line,
    LogLevel level)
    : _iLine(line),
    _eCurlevel(level),
    _kFile(file),
    _bToAbort(false)
{
    _kNetStream << _kLogTime.time();
}

NetLogger::NetLogger(SourceFile file,
    int line,
    LogLevel level,
    const char* func)
    : _iLine(line),
    _eCurlevel(level),
    _kFile(file),
    _bToAbort(false)
{
    _kNetStream << _kLogTime.time() << func << ", ";
}

NetLogger::NetLogger(SourceFile file, int line)
    : _iLine(line),
    _eCurlevel(NetLogger::E_LOG_SYSERR),
    _kFile(file),
    _bToAbort(true)
{
    _kNetStream << _kLogTime.time();
}
NetLogger::~NetLogger()
{
    finish();
    NetLogger::NetStrings kStr = std::move(_kNetStream.str());
    _spOutput(_eCurlevel, kStr.c_str(), kStr.size());
    if (_bToAbort)
    {
        _spFlush();
        ::abort();
    }
}
void NetLogger::finish()
{
    _kNetStream << "   --" << _kFile.Data() << ':' << _iLine << '\n';
}

NetLogger::LogLevel NetLogger::logLevel()
{
    return _seLoglevel;
}
void NetLogger::setLogLevel(NetLogger::LogLevel level)
{
    _seLoglevel = level;
}

void NetLogger::setOutput(OutputFunc out)
{
    _spOutput = out;
}

void NetLogger::setFlush(FlushFunc flush)
{
    _spFlush = flush;
}



// 获取当前应用程序名
int getAppName(char* fname, int iSize)
{
    char* path_buffer = NULL;
    int iRet = _get_pgmptr(&path_buffer);
    if (iRet != 0) return iRet;
    iRet = _splitpath_s(path_buffer, NULL, 0, NULL, 0, fname, iSize, NULL, 0);

    return iRet;
}





