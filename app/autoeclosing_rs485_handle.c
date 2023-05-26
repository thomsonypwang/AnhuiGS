#include "autoeclosing_rs485_handle.h"
#include "autoeclosing_rs485_dir.h"
#include "project_pin_use_config.h"
#include <stdarg.h>
#include <string.h>

#include "sys_os.h"
#include "sys_log.h"
#include "sys_errno.h"

static os_thread_t autoeclosing_thread= NULL;
static os_thread_stack_define(autoeclosing_stack, 1*1024);

//uint16_t autoeclosing_rx_sta = 0;
uint8_t timeout_flag=0;
uint8_t rx_cnt=0;
uint8_t autoeclosing_rx_buf[128];
uint8_t autoeclosing_tx_buf[128];
autoeclosing_register_data  autoeclosing_data;

const uint16_t autoeclosing_polynom = 0xA001;

uint16_t autoeclosing_modbus_crc16(uint8_t *ptr, uint16_t len)
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
				crc ^= autoeclosing_polynom;
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
static void USART7_RxFull_IrqCallback(void)
{
    uint8_t u8Data = (uint8_t)USART_ReadData(AUTOCLOSING_USART_ID);
	
	autoeclosing_rx_buf[rx_cnt] = u8Data;
	rx_cnt++;
	if(rx_cnt>128)
	{
		rx_cnt=0;
	}
}

/**
 * @brief  USART error IRQ callback.
 * @param  None
 * @retval None
 */
static void USART7_RxError_IrqCallback(void)
{
	(void)USART_ReadData(AUTOCLOSING_USART_ID);
    USART_ClearStatus(AUTOCLOSING_USART_ID, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

static void USART7_RxTimeout_IrqCallback(void)
{
	timeout_flag=1;
    TMR0_Stop(AUTOCLOSING_USART_TMR0, AUTOCLOSING_USART_TMR0_CH);
    USART_ClearStatus(AUTOCLOSING_USART_ID, USART_FLAG_RX_TIMEOUT);
}

int read_register_handle(uint16_t timeout)
{
    uint16_t time = 0;
	uint16_t checksum=0;
	uint16_t read_checksum=0;
	
    while (time < timeout)
    {
        if(timeout_flag==1)
        {
            timeout_flag = 0;
			rx_cnt=0;
			USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), DISABLE);
			if(autoeclosing_rx_buf[0]==AUTOCLOSING_ADDR)
			{
				if(autoeclosing_rx_buf[1]==0x03)
				{
					checksum=autoeclosing_modbus_crc16(&autoeclosing_rx_buf[0],autoeclosing_rx_buf[2]+3);
					read_checksum=(uint16_t)(autoeclosing_rx_buf[4+autoeclosing_rx_buf[2]]<<8)+autoeclosing_rx_buf[3+autoeclosing_rx_buf[2]];
					if(checksum==read_checksum)
					{
						autoeclosing_data.voltage_value=(uint16_t)(autoeclosing_rx_buf[3]<<8)+autoeclosing_rx_buf[4];
						autoeclosing_data.current_value=(uint16_t)(autoeclosing_rx_buf[5]<<8)+autoeclosing_rx_buf[6];
						autoeclosing_data.creepage_current_value=(uint16_t)(autoeclosing_rx_buf[7]<<8)+autoeclosing_rx_buf[8];
						autoeclosing_data.reclosing_times=(uint16_t)(autoeclosing_rx_buf[9]<<8)+autoeclosing_rx_buf[10];
						autoeclosing_data.over_voltage_value=(uint16_t)(autoeclosing_rx_buf[11]<<8)+autoeclosing_rx_buf[12];
						autoeclosing_data.low_voltage_value=(uint16_t)(autoeclosing_rx_buf[13]<<8)+autoeclosing_rx_buf[14];
						autoeclosing_data.device_status=(uint16_t)(autoeclosing_rx_buf[15]<<8)+autoeclosing_rx_buf[16];
						autoeclosing_data.fault_opening_times=(uint16_t)(autoeclosing_rx_buf[17]<<8)+autoeclosing_rx_buf[18];
						//log_i("autoeclosing ok");
						memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
						USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
						return SYS_OK;
					}
					else
					{
						log_i("read_register_handle checksum:%04x read_checksum:%04x",checksum,read_checksum);
						memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
						USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
						return SYS_FAIL;
					}					
				}
				else
				{
					log_i("autoeclosing_rx_buf[1]:%02x",autoeclosing_rx_buf[1]);
					memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
					USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
					return SYS_FAIL;
				}				
			}
			else
			{
				log_i("autoeclosing_rx_buf[0]:%02x",autoeclosing_rx_buf[0]);
				memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
				USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
				return SYS_FAIL;
			}
        }
        time++;
        os_thread_sleep(os_msec_to_ticks(10)); 
    }
	return SYS_E_TIMEOUT;
}

