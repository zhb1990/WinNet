#include "WinIocp.h"
#include "NetLog.h"


#define IocpHandAccept WinIOCP::getInstance()->handAccept
#define IocpHandRecv WinIOCP::getInstance()->handRecv
#define IocpHandSend WinIOCP::getInstance()->handSend
#define IocpHandConnect WinIOCP::getInstance()->handConnect
#define IocpHandDisconnect WinIOCP::getInstance()->handDisconnect

VOID CALLBACK IoCompletionCallback(PTP_CALLBACK_INSTANCE Instance,
    PVOID Context,
    PVOID Overlapped,
    ULONG IoResult,
    ULONG_PTR NumberOfBytesTransferred,
    PTP_IO Io)
{
    OverlappedBase* pkOL = NULL;
    if (Overlapped != NULL)
    {
        pkOL = CONTAINING_RECORD(Overlapped, OverlappedBase, kOverlapped);
    }
    do
    {
        if (pkOL == NULL || Io == NULL || Context == NULL)
        {
            break;
        }
        
        switch (pkOL->eType)
        {
        case E_OLT_ACCEPTEX:
            IocpHandAccept(IoResult, (CompleteKeyBase*)Context, pkOL, NumberOfBytesTransferred);
            break;
        case E_OLT_WSADISCONNECTEX:
            IocpHandDisconnect(IoResult, (CompleteKeyBase*)Context, pkOL, NumberOfBytesTransferred);
            break;
        case E_OLT_WSARECV:
            IocpHandRecv(IoResult, (CompleteKeyBase*)Context, pkOL, NumberOfBytesTransferred);
            break;
        case E_OLT_WSASEND:
            IocpHandSend(IoResult, (CompleteKeyBase*)Context, pkOL, NumberOfBytesTransferred);
            break;
        case E_OLT_CONNECTEX:
            IocpHandConnect(IoResult, (CompleteKeyBase*)Context, pkOL, NumberOfBytesTransferred);
            break;
        default:
            break;
        }
    } while (0);
}

WinIOCP::WinIOCP()
{
    WSADATA kWSAData;
    int iRet = ::WSAStartup(MAKEWORD(2, 2), &kWSAData);
    if (iRet != 0)
    {
        exit(0);
    }

    initExtensionFuncPtr();
}

WinIOCP * WinIOCP::getInstance()
{
    static WinIOCP _iocp;
    return &_iocp;
}

void WinIOCP::initExtensionFuncPtr()
{
    int iError = 0;
    SOCKET pvSocket = createSocket(iError, AF_INET);
    if (pvSocket == SOCKET_ERROR)
    {
        exit(0);
    }
    GUID guid = WSAID_ACCEPTEX;
    getExtensionFuncPtr((LPVOID*)&_pfnAcceptEx, pvSocket, guid);
    guid = WSAID_DISCONNECTEX;
    getExtensionFuncPtr((LPVOID*)&_pfnDisconnectEx, pvSocket, guid);
    guid = WSAID_GETACCEPTEXSOCKADDRS;
    getExtensionFuncPtr((LPVOID*)&_lpfnGetAcceptExSockAddrs, pvSocket, guid);
    guid = WSAID_CONNECTEX;
    getExtensionFuncPtr((LPVOID*)&_pfnConnectEx, pvSocket, guid);

    closesocket(pvSocket);
}

