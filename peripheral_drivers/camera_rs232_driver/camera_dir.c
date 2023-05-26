#include "camera_dir.h"
#include "project_pin_use_config.h"
#include <stdarg.h>
#include <string.h>

#include "sys_log.h"
#include "sys_errno.h"

static uint8_t camera_data_buf[8*1024];
static camera_ring_buf_t camera_ringbuf;

SemaphoreHandle_t camera_binsem_id= NULL;
static BaseType_t camera_xHigherPriorityTaskWoken;

static void USART1_RxError_IrqCallback(void)
{
	(void)USART_ReadData(CAMERA_USART_ID);
    USART_ClearStatus(CAMERA_USART_ID, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

static void USART1_RxFull_IrqCallback(void)
{
    uint8_t u8Data = (uint8_t)USART_ReadData(CAMERA_USART_ID);
	
    (void)camera_buf_write(&camera_ringbuf, &u8Data, 1UL);
}

static void USART1_RxTimeout_IrqCallback(void)
{
	if(camera_binsem_id!=NULL)
	{
		xSemaphoreGiveFromISR(camera_binsem_id,&camera_xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(camera_xHigherPriorityTaskWoken);	
	}	
	TMR0_Stop(CAMERA_USART_TMR0, CAMERA_USART_TMR0_CH);
    USART_ClearStatus(CAMERA_USART_ID, USART_FLAG_RX_TIMEOUT);
}

static void camera_irq_handler(const stc_irq_signin_config_t *pstcConfig, uint32_t u32Priority)
{
    if (NULL != pstcConfig) 
	{
        (void)INTC_IrqSignIn(pstcConfig);
        NVIC_ClearPendingIRQ(pstcConfig->enIRQn);
        NVIC_SetPriority(pstcConfig->enIRQn, u32Priority);
        NVIC_EnableIRQ(pstcConfig->enIRQn);
    }
}

static void camera_timerout_config(uint16_t u16TimeoutBits)
{
	uint16_t u16Div;
    uint16_t u16Delay;
    uint16_t u16CompareValue;
    stc_tmr0_init_t stcTmr0Init;

	FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);/* Enable AOS clock */
    AOS_SetTriggerEventSrc(AOS_TMR0, EVT_SRC_PORT_EIRQ0);/* Timer0 trigger event set */
    FCG_Fcg2PeriphClockCmd(CAMERA_USART_TMR0_FCG, ENABLE);

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
    (void)TMR0_Init(CAMERA_USART_TMR0, CAMERA_USART_TMR0_CH, &stcTmr0Init);

    TMR0_HWStartCondCmd(CAMERA_USART_TMR0, CAMERA_USART_TMR0_CH, ENABLE);
    TMR0_HWClearCondCmd(CAMERA_USART_TMR0, CAMERA_USART_TMR0_CH, ENABLE);
}

void camera_usart_init(func_ptr_t func_rx_err_callback,func_ptr_t func_rx_full_callback,func_ptr_t func_rx_timeout_callback)
{
	stc_usart_uart_init_t stcUartInit;
	stc_irq_signin_config_t stcIrqSigninConfig;

	/* MCU Peripheral registers write unprotected */
	LL_PERIPH_WE(PROJECT_PERIPH_WE);
	
	/* Configure USART RX/TX pin. */
	GPIO_SetFunc(CAMERA_RX_PORT, CAMERA_RX_PIN, CAMERA_RX_FUNC);
	GPIO_SetFunc(CAMERA_TX_PORT, CAMERA_TX_PIN, CAMERA_TX_FUNC);

	/* Enable peripheral clock */
	FCG_Fcg3PeriphClockCmd(CAMERA_USART_FCG, ENABLE);

	/* Initialize UART. */
	(void)USART_UART_StructInit(&stcUartInit);
	stcUartInit.u32ClockDiv = USART_CLK_DIV64;
	stcUartInit.u32CKOutput = USART_CK_OUTPUT_ENABLE;
	stcUartInit.u32Baudrate = CAMERA_USART_BAUDRATE;
	stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
	if (LL_OK != USART_UART_Init(CAMERA_USART_ID, &stcUartInit, NULL)) 
	{
		log_e("CAMERA","CAMERA UartInit create error!");
	}

	/* Register RX error IRQ handler && configure NVIC. */
	stcIrqSigninConfig.enIRQn = CAMERA_RX_ERR_IRQn;
	stcIrqSigninConfig.enIntSrc = CAMERA_RX_ERR_INT_SRC;
	stcIrqSigninConfig.pfnCallback = func_rx_err_callback;
	camera_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_05);

	/* Register RX full IRQ handler && configure NVIC. */
	stcIrqSigninConfig.enIRQn = CAMERA_RX_FULL_IRQn;
	stcIrqSigninConfig.enIntSrc = CAMERA_RX_FULL_INT_SRC;
	stcIrqSigninConfig.pfnCallback = func_rx_full_callback;
	camera_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_06);
	
	/* Register RX timeout IRQ handler. */
    stcIrqSigninConfig.enIRQn = CAMERA_RX_TIMEOUT_IRQn;
    stcIrqSigninConfig.enIntSrc = CAMERA_RX_TIMEOUT_INT_SRC;
    stcIrqSigninConfig.pfnCallback = func_rx_timeout_callback;
	camera_irq_handler(&stcIrqSigninConfig, DDL_IRQ_PRIO_07);
	
	camera_timerout_config(CAMERA_USART_TMR0_TIMEROUT);

	/* MCU Peripheral registers write protected */
	LL_PERIPH_WP(PROJECT_PERIPH_WP);

	/* Enable RX function */
	USART_FuncCmd(CAMERA_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
}

void camera_rs232_init(void)
{	
	camera_usart_init(USART1_RxError_IrqCallback,USART1_RxFull_IrqCallback,USART1_RxTimeout_IrqCallback);	
	camera_binsem_id = xSemaphoreCreateBinary(); /*创建二值信号量*/	
    (void)camera_buf_init(&camera_ringbuf, camera_data_buf, sizeof(camera_data_buf));/* Initialize ring buffer function. */
}

void send_camera_data(uint8_t *buf,uint8_t len)
{
	uint8_t i;
	
	//log_i("camera tx data:%s",buf);
	while (RESET == USART_GetStatus(CAMERA_USART_ID, USART_FLAG_TX_EMPTY)) {}  
	for (i = 0; i < len; i++)
	{
		USART_WriteData(CAMERA_USART_ID, buf[i]);
		while (RESET == USART_GetStatus(CAMERA_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}
}

uint32_t receive_camera_data(uint8_t *data, uint32_t size, uint32_t timeout)
{
	uint32_t rx_data_buf_len=0;
	uint32_t tmp=0;
	if(xSemaphoreTake(camera_binsem_id,pdMS_TO_TICKS(timeout))==pdTRUE)//等待uart中断接收完数据,最大等待时间为:wait_ack_dalay1ms
	{
		rx_data_buf_len=camera_buf_usedsize(&camera_ringbuf);
		log_i("camera rx len=%d size=%d",rx_data_buf_len,size);
		if(rx_data_buf_len>=size)
		{
			tmp=camera_buf_read(&camera_ringbuf, data, size);
			if(rx_data_buf_len>size)
			{
				camera_buf_clear(&camera_ringbuf,8*1024);
			}
		}
		else
		{
			tmp=camera_buf_read(&camera_ringbuf, data, rx_data_buf_len);
		}
		return rx_data_buf_len;
	}
	else
	{
		rx_data_buf_len=camera_buf_usedsize(&camera_ringbuf);
		log_e("camera","camera_uart_receive ,Time out!!");
		log_i("rx_data_buf_len=%d",rx_data_buf_len);
		return rx_data_buf_len;
	}		
}

int32_t camera_buf_clear(camera_ring_buf_t *pstcBuf,uint32_t u32Size)
{
    int32_t i32Ret = -1;

    if ((pstcBuf != NULL) && (u32Size != 0UL)) 
	{
        pstcBuf->u32In = 0;
        pstcBuf->u32Out = 0;
        pstcBuf->u32Size = u32Size;
        pstcBuf->u32FreeSize = u32Size;
        i32Ret = 0;
    }

    return i32Ret;
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
int32_t camera_buf_init(camera_ring_buf_t *pstcBuf, uint8_t *pu8Data, uint32_t u32Size)
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
uint32_t camera_buf_freeSize(const camera_ring_buf_t *pstcBuf)
{
    return pstcBuf->u32FreeSize;
}

/**
 * @brief  Ring buffer used size.
 * @param  [in] pstcBuf                 Pointer to a @ref stc_ring_buf_t structure
 * @retval Ring buffer used size
 */
uint32_t camera_buf_usedsize(const camera_ring_buf_t *pstcBuf)
{
    return (pstcBuf->u32Size - pstcBuf->u32FreeSize);
}

/**
 * @brief  Ring buffer full.
 * @param  [in] pstcBuf                 Pointer to a @ref stc_ring_buf_t structure
 * @retval Ring buffer status
 */
bool camera_buf_full(const camera_ring_buf_t *pstcBuf)
{
    return (0UL == pstcBuf->u32FreeSize);
}

/**
 * @brief  Check ring buffer empty.
 * @param  [in] pstcBuf                 Pointer to a @ref stc_ring_buf_t structure
 * @retval Ring buffer status
 */
bool camera_buf_empty(const camera_ring_buf_t *pstcBuf)
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
uint32_t camera_buf_read(camera_ring_buf_t *pstcBuf, uint8_t au8Data[], uint32_t u32Len)
{
    uint32_t u32CopyLen;
    uint32_t u32ReadLen = 0UL;

    if ((pstcBuf != NULL) && (au8Data != NULL) && (u32Len != 0UL)) 
	{
        u32ReadLen = camera_buf_usedsize(pstcBuf);
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
uint32_t camera_buf_write(camera_ring_buf_t *pstcBuf, uint8_t au8Data[], uint32_t u32Len)
{
    uint32_t u32CopyLen;
    uint32_t u32WriteLen = 0UL;

    if ((pstcBuf != NULL) && (au8Data != NULL) && (u32Len != 0UL)) 
	{
        u32WriteLen = camera_buf_freeSize(pstcBuf);

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
