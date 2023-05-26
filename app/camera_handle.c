#include "camera_handle.h"
#include "camera_com_pro.h"
#include "camera_dir.h"
//#include "eth_lock_to_tcp.h"
#include "lock_rs485_handle.h"

#include <stdarg.h>
#include <string.h>

#include "sys_os.h"
#include "sys_log.h"
#include "sys_errno.h"

#include "eth_handle.h"
#include "project_psm_control.h"
#include "w5500_dir.h"
#include "w5500_socket.h"
#include "w5500_dns.h"


static os_thread_t camera_thread= NULL;
static os_thread_stack_define(camera_stack, 5*1024);

static os_thread_t camera_tcpc_thread= NULL;
static os_thread_stack_define(camera_tcpc_stack, 7*1024);

uint8_t lock_open_close=0xff;
uint8_t send_num_flag=0;
uint8_t camera_tcpc_flag=0;
char camera_tcpc_buf[256];

#define 	LOCK_DATA_BUF_SIZE   128

uint8_t lock_rev_buf[LOCK_DATA_BUF_SIZE];
uint8_t lock_send_buf[LOCK_DATA_BUF_SIZE];
uint8_t lock_rev_len;

int32_t camera_tcpc(uint8_t sn,uint8_t* server_ip, uint16_t server_port)
{
	uint16_t len=0;
	int32_t ret;
	char temp_buf[64];
	
	switch(getSn_SR(sn))														
	{
		case SOCK_INIT:															
			connect(sn,server_ip,server_port);
		break;
		case SOCK_ESTABLISHED:											
            /* socket has been established */
            if(getSn_IR(sn) & Sn_IR_CON) 
			{
                setSn_IR(sn,Sn_IR_CON);
            }
			if(device_data.camera_send_flag==1)
			{
				if(device_data.camera_error_flag==0)
				{
					if(device_data.last_flag==1)
					{
	//					sprintf(camera_tcpc_buf,"client:%s,picsize:%d,all_frame:%d,frame:%d,Bytes:%d,data:",sys_psm.device_id,\
	//					device_data.camera_picLen,device_data.cntM,device_data.camera_buf_cnt,device_data.lastBytes); 

	//					len=strlen(camera_tcpc_buf);
	//					log_i("send %s ",camera_tcpc_buf);
	//					ret = send(sn,(uint8_t*)camera_tcpc_buf, strlen(camera_tcpc_buf));
	//					if (ret != len)
	//					{
	//						log_i("camera send fail 1 ret=%d",ret);
	//						device_data.camera_send_flag=0;
	//						close(sn);
	//						return ret;
	//					}
						len=device_data.lastBytes;
						ret = send(sn,(uint8_t*)device_data.camera_buf, device_data.lastBytes);
						if (ret != len)
						{
							log_i("camera send fail 2 ret=%d",ret);
							device_data.camera_send_flag=0;
							device_data.camera_send_to_tcp_flag=0;
							device_data.last_flag=0;
							close(sn);
							return ret;
						}	
						device_data.camera_send_to_tcp_flag=0;
						device_data.camera_send_flag=0;
						device_data.last_flag=0;
					}
					else
					{
						if(send_num_flag==1)
						{
							send_num_flag=0;
							device_data.camera_send_to_tcp_flag=1;
	//						sprintf(camera_tcpc_buf,"client:%s,picsize:%d,all_frame:%d,frame:%d,Bytes:%d,data:",sys_psm.device_id,\
	//						device_data.camera_picLen,device_data.cntM,device_data.camera_buf_cnt,N_BYTE); 
							sprintf(camera_tcpc_buf,"task:upload,client:%s,picsize:%d,all_frame:%d,data:",sys_psm.device_id,device_data.camera_picLen,device_data.cntM); 
							len=strlen(camera_tcpc_buf);
							log_i("send %s ",camera_tcpc_buf);
							ret = send(sn,(uint8_t*)camera_tcpc_buf, strlen(camera_tcpc_buf));
							if (ret != len)
							{
								log_i("camera send fail 3 ret=%d",ret);
								device_data.camera_send_flag=0;
								device_data.camera_send_to_tcp_flag=0;
								device_data.last_flag=0;
								close(sn);
								return ret;
							}							
						}

						len=N_BYTE;
						ret = send(sn,(uint8_t*)device_data.camera_buf, N_BYTE);
						if (ret != len)
						{
							log_i("camera send fail 4 ret=%d",ret);
							device_data.camera_send_flag=0;
							device_data.camera_send_to_tcp_flag=0;
							device_data.last_flag=0;
							close(sn);
							return ret;
						}
						device_data.camera_send_flag=0;
					}				
				}
			}
			else
			{
				//蓝牙门锁
				//////////////////////////////////////////////////////////////////////////
			    if(get_lock_send_tcp_status()==1)
				{
					log_i("%d:send size:%d", sn, get_lock_tcp_send_len());
					log_i("%d:send data:%s", sn, get_lock_tcp_send_data());
					ret = send(sn, get_lock_tcp_send_data(), get_lock_tcp_send_len());
					if (ret != get_lock_tcp_send_len())
					{
						log_i("%d:send fail\r\n", sn);
						close(sn);
						return ret;
					}
					set_lock_send_tcp_status(2);
				}
				else if(get_lock_send_tcp_status()==2)
				{
					if ((lock_rev_len = getSn_RX_RSR(sn)) > 0)
					{
						if (lock_rev_len > LOCK_DATA_BUF_SIZE)
						{
							lock_rev_len = LOCK_DATA_BUF_SIZE;
						}
						//recv data
						ret = recv(sn, lock_rev_buf, lock_rev_len);
						if (ret <= 0)
						{
							log_i("%d:recv fail", sn);
							return ret;
						}
						else
						{
							// The actual received size
							log_i("%d:recv size:%d", sn, lock_rev_len);
							log_i("%d:recv data:%s", sn, lock_rev_buf);
							set_lock_tcp_rev_data(lock_rev_buf, lock_rev_len);
							set_lock_tcp_rev_status(1);
						}
					}
				}
				else
				{
					if ((lock_rev_len = getSn_RX_RSR(sn)) > 0)
					{
						if (lock_rev_len > LOCK_DATA_BUF_SIZE)
						{
							lock_rev_len = LOCK_DATA_BUF_SIZE;
						}

						//recv data
						ret = recv(sn, lock_rev_buf, lock_rev_len);
						if (ret <= 0)
						{
							//log_i("%d:recv fail", sn);
							return ret;
						}
						else
						{
							// The actual received size
//	    					log_i("recv size:%d", lock_rev_len);
//	    					log_i("recv data:%s",lock_rev_buf);						
							if(strncmp((char*)lock_rev_buf,"{,s,s,t,s,p,q,l,w,l,202002#ID_}",31)==0)
							{

								sprintf(temp_buf,"{,s,s,t,s,p,q,l,w,l,202002#ID_%s}",sys_psm.device_id); 
								len=strlen(temp_buf);
//								log_i("send size:%d", len);
								log_i("send data:%s",temp_buf);
								ret = send(sn, (uint8_t*)temp_buf, len);
								if (ret != len)
								{
									log_i("%d:send fail\r\n", sn);
									close(sn);
									return ret;
								}
							}
						}
					}
				}
				//////////////////////////////////////////////////////////////////////////
			}
		break;
		case SOCK_CLOSE_WAIT:												  
			disconnect(sn);	
		break;
		case SOCK_CLOSED:														
				socket(sn,Sn_MR_TCP,server_port,0x00);		
		break;
	}
	return 1;
}

