#ifndef  __CAMERA_COM_PRO_H__
#define  __CAMERA_COM_PRO_H__

#include <string.h>
#include <stdio.h>
#include "hc32_ll.h"

/******************使用SD卡存储图片宏开关*************************/
#ifndef USING_SAVE_SD
	#define USING_SAVE_SD               0
#endif

#if USING_SAVE_SD
	#include "mmc_sd.h"
	#include "diskio.h"
	#include "ff.h"

	uint8_t camera_new_pathname(uint8_t *pname);
#endif


#define PTC08   0 /*30万像素*/ 
#define PTC1M3  1 /*130万像素*/
#define PTC2M0  2 /*200万像素*/

#define PTCAM  PTC2M0


#define  second	 1000

#define  L_TIME  (2*second)
#define  Z_TIME  (10*second)
#define  HH_TIME (90*second)
#define  H_TIME  (30*second) //设置串口接收的等待时间
//硬件引脚定义
//#define  camer_pwerH()    GPIO_SetBits(GPIOA,GPIO_Pin_1)
//#define  camer_pwerL()    GPIO_ResetBits(GPIOA,GPIO_Pin_1)

//用户自定义宏变量
#define N_BYTE  3072        //每次读取N_BYTE字节，N_BYTE必须是8的倍数

#define IMAGE_SIZE_160X120     0x22
#define IMAGE_SIZE_320X240     0x11
#define IMAGE_SIZE_640X480     0x00


#define COMPRESS_RATE_36       0x36   //该压缩率是默认压缩率，160x120和320x240可用此压缩率

#define COMPRESS_RATE_60       0x60   //640X480尺寸，默认压缩率36会占用45K左右的空间
                                      //选择60压缩率可将45K压缩到20K左右
typedef enum
{
    SERIAL_NUM_0 = 0x00,
    SERIAL_NUM_1,
    SERIAL_NUM_2,
    SERIAL_NUM_3,
    SERIAL_NUM_5,
    SERIAL_NUM_6,
    SERIAL_NUM_7,
    SERIAL_NUM_8,
    SERIAL_NUM_9,
    SERIAL_NUM_10
}nSerialNum;


//串口驱动接口函数，移植时需要修改接口函数
void cam_write(const uint8_t *buf,uint8_t len);
uint16_t cam_receiver( uint8_t *buf,uint16_t send_len);
int query_version(void);

// 应用实例函数
int camera_app(uint8_t Serialnumber,uint8_t nCameraImageSize);

int cmd_test(uint8_t SerialNum);
int camera_init(uint8_t Serialnumber,uint8_t nSetImageSize);
int send_cmd(const uint8_t *cmd,uint8_t n0,const uint8_t *rev,uint8_t n1);
void SetSerailNumber(uint8_t *DstCmd, const uint8_t *SrcCmd, uint8_t SrcCmdLength,
                     uint8_t *DstRcv, const uint8_t *SrcRcv, uint8_t SrcRcvLength,uint8_t nID);

//摄像头属性设置：复位/图片尺寸大小/图片压缩率
int send_reset(uint8_t Serialnumber);
int current_photo_size(uint8_t Serialnumber,uint8_t * nImageSize);
int send_photo_size(uint8_t Serialnumber,uint8_t nImageSize);
int send_compress_rate(uint8_t Serialnumber,uint8_t nCompressRate);

//移动侦测控制函数
int send_motion_sensitivity(uint8_t Serialnumber);
int send_open_motion(uint8_t Serialnumber);
int send_close_motion(uint8_t Serialnumber);
int Motion_Detecte_Idle(uint8_t *pSerialnumber);


//拍照处理函数
int send_photoBuf_cls(uint8_t Serialnumber);
int send_start_photo(uint8_t Serialnumber);
uint32_t send_read_len(uint8_t Serialnumber);
int send_get_photo(uint32_t add,uint32_t read_len,uint8_t *buf,uint8_t Serialnumber);
#endif
