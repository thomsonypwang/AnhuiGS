#include "led_dir.h"
#include "project_pin_use_config.h"

void led_arm_on(void)
{
	GPIO_SetPins(LED_ARM_PORT, LED_ARM_PIN);
}

void led_arm_off(void)
{
	GPIO_ResetPins(LED_ARM_PORT, LED_ARM_PIN);
}

void led_run_on(void)
{
	GPIO_SetPins(LED_RUN_PORT, LED_RUN_PIN);
}

void led_run_off(void)
{
	GPIO_ResetPins(LED_RUN_PORT, LED_RUN_PIN);
}

void led_net_on(void)
{
	GPIO_SetPins(LED_NET_PORT, LED_NET_PIN);
}

void led_net_off(void)
{
	GPIO_ResetPins(LED_NET_PORT, LED_NET_PIN);
}

void led_io_init(void)
{    
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);
	
    stcGpioInit.u16PinState = PIN_STAT_RST;
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    (void)GPIO_Init(LED_ARM_PORT, LED_ARM_PIN, &stcGpioInit);
	
	 stcGpioInit.u16PinState = PIN_STAT_RST;
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    (void)GPIO_Init(LED_RUN_PORT, LED_RUN_PIN, &stcGpioInit);
	
	 stcGpioInit.u16PinState = PIN_STAT_RST;
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    (void)GPIO_Init(LED_NET_PORT, LED_NET_PIN, &stcGpioInit);	
}
