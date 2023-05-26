#include "nb_iot_mqtt_handle.h"
#include "sys_log.h"
#include "sys_os.h"

#include "aiot_at_client_api.h"
#include "aiot_state_api.h"
#include "posix_port.h"
#include "aiot_module_api.h"
#include "aiot_mqtt_api.h"
//#include "aiot_ota_api.h"
#include "posix_port.h"
#include "core_string.h"
#include "project_psm_control.h"

static os_thread_t nb_mqtt_thread= NULL;
static os_thread_stack_define(nb_mqtt_stack, 4*1024);

extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

char topic_set[80];
char topic_post[80];
uint8_t NB_SendDataBuff[1024];
uint8_t NB_SendDataBuff_hex[2048];
uint32_t NB_SendDataBuff_hex_len;


/* 日志回调函数, SDK的日志会从这里输出 */
int32_t demo_state_logcb(char *message)
{
    log_i("%s", message);
    return 0;
}

void demo_event_handler(const aiot_mqtt_event_type_t event, void *userdata) 
{
    log_i("demo event handle event:%d", event);
    switch (event) 
	{

        case CLIENT_NO_INIT: //client 参数未初始化。
		{
            
        }
        break;
        case CLIENT_ALREADY_INIT: //client 参数已初始化
		{
            
        }
        break;
		case MQTT_SERVER_DISCONNECT: //和服务器已断开
		{
            device_data.nbiot_mqtt_close_flag=1;
        }
        break;
		case SEND_CONNECT: //发送 Connect 包，等待接收服务器 ack
		{
            
        }
        break;
		case MQTT_SERVER_RECONNECT: //正在重连服务器
		{
            
        }
        break;
		case MQTT_SERVER_READY_CONNECT: //和服务器已连接
		{
            
        }
        break;
		case MQTT_SERVER_TCP_CONNECTING: //建立 TCP 连接中
		{
            
        }
        break;
			case MQTT_SERVER_TCP_ALREADY: //TCP 连接已建立
		{
            
        }
        break;
        default:
		{

        }
    }
}

/* 默认执行的回调函数,免订阅的消息会会调到这个函数 */
void demo_default_recv_handler(aiot_mqtt_recv_t *packet, void *userdata) 
{
    switch (packet->type)
    {
    case AIOT_MQTTRECV_PUB:
        log_i("AIOT_MQTTRECV_PUB payload len:%d, payload:%s", packet->pub.payload_len, packet->pub.payload);
        /* code */
        break;
    case AIOT_MQTTRECV_SUB_ACK:
        log_i("AIOT_MQTTRECV_SUB_ACK sub ack packet id:%d res:%d", packet->sub_ack.packet_id, packet->sub_ack.res);
        /* code */
        break;
    case AIOT_MQTTRECV_UNSUB_ACK:
        log_i("AIOT_MQTTRECV_UNSUB_ACK unsub ack pakcet id:%d", packet->unsub_ack.packet_id);
        /* code */
        break;
    case AIOT_MQTTRECV_PUB_ACK:
        log_i("AIOT_MQTTRECV_PUB_ACK pub ack packet id:%d", packet->pub_ack.packet_id);
        /* code */
        break;
    default:
        break;
    }
}

/* 对应topic的回调函数，如果没有默认会调到 demo_topic_recv_handler*/
void demo_topic_recv_handler(aiot_mqtt_recv_t *packet, void *userdata) 
{
    log_i("demo_topic_recv_handler payload len:%d, paylod:%s", packet->pub.payload_len, packet->pub.payload);

    switch (packet->type)
    {
    case AIOT_MQTTRECV_PUB:
        /* code */
        break;
    
    default:
        break;
    }
}

void nb_mqtt_control_thread(void* param)
{
	int32_t res = 0;
    uint32_t i = 0;
	uint32_t err_cnt = 0;
	uint8_t hour_tmp = 0;
	uint8_t mqtt_up_flag=0;
	uint32_t nb_min_cnt=0;
	
	device_data.nb_start_flag=1;
	start_:	
	device_data.nbiot_mqtt_close_flag=0;
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);/* 配置SDK的底层依赖 */
    aiot_state_set_logcb(demo_state_logcb);/* 配置SDK的日志输出 */

    if ((res = aiot_module_init()) != STATE_SUCCESS) /* 初始化模组参数配置 */
	{
		log_e("nbmqtt","aiot_module_init");
        goto exit;
    }
    if (aiot_module_handshake(10000) != STATE_SUCCESS) /* 等待与模组建立AT命令握手 */
	{
        log_e("nbmqtt","With module handshake failed");
        goto exit;
    }
    if (qsdk_nb_quick_connect() != STATE_SUCCESS) /* 等待与模组连上网 */
	{
        log_e("nbmqtt","nb_quick_connect failed");
        goto exit;
    }
	
    if (aiot_mqtt_connect(sys_psm.mqtt_server_ip, sys_psm.mqtt_server_port,sys_psm.device_id,
		sys_psm.mqtt_user,sys_psm.mqtt_pass, demo_default_recv_handler, demo_event_handler, NULL) != STATE_SUCCESS)//mqtt客户端配置：服务器，设备ID，产品APIKEY等信息
    {
        log_e("nbmqtt","MQTT config failure");
    }
    log_i("MQTT config success");
	os_thread_sleep(os_msec_to_ticks(100)); 

	if (qsdk_mqtt_open() != STATE_SUCCESS) /* 初始化模组参数配置 */
	{
		log_e("nbmqtt","MQTT open failure");
    }	
