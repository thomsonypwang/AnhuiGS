#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "wdt_drv.h"
#include "sys_log.h"
#include "sys_os.h"
#include "sys_errno.h"
#include "project_pin_use_config.h"



#define MAX_TIMEOUT_VALUE 4

const uint32_t period_data[4]={SWDT_CNT_PERIOD256,SWDT_CNT_PERIOD4096,SWDT_CNT_PERIOD16384,SWDT_CNT_PERIOD65536};

int wdt_drv_set_timeout(uint8_t index)
{
	stc_swdt_init_t stcSwdtInit;

	if (index > MAX_TIMEOUT_VALUE) 
	{
		log_e("wdt","Incorrect value.\r\n");
		return SYS_E_INVAL;
	}
	LL_PERIPH_WE(PROJECT_PERIPH_WE);  
	
    /* SWDT configuration */
    stcSwdtInit.u32CountPeriod   = period_data[index];
    stcSwdtInit.u32ClockDiv      = SWDT_CLK_DIV32;
    stcSwdtInit.u32RefreshRange  = ICG_SWDT_RANGE_0TO100PCT;
    stcSwdtInit.u32LPMCount      = SWDT_LPM_CNT_STOP;
    stcSwdtInit.u32ExceptionType = SWDT_EXP_TYPE_RST;
    (void)SWDT_Init(&stcSwdtInit);
	
	LL_PERIPH_WP(PROJECT_PERIPH_WP);
	return SYS_OK;
}

void wdt_drv_strobe(void)
{
	SWDT_FeedDog();
}

void wdt_drv_start(void)
{
	SWDT_FeedDog();
}


