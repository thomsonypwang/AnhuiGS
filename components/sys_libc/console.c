#include <string.h>

#include "sys_stdio.h"
#include "hc32_ll.h"


stdio_funcs_t serial_console_funcs;

void serial_console_init(void)
{
	c_stdio_funcs = &serial_console_funcs;

}

//int serial_console_get_portid(int *port_id)
//{
//	*port_id = uart_drv_get_portid(mdev_uart_);
//	return 0;
//}

int serial_console_printf(char *str)
{
	int len;
	int32_t i = -1;
	
	len = strlen(str);
   
    if (NULL != str) 
	{
        for (i = 0; i < len; i++) 
		{
            if (LL_OK != DDL_ConsoleOutputChar(str[i])) 
			{
                break;
            }
        }
    }

    return i ? i : -1;	
	
//	uart_drv_write(mdev_uart_, (uint8_t *)str, len);
	return len;
}

int serial_console_flush()
{
	//uart_drv_tx_flush(mdev_uart_);
	return 0;
}

int serial_console_getchar(uint8_t *inbyte_p)
{
	return 0;//uart_drv_read(mdev_uart_, inbyte_p, 1);
}

int serial_console_putchar(char *inbyte_p)
{

	return DDL_ConsoleOutputChar(inbyte_p[0]);//uart_drv_write(mdev_uart_, (uint8_t *)inbyte_p, 1);
}

stdio_funcs_t serial_console_funcs = { 
	serial_console_printf, 
	serial_console_flush,
	serial_console_getchar,
	serial_console_putchar,
};
