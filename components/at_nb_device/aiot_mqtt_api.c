/**
 * @file aiot_mqtt_api.c
 * @brief mqtt相关处理api，包括mqtt的参数配置、连接、订阅、发布
 * @date 2021-12-17
 *
 * @copyright Copyright (C) 2015-2021 Alibaba Group Holding Limited
 *
 */

#include <aiot_state_api.h>
#include "aiot_at_client_api.h"
#include "aiot_mqtt_api.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "posix_port.h"
#include "core_log.h"
#include "sys_os.h"
//#include "core_auth.h"

typedef struct
{
    char *suback_topic;
    char *puback_topic;
    char *pub_mesg;
    int connect_status;
    int error_type;
    int suback_status;
    int unsuback_status;
    int puback_status;
    int puback_open;
    int pub_mesg_len;
    int pubrec_status;
    int pubcomp_status;
    int packid;
} MQTT_DEVICE;

MQTT_DEVICE mqtt_device;

aiot_mqtt_handle_t g_mqtt_handle = { 0 };
static int32_t _core_mqtt_sublist_insert(char *topic,
                                         aiot_mqtt_recv_handler_t handler, void *userdata);
static void _core_mqtt_sublist_remove(char *topic);
static void _core_mqtt_sublist_destroy(aiot_sysdep_portfile_t *sysdep);
static int32_t _core_mqtt_topic_compare(char *topic, uint32_t topic_len, char *cmp_topic, uint32_t cmp_topic_len);

void _urc_pub_recv_handler(const char *data, uint32_t size);
//void _urc_puback_handler(const char *data, uint32_t size);
//void _urc_suback_handler(const char *data, uint32_t size);
//void _urc_unsuback_handler(const char *data, uint32_t size);

typedef struct {
    char *topic;
    aiot_mqtt_recv_handler_t handler;
    void *userdata;
    struct core_list_head linked_node;
} core_mqtt_sub_node_t;

static int32_t _core_mqtt_sublist_insert(char *topic,aiot_mqtt_recv_handler_t handler, void *userdata)
{
    int32_t res = STATE_SUCCESS;
    core_mqtt_sub_node_t *node = NULL;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL || g_mqtt_handle.sub_mutex == NULL) 
	{
        return STATE_SYS_DEPEND_NOT_INIT;
    }
    sysdep->core_sysdep_mutex_lock(g_mqtt_handle.sub_mutex);
    core_list_for_each_entry(node, &g_mqtt_handle.sub_list, linked_node, core_mqtt_sub_node_t) 
	{
        if (node->topic != NULL && memcmp(node->topic, topic, strlen(topic)) == 0) 
		{
            node->handler = handler;
            node->userdata = userdata;
        }
    }
    if (&node->linked_node == &g_mqtt_handle.sub_list) 
	{
        /* new topic */
        node = sysdep->core_sysdep_malloc(sizeof(core_mqtt_sub_node_t), CORE_AT_CLIENT_MODULE);
        if (node == NULL) 
		{
            sysdep->core_sysdep_mutex_unlock(g_mqtt_handle.sub_mutex);
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(node, 0, sizeof(core_mqtt_sub_node_t));
        CORE_INIT_LIST_HEAD(&node->linked_node);

        node->topic = sysdep->core_sysdep_malloc(strlen(topic) + 1, CORE_AT_CLIENT_MODULE);
        if (node->topic == NULL) 
		{
            sysdep->core_sysdep_free(node);
            sysdep->core_sysdep_mutex_unlock(g_mqtt_handle.sub_mutex);
            return STATE_SYS_DEPEND_MALLOC_FAILED;
        }
        memset(node->topic, 0, strlen(topic) + 1);
        memcpy(node->topic, topic, strlen(topic));
        node->handler = handler;
        node->userdata = userdata;
        core_list_add_tail(&node->linked_node, &g_mqtt_handle.sub_list);
    }
    sysdep->core_sysdep_mutex_unlock(g_mqtt_handle.sub_mutex);
    return res;
}