CompleteKeySocket * WinIOCP::newCompleteKeySocket(int iAF, ECKType eType)
{
    CompleteKeySocket* pkCK = NULL;
    if (eType == E_CK_ACCEPT && iAF == AF_INET)
    {
        CLocalLock<CCriSec400> kLocker(_ccsCKSocketV4Accept);
        if (_dequeCKSocketIPV4Accept.size() > 0)
        {
            pkCK = _dequeCKSocketIPV4Accept.front();
            if (GetTickCount64() - pkCK->llLastDisconnectTime > NET_TCP_TIME_WAIT_DELAY)
            {
                _dequeCKSocketIPV4Accept.pop_front();
            }
            else
            {
                pkCK = NULL;
            }
        }
    }
    else if (eType == E_CK_ACCEPT)
    {
        CLocalLock<CCriSec400> kLocker(_ccsCKSocketV6Accept);
        if (_dequeCKSocketIPV6Accept.size() > 0)
        {
            pkCK = _dequeCKSocketIPV6Accept.front();
            if (GetTickCount64() - pkCK->llLastDisconnectTime > NET_TCP_TIME_WAIT_DELAY)
            {
                _dequeCKSocketIPV6Accept.pop_front();
            }
            else
            {
                pkCK = NULL;
            }
        }
    }
    else if (eType == E_CK_CONNECT && iAF == AF_INET)
    {
        CLocalLock<CCriSec400> kLocker(_ccsCKSocketV4Connect);
        if (_dequeCKSocketIPV4Connect.size() > 0)
        {
            pkCK = _dequeCKSocketIPV4Connect.front();
            if (GetTickCount64() - pkCK->llLastDisconnectTime > NET_TCP_TIME_WAIT_DELAY)
            {
                _dequeCKSocketIPV4Connect.pop_front();
            }
            else
            {
                pkCK = NULL;
            }
        }
    }
    else
    {
        CLocalLock<CCriSec400> kLocker(_ccsCKSocketV6Connect);
        if (_dequeCKSocketIPV6Connect.size() > 0)
        {
            pkCK = _dequeCKSocketIPV6Connect.front();
            if (GetTickCount64() - pkCK->llLastDisconnectTime > NET_TCP_TIME_WAIT_DELAY)
            {
                _dequeCKSocketIPV6Connect.pop_front();
            }
            else
            {
                pkCK = NULL;
            }
        }
    }

    if (pkCK == NULL)
    {
        pkCK = _prepareCkSocket(iAF, eType);
    }
    else
    {
        pkCK->kCKBase.eType = eType;
    }

    if (pkCK->kCKBase.eType == E_CK_ACCEPT && pkCK->iAF == AF_INET)
    {
        CLocalLock<CCriSec400> kLocker(_ccsCKSocketV4Accept);
        _setCKUsedV4Accept.insert(pkCK);
    }
    else if (pkCK->kCKBase.eType == E_CK_ACCEPT)
    {
        CLocalLock<CCriSec400> kLocker(_ccsCKSocketV6Accept);
        _setCKUsedV6Accept.insert(pkCK);
    }
    else if (pkCK->kCKBase.eType == E_CK_CONNECT && pkCK->iAF == AF_INET)
    {
        CLocalLock<CCriSec400> kLocker(_ccsCKSocketV4Connect);
        _setCKUsedV4Connect.insert(pkCK);
    }
    else
    {
        CLocalLock<CCriSec400> kLocker(_ccsCKSocketV6Connect);
        _setCKUsedV6Connect.insert(pkCK);
    }

    
    return pkCK;
}

void WinIOCP::delCompleteKeySocket(CompleteKeySocket* pkCK)
{
    do 
    {
        if (pkCK == NULL)
        {
            break;
        }
        pkCK->init();

        if (pkCK->kCKBase.eType == E_CK_ACCEPT && pkCK->iAF == AF_INET)
        {
            CLocalLock<CCriSec400> kLocker(_ccsCKSocketV4Accept);
            _setCKUsedV4Accept.erase(pkCK);
            _dequeCKSocketIPV4Accept.push_back(pkCK);
        }
        else if (pkCK->kCKBase.eType == E_CK_ACCEPT)
        {
            CLocalLock<CCriSec400> kLocker(_ccsCKSocketV6Accept);
            _setCKUsedV6Accept.erase(pkCK);
            _dequeCKSocketIPV6Accept.push_back(pkCK);
        }
        else if (pkCK->kCKBase.eType == E_CK_CONNECT && pkCK->iAF == AF_INET)
        {
            CLocalLock<CCriSec400> kLocker(_ccsCKSocketV4Connect);
            _setCKUsedV4Connect.erase(pkCK);
            _dequeCKSocketIPV4Connect.push_back(pkCK);
        }
        else
        {
            CLocalLock<CCriSec400> kLocker(_ccsCKSocketV6Connect);
            _setCKUsedV6Connect.erase(pkCK);
            _dequeCKSocketIPV6Connect.push_back(pkCK);
        }
    } while (0);
}

