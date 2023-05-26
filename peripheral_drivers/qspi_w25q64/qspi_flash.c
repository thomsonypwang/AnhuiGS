#include "qspi_flash.h"
#include "project_pin_use_config.h"
#include "sys_log.h"
/**
 * @addtogroup HC32F460_DDL_Examples
 * @{
 */

/**
 * @addtogroup QSPI_Base
 * @{
 */

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
#define QSPI_FLASH_ENTER_XIP_MD         (0x20U)
#define QSPI_FLASH_EXIT_XIP_MD          (0xFFU)


#define	FLASH_DEFAULT_INDEX	3

const struct flash_device_config fl_dev_list[] = 
{
	{"W25Q80BL", 	0xef4014, 1 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"W25Q16CL", 	0xef4015, 2 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"W25Q32BV", 	0xef4016, 4 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"W25Q64CV", 	0xef4017, 8 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"W25Q128BV", 	0xef4018, 16 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"GD25Q16B", 	0xc84015, 2 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"GD25Q16C", 	0xc84015, 2 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"GD25Q32C", 	0xc84016, 4 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"GD25LQ32C", 	0xc86016, 4 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"GD25Q127C", 	0xc84018, 16 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25L8035E", 	0xc22014, 1 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25L3233F", 	0xc22016, 4 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25L6433F", 	0xc22017, 8 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25L12835F", 0xc22018, 16 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25R512F", 	0xc22810, 64 * KILO_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25R1035F", 	0xc22811, 128 * KILO_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25R2035F", 	0xc22812, 256 * KILO_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25R4035F", 	0xc22813, 512 * KILO_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25R8035F", 	0xc22814, 1 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25R1635F", 	0xc22815, 2 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25R3235F", 	0xc22816, 4 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25R6435F", 	0xc22817, 8 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25V512F", 	0xc22310, 64 * KILO_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25V1035F", 	0xc22311, 128 * KILO_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25V2035F", 	0xc22312, 256 * KILO_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25V4035F",	0xc22313, 512 * KILO_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25V8035F", 	0xc22314, 1 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
	{"MX25V1635F", 	0xc22315, 2 * MEGA_BYTE, 4 * KILO_BYTE, 64 * KILO_BYTE,256},
};

/*@} end of group FLASH_Private_Variables */

/** @defgroup FLASH_Global_Variables
 *  @{
 */
static const struct flash_device_config *g_fl_cfg = &fl_dev_list[FLASH_DEFAULT_INDEX];

static const struct flash_device_config *FLASH_GetConfigFromID(uint32_t jedecID)
{
	int i, count = sizeof(fl_dev_list) / sizeof(struct flash_device_config);
	for (i = 0; i < count; i++)
	{
		//log_i("fl_dev_list[%d]=0x%x, jedecID=0x%x",i,fl_dev_list[i].jedec_id,jedecID);
		if (fl_dev_list[i].jedec_id == jedecID)
		{
			return &fl_dev_list[i];
		}
	}
	return NULL;
}

int32_t FLASH_SetConfig(uint32_t jedecID)
{
	const struct flash_device_config *cfg;
	cfg = FLASH_GetConfigFromID(jedecID);
	if (cfg) 
	{
		g_fl_cfg = cfg;
		return LL_OK;
	}
	return LL_ERR;
}

const struct flash_device_config *FLASH_GetConfig(void)
{
	return g_fl_cfg;
}

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @defgroup QSPI_FLASH_Global_Functions QSPI_FLASH Global Functions
 * @{
 */
/**
 * @brief  Convert word to bytes.
 * @param  [in] u32Word                 The word value.
 * @param  [in] pu8Byte                 Pointer to the byte buffer.
 * @retval None
 */
static void QSPI_FLASH_WordToByte(uint32_t u32Word, uint8_t *pu8Byte)
{
    uint32_t u32ByteNum;
    uint8_t u8Count = 0U;

    u32ByteNum = QSPI_FLASH_ADDR_WIDTH;
    do 
	{
        pu8Byte[u8Count++] = (uint8_t)(u32Word >> (u32ByteNum * 8U)) & 0xFFU;
    } while ((u32ByteNum--) != 0UL);
}

/**
 * @brief  QSPI write instruction.
 * @param  [in] u8Instr                 The instruction code.
 * @param  [in] pu8Addr                 Pointer to the address buffer.
 * @param  [in] u32AddrLen              Size of address buffer.
 * @param  [in] pu8WriteBuf             Pointer to the write buffer.
 * @param  [in] u32BufLen               Size of write buffer.
 * @retval None
 */
static void QSPI_FLASH_WriteInstr(uint8_t u8Instr, uint8_t *pu8Addr, uint32_t u32AddrLen,uint8_t *pu8WriteBuf, uint32_t u32BufLen)
{
    uint32_t u32Count;

    QSPI_EnterDirectCommMode();
    QSPI_WriteDirectCommValue(u8Instr);
    if ((NULL != pu8Addr) && (0UL != u32AddrLen)) 
	{
        for (u32Count = 0UL; u32Count < u32AddrLen; u32Count++) 
		{
            QSPI_WriteDirectCommValue(pu8Addr[u32Count]);
        }
    }
    if ((NULL != pu8WriteBuf) && (0UL != u32BufLen)) 
	{
        for (u32Count = 0UL; u32Count < u32BufLen; u32Count++) 
		{
            QSPI_WriteDirectCommValue(pu8WriteBuf[u32Count]);
        }
    }
    QSPI_ExitDirectCommMode();
}

/**
 * @brief  QSPI read instruction.
 * @param  [in] u8Instr                 The instruction code.
 * @param  [in] pu8Addr                 Pointer to the address buffer.
 * @param  [in] u32AddrLen              Size of address buffer.
 * @param  [out] pu8ReadBuf             Pointer to the read buffer.
 * @param  [in] u32BufLen               Size of read buffer.
 * @retval None
 */
static void QSPI_FLASH_ReadInstr(uint8_t u8Instr, uint8_t *pu8Addr, uint32_t u32AddrLen, uint8_t *pu8ReadBuf, uint32_t u32BufLen)
{
    uint32_t u32Count;

    QSPI_EnterDirectCommMode();
    QSPI_WriteDirectCommValue(u8Instr);
    if ((NULL != pu8Addr) && (0UL != u32AddrLen)) 
	{
        for (u32Count = 0UL; u32Count < u32AddrLen; u32Count++) 
		{
            QSPI_WriteDirectCommValue(pu8Addr[u32Count]);
        }
    }
    if ((NULL != pu8ReadBuf) && (0UL != u32BufLen))		
	{
        for (u32Count = 0UL; u32Count < u32BufLen; u32Count++) 
		{
            pu8ReadBuf[u32Count] = QSPI_ReadDirectCommValue();
        }
    }
    QSPI_ExitDirectCommMode();
}

/**
 * @brief  QSPI check process done.
 * @param  u32Timeout                   The timeout times (ms).
 * @retval int32_t:
 *           - LL_OK: No errors occurred.
 *           - LL_ERR_TIMEOUT: Works timeout.
 */
static int32_t QSPI_FLASH_CheckProcessDone(uint32_t u32Timeout)
{
    uint8_t u8Status;
    uint32_t u32Count;
    int32_t i32Ret = LL_ERR_TIMEOUT;

    u32Count = u32Timeout * (HCLK_VALUE / 20000UL);
    QSPI_EnterDirectCommMode();
    QSPI_WriteDirectCommValue(W25Q64_RD_STATUS_REG1);
    while ((u32Count--) != 0UL) 
	{
        u8Status = QSPI_ReadDirectCommValue();
        if (0U == (u8Status & W25Q64_FLAG_BUSY)) 
		{
            i32Ret = LL_OK;
            break;
        }
    }
    QSPI_ExitDirectCommMode();

    return i32Ret;
}

/**
 * @brief  De-initializes QSPI.
 * @param  None
 * @retval None
 */
void QSPI_FLASH_DeInit(void)
{
    (void)QSPI_DeInit();
}

/**
 * @brief  Initialize the QSPI Flash.
 * @param  None
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: Invalid parameter
 */
void QSPI_FLASH_Init(void)
{
    stc_qspi_init_t stcQspiInit;
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDrv = PIN_HIGH_DRV;

	(void)GPIO_Init(W25Q64_CS_PORT,  W25Q64_CS_PIN,  &stcGpioInit);
    (void)GPIO_Init(W25Q64_SCLK_PORT, W25Q64_SCLK_PIN, &stcGpioInit);
    (void)GPIO_Init(W25Q64_DATA0_PORT, W25Q64_DATA0_PIN, &stcGpioInit);
    (void)GPIO_Init(W25Q64_DATA1_PORT, W25Q64_DATA1_PIN, &stcGpioInit);
    (void)GPIO_Init(W25Q64_DATA2_PORT, W25Q64_DATA2_PIN, &stcGpioInit);
    (void)GPIO_Init(W25Q64_DATA3_PORT, W25Q64_DATA3_PIN, &stcGpioInit);
	
    GPIO_SetFunc(W25Q64_CS_PORT,  W25Q64_CS_PIN,  W25Q64_CS_FUNC);
    GPIO_SetFunc(W25Q64_SCLK_PORT, W25Q64_SCLK_PIN, W25Q64_SCLK_FUNC);
    GPIO_SetFunc(W25Q64_DATA0_PORT, W25Q64_DATA0_PIN, W25Q64_DATA0_FUNC);
    GPIO_SetFunc(W25Q64_DATA1_PORT, W25Q64_DATA1_PIN, W25Q64_DATA1_FUNC);
    GPIO_SetFunc(W25Q64_DATA2_PORT, W25Q64_DATA2_PIN, W25Q64_DATA2_FUNC);
    GPIO_SetFunc(W25Q64_DATA3_PORT, W25Q64_DATA3_PIN, W25Q64_DATA3_FUNC);

    FCG_Fcg1PeriphClockCmd(QSPI_FLASH_CLK, ENABLE);
    (void)QSPI_StructInit(&stcQspiInit);
	
    stcQspiInit.u32ClockDiv       = QSPI_CLK_DIV8;
    stcQspiInit.u32ReadMode       = QSPI_FLASH_RD_MD;
    stcQspiInit.u32PrefetchMode   = QSPI_PREFETCH_MD_EDGE_STOP;
    stcQspiInit.u32DummyCycle     = QSPI_FLASH_RD_DUMMY_CYCLE;
    stcQspiInit.u32AddrWidth      = QSPI_FLASH_ADDR_WIDTH;
    stcQspiInit.u32SetupTime      = QSPI_QSSN_SETUP_ADVANCE_QSCK1P5;
    stcQspiInit.u32ReleaseTime    = QSPI_QSSN_RELEASE_DELAY_QSCK1P5;
    stcQspiInit.u32IntervalTime   = QSPI_QSSN_INTERVAL_QSCK2;
    (void)QSPI_Init(&stcQspiInit);
}

/**
 * @brief  Reads data from the QSPI memory.
 * @param  [in] u32Addr                 Read start address.
 * @param  [out] pu8ReadBuf             Pointer to the read buffer.
 * @param  [in] u32Size                 Size of the read buffer.
 * @retval int32_t:
 *           - LL_OK: Read succeeded
 *           - LL_ERR_INVD_PARAM: pu8ReadBuf == NULL or u32Size == 0U
 */
int32_t QSPI_FLASH_Read(uint32_t u32Addr, uint8_t *pu8ReadBuf, uint32_t u32Size)
{
    uint32_t u32Count = 0U;
    int32_t i32Ret = LL_OK;
    __IO uint8_t *pu8Read;

    u32Addr += QSPI_ROM_BASE;
    if ((NULL == pu8ReadBuf) || (0UL == u32Size) || ((u32Addr + u32Size) > QSPI_ROM_END)) 
	{
        i32Ret = LL_ERR_INVD_PARAM;
    } 
	else 
	{
		#if (QSPI_XIP_FUNC_ENABLE == DDL_ON)
			QSPI_XipModeCmd(QSPI_FLASH_ENTER_XIP_MD, ENABLE);
		#endif
        pu8Read = (__IO uint8_t *)u32Addr;
        while (u32Count < u32Size) 
		{
            pu8ReadBuf[u32Count++] = *pu8Read++;
			#if (QSPI_XIP_FUNC_ENABLE == DDL_ON)
				if (u32Count == (u32Size - 1U)) 
				{
					QSPI_XipModeCmd(QSPI_FLASH_EXIT_XIP_MD, DISABLE);
				}
			#endif
        }
    }

    return i32Ret;
}

/**
 * @brief  Writes data to the QSPI memory.
 * @param  [in] u32Addr                 Write start address.
 * @param  [in] pu8WriteBuf             Pointer to the write buffer.
 * @param  [in] u32Size                 Size of the write buffer.
 * @retval int32_t:
 *           - LL_OK: Write succeeded
 *           - LL_ERR_INVD_PARAM: pu8WriteBuf == NULL or u32Size == 0U
 */
int32_t QSPI_FLASH_Write(uint32_t u32Addr, uint8_t *pu8WriteBuf, uint32_t u32Size)
{
    uint32_t u32TempSize;
    uint8_t u8AddrBuf[4U];
    uint32_t u32AddrOffset = 0U;
    int32_t i32Ret = LL_OK;

    if ((NULL == pu8WriteBuf) || (0UL == u32Size) ) //|| ((u32Addr % W25Q64_PAGE_SIZE) != 0U)
	{
        i32Ret = LL_ERR_INVD_PARAM;
    } 
	else 
	{
        while (u32Size != 0UL) 
		{
            if (u32Size >= W25Q64_PAGE_SIZE) 
			{
                u32TempSize = W25Q64_PAGE_SIZE;
            } 
			else 
			{
                u32TempSize = u32Size;
            }
            QSPI_FLASH_WriteInstr(W25Q64_WR_ENABLE, NULL, 0U, NULL, 0U);
            QSPI_FLASH_WordToByte(u32Addr, u8AddrBuf);
            QSPI_FLASH_WriteInstr(W25Q64_PAGE_PROGRAM, u8AddrBuf, (QSPI_FLASH_ADDR_WIDTH + 1U),
                                  (uint8_t *)&pu8WriteBuf[u32AddrOffset], u32TempSize);
            i32Ret = QSPI_FLASH_CheckProcessDone(500U);
            if (i32Ret != LL_OK) 
			{
                break;
            }
            u32Addr       += u32TempSize;
            u32AddrOffset += u32TempSize;
            u32Size       -= u32TempSize;
        }
    }

    return i32Ret;
}

/****************************************************************************//**
 * @brief      Write flash with any address and size
 *
 * @param[in]  programMode:  Flash program mode to be set
 * @param[in]  address:  Page address
 * @param[in]  buffer:  Buffer data to be programmed to flash
 * @param[in]  num:  Number of data to be programmed to flash
 *
 * @return     DSUCCESS or ERROR
 *
 *******************************************************************************/
int32_t FLASH_Write(uint32_t address, uint8_t *buffer, uint32_t num) 
{
	uint8_t *pBuf;
	uint32_t begPgNum;
	uint32_t endPgNum;
	uint32_t step;
	uint32_t addrCur;
	uint32_t i;
	uint32_t endPgAddr;
	int32_t funcStatus = LL_OK;  

	pBuf = buffer;
	addrCur = address;
	
	begPgNum = FLASH_PAGE_NUM(address);/* Get page number of start address */	
	endPgNum = FLASH_PAGE_NUM(address + num - 1);/* Get page number of end address */
	
	if(begPgNum == endPgNum)/* Both start address and end address are within the same page */
	{
		return( QSPI_FLASH_Write(address, buffer, num) );
	} 
	else /* Start address and end address are not in the same page */
	{		
		endPgAddr = (g_fl_cfg->page_size * (FLASH_PAGE_NUM(address) + 1) - 1);/* For first page */
		step = endPgAddr - address + 1;
		funcStatus = QSPI_FLASH_Write(address, pBuf, step);
		if(funcStatus != LL_OK)
		{
			return LL_ERR;
		}
		pBuf += step;
		addrCur += step;
		for(i=begPgNum+1; i<=endPgNum; i++)
		{
			if(i == endPgNum)/* For last page */
			{
				step = (address + num) & 0xFF;
				/* If step is 0, the last page has 256 bytes data to be writen ( num of data is 0x100 ) */
				if(step == 0)
				{
					step = 0x100;
				}
				return( QSPI_FLASH_Write(addrCur, pBuf, step) );
			} 
			else
			{
				funcStatus = QSPI_FLASH_Write(addrCur, pBuf, g_fl_cfg->page_size);
				if(funcStatus != LL_OK)
				{
					return LL_ERR;
				}
				pBuf += g_fl_cfg->page_size;
				addrCur += g_fl_cfg->page_size;
			}
		}
	}
	return funcStatus;
}

/**
 * @brief  Erase sector of the QSPI memory.
 * @param  [in] u32SectorAddr           The start address of the target sector.
 * @retval int32_t:
 *           - LL_OK: No errors occurred
 *           - LL_ERR_TIMEOUT: Erase sector timeout
 */
int32_t QSPI_FLASH_EraseSector(uint32_t u32SectorAddr)
{
    uint8_t u8AddrBuf[4U];

    QSPI_FLASH_WriteInstr(W25Q64_WR_ENABLE, NULL, 0U, NULL, 0U);
    QSPI_FLASH_WordToByte(u32SectorAddr, u8AddrBuf);
    QSPI_FLASH_WriteInstr(W25Q64_SECTOR_ERASE, u8AddrBuf, (QSPI_FLASH_ADDR_WIDTH + 1U), NULL, 0U);
    return QSPI_FLASH_CheckProcessDone(500U);
}

/****************************************************************************//**
 * @brief      Flash 32KB block erase
 *
 * @param[in]  sectorNumber:  block number to be erased
 *
 * @return     DSUCCESS or ERROR
 *
 *******************************************************************************/
int32_t  QSPI_FLASH_Erase_Block32K(uint32_t blockNumber) 
{
    uint8_t u8AddrBuf[4U];

    QSPI_FLASH_WriteInstr(W25Q64_WR_ENABLE, NULL, 0U, NULL, 0U);
    QSPI_FLASH_WordToByte(blockNumber, u8AddrBuf);
    QSPI_FLASH_WriteInstr(W25Q64_BLK_ERASE_32KB, u8AddrBuf, (QSPI_FLASH_ADDR_WIDTH + 1U), NULL, 0U);
    return QSPI_FLASH_CheckProcessDone(500U);
}

/****************************************************************************//**
 * @brief      Flash 64KB block erase
 *
 * @param[in]  sectorNumber:  block number to be erased
 *
 * @return     DSUCCESS or ERROR
 *
 *******************************************************************************/
int32_t QSPI_FLASH_Erase_Block64K(uint32_t blockNumber) 
{
    uint8_t u8AddrBuf[4U];

    QSPI_FLASH_WriteInstr(W25Q64_WR_ENABLE, NULL, 0U, NULL, 0U);
    QSPI_FLASH_WordToByte(blockNumber, u8AddrBuf);
    QSPI_FLASH_WriteInstr(W25Q64_BLK_ERASE_64KB, u8AddrBuf, (QSPI_FLASH_ADDR_WIDTH + 1U), NULL, 0U);
    return QSPI_FLASH_CheckProcessDone(500U);
}

/****************************************************************************//**
 * @brief      Erase specfied address of the flash
 *
 * @param[in]  startAddr:  Start address to be erased
 * @param[in]  endAddr:  End address to be erased
 *
 * @return     DSUCCESS or ERROR
 *
 *******************************************************************************/
int32_t FLASH_Erase(uint32_t startAddr, uint32_t endAddr) 
{
	int ret;
	uint32_t sectorNumber, blockNumber, length, validStart;

	length = endAddr - startAddr + 1;

	while (length != 0) 
	{
		if ((startAddr & (FLASH_64K_BLOCK_SIZE - 1)) == 0 && length > (FLASH_64K_BLOCK_SIZE - g_fl_cfg->sector_size)) 
		{
			/* Address is a multiple of 64K and length is > (64K block -4K sector)
			* So directly erase 64K from this address */
			blockNumber = startAddr / FLASH_64K_BLOCK_SIZE;
			ret = QSPI_FLASH_Erase_Block64K(blockNumber* FLASH_64K_BLOCK_SIZE);
			endAddr = startAddr + FLASH_64K_BLOCK_SIZE;
		} 
		else if ((startAddr & (FLASH_32K_BLOCK_SIZE - 1)) == 0 &&length > (FLASH_32K_BLOCK_SIZE - g_fl_cfg->sector_size)) 
		{
			/* Address is a multiple of 32K and length is > (32K block -4K sector)
			* So directly erase 32K from this address */
			blockNumber = startAddr / FLASH_32K_BLOCK_SIZE;
			ret = QSPI_FLASH_Erase_Block32K(blockNumber* FLASH_32K_BLOCK_SIZE);
			endAddr = startAddr + FLASH_32K_BLOCK_SIZE;
		} 
		else 
		{
			/* Find 4K aligned address and erase 4K sector */
			validStart = startAddr - (startAddr &(g_fl_cfg->sector_size - 1));
			sectorNumber = validStart / g_fl_cfg->sector_size;
			ret = QSPI_FLASH_EraseSector(sectorNumber* g_fl_cfg->sector_size);
			endAddr = validStart + g_fl_cfg->sector_size;
		}

		/* If erase operation fails then return error */
		if (ret != LL_OK)
		{
			return LL_ERR;
		}
		/* Calculate the remaining length that is to be erased yet */
		if (length < (endAddr - startAddr))
		{
			length = 0;
		}
		else
		{
			length -= (endAddr - startAddr);
		}
		startAddr = endAddr;
	}
	return LL_OK;
}

/**
 * @brief  Erase chip of the QSPI memory.
 * @param  None
 * @retval int32_t:
 *           - LL_OK: No errors occurred
 *           - LL_ERR_TIMEOUT: Erase sector timeout
 */
int32_t QSPI_FLASH_EraseChip(void)
{
    QSPI_FLASH_WriteInstr(W25Q64_WR_ENABLE, NULL, 0U, NULL, 0U);
    QSPI_FLASH_WriteInstr(W25Q64_CHIP_ERASE, NULL, 0U, NULL, 0U);
    return QSPI_FLASH_CheckProcessDone(5000U);
}

/**
 * @brief  Get the UID of the QSPI memory.
 * @param  [in] pu8UID                  The UID of the QSPI memory.
 * @retval None
 */
void QSPI_FLASH_GetUniqueID(uint8_t *pu8UID)
{
    uint32_t i;
    uint8_t u8Dummy[4U];

    /* Fill the dummy values */
    for (i = 0UL; i < 4UL; i++)
    {
        u8Dummy[i] = 0xFFU;
    }
    QSPI_FLASH_ReadInstr(W25Q64_RD_UNIQUE_ID, u8Dummy, 4U, pu8UID, W25Q64_UNIQUE_ID_SIZE);
}


uint32_t QSPI_FLASH_Getjedecid(void)
{
    uint32_t i;
	uint32_t jedecID = 0;
	uint8_t jedecID_buf[4];

    QSPI_FLASH_ReadInstr(W25Q64_JEDEC_ID, NULL, 0, jedecID_buf, 3);
		
	jedecID = jedecID_buf[0];	
	jedecID <<= 8;
	jedecID |= jedecID_buf[1];
	jedecID <<= 8;
	jedecID |= jedecID_buf[2];

	return jedecID;	
}


/**
 * @}
 */

/**
 * @}
 */

/**
* @}
*/

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
