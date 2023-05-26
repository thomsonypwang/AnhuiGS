#include "eth_mqtt_handle.h"
#include "eth_handle.h"
#include "sys_log.h"
#include "sys_os.h"

#include "w5500_dir.h"
#include "w5500_socket.h"
#include "w5500_dns.h"
#include "project_pin_use_config.h"
#include "project_psm_control.h"

//#include "MQTTConnect.h"
#include "MQTTPacket.h"
#include "eth_to_mqtt.h"
#include "eth_to_freertos.h"

#include "json_parser.h"
#include "jsmn.h"
#include "json_generator.h"

#include "autoeclosing_rs485_handle.h"
#include "monitor_board_handle.h"
#include "gps_control.h"
#include "circuit_breaker_rs485_handle.h"

#define MQTT_MSGLEN         (3000)

#define ETHERNET_BUF_MAX_SIZE (1024 * 3)
#define DEFAULT_TIMEOUT 1000 // 1 second
#define MQTT_KEEP_ALIVE 500 // 10 milliseconds


static os_thread_t mqtt_thread= NULL;
static os_thread_stack_define(mqtt_stack, 7*1024);
static os_thread_t yield_thread= NULL;
static os_thread_stack_define(yield_stack, 2*1024);


static uint8_t g_mqtt_send_buf[ETHERNET_BUF_MAX_SIZE] = {0,};
static uint8_t g_mqtt_recv_buf[ETHERNET_BUF_MAX_SIZE] = {0,};
uint8_t mqtt_ok=0;
static Network g_mqtt_network;
static MQTTClient g_mqtt_client;
static MQTTPacket_connectData g_mqtt_packet_connect_data = MQTTPacket_connectData_initializer;

uint8_t relay_buf[10];
uint8_t set_ok=0;

enum
{
	ATTR_FAN_O_INDEX=0,	
	ATTR_FAN_C_INDEX=1,
	ATTR_HOT_O_INDEX=2,
	ATTR_HOT_C_INDEX=3,
	ATTR_P_INDEX=4,
	ATTR_V_INDEX=5,
	ATTR_LED_INDEX=6,
	ATTR_FAN_INDEX=7,
	ATTR_RECLOSE_INDEX=8,
	ATTR_CBCLOSE_INDEX=9,	
	ATTR_SET_INDEX=10,	

	ATTR_SPUV_INDEX,// RW //单相欠压（报警）阈值
	ATTR_SPUVP_INDEX,// RW //单相欠压（预警）阈值
	ATTR_SPOV_INDEX,// RW //单相过压（报警）阈值
	ATTR_SPOVP_INDEX,// RW //单相过压（预警）阈值
	ATTR_SPVC_INDEX,// RW //单相过流（报警）阈值
	ATTR_SPVCP_INDEX,// RW //单相过流（预警）阈值
	ATTR_SPOT_INDEX,// RW //单相线温（报警）阈值
	ATTR_SPOTP_INDEX,// RW //单相线温（预警）阈值
	ATTR_SLEAKAGE_INDEX,// RW //软件漏电(报警)电流
	ATTR_SLEAKAGEP_INDEX,// RW //软件漏电(预警)电流
	ATTR_OP_INDEX,// RW //过功率(报警)阈值
	ATTR_OPP_INDEX,// RW //过功率(预警)阈值
	
	ATTR_UVA_INDEX,// RW //欠压(报警)阈值A
	ATTR_UVAP_INDEX,// RW //欠压(预警)阈值A
	ATTR_UVB_INDEX,// RW //欠压(报警)阈值B
	ATTR_UVBP_INDEX,// RW //欠压(预警)阈值B
	ATTR_UVC_INDEX,// RW //欠压(报警)阈值C
	ATTR_UVCP_INDEX,// RW //欠压(预警)阈值C
	
	ATTR_OVA_INDEX,// RW //过压(报警)阈值A
	ATTR_OVAP_INDEX,// RW //过压(预警)阈值A
	ATTR_OVB_INDEX,// RW //过压(报警)阈值B
	ATTR_OVBP_INDEX,// RW //过压(预警)阈值B
	ATTR_OVC_INDEX,// RW //过压(报警)阈值C
	ATTR_OVCP_INDEX,// RW //过压(预警)阈值C
	
	ATTR_OCA_INDEX,// RW //过流(报警)阈值A
	ATTR_OCAP_INDEX,// RW //过流(预警)阈值A
	ATTR_OCB_INDEX,// RW //过流(报警)阈值B
	ATTR_OCBP_INDEX,// RW //过流(预警)阈值B
	ATTR_OCC_INDEX,// RW //过流(报警)阈值C
	ATTR_OCCP_INDEX,// RW //过流(预警)阈值C
	
	ATTR_OTA_INDEX,// RW //温度(报警)阈值A
	ATTR_OTAP_INDEX,// RW //温度(预警)阈值A
	ATTR_OTB_INDEX,// RW //温度(报警)阈值B
	ATTR_OTBP_INDEX,// RW //温度(预警)阈值B
	ATTR_OTC_INDEX,// RW //温度(报警)阈值C
	ATTR_OTCP_INDEX,// RW //温度(预警)阈值C
	ATTR_OTN_INDEX,// RW //温度(报警)阈值N
	ATTR_OTNP_INDEX,// RW //温度(预警)阈值N	
	ATTR_MAX_NUMS,
};

enum
{
	H_HEADER_INDEX=0,
	H_CLIENT_INDEX,
	H_COMMAND_INDEX,
	H_TIME_INDEX,
	H_TYPE_INDEX,
	H_ID_INDEX,
	H_VER_INDEX,
	H_DATA_INDEX,
	H_MAX_NUMS
};

static char *device_post_attr_head[] =
{
	"header",
	"client",
	"command",
	"time",
	"type",
	"id",	
	"ver",
	"data",
	NULL
};

static char *device_post_attr[] =
{
	"fan_open",
	"fan_close",
	"hot_fan_open",
	"hot_fan_close",
	"P",
	"V",
	"led",
	"fan",
	"reclose",
	"cbclose",
	"set",

	"SPUV",//单相欠压（报警）阈值
	"SPUVP",//单相欠压（预警）阈值
	"SPOV",//单相过压（报警）阈值
	"SPOVP",//单相过压（预警）阈值	
	"SPVC",//单相过流（报警）阈值
	"SPVCP",//单相过流（预警）阈值	
	"SPOT",//单相线温（报警）阈值
	"SPOTP",//单相线温（报警）阈值
	"SLEAKAGE",//软件漏电(报警)电流
	"SLEAKAGEP",//软件漏电(预警)电流
	"OP",//过功率(报警)阈值
	"OPP",//过功率(预警)阈值
	
	"UVA",//欠压(报警)阈值A
	"UVAP",//欠压(预警)阈值A
	"UVB",//欠压(报警)阈值B
	"UVBP",//欠压(预警)阈值B
	"UVC",//欠压(报警)阈值C
	"UVCP",//欠压(预警)阈值C
	
	"OVA",//过压(报警)阈值A
	"OVAP",//过压(预警)阈值A
	"OVB",//过压(报警)阈值B
	"OVBP",//过压(预警)阈值B
	"OVC",//过压(报警)阈值C
	"OVCP",//过压(预警)阈值C
	
	"OCA",//过流(报警)阈值A
	"OCAP",//过流(预警)阈值A
	"OCB",//过流(报警)阈值B
	"OCBP",//过流(预警)阈值B
	"OCC",//过流(报警)阈值C
	"OCCP",//过流(预警)阈值C
	
	"OTA",//温度(报警)阈值A
	"OTAP",//温度(预警)阈值A
	"OTB",//温度(报警)阈值B
	"OTBP",//温度(预警)阈值B
	"OTC",//温度(报警)阈值C
	"OTCP",//温度(预警)阈值C
	"OTN",//温度(报警)阈值N
	"OTNP",//温度(预警)阈值N
	NULL
};

/* MQTT */
//static void mqtt_subscribe_alarm(MessageData *msg_data)
//{
//    MQTTMessage *message = msg_data->message;

//    printf("%.*s", (uint32_t)message->payloadlen, (uint8_t *)message->payload);
//}

static int parse_str(jobj_t *jobj, char *key, char *p_ret_str, int buf_len)
{
	int len = 0;

	memset(p_ret_str, 0, buf_len);
	if (json_get_val_str(jobj, key, p_ret_str, buf_len) == SYS_OK)
	{
		json_get_val_str_len(jobj, key, &len);
		log_i("key: %s, Got String: %s, length = %d", key, p_ret_str, len);
		return SYS_OK;
	}
	return SYS_FAIL;
}