void WinIOCP::prepareCkSocket(int iAF, size_t szNum, ECKType eType)
{
    CompleteKeySocket* pkCK = NULL;
    for (size_t i = 0; i < szNum; i++)
    {
        if (iAF != AF_INET && iAF != AF_INET6)
        {
            break;
        }
        if (eType != E_CK_ACCEPT && eType != E_CK_CONNECT)
        {
            break;
        }

        pkCK = _prepareCkSocket(iAF, eType);
        if (pkCK == NULL)
        {
            continue;
        }
        if (eType == E_CK_ACCEPT && iAF == AF_INET)
        {
            CLocalLock<CCriSec400> kLocker(_ccsCKSocketV4Accept);
            _dequeCKSocketIPV4Accept.push_front(pkCK);
        }
        else if (eType == E_CK_ACCEPT)
        {
            CLocalLock<CCriSec400> kLocker(_ccsCKSocketV6Accept);
            _dequeCKSocketIPV6Accept.push_front(pkCK);
        }
        else if (eType == E_CK_CONNECT && iAF == AF_INET)
        {
            CLocalLock<CCriSec400> kLocker(_ccsCKSocketV4Connect);
            _dequeCKSocketIPV4Connect.push_front(pkCK);
        }
        else
        {
            CLocalLock<CCriSec400> kLocker(_ccsCKSocketV6Connect);
            _dequeCKSocketIPV6Connect.push_front(pkCK);
        }       
    }
}

CompleteKeySocket* WinIOCP::_prepareCkSocket(int iAF, ECKType eType)
{
    int iError = ERROR_SUCCESS;
    CompleteKeySocket* pkCK = NULL;
    SOCKET pvSocket = createSocket(iError, iAF);
    do 
    {
        if (pvSocket == SOCKET_ERROR)
        {
            break;
        }

        iError = setTcpNoDelay(pvSocket);
        if (iError != ERROR_SUCCESS)
        {
            LOG_WARN << "warning: prepareCkSocket setTcpNoDelay " << iError;
            closesocket(pvSocket);
            break;
        }

        if (eType == E_CK_CONNECT)
        {
			iError = setReUseAddr(pvSocket);
			if (iError != ERROR_SUCCESS)
			{
				closesocket(pvSocket);
				break;
			}
            iError = bindCKSocket(iAF, pvSocket);
            if (iError != ERROR_SUCCESS)
            {
                LOG_WARN << "warning: prepareCkSocket bindCKSocket " << iError;
                closesocket(pvSocket);
                break;
            }
		}
                
        pkCK = new CompleteKeySocket(iAF);
        pkCK->kCKBase.eType = eType;
        pkCK->kCKBase.pvHandle = (HANDLE)pvSocket;
        pkCK->kCKBase.pkIO = CreateThreadpoolIo(pkCK->kCKBase.pvHandle,
            (PTP_WIN32_IO_CALLBACK)IoCompletionCallback,
            &pkCK->kCKBase,
            NULL);
    } while (0);

    return pkCK;
}

void WinIOCP::reqAccept(CompleteKeyListen * pkCKListen)
{
    int iError = ERROR_SUCCESS;
    CompleteKeySocket* pkCK = newCompleteKeySocket(pkCKListen->iAF, E_CK_ACCEPT);
    do 
    {
        if (pkCK == NULL)
        {
            break;
        }
        pkCK->kFunOnError = pkCKListen->kFunOnError;
        pkCK->pkCkListen = pkCKListen;
        OverlappedListen* pkOL = new OverlappedListen();
        pkOL->kOLBase.eType = E_OLT_ACCEPTEX;
        pkOL->pkCKAccept = pkCK;
        pkOL->kWSABuf.buf = pkOL->pcBuf;
        pkOL->kWSABuf.len = 0;

        iError = postAccept(_pfnAcceptEx, pkCKListen, pkOL);
        if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
        {
            LOG_WARN << "warning: reqAccept postAccept " << iError;
            delCompleteKeySocket(pkCK);
            delete pkOL;
            break;
        }
    } while (0);
}

void WinIOCP::reqAccept(CompleteKeyListen * pkCKListen, 
    size_t szAcceptNum)
{
    for (size_t i = 0; i < szAcceptNum; i++)
    {
        reqAccept(pkCKListen);
    }
}

