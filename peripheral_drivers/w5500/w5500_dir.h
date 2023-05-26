#ifndef  _W5500_DIR_H_
#define  _W5500_DIR_H_

#include "hc32_ll.h"
#include "wizchip_conf.h"

void w5500_io_init(void);
void w5500_spi_init(void);
void w5500_reset(void);
void w5500_register_function(void);//注缠函数
void w5500_parameters_configuration(void);//初始化芯片参数

#endif  