static void _core_mqtt_sublist_remove(char *topic)
{
    core_mqtt_sub_node_t *node = NULL, *next = NULL;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    if (sysdep == NULL || g_mqtt_handle.sub_mutex == NULL) 
	{
        return;
    }
    sysdep->core_sysdep_mutex_lock(g_mqtt_handle.sub_mutex);
    core_list_for_each_entry_safe(node, next, &g_mqtt_handle.sub_list, linked_node, core_mqtt_sub_node_t) 
	{
        if ((strlen(node->topic) == strlen(topic)) && memcmp(node->topic, topic, strlen(topic)) == 0) 
		{
            core_list_del(&node->linked_node);
            sysdep->core_sysdep_free(node->topic);
            sysdep->core_sysdep_free(node);
        }
    }
    sysdep->core_sysdep_mutex_unlock(g_mqtt_handle.sub_mutex);
}

static void _core_mqtt_sublist_destroy(aiot_sysdep_portfile_t *sysdep)
{
    core_mqtt_sub_node_t *node = NULL, *next = NULL;
    if (g_mqtt_handle.sub_mutex == NULL) 
	{
        return;
    }
    sysdep->core_sysdep_mutex_lock(g_mqtt_handle.sub_mutex);
    core_list_for_each_entry_safe(node, next, &g_mqtt_handle.sub_list, linked_node, core_mqtt_sub_node_t) 
	{
        core_list_del(&node->linked_node);
        sysdep->core_sysdep_free(node->topic);
        sysdep->core_sysdep_free(node);
    }
    sysdep->core_sysdep_mutex_unlock(g_mqtt_handle.sub_mutex);
}

static int32_t _core_mqtt_topic_compare(char *topic, uint32_t topic_len, char *cmp_topic, uint32_t cmp_topic_len)
{
    uint32_t idx = 0, cmp_idx = 0;

    for (idx = 0, cmp_idx = 0; idx < topic_len; idx++) 
	{
        /* compare topic alreay out of bounds */
        if (cmp_idx >= cmp_topic_len) 
		{
            /* compare success only in case of the left string of topic is "/#" */
            if ((topic_len - idx == 2) && (memcmp(&topic[idx], "/#", 2) == 0)) 
			{
                return STATE_SUCCESS;
            } 
			else 
			{
                return STATE_MQTT_TOPIC_COMPARE_FAILED;
            }
        }

        /* if topic reach the '#', compare success */
        if (topic[idx] == '#') 
		{
            return STATE_SUCCESS;
        }

        if (topic[idx] == '+') 
		{
            /* wildcard + exist */
            for (; cmp_idx < cmp_topic_len; cmp_idx++) 
			{
                if (cmp_topic[cmp_idx] == '/') 
				{
                    /* if topic already reach the bound, compare topic should not contain '/' */
                    if (idx + 1 == topic_len) 
					{
                        return STATE_MQTT_TOPIC_COMPARE_FAILED;
                    } 
					else 
					{
                        break;
                    }
                }
            }
        } 
		else 
		{
            /* compare each character */
            if (topic[idx] != cmp_topic[cmp_idx]) 
			{
                return STATE_MQTT_TOPIC_COMPARE_FAILED;
            }
            cmp_idx++;
        }
    }

    /* compare topic should be reach the end */
    if (cmp_idx < cmp_topic_len) 
	{
        return STATE_MQTT_TOPIC_COMPARE_FAILED;
    }
    return STATE_SUCCESS;
}

//static uint32_t _auth_param_is_valid(const char *str, uint16_t max_len) 
//{
//    if (str == NULL)
//	{
//		return -1;
//	}			
//    if (strlen(str) > max_len) 
//	{
//        return -1;
//    }
//    return STATE_SUCCESS;
//}

