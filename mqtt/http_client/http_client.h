#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

typedef struct HTTP_REQ_INFO{
    char host[64];
    char req_header[512];
    int port;
    int (*recv_deal_callback)(char*,int);
}HTTP_REQ_INFO_T;


enum HTTP_CALLBACK_STATE
{
    HTTP_RECV_CONTINUE = 0,
    HTTP_RECV_OVER,
    HTTP_RECV_ERROR
};

int SendHttpRequest(uint32_t arg);

#endif