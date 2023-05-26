#ifndef _CIRCUIT_BREAKER_RS485_HANDLE_H_
#define _CIRCUIT_BREAKER_RS485_HANDLE_H_

#include "hc32_ll.h"
#include "project_config.h"


typedef enum
{
	CB_1P_16A=0x11, //1P/16A
	CB_1P_32A=0x13, //1P/32A
	CB_1P_63A=0x15, //1P/63A
	CB_1P_16A_NR=0x17, //1P/16A  （不带重合闸功能）
	CB_1P_32A_NR=0x19, //1P/32A  （不带重合闸功能）
	CB_1P_63A_NR=0x1B, //1P/63A  （不带重合闸功能）
	CB_2P_16A=0x21, //2P/16A  
	CB_2P_32A=0x23, //2P/32A  
	CB_2P_63A=0x25, //2P/63A
	CB_2P_16A_LE=0x22, //2P/16A 漏电
	CB_2P_32A_LE=0x24, //2P/32A 漏电
	CB_2P_63A_LE=0x26, //2P/63A 漏电
	CB_2P_16A_NR=0x27, //2P/16A （不带重合闸功能）
	CB_2P_32A_NR=0x29, //2P/32A （不带重合闸功能）
	CB_2P_63A_NR=0x2B, //2P/63A  （不带重合闸功能）
	CB_2P_16A_LE_NR=0x28, //2P/16A 漏电    （不带重合闸功能）
	CB_2P_32A_LE_NR=0x2A, //2P/32A 漏电    （不带重合闸功能）
	CB_2P_63A_LE_NR=0x2C, //2P/63A 漏电    （不带重合闸功能）
	CB_3P_16A=0x31, //3P/16A 
	CB_3P_32A=0x33, //3P/32A
	CB_3P_63A=0x35, //3P/63A
	CB_3P_16A_NR=0x37, //3P/16A （不带重合闸功能）
	CB_3P_32A_NR=0x39, //3P/32A （不带重合闸功能）
	CB_3P_63A_NR=0x3B, //3P/63A （不带重合闸功能）
	CB_4P_16A=0x41, //4P/16A
	CB_4P_32A=0x43, //4P/32A
	CB_4P_63A=0x45, //4P/63A
	CB_4P_16A_LE=0x42, //4P/16A 漏电
	CB_4P_32A_LE=0x44, //4P/32A 漏电
	CB_4P_63A_LE=0x46, //4P/63A 漏电
	CB_4P_16A_NR=0x47, //4P/16A （不带重合闸功能）
	CB_4P_32A_NR=0x49, //4P/32A （不带重合闸功能）
	CB_4P_63A_NR=0x4B, //4P/63A （不带重合闸功能）
	CB_4P_16A_LE_NR=0x48, //4P/16A 漏电 （不带重合闸功能）
	CB_4P_32A_LE_NR=0x4A, //4P/32A 漏电 （不带重合闸功能）
	CB_4P_63A_LE_NR=0x4C, //4P/63A 漏电 （不带重合闸功能）
}circuit_breaker_type;

