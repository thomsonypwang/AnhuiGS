#include "eth_lock_to_tcp.h"
#include "lock_rs485_handle.h"
#include "eth_handle.h"
#include "project_psm_control.h"
#include "w5500_dir.h"
#include "w5500_socket.h"
#include "w5500_dns.h"
#include "sys_log.h"
#include "sys_os.h"

//#define 	LOCK_DATA_BUF_SIZE   128

//uint8_t lock_rev_buf[LOCK_DATA_BUF_SIZE];
//uint8_t lock_send_buf[LOCK_DATA_BUF_SIZE];
//uint8_t lock_rev_len;
//uint8_t lock_tcp_flag=0;

//static os_thread_t lock_tcpc_thread= NULL;
//static os_thread_stack_define(lock_tcpc_stack, 2*1024);

//int32_t lock_tcpc(uint8_t sn,uint8_t* server_ip, uint16_t server_port)
//{
//	uint16_t len=0;
//	int32_t ret;
//	
//	switch(getSn_SR(sn))														
//	{
//		case SOCK_INIT:															
//			connect(sn,server_ip,server_port);
//		break;
//		case SOCK_ESTABLISHED:											
//            /* socket has been established */
//            if(getSn_IR(sn) & Sn_IR_CON) 
//			{
//                setSn_IR(sn,Sn_IR_CON);
//            }
//            if(get_lock_send_tcp_status()==1)
//            {
//            	ret = send(sn, get_lock_tcp_send_data(), get_lock_tcp_send_len());
//            	if (ret != get_lock_tcp_send_len())
//            	{
//            		log_i("%d:send fail\r\n", sn);
//            		close(sn);
//            		return ret;
//            	}
//            	set_lock_send_tcp_status(2);
//            }
//            else if(get_lock_send_tcp_status()==2)
//            {
//                if ((lock_rev_len = getSn_RX_RSR(sn)) > 0)
//    			{
//                    if (lock_rev_len > LOCK_DATA_BUF_SIZE)
//    				{
//                        lock_rev_len = LOCK_DATA_BUF_SIZE;
//                    }
//                    //recv data
//                    ret = recv(sn, lock_rev_buf, lock_rev_len);
//                    if (ret <= 0)
//    				{
//                        //log_i("%d:recv fail", sn);
//                        return ret;
//                    }
//    				else
//    				{
//                        // The actual received size
//    					log_i("%d:recv size:%d", sn, lock_rev_len);
//    					log_i("%d:recv data:[%s]", sn, lock_rev_buf);
//    					set_lock_tcp_rev_data(lock_rev_buf, lock_rev_len);
//    					set_lock_tcp_rev_status(1);
//                    }
//    			}
//            }
//            else
//            {
//                if ((lock_rev_len = getSn_RX_RSR(sn)) > 0)
//    			{
//                    if (lock_rev_len > LOCK_DATA_BUF_SIZE)
//    				{
//                        lock_rev_len = LOCK_DATA_BUF_SIZE;
//                    }

//                    //recv data
//                    ret = recv(sn, lock_rev_buf, lock_rev_len);
//                    if (ret <= 0)
//    				{
//                        //log_i("%d:recv fail", sn);
//                        return ret;
//                    }
//    				else
//    				{
//                        // The actual received size
////    					log_i("recv size:%d", lock_rev_len);
////    					log_i("recv data:%s",lock_rev_buf);						
//    					if(strncmp((char*)lock_rev_buf,"{,s,s,t,s,p,q,l,w,l,202002#ID_}",31)==0)
//    					{
//    						len=sizeof(sys_psm.device_id);
//							log_i("send size:%d", len);
//							log_i("send data:%s",sys_psm.device_id);
//    		            	ret = send(sn, (uint8_t*)sys_psm.device_id, len);
//    		            	if (ret != len)
//    		            	{
//    		            		log_i("%d:send fail\r\n", sn);
//    		            		close(sn);
//    		            		return ret;
//    		            	}
//    					}
//                    }
//    			}
//            }
//		break;
//		case SOCK_CLOSE_WAIT:												  
//			disconnect(sn);	
//		break;
//		case SOCK_CLOSED:														
//				socket(sn,Sn_MR_TCP,server_port,0x00);		
//		break;
//	}
//	return 1;
//}

//void lock_tcpc_control_thread(void* param)
//{
//	int32_t ret;
//	
//    while (1)
//    {
//		ret = lock_tcpc(LOCK_SOCKET,sys_psm.server1_ip,sys_psm.server1_port);
//		if(ret<0)
//		{
//			close(LOCK_SOCKET);	
//			os_thread_sleep(os_msec_to_ticks(1000));
//		}
//        os_thread_sleep(os_msec_to_ticks(100));
//    }
//}

//void lock_handle(void)
//{

//	int ret=SYS_FAIL;
//	if(device_data.lock_flag==0)
//	{
//		device_data.lock_flag=1;
//		ret = os_thread_create(&lock_tcpc_thread, //任务控制块指针
//								"lock_tcpc_thread",//任务名字
//								lock_tcpc_control_thread, //任务入口函数
//								NULL,//任务入口函数参数
//								&lock_tcpc_stack,//任务栈大小
//								OS_PRIO_9);	//任务的优先级							
//	   if(ret==SYS_FAIL)
//	   {
//			log_e("mqtt","mqtt thread create error!");
//		   device_data.lock_flag=0;
//	   }	
//	}
//}

