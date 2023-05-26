/**
 * @file aiot_state_api.h
 * @brief SDK Core状态码头文件, 所有Core中的api返回值均在此列出
 * @date 2021-12-17
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 *
 */

#ifndef _AIOT_STATE_API_H_
#define _AIOT_STATE_API_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief SDK的日志信息输出回调函数原型
 */
typedef int32_t (* aiot_state_logcb_t)(char *message);

/**
 * @brief 设置SDK的日志信息输出使用的回调函数
 *
 * @param handler 日志回调函数
 *
 * @return int32_t 保留
 */
int32_t aiot_state_set_logcb(aiot_state_logcb_t handler);


#define AIOT_TRUE    1
#define AIOT_FALSE    0

/**
 * @brief API执行成功
 *
 */
#define STATE_SUCCESS                                               (0x0000)

/**
 * @brief API执行失败
 *
 */
#define STATE_FAILED                                               (-0x0001)

/**
 * @brief API执行失败,参数配置错误
 *
 */
#define STATE_FAILED_PARAM_ERROR                                    (-0x0002)

/**
 * @brief 用户输入参数中包含越界的值
 *
 */
#define STATE_USER_INPUT_OUT_RANGE                                  (-0x0003)

/**
 * @brief CRC16校验失败
 */
#define STATE_FAILED_CRC16_CHECK_ERROR                               (-0x0004)

/**
 * @brief 申请内存失败
 *
 */
#define STATE_SYS_DEPEND_NOT_INIT                                   (-0x0005)

/**
 * @brief 环形缓冲区初始化失败配置为空的内存地址
 *
 */
#define STATE_AT_ALREADY_INITED                                     (-0x0006)

/**
 * @brief 缓冲区已满
 *
 */
#define STATE_AT_RINGBUF_OVERFLOW                                   (-0x0007)

/**
 * @brief 收到升级信息错误
 *
 */
#define STATE_RECV_OTA_PARAM_ERROR                                  (-0x0008)

/**
 * @brief OTA模块下载固件时出现校验和签名验证错误
 *
 * @details
 *
 * 固件的md5或者sha256计算结果跟云端通知的期望值不匹配所致的错误
 *
 */
#define STATE_OTA_DIGEST_MISMATCH                                   (-0x0009)

/**
 * @brief 用户输入的JSON字符串解析失败
 *
 */
#define STATE_USER_INPUT_JSON_PARSE_FAILED                          (-0x0100)

/**
 * @brief 申请内存失败
 *
 */
#define STATE_SYS_DEPEND_MALLOC_FAILED                              (-0x0101)

/**
 * @brief MQTT接收消息时, 消息中的Topic与可理解的Topic列表匹配失败
 *
 */
#define STATE_MQTT_TOPIC_COMPARE_FAILED                             (-0x0102)


/* -0x2000~-0x2FFF OS错误码 */
/**
 * @brief 内存资源不足
 *
 */
#define STATE_FAILED_RAM_NOT_ENOUGH                                 (-0x0103)

/**
 * @brief 创建线程失败
 *
 */
#define STATE_FAILED_CREATE_NEW_THREAD                              (-0x0104)

/**
 * @brief 读取数据失败
 *
 */
#define STATE_FAILED_NOT_READ_DATA                                  (-0x0105)

/**
 * @brief 返回的响应函数为空
 *
 */
#define STATE_FAILED_RESPONSE_IS_NULL                                (-0x0106)

/**
 * @brief AT client的会话为空
 *
 */
#define STATE_FAILED_CLIENT_IS_NULL                                 (-0x0107)

/**
 * @brief AT 应答超时
 *
 */
#define STATE_FAILED_AT_RESPONSE_TIMEOUT                            (-0x0108)

/**
 * @brief AT 收到应答错误
 *
 */
#define STATE_FAILED_AT_RESPONSE_NOT_OK                            (-0x0109)

/**
 * @brief 读数据是，临时缓冲的buf溢出
 *
 */
#define STATE_FAILED_AT_RECV_BUFFER_IS_FULL                         (-0x0110)

/**
 * @brief 创建Response失败
 *
 */
#define STATE_FAILED_CREATE_RESP_ERROR                             (-0x0111)

/**
 * @brief 配置WiFi连接信息失败
 *
 */
#define STATE_FAILED_CONNECT_WIFI_PARAM_ERROR                       (-0x0112)

/**
 * @brief WiFi连接失败
 *
 */