int WinIOCP::reqRecv(CompleteKeySocket * pkCK)
{
    int iError = ERROR_SUCCESS;

    OverlappedRecv* pkOL = new OverlappedRecv();
    pkOL->kOLBase.eType = E_OLT_WSARECV;
    pkOL->kWSABuf.buf = pkOL->kBuffer.beginWrite();
    pkOL->kWSABuf.len = (ULONG)pkOL->kBuffer.writableBytes();

    iError = postRecv(pkCK, pkOL);
    if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
    {
        delete pkOL;
    }

    return iError;
}

int WinIOCP::autoPostSend(CompleteKeySocket * pkCK, 
    OverlappedSend * pkOLLast, 
    size_t szSendLast)
{
    OverlappedSend* pkOL = new OverlappedSend();
    pkOL->kOLBase.eType = E_OLT_WSASEND;

    size_t szReadAble = 0;
    size_t szSend = 0;

    // 查看前次发送中 实际未发送的
    for (int i = 0; i < pkOLLast->iNumBuf; i++)
    {
        szReadAble = pkOLLast->pkBuffer[i].readableBytes();
        if (szReadAble > 0)
        {
            pkOL->pkBuffer[pkOL->iNumBuf] = std::move(pkOLLast->pkBuffer[i]);
            pkOL->pkWSABuf[pkOL->iNumBuf].buf = pkOL->pkBuffer[pkOL->iNumBuf].beginRead();
            pkOL->pkWSABuf[pkOL->iNumBuf].len = (ULONG)szReadAble;
            pkOL->iNumBuf++;
            szSend += szReadAble;
        }
    }
    
    do 
    {
        CLocalLock<CCriSec400> kLocker(pkCK->ccsSend);
        pkCK->szSendBytes -= szSendLast;
        if (!pkCK->bEnable)
        {
            szSend = 0;
            break;
        }
        for (; pkOL->iNumBuf < NET_OL_BUF_NUM;)
        {
            if (pkCK->kSendList.size() == 0)
            {
                break;
            }
            pkOL->pkBuffer[pkOL->iNumBuf] = std::move(pkCK->kSendList.front());
            pkCK->kSendList.pop_front();
            szReadAble = pkOL->pkBuffer[pkOL->iNumBuf].readableBytes();
            if (szReadAble == 0)
            {
                continue;
            }
            pkOL->pkWSABuf[pkOL->iNumBuf].buf = pkOL->pkBuffer[pkOL->iNumBuf].beginRead();
            pkOL->pkWSABuf[pkOL->iNumBuf].len = (ULONG)szReadAble;
            pkOL->iNumBuf++;
            szSend += szReadAble;
        }
    } while (0);

    int iError = ERROR_SUCCESS;
    if (szSend > 0)
    {
        iError = postSend(pkCK, pkOL);
        if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
        {
            delete pkOL;
            CLocalLock<CCriSec400> kLocker(pkCK->ccsSend);
            pkCK->szSendBytes -= szSend;
        }
    }
    else
    {
        delete pkOL;
    }
    
    return iError;
}

WinIOCP::VectorCKListen WinIOCP::reqListen(
    int iAF,
    const char * pcIP,
    const char * pcPort,
    size_t szAcceptNum,
    const FunOnAccept& rFunAccept,
    const FunOnError& rFunError)
{
    addrinfo *result = NULL;
    addrinfo *ptr = NULL;
    addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = iAF;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int iError = getaddrinfo(pcIP, pcPort, &hints, &result);
    CompleteKeyListen * pkCK = NULL;
    VectorCKListen vectorCKListen;
    do 
    {
        if (iError != ERROR_SUCCESS)
        {
            break;
        }
        for (ptr = result; ptr != NULL; ptr = ptr->ai_next) 
        {
            pkCK = _reqListen(ptr,
                szAcceptNum, 
                rFunAccept, 
                rFunError);
            if (pkCK == NULL)
            {
                continue;
            }
            vectorCKListen.push_back(pkCK);
        }

        freeaddrinfo(result);
    } while (0);

    if (vectorCKListen.size() > 0)
    {
        CLocalLock<CCriSec400> kLocker(_ccsListen);
        _vectorListen.push_back(vectorCKListen);
    }
    
    return vectorCKListen;
}

