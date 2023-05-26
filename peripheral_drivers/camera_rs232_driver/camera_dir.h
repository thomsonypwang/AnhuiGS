#ifndef _CAMERA_DIR_H_
#define _CAMERA_DIR_H_

#include "hc32_ll.h"
#include "project_config.h"

typedef struct 
{
    uint8_t  *pu8Data;
    uint32_t u32In;
    uint32_t u32Out;
    uint32_t u32Size;
    uint32_t u32FreeSize;
} camera_ring_buf_t;

void camera_rs232_init(void);
void camera_usart_init(func_ptr_t func_rx_err_callback,func_ptr_t func_rx_full_callback,func_ptr_t func_rx_timeout_callback);
void send_camera_data(uint8_t *buf,uint8_t len);
uint32_t receive_camera_data(uint8_t *data, uint32_t size, uint32_t timeout);

int32_t camera_buf_clear(camera_ring_buf_t *pstcBuf,uint32_t u32Size);
int32_t camera_buf_init(camera_ring_buf_t *pstcBuf, uint8_t *pu8Data, uint32_t u32Size);
uint32_t camera_buf_freesize(const camera_ring_buf_t *pstcBuf);
uint32_t camera_buf_usedsize(const camera_ring_buf_t *pstcBuf);
bool camera_buf_full(const camera_ring_buf_t *pstcBuf);
bool at_buf_empty(const camera_ring_buf_t *pstcBuf);
uint32_t camera_buf_read(camera_ring_buf_t *pstcBuf, uint8_t au8Data[], uint32_t u32Len);
uint32_t camera_buf_write(camera_ring_buf_t *pstcBuf, uint8_t au8Data[], uint32_t u32Len);
#endif
