#include "key_dir.h"
#include "project_pin_use_config.h"

static BUTTON_T s_Btn_menu;	/* menu 键 */
static BUTTON_T s_Btn_up;	/* up 键 */
static BUTTON_T s_Btn_down;	/* down 键 */
static BUTTON_T s_Btn_clr;	/* clr键 */
static BUTTON_T s_Btn_ok;	/* ok键 */
static BUTTON_T s_Btn_rst;	/* rst键 */

static KEY_FIFO_T s_Key;		/* 按键FIFO变量,结构体 */

static void bsp_InitButtonVar(void);
static void bsp_InitButtonHard(void);
static void bsp_DetectButton(BUTTON_T *_pBtn);

/*

	定义函数判断按键是否按下，返回值1 表示按下，0表示未按下
*/
static uint8_t IsKeyDownMenu(void) 		{if (GPIO_ReadInputPins(MENU_KEY_PORT, MENU_KEY_PIN)== PIN_SET) return 0; return 1;}
static uint8_t IsKeyDownUp(void) 		{if (GPIO_ReadInputPins(UP_KEY_PORT, UP_KEY_PIN) == PIN_SET) return 0; return 1;}
static uint8_t IsKeyDownDown(void) 		{if (GPIO_ReadInputPins(DOWN_KEY_PORT, DOWN_KEY_PIN) == PIN_SET) return 0; return 1;}
static uint8_t IsKeyDownClr(void) 		{if (GPIO_ReadInputPins(CLR_KEY_PORT, CLR_KEY_PIN) == PIN_SET) return 0; return 1;}
static uint8_t IsKeyDownOk(void) 		{if (GPIO_ReadInputPins(OK_KEY_PORT, OK_KEY_PIN) == PIN_SET) return 0; return 1;}
static uint8_t IsKeyDownRST(void) 		{if (GPIO_ReadInputPins(RST_KEY_PORT, RST_KEY_PIN) == PIN_SET) return 0; return 1;}

