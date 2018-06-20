#include "mico.h"
#include "udp_client.h"

#define udp_log(M, ...) custom_log("udp client", M, ##__VA_ARGS__)

void UdpClientThread(void *arg)
{
    udp_log("start %s",__FUNCTION__);
    UDP_CONTEXT_T* udp_para = (UDP_CONTEXT_T*)arg;
    udp_log("udp.remote_port = %d",udp_para->remote_port);
    int err = 0;
    struct sockaddr_in addr;
    fd_set readfds;
    int len = 0;
    char ip_address[16];
    char buf[20] = {0};
    udp_para->udp_fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    #if 0
    if(IsValidSocket(udp_para->udp_fd) != 0)
    {
        udp_log("create udp fd fail,%d",IsValidSocket(udp_para->udp_fd));
        goto exit;
    }
    #endif
    strcpy(buf,"hello hjian");
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_BROADCAST;
    addr.sin_port = htons(8080);
    udp_log("broadcast port ...");
    while(1)
    {
        err = sendto(udp_para->udp_fd,buf,strlen(buf),0,(struct sockaddr*)&addr,sizeof(addr));
        udp_log("udp send err = %d ...",err);
        msleep(2000);
    }

    exit:
    udp_log("udp thread exit with err = %d",err);
    mico_rtos_delete_thread(NULL);
    return ;
}