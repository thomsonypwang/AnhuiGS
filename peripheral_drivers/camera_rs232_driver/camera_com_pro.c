/******************************************************************************
时间：2014-02-21
描述：VC0706摄像头模块驱动函数,支持设备类型： PTC01/PTC06/PTC08

////////////////////
2019/05/06更新

1.修改指令执行顺序解决2次拍照是同一张的问题
拍照片的指令顺序：
1)拍照指令
2)读图片长度指令
3)读图片数据指令
4)清空图片缓存(最后一个字节03)

2.屏蔽初始化camera_init，可选步骤，一般在上位机软件设置一次图片大小都会保持下来
3.增加PTC1M3上电延时2.5秒
4.增加cmd_test测试是否指令通讯正常
5.注意PTC1M3不支持压缩指令及图片设置指令，支持的指令请查阅PTC1M3说明文档
6.PTC1M3使用4个字节表示长度
////////////////////
2014/09/11更新
1. 新增修改协议序号，支持RS485
2. 移动侦测拍照

//////////////////
2014-02-21
V1.0版本
摄像头应用实例
CameraDemoApp()

拍照前[可选指令]
设置图片尺寸(默认值：320X240，修改尺寸需复位)
设置图片压缩率(默认值：36)

[这个顺序在一直上电状态下，连续拍照会出现同一张图片的问题，仅能适用在首次上电，拍照后关闭电源的场景]
拍照片的指令顺序：
1.清空图片缓存(最后一个字节02)
2.拍照指令
3.读图片长度指令
4.读图片数据指令

20190506修改指令执行顺序解决2次拍照是同一张的问题
拍照片的指令顺序：
1.拍照指令
2.读图片长度指令
3.读图片数据指令
4.清空图片缓存(最后一个字节03)

******************************************************************************/
#include "camera_com_pro.h"
#include "camera_dir.h"

#include "sys_log.h"
#include "sys_errno.h"
#include "sys_os.h"

#include "eth_handle.h"
#include "project_psm_control.h"
#include "w5500_dir.h"
#include "w5500_socket.h"
#include "w5500_dns.h"

#if     USING_SAVE_SD
	FIL JPGE;
	FATFS fs;
#endif

#define CLEAR_FRAME            1   	 //去掉返回图片数据携带的协议头和尾76 00 32 00
#define ECHO_CMD_DEBUG_INFO    0     //1，开启指令调试；0，关闭
#define ID_SERIAL_NUM       1        //序号在数组的所在位置

//复位指令与复位回复
const uint8_t reset_rsp[] = {0x76,0x00,0x26,0x00};
const uint8_t reset_cmd[] = {0x56,0x00,0x26,0x00};


//清除图片缓存指令与回复
//最后一个字节是0x02
/*
const uint8_t photoBufCls_cmd [] = {0x56,0x00,0x36,0x01,0x02};
const uint8_t photoBufCls_rsp[] = {0x76,0x00,0x36,0x00,0x00};  	
*/

//清除图片缓存指令与回复
//最后一个字节是0x03
const uint8_t photoBufCls_cmd [] = {0x56,0x00,0x36,0x01,0x03};
const uint8_t photoBufCls_rsp[] = {0x76,0x00,0x36,0x00,0x00};  	

//拍照指令与回复
const uint8_t start_photo_cmd[] = {0x56,0x00,0x36,0x01,0x00};    
const uint8_t start_photo_rsp[] = {0x76,0x00,0x36,0x00,0x00};   

//读图片长度指令与回复
//图片长度指令回复的前7个字节是固定的，最后2个字节表示图片的长度
//如0xA0,0x00,10进制表示是40960,即图片长度(大小)为40K
const uint8_t read_len_cmd[] = {0x56,0x00,0x34,0x01,0x00};
const uint8_t read_len_rsp[] = {0x76,0x00,0x34,0x00,0x04,0x00,0x00,0x00,0x00};

//读图片数据指令与回复
//get_photo_cmd前6个字节是固定的，
//第9,10字节是图片的起始地址
//第13,14字节是图片的末尾地址，即本次读取的长度

//如果是一次性读取，第9,10字节的起始地址是0x00,0x00;
//第13,14字节是图片长度的高字节，图片长度的低字节(如0xA0,0x00)