CompleteKeyListen * WinIOCP::_reqListen(
    const addrinfo * pkAddrInfo,
    size_t szAcceptNum,
    const FunOnAccept & rFunAccept,
    const FunOnError & rFunError)
{
    CompleteKeyListen * pkCK = NULL;
    int iError = ERROR_SUCCESS;
    do
    {
        /*create socket*/
        SOCKET pvListenSocket = 
            createSocket(iError, pkAddrInfo->ai_family);
        if (pvListenSocket == SOCKET_ERROR)
        {
            break;
        }

        iError = setReUseAddr(pvListenSocket);
        if (iError != ERROR_SUCCESS)
        {
            closesocket(pvListenSocket);
            break;
        }

		iError = setOnlyIPV6(pvListenSocket);
		if (iError != ERROR_SUCCESS)
		{
			closesocket(pvListenSocket);
			break;
		}
		
        iError = bindAndListen(pvListenSocket, 
            pkAddrInfo->ai_addr, 
            (socklen_t)pkAddrInfo->ai_addrlen);
        if (iError != ERROR_SUCCESS)
        {
            closesocket(pvListenSocket);
            break;
        }

        pkCK = new CompleteKeyListen(pkAddrInfo->ai_family);
        pkCK->kFunOnAccept = rFunAccept;
        pkCK->kFunOnError = rFunError;
        pkCK->kCKBase.pvHandle = (HANDLE)pvListenSocket;
        pkCK->kCKBase.pkIO = CreateThreadpoolIo(pkCK->kCKBase.pvHandle,
            (PTP_WIN32_IO_CALLBACK)IoCompletionCallback,
            &pkCK->kCKBase,
            NULL);

        reqAccept(pkCK, szAcceptNum);
    } while (0);

    return	pkCK;
}

CompleteKeySocket * WinIOCP::reqConnect(
    int iAF,
    const char * pcIP, 
    const char * pcPort,
    const FunOnConnect& rFunConnect,
    const FunOnError& rFunError)
{
    return _reqConnect(iAF,
        pcIP,
        pcPort,
        rFunConnect,
        rFunError,
        false);
}

CompleteKeySocket * WinIOCP::reqConnectAndWait(
    int iAF,
    const char * pcIP, 
    const char * pcPort,
    const FunOnConnect& rFunConnect,
    const FunOnError& rFunError)
{
    CompleteKeySocket * pkCK = _reqConnect(
        iAF, 
        pcIP, 
        pcPort, 
        rFunConnect, 
        rFunError, 
        true);

    int iError = ERROR_SUCCESS;
    do 
    {
        if (!pkCK)
        {
            break;
        }
        WaitForThreadpoolIoCallbacks(pkCK->kCKBase.pkIO, FALSE);
        iError = reqRecv(pkCK);
        if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
        {
            LOG_WARN << "warning: reqConnectAndWait reqRecv " << iError;
            handError(pkCK, iError);
            WaitForThreadpoolIoCallbacks(pkCK->kCKBase.pkIO, FALSE);
            pkCK = NULL;
        }
    } while (0);
    
    return pkCK;
}

CompleteKeySocket * WinIOCP::_reqConnect(
    int iAF,
    const char * pcIP,
    const char * pcPort,
    const FunOnConnect& rFunConnect,
    const FunOnError& rFunError,
    bool bWait)
{
    int iError = ERROR_SUCCESS;
    CompleteKeySocket* pkCK = NULL;
    do
    {
        addrinfo kAddrInfo;
        ZeroMemory(&kAddrInfo, sizeof(kAddrInfo));
        iError = prepareConnectAddr(iAF,
            pcIP, 
            pcPort, 
            &kAddrInfo);
        if (iError != ERROR_SUCCESS)
        {
            LOG_WARN << "warning: _reqConnect prepareConnectAddr " << iError;
            break;
        }

        pkCK = newCompleteKeySocket(kAddrInfo.ai_family, E_CK_CONNECT);
        if (pkCK == NULL)
        {
            LOG_WARN << "warning: _reqConnect newCompleteKeySocket = NULL";
            break;
        }
        pkCK->kFunOnConnect = rFunConnect;
        pkCK->kFunOnError = rFunError;
        pkCK->bWait = bWait;

        OverlappedConnect * pkOL = new OverlappedConnect();
        pkOL->kOLBase.eType = E_OLT_CONNECTEX;

        iError = postConnect(_pfnConnectEx,
            pkCK,
            pkOL,
            kAddrInfo.ai_addr,
            (socklen_t)kAddrInfo.ai_addrlen);
        if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
        {
            LOG_WARN << "warning: _reqConnect postConnect " << iError;
            delCompleteKeySocket(pkCK);
            pkCK = NULL;
            delete pkOL;
            break;
        }
        
    } while (0);

    return pkCK;
}

