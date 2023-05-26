#include "nb_uart_dir.h"
#include "project_pin_use_config.h"

#include "sys_os.h"
#include "sys_log.h"

/* Ring buffer size */
#define RING_BUF_SIZE	(2048)

static uint8_t at_data_buf[RING_BUF_SIZE];
static at_ring_buf_t at_ringbuf;

SemaphoreHandle_t at_binsem_id= NULL;
static BaseType_t at_xHigherPriorityTaskWoken;

uint8_t send_flag=0;

void nbiot_power_on(void)
{
	GPIO_SetPins(NBIOT_POWER_PORT, NBIOT_POWER_PIN);
}

void nbiot_power_off(void)
{
	GPIO_ResetPins(NBIOT_POWER_PORT, NBIOT_POWER_PIN);
}

static void USART6_RxError_IrqCallback(void)
{
	(void)USART_ReadData(NBIOT_USART_ID);
    USART_ClearStatus(NBIOT_USART_ID, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

static void USART6_RxFull_IrqCallback(void)
{
    uint8_t u8Data = (uint8_t)USART_ReadData(NBIOT_USART_ID);
	
    (void)at_buf_write(&at_ringbuf, &u8Data, 1UL);
}

static void USART6_RxTimeout_IrqCallback(void)
{
	if(at_binsem_id!=NULL)
	{
		xSemaphoreGiveFromISR(at_binsem_id,&at_xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(at_xHigherPriorityTaskWoken);	
	}	
	TMR0_Stop(NBIOT_USART_TMR0, NBIOT_USART_TMR0_CH);
    USART_ClearStatus(NBIOT_USART_ID, USART_FLAG_RX_TIMEOUT);
}

/**
 * @brief  Instal IRQ handler.
 * @param  [in] pstcConfig      Pointer to struct @ref stc_irq_signin_config_t
 * @param  [in] u32Priority     Interrupt priority
 * @retval None
 */
static void at_irq_handler(const stc_irq_signin_config_t *pstcConfig, uint32_t u32Priority)
{
    if (NULL != pstcConfig) 
	{
        (void)INTC_IrqSignIn(pstcConfig);
        NVIC_ClearPendingIRQ(pstcConfig->enIRQn);
        NVIC_SetPriority(pstcConfig->enIRQn, u32Priority);
        NVIC_EnableIRQ(pstcConfig->enIRQn);
    }
}

static void at_timerout_config(uint16_t u16TimeoutBits)
{
    uint16_t u16Div;
    uint16_t u16Delay;
	uint16_t u16CompareValue;
    stc_tmr0_init_t stcTmr0Init;

	FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);/* Enable AOS clock */
    AOS_SetTriggerEventSrc(AOS_TMR0, EVT_SRC_PORT_EIRQ0);/* Timer0 trigger event set */
    FCG_Fcg2PeriphClockCmd(NBIOT_USART_TMR0_FCG, ENABLE);

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
    (void)TMR0_Init(NBIOT_USART_TMR0, NBIOT_USART_TMR0_CH, &stcTmr0Init);

    TMR0_HWStartCondCmd(NBIOT_USART_TMR0, NBIOT_USART_TMR0_CH, ENABLE);
    TMR0_HWClearCondCmd(NBIOT_USART_TMR0, NBIOT_USART_TMR0_CH, ENABLE);
}

//////////////////////////////////////////////////
void at_uart_init(void)
{
	stc_usart_uart_init_t stcUartInit;
	stc_irq_signin_config_t stcIrqSigninConfig;
	stc_gpio_init_t stcGpioInit;
	
	LL_PERIPH_WE(PROJECT_PERIPH_WE);
	
	FCG_Fcg3PeriphClockCmd(NBIOT_USART_FCG, ENABLE);
	
	(void)GPIO_StructInit(&stcGpioInit);
	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_OUT;
    (void)GPIO_Init(NBIOT_POWER_PORT, NBIOT_POWER_PIN, &stcGpioInit);
	
	GPIO_SetFunc(NBIOT_TX_PORT,NBIOT_TX_PIN,NBIOT_TX_FUNC);//USART-TX    
    GPIO_SetFunc(NBIOT_RX_PORT,NBIOT_RX_PIN,NBIOT_RX_FUNC);//USART-RX
	nbiot_power_on();
    USART_DeInit(NBIOT_USART_ID);
	/* Initialize UART. */
	(void)USART_UART_StructInit(&stcUartInit);
	stcUartInit.u32ClockDiv = USART_CLK_DIV16;
	stcUartInit.u32CKOutput = USART_CK_OUTPUT_ENABLE;
	stcUartInit.u32Baudrate = NBIOT_USART_BAUDRATE;
	stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
	if (LL_OK != USART_UART_Init(NBIOT_USART_ID, &stcUartInit, NULL)) 
	{
		log_e("nbiot","nbiot UartInit create error!");
	}

	/* Register RX error IRQ handler && configure NVIC. */
	stcIrqSigninConfig.enIRQn = NBIOT_RX_ERR_IRQn;
	stcIrqSigninConfig.enIntSrc = NBIOT_RX_ERR_INT_SRC;
	stcIrqSigninConfig.pfnCallback = &USART6_RxError_IrqCallback;
	at_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_DEFAULT);

	/* Register RX full IRQ handler && configure NVIC. */
	stcIrqSigninConfig.enIRQn = NBIOT_RX_FULL_IRQn;
	stcIrqSigninConfig.enIntSrc = NBIOT_RX_FULL_INT_SRC;
	stcIrqSigninConfig.pfnCallback = &USART6_RxFull_IrqCallback;
	at_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_14);
	
	/* Register RX timeout IRQ handler. */
    stcIrqSigninConfig.enIRQn = NBIOT_RX_TIMEOUT_IRQn;
    stcIrqSigninConfig.enIntSrc = NBIOT_RX_TIMEOUT_INT_SRC;
    stcIrqSigninConfig.pfnCallback = &USART6_RxTimeout_IrqCallback;
	at_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_09);
	
	at_timerout_config(NBIOT_USART_TMR0_TIMEROUT);

	LL_PERIPH_WP(PROJECT_PERIPH_WP); 
	
	 /* Enable USART_TX | USART_RX | USART_RX_TIMEOUT | USART_INT_RX function */
    USART_FuncCmd(NBIOT_USART_ID, (USART_TX | USART_RX | USART_RX_TIMEOUT | USART_INT_RX | USART_INT_RX_TIMEOUT), ENABLE);
}