static void mqtt_subscribe_command(MessageData *msg_data)
{
    MQTTMessage *message = msg_data->message;
	jobj_t jobj;
	int ret, sub_test_cnt,i;
	char str_var[1024];
	char client_id_data[64];
	char id_data[64];
	char type_data[32];
	char name_data[32];
	int j;
	int p_data;
	int hold_flag;

    //log_i("%.*s", (uint32_t)message->payloadlen, (uint8_t *)message->payload);
	
	sub_test_cnt = 1;
	ret = json_parse_start(&jobj, (char *)message->payload, message->payloadlen);
	if (ret != SYS_OK)
	{
		json_parse_stop(&jobj);
		log_i("sub_test_cnt = %d ", sub_test_cnt);
		return ;
	}	
	for(i=0;i<H_MAX_NUMS;i++)
	{
		sub_test_cnt++;
		switch (i)
		{
			case H_HEADER_INDEX:
				ret = json_get_composite_object(&jobj, device_post_attr_head[i]);
			break;
			case H_CLIENT_INDEX:
				memset(str_var,0, sizeof(str_var));
				ret = parse_str(&jobj, device_post_attr_head[i], str_var, sizeof(str_var));
				if(ret==SYS_OK)
				{
					strcpy(client_id_data, str_var);
					if(strcmp(client_id_data,sys_psm.device_id)!=0)
					{
						json_release_composite_object(&jobj);
						json_parse_stop(&jobj);
						return ;
					}
				}
				else
				{
					json_release_composite_object(&jobj);
				}
			break;
			case H_COMMAND_INDEX:

			break;
			case H_TIME_INDEX:

			break;
			case H_TYPE_INDEX:
				memset(str_var,0, sizeof(str_var));
				ret = parse_str(&jobj, device_post_attr_head[i], str_var, sizeof(str_var));
				if(ret==SYS_OK)
				{
					strcpy(type_data, str_var);
				}

			break;
			case H_ID_INDEX:
				memset(str_var,0, sizeof(str_var));
				ret = parse_str(&jobj, device_post_attr_head[i], str_var, sizeof(str_var));
				if(ret==SYS_OK)
				{
					strcpy(id_data, str_var);
				}
				device_data.cbclose_data_addr=0xff;
				for (j = 0; j < CIRCUIT_BREAKER_MAX_NUM; j++)
				{
					if(device_data.cb_slave_addr[j]!=0xff)
					{
						snprintf(name_data, 16, "BRKRS_%d",device_data.cb_slave_addr[j]);
						if(strcmp(id_data,name_data)==0)
						{
							device_data.cbclose_data_addr=j;							
						}
					}				
				}
			break;
			case H_VER_INDEX:
				
				json_release_composite_object(&jobj);
			break;	
			case H_DATA_INDEX:	
				
				ret = json_get_composite_object(&jobj, device_post_attr_head[i]);
			break;			
		}
		if (ret != SYS_OK)
		{
			json_parse_stop(&jobj);
			log_i("json head: ret = %d  sub_test_cnt=%d", ret,sub_test_cnt);
			return ;
		}
	}
	set_ok=2;
	/////////////////////////////////////////////////////////////////////
	for(i=0;i<ATTR_MAX_NUMS;i++)
	{
		memset(str_var,0, sizeof(str_var));
		ret = parse_str(&jobj, device_post_attr[i], str_var, sizeof(str_var));
		if (ret == SYS_OK)
		{
			switch (i)
			{	
				case ATTR_FAN_O_INDEX:
					device_data.fan_open_temp=atof(str_var);
					device_data.fan_temp_flag=device_data.fan_temp_flag|0x01;
					set_ok=1;
				break;
				case ATTR_FAN_C_INDEX:
					device_data.fan_close_temp=atof(str_var);
					device_data.fan_temp_flag=device_data.fan_temp_flag|0x02;
					set_ok=1;				
				break;
				case ATTR_HOT_O_INDEX:
					device_data.hot_fan_open_humi=atoi(str_var);
					device_data.hot_humi_flag=device_data.hot_humi_flag|0x01;
					set_ok=1;
				break;
				case ATTR_HOT_C_INDEX:
					device_data.hot_fan_close_humi=atoi(str_var);
					device_data.hot_humi_flag=device_data.hot_humi_flag|0x01;
					set_ok=1;
				break;				
				case ATTR_P_INDEX:
					p_data=atoi(str_var);
				break;
				case ATTR_V_INDEX:
					for (j = 0; j < device_data.slave_device_max; j++)
					{
						if(device_data.own_slave_device[j].device_type==POWER_MONITOR_BOARD)
						{
							if(strcmp(id_data,device_data.own_slave_device[j].device_name)==0)
							{
								if((p_data>=1)&&(p_data<=10))
								{
									p_data=p_data-1;
									if (strcmp(str_var,"ON") == 0)
									{
										device_data.power_monitoring_relay_data[j]=device_data.power_monitoring_relay_data[j]|((1UL)<<(p_data));
									}
									else if (strcmp(str_var,"OFF") == 0)
									{
										device_data.power_monitoring_relay_data[j]&=~((1UL)<<(p_data));
									}
									set_ok=1;								
								}
							}
						}
					}
				break;
				case ATTR_LED_INDEX:
					if(device_data.own_slave_device[0].device_type==POWER_MONITOR_BOARD)
					{
						if(strcmp(id_data,device_data.own_slave_device[0].device_name)==0)
						{
							if (strcmp(str_var,"ON") == 0)
							{
								device_data.led_flag=1;
								device_data.led_data_flag=1;
								
							}
							else if (strcmp(str_var,"OFF") == 0)
							{
								if(device_data.lock_in_error_flag==0)
								{
									device_data.led_flag=1;
									device_data.led_data_flag=0;
								}
							}
							set_ok=1;							
						}
					}
				break;
				case ATTR_FAN_INDEX:
					if(device_data.own_slave_device[0].device_type==POWER_MONITOR_BOARD)
					{
						if(strcmp(id_data,device_data.own_slave_device[0].device_name)==0)
						{
							if (strcmp(str_var,"ON") == 0)
							{
								device_data.power_monitoring_relay_data[0]=device_data.power_monitoring_relay_data[0]|FAN_RELAY;
							}
							else if (strcmp(str_var,"OFF") == 0)
							{
								device_data.power_monitoring_relay_data[0]&=~FAN_RELAY;
							}
							set_ok=1;							
						}
					}
				break;
				case ATTR_RECLOSE_INDEX:
					if(device_data.autoeclosing_enable_flag==1)
					{
						if(strcmp(type_data,"KA22016AS")==0)
						{
							device_data.reclose_flag=1;
							if (strcmp(str_var,"ON") == 0)
							{
								device_data.reclose_data_flag=1;
							}
							else if (strcmp(str_var,"OFF") == 0)
							{
								device_data.reclose_data_flag=2;
							}
							set_ok=1;					
						}					
					}
				break;	
				case ATTR_CBCLOSE_INDEX:
					if(device_data.circuit_breaker_enable_flag==1)
					{
						for (j = 0; j < CIRCUIT_BREAKER_MAX_NUM; j++)
						{
							if(device_data.cb_slave_addr[j]!=0xff)
							{
								snprintf(name_data, 16, "BRKRS_%d",device_data.cb_slave_addr[j]);
								if(strcmp(id_data,name_data)==0)
								{
									device_data.cbclose_flag=1;
									device_data.cbclose_data_addr=j;
									if (strcmp(str_var,"ON") == 0)
									{
										device_data.cbclose_data_flag=1;
									}
									else if (strcmp(str_var,"OFF") == 0)
									{
										device_data.cbclose_data_flag=2;
									}
									set_ok=1;							
								}
							}				
						}					
					}					
				break;
				case ATTR_SET_INDEX:
					device_data.set_flag=1;
					set_ok=0;
				break;					
				///////////////////////////////////////////////////
				case ATTR_SPUV_INDEX:// RW //单相欠压（报警）阈值
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_SPUV=p_data*100;
					}					
				break;		
				case ATTR_SPUVP_INDEX:// RW //单相欠压（预警）阈值
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_SPUVP=p_data*100;
					}
				break;	
				case ATTR_SPOV_INDEX:// RW //单相过压（报警）阈值
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_SPOV=p_data*100;
					}
				break;	
				case ATTR_SPOVP_INDEX:// RW //单相过压（预警）阈值
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_SPOVP=p_data*100;
					}
				break;
				case ATTR_SPVC_INDEX:// RW //单相过流（报警）阈值
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_SPVC=p_data*10;
					}
				break;	
				case ATTR_SPVCP_INDEX:// RW //单相过流（预警）阈值
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_SPVCP=p_data*10;
					}
				break;	
				case ATTR_SPOT_INDEX:// RW //单相线温（报警）阈值
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_SPOT=p_data*10;
					}
				break;	
				case ATTR_SPOTP_INDEX:// RW //单相线温（预警）阈值
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_SPOTP=p_data*10;
					}
				break;
				case ATTR_SLEAKAGE_INDEX:// RW //软件漏电(报警)电流
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_SLEAKAGE=p_data;
					}
					if(device_data.circuit_breaker_enable_flag==1)
					{
						device_data.cb_hold_flag=1;
						set_ok=1;						
					}
				break;	
				case ATTR_SLEAKAGEP_INDEX:// RW //软件漏电(预警)电流
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_SLEAKAGEP=p_data;
					}
					if(device_data.circuit_breaker_enable_flag==1)
					{
						device_data.cb_hold_flag=1;
						set_ok=1;						
					}
				break;	
				case ATTR_OP_INDEX:// RW //过功率(报警)阈值
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OP=p_data;
					}
				break;	
				case ATTR_OPP_INDEX:// RW //过功率(预警)阈值
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OPP=p_data;
					}
				break;
				//////////////////////////////////////////////////////////////
				case ATTR_UVA_INDEX:// RW //欠压(报警)阈值A
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_UVA=p_data*100;
					}				
				break;	
				case ATTR_UVAP_INDEX:// RW //欠压(预警)阈值A
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_UVAP=p_data*100;
					}
				break;	
				case ATTR_UVB_INDEX:// RW //欠压(报警)阈值B
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_UVB=p_data*100;
					}
				break;	
				case ATTR_UVBP_INDEX:// RW //欠压(预警)阈值B
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_UVBP=p_data*100;
					}
				break;
				case ATTR_UVC_INDEX:// RW //欠压(报警)阈值C
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_UVC=p_data*100;
					}
				break;	
				case ATTR_UVCP_INDEX:// RW //欠压(预警)阈值C
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_UVCP=p_data*100;
					}
				break;	
				//////////////////////////////////////////////////////////////		
				case ATTR_OVA_INDEX:// RW //过压(报警)阈值A
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OVA=p_data*100;
					}
				break;	
				case ATTR_OVAP_INDEX:// RW //过压(预警)阈值A
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OVAP=p_data*100;
					}
				break;
				case ATTR_OVB_INDEX:// RW //过压(报警)阈值B
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OVB=p_data*100;
					}
				break;	
				case ATTR_OVBP_INDEX:// RW //过压(预警)阈值B
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OVBP=p_data*100;
					}
				break;	
				case ATTR_OVC_INDEX:// RW //过压(报警)阈值C
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OVC=p_data*100;
					}
				break;	
				case ATTR_OVCP_INDEX:// RW //过压(预警)阈值C
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OVCP=p_data*100;
					}
				break;
				//////////////////////////////////////////////////////////////
				case ATTR_OCA_INDEX:// RW //过流(报警)阈值A
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OCA=p_data*10;
					}
				break;	
				case ATTR_OCAP_INDEX:// RW //过流(预警)阈值A
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OCAP=p_data*10;
					}
				break;	
				case ATTR_OCB_INDEX:// RW //过流(报警)阈值B
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OCB=p_data*10;
					}
				break;	
				case ATTR_OCBP_INDEX:// RW //过流(预警)阈值B
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OCBP=p_data*10;
					}
				break;
				case ATTR_OCC_INDEX:// RW //过流(报警)阈值C
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OCC=p_data*10;
					}
				break;	
				case ATTR_OCCP_INDEX:// RW //过流(预警)阈值C
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OCCP=p_data*10;
					}
				break;	
				//////////////////////////////////////////////////////////////
				case ATTR_OTA_INDEX:// RW //温度(报警)阈值A
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OTA=p_data*10;
					}
				break;	
				case ATTR_OTAP_INDEX:// RW //温度(预警)阈值A
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OTAP=p_data*10;
					}
				break;
				case ATTR_OTB_INDEX:// RW //温度(报警)阈值B
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OTB=p_data*10;
					}
				break;	
				case ATTR_OTBP_INDEX:// RW //温度(预警)阈值B
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OTBP=p_data*10;
					}
				break;	
				case ATTR_OTC_INDEX:// RW //温度(报警)阈值C
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OTC=p_data*10;
					}
				break;	
				case ATTR_OTCP_INDEX:// RW //温度(预警)阈值C
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OTCP=p_data*10;
					}
				break;
				case ATTR_OTN_INDEX:// RW //温度(报警)阈值N
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OTN=p_data*10;
					}
				break;	
				case ATTR_OTNP_INDEX:// RW //温度(预警)阈值N	
					p_data=atoi(str_var);
					if(device_data.cbclose_data_addr!=0xff)
					{
						reg_hold[device_data.cbclose_data_addr].REG1_OTNP=p_data*10;
					}
				break;
			}
		}
	}
	json_release_composite_object(&jobj);
	json_parse_stop(&jobj);	
}