int WinIOCP::reqSend(CompleteKeySocket * pkCK, 
    const char * pcSend, 
    size_t szLen)
{
    return reqSend(pkCK, NetBuffer(pcSend, szLen));
}

int WinIOCP::reqSend(CompleteKeySocket * pkCK, 
    NetBuffer & rSend)
{
    int iError = ERROR_SUCCESS;
    bool bWSSend = false;
    size_t szSend = rSend.readableBytes();

    do
    {
        if (szSend == 0)
        {
            break;
        }
        CLocalLock<CCriSec400> kLocker(pkCK->ccsSend);
        if (!pkCK->bEnable)
        {
            break;
        }
        pkCK->szSendBytes += szSend;
        if (pkCK->szSendBytes > NET_SOCKET_SEND_BUF_MAX)
        {
            reqDisconnect(pkCK);
            break;
        }

        if (pkCK->szSendBytes == szSend)
        {
            bWSSend = true;
            break;
        }

        pkCK->kSendList.push_back(std::move(rSend));
    } while (0);

    if (bWSSend)
    {
        OverlappedSend* pkOL = new OverlappedSend();
        pkOL->kOLBase.eType = E_OLT_WSASEND;
        pkOL->iNumBuf = 1;
        pkOL->pkBuffer[0] = std::move(rSend);
        pkOL->pkWSABuf[0].buf = pkOL->pkBuffer[0].beginRead();
        pkOL->pkWSABuf[0].len = (ULONG)szSend;

        iError = postSend(pkCK, pkOL);
        if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
        {
            rSend = std::move(pkOL->pkBuffer[0]);
            delete pkOL;
            handError(pkCK, iError);
            CLocalLock<CCriSec400> kLocker(pkCK->ccsSend);
            pkCK->szSendBytes -= szSend;
        }
    }

    return iError;
}

bool WinIOCP::reqDisconnect(CompleteKeySocket * pkCK)
{
    bool bReqDisconnect = false;

    {
        CLocalLock<CCriSec400> kLocker(pkCK->ccsSend);
        if (pkCK->bEnable)
        {
            pkCK->bEnable = false;
            bReqDisconnect = true;
        }
    }

    if (bReqDisconnect)
    {
        OverlappedDisconnect* pkOL = new OverlappedDisconnect();
        pkOL->kOLBase.eType = E_OLT_WSADISCONNECTEX;

        int iError = postDisconnect(_pfnDisconnectEx, pkCK, pkOL);
        if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
        {
            bReqDisconnect = false;
            delete pkOL;
        }
    }

    return bReqDisconnect;
}

bool WinIOCP::reqDisconnectAndWait(CompleteKeySocket * pkCK)
{
    bool bReqDisconnect = reqDisconnect(pkCK);

    if (bReqDisconnect)
    {
        WaitForThreadpoolIoCallbacks(pkCK->kCKBase.pkIO, FALSE);
    }
    
    return bReqDisconnect;
}

