#include "mico.h"
#include "udp_client.h"

#define udp_log(M, ...) custom_log("udp client", M, ##__VA_ARGS__)


/*************************************************************************************************
 * @description:Udp客户端线程，在固定端口向局域网内或指定ip地址发送数据包，
 * @param arg：一个指向udp context的指针
 * @reutrn: none
**************************************************************************************************/
void UdpClientThread(void *arg)
{
    char* send_data = NULL;
    send_data = GetModuleInfo();
    udp_log("start %s",__FUNCTION__);
    UDP_CONTEXT_T* udp_para = (UDP_CONTEXT_T*)arg;
    udp_log("udp.remote_port = %d",udp_para->remote_port);
    int err = 0;
    struct sockaddr_in addr;
    int len = 0;
    char recv_buf[256] = {0};
    udp_para->udp_fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(IsValidSocket(udp_para->udp_fd) == 0)
    {
        udp_log("create udp fd fail,%d",IsValidSocket(udp_para->udp_fd));
        goto exit;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_BROADCAST;
    addr.sin_port = htons(udp_para->remote_port);
    udp_log("broadcast port ...");
    while(1)
    {
        err = sendto(udp_para->udp_fd,send_data,strlen(send_data),0,(struct sockaddr*)&addr,sizeof(addr));
        udp_log("udp send err = %d ...",err);
        msleep(2000);
        
    }

    exit:
    udp_log("udp client thread exit with err = %d",err);
    mico_rtos_delete_thread(NULL);
    return ;
}

/*************************************************************************************************
 * @description:Udp服务器线程，在固定端口接收局域网之内的广播包
 * @param none
 * @reutrn: none
**************************************************************************************************/
void UdpServerThread(void* arg)
{
    udp_log("start %s",__FUNCTION__);
    UDP_CONTEXT_T* udp_para = (UDP_CONTEXT_T*)arg;
     int err = 0;
    struct sockaddr_in addr;
    char recv_buf[1024] = {0};
    int len = 0;
    udp_para->udp_fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(IsValidSocket(udp_para->udp_fd) == 0)
    {
        udp_log("create udp fd fail,%d",IsValidSocket(udp_para->udp_fd));
        goto exit;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(udp_para->local_port);
    bind( udp_para->udp_fd, (struct sockaddr *) &addr, sizeof(addr) );
    while(1)
    {
        len = recvfrom(udp_para->udp_fd,recv_buf,sizeof(recv_buf),0,NULL,NULL);
        udp_log("udp recv len %d:%s",len,recv_buf);
    }
    exit:
    udp_log("udp server thread exit with err = %d",err);
    mico_rtos_delete_thread(NULL);
    return ;
}