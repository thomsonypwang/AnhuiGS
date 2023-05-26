#include "circuit_breaker_rs485_handle.h"
#include "circuit_breaker_rs485_dir.h"
#include "project_pin_use_config.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "sys_os.h"
#include "sys_log.h"
#include "sys_errno.h"
#include "am2301a_dir.h"
#include "project_psm_control.h"
#include "monitor_board_handle.h"

static os_thread_t circuit_breaker_thread= NULL;
static os_thread_stack_define(circuit_breaker_stack, 4*1024);

//uint16_t autoeclosing_rx_sta = 0;
uint8_t circuit_timeout_flag=0;
uint16_t circuit_rx_cnt=0;
uint8_t circuit_breaker_rx_buf[300];
uint8_t circuit_breaker_tx_buf[300];


cb_reg_input reg_in[CIRCUIT_BREAKER_MAX_NUM];
cb_reg_holding reg_hold[CIRCUIT_BREAKER_MAX_NUM];
cb_reg_discrete reg_discrete[CIRCUIT_BREAKER_MAX_NUM];

const uint16_t circuit_breaker_polynom = 0xA001;

uint16_t circuit_breaker_modbus_crc16(uint8_t *ptr, uint16_t len)
{
	uint8_t i;
	uint16_t crc = 0xffff;

	if (len == 0)
	{
		len = 1;
	}
	while (len--)
	{
		crc ^= *ptr;
		for (i = 0; i<8; i++)
		{
			if (crc & 1)
			{
				crc >>= 1;
				crc ^= circuit_breaker_polynom;
			}
			else
			{
				crc >>= 1;
			}
		}
		ptr++;
	}
	return(crc);
}

/**
 * @brief  USART RX IRQ callback
 * @param  None
 * @retval None
 */
static void circuit_breaker_USART7_RxFull_IrqCallback(void)
{
    uint8_t u8Data = (uint8_t)USART_ReadData(AUTOCLOSING_USART_ID);
	
	circuit_breaker_rx_buf[circuit_rx_cnt] = u8Data;
	circuit_rx_cnt++;
	if(circuit_rx_cnt>300)
	{
		circuit_rx_cnt=0;
	}
}

/**
 * @brief  USART error IRQ callback.
 * @param  None
 * @retval None
 */
static void circuit_breaker_USART7_RxError_IrqCallback(void)
{
	(void)USART_ReadData(AUTOCLOSING_USART_ID);
    USART_ClearStatus(AUTOCLOSING_USART_ID, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

static void circuit_breaker_USART7_RxTimeout_IrqCallback(void)
{
	circuit_timeout_flag=1;
    TMR0_Stop(AUTOCLOSING_USART_TMR0, AUTOCLOSING_USART_TMR0_CH);
    USART_ClearStatus(AUTOCLOSING_USART_ID, USART_FLAG_RX_TIMEOUT);
}


void set_coil_send_data(uint8_t addr_data,uint8_t open_close_flag)
{
	uint16_t i;
	uint16_t checksum=0;
	
	circuit_breaker_tx_buf[0]=addr_data;
	circuit_breaker_tx_buf[1]=0x05;
	circuit_breaker_tx_buf[2]=0x00;
	circuit_breaker_tx_buf[3]=0x00;
	if(open_close_flag==1)
	{
		circuit_breaker_tx_buf[4]=0xff;
	}
	else
	{
		circuit_breaker_tx_buf[4]=0x00;
	}
	circuit_breaker_tx_buf[5]=0x00;
	checksum=circuit_breaker_modbus_crc16(&circuit_breaker_tx_buf[0],6);
	circuit_breaker_tx_buf[6] =(checksum)&0xff;	
	circuit_breaker_tx_buf[7] =(checksum>>8)&0xff;	
	
	while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}  
	for (i = 0; i < 8; i++)
	{
		USART_WriteData(AUTOCLOSING_USART_ID, circuit_breaker_tx_buf[i]);
		while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}
}

int read_coil_handle(uint8_t addr_data,uint16_t timeout)
{
    uint16_t time = 0;
	uint16_t checksum=0;
	uint16_t read_checksum=0;
	
    while (time < timeout)
    {
		time++;
        os_thread_sleep(os_msec_to_ticks(10)); 
        if(circuit_timeout_flag==1)
        {
            circuit_timeout_flag = 0;
			circuit_rx_cnt=0;
			if(circuit_breaker_rx_buf[0]==addr_data)
			{
				if(circuit_breaker_rx_buf[1]==0x05)
				{
					checksum=circuit_breaker_modbus_crc16(&circuit_breaker_rx_buf[0],6);
					read_checksum=(uint16_t)(circuit_breaker_rx_buf[7]<<8)+circuit_breaker_rx_buf[6];
					if(checksum==read_checksum)
					{
						
						memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
						return SYS_OK;
					}
					else
					{
						log_i(" coil checksum:%04x read_checksum:%04x",checksum,read_checksum);
						memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
						return SYS_FAIL;
					}					
				}
				else
				{
					log_i("coil circuit_breaker_rx_buf[1]:%02x",circuit_breaker_rx_buf[1]);
					memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
					return SYS_FAIL;
				}				
			}
			else
			{
				log_i("coil circuit_breaker_rx_buf[0]:%02x",circuit_breaker_rx_buf[0]);
				memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
				return SYS_FAIL;
			}
        }
    }
	return SYS_E_TIMEOUT;
}

void read_hold_send_data(uint8_t addr_data)
{
	uint16_t i;
	uint16_t checksum=0;
	
	circuit_breaker_tx_buf[0]=addr_data;
	circuit_breaker_tx_buf[1]=0x03;
	circuit_breaker_tx_buf[2]=0x0f;
	circuit_breaker_tx_buf[3]=0xa0;
	circuit_breaker_tx_buf[4]=0x00;
	circuit_breaker_tx_buf[5]=0x31;
	checksum=circuit_breaker_modbus_crc16(&circuit_breaker_tx_buf[0],6);
	circuit_breaker_tx_buf[6] =(checksum)&0xff;	
	circuit_breaker_tx_buf[7] =(checksum>>8)&0xff;	
	
	while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}  
	for (i = 0; i < 8; i++)
	{
		USART_WriteData(AUTOCLOSING_USART_ID, circuit_breaker_tx_buf[i]);
		while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}
}

