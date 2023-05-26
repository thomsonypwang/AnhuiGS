#include "rtc_drv.h"
//#include "sys_stdio.h"
#include "sys_os.h"
//#include "rtc_dir.h"
#include <time.h>
#include "project_pin_use_config.h"


/* seconds per day */
#define SPD 24*60*60

uint8_t year_flag;

/* days per month -- nonleap! */
static const short __spm[13] =
{
    0,
    (31),
    (31 + 28),
    (31 + 28 + 31),
    (31 + 28 + 31 + 30),
    (31 + 28 + 31 + 30 + 31),
    (31 + 28 + 31 + 30 + 31 + 30),
    (31 + 28 + 31 + 30 + 31 + 30 + 31),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30),
    (31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31),
};

static int __isleap(int year)
{
    /* every fourth year is a leap year except for century years that are
     * not divisible by 400. */
    /*  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)); */
    return (!(year % 4) && ((year % 100) || !(year % 400)));
}

/**
 * @brief  RTC calendar configuration.
 * @param  None
 * @retval None
 */
void rtc_set_time(int year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec, uint8_t week,stc_rtc_date_t *CurrentDate,stc_rtc_time_t *CurrentTime)
{
    stc_rtc_date_t stcRtcDate;
    stc_rtc_time_t stcRtcTime;
	uint8_t year_tmp;

    /* Date configuration */
	if(year>=2000)
	{
		year_tmp=(uint8_t)(year-2000);
		stcRtcDate.u8Year    = year_tmp;
		stcRtcDate.u8Month   = month;
		stcRtcDate.u8Day     = day;
		stcRtcDate.u8Weekday = week;	
		year_flag=0;
	}
	else
	{
		year_tmp=(uint8_t)(year-1900);
		stcRtcDate.u8Year    = year_tmp;
		stcRtcDate.u8Month   = month;
		stcRtcDate.u8Day     = day;
		stcRtcDate.u8Weekday = week;
		year_flag=1;
	}

	stcRtcTime.u8Hour   = hour;
	stcRtcTime.u8Minute = min;
	stcRtcTime.u8Second = sec;
    if (LL_OK != RTC_SetDate(RTC_DATA_FMT_DEC, &stcRtcDate)) 
	{
        log_e("rtcdrv","Set Date failed!");
    }
	else
	{
		CurrentDate=&stcRtcDate;
	}
    if (LL_OK != RTC_SetTime(RTC_DATA_FMT_DEC, &stcRtcTime)) 
	{
        log_e("rtcdrv","Set Time failed!");
    }
	else
	{
		CurrentTime=&stcRtcTime;
	}
}

void rtc_get_time(stc_rtc_date_t *CurrentDate,stc_rtc_time_t *CurrentTime)
{	
	if (LL_OK == RTC_GetDate(RTC_DATA_FMT_DEC, CurrentDate)) /* Get current date */
	{
		if (LL_OK != RTC_GetTime(RTC_DATA_FMT_DEC, CurrentTime))/* Get current time */ 
		{
			log_e("rtcdrv","Get time failed!");
		} 
	} 
	else 
	{
		log_e("rtcdrv","Get date failed!");
	}
}

inline void rtc_drv_set_cb(void (*user_cb)())
{
	stc_irq_signin_config_t stcIrq;
	
	stcIrq.enIntSrc = INT_SRC_RTC_PRD;
	stcIrq.enIRQn = INT052_IRQn;
	stcIrq.pfnCallback = user_cb;
	(void)INTC_IrqSignIn(&stcIrq);	
}