int send_message_to_cloud(char* publish_topic_addr,char *msg_up_buf, uint32_t msg_len)
{	
	MQTTMessage g_mqtt_message;		
	uint8_t retval;
	
	//log_i("topic = %s", publish_topic_addr);
	//log_i("msg_len = %d---%s",msg_len, msg_up_buf);
    /* Configure publish message */
    g_mqtt_message.qos = QOS0;
    g_mqtt_message.retained = 0;
    g_mqtt_message.dup = 0;
    g_mqtt_message.payload = msg_up_buf;
    g_mqtt_message.payloadlen = strlen(g_mqtt_message.payload);//msg_len;//strlen(g_mqtt_message.payload);	
	
	/* Publish */
    retval = MQTTPublish(&g_mqtt_client, publish_topic_addr, &g_mqtt_message);
	if (retval < 0)
	{
		log_e("mqtt","SendMessage error occur when publish iret = %d",retval);
	}
	return retval;
}

void relay_to_bit(uint8_t slave_addr)
{
	uint16_t tmp=read_power_board_relay_status(slave_addr);
	
	for (int j =0; j<= 9; j++) 
	{
		if((tmp&0x0001)==0x0001)
		{
			relay_buf[j]=1;
		}
		else
		{
			relay_buf[j]=0;
		}
		tmp >>= 1;
	}
}

void autoeclosing_device_status_to_bit(void)
{
	uint16_t tmp=read_autoeclosing_device_status();
	if((tmp&0x0001)==0x0001)
	{
		relay_buf[0]=1;
	}
	else
	{
		relay_buf[0]=0;
	}
	if((tmp&0x0002)==0x0002)
	{
		relay_buf[1]=1;
	}
	else
	{
		relay_buf[1]=0;
	}
	if((tmp&0x0004)==0x0004)
	{
		relay_buf[2]=1;
	}
	else
	{
		relay_buf[2]=0;
	}
	if((tmp&0x0008)==0x0008)
	{
		relay_buf[3]=1;
	}
	else
	{
		relay_buf[3]=0;
	}
	if((tmp&0x0010)==0x0010)
	{
		relay_buf[4]=1;
	}
	else
	{
		relay_buf[4]=0;
	}
	if((tmp&0x0020)==0x0020)
	{
		relay_buf[5]=1;
	}
	else
	{
		relay_buf[5]=0;
	}
	if((tmp&0x0100)==0x0100)
	{
		relay_buf[6]=1;
	}
	else
	{
		relay_buf[6]=0;
	}
	if((tmp&0x0200)==0x0200)
	{
		relay_buf[7]=1;
	}
	else
	{
		relay_buf[7]=0;
	}
	if((tmp&0x0400)==0x0400)
	{
		relay_buf[8]=1;
	}
	else
	{
		relay_buf[8]=0;
	}
	if((tmp&0x0800)==0x0800)
	{
		relay_buf[9]=1;
	}
	else
	{
		relay_buf[9]=0;
	}
}

int report_autoeclosing_data(void)
{
	char msg_up_buf[MQTT_MSGLEN];
	char bt_buf[64];
	char timer_buf[64];
	uint32_t msg_len;
	int iret  = 0;
	struct json_str jjptr;
	struct json_str *jptr = &jjptr;

	json_str_init(jptr, msg_up_buf, sizeof(msg_up_buf));
	iret = json_start_object(jptr);
	if (iret != SYS_OK)
	{
		json_close_object(jptr);
		log_i("iotkit_Report_DeviceData: ret = %d ", iret);
		return iret;
	}
	autoeclosing_device_status_to_bit();
	
	sprintf(timer_buf,"%d/%02d/%02d %02d:%02d:%02d", \
	device_data.c_date.u8Year+2000,device_data.c_date.u8Month, \
	device_data.c_date.u8Day,device_data.c_time.u8Hour, \
	device_data.c_time.u8Minute,device_data.c_time.u8Second); 
	
	json_push_object(jptr, "header");
	json_set_val_str(jptr, "commamd","update");
	json_set_val_str(jptr, "client", sys_psm.device_id);
	json_set_val_str(jptr, "time", timer_buf);
	json_set_val_str(jptr, "type","KA22016AS");
	json_set_val_str(jptr, "id", "ZCH_1");
	json_set_val_str(jptr, "ver", "01");
	json_pop_object(jptr);	
	
	json_push_object(jptr, "data");
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P01");
	json_set_val_str(jptr, "N", "K01");
	json_set_val_str(jptr, "C", "001");
	snprintf(bt_buf, 16, "%d",read_autoeclosing_do_status());
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	////////////////////////////////////////////////////
	json_push_object(jptr, "P02");
	json_set_val_str(jptr, "N", "X02");
	json_set_val_str(jptr, "C", "002");
	snprintf(bt_buf, 16, "%d",read_autoeclosing_voltage_value());
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P03");
	json_set_val_str(jptr, "N", "X03");
	json_set_val_str(jptr, "C", "003");
	snprintf(bt_buf, 16, "%.2f",read_autoeclosing_current_value());
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P04");
	json_set_val_str(jptr, "N", "X04");
	json_set_val_str(jptr, "C", "004");
	snprintf(bt_buf, 16, "%d",read_autoeclosing_creepage_current_value());
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P05");
	json_set_val_str(jptr, "N", "X05");
	json_set_val_str(jptr, "C", "005");
	snprintf(bt_buf, 16, "%d",read_autoeclosing_reclosing_times());
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P06");
	json_set_val_str(jptr, "N", "X06");
	json_set_val_str(jptr, "C", "006");
	snprintf(bt_buf, 16, "%d",read_autoeclosing_over_voltage_value());
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P07");
	json_set_val_str(jptr, "N", "X07");
	json_set_val_str(jptr, "C", "007");
	snprintf(bt_buf, 16, "%d",read_autoeclosing_low_voltage_value());
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P08");
	json_set_val_str(jptr, "N", "X08");
	json_set_val_str(jptr, "C", "008");
	snprintf(bt_buf, 16, "%d%d%d%d%d%d%d%d%d%d%d",relay_buf[0],relay_buf[0],relay_buf[1],
	relay_buf[2],relay_buf[3],relay_buf[4],relay_buf[5],relay_buf[6],relay_buf[7],
	relay_buf[8],relay_buf[9]);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P09");
	json_set_val_str(jptr, "N", "X09");
	json_set_val_str(jptr, "C", "009");
	snprintf(bt_buf, 16, "%d",read_autoeclosing_fault_opening_times());
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_pop_object(jptr);

	json_close_object(jptr);
	msg_len=jptr->free_ptr;
	iret=send_message_to_cloud(sys_psm.mqtt_publish_sensor_buf,msg_up_buf,msg_len);
	return iret;
}

int report_gate_set_ack(uint8_t flag)
{
	char msg_up_buf[MQTT_MSGLEN];
	char bt_buf[64];
	char timer_buf[64];
	char ver_buf[16];
	uint32_t msg_len;
	int iret  = 0;
	struct json_str jjptr;
	struct json_str *jptr = &jjptr;

	sprintf(timer_buf,"%d/%02d/%02d %02d:%02d:%02d", \
	device_data.c_date.u8Year+2000,device_data.c_date.u8Month, \
	device_data.c_date.u8Day,device_data.c_time.u8Hour, \
	device_data.c_time.u8Minute,device_data.c_time.u8Second); 
	
	sprintf(ver_buf,"%d.%d.%d",MCU_SW_VERSION_MAJOR,MCU_SW_VERSION_MINOR,MCU_SW_VERSION_BUILD); 
	
	json_str_init(jptr, msg_up_buf, sizeof(msg_up_buf));
	iret = json_start_object(jptr);
	if (iret != SYS_OK)
	{
		json_close_object(jptr);
		log_i("iotkit_Report_DeviceData: ret = %d ", iret);
		return iret;
	}
	json_push_object(jptr, "header");
	json_set_val_str(jptr, "commamd","alarm");
	json_set_val_str(jptr, "client", sys_psm.device_id);
	json_set_val_str(jptr, "time", timer_buf);
	json_set_val_str(jptr, "type","GATE");
	json_set_val_str(jptr, "id", sys_psm.device_id);
	json_set_val_str(jptr, "ver", ver_buf);
	json_pop_object(jptr);
	
	json_push_object(jptr, "data");
	////////////////////////////////////////////////////
	if(flag==1)
	{
		json_set_val_str(jptr, "set", "OK");
	}
	else
	{
		json_set_val_str(jptr, "set", "ERROR");
	}	
	json_pop_object(jptr);
	
	json_close_object(jptr);
	msg_len=jptr->free_ptr;
	iret=send_message_to_cloud(sys_psm.mqtt_publish_ack_buf,msg_up_buf,msg_len);
	device_data.error_status_mqtt=device_data.error_status_old;
	return iret;
}

