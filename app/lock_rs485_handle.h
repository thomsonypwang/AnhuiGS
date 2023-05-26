#ifndef _LOCK_RS485_HANDLE_H_
#define _LOCK_RS485_HANDLE_H_

#include "hc32_ll.h"
#include "project_config.h"

void lock_process_init(void);

uint8_t get_lock_send_tcp_status(void);
void set_lock_send_tcp_status(uint8_t flag);
void set_lock_tcp_rev_status(uint8_t flag);
uint8_t get_lock_tcp_send_len(void);
uint8_t *get_lock_tcp_send_data(void);
void set_lock_tcp_send_data(uint8_t *buf,uint8_t len);
uint8_t get_lock_tcp_rev_len(void);
uint8_t *get_lock_tcp_rev_data(void);
void set_lock_tcp_rev_data(uint8_t *buf,uint8_t len);

#endif
