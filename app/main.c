/*
* Copyright (c) 2022 ,深圳市科安科技有限公司
* All rights reserved
*文件名称：
*摘要：
*当前版本：1.0   
*编写者；何东达 
*编写时间：2023.1.11 
*/
#include "project_config.h"
#include "project_pin_use_config.h"
#include "sys_log.h"
#include "sys_os.h"
#include "sys_stdio.h"
#include "sys_time.h"
#include "partition.h"
#include "psm_v2.h"
#include "project_psm_control.h"
//#include "psm_utils.h"

#include "oled_handle.h"
#include "key_handle.h"
#include "bat_dir.h"
#include "autoeclosing_rs485_handle.h"
#include "monitor_board_handle.h"
#include "lock_rs485_handle.h"
#include "gps_control.h"
#include "healthmon.h"
#include "nb_iot_mqtt_handle.h"
#include "camera_handle.h"
#include "posix_port.h"
#include "eth_handle.h"
#include "circuit_breaker_rs485_handle.h"
#include "rtc_drv.h"

sys_type device_data;

static os_thread_t main_start_thread;
static os_thread_stack_define(main_start_stack, 5*1024);

extern void xPortSysTickHandler (void);

void SysTick_IrqHandler(void)
{
    SysTick_IncTick();
    xPortSysTickHandler();
}

/**
 * @brief  BSP clock initialize.
 *         SET board system clock to PLLH@240MHz
 *         Flash: 5 wait
 *         SRAM_HS: 1 wait
 *         SRAM1_2_3_4_B: 2 wait
 *         PCLK0: 240MHz
 *         PCLK1: 120MHz
 *         PCLK2: 60MHz
 *         PCLK3: 60MHz
 *         PCLK4: 120MHz
 *         EXCLK: 120MHz
 *         HCLK:  240MHz
 * @param  None
 * @retval None
 */
void system_clk_init(void)
{
    stc_clock_xtal_init_t stcXtalInit;
    stc_clock_pll_init_t stcPLLHInit;

    CLK_SetClockDiv(CLK_BUS_CLK_ALL, \
                    (CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 | CLK_PCLK2_DIV4 | \
                     CLK_PCLK3_DIV4 | CLK_PCLK4_DIV2 | CLK_EXCLK_DIV2 | \
                     CLK_HCLK_DIV1));
	
	GPIO_AnalogCmd(BSP_XTAL_PORT, BSP_XTAL_IN_PIN | BSP_XTAL_OUT_PIN, ENABLE);
    (void)CLK_XtalStructInit(&stcXtalInit);
    /* Config Xtal and enable Xtal */
    stcXtalInit.u8Mode   = CLK_XTAL_MD_OSC;
    stcXtalInit.u8Drv    = CLK_XTAL_DRV_ULOW;
    stcXtalInit.u8State  = CLK_XTAL_ON;
    stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
    (void)CLK_XtalInit(&stcXtalInit);

    (void)CLK_PLLStructInit(&stcPLLHInit);
    /* VCO = (8/1)*100 = 800MHz*/
    stcPLLHInit.u8PLLState      = CLK_PLL_ON;
    stcPLLHInit.PLLCFGR         = 0UL;
    stcPLLHInit.PLLCFGR_f.PLLM  = 1UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLN  = 120UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLP  = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLQ  = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLR  = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    (void)CLK_PLLInit(&stcPLLHInit);

    /* Highspeed SRAM set to 1 Read/Write wait cycle */
    SRAM_SetWaitCycle(SRAM_SRAMH, SRAM_WAIT_CYCLE1, SRAM_WAIT_CYCLE0);
    /* SRAM1_2_3_4_backup set to 2 Read/Write wait cycle */
    SRAM_SetWaitCycle((SRAM_SRAM123 | SRAM_SRAM4 | SRAM_SRAMB), SRAM_WAIT_CYCLE1, SRAM_WAIT_CYCLE1);
    /* 0-wait @ 40MHz */
    EFM_SetWaitCycle(EFM_WAIT_CYCLE5);
    /* 4 cycles for 200 ~ 250MHz */
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT4);

    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_PLL);
	
	FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_FCM, ENABLE);
	FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_SRAMH, ENABLE);
	FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_SRAM1, ENABLE);
	FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_SRAM2, ENABLE);
	FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_SRAM3, ENABLE);
	FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_SRAM4, ENABLE);
}

