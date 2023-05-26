/**
 * @file	httpUtil.c
 * @brief	HTTP Server Utilities	
 * @version 1.0
 * @date	2014/07/15
 * @par Revision
 *			2014/07/15 - 1.0 Release
 * @author	
 * \n\n @par Copyright (C) 1998 - 2014 WIZnet. All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "project_config.h"
#include "http_util.h"
#include "project_psm_control.h"
//#include "types.h"
//#include "common.h"
#include "w5500_socket.h"
//#include "userHandler.h"
//#include "ConfigData.h"
#include "sys_log.h"

#ifdef _USE_FLASH_
#include "dataflash.h"
#endif


void make_json_info(uint8_t * buf, uint16_t * len)
{
	wiz_NetInfo netinfo;
	ctlnetwork(CN_GET_NETINFO, (void*) &netinfo);

	// DHCP: 1 - Static, 2 - DHCP
	*len = sprintf((char *)buf, "NetinfoCallback({ \
											\"pcode\":\"%s\",\
											\"fwver\":\"%d.%d.%d\",\
											\"devname\":\"%s\",\
											\"mac\":\"%.2X:%.2X:%.2X:%.2X:%.2X:%.2X\",\
											\"ip\":\"%d.%d.%d.%d\",\
											\"gw\":\"%d.%d.%d.%d\",\
											\"sub\":\"%d.%d.%d.%d\",\
											\"dns\":\"%d.%d.%d.%d\",\
											\"dhcp\":\"%d\",\
											\"use\":\"%s\",\
											\"pass\":\"%s\",\
											\"time\":\"%d\",\
											\"mip\":\"%d.%d.%d.%d\",\
											\"mport\":\"%d\",\
											\"sip1\":\"%d.%d.%d.%d\",\
											\"sport1\":\"%d\",\
											\"sip2\":\"%d.%d.%d.%d\",\
											\"sport2\":\"%d\",\
											\"sip3\":\"%d.%d.%d.%d\",\
											\"sport3\":\"%d\",\
											\"mpop\":\"%s\",\
											\"msub\":\"%s\",\
											\"mpopa\":\"%s\",\
											\"mpopc\":\"%s\",\
											\"mpopb\":\"%s\",\
											\"fan1\":\"%d\",\
											\"fan2\":\"%d\",\
											\"fhot1\":\"%d\",\
											\"fhot2\":\"%d\"\
											});",
											PRODUCT_NAME,
											MCU_SW_VERSION_MAJOR,MCU_SW_VERSION_MINOR,MCU_SW_VERSION_BUILD,
											sys_psm.device_id,
											netinfo.mac[0], netinfo.mac[1], netinfo.mac[2], netinfo.mac[3], netinfo.mac[4], netinfo.mac[5],
											netinfo.ip[0], netinfo.ip[1], netinfo.ip[2], netinfo.ip[3],
											netinfo.gw[0], netinfo.gw[1], netinfo.gw[2], netinfo.gw[3],
											netinfo.sn[0], netinfo.sn[1], netinfo.sn[2], netinfo.sn[3],
											netinfo.dns[0], netinfo.dns[1], netinfo.dns[2], netinfo.dns[3],
											netinfo.dhcp-1,
											sys_psm.mqtt_user,
											sys_psm.mqtt_pass,
											sys_psm.mqtt_updata_time,
											sys_psm.mqtt_server_ip[0],sys_psm.mqtt_server_ip[1],sys_psm.mqtt_server_ip[2],sys_psm.mqtt_server_ip[3],
											sys_psm.mqtt_server_port,
											sys_psm.server1_ip[0],sys_psm.server1_ip[1],sys_psm.server1_ip[2],sys_psm.server1_ip[3],
											sys_psm.server1_port,
											sys_psm.server2_ip[0],sys_psm.server2_ip[1],sys_psm.server2_ip[2],sys_psm.server2_ip[3],
											sys_psm.server2_port,
											sys_psm.server3_ip[0],sys_psm.server3_ip[1],sys_psm.server3_ip[2],sys_psm.server3_ip[3],
											sys_psm.server3_port,
											sys_psm.mqtt_publish_sensor_buf,
											sys_psm.mqtt_subscribe_command_buf,
											sys_psm.mqtt_publish_alarm_buf,
											sys_psm.mqtt_publish_ack_buf,
											sys_psm.mqtt_publish_back1_buf,
											sys_psm.high_temp_data,
											sys_psm.low_temp_data,
											sys_psm.high_humi_data,
											sys_psm.low_humi_data
											);
}


uint8_t predefined_get_cgi_processor(uint8_t * uri_name, uint8_t * buf, uint16_t * len)
{
	uint8_t ret = 1;	// ret = 1 means 'uri_name' matched

	if(strcmp((const char *)uri_name, "get_info.cgi") == 0)
	{
		make_json_info(buf, len);
	}
	else
	{

	}

	return ret;
}

uint8_t set_netinfo(uint8_t * uri)
{
	uint8_t ret = 0;
	uint8_t * param;

	log_i("set_netinfo %s",uri);
	if((param = get_http_param_value((char *)uri, "dhcp")))
	{		
		if(strstr((char const*)param, "1") != NULL)
		{
			sys_psm.local.dhcp=2;// DHCP mode
		}
		else 
		{
			sys_psm.local.dhcp=1;// Static mode
		}			
		ret = 1;
		read_write_local_dhcp_flash(WRITE_PSM);
	}		
	if(sys_psm.local.dhcp==1) // Static mode
	{
		if((param = get_http_param_value((char *)uri, "ip")))
		{
			inet_addr_((uint8_t*)param, sys_psm.local.ip);
			read_write_local_ip_flash(WRITE_PSM);
			ret = 1;
		}
		if((param = get_http_param_value((char *)uri, "gw")))
		{
			inet_addr_((uint8_t*)param, sys_psm.local.gw);
			read_write_local_gateway_flash(WRITE_PSM);
			ret = 1;
		}
		if((param = get_http_param_value((char *)uri, "sub")))
		{
			inet_addr_((uint8_t*)param, sys_psm.local.sn);
			read_write_local_mask_flash(WRITE_PSM);
			ret = 1;
		}
		if((param = get_http_param_value((char *)uri, "dns")))
		{
			inet_addr_((uint8_t*)param, sys_psm.local.dns);
			read_write_local_dns_flash(WRITE_PSM);
			ret = 1;
		}
	}
	return ret;
}

uint8_t set_mqttinfo(uint8_t * uri)
{
	uint8_t ret = 0;
	uint8_t * param;
	uint16_t str_size;
	
	if((param = get_http_param_value((char *)uri, "use")))
	{
		memset(sys_psm.mqtt_user, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_user, param, str_size);
		read_write_mqtt_user_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "pass")))
	{
		memset(sys_psm.mqtt_pass, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_pass, param, str_size);
		read_write_mqtt_pass_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "time")))
	{
		sys_psm.mqtt_updata_time= ATOI(param, 10);
		log_i("mqtt_updata_time=%d",sys_psm.mqtt_updata_time);
		read_write_mqtt_report_timer_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "mip")))
	{
		inet_addr_((uint8_t*)param, sys_psm.mqtt_server_ip);
		read_write_mqtt_server_ip_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "mport")))
	{
		sys_psm.mqtt_server_port= ATOI(param, 10);
		read_write_mqtt_server_port_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sip1")))
	{
		inet_addr_((uint8_t*)param, sys_psm.server1_ip);
		read_write_mqtt_server1_ip_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sport1")))
	{
		sys_psm.server1_port= ATOI(param, 10);
		read_write_mqtt_server1_port_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sip2")))
	{
		inet_addr_((uint8_t*)param, sys_psm.server2_ip);
		read_write_mqtt_server2_ip_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sport2")))
	{
		sys_psm.server2_port= ATOI(param, 10);
		read_write_mqtt_server2_port_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sip3")))
	{
		inet_addr_((uint8_t*)param, sys_psm.server3_ip);
		read_write_mqtt_server3_ip_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sport3")))
	{
		sys_psm.server3_port= ATOI(param, 10);
		read_write_mqtt_server3_port_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "mpop")))
	{
		memset(sys_psm.mqtt_publish_sensor_buf, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_publish_sensor_buf, param, str_size);
		read_write_mqtt_pub_sensor_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "msub")))
	{
		memset(sys_psm.mqtt_subscribe_command_buf, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_subscribe_command_buf, param, str_size);
		read_write_mqtt_sub_command_flash(WRITE_PSM);

		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "mpopa")))
	{
		memset(sys_psm.mqtt_publish_alarm_buf, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_publish_alarm_buf, param, str_size);
		
		read_write_mqtt_pub_alarm_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "mpopc")))
	{
		memset(sys_psm.mqtt_publish_ack_buf, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_publish_ack_buf, param, str_size);
		read_write_mqtt_sub_alarm_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "mpopb")))
	{
		memset(sys_psm.mqtt_subscribe_command_buf, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_subscribe_command_buf, param, str_size);
		read_write_mqtt_pub_back1_flash(WRITE_PSM);	
		ret = 1;
	}

	return ret;
}

uint8_t set_temp_humi_info(uint8_t * uri)
{
	uint8_t ret = 0;
	uint8_t * param;

	if((param = get_http_param_value((char *)uri, "fan1")))
	{
		sys_psm.high_temp_data= ATOI(param, 10);
		read_write_high_temp_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "fan2")))
	{
		sys_psm.low_temp_data= ATOI(param, 10);
		read_write_low_temp_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "fhot1")))
	{
		sys_psm.high_humi_data= ATOI(param, 10);
		read_write_high_humi_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "fhot2")))
	{
		sys_psm.low_humi_data= ATOI(param, 10);
		read_write_low_humi_flash(WRITE_PSM);
		ret = 1;
	}
	return ret;
}

uint8_t set_info(uint8_t * uri)
{
	uint8_t ret = 0;
	uint8_t * param;
	uint16_t str_size;
	int tmp=0;
	
	if((param = get_http_param_value((char *)uri, "use")))
	{
		memset(sys_psm.mqtt_user, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_user, param, str_size);
		read_write_mqtt_user_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "pass")))
	{
		memset(sys_psm.mqtt_pass, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_pass, param, str_size);
		read_write_mqtt_pass_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "time")))
	{
		sys_psm.mqtt_updata_time= ATOI(param, 10);
		log_i("mqtt_updata_time=%d",sys_psm.mqtt_updata_time);
		read_write_mqtt_report_timer_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "mip")))
	{
		inet_addr_((uint8_t*)param, sys_psm.mqtt_server_ip);
		read_write_mqtt_server_ip_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "mport")))
	{
		sys_psm.mqtt_server_port= ATOI(param, 10);
		read_write_mqtt_server_port_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sip1")))
	{
		inet_addr_((uint8_t*)param, sys_psm.server1_ip);
		read_write_mqtt_server1_ip_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sport1")))
	{
		sys_psm.server1_port= ATOI(param, 10);
		read_write_mqtt_server1_port_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sip2")))
	{
		inet_addr_((uint8_t*)param, sys_psm.server2_ip);
		read_write_mqtt_server2_ip_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sport2")))
	{
		sys_psm.server2_port= ATOI(param, 10);
		read_write_mqtt_server2_port_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sip3")))
	{
		inet_addr_((uint8_t*)param, sys_psm.server3_ip);
		read_write_mqtt_server3_ip_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sport3")))
	{
		sys_psm.server3_port= ATOI(param, 10);
		read_write_mqtt_server3_port_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "mpop")))
	{
		memset(sys_psm.mqtt_publish_sensor_buf, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_publish_sensor_buf, param, str_size);
		read_write_mqtt_pub_sensor_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "msub")))
	{
		memset(sys_psm.mqtt_subscribe_command_buf, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_subscribe_command_buf, param, str_size);
		read_write_mqtt_sub_command_flash(WRITE_PSM);

		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "mpopa")))
	{
		memset(sys_psm.mqtt_publish_alarm_buf, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_publish_alarm_buf, param, str_size);
		
		read_write_mqtt_pub_alarm_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "mpopc")))
	{
		memset(sys_psm.mqtt_publish_ack_buf, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_publish_ack_buf, param, str_size);
		read_write_mqtt_sub_alarm_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "mpopb")))
	{
		memset(sys_psm.mqtt_subscribe_command_buf, 0x00, MWM_VALUE_MED_SIZE);
		if((str_size = strlen((char*)param)) > MWM_VALUE_MED_SIZE-1)
		{
			str_size = MWM_VALUE_MED_SIZE; // exception handling
		}
		memcpy(sys_psm.mqtt_subscribe_command_buf, param, str_size);
		read_write_mqtt_pub_back1_flash(WRITE_PSM);	
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "fan1")))
	{
		sys_psm.high_temp_data= ATOI(param, 10);
		read_write_high_temp_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "fan2")))
	{
		sys_psm.low_temp_data= ATOI(param, 10);
		read_write_low_temp_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "fhot1")))
	{
		sys_psm.high_humi_data= ATOI(param, 10);
		read_write_high_humi_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "fhot2")))
	{
		sys_psm.low_humi_data= ATOI(param, 10);
		read_write_low_humi_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "ip")))
	{
		inet_addr_((uint8_t*)param, sys_psm.local.ip);
		read_write_local_ip_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "gw")))
	{
		inet_addr_((uint8_t*)param, sys_psm.local.gw);
		read_write_local_gateway_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "sub")))
	{
		inet_addr_((uint8_t*)param, sys_psm.local.sn);
		read_write_local_mask_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "dns")))
	{
		inet_addr_((uint8_t*)param, sys_psm.local.dns);
		read_write_local_dns_flash(WRITE_PSM);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "save")))
	{
		tmp= ATOI(param, 10);
		if(tmp==1)
		{
			NVIC_SystemReset();
		}
		ret = 1;
	}
	return ret;
}

/************************************************************************************************/
uint8_t predefined_set_cgi_processor(uint8_t * uri_name, uint8_t * uri, uint8_t * buf, uint16_t * len)
{
	uint8_t ret = 1;	// ret = '1' means 'uri_name' matched
	uint8_t val = 0;

	if(strcmp((const char *)uri_name, "set_netinfo.cgi") == 0)
	{
		val = set_netinfo(uri);
		*len = sprintf((char *)buf, "%d", val);
	}
	else if(strcmp((const char *)uri_name, "set_mqttinfo.cgi") == 0)
	{
		val = set_mqttinfo(uri);
		*len = sprintf((char *)buf, "%d", val);
	}
	else if(strcmp((const char *)uri_name, "set_thinfo.cgi") == 0)
	{
		val = set_temp_humi_info(uri);
		*len = sprintf((char *)buf, "%d", val);
	}
	else if(strcmp((const char *)uri_name, "set_info.cgi") == 0)
	{
		val = set_info(uri);
		*len = sprintf((char *)buf, "%d", val);	
	}
	else
	{
		ret = 0;
	}

	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t http_get_cgi_handler(uint8_t * uri_name, uint8_t * buf, uint32_t * file_len)
{
	uint8_t ret = HTTP_OK;
	uint16_t len = 0;

	if(predefined_get_cgi_processor(uri_name, buf, &len))
	{
		;
	}
	else
	{
		// CGI file not found
		ret = HTTP_FAILED;
	}

	if(ret)	*file_len = len;
	return ret;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t http_post_cgi_handler(uint8_t * uri_name, st_http_request * p_http_request, uint8_t * buf, uint32_t * file_len)
{
	uint8_t ret = HTTP_OK;
	uint16_t len = 0;
	uint8_t val = 0;
	uint8_t* newip;

	if(predefined_set_cgi_processor(uri_name, p_http_request->URI, buf, &len))
	{
		;
	}
	else
	{
		// CGI file not found
		ret = HTTP_FAILED;
	}

	if(ret)	*file_len = len;
	return ret;
}


