/*
 * smt_ring_fifo.c
 *
 *  Created on: 
 *      Author: zhiqin
 */

#include <core_ring_buffer.h>
#include "aiot_state_api.h"

#define CORE_IS_POWER_OF_2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
#define DEFINE_MIN(x, y)      ((x) < (y) ? (x) : (y))
#define DEFINE_MAX(x, y)      ((x) > (y) ? (x) : (y))

_core_bool core_ringbuf_is_empty(core_ringbuf_t *ring_buff) {
    return (ring_buff->head == ring_buff->tail);
}

_core_bool core_ringbuf_is_full(core_ringbuf_t *ring_buff) {
    return (((ring_buff->tail + 1) % ring_buff->buff_size) == ring_buff->head);
}

int32_t core_ringbuf_len(core_ringbuf_t *ring_buff) {
    if (core_ringbuf_is_empty(ring_buff)) {
        return 0;
    }
    return ((ring_buff->tail - ring_buff->head) & (ring_buff->buff_size - 1));
}
int32_t core_ringbuf_free(core_ringbuf_t* ring_buff) {
    uint32_t size, tail, head;

    tail = ring_buff->tail;
    head = ring_buff->head;
    if (tail == head) {
        size = ring_buff->buff_size;
    } else if (head > tail) {
        size = head - tail;
    } else {
        size = ring_buff->buff_size - (tail - head);
    }
    return size - 1;
}

int32_t core_ringbuf_init(core_ringbuf_t* ring_buff, void *data, uint32_t size) {
    if (!CORE_IS_POWER_OF_2(size) || data == NULL || ring_buff == NULL) {
        return -1;
    }
    
    ring_buff->buf = data;
    ring_buff->buff_size = size;
    ring_buff->head = 0;
    ring_buff->tail = 0;
    memset(data, 0, size);
    return 0;
}

void core_ringbuf_push_char(core_ringbuf_t *ring_buff, uint8_t data)
{
    if (core_ringbuf_is_full(ring_buff)) {
        ring_buff->head = (ring_buff->head + 1) % ring_buff->buff_size;
    }
    ring_buff->buf[ring_buff->tail] = data;
    ring_buff->tail = (ring_buff->tail + 1) % ring_buff->buff_size;
}

int32_t core_ringbuf_write(core_ringbuf_t *ring_buff, const uint8_t *data, uint32_t size) {
    uint32_t tocopy, free;

    free = core_ringbuf_free(ring_buff);
    size = DEFINE_MIN(free, size);
    if (size == 0) {
        return 0;
    }
    
    tocopy = DEFINE_MIN(ring_buff->buff_size - ring_buff->tail, size);
    memcpy(&ring_buff->buf[ring_buff->tail], data, tocopy);
    ring_buff->tail += tocopy;
    size -= tocopy;
    if (size > 0) {
        memcpy(ring_buff->buf, &data[tocopy], size);
        ring_buff->tail = size;
    }

    ring_buff->tail = (ring_buff->tail % ring_buff->buff_size);
    return tocopy + size;
}

int32_t core_ringbuf_pop_char(core_ringbuf_t *ring_buff, uint8_t *data)
{
    // assert(ring_buff || data);
    if (core_ringbuf_is_empty(ring_buff)) {
        return 0;
    }
    *data = ring_buff->buf[ring_buff->head];
    ring_buff->head = (ring_buff->head + 1) % ring_buff->buff_size;
    return 1;
}

int32_t core_ringbuf_read(core_ringbuf_t *ring_buff, uint8_t *data, uint32_t size) {
    if (ring_buff == NULL || data == NULL || core_ringbuf_is_empty(ring_buff)) {
        return 0;
    }
    uint32_t tocopy, len;
    uint8_t* pdata = data;

    len = core_ringbuf_len(ring_buff);
    size = DEFINE_MIN(len, size);
    if (size == 0) {
        return 0;
    }

    tocopy = DEFINE_MIN(ring_buff->buff_size - ring_buff->head, size);
    memcpy(pdata, &ring_buff->buf[ring_buff->head], tocopy);
    ring_buff->head += tocopy;
    size -= tocopy;

    if (size > 0) {
        memcpy(&pdata[tocopy], ring_buff->buf, size);
        ring_buff->head = size;
    }
    ring_buff->head = (ring_buff->head % ring_buff->buff_size);
    return tocopy + size;
}