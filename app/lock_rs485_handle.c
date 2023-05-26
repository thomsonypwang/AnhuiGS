#include "lock_rs485_handle.h"
#include "lock_rs485_dir.h"
#include "project_pin_use_config.h"
#include <stdarg.h>
#include <string.h>

#include "sys_os.h"
#include "sys_log.h"
#include "sys_errno.h"

#include "w5500_dir.h"

static os_thread_t lock_thread= NULL;
static os_thread_stack_define(lock_stack, 2*1024);

uint8_t lock_rx_buf[128];
uint8_t lock_tx_buf[128];
uint8_t lock_timeout_cnt;
uint8_t lock_rx_frame_flag=0;
uint8_t lock_rx_buf_len=0;

uint8_t lock_send_tcp_flag;//门锁收到信息TCP可发送标志
uint8_t lock_rev_tcp_flag;//TCP收到信息门所发送标志

uint8_t lock_send_tcp_buf[128];
uint8_t lock_send_tcp_len=0;
uint8_t lock_rev_tcp_buf[128];
uint8_t lock_rev_tcp_len=0;
uint8_t lock_send_rev_flag=0;
	
	
static void USART2_RxFull_IrqCallback(void)
{
    uint8_t u8Data = (uint8_t)USART_ReadData(LOCK_USART_ID);
	if(device_data.phy_check_flag == PHY_LINK_ON)
	{
		lock_rx_buf[lock_rx_buf_len] = u8Data;
		lock_rx_buf_len++;	
		if((lock_rx_buf_len )>128)
		{
			lock_rx_buf_len=0;
		}
	}
}

static void USART2_RxError_IrqCallback(void)
{
	(void)USART_ReadData(LOCK_USART_ID);
    USART_ClearStatus(LOCK_USART_ID, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

static void USART2_RxTimeout_IrqCallback(void)
{
	if(device_data.phy_check_flag == PHY_LINK_ON)
	{
		lock_rx_frame_flag=1;
		lock_timeout_cnt=0;
	}
	
    TMR0_Stop(LOCK_USART_TMR0, LOCK_USART_TMR0_CH);
    USART_ClearStatus(LOCK_USART_ID, USART_FLAG_RX_TIMEOUT);
}

void lock_control_thread(void* param)
{
	uint8_t i;
	
	lock_usart_init(USART2_RxError_IrqCallback,USART2_RxFull_IrqCallback,USART2_RxTimeout_IrqCallback);
	
	while(1)
	{
		if(device_data.phy_check_flag == PHY_LINK_ON)
		{
			///////////门锁收到信息存到缓存区////////////////////
			if(lock_rx_frame_flag==1)
			{
				if(lock_rx_buf_len!=0)
				{
					lock_send_tcp_len= lock_rx_buf_len;
					
					for (i = 0; i < lock_send_tcp_len; i++)
					{
						lock_send_tcp_buf[i]=lock_rx_buf[i];
						lock_rx_buf[i]=0x00;
					}
					log_i("lock rx data:%s",lock_send_tcp_buf);
					lock_send_tcp_flag=1;
					lock_send_rev_flag=1;
				}
				lock_rx_buf_len=0;
				lock_rx_frame_flag=0;
			}
			///////////TCP收到信息发回门锁////////////////////
			if((lock_rev_tcp_flag==1)&&(lock_send_rev_flag==1))
			{
				send_lock_data(lock_rev_tcp_buf,get_lock_tcp_rev_len());
				lock_timeout_cnt=0;
				lock_send_tcp_flag=0;
				lock_rev_tcp_flag=0;
				lock_timeout_cnt=120;
				lock_send_rev_flag=0;
			}
			//////////////////////////////////////
		}
		/////////////门锁收到信息计时超时时间///////////////////////////
		if(lock_send_tcp_flag==1)
		{
			lock_timeout_cnt++;
			if(lock_timeout_cnt>=2*60)
			{
				lock_timeout_cnt=0;
				lock_send_tcp_flag=0;
				lock_send_rev_flag=0;
				log_i("lock send tcp timeout");
			}
		}
		//////////////////////////////////////////////
		os_thread_sleep(os_msec_to_ticks(500)); 
	}
	os_thread_delete(NULL);
}


void lock_process_init(void)
{
	int ret=SYS_FAIL;
	
	ret = os_thread_create(&lock_thread, //任务控制块指针
							"lock_thread",//任务名字
							lock_control_thread, //任务入口函数
							NULL,//任务入口函数参数
							&lock_stack,//任务栈大小
							OS_PRIO_4);	//任务的优先级							
   if(ret==SYS_FAIL)
   {
		log_e("lock","lock thread create error!");
   }
}

uint8_t get_lock_send_tcp_status(void)
{
	return lock_send_tcp_flag;
}

void set_lock_send_tcp_status(uint8_t flag)
{
	lock_send_tcp_flag=flag;
}

void set_lock_tcp_rev_status(uint8_t flag)
{
	lock_rev_tcp_flag=flag;
}

uint8_t get_lock_tcp_send_len(void)
{
	return lock_send_tcp_len;
}

uint8_t *get_lock_tcp_send_data(void)
{
	return lock_send_tcp_buf;
}

void set_lock_tcp_send_data(uint8_t *buf,uint8_t len)
{
	lock_send_tcp_len=len;
	memcpy(lock_send_tcp_buf,buf,len);
}

uint8_t get_lock_tcp_rev_len(void)
{
	return lock_rev_tcp_len;
}

uint8_t *get_lock_tcp_rev_data(void)
{
	return lock_rev_tcp_buf;
}

void set_lock_tcp_rev_data(uint8_t *buf,uint8_t len)
{
	lock_rev_tcp_len=len;
	memcpy(lock_rev_tcp_buf,buf,len);
}