int report_gate_list_ack(void)
{
	char msg_up_buf[MQTT_MSGLEN];
	char bt_buf[64];
	char timer_buf[64];
	char ver_buf[16];
	uint32_t msg_len;
	int iret  = 0;
	int i  = 0;
	struct json_str jjptr;
	struct json_str *jptr = &jjptr;
	uint8_t list=1;
	uint8_t addr_tmp=0;

	sprintf(timer_buf,"%d/%02d/%02d %02d:%02d:%02d", \
	device_data.c_date.u8Year+2000,device_data.c_date.u8Month, \
	device_data.c_date.u8Day,device_data.c_time.u8Hour, \
	device_data.c_time.u8Minute,device_data.c_time.u8Second); 
	
	sprintf(ver_buf,"%d.%d.%d",MCU_SW_VERSION_MAJOR,MCU_SW_VERSION_MINOR,MCU_SW_VERSION_BUILD); 
	
	json_str_init(jptr, msg_up_buf, sizeof(msg_up_buf));
	iret = json_start_object(jptr);
	if (iret != SYS_OK)
	{
		json_close_object(jptr);
		log_i("iotkit_Report_DeviceData: ret = %d ", iret);
		return iret;
	}
	json_push_object(jptr, "header");
	json_set_val_str(jptr, "commamd","alarm");
	json_set_val_str(jptr, "client", sys_psm.device_id);
	json_set_val_str(jptr, "time", timer_buf);
	json_set_val_str(jptr, "type","GATE");
	json_set_val_str(jptr, "id", sys_psm.device_id);
	json_set_val_str(jptr, "ver", ver_buf);
	json_pop_object(jptr);
	
	json_push_object(jptr, "data");
	////////////////////////////////////////////////////
	for (i = 0; i < device_data.slave_device_max; i++)
	{
		if(device_data.own_slave_device[i].device_type==POWER_MONITOR_BOARD)
		{
			sprintf(bt_buf,"list%0d",list); 
			json_push_object(jptr, bt_buf);
			json_set_val_str(jptr, "type","GTPM");
			json_set_val_str(jptr, "id", device_data.own_slave_device[i].device_name);
			snprintf(bt_buf, 16, "%d",power_board_data[i].device_sw_ver);
			json_set_val_str(jptr, "ver", bt_buf);
			json_pop_object(jptr);	
			list++;
		}
	}	
	////////////////////重合闸上报//////////////////////////////////////
	if(device_data.autoeclosing_enable_flag==1)
	{
		sprintf(bt_buf,"list%0d",list); 
		json_push_object(jptr, bt_buf);
		json_set_val_str(jptr, "type","KA22016AS");
		json_set_val_str(jptr, "id", "ZCH_1");
		json_set_val_str(jptr, "ver", "01");
		json_pop_object(jptr);	
		list++;
	}
	//////////////////////断路器上报/////////////////////////////////////
	if(device_data.circuit_breaker_enable_flag==1)
	{
		for (i = 0; i < CIRCUIT_BREAKER_MAX_NUM; i++)
		{
			if(device_data.cb_slave_addr[i]!=0xff)
			{
				addr_tmp=device_data.cb_slave_addr[i];
				sprintf(bt_buf,"list%0d",list); 
				json_push_object(jptr, bt_buf);
				snprintf(bt_buf, 16, "BRKRS_0X%02x",reg_in[addr_tmp].REG0_TYPE);
				json_set_val_str(jptr, "type",bt_buf);
				snprintf(bt_buf, 16, "BRKRS_%d",addr_tmp);
				json_set_val_str(jptr, "id", bt_buf);
				snprintf(bt_buf, 16, "%d",reg_in[addr_tmp].REG0_VER);
				json_set_val_str(jptr, "ver", bt_buf);
				json_pop_object(jptr);	
				list++;					
			}				
		}			
	}
	////////////////////////////////////////////////////
	json_pop_object(jptr);
	
	json_close_object(jptr);
	msg_len=jptr->free_ptr;
	iret=send_message_to_cloud(sys_psm.mqtt_publish_ack_buf,msg_up_buf,msg_len);
	device_data.error_status_mqtt=device_data.error_status_old;
	return iret;
}

int report_gate_alarm(void)
{
	char msg_up_buf[MQTT_MSGLEN];
	char bt_buf[64];
	char timer_buf[64];
	char ver_buf[16];
	uint32_t msg_len;
	int iret  = 0;
	struct json_str jjptr;
	struct json_str *jptr = &jjptr;

	sprintf(timer_buf,"%d/%02d/%02d %02d:%02d:%02d", \
	device_data.c_date.u8Year+2000,device_data.c_date.u8Month, \
	device_data.c_date.u8Day,device_data.c_time.u8Hour, \
	device_data.c_time.u8Minute,device_data.c_time.u8Second); 
	
	sprintf(ver_buf,"%d.%d.%d",MCU_SW_VERSION_MAJOR,MCU_SW_VERSION_MINOR,MCU_SW_VERSION_BUILD); 
	
	json_str_init(jptr, msg_up_buf, sizeof(msg_up_buf));
	iret = json_start_object(jptr);
	if (iret != SYS_OK)
	{
		json_close_object(jptr);
		log_i("iotkit_Report_DeviceData: ret = %d ", iret);
		return iret;
	}
	json_push_object(jptr, "header");
	json_set_val_str(jptr, "commamd","alarm");
	json_set_val_str(jptr, "client", sys_psm.device_id);
	json_set_val_str(jptr, "time", timer_buf);
	json_set_val_str(jptr, "type","GATE");
	json_set_val_str(jptr, "id", sys_psm.device_id);
	json_set_val_str(jptr, "ver", ver_buf);
	json_pop_object(jptr);
	
	json_push_object(jptr, "data");
	////////////////////////////////////////////////////
	if((device_data.error_status_old&WATER_SENSOR_ERROR)!=(device_data.error_status_mqtt&WATER_SENSOR_ERROR))
	{
		if(device_data.water_in_error_flag==0)
		{
			json_set_val_str(jptr, "water", "normal");
		}
		else
		{
			json_set_val_str(jptr, "water", "alarm");
		}	
	}
	if((device_data.error_status_old&SMOKE_SENSOR_ERROR)!=(device_data.error_status_mqtt&SMOKE_SENSOR_ERROR))
	{
		if(device_data.smoke_in_error_flag==0)
		{
			json_set_val_str(jptr, "smoke", "normal");
		}
		else
		{
			json_set_val_str(jptr, "smoke", "alarm");
		}
	}
	if((device_data.error_status_old&LOCK_SENSOR_ERROR)!=(device_data.error_status_mqtt&LOCK_SENSOR_ERROR))
	{
		if(device_data.lock_in_error_flag==0)
		{
			json_set_val_str(jptr, "lock", "normal");
		}
		else
		{
			json_set_val_str(jptr, "lock", "alarm");
		}	
	}
	if((device_data.error_status_old&SPD_SENSOR_ERROR)!=(device_data.error_status_mqtt&SPD_SENSOR_ERROR))
	{
		if(device_data.spd_in_error_flag==0)
		{
			json_set_val_str(jptr, "spd", "normal");
		}
		else
		{
			json_set_val_str(jptr, "spd", "alarm");
		}
	}
	if((device_data.error_status_old&CAMERA_SENSOR_ERROR)!=(device_data.error_status_mqtt&CAMERA_SENSOR_ERROR))
	{
		if(device_data.camera_error_flag==0)
		{
			json_set_val_str(jptr, "camera", "normal");
		}
		else
		{
			json_set_val_str(jptr, "camera", "alarm");
		}
	}	
	json_pop_object(jptr);
	
	json_close_object(jptr);
	msg_len=jptr->free_ptr;
	iret=send_message_to_cloud(sys_psm.mqtt_publish_alarm_buf,msg_up_buf,msg_len);
	device_data.error_status_mqtt=device_data.error_status_old;
	return iret;
}