void nbiot_uart_init(void)
{	
	at_uart_init();	
	at_binsem_id = xSemaphoreCreateBinary(); /*创建二值信号量*/	
    (void)at_buf_init(&at_ringbuf, at_data_buf, sizeof(at_data_buf));/* Initialize ring buffer function. */
}

uint32_t at_uart_send(void *data, uint32_t size, uint32_t timeout)
{
	uint8_t * data_buf = (uint8_t *)data;
	
	send_flag=1;
	USART_UART_Trans(NBIOT_USART_ID,data_buf,size,0x80000);
	return size;
}

int32_t at_uart_receive(uint8_t *data, uint32_t size, uint32_t timeout)
{
	uint32_t rx_data_buf_len;
	
	if(send_flag==1)
	{
		send_flag=0;
		if(xSemaphoreTake(at_binsem_id,pdMS_TO_TICKS(timeout))==pdTRUE)//等待uart中断接收完数据,最大等待时间为:wait_ack_dalay1ms
		{
			rx_data_buf_len=at_buf_usedsize(&at_ringbuf);
			//log_i("at_uart_receive1 rx_data_buf_len=%d",rx_data_buf_len);
			return at_buf_read(&at_ringbuf, data, rx_data_buf_len);
		}
		else
		{
			rx_data_buf_len=at_buf_usedsize(&at_ringbuf);
			log_e("at","at_uart_receive ,Time out!!");
			log_i("rx_data_buf_len=%d",rx_data_buf_len);
			return -1;	
		}		
	}
	else
	{
		return -1;	
	}
}

/**
 * @brief  Initialize ring buffer.
 * @param  [in] pstcBuf                 Pointer to a @ref stc_ring_buf_t structure
 * @param  [in] pu8Data                 Data buffer
 * @param  [in] u32Size                 Data buffer size
 * @retval int32_t:
 *           - LL_OK:                   Initialize successfully.
 *           - LL_ERR_INVD_PARAM:       If one of following cases matches:
 *                                      - The pointer pstcBuf value is NULL.
 *                                      - The pointer pu8Data value is NULL.
 *                                      - The u32Size value is 0.
 */
int32_t at_buf_init(at_ring_buf_t *pstcBuf, uint8_t *pu8Data, uint32_t u32Size)
{
    int32_t i32Ret = -1;

    if ((pstcBuf != NULL) && (pu8Data != NULL) && (u32Size != 0UL)) 
	{
        pstcBuf->pu8Data = pu8Data;
        pstcBuf->u32In = 0;
        pstcBuf->u32Out = 0;
        pstcBuf->u32Size = u32Size;
        pstcBuf->u32FreeSize = u32Size;
        i32Ret = 0;
    }

    return i32Ret;
}

/**
 * @brief  Ring buffer free size.
 * @param  [in] pstcBuf                 Pointer to a @ref stc_ring_buf_t structure
 * @retval Ring buffer free size
 */