//int32_t aiot_mqtt_set_param(aiot_mqtt_param_t param, uint32_t value) 
//{  
//    int32_t res = STATE_SUCCESS;
//	memset(&mqtt_device, 0, sizeof(mqtt_device));
//	
//    at_response_t *resp = NULL;
//    char *param_tag = NULL;
//    if (AIOT_PARAM_MAX <= param)
//    {
//        return STATE_FAILED_PARAM_ERROR;
//    }
//    switch (param)
//    {
//    case AIOT_PARAM_OPT_TIMEOUT:
//        if (value == 0) {
//            return STATE_FAILED_PARAM_ERROR;
//        }
//        param_tag = "TIMEOUT";
//        break;
//    case AIOT_PARAM_OPT_KEEPALIVE:
//        if (value == 0) {
//            return STATE_FAILED_PARAM_ERROR;
//        }
//        param_tag = "KEEPALIVE";
//        break;
//    case AIOT_PARAM_OPT_TLS:
//        if (value >= AIOT_MQTT_TLS_MAX) {
//            return STATE_FAILED_PARAM_ERROR;
//        }
//        param_tag = "TLS";
//        break;
//    case AIOT_PARAM_OPT_AUTHMODE:
//        if (value >= AIOT_MQTT_AUTH_MODE_MAX) {
//            return STATE_FAILED_PARAM_ERROR;
//        }
//        param_tag = "AUTHMODE";
//        break;
//    default:
//        break;
//    }
//    resp = at_create_resp(64, 0, AIOT_WAIT_MAX_TIME, NULL, 0);

//    if ((res = at_exec_cmd(resp, AIOT_CMD_FMT_IMQTTPARA_STR, param_tag, value)) != STATE_SUCCESS)
//    {
//        log_error("failed, res=0x%X.", res);
//    }

//    if (resp)
//    {
//        at_delete_resp(resp);
//    }
//    return res;
//}

int32_t aiot_mqtt_connect(uint8_t *host_ip, uint16_t host_port,char *host_id,char *host_user,char *host_pass, 
	aiot_mqtt_recv_handler_t recv_handler, aiot_mqtt_event_handler_t even_handler, void *userdata) 
{
    int32_t res = STATE_SUCCESS;
    at_response_t *resp = NULL;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();

	char ip_buf[16];
	char port_buf[8];

	memset(&mqtt_device, 0, sizeof(mqtt_device));
	sprintf(ip_buf,"%d.%d.%d.%d",host_ip[0],host_ip[1],host_ip[2],host_ip[3]);
	sprintf(port_buf,"%d",host_port);
	
    if (sysdep == NULL) 
	{
        return STATE_SYS_DEPEND_NOT_INIT;
    }

    CORE_INIT_LIST_HEAD(&g_mqtt_handle.sub_list);
    g_mqtt_handle.sub_mutex = sysdep->core_sysdep_mutex_init();
    g_mqtt_handle.recv_handler = recv_handler;
    g_mqtt_handle.event_handler = even_handler;
    g_mqtt_handle.userdata = userdata;

    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);

    if ((res = at_exec_cmd(resp, "AT+MQTTCFG=\"%s\",%s,\"%s\",%d,\"%s\",\"%s\",1,0\r\n",ip_buf,port_buf,host_id,60,host_user,host_pass)) != STATE_SUCCESS)
    {
        at_log_error("aiot_mqtt_connect failed, res=0x%X.", res);
    }

    if (resp)
    {
        at_delete_resp(resp);
    }

    return res;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_check_config
 *
 *   函数功能：   查询 MQTT客户端配置
 *
 *   入口参数：   cmd:命令      result:需要判断的响应
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_check_config(uint8_t *host_ip)
{
	at_response_t *resp = NULL;
    int32_t res = STATE_SUCCESS;
	char ip_buf[16];
	
	sprintf(ip_buf,"%d.%d.%d.%d",host_ip[0],host_ip[1],host_ip[2],host_ip[3]);
	
    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);
	
    if (at_obj_exec_cmd(at_client_get(), resp, "AT+MQTTCFG?") != STATE_SUCCESS)
	{
		if (resp != NULL)
		{
			at_delete_resp(resp);
		}		
		return STATE_FAILED;
	}
	if (at_resp_get_line_by_kw(resp, ip_buf)!= NULL)
	{
	    if (resp != NULL)
		{
			at_delete_resp(resp);
		}
		return STATE_SUCCESS;
	}	
	else
	{
	    if (resp != NULL)
		{
			at_delete_resp(resp);
		}		
		return STATE_FAILED;
	}
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_open
 *
 *   函数功能：   连接MQTT服务器
 *
 *   入口参数：   cmd:命令      result:需要判断的响应
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_open(void)
{
	int count = 50;
    int32_t res = STATE_SUCCESS;
    at_response_t *resp = NULL;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();


    if (sysdep == NULL) 
	{
        return STATE_SYS_DEPEND_NOT_INIT;
    }

    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);
    if (at_exec_cmd(resp, "AT+MQTTOPEN=1,1,0,0,0,\"nbdata\",\"data\"") != STATE_SUCCESS)
    {
        at_log_error("aiot_mqtt_disconnect failed, res=0x%X.", res);
    }
    do
    {
        count--;
        os_thread_sleep(os_msec_to_ticks(50));
    } while (count > 0 && mqtt_device.connect_status == 0);

	return STATE_SUCCESS;		
}

