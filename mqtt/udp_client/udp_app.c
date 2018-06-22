#include "mico.h"
#include "udp_client.h"

#define udp_log(M, ...) custom_log("udp app", M, ##__VA_ARGS__)



int CreateUdpClient(UDP_CONTEXT_T* udp_context)
{
    if(udp_context->udp_type == UDP_CLIENT)
    {
        mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "udp_client", UdpClientThread,
                                    0x800,
                                    (mico_thread_t*)udp_context );
    }
    else{
        mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "udp_server", UdpServerThread,
                                    0x800,
                                    (mico_thread_t*)udp_context );
    }
    
}


void UdpUserAppStart(void)
{
    UDP_CONTEXT_T udp_connect;
    memset(&udp_connect,0,sizeof(UDP_CONTEXT_T));
    udp_connect.udp_type = UDP_SERVER;
    udp_connect.udp_fd = -1;
    strncpy(udp_connect.ip_str,"",0);
    udp_connect.remote_port = 8080;
    udp_connect.local_port = 8090;
    CreateUdpClient(&udp_connect);
}