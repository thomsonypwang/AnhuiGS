
#ifndef _AIOT_MODULE_H_
#define _AIOT_MODULE_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

#define QSDK_TIME_ZONE 8

struct nb_device
{
    int  sim_state;
    int  device_ok;
    int  reboot_open;
    int  fota_open;
    int  reboot_type;
    int  net_connect_ok;
    int  error;
    int  csq;
	int  ber;
    char imsi[16];
    char imei[16];
    char ip[16];
};
extern struct nb_device nb_device_table;
/**
 * @brief 初始化模组参数相关会话句柄
 * 
 * @param[in] module AT模组型号
 *
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败, 更多信息请参考@ref aiot_state_api.h
 * @retval >=STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_module_init(void);

/**
 * @brief 释放模组
 *
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败, 更多信息请参考@ref aiot_state_api.h
 * @retval >=STATE_SUCCESS 执行成功
 *
 */
void aiot_module_deinit(void);

/**
 * @brief 通过AT命令软件重启模组
 * 
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败, 更多信息请参考@ref aiot_state_api.h
 * @retval >=STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_module_software_reboot(void);

/**
 * @brief 与模组建立AT命令通信,查询模组的软硬件版本信息
 * 
 * @param[in] timeout 等待模组启动的超时时间
 * 
 * @return int32_t
 * @retval <STATE_SUCCESS 执行失败, 更多信息请参考@ref aiot_state_api.h
 * @retval >=STATE_SUCCESS 执行成功
 *
 */
int32_t aiot_module_handshake(uint32_t timeout);

//int32_t qsdk_nb_get_imsi(void);
int qsdk_nb_quick_connect(void);

#if defined(__cplusplus)
}
#endif

#endif
