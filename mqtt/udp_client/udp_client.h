#ifndef _UDP_CLIENT_H_
#define _UDP_CLIENT_H_

typedef int (*recv_func_t)(char*,int);
typedef int (*send_func_t)(char*,int);

enum {
    UDP_CLIENT = 0,
    UDP_SERVER
};

typedef struct UDP_CONTEXT{
    int udp_type;
    int udp_fd;
    char ip_str[20];
    int remote_port;
    int local_port;
    recv_func_t recv_deal;
    send_func_t send_data;
}UDP_CONTEXT_T;

void UdpClientThread(void *arg);
void UdpServerThread(void* arg);
#endif