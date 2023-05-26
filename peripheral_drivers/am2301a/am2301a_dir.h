#ifndef __AM2301A_DIR__
#define __AM2301A_DIR__

#include "hc32_ll.h"

uint8_t am2301a_init(void);
void am2301a_sda_out(void);
void am2301a_sda_in(void);
void am2301a_rst(void);
uint8_t am2301a_Check(void);
uint8_t am2301a_read_bit(void);
uint8_t am2301a_read_byte(void);
uint8_t am2301a_read_data(float *temperature,float *humidity);

#endif