//如果是分次读取，每次读N字节(N必须是8的倍数)长度，
//则起始地址最先从0x00,0x00读取N长度(即N & 0xff00, N & 0x00ff)，
//后几次读的起始地址就是上一次读取数据的末尾地址
const uint8_t get_photo_cmd [] = {0x56,0x00,0x32,0x0C,0x00,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF }; 
const uint8_t get_photo_rsp []  = {0x76,0x00,0x32,0x00,0x00};

//设置压缩率指令与回复，最后1个字节为压缩率选项
//范围是：00 - FF
//默认压缩率是36
const uint8_t set_compress_cmd [] = {0x56,0x00,0x31,0x05,0x01,0x01,0x12,0x04,0x36};
const uint8_t compress_rate_rsp [] = {0x76,0x00,0x31,0x00,0x00};

//设置图片尺寸指令与回复
//set_photo_size_cmd最后1个字节的意义
//0x22 - 160X120
//0x11 - 320X240
//0x00 - 640X480
const uint8_t set_photo_size_cmd [] = {0x56,0x00,0x31,0x05,0x04,0x01,0x00,0x19,0x11};
const uint8_t set_photo_size_rsp [] = {0x76,0x00,0x31,0x00,0x00 };

//读取图片尺寸指令与回复
//read_photo_size_rsp最后1个字节的意义
//0x22 - 160X120
//0x11 - 320X240
//0x00 - 640X480
const uint8_t read_photo_size_cmd [] = {0x56,0x00,0x30,0x04,0x04,0x01,0x00,0x19};
const uint8_t read_photo_size_rsp [] = {0x76,0x00,0x30,0x00,0x01,0x00};

//移动侦测指令
//motion_enable_cmd 打开移动侦测
//motion_disable_cmd 关闭移动侦测
const uint8_t motion_enable_cmd [] = {0x56,0x00,0x37,0x01,0x01};
const uint8_t motion_disable_cmd [] = {0x56,0x00,0x37,0x01,0x00};
const uint8_t motion_rsp [] = {0x76,0x00,0x37,0x00,0x00};

//当系统检测到有移动时，自动从串口输出motio_detecte
const uint8_t motion_detecte [] = {0x76,0x00,0x39,0x00,0x00};

//移动侦测灵敏度设置
const uint8_t motion_sensitivity_cmd [] = {0x56,0x00,0x31,0x05,0x01,0x01,0x1A,0x6E,0x03};
const uint8_t motion_sensitivity_rsp [] = {0x76,0x00,0x31,0x00,0x00};

///////////////////////////////////////////////////////////////////////////////////////////////
volatile uint8_t cameraReady = 0;
//uint32_t picLen = 0;   //数据长度

//CommandPacket和ResponsePacket用于拷贝只读区的指令及应答到内存
uint8_t CommandPacket[16];
uint8_t ResponsePacket[9];

const nSerialNum SerialNum_Byte;//序列号枚举变量
uint8_t g_SerialNumber = 0;//串口上报移动侦测时保存的当前序列号
//uint8_t camera_buf[5000] = "";
/****************************************************************
函数名：SetSerailNumber
函数描述: 修改协议中的序号
输入参数：目标指令缓存首地址，源指令首地址，源指令长度，
          目标应答缓存首地址，源应答首地址，源应答长度，需要修改的
          序号值
返回:无
******************************************************************/		
void SetSerailNumber(uint8_t *DstCmd, const uint8_t *SrcCmd, uint8_t SrcCmdLength,
                     uint8_t *DstRsp, const uint8_t *SrcRsp, uint8_t SrcRspLength,uint8_t nID)
{
    memset(&CommandPacket,0,sizeof(CommandPacket));
    memset(&ResponsePacket,0,sizeof(ResponsePacket));
    
    memcpy(DstCmd,SrcCmd,SrcCmdLength);
    memcpy(DstRsp,SrcRsp,SrcRspLength);
    
    DstCmd[ID_SERIAL_NUM] = nID & 0xFF;
    DstRsp[ID_SERIAL_NUM] = nID & 0xFF;
}

/****************************************************************
函数名：cam_write
函数描述: 接口函数，写入控制摄像头的串口
输入参数：数据的首地址，长度
返回:无
******************************************************************/		
void cam_write(const uint8_t *buf,uint8_t len)
{ 
    //需要调试时开启
    #if ECHO_CMD_DEBUG_INFO     
		log_i("cam_write:%s",cam_write);
    #endif
    //写串口驱动函数
    send_camera_data((uint8_t *)buf,len);   
}

