#ifndef _FLASH_DRV_H_
#define _FLASH_DRV_H_

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "sys_log.h"

#define mflash_drv_is_page_aligned(x)   (((x) % (MFLASH_PAGE_SIZE)) == 0)
#define mflash_drv_is_sector_aligned(x) (((x) % (MFLASH_SECTOR_SIZE)) == 0)

#define MFLASH_FILE_BASEADDR (0x00130000) /* mem address 0x1F130000 */
#define MFLASH_FILE_SIZE     (MFLASH_SECTOR_SIZE)

/* Flash constants */
#ifndef MFLASH_SECTOR_SIZE
#define MFLASH_SECTOR_SIZE (4096U)
#endif

#ifndef MFLASH_PAGE_SIZE
#define MFLASH_PAGE_SIZE (256U)
#endif

/* Device specific settings */
#ifndef MFLASH_QSPI
#define MFLASH_QSPI (QSPI)
#endif

#ifndef MFLASH_BASE_ADDRESS
#define MFLASH_BASE_ADDRESS QSPI_ROM_BASE
#endif

#define FLASH_SIZE 0x00001000U /* 4MB in KB unit */


int flash_drv_sector_erase(uint32_t sector_addr);
int flash_drv_page_program(uint32_t page_addr, uint32_t *data);

/* API - Get pointer to FLASH region */
void *flash_drv_phys2log(uint32_t addr, uint32_t len);
/* API - Get pointer to FLASH region */
uint32_t flash_drv_log2phys(void *ptr, uint32_t len);
int flash_drv_erase_chip(void);
int flash_drv_erase(uint32_t addr, uint32_t len);
int flash_drv_read(uint32_t addr, uint32_t *buffer, uint32_t len);
int flash_drv_write(uint32_t addr, uint32_t *buffer, uint32_t len);
uint32_t flash_to_qspi_read_jedecid(void);
uint64_t flash_to_qspi_read_uniqid(void);
int flash_drv_init(void);


#endif 