#define STATE_FAILED_CONNECT_WIFI_ERROR                               (-0x0113)

/**
 * @brief 用户输入参数中缺少deviceSecret
 *
 */
#define STATE_USER_INPUT_MISSING_DEVICE_SECRET                      (-0x0114)



///* 系统指令 */
#define AIOT_CMD_FMT_REBOOT_STR                          ("AT+NRB")
//#define AIOT_CMD_FMT_UART_STR                            ("AT+UART=%d,%d,%d,%d,%d")
//#define AIOT_CMD_FMT_CGMR_STR                            ("AT+CGMR")     /* 未实现 */
//#define AIOT_CMD_FMT_IDEFAULT_STR                        ("AT+IDEFAULT") /* 未实现 */
//#define AIOT_CMD_FMT_CWQAP_STR                           ("AT+CWQAP")    /* 未实现 */
//// /* HTTPC */
//// #define AIOT_CMD_FMT_HTTPC_STR                           ("AT+HTTPC=%d,%d,%s") 

///* 模组 AT命令格式 */
//#define AIOT_CMD_FMT_CWMODE_STR                         ("AT+CWMODE=%d")
//#define AIOT_CMD_FMT_CONNECT_STR                        ("AT+CWJAP=%s,%s")
//#define AIOT_CMD_FMT_NEWWORK_STATUS_STR                 ("AT+INETSTAT")
//#define AIOT_CMD_FMT_NEWWORK_STATUS_RSP_STR             ("+INETSTAT:")
//#define AIOT_CMD_FMT_NEWWORK_STATUS_RSP_SCANF_STR       ("+INETSTAT:%d,%s,%s")

///* 配网指令 */
//#define AIOT_CMD_FMT_IBLECONFIGSTART_STR                ("AT+IBLECONFIGSTART")
//#define AIOT_CMD_FMT_IBLECONFIGSTOP_STR                 ("AT+IBLECONFIGSTOP")
//#define AIOT_CMD_FMT_IAPCONFIGSTART_STR                 ("AT+IAPCONFIGSTART")
//#define AIOT_CMD_FMT_IAPCONFIGSTOP_STR                  ("AT+IAPCONFIGSTOP")

///* MQTT AT命令格式 */
//#define AIOT_CMD_FMT_IMQTTAUTH_DS_STR                   ("AT+IMQTTAUTH=%s,%s,%s")
//#define AIOT_CMD_FMT_IMQTTAUTH_PS_STR                   ("AT+IMQTTAUTH=%s,%s,,%s")
//#define AIOT_CMD_FMT_IMQTTAUTH_DPS_STR                  ("AT+IMQTTAUTH=%s,%s,%s,%s")
//#define AIOT_CMD_FMT_IMQTTPARA_STR                      ("AT+IMQTTPARA=%s,%d")
//#define AIOT_CMD_FMT_IMQTTCONN_STR                      ("AT+IMQTTCONN=%s,%d")
//#define AIOT_CMD_FMT_IMQTTPUB_STR                       ("AT+IMQTTPUB=%s,%d,%s")
//#define AIOT_CMD_FMT_IMQTTPUBHEX_STR                    ("AT+IMQTTPUBHEX=%s,%d,%d,%s")
//#define AIOT_CMD_FMT_IMQTTSUB_STR                       ("AT+IMQTTSUB=%s,%d")
//#define AIOT_CMD_FMT_IMQTTUNSUB_STR                     ("AT+IMQTTUNSUB=%s")
//#define AIOT_CMD_FMT_IMQTTCRED_STR                      ("AT+IMQTTCRED=%s,%s,%s")
//#define AIOT_CMD_FMT_IMQTTDISCONN_STR                   ("AT+IMQTTDISCONN")
//#define AIOT_CMD_FMT_IMQTTSTATE_STR                     ("AT+IMQTTSTATE=?")
//#define AIOT_CMD_FMT_IMQTTSTATE_RSP_STR                 ("+IMQTTSTATE:%d")
//#define AIOT_CMD_FMT_IMQTTRCVPUB_STR                    ("+IMQTTRCVPUB:%s,%d,%s")
//#define AIOT_CMD_FMT_IMQTTSUBACK_STR                    ("+IMQTTSUBACK:%d,%d")
//#define AIOT_CMD_FMT_IMQTTPUBACK_STR                    ("+IMQTTPUBACK:%d\r")
//#define AIOT_CMD_FMT_IMQTTUNSUBACK_STR                  ("+IMQTTUNSUBACK:%d")

