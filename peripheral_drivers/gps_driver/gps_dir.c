#include "gps_dir.h"
#include "project_pin_use_config.h"
#include "project_config.h"
#include "sys_log.h"

/**
 * @brief  Instal IRQ handler.
 * @param  [in] pstcConfig      Pointer to struct @ref stc_irq_signin_config_t
 * @param  [in] u32Priority     Interrupt priority
 * @retval None
 */
static void gps_irq_handler(const stc_irq_signin_config_t *pstcConfig, uint32_t u32Priority)
{
    if (NULL != pstcConfig) 
	{
        (void)INTC_IrqSignIn(pstcConfig);
        NVIC_ClearPendingIRQ(pstcConfig->enIRQn);
        NVIC_SetPriority(pstcConfig->enIRQn, u32Priority);
        NVIC_EnableIRQ(pstcConfig->enIRQn);
    }
}

void gps_usart_init(func_ptr_t func_rx_err_callback,func_ptr_t func_rx_full_callback)
{
	stc_usart_uart_init_t stcUartInit;
	stc_irq_signin_config_t stcIrqSigninConfig;

	/* MCU Peripheral registers write unprotected */
	LL_PERIPH_WE(PROJECT_PERIPH_WE);

	/* Configure USART RX/TX pin. */
	GPIO_SetFunc(GPS_TX_PORT, GPS_TX_PIN, GPS_TX_FUNC);
	GPIO_SetFunc(GPS_RX_PORT, GPS_RX_PIN, GPS_RX_FUNC);

	/* Enable peripheral clock */
	FCG_Fcg3PeriphClockCmd(GPS_USART_FCG, ENABLE);

	/* Initialize UART. */
	(void)USART_UART_StructInit(&stcUartInit);
	stcUartInit.u32ClockDiv = USART_CLK_DIV64;
	stcUartInit.u32Baudrate = GPS_USART_BAUDRATE;
	stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
	if (LL_OK != USART_UART_Init(GPS_USART_ID, &stcUartInit, NULL)) 
	{

		log_e("gps","modbus UartInit create error!");
	}

	/* Register RX error IRQ handler && configure NVIC. */
	stcIrqSigninConfig.enIRQn = GPS_RX_ERR_IRQn;
	stcIrqSigninConfig.enIntSrc = GPS_RX_ERR_INT_SRC;
	stcIrqSigninConfig.pfnCallback = func_rx_err_callback;
	gps_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_11);

	/* Register RX full IRQ handler && configure NVIC. */
	stcIrqSigninConfig.enIRQn = GPS_RX_FULL_IRQn;
	stcIrqSigninConfig.enIntSrc = GPS_RX_FULL_INT_SRC;
	stcIrqSigninConfig.pfnCallback = func_rx_full_callback;
	gps_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_10);

	/* MCU Peripheral registers write protected */
	LL_PERIPH_WP(PROJECT_PERIPH_WP);

	/* Enable RX function */
	USART_FuncCmd(GPS_USART_ID, (USART_RX|USART_INT_RX|USART_TX), ENABLE);
}

