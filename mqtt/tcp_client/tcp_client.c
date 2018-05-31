#include "mico.h"
#include "SocketUtils.h"
#include "tcp_client.h"
#include "tcp_client_app.h"

#define tcp_client_log(M, ...) custom_log("TCP", M, ##__VA_ARGS__)

tcp_client_t tcp_client = {0};
static int tcp_fd = -1;
static int connect_flag = 0;
int tcp_client_send_flag = 0;
mico_queue_t socket_queue = NULL;





/***************************************************************************************************
 * @description:获取tcp client连接状态
 * @return ： 1-已连接，0-未连接
 **************************************************************************************************/
int GetTcpClientConnetStatus(void)
{
    return connect_flag;
}


/***************************************************************************************************
 * @description:tcp client 线程
 * @param arg:开启线程的传参
 **************************************************************************************************/
void Tcp_Client_Thread(uint32_t arg)
{
    int err = 0;
    tcp_client_log("start tcp client thread");
    struct sockaddr_in addr;
    struct timeval t;
    fd_set readfds;
    int len;
    char *buf = NULL;

    buf = (char*)malloc(1024);
    if (buf == NULL)
    {
        tcp_client_log("int buf fail");
        goto exit;
    }

    tcp_fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(IsValidSocket(tcp_fd) == 0)
    {
        tcp_client_log("int tcp_fd fail");
        goto exit;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(tcp_client.remote_ip);
    addr.sin_port = htons(tcp_client.remote_port);

    tcp_client_log("Connecting to server: ip=%s port=%d",tcp_client.remote_ip,tcp_client.remote_port);
    err = connect(tcp_fd,(struct sockaddr*)&addr,sizeof(addr));
    if(err == 0)
    {
         tcp_client.conn_suc_callback();
         connect_flag = 1;
         tcp_client_log("connect success");
    }
    else
    {
        tcp_client_log("connect fail");
        goto exit;
    }
    
    t.tv_sec = 2;
    t.tv_usec = 0;

    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(tcp_fd,&readfds);

        if((select(tcp_fd+1,&readfds,NULL,NULL,&t))<0)
            goto exit;
        if(FD_ISSET(tcp_fd,&readfds))
        {
            len = recv(tcp_fd,buf,1024,0);
            if (len < 0)
            {
                tcp_client_log("recv fail");
                goto exit;
            }

            if(len == 0)
            {
                tcp_client_log("TCP Client disconnected,fd:%d",tcp_fd);
                goto exit;
            }

            tcp_client_log("Client fd:%d,recv data %d",tcp_fd,len);
            tcp_client.process_recv(buf,len);
           // len = send(tcp_fd,buf,len,0);
           // tcp_client_log("Client fd: %d,send data %d,len");
        }
    }
    exit:
    if(err != 0) tcp_client_log("tcp client thread exit with err:%d",err);
    connect_flag = 0;
    tcp_client.conn_fail_callback();
    if(buf != NULL) free(buf);
    SocketClose(&tcp_fd);
    mico_rtos_delete_thread(NULL);
}

/*************************************************************************************************
 * @description:Tcp Client发送数据至服务端线程
 * @param none
 * @reutrn none
**************************************************************************************************/
void Tcp_Client_Send_Thread(uint32_t arg)
{
    int err = 0;
    msg_t* msg = NULL;
    tcp_client_log("start tcp client send thread");
    while(GetTcpClientConnetStatus() == 0)
    {
        //tcp_client_log("connect flag = %d,Get flag = %d",connect_flag,GetTcpClientConnetStatus());
        msleep(300);
    }
    tcp_client_log("get tcp client connect status success");
    while(1)
    {
        while(mico_rtos_pop_from_queue(&socket_queue,&msg,0) != 0);
        if(GetTcpClientConnetStatus() == 0)
        {
            tcp_client_log("tcp client disconnect!"); 
            free(msg);
            continue;
        }
        tcp_client_log("%d  %s",msg->len,msg->data);   
        err = send(tcp_fd,msg->data,msg->len,0);
        if(err < 0)
        { 
            tcp_client_log("send data to server fail err = %d",err);
        }
        tcp_client_log("send %d data to server: %s",msg->len,msg->data);  
        free(msg);
    }   
    mico_rtos_delete_thread(NULL);
}


/*************************************************************************************************
 * @description:开启tcp client 线程
 * @param none
**************************************************************************************************/
void StartTcpClient(void)
{
    int err = 0;
    TcpClientParaInit(&tcp_client);
    err = mico_rtos_init_queue(&socket_queue,"socket queue",sizeof(int),8);
    if(err != 0)
    {
        tcp_client_log("init socket queue fail");
    }
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "tcp client",Tcp_Client_Thread,TCP_CLIENT_STACK_SIZE,0);
    if(err != 0)
        tcp_client_log("create tcp client thread fail");
    
    err = mico_rtos_create_thread(NULL,MICO_APPLICATION_PRIORITY,"tcp_client_send",Tcp_Client_Send_Thread,TCP_CLIENT_SEND_STACK_SIZE,0);
    if(err != 0)
        tcp_client_log("create tcp client send thread fail");
}