int report_gate_data(void)
{
	char msg_up_buf[MQTT_MSGLEN];
	char bt_buf[64];
	char timer_buf[64];
	char ver_buf[16];
	uint32_t msg_len;
	int iret  = 0;
	struct json_str jjptr;
	struct json_str *jptr = &jjptr;

	sprintf(timer_buf,"%d/%02d/%02d %02d:%02d:%02d", \
	device_data.c_date.u8Year+2000,device_data.c_date.u8Month, \
	device_data.c_date.u8Day,device_data.c_time.u8Hour, \
	device_data.c_time.u8Minute,device_data.c_time.u8Second); 
	
	sprintf(ver_buf,"%d.%d.%d",MCU_SW_VERSION_MAJOR,MCU_SW_VERSION_MINOR,MCU_SW_VERSION_BUILD); 
	
	json_str_init(jptr, msg_up_buf, sizeof(msg_up_buf));
	iret = json_start_object(jptr);
	if (iret != SYS_OK)
	{
		json_close_object(jptr);
		log_i("iotkit_Report_DeviceData: ret = %d ", iret);
		return iret;
	}
	json_push_object(jptr, "header");
	json_set_val_str(jptr, "commamd","update");
	json_set_val_str(jptr, "client", sys_psm.device_id);
	json_set_val_str(jptr, "time", timer_buf);
	json_set_val_str(jptr, "type","GATE");
	json_set_val_str(jptr, "id", sys_psm.device_id);
	json_set_val_str(jptr, "ver", ver_buf);
	json_pop_object(jptr);
	
	json_push_object(jptr, "data");
	////////////////////////////////////////////////////
	json_push_object(jptr, "P01");
	json_set_val_str(jptr, "N", "G01");
	json_set_val_str(jptr, "C", "001");
	if(device_data.water_in_error_flag==0)
	{
		json_set_val_str(jptr, "V", "1");
		json_set_val_str(jptr, "alert", "NORMAL");
	}
	else
	{
		json_set_val_str(jptr, "V", "0");
		json_set_val_str(jptr, "alert", "ALARM");	
	}
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P02");
	json_set_val_str(jptr, "N", "G02");
	json_set_val_str(jptr, "C", "002");
	if(device_data.smoke_in_error_flag==0)
	{
		json_set_val_str(jptr, "V", "1");
		json_set_val_str(jptr, "alert", "NORMAL");
	}
	else
	{
		json_set_val_str(jptr, "V", "0");
		json_set_val_str(jptr, "alert", "ALARM");	
	}
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P03");
	json_set_val_str(jptr, "N", "G03");
	json_set_val_str(jptr, "C", "003");
	if(device_data.lock_in_error_flag==0)
	{
		json_set_val_str(jptr, "V", "1");
		json_set_val_str(jptr, "alert", "NORMAL");
	}
	else
	{
		json_set_val_str(jptr, "V", "0");
		json_set_val_str(jptr, "alert", "ALARM");	
	}
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P04");
	json_set_val_str(jptr, "N", "G04");
	json_set_val_str(jptr, "C", "004");
	if(device_data.spd_in_error_flag==0)
	{
		json_set_val_str(jptr, "V", "1");
		json_set_val_str(jptr, "alert", "NORMAL");
	}
	else
	{
		json_set_val_str(jptr, "V", "0");
		json_set_val_str(jptr, "alert", "ALARM");	
	}
	json_pop_object(jptr);

	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P05");
	json_set_val_str(jptr, "N", "T01");
	json_set_val_str(jptr, "C", "005");
	snprintf(bt_buf, 16, "%.1f",device_data.temp_value);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P06");
	json_set_val_str(jptr, "N", "S01");
	json_set_val_str(jptr, "C", "006");
	snprintf(bt_buf, 16, "%.1f",device_data.humi_value);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P07");
	json_set_val_str(jptr, "N", "N01");
	json_set_val_str(jptr, "C", "007");
	snprintf(bt_buf, 16, "%.4f",device_data.n_latitude);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);	
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P08");
	json_set_val_str(jptr, "N", "S01");
	json_set_val_str(jptr, "C", "008");
	snprintf(bt_buf, 16, "%.4f",device_data.w_longitude);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P09");
	json_set_val_str(jptr, "N", "S01");
	json_set_val_str(jptr, "C", "009");
	snprintf(bt_buf, 16, "%d",device_data.power_flag);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);	
	///////////////////////////////////////////////////////
	json_pop_object(jptr);
	json_close_object(jptr);
	msg_len=jptr->free_ptr;
	iret=send_message_to_cloud(sys_psm.mqtt_publish_sensor_buf,msg_up_buf,msg_len);
	return iret;
}

int report_power_data(uint8_t slave_addr)
{
	char msg_up_buf[MQTT_MSGLEN];
	char bt_buf[64];
	char timer_buf[64];
	uint32_t msg_len;
	int iret  = 0;
	struct json_str jjptr;
	struct json_str *jptr = &jjptr;

	sprintf(timer_buf,"%d/%02d/%02d %02d:%02d:%02d", \
	device_data.c_date.u8Year+2000,device_data.c_date.u8Month, \
	device_data.c_date.u8Day,device_data.c_time.u8Hour, \
	device_data.c_time.u8Minute,device_data.c_time.u8Second); 

	json_str_init(jptr, msg_up_buf, sizeof(msg_up_buf));
	iret = json_start_object(jptr);
	if (iret != SYS_OK)
	{
		json_close_object(jptr);
		log_i("iotkit_Report_DeviceData: ret = %d ", iret);
		return iret;
	}
	relay_to_bit(slave_addr);
	
	json_push_object(jptr, "header");
	json_set_val_str(jptr, "commamd","update");
	json_set_val_str(jptr, "client", sys_psm.device_id);
	json_set_val_str(jptr, "time", timer_buf);
	json_set_val_str(jptr, "type","GTPM");
	json_set_val_str(jptr, "id", device_data.own_slave_device[slave_addr].device_name);
	snprintf(bt_buf, 16, "%d",power_board_data[slave_addr].device_sw_ver);
	json_set_val_str(jptr, "ver", bt_buf);
	json_pop_object(jptr);
	
	json_push_object(jptr, "data");
	////////////////////////////////////////////////////
	json_push_object(jptr, "P01");
	json_set_val_str(jptr, "N", "X01");
	json_set_val_str(jptr, "C", "010");
	snprintf(bt_buf, 16, "%.2f",read_power_board_ac_voltage_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P02");
	json_set_val_str(jptr, "N", "X02");
	json_set_val_str(jptr, "C", "011");
	snprintf(bt_buf, 16, "%.2f",read_power_board_ac_current_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P03");
	json_set_val_str(jptr, "N", "X03");
	json_set_val_str(jptr, "C", "012");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc1_voltage_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P04");
	json_set_val_str(jptr, "N", "X04");
	json_set_val_str(jptr, "C", "013");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc1_current_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P05");
	json_set_val_str(jptr, "N", "X05");
	json_set_val_str(jptr, "C", "014");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc2_voltage_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P06");
	json_set_val_str(jptr, "N", "X06");
	json_set_val_str(jptr, "C", "015");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc2_current_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P07");
	json_set_val_str(jptr, "N", "X07");
	json_set_val_str(jptr, "C", "016");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc3_voltage_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P08");
	json_set_val_str(jptr, "N", "X08");
	json_set_val_str(jptr, "C", "017");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc3_current_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P09");
	json_set_val_str(jptr, "N", "X09");
	json_set_val_str(jptr, "C", "018");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc4_voltage_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P10");
	json_set_val_str(jptr, "N", "X10");
	json_set_val_str(jptr, "C", "019");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc4_current_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P11");
	json_set_val_str(jptr, "N", "X11");
	json_set_val_str(jptr, "C", "020");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc5_voltage_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P12");
	json_set_val_str(jptr, "N", "X12");
	json_set_val_str(jptr, "C", "021");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc5_current_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P13");
	json_set_val_str(jptr, "N", "X13");
	json_set_val_str(jptr, "C", "022");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc6_voltage_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P14");
	json_set_val_str(jptr, "N", "X14");
	json_set_val_str(jptr, "C", "023");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc6_current_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P15");
	json_set_val_str(jptr, "N", "X15");
	json_set_val_str(jptr, "C", "024");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc7_voltage_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P16");
	json_set_val_str(jptr, "N", "X16");
	json_set_val_str(jptr, "C", "025");
	snprintf(bt_buf, 16, "%.2f",read_power_board_dc7_current_value(slave_addr));
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P17");
	json_set_val_str(jptr, "N", "X17");
	json_set_val_str(jptr, "C", "026");
	snprintf(bt_buf, 16, "%d",relay_buf[9]);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);	
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P18");
	json_set_val_str(jptr, "N", "X17");
	json_set_val_str(jptr, "C", "027");
	snprintf(bt_buf, 16, "%d",relay_buf[8]);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);	
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P19");
	json_set_val_str(jptr, "N", "X19");
	json_set_val_str(jptr, "C", "028");
	snprintf(bt_buf, 16, "%d%d%d%d%d%d%d%d",relay_buf[0],relay_buf[1],
	relay_buf[2],relay_buf[3],relay_buf[4],relay_buf[5],relay_buf[6],relay_buf[7]);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);	
	///////////////////////////////////////////////////////
	json_pop_object(jptr);

	json_close_object(jptr);
	msg_len=jptr->free_ptr;
	iret=send_message_to_cloud(sys_psm.mqtt_publish_sensor_buf,msg_up_buf,msg_len);
	return iret;
}

