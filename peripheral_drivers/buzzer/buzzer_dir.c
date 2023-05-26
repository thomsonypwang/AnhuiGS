#include "buzzer_dir.h"
#include "project_pin_use_config.h"


void buzzer_on(void)
{
	GPIO_SetPins(BUZZER_PORT, BUZZER_PIN);
}

void buzzer_off(void)
{
	GPIO_ResetPins(BUZZER_PORT, BUZZER_PIN);
}

void buzzer_io_init(void)
{    
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);
	
    stcGpioInit.u16PinState = PIN_STAT_RST;
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    (void)GPIO_Init(BUZZER_PORT, BUZZER_PIN, &stcGpioInit);
	buzzer_off();
}
