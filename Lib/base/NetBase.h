#pragma once
#include <string>
#include <sstream>

#include "NetConf.h"
#include "NetLog.h"

// ≤‚ ‘ƒ⁄¥Ê∑÷≈‰
#define NET_OPERATOR_NEW_DELETE_TEST 0

#if NET_OPERATOR_NEW_DELETE_TEST
#define OPERATOR_NEW_DELETE(name) \
void* operator new(size_t size) \
{\
    LOG_INFO << "operator new " << name \
    << " " << size;\
    return NET_NEW(size);\
}\
void operator delete(void* pvPtr)\
{\
    LOG_INFO << "operator delete " << name;\
    return NET_DELETE(pvPtr);\
}\
void* operator new[](size_t size)\
{\
    LOG_INFO << "operator new[] " << name \
        << " " << size;\
    return NET_NEW(size);\
}\
void operator delete[](void* pvPtr)\
{\
    LOG_INFO << "operator delete[] " << name;\
    return NET_DELETE(pvPtr);\
}
#else
#define OPERATOR_NEW_DELETE(name) \
void* operator new(size_t size) \
{\
    return NET_NEW(size);\
}\
void operator delete(void* pvPtr)\
{\
    return NET_DELETE(pvPtr);\
}\
void* operator new[](size_t size)\
{\
    return NET_NEW(size);\
}\
void operator delete[](void* pvPtr)\
{\
    return NET_DELETE(pvPtr);\
}
#endif