int report_circuit_breaker_2p_data(uint8_t slave_addr)
{
	char msg_up_buf[MQTT_MSGLEN];
	char bt_buf[64];
	char timer_buf[64];
	uint32_t msg_len;
	int iret  = 0;
	struct json_str jjptr;
	struct json_str *jptr = &jjptr;
	double temp;

	json_str_init(jptr, msg_up_buf, sizeof(msg_up_buf));
	iret = json_start_object(jptr);
	if (iret != SYS_OK)
	{
		json_close_object(jptr);
		log_i("iotkit_Report_DeviceData: ret = %d ", iret);
		return iret;
	}
	
	sprintf(timer_buf,"%d/%02d/%02d %02d:%02d:%02d", \
	device_data.c_date.u8Year+2000,device_data.c_date.u8Month, \
	device_data.c_date.u8Day,device_data.c_time.u8Hour, \
	device_data.c_time.u8Minute,device_data.c_time.u8Second); 
	//////////////////////////////////////////////////////////////////////////////
	json_push_object(jptr, "header");
	json_set_val_str(jptr, "commamd","update");
	json_set_val_str(jptr, "client", sys_psm.device_id);
	json_set_val_str(jptr, "time", timer_buf);
	snprintf(bt_buf, 16, "BRKRS_0X%02x",reg_in[slave_addr].REG0_TYPE);
	json_set_val_str(jptr, "type",bt_buf);
	snprintf(bt_buf, 16, "BRKRS_%d",slave_addr);
	json_set_val_str(jptr, "id", bt_buf);
	snprintf(bt_buf, 16, "%d",reg_in[slave_addr].REG0_VER);
	json_set_val_str(jptr, "ver", bt_buf);
	json_pop_object(jptr);
	//////////////////////////////////////////////////////////////////////////////
	json_push_object(jptr, "data");
	////////////////////////////////////////////////////
	json_push_object(jptr, "P01");
	json_set_val_str(jptr, "N", "TYPE");
	json_set_val_str(jptr, "C", "001");
	snprintf(bt_buf, 16, "0x%02x",reg_in[slave_addr].REG0_TYPE);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P02");
	json_set_val_str(jptr, "N", "SPC");
	json_set_val_str(jptr, "C", "002");
	temp=reg_in[slave_addr].REG0_SPC/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P03");
	json_set_val_str(jptr, "N", "SPV");
	json_set_val_str(jptr, "C", "003");
	temp=reg_in[slave_addr].REG0_SPV/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P04");
	json_set_val_str(jptr, "N", "SPP");
	json_set_val_str(jptr, "C", "004");
	temp=reg_in[slave_addr].REG0_SPP/1000.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P05");
	json_set_val_str(jptr, "N", "SPQ");
	json_set_val_str(jptr, "C", "005");
	temp=reg_in[slave_addr].REG0_SPQ/1000.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P06");
	json_set_val_str(jptr, "N", "SPS");
	json_set_val_str(jptr, "C", "006");
	temp=reg_in[slave_addr].REG0_SPS/1000.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P07");
	json_set_val_str(jptr, "N", "SPSE");
	json_set_val_str(jptr, "C", "007");
	temp=reg_in[slave_addr].REG0_SPSE/1000.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P08");
	json_set_val_str(jptr, "N", "SPTEMP");
	json_set_val_str(jptr, "C", "008");
	temp=reg_in[slave_addr].REG0_SPTEMP/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P09");
	json_set_val_str(jptr, "N", "LEAKAGEC");
	json_set_val_str(jptr, "C", "009");
	snprintf(bt_buf, 16, "%d",reg_in[slave_addr].REG0_LEAKAGE_C);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P10");
	json_set_val_str(jptr, "N", "RATEDC");
	json_set_val_str(jptr, "C", "010");
	snprintf(bt_buf, 16, "%d",reg_in[slave_addr].REG0_RATED_C);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P11");
	json_set_val_str(jptr, "N", "OVERH");
	json_set_val_str(jptr, "C", "011");
	snprintf(bt_buf, 16, "%d",reg_in[slave_addr].REG0_OVER_HARMONIC);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P12");
	json_set_val_str(jptr, "N", "FREQ");
	json_set_val_str(jptr, "C", "012");
	temp=reg_in[slave_addr].REG0_FREQ/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P13");
	json_set_val_str(jptr, "N", "LEAKAGE");
	json_set_val_str(jptr, "C", "013");
	snprintf(bt_buf, 16, "%d",reg_discrete[slave_addr].discrete2.bit.bit2);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P14");
	json_set_val_str(jptr, "N", "CLOSE");
	json_set_val_str(jptr, "C", "014");
	snprintf(bt_buf, 16, "%d",reg_discrete[slave_addr].discrete2.bit.bit3);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P15");
	json_set_val_str(jptr, "N", "SPUV");
	json_set_val_str(jptr, "C", "015");
	temp=reg_hold[slave_addr].REG1_SPUV/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P16");
	json_set_val_str(jptr, "N", "SPUVP");
	json_set_val_str(jptr, "C", "016");
	temp=reg_hold[slave_addr].REG1_SPUVP/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P17");
	json_set_val_str(jptr, "N", "SPOV");
	json_set_val_str(jptr, "C", "017");
	temp=reg_hold[slave_addr].REG1_SPOV/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P18");
	json_set_val_str(jptr, "N", "SPOVP");
	json_set_val_str(jptr, "C", "018");
	temp=reg_hold[slave_addr].REG1_SPOVP/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P19");
	json_set_val_str(jptr, "N", "SPVC");
	json_set_val_str(jptr, "C", "019");
	temp=reg_hold[slave_addr].REG1_SPVC/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P20");
	json_set_val_str(jptr, "N", "SPVCP");
	json_set_val_str(jptr, "C", "020");
	temp=reg_hold[slave_addr].REG1_SPVCP/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P21");
	json_set_val_str(jptr, "N", "SPOT");
	json_set_val_str(jptr, "C", "021");
	temp=reg_hold[slave_addr].REG1_SPOT/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P22");
	json_set_val_str(jptr, "N", "SPOTP");
	json_set_val_str(jptr, "C", "022");
	temp=reg_hold[slave_addr].REG1_SPOTP/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P23");
	json_set_val_str(jptr, "N", "SLEAKAGE");
	json_set_val_str(jptr, "C", "023");
	temp=reg_hold[slave_addr].REG1_SLEAKAGE;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P24");
	json_set_val_str(jptr, "N", "SLEAKAGEP");
	json_set_val_str(jptr, "C", "024");
	temp=reg_hold[slave_addr].REG1_SLEAKAGEP;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P25");
	json_set_val_str(jptr, "N", "OL");
	json_set_val_str(jptr, "C", "025");
	temp=reg_hold[slave_addr].REG1_OL/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P26");
	json_set_val_str(jptr, "N", "OLP");
	json_set_val_str(jptr, "C", "026");
	temp=reg_hold[slave_addr].REG1_OLP/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P27");
	json_set_val_str(jptr, "N", "OP");
	json_set_val_str(jptr, "C", "027");
	snprintf(bt_buf, 16, "%d",reg_hold[slave_addr].REG1_OP);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P28");
	json_set_val_str(jptr, "N", "OPP");
	json_set_val_str(jptr, "C", "015");
	snprintf(bt_buf, 16, "%d",reg_hold[slave_addr].REG1_OPP);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_pop_object(jptr);
	json_close_object(jptr);
	msg_len=jptr->free_ptr;
	iret=send_message_to_cloud(sys_psm.mqtt_publish_sensor_buf,msg_up_buf,msg_len);
	return iret;
}

