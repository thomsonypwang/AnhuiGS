#ifndef _MONITOR_BOARD_HANDLE_H_
#define _MONITOR_BOARD_HANDLE_H_

#include "project_config.h"

#define MCU_HEAD1_CODE 0xf5
#define MCU_HEAD2_CODE 0x5f

#define MCU_END1_CODE 0x0d
#define MCU_END2_CODE 0x0A

typedef struct
{
    uint8_t mcu_frame_header1;
    uint8_t mcu_frame_header2;
	uint8_t mcu_id;
	uint16_t rs485_addr_buf;
	uint16_t mcu_data_len;
    uint8_t mcu_function_cmd;
	uint16_t mcu_checksum;
    uint8_t mcu_frame_end1;
    uint8_t mcu_frame_end2;    
} rs485_info_conf;

typedef struct
{
	uint16_t relay_status;
    uint16_t ac_voltage_value;
	uint16_t ac_current_value;	
    uint16_t dc1_voltage_value;
	uint16_t dc1_current_value;		
    uint16_t dc2_voltage_value;
	uint16_t dc2_current_value;	
	uint16_t dc3_voltage_value;
	uint16_t dc3_current_value;	
	uint16_t dc4_voltage_value;
	uint16_t dc4_current_value;	
	uint16_t dc5_voltage_value;
	uint16_t dc5_current_value;	
	uint16_t dc6_voltage_value;
	uint16_t dc6_current_value;	
	uint16_t dc7_voltage_value;
	uint16_t dc7_current_value;	
	
	//uint8_t device_type;
	uint16_t device_sw_ver;	
	uint16_t device_hw_ver;
	uint16_t device_rs485_id;
	//char device_name[32+1];
	
} power_monitoring_board_data;


typedef enum
{
	AC_RELAY=0x0001,
	DC1_RELAY=0x0002,
	DC2_RELAY=0x0004,
	DC3_RELAY=0x0008,
	DC4_RELAY=0x0010,
	DC5_RELAY=0x0020,
	DC6_RELAY=0x0040,
	DC7_RELAY=0x0080,
	FAN_RELAY=0x0100,
	LIGHTING_RELAY=0x0200,
}sys_relay_addr;


typedef enum
{
	MCU_FUN_SEARCH=0x01,
	MCU_FUN_READ=0x02,	
	MCU_FUN_WRITE=0x03,
	
	MCU_FUN_ASK_SEARCH=0x81,
	MCU_FUN_ASK_READ=0x82,	
	MCU_FUN_ASK_WRITE=0x83,
} rs485_function_type;

typedef enum
{
	GATEWAY_BOARD=0x01,
	POWER_MONITOR_BOARD=0x02,
	DATA_COLLECTION_BOARD=0x03,
} slave_device_type;

typedef enum
{
	ADDR_RELAY1_STATUS=0x0000,
	ADDR_RELAY2_STATUS=0x0001,
	
	ADDR_AC1_VOLTAGE=0x0002,
	ADDR_AC1_CURRENT=0x0003,
	ADDR_AC2_VOLTAGE=0x0004,
	ADDR_AC2_CURRENT=0x0005,
	ADDR_AC3_VOLTAGE=0x0006,
	ADDR_AC3_CURRENT=0x0007,
	
	ADDR_DC1_VOLTAGE=0x0008,
	ADDR_DC1_CURRENT=0x0009,
	ADDR_DC2_VOLTAGE=0x000a,
	ADDR_DC2_CURRENT=0x000b,
	ADDR_DC3_VOLTAGE=0x000c,
	ADDR_DC3_CURRENT=0x000d,
	ADDR_DC4_VOLTAGE=0x000e,
	ADDR_DC4_CURRENT=0x000f,
	ADDR_DC5_VOLTAGE=0x0010,
	ADDR_DC5_CURRENT=0x0011,
	ADDR_DC6_VOLTAGE=0x0012,
	ADDR_DC6_CURRENT=0x0013,
	ADDR_DC7_VOLTAGE=0x0014,
	ADDR_DC7_CURRENT=0x0015,
	ADDR_DC8_VOLTAGE=0x0016,
	ADDR_DC8_CURRENT=0x0017,
	ADDR_DC9_VOLTAGE=0x0018,
	ADDR_DC9_CURRENT=0x0019,
	ADDR_DC10_VOLTAGE=0x001a,
	ADDR_DC10_CURRENT=0x001b,

	ADDR_TEMP1=0x001c,
	ADDR_HUMI1=0x001d,
	ADDR_TEMP2=0x001e,
	ADDR_HUMI2=0x001f,	
	
	ADDR_DRIVER_TYPE=0xff00,	
	ADDR_SOFTWARE_VER=0xff01,
	ADDR_HARDWARE_VER=0xff02,		
	ADDR_ERR_CODE=0xff03,
	ADDR_DRIVER_NAME=0xff04,
	ADDR_RS485_ID=0xff05,
} rs485_addr_type;

extern power_monitoring_board_data  power_board_data[SLAVE_MAX_NUM];

void modbus_process_init(void);

uint16_t read_power_board_relay_status(uint8_t num);
float read_power_board_ac_voltage_value(uint8_t num);
float read_power_board_ac_current_value(uint8_t num);
float read_power_board_dc1_voltage_value(uint8_t num);
float read_power_board_dc1_current_value(uint8_t num);
float read_power_board_dc2_voltage_value(uint8_t num);
float read_power_board_dc2_current_value(uint8_t num);
float read_power_board_dc3_voltage_value(uint8_t num);
float read_power_board_dc3_current_value(uint8_t num);
float read_power_board_dc4_voltage_value(uint8_t num);
float read_power_board_dc4_current_value(uint8_t num);
float read_power_board_dc5_voltage_value(uint8_t num);
float read_power_board_dc5_current_value(uint8_t num);
float read_power_board_dc6_voltage_value(uint8_t num);
float read_power_board_dc6_current_value(uint8_t num);
float read_power_board_dc7_voltage_value(uint8_t num);
float read_power_board_dc7_current_value(uint8_t num);
#endif