int read_hold_handle(uint8_t addr_data,uint16_t timeout)
{
    uint16_t time = 0;
	uint16_t checksum=0;
	uint16_t read_checksum=0;
	
    while (time < timeout)
    {
		time++;
        os_thread_sleep(os_msec_to_ticks(10)); 
        if(circuit_timeout_flag==1)
        {
            circuit_timeout_flag = 0;
			circuit_rx_cnt=0;
			if(circuit_breaker_rx_buf[0]==addr_data)
			{
				if(circuit_breaker_rx_buf[1]==0x03)
				{
					checksum=circuit_breaker_modbus_crc16(&circuit_breaker_rx_buf[0],circuit_breaker_rx_buf[2]+3);
					read_checksum=(uint16_t)(circuit_breaker_rx_buf[4+circuit_breaker_rx_buf[2]]<<8)+circuit_breaker_rx_buf[3+circuit_breaker_rx_buf[2]];
					if(checksum==read_checksum)
					{
						reg_hold[addr_data].REG1_RATED_CURRENT=(uint16_t)(circuit_breaker_rx_buf[3]<<8)+circuit_breaker_rx_buf[4]; 	// RW //额定电流
						reg_hold[addr_data].REG1_SPUV=(uint16_t)(circuit_breaker_rx_buf[5]<<8)+circuit_breaker_rx_buf[6]; 			// RW //单相欠压（报警）阈值
						reg_hold[addr_data].REG1_SPUVP=(uint16_t)(circuit_breaker_rx_buf[7]<<8)+circuit_breaker_rx_buf[8]; 			// RW //单相欠压（预警）阈值
						reg_hold[addr_data].REG1_SPOV=(uint16_t)(circuit_breaker_rx_buf[9]<<8)+circuit_breaker_rx_buf[10]; 			// RW //单相过压（报警）阈值
						reg_hold[addr_data].REG1_SPOVP=(uint16_t)(circuit_breaker_rx_buf[11]<<8)+circuit_breaker_rx_buf[12]; 		// RW //单相过压（预警）阈值
						reg_hold[addr_data].REG1_SPVC=(uint16_t)(circuit_breaker_rx_buf[13]<<8)+circuit_breaker_rx_buf[14]; 		// RW //单相过流（报警）阈值
						reg_hold[addr_data].REG1_SPVCP=(uint16_t)(circuit_breaker_rx_buf[15]<<8)+circuit_breaker_rx_buf[16]; 		// RW //单相过流（预警）阈值
						reg_hold[addr_data].REG1_SPOT=(uint16_t)(circuit_breaker_rx_buf[17]<<8)+circuit_breaker_rx_buf[18]; 		// RW //单相线温（报警）阈值
						reg_hold[addr_data].REG1_SPOTP=(uint16_t)(circuit_breaker_rx_buf[19]<<8)+circuit_breaker_rx_buf[20]; 		// RW //单相线温（预警）阈值
						reg_hold[addr_data].REG1_SLEAKAGE=(uint16_t)(circuit_breaker_rx_buf[21]<<8)+circuit_breaker_rx_buf[22]; 	// RW //软件漏电(报警)电流
						reg_hold[addr_data].REG1_SLEAKAGEP=(uint16_t)(circuit_breaker_rx_buf[23]<<8)+circuit_breaker_rx_buf[24]; 	// RW //软件漏电(预警)电流
						reg_hold[addr_data].REG1_OL=(uint16_t)(circuit_breaker_rx_buf[25]<<8)+circuit_breaker_rx_buf[26]; 			// RW //过载(报警)阈值
						reg_hold[addr_data].REG1_OLP=(uint16_t)(circuit_breaker_rx_buf[27]<<8)+circuit_breaker_rx_buf[28]; 			// RW //过载(预警)阈值
						reg_hold[addr_data].REG1_OP=(uint16_t)(circuit_breaker_rx_buf[29]<<8)+circuit_breaker_rx_buf[30]; 			// RW //过功率(报警)阈值
						reg_hold[addr_data].REG1_OPP=(uint16_t)(circuit_breaker_rx_buf[31]<<8)+circuit_breaker_rx_buf[32]; 			// RW //过功率(预警)阈值
						reg_hold[addr_data].REG1_UVA=(uint16_t)(circuit_breaker_rx_buf[33]<<8)+circuit_breaker_rx_buf[34]; 			// RW //欠压(报警)阈值A
						reg_hold[addr_data].REG1_UVAP=(uint16_t)(circuit_breaker_rx_buf[35]<<8)+circuit_breaker_rx_buf[36]; 		// RW //欠压(预警)阈值A
						reg_hold[addr_data].REG1_UVB=(uint16_t)(circuit_breaker_rx_buf[37]<<8)+circuit_breaker_rx_buf[38]; 			// RW //欠压(报警)阈值B
						reg_hold[addr_data].REG1_UVBP=(uint16_t)(circuit_breaker_rx_buf[39]<<8)+circuit_breaker_rx_buf[40]; 		// RW //欠压(预警)阈值B
						reg_hold[addr_data].REG1_UVC=(uint16_t)(circuit_breaker_rx_buf[41]<<8)+circuit_breaker_rx_buf[42]; 			// RW //欠压(报警)阈值C
						reg_hold[addr_data].REG1_UVCP=(uint16_t)(circuit_breaker_rx_buf[43]<<8)+circuit_breaker_rx_buf[44]; 		// RW //欠压(预警)阈值C
						reg_hold[addr_data].REG1_OVA=(uint16_t)(circuit_breaker_rx_buf[45]<<8)+circuit_breaker_rx_buf[46]; 			// RW //过压(报警)阈值A
						reg_hold[addr_data].REG1_OVAP=(uint16_t)(circuit_breaker_rx_buf[47]<<8)+circuit_breaker_rx_buf[48]; 		// RW //过压(预警)阈值A
						reg_hold[addr_data].REG1_OVB=(uint16_t)(circuit_breaker_rx_buf[49]<<8)+circuit_breaker_rx_buf[50]; 			// RW //过压(报警)阈值B
						reg_hold[addr_data].REG1_OVBP=(uint16_t)(circuit_breaker_rx_buf[51]<<8)+circuit_breaker_rx_buf[52]; 		// RW //过压(预警)阈值B
						reg_hold[addr_data].REG1_OVC=(uint16_t)(circuit_breaker_rx_buf[53]<<8)+circuit_breaker_rx_buf[54]; 			// RW //过压(报警)阈值C
						reg_hold[addr_data].REG1_OVCP=(uint16_t)(circuit_breaker_rx_buf[55]<<8)+circuit_breaker_rx_buf[56]; 		// RW //过压(预警)阈值C
						reg_hold[addr_data].REG1_OCA=(uint16_t)(circuit_breaker_rx_buf[57]<<8)+circuit_breaker_rx_buf[58]; 			// RW //过流(报警)阈值A
						reg_hold[addr_data].REG1_OCA_BK=(uint16_t)(circuit_breaker_rx_buf[59]<<8)+circuit_breaker_rx_buf[60]; 		// RW //过流(报警)阈值A
						reg_hold[addr_data].REG1_OCAP=(uint16_t)(circuit_breaker_rx_buf[61]<<8)+circuit_breaker_rx_buf[62]; 		// RW //过流(预警)阈值A
						reg_hold[addr_data].REG1_OCAP_BK=(uint16_t)(circuit_breaker_rx_buf[63]<<8)+circuit_breaker_rx_buf[64]; 		// RW //过流(预警)阈值A
						reg_hold[addr_data].REG1_OCB=(uint16_t)(circuit_breaker_rx_buf[65]<<8)+circuit_breaker_rx_buf[66]; 			// RW //过流(报警)阈值B
						reg_hold[addr_data].REG1_OCB_BK=(uint16_t)(circuit_breaker_rx_buf[67]<<8)+circuit_breaker_rx_buf[68]; 		// RW //过流(报警)阈值B
						reg_hold[addr_data].REG1_OCBP=(uint16_t)(circuit_breaker_rx_buf[69]<<8)+circuit_breaker_rx_buf[70]; 		// RW //过流(预警)阈值B
						reg_hold[addr_data].REG1_OCBP_BK=(uint16_t)(circuit_breaker_rx_buf[71]<<8)+circuit_breaker_rx_buf[72]; 		// RW //过流(预警)阈值B
						reg_hold[addr_data].REG1_OCC=(uint16_t)(circuit_breaker_rx_buf[73]<<8)+circuit_breaker_rx_buf[74]; 			// RW //过流(报警)阈值C
						reg_hold[addr_data].REG1_OCC_BK=(uint16_t)(circuit_breaker_rx_buf[75]<<8)+circuit_breaker_rx_buf[76]; 		// RW //过流(报警)阈值C
						reg_hold[addr_data].REG1_OCCP=(uint16_t)(circuit_breaker_rx_buf[77]<<8)+circuit_breaker_rx_buf[78]; 		// RW //过流(预警)阈值C
						reg_hold[addr_data].REG1_OCCP_BK=(uint16_t)(circuit_breaker_rx_buf[79]<<8)+circuit_breaker_rx_buf[80]; 		// RW //过流(预警)阈值C
						reg_hold[addr_data].REG1_OTA=(uint16_t)(circuit_breaker_rx_buf[81]<<8)+circuit_breaker_rx_buf[82]; 			// RW //温度(报警)阈值A
						reg_hold[addr_data].REG1_OTAP=(uint16_t)(circuit_breaker_rx_buf[83]<<8)+circuit_breaker_rx_buf[84]; 		// RW //温度(预警)阈值A
						reg_hold[addr_data].REG1_OTB=(uint16_t)(circuit_breaker_rx_buf[85]<<8)+circuit_breaker_rx_buf[86]; 			// RW //温度(报警)阈值B
						reg_hold[addr_data].REG1_OTBP=(uint16_t)(circuit_breaker_rx_buf[87]<<8)+circuit_breaker_rx_buf[88]; 		// RW //温度(预警)阈值B
						reg_hold[addr_data].REG1_OTC=(uint16_t)(circuit_breaker_rx_buf[89]<<8)+circuit_breaker_rx_buf[90]; 			// RW //温度(报警)阈值C
						reg_hold[addr_data].REG1_OTCP=(uint16_t)(circuit_breaker_rx_buf[91]<<8)+circuit_breaker_rx_buf[92]; 		// RW //温度(预警)阈值C
						reg_hold[addr_data].REG1_OTN=(uint16_t)(circuit_breaker_rx_buf[93]<<8)+circuit_breaker_rx_buf[94]; 			// RW //温度(报警)阈值N
						reg_hold[addr_data].REG1_OTNP=(uint16_t)(circuit_breaker_rx_buf[95]<<8)+circuit_breaker_rx_buf[96]; 		// RW //温度(预警)阈值N
						reg_hold[addr_data].REG1_UBL=(uint16_t)(circuit_breaker_rx_buf[97]<<8)+circuit_breaker_rx_buf[98]; 			// RW //负载不均衡(报警)阈值
						reg_hold[addr_data].REG1_UBLP=(uint16_t)(circuit_breaker_rx_buf[99]<<8)+circuit_breaker_rx_buf[100]; 		// RW //负载不均衡(预警)阈值
						memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
						return SYS_OK;
					}
					else
					{
						log_i("hold  checksum:%04x read_checksum:%04x",checksum,read_checksum);
						memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
						return SYS_FAIL;
					}					
				}
				else
				{
					log_i("hold circuit_breaker_rx_buf[1]:%02x",circuit_breaker_rx_buf[1]);
					memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
					return SYS_FAIL;
				}				
			}
			else
			{
				log_i("hold circuit_breaker_rx_buf[0]:%02x",circuit_breaker_rx_buf[0]);
				memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
				return SYS_FAIL;
			}
        }
    }
	return SYS_E_TIMEOUT;
}