time_t timegm(struct tm * const t)
{
    time_t day;
    time_t i;
    time_t years;

    if(t == NULL)
    {
        return (time_t)-1;
    }

    years = (time_t)t->tm_year - 70;
    if (t->tm_sec > 60)         /* seconds after the minute - [0, 60] including leap second */
    {
        t->tm_min += t->tm_sec / 60;
        t->tm_sec %= 60;
    }
    if (t->tm_min >= 60)        /* minutes after the hour - [0, 59] */
    {
        t->tm_hour += t->tm_min / 60;
        t->tm_min %= 60;
    }
    if (t->tm_hour >= 24)       /* hours since midnight - [0, 23] */
    {
        t->tm_mday += t->tm_hour / 24;
        t->tm_hour %= 24;
    }
    if (t->tm_mon >= 12)        /* months since January - [0, 11] */
    {
        t->tm_year += t->tm_mon / 12;
        t->tm_mon %= 12;
    }
    while (t->tm_mday > __spm[1 + t->tm_mon])
    {
        if (t->tm_mon == 1 && __isleap(t->tm_year + 1900))
        {
            --t->tm_mday;
        }
        t->tm_mday -= __spm[t->tm_mon];
        ++t->tm_mon;
        if (t->tm_mon > 11)
        {
            t->tm_mon = 0;
            ++t->tm_year;
        }
    }

    if (t->tm_year < 70)
    {
        return (time_t) -1;
    }

    /* Days since 1970 is 365 * number of years + number of leap years since 1970 */
    day = years * 365 + (years + 1) / 4;

    /* After 2100 we have to substract 3 leap years for every 400 years
     This is not intuitive. Most mktime implementations do not support
     dates after 2059, anyway, so we might leave this out for it's
     bloat. */
    if (years >= 131)
    {
        years -= 131;
        years /= 100;
        day -= (years >> 2) * 3 + 1;
        if ((years &= 3) == 3)
            years--;
        day -= years;
    }

    day += t->tm_yday = __spm[t->tm_mon] + t->tm_mday - 1 +
                        (__isleap(t->tm_year + 1900) & (t->tm_mon > 1));

    /* day is now the number of days since 'Jan 1 1970' */
    i = 7;
    t->tm_wday = (day + 4) % i; /* Sunday=0, Monday=1, ..., Saturday=6 */

    i = 24;
    day *= i;
    i = 60;
    return ((day + t->tm_hour) * i + t->tm_min) * i + t->tm_sec;
}

static int hc32_rtc_get_time_stamp(struct timeval *tv)
{
    stc_rtc_time_t stcRtcTime = {0};
    stc_rtc_date_t stcRtcDate = {0};
    struct tm tm_new = {0};

    RTC_GetTime(RTC_DATA_FMT_DEC, &stcRtcTime);
    RTC_GetDate(RTC_DATA_FMT_DEC, &stcRtcDate);

    tm_new.tm_sec  = stcRtcTime.u8Second;
    tm_new.tm_min  = stcRtcTime.u8Minute;
    tm_new.tm_hour = stcRtcTime.u8Hour;
    if(stcRtcDate.u8Month == 0)
    {
        tm_new.tm_mday = stcRtcDate.u8Day + 1;
        tm_new.tm_mon  = stcRtcDate.u8Month;
    }
    else
    {
        tm_new.tm_mday = stcRtcDate.u8Day ;
        tm_new.tm_mon  = stcRtcDate.u8Month - 1;
    }
    tm_new.tm_year = stcRtcDate.u8Year + 100;

    tv->tv_sec = timegm(&tm_new);

    return SYS_OK;
}

struct tm *gmtime_r_tmp(const time_t *timep, struct tm *r)
{
    time_t i;
    time_t work = *timep % (SPD);

    if(timep == NULL || r == NULL)
    {
        return NULL;
    }

    memset(r, NULL, sizeof(struct tm));

    r->tm_sec = work % 60;
    work /= 60;
    r->tm_min = work % 60;
    r->tm_hour = work / 60;
    work = *timep / (SPD);
    r->tm_wday = (4 + work) % 7;
    for (i = 1970;; ++i)
    {
        time_t k = __isleap(i) ? 366 : 365;
        if (work >= k)
            work -= k;
        else
            break;
    }
    r->tm_year = i - 1900;
    r->tm_yday = work;

    r->tm_mday = 1;
    if (__isleap(i) && (work > 58))
    {
        if (work == 59)
            r->tm_mday = 2; /* 29.2. */
        work -= 1;
    }

    for (i = 11; i && (__spm[i] > work); --i);

    r->tm_mon = i;
    r->tm_mday += work - __spm[i];

    r->tm_isdst = 0;
    return r;
}

