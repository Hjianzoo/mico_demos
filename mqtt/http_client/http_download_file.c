#include "mico.h"
#include "http_client.h"
#include "SocketUtils.h"
#include "cJSON.h"
#include "http_download_file.h"

#define http_download_log(M, ...) custom_log("http download", M, ##__VA_ARGS__)


#define HTTP_OTA_HOST  "ota.fogcloud.io"


#define HTTP_DONWLOAD_FILE_REQUEST  "\
GET %s HTTP/1.1\r\n\
Host: %s\r\n\
Accept: text/html,application\r\n\
Connection: keep-alive\r\n\
Upgrade-Insecure-Requests: 1\r\n\r\n\
"

char HTTP_REQUEST_HEADER_GET_OTA_INFO[] = {\
"PUT /v3/ota/product/ HTTP/1.1\r\n\
Host: ota.fogcloud.io\r\n\
Connection: Close\r\n\
Content-Type: application/json\r\n\
Content-Length: 69\r\n\r\n\
{\"productid\":\"cc085cdc581e11e8b7ac00163e30fc50\",\"dsn\":\"B0F893151E9F\"}\r\n\r\n"
};

static FOG_V3_OTA_INFO_T ota_context = {0};
static uint8_t ota_ready = 0;

int GetOtaInfoFromHttpResponse(char* buf);
int DealWithOtaHttpResponse(char* buf,int len);

static char http_download_host[64] = {0};
static char http_download_request[512];



FOG_V3_OTA_INFO_T* GetOtaContext(void)
{
    return &ota_context;
}
int DealWithDownloadFile(char* buf,int len);



/*************************************************************************************************
 * @description:开始固件下载
 * @param none
 * @reutrn: 0为正常，-1为异常
**************************************************************************************************/
int HttpClientOtaFileDownload(void)
{
    int err;
    FOG_V3_OTA_INFO_T* ota_info = GetOtaContext();      //Get已经得到的云端ota信息
    HTTP_REQ_INFO_T* download_file_req = NULL;          //定义固件下载的http req
    download_file_req = (HTTP_REQ_INFO_T*)malloc(sizeof(HTTP_REQ_INFO_T));
    if(download_file_req == NULL)
    {
        http_download_log("init download_file_req error");
        goto exit;
    }

    char *p = ota_info->file_url;               //获取host
    p = strstr(p,"http://");
    if( p == NULL)
        goto exit;
    p += strlen("http://");
    char *p_end = strchr(p,'/');

    strncpy(http_download_host,p,p_end-p);          //将获取到的host地址copy到req中
    sprintf(http_download_request,HTTP_DONWLOAD_FILE_REQUEST,ota_info->file_url,http_download_host);        //组包
    strncpy(download_file_req->host,http_download_host,strlen(http_download_host));
    strncpy(download_file_req->req_header,http_download_request,strlen(http_download_request));
    download_file_req->port = 80;
    download_file_req->recv_deal_callback = DealWithDownloadFile;


    err = SendHttpRequest(download_file_req);
    while(1);
    return 1;

    exit:
    return -1;
}

/*************************************************************************************************
 * @description:固件下载时的处理函数
 * @param buf:接收的数据起始指针
 * @param len:数据长度
 * @reutrn: 0为正常， -1为异常
**************************************************************************************************/
int DealWithDownloadFile(char* buf,int len)
{
    http_download_log("%s",__FUNCTION__);
    return 0;
}

/*************************************************************************************************
 * @description:处理http请求服务器返回ota信息的响应数据
 * @param buf:服务器返回的数据指针
 * @param len:数据长度
 * @reutrn: 0为正常， -1为异常
**************************************************************************************************/
int DealWithOtaHttpResponse(char* buf,int len)
{
    char* str_temp = NULL;
    char* sub_str = NULL;
    int len_temp = 0;
    char http_response_state[20];
    str_temp = strstr(buf,"HTTP/1.1");
    if(str_temp == NULL)
        return 1;
    sub_str = strstr(buf,"\r\n");
    len_temp = sub_str-str_temp;
    memcpy(http_response_state,str_temp,len_temp);
    if(strcmp(http_response_state,"HTTP/1.1 200 OK") != 0)      //状态码为200则response正常
    {
        http_download_log("bad http response!");
        return -1;
    }
    str_temp = strchr(buf,'{');     //获取response数据的起始地址
    sub_str = strstr(str_temp,"\r\n");  //获取response数据的结束地址
    len_temp = sub_str-str_temp;        //获取response数据的长度 

    return GetOtaInfoFromHttpResponse(str_temp);    //从response数据中提取ota信息
}


