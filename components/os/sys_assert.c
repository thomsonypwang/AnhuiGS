#include "sys_os.h"
#include "sys_assert.h"

void _sys_assert(const char *filename, int lineno, const char* fail_cond)
{
 	printf("\n\n\r*************** PANIC *************\n\r");
	printf("Filename  : %s ( %d )\n\r", filename, lineno);
	printf("Condition : %s\n\r", fail_cond);
	printf("***********************************\n\r");
	os_enter_critical_section();
	for( ;; );
}