/****************************************************************
函数名：cam_receiver
函数描述：接口函数，读取控制摄像头的串口
输出参数：接收数据的地址，长度
返回:接收到数据个数
******************************************************************/		
uint16_t cam_receiver(uint8_t *buf,uint16_t send_len)
{ 
    uint16_t i = 0;

    i = (uint16_t)receive_camera_data(buf,send_len,L_TIME);
    #if ECHO_CMD_DEBUG_INFO
			
    #endif
	//log_i("cam_receiver:%s",&buf[6]);
    return i;
}

/****************************************************************
函数名：camera_init
函数描述：摄像头初始化,PTC1M3不需要使用此函数，因不支持压缩指令和查询图片大小指令
输入参数：序列号，需要设置的图片尺寸
返回:初始化成功返回SYS_OK，初始化失败返回SYS_FAIL
******************************************************************/		
int camera_init(uint8_t Serialnumber,uint8_t nSetImageSize)
{    
    uint8_t CurrentImageSize = 0xFF;
    uint8_t CurrentCompressRate = COMPRESS_RATE_36;
    
    if(!current_photo_size(Serialnumber,&CurrentImageSize))//读取当前的图片尺寸到currentImageSize
    {
        log_e("camera","nread_photo_size error");
        return SYS_FAIL;
    }
    
    if(nSetImageSize != CurrentImageSize)//判断是否需要修改图片尺寸
    {
        if(!send_photo_size(Serialnumber,nSetImageSize))//设置图片尺寸，设置后复位生效，该项设置后会永久保存
        {
            log_e("camera","nset_photo_size error");
            return SYS_FAIL;
        }
        else
        {
            if(!send_reset(Serialnumber))//复位生效
            {
                log_e("camera","reset error");
                return SYS_FAIL;
            }
            os_thread_sleep(os_msec_to_ticks(1000));
            CurrentImageSize = nSetImageSize;
        }
    }
    
    //给不同图片尺寸设置适当的图片压缩率
    if(nSetImageSize == CurrentImageSize)
    {
        switch(CurrentImageSize)
        {
            case IMAGE_SIZE_160X120:
            case IMAGE_SIZE_320X240:
                 CurrentCompressRate = COMPRESS_RATE_36;
                 break;
            case IMAGE_SIZE_640X480:
                 CurrentCompressRate = COMPRESS_RATE_60;
                 break;
            default:
                break;
        }
    }
    if ( !send_compress_rate(Serialnumber,CurrentCompressRate))//设置图片压缩率，该项不保存，每次上电后需重新设置
    {
        log_e("camera","nsend_compress_rate error");
        return SYS_FAIL;
    }
	os_thread_sleep(os_msec_to_ticks(100)); //这里要注意,设置压缩率后要延时
    return SYS_OK;  
}

/****************************************************************
 函数名：send_cmd
 函数描述：发送指令并识别指令返回
 输入参数：指令的首地址，指令的长度，匹配指令的首地址，需验证的个数
 返回：成功返回SYS_OK,失败返回SYS_FAIL
******************************************************************/	
int send_cmd( const uint8_t *cmd,uint8_t n0,const uint8_t *rev,uint8_t n1)
{
    uint8_t  i;
    uint8_t  tmp[5] = {0x00,0x00,0x00,0x00,0x00};

    cam_write(cmd, n0);

    if(!cam_receiver(tmp,5)) 
    {
        return SYS_FAIL;
    }
   
    for (i = 0; i < n1; i++) //检验数据
    {  
        if (tmp[i] != rev[i]) 
        {
            return SYS_FAIL;
        }
    } 
    return SYS_OK;
}

/****************************************************************
函数名：current_photo_size
函数描述:读取当前设置的图片尺寸
输入参数：Serialnumber序列号，nImageSize传递图片尺寸的引用变量
返回:成功返回SYS_OK,失败返回SYS_FAIL
******************************************************************/	
int current_photo_size(uint8_t Serialnumber,uint8_t * nImageSize)
{  
    uint8_t  i;
    uint8_t  tmp[6] = {0x00,0x00,0x00,0x00,0x00,0x00};

    SetSerailNumber( CommandPacket,
                     read_photo_size_cmd,
                     sizeof(read_photo_size_cmd),
                     ResponsePacket,
                     read_photo_size_rsp,
                     sizeof(read_photo_size_rsp),
                     Serialnumber );
      
    cam_write(CommandPacket, sizeof(read_photo_size_cmd));

    if( !cam_receiver(tmp,6)) 
    {
        return SYS_FAIL;
    }
    for (i = 0; i < 5; i++) //检验数据,对比前5个字节
    {  
        if (tmp[i] != ResponsePacket[i]) 
        {
            return SYS_FAIL;
        }
    }
    *nImageSize = tmp[5];//最后一个字节表示当前的图片大小
    return SYS_OK;
}

