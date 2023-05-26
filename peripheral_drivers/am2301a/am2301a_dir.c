#include "am2301a_dir.h"
#include "project_pin_use_config.h"
#include "sys_log.h"

uint8_t am2301a_init(void)
{
	stc_gpio_init_t stcGpioInit;

	(void)GPIO_StructInit(&stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_OUT;
	(void)GPIO_Init(AM2301A_SDA_PORT, AM2301A_SDA_PIN, &stcGpioInit);
	GPIO_SetPins(AM2301A_SDA_PORT, AM2301A_SDA_PIN);
	
	am2301a_rst();  
	return am2301a_Check();
}

void am2301a_sda_out(void)
{
	stc_gpio_init_t stcGpioInit;

	LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU);
	(void)GPIO_StructInit(&stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_RST;
	stcGpioInit.u16PinDir = PIN_DIR_OUT;
	(void)GPIO_Init(AM2301A_SDA_PORT, AM2301A_SDA_PIN, &stcGpioInit);
	
	//GPIO_ResetPins(AM2301A_SDA_PORT, AM2301A_SDA_PIN);
	LL_PERIPH_WP(LL_PERIPH_GPIO|LL_PERIPH_EFM | LL_PERIPH_FCG);
}

void am2301a_sda_in(void)
{
	stc_gpio_init_t stcGpioInit;

	LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU);
	(void)GPIO_StructInit(&stcGpioInit);

	stcGpioInit.u16PinState = PIN_STAT_SET;
	stcGpioInit.u16PinDir = PIN_DIR_IN;
	stcGpioInit.u16PullUp=PIN_PU_ON;
	(void)GPIO_Init(AM2301A_SDA_PORT, AM2301A_SDA_PIN, &stcGpioInit);
	LL_PERIPH_WP(LL_PERIPH_GPIO|LL_PERIPH_EFM | LL_PERIPH_FCG);
}

//复位am2301a
void am2301a_rst(void)	   
{                 
	am2301a_sda_out(); 	//SET OUTPUT
    GPIO_ResetPins(AM2301A_SDA_PORT, AM2301A_SDA_PIN); 	//拉低DQ
    DDL_DelayMS(2);  	//拉低至少500us,此处1ms
    GPIO_SetPins(AM2301A_SDA_PORT, AM2301A_SDA_PIN); 	//DQ=1 
	DDL_DelayUS(30);     	//主机拉高20~40us
}

//等待am2301a的回应
//返回1:未检测到am2301a的存在
//返回0:存在出现由低到高的变化即可
uint8_t am2301a_Check(void) 	   
{   
	uint8_t retry=0;
	
	am2301a_sda_in();//SET INPUT	 	
    while (!GPIO_ReadInputPins(AM2301A_SDA_PORT, AM2301A_SDA_PIN)&&retry<100)//DHT22会拉低80us左右
	{
		retry++;
		DDL_DelayUS(1);
	} 
	if(retry>=100)
	{
		return 1;
	}	
	else
	{
		retry=0;
	}		
		
    while (GPIO_ReadInputPins(AM2301A_SDA_PORT, AM2301A_SDA_PIN)&&retry<100)//DHT22拉低后会再次拉高80us左右
	{
		retry++;
		DDL_DelayUS(1);
	}
	if(retry>=100)
	{
		return 1;
	}	
	return 0;		
}

//从DHT22读取一个位
//返回值：1/0
uint8_t am2301a_read_bit(void) 			 
{
 	uint8_t retry=0;
	while(GPIO_ReadInputPins(AM2301A_SDA_PORT, AM2301A_SDA_PIN)&&retry<50)//等待变为低电平
	{
		retry++;
		DDL_DelayUS(1);
	}
	retry=0;
	while(!GPIO_ReadInputPins(AM2301A_SDA_PORT, AM2301A_SDA_PIN)&&retry<50)//等待变高电平
	{
		retry++;
		DDL_DelayUS(1);
	}
	DDL_DelayUS(20);//等待40us
	if(GPIO_ReadInputPins(AM2301A_SDA_PORT, AM2301A_SDA_PIN))
	{
		return 1;
	}
	else
	{
		return 0;	
	}		   
}

//从DHT11读取一个字节
//返回值：读到的数据
uint8_t am2301a_read_byte(void)  
{        
	uint8_t i,dat;
	dat=0;
	
	for (i=0;i<8;i++) 
	{
		dat<<=1; 
		dat|=am2301a_read_bit();
	}				    
	return dat;
}

//从DHT22读取一次数据
//temp:温度值(范围:-40--80°)
//humi:湿度值(范围:0%--99.9%)
//返回值：0,正常;1,读取失败
uint8_t am2301a_read_data(float *temperature,float *humidity)  
{        
 	uint8_t buf[5];
	uint8_t i;
	uint8_t sum=0;
	
	am2301a_rst();
	if(am2301a_Check()==0)
	{
		for(i=0;i<5;i++)//读取40位数据
		{
			buf[i]=am2301a_read_byte();
		}
//		log_i("buf[0]=%d",buf[0]);
//		log_i("buf[1]=%d",buf[1]);
//		log_i("buf[2]=%d",buf[2]);
//		log_i("buf[3]=%d",buf[3]);
//		log_i("buf[4]=%d",buf[4]);
		sum = buf[0]+buf[1]+buf[2]+buf[3];
//		log_i("sum=%d",sum);
		if(sum==buf[4])
		{
			*humidity=((float)((buf[0]<<8)+buf[1]))/10;
			*temperature=((float)((buf[2]<<8)+buf[3]))/10;
//			log_i("humidity=%f",*humidity);
//			log_i("temperature=%f",*temperature);
		}
	}
	else 
	{
		//11111111111log_i("am2301a_Check err");
		return 1;
	}

//	log_i("am2301a ok");	
	return 0;	    
}

