#include "NetFile.h"
#include <time.h>



FileBase::FileBase(const NetStrings& filename)
    : _szWrittenBytes(0)
{
    int err = ::fopen_s(&_pFile, filename.c_str(), "a+");
    if (_pFile == NULL || err != 0)
    {
        fprintf(stderr, "LogFile::File create failed[%d]\n", err);
    }
}

FileBase::~FileBase()
{
    flush();
    ::fclose(_pFile);
}

void FileBase::append(const char* logline, const size_t len)
{
    size_t n = fwrite(logline, 1, len, _pFile);
    if (n == 0)
    {
        int err = ferror(_pFile);
        if (err)
        {
            fprintf(stderr, "LogFile::File::append() failed %d\n", err);
        }
    }
    _szWrittenBytes += n;
}



LogFile::LogFile(const NetStrings& basename,
    size_t rollSize,
    int flushInterval)
    : _ckBasename(basename),
    _cszRollSize(rollSize),
    _ciFlushInterval(flushInterval),
    _iAppendCnt(0),
    _llStartOfPeriod(0),
    _llLastRollTime(0),
    _llLastFlushTime(0)
{
    rollFile();
}

LogFile::~LogFile()
{
}


void LogFile::append(const char* logline, size_t len)
{
    CLocalLock<CCriSec400> lock(_kCCSecCK);
    appendUnlocked(logline, len);
}
void LogFile::appendUnlocked(const char* logline, size_t len)
{
    _kFilePtr->append(logline, len);
  
    if (_kFilePtr->writtenBytes() > _cszRollSize)
    {
        rollFile();
    }
    else
    {
        if (_iAppendCnt > _siCheckRollTimes)
        {
            _iAppendCnt = 0;
            time_t now = time(NULL);
            time_t thisPeriod_ = now / _siSecondsPerRoll * _siSecondsPerRoll;
            if (thisPeriod_ != _llStartOfPeriod)
            {
                rollFile();
            }
            else if (now - _llLastFlushTime > _ciFlushInterval)
            {
                _llLastFlushTime = now;
                _kFilePtr->flush();
            }
        }
        else
        {
            ++_iAppendCnt;
        }
    }
}

void LogFile::flush()
{
    CLocalLock<CCriSec400> lock(_kCCSecCK);
    _kFilePtr->flush();
}

void LogFile::rollFile()
{
    time_t now = 0;
    NetStrings filename = getLogFileName(_ckBasename, &now);
    time_t start = now / _siSecondsPerRoll * _siSecondsPerRoll;

    if (now > _llLastRollTime)
    {
        _llLastRollTime = now;
        _llLastFlushTime = now;
        _llStartOfPeriod = start;
        _kFilePtr.reset(new FileBase(filename));
    }
}

LogFile::NetStrings LogFile::getLogFileName(const NetStrings& basename, time_t* pNow)
{
    NetStrings filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    struct tm tmNow;
    ::time(pNow);
    localtime_s(&tmNow, pNow);
    ::strftime(timebuf, sizeof(timebuf), "_%m-%d_%H-%M-%S", &tmNow);
    filename += timebuf;
    filename += ".log";

    return filename;
}








