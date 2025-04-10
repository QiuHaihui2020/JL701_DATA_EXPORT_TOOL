#ifndef _AUDIO_DUT_CONTROL_
#define _AUDIO_DUT_CONTROL_

#include "generic/typedef.h"

/*******************CVP测试命令集*********************/
#define	CVP_DUT_DMS_MASTER_MIC				0x01		//DMS-测试主MIC		    SMS-关闭CVP算法
#define	CVP_DUT_DMS_SLAVE_MIC   			0x02 		//DMS-测试副MIC
#define CVP_DUT_DMS_OPEN_ALGORITHM    		0x03		//DMS-测试dms算法	    SMS-打开CVP算法

#define CVP_DUT_NS_OPEN						0x04		//打开NS模块
#define CVP_DUT_NS_CLOSE					0x05		//关闭NS模块

//ANC MIC 频响测试指令，且需切换到CVP_DUT_MODE_BYPASS，在通话HFP前设置
#define CVP_DUT_FF_MIC						0x06		//测试TWS FF MIC/头戴式左耳FF_MIC
#define CVP_DUT_FB_MIC						0x07		//测试TWS FB MIC/头戴式左耳FB_MIC
#define CVP_DUT_HEAD_PHONE_R_FF_MIC			0x08		//测试头戴式右耳FF_MIC
#define CVP_DUT_HEAD_PHONE_R_FB_MIC			0x09		//测试头戴式右耳FB_MIC

#define CVP_DUT_OPEN_ALGORITHM				0x0a		//CVP算法开启
#define CVP_DUT_CLOSE_ALGORITHM				0x0b		//CVP算法关闭
#define	CVP_DUT_MODE_ALGORITHM_SET			0x0c		//CVP_DUT 开关算法模式
#define CVP_DUT_POWEROFF					0x0d		//耳机关机指令

//三麦通话测试指令
#define CVP_DUT_3MIC_MASTER_MIC             CVP_DUT_DMS_MASTER_MIC      //测试三麦算法主麦频响
#define CVP_DUT_3MIC_SLAVE_MIC              CVP_DUT_DMS_SLAVE_MIC       //测试三麦算法副麦频响
#define CVP_DUT_3MIC_FB_MIC                 0x0e                        //测试三麦算法FBmic频响
#define CVP_DUT_3MIC_OPEN_ALGORITHM         CVP_DUT_DMS_OPEN_ALGORITHM  //测试三麦算法副麦频响

//======================以下命令，旧命令结构不支持=================================

#define CVP_ENC_MIC_DIFF_CMP_SET                       0x0f        //设置补偿值(主MIC - 副MIC)
#define CVP_ENC_MIC_DIFF_CMP_EN                        0x10        //补偿使能
#define CVP_ENC_MIC_DIFF_CMP_CLEAN                     0x11        //清除补偿

#define CVP_DUT_CVP_IOCTL           		0x12		//CVP算法单元独立控制

#define AUDIO_DUT_DEC_DUT_SET				0x13        //DEC 产测模式设置 (bypass数据处理)
#define CVP_ENC_TARGET_MIC_DIFF_SET         0x14        //设置MIC与目标金机MIC差值
#define CVP_ENC_TARGET_MIC_DIFF_GET         0x15        //读取MIC与目标金机MIC差值

//-------成功回复------
#define AUDIO_DUT_ACK_SUCCESS				0x01		//命令接收成功
#define AUDIO_DUT_ACK_DATA					0x10		//命令接收成功，回复指定数据
//-------失败回复------
#define AUDIO_DUT_ACK_ERR_UNKNOW			0xA1		//未知命令
#define AUDIO_DUT_ACK_CRC_ERR				0xA2		//CRC错误
#define AUDIO_DUT_ACK_VM_WRITE_ERR			0xA3		//VM写入错误
/*******************CVP测试命令集END*********************/

#define AUDIO_DUT_SPP_MAGIC					0x55BB
#define AUDIO_DUT_PACK_NUM					sizeof(audio_dut_data_t)			//数据包总长度

#define AUDIO_DUT_NEW_SPP_MAGIC			    0x55CC

typedef enum {
    /*
       CVP_DUT 算法模式
       通话中设置命令，可自由开关每个算法，测试性能指标
    */
    CVP_DUT_MODE_ALGORITHM = 1,

    /*
       CVP_DUT BYPASS模式
       通话前设置命令，不经过算法，可自由选择需要测试的MIC
       每次通话结束默认恢复成 CVP_DUT算法模式
    */
    CVP_DUT_MODE_BYPASS    = 2,

} CVP_DUT_mode_t;

typedef enum {
    CVP_3MIC_OUTPUT_SEL_DEFAULT = 0,     /*默认输出*/
    CVP_3MIC_OUTPUT_SEL_MASTER,          /*Talk mic原始数据*/
    CVP_3MIC_OUTPUT_SEL_SLAVE,           /*FF mic原始数据*/
    CVP_3MIC_OUTPUT_SEL_FBMIC,           /*FB mic原始数据*/
} CVP_3MIC_OUTPUT_ENUM;

//DEC 产测模式
typedef enum {
    AUDIO_DEC_DUT_MODE_CLOSE = 0,     /*DEC_DUT 关闭*/
    AUDIO_DEC_DUT_MODE_BYPASS,        /*DEC_DUT 开启：bypass所有数据处理*/
    AUDIO_DEC_DUT_MODE_SPK_EQ,        /*DEC_DUT 开启：仅保留SPK_EQ*/
} audio_dec_dut_mode_t;

typedef struct {
    u16 magic;
    u16 crc;
    u8 dat[4];
} audio_dut_data_t;

typedef struct {
    u8 parse_seq;
    u8 mic_ch_sel;				//bypass模式下，MIC通道号
    u8 esco_cfg_mic_ch;			//当前通话MIC通道配置
    u8 reset_flag;				//复位标志
    u8 mode;					//CVP DUT MODE
    u8 dec_dut_enable;			//DEC DUT 模式
    audio_dut_data_t rx_buf;		//spp接收buf
    audio_dut_data_t tx_buf;		//spp发送buf
    void (*ack_hdl)(u8 ack, u8 *packet, u8 size);
} audio_dut_t;
//audio_dut 测试初始化接口
void audio_dut_init(void);

//audio_dut 卸载接口
void audio_dut_unit(void);

//cvp_dut MIC 通道 获取接口
u8 cvp_dut_mic_ch_get(void);

//cvp_dut 模式获取
u8 cvp_dut_mode_get(void);

//cvp_dut 模式设置
void cvp_dut_mode_set(u8 mode);

/*
   DEC产测状态获取
	param: music_decide 1 需判断播歌状态； 0 不需判断
*/
u8 audio_dec_dut_en_get(u8 music_decide);

//audio_dut 数据解析函数
int audio_dut_new_rx_packet(u8 *dat, u8 len);

//audio_dut 注册 send data接口, 默认走spp
void audio_dut_new_send_data_handler(void (*handler)(u8 ack, u8 *packet, u8 size));

#endif/*_AUDIO_DUT_CONTROL_*/