/**********************************************************************************************************
*	函 数 名: key_hard
*	功能说明: 初始化按键硬件
*	形    参：无
*	返 回 值: 无
**********************************************************************************************************/
static void key_hard(void)
{
	stc_gpio_init_t stcGpioInit;

	(void)GPIO_StructInit(&stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(MENU_KEY_PORT, MENU_KEY_PIN, &stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(UP_KEY_PORT, UP_KEY_PIN, &stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(DOWN_KEY_PORT, DOWN_KEY_PIN, &stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(OK_KEY_PORT, OK_KEY_PIN, &stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(CLR_KEY_PORT, CLR_KEY_PIN, &stcGpioInit);
	
	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(RST_KEY_PORT, RST_KEY_PIN, &stcGpioInit);
}
	
/**********************************************************************************************************
*	函 数 名: key_var
*	功能说明: 初始化按键变量
*	形    参：strName : 例程名称字符串
*			  strDate : 例程发布日期
*	返 回 值: 无
**********************************************************************************************************/
static void key_var(void)
{
	/* 对按键FIFO读写指针清零 */
	s_Key.Read = 0;
	s_Key.Write = 0;

	/* 初始化Menu按键变量，支持按下、弹起、长按 */
	s_Btn_menu.IsKeyDownFunc = IsKeyDownMenu;		/* 判断按键按下的函数 */
	s_Btn_menu.FilterTime = BUTTON_FILTER_TIME;		/* 按键滤波时间 */
	s_Btn_menu.LongTime = 0;						/* 长按时间 */
	s_Btn_menu.Count = s_Btn_menu.FilterTime / 2;		/* 计数器设置为滤波时间的一半 */
	s_Btn_menu.State = 0;							/* 按键缺省状态，0为未按下 */
	s_Btn_menu.KeyCodeDown = KEY_DOWN_MENU;			/* 按键按下的键值代码 */
	s_Btn_menu.KeyCodeUp = KEY_UP_MENU;				/* 按键弹起的键值代码 */
	s_Btn_menu.KeyCodeLong = 0;						/* 按键被持续按下的键值代码 */
	s_Btn_menu.RepeatSpeed = 0;						/* 按键连发的速度，0表示不支持连发 */
	s_Btn_menu.RepeatCount = 0;						/* 连发计数器 */		

	/* 初始化Up按键变量，支持按下 */
	s_Btn_up.IsKeyDownFunc = IsKeyDownUp;	/* 判断按键按下的函数 */
	s_Btn_up.FilterTime = BUTTON_FILTER_TIME;	/* 按键滤波时间 */
	s_Btn_up.LongTime = 0;						/* 长按时间, 0表示不检测 */
	s_Btn_up.Count = s_Btn_up.FilterTime / 2;	/* 计数器设置为滤波时间的一半 */
	s_Btn_up.State = 0;							/* 按键缺省状态，0为未按下 */
	s_Btn_up.KeyCodeDown = KEY_DOWN_UP;		/* 按键按下的键值代码 */
	s_Btn_up.KeyCodeUp = KEY_UP_UP;			/* 按键弹起的键值代码 */
	s_Btn_up.KeyCodeLong = 0;					/* 按键被持续按下的键值代码 */
	s_Btn_up.RepeatSpeed = 0;					/* 按键连发的速度，0表示不支持连发 */
	s_Btn_up.RepeatCount = 0;					/* 连发计数器 */	

	/* 初始化down按键变量，支持按下 */
	s_Btn_down.IsKeyDownFunc = IsKeyDownDown;	/* 判断按键按下的函数 */
	s_Btn_down.FilterTime = BUTTON_FILTER_TIME;	/* 按键滤波时间 */
	s_Btn_down.LongTime = 0;						/* 长按时间, 0表示不检测 */
	s_Btn_down.Count = s_Btn_down.FilterTime / 2;	/* 计数器设置为滤波时间的一半 */
	s_Btn_down.State = 0;							/* 按键缺省状态，0为未按下 */
	s_Btn_down.KeyCodeDown = KEY_DOWN_DOWN;		/* 按键按下的键值代码 */
	s_Btn_down.KeyCodeUp = KEY_UP_DOWN;			/* 按键弹起的键值代码 */
	s_Btn_down.KeyCodeLong = 0;					/* 按键被持续按下的键值代码 */
	s_Btn_down.RepeatSpeed = 0;					/* 按键连发的速度，0表示不支持连发 */
	s_Btn_down.RepeatCount = 0;					/* 连发计数器 */	
	
	/* 初始化clr按键变量，支持按下 */
	s_Btn_clr.IsKeyDownFunc = IsKeyDownClr;	/* 判断按键按下的函数 */
	s_Btn_clr.FilterTime = BUTTON_FILTER_TIME;	/* 按键滤波时间 */
	s_Btn_clr.LongTime = 0;						/* 长按时间, 0表示不检测 */
	s_Btn_clr.Count = s_Btn_clr.FilterTime / 2;	/* 计数器设置为滤波时间的一半 */
	s_Btn_clr.State = 0;							/* 按键缺省状态，0为未按下 */
	s_Btn_clr.KeyCodeDown = KEY_DOWN_CLR;		/* 按键按下的键值代码 */
	s_Btn_clr.KeyCodeUp = KEY_UP_CLR;			/* 按键弹起的键值代码 */
	s_Btn_clr.KeyCodeLong = 0;					/* 按键被持续按下的键值代码 */
	s_Btn_clr.RepeatSpeed = 0;					/* 按键连发的速度，0表示不支持连发 */
	s_Btn_clr.RepeatCount = 0;					/* 连发计数器 */	
	
	/* 初始化ok按键变量，支持按下 */
	s_Btn_ok.IsKeyDownFunc = IsKeyDownOk;	/* 判断按键按下的函数 */
	s_Btn_ok.FilterTime = BUTTON_FILTER_TIME;	/* 按键滤波时间 */
	s_Btn_ok.LongTime = BUTTON_LONG_TIME;						/* 长按时间, 0表示不检测 */
	s_Btn_ok.Count = s_Btn_ok.FilterTime / 2;	/* 计数器设置为滤波时间的一半 */
	s_Btn_ok.State = 0;							/* 按键缺省状态，0为未按下 */
	s_Btn_ok.KeyCodeDown = KEY_DOWN_OK;		/* 按键按下的键值代码 */
	s_Btn_ok.KeyCodeUp = KEY_UP_OK;			/* 按键弹起的键值代码 */
	s_Btn_ok.KeyCodeLong = KEY_DOWN_LONG_OK;					/* 按键被持续按下的键值代码 */
	s_Btn_ok.RepeatSpeed = 0;					/* 按键连发的速度，0表示不支持连发 */
	s_Btn_ok.RepeatCount = 0;					/* 连发计数器 */	
	
	s_Btn_rst.IsKeyDownFunc = IsKeyDownRST;	/* 判断按键按下的函数 */
	s_Btn_rst.FilterTime = BUTTON_FILTER_TIME;	/* 按键滤波时间 */
	s_Btn_rst.LongTime = BUTTON_LONG_TIME;						/* 长按时间, 0表示不检测 */
	s_Btn_rst.Count = s_Btn_rst.FilterTime / 2;	/* 计数器设置为滤波时间的一半 */
	s_Btn_rst.State = 0;							/* 按键缺省状态，0为未按下 */
	s_Btn_rst.KeyCodeDown = KEY_DOWN_RST;		/* 按键按下的键值代码 */
	s_Btn_rst.KeyCodeUp = KEY_UP_RST;			/* 按键弹起的键值代码 */
	s_Btn_rst.KeyCodeLong = KEY_DOWN_LONG_RST;					/* 按键被持续按下的键值代码 */
	s_Btn_rst.RepeatSpeed = 0;					/* 按键连发的速度，0表示不支持连发 */
	s_Btn_rst.RepeatCount = 0;					/* 连发计数器 */
}

/**********************************************************************************************************
*	函 数 名: key_var
*	功能说明: 初始化按键
*	形    参：无
*	返 回 值: 无
**********************************************************************************************************/
void key_init(void)
{
	key_var();		/* 初始化按键变量 */
	key_hard();		/* 初始化按键硬件 */
}

/**********************************************************************************************************
*	函 数 名: put_key_value
*	功能说明: 将1个键值压入按键FIFO缓冲区。可用于模拟一个按键。
*	形    参：_KeyCode : 按键代码
*	返 回 值: 无
**********************************************************************************************************/
void put_key_value(uint8_t _KeyCode)
{
	s_Key.Buf[s_Key.Write] = _KeyCode;

	if (++s_Key.Write  >= KEY_FIFO_SIZE)
	{
		s_Key.Write = 0;
	}
}

/**********************************************************************************************************
*	函 数 名: get_key_value
*	功能说明: 从按键FIFO缓冲区读取一个键值。
*	形    参：无
*	返 回 值: 按键代码
**********************************************************************************************************/
uint8_t get_key_value(void)
{
	uint8_t ret;

	if (s_Key.Read == s_Key.Write)
	{
		return KEY_NONE;
	}
	else
	{
		ret = s_Key.Buf[s_Key.Read];

		if (++s_Key.Read >= KEY_FIFO_SIZE)
		{
			s_Key.Read = 0;
		}
		return ret;
	}
}

/**********************************************************************************************************
*	函 数 名: read_key_state
*	功能说明: 读取按键的状态
*	形    参：无
*	返 回 值: 无
**********************************************************************************************************/
uint8_t read_key_state(uint8_t _ucKeyID)
{
	uint8_t ucState = 0;

	switch (_ucKeyID)
	{
		case KID_MENU:
			ucState = s_Btn_menu.State;
			break;

		case KID_UP:
			ucState = s_Btn_up.State;
			break;

		case KID_DOWN:
			ucState = s_Btn_down.State;
			break;

		case KID_CLR:
			ucState = s_Btn_clr.State;
			break;

		case KID_OK:
			ucState = s_Btn_ok.State;
			break;
		case KID_RST:
			ucState = s_Btn_rst.State;
			break;
	}

	return ucState;
}


/**********************************************************************************************************
*	函 数 名: key_detect
*	功能说明: 检测一个按键。非阻塞状态，必须被周期性的调用。
*	形    参：按键结构变量指针
*	返 回 值: 无
**********************************************************************************************************/
static void key_detect(BUTTON_T *_pBtn)
{
	/* 如果没有初始化按键函数，则报错
	if (_pBtn->IsKeyDownFunc == 0)
	{
		printf("Fault : DetectButton(), _pBtn->IsKeyDownFunc undefine");
	}
	*/

	if (_pBtn->IsKeyDownFunc())
	{
		if (_pBtn->Count < _pBtn->FilterTime)
		{
			_pBtn->Count = _pBtn->FilterTime;
		}
		else if(_pBtn->Count < 2 * _pBtn->FilterTime)
		{
			_pBtn->Count++;
		}
		else
		{
			if (_pBtn->State == 0)
			{
				_pBtn->State = 1;

				/* 发送按钮按下的消息 */
				if (_pBtn->KeyCodeDown > 0)
				{
					/* 键值放入按键FIFO */
					put_key_value(_pBtn->KeyCodeDown);
				}
			}

			if (_pBtn->LongTime > 0)
			{
				if (_pBtn->LongCount < _pBtn->LongTime)
				{
					/* 发送按钮持续按下的消息 */
					if (++_pBtn->LongCount == _pBtn->LongTime)
					{
						/* 键值放入按键FIFO */
						put_key_value(_pBtn->KeyCodeLong);						
					}
				}
				else
				{
					if (_pBtn->RepeatSpeed > 0)
					{
						if (++_pBtn->RepeatCount >= _pBtn->RepeatSpeed)
						{
							_pBtn->RepeatCount = 0;
							/* 常按键后，每隔10ms发送1个按键 */
							put_key_value(_pBtn->KeyCodeDown);														
						}
					}
				}
			}
		}
	}
	else
	{
		if(_pBtn->Count > _pBtn->FilterTime)
		{
			_pBtn->Count = _pBtn->FilterTime;
		}
		else if(_pBtn->Count != 0)
		{
			_pBtn->Count--;
		}
		else
		{
			if (_pBtn->State == 1)
			{
				_pBtn->State = 0;

				/* 发送按钮弹起的消息 */
				if (_pBtn->KeyCodeUp > 0)
				{
					/* 键值放入按键FIFO */
					put_key_value(_pBtn->KeyCodeUp);			
				}
			}
		}

		_pBtn->LongCount = 0;
		_pBtn->RepeatCount = 0;
	}
}

/*
*********************************************************************************************************
*	函 数 名: key_detect_all
*	功能说明: 检测所有按键。非阻塞状态，必须被周期性的调用。
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void key_detect_all(void)
{
	key_detect(&s_Btn_menu);	/* menu 键 */
	key_detect(&s_Btn_up);		/* up 键 */
	key_detect(&s_Btn_down);	/* down 键 */
	key_detect(&s_Btn_clr);		/* clr键 */
	key_detect(&s_Btn_ok);		/* ok键 */
	key_detect(&s_Btn_rst);		/* rst键 */
}

