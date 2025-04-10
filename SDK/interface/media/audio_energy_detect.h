/*******************************************************************************************
  File : audio_energy_detect.h
  By   : LinHaibin
  brief: 数据能量检测
         memory : 148 + 56 * channels (Byte)
         mips:
            dcc   mode: 3MHz   / 44100 points / 2channel / sec
            nodcc mode: 2.3MHz / 44100 points / 2channel / sec
  Email: linhaibin@zh-jieli.com
  date : Fri, 24 Jul 2020 18:11:37 +0800
********************************************************************************************/
#ifndef _AUDIO_ENERGY_DETECT_H_
#define _AUDIO_ENERGY_DETECT_H_

#include "typedef.h"

#define AUDIO_E_DET_UNMUTE      (0x00)
#define AUDIO_E_DET_MUTE        (0x01)

typedef struct _audio_energy_detect_param {
    s16 mute_energy;                    // mute 阈值
    s16 unmute_energy;                  // unmute 阈值
    u16 mute_time_ms;                   // 能量低于mute阈值进入mute状态时间
    u16 unmute_time_ms;                 // 能量高于unmute阈值进入unmute状态时间
    u32 sample_rate;                    // 采样率
    void *prive;
    void (*event_handler)(void *prive, u8, u8, u8 ch_total);      // 事件回调函数 event channel
    u16 count_cycle_ms;                 // 能量计算的时间周期
    u8  ch_total;                       // 通道总数
    u8  pcm_mute;                    // 保留，未使用
    u8  onoff;                       // 保留，未使用
    u8  skip;                        // 1:数据不处理 0:处理
    u8  dcc;                         // 1:去直流打开 0:关闭
    u8 bit_width;
} audio_energy_detect_param;

extern void *audio_energy_detect_entry_get(void *_hdl);
extern void *audio_energy_detect_open(audio_energy_detect_param *param);
extern int audio_energy_detect_run(void *_hdl, s16 *data, u32 len);
extern int audio_energy_detect_close(void *_hdl);
extern int audio_energy_detect_skip(void *_hdl, u32 channel, u8 skip);
extern int audio_energy_detect_energy_get(void *_hdl, u8 ch);
extern int audio_energy_detect_sample_rate_update(void *_hdl, u32 sample_rate);
extern void audio_energy_set_event_handler(void *_hdl, void *prive, void (*event_handler)(void *prive, u8, u8, u8 ch_total));


struct energy_detect_parm_tool_cfg {
    s16 is_bypass;
    s16 mute_energy;                    // mute 阈值 (0~32767)
    s16 unmute_energy;                  // unmute 阈值(0~32767) unmute > mute
    u16 mute_time_ms;                   // 能量低于mute阈值进入mute状态时间(1~30000)ms (min >= cyc_time)
    u16 unmute_time_ms;                 // 能量高于unmute阈值进入unmute状态时间(1~30000)ms
    u16 count_cycle_ms;                 // 能量计算的时间周期(1~1000)ms
    u16 dcc;                         // 1:去直流打开 0:关闭
} __attribute__((packed));

struct energy_detect_get_parm {
    char name[16];
    int type;
    void (*event_handler)(void *prive, u8, u8, u8 ch_total);    // 事件回调函数 event channel
};


struct audio_get_energy {
    char name[16];
    int tar_ch;//目标通道,(BIT(0)对应通道0，如此类推)
    int energy;//目标通道的能量值
};

#define SET_ENERGY_DET_EVENT_HANDLER 0xaa
/******************************************************************************
                 audio energy detect demo

INCLUDE:
	#include "application/audio_energy_detect.h"
	void *audio_e_det_hdl = NULL;
	struct list_head *audio_e_det_entry = NULL;
	void test_e_det_handler(void *prive,u8 event, u8 ch)
	{
		printf(">>>> ch:%d %s\n", ch, event ? ("MUTE") : ("UNMUTE"));
        // ch < ch_total 时：表示通道(ch)触发的事件
        // ch = ch_total 时：表示全部通道都触发的事件
	}

OPEN:
    audio_energy_detect_param e_det_param = {0};
    e_det_param.mute_energy = 5;
    e_det_param.unmute_energy = 10;
    e_det_param.mute_time_ms = 100;
    e_det_param.unmute_time_ms = 50;
    e_det_param.count_cycle_ms = 100;
    e_det_param.sample_rate = 44100;
    e_det_param.event_handler = test_e_det_handler;
    e_det_param.ch_total = 2;
    e_det_param.dcc = 1;
    audio_e_det_hdl = audio_energy_detect_open(&e_det_param);

    1.接入 audio stream
        audio_e_det_entry = audio_energy_detect_entry_get(audio_e_det_hdl);
        entries[entry_cnt++] = audio_e_det_entry;
    2.手动调用
        audio_energy_detect_run(audio_e_det_hdl, data, len);
        // data:需要运算的音频数据起始地址  len:需要运算的音频数据总字节长度

CLOSE:
	audio_energy_detect_close(audio_e_det_hdl);

******************************************************************************/

#endif  // #ifndef _AUDIO_ENERGY_DETECT_H_