/****************************************************************
函数名：send_photo_size
函数描述：设置拍照的图片尺寸（可选择：160X120,320X240,640X480）
输入参数：序列号，需要设置的图片尺寸
返回:成功返回SYS_OK,失败返回SYS_FAIL
******************************************************************/	
int send_photo_size(uint8_t Serialnumber,uint8_t nImageSize)
{  
    uint8_t  i;
    
    SetSerailNumber( CommandPacket,
                     set_photo_size_cmd,
                     sizeof(set_photo_size_cmd),
                     ResponsePacket,
                     set_photo_size_rsp,
                     sizeof(set_photo_size_rsp),
                     Serialnumber );
    
    CommandPacket [sizeof(set_photo_size_cmd) - 1] = nImageSize;
    
    i = send_cmd( CommandPacket,
                  sizeof(set_photo_size_cmd),
                  ResponsePacket,
                  sizeof(set_photo_size_rsp) );
    return i;
}

/****************************************************************
函数名：send_reset
函数描述：发送复位指令复位后要延时1-2秒
输入参数：序列号
返回:成功返回SYS_OK 失败返回SYS_FAIL
******************************************************************/		
int send_reset(uint8_t Serialnumber)
{  
    uint8_t i;
    //复制命令与应答，修改序号
    SetSerailNumber( CommandPacket,
                     reset_cmd,
                     sizeof(reset_cmd),
                     ResponsePacket,
                     reset_rsp,
                     sizeof(reset_rsp),
                     Serialnumber );
    
    i = send_cmd( CommandPacket,
                  sizeof(reset_cmd),
                  ResponsePacket,
                  sizeof(reset_rsp) );
    
    return i;

}

/****************************************************************
函数名：send_stop_photo
函数描述：清空图片缓存
输入参数：序列号
返回:成功返回SYS_OK,失败返回SYS_FAIL
******************************************************************/		 
int send_photoBuf_cls(uint8_t Serialnumber)
{ 
    uint8_t i;
    
    SetSerailNumber( CommandPacket,
                     photoBufCls_cmd,
                     sizeof(photoBufCls_cmd),
                     ResponsePacket,
                     photoBufCls_rsp,
                     sizeof(photoBufCls_rsp),
                     Serialnumber );
    
    i = send_cmd( CommandPacket,
                  sizeof(photoBufCls_cmd),
                  ResponsePacket,
                  sizeof(photoBufCls_rsp) );
    return i;
}  

/****************************************************************
函数名：send_compress_rate
函数描述：发送设置图片压缩率
输入参数：序列号
返回:成功返回SYS_OK,失败返回SYS_FAIL
******************************************************************/		 
int send_compress_rate(uint8_t Serialnumber,uint8_t nCompressRate)
{
    uint8_t i;
    
    SetSerailNumber( CommandPacket,
                     set_compress_cmd,
                     sizeof(set_compress_cmd),
                     ResponsePacket,
                     compress_rate_rsp,
                     sizeof(compress_rate_rsp),
                     Serialnumber );
    
    if(nCompressRate > 0x36)
    {
        CommandPacket [sizeof(set_compress_cmd) - 1] = nCompressRate;//最后一个字节表示压缩率
    }
    
    i = send_cmd( CommandPacket,
                  sizeof(set_compress_cmd),
                  ResponsePacket,
                  sizeof(compress_rate_rsp) );
    return i;
}

/****************************************************************
函数名：send_start_photo
函数描述：发送开始拍照的指令
输入参数：序列号
返回:识别成功返回SYS_OK 失败返回SYS_FAIL
******************************************************************/		
int send_start_photo(uint8_t Serialnumber)
{
    uint8_t i;
    
    SetSerailNumber( CommandPacket,
                     start_photo_cmd,
                     sizeof(start_photo_cmd),
                     ResponsePacket,
                     start_photo_rsp,
                     sizeof(start_photo_rsp),
                     Serialnumber );
    
    i = send_cmd( CommandPacket,
                  sizeof(start_photo_cmd),
                  ResponsePacket,
                  sizeof(start_photo_rsp) );
    return i;
}	  

