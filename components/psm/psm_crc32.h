#ifndef __PSM_CRC32_H
#define __PSM_CRC32_H

#include <stdint.h>

uint32_t soft_crc32(const void *data, int data_size, uint32_t crc);
//uint32_t crc32(const void *data, int data_size, uint32_t crc);
void crc32_init(void);

#endif
