#include <stdio.h>

#include "WinIocp.h"
#include "NetLog.h"
#include "AsyncLogFile.h"


AsyncLogFile* g_pkAsyncLogFile = NULL;

void testOutput(NetLogger::LogLevel iLevel, const char* msg, size_t len)
{
    g_pkAsyncLogFile->append(msg, len);
    //if (iLevel == NetLogger::E_LOG_WARN
    //    || iLevel == NetLogger::E_LOG_ERROR
    //    || iLevel == NetLogger::E_LOG_SYSERR)
    //{
    //    fwrite(msg, 1, len, stdout);
    //}
    fwrite(msg, 1, len, stdout);
}

void testFlush()
{
    g_pkAsyncLogFile->flush();
    fflush(stdout);
}

void initLog()
{
    char pcName[64] = { 0 };
    getAppName(pcName);
    g_pkAsyncLogFile = new AsyncLogFile(pcName);

    NetLogger::setOutput(testOutput);
    NetLogger::setFlush(testFlush);
    g_pkAsyncLogFile->start();
}

CompleteKeyListen* pkListen = NULL;

void onRecv(CompleteKeySocket* pkCK, NetBuffer& rBuffer)
{
    std::string str = std::move(rBuffer.retrieveAll2String());
    if (pkCK->kCKBase.eType == E_CK_ACCEPT)
    {
        LOG_ERROR << "PORT[" << pkCK->usLocalPort << "], Recv ip["
            << pkCK->pcRemoteIp << "], port[" << pkCK->usRemotePort
            << "], msg[" << str.c_str() << "]";

        IocpSend(pkCK, str.c_str(), str.size());
    }
    else
    {
        LOG_ERROR << "CLIENT, Recv ip[" << pkCK->pcRemoteIp
            << "], port[" << pkCK->usRemotePort
            << "], msg[" << str.c_str() << "]";
        
    }
}

void onAcccept(CompleteKeyListen* pkListen, CompleteKeySocket* pkCK)
{
    LOG_ERROR << "PORT[" << pkCK->usLocalPort << "], Accept ip[" << pkCK->pcRemoteIp
        << "], port[" << pkCK->usRemotePort << "]";
    pkCK->kFunOnRecv = onRecv;
}

void onDisconnect(CompleteKeySocket* pkCK)
{
    LOG_ERROR << "CLIENT, disconnect ip[" << pkCK->pcRemoteIp << "], port[" << pkCK->usRemotePort << "]";
}

void onConnect(CompleteKeySocket* pkCK)
{
    LOG_ERROR << "CLIENT, connect ip[" << pkCK->pcRemoteIp << "], port[" << pkCK->usRemotePort << "]";
    pkCK->kFunOnRecv = onRecv;
    pkCK->kFunOnDisconnect = onDisconnect;
}

void onError(CompleteKeySocket* pkCK, int iError)
{
    if (pkCK->kCKBase.eType == E_CK_ACCEPT)
    {
        LOG_ERROR << "ACCEPT socket error [" << iError << "]";
    }
    else
    {
        LOG_ERROR << "CONNECT socket error [" << iError << "]";

    }
}

int main(int argc, char **argv)
{  
    initLog();

    IocpPrepareCompleteKey(AF_INET6, 10, E_CK_ACCEPT);

    WinIOCP::VectorCKListen vectorListen = std::move(
        IocpListen(AF_UNSPEC,
            NULL, 
            "10099", 
            2, 
            FunOnAccept(onAcccept), 
            FunOnError(onError)));

    CompleteKeySocket* pkCKClient = IocpConnectAndWait(
        AF_INET6,
        "localhost",
        "10099",
        FunOnConnect(onConnect), 
        FunOnError(onError));
	CompleteKeySocket* pkCKClient2 = IocpConnectAndWait(
		AF_INET,
		"localhost",
		"10099",
		FunOnConnect(onConnect),
		FunOnError(onError));
    do 
    {
        if (pkCKClient == NULL)
        {
            break;
        }

        char pcSend[1024] = { 0 };
        std::cin >> pcSend;
        IocpSend(pkCKClient, pcSend, strlen(pcSend));
    } while (1);

    return 1;
}
