void write_hold_send_data(uint8_t addr_data)
{
	uint16_t i;
	uint16_t checksum=0;
	
	circuit_breaker_tx_buf[0]=addr_data;
	circuit_breaker_tx_buf[1]=0x10;
	circuit_breaker_tx_buf[2]=0x0f;
	circuit_breaker_tx_buf[3]=0xa1;
	circuit_breaker_tx_buf[4]=0x00;
	circuit_breaker_tx_buf[5]=0x30;
	circuit_breaker_tx_buf[6]=0x60;
	circuit_breaker_tx_buf[7] =(reg_hold[addr_data].REG1_SPUV>>8)&0xff;	
	circuit_breaker_tx_buf[8] =(reg_hold[addr_data].REG1_SPUV)&0xff;	
	circuit_breaker_tx_buf[9] =(reg_hold[addr_data].REG1_SPUVP>>8)&0xff;	
	circuit_breaker_tx_buf[10] =(reg_hold[addr_data].REG1_SPUVP)&0xff;	
	circuit_breaker_tx_buf[11] =(reg_hold[addr_data].REG1_SPOV>>8)&0xff;	
	circuit_breaker_tx_buf[12] =(reg_hold[addr_data].REG1_SPOV)&0xff;	
	circuit_breaker_tx_buf[13] =(reg_hold[addr_data].REG1_SPOVP>>8)&0xff;	
	circuit_breaker_tx_buf[14] =(reg_hold[addr_data].REG1_SPOVP)&0xff;		
	circuit_breaker_tx_buf[15] =(reg_hold[addr_data].REG1_SPVC>>8)&0xff;	
	circuit_breaker_tx_buf[16] =(reg_hold[addr_data].REG1_SPVC)&0xff;	
	circuit_breaker_tx_buf[17] =(reg_hold[addr_data].REG1_SPVCP>>8)&0xff;	
	circuit_breaker_tx_buf[18] =(reg_hold[addr_data].REG1_SPVCP)&0xff;	
	circuit_breaker_tx_buf[19] =(reg_hold[addr_data].REG1_SPOT>>8)&0xff;	
	circuit_breaker_tx_buf[20] =(reg_hold[addr_data].REG1_SPOT)&0xff;	
	circuit_breaker_tx_buf[21] =(reg_hold[addr_data].REG1_SPOTP>>8)&0xff;	
	circuit_breaker_tx_buf[22] =(reg_hold[addr_data].REG1_SPOTP)&0xff;	
	circuit_breaker_tx_buf[23] =(reg_hold[addr_data].REG1_SLEAKAGE>>8)&0xff;	
	circuit_breaker_tx_buf[24] =(reg_hold[addr_data].REG1_SLEAKAGE)&0xff;	
	circuit_breaker_tx_buf[25] =(reg_hold[addr_data].REG1_SLEAKAGEP>>8)&0xff;	
	circuit_breaker_tx_buf[26] =(reg_hold[addr_data].REG1_SLEAKAGEP)&0xff;	
	circuit_breaker_tx_buf[27] =(reg_hold[addr_data].REG1_OL>>8)&0xff;	
	circuit_breaker_tx_buf[28] =(reg_hold[addr_data].REG1_OL)&0xff;	
	circuit_breaker_tx_buf[29] =(reg_hold[addr_data].REG1_OLP>>8)&0xff;	
	circuit_breaker_tx_buf[30] =(reg_hold[addr_data].REG1_OLP)&0xff;	
	circuit_breaker_tx_buf[31] =(reg_hold[addr_data].REG1_OP>>8)&0xff;	
	circuit_breaker_tx_buf[32] =(reg_hold[addr_data].REG1_OP)&0xff;	
	circuit_breaker_tx_buf[33] =(reg_hold[addr_data].REG1_OPP>>8)&0xff;	
	circuit_breaker_tx_buf[34] =(reg_hold[addr_data].REG1_OPP)&0xff;	
	circuit_breaker_tx_buf[35] =(reg_hold[addr_data].REG1_UVA>>8)&0xff;	
	circuit_breaker_tx_buf[36] =(reg_hold[addr_data].REG1_UVA)&0xff;	
	circuit_breaker_tx_buf[37] =(reg_hold[addr_data].REG1_UVAP>>8)&0xff;	
	circuit_breaker_tx_buf[38] =(reg_hold[addr_data].REG1_UVAP)&0xff;		
	circuit_breaker_tx_buf[39] =(reg_hold[addr_data].REG1_UVB>>8)&0xff;	
	circuit_breaker_tx_buf[40] =(reg_hold[addr_data].REG1_UVB)&0xff;		
	circuit_breaker_tx_buf[41] =(reg_hold[addr_data].REG1_UVBP>>8)&0xff;	
	circuit_breaker_tx_buf[42] =(reg_hold[addr_data].REG1_UVBP)&0xff;	
	circuit_breaker_tx_buf[43] =(reg_hold[addr_data].REG1_UVC>>8)&0xff;	
	circuit_breaker_tx_buf[44] =(reg_hold[addr_data].REG1_UVC)&0xff;		
	circuit_breaker_tx_buf[45] =(reg_hold[addr_data].REG1_UVCP>>8)&0xff;	
	circuit_breaker_tx_buf[46] =(reg_hold[addr_data].REG1_UVCP)&0xff;	
	circuit_breaker_tx_buf[47] =(reg_hold[addr_data].REG1_OVA>>8)&0xff;	
	circuit_breaker_tx_buf[48] =(reg_hold[addr_data].REG1_OVA)&0xff;	
	circuit_breaker_tx_buf[49] =(reg_hold[addr_data].REG1_OVAP>>8)&0xff;	
	circuit_breaker_tx_buf[50] =(reg_hold[addr_data].REG1_OVAP)&0xff;	
	circuit_breaker_tx_buf[51] =(reg_hold[addr_data].REG1_OVB>>8)&0xff;	
	circuit_breaker_tx_buf[52] =(reg_hold[addr_data].REG1_OVB)&0xff;		
	circuit_breaker_tx_buf[53] =(reg_hold[addr_data].REG1_OVBP>>8)&0xff;	
	circuit_breaker_tx_buf[54] =(reg_hold[addr_data].REG1_OVBP)&0xff;	
	circuit_breaker_tx_buf[55] =(reg_hold[addr_data].REG1_OVC>>8)&0xff;	
	circuit_breaker_tx_buf[56] =(reg_hold[addr_data].REG1_OVC)&0xff;	
	circuit_breaker_tx_buf[57] =(reg_hold[addr_data].REG1_OVCP>>8)&0xff;	
	circuit_breaker_tx_buf[58] =(reg_hold[addr_data].REG1_OVCP)&0xff;		
	circuit_breaker_tx_buf[59] =(reg_hold[addr_data].REG1_OCA>>8)&0xff;	
	circuit_breaker_tx_buf[60] =(reg_hold[addr_data].REG1_OCA)&0xff;	
	circuit_breaker_tx_buf[61] =(reg_hold[addr_data].REG1_OCA_BK>>8)&0xff;	
	circuit_breaker_tx_buf[62] =(reg_hold[addr_data].REG1_OCA_BK)&0xff;	
	circuit_breaker_tx_buf[63] =(reg_hold[addr_data].REG1_OCAP>>8)&0xff;	
	circuit_breaker_tx_buf[64] =(reg_hold[addr_data].REG1_OCAP)&0xff;	
	circuit_breaker_tx_buf[65] =(reg_hold[addr_data].REG1_OCAP_BK>>8)&0xff;	
	circuit_breaker_tx_buf[66] =(reg_hold[addr_data].REG1_OCAP_BK)&0xff;	
	circuit_breaker_tx_buf[67] =(reg_hold[addr_data].REG1_OCB>>8)&0xff;	
	circuit_breaker_tx_buf[68] =(reg_hold[addr_data].REG1_OCB)&0xff;	
	circuit_breaker_tx_buf[69] =(reg_hold[addr_data].REG1_OCB_BK>>8)&0xff;	
	circuit_breaker_tx_buf[70] =(reg_hold[addr_data].REG1_OCB_BK)&0xff;	
	circuit_breaker_tx_buf[71] =(reg_hold[addr_data].REG1_OCBP>>8)&0xff;	
	circuit_breaker_tx_buf[72] =(reg_hold[addr_data].REG1_OCBP)&0xff;	
	circuit_breaker_tx_buf[73] =(reg_hold[addr_data].REG1_OCBP_BK>>8)&0xff;	
	circuit_breaker_tx_buf[74] =(reg_hold[addr_data].REG1_OCBP_BK)&0xff;	
	circuit_breaker_tx_buf[75] =(reg_hold[addr_data].REG1_OCC>>8)&0xff;	
	circuit_breaker_tx_buf[76] =(reg_hold[addr_data].REG1_OCC)&0xff;		
	circuit_breaker_tx_buf[77] =(reg_hold[addr_data].REG1_OCC_BK>>8)&0xff;	
	circuit_breaker_tx_buf[78] =(reg_hold[addr_data].REG1_OCC_BK)&0xff;	
	circuit_breaker_tx_buf[79] =(reg_hold[addr_data].REG1_OCCP>>8)&0xff;	
	circuit_breaker_tx_buf[80] =(reg_hold[addr_data].REG1_OCCP)&0xff;	
	circuit_breaker_tx_buf[81] =(reg_hold[addr_data].REG1_OCCP_BK>>8)&0xff;	
	circuit_breaker_tx_buf[82] =(reg_hold[addr_data].REG1_OCCP_BK)&0xff;	
	circuit_breaker_tx_buf[83] =(reg_hold[addr_data].REG1_OTA>>8)&0xff;	
	circuit_breaker_tx_buf[84] =(reg_hold[addr_data].REG1_OTA)&0xff;	
	
	circuit_breaker_tx_buf[85] =(reg_hold[addr_data].REG1_OTAP>>8)&0xff;	
	circuit_breaker_tx_buf[86] =(reg_hold[addr_data].REG1_OTAP)&0xff;	
	circuit_breaker_tx_buf[87] =(reg_hold[addr_data].REG1_OTB>>8)&0xff;	
	circuit_breaker_tx_buf[88] =(reg_hold[addr_data].REG1_OTB)&0xff;	
	circuit_breaker_tx_buf[89] =(reg_hold[addr_data].REG1_OTBP>>8)&0xff;	
	circuit_breaker_tx_buf[90] =(reg_hold[addr_data].REG1_OTBP)&0xff;	
	circuit_breaker_tx_buf[91] =(reg_hold[addr_data].REG1_OTC>>8)&0xff;	
	circuit_breaker_tx_buf[92] =(reg_hold[addr_data].REG1_OTC)&0xff;	
	circuit_breaker_tx_buf[93] =(reg_hold[addr_data].REG1_OTCP>>8)&0xff;	
	circuit_breaker_tx_buf[94] =(reg_hold[addr_data].REG1_OTCP)&0xff;	
	circuit_breaker_tx_buf[95] =(reg_hold[addr_data].REG1_OTN>>8)&0xff;	
	circuit_breaker_tx_buf[96] =(reg_hold[addr_data].REG1_OTN)&0xff;	
	circuit_breaker_tx_buf[97] =(reg_hold[addr_data].REG1_OTNP>>8)&0xff;	
	circuit_breaker_tx_buf[98] =(reg_hold[addr_data].REG1_OTNP)&0xff;	
	circuit_breaker_tx_buf[99] =(reg_hold[addr_data].REG1_UBL>>8)&0xff;	
	circuit_breaker_tx_buf[100] =(reg_hold[addr_data].REG1_UBL)&0xff;	
	circuit_breaker_tx_buf[101] =(reg_hold[addr_data].REG1_UBLP>>8)&0xff;	
	circuit_breaker_tx_buf[102] =(reg_hold[addr_data].REG1_UBLP)&0xff;	
	
	checksum=circuit_breaker_modbus_crc16(&circuit_breaker_tx_buf[0],103);
	circuit_breaker_tx_buf[103] =(checksum)&0xff;	
	circuit_breaker_tx_buf[104] =(checksum>>8)&0xff;	
	
	while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}  
	for (i = 0; i < 105; i++)
	{
		USART_WriteData(AUTOCLOSING_USART_ID, circuit_breaker_tx_buf[i]);
		while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}
}