/****************************************************************
函数名：send_read_len
函数描述：读取拍照后的图片长度，即图片占用空间大小
输入参数：序列号
返回:图片的长度
******************************************************************/	
uint32_t send_read_len(uint8_t Serialnumber)
{
    uint8_t i;
    uint32_t len;
    uint8_t tmp[9];	
    
    SetSerailNumber( CommandPacket,
                     read_len_cmd,
                     sizeof(read_len_cmd),
                     ResponsePacket,
                     read_len_rsp,
                     sizeof(read_len_rsp),
                     Serialnumber );
    
    
    cam_write(CommandPacket, 5);//发送读图片长度指令
    if(!cam_receiver(tmp,9)) 
    {
        return 0;
    }  
    for (i = 0; i < 5; i++)//检验数据,只校验前5个
    {
        if ( tmp[i] != ResponsePacket[i]) 
        {
            return 0;
        }
    }
    /*
    len = (uint32_t)tmp[7] << 8;//高字节
    len |= tmp[8];//低字节*/
    /*使用4个字节表示长度*/
    len = (tmp[5] << 24) | (tmp[6] << 16) | (tmp[7] << 8) | (tmp[8]);  
    return len;
}

/****************************************************************
函数名：send_get_photo
函数描述：读取图片数据
输入参数：读图片起始地址StaAdd, 
          读取的长度readLen ，
          接收数据的缓冲区buf
          序列号
返回:成功返回SYS_OK，失败返回SYS_FAIL
FF D8 ... FF D9 是JPG的图片格式

1.一次性读取的回复格式：76 00 32 00 00 FF D8 ... FF D9 76 00 32 00 00

2.分次读取，每次读N字节,循环使用读取图片数据指令读取M次或者(M + 1)次读取完毕：
如第一次执行后回复格式
76 00 32 00 <FF D8 ... N> 76 00 32 00
下次执行读取指令时，起始地址需要偏移N字节，即上一次的末尾地址，回复格式
76 00 32 00 <... N> 76 00 32 00
......
76 00 32 00 <... FF D9> 76 00 32 00 //lastBytes <= N

Length = N * M 或 Length = N * M + lastBytes

******************************************************************/	
int send_get_photo(uint32_t staAdd,uint32_t readLen,uint8_t *buf,uint8_t Serialnumber)
{
    uint8_t i = 0;
    uint8_t *ptr = NULL;
	
    SetSerailNumber( CommandPacket,
                     get_photo_cmd,
                     sizeof(get_photo_cmd),
                     ResponsePacket,
                     get_photo_rsp,
                     sizeof(get_photo_rsp),
                     Serialnumber );
    
	/*
    //装入起始地址高低字节
    CommandPacket[8] = (staAdd >> 8) & 0xff;
    CommandPacket[9] = staAdd & 0xff;
    //装入末尾地址高低字节
    CommandPacket[12] = (readLen >> 8) & 0xff;
    CommandPacket[13] = readLen & 0xff;*/
    /*填充起始地址*/
    CommandPacket[6] = (staAdd >> 24) & 0xff;
    CommandPacket[7] = (staAdd >> 16) & 0xff;
    CommandPacket[8] = (staAdd >> 8) & 0xff;
    CommandPacket[9] = staAdd & 0xff;
	
    /*填充读取长度*/
    CommandPacket[10] = (readLen >> 24) & 0xff;
    CommandPacket[11] = (readLen >> 16) & 0xff;
    CommandPacket[12] = (readLen >> 8) & 0xff;
    CommandPacket[13] = readLen & 0xff;
    
    //执行指令
    cam_write(CommandPacket,16);
    
    //等待图片数据存储到buf，超时或无数据回复则返回0
    if(!cam_receiver(buf,readLen + 10))
    {
        return SYS_FAIL;
    }
    
    //检验帧头76 00 32 00 00
    for(i = 0; i < 5; i++)
    {
        if(buf[i] != ResponsePacket[i] )
        {
            return SYS_FAIL;
        }
    }

    //检验帧尾76 00 32 00 00
    for (i = 0; i < 5; i++)
    {
        if ( buf[i + 5 + readLen] != ResponsePacket[i] )
        {
            return SYS_FAIL;
        }
    }

    //宏开关选择丢弃/保留 帧头帧尾76 00 32 00 00
    #if CLEAR_FRAME
//   	memcpy(buf,buf + 5,read_len);
		ptr = buf;
		
		for (; readLen > 0; ++ptr)
		{
			*(ptr) = *(ptr + 5);
			readLen--;
		}
    #endif
    
    return SYS_OK;
}

