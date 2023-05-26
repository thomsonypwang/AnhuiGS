#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <aiot_at_client_api.h>
#include <core_log.h>
#include "posix_port.h"
#include "sys_os.h"

//#define LOG_TAG             "at.clnt"
//#define LOG_E(arg...)       at_log_error(arg)
//#define LOG_W(arg...)       at_log_warn(arg)
//#define LOG_D(arg...)       at_log_info(arg)

#define AT_RESP_END_OK                 "OK"
#define AT_RESP_END_ERROR              "ERROR"
#define AT_RESP_END_FAIL               "FAIL"
#define AT_END_CR_LF                   "\r\n"

static at_client_t at_client_table = { 0 };

#ifndef __INT_MAX__
#define __INT_MAX__     2147483647
#endif

#define CORE_ASSERT_ERR(ex, err)                                              \
if (!(ex))                                                                    \
{                                                                             \
    at_log_error("(%s) assertion failed at function:%s, line number:%d \n",          \
        #ex, __FUNCTION__, __LINE__);                                         \
    return err;                                                               \
}

uint32_t at_vprintfln(const char *format, va_list args);
const char *at_get_last_cmd(uint32_t *cmd_size);

static char nb_send_buf[AIOT_SEND_CMD_MAX_SIZE] = {0};
static int last_cmd_len = 0;

uint32_t at_vprintfln(const char *format, va_list args)
{
    uint32_t len;
	
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    last_cmd_len = vsnprintf(nb_send_buf, sizeof(nb_send_buf) - 2, format, args);
    if(last_cmd_len > sizeof(nb_send_buf) - 2) 
	{
        last_cmd_len = sizeof(nb_send_buf) - 2;
    }
    memcpy(nb_send_buf + last_cmd_len, "\r\n", 2);
    len = last_cmd_len + 2;
    
	#if (AIOT_AT_DUMP_RAW_CMD)
		at_log_info(">>>>");
		at_log_dump("sendline", nb_send_buf, len);
	#endif
    return sysdep->core_usart_send(nb_send_buf, len, AIOT_WAIT_MAX_TIME);
}

const char *at_get_last_cmd(uint32_t *cmd_size)
{
    *cmd_size = last_cmd_len;
    return nb_send_buf;
}

/**
 * Create response object.
 *
 * @param buf_size the maximum response buffer size
 * @param line_num the number of setting response lines
 *         = 0: the response data will auto return when received 'OK' or 'ERROR'
 *        != 0: the response data will return when received setting lines number data
 * @param timeout the maximum response time
 * @param rsp_exp 期望读取到的关键字 
 * @param rsp_exp_len 期望读取到的长度
 *
 * @return != NULL: response object
 *          = NULL: no memory
 */
at_response_t *at_create_resp(uint32_t buf_size, uint32_t line_num, int32_t timeout, const char *rsp_exp, uint32_t rsp_exp_len)
{
    at_response_t *resp = NULL;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();

    resp = (at_response_t *)sysdep->core_sysdep_malloc(sizeof(at_response_t), CORE_AT_CLIENT_MODULE);
    if (resp == NULL)
    {
        at_log_error("AT create response object failed! No memory for response object!");
        return NULL;
    }
    resp->buf = (char *) sysdep->core_sysdep_malloc(buf_size, CORE_AT_CLIENT_MODULE);
    if (resp->buf == NULL)
    {
        at_log_error("AT create response object failed! No memory for response buffer!");
        sysdep->core_sysdep_free(resp);
        return NULL;
    }
    memset(resp->buf, '\0', buf_size);
    if (rsp_exp != NULL) 
	{
        uint32_t rsp_len = strlen(rsp_exp);
        if ((resp->rsp_exp = (char *) sysdep->core_sysdep_malloc(rsp_len + 1, CORE_AT_CLIENT_MODULE)) == NULL)
        {
            sysdep->core_sysdep_free(resp->buf);
            sysdep->core_sysdep_free(resp);
            return NULL;
        }
        memset(resp->rsp_exp, '\0', rsp_len + 1);
        memcpy(resp->rsp_exp, rsp_exp, rsp_len);
    } 
	else 
	{
        resp->rsp_exp = NULL;
    }
    resp->rsp_exp_len = rsp_exp_len;
    resp->buf_size = buf_size;
    resp->line_num = line_num;
    resp->line_counts = 0;
    resp->timeout = timeout;
    return resp;
}

/**
 * Delete and free response object.
 *
 * @param resp response object
 */
void at_delete_resp(at_response_t *resp)
{
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    if (resp && resp->buf)
    {
        sysdep->core_sysdep_free(resp->buf);
    }

    if (resp && resp->rsp_exp)
    {
        sysdep->core_sysdep_free(resp->rsp_exp);
    }
    
    if (resp)
    {
        resp->rsp_exp_len = 0;
        sysdep->core_sysdep_free(resp);
        resp = NULL;
    }
}

/**
 * at_sscanf - Unformat a buffer into a list of arguments, rewrite sscanf
 * @param buf:	input buffer
 * @param fmt:	format of buffer
 * @param args:	arguments
 */
int at_sscanf(const char * buf, const char * fmt, va_list args)
{
    const char *str = buf;
    char *next;
    int num = 0;
    int qualifier;
    int base;
    int field_width = -1;
    int is_sign = 0;

    while(*fmt && *str) 
	{
        /* skip any white space in format */
        /* white space in format matchs any amount of
         * white space, including none, in the input.
         */
        if (isspace(*fmt)) 
		{
            while (isspace(*fmt))
                ++fmt;
            while (isspace(*str))
                ++str;
        }

        /* anything that is not a conversion must match exactly */
        if (*fmt != '%' && *fmt) 
		{
            if (*fmt++ != *str++)
			{
				break;
			} 
            continue;
        }

        if (!*fmt)
		{
			break;
		}  
        ++fmt;
        
        /* skip this conversion.
         * advance both strings to next white space
         */
        if (*fmt == '*') 
		{
            while (!isspace(*fmt) && *fmt)
			{
				fmt++;
			}
            while (!isspace(*str) && *str)
			{
				str++;
			}               
            continue;
        }

        /* get field width */
        if (isdigit(*fmt))
		{
			field_width = atoi(fmt);
		}
        /* get conversion qualifier */
        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z') 
		{
            qualifier = *fmt;
            fmt++;
        }
        base = 10;
        is_sign = 0;

        if (!*fmt || !*str)
		{
			break;
		}
        switch(*fmt++) 
		{
			case 'c':
			{
				char *s = (char *) va_arg(args,char*);
				if (field_width == -1)
				{
					field_width = 1;
				}
				do 
				{
					*s++ = *str++;
				} while(field_width-- > 0 && *str);
				num++;
			}
			continue;
			case 's':
			{
				char *s = (char *) va_arg(args, char *);
				if(field_width == -1)
				{
					field_width = __INT_MAX__;
				}
				/* first, skip leading white space in buffer */
				while (isspace(*str))
				{
					str++;
				}
				/* now copy until next white space */
				while (*str && ((*str) != ',')) 
				{
					if(isspace(*str))
					{
						str++;
					}
					else
					{
						*s++ = *str++;
					}			
				}
				*s = '\0';
				num++;
			}
			continue;
			/* S for special handling for MQTTPUB JSON content */
			case 'S':
			{
				char *s = (char *) va_arg(args, char *);
				if(field_width == -1)
				{
					field_width = __INT_MAX__;
				}
				/* first, skip leading white space in buffer */
				while (isspace(*str))
				{
					str++;
				}
				/* now copy until next white space */
				while (*str) 
				{
					if(isspace(*str))
					{
						str++;
					}
					else
					{
						*s++ = *str++;
					}			
				}
				*s = '\0';
				num++;
			}
			continue;
			case 'n':
			{
				int *i = (int *)va_arg(args,int*);/* return number of characters read so far */
				*i = str - buf;
			}
			continue;
			case 'o':
				base = 8;
				break;
			case 'x':
			case 'X':
				base = 16;
				break;
			case 'd':
			case 'i':
				is_sign = 1;
			case 'u':
				break;
			case '%':
				/* looking for '%' in str */
				if (*str++ != '%') 
				{
					return num;
				}	
				continue;
			default:
				/* invalid format; stop here */
			return num;
        }

        /* have some sort of integer conversion.
         * first, skip white space in buffer.
         */
        while (isspace(*str))
		{
			str++;
		}  
        if (!*str || !isdigit(*str))
		{
			break;
		}
        switch(qualifier) 
		{
			case 'h':
				if (is_sign) 
				{
					short *s = (short *) va_arg(args,short *);
					*s = (short) strtol(str,&next,base);
				} 
				else 
				{
					unsigned short *s = (unsigned short *) va_arg(args, unsigned short *);
					*s = (unsigned short) strtoul(str, &next, base);
				}
				break;
			case 'l':
				if (is_sign) 
				{
					long *l = (long *) va_arg(args,long *);
					*l = strtol(str,&next,base);
				} 
				else 
				{
					unsigned long *l = (unsigned long*) va_arg(args,unsigned long*);
					*l = strtoul(str,&next,base);
				}
				break;
			case 'L':
				if (is_sign) 
				{
					long long *l = (long long*) va_arg(args,long long *);
					*l = strtoll(str,&next,base);
				} 
				else 
				{
					unsigned long long *l = (unsigned long long*) va_arg(args,unsigned long long*);
					*l = strtoull(str,&next,base);
				}
				break;
			case 'Z':
			{
				unsigned long *s = (unsigned long*) va_arg(args,unsigned long*);
				*s = (unsigned long) strtoul(str,&next,base);
			}
			break;
			default:
				if (is_sign) 
				{
					int *i = (int *) va_arg(args, int*);
					*i = (int) strtol(str,&next,base);
				} 
				else 
				{
					unsigned int *i = (unsigned int*) va_arg(args, unsigned int*);
					*i = (unsigned int) strtoul(str,&next,base);
				}
			break;
        }
        num++;

        if (!next)
		{
			break;
		}  
        str = next;
    }
    return num;
}

int at_req_parse_args(const char *req_args, const char *req_expr, ...)
{
    va_list args;
    int req_args_num = 0;

    CORE_ASSERT_ERR(req_args, -1);
    CORE_ASSERT_ERR(req_expr, -1);

    va_start(args, req_expr);

    req_args_num = at_sscanf(req_args, req_expr, args);
    va_end(args);

    return req_args_num;
}
/**
 * Get one line AT response buffer by line number.
 *
 * @param resp response object
 * @param resp_line line number, start from '1'
 *
 * @return != NULL: response line buffer
 *          = NULL: input response line error
 */
const char *at_resp_get_line(at_response_t *resp, uint32_t resp_line)
{
    char *resp_buf = resp->buf;
    char *resp_line_buf = NULL;
    uint32_t line_num = 1;

    CORE_ASSERT_ERR(resp, NULL);

    if (resp_line > resp->line_counts || resp_line <= 0)
    {
        at_log_error("AT response get line failed! Input response line(%d) error!", resp_line);
        return NULL;
    }

    for (line_num = 1; line_num <= resp->line_counts; line_num++)
    {
        if (resp_line == line_num)
        {
            resp_line_buf = resp_buf;
			//LOG_D("resp_line_buf==%s", resp_line_buf);
            return resp_line_buf;
        }
        resp_buf += strlen(resp_buf) + 1;
    }

    return NULL;
}

/**
 * Get one line AT response buffer by keyword
 *
 * @param resp response object
 * @param keyword query keyword
 *
 * @return != NULL: response line buffer
 *          = NULL: no matching data
 */
const char *at_resp_get_line_by_kw(at_response_t *resp, const char *keyword)
{
    char *resp_buf = resp->buf;
    char *keyword_buf = NULL;
    char *resp_line_buf = NULL;
    uint32_t line_num = 1;

    CORE_ASSERT_ERR(resp, NULL);
    CORE_ASSERT_ERR(keyword, NULL);

    for (line_num = 1; line_num <= resp->line_counts; line_num++)
    {
        if ((keyword_buf =strstr(resp_buf, keyword)))
        {
            resp_line_buf = keyword_buf;
            return resp_line_buf;
        }
        resp_buf += strlen(resp_buf) + 1;
    }

    return NULL;
}

/**
 * Get and parse AT response buffer arguments by line number.
 *
 * @param resp response object
 * @param resp_line line number, start from '1'
 * @param resp_expr response buffer expression
 *
 * @return -1 : input response line number error or get line buffer error
 *          0 : parsed without match
 *         >0 : the number of arguments successfully parsed
 */
int at_resp_parse_line_args(at_response_t *resp, uint32_t resp_line, const char *resp_expr, ...)
{
    va_list args;
    int resp_args_num = 0;
    const char *resp_line_buf = NULL;

    CORE_ASSERT_ERR(resp, -1);
    CORE_ASSERT_ERR(resp_expr, -1);

    if ((resp_line_buf = at_resp_get_line(resp, resp_line)) == NULL)
    {
        return -1;
    }
    va_start(args, resp_expr);
	at_log_info("resp_line_buf:%s\n", resp_line_buf);
    resp_args_num = at_sscanf(resp_line_buf, resp_expr, args);
    va_end(args);

    return resp_args_num;
}

/**
 * Get and parse AT response buffer arguments by keyword.
 *
 * @param resp response object
 * @param keyword query keyword
 * @param resp_expr response buffer expression
 *
 * @return -1 : input keyword error or get line buffer error
 *          0 : parsed without match
 *         >0 : the number of arguments successfully parsed
 */
int at_resp_parse_line_args_by_kw(at_response_t *resp, const char *keyword, const char *resp_expr, ...)
{
    va_list args;
    int resp_args_num = 0;
    const char *resp_line_buf = NULL;

    CORE_ASSERT_ERR(resp, -1);
    CORE_ASSERT_ERR(resp_expr, -1);

    if ((resp_line_buf = at_resp_get_line_by_kw(resp, keyword)) == NULL)
    {
        return -1;
    }    
    va_start(args, resp_expr);
    resp_args_num = at_sscanf(resp_line_buf, resp_expr, args);
    va_end(args);

    return resp_args_num;
}

/**
 * Send commands to AT server and wait response.
 *
 * @param client current AT client object
 * @param resp AT response object, using NULL when you don't care response
 * @param cmd_expr AT commands expression
 *
 * @return 0 : success
 *        -1 : response status error
 *        -2 : wait timeout
 *        -7 : enter AT CLI mode
 */
int at_obj_exec_cmd(at_client_t *client, at_response_t *resp, const char *cmd_expr, ...)
{
    va_list args;
    uint32_t cmd_size = 0;
    int32_t res = STATE_SUCCESS;
    const char *cmd = NULL;

	memset(nb_send_buf, 0, AIOT_SEND_CMD_MAX_SIZE);
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    if (client == NULL)
    {
        at_log_error("input AT Client object is NULL, please create or get AT Client object!");
        return STATE_FAILED_CLIENT_IS_NULL;
    }

    /* check AT CLI mode */
    if (client->status == AT_STATUS_CLI && resp)
    {
        return STATE_FAILED_RESPONSE_IS_NULL;
    }
    if (resp != NULL) 
	{
        sysdep->core_sysdep_mutex_lock(client->lock);
    }

    client->resp_status = AT_RESP_OK;

    if (resp != NULL)
    {
        resp->buf_len = 0;
        resp->line_counts = 0;
    }

    client->resp = resp;
    va_start(args, cmd_expr);
    at_vprintfln(cmd_expr, args);
    va_end(args);

    if (resp != NULL)
    {
        if (sysdep->core_sysdep_sem_take(client->resp_sem, resp->timeout) != STATE_SUCCESS)
        {
            cmd = at_get_last_cmd(&cmd_size);
            at_log_warn("execute command (%.*s) timeout (%d ticks)!\r\n", cmd_size, cmd, resp->timeout);
            client->resp_status = AT_RESP_TIMEOUT;
            res = STATE_FAILED_AT_RESPONSE_TIMEOUT;
            goto __exit;
        }
        if (client->resp_status != AT_RESP_OK)
        {
            cmd = at_get_last_cmd(&cmd_size);
            at_log_error("execute command (%.*s) failed!\r\n", cmd_size, cmd);
            res = STATE_FAILED_AT_RESPONSE_NOT_OK;
            goto __exit;
        }
    }

__exit:
    client->resp = NULL;
    if (resp != NULL) 
	{
        sysdep->core_sysdep_mutex_unlock(client->lock);
    }
    return res;
}

/**
 * Waiting for connection to external devices.
 *
 * @param client current AT client object
 * @param timeout millisecond for timeout
 *
 * @return 0 : success
 *        -2 : timeout
 *        -5 : no memory
 */
int at_client_obj_wait_connect(at_client_t *client, uint32_t timeout)
{
    int32_t res = STATE_SUCCESS;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    at_response_t *resp = NULL;
    uint64_t start_time = 0;

    if (client == NULL)
    {
        at_log_error("input AT client object is NULL, please create or get AT Client object!");
        return STATE_FAILED_CLIENT_IS_NULL;
    }

    resp = at_create_resp(64, 0, 1000, NULL, 0);
    if (resp == NULL)
    {
        at_log_error("no memory for AT client response object.");
        return STATE_FAILED_RAM_NOT_ENOUGH;
    }

    sysdep->core_sysdep_mutex_lock(client->lock);
    client->resp = resp;

    start_time = sysdep->core_sysdep_time();

    while (1)
    {
        /* Check whether it is timeout */
        if (sysdep->core_sysdep_time() - start_time > timeout)
        {
            at_log_error("wait AT client connect timeout(%d tick).", timeout);
            res = -STATE_FAILED_AT_RESPONSE_TIMEOUT;
            break;
        }
        /* Check whether it is already connected */
        resp->buf_len = 0;
        resp->line_counts = 0;
        sysdep->core_usart_send("AT\r\n", 4, AIOT_WAIT_MAX_TIME);
        if (sysdep->core_sysdep_sem_take(client->resp_sem, resp->timeout) != 0) 
		{
            continue;
        } 
		else 
		{
            at_log_info("Connect OK.\r\n");
            break;
        }
    }

    at_delete_resp(resp);
    client->resp = NULL;
    sysdep->core_sysdep_mutex_unlock(client->lock);
    return res;
}

/**
 * Send data to AT server, send data don't have end sign(eg: \r\n).
 *
 * @param client current AT client object
 * @param buf   send data buffer
 * @param size  send fixed data size
 *
 * @return >0: send data size
 *         =0: send failed
 */
uint32_t at_client_obj_send(at_client_t *client, const char *buf, uint32_t size)
{
    uint32_t len;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    CORE_ASSERT_ERR(buf, 0);

    if (client == NULL)
    {
        at_log_error("input AT Client object is NULL, please create or get AT Client object!");
        return 0;
    }

	#if (AIOT_AT_DUMP_RAW_CMD)
//		log_dump("sendline", buf, size);
	#endif

    sysdep->core_sysdep_mutex_lock(client->lock);
    len = sysdep->core_usart_send((char *)buf, size, AIOT_WAIT_MAX_TIME);
    sysdep->core_sysdep_mutex_unlock(client->lock);
    return len;
}

static int32_t at_client_getchar(at_client_t *client, char *ch, int32_t timeout)
{
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    uint64_t time = sysdep->core_sysdep_time();
    int32_t len = 0;
    while ((len = core_ringbuf_pop_char(&(client->ring_buff), (uint8_t *)ch)) ==  0)
    {   
        if (sysdep->core_sysdep_time() - time > timeout) 
		{
            return STATE_FAILED_NOT_READ_DATA;
        }
        if (client->thread_runing == 0) 
		{
            return STATE_FAILED_NOT_READ_DATA;
        }
        os_thread_sleep(os_msec_to_ticks(10));
    }
    return STATE_SUCCESS;
}

/**
 * AT client receive fixed-length data.
 *
 * @param client current AT client object
 * @param buf   receive data buffer
 * @param size  receive fixed data size
 * @param timeout  receive data timeout (ms)
 *
 * @note this function can only be used in execution function of URC data
 *
 * @return >0: receive data size
 *         =0: receive failed
 */
uint32_t at_client_obj_recv(at_client_t *client, char *buf, uint32_t size, int32_t timeout)
{
    uint32_t len = 0;
    uint32_t read_len = 0;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    uint64_t time = sysdep->core_sysdep_time();
    
    CORE_ASSERT_ERR(buf, 0);
    if (client == NULL)
    {
        at_log_error("input AT Client object is NULL, please create or get AT Client object!");
        return 0;
    }

    while (1)
    {
        read_len = core_ringbuf_read(&(client->ring_buff), (uint8_t *)(buf + len), size);
        if(read_len > 0)
        {
            len += read_len;
            size -= read_len;
            if(size == 0) 
			{
                break;
            }
            continue;
        }
        if (sysdep->core_sysdep_time() - time > timeout) 
		{
            return STATE_FAILED_NOT_READ_DATA;
        }
    }

#if (AIOT_AT_DUMP_RAW_CMD)
    at_log_dump("urc_recv", buf, len);
#endif
    return len;
}

/**
 *  AT client set end sign.
 *
 * @param client current AT client object
 * @param ch the end sign, can not be used when it is '\0'
 */
void at_obj_set_end_sign(at_client_t *client, char ch)
{
    if (client == NULL)
    {
        at_log_error("input AT Client object is NULL, please create or get AT Client object!");
        return;
    }

    client->end_sign = ch;
}

/**
 * set URC(Unsolicited Result Code) table
 *
 * @param client current AT client object
 * @param urc_table URC table
 * @param table_sz table size
 */
int at_obj_set_urc_table(at_client_t *client, const at_urc_t *urc_table, uint32_t table_sz)
{
    uint32_t idx;

    if (client == NULL)
    {
        at_log_error("input AT Client object is NULL, please create or get AT Client object!");
        return STATE_FAILED_CLIENT_IS_NULL;
    }

    for (idx = 0; idx < table_sz; idx++)
    {
        CORE_ASSERT_ERR(urc_table[idx].cmd_prefix, -1);
        CORE_ASSERT_ERR(urc_table[idx].cmd_suffix, -1);
    }

    client->urc_table = urc_table;
    client->urc_size = table_sz;

    return STATE_SUCCESS;
}

/**
 * get AT client object by AT device name.
 *
 * @dev_name AT client device name
 *
 * @return AT client object
 */
at_client_t *at_client_get()
{
    return &at_client_table;
}

static const at_urc_t *get_urc_obj(at_client_t *client)
{
    uint32_t j, prefix_len, suffix_len;
    uint32_t bufsz;
    char *buffer = NULL;
    const at_urc_t *urc = NULL;

    if (client->urc_table == NULL)
    {
        return NULL;
    }

    buffer = client->recv_line_buf;
    bufsz = client->recv_line_len;

    for (j = 0; j < client->urc_size; j++)
    {
        urc = client->urc_table + j;

        prefix_len = strlen(urc->cmd_prefix);
        suffix_len = strlen(urc->cmd_suffix);
        if (bufsz < prefix_len + suffix_len)
        {
            continue;
        }
        if ((prefix_len ? !strncmp(buffer, urc->cmd_prefix, prefix_len) : 1) && 
                (suffix_len ? !strncmp(buffer + bufsz - suffix_len, urc->cmd_suffix, suffix_len) : 1))
        {
            return urc;
        }
    }

    return NULL;
}

static int at_recv_readline(at_client_t *client)
{
    uint32_t read_len = 0;
    char ch = 0, last_ch = 0;
    uint8_t is_full = AIOT_FALSE;
    int32_t res = STATE_SUCCESS;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    memset(client->recv_line_buf, 0, client->recv_bufsz);
    client->recv_line_len = 0;
    while (1)
    {
        if ((res = at_client_getchar(client, &ch, AIOT_WAIT_MAX_TIME)) != STATE_SUCCESS) 
		{ 
            return STATE_FAILED_NOT_READ_DATA;
        }
        if (read_len < client->recv_bufsz)
        {
            client->recv_line_buf[read_len++] = ch;
            client->recv_line_len = read_len;
        }
        else
        {
            is_full = AIOT_TRUE;
        }

        /* is newline or URC data */
        if ((ch == '\n' && last_ch == '\r') || (client->end_sign != 0 && ch == client->end_sign)
                || get_urc_obj(client))
        {
            if (is_full)
            {
                at_log_error("read line failed. The line data length is out of buffer size(%d)!", client->recv_bufsz);
                memset(client->recv_line_buf, 0x00, client->recv_bufsz);
                client->recv_line_len = 0;
                return STATE_FAILED_AT_RECV_BUFFER_IS_FULL;
            }
            break;
        }
        last_ch = ch;
        os_thread_sleep(os_msec_to_ticks(10));
    }
	#if (AIOT_AT_DUMP_RAW_CMD)
		at_log_info("<<<<");
		at_log_dump("recvline", client->recv_line_buf, read_len);
	#endif
    return read_len;
}

static void client_parser(void *arg)
{
    const at_urc_t *urc;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    at_client_t *client = at_client_get();
    //log_debug("Entry client_parser\r\n");
    
    while(1)
    {
        if (client->thread_runing == 0) 
		{
            break;
        }
        if (at_recv_readline(client) > 0)
        {
            if ((urc = get_urc_obj(client)) != NULL)
            {
                /* current receive is request, try to execute related operations */
                if (urc->func != NULL)
                {
                    urc->func(/*client,*/ client->recv_line_buf, client->recv_line_len);
                }
            }
            else if (client->resp != NULL)
            {
                at_response_t *resp = client->resp;
                char end_ch = client->recv_line_buf[client->recv_line_len - 1];
                if (resp != NULL && resp->rsp_exp != NULL) 
				{
                    if (resp->buf_len + client->recv_line_len < resp->buf_size)
                    {
                        memcpy(resp->buf + resp->buf_len, client->recv_line_buf, client->recv_line_len);/* copy response lines, separated by '\0' */                        
                        resp->buf_len += client->recv_line_len;/* update the current response information */
                    }
                    else
                    {
                        client->resp_status = AT_RESP_BUFF_FULL;
                        at_log_error("Read response buffer failed. The Response buffer size is out of buffer size(%d)!", resp->buf_size);
                    }

                    uint32_t rsp_exp_len = strlen(resp->rsp_exp);
                    if (resp->buf_len >= rsp_exp_len && strstr(client->recv_line_buf, resp->rsp_exp) && resp->buf_len >= resp->rsp_exp_len) 
					{
                        resp->line_counts = 1;
                        resp->buf[resp->buf_len - 1] = '\0';
                    }
					else 
					{
                        continue;
                    }     
                } 
				else 
				{
					/* current receive is response */
					client->recv_line_buf[client->recv_line_len - 1] = '\0';
					if (resp->buf_len + client->recv_line_len < resp->buf_size)
					{
						/* copy response lines, separated by '\0' */
						memcpy(resp->buf + resp->buf_len, client->recv_line_buf, client->recv_line_len);
						/* update the current response information */
						resp->buf_len += client->recv_line_len;
						resp->line_counts++;
					}
					else
					{
						client->resp_status = AT_RESP_BUFF_FULL;
						at_log_error("Read response buffer failed. The Response buffer size is out of buffer size(%d)!", resp->buf_size);
					} 
                }
                /* check response res */
                if ((client->end_sign != 0) && (end_ch == client->end_sign) && (resp->line_num == 0))
                {
                    /* get the end sign, return response state END_OK.*/
                    client->resp_status = AT_RESP_OK;
                }
                else if (strstr(client->recv_line_buf, AT_RESP_END_OK) && resp->line_num == 0)
                {
                    /* get the end data by response res, return response state END_OK. */
                    client->resp_status = AT_RESP_OK;
                }
                else if (strstr(client->recv_line_buf, AT_RESP_END_ERROR)
                        || (memcmp(client->recv_line_buf, AT_RESP_END_FAIL, strlen(AT_RESP_END_FAIL)) == 0))
                {
                    client->resp_status = AT_RESP_ERROR;
                }
                else if (resp->line_counts == resp->line_num && resp->line_num)
                {
                    /* get the end data by response line, return response state END_OK.*/
                    client->resp_status = AT_RESP_OK;
                }
                else
                {
                    continue;
                }
                client->resp = NULL;
                sysdep->core_sysdep_sem_release(client->resp_sem);
            }
            else
            {
                /* 没有读到数据 */
            }
        }
        os_thread_sleep(os_msec_to_ticks(10));
    }
    sysdep->core_sysdep_thread_destroy(&client->thread_id);
}

/* initialize the client object parameters */
static int at_client_para_init(at_client_t *client)
{
    int32_t res = STATE_SUCCESS;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    char *buf = NULL;

    client->status = AT_STATUS_UNINITIALIZED;
   // LOG_D("### Start OS init.\r\n");
    buf = sysdep->core_sysdep_malloc(AIOT_DEFAULT_RING_BUFF_MAX_SIZE, CORE_AT_CLIENT_MODULE);
    if (buf == NULL) 
	{
        res = STATE_FAILED_RAM_NOT_ENOUGH;
        at_log_error("[ERROR] core_sysdep_malloc\r\n");
        goto exit;
    }
    core_ringbuf_init(&(client->ring_buff), buf, AIOT_DEFAULT_RING_BUFF_MAX_SIZE);
    client->recv_line_buf = sysdep->core_sysdep_malloc(AIOT_DEFAULT_RECV_PUB_MAX_SIZE, CORE_AT_CLIENT_MODULE);
    if (client->recv_line_buf == NULL) 
	{
        res = STATE_FAILED_RAM_NOT_ENOUGH;
        at_log_error("[ERROR] recv_line_buf\r\n");
        goto exit;
    }
    client->lock = sysdep->core_sysdep_mutex_init();
    if (client->lock == NULL) 
	{
        at_log_error("[ERROR] core_sysdep_mutex_init\r\n");
        res = STATE_FAILED_RAM_NOT_ENOUGH;
        goto exit;
    }
    client->resp_sem = sysdep->core_sysdep_sem_init(); 
    if (client->resp_sem == NULL) 
	{
        res = STATE_FAILED_RAM_NOT_ENOUGH;
        at_log_error("[ERROR] core_sysdep_sem_init\r\n");
        goto exit;
    }
    client->parser = client_parser;
    client->thread_runing = 1;
    client->thread_id = sysdep->core_sysdep_thread_create(client->parser,
                                                            client,
                                                            AIOT_DEFAULT_AT_THREAD_STACK_SIZE,
                                                            AIOT_DEFAULT_AT_THREAD_PRIOTITY);
    if (client->thread_id == NULL)
    {
        at_log_error("[ERROR] core_sysdep_thread_create\r\n");
        res = STATE_FAILED_CREATE_NEW_THREAD;
        goto exit;
    }

    client->urc_table = NULL;
    client->urc_size = 0;

    client->recv_line_len = 0;
    client->recv_bufsz = AIOT_DEFAULT_RECV_PUB_MAX_SIZE;

    client->resp = NULL;
    client->status = AT_STATUS_INITIALIZED;

    return res;
exit:
    if (client->ring_buff.buf != NULL) 
	{
        sysdep->core_sysdep_free(client->ring_buff.buf);
    }
    if (client->recv_line_buf != NULL) 
	{
        sysdep->core_sysdep_free(client->recv_line_buf);
    }
    if (client->lock != NULL) 
	{
        sysdep->core_sysdep_mutex_deinit(&client->lock);
    }
    if (client->resp_sem != NULL) 
	{
        sysdep->core_sysdep_sem_deinit(&client->resp_sem);
    }
    if (client->thread_id != NULL) 
	{
        sysdep->core_sysdep_thread_destroy(&(client->thread_id));
        client->thread_runing = 0;
    }
    return res;
}

/**
 * AT client initialize.
 *
 * @param dev_name AT client device name
 * @param recv_bufsz the maximum number of receive buffer length
 *
 * @return 0 : initialize success
 *        -1 : initialize failed
 *        -5 : no memory
 */
int at_client_init()
{
    int res = STATE_SUCCESS;
    at_client_t *client = NULL;
    client = at_client_get();
    if (client == NULL) 
	{
        return STATE_FAILED_CLIENT_IS_NULL;
    }
    if (client->status == AT_STATUS_INITIALIZED) 
	{
        return STATE_SUCCESS;
    }
    res = at_client_para_init(client);
    return res;
}

void at_client_deinit() 
{
    at_client_t *client = NULL;
    aiot_sysdep_portfile_t *sysdep = aiot_sysdep_get_portfile();
    //LOG_D("at_client_deinit\r\n");
    if (sysdep == NULL) 
	{
        return;
    }
    client = at_client_get();
    if (client == NULL) 
	{
        return;
    }
	if (client->status == AT_STATUS_UNINITIALIZED) 		
	{
		return;
	}
	client->status = AT_STATUS_UNINITIALIZED;
    client->thread_runing = 0;

    os_thread_sleep(os_msec_to_ticks(1000));
    if (client->ring_buff.buf != NULL) 
	{
        sysdep->core_sysdep_free(client->ring_buff.buf);
    }
    if (client->recv_line_buf != NULL) 
	{
        sysdep->core_sysdep_free(client->recv_line_buf);
    }
    if (client->lock != NULL) 
	{
        sysdep->core_sysdep_mutex_deinit(&client->lock);
    }
    if (client->resp_sem != NULL) 
	{
        sysdep->core_sysdep_sem_deinit(&client->resp_sem);
    }
    if (client->thread_id != NULL) 
	{
        sysdep->core_sysdep_thread_destroy(&client->thread_id);
    }
    client->urc_table = NULL;
    client->urc_size = 0;
}
