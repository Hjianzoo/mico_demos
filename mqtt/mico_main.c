#include "mico.h"
#include "mico_app_define.h"
//#include "http_client.h"
#define app_log(M, ...) custom_log("APP", M, ##__VA_ARGS__)
#define app_log_trace() custom_log_trace("APP")

extern OSStatus start_mqtt_sub_pub( void );

static mico_semaphore_t wait_sem = NULL;

static mico_timer_t app_sys_timer;

/* MICO system callback: Restore default configuration provided by application */
void appRestoreDefault_callback( void * const user_config_data, uint32_t size )
{
    memset( user_config_data, 0x0, size );
}

static void micoNotify_WifiStatusHandler( WiFiEvent status, void* const inContext )
{
    switch ( status )
    {
        case NOTIFY_STATION_UP:
            if( wait_sem != NULL ){
                mico_rtos_set_semaphore( &wait_sem );
            }
            break;
        case NOTIFY_STATION_DOWN:
            case NOTIFY_AP_UP:
            case NOTIFY_AP_DOWN:
            break;
    }
}

void AppMianTimerHandler(void* arg)
{
    static int i;

    if((i++)%5 == 0)
         app_log("free Memory %d bytes", MicoGetMemoryInfo()->free_memory);  

    if(i>10000)
        i = 0;
}
void ApplicationThread(uint32_t arg)
{
    #ifdef MQTT_TEST
    start_mqtt_sub_pub();
    #endif

    #ifdef TCP_CLIENT_TEST
    TcpClientAppStart();
    #endif

    #ifdef HTTP_CLIENT_TEST
    HttpClientAppStart();
    #endif
    mico_rtos_delete_thread( NULL );
}

int application_start( void )
{
    app_log_trace();
    OSStatus err = kNoErr;
    mico_Context_t* mico_context;

    mico_rtos_init_semaphore( &wait_sem, 1 );

    /*Register user function for MiCO nitification: WiFi status changed */
    err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED,
                                       (void *) micoNotify_WifiStatusHandler,
                                       NULL );
    require_noerr( err, exit );

    /* Create mico system context and read application's config data from flash */
    mico_context = mico_system_context_init( 0 );

    /* mico system initialize */
    err = mico_system_init( mico_context );
    require_noerr( err, exit );

    mico_rtos_init_timer(&app_sys_timer,1000,AppMianTimerHandler,NULL);
    mico_rtos_start_timer(&app_sys_timer);

    /* Wait for wlan connection*/
    mico_rtos_get_semaphore( &wait_sem, MICO_WAIT_FOREVER );
    app_log("wifi connected successful");

    err = mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "application", ApplicationThread,
                                    0x2000,
                                    0 );
    if (err < 0)
        app_log("create app thread fail");
    

    exit:
    mico_system_notify_remove( mico_notify_WIFI_STATUS_CHANGED,
                               (void *) micoNotify_WifiStatusHandler );
    mico_rtos_deinit_semaphore( &wait_sem );
    mico_rtos_delete_thread( NULL );
    return err;
}

