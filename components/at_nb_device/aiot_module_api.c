#include <aiot_state_api.h>
#include "posix_port.h"
#include "aiot_at_client_api.h"
#include "aiot_module_api.h"
#include "core_log.h"
#include "sys_os.h"
#include "sys_log.h"

#include "rtc_drv.h"
#include "project_config.h"
#include "nb_uart_dir.h"

at_response_t *resp = NULL;
struct nb_device nb_device_table = {0};

extern void aiot_mqtt_recv_handler(const char *data, uint32_t size);
extern void aiot_mqtt_event_handler(const char *data, uint32_t size);
//extern void aiot_dm_recv_handler(const char *data, uint32_t size);
//extern void aiot_dm_event_handler(const char *data, uint32_t size);
//extern void aiot_ota_recv_handler(const char *data, uint32_t size);
/* 注册回调 */ 
static at_urc_t g_urc_table[] = 
{
	//模块开机重启检测
    {"REBOOT_", "\r", aiot_mqtt_recv_handler},
    {"+NPING:", "\r", aiot_mqtt_recv_handler},
    {"+NPINGERR:", "\r", aiot_mqtt_recv_handler},
    //PSM指示
    {"+NPSMR:1", "\r", aiot_mqtt_recv_handler},
//  {"+NPSMR:0",           "\r",nb_event_func},

	//如果启用mqtt支持，增加mqtt函数调用
    {"+MQTTOPEN:", "\r", aiot_mqtt_recv_handler},
    {"+MQTTSUBACK:", "\r", aiot_mqtt_recv_handler},
    {"+MQTTPUBLISH:", "\r",aiot_mqtt_recv_handler},
    {"+MQTTPUBACK:", "\r", aiot_mqtt_recv_handler},
    {"+MQTTUNSUBACK:", "\r", aiot_mqtt_recv_handler},
    {"+MQTTDISC:", "\r", aiot_mqtt_recv_handler},
    {"+MQTTPUBREC:", "\r", aiot_mqtt_recv_handler},
    {"+MQTTPUBCOMP:", "\r",aiot_mqtt_recv_handler},
    {"+MQTTPINGRESP:", "\r", aiot_mqtt_recv_handler},
    {"+MQTTTO:", "\r", aiot_mqtt_recv_handler},
    {"+MQTTREC:", "\r", aiot_mqtt_recv_handler},
	{"+MQTTSTAT", "\r\n", aiot_mqtt_event_handler},  
};

int32_t aiot_module_init(void)
{
	aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    int32_t res = STATE_SUCCESS;
   // at_log_infonfo("Start moduel init param.");

    /* 初始化AT Client 参数和会话句柄 */
    res = at_client_init();
    if (res != STATE_SUCCESS)
    {
        at_log_error("ERROR at_client_init res=0x%X\r\n", res);
    }
    at_set_urc_table(g_urc_table, sizeof(g_urc_table) / sizeof(g_urc_table[0]));	
    /* 重启模组 */
    // TODO:增加硬件重启配置
	nbiot_power_off();
	os_thread_sleep(os_msec_to_ticks(500));
	nbiot_power_on();
	os_thread_sleep(os_msec_to_ticks(1000));
    return res;
}

void aiot_module_deinit(void) 
{
    at_log_info("aiot_module_deinit");
    at_client_deinit();
}

int32_t aiot_module_handshake(uint32_t timeout)
{
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    int32_t res = STATE_SUCCESS;

    while ((res = at_client_wait_connect(timeout)) != STATE_SUCCESS)
    {
        at_log_error("Wait OK ack failed,res = 0x%X.\r\n", res);
    }
    aiot_module_software_reboot();
   os_thread_sleep(os_msec_to_ticks(2000));
    while ((res = at_client_wait_connect(timeout)) != STATE_SUCCESS)
    {
        at_log_error("Wait OK ack failed,res = 0x%X.\r\n", res);
    }
    return res;
}

int32_t aiot_module_software_reboot(void)
{
    int32_t res = STATE_SUCCESS;

    if ((res = at_exec_cmd(NULL, AIOT_CMD_FMT_REBOOT_STR)) != STATE_SUCCESS)
    {
        at_log_error("AT command failed, res=0x%X.", res);
    }
    return res;
}