int report_circuit_breaker_4p_data(uint8_t slave_addr)
{
	char msg_up_buf[MQTT_MSGLEN];
	char bt_buf[64];
	char timer_buf[64];
	uint32_t msg_len;
	int iret  = 0;
	struct json_str jjptr;
	struct json_str *jptr = &jjptr;
	double temp;

	json_str_init(jptr, msg_up_buf, sizeof(msg_up_buf));
	iret = json_start_object(jptr);
	if (iret != SYS_OK)
	{
		json_close_object(jptr);
		log_i("iotkit_Report_DeviceData: ret = %d ", iret);
		return iret;
	}
	
	sprintf(timer_buf,"%d/%02d/%02d %02d:%02d:%02d", \
	device_data.c_date.u8Year+2000,device_data.c_date.u8Month, \
	device_data.c_date.u8Day,device_data.c_time.u8Hour, \
	device_data.c_time.u8Minute,device_data.c_time.u8Second); 
	//////////////////////////////////////////////////////////////////////////////
	json_push_object(jptr, "header");
	json_set_val_str(jptr, "commamd","update");
	json_set_val_str(jptr, "client", sys_psm.device_id);
	json_set_val_str(jptr, "time", timer_buf);
	snprintf(bt_buf, 16, "BRKRS_0X%02x",reg_in[slave_addr].REG0_TYPE);
	json_set_val_str(jptr, "type",bt_buf);
	snprintf(bt_buf, 16, "BRKRS_%d",slave_addr);
	json_set_val_str(jptr, "id", bt_buf);
	snprintf(bt_buf, 16, "%d",reg_in[slave_addr].REG0_VER);
	json_set_val_str(jptr, "ver", bt_buf);
	json_pop_object(jptr);
	//////////////////////////////////////////////////////////////////////////////
	json_push_object(jptr, "data");
	////////////////////////////////////////////////////
	json_push_object(jptr, "P01");
	json_set_val_str(jptr, "N", "TYPE");
	json_set_val_str(jptr, "C", "001");
	snprintf(bt_buf, 16, "0x%02x",reg_in[slave_addr].REG0_TYPE);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P02");
	json_set_val_str(jptr, "N", "AC");
	json_set_val_str(jptr, "C", "002");
	temp=reg_in[slave_addr].REG0_AC/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P03");
	json_set_val_str(jptr, "N", "VA");
	json_set_val_str(jptr, "C", "002");
	temp=reg_in[slave_addr].REG0_VA/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P04");
	json_set_val_str(jptr, "N", "PA");
	json_set_val_str(jptr, "C", "004");
	temp=reg_in[slave_addr].REG0_PA/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P05");
	json_set_val_str(jptr, "N", "QA");
	json_set_val_str(jptr, "C", "005");
	temp=reg_in[slave_addr].REG0_QA/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P06");
	json_set_val_str(jptr, "N", "SA");
	json_set_val_str(jptr, "C", "006");
	temp=reg_in[slave_addr].REG0_SA/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P07");
	json_set_val_str(jptr, "N", "SEA");
	json_set_val_str(jptr, "C", "007");
	temp=reg_in[slave_addr].REG0_SEA/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P08");
	json_set_val_str(jptr, "N", "TEMPA");
	json_set_val_str(jptr, "C", "005");
	temp=reg_in[slave_addr].REG0_TEMPA/10.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P09");
	json_set_val_str(jptr, "N", "AEA");
	json_set_val_str(jptr, "C", "009");
	temp=reg_in[slave_addr].REG0_AEA/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P10");
	json_set_val_str(jptr, "N", "RAEA");
	json_set_val_str(jptr, "C", "010");
	temp=reg_in[slave_addr].REG0_RAEA/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	

	json_push_object(jptr, "P11");
	json_set_val_str(jptr, "N", "BC");
	json_set_val_str(jptr, "C", "011");
	temp=reg_in[slave_addr].REG0_BC/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P12");
	json_set_val_str(jptr, "N", "VB");
	json_set_val_str(jptr, "C", "012");
	temp=reg_in[slave_addr].REG0_VB/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P13");
	json_set_val_str(jptr, "N", "PB");
	json_set_val_str(jptr, "C", "013");
	temp=reg_in[slave_addr].REG0_PA/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P14");
	json_set_val_str(jptr, "N", "QB");
	json_set_val_str(jptr, "C", "014");
	temp=reg_in[slave_addr].REG0_QB/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P15");
	json_set_val_str(jptr, "N", "SB");
	json_set_val_str(jptr, "C", "015");
	temp=reg_in[slave_addr].REG0_SA/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P16");
	json_set_val_str(jptr, "N", "SEB");
	json_set_val_str(jptr, "C", "016");
	temp=reg_in[slave_addr].REG0_SEB/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P17");
	json_set_val_str(jptr, "N", "TEMPB");
	json_set_val_str(jptr, "C", "017");
	temp=reg_in[slave_addr].REG0_TEMPB/10.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P18");
	json_set_val_str(jptr, "N", "AEB");
	json_set_val_str(jptr, "C", "018");
	temp=reg_in[slave_addr].REG0_AEB/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P19");
	json_set_val_str(jptr, "N", "RAEB");
	json_set_val_str(jptr, "C", "019");
	temp=reg_in[slave_addr].REG0_RAEB/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P20");
	json_set_val_str(jptr, "N", "CC");
	json_set_val_str(jptr, "C", "020");
	temp=reg_in[slave_addr].REG0_CC/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P21");
	json_set_val_str(jptr, "N", "VC");
	json_set_val_str(jptr, "C", "021");
	temp=reg_in[slave_addr].REG0_VC/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P22");
	json_set_val_str(jptr, "N", "PC");
	json_set_val_str(jptr, "C", "022");
	temp=reg_in[slave_addr].REG0_PC/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P23");
	json_set_val_str(jptr, "N", "QC");
	json_set_val_str(jptr, "C", "023");
	temp=reg_in[slave_addr].REG0_QC/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P24");
	json_set_val_str(jptr, "N", "SC");
	json_set_val_str(jptr, "C", "024");
	temp=reg_in[slave_addr].REG0_SC/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P25");
	json_set_val_str(jptr, "N", "SEC");
	json_set_val_str(jptr, "C", "025");
	temp=reg_in[slave_addr].REG0_SEC/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P26");
	json_set_val_str(jptr, "N", "TEMPC");
	json_set_val_str(jptr, "C", "026");
	temp=reg_in[slave_addr].REG0_TEMPC/10.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P27");
	json_set_val_str(jptr, "N", "AEC");
	json_set_val_str(jptr, "C", "027");
	temp=reg_in[slave_addr].REG0_AEC/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P28");
	json_set_val_str(jptr, "N", "RAEC");
	json_set_val_str(jptr, "C", "028");
	temp=reg_in[slave_addr].REG0_RAEC/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P29");
	json_set_val_str(jptr, "N", "NC");
	json_set_val_str(jptr, "C", "029");
	temp=reg_in[slave_addr].REG0_NC/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P30");
	json_set_val_str(jptr, "N", "TEMPA");
	json_set_val_str(jptr, "C", "030");
	temp=reg_in[slave_addr].REG0_TEMPA/10.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P31");
	json_set_val_str(jptr, "N", "LEAKAGEC");
	json_set_val_str(jptr, "C", "031");
	snprintf(bt_buf, 16, "%d",reg_in[slave_addr].REG0_LEAKAGE_C);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P32");
	json_set_val_str(jptr, "N", "RATEDC");
	json_set_val_str(jptr, "C", "032");
	snprintf(bt_buf, 16, "%d",reg_in[slave_addr].REG0_RATED_C);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P33");
	json_set_val_str(jptr, "N", "OVERH");
	json_set_val_str(jptr, "C", "033");
	snprintf(bt_buf, 16, "%d",reg_in[slave_addr].REG0_OVER_HARMONIC);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P34");
	json_set_val_str(jptr, "N", "FREQ");
	json_set_val_str(jptr, "C", "0134");
	temp=reg_in[slave_addr].REG0_FREQ/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P35");
	json_set_val_str(jptr, "N", "LEAKAGE");
	json_set_val_str(jptr, "C", "035");
	snprintf(bt_buf, 16, "%d",reg_discrete[slave_addr].discrete2.bit.bit2);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P36");
	json_set_val_str(jptr, "N", "CLOSE");
	json_set_val_str(jptr, "C", "036");
	snprintf(bt_buf, 16, "%d",reg_discrete[slave_addr].discrete2.bit.bit3);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P37");
	json_set_val_str(jptr, "N", "SESTA");
	json_set_val_str(jptr, "C", "037");
	temp=reg_in[slave_addr].REG0_SESTA/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P38");
	json_set_val_str(jptr, "N", "AE");
	json_set_val_str(jptr, "C", "038");
	temp=reg_in[slave_addr].REG0_AE/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P39");
	json_set_val_str(jptr, "N", "RAE");
	json_set_val_str(jptr, "C", "039");
	temp=reg_in[slave_addr].REG0_RAE/1000.0;
	snprintf(bt_buf, 16, "%.3f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P40");
	json_set_val_str(jptr, "N", "SLEAKAGE");
	json_set_val_str(jptr, "C", "040");
	temp=reg_hold[slave_addr].REG1_SLEAKAGE;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P41");
	json_set_val_str(jptr, "N", "SLEAKAGEP");
	json_set_val_str(jptr, "C", "041");
	temp=reg_hold[slave_addr].REG1_SLEAKAGEP;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P42");
	json_set_val_str(jptr, "N", "OL");
	json_set_val_str(jptr, "C", "042");
	temp=reg_hold[slave_addr].REG1_OL/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P43");
	json_set_val_str(jptr, "N", "OLP");
	json_set_val_str(jptr, "C", "043");
	temp=reg_hold[slave_addr].REG1_OLP/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P44");
	json_set_val_str(jptr, "N", "OP");
	json_set_val_str(jptr, "C", "044");
	snprintf(bt_buf, 16, "%d",reg_hold[slave_addr].REG1_OP);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P45");
	json_set_val_str(jptr, "N", "OPP");
	snprintf(bt_buf, 16, "%d",reg_hold[slave_addr].REG1_OPP);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P46");
	json_set_val_str(jptr, "N", "UVA");
	json_set_val_str(jptr, "C", "046");
	temp=reg_hold[slave_addr].REG1_UVA/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P47");
	json_set_val_str(jptr, "N", "UVAP");
	json_set_val_str(jptr, "C", "047");
	temp=reg_hold[slave_addr].REG1_UVAP/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P48");
	json_set_val_str(jptr, "N", "UVB");
	json_set_val_str(jptr, "C", "048");
	temp=reg_hold[slave_addr].REG1_UVB/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P49");
	json_set_val_str(jptr, "N", "UVBP");
	json_set_val_str(jptr, "C", "049");
	temp=reg_hold[slave_addr].REG1_UVBP/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////
	json_push_object(jptr, "P50");
	json_set_val_str(jptr, "N", "UVC");
	json_set_val_str(jptr, "C", "050");
	temp=reg_hold[slave_addr].REG1_UVC/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P51");
	json_set_val_str(jptr, "N", "UVCP");
	json_set_val_str(jptr, "C", "051");
	temp=reg_hold[slave_addr].REG1_UVCP/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P52");
	json_set_val_str(jptr, "N", "OVA");
	json_set_val_str(jptr, "C", "052");
	temp=reg_hold[slave_addr].REG1_OVA/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P53");
	json_set_val_str(jptr, "N", "OVAP");
	json_set_val_str(jptr, "C", "053");
	temp=reg_hold[slave_addr].REG1_OVAP/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);	
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P54");
	json_set_val_str(jptr, "N", "OVB");
	json_set_val_str(jptr, "C", "054");
	temp=reg_hold[slave_addr].REG1_OVB/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P55");
	json_set_val_str(jptr, "N", "OVBP");
	json_set_val_str(jptr, "C", "055");
	temp=reg_hold[slave_addr].REG1_OVBP/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);	
	///////////////////////////////////////////////////////	
	json_push_object(jptr, "P56");
	json_set_val_str(jptr, "N", "OVC");
	json_set_val_str(jptr, "C", "056");
	temp=reg_hold[slave_addr].REG1_OVC/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P57");
	json_set_val_str(jptr, "N", "OVCP");
	json_set_val_str(jptr, "C", "057");
	temp=reg_hold[slave_addr].REG1_OVCP/100.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);	
	///////////////////////////////////////////////////////				
	json_push_object(jptr, "P58");
	json_set_val_str(jptr, "N", "OCA");
	json_set_val_str(jptr, "C", "058");
	temp=reg_hold[slave_addr].REG1_OCA/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P59");
	json_set_val_str(jptr, "N", "OCAP");
	json_set_val_str(jptr, "C", "059");
	temp=reg_hold[slave_addr].REG1_OCAP/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);	
	///////////////////////////////////////////////////////				
	json_push_object(jptr, "P60");
	json_set_val_str(jptr, "N", "OCB");
	json_set_val_str(jptr, "C", "060");
	temp=reg_hold[slave_addr].REG1_OCB/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P61");
	json_set_val_str(jptr, "N", "OCBP");
	json_set_val_str(jptr, "C", "061");
	temp=reg_hold[slave_addr].REG1_OCBP/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);	
	///////////////////////////////////////////////////////				
	json_push_object(jptr, "P62");
	json_set_val_str(jptr, "N", "OCC");
	json_set_val_str(jptr, "C", "062");
	temp=reg_hold[slave_addr].REG1_OCC/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P63");
	json_set_val_str(jptr, "N", "OCCP");
	json_set_val_str(jptr, "C", "063");
	temp=reg_hold[slave_addr].REG1_OCCP/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);	
	///////////////////////////////////////////////////////				
	json_push_object(jptr, "P64");
	json_set_val_str(jptr, "N", "OTA");
	json_set_val_str(jptr, "C", "064");
	temp=reg_hold[slave_addr].REG1_OTA/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P65");
	json_set_val_str(jptr, "N", "OTAP");
	json_set_val_str(jptr, "C", "065");
	temp=reg_hold[slave_addr].REG1_OTAP/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////				
	json_push_object(jptr, "P66");
	json_set_val_str(jptr, "N", "OTB");
	json_set_val_str(jptr, "C", "066");
	temp=reg_hold[slave_addr].REG1_OTB/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P67");
	json_set_val_str(jptr, "N", "OTBP");
	json_set_val_str(jptr, "C", "067");
	temp=reg_hold[slave_addr].REG1_OTBP/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////				
	json_push_object(jptr, "P68");
	json_set_val_str(jptr, "N", "OTC");
	json_set_val_str(jptr, "C", "068");
	temp=reg_hold[slave_addr].REG1_OTC/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P69");
	json_set_val_str(jptr, "N", "OTCP");
	json_set_val_str(jptr, "C", "069");
	temp=reg_hold[slave_addr].REG1_OTCP/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////				
	json_push_object(jptr, "P70");
	json_set_val_str(jptr, "N", "OTN");
	json_set_val_str(jptr, "C", "070");
	temp=reg_hold[slave_addr].REG1_OTN/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);
	///////////////////////////////////////////////////////		
	json_push_object(jptr, "P71");
	json_set_val_str(jptr, "N", "OTNP");
	json_set_val_str(jptr, "C", "071");
	temp=reg_hold[slave_addr].REG1_OTNP/10.0;
	snprintf(bt_buf, 16, "%.2f",temp);
	json_set_val_str(jptr, "V", bt_buf);
	json_set_val_str(jptr, "alert", "NORMAL");
	json_pop_object(jptr);	
	///////////////////////////////////////////////////////
	
	json_pop_object(jptr);
	json_close_object(jptr);
	msg_len=jptr->free_ptr;
	iret=send_message_to_cloud(sys_psm.mqtt_publish_sensor_buf,msg_up_buf,msg_len);
	return iret;
}

