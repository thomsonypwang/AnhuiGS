#ifndef _KEY_DIR_H_
#define _KEY_DIR_H_

#include "hc32_ll.h"

/* 按键滤波时间50ms, 单位10ms
 只有连续检测到50ms状态不变才认为有效，包括弹起和按下两种事件
*/
#define BUTTON_FILTER_TIME 	5
#define BUTTON_LONG_TIME 	250		/* 持续1秒，认为长按事件 */

/*
	每个按键对应1个全局的结构体变量。
	其成员变量是实现滤波和多种按键状态所必须的
*/
typedef struct
{
	/* 下面是一个函数指针，指向判断按键手否按下的函数 */
	uint8_t (*IsKeyDownFunc)(void); /* 按键按下的判断函数,1表示按下 */

	uint8_t Count;			/* 滤波器计数器 */
	uint8_t FilterTime;		/* 滤波时间(最大255,表示2550ms) */
	uint16_t LongCount;		/* 长按计数器 */
	uint16_t LongTime;		/* 按键按下持续时间, 0表示不检测长按 */
	uint8_t  State;			/* 按键当前状态（按下还是弹起） */
	uint8_t KeyCodeUp;		/* 按键弹起的键值代码, 0表示不检测按键弹起 */
	uint8_t KeyCodeDown;	/* 按键按下的键值代码, 0表示不检测按键按下 */
	uint8_t KeyCodeLong;	/* 按键长按的键值代码, 0表示不检测长按 */
	uint8_t RepeatSpeed;	/* 连续按键周期 */
	uint8_t RepeatCount;	/* 连续按键计数器 */
}BUTTON_T;

/* 定义键值代码
	推荐使用enum, 不用#define，原因：
	(1) 便于新增键值,方便调整顺序，使代码看起来舒服点
	(2)	编译器可帮我们避免键值重复。
*/
typedef enum
{
	KEY_NONE = 0,	/* 0 表示按键事件 */

	KEY_DOWN_MENU,	/* MENU键按下 */
	KEY_UP_MENU,	/* MENU键弹起 */

	KEY_DOWN_UP,	/* Up键按下 */
	KEY_UP_UP,	    /* Up键弹起 */

	KEY_DOWN_DOWN,	/* DOWN键按下 */
	KEY_UP_DOWN,	/* DOWN键弹起 */

	KEY_DOWN_CLR,	/* CLR键按下 */
	KEY_UP_CLR,		/* CLR键弹起 */

	KEY_DOWN_OK,	/* OK键按下 */
	KEY_UP_OK,		/* OK键弹起 */
	
	KEY_DOWN_LONG_OK,	/* OK键按下 */
	
	KEY_DOWN_RST,	/* RST键按下 */
	KEY_UP_RST,		/* RST键弹起 */
	
	KEY_DOWN_LONG_RST,	/* RST键按下 */
}KEY_ENUM;

/* 按键ID */
enum
{
	KID_MENU = 0,
	KID_UP,
	KID_DOWN,
	KID_CLR,
	KID_OK,
	KID_RST,
};

/* 按键FIFO用到变量 */
#define KEY_FIFO_SIZE	20
typedef struct
{
	uint8_t Buf[KEY_FIFO_SIZE];		/* 键值缓冲区 */
	uint8_t Read;					/* 缓冲区读指针 */
	uint8_t Write;					/* 缓冲区写指针 */
}KEY_FIFO_T;


void key_init(void);
void put_key_value(uint8_t _KeyCode);
uint8_t get_key_value(void);
void key_detect_all(void);
uint8_t read_key_state(uint8_t _ucKeyID);

#endif