int32_t aiot_mqtt_disconnect(void) 
{
	int count = 50;
    int32_t res = STATE_SUCCESS;
    at_response_t *resp = NULL;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();

    at_log_info("aiot_mqtt_disconnect.");
    if (sysdep == NULL) 
	{
        return STATE_SYS_DEPEND_NOT_INIT;
    }

    /* 断开MQTT连接 */
    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);
    if ((res = at_exec_cmd(resp, "AT+MQTTDISC")) != STATE_SUCCESS)
    {
        at_log_error("aiot_mqtt_disconnect failed, res=0x%X.", res);
    }
    do
    {
        count--;
        os_thread_sleep(os_msec_to_ticks(50));
    } while (count > 0 && mqtt_device.connect_status == 1);

    if (resp)
    {
        at_delete_resp(resp);
    }

    /* 释放 sublist 资源 */
    _core_mqtt_sublist_destroy(sysdep);
    if (g_mqtt_handle.sub_mutex != NULL) 
	{
        sysdep->core_sysdep_mutex_deinit(&g_mqtt_handle.sub_mutex);
    }
    return res;
}

int32_t aiot_mqtt_pub(const char *topic, int32_t qos, char *payload) 
{
    int32_t res = STATE_SUCCESS;
    at_response_t *resp = NULL;

    if (topic == NULL || payload == NULL || qos > AIOT_MQTT_QOS_MAX) 
	{
        return STATE_FAILED_PARAM_ERROR;
    }

    resp = at_create_resp(2048, 0, AIOT_WAIT_MAX_TIME*3, NULL, 0);

    if ((res = at_exec_cmd(resp, "AT+MQTTPUB=\"%s\",%d,0,0,0,\"%s\"", topic, qos, payload)) != STATE_SUCCESS)
    {
        at_log_error("aiot_mqtt_pub failed, res=0x%X.", res);
    }
    
    if (resp)
    {
        at_delete_resp(resp);
    }

    return res;
}

int32_t aiot_mqtt_pubhex(const char *topic, int32_t qos, uint32_t payload_len, uint8_t *payload) 
{
    int32_t res = STATE_SUCCESS;
    at_response_t *resp = NULL;

    if (topic == NULL || payload == NULL || qos > AIOT_MQTT_QOS_MAX) 
	{
        return STATE_FAILED_PARAM_ERROR;
    }

    resp = at_create_resp(2048, 0, AIOT_WAIT_MAX_TIME*3, NULL, 0);

    if ((res = at_exec_cmd(resp, "AT+MQTTPUB=\"%s\",%d,0,0,%d,\"%s\"", topic, qos,payload_len/2, payload)) != STATE_SUCCESS)
    {
        at_log_error("aiot_mqtt_pub failed, res=0x%X.", res);
    }
    
    if (resp)
    {
        at_delete_resp(resp);
    }
    return res;
}