int read_do_handle(uint16_t timeout)
{
    uint16_t time = 0;
	uint16_t checksum=0;
	uint16_t read_checksum=0;
	
    while (time < timeout)
    {	
        if (timeout_flag==1)
        {
            timeout_flag = 0;
			rx_cnt=0;
			USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), DISABLE);
			if(autoeclosing_rx_buf[0]==AUTOCLOSING_ADDR)
			{
				if(autoeclosing_rx_buf[1]==0x01)
				{
					checksum=autoeclosing_modbus_crc16(&autoeclosing_rx_buf[0],4);
					read_checksum=(uint16_t)(autoeclosing_rx_buf[4+autoeclosing_rx_buf[2]]<<8)+autoeclosing_rx_buf[3+autoeclosing_rx_buf[2]];
					if(checksum==read_checksum)
					{
						autoeclosing_data.do_status=autoeclosing_rx_buf[3];
						memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
						USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
						//log_i("do_status:%d",autoeclosing_data.do_status);
						return SYS_OK;
					}
					else
					{
						log_i("checksum:%04x read_checksum 2:%04x",checksum,read_checksum);
						memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
						USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
						return SYS_FAIL;
					}					
				}
				else
				{
					log_i("autoeclosing_rx_buf[1]:%02x",autoeclosing_rx_buf[1]);
					memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
					USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
					return SYS_FAIL;
				}				
			}
			else
			{
				log_i("autoeclosing_rx_buf[0]:%02x",autoeclosing_rx_buf[0]);
				memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
				USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
				return SYS_FAIL;
			}
        }
        time++;
        os_thread_sleep(os_msec_to_ticks(10)); 
    }
	return SYS_E_TIMEOUT;
}


int send_closing_on_off_handle(uint8_t cmd,uint16_t timeout)
{
    uint16_t time = 0;
	uint16_t checksum=0;
	uint16_t read_checksum=0;
	
    while (time < timeout)
    {
        if (timeout_flag==1)
        {
            timeout_flag = 0;
			rx_cnt=0;
			USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), DISABLE);
			if(autoeclosing_rx_buf[0]==AUTOCLOSING_ADDR)
			{
				if(autoeclosing_rx_buf[1]==0x05)
				{
					checksum=autoeclosing_modbus_crc16(&autoeclosing_rx_buf[0],autoeclosing_rx_buf[2]+3);
					read_checksum=(uint16_t)(autoeclosing_rx_buf[4+autoeclosing_rx_buf[2]]<<8)+autoeclosing_rx_buf[3+autoeclosing_rx_buf[2]];
					if(checksum==read_checksum)
					{
						if(autoeclosing_rx_buf[3]==cmd)
						{
							memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
							USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
							return SYS_OK;
						}
						else
						{
							log_i("autoeclosing_rx_buf[3]:%02x",autoeclosing_rx_buf[3]);
							memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
							USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
							return SYS_FAIL;
						}
					}
					else
					{
						log_i("checksum:%04x read_checksum 2:%04x",checksum,read_checksum);
						memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
						USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
						return SYS_FAIL;
					}					
				}
				else
				{
					log_i("autoeclosing_rx_buf[0]:%02x",autoeclosing_rx_buf[0]);
					memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
					USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
					return SYS_FAIL;
				}				
			}
			else
			{
				memset(autoeclosing_rx_buf, 0, sizeof(autoeclosing_rx_buf));
				USART_FuncCmd(AUTOCLOSING_USART_ID, (USART_RX | USART_INT_RX| USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT|USART_TX), ENABLE);
				return SYS_FAIL;
			}
        }
        time++;
        os_thread_sleep(os_msec_to_ticks(10)); 
    }
	return SYS_E_TIMEOUT;
}

