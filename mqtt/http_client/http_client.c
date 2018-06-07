#include "mico.h"
#include "SocketUtils.h"
#include "cJSON.h"
#include "http_client.h"

#define http_client_log(M, ...) custom_log("HTTP_Client", M, ##__VA_ARGS__)


void SendHttpRequest(HTTP_REQ_INFO_T* req)
{
    int err = 0;
    http_client_log("start------------>%s",__FUNCTION__);
    struct sockaddr_in addr;
    int tcp_fd = -1;
    struct timeval t;
    fd_set readfds;
    int len;
    char *buf = NULL;
    char ipstr[16];
    struct hostent* hostent_content = NULL;
    char **pptr = NULL;
    struct in_addr in_addr;
    buf = (char*)malloc(1024);
    if (buf == NULL)
    {
        http_client_log("init buf fail");
        goto exit;
    }
    hostent_content = gethostbyname(req->host);
    if(hostent_content == NULL)
    {
        http_client_log("gethostbyname fail");
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
        goto exit;
    }

    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = in_addr.s_addr;
    addr.sin_port = htons(req->port);

    err = connect(tcp_fd,(struct sockaddr*)&addr,sizeof(addr));
    if(err == 0)
        {
            http_client_log("connect http server success");
        }
        else
        {
            http_client_log("connect http server fail");
            goto exit;
        }

    t.tv_sec = 2;
    t.tv_usec = 0;
    err = send(tcp_fd,req->req_header,strlen(req->req_header),0);
    if(err == 0)
        goto exit;
    msleep(2);
    http_client_log("send to http server:\r\n%s",req->req_header);
    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(tcp_fd,&readfds);
        if((select(tcp_fd+1,&readfds,NULL,NULL,&t))<0)
            continue;
        if(FD_ISSET(tcp_fd,&readfds))
        {
            len = recv(tcp_fd,buf,1024,0);
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
            http_client_log("recv data(len %d): %s",len,buf);
            req->recv_deal_callback(buf,len);
        }
    }
    exit:
    http_client_log("http client thread exit with err:%d",err);
    if(buf != NULL) free(buf);
    SocketClose(&tcp_fd);
}

