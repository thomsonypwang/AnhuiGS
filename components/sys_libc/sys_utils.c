#include <stdlib.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sys_os.h"
#include "sys_utils.h"
#include "sys_stdio.h"
#include "psm_crc32.h"

#define WM_MAX_FLOAT_PRECISION 9

int __ctype_isdigit(int __c)
{
	return ((unsigned)__c - '0') <= 9;
}

/* Function to convert a string to float.
 * \param[in] str Pointer to a string to be addressed
 * \param[out] endptr Pointer pointing to next character in a string */
float sys_strtof(const char *str, char **endptr)
{
	char *start_ptr = (char *)str;
	int sign = 1;

	if (endptr == NULL) 
	{
		char *end_ptr;
		endptr = &end_ptr;
	}

	if (*start_ptr == '-') 
	{
		sign = -1;
		start_ptr++;
	}
	uint32_t dec_val = 0;
	uint32_t powten = 1;
	uint32_t int_val = strtoul(start_ptr, endptr, 10);

	/* For Overload Handling because of iOS 11*/
	char temp_buf[WM_MAX_FLOAT_PRECISION+1];

	if (**endptr == '.') 
	{
		start_ptr = *endptr + 1;
		char *temp;
		temp = start_ptr;
		uint32_t len = 0;
		while (__ctype_isdigit((unsigned char)temp[len])) 
		{
			len++;
			(*endptr)++;
		}
		/* If digits after decimal are greater than maximum float
		 * precision*/
		if (len > WM_MAX_FLOAT_PRECISION) 
		{
			memcpy(temp_buf, start_ptr,
					(WM_MAX_FLOAT_PRECISION)*sizeof(char));
			temp_buf[WM_MAX_FLOAT_PRECISION] = '\0';
			dec_val = strtoul(temp_buf, NULL, 10);
			len  = WM_MAX_FLOAT_PRECISION;
			while (len--)
				powten *= 10;
			/* endptr is right now pointing just before '}'*/
			(*endptr)++;
		} 
		else 
		{
			dec_val = strtoul(start_ptr,endptr,10);
			while (start_ptr++ != *endptr)
				powten *= 10;
		}
	} 
	else
		return sign * (float)int_val;


	float dec_frac = (float)dec_val/powten;
	float result = (float)(int_val + dec_frac);

	/* Below part is done in order to improve the accuracy of the result.
	 * Since addition above results in float value being drifted from
	 * the actual value by narrow margin. e.g 50.10 results in float
	 * equivalent of 50.09.*/
	/* TODO: Visit again to see if the below code really helps.
	 * Sometimes, reporting values differently is a result of the
	 * way float values are stored in memory. In that case, the below
	 * code will not improve anything. Eg. If 50.1 is stored as 50.09
	 * in memory, we cannot do much about it.
	 */
	uint32_t result_int_value = sys_int_part_of(result);
	float result_frac_value = (float)(result) - result_int_value;

	/* Generally difference between two float values comes out to be in
	 * the order of 1/powten. e.g 0.10-0.09 comes out to be 0.00...1.
	 * Hence we multiply the result of subtraction to achieve the accuracy
	 * within desired float precision. */
	if (sys_frac_part_of(dec_frac, powten/10) >sys_frac_part_of(result_frac_value, powten/10))
		result += ((dec_frac - result_frac_value) * powten);
	if (sys_frac_part_of(result_frac_value, powten/10) >sys_frac_part_of(dec_frac, powten/10))
		result += ((result_frac_value - dec_frac) * powten);

	return sign * result;
}
