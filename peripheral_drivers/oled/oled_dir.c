#include "oled_dir.h"
#include <stdio.h>
#include "stdlib.h"
#include "oledfont.h"

#include "sys_os.h"
#include "project_pin_use_config.h"

#define BSP_SPI_TIMEOUT                 ((HCLK_VALUE / 1000UL))

uint8_t OLED_GRAM[128][8];

void oled_io_init(void)
{
	stc_gpio_init_t stcGpioInit;

	(void)GPIO_StructInit(&stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_OUT;
	(void)GPIO_Init(OLED_DC_PORT, OLED_DC_PIN, &stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_OUT;
	(void)GPIO_Init(OLED_RES_PORT, OLED_RES_PIN, &stcGpioInit);

    stcGpioInit.u16PinDrv = PIN_HIGH_DRV;

    (void)GPIO_Init(OLED_SDA_PORT,  OLED_SDA_PIN,  &stcGpioInit);
    (void)GPIO_Init(OLED_SCLK_PORT, OLED_SCLK_PIN, &stcGpioInit);
	
	GPIO_SetFunc(OLED_SDA_PORT,  OLED_SDA_PIN,  OLED_SDA_FUNC);
	GPIO_SetFunc(OLED_SCLK_PORT, OLED_SCLK_PIN, OLED_SCLK_FUNC);
}

void oled_spi_init(void)
{
	stc_spi_init_t stcSpiInit;
	stc_spi_delay_t stcSpiDelayCfg;
	
    /* Clear initialize structure */
    (void)SPI_StructInit(&stcSpiInit);
    (void)SPI_DelayStructInit(&stcSpiDelayCfg);
	

    /* Configuration SPI */
    FCG_Fcg1PeriphClockCmd(OLED_CLK_ID, ENABLE);
    SPI_StructInit(&stcSpiInit);
    stcSpiInit.u32WireMode          = SPI_3_WIRE;
    stcSpiInit.u32TransMode         = SPI_SEND_ONLY;
    stcSpiInit.u32MasterSlave       = SPI_MASTER;
	 stcSpiInit.u32ModeFaultDetect  = SPI_MD_FAULT_DETECT_DISABLE;
    stcSpiInit.u32Parity            = SPI_PARITY_INVD;
    stcSpiInit.u32SpiMode           = SPI_MD_0;
    stcSpiInit.u32BaudRatePrescaler = SPI_BR_CLK_DIV16;
    stcSpiInit.u32DataBits          = SPI_DATA_SIZE_8BIT;
    stcSpiInit.u32FirstBit          = SPI_FIRST_MSB;
    stcSpiInit.u32FrameLevel        = SPI_1_FRAME;
	
    (void)SPI_Init(OLED_SPI_ID, &stcSpiInit);
    stcSpiDelayCfg.u32IntervalDelay = SPI_INTERVAL_TIME_8SCK;
    stcSpiDelayCfg.u32ReleaseDelay  = SPI_RELEASE_TIME_8SCK;
    stcSpiDelayCfg.u32SetupDelay    = SPI_SETUP_TIME_1SCK;
    (void)SPI_DelayTimeConfig(OLED_SPI_ID, &stcSpiDelayCfg);
    SPI_Cmd(OLED_SPI_ID, ENABLE);
}
/*******************************************************************************
function:
            Hardware reset
*******************************************************************************/
void oled_reset(void)
{
	GPIO_SetPins(OLED_RES_PORT, OLED_RES_PIN);
	os_thread_sleep(os_msec_to_ticks(100)); 
	GPIO_ResetPins(OLED_RES_PORT, OLED_RES_PIN);
	os_thread_sleep(os_msec_to_ticks(100)); 
	GPIO_SetPins(OLED_RES_PORT, OLED_RES_PIN);
	os_thread_sleep(os_msec_to_ticks(100)); 
}

void oled_spi_write(uint8_t *data_buf, uint16_t size)
{
	SPI_Trans(OLED_SPI_ID, data_buf, size, BSP_SPI_TIMEOUT);
}

/*******************************************************************************
function:
            Write register address and data
*******************************************************************************/
void oled_write_reg(uint8_t Reg)
{
	GPIO_ResetPins(OLED_DC_PORT, OLED_DC_PIN);
//    OLED_CS_0;
	oled_spi_write(&Reg,1);

//    OLED_CS_1;
}

void oled_write_data(uint8_t Data)
{
	GPIO_SetPins(OLED_DC_PORT, OLED_DC_PIN);
//    OLED_CS_0;
	oled_spi_write(&Data,1);
//    OLED_CS_1;
	GPIO_ResetPins(OLED_DC_PORT, OLED_DC_PIN);
}

void oled_write_data_all(uint8_t *data_buf, uint16_t size)
{
	GPIO_SetPins(OLED_DC_PORT, OLED_DC_PIN);
//    OLED_CS_0;
	oled_spi_write(data_buf,size);
//    OLED_CS_1;
	GPIO_ResetPins(OLED_DC_PORT, OLED_DC_PIN);
}

/*******************************************************************************
function:
		Common register initialization
*******************************************************************************/
void oled_init_reg(void)
{
    oled_write_reg(0xAE);//�ر���ʾ
	oled_write_reg(0x2e);	//�رչ���
	
    oled_write_reg(0x02); //���õ��е�ַ
    oled_write_reg(0x10); //���ø��е�ַ
    oled_write_reg(0x40); //������ʼ�е�ַ
    oled_write_reg(0xB0); //����ҳ��ַ Mode,0-7	   

	oled_write_reg(0x81);// �Աȶ����ã�����������
	oled_write_reg(0xff);//  265
	////////////////////////////////////////////////////////////////////////////	
	#ifdef MIRROR_HORIZ
		oled_write_reg(0xA0); // Mirror horizontally
	#else
		oled_write_reg(0xA1); //���öΣ�SEG������ʼӳ���ַ��column��127��ַ��SEG0�ĵ�ַ
	#endif
	
	#ifdef INVERSE_COLOR
		oled_write_reg(0xA7); //--set inverse color
	#else
		oled_write_reg(0xA6); //--set normal color//������ʾ��0xa7����ʾ
	#endif	
	////////////////////////////////////////////////////////////////////////////
		// Set multiplex ratio.
	#if (OLED_HEIGHT == 128)
		// Found in the Luma Python lib for SH1106.
		oled_write_reg(0xFF);
	#else
		oled_write_reg(0xA8); //--set multiplex ratio(1 to 64) - CHECK//��������·��
	#endif

	#if (OLED_HEIGHT == 32)
		oled_write_reg(0x1F); //
	#elif (OLED_HEIGHT == 64)
		oled_write_reg(0x3F); ////1/64duty
	#elif (OLED_HEIGHT == 128)
		oled_write_reg(0x3F); // Seems to work for 128px high displays too.
	#else
		#error "Only 32, 64, or 128 lines of height are supported!"
	#endif
	////////////////////////////////////////////////////////////////////////////
	#ifdef MIRROR_VERT
		oled_write_reg(0xC0); // Mirror vertically
	#else
		oled_write_reg(0xC8); //Set COM Output Scan Direction//��ӳ��ģʽ��COM[N-1]~COM0ɨ��
	#endif	
	
    oled_write_reg(0xD3); //-set display offset - CHECK//������ʾƫ��
    oled_write_reg(0x00); //-not offset	//��ƫ��
	
    oled_write_reg(0xD5); //����������Ƶ��Ĭ�ϣ�
    oled_write_reg(0x80); //--set divide ratio	
	
	oled_write_reg(0xD8);	//���� area color mode off��û�У�
	oled_write_reg(0x05);
	
	oled_write_reg(0xD6);	//�Ŵ���ʾ
	oled_write_reg(0x00);	
	
	oled_write_reg(0xD9); //--set pre-charge period
	oled_write_reg(0xf1); //
	
	oled_write_reg(0xDA); //--set com pins hardware configuration - CHECK
	#if (OLED_HEIGHT == 32)
		oled_write_reg(0x02);
	#elif (OLED_HEIGHT == 64)
		oled_write_reg(0x12);
	#elif (OLED_HEIGHT == 128)
		oled_write_reg(0x12);
	#else
		#error "Only 32, 64, or 128 lines of height are supported!"
	#endif
	
	oled_write_reg(0xDB); //--set vcomh���� Vcomh���ɵ������ȣ�Ĭ�ϣ�
    oled_write_reg(0x40); //0x20,0.77xVcc
	oled_write_reg(0x20);//-Set Page Addressing Mode (0x00/0x01/0x02)
	oled_write_reg(0x02);//	
	oled_write_reg(0x8D); //--set DC-DC enable//����OLED��ɱ�
    oled_write_reg(0x14); //����ʾ
	
	oled_write_reg(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	oled_write_reg(0xA6);	// Disable Inverse Display On (0xa6/a7)
}

/********************************************************************************
function:
            initialization
********************************************************************************/
void oled_init(void)
{  
	oled_io_init();
	oled_spi_init();
    oled_reset();//Hardware reset  
    oled_init_reg();//Set the initialization register
    oled_clear();
	os_thread_sleep(os_msec_to_ticks(10));
	oled_write_reg(0xaf);//Turn on the OLED display
	os_thread_sleep(os_msec_to_ticks(100)); 
}

/********************************************************************************
function:
			Clear screen
********************************************************************************/
void oled_clear(void)
{
	uint16_t i;
	uint8_t TxBuf_tmp[128];
	
	memset(TxBuf_tmp, 0, 128);
	uint8_t page;
    for (page=0; page<8; page++) 
	{        
        oled_write_reg(0xB0 + page);/* set page address */    
		oled_write_reg(0x02);/* set low column address */
		oled_write_reg(0x10);/* set high column address */ 			
		oled_write_data_all(TxBuf_tmp,128);
    }
}

/********************************************************************************
function:	Update memory to OLED
********************************************************************************/
void oled_display(void)
{
    uint16_t page, column, temp;
	uint8_t TxBuf_tmp[128];
	
    for (page=0; page<8; page++) 
	{        
        oled_write_reg(0xB0 + page);/* set page address */        
        oled_write_reg(0x02);/* set low column address */        
        oled_write_reg(0x10);/* set high column address */        
        for(column=0; column<128; column++) // write data 
		{
            temp = OLED_GRAM[column][page];
            //oled_write_data(temp);
			TxBuf_tmp[column]=temp;
        }     
		oled_write_data_all(TxBuf_tmp,128);		
    }
}

void oled_paint_clear(void)
{
	uint8_t i, n;
	
	for (i = 0; i < 8; i++)
	{
		for (n = 0; n < 128; n++)
		{
			OLED_GRAM[n][i] = 0; //�����������
		}
	}
}

//����
//x:0~127
//y:0~63
void oled_draw_point(uint8_t x, uint8_t y)
{
	uint8_t i, m, n;
	
	i = y / 8;
	m = y % 8;
	n = 1 << m;
	OLED_GRAM[x][i] |= n;
}

//���һ����
//x:0~127
//y:0~63
void oled_clear_point(uint8_t x, uint8_t y)
{
	uint8_t i, m, n;
	
	i = y / 8;
	m = y % 8;
	n = 1 << m;
	OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
	OLED_GRAM[x][i] |= n;
	OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
}

//��ָ��λ����ʾһ���ַ�,���������ַ�
//x:0~127
//y:0~63
//size:ѡ������ 12/16/24
//ȡģ��ʽ ����ʽ
void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1)
{
	uint8_t i, m, temp, size2, chr1;
	uint8_t y0 = y;
	
	size2 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2); //�õ�����һ���ַ���Ӧ������ռ���ֽ���
	chr1 = chr - ' ';										   //����ƫ�ƺ��ֵ
	for (i = 0; i < size2; i++)
	{
		if (size1 == 12)
		{
			temp = asc2_1206[chr1][i];//����1206����
		} 
		else if (size1 == 16)
		{
			temp = asc2_1608[chr1][i];//����1608����
		} 
		else
		{
			return;
		}
			
		for (m = 0; m < 8; m++) //д������
		{
			if (temp & 0x80)
			{
				oled_draw_point(x, y);
			}
			else
			{
				oled_clear_point(x, y);
			}
			temp <<= 1;
			y++;
			if ((y - y0) == size1)
			{
				y = y0;
				x++;
				break;
			}
		}
	}
}

//��ʾ�ַ���
//x,y:�������
//size1:�����С
//*chr:�ַ�����ʼ��ַ
void oled_show_string(uint8_t x, uint8_t y, uint8_t *chr, uint8_t size1)
{
	while ((*chr >= ' ') && (*chr <= '~')) //�ж��ǲ��ǷǷ��ַ�!
	{
		oled_show_char(x, y, *chr, size1);
		x += size1 / 2;
		if (x > 128 - size1) //����
		{
			x = 0;
			y += size1;
		}
		chr++;
	}
}

//m^n
uint32_t oled_pow(uint8_t m, uint8_t n)
{
	uint32_t result = 1;
	while (n--)
	{
		result *= m;
	}
	return result;
}

////��ʾ2������
////x,y :�������
////len :���ֵ�λ��
////size:�����С
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1)
{
	uint8_t t, temp;
	
	for (t = 0; t < len; t++)
	{
		temp = (num / oled_pow(10, len - t - 1)) % 10;
		if (temp == 0)
		{
			oled_show_char(x + (size1 / 2) * t, y, '0', size1);
		}
		else
		{
			oled_show_char(x + (size1 / 2) * t, y, temp + '0', size1);
		}
	}
}

