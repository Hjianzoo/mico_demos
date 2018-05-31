#include "mico.h"
#include "tcp_client.h"
#include "tcp_client_app.h"

#define tcp_client_app_log(M, ...) custom_log("TCP_APP", M, ##__VA_ARGS__)


extern mico_queue_t socket_queue;

mico_timer_t app_timer;


/*************************************************************************************************
 * @description:Tcp Client参数初始化函数
 * @param none
 * @return : 0
**************************************************************************************************/
int TcpClientParaInit(tcp_client_t* p)
{
    strcpy(p->remote_ip,"192.168.1.104");
    p->remote_port = 6000;
    p->local_port = 8000;
    p->conn_suc_callback = TcpClientConnectSuccess;
    p->conn_fail_callback = TcpClientConnectFail;
    p->process_recv = TcpClientRecvFromServer;
}

/*************************************************************************************************
 * @description:Tcp client连接成功回调
 * @param none
 * @return : 0
**************************************************************************************************/
int TcpClientConnectSuccess(void)
{
    tcp_client_app_log("p->conn_suc_callback:%s",__FUNCTION__);
}

/*************************************************************************************************
 * @description:Tcp client连接失败回调
 * @param none
 * @return : 0
**************************************************************************************************/
int TcpClientConnectFail(void)
{
    tcp_client_app_log("p->conn_fail_callback:%s",__FUNCTION__);
}



/*************************************************************************************************
 * @description:Tcp Client接收服务器内容
 * @param buf: 发送缓存buf
 * @param len: 发送长度
 * @return : 0
**************************************************************************************************/
int TcpClientRecvFromServer(uint8_t* buf,int len)
{
    buf[len] = '\0';
    tcp_client_app_log("tcp client recv from server: %s",buf);
    TcpClientSendToServer(buf,len);
    return 0; 
}


/*************************************************************************************************
 * @description:Tcp Client发送至服务器内容
 * @param buf: 发送缓存buf
 * @param len: 发送长度
 * @return : 0
**************************************************************************************************/
int TcpClientSendToServer(uint8_t* buf,int len)
{
    msg_t* push_msg;
    msg_t* pop_msg;
    int err = 0;
    push_msg = (msg_t*)malloc(sizeof(msg_t)-1+len);
    if(push_msg == NULL)
    {
        tcp_client_app_log("init msg fail");
        goto exit;
    }
    push_msg->len = len;
    memcpy(push_msg->data,buf,len);
    if(socket_queue == NULL)
    {
        tcp_client_app_log("socket_queue is NULL");
        goto exit;
    }
    tcp_client_app_log("send data to server:%s len:%d",push_msg->data,push_msg->len);
    tcp_client_app_log("push msg addr :%d",push_msg);
    err = mico_rtos_push_to_queue(&socket_queue,&push_msg,0);
    if(err != 0)
    {
        tcp_client_app_log("push socket data fail");
        goto exit;
    }
    return 0;

    exit:
    return -1;
}

/*************************************************************************************************
 * @description:定时器中断内容
 * @param none
 * @return  none
**************************************************************************************************/
void AppTimerHandler(uint32_t arg)
{
    int err = 0;
    msg_t* pop_msg = NULL;
    
    tcp_client_app_log("%s  connetstatus: %d",__FUNCTION__,GetTcpClientConnetStatus());
    char *buf_temp = "hello hjian";
    TcpClientSendToServer(buf_temp,strlen(buf_temp));
    tcp_client_app_log ("free Memory %d bytes", MicoGetMemoryInfo()->free_memory);  
}


/*************************************************************************************************
 * @description:TCP client应用入口函数；
 * @param none
 * @return: -1:返回不正常；
**************************************************************************************************/
int TcpClientAppStart(void)
{
    int err = 0;
    StartTcpClient();
    err = mico_rtos_init_timer(&app_timer,APP_TIMER_PRIOR,AppTimerHandler,NULL);

    if(err != 0)
    {
        tcp_client_app_log("init timer fail");
        goto exit;
    }
    mico_rtos_start_timer(&app_timer);
    return;
    exit:
    return -1;
}
