#include "lock_rs485_dir.h"
#include "project_pin_use_config.h"
#include <stdarg.h>
#include <string.h>

#include "sys_log.h"
#include "sys_errno.h"


static void lock_irq_handler(const stc_irq_signin_config_t *pstcConfig, uint32_t u32Priority)
{
    if (NULL != pstcConfig) 
	{
        (void)INTC_IrqSignIn(pstcConfig);
        NVIC_ClearPendingIRQ(pstcConfig->enIRQn);
        NVIC_SetPriority(pstcConfig->enIRQn, u32Priority);
        NVIC_EnableIRQ(pstcConfig->enIRQn);
    }
}

static void lock_timerout_config(uint16_t u16TimeoutBits)
{
    uint16_t u16Div;
    uint16_t u16Delay;    
	uint16_t u16CompareValue;
    stc_tmr0_init_t stcTmr0Init;

	FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);/* Enable AOS clock */
    AOS_SetTriggerEventSrc(AOS_TMR0, EVT_SRC_PORT_EIRQ0);/* Timer0 trigger event set */
    FCG_Fcg2PeriphClockCmd(LOCK_USART_TMR0_FCG, ENABLE);

    /* Initialize TMR0 base function. */
    stcTmr0Init.u32ClockSrc = TMR0_CLK_SRC_XTAL32;
    stcTmr0Init.u32ClockDiv = TMR0_CLK_DIV8;
    stcTmr0Init.u32Func     = TMR0_FUNC_CMP;
    if (TMR0_CLK_DIV1 == stcTmr0Init.u32ClockDiv) 
	{
        u16Delay = 7U;
    } 
	else if (TMR0_CLK_DIV2 == stcTmr0Init.u32ClockDiv) 
	{
        u16Delay = 5U;
    } 
	else if ((TMR0_CLK_DIV4 == stcTmr0Init.u32ClockDiv) || \
               (TMR0_CLK_DIV8 == stcTmr0Init.u32ClockDiv) || \
               (TMR0_CLK_DIV16 == stcTmr0Init.u32ClockDiv)) 
	{
        u16Delay = 3U;
    } 
	else 
	{
        u16Delay = 2U;
    }	
	u16Div = (uint16_t)1U << (stcTmr0Init.u32ClockDiv >> TMR0_BCONR_CKDIVA_POS);
    u16CompareValue = ((u16TimeoutBits + u16Div - 1U) / u16Div) - u16Delay;
	
    stcTmr0Init.u16CompareValue = u16CompareValue;
    (void)TMR0_Init(LOCK_USART_TMR0, LOCK_USART_TMR0_CH, &stcTmr0Init);

    TMR0_HWStartCondCmd(LOCK_USART_TMR0, LOCK_USART_TMR0_CH, ENABLE);
    TMR0_HWClearCondCmd(LOCK_USART_TMR0, LOCK_USART_TMR0_CH, ENABLE);
}

void lock_usart_init(func_ptr_t func_rx_err_callback,func_ptr_t func_rx_full_callback,func_ptr_t func_rx_timeout_callback)
{
	stc_usart_uart_init_t stcUartInit;
	stc_irq_signin_config_t stcIrqSigninConfig;

	/* MCU Peripheral registers write unprotected */
	LL_PERIPH_WE(PROJECT_PERIPH_WE);
	
	/* Configure USART RX/TX pin. */
	GPIO_SetFunc(LOCK_RX_PORT, LOCK_RX_PIN, LOCK_RX_FUNC);
	GPIO_SetFunc(LOCK_TX_PORT, LOCK_TX_PIN, LOCK_TX_FUNC);

	/* Enable peripheral clock */
	FCG_Fcg3PeriphClockCmd(LOCK_USART_FCG, ENABLE);

	/* Initialize UART. */
	(void)USART_UART_StructInit(&stcUartInit);
	stcUartInit.u32ClockDiv = USART_CLK_DIV64;
	stcUartInit.u32CKOutput = USART_CK_OUTPUT_ENABLE;
	stcUartInit.u32Baudrate = LOCK_USART_BAUDRATE;
	stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
	if (LL_OK != USART_UART_Init(LOCK_USART_ID, &stcUartInit, NULL)) 
	{
		log_e("autoeclosing","autoeclosing UartInit create error!");
	}

	/* Register RX error IRQ handler && configure NVIC. */
	stcIrqSigninConfig.enIRQn = LOCK_RX_ERR_IRQn;
	stcIrqSigninConfig.enIntSrc = LOCK_RX_ERR_INT_SRC;
	stcIrqSigninConfig.pfnCallback = func_rx_err_callback;
	lock_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_12);

	/* Register RX full IRQ handler && configure NVIC. */
	stcIrqSigninConfig.enIRQn = LOCK_RX_FULL_IRQn;
	stcIrqSigninConfig.enIntSrc = LOCK_RX_FULL_INT_SRC;
	stcIrqSigninConfig.pfnCallback = func_rx_full_callback;
	lock_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_14);
	
	/* Register RX timeout IRQ handler. */
    stcIrqSigninConfig.enIRQn = LOCK_RX_TIMEOUT_IRQn;
    stcIrqSigninConfig.enIntSrc = LOCK_RX_TIMEOUT_INT_SRC;
    stcIrqSigninConfig.pfnCallback = func_rx_timeout_callback;
	lock_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_13);
	
	lock_timerout_config(LOCK_USART_TMR0_TIMEROUT);

	/* MCU Peripheral registers write protected */
	LL_PERIPH_WP(PROJECT_PERIPH_WP);

	/* Enable RX function */
	USART_FuncCmd(LOCK_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
}

void send_lock_data(uint8_t *buf,uint8_t len)
{
	uint8_t i;
	
	log_i("lock tx len=%d",len);
	log_i("lock tx data:%s",buf);
	while (RESET == USART_GetStatus(LOCK_USART_ID, USART_FLAG_TX_EMPTY)) {}  
	for (i = 0; i < len; i++)
	{
		USART_WriteData(LOCK_USART_ID, buf[i]);
		while (RESET == USART_GetStatus(LOCK_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}
}