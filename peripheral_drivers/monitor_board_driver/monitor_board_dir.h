#ifndef _MONITOR_BOARD_DIR_H_
#define _MONITOR_BOARD_DIR_H_

#include "hc32_ll.h"

void monitor_usart_init(func_ptr_t func_rx_err_callback,func_ptr_t func_rx_full_callback);
#endif
