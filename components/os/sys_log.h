#ifndef __SYS_LOG_H__
#define __SYS_LOG_H__

#include "sys_stdio.h"

#include "project_config.h"

//#ifdef DBG_ENABLE_LOG
	#define log_e(_mod_name_, _fmt_, ...) sys_printf("[%s]%s" _fmt_ "\n\r", _mod_name_, " Error: ", ##__VA_ARGS__)
	#define log_w(_mod_name_, _fmt_, ...) sys_printf("[%s]%s" _fmt_ "\n\r", _mod_name_, " Warn: ", ##__VA_ARGS__)
	#define log(_mod_name_, _fmt_, ...)   sys_printf("[%s]:" _fmt_ "\n\r", _mod_name_, ##__VA_ARGS__)	
	#define log_i( _fmt_, ...)   sys_printf("[I]:" _fmt_ "\n\r", ##__VA_ARGS__)		
//#else
//	#define log_e(_mod_name_, _fmt_, ...) //sys_printf("[%s]%s" _fmt_ "\n\r", _mod_name_, " Error: ", ##__VA_ARGS__)
//	#define log_w(_mod_name_, _fmt_, ...) //sys_printf("[%s]%s" _fmt_ "\n\r", _mod_name_, " Warn: ", ##__VA_ARGS__)
//	#define log_i( _fmt_, ...)
//	#define log(_mod_name_, _fmt_, ...)  
//#endif 


#endif