#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "flash_drv.h"
#include "qspi_flash.h"

#include "sys_os.h"

#define fl_e(...)	log_e("fl", ##__VA_ARGS__)
#define fl_w(...)	log_w("fl", ##__VA_ARGS__)

#ifdef DBG_ENABLE_FLASH
	#define fl_d(...) 		log("fl", ##__VA_ARGS__)
#else
	#define fl_d(...)
#endif

#define FLASH_PERIPH_WE               (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM)
#define FLASH_PERIPH_WP               (LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_SRAM)

static os_mutex_t iflash_mutex;

/* Calling wrapper for 'mflash_drv_sector_erase_internal'.
 * Erase one sector starting at 'sector_addr' - must be sector aligned.
 */
int flash_drv_sector_erase(uint32_t sector_addr)
{
	int ret = 0;

	if (0 == mflash_drv_is_sector_aligned(sector_addr))
	{
		return SYS_FAIL;
	}

	ret = os_mutex_get(&iflash_mutex, OS_WAIT_FOREVER);
	if (ret == SYS_FAIL)
	{
		return ret;
	}
	ret =FLASH_Erase(sector_addr, sector_addr + MFLASH_SECTOR_SIZE - 1U); 
	os_mutex_put(&iflash_mutex);
	if (ret == LL_OK)
	{
		return SYS_OK;
	}
	else
	{
		return SYS_FAIL;
	}
}

/* Calling wrapper for 'mflash_drv_page_program_internal'.
 * Write 'data' to 'page_addr' - must be page aligned.
 * NOTE: Don't try to store constant data that are located in XIP !!
 */
int flash_drv_page_program(uint32_t page_addr, uint32_t *data)
{
	int ret;

	if (0 == mflash_drv_is_page_aligned(page_addr))
	{
		return SYS_FAIL;
	}

	if (!data)
	{
		return SYS_FAIL;
	}

	ret = os_mutex_get(&iflash_mutex, OS_WAIT_FOREVER);
	if (ret == SYS_FAIL)
	{
		return ret;
	}
	ret = QSPI_FLASH_Write(page_addr, (uint8_t *)data, MFLASH_PAGE_SIZE);
	os_mutex_put(&iflash_mutex);

	if (ret == LL_OK)
	{
		return SYS_OK;
	}	
	else 
	{
		return SYS_FAIL;
	}
}

/* API - Get pointer to FLASH region */
void *flash_drv_phys2log(uint32_t addr, uint32_t len)
{
    return (void *)(addr + MFLASH_BASE_ADDRESS );
}

/* API - Get pointer to FLASH region */
uint32_t flash_drv_log2phys(void *ptr, uint32_t len)
{
    return ((uint32_t)ptr  - MFLASH_BASE_ADDRESS);
}

int flash_drv_erase_chip(void)
{
	int ret = 0;

	ret = os_mutex_get(&iflash_mutex, OS_WAIT_FOREVER);
	if (ret == SYS_FAIL)
	{
		return ret;
	}
	ret = QSPI_FLASH_EraseChip();
	os_mutex_put(&iflash_mutex);
	if (ret == LL_OK)
	{
		return SYS_OK;
	}
	else
	{
		return SYS_FAIL;
	}
}

int flash_drv_erase(uint32_t addr, uint32_t len)
{
	int ret = 0;
	uint32_t chunk_size = W25Q64_SECTOR_SIZE; 
	uint32_t erased_size = 0;

	ret = os_mutex_get(&iflash_mutex, OS_WAIT_FOREVER);
	if (ret == SYS_FAIL)
	{
		return ret;
	}
	ret =FLASH_Erase(addr, addr + len - 1U); 
	os_mutex_put(&iflash_mutex);
	if (ret == LL_OK)
	{
		return SYS_OK;
	}
	else
	{
		return SYS_FAIL;
	}
}

