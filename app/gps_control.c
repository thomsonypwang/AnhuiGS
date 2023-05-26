#include "gps_control.h"
#include "gps_dir.h"
#include "project_pin_use_config.h"
#include "sys_os.h"
#include "sys_log.h"
#include "sys_errno.h"
#include "rtc_drv.h"

//#include "math.h"

char gps_rx_buf[GPS_RX_LEN];
char tar_value[GPS_RX_LEN];  

uint16_t gps_rx_buf_cnt;
uint8_t gps_ok;

GPS_INFO GPS;

/**
 * @brief  USART RX IRQ callback
 * @param  None
 * @retval None
 */
static void USART3_RxFull_IrqCallback(void)
{
    uint8_t tmp = (uint8_t)USART_ReadData(GPS_USART_ID);
	
	if(tmp == '$')
	{
		gps_rx_buf_cnt=0;
	}
	
	gps_rx_buf[gps_rx_buf_cnt++] = tmp;

	//GNRMC\GPRMC
	if(gps_rx_buf[0] == '$' && gps_rx_buf[4] == 'M' && gps_rx_buf[5] == 'C')	
	{
		if(tmp == '\n')									   
		{
			memset(tar_value, 0, GPS_RX_LEN);      //清空
			memcpy(tar_value, gps_rx_buf, gps_rx_buf_cnt); 	//保存数据
			gps_ok = 1;
			gps_rx_buf_cnt = 0;
			memset(gps_rx_buf, 0, GPS_RX_LEN);      //清空				
		}					
	}
	if(gps_rx_buf_cnt >= GPS_RX_LEN)
	{
		gps_rx_buf_cnt=0;
	}	
}

/**
 * @brief  USART error IRQ callback.
 * @param  None
 * @retval None
 */
