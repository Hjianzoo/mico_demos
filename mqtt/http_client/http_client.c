#include "mico.h"
#include "SocketUtils.h"
#include "cJSON.h"
#include "http_client.h"

#define http_client_log(M, ...) custom_log("HTTP_Client", M, ##__VA_ARGS__)

/*************************************************************************************************
 * @description:发送一个http请求并处理响应数据
 * @param req:HTTP_REQ_INFO_T结构体类型的入口参数，包括HOST、REQ Header、端口号以及处理相应数据的回调
 * @reutrn: -1:fail,返回其他值为成功；
**************************************************************************************************/
int SendHttpRequest(uint32_t arg)
{
    int err = 0;
    http_client_log("start------------>%s",__FUNCTION__);
    HTTP_REQ_INFO_T* req = (HTTP_REQ_INFO_T*)arg;
    struct sockaddr_in addr;
    int tcp_fd = -1;
    struct timeval t;
    fd_set readfds;
    int len;
    //char *buf = NULL;
    char buf[2048] = {0};
    char ipstr[16];
    struct hostent* hostent_content = NULL;
    char **pptr = NULL;
    struct in_addr in_addr;
    //buf = (char*)malloc(1024);
    if (buf == NULL)
    {
        http_client_log("init buf fail");
        err = -1;
        goto exit;
    }
    hostent_content = gethostbyname(req->host);
    if(hostent_content == NULL)
    {
        http_client_log("gethostbyname fail: %s",req->host);
        err = -1;
        goto exit;
    }
    pptr = hostent_content->h_addr_list;
    in_addr.s_addr = *(uint32_t*)(*pptr);
    strcpy(ipstr,inet_ntoa(in_addr));
    http_client_log("HTTP server address:%s,host ip:%s",req->host,ipstr);

    tcp_fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(IsValidSocket(tcp_fd) == 0)
    {
        http_client_log("int tcp_fd fail");
        err = -1;
        goto exit;
    }

    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = in_addr.s_addr;
    addr.sin_port = htons(req->port);

    err = connect(tcp_fd,(struct sockaddr*)&addr,sizeof(addr));     //连接http服务器
    if(err == 0)
        {
            http_client_log("connect http server success");
        }
        else
        {
            http_client_log("connect http server fail");
            err = -1;
            goto exit;
        }

    t.tv_sec = 3;
    t.tv_usec = 0;
    err = send(tcp_fd,req->req_header,strlen(req->req_header),0);       //发送http请求
    if(err <= 0)
    {
        err = -1;
        goto exit;
    }
    msleep(2);
    http_client_log("send to http server(len:%d):\r\n%s",err,req->req_header);
    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(tcp_fd,&readfds);
        if(select(tcp_fd+1,&readfds,NULL,NULL,&t)<0)
            goto exit;
        http_client_log("http client idle");
        if(FD_ISSET(tcp_fd,&readfds))
        {
            len = recv(tcp_fd,buf,2048,0);
            if(len<0)
            {
                http_client_log("recv fail");
                continue;
            }
            if(len == 0)
            {
                http_client_log("TCP Client disconnected,fd:%d",tcp_fd);
                goto exit;
            }
            buf[len] = '\0';
           http_client_log("recv data(len %d)",len);
            err = req->recv_deal_callback(buf,len);   //利用req的回调函数处理响应数据
            if(err == HTTP_RECV_OVER)
                goto exit;
        }
    }
    exit:
    http_client_log("http client thread exit with err:%d",err);
    //if(buf != NULL) free(buf);
    SocketClose(&tcp_fd);
    mico_rtos_delete_thread( NULL );
    return err;
}

