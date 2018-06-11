#include "mico.h"
#include "http_client.h"
#include "SocketUtils.h"
#include "cJSON.h"
#include "http_download_file.h"
#include "http_tool.h"

#define http_download_log(M, ...) custom_log("http download", M, ##__VA_ARGS__)


#define HTTP_OTA_HOST  "ota.fogcloud.io"


char HTTP_DONWLOAD_FILE_REQUEST[] =   {"\
GET %s HTTP/1.1\r\n\
Host: %s\r\n\
Accept: text/html,application\r\n\
Connection: keep-alive\r\n\r\n\
"};

char HTTP_REQUEST_HEADER_GET_OTA_INFO[] = {\
"PUT /v3/ota/product/ HTTP/1.1\r\n\
Host: ota.fogcloud.io\r\n\
Connection: Close\r\n\
Content-Type: application/json\r\n\
Content-Length: 69\r\n\r\n\
{\"productid\":\"cc085cdc581e11e8b7ac00163e30fc50\",\"dsn\":\"B0F893151E89\"}\r\n\r\n"
};

static FOG_V3_OTA_INFO_T ota_context = {0};
static uint8_t ota_ready = 0;

int GetOtaInfoFromHttpResponse(char* buf);
int DealWithOtaHttpResponse(char* buf,int len);

static char http_download_host[64] = {0};
static char http_download_request[512];
static file_info_t file_info = {0}; 


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
    bzero(download_file_req,sizeof(HTTP_REQ_INFO_T));       //可以的话对申请到的内存地址清零，否则会导致字符串拷贝的时候出问题
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
    strncpy(download_file_req->host,http_download_host,sizeof(http_download_host));
    strncpy(download_file_req->req_header,http_download_request,sizeof(http_download_request));
    download_file_req->port = 80;
    download_file_req->recv_deal_callback = DealWithDownloadFile;

    http_download_log("SendHttpRequest:%s",download_file_req->host);
    err = StartHttpClient(download_file_req);
    while(1);

    if(download_file_req != NULL)
        free(download_file_req);
    return 1;

    exit:
    if(download_file_req != NULL)
        free(download_file_req);
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
    //http_download_log("%s",__FUNCTION__);
    int err = 0;
    static uint8_t ota_start_flag = 0;
    static uint8_t ota_over_flag = 0;
    char *p = NULL;
    char *p_end = NULL;
    char http_state[3] = {0}; 
    char ota_size_str[10] = {0};
    static int offset_size = 0;
    static uint32_t file_length = 0;
    static uint32_t offset_addr = 0;
    uint32_t write_len = 0;
    p = buf;
    if(ota_start_flag == 0)
    {
        p = strstr(buf,"HTTP/1.1");
        p += (strlen("HTTP/1.1")+1);
        strncpy(http_state,p,3);
        http_download_log("http_state : %s",http_state);
        if(0 != (strcmp(http_state,"200"))) 
        {
            http_download_log("response error http state");
            goto exit;
        }
        p = strstr(p,"Content-Length:");
        if(p == NULL)
        {
            goto exit;
        }
        p += strlen("Content-Length:");
        p_end = strstr(p,"\r\n");
        if(p_end == NULL)
            goto exit;
        if((p_end - p) > 10)
        {
             goto exit;
        }
        strncpy(ota_size_str,p,p_end-p);
        http_download_log("ota size:%s",ota_size_str);
        offset_size = num_str_to_int(ota_size_str,p_end-p);
        http_download_log("offset size:%d",offset_size);
        ota_start_flag = 1;
        p = strstr(p,"\r\n\r\n");
        p += sizeof("\r\n\r\n");

        file_length = offset_size;
        mico_logic_partition_t* ota_partition = MicoFlashGetInfo(MICO_PARTITION_OTA_TEMP);
        file_info.file_len = ota_partition->partition_length;
        file_info.flash_type = MICO_PARTITION_OTA_TEMP;
        file_info.flash_addr = 0x00;
        MicoFlashErase(file_info.flash_type,file_info.flash_addr,file_info.file_len);
    }
    if(ota_start_flag == 1)
    {
        write_len = len-(p-buf);
        write_len = (write_len>file_length)?file_length:write_len;
        //err = MicoFlashWrite(MICO_PARTITION_OTA_TEMP,&offset_addr,p,write_len);
        //http_download_log("write in download offset = %d err = %d",offset_addr,err);
        file_length -= write_len;
        http_download_log("download file_length = %d",file_length);
        if(file_length == 0)
            ota_over_flag = 1;
    }
    
    if(ota_over_flag == 1)
        return HTTP_RECV_OVER;
    return HTTP_RECV_CONTINUE;
    exit:
    http_download_log("%s  err ",__FUNCTION__);
    return HTTP_RECV_ERROR;
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
        return HTTP_RECV_ERROR;
    sub_str = strstr(buf,"\r\n");
    len_temp = sub_str-str_temp;
    memcpy(http_response_state,str_temp,len_temp);
    if(strcmp(http_response_state,"HTTP/1.1 200 OK") != 0)      //状态码为200则response正常
    {
        http_download_log("bad http response!");
        return HTTP_RECV_ERROR;
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

    return HTTP_RECV_CONTINUE;

    exit:
    http_download_log("cJSON parse error");
    cJSON_Delete(p_json);
    cJSON_Delete(p_sub);
    return HTTP_RECV_OVER;
}


int StartHttpClient(HTTP_REQ_INFO_T* http_req)
{
    int err = 0;
    err =  mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "http client", SendHttpRequest,
                                    0x2000,
                                    (mico_thread_arg_t*)http_req );
    return err;
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

    err = StartHttpClient(http_get_url_req);    //发送获取file url的http请求
    if(err <= -1)
    {
        goto exit;
    }
    msleep(50);
    while(ota_ready == 0)
    {
        msleep(50);
    }
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