/*************************************************************
 *   函数名称：   nb_sim_check
 *
 *   函数功能：   检测模块是否为正常模式
 *
 *   入口参数：   无
 *
 *   返回参数：   0：成功   1：最小功能模式
 *
 *   说明：
 *************************************************************/
int32_t qsdk_nb_sim_check(void)
{
	//at_response_t *resp = NULL;
    int32_t res = STATE_SUCCESS;
	
    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);
	
    if (at_obj_exec_cmd(at_client_get(), resp, "AT+CFUN?") != STATE_SUCCESS)
	{
		if (resp != NULL)
		{
			at_delete_resp(resp);
		}		
		return STATE_FAILED;
	}
	at_resp_parse_line_args_by_kw(resp, "+CFUN:", "+CFUN:%d",&nb_device_table.sim_state);
   // at_resp_parse_line_args(resp, 2, "+CFUN:%d\r\n", &nb_device_table.sim_state);
	at_log_info("sim_state=%d",nb_device_table.sim_state);
    if (resp != NULL)
    {
        at_delete_resp(resp);
    }
    return STATE_SUCCESS;
}

/*************************************************************
 *   函数名称：   nb_get_imsi
 *
 *   函数功能：   获取 SIM 卡的 imsi
 *
 *   入口参数：   无
 *
 *   返回参数：   IMSI指针:成功    RT_NULL:失败
 *
 *   说明：
 *************************************************************/
int32_t qsdk_nb_get_imsi(void)	
{	
	//at_response_t *resp = NULL;
    int32_t res = STATE_SUCCESS;
	
    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);
	
    if (at_obj_exec_cmd(at_client_get(), resp, "AT+CIMI") != STATE_SUCCESS)
	{
		if (resp != NULL)
		{
			at_delete_resp(resp);
		}		
		return STATE_FAILED;
	}
    at_resp_parse_line_args_by_kw(resp, "+CIMI:", "+CIMI:%s",&nb_device_table.imsi);    
//    at_resp_parse_line_args(resp, 2, "+CIMI:%s\r\n", nb_device_table.imsi);
	//at_log_info("imsi=%s",nb_device_table.imsi);
    if (resp != NULL)
    {
        at_delete_resp(resp);
    }
    return STATE_SUCCESS;	
}

/*************************************************************
 *   函数名称：   nb_get_imei
 *
 *   函数功能：   获取模块的 imei
 *
 *   入口参数：   无
 *
 *   返回参数：   IMEI指针:成功    RT_NULL:失败
 *
 *   说明：
 *************************************************************/
int32_t qsdk_nb_get_imei(void)
{
	//at_response_t *resp = NULL;
    int32_t res = STATE_SUCCESS;
	
    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);
	
    if (at_obj_exec_cmd(at_client_get(), resp, "AT+CGSN=1") != STATE_SUCCESS)
	{
		if (resp != NULL)
		{
			at_delete_resp(resp);
		}		
		return STATE_FAILED;
	}
    at_resp_parse_line_args_by_kw(resp, "+CGSN:", "+CGSN:%s",&nb_device_table.imei);     
    //at_resp_parse_line_args(resp, 2, "+CGSN:%s", nb_device_table.imei);
	//at_log_info("imei=%s",nb_device_table.imei);
    if (resp != NULL)
    {
        at_delete_resp(resp);
    }
    return STATE_SUCCESS;	
}

/*************************************************************
 *   函数名称：   nb_set_rtc_time
 *
 *   函数功能：   设置RTC时间为当前时间
 *
 *   入口参数：   year:年   month:月    day:日   hour:时  min:分   sec:秒
 *
 *   返回参数：   无
 *
 *   说明：
 *************************************************************/
static void qsdk_nb_set_rtc_time(int year, int month, int day, int hour, int min, int sec)
{
    int week, lastday;
    hour += QSDK_TIME_ZONE;
    if ((0 == year % 4 && 0 != year % 100) || 0 == year % 400)
        lastday = 29;
    else if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12)
        lastday = 31;
    else if (month == 4 || month == 6 || month == 9 || month == 11)
        lastday = 30;
    else
        lastday = 28;
    if (hour > 24)
    {
        hour -= 24;
        day++;
        if (day > lastday)
        {
            day -= lastday;
            month++;
        }
        if (month > 12)
        {
            month -= 12;
            year++;
        }
    }
    week = (day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400) % 7 + 1;
#ifndef QSDK_USING_ME3616
    year += 2000;
