#pragma once
#include <list>
#include <functional>
#include <iostream>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#include "lock.h"
#include "NetBuffer.h"
#include "NetBase.h"


#define ACCEPTEX_ADDRESS_BUFFER_SIZE (sizeof(struct sockaddr_storage) + 32)
#define NET_IP_STR_LEN 46 /* INET6_ADDRSTRLEN is 46 */

enum EOverlappedType // 操作类型
{
    E_OLT_ACCEPTEX = 0,
    E_OLT_CONNECTEX,
    E_OLT_WSARECV,
    E_OLT_WSASEND,
    E_OLT_WSADISCONNECTEX,
    E_OLT_UNKNOW
};
struct OverlappedBase
{
    OVERLAPPED kOverlapped;
    EOverlappedType eType;
};

struct CompleteKeySocket;
struct OverlappedListen
{
    OverlappedBase kOLBase;
    CompleteKeySocket* pkCKAccept;
    WSABUF kWSABuf;
    char pcBuf[ACCEPTEX_ADDRESS_BUFFER_SIZE * 2];

    OverlappedListen()
    {
        memset(this, 0, sizeof(*this));
    }
    OPERATOR_NEW_DELETE("OverlappedListen");
};

struct OverlappedSend
{
    OverlappedBase kOLBase;
    int iNumBuf;
    WSABUF pkWSABuf[NET_OL_BUF_NUM];
    NetBuffer pkBuffer[NET_OL_BUF_NUM];

    OverlappedSend()
    {
        memset(&kOLBase, 0, sizeof(kOLBase));
        iNumBuf = 0;
        memset(pkWSABuf, 0, sizeof(pkWSABuf));
    }
    OPERATOR_NEW_DELETE("OverlappedSend");
};

struct OverlappedRecv
{
    OverlappedBase kOLBase;
    WSABUF kWSABuf;
    NetBuffer kBuffer;

    OverlappedRecv();
    OPERATOR_NEW_DELETE("OverlappedRecv");
};

struct OverlappedConnect
{
    OverlappedBase kOLBase;

    OverlappedConnect()
    {
        memset(this, 0, sizeof(*this));
    }
    OPERATOR_NEW_DELETE("OverlappedConnect");
};

struct OverlappedDisconnect
{
    OverlappedBase kOLBase;

    OverlappedDisconnect()
    {
        memset(this, 0, sizeof(*this));
    }
    OPERATOR_NEW_DELETE("OverlappedDisconnect");
};

enum ECKType
{
    E_CK_LISTEN,
    E_CK_ACCEPT,
    E_CK_CONNECT
};

struct CompleteKeyBase
{
    HANDLE pvHandle; // 对象句柄 （socket， 文件）
    PTP_IO pkIO; // 绑定到 ThreadpoolIO 的对象
    ECKType eType;
};

struct CompleteKeyListen;

typedef std::list<NetBuffer, NetAllocator<NetBuffer>> NetBufferList;
typedef std::function<void(CompleteKeySocket*, NetBuffer&)> FunOnSend;
typedef std::function<void(CompleteKeySocket*, NetBuffer&)> FunOnRecv;
typedef std::function<void(CompleteKeySocket*, int)> FunOnError;
typedef std::function<void(CompleteKeySocket*)> FunOnConnect;
typedef std::function<void(CompleteKeySocket*)> FunOnDisconnect;
struct CompleteKeySocket
{
    CompleteKeyBase kCKBase;
    unsigned short usRemotePort; // 远程端口
    unsigned short usLocalPort;	// 本地端口
    char pcRemoteIp[NET_IP_STR_LEN]; // 远程ip
    char pcLocalIp[NET_IP_STR_LEN];	// 本地ip

    int iAF;

    CCriSec400 ccsSend;
    bool bEnable;
    bool bWait; // E_CK_CONNECT connect 后不立即 recv
    size_t szSendBytes;
    NetBufferList kSendList;

    NetBuffer kBufRecv;

    FunOnSend kFunOnSend;
    FunOnRecv kFunOnRecv;
    FunOnError kFunOnError;
    FunOnDisconnect kFunOnDisconnect;

    FunOnConnect kFunOnConnect; // E_CK_CONNECT

    CompleteKeyListen* pkCkListen; // E_CK_ACCEPT

    union 
    {
        __int64 llLastRecvTime;
        __int64 llLastDisconnectTime;
    };

    CompleteKeySocket(int AF);
    void init();

    OPERATOR_NEW_DELETE("CompleteKeySocket");
};

typedef std::function<
    void(CompleteKeyListen*, CompleteKeySocket*)> FunOnAccept;

struct CompleteKeyListen 
{
    CompleteKeyBase kCKBase;
    int iAF;
    FunOnAccept kFunOnAccept;
    FunOnError kFunOnError;

    volatile __int64 vllRecvBytes;
    volatile __int64 vllSendBytes;
	volatile long vlAcceptNum;

    CompleteKeyListen(int AF)
        :iAF(AF)
    {
        memset(&kCKBase, 0, sizeof(kCKBase));
        vllRecvBytes = 0;
        vllSendBytes = 0;
		vlAcceptNum = 0;
    }
    OPERATOR_NEW_DELETE("CompleteKeyListen");
};



// 获取 Socket 的某个扩展函数的指针
int getExtensionFuncPtr(LPVOID* plpf, SOCKET sock, GUID guid);

int postSend(CompleteKeySocket* pkCKAccept, OverlappedSend* pkOLSend);

int postAccept(LPFN_ACCEPTEX pfnAcceptEx,
    CompleteKeyListen* pkCkListen, OverlappedListen* pkOLSendListen);

int postRecv(CompleteKeySocket* pkCKAccept, OverlappedRecv* pkOLRecv);

int postDisconnect(LPFN_DISCONNECTEX pfnDisconnectEx, 
    CompleteKeySocket* pkCK, OverlappedDisconnect* pkOLDisconnect);

int postConnect(LPFN_CONNECTEX pfnConnectEx,
    CompleteKeySocket* pkCKConnect,
    OverlappedConnect* pkOLConnect,
    const SOCKADDR* name,
    int namelen);

void convertAddrs(char* pcIP, 
    unsigned short* pusPort, sockaddr_storage* pkAddr);

void getSocketAddrs(
    LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockAddrs,
    CompleteKeySocket* pkCKAccept,
    char* pAcceptInfo, int iRecvBytes);

void getSocketAddrs(CompleteKeySocket * pkCKConnect);

int setTcpNoDelay(SOCKET pvSocket);

int setSoAccept(SOCKET pvSocket, SOCKET pvSocketListen);

int setSoConnect(SOCKET pvSocket);

int setReUseAddr(SOCKET pvSocket);

int setOnlyIPV6(SOCKET pvSocket);

SOCKET createSocket(int & iError, int iAF);

int bindAndListen(SOCKET pvSocket,
    const SOCKADDR *socketAddr, socklen_t addrLen);

int bindCKSocket(int iAF, SOCKET pvSocket);

int prepareConnectAddr(int iAF,
    const char * pcIP,
    const char * pcPort, 
    addrinfo * pkAddrInfo);








