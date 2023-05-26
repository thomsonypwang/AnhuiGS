#ifndef _PROJECT_PSM_CONTROL_H_
#define _PROJECT_PSM_CONTROL_H_

#include "project_config.h"
#include "wizchip_conf.h"

#define DEVICE_SYSTEM					"sys"
	#define DEVICE_SYSTEM_HIGH_TEMP		"h_temp"
	#define DEVICE_SYSTEM_LOW_TEMP		"l_temp"
	#define DEVICE_SYSTEM_HIGH_HUMI		"h_humi"
	#define DEVICE_SYSTEM_LOW_HUMI		"l_humi"
	#define DEVICE_SYSTEM_DEVICE_ID		"device_id"
	#define DEVICE_SYSTEM_RS485_ID		"rs485_id"
	#define DEVICE_SYSTEM_QUERY_TIMER	"query_time"//秒

#define ETH_SYSTEM 						"eth"
	#define ETH_SYSTEM_MAC 				"mac"
	#define ETH_SYSTEM_IP 				"ip"
	#define ETH_SYSTEM_IP_PORT 			"port"
	#define ETH_SYSTEM_IP_MASK 			"maskr"
	#define ETH_SYSTEM_GATEWAY 			"gw"
	#define ETH_SYSTEM_DNS 				"dns"
	#define ETH_SYSTEM_DHCP 			"dhcp"

#define MQTT_SYSTEM 					"mqtt"
	#define MQTT_SYSTEM_USER 			"user"
	#define MQTT_SYSTEM_PASS 			"pass"
	#define MQTT_SYSTEM_IP 				"mip"
	#define MQTT_SYSTEM_PORT 			"mport"
	#define MQTT_SYSTEM_SERVER1_IP 		"ip_s1"
	#define MQTT_SYSTEM_SERVER1_PORT 	"port_s1"	
	#define MQTT_SYSTEM_SERVER2_IP 		"ip_s2"
	#define MQTT_SYSTEM_SERVER2_PORT 	"port_s2"	
	#define MQTT_SYSTEM_SERVER3_IP 		"ip_s3"
	#define MQTT_SYSTEM_SERVER3_PORT 	"port_s3"		
	#define MQTT_SYSTEM_REPORT_TIMER	"updata_time"//分
	
	#define MQTT_SYSTEM_PUB_SENSOR		"mqtt_pub_sensor"
	#define MQTT_SYSTEM_PUB_ALARM		"mqtt_pub_alarm"
	#define MQTT_SYSTEM_PUB_ACK			"mqtt_sub_alarm"
	#define MQTT_SYSTEM_PUB_BACK1		"mqtt_pub_back1"
	#define MQTT_SYSTEM_SUB_COMMAND		"mqtt_sub_command"	
	
#define SLAVE_SYSTEM					"slave"	
	#define SLAVE_SYSTEM_SLAVE_OK		"ok"
	#define SLAVE_SYSTEM_SLAVE_ADDR		"addr"
	#define SLAVE_SYSTEM_SLAVE_TYPE		"type"
	#define SLAVE_SYSTEM_SLAVE_NAME		"name"

//////////////////////////////////////////////////////////////////////////
#define DAT_H_TEMP 			25 	//50度
#define DAT_L_TEMP 			15	//0度
#define DAT_H_HUMI 			85	//80湿度
#define DAT_L_HUMI 			60	//80湿度
#define DAT_RS485_ID 		1	
#define DAT_QUERY_TIMER 	5	
/////////////////////////////////////////////////////////////////////////
#define DAT_ETH_IP		"192.168.0.100"
#define DAT_ETH_SN		"255.255.255.0"
#define DAT_ETH_GW		"192.168.0.1"
#define DAT_ETH_DNS		"202.96.128.166"
#define DAT_ETH_PORT	6000
#define DAT_ETH_DHCP	NETINFO_STATIC
/////////////////////////////////////////////////////////////////////////
#define DAT_MQTT_USER 				"wzkean"
#define DAT_MQTT_PASS 				"wzkj@0627"
#define DAT_MQTT_IP 				"120.76.135.228"
#define DAT_MQTT_PORT 				1883
#define DAT_MQTT_SERVER1_IP 		"120.76.135.228"
#define DAT_MQTT_SERVER1_PORT 		61618
#define DAT_MQTT_SERVER2_IP 		"120.76.135.228"
#define DAT_MQTT_SERVER2_PORT 		61619
#define DAT_MQTT_SERVER3_IP 		"120.76.135.228"
#define DAT_MQTT_SERVER3_PORT 		61620
#define DAT_MQTT_UPDATA_TIMER 		1

#define DAT_MQTT_PUBLISH_SENSOR  	"HWSSH/PRD/HWSSH/SSTSP/MES-EM/SENSOR/SAMPLE/RPT"	//推送主题
#define DAT_MQTT_PUBLISH_ALARM  	"HWSSH/PRD/HWSSH/SSTSP/MES-EM/SENSOR/ALARM/RPT"		//推送报警主题
#define DAT_MQTT_PUBLISH_ACK 		"HWSSH/PRD/HWSSH/SSTSP/MES-EM/SENSOR/SETALM/RPT"	//推送回复设置

