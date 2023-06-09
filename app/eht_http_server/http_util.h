/**
 * @file	httpUtil.h
 * @brief	Header File for HTTP Server Utilities
 * @version 1.0
 * @date	2014/07/15
 * @par Revision
 *			2014/07/15 - 1.0 Release
 * @author	
 * \n\n @par Copyright (C) 1998 - 2014 WIZnet. All rights reserved.
 */

#ifndef	__HTTP_UTIL_H__
#define	__HTTP_UTIL_H__

#include "http_server.h"
#include "http_parser.h"
//#include "types.h"

uint8_t http_get_cgi_handler(uint8_t * uri_name, uint8_t * buf, uint32_t * file_len);
uint8_t http_post_cgi_handler(uint8_t * uri_name, st_http_request * p_http_request, uint8_t * buf, uint32_t * file_len);

#endif
