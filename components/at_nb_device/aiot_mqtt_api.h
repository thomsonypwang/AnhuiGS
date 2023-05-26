#ifndef _AIOT_MQTT_H_
#define _AIOT_MQTT_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include "core_list.h"

#define AIOT_MQTT_PRODUCT_KEY_MAX_SIZE             	(64)
#define AIOT_MQTT_DEVICE_NAME_MAX_SIZE             	(64)
#define AIOT_MQTT_DEVICE_SECRET_MAX_SIZE            (64)
#define AIOT_MQTT_PRODUCT_SECRET_MAX_SIZE           (64)

#define AIOT_MQTT_QOS0                              (0x00)
#define AIOT_MQTT_QOS1                              (0x01)
#define AIOT_MQTT_QOS_MAX                           (1)
#define AIOT_MQTT_TOPIC_MAXLEN                      (1024)

typedef enum
{
    AIOT_MQTT_STA_UNKNOWN,
    AIOT_MQTT_STA_CONNECTED,
    AIOT_MQTT_STA_DISCONNECTED,
} aiot_mqtt_conn_state_s;

/**
 * @brief MQTT内部事件类型
 */
typedef enum {
	
	CLIENT_NO_INIT=0,//client 参数未初始化
	CLIENT_ALREADY_INIT,//client 参数已初始化
	MQTT_SERVER_DISCONNECT,//和服务器已断开
	SEND_CONNECT,//发送 Connect 包，等待接收服务器 ack
	MQTT_SERVER_RECONNECT,//正在重连服务器
	MQTT_SERVER_READY_CONNECT,//和服务器已连接
	MQTT_SERVER_TCP_CONNECTING,//建立 TCP 连接中。
	MQTT_SERVER_TCP_ALREADY,//TCP 连接已建立
} aiot_mqtt_event_type_t;

/**
 * @brief MQTT事件回调函数
 *
 * @details
 *
 * 当MQTT内部事件被触发时, 调用此函数. 如连接成功/断开连接/重连成功
 *
 */
typedef void (*aiot_mqtt_event_handler_t)(const aiot_mqtt_event_type_t event, void *userdata);


typedef enum 
{
    /* TIMEOUT: MQTT请求超时时间，单位s */
    AIOT_PARAM_OPT_TIMEOUT,
    /* KEEPALIVE: 与云端约定的心跳超时间隔,单位秒 */
    AIOT_PARAM_OPT_KEEPALIVE,
//    AIOT_PARAM_OPT_AUTHMODE,
//    AIOT_PARAM_OPT_TLS,
    AIOT_PARAM_MAX
}aiot_mqtt_param_t;

/**
 * @brief MQTT报文类型
 *
 * @details
 *
 * 传入@ref aiot_mqtt_recv_handler_t 的MQTT报文类型
 */
typedef enum {
    /**
     * @brief MQTT PUBLISH报文
     */
    AIOT_MQTTRECV_PUB,

    /**
     * @brief MQTT SUBACK报文
     */
    AIOT_MQTTRECV_SUB_ACK,

    /**
     * @brief MQTT UNSUB报文
     */
    AIOT_MQTTRECV_UNSUB_ACK,

    /**
     * @brief MQTT PUBACK报文
     */
    AIOT_MQTTRECV_PUB_ACK,

} aiot_mqtt_recv_type_t;

typedef struct {
    /**
     * @brief MQTT报文类型, 更多信息请参考@ref aiot_mqtt_recv_type_t
     */
    aiot_mqtt_recv_type_t type;
     /**
     * @brief MQTT PUBLISH报文
     */
    struct {
        char *topic;
        uint16_t topic_len;
        uint8_t *payload;
        uint32_t payload_len;
    } pub;
    /**
     * @brief AIOT_MQTTRECV_SUB_ACK
     */
    struct {
        int32_t res;
        uint16_t packet_id;
    } sub_ack;
    /**
     * @brief AIOT_MQTTRECV_UNSUB_ACK
     */
    struct {
        uint16_t packet_id;
    } unsub_ack;
    /**
     * @brief AIOT_MQTTRECV_PUB_ACK
     */
    struct {
        uint16_t packet_id;
    } pub_ack;

} aiot_mqtt_recv_t;

/**
 * @brief MQTT报文接收回调函数原型
 *
 * @param[in] packet MQTT报文结构体, 存放收到的MQTT报文
 *
 * @return void
 */
typedef void (*aiot_mqtt_recv_handler_t)(aiot_mqtt_recv_t *packet, void *userdata);

typedef struct 
{
    aiot_mqtt_conn_state_s state;
    aiot_mqtt_recv_handler_t recv_handler;  /* 默认免订阅的消息会调用该回调 */
    aiot_mqtt_event_handler_t event_handler;
    void *userdata;

    struct core_list_head sub_list;
    void *sub_mutex;
}aiot_mqtt_handle_t;



/**
 * @brief 初始化mqtt的配置参数
 * 
 * @param[in] 
 * param 参数配置
 * value 参数值
 * 
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败, 更多信息请参考@ref aiot_state_api.h
 * @retval >=STATE_SUCCESS 执行成功
 *
 */
//int32_t aiot_mqtt_set_param(aiot_mqtt_param_t param, uint32_t value);

/**
 * @brief 连接MQTT物联网平台
 * 
 * @param[in] host 远程服务器地址 
 * @param[in] port 端口
 *
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败, 更多信息请参考@ref aiot_state_api.h
 * @retval >=STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_mqtt_connect(uint8_t *host_ip, uint16_t host_port,char *host_id,char *host_user,char *host_pass, 
	aiot_mqtt_recv_handler_t recv_handler, aiot_mqtt_event_handler_t even_handler, void *userdata);
/**
 * @brief 断开MQTT连接
 * 
 *
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败, 更多信息请参考@ref aiot_state_api.h
 * @retval >=STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_mqtt_disconnect(void);

/**
 * @brief 向服务端pub消息，字符串格式
 * 
 *
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败, 更多信息请参考@ref aiot_state_api.h
 * @retval >=STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_mqtt_pub(const char *topic, int32_t qos, char *payload);

/**
 * @brief 向服务端pub消息，hex模式
 * 
 *
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败, 更多信息请参考@ref aiot_state_api.h
 * @retval >=STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_mqtt_pubhex(const char *topic, int32_t qos, uint32_t payload_len, uint8_t *payload);

/**
 * @brief 向服务端订阅消息
 * 
 *
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败, 更多信息请参考@ref aiot_state_api.h
 * @retval >=STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_mqtt_sub(const char *topic, int32_t qos, aiot_mqtt_recv_handler_t handler, void *userdata);

/**
 * @brief 取消服务端订阅消息
 * 
 *
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败, 更多信息请参考@ref aiot_state_api.h
 * @retval >=STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_mqtt_unsub(const char *topic);

/**
 * @brief 查询服务端的状态
 * 
 *
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败, 更多信息请参考@ref aiot_state_api.h
 * @retval >=STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_mqtt_query_state(void);

int qsdk_mqtt_open(void);
int qsdk_mqtt_get_connect(void);

#if defined(__cplusplus)
}
#endif

#endif
