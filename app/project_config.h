#ifndef _PROJECT_CONFIG_H_
#define _PROJECT_CONFIG_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "hc32f4a0.h"
#include "hc32_ll_utility.h"
#include "hc32_ll.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "FreeRTOSConfig.h"
#include "portmacro.h"


#define PRODUCT_NAME "GATE"

#define MCU_SW_VERSION_MAJOR        2
#define MCU_SW_VERSION_MINOR        0
#define MCU_SW_VERSION_BUILD        7


#define AUTOCLOSING_ADDR  0x01

#define SLAVE_MAX_NUM  16
#define CIRCUIT_BREAKER_MAX_NUM 32
/////////////////////////////////////////////////////////////
#define DBG_ENABLE_ASSERTS 1
#define DBG_ENABLE_LOG 1
#define DBG_ENABLE_WQ 1
#define DBG_ENABLE_FLASH 1
#define DBG_ENABLE_PART 1
#define DBG_ENABLE_PSM 1


#define CONFIG_JSMN_PARENT_LINKS 1
#define CONFIG_JSMN_STRICT 1
#define CONFIG_JSMN_SHORT_TOKENS 1

#define CONFIG_HW_RTC 1
/////////////////////////////////////////////////////////////
#define FULL_VAR_NAME_SIZE 64
#define MWM_VAL_SIZE (128+1)
#define MWM_MOD_SIZE (32+1)

#define MWM_VALUE_BIG_SIZE (128+1)
#define MWM_VALUE_MED_SIZE (64+1)
#define MWM_VALUE_SMALL_SIZE (32+1)
/////////////////////////////////////////////////////////////

typedef enum
{
	SENSOR_NO_ERROR=0x00,
	SMOKE_SENSOR_ERROR=0x0001,		//烟感异常
	WATER_SENSOR_ERROR=0x0002,		//水浸异常
	LOCK_SENSOR_ERROR=0x0004,		//开箱异常
	SPD_SENSOR_ERROR=0x0008,		//SPD异常
	CAMERA_SENSOR_ERROR=0x0008,		//摄像头异常
}sys_device_error;

//重合闸，断路器功能公用RS485,重合闸，断路器公用时没调试
typedef enum
{
	MODE_NO_NB_AC_CB2_CB4=0x00,	//无NB，无重合闸，无断路器功能
	MODE_NO_AC_CB2_CB4=0x01,	//有NB，无重合闸，无断路器功能
	MODE_NO_NB_CB2_CB4=0x02,	//无NB，有重合闸，无断路器功能
	MODE_NO_CB2_CB4=0x03,		//有NB，有重合闸，无断路器功能
	MODE_NO_NB_AC_CB4=0x04,		//无NB，无重合闸，有断路器功能
	MODE_NO_AC_CB4=0x05,		//有NB，无重合闸，有断路器功能
	MODE_NO_NB_CB4=0x06,		//无NB，有重合闸，有断路器功能
	MODE_NO_CB4=0x07,			//有NB，有重合闸，有断路器功能
}sys_mode_type;

typedef struct
{
	uint8_t slave_addr;
	uint8_t device_type;
	char device_name[32+1];		
} sys_slave_type;

typedef struct
{
	double n_latitude;
	double w_longitude;
	uint32_t error_status;	//错误
	uint32_t error_status_old;
	uint32_t error_status_mqtt;
	uint32_t camera_picLen;   //数据长度
	
	float temp_value;
	float humi_value;	
	
	float electric_value;	
	float voltage_value;
	
	float  fan_open_temp; 		
	float  fan_close_temp;
	
	uint16_t cntM;
	uint16_t camera_buf_cnt;
	uint16_t lastBytes;	
	
	uint8_t factory_flag;
	uint8_t rst_long_flag;
	uint8_t run_mode_id;
	uint8_t lock_flag;
	uint8_t last_flag;
	uint8_t camera_send_flag;
	uint8_t camera_buf[4096];
	uint8_t camera_flag;
	uint8_t camera_send_to_tcp_flag;
	uint8_t nbiot_mqtt_close_flag;
	//////////////////////////
	uint8_t nb_rest_cnt;
	uint8_t nb_start_flag;
	uint8_t display_num;
	uint8_t set_timer_flag;
	stc_rtc_date_t c_date;
	stc_rtc_time_t c_time;
	////////////////////////////////////////////////
	uint8_t smoke_in_error_flag;
	uint8_t water_in_error_flag;
	uint8_t lock_in_error_flag;
	uint8_t spd_in_error_flag;
	uint8_t camera_error_flag;
	////////////////////////////////////////////////
	uint8_t alarm_change_flag;
	uint8_t buzzer_change_flag;
	uint8_t buzzer_enable_flag;
	
	uint8_t power_flag;
	uint8_t mqtt_connect_flag;
	uint8_t mqtt_start_flag;
	uint8_t phy_check_flag;
	////////////////////////////////////////////////
	uint8_t fan_temp_flag;
	uint8_t hot_humi_flag;
 	
	uint8_t  hot_fan_open_humi; 		
	uint8_t  hot_fan_close_humi; 
	uint8_t led_flag;
	uint8_t led_data_flag;

	uint8_t set_flag;
	uint8_t reclose_flag;
	uint8_t reclose_data_flag;
	uint8_t cbclose_flag;
	uint8_t cbclose_data_flag;	
	uint8_t cbclose_data_addr;	
	uint8_t cb_hold_flag;
	////////////////////////////////////////////////
	uint8_t autoeclosing_on_off_code;
	uint8_t autoeclosing_on_off_flag;
	uint8_t autoeclosing_enable_flag;
	uint8_t circuit_breaker_enable_flag;
	////////////////////////////////////////////////
	uint8_t slave_device_have_ack_flag;//查询有从设备反馈标志	
	uint8_t query_slave_cnt;//查询从设备ID计数器	
	uint8_t query_slave_times_cnt;//查询从设备循环次数计数器
	uint8_t save_slave_device_flag;	//保存过从设备标志
	uint8_t query_slave_finish_flag;//此次查询设备结束标志
	////////////////////////////////////////////////
	uint8_t cb_slave_device_have_ack_flag;//查询有从设备反馈标志	
	uint8_t cb_query_slave_cnt;//查询从设备ID计数器	
	uint8_t cb_slave_addr[CIRCUIT_BREAKER_MAX_NUM];
	uint8_t cb_slave_addr_cnt;
	//////////////////////////////////////////////////////////////////
	uint8_t set_scan_flag;
	sys_slave_type own_slave_device[SLAVE_MAX_NUM];//
	uint16_t power_monitoring_relay_data[SLAVE_MAX_NUM];
	uint8_t read_slave_device_cnt;	//从设备计数器
	uint8_t slave_device_max;	//从设备最大数量	
} sys_type;


extern sys_type device_data;

#endif
