#ifndef _AUTOECLOSING_RS485_HANDLE_H_
#define _AUTOECLOSING_RS485_HANDLE_H_

#include "hc32_ll.h"
#include "project_config.h"

typedef struct
{
    uint16_t voltage_value;
	uint16_t current_value;	
	uint16_t creepage_current_value;
	uint16_t reclosing_times;
    uint16_t over_voltage_value;
    uint16_t low_voltage_value;
	uint16_t device_status;
	uint16_t do_status;
	uint16_t fault_opening_times;
} autoeclosing_register_data;

void autoeclosing_process_init(void);

uint16_t read_autoeclosing_voltage_value(void);
float read_autoeclosing_current_value(void);
uint16_t read_autoeclosing_creepage_current_value(void);
uint16_t read_autoeclosing_reclosing_times(void);
uint16_t read_autoeclosing_over_voltage_value(void);
uint16_t read_autoeclosing_low_voltage_value(void);
uint16_t read_autoeclosing_device_status(void);
uint16_t read_autoeclosing_fault_opening_times(void);
uint16_t read_autoeclosing_do_status(void);

#endif
