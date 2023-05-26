#include "w5500_dir.h"
#include "project_pin_use_config.h"
#include "sys_log.h"

#include "wizchip_conf.h"
#include "sys_os.h"

#define W5500_SPI_TIMEOUT                 ((HCLK_VALUE / 1000UL))

void w5500_io_init(void)
{
	stc_gpio_init_t stcGpioInit;

	(void)GPIO_StructInit(&stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_OUT;
	(void)GPIO_Init(W5500_REST_PORT, W5500_REST_PIN, &stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	(void)GPIO_Init(W5500_INT_PORT, W5500_INT_PIN, &stcGpioInit);
	
	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_OUT;
	(void)GPIO_Init(W5500_CS_PORT, W5500_CS_PIN, &stcGpioInit);
	
    stcGpioInit.u16PinDrv = PIN_HIGH_DRV;
    (void)GPIO_Init(W5500_MOSI_PORT,  W5500_MOSI_PIN,  &stcGpioInit);
    (void)GPIO_Init(W5500_MISO_PORT, W5500_MISO_PIN, &stcGpioInit);
	(void)GPIO_Init(W5500_SCLK_PORT, W5500_SCLK_PIN, &stcGpioInit);
	
	GPIO_SetFunc(W5500_MOSI_PORT,  W5500_MOSI_PIN,  W5500_MOSI_FUNC);
	GPIO_SetFunc(W5500_MISO_PORT,  W5500_MISO_PIN,  W5500_MISO_FUNC);
	GPIO_SetFunc(W5500_SCLK_PORT,  W5500_SCLK_PIN,  W5500_SCLK_FUNC);
}

void w5500_spi_init(void)
{
	stc_spi_init_t stcSpiInit;
	stc_spi_delay_t stcSpiDelayCfg;
	
    /* Configuration SPI */
    FCG_Fcg1PeriphClockCmd(W5500_CLK_ID, ENABLE);
    SPI_StructInit(&stcSpiInit);
    stcSpiInit.u32WireMode          = SPI_4_WIRE;
    stcSpiInit.u32TransMode         = SPI_FULL_DUPLEX;
    stcSpiInit.u32MasterSlave       = SPI_MASTER;
    stcSpiInit.u32Parity            = SPI_PARITY_INVD;
	stcSpiInit.u32ModeFaultDetect  = SPI_MD_FAULT_DETECT_DISABLE;
    stcSpiInit.u32SpiMode           = SPI_MD_0;
    stcSpiInit.u32BaudRatePrescaler = SPI_BR_CLK_DIV4;
    stcSpiInit.u32DataBits          = SPI_DATA_SIZE_8BIT;
    stcSpiInit.u32FirstBit          = SPI_FIRST_MSB;
    stcSpiInit.u32FrameLevel        = SPI_1_FRAME;
    (void)SPI_Init(W5500_SPI_ID, &stcSpiInit);
	
	(void)SPI_DelayStructInit(&stcSpiDelayCfg);
	stcSpiDelayCfg.u32IntervalDelay = SPI_INTERVAL_TIME_8SCK;
    stcSpiDelayCfg.u32ReleaseDelay  = SPI_RELEASE_TIME_8SCK;
    stcSpiDelayCfg.u32SetupDelay    = SPI_SETUP_TIME_3SCK;
    (void)SPI_DelayTimeConfig(W5500_SPI_ID, &stcSpiDelayCfg);
    SPI_Cmd(W5500_SPI_ID, ENABLE);
}

/**
*@brief		W5500复位设置函数
*@param		无
*@return	无
*/
void w5500_reset(void)
{
    GPIO_ResetPins(W5500_REST_PORT, W5500_REST_PIN);
    os_thread_sleep(os_msec_to_ticks(100)); 
    GPIO_SetPins(W5500_REST_PORT, W5500_REST_PIN);
    os_thread_sleep(os_msec_to_ticks(100)); 
}


/**
* @brief? 进入临界区
* @retval None
*/
void w5500_CrisEnter(void)
{
    vPortEnterCritical();
}

/**
* @brief? 退出临界区
* @retval None
*/
void w5500_CrisExit(void)
{
    vPortExitCritical();
}

/**
* @brief? 片选信号输出低电平
* @retval None
*/
void w5500_cs_select(void)
{
		//GPIO_ResetPins(W5500_CS1_PORT, W5500_CS1_PIN);
		GPIO_ResetPins(W5500_CS_PORT, W5500_CS_PIN);
}

/**
* @brief 片选信号输出高电平
* @retval None
*/
void w5500_cs_deselect(void)
{
		GPIO_SetPins(W5500_CS_PORT, W5500_CS_PIN);
		//GPIO_SetPins(W5500_CS1_PORT, W5500_CS1_PIN);
}

/**
  * @brief  写1字节数据到SPI总线
  * @param  TxData 写到总线的数据
  * @retval None
  */
void w5500_write_byte(uint8_t TxData)
{        
	uint8_t tmp[2];
	tmp[0]=TxData;
	SPI_Trans(W5500_SPI_ID, tmp, 1, W5500_SPI_TIMEOUT);
}


/**
  * @brief  从SPI总线读取1字节数据
  * @retval 读到的数据
  */
uint8_t w5500_read_byte(void)
{         
	uint8_t tmp[2];	
	
	SPI_Receive(W5500_SPI_ID, &tmp, 1, W5500_SPI_TIMEOUT);
	return tmp[0];
}

/**
 * @brief  BSP SPI transmit data.
 * @param  [in]  pu8TxBuf               The data buffer that to be transmitted.
 * @param  [in]  u32Size                Number of data bytes to be transmitted.
 * @retval int32_t:
 *           - LL_OK:                   Data transmission successful.
 *           - LL_ERR_TIMEOUT:          Data transmission timeout.
 */
void w5500_write_buff(uint8_t *pu8TxBuf, uint16_t u32Size)
{
	SPI_Trans(W5500_SPI_ID, pu8TxBuf, u32Size, W5500_SPI_TIMEOUT);
}

/**
 * @brief  BSP SPI receive data.
 * @param  [in]  pu8RxBuf               The buffer that received data to be stored.
 * @param  [in]  u32Size                Number of data bytes to be received.
 * @retval int32_t:
 *           - LL_OK:                   Data receive successful.
 *           - LL_ERR_TIMEOUT:          Data receive timeout.
 */
void w5500_read_buff(uint8_t *pu8RxBuf, uint16_t u32Size)
{
	SPI_Receive(W5500_SPI_ID, pu8RxBuf, u32Size, W5500_SPI_TIMEOUT);
}


//注缠函数
void w5500_register_function(void)
{ 
    reg_wizchip_cris_cbfunc(w5500_CrisEnter, w5500_CrisExit);   //注册临界区函数
	reg_wizchip_cs_cbfunc(w5500_cs_select, w5500_cs_deselect);//注册SPI片选信号函数
    reg_wizchip_spi_cbfunc(w5500_read_byte, w5500_write_byte);    //注册读写函数
	reg_wizchip_spiburst_cbfunc(w5500_read_buff, w5500_write_buff);
}


//初始化芯片参数
void w5500_parameters_configuration(void)
{
    //uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2},{2,2,2,2,2,2,2,2}};
	uint8_t memsize[2][8] = {{4,2,2,0,2,2,2,2},{4,2,2,0,2,2,2,2}};      
		
    //WIZCHIP SOCKET缓存区初始化
    if(ctlwizchip(CW_INIT_WIZCHIP,(void*)memsize) == -1)
	{
        log_e("w5500","WIZCHIP Initialized fail.");
    }
}

int wizchip_check(void)
{
    /* Read version register */
    if (getVERSIONR() != 0x04)
    {
        log_e("w5500"," ACCESS ERR : VERSION != 0x04, read value = 0x%02x", getVERSIONR());
		return -1;
    }
	return 0;
}