/*************************************************************************************************
 * @description:从服务器返回的response数据中提取ota固件信息
 * @param buf:数据起始指针
 * @reutrn: 提取ota信息的结果，-1为异常，0为正常
**************************************************************************************************/
int GetOtaInfoFromHttpResponse(char* buf)
{
    cJSON *p_json,*p_sub;
    p_json = cJSON_Parse(buf);      //将字符串格式化为json格式
    if(p_json == NULL)
    {
        http_download_log("cJSON parse error");
        goto exit;
    }
    // char *data2 = cJSON_Print(p_sub);    //将json对象以字符串形式打印出来
    // http_client_log("pritn json : %s",data2);

    p_json = cJSON_GetObjectItem(p_json,"data");    //从json对象中根据字符串提取次级json对象
    p_json = cJSON_GetObjectItem(p_json,"files");
    //int array_size = cJSON_GetArraySize(p_json);  //获取file的数量（获取json数组中的sizeof(list))
    p_json = cJSON_GetArrayItem(p_json,0);      //获取json数组中的第一个json对象

    p_sub = cJSON_GetObjectItem(p_json,"file_url"); //提取file url
    if(p_sub == NULL)
    {
        goto exit;
    }
    strcpy(ota_context.file_url,p_sub->valuestring);    //json格式中value为字符串，则直接获取 json_object->valuestring,若为int数值，则返回json_object->valueint

    p_sub = cJSON_GetObjectItem(p_json,"component");       
    if(p_sub == NULL)
    {
        goto exit;
    }
    strcpy(ota_context.component_name,p_sub->valuestring);

    p_sub = cJSON_GetObjectItem(p_json,"md5");
    if(p_sub == NULL)
    {
        goto exit;
    }
    strcpy(ota_context.file_MD5,p_sub->valuestring);

    p_sub = cJSON_GetObjectItem(p_json,"version");
    if(p_sub == NULL)
    {
        goto exit;
    }
    strcpy(ota_context.ota_version,p_sub->valuestring);

    p_sub = cJSON_GetObjectItem(p_json,"customize");
    if(p_sub == NULL)
    {
        http_download_log("cJSON error");
        goto exit;
    }
    strcpy(ota_context.custom_string,p_sub->valuestring);

    http_download_log("file_url:%s",ota_context.file_url);        
    http_download_log("file_component:%s",ota_context.component_name);
    http_download_log("ota_version:%s",ota_context.ota_version);
    http_download_log("file_md5:%s",ota_context.file_MD5);
    http_download_log("custom_string:%s",ota_context.custom_string);
    ota_ready = 1;                                                         //获取ota信息成功，可以进行下一步
    cJSON_Delete(p_json);                                                  //释放已经定义的json对象
    cJSON_Delete(p_sub);

    return 0;

    exit:
    http_download_log("cJSON parse error");
    cJSON_Delete(p_json);
    cJSON_Delete(p_sub);
    return -1;
}



/*************************************************************************************************
 * @description:获取ota固件起始函数
 * @param none
 * @reutrn: -1:fail,返回其他值为成功；
**************************************************************************************************/
int HttpClientAppStart(void)
{
    int err = 0;
    HTTP_REQ_INFO_T* http_get_url_req = NULL;
    http_get_url_req = (HTTP_REQ_INFO_T*)malloc(sizeof(HTTP_REQ_INFO_T));
    if(http_get_url_req == NULL)
    {
        http_download_log("init http_get_url_req error");
        goto exit;
    }

    strncpy(http_get_url_req->host,HTTP_OTA_HOST,sizeof(HTTP_OTA_HOST));        //初始化http req
    strncpy(http_get_url_req->req_header,HTTP_REQUEST_HEADER_GET_OTA_INFO,sizeof(HTTP_REQUEST_HEADER_GET_OTA_INFO));
    http_get_url_req->port = 80;
    http_get_url_req->recv_deal_callback = DealWithOtaHttpResponse;     


    err = SendHttpRequest(http_get_url_req);    //发送获取file url的http请求
    if(err <= -1)
    {
        goto exit;
    }
    msleep(50);
    if(ota_ready)           //如果获取ota信息成功，则开始下载ota固件
    {
        HttpClientOtaFileDownload();
    }
    free(http_get_url_req);
    return err;
    exit:
    free(http_get_url_req);
    return -1;
}
