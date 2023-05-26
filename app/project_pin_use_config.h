/*
* Copyright (c) 2022 ,深圳市科安科技有限公司
* All rights reserved
*文件名称：
*摘要：
*当前版本：1.0   
*编写者；何东达 
*编写时间：2023.1.11 
*/
#ifndef _PROJECT_PIN_USE_CONFIG_H_
#define _PROJECT_PIN_USE_CONFIG_H_

#include "hc32_ll_qspi.h"
#include "hc32_ll_aos.h"
#include "hc32_ll_clk.h"
#include "hc32_ll_dma.h"
#include "hc32_ll_efm.h"
#include "hc32_ll_fcg.h"
#include "hc32_ll_gpio.h"
#include "hc32_ll_i2c.h"
#include "hc32_ll_i2s.h"
#include "hc32_ll_interrupts.h"
#include "hc32_ll_keyscan.h"
#include "hc32_ll_pwc.h"
#include "hc32_ll_spi.h"
#include "hc32_ll_usart.h"
#include "hc32_ll_utility.h"


#define PROJECT_PERIPH_WE               (LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | \
                                         LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM)
#define PROJECT_PERIPH_WP               (LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_SRAM)

#define BSP_XTAL_PORT          	GPIO_PORT_H
#define BSP_XTAL_IN_PIN        	GPIO_PIN_01
#define BSP_XTAL_OUT_PIN       	GPIO_PIN_00
///////////////////////////DBG使用IO脚情况//////////////////////
#define DBG_TX_PIN     			GPIO_PIN_09
#define DBG_TX_PORT				GPIO_PORT_A
#define DBG_TX_FUNC            		(GPIO_FUNC_34)

#define DBG_RX_PIN     			GPIO_PIN_10
#define DBG_RX_PORT				GPIO_PORT_A
#define DBG_RX_FUNC            		(GPIO_FUNC_35)

#define DBG_DEVICE_FCG           	(FCG3_PERIPH_USART5)
#define DBG_DEVICE               	(CM_USART5)
#define DBG_BAUDRATE             	(115200UL)
#define DBG_BAUDRATE_ERR_MAX     	(0.025F)
///////////////////////////LED使用IO脚情况//////////////////////////
#define LED_ARM_PIN      		GPIO_PIN_08
#define LED_ARM_PORT			GPIO_PORT_E

#define LED_RUN_PIN      		GPIO_PIN_09
#define LED_RUN_PORT			GPIO_PORT_E

#define LED_NET_PIN      		GPIO_PIN_10
#define LED_NET_PORT			GPIO_PORT_E
////////////////////////key的IO使用////////////////////////////////
#define MENU_KEY_PIN      		GPIO_PIN_05	
#define MENU_KEY_PORT			GPIO_PORT_C

#define UP_KEY_PIN      		GPIO_PIN_00	
#define UP_KEY_PORT				GPIO_PORT_B

#define DOWN_KEY_PIN      		GPIO_PIN_02	
#define DOWN_KEY_PORT			GPIO_PORT_B

#define OK_KEY_PIN      		GPIO_PIN_01	
#define OK_KEY_PORT				GPIO_PORT_B

#define CLR_KEY_PIN      		GPIO_PIN_07	
#define CLR_KEY_PORT			GPIO_PORT_E

#define RST_KEY_PIN      		GPIO_PIN_02	
#define RST_KEY_PORT			GPIO_PORT_C
///////////////////////////OLED使用IO脚情况//////////////////////////
#define OLED_DC_PIN      		GPIO_PIN_11
#define OLED_DC_PORT			GPIO_PORT_E

#define OLED_RES_PIN      		GPIO_PIN_12
#define OLED_RES_PORT			GPIO_PORT_E

#define OLED_SDA_PIN      		GPIO_PIN_10
#define OLED_SDA_PORT			GPIO_PORT_B
#define OLED_SDA_FUNC      			(GPIO_FUNC_41)

#define OLED_SCLK_PIN      		GPIO_PIN_11
#define OLED_SCLK_PORT			GPIO_PORT_B
#define OLED_SCLK_FUNC      		(GPIO_FUNC_40)

