#include "sensor_in_dir.h"
#include "project_pin_use_config.h"
#include "sys_log.h"

void sensor_in_io_init(void)
{     
	stc_gpio_init_t stcGpioInit;

	(void)GPIO_StructInit(&stcGpioInit);
	
	stcGpioInit.u16PinState = PIN_STAT_SET;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(SPD_IN_PORT, SPD_IN_PIN, &stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_SET;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(DOOR_IN_PORT, DOOR_IN_PIN, &stcGpioInit);
	
	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(SMOKE_IN_PORT, SMOKE_IN_PIN, &stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(WATER_IM_IN_PORT, WATER_IM_IN_PIN, &stcGpioInit);

}

uint8_t read_smoke_sensor(void)
{
	if(GPIO_ReadInputPins(SMOKE_IN_PORT, SMOKE_IN_PIN)==PIN_RESET)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint8_t read_water_sensor(void)
{
	if(GPIO_ReadInputPins(WATER_IM_IN_PORT, WATER_IM_IN_PIN)==PIN_RESET)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint8_t read_lock_sensor(void)
{
	if(GPIO_ReadInputPins(DOOR_IN_PORT, DOOR_IN_PIN)==PIN_RESET)
	{
		return 1;
	}
	else
	{
	
		return 0;
	}
}

uint8_t read_spd_sensor(void)
{
	if(GPIO_ReadInputPins(SPD_IN_PORT, SPD_IN_PIN)==PIN_RESET)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