int write_hold_handle(uint8_t addr_data,uint16_t timeout)
{
    uint16_t time = 0;
	uint16_t checksum=0;
	uint16_t read_checksum=0;
	
    while (time < timeout)
    {
		time++;
        os_thread_sleep(os_msec_to_ticks(10)); 
        if(circuit_timeout_flag==1)
        {
            circuit_timeout_flag = 0;
			circuit_rx_cnt=0;
			if(circuit_breaker_rx_buf[0]==addr_data)
			{
				if(circuit_breaker_rx_buf[1]==0x10)
				{
					checksum=circuit_breaker_modbus_crc16(&circuit_breaker_rx_buf[0],6);
					read_checksum=(uint16_t)(circuit_breaker_rx_buf[7]<<8)+circuit_breaker_rx_buf[6];
					if(checksum==read_checksum)
					{						
						memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
						return SYS_OK;
					}
					else
					{
						log_i(" w hold checksum:%04x read_checksum:%04x",checksum,read_checksum);
						memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
						return SYS_FAIL;
					}					
				}
				else
				{
					log_i("w hold circuit_breaker_rx_buf[1]:%02x",circuit_breaker_rx_buf[1]);
					memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
					return SYS_FAIL;
				}				
			}
			else
			{
				log_i("w hold circuit_breaker_rx_buf[0]:%02x",circuit_breaker_rx_buf[0]);
				memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
				return SYS_FAIL;
			}
        }
    }
	return SYS_E_TIMEOUT;
}