static int hc32_rtc_set_time_stamp(time_t time_stamp)
{
    stc_rtc_time_t stcRtcTime = {0};
    stc_rtc_date_t stcRtcDate = {0};
    struct tm p_tm = {0};

    gmtime_r_tmp(&time_stamp, &p_tm);

    if (p_tm.tm_year < 100)
    {
        return SYS_FAIL;
    }

    stcRtcTime.u8Second  = p_tm.tm_sec ;
    stcRtcTime.u8Minute  = p_tm.tm_min ;
    stcRtcTime.u8Hour    = p_tm.tm_hour;
    stcRtcDate.u8Day     = p_tm.tm_mday;
    stcRtcDate.u8Month   = p_tm.tm_mon + 1;
    stcRtcDate.u8Year    = p_tm.tm_year - 100;
    stcRtcDate.u8Weekday = p_tm.tm_wday;

    if (LL_OK != RTC_SetTime(RTC_DATA_FMT_DEC, &stcRtcTime))
    {
        return SYS_FAIL;
    }
    if (LL_OK != RTC_SetDate(RTC_DATA_FMT_DEC, &stcRtcDate))
    {
        return SYS_FAIL;
    }
    return SYS_OK;
}

inline void rtc_drv_set(uint32_t cnt_upp)
{
	hc32_rtc_set_time_stamp(cnt_upp);
}

inline uint32_t rtc_drv_get(void)
{
    struct timeval tv;

    hc32_rtc_get_time_stamp(&tv);
    return tv.tv_sec;
}

inline void rtc_drv_stop(void)
{
	RTC_Cmd(DISABLE);
}

inline void rtc_drv_start(void)
{
	RTC_Cmd(ENABLE);
}

inline int rtc_drv_close(void)
{
	return 0;
}

/**
 * @brief  XTAL32 clock initialize.
 * @param  None
 * @retval None
 */
void xtal32_clock_init(void)
{
    stc_clock_xtal32_init_t stcXtal32Init;

    /* Xtal32 config */
    stcXtal32Init.u8State  = CLK_XTAL32_ON;
    stcXtal32Init.u8Drv    = CLK_XTAL32_DRV_HIGH;
    stcXtal32Init.u8Filter = CLK_XTAL32_FILTER_RUN_MD;
    (void)CLK_Xtal32Init(&stcXtal32Init);
    /* Waiting for XTAL32 stabilization */
    DDL_DelayMS(1000U);
}

void rtc_init(void)
{
    stc_rtc_init_t stcRtcInit;
    stc_rtc_date_t stcRtcDate;
    stc_rtc_time_t stcRtcTime;

    if (DISABLE == RTC_GetCounterState()) /* RTC stopped */
	{	        
        PWC_VBAT_Reset();/* Reset the VBAT area */
		if (LL_ERR_TIMEOUT != RTC_DeInit()) /* Reset RTC counter */
		{
			(void)RTC_StructInit(&stcRtcInit);/* Configure structure initialization */

			/* Configuration RTC structure */
			stcRtcInit.u8ClockSrc = RTC_CLK_SRC_XTAL32;
			stcRtcInit.u8HourFormat = RTC_HOUR_FMT_24H;
			stcRtcInit.u8IntPeriod = RTC_INT_PERIOD_PER_SEC;
			stcRtcInit.u8ClockCompen = RTC_CLK_COMPEN_DISABLE;
			(void)RTC_Init(&stcRtcInit);

			/* Update date and time */
			/* Date configuration */
			stcRtcDate.u8Year = 22U;
			stcRtcDate.u8Month = RTC_MONTH_DECEMBER;
			stcRtcDate.u8Day = 5U;
			stcRtcDate.u8Weekday = RTC_WEEKDAY_MONDAY;

			/* Time configuration */
			stcRtcTime.u8Hour = 3U;
			stcRtcTime.u8Minute = 0U;
			stcRtcTime.u8Second = 0U;
			stcRtcTime.u8AmPm = RTC_HOUR_12H_PM;
			year_flag=0;
			if (LL_OK != RTC_SetDate(RTC_DATA_FMT_DEC, &stcRtcDate)) 
			{
				return;
			}

			if (LL_OK != RTC_SetTime(RTC_DATA_FMT_DEC, &stcRtcTime)) 
			{
				return;
			}
			RTC_Cmd(ENABLE);/* Startup RTC count */	
		}
	}
}

int rtc_drv_init(void)
{	
	LL_PERIPH_WE(PROJECT_PERIPH_WE);
	xtal32_clock_init();	
	rtc_init();/* Configure if not already running */
	RTC_IntCmd(RTC_INT_PERIOD, ENABLE);/* Enable RTC interrupt */

	NVIC_ClearPendingIRQ(INT052_IRQn);
	NVIC_SetPriority(INT052_IRQn, DDL_IRQ_PRIO_15);
	NVIC_EnableIRQ(INT052_IRQn);	
	LL_PERIPH_WP(PROJECT_PERIPH_WP);
	
	return SYS_OK;
}