#endif
    at_log_info("time:%d-%d-%d,%d:%d:%d,week:%d\n", year, month, day, hour, min, sec, week);
	//rtc_set_time(year, month, day, hour, min, sec, week,&device_data.current_date,&device_data.current_time);
   // qsdk_rtc_set_time_callback(year, month, day, hour, min, sec, week);
}

/*************************************************************
 *   函数名称：   qsdk_nb_get_time
 *
 *   函数功能：   获取网络时间
 *
 *   入口参数：   无
 *
 *   返回参数：   0:成功  1:失败
 *
 *   说明：
 *************************************************************/
int32_t qsdk_nb_get_time(void)
{
	//at_response_t *resp = NULL;
    int32_t res = STATE_SUCCESS;
	int year, mouth, day, hour, min, sec;
    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);
	
    if (at_obj_exec_cmd(at_client_get(), resp, "AT+CCLK?") != STATE_SUCCESS)
	{
		if (resp != NULL)
		{
			at_delete_resp(resp);
		}		
		return STATE_FAILED;
	}
    at_resp_parse_line_args_by_kw(resp, "+CCLK:", "+CCLK:%d/%d/%d,%d:%d:%d+",&year, &mouth, &day, &hour, &min, &sec);     
    //at_resp_parse_line_args(resp, 2, "+CCLK:%d/%d/%d,%d:%d:%d+", &year, &mouth, &day, &hour, &min, &sec);
	qsdk_nb_set_rtc_time(year, mouth, day, hour, min, sec);
    if (resp != NULL)
    {
        at_delete_resp(resp);
    }
    return STATE_SUCCESS;
}
/*************************************************************
 *   函数名称：   qsdk_nb_get_csq
 *
 *   函数功能：   获取当前信号值
 *
 *   入口参数：   无
 *
 *   返回参数：   0-99:成功    -1:失败
 *
 *   说明：
 *************************************************************/
int32_t qsdk_nb_get_csq(void)
{
	//at_response_t *resp = NULL;
    int32_t res = STATE_SUCCESS;
	
    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);
	
    if (at_obj_exec_cmd(at_client_get(), resp, "AT+CSQ") != STATE_SUCCESS)
	{
		if (resp != NULL)
		{
			at_delete_resp(resp);
		}		
		return STATE_FAILED;
	}
    at_resp_parse_line_args_by_kw(resp, "+CSQ:", "+CSQ:%d,%d",&nb_device_table.csq, &nb_device_table.ber);         
    //at_resp_parse_line_args(resp, 2, "+CSQ:%d,%d", &nb_device_table.csq, &nb_device_table.ber);
	//at_log_info("csq=%d",nb_device_table.csq);
    if (resp != NULL)
    {
        at_delete_resp(resp);
    }
    return STATE_SUCCESS;
}

/*************************************************************
 *   函数名称：   qsdk_nb_get_net_connect
 *
 *   函数功能：   获取当前网络状态
 *
 *   入口参数：   无
 *
 *   返回参数：   0:成功  1:失败
 *
 *   说明：
 *************************************************************/
int32_t qsdk_nb_get_net_connect(void)
{
	//at_response_t *resp = NULL;
    int32_t res = STATE_SUCCESS;
	 int i;
	
    resp = at_create_resp(128, 0, AIOT_WAIT_MAX_TIME, NULL, 0);
	
    if (at_obj_exec_cmd(at_client_get(), resp, "AT+CEREG?") != STATE_SUCCESS)
	{
		if (resp != NULL)
		{
			at_delete_resp(resp);
		}		
		return STATE_FAILED;
	}
        
    at_resp_parse_line_args_by_kw(resp, "+CEREG:", "+CEREG:%d,%d", &i, &nb_device_table.net_connect_ok);
	//at_log_info("net_connect_ok=%d",nb_device_table.net_connect_ok);
    if (resp != NULL)
    {
        at_delete_resp(resp);
    }
    if (nb_device_table.net_connect_ok == 1 || nb_device_table.net_connect_ok == 5)
    {
        return STATE_SUCCESS;
    }
    else
    {
        return STATE_FAILED;
    }	
}

/*************************************************************
 *   函数名称：   qsdk_nb_get_net_connect_status
 *
 *   函数功能：   获取查询到的网络状态
 *
 *   入口参数：   无
 *
 *   返回参数：   0:成功  1:失败
 *
 *   说明：
 *************************************************************/