int32_t aiot_mqtt_sub(const char *topic, int32_t qos, aiot_mqtt_recv_handler_t handler, void *userdata) 
{
    int32_t res = STATE_SUCCESS;
    at_response_t *resp = NULL;
	int count = 50;
	
	aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
	
	mqtt_device.suback_status = 0x80;
    if (topic == NULL) 
	{
        return STATE_FAILED_PARAM_ERROR;
    }

    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);
    _core_mqtt_sublist_insert((char *)topic, handler, userdata);
    if ((res = at_exec_cmd(resp, "AT+MQTTSUB=\"%s\",%d", topic, qos)) != STATE_SUCCESS)
    {
        at_log_error("aiot_mqtt_sub failed, res=0x%X.", res);
    }
    do
    {
        count--;
        os_thread_sleep(os_msec_to_ticks(50));
    } while (count > 0 && mqtt_device.suback_status == 0x80);
	
    if (resp)
    {
        at_delete_resp(resp);
    }
	//topic->suback_ststus = 1;
    return res;
}

int32_t aiot_mqtt_unsub(const char *topic) 
{
    int32_t res = STATE_SUCCESS;
    at_response_t *resp = NULL;
    
    if (topic == NULL) 
	{
        return STATE_FAILED_PARAM_ERROR;
    }
    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);
    
    _core_mqtt_sublist_remove((char *)topic);
    if ((res = at_exec_cmd(resp, "AT+MQTTUNSUB=\"%s\"", topic)) != STATE_SUCCESS)
    {
        at_log_error("aiot_mqtt_unsub failed, res=0x%X.", res);
    }

    if (resp)
    {
        at_delete_resp(resp);
    }

    return res;
}

int32_t aiot_mqtt_query_state(void) 
{
    int32_t res = STATE_SUCCESS;
    at_response_t *resp = NULL;
	int status = 0;
	
    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);
    if ((res = at_exec_cmd(resp, "AT+MQTTSTAT?")) != STATE_SUCCESS)
    {
        at_log_error("aiot_mqtt_query_state failed, res=0x%X.", res);
    }
	at_resp_parse_line_args(resp, 2, "+MQTTSTAT:%d", &status);
    if (resp)
    {
        at_delete_resp(resp);
    }

    return res;
}

/*************************************************************
 *   函数名称：   qsdk_mqtt_get_connect
 *
 *   函数功能：   关闭 MQTT客户端 ACK显示
 *
 *   入口参数：   无
 *
 *   返回参数：   0：连接成功   1：连接失败
 *
 *   说明：
 *************************************************************/
int qsdk_mqtt_get_connect(void)
{
    if (mqtt_device.connect_status)
        return STATE_SUCCESS;
    return STATE_FAILED;
}

//void _urc_pub_recv_handler(const char *data, uint32_t size) 
//{
//    log_trace("_urc_pub_recv_handler size:%d\r\n", size);
//    core_mqtt_sub_node_t *sub_node = NULL;
//    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
//    if (sysdep == NULL) 
//	{
//        return;
//    }
//    int32_t res = 0;
//    int32_t payload_len = 0;
//    char *topic = sysdep->core_sysdep_malloc(AIOT_MQTT_TOPIC_MAXLEN, CORE_AT_CLIENT_MODULE);
//    char *payload = sysdep->core_sysdep_malloc(AIOT_DEFAULT_RECV_PUB_MAX_SIZE, CORE_AT_CLIENT_MODULE);
//    /* parameters parsing */
//    memset(topic, 0, AIOT_MQTT_TOPIC_MAXLEN);
//    memset(payload, 0, AIOT_DEFAULT_RECV_PUB_MAX_SIZE);
//    res = at_req_parse_args(data, AIOT_CMD_FMT_IMQTTRCVPUB_STR, topic, &payload_len, payload);
//    if (res != 3) 
//	{
//        log_error("Receive fmt error. res=%d, payload_len:%d\r\n", res, payload_len);
//        sysdep->core_sysdep_free(topic);
//        sysdep->core_sysdep_free(payload);
//        return;
//    } 

