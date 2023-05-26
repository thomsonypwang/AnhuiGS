#include "oled_handle.h"
#include "sys_os.h"
#include "sys_log.h"
#include "project_pin_use_config.h"
#include "led_dir.h"
#include "oled_dir.h"
#include "am2301a_dir.h"
#include "project_psm_control.h"
#include "monitor_board_handle.h"

#define OLED_TAG "oled"

static os_thread_t display_oled_thread= NULL;
static os_thread_stack_define(oled_stack, 3*512);

float temp_value_old;
float humi_value_old;


uint8_t smoke_in_error_flag_old;		
uint8_t water_in_error_flag_old;
uint8_t lock_in_error_flag_old;
uint8_t spd_in_error_flag_old;
uint8_t display_num_old;
uint8_t display_brush_flag=0;
uint8_t display_brush_reset_flag=0;

uint8_t open_fan=0;
uint8_t open_heat=0;

uint8_t rs485_id_old;
char device_id_old[32+1];

float electric_value_old;	
float voltage_value_old;

void oled_control_thread(void* param)
{		
	uint16_t timerout_cnt=0;

	char string_buf[32+1];
	uint8_t led_500ms_cnt=0;
	uint8_t read_500ms_cnt=0;
	uint8_t err_flag=0;
	
	LL_PERIPH_WE(PROJECT_PERIPH_WE);
	led_io_init();
	oled_init();
	am2301a_init();
	LL_PERIPH_WP(PROJECT_PERIPH_WP);
	oled_paint_clear();

	display_brush_flag=1;
	display_brush_reset_flag=1;
	device_data.display_num=0;
	read_500ms_cnt=20;
	os_thread_sleep(os_msec_to_ticks(1000)); 
	while(1)
	{
		//////////////////////////////////////////////////////////
		read_500ms_cnt++;
		if(read_500ms_cnt>=10)
		{
			read_500ms_cnt=0;	
			if(device_data.camera_flag==0)
			{
				taskENTER_CRITICAL();			
				err_flag=am2301a_read_data(&device_data.temp_value,&device_data.humi_value);
				taskEXIT_CRITICAL();			
			}
			if(err_flag==0)
			{
				if(open_fan==0)
				{
					if(device_data.temp_value>=sys_psm.high_temp_data)
					{
						device_data.power_monitoring_relay_data[0]=device_data.power_monitoring_relay_data[0]|FAN_RELAY;
						open_fan=1;
					}				
				}
				else
				{
					if(device_data.temp_value<=sys_psm.high_temp_data-5)
					{
						device_data.power_monitoring_relay_data[0]&=~FAN_RELAY;
						open_fan=0;
					}				
				}
				if(open_heat==0)
				{
					if((device_data.humi_value>=sys_psm.high_humi_data)||(device_data.temp_value<=sys_psm.low_temp_data))
					{
						device_data.power_monitoring_relay_data[0]=device_data.power_monitoring_relay_data[0]|DC7_RELAY;
						//log_i("relay_on_off_data1111=0x%04x",device_data.relay_on_off_data);
						open_heat=1;
					}				
				}
				else
				{
					
					if((device_data.humi_value<=sys_psm.high_humi_data-10)&&(device_data.temp_value>=sys_psm.low_temp_data+15))
					{
						device_data.power_monitoring_relay_data[0]&=~DC7_RELAY;
						//log_i("relay_on_off_data1111=0x%04x",device_data.relay_on_off_data);
						open_heat=0;
					}	
				}	
			}
		}
		/////////////////////////////////////////////////////////////////////
		if(sys_psm.slave_ok==1)
		{
			led_500ms_cnt++;
			if(led_500ms_cnt>=3)
			{
				GPIO_TogglePins(LED_RUN_PORT, LED_RUN_PIN);
				led_500ms_cnt=0;
			}		
		}
		else
		{
			GPIO_TogglePins(LED_RUN_PORT, LED_RUN_PIN);		
		}			
		if(device_data.error_status_old==0)
		{
			led_arm_off();
		}
		else
		{
			led_arm_on();
		}
		if(device_data.mqtt_connect_flag==0)
		{
			led_net_off();
		}
		else
		{
			led_net_on();
		}		
		////////////////////////////////////////////////////////////////////
		if(display_num_old!=device_data.display_num)
		{
			timerout_cnt=0;
			oled_paint_clear();
			display_brush_flag=1;
			display_brush_reset_flag=1;
			display_num_old=device_data.display_num;
		}
		
		if(display_num_old==0)
		{
			//////////////////////////////////////////////////////////////////////////////////////////////
			if((water_in_error_flag_old!=device_data.water_in_error_flag)||(display_brush_reset_flag==1))
			{
				water_in_error_flag_old=device_data.water_in_error_flag;
				if(water_in_error_flag_old==0)
				{
					oled_show_string(0+32,0,(uint8_t *)"IN0:",16);
					oled_show_chinese(32+32,0,0,16);
					oled_show_chinese(48+32,0,1,16);
				}
				else
				{
					oled_show_string(0+32,0,(uint8_t *)"IN0:",16);
					oled_show_chinese(32+32,0,2,16);
					oled_show_chinese(48+32,0,3,16);				
				}
				display_brush_flag=1;
			}
			//////////////////////////////////////////////////////////////////////////////////////////////
			if((smoke_in_error_flag_old!=device_data.smoke_in_error_flag)||(display_brush_reset_flag==1))
			{
				smoke_in_error_flag_old=device_data.smoke_in_error_flag;
				if(smoke_in_error_flag_old==0)
				{
					oled_show_string(0+32,16,(uint8_t *)"IN1:",16);
					oled_show_chinese(32+32,16,0,16);
					oled_show_chinese(48+32,16,1,16);
				}
				else
				{
					oled_show_string(0+32,16,(uint8_t *)"IN1:",16);
					oled_show_chinese(32+32,16,2,16);
					oled_show_chinese(48+32,16,3,16);				
				}
				display_brush_flag=1;
			}
			//////////////////////////////////////////////////////////////////////////////////////////////
			if((lock_in_error_flag_old!=device_data.lock_in_error_flag)||(display_brush_reset_flag==1))
			{
				lock_in_error_flag_old=device_data.lock_in_error_flag;
				if(lock_in_error_flag_old==0)
				{
					oled_show_string(0+32,32,(uint8_t *)"IN2:",16);
					oled_show_chinese(32+32,32,0,16);
					oled_show_chinese(48+32,32,1,16);
				}
				else
				{
					oled_show_string(0+32,32,(uint8_t *)"IN2:",16);
					oled_show_chinese(32+32,32,2,16);
					oled_show_chinese(48+32,32,3,16);				
				}
				display_brush_flag=1;
			}
			//////////////////////////////////////////////////////////////////////////////////////////////
			if((spd_in_error_flag_old!=device_data.spd_in_error_flag)||(display_brush_reset_flag==1))
			{
				spd_in_error_flag_old=device_data.spd_in_error_flag;
				if(spd_in_error_flag_old==0)
				{
					oled_show_string(0+32,48,(uint8_t *)"IN3:",16);
					oled_show_chinese(32+32,48,0,16);
					oled_show_chinese(48+32,48,1,16);
				}
				else
				{
					oled_show_string(0+32,48,(uint8_t *)"IN3:",16);
					oled_show_chinese(32+32,48,2,16);
					oled_show_chinese(48+32,48,3,16);				
				}
				display_brush_flag=1;
			}
			//////////////////////////////////////////////////////////////////////////////////////////////
		}
		else if(display_num_old==1)
		{
			if((strcmp(device_id_old, sys_psm.device_id)!=0)||(display_brush_reset_flag==1))
			{
				memcpy(device_id_old, sys_psm.device_id, strlen(sys_psm.device_id)+1);
				sprintf(string_buf,"ID:%s",sys_psm.device_id);  
				oled_show_string(0,0,(uint8_t *)string_buf,16);  
				//oled_show_string(0,0,(uint8_t *)"ID:",16);
				display_brush_flag=1;
			}
			if((rs485_id_old!=sys_psm.rs485_id)||(display_brush_reset_flag==1))
			{
				rs485_id_old=sys_psm.rs485_id;
				sprintf(string_buf,"RS485 ID:%d",rs485_id_old);  	
				oled_show_string(0,32,(uint8_t *)string_buf,16); 
				display_brush_flag=1;
			}
		}
		else if(display_num_old==2)
		{
			if(display_brush_reset_flag==1)
			{				
				sprintf(string_buf,"Local IP:%d.%d.%d.%d",sys_psm.local.ip[0],sys_psm.local.ip[1],sys_psm.local.ip[2],sys_psm.local.ip[3]);  	
				oled_show_string(0,0,(uint8_t *)string_buf,16); 
				
				sprintf(string_buf,"Server IP:%d.%d.%d.%d",sys_psm.mqtt_server_ip[0],sys_psm.mqtt_server_ip[1],sys_psm.mqtt_server_ip[2],sys_psm.mqtt_server_ip[3]); 
				oled_show_string(0,32,(uint8_t *)string_buf,16); 
				display_brush_flag=1;
			}			
		}
		else if(display_num_old==3)
		{
			if((temp_value_old!=device_data.temp_value)||(display_brush_reset_flag==1))
			{
				temp_value_old=device_data.temp_value;
				oled_show_chinese(0,16,4,16);
				oled_show_chinese(16,16,5,16);                       
				sprintf(string_buf,":%.02f",temp_value_old);                      
				oled_show_string(32,16,(uint8_t *)string_buf,16); 
				oled_show_chinese(32+8*strlen(string_buf),16,7,16);   
				display_brush_flag=1;				
			}
			if((humi_value_old!=device_data.humi_value)||(display_brush_reset_flag==1))
			{	
				humi_value_old=device_data.humi_value;
				oled_show_chinese(0,32,6,16);
				oled_show_chinese(16,32,5,16);
				sprintf(string_buf,":%.02f",humi_value_old);                      
				oled_show_string(32,32,(uint8_t *)string_buf,16);  
				oled_show_chinese(32+8*strlen(string_buf),32,8,16);  
				display_brush_flag=1;
			}		
		}
		else if(display_num_old==4)
		{
			device_data.voltage_value=read_power_board_ac_voltage_value(0);
			device_data.electric_value=read_power_board_ac_current_value(0);
			if((voltage_value_old!=device_data.voltage_value)||(display_brush_reset_flag==1))
			{
				voltage_value_old=device_data.voltage_value;
				oled_show_chinese(0,0,9,16);
				oled_show_chinese(16,0,10,16);
				sprintf(string_buf,":%.01f V",voltage_value_old);                      
				oled_show_string(32,0,(uint8_t *)string_buf,16); 
				display_brush_flag=1;
			}	
			if((electric_value_old!=device_data.electric_value)||(display_brush_reset_flag==1))
			{
				electric_value_old=device_data.electric_value;
				oled_show_chinese(0,16,9,16);
				oled_show_chinese(16,16,11,16);
				sprintf(string_buf,":%.01f A",electric_value_old);                      
				oled_show_string(32,16,(uint8_t *)string_buf,16); 
				display_brush_flag=1;
			}
		}
		if(display_num_old!=0)
		{
			timerout_cnt++;
			if(timerout_cnt>=5*30)
			{
				timerout_cnt=0;
				device_data.display_num=0;
			}		
		}
		if((display_brush_flag==1)||(display_brush_reset_flag==1))
		{
			display_brush_flag=0;
			display_brush_reset_flag=0;
			oled_display();
		}		
		os_thread_sleep(os_msec_to_ticks(200)); 
	}
	os_thread_delete(NULL);
}

void display_oled_process_init(void)
{
	int ret=SYS_FAIL;
	ret = os_thread_create(&display_oled_thread, //任务控制块指针
							"oled_thread",//任务名字
							oled_control_thread, //任务入口函数
							NULL,//任务入口函数参数
							&oled_stack,//任务栈大小
							OS_PRIO_5);	//任务的优先级							
   if(ret==SYS_FAIL)
   {
		log_e(OLED_TAG,"oled thread create error!");
   }
}
