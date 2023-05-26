#include "monitor_board_handle.h"
#include "monitor_board_dir.h"
#include "project_pin_use_config.h"
#include "sys_os.h"
#include "sys_log.h"
#include "sys_errno.h"
#include "project_psm_control.h"

static os_thread_t modbus_thread= NULL;
static os_thread_stack_define(modbus_stack, 2*1024);

uint8_t buf_rev[256];
uint8_t buf_rev_cnt=0;
rs485_info_conf mcu_info_data;
power_monitoring_board_data  power_board_data[SLAVE_MAX_NUM];

sys_slave_type slave_device_tmp;
uint8_t rev_timerout_cnt;
uint8_t rev_timerout_flag;


uint8_t control_slave_relay_cnt;

const uint16_t polynom = 0xA001;

uint16_t modbus_crc16(uint8_t *ptr, uint16_t len)
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
				crc ^= polynom;
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
static void USART4_RxFull_IrqCallback(void)
{
	uint16_t mcu_checksum_cnt=0;
	uint16_t mcu_checksum_read=0;
	uint16_t t = 0;
	
    uint8_t tmp = (uint8_t)USART_ReadData(MOD_485_USART_ID);
	
	rev_timerout_cnt=0;
	if(buf_rev_cnt==0)
	{
		if(tmp==MCU_HEAD1_CODE)
		{
			buf_rev[buf_rev_cnt]=tmp;
			buf_rev_cnt++;
		}
	}
	else if(buf_rev_cnt==1)
	{
		if(tmp==MCU_HEAD2_CODE)
		{
			buf_rev[buf_rev_cnt]=tmp;
			buf_rev_cnt++;	
			rev_timerout_flag=1;
		}
		else
		{
			buf_rev_cnt=0;
		}
	}
	else if(buf_rev_cnt==2)
	{
		buf_rev[buf_rev_cnt]=tmp;
		mcu_info_data.mcu_id=buf_rev[buf_rev_cnt];
		buf_rev_cnt++;
	}
	else if((buf_rev_cnt==3)||(buf_rev_cnt==4))
	{
		buf_rev[buf_rev_cnt]=tmp;
		buf_rev_cnt++;
		if(buf_rev_cnt==5)
		{
			mcu_info_data.mcu_data_len=(uint16_t)(buf_rev[3]<<8)+buf_rev[4];
			if(mcu_info_data.mcu_data_len>=256)
			{
				buf_rev_cnt=0;
				//log_i("mcu error mcu_data_len==%d",mcu_info_data.mcu_data_len);
			}
		}
	}
	else
	{
		buf_rev[buf_rev_cnt]=tmp;
		buf_rev_cnt++;
		if(buf_rev_cnt>=mcu_info_data.mcu_data_len+7)
		{
			mcu_checksum_read=(uint16_t)(buf_rev[mcu_info_data.mcu_data_len+3]<<8)+buf_rev[mcu_info_data.mcu_data_len+4];
			mcu_checksum_cnt=modbus_crc16(&buf_rev[2],mcu_info_data.mcu_data_len+1);
			if(mcu_checksum_read==mcu_checksum_cnt)
			{
				if(buf_rev[5]==MCU_FUN_ASK_SEARCH)
				{
					slave_device_tmp.slave_addr=buf_rev[2];
					mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[6]<<8)+buf_rev[7];
					if(mcu_info_data.rs485_addr_buf==ADDR_DRIVER_TYPE)
					{
						slave_device_tmp.device_type=buf_rev[9];
					}
					////////////////////////////////////////////////////////////////////////
					mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[18]<<8)+buf_rev[19];
					if(mcu_info_data.rs485_addr_buf==ADDR_DRIVER_NAME)
					{
						for(t = 0; t <34; t++) 
						{
							slave_device_tmp.device_name[t]=buf_rev[20+t];
						}
					}
					device_data.slave_device_have_ack_flag=1;
				}
				else if(buf_rev[5]==MCU_FUN_ASK_READ)
				{
					if(device_data.own_slave_device[device_data.read_slave_device_cnt].slave_addr==buf_rev[2])
					{
						if(device_data.own_slave_device[device_data.read_slave_device_cnt].device_type==POWER_MONITOR_BOARD)
						{
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[6]<<8)+buf_rev[7];
							if(mcu_info_data.rs485_addr_buf==ADDR_RELAY1_STATUS)
							{
								power_board_data[device_data.read_slave_device_cnt].relay_status=(uint16_t)(buf_rev[8]<<8)+buf_rev[9];
							}
							//////////////////////////////////////////////////////////////////////////
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[10]<<8)+buf_rev[11];
							if(mcu_info_data.rs485_addr_buf==ADDR_AC1_VOLTAGE)
							{
								power_board_data[device_data.read_slave_device_cnt].ac_voltage_value=(uint16_t)(buf_rev[12]<<8)+buf_rev[13];
							}
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[14]<<8)+buf_rev[15];
							if(mcu_info_data.rs485_addr_buf==ADDR_AC1_CURRENT)
							{
								power_board_data[device_data.read_slave_device_cnt].ac_current_value=(uint16_t)(buf_rev[16]<<8)+buf_rev[17];
							}
							///////////////////////////////DC1/////////////////////////////////////////
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[18]<<8)+buf_rev[19];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC1_VOLTAGE)
							{
								power_board_data[device_data.read_slave_device_cnt].dc1_voltage_value=(uint16_t)(buf_rev[20]<<8)+buf_rev[21];
							}
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[22]<<8)+buf_rev[23];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC1_CURRENT)
							{
								power_board_data[device_data.read_slave_device_cnt].dc1_current_value=(uint16_t)(buf_rev[24]<<8)+buf_rev[25];
							}
							/////////////////////////////DC2/////////////////////////////////////////	
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[26]<<8)+buf_rev[27];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC2_VOLTAGE)
							{
								power_board_data[device_data.read_slave_device_cnt].dc2_voltage_value=(uint16_t)(buf_rev[28]<<8)+buf_rev[29];
							}
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[30]<<8)+buf_rev[31];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC2_CURRENT)
							{
								power_board_data[device_data.read_slave_device_cnt].dc2_current_value=(uint16_t)(buf_rev[32]<<8)+buf_rev[33];
							}
							//////////////////////////////////////////////////////////////////////////
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[34]<<8)+buf_rev[35];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC3_VOLTAGE)
							{
								power_board_data[device_data.read_slave_device_cnt].dc3_voltage_value=(uint16_t)(buf_rev[36]<<8)+buf_rev[37];
							}
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[38]<<8)+buf_rev[39];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC3_CURRENT)
							{
								power_board_data[device_data.read_slave_device_cnt].dc3_current_value=(uint16_t)(buf_rev[40]<<8)+buf_rev[41];
							}
							//////////////////////////////////////////////////////////////////////////
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[42]<<8)+buf_rev[43];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC4_VOLTAGE)
							{
								power_board_data[device_data.read_slave_device_cnt].dc4_voltage_value=(uint16_t)(buf_rev[44]<<8)+buf_rev[45];
							}
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[46]<<8)+buf_rev[47];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC4_CURRENT)
							{
								power_board_data[device_data.read_slave_device_cnt].dc4_current_value=(uint16_t)(buf_rev[48]<<8)+buf_rev[49];
							}
							//////////////////////////////////////////////////////////////////////////
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[50]<<8)+buf_rev[51];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC5_VOLTAGE)
							{
								power_board_data[device_data.read_slave_device_cnt].dc5_voltage_value=(uint16_t)(buf_rev[52]<<8)+buf_rev[53];
							}
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[54]<<8)+buf_rev[55];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC5_CURRENT)
							{
								power_board_data[device_data.read_slave_device_cnt].dc5_current_value=(uint16_t)(buf_rev[56]<<8)+buf_rev[57];
							}
							//////////////////////////////////////////////////////////////////////////
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[58]<<8)+buf_rev[59];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC6_VOLTAGE)
							{
								power_board_data[device_data.read_slave_device_cnt].dc6_voltage_value=(uint16_t)(buf_rev[60]<<8)+buf_rev[61];
							}
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[62]<<8)+buf_rev[63];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC6_CURRENT)
							{
								power_board_data[device_data.read_slave_device_cnt].dc6_current_value=(uint16_t)(buf_rev[64]<<8)+buf_rev[65];
							}
							//////////////////////////////////////////////////////////////////////////
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[66]<<8)+buf_rev[67];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC7_VOLTAGE)
							{
								power_board_data[device_data.read_slave_device_cnt].dc7_voltage_value=(uint16_t)(buf_rev[68]<<8)+buf_rev[69];
							}
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[70]<<8)+buf_rev[71];
							if(mcu_info_data.rs485_addr_buf==ADDR_DC7_CURRENT)
							{
								power_board_data[device_data.read_slave_device_cnt].dc7_current_value=(uint16_t)(buf_rev[72]<<8)+buf_rev[73];
							}
							//////////////////////////////////////////////////////////////////////////
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[74]<<8)+buf_rev[75];
							if(mcu_info_data.rs485_addr_buf==ADDR_SOFTWARE_VER)
							{
								power_board_data[device_data.read_slave_device_cnt].device_sw_ver=(uint16_t)(buf_rev[76]<<8)+buf_rev[77];
							}
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[78]<<8)+buf_rev[79];
							if(mcu_info_data.rs485_addr_buf==ADDR_HARDWARE_VER)
							{
								power_board_data[device_data.read_slave_device_cnt].device_hw_ver=(uint16_t)(buf_rev[80]<<8)+buf_rev[81];
							}
						}
					}
					//////////////////////////////////////////////////////////////////////////
				}	
				else if(buf_rev[5]==MCU_FUN_ASK_WRITE)
				{
					if(device_data.own_slave_device[control_slave_relay_cnt].slave_addr==buf_rev[2])
					{
						if(device_data.own_slave_device[control_slave_relay_cnt].device_type==POWER_MONITOR_BOARD)
						{
							mcu_info_data.rs485_addr_buf=(uint16_t)(buf_rev[6]<<8)+buf_rev[7];
							if(mcu_info_data.rs485_addr_buf==ADDR_RELAY1_STATUS)
							{
								power_board_data[control_slave_relay_cnt].relay_status=(uint16_t)(buf_rev[8]<<8)+buf_rev[9];
							}						
						}
					}
				}						
			}
			else
			{
				//log_i("err mcu_checksum_read=0x%04x mcu_checksum_cnt=0x%04x ",mcu_checksum_read,mcu_checksum_cnt);
			}
			buf_rev_cnt=0;
		}		
	}
}