void read_discrete_send_data(uint8_t addr_data)
{
	uint16_t i;
	uint16_t checksum=0;
	
	circuit_breaker_tx_buf[0]=addr_data;
	circuit_breaker_tx_buf[1]=0x02;
	circuit_breaker_tx_buf[2]=0x07;
	circuit_breaker_tx_buf[3]=0xd0;
	circuit_breaker_tx_buf[4]=0x00;
	circuit_breaker_tx_buf[5]=0x39;
	checksum=circuit_breaker_modbus_crc16(&circuit_breaker_tx_buf[0],6);
	circuit_breaker_tx_buf[6] =(checksum)&0xff;	
	circuit_breaker_tx_buf[7] =(checksum>>8)&0xff;	
	
	while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}  
	for (i = 0; i < 8; i++)
	{
		USART_WriteData(AUTOCLOSING_USART_ID, circuit_breaker_tx_buf[i]);
		while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}
}

int read_discrete_handle(uint8_t addr_data,uint16_t timeout)
{
    uint16_t time = 0;
	uint16_t checksum=0;
	uint16_t read_checksum=0;
	
    while (time < timeout)
    {
		time++;
        os_thread_sleep(os_msec_to_ticks(10)); 		
        if(circuit_timeout_flag==1)
        {
            circuit_timeout_flag = 0;
			circuit_rx_cnt=0;

			if(circuit_breaker_rx_buf[0]==addr_data)
			{
				if(circuit_breaker_rx_buf[1]==0x02)
				{
					checksum=circuit_breaker_modbus_crc16(&circuit_breaker_rx_buf[0],circuit_breaker_rx_buf[2]+3);
					read_checksum=(uint16_t)(circuit_breaker_rx_buf[4+circuit_breaker_rx_buf[2]]<<8)+circuit_breaker_rx_buf[3+circuit_breaker_rx_buf[2]];
					if(checksum==read_checksum)
					{
						reg_discrete[addr_data].discrete2.all=circuit_breaker_rx_buf[4];//合闸状态
						//log_i("reg_discrete[%d].discrete2=0x%02x",addr_data,reg_discrete[addr_data].discrete2.all);
						memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
						return SYS_OK;
					}
					else
					{
						log_i("discrete checksum:%04x read_checksum:%04x",checksum,read_checksum);
						memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
						return SYS_FAIL;
					}					
				}
				else
				{
					log_i("discrete circuit_breaker_rx_buf[1]:%02x",circuit_breaker_rx_buf[1]);
					memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
					return SYS_FAIL;
				}				
			}
			else
			{
				log_i("discrete circuit_breaker_rx_buf[0]:%02x",circuit_breaker_rx_buf[0]);
				memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
				return SYS_FAIL;
			}
        }
    }
	return SYS_E_TIMEOUT;
}

void read_input_send_data(uint8_t addr_data)//3001
{
	uint16_t i;
	uint16_t checksum=0;
	
	//log_i("addr_data %d",addr_data);
	circuit_breaker_tx_buf[0]=addr_data;
	circuit_breaker_tx_buf[1]=0x04;
	circuit_breaker_tx_buf[2]=0x0b;
	circuit_breaker_tx_buf[3]=0xb8;
	circuit_breaker_tx_buf[4]=0x00;
	circuit_breaker_tx_buf[5]=0x6e;
	checksum=circuit_breaker_modbus_crc16(&circuit_breaker_tx_buf[0],6);
	circuit_breaker_tx_buf[6] =(checksum)&0xff;	
	circuit_breaker_tx_buf[7] =(checksum>>8)&0xff;	
	
	while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}  
	for (i = 0; i < 8; i++)
	{
		USART_WriteData(AUTOCLOSING_USART_ID, circuit_breaker_tx_buf[i]);
		while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}
}

