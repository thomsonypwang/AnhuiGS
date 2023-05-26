#ifndef __PSM_INTERNAL_H__
#define __PSM_INTERNAL_H__

//#define CONFIG_PSM_INIT_SCAN_DEBUG 
//#define CONFIG_PSM_EXTRA_DEBUG 
////#define CONFIG_PSM_FUNC_DEBUG 
//#define CONFIG_PSM_INDEXING_DEBUG 
//#define CONFIG_PSM_LOWLEVEL_FLASH_OPR_DEBUG 
//#define CONFIG_PSM_LOCKS_DEBUG 
//#define CONFIG_PSM_DEBUG

#include "sys_log.h"

#ifdef CONFIG_PSM_INIT_SCAN_DEBUG
#define psm_isc(...) log("psm-isc", ##__VA_ARGS__)
#else
#define psm_isc(...)
#endif

#define psm_e(...) log_e("psm", ##__VA_ARGS__)
#define psm_w(...) log_w("psm", ##__VA_ARGS__)

#ifdef CONFIG_PSM_DEBUG
	#define psm_d(...) log("psm", ##__VA_ARGS__)
#else
	#define psm_d(...)
#endif /* ! CONFIG_PSM_DEBUG */

#ifdef CONFIG_PSM_EXTRA_DEBUG
	#define psm_ed(...) log("psm-ed", ##__VA_ARGS__)
#else
	#define psm_ed(...)
#endif /* ! CONFIG_PSM_EXTRA_DEBUG */

#ifdef CONFIG_PSM_INDEXING_DEBUG
	#define psm_i(...) log("psm-i", ##__VA_ARGS__)
#else
	#define psm_i(...)
#endif /* ! CONFIG_PSM_INDEXING_DEBUG */

#ifdef CONFIG_PSM_LOWLEVEL_FLASH_OPR_DEBUG
	#define psm_ll(...) log("psm-ll", ##__VA_ARGS__)
#else
	#define psm_ll(...)
#endif /* ! CONFIG_PSM_LOWLEVEL_FLASH_OPR_DEBUG */

#ifdef CONFIG_PSM_FUNC_DEBUG
	#define psm_entry(_fmt_, ...) printf("######## > %s (" _fmt_ ") ####\n\r", __func__, ##__VA_ARGS__)
	#define psm_entry_i(...)      wmlog_entry(__VA_ARGS__)
#else
	#define psm_entry(...)
	#define psm_entry_i(...)
#endif /* ! CONFIG_PSM_DEBUG */

typedef enum
{
    PSM_EVENT_READ,
    PSM_EVENT_WRITE,
    PSM_EVENT_ERASE,
    PSM_EVENT_COMPACTION,
} psm_event_t;

typedef enum
{
    PSM_COMPACTION_1,
    PSM_COMPACTION_2,
    PSM_COMPACTION_3,
    PSM_COMPACTION_4,
    PSM_COMPACTION_5,
    PSM_COMPACTION_6,
    PSM_COMPACTION_7,
    PSM_COMPACTION_8,
    PSM_COMPACTION_9,
    PSM_COMPACTION_10,
    PSM_COMPACTION_MAX,
} psm_compaction_evt_t;

typedef void (*psm_event_callback)(psm_event_t event, void *data, void *data1);

int psm_register_event_callback(psm_event_callback cb);

#endif /* __PSM_INTERNAL_H__ */
