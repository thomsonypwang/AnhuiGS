#ifndef _NB_UART_DIR_H
#define _NB_UART_DIR_H

#include "project_config.h"

/**
 * @brief Ring buffer structure definition
 */
typedef struct 
{
    uint8_t  *pu8Data;
    uint32_t u32In;
    uint32_t u32Out;
    uint32_t u32Size;
    uint32_t u32FreeSize;
} at_ring_buf_t;

void nbiot_uart_init(void);
uint32_t at_uart_send(void *data, uint32_t size, uint32_t timeout);
int32_t at_uart_receive(uint8_t *data, uint32_t size, uint32_t timeout);
/**
 * @addtogroup Ring_Buffer_Global_Functions
 * @{
 */

int32_t at_buf_init(at_ring_buf_t *pstcBuf, uint8_t *pu8Data, uint32_t u32Size);

uint32_t at_buf_freesize(const at_ring_buf_t *pstcBuf);
uint32_t at_buf_usedsize(const at_ring_buf_t *pstcBuf);

bool at_buf_full(const at_ring_buf_t *pstcBuf);
bool at_buf_empty(const at_ring_buf_t *pstcBuf);

uint32_t at_buf_read(at_ring_buf_t *pstcBuf, uint8_t au8Data[], uint32_t u32Len);
uint32_t at_buf_write(at_ring_buf_t *pstcBuf, uint8_t au8Data[], uint32_t u32Len);

void nbiot_power_on(void);
void nbiot_power_off(void);

#endif