#define OLED_SPI_ID     			(CM_SPI1)
#define OLED_CLK_ID             	(FCG1_PERIPH_SPI1)
///////////////////////////温湿度传感器使用IO脚情况/////////////////
#define AM2301A_SDA_PIN     	GPIO_PIN_00
#define AM2301A_SDA_PORT		GPIO_PORT_C
///////////////////////////输入传感器使用IO脚情况//////////////////////////
#define SMOKE_IN_PIN     		GPIO_PIN_08
#define SMOKE_IN_PORT			GPIO_PORT_B

#define WATER_IM_IN_PIN    		GPIO_PIN_09
#define WATER_IM_IN_PORT		GPIO_PORT_B

#define DOOR_IN_PIN    			GPIO_PIN_06
#define DOOR_IN_PORT			GPIO_PORT_B

#define SPD_IN_PIN    			GPIO_PIN_05
#define SPD_IN_PORT				GPIO_PORT_B

///////////////////////////蜂鸣器使用IO脚情况//////////////////////////
#define BUZZER_PIN     			GPIO_PIN_06
#define BUZZER_PORT				GPIO_PORT_E

///////////////////////////电源使用IO脚情况//////////////////////
#define POWER_EN_PIN     		GPIO_PIN_02
#define POWER_EN_PORT			GPIO_PORT_A
#define POWER_EN_EXTINT_CH       	(EXTINT_CH02)
#define POWER_EN_INT_IRQn          	(INT000_IRQn)
#define POWER_EN_INT_SRC           	(INT_SRC_PORT_EIRQ0)

#define BAT_5V_EN_PIN      		GPIO_PIN_07
#define BAT_5V_EN_PORT			GPIO_PORT_B
///////////////////////////25Q64使用IO脚情况//////////////////////
#define W25Q64_CS_PIN     		GPIO_PIN_07
#define W25Q64_CS_PORT			GPIO_PORT_C
#define W25Q64_CS_FUNC      		(GPIO_FUNC_18)

#define W25Q64_SCLK_PIN     	GPIO_PIN_06
#define W25Q64_SCLK_PORT		GPIO_PORT_C
#define W25Q64_SCLK_FUNC      		(GPIO_FUNC_18)

#define W25Q64_DATA3_PIN    	GPIO_PIN_11
#define W25Q64_DATA3_PORT		GPIO_PORT_D
#define W25Q64_DATA3_FUNC      		(GPIO_FUNC_18)

#define W25Q64_DATA2_PIN    	GPIO_PIN_10
#define W25Q64_DATA2_PORT		GPIO_PORT_D
#define W25Q64_DATA2_FUNC      		(GPIO_FUNC_18)

#define W25Q64_DATA1_PIN    	GPIO_PIN_09
#define W25Q64_DATA1_PORT		GPIO_PORT_D
#define W25Q64_DATA1_FUNC      		(GPIO_FUNC_18)

#define W25Q64_DATA0_PIN    	GPIO_PIN_08
#define W25Q64_DATA0_PORT		GPIO_PORT_D
#define W25Q64_DATA0_FUNC      		(GPIO_FUNC_18)
///////////////////////////W5500使用IO脚情况//////////////////////
#define W5500_REST_PIN     		GPIO_PIN_01
#define W5500_REST_PORT			GPIO_PORT_D

#define W5500_INT_PIN     		GPIO_PIN_00
#define W5500_INT_PORT			GPIO_PORT_D

#define W5500_CS_PIN     		GPIO_PIN_02
#define W5500_CS_PORT			GPIO_PORT_D
#define W5500_CS_FUNC     			(GPIO_FUNC_46)

#define W5500_MOSI_PIN     		GPIO_PIN_12
#define W5500_MOSI_PORT			GPIO_PORT_C
#define W5500_MOSI_FUNC     		(GPIO_FUNC_44)

#define W5500_MISO_PIN     		GPIO_PIN_11
#define W5500_MISO_PORT			GPIO_PORT_C
#define W5500_MISO_FUNC     		(GPIO_FUNC_45)

#define W5500_SCLK_PIN     		GPIO_PIN_10
#define W5500_SCLK_PORT			GPIO_PORT_C
#define W5500_SCLK_FUNC     		(GPIO_FUNC_43)

#define W5500_SPI_ID     			(CM_SPI4)
#define W5500_CLK_ID            	(FCG1_PERIPH_SPI4)

///////////////////////////modbus RS485使用IO脚情况//////////////////////	
#define MOD_485_TX_PIN     		GPIO_PIN_05
#define MOD_485_TX_PORT			GPIO_PORT_E
#define MOD_485_TX_FUNC     		(GPIO_FUNC_32)