int qsdk_nb_get_net_connect_status(void)
{
    if (nb_device_table.net_connect_ok)
	{
		return STATE_SUCCESS;
	}
    return STATE_FAILED;
}
/*************************************************************
 *   函数名称：   qsdk_nb_open_net_light
 *
 *   函数功能：   打开网络指示灯
 *
 *   入口参数：   无
 *
 *   返回参数：   0:成功    1:失败
 *
 *   说明：
 *************************************************************/
int qsdk_nb_open_net_light(void)
{
#ifdef QSDK_USING_M5311
#if (defined QSDK_USING_LOG)
    LOG_D("AT+CMSYSCTRL=0,2\n");
#endif
    if (at_obj_exec_cmd(at_client_get(), NULL, "AT+CMSYSCTRL=0,2") != STATE_SUCCESS)
        return STATE_FAILED;
#else
#if (defined QSDK_USING_LOG)
    LOG_D("AT+ZCONTLED=1\n");
#endif
    if (at_obj_exec_cmd(at_client_get(), NULL, "AT+CMSYSCTRL=2") != STATE_SUCCESS)
        return STATE_FAILED;
#endif
    return STATE_SUCCESS;
}

/*************************************************************
 *   函数名称：   qsdk_nb_quick_connect
 *
 *   函数功能：   NB-IOT 模块一键联网初始化
 *
 *   入口参数：   无
 *
 *   返回参数：   0：成功   1：失败
 *
 *   说明：
 *************************************************************/