uint32_t at_buf_freeSize(const at_ring_buf_t *pstcBuf)
{
    return pstcBuf->u32FreeSize;
}

/**
 * @brief  Ring buffer used size.
 * @param  [in] pstcBuf                 Pointer to a @ref stc_ring_buf_t structure
 * @retval Ring buffer used size
 */
uint32_t at_buf_usedsize(const at_ring_buf_t *pstcBuf)
{
    return (pstcBuf->u32Size - pstcBuf->u32FreeSize);
}

/**
 * @brief  Ring buffer full.
 * @param  [in] pstcBuf                 Pointer to a @ref stc_ring_buf_t structure
 * @retval Ring buffer status
 */
bool at_buf_full(const at_ring_buf_t *pstcBuf)
{
    return (0UL == pstcBuf->u32FreeSize);
}

/**
 * @brief  Check ring buffer empty.
 * @param  [in] pstcBuf                 Pointer to a @ref stc_ring_buf_t structure
 * @retval Ring buffer status
 */
bool at_buf_empty(const at_ring_buf_t *pstcBuf)
{
    return (pstcBuf->u32Size == pstcBuf->u32FreeSize);
}

/**
 * @brief  Read ring buffer.
 * @param  [in] pstcBuf                 Pointer to a @ref stc_ring_buf_t structure
 * @param  [in] au8Data                 Pointer to data buffer to read
 * @param  [in] u32Len                  Data length
 * @retval Read length
 */
uint32_t at_buf_read(at_ring_buf_t *pstcBuf, uint8_t au8Data[], uint32_t u32Len)
{
    uint32_t u32CopyLen;
    uint32_t u32ReadLen = 0UL;

    if ((pstcBuf != NULL) && (au8Data != NULL) && (u32Len != 0UL)) 
	{
        u32ReadLen = at_buf_usedsize(pstcBuf);
        if (u32ReadLen >= u32Len) 
		{
            u32ReadLen = u32Len;
        }

        if (pstcBuf->u32Out + u32ReadLen <= pstcBuf->u32Size) 
		{
            (void)memcpy(au8Data, &pstcBuf->pu8Data[pstcBuf->u32Out], u32ReadLen);
        } 
		else 
		{
            u32CopyLen = pstcBuf->u32Size - pstcBuf->u32Out;
            (void)memcpy(&au8Data[0], &pstcBuf->pu8Data[pstcBuf->u32Out], u32CopyLen);
            (void)memcpy(&au8Data[u32CopyLen], &pstcBuf->pu8Data[0], u32ReadLen - u32CopyLen);
        }

        __disable_irq();
        pstcBuf->u32FreeSize += u32ReadLen;
        pstcBuf->u32Out += u32ReadLen;
        if (pstcBuf->u32Out >= pstcBuf->u32Size) 
		{
            pstcBuf->u32Out %= pstcBuf->u32Size;
        }
        __enable_irq();
    }

    return u32ReadLen;
}

/**
 * @brief  Write ring buffer.
 * @param  [in] pstcBuf                 Pointer to a @ref stc_ring_buf_t structure
 * @param  [in] au8Data                 Pointer to data buffer to write
 * @param  [in] u32Len                  Data length
 * @retval Write length
 */
uint32_t at_buf_write(at_ring_buf_t *pstcBuf, uint8_t au8Data[], uint32_t u32Len)
{
    uint32_t u32CopyLen;
    uint32_t u32WriteLen = 0UL;

    if ((pstcBuf != NULL) && (au8Data != NULL) && (u32Len != 0UL)) 
	{
        u32WriteLen = at_buf_freeSize(pstcBuf);

        if (u32Len <= u32WriteLen) 
		{
            u32WriteLen = u32Len;
        }

        if (pstcBuf->u32In + u32WriteLen <= pstcBuf->u32Size) 
		{
            (void)memcpy(&pstcBuf->pu8Data[pstcBuf->u32In], au8Data, u32WriteLen);
        } 
		else 
		{
            u32CopyLen = pstcBuf->u32Size - pstcBuf->u32In;
            (void)memcpy(&pstcBuf->pu8Data[pstcBuf->u32In], &au8Data[0], u32CopyLen);
            (void)memcpy(&pstcBuf->pu8Data[0], &au8Data[u32CopyLen], u32WriteLen - u32CopyLen);
        }

        __disable_irq();
        pstcBuf->u32FreeSize -= u32WriteLen;
        pstcBuf->u32In += u32WriteLen;
        if (pstcBuf->u32In >= pstcBuf->u32Size) 
		{
            pstcBuf->u32In %= pstcBuf->u32Size;
        }
        __enable_irq();
    }

    return u32WriteLen;
}
