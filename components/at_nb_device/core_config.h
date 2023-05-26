
#ifndef __CORE_CONFIG_H__
#define __CORE_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif



#define CORE_AT_CLIENT_MODULE	            ("at_client")

/* 发送的数据+Command命令的最大长度 */
#define AIOT_SEND_CMD_MAX_SIZE              (2048)

/* 接收命令可能的最大长度，包括OTA升级包 */
#define AIOT_RECV_CMD_MAX_SIZE              (2048)

/* 超时等待的最大时间ms */
#define AIOT_WAIT_MAX_TIME                  (10000)

/* 接收数据包的最大长度 */
#define AIOT_DEFAULT_RING_BUFF_MAX_SIZE		(AIOT_RECV_CMD_MAX_SIZE)
/* 接收AT命令的最大长度 */
#define AIOT_DEFAULT_RECV_PUB_MAX_SIZE		(2560)

#define AIOT_DEAULT_RECV_DM_MSG_SIZE        (1024)
#define AIOT_DEAULT_RECV_DM_MSG_STATE_SIZE  (128)

// OS config
#define AIOT_DEFAULT_AT_THREAD_STACK_SIZE	(6*1024)
#define AIOT_DEFAULT_AT_THREAD_PRIOTITY		(3)


/* 模块功能选择 */
/* 使能基础的MQTT能力 */
#define AIOT_AT_MQTT_ENABLE			        (1)
/* 使能物模型能力 */
#define AIOT_AT_DATA_MODUL_ENABLE			(1)
/* 使能OTA能力 */
#define AIOT_AT_OTA_ENABLE                  (1)
/* 使能HTTP能力 */
#define AIOT_AT_HTTP_ENABLE                 (1)
/* 是否使能日志功能 */
#define AIOT_LOG_ENABLE                     (1)
/* 是否使能日志dump功能 */
#define AIOT_AT_DUMP_RAW_CMD                (0)



#ifdef __cplusplus
}
#endif

#endif
