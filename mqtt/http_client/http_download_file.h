#ifndef _http_download_file_h_
#define _http_download_file_h_



typedef struct FOG_V3_OTA_INFO{
    char file_url[256];
    char ota_version[32];
    char component_name[64];
    char file_MD5[64];
    char custom_string[256];
}FOG_V3_OTA_INFO_T;


int HttpClientAppStart(void);
FOG_V3_OTA_INFO_T* GetOtaContext(void);






#endif