//    aiot_mqtt_recv_t msg = { 0 };
//    msg.type = AIOT_MQTTRECV_PUB;
//    msg.pub.payload = (uint8_t *)payload;
//    msg.pub.payload_len = payload_len;
//    msg.pub.topic = topic;
//    msg.pub.topic_len = sizeof(topic);
//    uint8_t topic_run_flag = 0;
//    if (g_mqtt_handle.sub_mutex != NULL) 
//	{
//        sysdep->core_sysdep_mutex_lock(g_mqtt_handle.sub_mutex);
//        core_list_for_each_entry(sub_node, &g_mqtt_handle.sub_list, linked_node, core_mqtt_sub_node_t) 
//		{
//            if (_core_mqtt_topic_compare(sub_node->topic, (uint32_t)(strlen(sub_node->topic)), topic,strlen(topic)) == STATE_SUCCESS) 
//			{
//                log_trace("recv topic:%s\r\n", topic);
//                if (sub_node->handler) 
//				{
//                    sub_node->handler(&msg, sub_node->userdata);
//                    topic_run_flag = 1;
//                    break;
//                }
//            }
//        }
//        sysdep->core_sysdep_mutex_unlock(g_mqtt_handle.sub_mutex);
//    }
//    if ( topic_run_flag == 0 && g_mqtt_handle.recv_handler) 
//	{
//        g_mqtt_handle.recv_handler(&msg, g_mqtt_handle.userdata);
//    }
//    sysdep->core_sysdep_free(topic);
//    sysdep->core_sysdep_free(payload);
//}

void aiot_mqtt_recv_handler(const char *data, uint32_t size) 
{
	char *result = NULL;

    if (strstr(data, "+MQTTOPEN:"))
    {
        if (strstr(data, "+MQTTOPEN:OK"))
		{
			mqtt_device.connect_status = 1;
		}
        else
		{
			mqtt_device.connect_status = 0;
		}   		
    }
    else if (strstr(data, "+MQTTSUBACK:"))
    {
        char *id = NULL;
        char *code = NULL;
        /* 获取第一个子字符串 */
        result = strtok((char *)data, ":");
        /* 继续获取其他的子字符串 */
        id = strtok(NULL, ",");
        code = strtok(NULL, ",");
        mqtt_device.suback_topic = strtok(NULL, ",");
        mqtt_device.suback_status = atoi(code);
		//_urc_suback_handler(data, size); 
		#ifdef QSDK_USING_DEBUG
			LOG_D("suback_topic=%s\r\n", mqtt_device.suback_topic);
			LOG_D("suback_status=%d\r\n", mqtt_device.suback_status);
		#endif
    }
    else if (strstr(data, "+MQTTPUBACK:"))
    {
        mqtt_device.puback_status = 1;
		//_urc_puback_handler(data, size);
    }
    else if (strstr(data, "+MQTTUNSUBACK:"))
    {
        mqtt_device.unsuback_status = 1;
		//_urc_unsuback_handler(data, size);  
		
    }
    else if (strstr(data,"+MQTTDISC:"))
    {
        if (strstr(data, "+MQTTDISC:OK"))
		{
			mqtt_device.connect_status = 0;
		}  
    }
    else if (strstr(data, "+MQTTPUBREC:"))
    {
        mqtt_device.pubrec_status = 1;
    }
    else if (strstr(data, "+MQTTPUBCOMP:"))
    {
        mqtt_device.pubcomp_status = 1;
    }
    else if (strstr(data, "+MQTTPUBLISH:"))
    {
        char *dup = NULL;
        char *qos = NULL;
        char *retained = NULL;
        char *packId = NULL;
        result = strtok((char *)data, ":");
        dup = strtok(NULL, ",");
        qos = strtok(NULL, ",");
        retained = strtok(NULL, ",");
        mqtt_device.packid = atoi(strtok(NULL, ","));
        mqtt_device.puback_topic = strtok(NULL, ",");
        mqtt_device.pub_mesg_len = atoi(strtok(NULL, ","));
        mqtt_device.pub_mesg = strtok(NULL, ",");

//        if (!mqtt_device.puback_open)
//        {
//            if (qsdk_mqtt_data_callback(mqtt_device.puback_topic, mqtt_device.pub_mesg,
//                                        mqtt_device.pub_mesg_len) != RT_EOK)
//            {
//                LOG_E("mqtt topic callback failure\r\n");
//            }
//        }
		//_urc_pub_recv_handler(data, size);
    }
	else if (strstr(data, "+MQTTPINGRESP:"))
    {
        mqtt_device.connect_status = 1;
    }
    else if (strstr(data, "+MQTTTO:"))
    {
        char *type = NULL;
        result = strtok((char *)data, ":");
        type = strtok(NULL, ":");
        mqtt_device.error_type = atoi(type);
        if (mqtt_device.error_type == 5)
		{
			mqtt_device.connect_status = 0;
		}        
    }
    else if (strstr(data,"+MQTTREC:"))
    {
    }
}