/****************************************************************
函数名：send_open_motion
函数描述：发送打开移动侦测指令
输入参数：序列号
返回:识别成功返回SYS_OK 失败返回SYS_FAIL
******************************************************************/		
int send_motion_sensitivity(uint8_t Serialnumber)
{
    uint8_t i;
    
    SetSerailNumber( CommandPacket,
                     motion_sensitivity_cmd,
                     sizeof(motion_sensitivity_cmd),
                     ResponsePacket,
                     motion_sensitivity_rsp,
                     sizeof(motion_sensitivity_rsp),
                     Serialnumber );
    
    i = send_cmd( CommandPacket,
                  sizeof(motion_sensitivity_cmd),
                  ResponsePacket,
                  sizeof(motion_sensitivity_rsp) );
    return i;
}

/****************************************************************
函数名：send_open_motion
函数描述：发送打开移动侦测指令
输入参数：序列号
返回:识别成功返回SYS_OK 失败返回SYS_FAIL
******************************************************************/		
int send_open_motion(uint8_t Serialnumber)
{
    uint8_t i;
    
    SetSerailNumber( CommandPacket,
                     motion_enable_cmd,
                     sizeof(motion_enable_cmd),
                     ResponsePacket,
                     motion_rsp,
                     sizeof(motion_rsp),
                     Serialnumber );
    
    i = send_cmd( CommandPacket,
                  sizeof(motion_enable_cmd),
                  ResponsePacket,
                  sizeof(motion_rsp));
    return i;
}

/****************************************************************
函数名：send_close_motion
函数描述：发送关闭移动侦测指令
输入参数：序列号
返回:识别成功返回SYS_OK 失败返回SYS_FAIL
******************************************************************/		
int send_close_motion(uint8_t Serialnumber)
{
    uint8_t i;
    
    SetSerailNumber( CommandPacket,
                     motion_disable_cmd,
                     sizeof(motion_disable_cmd),
                     ResponsePacket,
                     motion_rsp,
                     sizeof(motion_rsp),
                     Serialnumber );
    
    i = send_cmd( CommandPacket,
                  sizeof(motion_disable_cmd),
                  ResponsePacket,
                  sizeof(motion_rsp) );
    return i;
}

/****************************************************************
函数名：Motion_Detecte_Idle
函数描述: 等待移动侦测事件,该函数可在RS485同时接多个摄像头时，传递
          当前是第几个序列号上报移动侦测
输入参数：传递一个指针变量
返回:成功返回SYS_OK 失败返回SYS_FAIL
******************************************************************/		
int Motion_Detecte_Idle(uint8_t *pSerialnumber)
{
    uint8_t  tmp[5] = {0x00,0x00,0x00,0x00,0x00};
    
    if(!cam_receiver(tmp,5)) 
    {
        return SYS_FAIL;
    }
    
    //检验数据
    //全部有5个数据，只校验4个，其中数组下标为1是序列号
    if(!(tmp[0] == motion_detecte[0] && 
         tmp[2] == motion_detecte[2] &&
         tmp[3] == motion_detecte[3] &&
         tmp[4] == motion_detecte[4] ))
    {
        return SYS_FAIL;
    }
    
    //取出序列号
    *pSerialnumber = tmp[1];
    return SYS_OK;
}

