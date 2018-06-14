#ifndef _http_download_file_h_
#define _http_download_file_h_



typedef struct FOG_V3_OTA_INFO{
    char file_url[256];
    char ota_version[32];
    char component_name[64];
    char file_MD5[64];
    char custom_string[256];
}FOG_V3_OTA_INFO_T;

typedef struct {
    uint32_t file_len;
    int flash_type;
    uint32_t flash_addr;
}file_info_t;




int HttpClientAppStart(void);
FOG_V3_OTA_INFO_T* GetOtaContext(void);
int StartHttpClient(HTTP_REQ_INFO_T* http_req);





#endif