void mqtt_control_thread(void* param)
{
	int retval;
	uint32_t min_cnt=0;
	int rc = 0;
	int i;
	uint8_t addr_tmp=0;
	
	//log_i(" mqtt_control_thread");
    NetworkInit(&g_mqtt_network, MQTT_SOCKET);
    retval = NetworkConnect(&g_mqtt_network, sys_psm.mqtt_server_ip, sys_psm.mqtt_server_port);
    if (retval != 1)
    {
        log_e("mqtt","Network connect failed");
		retval=0;
        while (1)
        {
			retval++;
			if(retval>30)
			{
				log_i("close 1");
				goto ext1;
			}
            os_thread_sleep(os_msec_to_ticks(2000));
        }
    }
    /* Initialize MQTT client */
    MQTTClientInit(&g_mqtt_client, &g_mqtt_network, DEFAULT_TIMEOUT, g_mqtt_send_buf, ETHERNET_BUF_MAX_SIZE, g_mqtt_recv_buf, ETHERNET_BUF_MAX_SIZE);
	
    /* Connect to the MQTT broker */
    g_mqtt_packet_connect_data.MQTTVersion = 3;
    g_mqtt_packet_connect_data.cleansession = 1;
    g_mqtt_packet_connect_data.willFlag = 0;
    g_mqtt_packet_connect_data.keepAliveInterval =0;// MQTT_KEEP_ALIVE;
    g_mqtt_packet_connect_data.clientID.cstring = sys_psm.device_id;
    g_mqtt_packet_connect_data.username.cstring = sys_psm.mqtt_user;
    g_mqtt_packet_connect_data.password.cstring = sys_psm.mqtt_pass;
    retval = MQTTConnect(&g_mqtt_client, &g_mqtt_packet_connect_data);
    if (retval < 0)
    {
        log_e("mqtt","MQTT connect failed : %d\n", retval);
		retval=0;
        while (1)
        {
			retval++;
			if(retval>30)
			{
				goto ext;
			}
            os_thread_sleep(os_msec_to_ticks(2000));
//			retval = MQTTConnect(&g_mqtt_client, &g_mqtt_packet_connect_data);
//			if(retval==SOCK_OK)
//			{
//				break;
//			}
        }
    }

    log_i(" MQTT connected");
    /* Subscribe */
    retval = MQTTSubscribe(&g_mqtt_client, sys_psm.mqtt_subscribe_command_buf, QOS0, mqtt_subscribe_command);

    if (retval < 0)
    {
        printf(" Subscribe failed : %d\n", retval);

        while (1)
        {
           os_thread_sleep(os_msec_to_ticks(1000));
        }
    }
	
//	retval = MQTTSubscribe(&g_mqtt_client, sys_psm.mqtt_subscribe_alarm_buf, QOS0, mqtt_subscribe_alarm);
//    if (retval < 0)
//    {
//        printf(" Subscribe failed : %d\n", retval);

//        while (1)
//        {
//           os_thread_sleep(os_msec_to_ticks(1000));
//        }
//    }
	mqtt_ok=1;
	device_data.mqtt_connect_flag = 1;
	min_cnt=5*60*sys_psm.mqtt_updata_time;
    while (1)
    {
		min_cnt++;
		//log_i("min_cnt: %d time:%d",min_cnt,5*60*sys_psm.mqtt_updata_time);
		if(min_cnt>=5*60*sys_psm.mqtt_updata_time)
		{
			min_cnt=0;
			if(device_data.camera_send_to_tcp_flag==0)
			{
				//////////////////////网关板上报///////////////////////////
				report_gate_data();
				/////////////////////电源板上报/////////////////////
				if(sys_psm.slave_ok==1)
				{
					os_thread_sleep(os_msec_to_ticks(100));
					
					for (i = 0; i < device_data.slave_device_max; i++)
					{
						if(device_data.own_slave_device[i].device_type==POWER_MONITOR_BOARD)
						{
							report_power_data(device_data.own_slave_device[i].slave_addr);
							os_thread_sleep(os_msec_to_ticks(100));						
						}
					}				
				}
				os_thread_sleep(os_msec_to_ticks(100));
				////////////////////重合闸上报//////////////////////////////////////
				if(device_data.autoeclosing_enable_flag==1)
				{
					report_autoeclosing_data();
				}
				//////////////////////断路器上报/////////////////////////////////////
				if(device_data.circuit_breaker_enable_flag==1)
				{
					for (i = 0; i < CIRCUIT_BREAKER_MAX_NUM; i++)
					{
						if(device_data.cb_slave_addr[i]!=0xff)
						{
							addr_tmp=device_data.cb_slave_addr[i];
							if((reg_in[addr_tmp].REG0_TYPE==CB_2P_16A)|(reg_in[addr_tmp].REG0_TYPE==CB_2P_32A)|(reg_in[addr_tmp].REG0_TYPE==CB_2P_63A))
							{
								report_circuit_breaker_2p_data(addr_tmp);
							}
							else if((reg_in[addr_tmp].REG0_TYPE==CB_4P_16A)|(reg_in[addr_tmp].REG0_TYPE==CB_4P_32A)|(reg_in[addr_tmp].REG0_TYPE==CB_4P_63A))
							{
								report_circuit_breaker_4p_data(addr_tmp);
							}					
						}				
					}			
				}
				/////////////////////////////////////////////////////////////////////			
			}
		}
		if(device_data.camera_send_to_tcp_flag==0)
		{
			if(device_data.set_flag==1)
			{
				device_data.set_flag=0;
				report_gate_list_ack();
			}
			if((set_ok==1)||(set_ok==2))
			{
				report_gate_set_ack(set_ok);
				set_ok=0;
			}		
			if(device_data.alarm_change_flag==1)
			{
				device_data.alarm_change_flag=0;
				report_gate_alarm();
			}		
		}

        os_thread_sleep(os_msec_to_ticks(200));
		//////////////////////////////////////////////////////////////////
		if(device_data.mqtt_start_flag==2)
		{
			break;
		}
		//////////////////////////////////////////////////////////////////
    }
	MQTTUnsubscribe(&g_mqtt_client, sys_psm.mqtt_subscribe_command_buf);
	MQTTYield(&g_mqtt_client, 200);
	ext:
	MQTTDisconnect(&g_mqtt_client);
	ext1:
	log_i("close mqtt");
	device_data.mqtt_start_flag=0;
	device_data.mqtt_connect_flag=0;
	os_thread_sleep(os_msec_to_ticks(10));
	vTaskDelete(yield_thread);
	yield_thread= NULL;
	vTaskDelete(mqtt_thread);
	mqtt_thread= NULL;
}

void yield_task(void *argument)
{
    int retval;

    while (1)
    {
        if (device_data.mqtt_connect_flag == 1)
        {
            if ((retval = MQTTYield(&g_mqtt_client, g_mqtt_packet_connect_data.keepAliveInterval)) < 0)
            {
               // printf(" Yield error : %d\n", retval);
                while (1)
                {
                    os_thread_sleep(os_msec_to_ticks(1000));
                }
            }
        }
        os_thread_sleep(os_msec_to_ticks(100));
    }
}

void mqtt_process_init(void)
{
   	int ret=SYS_FAIL;
	ret = os_thread_create(&mqtt_thread, //任务控制块指针
							"mqtt_thread",//任务名字
							mqtt_control_thread, //任务入口函数
							NULL,//任务入口函数参数
							&mqtt_stack,//任务栈大小
							OS_PRIO_12);	//任务的优先级							
   if(ret==SYS_FAIL)
   {
		log_e("mqtt","mqtt thread create error!");
   }
	ret = os_thread_create(&yield_thread, //任务控制块指针
							"YIEDL_Task",//任务名字
							yield_task, //任务入口函数
							NULL,//任务入口函数参数
							&yield_stack,//任务栈大小
							OS_PRIO_15);	//任务的优先级							
   if(ret==SYS_FAIL)
   {
		log_e("mqtt","yield_task thread create error!");
   }
}