typedef struct
{
	uint16_t REG0_ROCKER_SW;	// R //拨动开关暂未提供支持
	uint16_t REG0_TYPE; 		// R //设备类型详见型号对应表
	uint16_t REG0_VER; 			// R //软件版本号
	uint32_t REG0_SPC; 			// R //单相两线电流
	uint16_t REG0_SPV; 			// R //单相电压
	uint32_t REG0_SPP; 			// R //单相两线有功功率
	uint32_t REG0_SPQ; 			// R //单相两线无功功率
	uint32_t REG0_SPS; 			// R //单相两线视在功率
	uint64_t REG0_SPSE; 		// R //单相视在电能
	uint16_t REG0_SPTEMP; 		// R //单相L线温
	uint16_t REG0_LEAKAGE_C; 	// R //剩余电流
	uint16_t REG0_TEMPINT; 		// R //内部温度（预留）
	uint32_t REG0_AC; 			// R //A相电流
	uint32_t REG0_BC; 			// R //B相电流
	uint32_t REG0_CC; 			// R //C相电流
	uint32_t REG0_NC; 			// R //零线电流
	uint16_t REG0_RATED_C; 		// R //额定电流
	uint16_t REG0_RSV; 			// R //预留
	uint16_t REG0_VA; 			// R //A相电压
	uint16_t REG0_VB; 			// R //B相电压
	uint16_t REG0_VC; 			// R //C相电压
	uint16_t REG0_VAVG; 		// R //状态映射寄存器
	uint32_t REG0_PA; 			// R //有功功率A
	uint32_t REG0_PB; 			// R //有功功率B
	uint32_t REG0_PC; 			// R //有功功率C
	uint32_t REG0_QA; 			// R //无功功率A
	uint32_t REG0_QB; 			// R //无功功率B
	uint32_t REG0_QC; 			// R //无功功率C
	uint32_t REG0_SA; 			// R //视在功率A
	uint32_t REG0_SB; 			// R //视在功率B
	uint32_t REG0_SC; 			// R //视在功率C
	uint64_t REG0_SEA; 			// R //视在能量A
	uint64_t REG0_SEB; 			// R //视在能量B
	uint64_t REG0_SEC; 			// R //视在能量C
	uint64_t REG0_SESTA; 		// R //合视在能量
	uint16_t REG0_TEMPA; 		// R //A线温度
	uint16_t REG0_TEMPB; 		// R //B线温度
	uint16_t REG0_TEMPC; 		// R //C线温度
	uint16_t REG0_TEMPN; 		// R //N线温度
	uint16_t REG0_SPARK; 		// R //打火报警
	uint16_t REG0_NO_NLINE; 	// R //缺零线
	uint16_t REG0_OVER_HARMONIC;// R //谐波超标
	uint16_t REG0_OPERATOR; 	// R //最后一次分合操作者
	uint64_t REG0_AE; 			// R //单相有功能量
	uint64_t REG0_AEA; 			// R //A相有功能量
	uint64_t REG0_AEB; 			// R //B相有功能量
	uint64_t REG0_AEC; 			// R //C相有功能量
	uint64_t REG0_RAE; 			// R //单相无功能量
	uint64_t REG0_RAEA; 		// R //A相无功能量
	uint64_t REG0_RAEB; 		// R //B相无功能量
	uint64_t REG0_RAEC; 		// R //C相无功能量
	uint16_t REG0_FREQ; 		// R //电网频率
	uint16_t REG0_ALARM_MAP1; 	// R //报警位映射1
	uint16_t REG0_ALARM_MAP2; 	// R //报警位映射2
	uint16_t REG0_ALARM_MAP3; 	// R //报警位映射3
} cb_reg_input;

typedef struct
{
	uint16_t REG1_RATED_CURRENT; 	// RW //额定电流
	uint16_t REG1_SPUV; 			// RW //单相欠压（报警）阈值
	uint16_t REG1_SPUVP; 			// RW //单相欠压（预警）阈值
	uint16_t REG1_SPOV; 			// RW //单相过压（报警）阈值
	uint16_t REG1_SPOVP; 			// RW //单相过压（预警）阈值
	uint16_t REG1_SPVC; 			// RW //单相过流（报警）阈值
	uint16_t REG1_SPVCP; 			// RW //单相过流（预警）阈值
	uint16_t REG1_SPOT; 			// RW //单相线温（报警）阈值
	uint16_t REG1_SPOTP; 			// RW //单相线温（预警）阈值
	uint16_t REG1_SLEAKAGE; 		// RW //软件漏电(报警)电流
	uint16_t REG1_SLEAKAGEP; 		// RW //软件漏电(预警)电流
	uint16_t REG1_OL; 				// RW //过载(报警)阈值
	uint16_t REG1_OLP; 				// RW //过载(预警)阈值
	uint16_t REG1_OP; 				// RW //过功率(报警)阈值
	uint16_t REG1_OPP; 				// RW //过功率(预警)阈值
	uint16_t REG1_UVA; 				// RW //欠压(报警)阈值A
	uint16_t REG1_UVAP; 			// RW //欠压(预警)阈值A
	uint16_t REG1_UVB; 				// RW //欠压(报警)阈值B
	uint16_t REG1_UVBP; 			// RW //欠压(预警)阈值B
	uint16_t REG1_UVC; 				// RW //欠压(报警)阈值C
	uint16_t REG1_UVCP; 			// RW //欠压(预警)阈值C
	uint16_t REG1_OVA; 				// RW //过压(报警)阈值A
	uint16_t REG1_OVAP; 			// RW //过压(预警)阈值A
	uint16_t REG1_OVB; 				// RW //过压(报警)阈值B
	uint16_t REG1_OVBP; 			// RW //过压(预警)阈值B
	uint16_t REG1_OVC; 				// RW //过压(报警)阈值C
	uint16_t REG1_OVCP; 			// RW //过压(预警)阈值C
	uint16_t REG1_OCA; 				// RW //过流(报警)阈值A
	uint16_t REG1_OCA_BK; 			// RW //过流(报警)阈值A
	uint16_t REG1_OCAP; 			// RW //过流(预警)阈值A
	uint16_t REG1_OCAP_BK; 			// RW //过流(预警)阈值A
	uint16_t REG1_OCB; 				// RW //过流(报警)阈值B
	uint16_t REG1_OCB_BK; 			// RW //过流(报警)阈值B
	uint16_t REG1_OCBP; 			// RW //过流(预警)阈值B
	uint16_t REG1_OCBP_BK; 			// RW //过流(预警)阈值B
	uint16_t REG1_OCC; 				// RW //过流(报警)阈值C
	uint16_t REG1_OCC_BK; 			// RW //过流(报警)阈值C
	uint16_t REG1_OCCP; 			// RW //过流(预警)阈值C
	uint16_t REG1_OCCP_BK; 			// RW //过流(预警)阈值C
	uint16_t REG1_OTA; 				// RW //温度(报警)阈值A
	uint16_t REG1_OTAP; 			// RW //温度(预警)阈值A
	uint16_t REG1_OTB; 				// RW //温度(报警)阈值B
	uint16_t REG1_OTBP; 			// RW //温度(预警)阈值B
	uint16_t REG1_OTC; 				// RW //温度(报警)阈值C
	uint16_t REG1_OTCP; 			// RW //温度(预警)阈值C
	uint16_t REG1_OTN; 				// RW //温度(报警)阈值N
	uint16_t REG1_OTNP; 			// RW //温度(预警)阈值N
	uint16_t REG1_UBL; 				// RW //负载不均衡(报警)阈值
	uint16_t REG1_UBLP; 			// RW //负载不均衡(预警)阈值
} cb_reg_holding;

