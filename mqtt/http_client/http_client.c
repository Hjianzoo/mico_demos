#include "mico.h"
#include "SocketUtils.h"

#define http_client_log(M, ...) custom_log("HTTP_Client", M, ##__VA_ARGS__)

char HTTP_REQUEST_HEADER_1[] = {\
"PUT /v3/ota/product/ HTTP/1.1\r\n\
Host: ota.fogcloud.io\r\n\
Connection: Close\r\n\
Content-Type: application/json\r\n\
Content-Length: 69\r\n\r\n\
{\"productid\":\"cc085cdc581e11e8b7ac00163e30fc50\",\"dsn\":\"B0F893151E9F\"}\r\n\r\n"
};

#define HTTP_OTA_HOST  "ota.fogcloud.io"


void GetDownloadUrlTcpClientThread(uint32_t arg)
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
        http_client_log("int buf fail");
        goto exit;
    }
    hostent_content = gethostbyname(HTTP_OTA_HOST);
    if(hostent_content == NULL)
    {
        http_client_log("gethostbyname fail");
        goto exit;
    }
    pptr = hostent_content->h_addr_list;
    in_addr.s_addr = *(uint32_t*)(*pptr);
    strcpy(ipstr,inet_ntoa(in_addr));
    http_client_log("HTTP server address:%s,host ip:%s",HTTP_OTA_HOST,ipstr);

    tcp_fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(IsValidSocket(tcp_fd) == 0)
    {
        http_client_log("int tcp_fd fail");
        goto exit;
    }

    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = in_addr.s_addr;
    addr.sin_port = htons(80);

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
    err = send(tcp_fd,HTTP_REQUEST_HEADER_1,sizeof(HTTP_REQUEST_HEADER_1),0);
    if(err == 0)
        goto exit;
    msleep(2);
    http_client_log("send to http server:\r\n%s",HTTP_REQUEST_HEADER_1);
    // err = send(tcp_fd,HTTP_REQUEST_PREVIEW_1,sizeof(HTTP_REQUEST_PREVIEW_1),0);
    // if(err == 0)
    //     goto exit;
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
        }
    }
    exit:
    http_client_log("http client thread exit with err:%d",err);
    if(buf != NULL) free(buf);
    SocketClose(&tcp_fd);
    mico_rtos_delete_thread(NULL);
}


int HttpClientAppStart(void)
{
    int err = 0;
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "http client",GetDownloadUrlTcpClientThread,0x600,0);
    if(err != 0)
        http_client_log("create tcp client thread fail");
    return err;
}