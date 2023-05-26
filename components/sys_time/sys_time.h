#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "project_config.h"

typedef long suseconds_t;


#ifdef CONFIG_HW_RTC
	#define rtc_init       hwrtc_init
	#define rtc_time_set   hwrtc_time_set
	#define rtc_time_get   hwrtc_time_get

	extern void hwrtc_init(void);
	extern int hwrtc_time_set(time_t time);
	extern time_t hwrtc_time_get(void);
	extern void hwrtc_time_update(void);
#else
	/* Use Software RTC (Real Time Clock) if no hardware RTC is available */
	#define rtc_init       swrtc_init
	#define rtc_time_set   swrtc_time_set
	#define rtc_time_get   swrtc_time_get

	extern void swrtc_init(void);
	extern int swrtc_time_set(time_t time);
	extern time_t swrtc_time_get(void);

	/** Time stored in POSIX format (Seconds since epoch) */
	extern time_t cur_posix_time;

	/** Absolute number of ticks */
	extern unsigned int abs_tick_count;
#endif

/** Convert HTTP date format to POSIX time format
 *
 * \param[in] date HTTP date format
 *
 * \return success or failure as:
 *     -WM_FAIL: Conversion failed. Invalid format/data
 *     else valid time_t value
 */
time_t http_date_to_time(const unsigned char *date);

/** Set the date and time
 *
 * \param[in] tm The rtc value is updated with the values in tm structure
 * \return success or failure as:
 *     0: Success
 *     -1: Failed validation of tm structure
 */
extern int sys_time_time_set(const struct tm *tm);

/**
 * Get date and time
 *
 * \param[out] tm tm structure is updated to get the current value in rtc
 * \return success or failure as:
 *     0: Success
 *     non-zero: Internal error
 */
extern int sys_time_time_get(struct tm *tm);

/** Set the date and time using posix time
*
*  \param[in] time The rtc value is updated with the value present in time
*  \return success or failure as:
*      0: Success
*      non-zero: Internal error
*/
extern int sys_time_time_set_posix(time_t time);

/**
 * Get date and time in posix format
 *
*  \return time_t value from RTC
 */
extern time_t sys_time_time_get_posix(void);

/**
 * Convert to tm structure from POSIX/Unix time (Seconds since epoch)
 *
 * \param[in] time This is POSIX time that is to be converted into \ref tm
 * \param[out] result This should point to pre-allocated \ref tm instance
 * \return pointer to struct tm; NULL in case of error
 */
struct tm *gmtime_r(const time_t *time, struct tm *result);

/**
 * Converts to POSIX/Unix time from tm structure
 *
 * \param[in] tm This is \ref tm instance that is to be converted into
 * time_t format
 * \return time_t POSIX/Unix time equivalent
 */
extern time_t mktime(struct tm *tm);

/**
 * Converts the broken-down time value tm into a null-terminated string.
 *
 * \param[in] tm This is \ref tm instance that is to be converted string.
 *
 * @return Pointer to a statically allocated string which contains the date
 * and time format as follows "Tue Mar 24 09:20:14 2015". The statically
 * allocated string might be overwritten by subsequent calls to any of the
 * date and time functions.
 */
char *asctime(const struct tm *tm);

/**
 * Initialize time subsystem including RTC. Sets system time to 1/1/1970 00:00
 * (i.e. epoch 0)
 *
 * \return WM_SUCCESS on success, zero otherwise
 */
extern int sys_time_init(void);

/** Get current wmtime initialization status
 *
 *\return true if initialized, false if not.
 */
bool is_sys_time_init_done();


#endif
