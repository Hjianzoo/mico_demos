#ifndef __TCP_CLIENT_H_
#define __TCP_CLIENT_H_
#define TCP_CLIENT_STACK_SIZE            0x500
#define TCP_CLIENT_SEND_STACK_SIZE       0x600

typedef int (*pfunc)(void);
typedef int (*option_func)(char*,int);

typedef struct tcp_client_para{
    int tcp_fd;
    char remote_ip[20];
    uint16_t remote_port;
    uint16_t local_port;
    pfunc conn_suc_callback;
    pfunc conn_fail_callback;
    //option_func process_send;
    option_func process_recv;
}tcp_client_t;


void StartTcpClient(void);
void Tcp_Client_Thread(uint32_t arg);
int GetTcpClientConnetStatus(void);


#endif