int qsdk_nb_quick_connect(void)
{
    uint32_t i = 5;
	aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    int32_t res = STATE_SUCCESS;
    /*********************模块上电强制设置**************************/

//#if (defined QSDK_USING_M5310) || (defined QSDK_USING_M5310A)
//    //开启进入PSM提示
//#if (defined QSDK_USING_DEBUG)
//    LOG_D("AT+NPSMR=1\n");
//#endif
//    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+NPSMR=1") != RT_EOK)
//        return RT_ERROR;

//    if (qsdk_nb_get_psm_status() != RT_EOK)
//    {
//        qsdk_nb_set_psm_mode(0, RT_NULL, RT_NULL);
//    }
//#elif (defined QSDK_USING_M5311)
//    //关闭回显
//#if (defined QSDK_USING_DEBUG)
//    LOG_D("ATE0\n");
//#endif
//    if (at_obj_exec_cmd(nb_client, nb_resp, "ATE0") != RT_EOK)
//        return RT_ERROR;

//        //开启进入PSM提示
//#if (defined QSDK_USING_DEBUG)
//    LOG_D("AT*SLEEP=1\n");
//#endif
//    if (at_obj_exec_cmd(nb_client, nb_resp, "AT*SLEEP=1") != RT_EOK)
//        return RT_ERROR;

//        //开启退出PSM提示
//#if (defined QSDK_USING_DEBUG)
//    LOG_D("AT*MATWAKEUP=1\n");
//#endif
//    if (at_obj_exec_cmd(nb_client, nb_resp, "AT*MATWAKEUP=1") != RT_EOK)
//        return RT_ERROR;

//        //开启wakeup_out 提示
//#if (defined QSDK_USING_DEBUG)
//    LOG_D("AT+CMSYSCTRL=1,1\n");
//#endif
//    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+CMSYSCTRL=1,1") != RT_EOK)
//        return RT_ERROR;

//    if (qsdk_nb_get_psm_status() != RT_EOK)
//    {
//        qsdk_nb_close_auto_psm();
//    }
//    qsdk_nb_open_net_light();

//#if (defined QSDK_USING_NET)
//    qsdk_net_set_out_format();
//#endif

//#elif (defined QSDK_USING_ME3616)
//    //关闭回显
//#if (defined QSDK_USING_DEBUG)
//    LOG_D("ATE0\n");
//#endif
//    if (at_obj_exec_cmd(nb_client, nb_resp, "ATE0") != RT_EOK)
//        return RT_ERROR;

//        //开启进入PSM提示
//#if (defined QSDK_USING_DEBUG)
//    LOG_D("AT*MNBIOTEVENT=1,1\n");
//#endif
//    if (at_obj_exec_cmd(nb_client, nb_resp, "AT*MNBIOTEVENT=1,1") != RT_EOK)
//        return RT_ERROR;

//    qsdk_nb_open_net_light();

//    if (qsdk_nb_get_psm_status() != RT_EOK)
//    {
//        qsdk_nb_set_psm_mode(0, RT_NULL, RT_NULL);
//    }
//#if (defined QSDK_USING_IOT)
//    //设置数据发送模式
//#if (defined QSDK_USING_DEBUG)
//    LOG_D("AT+M2MCLICFG=1,0\n");
//#endif
//    if (at_obj_exec_cmd(nb_client, nb_resp, "AT+M2MCLICFG=1,0") != RT_EOK)
//        return RT_ERROR;
//#endif
//#endif
   os_thread_sleep(os_msec_to_ticks(1000));
   //////////////////////////////////////////////////////////////////////////////
    //首先确定模块是否开机
    do
    {
        i--;
        if (qsdk_nb_sim_check() != STATE_SUCCESS)
        {
            os_thread_sleep(os_msec_to_ticks(1000));
        }
        at_log_info("+CFUN=%d\n", nb_device_table.sim_state);

        if (nb_device_table.sim_state != 1)
		{
			os_thread_sleep(os_msec_to_ticks(1000));
		}
    } while (nb_device_table.sim_state == 0 && i > 0);

    if (i <= 0)
    {
        at_log_error("NB-IOT boot failure, please check SIM card\n");
        return STATE_FAILED;
    }
    else
    {
        i = 5;
        os_thread_sleep(os_msec_to_ticks(1000));
    }
	//////////////////////////////////////////////////////////////////////////////
    //获取SIM卡的IMSI号码
    do
    {
        if (qsdk_nb_get_imsi() != STATE_SUCCESS)
        {
            os_thread_sleep(os_msec_to_ticks(500));
        }
        else
        {
            at_log_info("IMSI=%s\n", nb_device_table.imsi);
            break;
        }
		i--;
    } while (i > 0);

    if (i <= 0)
    {
        at_log_info("No SIM card found\n");
        return STATE_FAILED;
    }
    else
    {
        i = 15;
		os_thread_sleep(os_msec_to_ticks(100));
    }

    //获取模块IMEI
    if (qsdk_nb_get_imei() != STATE_SUCCESS)
    {
        at_log_error("Nb-iot IMEI not foundr\n");
        return STATE_FAILED;
    }
    else
    {
        at_log_info("IMEI=%s\n", nb_device_table.imei);
    }
	qsdk_nb_open_net_light();
    //获取信号值
	i = 15;
    do
    {
        i--;
		if (qsdk_nb_get_csq() == STATE_SUCCESS)
		{
			if (nb_device_table.csq != 99 && nb_device_table.csq != 0)
			{
				break;
			}
			else
			{
				at_log_info("CSQ=%d\n", nb_device_table.csq);
				os_thread_sleep(os_msec_to_ticks(3000));
			}
		}
		os_thread_sleep(os_msec_to_ticks(500));
    } while (i > 0);

    if (i <= 0)
    {
        at_log_error("nb-iot not find the signal\n");
        return STATE_FAILED;
    }
    else
    {
        at_log_info("CSQ=%d\n", nb_device_table.csq);
        i = 40;
        os_thread_sleep(os_msec_to_ticks(100));
    }
    do
    {
        i--;
        if (qsdk_nb_get_net_connect() == STATE_SUCCESS)
		{
			break;
		}
        at_log_info("CEREG=%d\n", nb_device_table.net_connect_ok);
        os_thread_sleep(os_msec_to_ticks(1000));
    } while (i > 0);

    if (i <= 0)
    {
        at_log_error("nb-iot connect network failurer\n");
        return STATE_FAILED;
    }
    os_thread_sleep(os_msec_to_ticks(2000));
    at_log_info("CEREG=%d\n", nb_device_table.net_connect_ok);

    //获取ntp服务器时间

    if (qsdk_nb_get_time() != STATE_SUCCESS)
    {
        at_log_error("getting network time errorsr\n");
    }
    at_log_info("nb-iot connect network success\n");

    return STATE_SUCCESS;
}

extern at_client_t *at_client_get();
int32_t aiot_at_hal_recv_handle(uint8_t *data, uint32_t size) 
{
    at_client_t *client = at_client_get();
    if (client == NULL || client->status == AT_STATUS_UNINITIALIZED ||client->ring_buff.buf == NULL) 
	{
        return -1;
    }
    return core_ringbuf_write(&(client->ring_buff), (const uint8_t *)data, size);
}