void read_register_autoeclosing(void)
{
	uint16_t i;
	uint16_t checksum=0;
	
	autoeclosing_tx_buf[0]=AUTOCLOSING_ADDR;
	autoeclosing_tx_buf[1]=0x03;
	autoeclosing_tx_buf[2]=0x00;
	autoeclosing_tx_buf[3]=0x01;
	autoeclosing_tx_buf[4]=0x00;
	autoeclosing_tx_buf[5]=0x0b;
	checksum=autoeclosing_modbus_crc16(&autoeclosing_tx_buf[0],6);
	autoeclosing_tx_buf[6] =(checksum)&0xff;	
	autoeclosing_tx_buf[7] =(checksum>>8)&0xff;

    while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}           
	for (i = 0; i < 8; i++)
	{
		USART_WriteData(AUTOCLOSING_USART_ID, autoeclosing_tx_buf[i]);
		while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}
}

void read_do_autoeclosing(void)
{
	uint16_t i;
	uint16_t checksum=0;
	
	autoeclosing_tx_buf[0]=AUTOCLOSING_ADDR;
	autoeclosing_tx_buf[1]=0x01;
	autoeclosing_tx_buf[2]=0x00;
	autoeclosing_tx_buf[3]=0x30;
	autoeclosing_tx_buf[4]=0x00;
	autoeclosing_tx_buf[5]=0x04;
	
	checksum=autoeclosing_modbus_crc16(&autoeclosing_tx_buf[0],6);
	autoeclosing_tx_buf[6] =(checksum)&0xff;
	autoeclosing_tx_buf[7] =(checksum>>8)&0xff;

	while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}  
	for (i = 0; i < 8; i++)
	{
		USART_WriteData(AUTOCLOSING_USART_ID, autoeclosing_tx_buf[i]);
		while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}
}

void read_di_autoeclosing(void)
{
	uint16_t i;
	uint16_t checksum=0;
	
	autoeclosing_tx_buf[0]=AUTOCLOSING_ADDR;
	autoeclosing_tx_buf[1]=0x02;
	autoeclosing_tx_buf[2]=0x00;
	autoeclosing_tx_buf[3]=0x00;
	autoeclosing_tx_buf[4]=0x00;
	autoeclosing_tx_buf[5]=0x0c;
	checksum=autoeclosing_modbus_crc16(&autoeclosing_tx_buf[0],6);
	autoeclosing_tx_buf[6] =(checksum)&0xff;	
	autoeclosing_tx_buf[7] =(checksum>>8)&0xff;	

	while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}  
	for (i = 0; i < 8; i++)
	{
		USART_WriteData(AUTOCLOSING_USART_ID, autoeclosing_tx_buf[i]);
		while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}
}

void send_closing_on_autoeclosing(void)
{
	uint16_t i;
	uint16_t checksum=0;
	
	log_i("send_closing_on_autoeclosing");
	autoeclosing_tx_buf[0]=AUTOCLOSING_ADDR;
	autoeclosing_tx_buf[1]=0x05;
	autoeclosing_tx_buf[2]=0x00;
	autoeclosing_tx_buf[3]=0x31;
	autoeclosing_tx_buf[4]=0xff;
	autoeclosing_tx_buf[5]=0x00;
	checksum=autoeclosing_modbus_crc16(&autoeclosing_tx_buf[0],6);
	autoeclosing_tx_buf[6] =(checksum)&0xff;	
	autoeclosing_tx_buf[7] =(checksum>>8)&0xff;	
	
	while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}  
	for (i = 0; i < 8; i++)
	{
		USART_WriteData(AUTOCLOSING_USART_ID, autoeclosing_tx_buf[i]);
		while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}
}

