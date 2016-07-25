#pragma once


// 定义内存分配
#define NET_NEW ::operator new
#define NET_DELETE ::operator delete

#define NET_OL_BUF_NUM 16

// 每个socket 对应的发送缓存 最大 长度 40k
#define NET_SOCKET_SEND_BUF_MAX 40960 

// 每个socket 对应的接收缓存 最大 长度 40k
#define NET_SOCKET_RECV_BUF_MAX 40960 

// 每次投递接收请求时 buf的长度
#define NET_POST_RECV_BUF_LEN 4096

// 确定 TCP/IP 可释放已关闭连接并重用其资源前，必须经过的时间。
// 需和注册表中的值对应 
// HKEY_LOCAL_MACHINE\ System\ CurrentControlSet\ Services\ TCPIP\ Parameters\ TcpTimedWaitDelay
// 注意： 此处填写的是毫秒数，注册表中为 秒
#define NET_TCP_TIME_WAIT_DELAY 30000





