void camera_control_thread(void* param)
{
	int ret=0;
	int camera_err_cnt=0;
	
	camera_rs232_init();
	os_thread_sleep(os_msec_to_ticks(5000)); 
	ret=query_version();
	while(1)
	{
		os_thread_sleep(os_msec_to_ticks(100)); 
		if(ret==SYS_OK)
		{
			if(lock_open_close!=device_data.lock_in_error_flag)
			{
				lock_open_close=device_data.lock_in_error_flag;
				os_thread_sleep(os_msec_to_ticks(1000)); 
				if(device_data.camera_send_to_tcp_flag==0)
				{
					if(lock_open_close==1)
					{
						device_data.camera_flag=1;
						send_num_flag=1;
						ret=camera_app(SERIAL_NUM_0,IMAGE_SIZE_640X480);
						if(ret!=SYS_OK)
						{
							camera_err_cnt++;
							send_reset(SERIAL_NUM_0);
							os_thread_sleep(os_msec_to_ticks(3000)); 
							ret=query_version();
						}
						else
						{
							camera_err_cnt=0;
							device_data.error_status&=~CAMERA_SENSOR_ERROR;
							device_data.camera_error_flag=0;
						}
						send_num_flag=0;
						device_data.camera_flag=0;					
					}				
				}
			}		
		}
		else
		{
			camera_err_cnt++;
			ret=query_version();
			os_thread_sleep(os_msec_to_ticks(1000)); 
			if(ret==SYS_OK)
			{
				camera_err_cnt=0;
				device_data.error_status&=~CAMERA_SENSOR_ERROR;			
			}
		}	
		if(camera_err_cnt>=5)
		{
			device_data.error_status=device_data.error_status|CAMERA_SENSOR_ERROR;
			device_data.camera_error_flag=1;
		}
	}
	os_thread_delete(NULL);
}

void camera_tcpc_control_thread(void* param)
{
	int32_t ret;
	
    while (1)
    {
		if(device_data.phy_check_flag == PHY_LINK_ON)
		{
			ret = camera_tcpc(CAMERA_SOCKET,sys_psm.server1_ip,sys_psm.server1_port);
			if(ret<0)
			{
				close(CAMERA_SOCKET);	
				os_thread_sleep(os_msec_to_ticks(1000));
			}			
		}
        os_thread_sleep(os_msec_to_ticks(30));
    }
}

void camera_process_init(void)
{
	int ret=SYS_FAIL;
	ret = os_thread_create(&camera_thread, //任务控制块指针
							"camera_thread",//任务名字
							camera_control_thread, //任务入口函数
							NULL,//任务入口函数参数
							&camera_stack,//任务栈大小
							OS_PRIO_10);	//任务的优先级							
   if(ret==SYS_FAIL)
   {
		log_e("camera","camera thread create error!");
   }
}
void camera_tcpc_init(void)
{
	int ret=SYS_FAIL;
	if(camera_tcpc_flag==0)
	{
		camera_tcpc_flag=1;
		ret = os_thread_create(&camera_tcpc_thread, //任务控制块指针
								"camera_tcpc_thread",//任务名字
								camera_tcpc_control_thread, //任务入口函数
								NULL,//任务入口函数参数
								&camera_tcpc_stack,//任务栈大小
								OS_PRIO_9);	//任务的优先级							
	   if(ret==SYS_FAIL)
	   {
			log_e("camera","camera tcpc thread create error!");
	   }	
	}
}