/****************************************************************
函数名：cmd_test
函数描述：指令测试
输入参数：序列号
返回:成功返回SYS_OK，失败返回SYS_FAIL
******************************************************************/		
int cmd_test(uint8_t SerialNum)
{
    uint8_t _SendVerCmd[] = {0x56,0x00,0x11,0x00};
    uint8_t _AckVerCmd[] = {0x76,0x00,0x11,0x00};
    uint8_t tmp[16] = ""; 
    uint8_t i = 0;
    
    _SendVerCmd[1] = SerialNum; /*修改序号字节*/
    _AckVerCmd[1] = SerialNum;
    
    for(i = 0; i < 3; i++)  /*最多查询3次*/
	{      
        cam_write(_SendVerCmd, sizeof(_SendVerCmd));//发送查版本指令
        if ( !cam_receiver(tmp,sizeof(_AckVerCmd))) //等待接收数据
        {
            os_thread_sleep(os_msec_to_ticks(1000)); //1秒发一次
            continue; //读不到则继续读
        }
        else 
		{
            if(!memcmp((uint8_t *)_AckVerCmd,(uint8_t *)tmp,sizeof(_AckVerCmd))) /*比较数据，数据正确*/
			{
                return SYS_OK;
            }
        }
    }
    return SYS_FAIL;
}

