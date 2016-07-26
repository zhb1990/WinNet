#include "WinNetHelp.h"

#pragma comment (lib, "WS2_32")
#pragma comment (lib, "Mswsock")



CompleteKeySocket::CompleteKeySocket(int AF)
    :iAF(AF)
{
    memset(&kCKBase, 0, sizeof(kCKBase));
    usRemotePort = 0;
    usLocalPort = 0;
    bEnable = false;
    bWait = false;
    szSendBytes = 0;
    pkCkListen = NULL;
    llLastRecvTime = 0;
    memset(pcRemoteIp, 0, sizeof(pcRemoteIp));
    memset(pcLocalIp, 0, sizeof(pcLocalIp));
}

void CompleteKeySocket::init()
{
    kFunOnSend.swap(FunOnSend());
    kFunOnRecv.swap(FunOnRecv());;
    kFunOnError.swap(FunOnError());
    kFunOnConnect.swap(FunOnConnect());
    kFunOnDisconnect.swap(FunOnDisconnect());

    {
        CLocalLock<CCriSec400> kLocker(ccsSend);
        szSendBytes = 0;
        kSendList.clear();
        bEnable = false;
    }
    
    kBufRecv.reset();

    bWait = false;
    usRemotePort = 0;
    usLocalPort = 0;
    pkCkListen = NULL;
    memset(pcRemoteIp, 0, sizeof(pcRemoteIp));
    memset(pcLocalIp, 0, sizeof(pcLocalIp));
}





int getExtensionFuncPtr(LPVOID* plpf, SOCKET sock, GUID guid)
{
    DWORD dwBytes;
    *plpf = NULL;

    int iError = ERROR_SUCCESS;
    if (::WSAIoctl(sock,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guid,
        sizeof(guid),
        plpf,
        sizeof(*plpf),
        &dwBytes,
        NULL,
        NULL) == SOCKET_ERROR) {
        iError = WSAGetLastError();
    }

    return iError;
}

int postSend(CompleteKeySocket * pkCKAccept, OverlappedSend* pkOLSend)
{
    unsigned long	ulTransmitBytes = 0;

    StartThreadpoolIo(pkCKAccept->kCKBase.pkIO);

    int iError = ERROR_SUCCESS;
    if (::WSASend(
        (SOCKET)pkCKAccept->kCKBase.pvHandle,
        pkOLSend->pkWSABuf,
        pkOLSend->iNumBuf,
        &ulTransmitBytes,
        0,
        &pkOLSend->kOLBase.kOverlapped,
        NULL) == SOCKET_ERROR)
    {
        iError = ::WSAGetLastError();
        if (iError != WSA_IO_PENDING)
        {
            CancelThreadpoolIo(pkCKAccept->kCKBase.pkIO);
        }
    }

    return iError;
}

int postAccept(LPFN_ACCEPTEX pfnAcceptEx, 
    CompleteKeyListen* pkCkListen, 
    OverlappedListen* pkOLSendListen)
{
    int iError = ERROR_SUCCESS;

    DWORD ulRecvBytes = 0;
    DWORD ulReceiveDataBytes = 0;

    StartThreadpoolIo(pkCkListen->kCKBase.pkIO);

    if (pfnAcceptEx(
        (SOCKET)pkCkListen->kCKBase.pvHandle,
        (SOCKET)pkOLSendListen->pkCKAccept->kCKBase.pvHandle,
        pkOLSendListen->kWSABuf.buf,
        ulReceiveDataBytes,
        ACCEPTEX_ADDRESS_BUFFER_SIZE,
        ACCEPTEX_ADDRESS_BUFFER_SIZE,
        &ulRecvBytes,
        &pkOLSendListen->kOLBase.kOverlapped) == SOCKET_ERROR)
    {
        iError = ::WSAGetLastError();
        if (iError != WSA_IO_PENDING)
        {
            CancelThreadpoolIo(pkCkListen->kCKBase.pkIO);
        }
    }

    return iError;
}

int postRecv(CompleteKeySocket * pkCKAccept, OverlappedRecv* pkOLRecv)
{
    int iError = ERROR_SUCCESS;
    DWORD	ulFlag = 0;
    DWORD	ulTransmitBytes = 0;

    StartThreadpoolIo(pkCKAccept->kCKBase.pkIO);

    if (::WSARecv(
        (SOCKET)pkCKAccept->kCKBase.pvHandle,
        &pkOLRecv->kWSABuf,
        1,
        &ulTransmitBytes,
        &ulFlag,
        &pkOLRecv->kOLBase.kOverlapped,
        NULL) == SOCKET_ERROR)
    {
        iError = ::WSAGetLastError();
        if (iError != WSA_IO_PENDING)
        {
            CancelThreadpoolIo(pkCKAccept->kCKBase.pkIO);
        }
    }

    return iError;
}

