#include "monitor_board_dir.h"
#include "project_pin_use_config.h"
#include "project_config.h"
#include "sys_log.h"

/**
 * @brief  Instal IRQ handler.
 * @param  [in] pstcConfig      Pointer to struct @ref stc_irq_signin_config_t
 * @param  [in] u32Priority     Interrupt priority
 * @retval None
 */
static void monitor_irq_handler(const stc_irq_signin_config_t *pstcConfig, uint32_t u32Priority)
{
    if (NULL != pstcConfig) 
	{
        (void)INTC_IrqSignIn(pstcConfig);
        NVIC_ClearPendingIRQ(pstcConfig->enIRQn);
        NVIC_SetPriority(pstcConfig->enIRQn, u32Priority);
        NVIC_EnableIRQ(pstcConfig->enIRQn);
    }
}

void monitor_usart_init(func_ptr_t func_rx_err_callback,func_ptr_t func_rx_full_callback)
{
	stc_usart_uart_init_t stcUartInit;
	stc_irq_signin_config_t stcIrqSigninConfig;

	/* MCU Peripheral registers write unprotected */
	LL_PERIPH_WE(PROJECT_PERIPH_WE);

	/* Configure USART RX/TX pin. */
	GPIO_SetFunc(MOD_485_RX_PORT, MOD_485_RX_PIN, MOD_485_RX_FUNC);
	GPIO_SetFunc(MOD_485_TX_PORT, MOD_485_TX_PIN, MOD_485_TX_FUNC);

	/* Enable peripheral clock */
	FCG_Fcg3PeriphClockCmd(MOD_485_USART_FCG, ENABLE);

	/* Initialize UART. */
	(void)USART_UART_StructInit(&stcUartInit);
	stcUartInit.u32ClockDiv = USART_CLK_DIV64;
	stcUartInit.u32Baudrate = MOD_USART_BAUDRATE;
	stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
	if (LL_OK != USART_UART_Init(MOD_485_USART_ID, &stcUartInit, NULL)) 
	{

		log_e("modbus","modbus UartInit create error!");
	}

	/* Register RX error IRQ handler && configure NVIC. */
	stcIrqSigninConfig.enIRQn = MOD_RX_ERR_IRQn;
	stcIrqSigninConfig.enIntSrc = MOD_RX_ERR_INT_SRC;
	stcIrqSigninConfig.pfnCallback = func_rx_err_callback;
	monitor_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_11);

	/* Register RX full IRQ handler && configure NVIC. */
	stcIrqSigninConfig.enIRQn = MOD_RX_FULL_IRQn;
	stcIrqSigninConfig.enIntSrc = MOD_RX_FULL_INT_SRC;
	stcIrqSigninConfig.pfnCallback = func_rx_full_callback;
	monitor_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_10);

	/* MCU Peripheral registers write protected */
	LL_PERIPH_WP(PROJECT_PERIPH_WP);

	/* Enable RX function */
	USART_FuncCmd(MOD_485_USART_ID, (USART_RX|USART_INT_RX|USART_TX), ENABLE);
}