int query_version(void)
{
    uint8_t _SendVerCmd[] = {0x56,0x00,0x11,0x00};
    uint8_t _AckVerCmd[] = {0x76,0x00,0x11,0x00};
    uint8_t tmp[16] = ""; 
    uint8_t i = 0;

    for(i = 0; i < 3; i++)  /*最多查询3次*/
	{      
        cam_write(_SendVerCmd, sizeof(_SendVerCmd));//发送查版本指令
        if ( !cam_receiver(tmp,sizeof(tmp))) //等待接收数据
        {
            os_thread_sleep(os_msec_to_ticks(1000)); //1秒发一次
            continue; //读不到则继续读
        }
        else 
		{
            if(!memcmp((uint8_t *)_AckVerCmd,(uint8_t *)tmp,sizeof(_AckVerCmd))) /*比较数据，数据正确*/
			{
                return SYS_OK;
            }
        }
    }
    return SYS_FAIL;
}
/****************************************************************
函数名：CameraDemoApp
函数描述：摄像头应用实例
输入参数：序列号,图片尺寸
返回:成功返回SYS_OK，失败返回SYS_FAIL
******************************************************************/		
int camera_app(uint8_t Serialnumber,uint8_t nCameraImageSize)
{  
	int ret=0;
	uint16_t timeout_cnt=0;
	
    #if USING_SAVE_SD
		FRESULT res;
		uint8_t pname[20];
		uint8_t Issuccess = 0;
		uint32_t defaultbw = 1;
    #endif
    
    #if (PTCAM == PTC1M3 || PTCAM == PTC2M0)
		ret=cmd_test(Serialnumber);
		if(ret==SYS_FAIL) /*上电后测试一下指令*/
		{
			log_e("camera","cmd_test error");
			return SYS_FAIL;
		}    
    #else
		//初始化摄像头
		//注意PTC1M3不支持压缩指令及查图片大小指令
		cameraReady = camera_init(Serialnumber,nCameraImageSize);
		if(!cameraReady)
		{
			return 0;
		} 
    #endif
	device_data.camera_picLen=0;
	device_data.cntM =0;
    ret=send_start_photo(Serialnumber);
    if(ret==SYS_FAIL)
    {
	    log_e("camera","send_start_photo error");//1.开始拍照
        return SYS_FAIL;
    }
    else
    {
         device_data.camera_picLen = send_read_len(Serialnumber);//2.读取拍照后的图片长度
    }
    
    if(!device_data.camera_picLen)
    {
        log_e("camera","send_read_len error");
        return SYS_FAIL;
    }
    else
    {
        device_data.cntM = device_data.camera_picLen / N_BYTE;
        device_data.lastBytes = device_data.camera_picLen % N_BYTE;
		log_i("camera_picLen=%d",device_data.camera_picLen);
		log_i("cntM=%d lastBytes=%d",device_data.cntM,device_data.lastBytes);
        #if USING_SAVE_SD
			disk_initialize(0);
			if(SD_OK)
			{
				f_mount(0,&fs);	
				res = f_mkdir("0:/PHOTO");		//创建PHOTO文件夹
				if(res != FR_EXIST && res != FR_OK) 	//发生了错误
				{	
					f_mount(0,0);
					Issuccess = 0;
				}
				else
				{
					if(!camera_new_pathname(pname))
					{
						res = f_open(&JPGE,(const TCHAR*)pname,FA_CREATE_ALWAYS | FA_WRITE); 
						if(res != FR_OK)
						{
							Issuccess = 0;
						}
						else
						{
							Issuccess = 1;
						}
					}
				}
			}
        #endif
    }
    //分M次，每次读N_BYTE字节
    if( device_data.cntM )
    {
        for( device_data.camera_buf_cnt = 0; device_data.camera_buf_cnt < device_data.cntM; device_data.camera_buf_cnt++)
        {	 
			log_i("camera_buf_cnt=%d ",device_data.camera_buf_cnt);
            memset(device_data.camera_buf, 0, sizeof(device_data.camera_buf));
            ret=send_get_photo( device_data.camera_buf_cnt * N_BYTE,
								N_BYTE,
								device_data.camera_buf,
								Serialnumber);
            //按图片长度读取数据
            if(ret==SYS_FAIL)
            {
				device_data.camera_send_to_tcp_flag=0;
                log_e("camera","send_get_photo error 1");
                return SYS_FAIL;
            }
            else
            {
                //此分支可将图片数据输出到指定的串口
                //如接口函数，将图片数据写入到串口2
                //UART1Write(gprs_buf,N_BYTE);
                #if USING_SAVE_SD
					if(Issuccess)
					{
						res = f_write(&JPGE, 
									  gprs_buf,
									  N_BYTE,
									  &defaultbw);
					}
                #endif
				if(device_data.phy_check_flag == PHY_LINK_ON)
				{
					device_data.camera_send_flag=1;
					timeout_cnt=0;
					while(1)
					{
						timeout_cnt++;
						os_thread_sleep(os_msec_to_ticks(10));
						if(timeout_cnt>=100)
						{
							device_data.camera_send_flag=0;
							device_data.last_flag=0;
							break;
						}
						if(device_data.camera_send_flag==0)
						{
							break;
						}
					}
				}
            }
            os_thread_sleep(os_msec_to_ticks(200));
        }
    }
    //剩余图片长度
    if(device_data.lastBytes)
    {
        memset(device_data.camera_buf, 0, sizeof(device_data.camera_buf));	
        ret=send_get_photo( device_data.camera_buf_cnt * N_BYTE,//读取剩余长度
                            device_data.lastBytes,
                            device_data.camera_buf,
                            Serialnumber);
       if(ret==SYS_FAIL)
        {
			device_data.camera_send_to_tcp_flag=0;
            log_e("camera","send_get_photo error");
            return SYS_FAIL;
        }
        else
        {
           // UART1Write(gprs_buf,lastBytes);
            #if USING_SAVE_SD
				if(Issuccess)
				{
					res = f_write(&JPGE, 
								  gprs_buf,
								  N_BYTE,
								  &defaultbw);
					f_close(&JPGE);
				}
            #endif
			
			if(device_data.phy_check_flag == PHY_LINK_ON)
			{
				device_data.last_flag=1;
				device_data.camera_send_flag=1;
				timeout_cnt=0;
				while(1)
				{
					timeout_cnt++;
					os_thread_sleep(os_msec_to_ticks(10));
					if(timeout_cnt>=100)
					{
						device_data.camera_send_flag=0;
						device_data.last_flag=0;
						break;
					}
					if(device_data.camera_send_flag==0)
					{
						break;
					}
				}
			}
        }
        os_thread_sleep(os_msec_to_ticks(200));
    }
    //4.清空图片缓存
	ret=send_photoBuf_cls(Serialnumber);
    if(ret==SYS_FAIL)
    {
        log_e("camera","send_photoBuf_cls error");
        return SYS_FAIL;	
	}
    return SYS_OK;
}

#if USING_SAVE_SD
	/*******************************************************
	函数名：camera_new_pathname
	功能描述：存的图片名字
	输入参数：无
	函数返回值：无
	********************************************************/ 
	uint8_t camera_new_pathname(uint8_t *pname)//存的图片名字
	{	 
		FRESULT res;			 
		uint16_t index = 0;
		while(index < 0XFFFF)
		{
			sprintf((char*)pname,"0:PHOTO/PTC%04d.jpg",index);
			res = f_open(&JPGE,(const TCHAR*)pname,FA_READ);//尝试打开这个文件
			if(res==FR_NO_FILE)
			{
				break;		//该文件名不存在,正是我们需要的.
			}
			else if(res == FR_OK)
			{
				f_close(&JPGE);
			}
			else
			{
				return 1;
			}
			index++;
		}
		return 0;
	}
#endif
