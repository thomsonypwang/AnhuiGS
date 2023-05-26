#ifndef _CORE_LOG_H_
#define _CORE_LOG_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "core_stdinc.h"
#include "core_string.h"
#include "aiot_state_api.h"
#include "posix_port.h"
#include <core_config.h>

#include "sys_log.h"

#define CORE_LOG_MODULE_NAME "LOG"
#define CORE_LOG_MAXLEN (2048)


typedef enum {
	CORE_LOG_TRACE = 0,
 	CORE_LOG_DEBUG = 1,
 	CORE_LOG_INFO = 2,
	CORE_LOG_WARN = 3,
 	CORE_LOG_ERROR = 4,
	CORE_LOG_MAX
} core_log_level_t;

void core_set_log_level(core_log_level_t level);
core_log_level_t core_get_log_level();
void writeLog(const char *fname, const int line, int level, const char *format, ...);

void _log_dump(const char *name, const char *buffer, uint32_t len);


#if (AIOT_LOG_ENABLE)
#define at_log_trace(args...) log("at",args)//writeLog(__FUNCTION__, __LINE__, CORE_LOG_TRACE, args)
#define at_log_debug(args...) log("at",args)//writeLog(__FUNCTION__, __LINE__, CORE_LOG_DEBUG, args)
#define at_log_info(args...)  log("at",args)//writeLog(__FUNCTION__, __LINE__, CORE_LOG_INFO, args)
#define at_log_warn(args...)  log("at",args)//writeLog(__FUNCTION__, __LINE__, CORE_LOG_WARN, args)
#define at_log_error(args...) log("at",args)//writeLog(__FUNCTION__, __LINE__, CORE_LOG_ERROR, args)
#define at_log_dump(name, buffer, len) _log_dump(name, buffer, len)

#else 

#define at_log_trace(args...) 	
#define at_log_debug(args...) 	
#define at_log_info(args...)  
#define at_log_warn(args...)  
#define at_log_error(args...) 
#define at_log_dump(name, buffer, len) 

#endif
#if defined(__cplusplus)
}
#endif

#endif