//��ʾ����
//x,y:�������
//num:���ֶ�Ӧ�����
//ȡģ��ʽ ����ʽ
void oled_show_chinese(uint8_t x, uint8_t y, uint8_t num, uint8_t size1)
{
	uint8_t i, m, n = 0, temp, chr1;
	uint8_t x0 = x, y0 = y;
	uint8_t size3 = size1 / 8;
	
	while (size3--)
	{
		chr1 = num * size1 / 8 + n;
		n++;
		for (i = 0; i < size1; i++)
		{
			if (size1 == 16)
			{
				temp = Hzk1[chr1][i];//����16*16����
			} 
			else if (size1 == 24)
			{
				temp = Hzk2[chr1][i];//����24*24����
			} 
			else
			{
				return;
			}
			for (m = 0; m < 8; m++)
			{
				if (temp & 0x01)
				{
					oled_draw_point(x, y);
				}	
				else
				{
					oled_clear_point(x, y);
				}
					
				temp >>= 1;
				y++;
			}
			x++;
			if ((x - x0) == size1)
			{
				x = x0;
				y0 = y0 + 8;
			}
			y = y0;
		}
	}
}

//num ��ʾ���ֵĸ���
//space ÿһ����ʾ�ļ��
void oled_scroll_display(uint8_t num, uint8_t space)
{
	uint8_t i, n, t = 0, m = 0, r;
	while (1)
	{
		if (m == 0)
		{
			oled_show_chinese(128, 24, t, 16); //д��һ�����ֱ�����OLED_GRAM[][]������
			t++;
		}
		if (t == num)
		{
			for (r = 0; r < 16 * space; r++) //��ʾ���
			{
				for (i = 0; i < 144; i++)
				{
					for (n = 0; n < 8; n++)
					{
						OLED_GRAM[i - 1][n] = OLED_GRAM[i][n];
					}
				}
				//OLED_Refresh();
			}
			t = 0;
		}
		m++;
		if (m == 16)
		{
			m = 0;
		}
		for (i = 0; i < 144; i++) //ʵ������
		{
			for (n = 0; n < 8; n++)
			{
				OLED_GRAM[i - 1][n] = OLED_GRAM[i][n];
			}
		}
		//OLED_Refresh();
	}
}
