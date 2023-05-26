#include "project_psm_control.h"
#include "partition.h"
#include "psm_v2.h"
#include "sys_errno.h"
//#include "psm_utils.h"
#include "soft_md5.h"
#include "sys_os.h"
#include "psm_crc32.h"

static bool psm_valid;
static psm_hnd_t psm_hnd;

sys_psm_type sys_psm;


/*
*函数介绍：
*参数：无
*返回值：
*备注：无
*/
void psm_erase_control(void)
{
	int rv=SYS_FAIL;

	if (psm_valid)
	{
		rv = psm_format(psm_hnd);
	}
	if (rv != SYS_OK)
	{
		log_i("Could not erase rv\r\n");
	}
}
/*
*函数介绍：初始化psm接口
*参数：无
*返回值：成功返回SYS_OK，失败返回SYS_FAIL
*备注：无
*/
int psm_control_init(void)
{
	int ret =SYS_FAIL;
	flash_desc_t fl;
	struct partition_entry *p;
	psm_cfg_t psm_cfg;
	memset(&psm_cfg, 0, sizeof(psm_cfg_t));
	psm_cfg.secure = 0;
	
	if (psm_valid)
	{
		return SYS_OK;
	}
	ret = flash_drv_init();
	if (ret != SYS_OK)
	{
		return SYS_FAIL;
	}	
	ret = part_init();
	if (ret != SYS_OK)
	{
		return SYS_FAIL;
	}
	
	p = part_get_layout_by_id(FC_COMP_PSM, NULL);
	if (p == NULL)
	{
		return SYS_FAIL;
	}
	part_to_flash_desc(p, &fl);
	ret = psm_module_init(&fl, &psm_hnd, &psm_cfg);

	if (ret == SYS_E_PERM)
	{
		//dbg("Initializing psm partition as Read-Only");
		psm_cfg.read_only = true;
		ret = psm_module_init(&fl, &psm_hnd,&psm_cfg);
	}

	if (ret == SYS_OK)
	{
		psm_valid = true;
	}
	return ret;
}

/*
*函数介绍：关闭psm接口
*参数：无
*返回值：成功返回SYS_OK，失败返回SYS_FAIL
*备注：无
*/
int psm_control_cleanup(void)
{
	if (!psm_valid)
	{
		return SYS_OK;
	}
	psm_valid = false;
	return psm_module_deinit(&psm_hnd);
}
////////////////////////////////////////////////////////////////////////////////////////////////
/*
*函数介绍：获取psm中字符数据
*参数：无
*返回值：成功返回获取到字节总数数，失败返回SYS_FAIL
*备注：无
*/
static int psm_get_val(const char *mod_name, const char *var_name, char *value, int max_len)
{
	char variable[FULL_VAR_NAME_SIZE];

	snprintf(variable, FULL_VAR_NAME_SIZE, "%s.%s", mod_name, var_name);
	if (psm_valid)
	{
		return psm_get_variable_str(psm_hnd, variable, value, max_len);
	}
	return SYS_FAIL;
}

/*
*函数介绍：获取psm中字符数据
*参数：无
*返回值：成功返回SYS_OK，失败返回SYS_FAIL
*备注：无
*/
int get_psm_conf(const char *mod, const char *var, char *val, const int len)
{
    int ret;

	if ((ret =psm_get_val(mod, var, val, len)) <= 0)
	{
		return SYS_FAIL;
	}
	return SYS_OK;
}

/*
*函数介绍：设置psm中字符数据
*参数：无
*返回值：成功返回获取到字节总数数，失败返回SYS_FAIL
*备注：无
*/
static int psm_set_val(const char *mod_name, const char *var_name, const char *value)
{
	char variable[FULL_VAR_NAME_SIZE];

	snprintf(variable, FULL_VAR_NAME_SIZE, "%s.%s", mod_name, var_name);
	if (psm_valid)
	{
		return psm_set_variable_str(psm_hnd, variable, value);
	}
	return SYS_FAIL;
}

/*
*函数介绍：设置psm中字符数据
*参数：无
*返回值：成功返回SYS_OK，失败返回SYS_FAIL
*备注：无
*/
int set_psm_conf(const char *mod, const char *var, const char *val)
{
	int ret;

	ret = psm_set_val(mod, var, val);
	if (ret != SYS_OK)
	{
		return ret;
	}
	return SYS_OK;
}

