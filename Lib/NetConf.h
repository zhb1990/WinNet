#pragma once


// �����ڴ����
#define NET_NEW ::operator new
#define NET_DELETE ::operator delete

#define NET_OL_BUF_NUM 16

// ÿ��socket ��Ӧ�ķ��ͻ��� ��� ���� 40k
#define NET_SOCKET_SEND_BUF_MAX 40960 

// ÿ��socket ��Ӧ�Ľ��ջ��� ��� ���� 40k
#define NET_SOCKET_RECV_BUF_MAX 40960 

// ÿ��Ͷ�ݽ�������ʱ buf�ĳ���
#define NET_POST_RECV_BUF_LEN 4096

// ȷ�� TCP/IP ���ͷ��ѹر����Ӳ���������Դǰ�����뾭����ʱ�䡣
// ���ע����е�ֵ��Ӧ 
// HKEY_LOCAL_MACHINE\ System\ CurrentControlSet\ Services\ TCPIP\ Parameters\ TcpTimedWaitDelay
// ע�⣺ �˴���д���Ǻ�������ע�����Ϊ ��
#define NET_TCP_TIME_WAIT_DELAY 30000





















