#ifndef _AUTOECLOSING_RS485_DIR_H_
#define _AUTOECLOSING_RS485_DIR_H_

#include "hc32_ll.h"
#include "project_config.h"

void autoeclosing_usart_init(func_ptr_t func_rx_err_callback,func_ptr_t func_rx_full_callback,func_ptr_t func_rx_timeout_callback);

#endif
