#ifndef __tcp_client_app_h__
#define __tcp_client_app_h__

#define APP_TIMER_PRIOR 2000

typedef struct MSG{
    int len;
    unsigned char data[1];
}msg_t;

int TcpClientConnectSuccess(void);
int TcpClientConnectFail(void);
int TcpClientParaInit(tcp_client_t* p);
int TcpClientSendToServer(char* buf,int len);
int TcpClientRecvFromServer(char* buf,int len);

#endif