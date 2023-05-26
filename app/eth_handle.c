#include "eth_handle.h"
#include "sys_log.h"
#include "sys_os.h"

#include "w5500_dir.h"
#include "w5500_socket.h"
#include "w5500_dns.h"
#include "project_pin_use_config.h"
#include "project_psm_control.h"

//#include "MQTTConnect.h"
#include "MQTTPacket.h"
#include "eth_to_mqtt.h"
#include "eth_to_freertos.h"
#include "http_util.h"
//#include "eth_lock_to_tcp.h"
#include "eth_mqtt_handle.h"
#include "camera_handle.h"

#define HTTP_DATA_BUF_SIZE   2048

uint8_t http_rx_buf[HTTP_DATA_BUF_SIZE];
uint8_t http_tx_buf[HTTP_DATA_BUF_SIZE];
uint8_t socknumlist[] = {4, 5, 6, 7};


static os_thread_t eth_thread= NULL;
static os_thread_stack_define(eth_stack, 5*1024);

wiz_NetInfo net_parameter_data;



void w5500_network_init(void)
{
    uint8_t chipid[6];

    memcpy(net_parameter_data.ip, sys_psm.local.ip, 4);
    memcpy(net_parameter_data.sn, sys_psm.local.sn, 4);
    memcpy(net_parameter_data.gw, sys_psm.local.gw, 4);
    memcpy(net_parameter_data.mac,sys_psm.local.mac,6);
    memcpy(net_parameter_data.dns,sys_psm.local.dns,4);
    net_parameter_data.dhcp = sys_psm.local.dhcp; //< 1 - Static, 2 - DHCP
	
    ctlnetwork(CN_SET_NETINFO, (void*)&net_parameter_data);
	os_thread_sleep(os_msec_to_ticks(200));
    ctlnetwork(CN_GET_NETINFO, (void*)&net_parameter_data);
    // Display Network Information
    ctlwizchip(CW_GET_ID,(void*)chipid);
    log_i("=== %s NET CONF ===",(char*)chipid);
    log_i("MAC: %02X:%02X:%02X:%02X:%02X:%02X",net_parameter_data.mac[0],net_parameter_data.mac[1],net_parameter_data.mac[2],
    net_parameter_data.mac[3],net_parameter_data.mac[4],net_parameter_data.mac[5]);
    log_i("SIP: %d.%d.%d.%d", net_parameter_data.ip[0],net_parameter_data.ip[1],net_parameter_data.ip[2],net_parameter_data.ip[3]);
    log_i("GAR: %d.%d.%d.%d", net_parameter_data.gw[0],net_parameter_data.gw[1],net_parameter_data.gw[2],net_parameter_data.gw[3]);
    log_i("SUB: %d.%d.%d.%d", net_parameter_data.sn[0],net_parameter_data.sn[1],net_parameter_data.sn[2],net_parameter_data.sn[3]);
    log_i("DNS: %d.%d.%d.%d", net_parameter_data.dns[0],net_parameter_data.dns[1],net_parameter_data.dns[2],net_parameter_data.dns[3]);
    log_i("======================");
    wizchip_init(NULL, NULL);
}

void w5500_control_thread(void* param)
{
	uint8_t i;
	int32_t ret;
	uint8_t phy_check_time_cnt=0;
	uint8_t phy_check_err_cnt=0;
	uint8_t phy_check_err_flag=0;	
	
	LL_PERIPH_WE(PROJECT_PERIPH_WE);
	w5500_io_init();
	w5500_spi_init();
	LL_PERIPH_WP(PROJECT_PERIPH_WP);

	w5500_reset();
    w5500_register_function();
	
	httpServer_init(http_tx_buf, http_rx_buf, MAX_HTTPSOCK, socknumlist);
	reg_httpServer_cbfunc(NVIC_SystemReset, NULL); 
	loadwebpages();
    w5500_network_init();
    w5500_parameters_configuration();
	device_data.mqtt_start_flag=0;
	device_data.mqtt_connect_flag=0;

	while(1)
	{		
		if(device_data.phy_check_flag == PHY_LINK_ON)
		{
			for(i = 0; i < MAX_HTTPSOCK; i++)	
			{
				httpServer_run(i);	// HTTP server handler
			}	
			//////////////////////////////////////////////////////////////////////////
			if(phy_check_time_cnt>=5*2)
			{
				phy_check_time_cnt=0;
				if(ctlwizchip(CW_GET_PHYLINK, (void*)&device_data.phy_check_flag) == -1)/* PHY link status check */
				{
					log_i("Unknown PHY Link stauts");
				}
				//log_i("PHY Link stauts=%d",device_data.phy_check_flag);
			}
			////////////////////////////////////////////////////////////////////////
			camera_tcpc_init();
			///////////////////////////////////////////////////////////////		
			if(device_data.mqtt_start_flag==0)
			{
				phy_check_err_cnt=0;
				device_data.mqtt_start_flag=1;
				mqtt_process_init();
			}		
		}
		else
		{
			phy_check_time_cnt=0;
			if(ctlwizchip(CW_GET_PHYLINK, (void*)&device_data.phy_check_flag) == -1)/* PHY link status check */
			{
				log_i("Unknown PHY Link stauts");
			}
			//log_i("PHY Link stauts=%d",device_data.phy_check_flag);
			if(device_data.mqtt_start_flag==1)
			{
				phy_check_err_cnt++;
				if(phy_check_err_cnt>=6)
				{
					phy_check_err_cnt=0;
					device_data.mqtt_start_flag=2;
				}			
			}			
		}
		phy_check_time_cnt++;

		os_thread_sleep(os_msec_to_ticks(500));		
	}
	os_thread_delete(NULL);
}

void w5500_process_init(void)
{
	int ret=SYS_FAIL;
	ret = os_thread_create(&eth_thread, //任务控制块指针
							"eth_thread",//任务名字
							w5500_control_thread, //任务入口函数
							NULL,//任务入口函数参数
							&eth_stack,//任务栈大小
							OS_PRIO_11);	//任务的优先级							
   if(ret==SYS_FAIL)
   {
		log_e("w5500","thread create error!");
   }
}