/*
*函数介绍：数据初始化
*参数：无
*返回值：无
*备注：无
*/
void data_init(void)
{
	int i;

	if (psm_control_init() == SYS_OK) 
	{
		os_thread_sleep(os_msec_to_ticks(10));

		read_write_device_id_flash(READ_PSM);
		read_write_rs485_id_flash(READ_PSM);
		read_write_query_time_flash(READ_PSM);
		read_write_high_temp_flash(READ_PSM);
		read_write_low_temp_flash(READ_PSM);
		read_write_high_humi_flash(READ_PSM);
		read_write_low_humi_flash(READ_PSM);
		/////////////////////////////////////////////////////////////////////
		read_write_local_ip_flash(READ_PSM);
		read_write_local_mask_flash(READ_PSM);
		read_write_local_gateway_flash(READ_PSM);
		read_write_local_dns_flash(READ_PSM);
		read_write_local_mac_flash(READ_PSM);
		read_write_local_port_flash(READ_PSM);	
		read_write_local_dhcp_flash(READ_PSM);
		/////////////////////////////////////////////////////////////////////
		read_write_mqtt_user_flash(READ_PSM);
		read_write_mqtt_pass_flash(READ_PSM);
		read_write_mqtt_server_ip_flash(READ_PSM);
		read_write_mqtt_server_port_flash(READ_PSM);
		read_write_mqtt_server1_ip_flash(READ_PSM);
		read_write_mqtt_server1_port_flash(READ_PSM);
		read_write_mqtt_server2_ip_flash(READ_PSM);
		read_write_mqtt_server2_port_flash(READ_PSM);
		read_write_mqtt_server3_ip_flash(READ_PSM);
		read_write_mqtt_server3_port_flash(READ_PSM);
		read_write_mqtt_report_timer_flash(READ_PSM);
		
		read_write_mqtt_pub_sensor_flash(READ_PSM);
		read_write_mqtt_pub_alarm_flash(READ_PSM);
		read_write_mqtt_sub_command_flash(READ_PSM);
		read_write_mqtt_sub_alarm_flash(READ_PSM);
		read_write_mqtt_pub_back1_flash(READ_PSM);		
		/////////////////////////////////////////////////////////////////////
		read_write_slave_ok_flash(READ_PSM);
		device_data.slave_device_max=0;
		if(sys_psm.slave_ok==1)
		{
			for (i = 0; i < SLAVE_MAX_NUM; i++)
			{
				read_write_slave_information_flash(READ_PSM,i);
				if(DAT_SLAVE_TYPE!=sys_psm.slave_device[i].device_type)
				{
					device_data.own_slave_device[device_data.slave_device_max]=sys_psm.slave_device[i];
					device_data.slave_device_max++;
				}
			}
		}
	} 
	else
	{
		log_e("app","flash_init error");
	}
}

/* This function is defined for handling critical error.
 * For this application, we just stall and do nothing when
 * a critical error occurs.
 *
 */
void final_about_to_die()
{
	log_e("main","swdt failed");/* do nothing ... */
}

uint8_t read_mode(void)
{
	uint8_t i=0,j;

	j=GPIO_ReadInputPins(MODE1_PORT, MODE1_PIN);
	if(j==PIN_RESET)
	{
		i|=1;
	}
	else
	{
		i&=~1;
	}

	j=GPIO_ReadInputPins(MODE2_PORT, MODE2_PIN);
	if(j==PIN_RESET)
	{
		i|=2;
	}
	else
	{
		i&=~2;
	}
	j=GPIO_ReadInputPins(MODE3_PORT, MODE3_PIN);
	if(j==PIN_RESET)
	{
		i|=4;
	}
	else
	{
		i&=~4;
	}

	j=GPIO_ReadInputPins(MODE4_PORT, MODE4_PIN);
	if(j==PIN_RESET)
	{
		i|=8;
	}
	else
	{
		i&=~8;
	}
	return i;
}

