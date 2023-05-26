#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "sys_time.h"
#include "sys_errno.h"
#include "sys_os.h"

#include "rtc_drv.h"

/**
 * Global variable to avoid multiple initialization of rtc
 */
static int hwrtc_init_flag;
static uint64_t rtc_ticks;
static uint32_t rtc_sig;

/*
 * It is observed that it takes ~5ms for update, reset, start
 * cycle of the RTC. So we need to add the lost time everytime.
 * NOTE: This is an approximate compensation, so some inaccuracy
 * will persist in the time kept by the RTC.
 */
uint8_t tcalib = 5;

void hwrtc_time_update(void)
{
	rtc_ticks += (uint64_t)rtc_drv_get() + (uint64_t)tcalib;
}

static void hwrtc_cb(void)
{
	RTC_ClearStatus(RTC_FLAG_PERIOD);
	rtc_ticks +=(uint64_t)1;
}

void hwrtc_init(void)
{
	if (hwrtc_init_flag)
	{
		return;
	}
	hwrtc_init_flag = 1;

	/* Initialize if nvram is empty */
	if (rtc_sig != 0xdeadbeef) 
	{
		rtc_ticks = 0;
		rtc_sig = 0xdeadbeef;
	}

	rtc_drv_set_cb(hwrtc_cb);
	rtc_drv_init();
	/* Update the time before reseting RTC */
	hwrtc_time_update();
	rtc_drv_start();
}

int hwrtc_time_set(time_t time)
{
	rtc_drv_set(time);
	/* RTC runs at 1024 Hz */
	rtc_ticks = (uint64_t)time;
	return 0;
}

time_t hwrtc_time_get(void)
{
	/* Convert to seconds before returning */
	return rtc_ticks ;
}