//void _urc_puback_handler(const char *data, uint32_t size) 
//{
////    log_trace("_urc_puback_handler size:%d\r\n", size);

//    uint16_t packet_id = 0;
//    int32_t res = 0;

//	/* parameters parsing */
//    res = at_req_parse_args(data, AIOT_CMD_FMT_IMQTTPUBACK_STR, &packet_id);
//    if (res == 1) 
//	{
//        aiot_mqtt_recv_t msg = { 0 };
//        msg.type = AIOT_MQTTRECV_PUB_ACK;
//        msg.pub_ack.packet_id = packet_id;
//        if (g_mqtt_handle.recv_handler) 
//		{
//            g_mqtt_handle.recv_handler(&msg, g_mqtt_handle.userdata);
//        }
//    } 
//	else 
//	{
//        log_error("Receive fmt error. res=%d\r\n", res);
//        return;
//    }
//}

//void _urc_suback_handler(const char *data, uint32_t size) 
//{
////    log_trace("_urc_suback_handler size:%d\r\n", size);

//    uint16_t packet_id = 0;
//    int32_t ret = 0;
//    int32_t res = 0;
//	
//	/* parameters parsing */
//    res = at_req_parse_args(data, AIOT_CMD_FMT_IMQTTSUBACK_STR, &packet_id, &ret);
//    log_info("res=%d\r\n", res);
//    if (res == 2) 
//	{
//        aiot_mqtt_recv_t msg = { 0 };
//        msg.type = AIOT_MQTTRECV_SUB_ACK;
//        msg.sub_ack.packet_id = packet_id;
//        msg.sub_ack.res = ret;
//        if (g_mqtt_handle.recv_handler) 
//		{
//            g_mqtt_handle.recv_handler(&msg, g_mqtt_handle.userdata);
//        }
//    } 
//	else 
//	{
//        log_error("Receive fmt error. res=%d\r\n", res);
//        return;
//	}    
//}

//void _urc_unsuback_handler(const char *data, uint32_t size) 
//{
//    log_trace("_urc_unsuback_handler size:%d\r\n", size);

//    uint16_t packet_id = 0;
//    int32_t res = 0;
//	
//	/* parameters parsing */
//    res = at_req_parse_args(data, AIOT_CMD_FMT_IMQTTUNSUBACK_STR, &packet_id);
//    if (res == 1) 
//	{    
//        aiot_mqtt_recv_t msg = { 0 };
//        msg.type = AIOT_MQTTRECV_UNSUB_ACK;
//        msg.unsub_ack.packet_id = packet_id;
//        if (g_mqtt_handle.recv_handler) 
//		{
//            g_mqtt_handle.recv_handler(&msg, g_mqtt_handle.userdata);
//        }
//    } 
//	else 
//	{
//        log_error("Receive fmt error. res=%d\r\n", res);
//        return;
//	}
//}

void aiot_mqtt_event_handler(const char *data, uint32_t size) 
{
    int32_t res = 0;
	int32_t temp_state = 0;
    
    //log_trace("aiot_mqtt_event_handler size:%d\r\n", size);

    res = at_req_parse_args(data, "+MQTTSTAT:%d", &temp_state);
    //log_info("res = %d\r\n", res);
    if (res == 1) 
	{
        if (g_mqtt_handle.event_handler != NULL)			
		{
            g_mqtt_handle.event_handler(temp_state, g_mqtt_handle.userdata);
        }
        g_mqtt_handle.state = temp_state;
        at_log_info("Receive mqtt state:%d", temp_state);
    } 
	else 
	{
       at_log_error("Parse +IMQTTSTATE failed.");
	}  
}