#define MOD_485_RX_PIN     		GPIO_PIN_04
#define MOD_485_RX_PORT			GPIO_PORT_E
#define MOD_485_RX_FUNC     		(GPIO_FUNC_33)

#define MOD_485_USART_ID     		(CM_USART4)
#define MOD_485_USART_FCG       	(FCG3_PERIPH_USART4)
#define MOD_USART_BAUDRATE        	(9600)

#define MOD_RX_ERR_IRQn      		(INT001_IRQn)
#define MOD_RX_ERR_INT_SRC   		(INT_SRC_USART4_EI)

#define MOD_RX_FULL_IRQn      		(INT002_IRQn)
#define MOD_RX_FULL_INT_SRC   		(INT_SRC_USART4_RI)

/////////////////////////////自动重合闸 RS485使用IO脚情况//////////////////////
#define AUTOCLOSING_TX_PIN		GPIO_PIN_00
#define AUTOCLOSING_TX_PORT		GPIO_PORT_A
#define AUTOCLOSING_TX_FUNC			(GPIO_FUNC_38)

#define AUTOCLOSING_RX_PIN		GPIO_PIN_01
#define AUTOCLOSING_RX_PORT		GPIO_PORT_A
#define AUTOCLOSING_RX_FUNC			(GPIO_FUNC_39)

#define AUTOCLOSING_USART_ID		(CM_USART7)
#define AUTOCLOSING_USART_FCG       (FCG3_PERIPH_USART7)
#define AUTOCLOSING_USART_BAUDRATE  (9600)
#define CIRCUIT_BREAKER_USART_BAUDRATE  (19200)

#define AUTOCLOSING_USART_TMR0      CM_TMR0_2
#define AUTOCLOSING_USART_TMR0_CH   TMR0_CH_B
#define AUTOCLOSING_USART_TMR0_TIMEROUT   100
#define AUTOCLOSING_USART_TMR0_FCG  (FCG2_PERIPH_TMR0_2)

#define AUTOCLOSING_RX_ERR_IRQn     (INT003_IRQn)
#define AUTOCLOSING_RX_ERR_INT_SRC  (INT_SRC_USART7_EI)

#define AUTOCLOSING_RX_FULL_IRQn    (INT004_IRQn)
#define AUTOCLOSING_RX_FULL_INT_SRC (INT_SRC_USART7_RI)

#define AUTOCLOSING_RX_TIMEOUT_IRQn (INT005_IRQn)
#define AUTOCLOSING_RX_TIMEOUT_INT_SRC (INT_SRC_USART7_RTO)
///////////////////////////NBIOT使用IO脚情况//////////////////////
#define NBIOT_TX_PIN			GPIO_PIN_11
#define NBIOT_TX_PORT			GPIO_PORT_A
#define NBIOT_TX_FUNC     			(GPIO_FUNC_37)

#define NBIOT_RX_PIN    		GPIO_PIN_12
#define NBIOT_RX_PORT			GPIO_PORT_A
#define NBIOT_RX_FUNC     			(GPIO_FUNC_36)

#define NBIOT_POWER_PIN     	GPIO_PIN_05
#define NBIOT_POWER_PORT		GPIO_PORT_D

#define NBIOT_USART_ID     			(CM_USART6)
#define NBIOT_USART_FCG           	(FCG3_PERIPH_USART6)
#define NBIOT_USART_BAUDRATE        (9600)

#define NBIOT_USART_TMR0_FCG  		(FCG2_PERIPH_TMR0_2)
#define NBIOT_USART_TMR0           	CM_TMR0_2
#define NBIOT_USART_TMR0_CH        	TMR0_CH_A
#define NBIOT_USART_TMR0_TIMEROUT   1000

#define NBIOT_RX_ERR_IRQn           (INT006_IRQn)
#define NBIOT_RX_ERR_INT_SRC        (INT_SRC_USART6_EI)

#define NBIOT_RX_FULL_IRQn          (INT007_IRQn)
#define NBIOT_RX_FULL_INT_SRC       (INT_SRC_USART6_RI)

#define NBIOT_RX_TIMEOUT_IRQn       (INT008_IRQn)
#define NBIOT_RX_TIMEOUT_INT_SRC    (INT_SRC_USART6_RTO)

///////////////////////////lock使用IO脚情况//////////////////////
#define LOCK_TX_PIN				GPIO_PIN_13
#define LOCK_TX_PORT			GPIO_PORT_B
#define LOCK_TX_FUNC     			(GPIO_FUNC_34)