int read_input_register_handle(uint8_t addr_data,uint16_t timeout)
{
    uint16_t time = 0;
	uint16_t checksum=0;
	uint16_t read_checksum=0;

    while (time < timeout)
    {
		time++;
        os_thread_sleep(os_msec_to_ticks(10)); 		
		//log_i("time %d ,addr_data %d",time,addr_data);
        if(circuit_timeout_flag==1)
        {
            circuit_timeout_flag = 0;
			circuit_rx_cnt=0;
			if(circuit_breaker_rx_buf[0]==addr_data)
			{
				if(circuit_breaker_rx_buf[1]==0x04)
				{
					checksum=circuit_breaker_modbus_crc16(&circuit_breaker_rx_buf[0],circuit_breaker_rx_buf[2]+3);
					read_checksum=(uint16_t)(circuit_breaker_rx_buf[4+circuit_breaker_rx_buf[2]]<<8)+circuit_breaker_rx_buf[3+circuit_breaker_rx_buf[2]];
					if(checksum==read_checksum)
					{
						reg_in[addr_data].REG0_ROCKER_SW=(uint16_t)(circuit_breaker_rx_buf[3]<<8)+circuit_breaker_rx_buf[4];	// R //拨动开关暂未提供支持
						reg_in[addr_data].REG0_TYPE=(uint16_t)(circuit_breaker_rx_buf[5]<<8)+circuit_breaker_rx_buf[6]; 		// R //设备类型详见型号对应表
						reg_in[addr_data].REG0_VER=(uint16_t)(circuit_breaker_rx_buf[7]<<8)+circuit_breaker_rx_buf[8]; 			// R //软件版本号
						reg_in[addr_data].REG0_SPC=(uint32_t)(circuit_breaker_rx_buf[9]<<24)+(uint32_t)(circuit_breaker_rx_buf[10]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[11]<<8)+(uint32_t)(circuit_breaker_rx_buf[12]); // R //单相两线电流
						reg_in[addr_data].REG0_SPV=(uint16_t)(circuit_breaker_rx_buf[13]<<8)+circuit_breaker_rx_buf[14]; 			// R //单相电压
						reg_in[addr_data].REG0_SPP=(uint32_t)(circuit_breaker_rx_buf[15]<<24)+(uint32_t)(circuit_breaker_rx_buf[16]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[17]<<8)+(uint32_t)(circuit_breaker_rx_buf[18]); 			// R //单相两线有功功率
						reg_in[addr_data].REG0_SPQ=(uint32_t)(circuit_breaker_rx_buf[19]<<24)+(uint32_t)(circuit_breaker_rx_buf[20]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[21]<<8)+(uint32_t)(circuit_breaker_rx_buf[22]); 			// R //单相两线无功功率
						reg_in[addr_data].REG0_SPS=(uint32_t)(circuit_breaker_rx_buf[23]<<24)+(uint32_t)(circuit_breaker_rx_buf[24]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[25]<<8)+(uint32_t)(circuit_breaker_rx_buf[26]); 			// R //单相两线视在功率
						reg_in[addr_data].REG0_SPSE=((uint64_t)circuit_breaker_rx_buf[27]<<56)+((uint64_t)circuit_breaker_rx_buf[28]<<48) \
												+((uint64_t)circuit_breaker_rx_buf[29]<<40)+((uint64_t)circuit_breaker_rx_buf[30]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[31]<<24)+(uint64_t)(circuit_breaker_rx_buf[32]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[33]<<8)+(uint64_t)(circuit_breaker_rx_buf[34]); 		// R //单相视在电能				
						reg_in[addr_data].REG0_SPTEMP=(uint16_t)(circuit_breaker_rx_buf[35]<<8)+circuit_breaker_rx_buf[36]; 		// R //单相L线温
						reg_in[addr_data].REG0_LEAKAGE_C=(uint16_t)(circuit_breaker_rx_buf[37]<<8)+circuit_breaker_rx_buf[38]; 	// R //剩余电流
						reg_in[addr_data].REG0_TEMPINT=(uint16_t)(circuit_breaker_rx_buf[39]<<8)+circuit_breaker_rx_buf[40]; 		// R //内部温度（预留）
						reg_in[addr_data].REG0_AC=(uint32_t)(circuit_breaker_rx_buf[41]<<24)+(uint32_t)(circuit_breaker_rx_buf[42]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[43]<<8)+(uint32_t)(circuit_breaker_rx_buf[44]); ; 			// R //A相电流
						reg_in[addr_data].REG0_BC=(uint32_t)(circuit_breaker_rx_buf[45]<<24)+(uint32_t)(circuit_breaker_rx_buf[46]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[47]<<8)+(uint32_t)(circuit_breaker_rx_buf[48]); ; 			// R //B相电流
						reg_in[addr_data].REG0_CC=(uint32_t)(circuit_breaker_rx_buf[49]<<24)+(uint32_t)(circuit_breaker_rx_buf[50]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[51]<<8)+(uint32_t)(circuit_breaker_rx_buf[52]); ; 			// R //C相电流
						reg_in[addr_data].REG0_NC=(uint16_t)(circuit_breaker_rx_buf[53]<<8)+circuit_breaker_rx_buf[54]; 			// R //零线电流
						reg_in[addr_data].REG0_RATED_C=(uint16_t)(circuit_breaker_rx_buf[55]<<8)+circuit_breaker_rx_buf[56]; 		// R //额定电流
						reg_in[addr_data].REG0_RSV=(uint16_t)(circuit_breaker_rx_buf[57]<<8)+circuit_breaker_rx_buf[58]; 			// R //预留
						reg_in[addr_data].REG0_VA=(uint16_t)(circuit_breaker_rx_buf[59]<<8)+circuit_breaker_rx_buf[60]; 			// R //A相电压
						reg_in[addr_data].REG0_VB=(uint16_t)(circuit_breaker_rx_buf[61]<<8)+circuit_breaker_rx_buf[62]; 			// R //B相电压
						reg_in[addr_data].REG0_VC=(uint16_t)(circuit_breaker_rx_buf[63]<<8)+circuit_breaker_rx_buf[64]; 			// R //C相电压
						reg_in[addr_data].REG0_VAVG=(uint16_t)(circuit_breaker_rx_buf[65]<<8)+circuit_breaker_rx_buf[66]; 		// R //状态映射寄存器
						reg_in[addr_data].REG0_PA=(uint32_t)(circuit_breaker_rx_buf[67]<<24)+(uint32_t)(circuit_breaker_rx_buf[68]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[69]<<8)+(uint32_t)(circuit_breaker_rx_buf[70]); ; 			// R //有功功率A
						reg_in[addr_data].REG0_PB=(uint32_t)(circuit_breaker_rx_buf[71]<<24)+(uint32_t)(circuit_breaker_rx_buf[72]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[73]<<8)+(uint32_t)(circuit_breaker_rx_buf[74]); ; 			// R //有功功率B
						reg_in[addr_data].REG0_PC=(uint32_t)(circuit_breaker_rx_buf[75]<<24)+(uint32_t)(circuit_breaker_rx_buf[76]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[77]<<8)+(uint32_t)(circuit_breaker_rx_buf[78]); ; 			// R //有功功率C
						reg_in[addr_data].REG0_QA=(uint32_t)(circuit_breaker_rx_buf[79]<<24)+(uint32_t)(circuit_breaker_rx_buf[80]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[81]<<8)+(uint32_t)(circuit_breaker_rx_buf[82]);  			// R //无功功率A
						reg_in[addr_data].REG0_QB=(uint32_t)(circuit_breaker_rx_buf[83]<<24)+(uint32_t)(circuit_breaker_rx_buf[84]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[85]<<8)+(uint32_t)(circuit_breaker_rx_buf[86]); 			// R //无功功率B
						reg_in[addr_data].REG0_QC=(uint32_t)(circuit_breaker_rx_buf[87]<<24)+(uint32_t)(circuit_breaker_rx_buf[88]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[89]<<8)+(uint32_t)(circuit_breaker_rx_buf[90]);  			// R //无功功率C
						reg_in[addr_data].REG0_SA=(uint32_t)(circuit_breaker_rx_buf[91]<<24)+(uint32_t)(circuit_breaker_rx_buf[92]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[93]<<8)+(uint32_t)(circuit_breaker_rx_buf[94]); 			// R //视在功率A
						reg_in[addr_data].REG0_SB=(uint32_t)(circuit_breaker_rx_buf[95]<<24)+(uint32_t)(circuit_breaker_rx_buf[96]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[97]<<8)+(uint32_t)(circuit_breaker_rx_buf[98]); 			// R //视在功率B
						reg_in[addr_data].REG0_SC=(uint32_t)(circuit_breaker_rx_buf[99]<<24)+(uint32_t)(circuit_breaker_rx_buf[100]<<16) \
												+(uint32_t)(circuit_breaker_rx_buf[101]<<8)+(uint32_t)(circuit_breaker_rx_buf[102]); 			// R //视在功率C
												
						reg_in[addr_data].REG0_SEA=((uint64_t)circuit_breaker_rx_buf[103]<<56)+((uint64_t)circuit_breaker_rx_buf[104]<<48) 	\
												+((uint64_t)circuit_breaker_rx_buf[105]<<40)+((uint64_t)circuit_breaker_rx_buf[106]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[107]<<24)+(uint64_t)(circuit_breaker_rx_buf[108]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[109]<<8)+(uint64_t)(circuit_breaker_rx_buf[110]);			// R //视在能量A
						reg_in[addr_data].REG0_SEB=((uint64_t)circuit_breaker_rx_buf[111]<<56)+((uint64_t)circuit_breaker_rx_buf[112]<<48) 	\
												+((uint64_t)circuit_breaker_rx_buf[113]<<40)+((uint64_t)circuit_breaker_rx_buf[114]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[115]<<24)+(uint64_t)(circuit_breaker_rx_buf[116]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[117]<<8)+(uint64_t)(circuit_breaker_rx_buf[118]); 			// R //视在能量B
						reg_in[addr_data].REG0_SEC=((uint64_t)circuit_breaker_rx_buf[119]<<56)+((uint64_t)circuit_breaker_rx_buf[120]<<48) 	\
												+((uint64_t)circuit_breaker_rx_buf[121]<<40)+((uint64_t)circuit_breaker_rx_buf[122]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[123]<<24)+(uint64_t)(circuit_breaker_rx_buf[124]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[125]<<8)+(uint64_t)(circuit_breaker_rx_buf[126]); 			// R //视在能量C
						reg_in[addr_data].REG0_SESTA=((uint64_t)circuit_breaker_rx_buf[127]<<56)+((uint64_t)circuit_breaker_rx_buf[128]<<48) \
												+((uint64_t)circuit_breaker_rx_buf[129]<<40)+((uint64_t)circuit_breaker_rx_buf[130]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[131]<<24)+(uint64_t)(circuit_breaker_rx_buf[132]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[133]<<8)+(uint64_t)(circuit_breaker_rx_buf[134]); 		// R //合视在能量
						reg_in[addr_data].REG0_TEMPA=(uint16_t)(circuit_breaker_rx_buf[135]<<8)+circuit_breaker_rx_buf[136]; 		// R //A线温度
						reg_in[addr_data].REG0_TEMPB=(uint16_t)(circuit_breaker_rx_buf[137]<<8)+circuit_breaker_rx_buf[138]; 		// R //B线温度
						reg_in[addr_data].REG0_TEMPC=(uint16_t)(circuit_breaker_rx_buf[139]<<8)+circuit_breaker_rx_buf[140]; 		// R //C线温度
						reg_in[addr_data].REG0_TEMPN=(uint16_t)(circuit_breaker_rx_buf[141]<<8)+circuit_breaker_rx_buf[142]; 		// R //N线温度
						reg_in[addr_data].REG0_SPARK=(uint16_t)(circuit_breaker_rx_buf[143]<<8)+circuit_breaker_rx_buf[144]; 		// R //打火报警
						reg_in[addr_data].REG0_NO_NLINE=(uint16_t)(circuit_breaker_rx_buf[145]<<8)+circuit_breaker_rx_buf[146]; 	// R //缺零线
						reg_in[addr_data].REG0_OVER_HARMONIC=(uint16_t)(circuit_breaker_rx_buf[147]<<8)+circuit_breaker_rx_buf[148];// R //谐波超标
						reg_in[addr_data].REG0_OPERATOR=(uint16_t)(circuit_breaker_rx_buf[149]<<8)+circuit_breaker_rx_buf[150]; 	// R //最后一次分合操作者
						reg_in[addr_data].REG0_AE=((uint64_t)circuit_breaker_rx_buf[151]<<56)+((uint64_t)circuit_breaker_rx_buf[152]<<48) 	\
												+((uint64_t)circuit_breaker_rx_buf[153]<<40)+((uint64_t)circuit_breaker_rx_buf[154]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[155]<<24)+(uint64_t)(circuit_breaker_rx_buf[156]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[157]<<8)+(uint64_t)(circuit_breaker_rx_buf[158]); 			// R //单相有功能量
						reg_in[addr_data].REG0_AEA=((uint64_t)circuit_breaker_rx_buf[159]<<56)+((uint64_t)circuit_breaker_rx_buf[160]<<48) 	\
												+((uint64_t)circuit_breaker_rx_buf[161]<<40)+((uint64_t)circuit_breaker_rx_buf[162]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[163]<<24)+(uint64_t)(circuit_breaker_rx_buf[164]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[165]<<8)+(uint64_t)(circuit_breaker_rx_buf[166]); 			// R //A相有功能量
						reg_in[addr_data].REG0_AEB=((uint64_t)circuit_breaker_rx_buf[167]<<56)+((uint64_t)circuit_breaker_rx_buf[168]<<48) 	\
												+((uint64_t)circuit_breaker_rx_buf[169]<<40)+((uint64_t)circuit_breaker_rx_buf[170]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[171]<<24)+(uint64_t)(circuit_breaker_rx_buf[172]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[173]<<8)+(uint64_t)(circuit_breaker_rx_buf[174]); 			// R //B相有功能量
						reg_in[addr_data].REG0_AEC=((uint64_t)circuit_breaker_rx_buf[175]<<56)+((uint64_t)circuit_breaker_rx_buf[176]<<48) 	\
												+((uint64_t)circuit_breaker_rx_buf[177]<<40)+((uint64_t)circuit_breaker_rx_buf[178]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[179]<<24)+(uint64_t)(circuit_breaker_rx_buf[180]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[181]<<8)+(uint64_t)(circuit_breaker_rx_buf[182]); 			// R //C相有功能量
						reg_in[addr_data].REG0_RAE=((uint64_t)circuit_breaker_rx_buf[183]<<56)+((uint64_t)circuit_breaker_rx_buf[184]<<48) 	\
												+((uint64_t)circuit_breaker_rx_buf[185]<<40)+((uint64_t)circuit_breaker_rx_buf[186]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[187]<<24)+(uint64_t)(circuit_breaker_rx_buf[188]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[189]<<8)+(uint64_t)(circuit_breaker_rx_buf[190]); 			// R //单相无功能量
						reg_in[addr_data].REG0_RAEA=((uint64_t)circuit_breaker_rx_buf[191]<<56)+((uint64_t)circuit_breaker_rx_buf[192]<<48) \
												+((uint64_t)circuit_breaker_rx_buf[193]<<40)+((uint64_t)circuit_breaker_rx_buf[194]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[195]<<24)+(uint64_t)(circuit_breaker_rx_buf[196]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[197]<<8)+(uint64_t)(circuit_breaker_rx_buf[198]); 		// R //A相无功能量
												
						reg_in[addr_data].REG0_RAEB=((uint64_t)circuit_breaker_rx_buf[199]<<56)+((uint64_t)circuit_breaker_rx_buf[200]<<48) \
												+((uint64_t)circuit_breaker_rx_buf[201]<<40)+((uint64_t)circuit_breaker_rx_buf[202]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[203]<<24)+(uint64_t)(circuit_breaker_rx_buf[204]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[205]<<8)+(uint64_t)(circuit_breaker_rx_buf[206]); 		// R //B相无功能量
						reg_in[addr_data].REG0_RAEC=((uint64_t)circuit_breaker_rx_buf[207]<<56)+((uint64_t)circuit_breaker_rx_buf[208]<<48) 	\
												+((uint64_t)circuit_breaker_rx_buf[209]<<40)+((uint64_t)circuit_breaker_rx_buf[210]<<32)    \
												+(uint64_t)(circuit_breaker_rx_buf[211]<<24)+(uint64_t)(circuit_breaker_rx_buf[212]<<16)    \
												+(uint64_t)(circuit_breaker_rx_buf[213]<<8)+(uint64_t)(circuit_breaker_rx_buf[214]); 		// R //C相无功能量
						reg_in[addr_data].REG0_FREQ=(uint16_t)(circuit_breaker_rx_buf[215]<<8)+circuit_breaker_rx_buf[216]; 		// R //电网频率
						reg_in[addr_data].REG0_ALARM_MAP1=(uint16_t)(circuit_breaker_rx_buf[217]<<8)+circuit_breaker_rx_buf[218]; 	// R //报警位映射1
						reg_in[addr_data].REG0_ALARM_MAP2=(uint16_t)(circuit_breaker_rx_buf[219]<<8)+circuit_breaker_rx_buf[220]; 	// R //报警位映射2
						reg_in[addr_data].REG0_ALARM_MAP3=(uint16_t)(circuit_breaker_rx_buf[221]<<8)+circuit_breaker_rx_buf[222]; 	// R //报警位映射3						
						device_data.cb_slave_device_have_ack_flag=1;
						memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
						return SYS_OK;
					}
					else
					{
						log_i("input checksum:%04x read_checksum:%04x",checksum,read_checksum);
						memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
						return SYS_FAIL;
					}					
				}
				else
				{
					log_i("input circuit_breaker_rx_buf[1]:%02x",circuit_breaker_rx_buf[1]);
					memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
					return SYS_FAIL;
				}				
			}
			else
			{
				log_i("input circuit_breaker_rx_buf[0]:%02x addr_data:%02x",circuit_breaker_rx_buf[0],addr_data);
				memset(circuit_breaker_rx_buf, 0, sizeof(circuit_breaker_rx_buf));
				return SYS_FAIL;
			}
        }
    }
	return SYS_E_TIMEOUT;
}

