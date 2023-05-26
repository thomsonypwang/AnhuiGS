#ifndef _SENSOR_IN_DIR_H_
#define _SENSOR_IN_DIR_H_


#include "hc32_ll.h"

void sensor_in_io_init(void);
uint8_t read_smoke_sensor(void);
uint8_t read_water_sensor(void);
uint8_t read_lock_sensor(void);
uint8_t read_spd_sensor(void);

#endif
