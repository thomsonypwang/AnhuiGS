#ifndef _WDT_DRV_H_
#define _WDT_DRV_H_

#include "hc32_ll.h"

int wdt_drv_set_timeout(uint8_t index);
void wdt_drv_strobe(void);
void wdt_drv_start(void);

#endif 

