
#ifndef __ICSD_DOT_APP_H_
#define __ICSD_DOT_APP_H_

#include "typedef.h"
#include "icsd_dot.h"

#define ICSD_DOT_EN_MODE_NEXT_SW		1	/*贴合度提示音播完才允许下一次切换*/

//ICSD DOT 状态
enum {
    ICSD_DOT_STATE_CLOSE = 0,
    ICSD_DOT_STATE_OPEN,
};

//ICSD DOT TONE状态
enum {
    ICSD_DOT_STATE_TONE_STOP = 0,
    ICSD_DOT_STATE_TONE_START,
};

/*
   贴合度检测打开
	param: tone_en  1 使用提示音检测； 0 使用音乐检测(可能存在结果不可靠的问题)
		   result_cb,	检查结果func_callback
	return 0  打开成功； 1 打开失败
*/
int audio_icsd_dot_open(u8 tone_en, void (*result_cb)(u8 result));

//贴合度检测关闭
int audio_icsd_dot_close();

int audio_icsd_dot_end(int result_thr);

//查询DOT是否活动中
u8 audio_icsd_dot_is_running(void);

void audio_icsd_dot_anc_dma_done(void);

#endif/*__ICSD_DOT_APP_H_*/