void WinIOCP::handSend(ULONG IoResult, 
    CompleteKeyBase * pkCKBase, 
    OverlappedBase * pkOLBase, 
    ULONG_PTR NumberOfBytesTransferred)
{
    OverlappedSend * pkOL = CONTAINING_RECORD(pkOLBase, OverlappedSend, kOLBase);
    bool bSucc = false;
    int iError = IoResult;
    CompleteKeySocket* pkCK = NULL;
    do
    {
        if (pkCKBase->eType != E_CK_ACCEPT && pkCKBase->eType != E_CK_CONNECT)
        {
            // 理论上不存在
            break;
        }
        pkCK = CONTAINING_RECORD(pkCKBase, CompleteKeySocket, kCKBase);

        if (IoResult != ERROR_SUCCESS)
        {
            break;
        }

        NetBuffer kBufSendLast(NumberOfBytesTransferred);
        size_t szBufLen = 0;
        size_t szSendLast = 0;
        size_t szRead = 0;
        for (int i = 0; i < pkOL->iNumBuf; i++)
        {
            if (szSendLast >= NumberOfBytesTransferred)
            {
                break;
            }

            szBufLen = pkOL->pkBuffer[i].readableBytes();
            if (szSendLast + szBufLen <= NumberOfBytesTransferred)
            {
                szRead = szBufLen;
            }
            else
            {
                szRead = NumberOfBytesTransferred - szSendLast;
            }
            szSendLast += szRead;

            if (szRead > 0)
            {
                kBufSendLast.append(pkOL->pkBuffer[i].beginRead(), szRead);
                pkOL->pkBuffer[i].hasRead(szRead);
            }
        }

        if (pkCK->pkCkListen)
        {
            InterlockedAdd64(&pkCK->pkCkListen->vllSendBytes, NumberOfBytesTransferred);
        }
        
        if (NumberOfBytesTransferred > 0 && pkCK->kFunOnSend)
        {
            pkCK->kFunOnSend(pkCK, kBufSendLast);
        }

        bSucc = true;
    } while (0);

    do 
    {
        if (pkCK == NULL)
        {
            break;
        }
        iError = autoPostSend(pkCK, pkOL, NumberOfBytesTransferred);
        if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
        {
            bSucc = false;
            break;
        }
    } while (0);

    if (!bSucc && pkCK)
    {
        handError(pkCK, iError);
    }

    delete pkOL;
}

void WinIOCP::handRecv(ULONG IoResult, 
    CompleteKeyBase * pkCKBase, 
    OverlappedBase * pkOLBase, 
    ULONG_PTR NumberOfBytesTransferred)
{
    OverlappedRecv * pkOL = CONTAINING_RECORD(pkOLBase, OverlappedRecv, kOLBase);
    bool bSucc = false;
    int iError = IoResult;
    CompleteKeySocket* pkCK = NULL;
    do
    {
        if (pkCKBase->eType != E_CK_ACCEPT && pkCKBase->eType != E_CK_CONNECT)
        {
            // 理论上不存在
            break;
        }
        pkCK = CONTAINING_RECORD(pkCKBase, CompleteKeySocket, kCKBase);

        if (NumberOfBytesTransferred == 0)
        {
            break;
        }
        if (IoResult != ERROR_SUCCESS)
        {
            break;
        }
        pkOL->kBuffer.hasWritten((int)NumberOfBytesTransferred);
        pkCK->kBufRecv.append(pkOL->kBuffer);

        if (pkCK->kFunOnRecv)
        {
            pkCK->kFunOnRecv(pkCK, pkCK->kBufRecv);
        }

        if (pkCK->kBufRecv.readableBytes() > NET_SOCKET_RECV_BUF_MAX)
        {
            pkCK->kBufRecv.reset();
            pkCK->kBufRecv.shrink_to_fit();
            break;
        }
        if (pkCK->pkCkListen)
        {
            InterlockedAdd64(&pkCK->pkCkListen->vllRecvBytes, NumberOfBytesTransferred);
        }
                
        pkCK->llLastRecvTime = ::GetTickCount64();
        
        int iError = reqRecv(pkCK);
        if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
        {
            break;
        }

        bSucc = true;
    } while (0);

    
    if (!bSucc && pkCK)
    {
        handError(pkCK, iError);
    }
    delete pkOL;
}