/**
 * @brief  USART error IRQ callback.
 * @param  None
 * @retval None
 */
static void USART4_RxError_IrqCallback(void)
{
	(void)USART_ReadData(MOD_485_USART_ID);
    USART_ClearStatus(MOD_485_USART_ID, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

void search_slave_driver_id(uint8_t slave_addr)
{
	uint16_t checksum=0;
	uint16_t len=6;
	uint16_t t = 0;
	uint8_t buf_send[64];

	buf_send[0]	=MCU_HEAD1_CODE;
	buf_send[1]	=MCU_HEAD2_CODE;
	buf_send[2]	=slave_addr;
	buf_send[3] =(len>>8)&0xff;
	buf_send[4] =(len)&0xff;	
	buf_send[5]	=MCU_FUN_SEARCH;

	buf_send[6] =(ADDR_DRIVER_TYPE>>8)&0xff;
	buf_send[7] =(ADDR_DRIVER_TYPE)&0xff;
	buf_send[8] =0xfe;

	checksum=modbus_crc16(&buf_send[2],len+1);
	buf_send[9] =(checksum>>8)&0xff;
	buf_send[10] =(checksum)&0xff;

	buf_send[11] =MCU_END1_CODE;
	buf_send[12] =MCU_END2_CODE;

//    while (RESET == USART_GetStatus(MOD_485_USART_ID, USART_FLAG_TX_EMPTY)) {}       
//	for(t = 0; t < 13; t++) 
//	{
//		USART_WriteData(MOD_485_USART_ID, buf_send[t]);
//		while (RESET == USART_GetStatus(MOD_485_USART_ID, USART_FLAG_TX_EMPTY)) {}
//	}	
	USART_UART_Trans(MOD_485_USART_ID,buf_send,13,200000);
}

void control_slave_relay(uint8_t slave_addr,uint16_t relay_data)
{
	uint16_t checksum=0;
	uint16_t len=7;
	uint16_t t = 0;
	uint8_t buf_send[64];

	buf_send[0]	=MCU_HEAD1_CODE;
	buf_send[1]	=MCU_HEAD2_CODE;
	buf_send[2]	=slave_addr;
	buf_send[3] =(len>>8)&0xff;
	buf_send[4] =(len)&0xff;	
	buf_send[5]	=MCU_FUN_WRITE;

	buf_send[6] =(ADDR_RELAY1_STATUS>>8)&0xff;
	buf_send[7] =(ADDR_RELAY1_STATUS)&0xff;
	buf_send[8] =(relay_data>>8)&0xff;
	buf_send[9] =(relay_data)&0xff;	
	
	checksum=modbus_crc16(&buf_send[2],len+1);
	buf_send[10] =(checksum>>8)&0xff;
	buf_send[11] =(checksum)&0xff;

	buf_send[12] =MCU_END1_CODE;
	buf_send[13] =MCU_END2_CODE;

//    while (RESET == USART_GetStatus(MOD_485_USART_ID, USART_FLAG_TX_EMPTY)) {}       
//	for(t = 0; t < 14; t++) 
//	{
//		USART_WriteData(MOD_485_USART_ID, buf_send[t]);
//		while (RESET == USART_GetStatus(MOD_485_USART_ID, USART_FLAG_TX_EMPTY)) {}
//	}
	USART_UART_Trans(MOD_485_USART_ID,buf_send,14,200000);
}

void read_slave_data(uint8_t slave_addr,uint8_t slave_type)
{
	uint16_t checksum=0;
	uint16_t len=7;
	uint16_t t = 0;
	uint8_t buf_send[64];

	buf_send[0]	=MCU_HEAD1_CODE;
	buf_send[1]	=MCU_HEAD2_CODE;
	buf_send[2]	=slave_addr;
	buf_send[3] =(len>>8)&0xff;
	buf_send[4] =(len)&0xff;	
	buf_send[5]	=MCU_FUN_READ;

	buf_send[6] =(ADDR_DRIVER_TYPE>>8)&0xff;
	buf_send[7] =(ADDR_DRIVER_TYPE)&0xff;
	buf_send[8] =0x00;
	buf_send[9] =slave_type;
	
	checksum=modbus_crc16(&buf_send[2],len+1);
	buf_send[10] =(checksum>>8)&0xff;
	buf_send[11] =(checksum)&0xff;

	buf_send[12] =MCU_END1_CODE;
	buf_send[13] =MCU_END2_CODE;

    while (RESET == USART_GetStatus(MOD_485_USART_ID, USART_FLAG_TX_EMPTY)) {}       
	for(t = 0; t < 14; t++) 
	{
		USART_WriteData(MOD_485_USART_ID, buf_send[t]);
		while (RESET == USART_GetStatus(MOD_485_USART_ID, USART_FLAG_TX_EMPTY)) {}
	}	
	USART_UART_Trans(MOD_485_USART_ID,buf_send,14,200000);
}

void modbus_control_thread(void* param)
{
	uint16_t timer_5s_cnt;
	int i;
	
	monitor_usart_init(USART4_RxError_IrqCallback,USART4_RxFull_IrqCallback);
	for (i = 0; i < SLAVE_MAX_NUM; i++)
	{
		device_data.power_monitoring_relay_data[i]=0x03ff;
	}
	device_data.power_monitoring_relay_data[0]&=~FAN_RELAY;
	device_data.power_monitoring_relay_data[0]&=~LIGHTING_RELAY;
	device_data.power_monitoring_relay_data[0]&=~DC7_RELAY;
	while(1)
	{
		if(sys_psm.slave_ok==1)
		{
			//////////////////////////////////////////////////////////////////////
			timer_5s_cnt++;
			if(timer_5s_cnt>=10*5)
			{
				device_data.read_slave_device_cnt=0;
				for (i = 0; i < device_data.slave_device_max; i++)
				{
					read_slave_data(device_data.own_slave_device[i].slave_addr,device_data.own_slave_device[i].device_type);
					os_thread_sleep(os_msec_to_ticks(200)); 
					device_data.read_slave_device_cnt++;
				}
				timer_5s_cnt=0;
			}
			//////////////////////////////////////////////////////////////////////
			for (i = 0; i < device_data.slave_device_max; i++)
			{
				if(device_data.own_slave_device[i].device_type==POWER_MONITOR_BOARD)
				{
					if(power_board_data[i].relay_status!=device_data.power_monitoring_relay_data[i])
					{
						power_board_data[i].relay_status=device_data.power_monitoring_relay_data[i];
						control_slave_relay_cnt=i;
						control_slave_relay(device_data.own_slave_device[i].slave_addr,power_board_data[i].relay_status);
						os_thread_sleep(os_msec_to_ticks(200)); 
					}				
				}
			}
			//////////////////////////////////////////////////////////////////////
			if(device_data.set_scan_flag==1)
			{
				device_data.set_scan_flag=0;				
				device_data.query_slave_times_cnt=0;
				device_data.query_slave_cnt=0;
				sys_psm.slave_ok=0;
				read_write_slave_ok_flash(WRITE_PSM);
				device_data.slave_device_max=0;
			}
			//////////////////////////////////////////////////////////////////////
		}
		else//查询从设备
		{
			if(device_data.query_slave_finish_flag==0)
			{
				timer_5s_cnt++;
				if(timer_5s_cnt>=2)
				{
					timer_5s_cnt=0;
					search_slave_driver_id(device_data.query_slave_cnt);//发送查询设备命令
					os_thread_sleep(os_msec_to_ticks(100)); 
					///////////////////////保存从设备信息/////////////////////////////////////////
					if(device_data.slave_device_have_ack_flag==1)
					{
						device_data.slave_device_have_ack_flag=0;
						//log_i("slave_device_max=%d",device_data.slave_device_max);
						//log_i("slave_addr=%d",slave_device_tmp.slave_addr);
						sys_psm.slave_device[device_data.slave_device_max]=slave_device_tmp;
						read_write_slave_information_flash(WRITE_PSM,device_data.slave_device_max);
						device_data.slave_device_max++;
						os_thread_sleep(os_msec_to_ticks(50)); 
						////////////////////////////////////////////////////////////////////////////
						device_data.save_slave_device_flag=1;
					}
					//////////////////////////////////////////////////////////////////////
					device_data.query_slave_cnt++;
					if(device_data.query_slave_cnt>=16)
					{
						device_data.query_slave_cnt=0;
						if(device_data.save_slave_device_flag==1)
						{
							device_data.save_slave_device_flag=0;
							////////////////////////////////////////////////////////////////////////////
							for (i = device_data.slave_device_max; i < SLAVE_MAX_NUM; i++)
							{
								read_write_slave_information_flash(WRITE_PSM_DEFULT,i);
							}	
							////////////////////////////////////////////////////////////////////////////
							sys_psm.slave_ok=1;
							read_write_slave_ok_flash(WRITE_PSM);
							os_thread_sleep(os_msec_to_ticks(100)); 
							//////////////////更新从设备列表//////////////////////////////////////////////
							device_data.slave_device_max=0;
							for (i = 0; i < SLAVE_MAX_NUM; i++)
							{
								read_write_slave_information_flash(READ_PSM,i);
								if(DAT_SLAVE_TYPE!=sys_psm.slave_device[i].device_type)
								{
									device_data.own_slave_device[device_data.slave_device_max]=sys_psm.slave_device[i];
									device_data.slave_device_max++;
								}
							}	
							////////////////////////////////////////////////////////////////////////////							
						}	
						else
						{
							device_data.query_slave_times_cnt++;
							if(device_data.query_slave_times_cnt>3)
							{
								device_data.query_slave_finish_flag=1;
							}						
						}
					}
				}			
			}
		}
		
		if(rev_timerout_flag==1)
		{
			rev_timerout_cnt++;
			if(rev_timerout_cnt>=10)
			{
				rev_timerout_flag=0;
				rev_timerout_cnt=0;
				buf_rev_cnt=0;
			}
		}
		os_thread_sleep(os_msec_to_ticks(100)); 
	}
	os_thread_delete(NULL);
}


void modbus_process_init(void)
{
	int ret=SYS_FAIL;
	ret = os_thread_create(&modbus_thread, //任务控制块指针
							"modbus_thread",//任务名字
							modbus_control_thread, //任务入口函数
							NULL,//任务入口函数参数
							&modbus_stack,//任务栈大小
							OS_PRIO_3);	//任务的优先级							
   if(ret==SYS_FAIL)
   {
		log_e("modbus","modbus thread create error!");
   }
}

uint16_t read_power_board_relay_status(uint8_t num)
{
	return power_board_data[num].relay_status;
}

float read_power_board_ac_voltage_value(uint8_t num)
{
	return power_board_data[num].ac_voltage_value*0.01;
}

float read_power_board_ac_current_value(uint8_t num)
{
	return power_board_data[num].ac_current_value*0.01;
}

float read_power_board_dc1_voltage_value(uint8_t num)
{
	return power_board_data[num].dc1_voltage_value*0.01;
}

float read_power_board_dc1_current_value(uint8_t num)
{
	return power_board_data[num].dc1_current_value*0.01;
}

float read_power_board_dc2_voltage_value(uint8_t num)
{
	return power_board_data[num].dc1_voltage_value*0.01;
}

float read_power_board_dc2_current_value(uint8_t num)
{
	return power_board_data[num].dc1_current_value*0.01;
}

float read_power_board_dc3_voltage_value(uint8_t num)
{
	return power_board_data[num].dc1_voltage_value*0.01;
}

float read_power_board_dc3_current_value(uint8_t num)
{
	return power_board_data[num].dc1_current_value*0.01;
}

float read_power_board_dc4_voltage_value(uint8_t num)
{
	return power_board_data[num].dc1_voltage_value*0.01;
}

float read_power_board_dc4_current_value(uint8_t num)
{
	return power_board_data[num].dc1_current_value*0.01;
}

float read_power_board_dc5_voltage_value(uint8_t num)
{
	return power_board_data[num].dc1_voltage_value*0.01;
}

float read_power_board_dc5_current_value(uint8_t num)
{
	return power_board_data[num].dc1_current_value*0.01;
}

float read_power_board_dc6_voltage_value(uint8_t num)
{
	return power_board_data[num].dc1_voltage_value*0.01;
}

float read_power_board_dc6_current_value(uint8_t num)
{
	return power_board_data[num].dc1_current_value*0.01;
}

float read_power_board_dc7_voltage_value(uint8_t num)
{
	return power_board_data[num].dc1_voltage_value*0.01;
}

float read_power_board_dc7_current_value(uint8_t num)
{
	return power_board_data[num].dc1_current_value*0.01;
}