int postDisconnect(LPFN_DISCONNECTEX pfnDisconnectEx, 
    CompleteKeySocket* pkCK,
    OverlappedDisconnect * pkOLDisconnect)
{
    int iError = ERROR_SUCCESS;

    StartThreadpoolIo(pkCK->kCKBase.pkIO);

    if (pfnDisconnectEx(
        (SOCKET)pkCK->kCKBase.pvHandle,
        &pkOLDisconnect->kOLBase.kOverlapped,
        TF_REUSE_SOCKET,
        0) == SOCKET_ERROR)
    {
        iError = ::WSAGetLastError();
        if (iError != WSA_IO_PENDING)
        {
            CancelThreadpoolIo(pkCK->kCKBase.pkIO);
        }
    }

    return iError;
}

int postConnect(LPFN_CONNECTEX pfnConnectEx, 
    CompleteKeySocket * pkCKConnect,
    OverlappedConnect * pkOLConnect, 
    const SOCKADDR * name, int namelen)
{
    int iError = ERROR_SUCCESS;

    StartThreadpoolIo(pkCKConnect->kCKBase.pkIO);
    if (pfnConnectEx((SOCKET)pkCKConnect->kCKBase.pvHandle,
        name,
        namelen,
        NULL,
        0,
        NULL,
        &pkOLConnect->kOLBase.kOverlapped) == SOCKET_ERROR)
    {
        iError = ::WSAGetLastError();
        if (iError != WSA_IO_PENDING)
        {
            CancelThreadpoolIo(pkCKConnect->kCKBase.pkIO);
        }
    }

    return iError;
}

void convertAddrs(char* pcIP, 
    unsigned short* pusPort, sockaddr_storage* pkAddr)
{
    if (pkAddr->ss_family == AF_INET)
    {
        InetNtop(AF_INET,
            (PVOID)&(((sockaddr_in *)pkAddr)->sin_addr),
            pcIP, 
            NET_IP_STR_LEN);

        *pusPort = ntohs(((sockaddr_in *)pkAddr)->sin_port);
    }
    else
    {
        InetNtop(AF_INET6,
            (PVOID)&(((sockaddr_in6 *)pkAddr)->sin6_addr),
            pcIP, 
            NET_IP_STR_LEN);

        *pusPort = ntohs(((sockaddr_in6 *)pkAddr)->sin6_port);
    }
}

void getSocketAddrs(
    LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockAddrs, 
    CompleteKeySocket * pkCKAccept, 
    char * pAcceptInfo, int iRecvBytes)
{
    sockaddr_storage* pkLocalAddr = NULL;
    int iLocalBytes = sizeof(sockaddr_storage);
    sockaddr_storage* pkRemoteAddr = NULL;
    int iRemoteBytes = sizeof(sockaddr_storage);

    lpfnGetAcceptExSockAddrs(pAcceptInfo, 
        iRecvBytes, 
        ACCEPTEX_ADDRESS_BUFFER_SIZE, 
        ACCEPTEX_ADDRESS_BUFFER_SIZE,
        (sockaddr**)&pkLocalAddr, 
        &iLocalBytes, 
        (sockaddr**)&pkRemoteAddr, 
        &iRemoteBytes);
    
    convertAddrs(pkCKAccept->pcLocalIp, 
        &pkCKAccept->usLocalPort, pkLocalAddr);

    convertAddrs(pkCKAccept->pcRemoteIp, 
        &pkCKAccept->usRemotePort, pkRemoteAddr);
}

void getSocketAddrs(CompleteKeySocket * pkCKConnect)
{
    sockaddr_storage kLocalAddr = { 0 };
    int iLocalBytes = sizeof(sockaddr_storage);
    sockaddr_storage kRemoteAddr = { 0 };
    int iRemoteBytes = sizeof(sockaddr_storage);

    getsockname(
        (SOCKET)pkCKConnect->kCKBase.pvHandle, 
        (sockaddr *)&kLocalAddr,
        &iLocalBytes);

    convertAddrs(pkCKConnect->pcLocalIp,
        &pkCKConnect->usLocalPort, &kLocalAddr);

    getpeername(
        (SOCKET)pkCKConnect->kCKBase.pvHandle, 
        (sockaddr *)&kRemoteAddr,
        &iRemoteBytes);

    convertAddrs(pkCKConnect->pcRemoteIp,
        &pkCKConnect->usRemotePort, &kRemoteAddr);
}

int setTcpNoDelay(SOCKET pvSocket)
{
    int iError = ERROR_SUCCESS;
    bool bNoDelay = true;
    if (setsockopt(pvSocket, 
        IPPROTO_TCP, 
        TCP_NODELAY, 
        (char *)&bNoDelay, 
        sizeof(bNoDelay)) == SOCKET_ERROR)
    {
        iError = WSAGetLastError();
    }

    return iError;
}