typedef union {
    uint8_t all;  
    struct {
        uint8_t bit0:1;
        uint8_t bit1:1;
        uint8_t bit2:1;
        uint8_t bit3:1;
        uint8_t bit4:1;
        uint8_t bit5:1;
        uint8_t bit6:1;
        uint8_t bit7:1;
    } bit;
} REG8_VALUE;

//状态标志（Discrete Inputs）
//#define REG_DISCRETEBC_SPUV 	(2000 - REG_DISCRETEBC_START)//单相欠压（报警）
//#define REG_DISCRETEBC_SPUVP 	(2001 - REG_DISCRETEBC_START)//单相欠压（预警）
//#define REG_DISCRETEBC_SPOV 	(2002 - REG_DISCRETEBC_START)//单相过压（报警A）
//#define REG_DISCRETEBC_SPOVP 	(2003 - REG_DISCRETEBC_START)//单相过压（预警B）
//#define REG_DISCRETEBC_SPVC 	(2004 - REG_DISCRETEBC_START)//单相过流（报警A）
//#define REG_DISCRETEBC_SPVCP 	(2005 - REG_DISCRETEBC_START)//单相过流（预警B）
//#define REG_DISCRETEBC_SPOT 	(2006 - REG_DISCRETEBC_START)//单相线温（报警C）
//#define REG_DISCRETEBC_SPOTP 	(2007 - REG_DISCRETEBC_START)//单相线温（预警C）
//#define REG_DISCRETEBC_SLEAKAGE (2008 - REG_DISCRETEBC_START)//漏电(报警)
//#define REG_DISCRETEBC_SLEAKAGEP(2009 - REG_DISCRETEBC_START)//漏电(预警)
//#define REG_DISCRETEBC_LEAKAGE 	(2010 - REG_DISCRETEBC_START)//预留
//#define REG_DISCRETEBC_CLOSE 	(2011 - REG_DISCRETEBC_START)//合闸状态
//#define REG_DISCRETEBC_OP 		(2012 - REG_DISCRETEBC_START)//过功率报警(报警)
//#define REG_DISCRETEBC_OPP 		(2013 - REG_DISCRETEBC_START)//过功率报警(预警)
//#define REG_DISCRETEBC_SC 		(2014 - REG_DISCRETEBC_START)//短路报警(报警)
//#define REG_DISCRETEBC_SCP 		(2015 - REG_DISCRETEBC_START)//短路报警(预警)
//#define REG_DISCRETEBC_OL 		(2016 - REG_DISCRETEBC_START)//过载报警(报警)
//#define REG_DISCRETEBC_OLP 		(2017 - REG_DISCRETEBC_START)//过载报警(预警)
//#define REG_DISCRETEBC_LEAKAGE_ACTION (2018 - REG_DISCRETEBC_START)//漏电测试执行结果
//#define REG_DISCRETEBC_ME 		(2019 - REG_DISCRETEBC_START)//重合闸机械故障
//#define REG_DISCRETEBC_PSE 		(2020 - REG_DISCRETEBC_START)//相序错误报警
//#define REG_DISCRETEBC_UVA 		(2021 - REG_DISCRETEBC_START)//欠压（报警）A
//#define REG_DISCRETEBC_UVAP 	(2022 - REG_DISCRETEBC_START)//欠压（预警）A
//#define REG_DISCRETEBC_UVB 		(2023 - REG_DISCRETEBC_START)//欠压（报警）B
//#define REG_DISCRETEBC_UVBP 	(2024 - REG_DISCRETEBC_START)//欠压（预警）B
//#define REG_DISCRETEBC_UVC 		(2025 - REG_DISCRETEBC_START)//欠压（报警）C
//#define REG_DISCRETEBC_UVCP 	(2026 - REG_DISCRETEBC_START)//欠压（预警）C
//#define REG_DISCRETEBC_OVA 		(2027 - REG_DISCRETEBC_START)//过压（报警）A
//#define REG_DISCRETEBC_OVAP 	(2028 - REG_DISCRETEBC_START)//过压（预警）A
//#define REG_DISCRETEBC_OVB (2029 - REG_DISCRETEBC_START)//过压（报警）B
//#define REG_DISCRETEBC_OVBP (2030 - REG_DISCRETEBC_START)//过压（预警）B
//#define REG_DISCRETEBC_OVC (2031 - REG_DISCRETEBC_START)//过压（报警）C
//#define REG_DISCRETEBC_OVCP (2032 - REG_DISCRETEBC_START)//过压（预警）C
//#define REG_DISCRETEBC_OCA (2033 - REG_DISCRETEBC_START)//过流（报警）A
//#define REG_DISCRETEBC_OCAP (2034 - REG_DISCRETEBC_START)//过流（预警）A
//#define REG_DISCRETEBC_OCB (2035 - REG_DISCRETEBC_START)//过流（报警）B
//#define REG_DISCRETEBC_OCBP (2036 - REG_DISCRETEBC_START)//过流（预警）B
//#define REG_DISCRETEBC_OCC (2037 - REG_DISCRETEBC_START)//过流（报警）C
//#define REG_DISCRETEBC_OCCP (2038 - REG_DISCRETEBC_START)//过流（预警）C
//#define REG_DISCRETEBC_OTA (2039 - REG_DISCRETEBC_START)//温度(报警)A
//#define REG_DISCRETEBC_OTAP (2040 - REG_DISCRETEBC_START)//温度(预警)A
//#define REG_DISCRETEBC_OTB (2041 - REG_DISCRETEBC_START)//温度(报警)B
//#define REG_DISCRETEBC_OTBP (2042 - REG_DISCRETEBC_START)//温度(预警)B
//#define REG_DISCRETEBC_OTC (2043 - REG_DISCRETEBC_START)//温度(报警)C
//#define REG_DISCRETEBC_OTCP (2044 - REG_DISCRETEBC_START)//温度(预警)C
//#define REG_DISCRETEBC_OTN (2045 - REG_DISCRETEBC_START)//温度(报警)N
//#define REG_DISCRETEBC_OTNP (2046 - REG_DISCRETEBC_START)//温度(预警)N
//#define REG_DISCRETEBC_LOPA (2047 - REG_DISCRETEBC_START)//缺相(报警)A
//#define REG_DISCRETEBC_LOPAP (2048 - REG_DISCRETEBC_START)//缺相(预警)A
//#define REG_DISCRETEBC_LOPB (2049 - REG_DISCRETEBC_START)//缺相(报警)B
//#define REG_DISCRETEBC_LOPBP (2050 - REG_DISCRETEBC_START)//缺相(预警)B
//#define REG_DISCRETEBC_LOPC (2051 - REG_DISCRETEBC_START)//缺相(报警)C
//#define REG_DISCRETEBC_LOPCP (2052 - REG_DISCRETEBC_START)//缺相(预警)C
//#define REG_DISCRETEBC_UBL (2053 - REG_DISCRETEBC_START)//负载不均衡(报警)
//#define REG_DISCRETEBC_UBLP (2054 - REG_DISCRETEBC_START)//负载不均衡(预警)
//#define REG_DISCRETEBC_SPOVC (2055 - REG_DISCRETEBC_START)//单相过压（报警C）
//#define REG_DISCRETEBC_SPVCC (2056 - REG_DISCRETEBC_START)//单相过流（报警C）
//#define REG_DISCRETEBC_SPOTC (2057 - REG_DISCRETEBC_START)//单相线温（报警C）
typedef struct
{
	REG8_VALUE discrete1;
	REG8_VALUE discrete2;
	REG8_VALUE discrete3;
	REG8_VALUE discrete4;
	REG8_VALUE discrete5;
	REG8_VALUE discrete6;
	REG8_VALUE discrete7;
	REG8_VALUE discrete8;	
} cb_reg_discrete;

extern cb_reg_input reg_in[CIRCUIT_BREAKER_MAX_NUM];
extern cb_reg_holding reg_hold[CIRCUIT_BREAKER_MAX_NUM];
extern cb_reg_discrete reg_discrete[CIRCUIT_BREAKER_MAX_NUM];

void circuit_breaker_process_init(void);

#endif
