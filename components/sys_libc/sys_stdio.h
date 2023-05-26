#ifndef _SYS_STDIO_H_
#define _SYS_STDIO_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "hc32_ll.h"


/** Maximum number of characters that can be printed using wmprintf */
#define MAX_MSG_LEN 127

typedef struct stdio_funcs {
	int (*sf_printf)(char *);
	int (*sf_flush)();
	int (*sf_getchar)(uint8_t *);
	int (*sf_putchar)(char *);
} stdio_funcs_t;

extern stdio_funcs_t *c_stdio_funcs;


/** The printf function
 *
 * This function prints data to the output port. The function prototype is analogous
 * to the printf() function of the C language.
 *
 * \param[in] format The format string
 *
 * \return Number of bytes written to the console.
 */
extern int sys_printf(const char *format, ...);

/** The printf function
 *
 * This function prints character to the output port. This function should be
 * used for printing data character by character.
 *
 * \param[in] ch Pointer to the character to written to the console
 *
 * \return Number of bytes written to the console.
 */
extern int sys_stdio_putchar(char *ch);

/** Flush Standard I/O
 *
 * This function flushes the send buffer to the output port.
 *
 * @return WM_SUCCESS
 */
extern uint32_t sys_stdio_flush(void);

/**
 * Read one byte
 *
 * \param pu8c Pointer to the location to store the byte read.
 *
 * \return Number of bytes read from console port.
 */
extern uint32_t sys_stdio_getchar(uint8_t *pu8c);

/**
 * Initialize the standard input output facility
 *
 * \param[in] uart_port UART Port to be used as the console (e.g. UART0_ID,
 * UART1_ID etc.
 * \param[in] baud Baud rate to be used on the port. 0 implies the default baud
 * rate which is 115200 bps.
 * \return WM_SUCCESS on success, error code otherwise.
 */
extern int sys_stdio_init(void *vpDevice, uint32_t u32Param);


/* To be used in situations where you HAVE to send data to the console */
uint32_t __console_wmprintf(const char *format, ...);

/* Helper functions to print a float value. Some compilers have a problem
 * interpreting %f
 */

#define sys_int_part_of(x)     ((int)(x))
static inline int sys_frac_part_of(float x, short precision)
{
	int scale = 1;

	while (precision--)
	{
		scale *= 10;
	}
	return (x < 0 ? (int)(((int)x - x) * scale) : (int)((x - (int)x) * scale));
}

#endif
