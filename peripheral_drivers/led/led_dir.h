#ifndef _LED_DIR_H_
#define _LED_DIR_H_

#include "hc32_ll.h"

void led_arm_on(void);
void led_arm_off(void);
void led_run_on(void);
void led_run_off(void);
void led_net_on(void);
void led_net_off(void);
void led_io_init(void);

#endif
