#include "bat_dir.h"
#include "project_pin_use_config.h"
#include "sys_os.h"
#include "sys_log.h"

static void EXTINT_POWER_OFF_IrqCallback(void)
{
    if (SET == EXTINT_GetExtIntStatus(POWER_EN_EXTINT_CH)) 
	{
        if (GPIO_ReadInputPins(POWER_EN_PORT, POWER_EN_PIN)==PIN_SET) 
		{
			bat_en_off();
			device_data.power_flag=0;
        }
		else
		{
			bat_en_on();
			device_data.power_flag=1;
		}
        EXTINT_ClearExtIntStatus(POWER_EN_EXTINT_CH);
    }
}

void bat_io_init(void)
{    
	stc_gpio_init_t stcGpioInit;
	stc_extint_init_t stcExtIntInit;
	stc_irq_signin_config_t stcIrqSignConfig;
	
    (void)GPIO_StructInit(&stcGpioInit);
	GPIO_SetDebugPort(GPIO_PIN_DEBUG_JTAG,DISABLE);
	
    stcGpioInit.u16PinState = PIN_STAT_RST;
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    (void)GPIO_Init(BAT_5V_EN_PORT, BAT_5V_EN_PIN, &stcGpioInit);
	bat_en_off();
	
	(void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16ExtInt = PIN_EXTINT_ON;
    stcGpioInit.u16PullUp = PIN_PU_ON;
    (void)GPIO_Init(POWER_EN_PORT, POWER_EN_PIN, &stcGpioInit);
	GPIO_ExIntCmd(POWER_EN_PORT, POWER_EN_PIN,ENABLE);
	 /* ExtInt config */
    (void)EXTINT_StructInit(&stcExtIntInit);
    stcExtIntInit.u32Filter      = EXTINT_FILTER_ON;
    stcExtIntInit.u32FilterClock = EXTINT_FCLK_DIV8;
    stcExtIntInit.u32Edge = EXTINT_TRIG_BOTH;
    (void)EXTINT_Init(POWER_EN_EXTINT_CH, &stcExtIntInit);
	
	    /* IRQ sign-in */
    stcIrqSignConfig.enIntSrc = POWER_EN_INT_SRC;
    stcIrqSignConfig.enIRQn   = POWER_EN_INT_IRQn;
    stcIrqSignConfig.pfnCallback = &EXTINT_POWER_OFF_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);

    /* NVIC config */
    NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
    NVIC_SetPriority(stcIrqSignConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);
	
}


void bat_init(void)
{     
	bat_io_init();
}

void bat_en_on(void)
{
	GPIO_SetPins(BAT_5V_EN_PORT, BAT_5V_EN_PIN);
}

void bat_en_off(void)
{
	GPIO_ResetPins(BAT_5V_EN_PORT, BAT_5V_EN_PIN);
}

