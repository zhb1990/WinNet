#include "AsyncLogFile.h"
#include "NetFile.h"


AsyncLogFile::AsyncLogFile(
    const NetStrings& basename, 
    size_t rollSize, 
    int flushInterval)
    :mBasename(basename), 
    mRollSize(rollSize),
    mFlushInterval(flushInterval),
    mRun(false),
    _kBufferCurrent(new Buffer),
    _kBufferNext(new Buffer)
{
	mEvent = CreateEvent(NULL, false, false, NULL);
	_vectorbuffers.reserve(16);
}

void AsyncLogFile::append(const char* logline, size_t len)
{
	CLocalLock<CCriSec400> lock(mCCSecCK);
	if (_kBufferCurrent->free() > len)
	{
		_kBufferCurrent->append(logline, len);
	}
	else
	{
		_vectorbuffers.push_back(std::move(_kBufferCurrent));

		if (_kBufferNext)
		{
			_kBufferCurrent = std::move(_kBufferNext);
		}
		else
		{
			_kBufferCurrent.reset(new Buffer); 
		}
		_kBufferCurrent->append(logline, len);
		SetEvent(mEvent);
	}
}
void AsyncLogFile::flush()
{
	SetEvent(mEvent);
}
DWORD AsyncLogFile::WorkerThreadProc( void )
{
    if (!mRun)
    {
        return -1;
    }

	LogFile output(mBasename, mRollSize);
	BufferPtr newBuffer1(new Buffer);
	BufferPtr newBuffer2(new Buffer);
	BufferVector buffersToWrite;
	buffersToWrite.reserve(16);

	while (mRun)
	{
		DWORD dwWaitRes = WaitForSingleObject(mEvent, 5000);
        if (dwWaitRes != WAIT_OBJECT_0 && dwWaitRes != WAIT_TIMEOUT)
        {
            continue;
        }

		{
			CLocalLock<CCriSec400> lock(mCCSecCK);
			ResetEvent(mEvent);		/// 将 在竞争锁的过程中 产生的事件 重置
			_vectorbuffers.push_back(std::move(_kBufferCurrent));
			_kBufferCurrent = std::move(newBuffer1);
			buffersToWrite.swap(_vectorbuffers);
			if (!_kBufferNext)
			{
				_kBufferNext = std::move(newBuffer2);
			}
		}

        if (buffersToWrite.empty())
        {
            break;
        }

		if (buffersToWrite.size() > 25)
		{
			LogTime logTime;
			char buf[256];
			_snprintf_s(buf, sizeof(buf), sizeof(buf), 
                "Dropped log messages at %s %zu larger buffers\n",
				logTime.time(),
				buffersToWrite.size()-2);

			fputs(buf, stderr);
			output.append(buf, strlen(buf));
			buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
		}
		for (size_t i = 0; i < buffersToWrite.size(); ++i)
		{
			output.append(buffersToWrite[i]->data(), buffersToWrite[i]->len());
		}
		if (buffersToWrite.size() > 2)
		{
			buffersToWrite.resize(2);
		}
		if (!newBuffer1)
		{
			newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
			newBuffer1->reset();
		}
		if (!newBuffer2)
		{
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
			newBuffer2->reset();
		}
		buffersToWrite.clear();
		output.flush();
	}
	output.flush();

	return 1;
}

DWORD WINAPI AsyncLogFile::Run( LPVOID lpParameter )
{
	AsyncLogFile* asyncLogFile = (AsyncLogFile*) lpParameter;
	if (asyncLogFile != NULL)
	{
		return asyncLogFile->WorkerThreadProc();
	}

	return 0;
}

void AsyncLogFile::start( void )
{
	mRun = true;
	HANDLE hThread = ::CreateThread(NULL, 0, Run, (void*)this, 0, NULL);
	if ( hThread != NULL )
	{
		CloseHandle(hThread);
	}
}






