///* 物模型 AT命令格式 */
//#define AIOT_CMD_FMT_IDMPP_STR                          ("AT+IDMPP=%s")                     /* 属性上报 */
//#define AIOT_CMD_FMT_IDMPS_RSP_STR                      ("+IDMPS:%ld,%d,%s")                 /* 属性下发 */
//#define AIOT_CMD_FMT_IDMEP_STR                          ("AT+IDMEP=%s,%s")                  /* 事件上报 */
//#define AIOT_CMD_FMT_IDMSS_RSP_STR                      ("+IDMSS:%ld,%s,%s,%d,%s")           /* 同步服务请求 */
//#define AIOT_CMD_FMT_IDMSSR_STR                         ("AT+IDMSSR=%s,%s,%s,%d,%s")        /* 同步服务回复 */
//#define AIOT_CMD_FMT_IDMGPR_RSP_STR                     ("+IDMGPR:%ld,%d,%s,%s")             /* 云端通用回复 */
//#define AIOT_CMD_FMT_IDMAS_RSP_STR                      ("+IDMAS:%ld,%s,%s")                 /* 异步服务请求 */
//#define AIOT_CMD_FMT_IDMASR_STR                         ("AT+IDMASR=%s,%s,%d,%s")           /* 异步服务回复命令 */

//#define AIOT_CMD_FMT_IDMAUTH_DS_STR                     ("AT+IDMAUTH=%s,%s,%s")             /* 物模型认证信息 */
//#define AIOT_CMD_FMT_IDMAUTH_PS_STR                     ("AT+IDMAUTH=%s,%s,,%s")            /* 物模型认证信息 */
//#define AIOT_CMD_FMT_IDMAUTH_DPS_STR                    ("AT+IDMAUTH=%s,%s,%s,%s")          /* 物模型认证信息 */
//#define AIOT_CMD_FMT_IDMPARA_STR                        ("AT+IDMPARA=%s,%d")                /* 物模型参数 */
//#define AIOT_CMD_FMT_IDMCONN_STR                        ("AT+IDMCONN=%s,%d")                /* 通过物模型连云 */
//#define AIOT_CMD_FMT_IDMSTATE_STR                       ("AT+IDMSTATE=?")
//#define AIOT_CMD_FMT_IDMSTATE_RSP_STR                   ("+IDMSTATE:%d")                    /* 查询当前物模型连接状态 */
//#define AIOT_CMD_FMT_IDMDISCONN_STR                     ("AT+IDMDISCONN")                   /* 物模型断开连接 */
//#define AIOT_CMD_FMT_IDMCRED_STR                        ("AT+IDMCRED=%s,%s,%s")             /* 物模型校验 */

///* OTA AT命令格式 */
//#define AIOT_CMD_FMT_IOTAPARA_STR                       ("AT+IOTAPARA=%s,%d")               /* 配置OTA参数 */
//#define AIOT_CMD_FMT_IOTAVER_STR                        ("AT+IOTAVER=%s")                   /* 上报MCU版本信息 */
//#define AIOT_CMD_FMT_IOTAQUERY_STR                      ("AT+IOTAQUERY")                    /* 主动查询云端是否有升级信息 */
//#define AIOT_CMD_FMT_IOTAINFO_RSP_STR                   ("+IOTASTATE:%d")                   /* 模组下行MCU升级固件信息 */
//#define AIOT_CMD_FMT_IOTASTART_STR                      ("AT+IOTASTART=%d")                 /* 允许下载MCU固件 */
//#define AIOT_CMD_FMT_IOTASHO_STR                        ("AT+IOTASHO")                      /* MCU请求固件帧头信息 */
//#define AIOT_CMD_FMT_IOTASHO_RSP_STR                    ("+IOTASHO:%d,%d,%s,%d,%d,%s")      /* 允许下载MCU固件 */
//#define AIOT_CMD_FMT_IOTASTX_STR                        ("AT+IOTASTX=%d")                   /* 请求模组传输对应的包编号 */
//#define AIOT_CMD_FMT_IOTASTX_RSP_STR                    ("+IOTASTX:%d,%d,%d")               /* 模组返回对应请求的分包 */
//#define AIOT_CMD_FMT_IOTAEOT_RSP_STR                    ("+IOTAEOT:%d")                     /* 模组固件包发送完成 */


#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _AIOT_STATE_API_H_ */

