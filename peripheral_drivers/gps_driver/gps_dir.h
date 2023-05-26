#ifndef _GPS_DIR_H_
#define _GPS_DIR_H_

#include "hc32_ll.h"

void gps_usart_init(func_ptr_t func_rx_err_callback,func_ptr_t func_rx_full_callback);
#endif