void WinIOCP::handAccept(ULONG IoResult, 
    CompleteKeyBase * pkCKBase, 
    OverlappedBase * pkOLBase, 
    ULONG_PTR NumberOfBytesTransferred)
{
    OverlappedListen * pkOL = CONTAINING_RECORD(pkOLBase, OverlappedListen, kOLBase);

    bool bSucc = false;
    bool bRelease = true;
    int iError = IoResult;
    CompleteKeyListen* pkCKListen = NULL;
    do
    {
        if (pkCKBase->eType != E_CK_LISTEN)
        {
            // 理论上不存在
            break;
        }
        pkCKListen = CONTAINING_RECORD(pkCKBase, CompleteKeyListen, kCKBase);
        if (IoResult != ERROR_SUCCESS)
        {
            break;
        }

        bRelease = false;

        getSocketAddrs(_lpfnGetAcceptExSockAddrs,
            pkOL->pkCKAccept,
            pkOL->kWSABuf.buf,
            (int)NumberOfBytesTransferred);
        
        setSoAccept((SOCKET)pkOL->pkCKAccept->kCKBase.pvHandle, (SOCKET)pkCKBase->pvHandle);

        pkOL->pkCKAccept->llLastRecvTime = ::GetTickCount64();

        {
            CLocalLock<CCriSec400> kLocker(pkOL->pkCKAccept->ccsSend);
            pkOL->pkCKAccept->bEnable = true;
        }

        if (pkCKListen->kFunOnAccept)
        {
            pkCKListen->kFunOnAccept(pkCKListen, pkOL->pkCKAccept);
        }

		InterlockedIncrement(&pkCKListen->vlAcceptNum);
		
        int iError = reqRecv(pkOL->pkCKAccept);
        if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
        {
            break;
        }
        bSucc = true;
    } while (0);

    if (!bSucc)
    {
        handError(pkOL->pkCKAccept, iError);
    }

    if (bRelease)
    {
        delCompleteKeySocket(pkOL->pkCKAccept);
    }

    if (pkCKListen)
    {
        reqAccept(pkCKListen);
    }
    
    delete pkOL;
}

void WinIOCP::handConnect(ULONG IoResult, 
    CompleteKeyBase * pkCKBase, 
    OverlappedBase * pkOLBase, 
    ULONG_PTR NumberOfBytesTransferred)
{
    OverlappedConnect * pkOL = CONTAINING_RECORD(pkOLBase, OverlappedConnect, kOLBase);
    bool bSucc = false;
    bool bRelease = true;
    int iError = IoResult;
    CompleteKeySocket* pkCK = NULL;
    do
    {
        if (pkCKBase->eType != E_CK_ACCEPT && pkCKBase->eType != E_CK_CONNECT)
        {
            // 理论上不存在
            break;
        }
        pkCK = CONTAINING_RECORD(pkCKBase, CompleteKeySocket, kCKBase);

        if (IoResult != ERROR_SUCCESS)
        {
            break;
        }
        bRelease = false;
        setSoConnect((SOCKET)pkCK->kCKBase.pvHandle);
        getSocketAddrs(pkCK);

        pkCK->llLastRecvTime = ::GetTickCount64();

        {
            CLocalLock<CCriSec400> kLocker(pkCK->ccsSend);
            pkCK->bEnable = true;
        }

        if (pkCK->kFunOnConnect)
        {
            pkCK->kFunOnConnect(pkCK);
        }
        if (!pkCK->bWait)
        {
            iError = reqRecv(pkCK);
            if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
            {
                break;
            }
        }

        bSucc = true;
    } while (0);

    if (pkCK)
    {
        if (!bSucc)
        {
            handError(pkCK, iError);
        }
        if (bRelease)
        {
            delCompleteKeySocket(pkCK);
        }
    }
    delete pkOL;
}

void WinIOCP::handDisconnect(ULONG IoResult, 
    CompleteKeyBase * pkCKBase, 
    OverlappedBase * pkOLBase, 
    ULONG_PTR NumberOfBytesTransferred)
{
    do 
    {
        OverlappedDisconnect * pkOL = CONTAINING_RECORD(pkOLBase, OverlappedDisconnect, kOLBase);
        delete pkOL;

        if (pkCKBase->eType != E_CK_ACCEPT && pkCKBase->eType != E_CK_CONNECT)
        {
            // 理论上不存在
            break;
        }
        
        CompleteKeySocket* pkCK = CONTAINING_RECORD(pkCKBase, CompleteKeySocket, kCKBase);
        if (pkCK->kFunOnDisconnect)
        {
            pkCK->kFunOnDisconnect(pkCK);
        }
        pkCK->llLastDisconnectTime = ::GetTickCount64();
		if (pkCK->pkCkListen)
		{
			InterlockedIncrement(&pkCK->pkCkListen->vlAcceptNum);
		}
        delCompleteKeySocket(pkCK);
    } while (0);
}

void WinIOCP::handError(CompleteKeySocket * pkCK, int iError)
{
    if (pkCK->kFunOnError)
    {
        pkCK->kFunOnError(pkCK, iError);
    }

    reqDisconnect(pkCK);
}
