static void USART3_RxError_IrqCallback(void)
{
	(void)USART_ReadData(GPS_USART_ID);
    USART_ClearStatus(GPS_USART_ID, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

void atgm336h_data_init(void)
{
	memset(gps_rx_buf, 0, GPS_RX_LEN);
	memset(tar_value, 0, GPS_RX_LEN);
}

void gps_init(void)
{
	gps_usart_init(USART3_RxError_IrqCallback,USART3_RxFull_IrqCallback);
	atgm336h_data_init();
}

static int GetComma(int num,char *str)  
{  
    int i,j=0;  
    int len=strlen(str); 
	
    for(i=0;i<len;i++)  
    {  
        if(str[i]==',')
		{
			j++;  	
		}
        if(j==num)
		{
			return i+1;   /*返回当前找到的逗号位置的下一个位置*/  
		}
    }  
    return 0;     
}

static double get_double_number(char *s)  
{  
    char buf_tmp[128];  
    int i;  
    double rev;  

    i=GetComma(1,s);    /*得到数据长度 */
    strncpy(buf_tmp,s,i);  
    buf_tmp[i]=0;           /*加字符串结束标志*/ 
    rev=atof(buf_tmp);      /*字符串转float */ 
    return rev;  
}

static int get_int_number(char *s)  
{  
    char buf_tmp1[128];  
    int i;  
    double rev;  

    i=GetComma(1,s);    /*得到数据长度*/  
    strncpy(buf_tmp1,s,i);  
    buf_tmp1[i]=0;           /*加字符串结束标志 */
    rev=atoi(buf_tmp1);      /*字符串转float */ 
    return rev;  
}

static void UTC2BTC(date_time *GPS) /*如果秒号先出,再出时间数据,则将时间数据+1秒 */  
{  
	GPS->second++; /*加一秒*/  
	if(GPS->second>59)
	{  
		GPS->second=0;  
		GPS->minute++;  
		if(GPS->minute>59)
		{  
			GPS->minute=0;  
			GPS->hour++;  
		}  
	}     
	GPS->hour+=8;        /*北京时间跟UTC时间相隔8小时 */ 
	if(GPS->hour>23)  
	{  
		GPS->hour-=24;  
		GPS->day+=1;  
		if(GPS->month==2 ||GPS->month==4 ||GPS->month==6 ||GPS->month==9 ||GPS->month==11 )
		{  
			if(GPS->day>30)/*上述几个月份是30天每月，2月份还不足30*/  
			{          
				GPS->day=1;  
				GPS->month++;  
			}  
		}  
		else
		{  
			if(GPS->day>31)/*剩下的几个月份都是31天每月 */
			{          
				GPS->day=1;  
				GPS->month++;  
			}  
		}  

		if(GPS->year % 4 == 0 )
		{  
			if(GPS->day > 29 && GPS->month ==2)/*闰年的二月是29天*/  
			{       
				GPS->day=1;  
				GPS->month++;  
			}  
		}  
		else
		{  
			if(GPS->day>28 &&GPS->month ==2)/*其他的二月是28天每月*/  
			{      
				GPS->day=1;  
				GPS->month++;  
			}  
		}  
		if(GPS->month>12)
		{  
			GPS->month-=12;  
			GPS->year++;  
		}         
	}  
}

void gps_parse(char *line,GPS_INFO *GPS)  
{  
    int tmp;  
    char c;  
    char* buf=line;  
    c=buf[5];  
    if(c=='C')/* "GPRMC" */ 
    {
		GPS->D.hour   =(buf[7]-'0')*10+(buf[8]-'0');  
		GPS->D.minute =(buf[9]-'0')*10+(buf[10]-'0');  
		GPS->D.second =(buf[11]-'0')*10+(buf[12]-'0');  
		tmp = GetComma(9,buf);      /*得到第9个逗号的下一字符序号*/  
		GPS->D.day    =(buf[tmp+0]-'0')*10+(buf[tmp+1]-'0');  
		GPS->D.month  =(buf[tmp+2]-'0')*10+(buf[tmp+3]-'0');  
		GPS->D.year   =(buf[tmp+4]-'0')*10+(buf[tmp+5]-'0')+2000;  
		/*********************************************************/  
		GPS->status   =buf[GetComma(2,buf)];     /*状态*/  
		GPS->latitude =get_double_number(&buf[GetComma(3,buf)]); /*纬度*/  
		GPS->NS       =buf[GetComma(4,buf)];             /*南北纬 */ 
		GPS->longitude=get_double_number(&buf[GetComma(5,buf)]); /*经度*/  
		GPS->EW       =buf[GetComma(6,buf)];             /*东西经 */ 
		UTC2BTC(&GPS->D);                        /*转北京时间*/  
    }  
    if(c=='A') /*"$GPGGA" */
    {
		GPS->high =get_double_number(&buf[GetComma(9,buf)]); 
		GPS->num =get_int_number(&buf[GetComma(7,buf)]); 
    }  
} 
void show_gps(GPS_INFO *GPS)
{  
     log_i("年份     : %ld-%02d-%02d",GPS->D.year,GPS->D.month,GPS->D.day);
     log_i("时间     : %02d:%02d:%02d",GPS->D.hour,GPS->D.minute,GPS->D.second);
     log_i("纬度     : %s %10.4f",(GPS->NS=='N')?"北纬":"南纬",GPS->latitude);
     log_i("经度     : %s %10.4f",(GPS->EW=='W')?"西经":"东经",GPS->longitude);
     log_i("卫星数   : %02d",GPS->num);
     log_i("高度     : %.4f",GPS->high);
     log_i("状态     : %s",(GPS->status=='A')?"定位":"导航");
     log_i("--------------------");  
}

void gps_data_resolving(void)
{
	int tmp;

	
	if (gps_ok==1)
	{
		gps_ok = 0;
		//log_i("gps::%s",tar_value);
		gps_parse(tar_value,&GPS);
		
		tmp=GPS.latitude / 100;
		device_data.n_latitude = (double)tmp+(double)((GPS.latitude-100*tmp)/60.0);
//		log_i("latitude%10.4f",GPS.latitude);
		//log_i("n_latitude%8.4f",device_data.n_latitude);
		
		tmp=GPS.longitude / 100;
		device_data.w_longitude = (double)tmp+(double)((GPS.longitude-100*tmp)/60.0);
//		log_i("longitude%10.4f",GPS.longitude);
		//log_i("w_longitude%.4f",device_data.w_longitude);
		
//		abc1=GPS.longitude / 100;
//		de=(GPS.longitude-100*abc1)/1;
//		fghi=(GPS.longitude-de-100*abc1)*10000;		
//		device_data.w_longitude = (double)abc1+(double)(de/60.0)+(double)(fghi/600000.0);
//		log_i("longitude%10.4f",GPS.longitude);
//		log_i("abc=%d,de=%d,fghi=%d",abc1,de,fghi);
//		log_i("w_longitude%10.4f",device_data.w_longitude);

		//show_gps(&GPS);
		if(device_data.set_timer_flag==0)
		{
			if((GPS.D.month<=31)&&(GPS.D.year>=2023))
			{
				rtc_set_time(GPS.D.year,GPS.D.month,GPS.D.day,GPS.D.hour, GPS.D.minute, GPS.D.second, 1,&device_data.c_date,&device_data.c_time);
				device_data.set_timer_flag=1;			
			}
		}
	}
}

