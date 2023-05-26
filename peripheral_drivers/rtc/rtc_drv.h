#ifndef _RTC_DRV_H_
#define _RTC_DRV_H_

#include "sys_log.h"

struct timeval {
	long tv_sec; 	/* seconds */
	long tv_usec; 	/* and microseconds */
};

/** Register RTC Device
 *
 * This function registers rtc driver with mdev interface.
 *
 * \return WM_SUCCESS on success, error code otherwise.
 */
int rtc_drv_init(void);

/** Set RTC Configuration
 *
 * This function sets configuration for rtc device.
 *
 * \param[in] dev mdev_t handle to the driver.
 * \param[in] cnt_upp Overflow value. Counting starts from 0
 * and does to to this value. Frequency is 1KHz by default for A0 version of
 * chip and 32KHz for Z1 or earlier chips.
 */
void rtc_drv_set(uint32_t cnt_upp);

/** Get RTC counter value.
 *
 * This function gets the value of RTC counter.
 *
 * \param[in] dev mdev_t handle to the driver.
 * \return the counter value.
 */
uint32_t rtc_drv_get(void);

/** Set RTC callback handler for UPP Val
 *
 * This function sets callback handler with RTC driver.
 *
 * \param[in] user_cb application registered callback handler, if func is NULL
 * then the callback is unset.
 */
void rtc_drv_set_cb(void (*user_cb)());

/** Start RTC device
 *
 * This function starts RTC operation.
 *
 * \param[in] dev mdev_t handle to the driver.
 */
void rtc_drv_start(void);

/** Stop RTC device
 *
 * This function stops RTC operation.
 *
 * \param[in] dev mdev_t handle to the driver.
 */
void rtc_drv_stop(void);

/** Close RTC Device
 *
 * This function closes the device after use and frees up resources.
 *
 * \param[in] dev mdev handle to the driver to be closed.
 * \return WM_SUCCESS on success, -WM_FAIL on error.
 */
int rtc_drv_close(void);

void rtc_get_time(stc_rtc_date_t *CurrentDate,stc_rtc_time_t *CurrentTime);
void rtc_set_time(int year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec, uint8_t week,stc_rtc_date_t *CurrentDate,stc_rtc_time_t *CurrentTime);

#endif /* _MDEV_RTC_H_ */
