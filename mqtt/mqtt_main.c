#include "mico.h"
#include "mico_app_define.h"
#include "mqtt_client_interface.h"

#define mqtt_log(M, ...) custom_log("mqtt", M, ##__VA_ARGS__)

static mico_queue_t mqtt_queue;
static mico_timer_t mqtt_timer;

OSStatus MqttTopicPublish(char* topic,char* payload,int len);
static void MqttTimerHandler(void);

char *mqtt_client_id_get( char clientid[30] )
{
    uint8_t mac[6];
    char mac_str[13];

    mico_wlan_get_mac_address( mac );
    sprintf( mac_str, "%02X%02X%02X%02X%02X%02X",
             mac[0],
             mac[1], mac[2], mac[3], mac[4], mac[5] );
    sprintf( clientid, "MiCO_%s", mac_str );

    return clientid;
}


void iot_subscribe_callback_handler( MQTT_Client *pClient, char *topicName,
                                     uint16_t topicNameLen,
                                     IoT_Publish_Message_Params *params,
                                     void *pData )
{
    IOT_UNUSED( pData );
    IOT_UNUSED( pClient );
    mqtt_log("Subscribe callback");
    mqtt_log("%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *)(params->payload));
}

static void mqtt_sub_pub_main( mico_thread_arg_t arg )
{
    IoT_Error_t rc = MQTT_FAILURE;

    char clientid[40];
    char cPayload[100];
    mqtt_msg_t* mqtt_msg_pop;
    int i = 0;
    MQTT_Client client;
    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;
    IoT_Publish_Message_Params paramsQOS0;
    //IoT_Publish_Message_Params paramsQOS1;

    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    mqttInitParams.enableAutoReconnect = false;
    mqttInitParams.pHostURL = MQTT_HOST;
    mqttInitParams.port = MQTT_PORT;
    mqttInitParams.mqttPacketTimeout_ms = 20000;
    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.disconnectHandler = NULL;
    mqttInitParams.disconnectHandlerData = NULL;
#ifdef MQTT_USE_SSL
    mqttInitParams.pRootCALocation = MQTT_ROOT_CA_FILENAME;
    mqttInitParams.pDeviceCertLocation = MQTT_CERTIFICATE_FILENAME;
    mqttInitParams.pDevicePrivateKeyLocation = MQTT_PRIVATE_KEY_FILENAME;
    mqttInitParams.isSSLHostnameVerify = false;
    mqttInitParams.isClientnameVerify = false;
    mqttInitParams.isUseSSL = true;
#else
    mqttInitParams.pRootCALocation = NULL;
    mqttInitParams.pDeviceCertLocation = NULL;
    mqttInitParams.pDevicePrivateKeyLocation = NULL;
    mqttInitParams.isSSLHostnameVerify = false;
    mqttInitParams.isClientnameVerify = false;
    mqttInitParams.isUseSSL = false;
#endif

    rc = mqtt_init( &client, &mqttInitParams );
    if ( MQTT_SUCCESS != rc )
    {
        mqtt_log("aws_iot_mqtt_init returned error : %d ", rc);
        goto exit;
    }

    connectParams.keepAliveIntervalInSec = 30;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    connectParams.pClientID = MQTT_CLIENT_ID;
    connectParams.clientIDLen = (uint16_t) strlen( MQTT_CLIENT_ID );
    connectParams.isWillMsgPresent = false;
    connectParams.pUsername = NULL;//MQTT_USERNAME;
    connectParams.usernameLen = 0;//strlen(MQTT_USERNAME);
    connectParams.pPassword = NULL;//MQTT_PASSWORD;
    connectParams.passwordLen = 0;//strlen(MQTT_PASSWORD);

RECONN:
    mqtt_log("Connecting...");
    rc = mqtt_connect( &client, &connectParams );
    if ( MQTT_SUCCESS != rc )
    {
        mqtt_log("Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
        sleep(2);
        goto RECONN;
    }

    mqtt_log("Subscribing  : %s",MQTT_SUB_NAME);
    rc = mqtt_subscribe( &client, MQTT_SUB_NAME, strlen( MQTT_SUB_NAME ), QOS0,
                         iot_subscribe_callback_handler, NULL );
    if ( MQTT_SUCCESS != rc )
    {
        mqtt_log("Error subscribing : %d ", rc);
        goto RECONN;
    }

    mqtt_log("publish...");
    sprintf( cPayload, "%s : %d ", "hello from SDK", i );

    paramsQOS0.qos = QOS0;
    paramsQOS0.payload = (void *) cPayload;
    paramsQOS0.isRetained = 0;

    // paramsQOS1.qos = QOS1;
    // paramsQOS1.payload = (void *) cPayload;
    // paramsQOS1.isRetained = 0;

    while ( 1 )
    {
        //Max time the yield function will wait for read messages
        rc = mqtt_yield( &client, 100 );
        if ( MQTT_SUCCESS != rc )
        {
            // If the client is attempting to reconnect we will skip the rest of the loop.
            mico_rtos_thread_sleep( 1 );
            mqtt_log("mqtt disconnect");
            mqtt_disconnect( &client );
            goto RECONN;
        }
        if(mico_rtos_pop_from_queue(&mqtt_queue,&mqtt_msg_pop,0) == 0)
        {
            if(mqtt_msg_pop == NULL)
                continue;
            paramsQOS0.payloadLen = mqtt_msg_pop->len;
            paramsQOS0.payload = (void*)mqtt_msg_pop->data;
            rc = mqtt_publish(&client,mqtt_msg_pop->topic,strlen(mqtt_msg_pop->topic),&paramsQOS0);
            mqtt_log("publish err = %d, %s(%d) : %s",rc,mqtt_msg_pop->topic,mqtt_msg_pop->len,mqtt_msg_pop->data);
            free(mqtt_msg_pop);
        }
        else
            continue;
        #if 0
        mqtt_log("-->sleep, rc:%d", rc);
        sprintf( cPayload, "%s : %d ", "hello from SDK QOS0", i++ );
        paramsQOS0.payloadLen = strlen( cPayload );
        mqtt_publish( &client, MQTT_PUB_NAME, strlen( MQTT_PUB_NAME ), &paramsQOS0 );
        sprintf( cPayload, "%s : %d ", "hello from SDK QOS1", i++ );
        paramsQOS1.payloadLen = strlen( cPayload );
        rc = mqtt_publish( &client, MQTT_PUB_NAME, strlen( MQTT_PUB_NAME ), &paramsQOS1 );
        if ( rc == MQTT_REQUEST_TIMEOUT_ERROR )
        {
            mqtt_log("QOS1 publish ack not received");
            rc = MQTT_SUCCESS;
        }
#endif
    }

    exit:
    mico_rtos_delete_thread( NULL );
}

OSStatus start_mqtt_sub_pub( void )
{
#ifdef MQTT_USE_SSL
    uint32_t stack_size = 0x3000;
#else
    uint32_t stack_size = 0x2000;
#endif
mico_rtos_init_queue(&mqtt_queue,"mqtt queue",sizeof(int),8);
mico_rtos_init_timer(&mqtt_timer,MQTT_TIMER_PERIO,MqttTimerHandler,NULL);
mico_rtos_start_timer(&mqtt_timer);
return mico_rtos_create_thread( NULL, MICO_APPLICATION_PRIORITY, "mqtt", mqtt_sub_pub_main,
                                    stack_size,
                                    0 );
}
/*************************************************************************************************
 * @description:mqtt timer定时函数
 * @param none
 * @return none
**************************************************************************************************/
static void MqttTimerHandler(void)
{
    char str_temp[30] = {};
    static int i = 0;
    sprintf(str_temp,"%s : %d","hello mqtt timer handle",i++);
    mqtt_log("%s",str_temp);
    if((i%2) == 0)
        MqttTopicPublish(MQTT_PUB_NAME,str_temp,sizeof(str_temp));
    else
        MqttTopicPublish(MQTT_PUB_NAME1,str_temp,sizeof(str_temp));
    mqtt_log ("free Memory %d bytes", MicoGetMemoryInfo()->free_memory);  
    if(i>10000)
        i = 0;
}
/*************************************************************************************************
 * @description:mqtt Publish接口，将需要publish的数据放入队列中
 * @param topic：publish的topic
 * @param payload:publish的数据
 * @param len:数据长度
 * @return: -1:返回不正常，0:返回正常
**************************************************************************************************/
OSStatus MqttTopicPublish(char* topic,char* payload,int len)
{
    int err = 0;
    mqtt_msg_t* mqtt_msg_push = NULL;
    mqtt_msg_push = (mqtt_msg_t*)malloc(sizeof(mqtt_msg_t)-1+len);
    if(NULL == mqtt_msg_push)
    {
        mqtt_log("malloc mqtt_msg_push error");
        return -1;
    }
    mqtt_msg_push->len = len;
    mqtt_msg_push->topic = topic;
    memcpy(mqtt_msg_push->data,payload,len);
    err = mico_rtos_push_to_queue(&mqtt_queue,&mqtt_msg_push,0);
    if(err != 0)
    {
        mqtt_log("push mqtt_msg_push fail");
        if(mqtt_msg_push != NULL)
            free(mqtt_msg_push);
        return -1;
    }
    return 0;
}
