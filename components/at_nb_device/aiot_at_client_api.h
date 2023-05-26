#ifndef __AT_CLIENT_H__
#define __AT_CLIENT_H__


#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include "core_stdinc.h"
#include <core_config.h>
#include "core_ring_buffer.h"

#define AT_SW_VERSION                  "1.3.1"

#define AT_CMD_NAME_LEN                16
#define AT_END_MARK_LEN                4

#define AT_CMD_END_MARK                "\r\n"

typedef enum 
{
    AT_STATUS_UNINITIALIZED = 0,
    AT_STATUS_INITIALIZED,
    AT_STATUS_CLI,
} at_status_t;

typedef enum 
{
    AT_RESP_OK = 0,                   /* AT response end is OK */
    AT_RESP_ERROR = -1,               /* AT response end is ERROR */
    AT_RESP_TIMEOUT = -2,             /* AT response is timeout */
    AT_RESP_BUFF_FULL= -3,            /* AT response buffer is full */
} at_resp_status_t;

typedef struct 
{
    /* response buffer */
    char *buf;
    /* the maximum response buffer size, it set by `at_create_resp()` function */
    int32_t buf_size;
    /* the length of current response buffer */
    int32_t buf_len;
    /* the number of setting response lines, it set by `at_create_resp()` function
     * == 0: the response data will auto return when received 'OK' or 'ERROR'
     * != 0: the response data will return when received setting lines number data */
    int32_t line_num;
    /* the count of received response lines */
    int32_t line_counts;
    /* the maximum response time */
    uint32_t timeout;
    /* 期望读取到的关键字 */
    char *rsp_exp;
    /* 期望读取到的长度 */
    uint32_t rsp_exp_len;
} at_response_t;

typedef struct 
{
    const char *cmd_prefix;
    const char *cmd_suffix;
    void (*func)(const char *data, uint32_t size);
} at_urc_t;

typedef void (*_parse_function)(void *user_data);

typedef struct
{
    at_status_t status;
    char end_sign;
    core_ringbuf_t ring_buff;
    char *recv_line_buf;
    uint32_t recv_line_len;
    uint32_t recv_bufsz;

    void *lock;

    at_response_t *resp;
    at_resp_status_t resp_status;
	void *resp_sem;

    const at_urc_t *urc_table;
    uint32_t urc_size;
    
    _parse_function parser;
    void *thread_id;
    uint32_t thread_runing;
} at_client_t;

/* AT client initialize and start*/
int at_client_init();
void at_client_deinit();
/* ========================== multiple AT client function ============================ */

/* get AT client object */
at_client_t *at_client_get();

/* AT client wait for connection to external devices. */
int at_client_obj_wait_connect(at_client_t *client, uint32_t timeout);

/* AT client send or receive data */
uint32_t at_client_obj_send(at_client_t *client, const char *buf, uint32_t size);
uint32_t at_client_obj_recv(at_client_t *client, char *buf, uint32_t size, int32_t timeout);

/* set AT client a line end sign */
void at_obj_set_end_sign(at_client_t *client, char ch);

/* Set URC(Unsolicited Result Code) table */
int at_obj_set_urc_table(at_client_t *client, const at_urc_t *urc_table, uint32_t table_sz);

/* AT client send commands to AT server and waiter response */
int at_obj_exec_cmd(at_client_t *client, at_response_t *resp, const char *cmd_expr, ...);

/* AT response object create and delete */
at_response_t *at_create_resp(uint32_t buf_size, uint32_t line_num, int32_t timeout, const char *rsp_exp, uint32_t rsp_exp_len);
void at_delete_resp(at_response_t *resp);

/* AT response line buffer get and parse response buffer arguments */
const char *at_resp_get_line(at_response_t *resp, uint32_t resp_line);
const char *at_resp_get_line_by_kw(at_response_t *resp, const char *keyword);
int at_resp_parse_line_args(at_response_t *resp, uint32_t resp_line, const char *resp_expr, ...);
int at_resp_parse_line_args_by_kw(at_response_t *resp, const char *keyword, const char *resp_expr, ...);

/* ========================== single AT client function ============================ */

/**
 * NOTE: These functions can be used directly when there is only one AT client.
 * If there are multiple AT Client in the program, these functions can operate on the first initialized AT client.
 */

#define at_exec_cmd(resp, ...)                   at_obj_exec_cmd(at_client_get(), resp, __VA_ARGS__)
#define at_client_wait_connect(timeout)          at_client_obj_wait_connect(at_client_get(), timeout)
#define at_client_send(buf, size)                at_client_obj_send(at_client_get(), buf, size)
#define at_set_end_sign(ch)                      at_obj_set_end_sign(at_client_get(), ch)
#define at_set_urc_table(urc_table, table_sz)    at_obj_set_urc_table(at_client_get(), urc_table, table_sz)

int at_req_parse_args(const char *req_args, const char *req_expr, ...);

#ifdef __cplusplus
}
#endif

#endif /* __AT_CLIENT_H__ */