int flash_drv_read(uint32_t addr, uint32_t *buffer, uint32_t len)
{
	int ret;

	if (len == 0)
	{
		return 0;
	}		
	if (!buffer)
	{
		return SYS_FAIL;
	}
	ret = os_mutex_get(&iflash_mutex, OS_WAIT_FOREVER);
	if (ret == SYS_FAIL)
	{
		return ret;
	}
	ret = QSPI_FLASH_Read(addr, (uint8_t *)buffer, len);

	os_mutex_put(&iflash_mutex);

	if (ret == LL_OK)
	{
		return SYS_OK;
	}	
	else 
	{
		return SYS_FAIL;
	}
}

int flash_drv_write(uint32_t addr, uint32_t *buffer, uint32_t len)
{
	int ret;

	if (len == 0)
	{
		return 0;
	}		
	if (!buffer)
	{
		return SYS_FAIL;
	}

	ret = os_mutex_get(&iflash_mutex, OS_WAIT_FOREVER);
	if (ret == SYS_FAIL)
	{
		return ret;
	}
	ret = FLASH_Write(addr, (uint8_t *)buffer, len);
	os_mutex_put(&iflash_mutex);

	if (ret == LL_OK)
	{
		return SYS_OK;
	}	
	else 
	{
		return SYS_FAIL;
	}
}

uint32_t flash_to_qspi_read_jedecid(void)
{
	return QSPI_FLASH_Getjedecid();
}

uint64_t flash_to_qspi_read_uniqid(void)
{
	uint64_t id;
	uint8_t u8UID[W25Q64_UNIQUE_ID_SIZE] = {0U};

	//Disable_IRQs();
	QSPI_FLASH_GetUniqueID(u8UID);
	id = u8UID[0];
	id <<= 8;
	id |= u8UID[1];
	id <<= 8;
	id |= u8UID[2];
	id <<= 8;
	id |= u8UID[3];
	id <<= 8;
	id |=u8UID[4];
	id <<= 8;
	id |= u8UID[5];
	id <<= 8;
	id |=	u8UID[6];
	id <<= 8;
	id |=	u8UID[7];  
	//Enable_IRQs();
	
	return id;
}

int flash_drv_init(void)
{
	int ret;
	uint32_t jedec_id;

	ret = os_mutex_create(&iflash_mutex, "flash", OS_MUTEX_INHERIT);
	if (ret == SYS_FAIL)
	{
		return SYS_FAIL;
	}
    LL_PERIPH_WE(FLASH_PERIPH_WE);/* Peripheral registers write unprotected */
    QSPI_FLASH_Init();/* Configure QSPI */
    LL_PERIPH_WP(FLASH_PERIPH_WP);/* Peripheral registers write protected */
	os_thread_sleep(os_msec_to_ticks(10));
	
	
	jedec_id = flash_to_qspi_read_jedecid();/* Read the JEDEC id of the primary flash that is connected */
	
	ret = FLASH_SetConfig(jedec_id);/* Set the flash configuration as per the JEDEC id */
	if (ret != LL_OK)/* In case of an error, print error message and continue */
	{
		fl_e("Flash JEDEC ID 0x%x not present in supported flash list,using default config for W25Q32BV", jedec_id);
		ret=SYS_FAIL;
	}
	else
	{
		ret=SYS_OK;
	}
	const struct flash_device_config *fl_conf = FLASH_GetConfig();
	fl_d("Flash configuration:");
	fl_d("Name: %s, JEDEC ID: 0x%x\r\n"
	      "Chip Size: 0x%x, Sector Size: 0x%x\r\n"
	      "Block Size: 0x%x, Page Size: 0x%x", fl_conf->name,
	      fl_conf->jedec_id, fl_conf->chip_size,
	      fl_conf->sector_size, fl_conf->block_size,
	      fl_conf->page_size);
	fl_d("Flash Unique ID is %llx", flash_to_qspi_read_uniqid());
	
	return ret;
}