int setSoAccept(SOCKET pvSocket, SOCKET pvSocketListen)
{
    int iError = ERROR_SUCCESS;
    if (setsockopt(pvSocket,
        SOL_SOCKET,
        SO_UPDATE_ACCEPT_CONTEXT,
        (const char*)&pvSocketListen,
        sizeof(SOCKET)) == SOCKET_ERROR)
    {
        iError = WSAGetLastError();
    }

    return iError;
}

int setSoConnect(SOCKET pvSocket)
{
    int iError = ERROR_SUCCESS;
    if (setsockopt(pvSocket,
        SOL_SOCKET,
        SO_UPDATE_CONNECT_CONTEXT,
        NULL,
        0) == SOCKET_ERROR)
    {
        iError = WSAGetLastError();
    }

    return iError;
}

int setReUseAddr(SOCKET pvSocket)
{
    int iError = ERROR_SUCCESS;
    BOOL bReUseAddr = TRUE;
    if (setsockopt(pvSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        (char*)&bReUseAddr,
        sizeof(bReUseAddr)) == SOCKET_ERROR)
    {
        iError = WSAGetLastError();
    }
    return iError;
}

int setOnlyIPV6(SOCKET pvSocket, BOOL BOnlyIPV6)
{
	int iError = ERROR_SUCCESS;
	if (setsockopt(pvSocket, 
		IPPROTO_IPV6, 
		IPV6_V6ONLY, 
		(char*)&BOnlyIPV6, 
		sizeof(BOnlyIPV6)) == SOCKET_ERROR)
	{
		iError = WSAGetLastError();
	}
	return iError;
}

SOCKET createSocket(int & iError, int iAF)
{
    iError = ERROR_SUCCESS;
    SOCKET pvSocket = WSASocketW(
        iAF,
        SOCK_STREAM,
        IPPROTO_TCP,
        NULL,
        0,
        WSA_FLAG_OVERLAPPED);
    if (pvSocket == SOCKET_ERROR)
    {
        iError = WSAGetLastError();
    }

    return pvSocket;
}

int bindAndListen(SOCKET pvSocket, 
    const SOCKADDR *socketAddr, socklen_t addrLen)
{
    /*bind socket*/
    int iError = ERROR_SUCCESS;
    do 
    {
        if (bind(pvSocket, socketAddr, addrLen) == SOCKET_ERROR)
        {
            iError = WSAGetLastError();
            break;
        }

        /*listen socket*/
        if (listen(pvSocket, SOMAXCONN) == SOCKET_ERROR)
        {
            iError = WSAGetLastError();
            break;
        }
    } while (0);

    return iError;
}

int bindCKSocket(int iAF, SOCKET pvSocket)
{
	int iError = ERROR_SUCCESS;
	if (iAF == AF_INET)
	{
		SOCKADDR_IN addr;
		memset(&addr, 0, sizeof(SOCKADDR_IN));
		addr.sin_family = AF_INET;
		addr.sin_addr.S_un.S_addr = INADDR_ANY;
		addr.sin_port = 0;
		if (bind(pvSocket,
			(SOCKADDR*)&addr,
			sizeof(addr)) == SOCKET_ERROR)
		{
			iError = WSAGetLastError();
		}
	}
	else
	{
		SOCKADDR_IN6 addr;
		memset(&addr, 0, sizeof(SOCKADDR_IN6));
		addr.sin6_family = AF_INET6;
		memset(&(addr.sin6_addr.u.Byte), 0, 16);
		addr.sin6_port = 0;
		if (bind((SOCKET)pvSocket,
			(SOCKADDR*)&addr,
			sizeof(addr)) == SOCKET_ERROR)
		{
			iError = WSAGetLastError();
		}
	}

	return iError;
}

int prepareConnectAddr(int iAF,
    const char * pcIP, 
    const char * pcPort, 
	SOCKADDR *socketAddr,
	socklen_t &addrLen)
{
    addrinfo *result = NULL;
    addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = iAF;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_NUMERICSERV;
    int iError = getaddrinfo(pcIP, pcPort, &hints, &result);
    if (iError == ERROR_SUCCESS)
    {
		addrLen = (socklen_t)result->ai_addrlen;
        memcpy(socketAddr, result->ai_addr, addrLen);
        freeaddrinfo(result);
    }

    return iError;
}

OverlappedRecv::OverlappedRecv()
    :kBuffer(NET_POST_RECV_BUF_LEN)
{
    memset(&kOLBase, 0, sizeof(kOLBase));
    memset(&kWSABuf, 0, sizeof(kWSABuf));
}
