#include "key_handle.h"
#include "sys_os.h"
#include "sys_log.h"

#include "project_pin_use_config.h"
#include "key_dir.h"
#include "sensor_in_dir.h"
#include "buzzer_dir.h"
#include "monitor_board_handle.h"

#define KEY_TAG "key"

static os_thread_t key_thread= NULL;
static os_thread_stack_define(key_stack, 1*1024);

uint8_t smoke_value=1;
uint8_t smoke_value_old=0xff;
uint8_t smoke_value_cnt=0;

uint8_t water_value=1;
uint8_t water_value_old=0xff;
uint8_t water_value_cnt=0;

uint8_t lock_value=1;
uint8_t lock_value_old=0xff;
uint8_t lock_value_cnt=0;

uint8_t spd_value=1;
uint8_t spd_value_old=0xff;
uint8_t spd_value_cnt=0;


void key_control_thread(void* param)
{
	uint8_t ucKeyCode;		// 按键代码
	uint16_t  tmp;
	uint16_t led_1min_cnt=0;
	
	LL_PERIPH_WE(PROJECT_PERIPH_WE);
	buzzer_io_init();
	key_init();		
	sensor_in_io_init();
	LL_PERIPH_WP(PROJECT_PERIPH_WP);/* Register write protected for some required peripherals. */
	
	while(1)
	{
		os_thread_sleep(os_msec_to_ticks(20)); 
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		key_detect_all();//实现所有按键的检测。按键检测每隔10ms一次就行了，一般做40ms的滤波处理就可以有效过滤掉机械动作造成的按键抖动。
		ucKeyCode = get_key_value();	// 读取键值, 无键按下时返回 KEY_NONE = 0
		if (ucKeyCode != KEY_NONE)
		{
			//对于按键弹起事件
			switch (ucKeyCode)
			{
				case KEY_DOWN_MENU:			// MENU键按下
					log_i("MENU DOWN");
					break;

				case KEY_UP_MENU:			// MENU键弹起
					log_i("MENU UP");
					break;

				case KEY_DOWN_UP:			// UP键按下
					log_i("UP DOWN");
					if(device_data.display_num==0)
					{
						device_data.display_num=4;
					}
					else if(device_data.display_num==1)
					{
						device_data.display_num=0;
					}
					else if(device_data.display_num==2)
					{
						device_data.display_num=1;
					}
					else if(device_data.display_num==3)
					{
						device_data.display_num=2;
					}
					else if(device_data.display_num==4)
					{
						device_data.display_num=3;
					}		
					log_i("display_num=%d",device_data.display_num);	
					break;

				case KEY_UP_UP:				// UP键弹起
					log_i("UP UP");
					break;
				
				case KEY_DOWN_DOWN:			// DOWN键按下
					log_i("DOWN DOWN");
					if(device_data.display_num==0)
					{
						device_data.display_num=1;
					}
					else if(device_data.display_num==1)
					{
						device_data.display_num=2;
					}
					else if(device_data.display_num==2)
					{
						device_data.display_num=3;
					}
					else if(device_data.display_num==3)
					{
						device_data.display_num=4;
					}
					else if(device_data.display_num==4)
					{
						device_data.display_num=0;
					}	
					log_i("display_num=%d",device_data.display_num);					
					break;

				case KEY_UP_DOWN:			// DOWN键弹起
					log_i("DOWN UP");
					break;
				
				case KEY_DOWN_CLR:			// CLR键按下
					log_i("CLR DOWN");
					if(device_data.error_status!=0)
					{
						if(device_data.buzzer_enable_flag==1)
						{
							device_data.buzzer_enable_flag=0;
						}
						else
						{
							device_data.buzzer_enable_flag=1;
						}
						device_data.buzzer_change_flag=1;
					}
					break;	
				
				case KEY_UP_CLR:			// CLR键弹起
					log_i("CLR UP");
					break;

				case KEY_DOWN_OK:			// OK键按下
					log_i("OK DOWN");
					break;
				case KEY_UP_OK:				// OK键弹起
					log_i("OK UP");
					break;
				case KEY_DOWN_LONG_OK:
					log_i("OK LONG");
					device_data.set_scan_flag=1;
					device_data.slave_device_have_ack_flag=0;
					device_data.query_slave_cnt=0;
					device_data.query_slave_times_cnt=0;
					device_data.save_slave_device_flag=0;	
					device_data.query_slave_finish_flag=0;
					break;
				case KEY_DOWN_RST:			// OK键按下
					log_i("RST DOWN");
					break;
				case KEY_UP_RST:				// OK键弹起
					log_i("RST UP");
					if(device_data.rst_long_flag==0)
					{
						device_data.set_scan_flag=1;
						device_data.slave_device_have_ack_flag=0;
						device_data.query_slave_cnt=0;
						device_data.query_slave_times_cnt=0;
						device_data.save_slave_device_flag=0;	
						device_data.query_slave_finish_flag=0;						
					}
					device_data.rst_long_flag=0;
					device_data.factory_flag=1;
					break;
				case KEY_DOWN_LONG_RST:
					log_i("RST LONG");
					device_data.rst_long_flag=1;
					break;
				default:
					log_i("no keycode");
					break;
			}
		}
		///////////////////////////////////////////////////////////////////////////////////////
		smoke_value=read_smoke_sensor();
		if(smoke_value_old!=smoke_value)
		{
			smoke_value_cnt++;
			if(smoke_value_cnt>=3)
			{
				smoke_value_cnt=0;
				smoke_value_old=smoke_value;
				if(smoke_value_old==1)
				{
					device_data.error_status=device_data.error_status|SMOKE_SENSOR_ERROR;
					device_data.smoke_in_error_flag=1;
					log_i("SMOKE_SENSOR_ERROR");
					
				}
				else
				{
					device_data.error_status&=~SMOKE_SENSOR_ERROR;
					device_data.smoke_in_error_flag=0;
					log_i("NO SMOKE_SENSOR_ERROR");
				}
			}
		}
		else
		{
			smoke_value_cnt=0;
		}
		///////////////////////////////////////////////////////////////////////////////////////
		water_value=read_water_sensor();
		if(water_value_old!=water_value)
		{
			water_value_cnt++;
			if(water_value_cnt>=3)
			{
				water_value_cnt=0;
				water_value_old=water_value;
				if(water_value_old==1)
				{
					device_data.error_status=device_data.error_status|WATER_SENSOR_ERROR;
					device_data.water_in_error_flag=1;
					log_i("WATER_SENSOR_ERROR");
				}
				else
				{
					device_data.error_status&=~WATER_SENSOR_ERROR;
					device_data.water_in_error_flag=0;
					log_i("NO WATER_SENSOR_ERROR");
				}
			}
		}
		else
		{
			water_value_cnt=0;
		}		
		///////////////////////////////////////////////////////////////////////////////////////
		tmp=GPIO_ReadInputPort(DOOR_IN_PORT);
		lock_value=read_lock_sensor();
		if(lock_value_old!=lock_value)
		{
			lock_value_cnt++;
			if(lock_value_cnt>=3)
			{
				lock_value_cnt=0;
				lock_value_old=lock_value;
				if(lock_value_old==1)
				{
					device_data.error_status=device_data.error_status|LOCK_SENSOR_ERROR;
					device_data.lock_in_error_flag=1;
					device_data.power_monitoring_relay_data[0]=device_data.power_monitoring_relay_data[0]|LIGHTING_RELAY;
					//log_i("relay_on_off_data=0x%04x",device_data.relay_on_off_data);
					log_i("LOCK_SENSOR_ERROR");
				}
				else
				{
					device_data.error_status&=~LOCK_SENSOR_ERROR;
					device_data.lock_in_error_flag=0;
					device_data.power_monitoring_relay_data[0]&=~LIGHTING_RELAY;
					//log_i("relay_on_off_data=0x%04x",device_data.relay_on_off_data);
					log_i("NO LOCK_SENSOR_ERROR");
				}
			}
		}
		else
		{
			lock_value_cnt=0;
		}
		if(device_data.led_flag==1)
		{
			if(device_data.led_data_flag==1)
			{
				led_1min_cnt=0;
				device_data.power_monitoring_relay_data[0]=device_data.power_monitoring_relay_data[0]|LIGHTING_RELAY;
				device_data.led_data_flag=2;
			}
			else if(device_data.led_data_flag==0)
			{
				device_data.power_monitoring_relay_data[0]&=~LIGHTING_RELAY;
				device_data.led_flag=0;
			}
			else
			{
				led_1min_cnt++;
				if(led_1min_cnt>=50*60)
				{
					device_data.led_data_flag=0;
				}			
			}
		}
		///////////////////////////////////////////////////////////////////////////////////////
		spd_value=read_spd_sensor();
		if(spd_value_old!=spd_value)
		{
			spd_value_cnt++;
			if(spd_value_cnt>=3)
			{
				spd_value_cnt=0;
				spd_value_old=spd_value;
				if(spd_value_old==1)
				{
					device_data.error_status=device_data.error_status|SPD_SENSOR_ERROR;
					device_data.spd_in_error_flag=1;
					log_i("SPD_SENSOR_ERROR");
				}
				else
				{
					device_data.error_status&=~SPD_SENSOR_ERROR;
					device_data.spd_in_error_flag=0;
					log_i("NO SPD_SENSOR_ERROR");
				}
			}
		}
		else
		{
			spd_value_cnt=0;
		}
		/////////////////////////////////////////////////////////////////////////////////////////
		if(device_data.error_status_old!=device_data.error_status)
		{
			device_data.error_status_old=device_data.error_status;	
			device_data.buzzer_change_flag=1;
			device_data.alarm_change_flag=1;
			//log_i("error_status=0x%08x",device_data.error_status_old);		
		}
		if(device_data.buzzer_change_flag==1)
		{
			device_data.buzzer_change_flag=0;
			if(device_data.error_status_old==0)
			{
				buzzer_off();
				//log_i("buzzer_off 1");
			}
			else
			{
				if(device_data.buzzer_enable_flag==0)
				{
					buzzer_on();
					//log_i("buzzer_on 1");
				}
				else
				{
					buzzer_off();
					//log_i("buzzer_off 2");
				}				
			}				
		}
	}
	os_thread_delete(NULL);
}


void key_process_init(void)
{
	int ret=SYS_FAIL;
	ret = os_thread_create(&key_thread, //任务控制块指针
							"keyscan_thread",//任务名字
							key_control_thread, //任务入口函数
							NULL,//任务入口函数参数
							&key_stack,//任务栈大小
							OS_PRIO_2);	//任务的优先级							
   if(ret==SYS_FAIL)
   {
		log_e(KEY_TAG,"key thread create error!");
   }
}