//	snprintf(topic_set, sizeof(topic_set), "GATEWAY/NBIOT/USER/GET/%s",sys_psm.device_id);
//	log_i("Subscribe Topic: %s", topic_set);
	
//	snprintf(topic_post, sizeof(topic_post), "GATEWAY/NBIOT/USER/UPDATE/%s",sys_psm.device_id);
//	log_i("Publish Topic: %s",topic_post);
	
    aiot_mqtt_sub(sys_psm.mqtt_subscribe_command_buf, AIOT_MQTT_QOS0, demo_topic_recv_handler, NULL); /* 订阅topic，如果不指定 demo_topic_recv_handler 的回调，接收到的消息都会会调到 demo_topic_recv_handler */
    nb_min_cnt=60*sys_psm.mqtt_updata_time;
    while (1)
    {  
		if((device_data.power_flag==1)&&(device_data.phy_check_flag != PHY_LINK_ON))
		{
			nb_min_cnt++;
			if(nb_min_cnt>=60*sys_psm.mqtt_updata_time)	
			{
				nb_min_cnt=0;
				mqtt_up_flag=1;
			}				
		}	
		if(device_data.nbiot_mqtt_close_flag==1)
		{
			break;
		}
		if (aiot_mqtt_query_state() != STATE_SUCCESS)
		{
			log_i("Now the MQTT link is disconnected");
			err_cnt++;
			if(err_cnt>50)
			{
				err_cnt=0;
				if (qsdk_mqtt_open() != STATE_SUCCESS)
				{
					log_i("Now MQTT reconnection");
					goto exit1;
				}			
			}
		}			
		if ((qsdk_mqtt_get_connect() == STATE_SUCCESS)&&mqtt_up_flag==1)
		{	
			mqtt_up_flag=0;		
			sprintf((char *) NB_SendDataBuff,
			"{\"client\":\"%s\", 	\
			\"time\":\"%d/%02d/%02d %02d:%02d:%02d\", \
			\"type\":\"NBIOT\", \
			\"POWER\":OFF}", \
			sys_psm.device_id,device_data.
			c_date.u8Year+2000,device_data.c_date.u8Month,device_data.c_date.u8Day,
			device_data.c_time.u8Hour,device_data.c_time.u8Minute,device_data.c_time.u8Second); 
			core_uint2hexstr(NB_SendDataBuff,strlen((char *)NB_SendDataBuff),(char *)NB_SendDataBuff_hex,&NB_SendDataBuff_hex_len);
			//aiot_mqtt_pub(topic_post, 0, (char *) NB_SendDataBuff);	
			aiot_mqtt_pubhex(sys_psm.mqtt_publish_sensor_buf,0,NB_SendDataBuff_hex_len,NB_SendDataBuff_hex);/* 发布消息 */	
		}
		os_thread_sleep(os_msec_to_ticks(1000)); 		
    }
exit1:
	aiot_mqtt_disconnect();
exit:
    aiot_module_deinit();
	device_data.nb_start_flag=0;
	device_data.nb_rest_cnt=0;
	os_thread_sleep(os_msec_to_ticks(100)); 
//	goto start_;
	os_thread_delete(&nb_mqtt_thread);
}

void nb_mqtt_process_init(void)
{
	
	int ret=SYS_FAIL;
	ret = os_thread_create(&nb_mqtt_thread, //任务控制块指针
							"nb_mqtt_thread",//任务名字
							nb_mqtt_control_thread, //任务入口函数
							NULL,//任务入口函数参数
							&nb_mqtt_stack,//任务栈大小
							OS_PRIO_3);	//任务的优先级							
   if(ret==SYS_FAIL)
   {
		log_e("nbmqtt","nb_mqtt thread create error!");
   }
}