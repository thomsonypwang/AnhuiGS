
#ifndef __CORE_RING_BUFFER_H__
#define __CORE_RING_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "core_stdinc.h"

#define _core_bool uint8_t

typedef struct{
    uint8_t *buf;  
    uint32_t buff_size;
    uint32_t head;   
    uint32_t tail;  
} core_ringbuf_t, *core_ringbuf_pt;

int32_t core_ringbuf_init(core_ringbuf_t* ring_buff, void *data, uint32_t size);

void core_ringbuf_push_char(core_ringbuf_t *ring_buff, uint8_t data);

int32_t core_ringbuf_write(core_ringbuf_t *ring_buff, const uint8_t *data, uint32_t len);

int32_t core_ringbuf_pop_char(core_ringbuf_t *ring_buff, uint8_t *data);

int32_t core_ringbuf_read(core_ringbuf_t *ring_buff, uint8_t *data, uint32_t len);

int32_t core_ringbuf_len(core_ringbuf_t *ring_buff);

int32_t core_ringbuf_free(core_ringbuf_t* ring_buff);

#ifdef __cplusplus
}
#endif

#endif
