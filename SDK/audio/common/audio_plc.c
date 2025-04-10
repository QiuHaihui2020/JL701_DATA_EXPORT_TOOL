#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_plc.data.bss")
#pragma data_seg(".audio_plc.data")
#pragma const_seg(".audio_plc.text.const")
#pragma code_seg(".audio_plc.text")
#endif
/*
 ************************************************************
 *
 *
 *
 ************************************************************
 */
#include "system/includes.h"
#include "PLC.h"
#include "os/os_api.h"
#include "app_config.h"
#include "audio_config.h"
#include "effects/AudioEffect_DataType.h"
#include "media/audio_splicing.h"

#if TCFG_ESCO_PLC

//#define AUDIO_PLC_LOG_ENABLE
#ifdef AUDIO_PLC_LOG_ENABLE
#define PLC_LOG		y_printf
#else
#define PLC_LOG(...)
#endif/*AUDIO_PLC_LOG_ENABLE*/

extern const int config_media_24bit_enable;
enum {
    PLC_STA_CLOSE = 0,
    PLC_STA_OPEN,
    PLC_STA_RUN,
};


#define PLC_FRAME_LEN	120
typedef struct {
    u8 state;
    u8 repair;
    u8 wideband;
    u8 ch_num;
    OS_MUTEX mutex;
    s16 *run_buf;
    af_DataType dataTypeobj;
} audio_plc_t;

void *audio_plc_open(u16 sr, u16 ch_num, af_DataType *dataTypeobj)
{
    PLC_LOG("audio_plc_open:%d\n", sr);
    audio_plc_t *plc = zalloc(sizeof(audio_plc_t));
    /* if (plc->state == PLC_STA_OPEN) { */
    if (!plc) {
        return NULL;
    }
    memcpy(&plc->dataTypeobj, dataTypeobj, sizeof(af_DataType));
    struct esco_plc_parm plc_parm = {0};
    plc_parm.nch = ch_num;
    plc_parm.sr = sr;
    plc->ch_num = ch_num;
    plc->run_buf	= malloc(PLC_query(&plc_parm, dataTypeobj)); /*buf_size:1040*/
    PLC_LOG("PLC_buf:%x,size:%d\n", plc->run_buf, PLC_query(&plc_parm, dataTypeobj));
    if (plc->run_buf) {
        s8 err = PLC_init(plc->run_buf, &plc_parm, dataTypeobj);
        if (err) {
            PLC_LOG("PLC_init err:%d", err);
            free(plc->run_buf);
            free(plc);
            return NULL;
        }
        os_mutex_create(&plc->mutex);
        if (sr == 16000) {
            plc->wideband = 1;
        }
        plc->state = PLC_STA_OPEN;
    }
    PLC_LOG("audio_plc_open succ\n");
    return plc;
}

void audio_plc_run_base(audio_plc_t *plc, s16 *data, u16 len, u8 repair)
{
    if (!plc) {
        return ;
    }

    u16 repair_point, tmp_point;
    s16 *p_in, *p_out;
    p_in    = data;
    p_out   = data;
    u16 point_offset = plc->dataTypeobj.IndataBit ? 2 : 1;
    tmp_point = len >> point_offset;
    u8 repair_flag = 0;

    os_mutex_pend(&plc->mutex, 0);
    if (plc->state == PLC_STA_CLOSE) {
        os_mutex_post(&plc->mutex);
        return;
    }

#if 0	//debug
    static u16 repair_cnt = 0;
    if (repair) {
        repair_cnt++;
        y_printf("[E%d]", repair_cnt);
    } else {
        repair_cnt = 0;
    }
    //printf("[%d]",point);
#endif/*debug*/

    repair_flag = repair;
    if (plc->wideband) {
        /*
         *msbc plc deal
         *如果上一帧是错误，则当前帧也要修复
         */
        if (plc->repair) {
            repair_flag = 1;
        }
        plc->repair = repair;
    }

    while (tmp_point) {
        repair_point = (tmp_point > PLC_FRAME_LEN) ? PLC_FRAME_LEN : tmp_point;
        tmp_point = tmp_point - repair_point;
        PLC_run(p_in, p_out, repair_point, repair_flag);
        p_in  = (s16 *)((int)p_in + (repair_point << point_offset));
        p_out = (s16 *)((int)p_out + (repair_point << point_offset));
    }
    os_mutex_post(&plc->mutex);
}

void audio_plc_run(void *_plc, s16 *data, u16 len, u8 repair)
{
    audio_plc_t *plc = _plc;
    if (!plc) {
        return ;
    }
    if (plc->ch_num == 1) {
        audio_plc_run_base(plc, data, len, repair);
    } else {
        if (plc->dataTypeobj.IndataBit) {
            if (config_media_24bit_enable) {
                pcm_dual_to_single_32bit(data, data, len);
                void *tmp_data = (void *)((int)data + len / 2);
                audio_plc_run_base(plc, tmp_data, len / 2, repair);
                pcm_single_to_dual_32bit(data, tmp_data, len / 2);
            }
        } else {
            pcm_dual_to_single(data, data, len);
            void *tmp_data = (void *)((int)data + len / 2);
            audio_plc_run_base(plc, tmp_data, len / 2, repair);
            pcm_single_to_dual(data, tmp_data, len / 2);
        }
    }

}
int audio_plc_close(void *_plc)
{
    audio_plc_t *plc = _plc;
    if (!plc) {
        return -1;
    }

    PLC_LOG("audio_plc_close\n");
    os_mutex_pend(&plc->mutex, 0);
    plc->state = PLC_STA_CLOSE;
    if (plc->run_buf) {
        free(plc->run_buf);
        plc->run_buf = NULL;
    }
    os_mutex_post(&plc->mutex);
    free(plc);
    plc = NULL;
    PLC_LOG("audio_plc_close succ\n");
    return 0;
}
#else
void *audio_plc_open(u16 sr, u16 ch_num, af_DataType *dataTypeobj)
{
    return 0;
}

void audio_plc_run(void *_plc, s16 *data, u16 len, u8 repair)
{
}

int audio_plc_close(void *_plc)
{
    return 0;
}
#endif/*TCFG_ESCO_PLC*/
