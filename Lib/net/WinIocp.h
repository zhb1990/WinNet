#pragma once

#include <map>
#include <deque>
#include <set>
#include <unordered_set>
#include "WinNetHelp.h"

class WinIOCP
{
private:
    WinIOCP();

public:
    typedef std::vector<CompleteKeyListen*,
        NetAllocator<CompleteKeyListen*>> VectorCKListen;

    static WinIOCP* getInstance();

    //************************************
    // Method:    reqListen
    // FullName:  WinIOCP::reqListen
    // Access:    public 
    // Returns:   WinIOCP::VectorCKListen
    // Qualifier: 只添加监听，不提供释放
    // Parameter: int iAF
    // Parameter: const char * pcIP 若未指定则为 ADDR_ANY
    // Parameter: const char * pcPort
    // Parameter: size_t szAcceptNum
    // Parameter: const FunOnAccept & rFunAccept
    // Parameter: const FunOnError & rFunError
    //************************************
    VectorCKListen reqListen(int iAF,
        const char * pcIP,
        const char * pcPort,
        size_t szAcceptNum,
        const FunOnAccept& rFunAccept,
        const FunOnError& rFunError);

    CompleteKeySocket* reqConnect(int iAF, 
        const char * pcIP,
        const char * pcPort,
        const FunOnConnect& rFunConnect,
        const FunOnError& rFunError);

    CompleteKeySocket* reqConnectAndWait(int iAF, 
        const char * pcIP,
        const char * pcPort,
        const FunOnConnect& rFunConnect,
        const FunOnError& rFunError);

    int reqSend(CompleteKeySocket* pkCK, const char* pcSend, size_t szLen);
    int reqSend(CompleteKeySocket* pkCK, NetBuffer& rSend);

    //************************************
    // Method:    reqDisconnect
    // FullName:  WinIOCP::reqDisconnect
    // Access:    public 
    // Returns:   bool
    // Qualifier: 连接不一定立即断开, 返回是否成功调用 postDisconnect
    // Parameter: CompleteKeySocket * pkCK
    //************************************
    bool reqDisconnect(CompleteKeySocket* pkCK);

    //************************************
    // Method:    reqDisconnectAndWait
    // FullName:  WinIOCP::reqDisconnectAndWait
    // Access:    public 
    // Returns:   bool
    // Qualifier: 如果 成功调用 postDisconnect 函数会阻塞，直到断开连接成功
    //            否则 立即返回
    // Parameter: CompleteKeySocket * pkCK
    //************************************
    bool reqDisconnectAndWait(CompleteKeySocket* pkCK);
    
    friend VOID CALLBACK IoCompletionCallback(
        PTP_CALLBACK_INSTANCE Instance,
        PVOID Context,
        PVOID Overlapped,
        ULONG IoResult,
        ULONG_PTR NumberOfBytesTransferred,
        PTP_IO Io);

    //************************************
    // Method:    prepareCkSocket
    // FullName:  WinIOCP::prepareCkSocket
    // Access:    protected 
    // Returns:   void
    // Qualifier: 预先创建多少个 用于accept 或 connect的完成键
    // Parameter: int iAF
    // Parameter: size_t szNum
    // Parameter: ECKType eType
    //************************************
    void prepareCkSocket(int iAF, size_t szNum, ECKType eType);
protected:
    void initExtensionFuncPtr();

    CompleteKeySocket* newCompleteKeySocket(int iAF, ECKType eType);
    void delCompleteKeySocket(CompleteKeySocket*);

    CompleteKeySocket* _prepareCkSocket(int iAF, ECKType eType);

    CompleteKeyListen* _reqListen(int iAF,
        const addrinfo * pkAddrInfo,
        size_t szAcceptNum,
        const FunOnAccept& rFunAccept,
        const FunOnError& rFunError);

    void reqAccept(CompleteKeyListen* pkCKListen);

    void reqAccept(CompleteKeyListen* pkCKListen, size_t szAcceptNum);

    CompleteKeySocket* _reqConnect(int iAF,
        const char * pcIP,
        const char * pcPort,
        const FunOnConnect& rFunConnect,
        const FunOnError& rFunError,
        bool bWait);

    int reqRecv(CompleteKeySocket*);

    int autoPostSend(CompleteKeySocket* pkCK, 
        OverlappedSend* pkOL, 
        size_t szSendLast);

    void handSend(ULONG IoResult,
        CompleteKeyBase * pkCKBase,
        OverlappedBase* pkOLBase,
        ULONG_PTR NumberOfBytesTransferred);
    void handRecv(ULONG IoResult,
        CompleteKeyBase * pkCKBase,
        OverlappedBase* pkOLBase,
        ULONG_PTR NumberOfBytesTransferred);
    void handAccept(ULONG IoResult,
        CompleteKeyBase * pkCKBase,
        OverlappedBase* pkOLBase,
        ULONG_PTR NumberOfBytesTransferred);
    void handConnect(ULONG IoResult,
        CompleteKeyBase * pkCKBase,
        OverlappedBase* pkOLBase,
        ULONG_PTR NumberOfBytesTransferred);
    void handDisconnect(ULONG IoResult,
        CompleteKeyBase * pkCKBase,
        OverlappedBase* pkOLBase,
        ULONG_PTR NumberOfBytesTransferred);
    void handError(CompleteKeySocket* pkCK, int iError);

protected:
    LPFN_ACCEPTEX _pfnAcceptEx;
    LPFN_DISCONNECTEX _pfnDisconnectEx;
    LPFN_GETACCEPTEXSOCKADDRS _lpfnGetAcceptExSockAddrs;
    LPFN_CONNECTEX _pfnConnectEx;

    typedef std::deque<CompleteKeySocket*, 
        NetAllocator<CompleteKeySocket*>> DequeCKSocket;

    typedef std::set<CompleteKeySocket*,
        std::less<CompleteKeySocket*>,
        NetAllocator<CompleteKeySocket*>> SetCKSocket;

    typedef std::vector<VectorCKListen,
        NetAllocator<VectorCKListen>> VectorListen;

    CCriSec400 _ccsListen;
    VectorListen _vectorListen;

    CCriSec400 _ccsCKSocketV6Accept;
    DequeCKSocket _dequeCKSocketIPV6Accept;
    SetCKSocket _setCKUsedV6Accept;

    CCriSec400 _ccsCKSocketV4Accept;
    DequeCKSocket _dequeCKSocketIPV4Accept;
    SetCKSocket _setCKUsedV4Accept;

    CCriSec400 _ccsCKSocketV6Connect;
    DequeCKSocket _dequeCKSocketIPV6Connect;
    SetCKSocket _setCKUsedV6Connect;

    CCriSec400 _ccsCKSocketV4Connect;
    DequeCKSocket _dequeCKSocketIPV4Connect;
    SetCKSocket _setCKUsedV4Connect;
};

#define IocpPrepareCompleteKey WinIOCP::getInstance()->prepareCkSocket

#define IocpListen WinIOCP::getInstance()->reqListen 

#define IocpSend WinIOCP::getInstance()->reqSend

#define IocpConnect WinIOCP::getInstance()->reqConnect
#define IocpConnectAndWait WinIOCP::getInstance()->reqConnectAndWait

#define IocpDisconnect WinIOCP::getInstance()->reqDisconnect
#define IocpDisconnectAndWait WinIOCP::getInstance()->reqDisconnectAndWait
