/*
*函数介绍：初始化psm中字符数据
*参数：无
*返回值：成功返回SYS_OK，失败返回SYS_FAIL
*备注：无
*/
void add_psm_entry_str(const char *module, const char *variable, const char *default_val)
{
	char value[MWM_VAL_SIZE];
	int ret;

	/* Check if variable already exists in psm */
	ret = get_psm_conf(module, variable, value, sizeof(value));
	if (ret != SYS_OK)
	{
		/* Write default config value to psm */
		set_psm_conf(module, variable, default_val);
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////
/*
*函数介绍：获取psm中整型数据
*参数：无
*返回值：成功返回获取到字节总数数，失败返回SYS_FAIL
*备注：无
*/
static int psm_get_int(const char *mod_name, const char *var_name, int *value)
{
	char variable[FULL_VAR_NAME_SIZE];

	snprintf(variable, FULL_VAR_NAME_SIZE, "%s.%s", mod_name, var_name);
	if (psm_valid)
	{
		return psm_get_variable_int(psm_hnd, variable, value);
	}
	return SYS_FAIL;
}

/*
*函数介绍：获取psm中整型数据
*参数：无
*返回值：成功返回获取到字节总数数，失败返回SYS_FAIL
*备注：无
*/
int get_psm_conf_int(const char *mod, const char *var, int *val)
{
    int ret;

    ret =psm_get_int(mod, var, val);

	return ret;
}

/*
*函数介绍：设置psm中整型数据
*参数：无
*返回值：成功返回获取到字节总数数，失败返回SYS_FAIL
*备注：无
*/
static int psm_set_int(const char *mod_name, const char *var_name, int value)
{
	char variable[FULL_VAR_NAME_SIZE];

	snprintf(variable, FULL_VAR_NAME_SIZE, "%s.%s", mod_name, var_name);
	if (psm_valid)
	{
		return psm_set_variable_int(psm_hnd, variable, value);
	}
	return SYS_FAIL;
}

/*
*函数介绍：设置psm中整型数据
*参数：无
*返回值：成功返回SYS_OK，失败返回SYS_FAIL
*备注：无
*/
int set_psm_conf_int(const char *mod, const char *var, int value)
{
	int ret;

	ret = psm_set_int(mod, var, value);
	if (ret != SYS_OK)
	{
		return ret;
	}
	return SYS_OK;
}

/*
*函数介绍：初始化psm整型数据
*参数：无
*返回值：无
*备注：无
*/
void add_psm_entry_int(const char *module, const char *variable, int default_val)
{
	int value;
	int ret;

	/* Check if variable already exists in psm */
	ret = get_psm_conf_int(module, variable, &value);
	if (ret != SYS_OK)
	{
		/* Write default config value to psm */
		log_i("set_psm_conf_int");
		set_psm_conf_int(module, variable, default_val);
	}
}

void write_device_name_conf(void)
{
	stc_efm_unique_id_t chip_uuid;
	char encrypt[64];
	utils_md5_context_t *ctx = os_mem_calloc(sizeof(utils_md5_context_t));
	uint8_t decrypt[16];
	char tmp_uid[32+1];
	char device_name[32+1];


	EFM_GetUID(&chip_uuid);
	//log_i("chip_uuid: %08x-%08x-%08x",chip_uuid.u32UniqueID0, chip_uuid.u32UniqueID1,chip_uuid.u32UniqueID2);
	sprintf(encrypt,"kiloamp:%08x%08x%08x",chip_uuid.u32UniqueID0, chip_uuid.u32UniqueID1,chip_uuid.u32UniqueID2);
 
    utils_md5_init(ctx);
    utils_md5_starts(ctx);
	utils_md5_update(ctx, (const unsigned char *)encrypt, strlen(encrypt));
	utils_md5_finish(ctx,decrypt);
	
	sprintf (tmp_uid, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			decrypt[0],decrypt[1],decrypt[2],
			decrypt[3],decrypt[4],decrypt[5],
			decrypt[6],decrypt[7],decrypt[8],
			decrypt[9],decrypt[10],decrypt[11],
			decrypt[12],decrypt[13],decrypt[14],decrypt[15]);
	//log_i("md5 : %s", tmp_uid);		
	
	sprintf (device_name, "SGATE%02X%02X%02X%02X%02X%02X%02X%02X",
			decrypt[4],decrypt[5],decrypt[6],
			decrypt[7],decrypt[8],decrypt[9],
			decrypt[10],decrypt[11]);
	//log_i("device_name : %s", device_name);			
	utils_md5_free(ctx);
	os_mem_free(ctx);
	
	add_psm_entry_str(DEVICE_SYSTEM, DEVICE_SYSTEM_DEVICE_ID, device_name);
}

void write_eth_mac_conf(void)
{
	stc_efm_unique_id_t chip_uuid;
	uint8_t buf_data[12];
	uint32_t crc_buf;
	uint8_t mac[6];
	char psm_val[MWM_VALUE_MED_SIZE];
	
	EFM_GetUID(&chip_uuid);

	buf_data[0] = (uint8_t)chip_uuid.u32UniqueID0;
	buf_data[1] = (uint8_t)(chip_uuid.u32UniqueID0 >> 8);
	buf_data[2] = (uint8_t)(chip_uuid.u32UniqueID0 >> 16);
	buf_data[3] = (uint8_t)(chip_uuid.u32UniqueID0 >> 24);
	buf_data[4] = (uint8_t)chip_uuid.u32UniqueID1;
	buf_data[5] = (uint8_t)(chip_uuid.u32UniqueID1 >> 8);
	buf_data[6] = (uint8_t)(chip_uuid.u32UniqueID1 >> 16);
	buf_data[7] = (uint8_t)(chip_uuid.u32UniqueID1 >> 24);
	buf_data[8] = (uint8_t)chip_uuid.u32UniqueID2;
	buf_data[9] = (uint8_t)(chip_uuid.u32UniqueID2 >> 8);
	buf_data[10] = (uint8_t)(chip_uuid.u32UniqueID2 >> 16);
	buf_data[11] = (uint8_t)(chip_uuid.u32UniqueID2 >> 24);
	crc_buf=soft_crc32(&buf_data[0],12, 0);
	
	mac[0]=0x04;
	mac[1]=0xa0;
	mac[2] = (uint8_t)crc_buf;
	mac[3] = (uint8_t)(crc_buf>> 8);
	mac[4] = (uint8_t)(crc_buf >> 16);
	mac[5] = (uint8_t)(crc_buf >> 24);

	snprintf(psm_val, sizeof(psm_val), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);		
	//log_i("mac : %s", psm_val);
	add_psm_entry_str(ETH_SYSTEM, ETH_SYSTEM_MAC,psm_val);
}


int sys_sscanf(const char *str, const char *format, ...)
{
	va_list ap;
	int rv;

	va_start(ap, format);
	rv = vsscanf(str, format, ap);
	va_end(ap);

	return rv;
}

void write_slave_conf(uint8_t num)
{
	if(num>SLAVE_MAX_NUM)
	{
		return;
	}
	char psm_val[MWM_VALUE_MED_SIZE];
	char tmp_addr[MWM_VALUE_MED_SIZE];
	char tmp_type[MWM_VALUE_MED_SIZE];
	char tmp_name[MWM_VALUE_MED_SIZE];
	int buf;
	
	snprintf(tmp_addr, sizeof(tmp_addr), "%s%d",SLAVE_SYSTEM_SLAVE_ADDR,num);
	snprintf(tmp_type, sizeof(tmp_type), "%s%d",SLAVE_SYSTEM_SLAVE_TYPE,num);
	snprintf(tmp_name, sizeof(tmp_name), "%s%d",SLAVE_SYSTEM_SLAVE_NAME,num);
	
	add_psm_entry_int(SLAVE_SYSTEM, tmp_addr, DAT_SLAVE_ADDR);
	add_psm_entry_int(SLAVE_SYSTEM, tmp_type, DAT_SLAVE_TYPE);
	add_psm_entry_str(SLAVE_SYSTEM, tmp_name,DAT_SLAVE_NAME);
}

/*
*函数介绍：写默认信息
*参数：无
*返回值：无
*备注：无
*/
void conf_default_psm(void)
{	
	int i;
	/////////////////////////////////////////////////////////////////////////////
	write_device_name_conf();
	add_psm_entry_int(DEVICE_SYSTEM, DEVICE_SYSTEM_HIGH_TEMP, DAT_H_TEMP);
	add_psm_entry_int(DEVICE_SYSTEM, DEVICE_SYSTEM_LOW_TEMP, DAT_L_TEMP);
	add_psm_entry_int(DEVICE_SYSTEM, DEVICE_SYSTEM_HIGH_HUMI, DAT_H_HUMI);	
	add_psm_entry_int(DEVICE_SYSTEM, DEVICE_SYSTEM_LOW_HUMI, DAT_L_HUMI);		
	add_psm_entry_int(DEVICE_SYSTEM, DEVICE_SYSTEM_RS485_ID, DAT_RS485_ID);
	add_psm_entry_int(DEVICE_SYSTEM, DEVICE_SYSTEM_QUERY_TIMER, DAT_QUERY_TIMER);	
	/////////////////////////////////////////////////////////////////////////////
	//add_psm_entry(ETH_SYSTEM, ETH_SYSTEM_MAC,DEF_NETWORK_SSID);
	write_eth_mac_conf();
	add_psm_entry_str(ETH_SYSTEM, ETH_SYSTEM_IP,DAT_ETH_IP);
	add_psm_entry_str(ETH_SYSTEM, ETH_SYSTEM_IP_MASK,DAT_ETH_SN);
	add_psm_entry_str(ETH_SYSTEM, ETH_SYSTEM_GATEWAY,DAT_ETH_GW);
	add_psm_entry_str(ETH_SYSTEM, ETH_SYSTEM_DNS,DAT_ETH_DNS);
	add_psm_entry_int(ETH_SYSTEM, ETH_SYSTEM_IP_PORT, DAT_ETH_PORT);
	add_psm_entry_int(ETH_SYSTEM, ETH_SYSTEM_DHCP, DAT_ETH_DHCP);
	/////////////////////////////////////////////////////////////////////////////
	add_psm_entry_str(MQTT_SYSTEM, MQTT_SYSTEM_USER,DAT_MQTT_USER);
	add_psm_entry_str(MQTT_SYSTEM, MQTT_SYSTEM_PASS,DAT_MQTT_PASS);	
	add_psm_entry_str(MQTT_SYSTEM, MQTT_SYSTEM_IP,DAT_MQTT_IP);
	add_psm_entry_int(MQTT_SYSTEM, MQTT_SYSTEM_PORT, DAT_MQTT_PORT);
	
	add_psm_entry_str(MQTT_SYSTEM, MQTT_SYSTEM_SERVER1_IP ,DAT_MQTT_SERVER1_IP);
	add_psm_entry_int(MQTT_SYSTEM, MQTT_SYSTEM_SERVER1_PORT, DAT_MQTT_SERVER1_PORT);
	
	add_psm_entry_str(MQTT_SYSTEM, MQTT_SYSTEM_SERVER2_IP,DAT_MQTT_SERVER2_IP);
	add_psm_entry_int(MQTT_SYSTEM, MQTT_SYSTEM_SERVER2_PORT, DAT_MQTT_SERVER2_PORT);
	
	add_psm_entry_str(MQTT_SYSTEM, MQTT_SYSTEM_SERVER3_IP,DAT_MQTT_SERVER3_IP);
	add_psm_entry_int(MQTT_SYSTEM, MQTT_SYSTEM_SERVER3_PORT, DAT_MQTT_SERVER3_PORT);
	
	add_psm_entry_int(MQTT_SYSTEM, MQTT_SYSTEM_REPORT_TIMER, DAT_MQTT_UPDATA_TIMER);
	/////////////////////////////////////////////////////////////////////////////		
	add_psm_entry_str(MQTT_SYSTEM, MQTT_SYSTEM_PUB_SENSOR,DAT_MQTT_PUBLISH_SENSOR);
	add_psm_entry_str(MQTT_SYSTEM, MQTT_SYSTEM_PUB_ALARM,DAT_MQTT_PUBLISH_ALARM);
	add_psm_entry_str(MQTT_SYSTEM, MQTT_SYSTEM_PUB_ACK,DAT_MQTT_PUBLISH_ACK);	
	add_psm_entry_str(MQTT_SYSTEM, MQTT_SYSTEM_PUB_BACK1,DAT_MQTT_PUBLISH_BACK1);
	add_psm_entry_str(MQTT_SYSTEM, MQTT_SYSTEM_SUB_COMMAND,DAT_MQTT_SUBSCRIBE_COMMAND);
	/////////////////////////////////////////////////////////////////////////////
	add_psm_entry_int(SLAVE_SYSTEM, SLAVE_SYSTEM_SLAVE_OK, DAT_SLAVE_OK);
	for (i = 0; i < SLAVE_MAX_NUM; i++)
	{
		write_slave_conf(i);
	}	
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_high_temp_flash(uint8_t flag)
{
	int buf;

	if(flag==READ_PSM)
	{
		get_psm_conf_int(DEVICE_SYSTEM,DEVICE_SYSTEM_HIGH_TEMP, &buf);
		sys_psm.high_temp_data=(uint8_t)buf;
		log_i("high_temp=%d",sys_psm.high_temp_data);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.high_temp_data;
		set_psm_conf_int(DEVICE_SYSTEM,DEVICE_SYSTEM_HIGH_TEMP, buf);
	}	
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_low_temp_flash(uint8_t flag)
{
	int buf;

	if(flag==READ_PSM)
	{
		get_psm_conf_int(DEVICE_SYSTEM,DEVICE_SYSTEM_LOW_TEMP, &buf);
		sys_psm.low_temp_data=(uint8_t)buf;
		log_i("low_temp=%d",sys_psm.low_temp_data);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.low_temp_data;
		set_psm_conf_int(DEVICE_SYSTEM,DEVICE_SYSTEM_LOW_TEMP, buf);
	}		
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_high_humi_flash(uint8_t flag)
{
	int buf;

	if(flag==READ_PSM)
	{
		get_psm_conf_int(DEVICE_SYSTEM,DEVICE_SYSTEM_HIGH_HUMI, &buf);
		sys_psm.high_humi_data=(uint8_t)buf;
		log_i("high_humi=%d",sys_psm.high_humi_data);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.high_humi_data;
		set_psm_conf_int(DEVICE_SYSTEM,DEVICE_SYSTEM_HIGH_HUMI, buf);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_low_humi_flash(uint8_t flag)
{
	int buf;

	if(flag==READ_PSM)
	{
		get_psm_conf_int(DEVICE_SYSTEM,DEVICE_SYSTEM_LOW_HUMI, &buf);
		sys_psm.low_humi_data=(uint8_t)buf;
		log_i("low_humi=%d",sys_psm.low_humi_data);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.low_humi_data;
		set_psm_conf_int(DEVICE_SYSTEM,DEVICE_SYSTEM_LOW_HUMI, buf);
	}
}


/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_device_id_flash(uint8_t flag)
{
	if(flag==READ_PSM)
	{
		get_psm_conf(DEVICE_SYSTEM, DEVICE_SYSTEM_DEVICE_ID,sys_psm.device_id, sizeof(sys_psm.device_id));
		log_i("device_id:%s",sys_psm.device_id);
	}
	else if(flag==WRITE_PSM)
	{
		set_psm_conf(DEVICE_SYSTEM, DEVICE_SYSTEM_DEVICE_ID,sys_psm.device_id);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_rs485_id_flash(uint8_t flag)
{
	int buf;

	if(flag==READ_PSM)
	{
		get_psm_conf_int(DEVICE_SYSTEM,DEVICE_SYSTEM_RS485_ID, &buf);
		sys_psm.rs485_id=(uint8_t)buf;
		log_i("rs485_id=%d",sys_psm.rs485_id);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.rs485_id;
		set_psm_conf_int(DEVICE_SYSTEM,DEVICE_SYSTEM_RS485_ID, buf);
	}	
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_query_time_flash(uint8_t flag)
{
	int buf;

	if(flag==READ_PSM)
	{
		get_psm_conf_int(DEVICE_SYSTEM,DEVICE_SYSTEM_QUERY_TIMER, &buf);
		sys_psm.query_time=(uint8_t)buf;
		log_i("query_time=%d",sys_psm.query_time);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.query_time;
		set_psm_conf_int(DEVICE_SYSTEM,DEVICE_SYSTEM_QUERY_TIMER, buf);
	}	
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_local_ip_flash(uint8_t flag)
{
	char psm_val[MWM_VALUE_MED_SIZE];
	if(flag==READ_PSM)
	{
		get_psm_conf(ETH_SYSTEM, ETH_SYSTEM_IP,psm_val, sizeof(psm_val));
		
		sys_sscanf(psm_val, "%d.%d.%d.%d", &sys_psm.local.ip[0],&sys_psm.local.ip[1],&sys_psm.local.ip[2],&sys_psm.local.ip[3]);
		log_i("local_ip=%d.%d.%d.%d",sys_psm.local.ip[0],sys_psm.local.ip[1],sys_psm.local.ip[2],sys_psm.local.ip[3]);
	}
	else if(flag==WRITE_PSM)
	{
		snprintf(psm_val, sizeof(psm_val), "%d.%d.%d.%d",sys_psm.local.ip[0],sys_psm.local.ip[1],sys_psm.local.ip[2],sys_psm.local.ip[3]);	
		set_psm_conf(ETH_SYSTEM, ETH_SYSTEM_IP,psm_val);
	}
	else if(flag==WRITE_PSM_DEFULT)
	{	
		set_psm_conf(ETH_SYSTEM, ETH_SYSTEM_IP,DAT_ETH_IP);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_local_mask_flash(uint8_t flag)
{
	char psm_val[MWM_VALUE_MED_SIZE];
	if(flag==READ_PSM)
	{
		get_psm_conf(ETH_SYSTEM, ETH_SYSTEM_IP_MASK,psm_val, sizeof(psm_val));
		
		sys_sscanf(psm_val, "%d.%d.%d.%d", &sys_psm.local.sn[0],&sys_psm.local.sn[1],&sys_psm.local.sn[2],&sys_psm.local.sn[3]);
		log_i("local_mask=%d.%d.%d.%d",sys_psm.local.sn[0],sys_psm.local.sn[1],sys_psm.local.sn[2],sys_psm.local.sn[3]);
	}
	else if(flag==WRITE_PSM)
	{
		snprintf(psm_val, sizeof(psm_val), "%d.%d.%d.%d",sys_psm.local.sn[0],sys_psm.local.sn[1],sys_psm.local.sn[2],sys_psm.local.sn[3]);	
		set_psm_conf(ETH_SYSTEM, ETH_SYSTEM_IP_MASK,psm_val);
	}
	else if(flag==WRITE_PSM_DEFULT)
	{	
		set_psm_conf(ETH_SYSTEM, ETH_SYSTEM_IP_MASK,DAT_ETH_SN);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_local_gateway_flash(uint8_t flag)
{
	char psm_val[MWM_VALUE_MED_SIZE];
	if(flag==READ_PSM)
	{
		get_psm_conf(ETH_SYSTEM, ETH_SYSTEM_GATEWAY,psm_val, sizeof(psm_val));
		
		sys_sscanf(psm_val, "%d.%d.%d.%d", &sys_psm.local.gw[0],&sys_psm.local.gw[1],&sys_psm.local.gw[2],&sys_psm.local.gw[3]);
		log_i("local_gateway=%d.%d.%d.%d",sys_psm.local.gw[0],sys_psm.local.gw[1],sys_psm.local.gw[2],sys_psm.local.gw[3]);
	}
	else if(flag==WRITE_PSM)
	{
		snprintf(psm_val, sizeof(psm_val), "%d.%d.%d.%d",sys_psm.local.gw[0],sys_psm.local.gw[1],sys_psm.local.gw[2],sys_psm.local.gw[3]);	
		set_psm_conf(ETH_SYSTEM, ETH_SYSTEM_GATEWAY,psm_val);
	}
	else if(flag==WRITE_PSM_DEFULT)
	{	
		set_psm_conf(ETH_SYSTEM, ETH_SYSTEM_GATEWAY,DAT_ETH_GW);
	}	
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_local_dns_flash(uint8_t flag)
{
	char psm_val[MWM_VALUE_MED_SIZE];
	if(flag==READ_PSM)
	{
		get_psm_conf(ETH_SYSTEM, ETH_SYSTEM_DNS,psm_val, sizeof(psm_val));
		
		sys_sscanf(psm_val, "%d.%d.%d.%d", &sys_psm.local.dns[0],&sys_psm.local.dns[1],&sys_psm.local.dns[2],&sys_psm.local.dns[3]);
		log_i("local_dns=%d.%d.%d.%d",sys_psm.local.dns[0],sys_psm.local.dns[1],sys_psm.local.dns[2],sys_psm.local.dns[3]);
	}
	else if(flag==WRITE_PSM)
	{
		snprintf(psm_val, sizeof(psm_val), "%d.%d.%d.%d",sys_psm.local.dns[0],sys_psm.local.dns[1],sys_psm.local.dns[2],sys_psm.local.dns[3]);	
		set_psm_conf(ETH_SYSTEM, ETH_SYSTEM_DNS,psm_val);
	}
	else if(flag==WRITE_PSM_DEFULT)
	{	
		set_psm_conf(ETH_SYSTEM, ETH_SYSTEM_DNS,DAT_ETH_DNS);
	}	
}

/* Parse string 'arg' formatted "AA:BB:CC:DD:EE:FF" (assuming 'sep' is ':')
 * into a 6-byte array 'dest' such that dest = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF} 
 * set 'sep' accordingly. */
int get_mac(const char *arg, char *dest, char sep)
{
	unsigned char n;
	int i, j, k;

	if (strlen(arg) < 17)
		return 1;

	memset(dest, 0, 6);

	for (i = 0, k = 0; i < 17; i += 3, k++) 
	{
		for (j = 0; j < 2; j++) 
		{
			if (arg[i + j] >= '0' && arg[i + j] <= '9')
				n = arg[i + j] - '0';
			else if (arg[i + j] >= 'A' && arg[i + j] <= 'F')
				n = arg[i + j] - 'A' + 10;
			else if (arg[i + j] >= 'a' && arg[i + j] <= 'f')
				n = arg[i + j] - 'a' + 10;
			else
				return 1;

			n <<= 4 * (1 - j);
			dest[k] += n;
		}
		if (i < 15 && arg[i + 2] != sep)
			return 1;
	}

	return 0;
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_local_mac_flash(uint8_t flag)
{
	char psm_val[MWM_VALUE_MED_SIZE];
	
	if(flag==READ_PSM)
	{
		get_psm_conf(ETH_SYSTEM, ETH_SYSTEM_MAC,psm_val, sizeof(psm_val));
		get_mac(psm_val, (char *)sys_psm.local.mac, ':');
		
		log_i("local_mac=%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",sys_psm.local.mac[0],sys_psm.local.mac[1],
			sys_psm.local.mac[2],sys_psm.local.mac[3],sys_psm.local.mac[4],sys_psm.local.mac[5]);
	}
	else if(flag==WRITE_PSM)
	{
		snprintf(psm_val, sizeof(psm_val), "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",sys_psm.local.mac[0],
				sys_psm.local.mac[1],sys_psm.local.mac[2],sys_psm.local.mac[3],sys_psm.local.mac[4],sys_psm.local.mac[5]);	
		set_psm_conf(ETH_SYSTEM, ETH_SYSTEM_MAC,psm_val);
	}	
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_local_port_flash(uint8_t flag)
{
	int buf;
	if(flag==READ_PSM)
	{
		get_psm_conf_int(ETH_SYSTEM,ETH_SYSTEM_IP_PORT, &buf);
		sys_psm.local_port=(uint16_t)buf;
		log_i("local_port=%d",sys_psm.local_port);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.local_port;
		set_psm_conf_int(ETH_SYSTEM,ETH_SYSTEM_IP_PORT, buf);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_local_dhcp_flash(uint8_t flag)
{
	int buf;
	if(flag==READ_PSM)
	{
		get_psm_conf_int(ETH_SYSTEM,ETH_SYSTEM_DHCP, &buf);
		sys_psm.local.dhcp=(uint8_t)buf;
		log_i("local_dhcp=%d",sys_psm.local.dhcp);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.local.dhcp;
		set_psm_conf_int(ETH_SYSTEM,ETH_SYSTEM_DHCP, buf);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_user_flash(uint8_t flag)
{
	if(flag==READ_PSM)
	{
		get_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_USER,sys_psm.mqtt_user, sizeof(sys_psm.mqtt_user));
		log_i("mqtt_user:%s",sys_psm.mqtt_user);
	}
	else if(flag==WRITE_PSM)
	{

		set_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_USER,sys_psm.mqtt_user);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_pass_flash(uint8_t flag)
{
	if(flag==READ_PSM)
	{
		get_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_PASS,sys_psm.mqtt_pass, sizeof(sys_psm.mqtt_pass));
		log_i("mqtt_pass:%s",sys_psm.mqtt_pass);
	}
	else if(flag==WRITE_PSM)
	{

		set_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_PASS,sys_psm.mqtt_pass);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_server_ip_flash(uint8_t flag)
{
	char psm_val[MWM_VALUE_MED_SIZE];
	if(flag==READ_PSM)
	{
		get_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_IP,psm_val, sizeof(psm_val));
		
		sys_sscanf(psm_val, "%d.%d.%d.%d", &sys_psm.mqtt_server_ip[0],&sys_psm.mqtt_server_ip[1],&sys_psm.mqtt_server_ip[2],&sys_psm.mqtt_server_ip[3]);
		log_i("mqtt_server_ip=%d.%d.%d.%d",sys_psm.mqtt_server_ip[0],sys_psm.mqtt_server_ip[1],sys_psm.mqtt_server_ip[2],sys_psm.mqtt_server_ip[3]);
	}
	else if(flag==WRITE_PSM)
	{
		snprintf(psm_val, sizeof(psm_val), "%d.%d.%d.%d",sys_psm.mqtt_server_ip[0],sys_psm.mqtt_server_ip[1],sys_psm.mqtt_server_ip[2],sys_psm.mqtt_server_ip[3]);	
		set_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_IP,psm_val);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_server_port_flash(uint8_t flag)
{
	int buf;
	if(flag==READ_PSM)
	{
		get_psm_conf_int(MQTT_SYSTEM,MQTT_SYSTEM_PORT, &buf);
		sys_psm.mqtt_server_port=(uint16_t)buf;
		log_i("mqtt_server_port=%d",sys_psm.mqtt_server_port);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.mqtt_server_port;
		set_psm_conf_int(MQTT_SYSTEM,MQTT_SYSTEM_PORT, buf);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_server1_ip_flash(uint8_t flag)
{
	char psm_val[MWM_VALUE_MED_SIZE];
	if(flag==READ_PSM)
	{
		get_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_SERVER1_IP,psm_val, sizeof(psm_val));
		
		sys_sscanf(psm_val, "%d.%d.%d.%d", &sys_psm.server1_ip[0],&sys_psm.server1_ip[1],&sys_psm.server1_ip[2],&sys_psm.server1_ip[3]);
		log_i("mqtt_server1_ip=%d.%d.%d.%d",sys_psm.server1_ip[0],sys_psm.server1_ip[1],sys_psm.server1_ip[2],sys_psm.server1_ip[3]);
	}
	else if(flag==WRITE_PSM)
	{
		snprintf(psm_val, sizeof(psm_val), "%d.%d.%d.%d",sys_psm.server1_ip[0],sys_psm.server1_ip[1],sys_psm.server1_ip[2],sys_psm.server1_ip[3]);	
		set_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_SERVER1_IP,psm_val);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_server1_port_flash(uint8_t flag)
{
	int buf;
	if(flag==READ_PSM)
	{
		get_psm_conf_int(MQTT_SYSTEM,MQTT_SYSTEM_SERVER1_PORT, &buf);
		sys_psm.server1_port=(uint16_t)buf;
		log_i("mqtt_server1_port=%d",sys_psm.server1_port);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.server1_port;
		set_psm_conf_int(MQTT_SYSTEM,MQTT_SYSTEM_SERVER1_PORT, buf);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_server2_ip_flash(uint8_t flag)
{
	char psm_val[MWM_VALUE_MED_SIZE];
	if(flag==READ_PSM)
	{
		get_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_SERVER2_IP,psm_val, sizeof(psm_val));
		
		sys_sscanf(psm_val, "%d.%d.%d.%d", &sys_psm.server2_ip[0],&sys_psm.server2_ip[1],&sys_psm.server2_ip[2],&sys_psm.server2_ip[3]);
		log_i("mqtt_server2_ip=%d.%d.%d.%d",sys_psm.server2_ip[0],sys_psm.server2_ip[1],sys_psm.server2_ip[2],sys_psm.server2_ip[3]);
	}
	else if(flag==WRITE_PSM)
	{
		snprintf(psm_val, sizeof(psm_val), "%d.%d.%d.%d",sys_psm.server2_ip[0],sys_psm.server2_ip[1],sys_psm.server2_ip[2],sys_psm.server2_ip[3]);	
		set_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_SERVER2_IP,psm_val);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_server2_port_flash(uint8_t flag)
{
	int buf;
	if(flag==READ_PSM)
	{
		get_psm_conf_int(MQTT_SYSTEM,MQTT_SYSTEM_SERVER2_PORT, &buf);
		sys_psm.server2_port=(uint16_t)buf;
		log_i("mqtt_server2_port=%d",sys_psm.server2_port);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.server2_port;
		set_psm_conf_int(MQTT_SYSTEM,MQTT_SYSTEM_SERVER2_PORT, buf);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_server3_ip_flash(uint8_t flag)
{
	char psm_val[MWM_VALUE_MED_SIZE];
	if(flag==READ_PSM)
	{
		get_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_SERVER3_IP,psm_val, sizeof(psm_val));
		
		sys_sscanf(psm_val, "%d.%d.%d.%d", &sys_psm.server3_ip[0],&sys_psm.server3_ip[1],&sys_psm.server3_ip[2],&sys_psm.server3_ip[3]);
		log_i("mqtt_server3_ip=%d.%d.%d.%d",sys_psm.server3_ip[0],sys_psm.server3_ip[1],sys_psm.server3_ip[2],sys_psm.server3_ip[3]);
	}
	else if(flag==WRITE_PSM)
	{
		snprintf(psm_val, sizeof(psm_val), "%d.%d.%d.%d",sys_psm.server3_ip[0],sys_psm.server3_ip[1],sys_psm.server3_ip[2],sys_psm.server3_ip[3]);	
		set_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_SERVER3_IP,psm_val);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_server3_port_flash(uint8_t flag)
{
	int buf;
	if(flag==READ_PSM)
	{
		get_psm_conf_int(MQTT_SYSTEM,MQTT_SYSTEM_SERVER3_PORT, &buf);
		sys_psm.server3_port=(uint16_t)buf;
		log_i("mqtt_server3_port=%d",sys_psm.server3_port);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.server3_port;
		set_psm_conf_int(MQTT_SYSTEM,MQTT_SYSTEM_SERVER3_PORT, buf);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_report_timer_flash(uint8_t flag)
{
	int buf;

	if(flag==READ_PSM)
	{
		get_psm_conf_int(MQTT_SYSTEM,MQTT_SYSTEM_REPORT_TIMER, &buf);
		sys_psm.mqtt_updata_time=(uint8_t)buf;
		log_i("mqtt_updata_time=%d",sys_psm.mqtt_updata_time);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.mqtt_updata_time;
		set_psm_conf_int(MQTT_SYSTEM,MQTT_SYSTEM_REPORT_TIMER, buf);
		os_thread_sleep(os_msec_to_ticks(10));
		
		get_psm_conf_int(MQTT_SYSTEM,MQTT_SYSTEM_REPORT_TIMER, &buf);
		sys_psm.mqtt_updata_time=(uint8_t)buf;
		log_i("mqtt_updata_time=%d",sys_psm.mqtt_updata_time);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_pub_sensor_flash(uint8_t flag)
{
	
	if(flag==READ_PSM)
	{
		get_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_PUB_SENSOR,sys_psm.mqtt_publish_sensor_buf, sizeof(sys_psm.mqtt_publish_sensor_buf));
		log_i("mqtt_publish_sensor_buf:%s",sys_psm.mqtt_publish_sensor_buf);
	}
	else if(flag==WRITE_PSM)
	{

		set_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_PUB_SENSOR,sys_psm.mqtt_publish_sensor_buf);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_pub_alarm_flash(uint8_t flag)
{
	if(flag==READ_PSM)
	{
		get_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_PUB_ALARM,sys_psm.mqtt_publish_alarm_buf, sizeof(sys_psm.mqtt_publish_alarm_buf));
		log_i("mqtt_publish_alarm_buf:%s",sys_psm.mqtt_publish_alarm_buf);
	}
	else if(flag==WRITE_PSM)
	{

		set_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_PUB_ALARM,sys_psm.mqtt_publish_alarm_buf);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_sub_command_flash(uint8_t flag)
{
	if(flag==READ_PSM)
	{
		get_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_SUB_COMMAND,sys_psm.mqtt_subscribe_command_buf, sizeof(sys_psm.mqtt_subscribe_command_buf));
		log_i("mqtt_subscribe_command_buf:%s",sys_psm.mqtt_subscribe_command_buf);
	}
	else if(flag==WRITE_PSM)
	{

		set_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_SUB_COMMAND,sys_psm.mqtt_subscribe_command_buf);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_sub_alarm_flash(uint8_t flag)
{
	if(flag==READ_PSM)
	{
		get_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_PUB_ACK,sys_psm.mqtt_publish_ack_buf, sizeof(sys_psm.mqtt_publish_ack_buf));
		log_i("mqtt_publish_ack_buf:%s",sys_psm.mqtt_publish_ack_buf);
	}
	else if(flag==WRITE_PSM)
	{

		set_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_PUB_ACK,sys_psm.mqtt_publish_ack_buf);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_mqtt_pub_back1_flash(uint8_t flag)
{
	
	if(flag==READ_PSM)
	{
		get_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_PUB_BACK1,sys_psm.mqtt_publish_back1_buf, sizeof(sys_psm.mqtt_publish_back1_buf));
		log_i("mqtt_publish_back1_buf:%s",sys_psm.mqtt_publish_back1_buf);
	}
	else if(flag==WRITE_PSM)
	{

		set_psm_conf(MQTT_SYSTEM, MQTT_SYSTEM_PUB_BACK1,sys_psm.mqtt_publish_back1_buf);
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_slave_information_flash(uint8_t flag,uint8_t num)
{
	if(num>16)
	{
		return;
	}
	char psm_val[MWM_VALUE_MED_SIZE];
	char tmp_addr[MWM_VALUE_MED_SIZE];
	char tmp_type[MWM_VALUE_MED_SIZE];
	char tmp_name[MWM_VALUE_MED_SIZE];
	int buf;
	
	snprintf(tmp_addr, sizeof(tmp_addr), "%s%d",SLAVE_SYSTEM_SLAVE_ADDR,num);
	snprintf(tmp_type, sizeof(tmp_type), "%s%d",SLAVE_SYSTEM_SLAVE_TYPE,num);
	snprintf(tmp_name, sizeof(tmp_name), "%s%d",SLAVE_SYSTEM_SLAVE_NAME,num);
	if(flag==READ_PSM)
	{
		get_psm_conf_int(SLAVE_SYSTEM,tmp_addr, &buf);
		sys_psm.slave_device[num].slave_addr=(uint8_t)buf;
		
		get_psm_conf_int(SLAVE_SYSTEM,tmp_type, &buf);
		sys_psm.slave_device[num].device_type=(uint8_t)buf;
		
		get_psm_conf(SLAVE_SYSTEM, tmp_name,sys_psm.slave_device[num].device_name, sizeof(sys_psm.slave_device[num].device_name));
		
		//log_i("r sys_psm.slave_device[%d].slave_addr=%d",num,sys_psm.slave_device[num].slave_addr);
		//log_i("r sys_psm.slave_device[%d].device_type=%d",num,sys_psm.slave_device[num].device_type);
		//log_i("r sys_psm.slave_device[%d].device_name=%s",num,sys_psm.slave_device[num].device_name);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.slave_device[num].slave_addr;
		set_psm_conf_int(SLAVE_SYSTEM,tmp_addr, buf);
		
		buf=(int)sys_psm.slave_device[num].device_type;
		set_psm_conf_int(SLAVE_SYSTEM,tmp_type, buf);		
		
		set_psm_conf(SLAVE_SYSTEM, tmp_name,sys_psm.slave_device[num].device_name);
		
		//log_i("w sys_psm.slave_device[%d].slave_addr=%d",num,sys_psm.slave_device[num].slave_addr);
		//log_i("w sys_psm.slave_device[%d].device_type=%d",num,sys_psm.slave_device[num].device_type);
		//log_i("w sys_psm.slave_device[%d].device_name=%s",num,sys_psm.slave_device[num].device_name);
	}
	else if(flag==WRITE_PSM_DEFULT)
	{
		buf=DAT_SLAVE_ADDR;
		set_psm_conf_int(SLAVE_SYSTEM,tmp_addr, buf);
		
		buf=DAT_SLAVE_TYPE;
		set_psm_conf_int(SLAVE_SYSTEM,tmp_type, buf);		
		
		set_psm_conf(SLAVE_SYSTEM, tmp_name,DAT_SLAVE_NAME);	
	}
}

/*
*函数介绍：
*参数：无
*返回值：无
*备注：无
*/
void read_write_slave_ok_flash(uint8_t flag)
{
	int buf;

	if(flag==READ_PSM)
	{
		get_psm_conf_int(SLAVE_SYSTEM,SLAVE_SYSTEM_SLAVE_OK, &buf);
		sys_psm.slave_ok=(uint8_t)buf;
		log_i("SLAVE_OK=%d",sys_psm.slave_ok);
	}
	else if(flag==WRITE_PSM)
	{
		buf=(int)sys_psm.slave_ok;
		set_psm_conf_int(SLAVE_SYSTEM,SLAVE_SYSTEM_SLAVE_OK, buf);
	}
}

