#ifndef _KEY_DIR_H_
#define _KEY_DIR_H_

#include "hc32_ll.h"

/* �����˲�ʱ��50ms, ��λ10ms
 ֻ��������⵽50ms״̬�������Ϊ��Ч����������Ͱ��������¼�
*/
#define BUTTON_FILTER_TIME 	5
#define BUTTON_LONG_TIME 	250		/* ����1�룬��Ϊ�����¼� */

/*
	ÿ��������Ӧ1��ȫ�ֵĽṹ�������
	���Ա������ʵ���˲��Ͷ��ְ���״̬�������
*/
typedef struct
{
	/* ������һ������ָ�룬ָ���жϰ����ַ��µĺ��� */
	uint8_t (*IsKeyDownFunc)(void); /* �������µ��жϺ���,1��ʾ���� */

	uint8_t Count;			/* �˲��������� */
	uint8_t FilterTime;		/* �˲�ʱ��(���255,��ʾ2550ms) */
	uint16_t LongCount;		/* ���������� */
	uint16_t LongTime;		/* �������³���ʱ��, 0��ʾ����ⳤ�� */
	uint8_t  State;			/* ������ǰ״̬�����»��ǵ��� */
	uint8_t KeyCodeUp;		/* ��������ļ�ֵ����, 0��ʾ����ⰴ������ */
	uint8_t KeyCodeDown;	/* �������µļ�ֵ����, 0��ʾ����ⰴ������ */
	uint8_t KeyCodeLong;	/* ���������ļ�ֵ����, 0��ʾ����ⳤ�� */
	uint8_t RepeatSpeed;	/* ������������ */
	uint8_t RepeatCount;	/* �������������� */
}BUTTON_T;

/* �����ֵ����
	�Ƽ�ʹ��enum, ����#define��ԭ��
	(1) ����������ֵ,�������˳��ʹ���뿴���������
	(2)	�������ɰ����Ǳ����ֵ�ظ���
*/
typedef enum
{
	KEY_NONE = 0,	/* 0 ��ʾ�����¼� */

	KEY_DOWN_MENU,	/* MENU������ */
	KEY_UP_MENU,	/* MENU������ */

	KEY_DOWN_UP,	/* Up������ */
	KEY_UP_UP,	    /* Up������ */

	KEY_DOWN_DOWN,	/* DOWN������ */
	KEY_UP_DOWN,	/* DOWN������ */

	KEY_DOWN_CLR,	/* CLR������ */
	KEY_UP_CLR,		/* CLR������ */

	KEY_DOWN_OK,	/* OK������ */
	KEY_UP_OK,		/* OK������ */
	
	KEY_DOWN_LONG_OK,	/* OK������ */
	
	KEY_DOWN_RST,	/* RST������ */
	KEY_UP_RST,		/* RST������ */
	
	KEY_DOWN_LONG_RST,	/* RST������ */
}KEY_ENUM;

/* ����ID */
enum
{
	KID_MENU = 0,
	KID_UP,
	KID_DOWN,
	KID_CLR,
	KID_OK,
	KID_RST,
};

/* ����FIFO�õ����� */
#define KEY_FIFO_SIZE	20
typedef struct
{
	uint8_t Buf[KEY_FIFO_SIZE];		/* ��ֵ������ */
	uint8_t Read;					/* ��������ָ�� */
	uint8_t Write;					/* ������дָ�� */
}KEY_FIFO_T;


void key_init(void);
void put_key_value(uint8_t _KeyCode);
uint8_t get_key_value(void);
void key_detect_all(void);
uint8_t read_key_state(uint8_t _ucKeyID);

#endif
