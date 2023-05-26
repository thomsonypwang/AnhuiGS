#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "sys_os.h"
#include "sys_assert.h"
#include "sys_stdio.h"
#include "project_pin_use_config.h"


static char stdio_msg_buf[MAX_MSG_LEN];

stdio_funcs_t empty_funcs = {NULL, NULL, NULL, NULL};
stdio_funcs_t *c_stdio_funcs = &empty_funcs;
static os_mutex_t wmstdio_mutex;
void serial_console_init(void);

int sys_vprintf(const char *format, va_list ap)
{
	if (!c_stdio_funcs->sf_printf) 
	{
		/* The stdio facility is not available */
		return 0;
	}

	os_mutex_get(&wmstdio_mutex, OS_WAIT_FOREVER);
	vsnprintf(stdio_msg_buf, MAX_MSG_LEN, &format[0], ap);
	int ret = c_stdio_funcs->sf_printf(stdio_msg_buf);
	os_mutex_put(&wmstdio_mutex);
	return ret;
}

/**
 * Format the string printf style and send it to the output port
 * 
 * @param format 
 * 
 * @return int
 */
int sys_printf(const char *format, ...)
{
	va_list args;
	/* Format the string */
	va_start(args, format);
	uint32_t ret = sys_vprintf(format, args);
	va_end(args);
	return ret;
}

int __wrap_printf(const char *format, ...)
{
	va_list args;
	/* Format the string */
	va_start(args, format);
	uint32_t ret = sys_vprintf(format, args);
	va_end(args);
	return ret;
}

/**
 * char_printf should be used to print debug messages character by character
 *
 * @param ch
 *
 * @return int
 */
int sys_stdio_putchar(char *ch_)
{
	uint32_t ret;
	os_mutex_get(&wmstdio_mutex, OS_WAIT_FOREVER);
	ret = c_stdio_funcs->sf_putchar(ch_);
	os_mutex_put(&wmstdio_mutex);
	return ret;
}

/**
 * Flush the send buffer to the stdio port
 * 
 * @return uint32_t
 */
uint32_t sys_stdio_flush(void)
{
    if (! c_stdio_funcs->sf_flush)
    {
        /* The stdio facility is not available */
        return 1;
    }
    return c_stdio_funcs->sf_flush();
}

/**
 * Read one byte
 * 
 * @param pu8c 
 * 
 * @return uint8_t
 */
uint32_t sys_stdio_getchar(uint8_t *inbyte_p)
{
    if (! c_stdio_funcs->sf_getchar)
    {
        /* The stdio facility is not available */
        return 0;
    }
    return c_stdio_funcs->sf_getchar(inbyte_p);
}

int32_t dbg_init(void *vpDevice, uint32_t u32Baudrate)
{
    uint32_t u32Div;
    float32_t f32Error;
    stc_usart_uart_init_t stcUartInit;
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    (void)vpDevice;

    if (0UL != u32Baudrate) 
	{
        /* Set TX port function */
        GPIO_SetFunc(DBG_TX_PORT, DBG_TX_PIN, DBG_TX_FUNC);
		GPIO_SetFunc(DBG_RX_PORT, DBG_RX_PIN, DBG_RX_FUNC);
        /* Enable clock  */
        FCG_Fcg3PeriphClockCmd(DBG_DEVICE_FCG, ENABLE);
        /* Configure UART */
        (void)USART_UART_StructInit(&stcUartInit);
        stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
        (void)USART_UART_Init(DBG_DEVICE, &stcUartInit, NULL);
        for (u32Div = 0UL; u32Div <= USART_CLK_DIV64; u32Div++) 
		{
            USART_SetClockDiv(DBG_DEVICE, u32Div);
            i32Ret = USART_SetBaudrate(DBG_DEVICE, u32Baudrate, &f32Error);
            if ((LL_OK == i32Ret) && ((-DBG_BAUDRATE_ERR_MAX <= f32Error) && (f32Error <= DBG_BAUDRATE_ERR_MAX))) 
			{
                USART_FuncCmd(DBG_DEVICE, (USART_RX  | USART_TX), ENABLE);
                break;
            } 
			else 
			{
                i32Ret = LL_ERR;
            }
        }
    }
    return i32Ret;
}

/**
 * Initialize the standard input output facility
 */

int sys_stdio_init(void *vpDevice, uint32_t u32Param)
{
	static char init_done;
	int ret;

	if (init_done)
	{
		return SYS_OK;
	}
	//LL_PERIPH_WE(PROJECT_PERIPH_WE);
	DDL_PrintfInit(vpDevice, u32Param, dbg_init);/* Printf init */
	//LL_PERIPH_WP(PROJECT_PERIPH_WP);
	ret = os_mutex_create(&wmstdio_mutex, "uart-rd",OS_MUTEX_INHERIT);

	if (ret != SYS_OK)
	{
		return SYS_FAIL;
	}
	serial_console_init();
	init_done = 1;
	return SYS_OK;
}

/* For sending data only to the SERIAL console and nowhere else 
 * Usually for debugging purposes.
 */
int serial_console_printf(char *str);

uint32_t __console_wmprintf(const char *format, ...)
{
	va_list args;
	/* Format the string */
	if (c_stdio_funcs != &empty_funcs) 
	{
		va_start(args, format);
		vsnprintf(stdio_msg_buf, MAX_MSG_LEN, &format[0], args);
		va_end(args);
		return serial_console_printf(stdio_msg_buf);
	} 
	else 
	{
		return 0;
	}
}

