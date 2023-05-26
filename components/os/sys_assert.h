#ifndef _SYS_ASSERT_H_
#define _SYS_ASSERT_H_

#include <string.h>

#include "project_config.h"

#ifdef DBG_ENABLE_ASSERTS
	#define ASSERT(_cond_) if(!(_cond_)) _sys_assert(__FILE__, __LINE__, #_cond_)
#else
	#define ASSERT(_cond_)
#endif 

void _sys_assert(const char *filename, int lineno, const char* fail_cond);



#endif 
