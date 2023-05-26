#ifndef _GPS_CONTROL_H_
#define _GPS_CONTROL_H_

#include "project_config.h"

#define GPS_RX_LEN 512       /*可随意取，但要大于GPGGA_MAX*/
#define GPGGA_MAX 100       /*"$GPGGA……"的最大值，待定*/ 

typedef struct
{  
    int year;    
    int month;   
    int day;  
    int hour;  
    int minute;  
    int second;  
}date_time;  

typedef struct
{  
     date_time D;/*时间*/  
     char status;       /*接收状态 */
     double latitude;   /*纬度*/  
     double longitude;  /*经度 */ 
     char NS;           /*南北极*/  
     char EW;           /*东西 */ 
     int num;           /*卫星数*/ 
     double speed;      /*速度 */ 
     double high;       /*高度*/  
}GPS_INFO;

extern GPS_INFO GPS;

void gps_init(void);
void gps_data_resolving(void);
#endif