#define LOCK_RX_PIN    			GPIO_PIN_14
#define LOCK_RX_PORT			GPIO_PORT_B
#define LOCK_RX_FUNC     			(GPIO_FUNC_35)

#define LOCK_USART_ID     			(CM_USART2)
#define LOCK_USART_BAUDRATE        	(9600)
#define LOCK_USART_FCG           	(FCG3_PERIPH_USART2)

#define LOCK_USART_TMR0_FCG  		(FCG2_PERIPH_TMR0_1)
#define LOCK_USART_TMR0           	CM_TMR0_1
#define LOCK_USART_TMR0_CH        	TMR0_CH_B
#define LOCK_USART_TMR0_TIMEROUT   	1000

#define LOCK_RX_ERR_IRQn            (INT009_IRQn)
#define LOCK_RX_ERR_INT_SRC         (INT_SRC_USART2_EI)

#define LOCK_RX_FULL_IRQn           (INT010_IRQn)
#define LOCK_RX_FULL_INT_SRC        (INT_SRC_USART2_RI)

#define LOCK_RX_TIMEOUT_IRQn        (INT011_IRQn)
#define LOCK_RX_TIMEOUT_INT_SRC     (INT_SRC_USART2_RTO)
///////////////////////////GPS使用IO脚情况//////////////////////
#define GPS_TX_PIN     			GPIO_PIN_14
#define GPS_TX_PORT				GPIO_PORT_D
#define GPS_TX_FUNC     			(GPIO_FUNC_32)

#define GPS_RX_PIN     			GPIO_PIN_13
#define GPS_RX_PORT				GPIO_PORT_D
#define GPS_RX_FUNC     			(GPIO_FUNC_33)

#define GPS_USART_ID     			(CM_USART3)
#define GPS_USART_FCG           	(FCG3_PERIPH_USART3)
#define GPS_USART_BAUDRATE        	(9600)

#define GPS_RX_ERR_IRQn      		(INT012_IRQn)
#define GPS_RX_ERR_INT_SRC   		(INT_SRC_USART3_EI)

#define GPS_RX_FULL_IRQn      		(INT013_IRQn)
#define GPS_RX_FULL_INT_SRC   		(INT_SRC_USART3_RI)
//////////////////////Camera RS232/////////////////////////////////////
#define CAMERA_TX_PIN			GPIO_PIN_01
#define CAMERA_TX_PORT			GPIO_PORT_E
#define CAMERA_TX_FUNC     			(GPIO_FUNC_32)

#define CAMERA_RX_PIN    		GPIO_PIN_00
#define CAMERA_RX_PORT			GPIO_PORT_E
#define CAMERA_RX_FUNC     			(GPIO_FUNC_33)

#define CAMERA_USART_ID     		(CM_USART1)
#define CAMERA_USART_BAUDRATE        (115200)
#define CAMERA_USART_FCG           	(FCG3_PERIPH_USART1)

#define CAMERA_USART_TMR0_FCG  		(FCG2_PERIPH_TMR0_1)
#define CAMERA_USART_TMR0			CM_TMR0_1
#define CAMERA_USART_TMR0_CH        TMR0_CH_A
#define CAMERA_USART_TMR0_TIMEROUT   1000

#define CAMERA_RX_ERR_IRQn           (INT014_IRQn)
#define CAMERA_RX_ERR_INT_SRC        (INT_SRC_USART1_EI)

#define CAMERA_RX_FULL_IRQn          (INT015_IRQn)
#define CAMERA_RX_FULL_INT_SRC       (INT_SRC_USART1_RI)

#define CAMERA_RX_TIMEOUT_IRQn       (INT016_IRQn)
#define CAMERA_RX_TIMEOUT_INT_SRC    (INT_SRC_USART1_RTO)
////////////////////////////////////////////////////////////////
#define MODE1_PIN     		GPIO_PIN_06
#define MODE1_PORT			GPIO_PORT_D

#define MODE2_PIN     		GPIO_PIN_07
#define MODE2_PORT			GPIO_PORT_D

#define MODE3_PIN     		GPIO_PIN_03
#define MODE3_PORT			GPIO_PORT_B

#define MODE4_PIN     		GPIO_PIN_04
#define MODE4_PORT			GPIO_PORT_B


#endif
