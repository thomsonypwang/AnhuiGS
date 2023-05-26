#ifndef __ETH_HANDLE_H_
#define __ETH_HANDLE_H_

#include "project_config.h"

#define CAMERA_SOCKET      0
#define	MQTT_SOCKET		1
#define	LOCK_SOCKET		2
#define MAX_HTTPSOCK	4

void w5500_process_init(void);
void w5500_network_init(void);
#endif