/*
*函数介绍：为了方便管理，所有的任务创建函数都放在这个函数里面
*参数：无
*返回值：
*备注：无
*/
void main_start(os_thread_arg_t data)
{
	int ret;
//	uint8_t pcWriteBuffer[1024];
	
	log_i("Build Time: " __DATE__ " " __TIME__ "");
	sys_time_init();
	///////////////////////////////////////////////////////////////////////////
	ret = healthmon_init();
	if (ret != SYS_OK) 
	{
		log_e("main","Error: healthmon_init failed");
	}
	ret = healthmon_start();
	if (ret != SYS_OK) 
	{
		log_e("main","Error: healthmon_start failed");
	}
	healthmon_set_final_about_to_die_handler((void (*)())final_about_to_die);/* Set the final_about_to_die handler of the healthmon */	
	///////////////////////////////////////////////////////////////////////////
	psm_control_init();
	conf_default_psm();
//	psm_control_cleanup();
	data_init();
	////////////////////////////////
	display_oled_process_init();
	key_process_init();
	lock_process_init();	
	modbus_process_init();
	gps_init();
	if((device_data.run_mode_id&0x01)==0x01)
	{
		at_hal_init();
		nb_mqtt_process_init();		
	}
	if((device_data.run_mode_id&0x02)==0x02)
	{
		device_data.autoeclosing_enable_flag=1;
		autoeclosing_process_init();
	}
	else if((device_data.run_mode_id&0x02)==0x04)
	{
		device_data.circuit_breaker_enable_flag=1;
		circuit_breaker_process_init();
	}	
	w5500_process_init();
	camera_process_init();

	while(1)
	{
//		printf("=================================================\r\n");
//		printf("任务名            任务状态   优先级   剩余栈 任务序号\r\n");
//		vTaskList((char *)&pcWriteBuffer);
//		printf("%s\r\n", pcWriteBuffer);	
//		printf("任务名             运行计数         使用率\r\n");
//		vTaskGetRunTimeStats((char *)&pcWriteBuffer);
//		printf("%s\r\n", pcWriteBuffer);
//		printf("当前动态内存剩余大小 = %d字节\r\n", xPortGetFreeHeapSize());
//		printf("=================================================\r\n");
		
		gps_data_resolving();
		
		if((device_data.run_mode_id&0x01)==0x01)
		{
			if(device_data.nb_start_flag==0)
			{
				device_data.nb_rest_cnt++;
				if(device_data.nb_rest_cnt>=60)
				{
					device_data.nb_rest_cnt=0;
					nb_mqtt_process_init();	
				}
			}		
		}
		////////////////////////////////////////////////////////////////
		if(device_data.fan_temp_flag!=0)
		{
			if(device_data.fan_temp_flag&0x01)
			{
				sys_psm.high_temp_data=device_data.fan_open_temp;
				read_write_high_temp_flash(WRITE_PSM);
			}
			if(device_data.fan_temp_flag&0x02)
			{
				sys_psm.low_temp_data=device_data.fan_close_temp;
				read_write_low_temp_flash(WRITE_PSM);
			}
			device_data.fan_temp_flag=0;
		}
		if(device_data.hot_humi_flag!=0)
		{
			if(device_data.hot_humi_flag&0x01)
			{
				sys_psm.high_humi_data=device_data.hot_fan_open_humi;
				read_write_high_humi_flash(WRITE_PSM);			
			}
			if(device_data.hot_humi_flag&0x02)
			{
				sys_psm.low_humi_data=device_data.hot_fan_close_humi;
				read_write_low_humi_flash(WRITE_PSM);			
			}
			device_data.hot_humi_flag=0;
		}	
		if(device_data.factory_flag==1)
		{
			device_data.factory_flag=0;
			read_write_local_ip_flash(WRITE_PSM_DEFULT);
			read_write_local_mask_flash(WRITE_PSM_DEFULT);
			read_write_local_gateway_flash(WRITE_PSM_DEFULT);
			read_write_local_dns_flash(WRITE_PSM_DEFULT);	
			os_thread_sleep(os_msec_to_ticks(1000));
			NVIC_SystemReset();
		}
		////////////////////////////////////////////////////////////////
		rtc_get_time(&device_data.c_date,&device_data.c_time);	
		
//		device_data.run_mode_id=read_mode();
//		log_i("run_mode_id:%d",device_data.run_mode_id);
		log_i("men:%d",xPortGetFreeHeapSize());
		os_thread_sleep(os_msec_to_ticks(1000));
	}
	os_thread_delete(NULL);
}

/*
*函数介绍：
*参数：无
*返回值：
*备注：无
*/
int os_init(void)
{
	int ret;

	ret = os_thread_create(&main_start_thread, 
							"main_start",
							main_start, 
							0,
							&main_start_stack,
							tskIDLE_PRIORITY + 3);	
	vTaskStartScheduler();

	return ret == pdPASS ? SYS_OK : SYS_FAIL;
}

/*
*函数介绍：
*参数：无
*返回值：
*备注：无
*/
static void mode_init(void)
{
	stc_gpio_init_t stcGpioInit;

	(void)GPIO_StructInit(&stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(MODE1_PORT, MODE1_PIN, &stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(MODE2_PORT, MODE2_PIN, &stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(MODE3_PORT, MODE3_PIN, &stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(MODE4_PORT, MODE4_PIN, &stcGpioInit);
}

/*
*函数介绍：
*参数：无
*返回值：
*备注：无
*/
int32_t main(void)
{
	LL_PERIPH_WE(PROJECT_PERIPH_WE);
	system_clk_init(); 		
	(void)SysTick_Init(1000); 
	sys_stdio_init(DBG_DEVICE, DBG_BAUDRATE);
	mode_init();
	bat_init();
	device_data.run_mode_id=read_mode();
	LL_PERIPH_WP(PROJECT_PERIPH_WP);
	os_init();
	while (1) 
	{

	}
}