#define DAT_MQTT_PUBLISH_BACK1  	"HWSSH/PRD/HWSSH/MES-EM/SSTSP/CAMERA/RPT"		//主题推送保留1主题
#define DAT_MQTT_SUBSCRIBE_COMMAND 	"HWSSH/PRD/HWSSH/MES-EM/SSTSP/COMMAND/RPT"		//订阅主题

//////////////////////////////////////////////////////////////////////////
#define DAT_SLAVE_OK 		0
#define DAT_SLAVE_ADDR 		255	
#define DAT_SLAVE_TYPE 		255
#define DAT_SLAVE_NAME 		"nc"
//////////////////////////////////////////////////////////////////////////

#define WRITE_PSM    			0
#define READ_PSM   				1
#define WRITE_PSM_DEFULT   		2


typedef struct
{
	uint16_t query_time;
	uint8_t rs485_id;
		
	uint8_t high_temp_data;
	uint8_t low_temp_data;
	uint8_t high_humi_data;	
	uint8_t low_humi_data;
	char device_id[32+1];
	////////////////////////////////////////
	wiz_NetInfo local;
	uint16_t local_port;
	////////////////////////////////////////	
	char mqtt_user[MWM_VALUE_MED_SIZE];
	char mqtt_pass[MWM_VALUE_MED_SIZE];
	uint8_t  mqtt_server_ip[4];
	uint16_t mqtt_server_port;
	uint8_t  server1_ip[4];
	uint16_t server1_port;
	uint8_t  server2_ip[4];
	uint16_t server2_port;
	uint8_t  server3_ip[4];
	uint16_t server3_port;
	uint8_t  mqtt_updata_time;
	////////////////////////////////////////
	char mqtt_publish_sensor_buf[MWM_VALUE_MED_SIZE];
	char mqtt_publish_alarm_buf[MWM_VALUE_MED_SIZE];
	char mqtt_publish_ack_buf[MWM_VALUE_MED_SIZE];
	char mqtt_publish_back1_buf[MWM_VALUE_MED_SIZE];
	char mqtt_subscribe_command_buf[MWM_VALUE_MED_SIZE];	
	////////////////////////////////////////	
	uint8_t slave_ok;
	sys_slave_type slave_device[SLAVE_MAX_NUM];
} sys_psm_type;

extern sys_psm_type sys_psm;

int psm_control_init(void);
int psm_control_cleanup(void);
void psm_erase_control(void);
int get_psm_conf(const char *mod, const char *var, char *val, const int len);
int set_psm_conf(const char *mod, const char *var, const char *val);
void add_psm_entry_str(const char *module, const char *variable, const char *default_val);
int get_psm_conf_int(const char *mod, const char *var, int *val);
int set_psm_conf_int(const char *mod, const char *var, int value);
void add_psm_entry_int(const char *module, const char *variable, int default_val);
void conf_default_psm(void);

//////////////////////////////////////////////////////////////////////
void read_write_high_temp_flash(uint8_t flag);
void read_write_low_temp_flash(uint8_t flag);
void read_write_high_humi_flash(uint8_t flag);
void read_write_low_humi_flash(uint8_t flag);
void read_write_device_id_flash(uint8_t flag);
void read_write_rs485_id_flash(uint8_t flag);
void read_write_query_time_flash(uint8_t flag);
/////////////////////////////////////////////////////////////////////
void read_write_local_ip_flash(uint8_t flag);
void read_write_local_mask_flash(uint8_t flag);
void read_write_local_gateway_flash(uint8_t flag);
void read_write_local_dns_flash(uint8_t flag);
void read_write_local_mac_flash(uint8_t flag);
void read_write_local_port_flash(uint8_t flag);
void read_write_local_dhcp_flash(uint8_t flag);
/////////////////////////////////////////////////////////////////////
void read_write_mqtt_user_flash(uint8_t flag);
void read_write_mqtt_pass_flash(uint8_t flag);
void read_write_mqtt_server_ip_flash(uint8_t flag);
void read_write_mqtt_server_port_flash(uint8_t flag);
void read_write_mqtt_server1_ip_flash(uint8_t flag);
void read_write_mqtt_server1_port_flash(uint8_t flag);
void read_write_mqtt_server2_ip_flash(uint8_t flag);
void read_write_mqtt_server2_port_flash(uint8_t flag);
void read_write_mqtt_server3_ip_flash(uint8_t flag);
void read_write_mqtt_server3_port_flash(uint8_t flag);
void read_write_mqtt_report_timer_flash(uint8_t flag);
/////////////////////////////////////////////////////////////////////
void read_write_mqtt_pub_sensor_flash(uint8_t flag);
void read_write_mqtt_pub_alarm_flash(uint8_t flag);
void read_write_mqtt_sub_command_flash(uint8_t flag);
void read_write_mqtt_sub_alarm_flash(uint8_t flag);
void read_write_mqtt_pub_back1_flash(uint8_t flag);
////////////////////////////////////////////////////////////////////////
void read_write_slave_information_flash(uint8_t flag,uint8_t num);
void read_write_slave_ok_flash(uint8_t flag);

#endif