void circuit_breaker_control_thread(void* param)
{
	int err_code;
	uint8_t scan_flag=0;
	uint8_t scan_ok=0;
	uint8_t addr_tmp=0;
	uint8_t read_hold_flag=0;

	

	
	circuit_breaker_usart_init(circuit_breaker_USART7_RxError_IrqCallback,circuit_breaker_USART7_RxFull_IrqCallback,circuit_breaker_USART7_RxTimeout_IrqCallback);
	
	memset(reg_in, 0x00, sizeof(cb_reg_input)*CIRCUIT_BREAKER_MAX_NUM);
	memset(reg_hold, 0x00, sizeof(cb_reg_holding)*CIRCUIT_BREAKER_MAX_NUM);
	memset(reg_discrete, 0x00, sizeof(cb_reg_discrete)*CIRCUIT_BREAKER_MAX_NUM);
	device_data.cb_query_slave_cnt=1;
	device_data.cb_slave_addr_cnt=0;
	
	while(1)
	{
		/////////////////////////////////////////////////
		if(scan_flag==0)//查询设备
		{	
			//log_i("cb_query_slave_cnt %d",device_data.cb_query_slave_cnt);
			read_input_send_data(device_data.cb_query_slave_cnt);
			err_code=read_input_register_handle(device_data.cb_query_slave_cnt,20);

			if(device_data.cb_slave_device_have_ack_flag==1)
			{
				device_data.cb_slave_device_have_ack_flag=0;
				scan_ok=1;
				device_data.cb_slave_addr[device_data.cb_slave_addr_cnt]=device_data.cb_query_slave_cnt;
			}
			else
			{
				device_data.cb_slave_addr[device_data.cb_slave_addr_cnt]=0xff;
			}
			device_data.cb_query_slave_cnt++;
			device_data.cb_slave_addr_cnt++;
			if(device_data.cb_slave_addr_cnt>=0x20)
			{
				device_data.cb_query_slave_cnt=1;
				device_data.cb_slave_addr_cnt=0;
				if(scan_ok==1)
				{
					scan_flag=1;
					device_data.cb_slave_addr_cnt=0;
				}
			}
		}
		else
		{
			/////////////////////////////////////////////////////////////////////////////
			if(device_data.cb_slave_addr[device_data.cb_slave_addr_cnt]!=0xff)
			{
				addr_tmp=device_data.cb_slave_addr[device_data.cb_slave_addr_cnt];
				if(read_hold_flag==0)
				{
					read_hold_send_data(addr_tmp);
					err_code=read_hold_handle(addr_tmp,20);
				}
				//////////////////////////////////////////////////////////
				read_input_send_data(addr_tmp);//3000
				err_code=read_input_register_handle(addr_tmp,20);	
				device_data.cb_slave_device_have_ack_flag=0;	
				//////////////////////////////////////////////////////////
				read_discrete_send_data(addr_tmp);//2000
				err_code=read_discrete_handle(addr_tmp,20);
				//////////////////////////////////////////////////////////
			}
			device_data.cb_slave_addr_cnt++;
			if(device_data.cb_slave_addr_cnt>=0x20)
			{
				device_data.cb_slave_addr_cnt=0;
				read_hold_flag=1;
			}
			/////////////////////////////////////////////////////////////////////////////////
			if(device_data.cbclose_flag==1)
			{
				device_data.cbclose_flag=0;
				set_coil_send_data(device_data.cbclose_data_addr,device_data.reclose_data_flag);
				read_coil_handle(device_data.cbclose_data_addr,30);
				device_data.reclose_data_flag=0;
				device_data.cbclose_data_addr=0xff;
			}
			if(device_data.cb_hold_flag==1)
			{
				device_data.cb_hold_flag=0;
				write_hold_send_data(device_data.cbclose_data_addr);
				write_hold_handle(device_data.cbclose_data_addr,30);
			}
		}
		os_thread_sleep(os_msec_to_ticks(50)); 
	}
	os_thread_delete(NULL);
}

void circuit_breaker_process_init(void)
{
	int ret=SYS_FAIL;
	ret = os_thread_create(&circuit_breaker_thread, //任务控制块指针
							"circuit_breaker_thread",//任务名字
							circuit_breaker_control_thread, //任务入口函数
							NULL,//任务入口函数参数
							&circuit_breaker_stack,//任务栈大小
							OS_PRIO_5);	//任务的优先级							
   if(ret==SYS_FAIL)
   {
		log_e("circuit_breaker","circuit_breaker thread create error!");
   }
}