void send_closing_off_autoeclosing(void)
{
	uint16_t i;
	uint16_t checksum=0;
	
	log_i("send_closing_off_autoeclosing");
	autoeclosing_tx_buf[0]=AUTOCLOSING_ADDR;
	autoeclosing_tx_buf[1]=0x05;
	autoeclosing_tx_buf[2]=0x00;
	autoeclosing_tx_buf[3]=0x32;
	autoeclosing_tx_buf[4]=0xff;
	autoeclosing_tx_buf[5]=0x00;
	checksum=autoeclosing_modbus_crc16(&autoeclosing_tx_buf[0],6);
	autoeclosing_tx_buf[6] =(checksum)&0xff;	
	autoeclosing_tx_buf[7] =(checksum>>8)&0xff;	
	
	while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}  
	for (i = 0; i < 8; i++)
	{
		USART_WriteData(AUTOCLOSING_USART_ID, autoeclosing_tx_buf[i]);
		while (RESET == USART_GetStatus(AUTOCLOSING_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}
}

void autoeclosing_control_thread(void* param)
{
	uint8_t time_1sec_cnt=0;
	int err_code;
	
	autoeclosing_usart_init(USART7_RxError_IrqCallback,USART7_RxFull_IrqCallback,USART7_RxTimeout_IrqCallback);
	
	while(1)
	{
		time_1sec_cnt++;
		if(time_1sec_cnt>=50)
		{
			time_1sec_cnt=0;
			read_register_autoeclosing();
			err_code=read_register_handle(200);
			if(err_code!=SYS_OK)
			{
				log_i("autoeclosing err");
			}
			os_thread_sleep(os_msec_to_ticks(50)); 
			read_do_autoeclosing();
			err_code=read_do_handle(200);
			if(err_code!=SYS_OK)
			{
				log_i("autoeclosing err");
			}
			os_thread_sleep(os_msec_to_ticks(100)); 
		}
		if(device_data.autoeclosing_on_off_flag==1)
		{
			device_data.autoeclosing_on_off_flag=0;
			if(device_data.autoeclosing_on_off_code==1)
			{
				send_closing_on_autoeclosing();
				err_code=send_closing_on_off_handle(0x31,10);
				if(err_code!=SYS_OK)
				{
					log_i("autoeclosing err");
				}
			}
			else if(device_data.autoeclosing_on_off_code==2)
			{
				send_closing_off_autoeclosing();
				err_code=send_closing_on_off_handle(0x32,10);
				if(err_code!=SYS_OK)
				{
					log_i("autoeclosing err");
				}
			}
			device_data.autoeclosing_on_off_code=0;
		}
		os_thread_sleep(os_msec_to_ticks(100)); 
//		read_do_autoeclosing();
//		os_thread_sleep(os_msec_to_ticks(30000)); 
//		send_closing_on_autoeclosing();
//		os_thread_sleep(os_msec_to_ticks(30000)); 
//		send_closing_off_autoeclosing();
//		os_thread_sleep(os_msec_to_ticks(30000)); 
	}
	os_thread_delete(NULL);
}

void autoeclosing_process_init(void)
{
	int ret=SYS_FAIL;
	ret = os_thread_create(&autoeclosing_thread, //任务控制块指针
							"autoeclosing_thread",//任务名字
							autoeclosing_control_thread, //任务入口函数
							NULL,//任务入口函数参数
							&autoeclosing_stack,//任务栈大小
							OS_PRIO_2);	//任务的优先级							
   if(ret==SYS_FAIL)
   {
		log_e("autoeclosing","autoeclosing thread create error!");
   }
}

uint16_t read_autoeclosing_voltage_value(void)
{
	return autoeclosing_data.voltage_value;
}

float read_autoeclosing_current_value(void)
{
	return autoeclosing_data.current_value*0.01;
}

uint16_t read_autoeclosing_creepage_current_value(void)
{
	return autoeclosing_data.creepage_current_value;
}

uint16_t read_autoeclosing_reclosing_times(void)
{
	return autoeclosing_data.reclosing_times;
}

uint16_t read_autoeclosing_over_voltage_value(void)
{
	return autoeclosing_data.over_voltage_value;
}

uint16_t read_autoeclosing_low_voltage_value(void)
{
	return autoeclosing_data.low_voltage_value;
}

uint16_t read_autoeclosing_device_status(void)
{
	return autoeclosing_data.device_status;
}

uint16_t read_autoeclosing_fault_opening_times(void)
{
	return autoeclosing_data.fault_opening_times;
}

uint16_t read_autoeclosing_do_status(void)
{
	return autoeclosing_data.do_status;
}