#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "app_config.h"
#include "system/includes.h"
#include "printf.h"
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"
#include "generic/circular_buf.h"
#include "system/event.h"

#include "usb/device/uac_audio.h"
#include "uac_stream.h"
#include "audio_config.h"
#include "pc_spk_player.h"
#include "pc_spk_file.h"
#include "pc_mic_recoder.h"
#include "app_msg.h"


#define LOG_TAG_CONST       USB
#define LOG_TAG             "[UAC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#define UAC_SPEAKER_STREAM_DETECT_INTERVAL  2
#define UAC_SPEAKER_STREAM_DETECT_COUNT     5

static volatile u8 speaker_stream_is_open = 0;
static volatile u8 mic_stream_is_open = 0;

struct uac_speaker_handle {
    cbuffer_t cbuf;
    volatile u8 need_resume;
    u8 channel;
    u8 stream_state;
    u32 samplerate;
    u32 bitwidth;
    u32 timestamp;
    //void (*rx_handler)(int, void *, int);
    u16 timer_id;
    u16 stream_halt_cnt;
};

static void (*uac_rx_handler)(int, void *, int) = NULL;

static u16 speaker_close_tid = 0;

static struct uac_speaker_handle *uac_speaker = NULL;

#if USB_MALLOC_ENABLE
#else
static struct uac_speaker_handle uac_speaker_handle SEC(.usb.data.bss.exchange);
#endif

u8 uac_audio_is_24bit_in_4byte()
{
    return UAC_24BIT_IN_4BYTE;
}

void set_uac_speaker_rx_handler(void *priv, void (*rx_handler)(int, void *, int))
{
    uac_rx_handler = rx_handler;
    /* if (uac_speaker) { */
    /* uac_speaker->rx_handler = rx_handler; */
    /* } */
}


//这里起中断往后面推数
void uac_speaker_stream_write(const u8 *obuf, u32 len)
{

    if (!uac_speaker) {
        return;
    }

    uac_speaker->stream_halt_cnt = 0;
    if (speaker_stream_is_open) {
        if (uac_speaker->stream_state == 0) {
            /* g_printf("-APP_MSG_PC_AUDIO_PLAY_OPEN-"); */
            app_send_message(APP_MSG_PC_AUDIO_PLAY_OPEN,
                             (int)(((uac_speaker->bitwidth == 24 ? 1 : 0) << 28) |
                                   (uac_speaker->channel << 24) |
                                   uac_speaker->samplerate));
            uac_speaker->stream_state = 1;
        }
    }

#if 0  //multiple channel test
    if (uac_speaker) {
        if (uac_speaker->channel == 4) {
            s16 *ch0 = (s16 *)obuf;
            s16 *ch1 = (s16 *)obuf;
            ch1++;
            for (int i = 0; i < len / 8; i++) {
                ch0[i * 2] = ch0[i * 4];
                ch1[i * 2] = ch0[i * 4 + 1];
            }
            len /= 2;
        } else if (uac_speaker->channel == 6) {
            s16 *ch0 = (s16 *)obuf;
            s16 *ch1 = (s16 *)obuf;
            ch1++;
            for (int i = 0; i < len / 12; i++) {
                ch0[i * 2] = ch0[i * 6];
                ch1[i * 2] = ch0[i * 6 + 1];
            }
            len /= 3;
        } else if (uac_speaker->channel == 8) {
            s16 *ch0 = (s16 *)obuf;
            s16 *ch1 = (s16 *)obuf;
            ch1++;
            for (int i = 0; i < len / 16; i++) {
                ch0[i * 2] = ch0[i * 8];
                ch1[i * 2] = ch0[i * 8 + 1];
            }
            len /= 4;
        }
    }
#endif
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    pc_spk_data_isr_cb((void *)obuf, len);
#endif
}

static void uac_speaker_stream_halt_detect(void *priv)
{
    if (!uac_speaker) {
        return;
    }
    if (uac_speaker->stream_halt_cnt >= UAC_SPEAKER_STREAM_DETECT_COUNT) {
        if (uac_speaker->stream_state) {
            /* g_printf("-APP_MSG_PC_AUDIO_PLAY_CLOSE-"); */
            app_send_message(APP_MSG_PC_AUDIO_PLAY_CLOSE, 0);
            uac_speaker->stream_state = 0;
        }
    }
    uac_speaker->stream_halt_cnt++;
}

void uac_speaker_stream_get_volume(u16 *l_vol, u16 *r_vol)
{
    uac_get_cur_vol(0, l_vol, r_vol);
}

u8 uac_speaker_stream_status(void)
{
    return speaker_stream_is_open;
}

void uac_speaker_stream_open(u32 samplerate, u32 ch, u32 bitwidth)
{
    u32 last_sr = 0;

#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    last_sr = pc_spk_get_fmt_sample_rate();
#endif
    if (speaker_stream_is_open) {
        if (speaker_close_tid) {
            sys_hi_timeout_del(speaker_close_tid);
            speaker_close_tid = 0;
        }
        if (uac_speaker->samplerate != samplerate ||
            uac_speaker->channel != ch ||
            uac_speaker->bitwidth != bitwidth) {
            uac_speaker->samplerate = samplerate;
            uac_speaker->channel = ch;
            uac_speaker->bitwidth = bitwidth;
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
            pc_spk_set_fmt(ch, bitwidth, samplerate);
            //快速切换位宽 or 采样率 or 声道数时，如果数据流运行中
            //（stream_state = 1），立即响应切换
            if (uac_speaker->stream_state) {
                app_send_message(APP_MSG_PC_AUDIO_PLAY_CLOSE, 0);
                app_send_message(APP_MSG_PC_AUDIO_PLAY_OPEN,
                                 (int)(((bitwidth == 24 ? 1 : 0) << 28) |
                                       (ch << 24) |
                                       samplerate));
            }
#endif
        }
        uac_speaker->timestamp = jiffies;
        return;
    }
    log_info("%s", __func__);

    if (!uac_speaker) {
#if USB_MALLOC_ENABLE
        uac_speaker = zalloc(sizeof(struct uac_speaker_handle));
        if (!uac_speaker) {
            return;
        }
#else
        uac_speaker = &uac_speaker_handle;
        memset(uac_speaker, 0, sizeof(struct uac_speaker_handle));
#endif
    }
    //uac_speaker->rx_handler = NULL;
    uac_speaker->channel = ch;
    uac_speaker->samplerate = samplerate;
    uac_speaker->bitwidth = bitwidth;

    speaker_stream_is_open = 1;

#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    log_info(">> Case : USB_AUDIO_PLAY_OPEN\n");
    pc_spk_set_fmt(ch, bitwidth, samplerate);
    //不在这里打开了，在数据流中断处打开，检测到有数据才打开
    //pcspk_open_player_by_taskq();
    //app_send_message(APP_MSG_PC_AUDIO_PLAY_OPEN, (int)((ch << 24) | samplerate));
#endif

    uac_speaker->timestamp = jiffies;
    uac_speaker->timer_id = sys_hi_timer_add(NULL, uac_speaker_stream_halt_detect, UAC_SPEAKER_STREAM_DETECT_INTERVAL);
}

void uac_speaker_stream_close_delay(void *priv)
{
    int release = (int)priv;
    speaker_close_tid = 0;
    if (speaker_stream_is_open == 0) {
        return;
    }

    log_info("%s", __func__);
    speaker_stream_is_open = 0;

    if (uac_speaker) {
        if (uac_speaker->timer_id) {
            sys_hi_timer_del(uac_speaker->timer_id);
            uac_speaker->timer_id = 0;
        }
        if (uac_speaker->stream_state) {
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
            log_info(">> Case : USB_AUDIO_PLAY_CLOSE\n");
            if (release) {
                pc_spk_player_close();
            } else {
                pcspk_close_player_by_taskq();
            }
            app_send_message(APP_MSG_PC_AUDIO_PLAY_CLOSE, 0);
#endif
        }
#if USB_MALLOC_ENABLE
        free(uac_speaker);
#endif
        uac_speaker = NULL;
    }
}

void uac_speaker_stream_close(int release)
{
    if (speaker_stream_is_open == 0) {
        return;
    }

    if (uac_speaker) {
        if (jiffies_to_msecs(jiffies - uac_speaker->timestamp) > 150 || release) {
            if (speaker_close_tid) {
                sys_hi_timeout_del(speaker_close_tid);
                speaker_close_tid = 0;
            }
            uac_speaker_stream_close_delay((void *)release);
        } else {
            if (speaker_close_tid == 0) {
                speaker_close_tid = sys_hi_timeout_add(NULL, uac_speaker_stream_close_delay, 150);
            } else {
                sys_hi_timeout_modify(speaker_close_tid, 150);
            }
        }
    }
}

int uac_get_spk_vol()
{
    int max_vol = app_audio_volume_max_query(AppVol_USB);
    int vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    if (vol * 100 / max_vol < 100) {
        return vol * 100 / max_vol;
    } else {
        return 99;
    }
    return 0;
}

void uac_mute_volume(u32 type, u32 l_vol, u32 r_vol)
{
    static u32 last_spk_l_vol = (u32) - 1, last_spk_r_vol = (u32) - 1;
    static u32 last_mic_vol = (u32) - 1;

    switch (type) {
    case MIC_FEATURE_UNIT_ID: //MIC
        if (mic_stream_is_open == 0) {
            return ;
        }
        if (l_vol == last_mic_vol) {
            return;
        }
        last_mic_vol = l_vol;
        pc_mic_set_volume_by_taskq(l_vol);
        break;
    case SPK_FEATURE_UNIT_ID: //SPK
        /* if (speaker_stream_is_open == 0) { */
        /*     return; */
        /* } */
        if (l_vol == last_spk_l_vol && r_vol == last_spk_r_vol) {
            return;
        }
        last_spk_l_vol = l_vol;
        last_spk_r_vol = r_vol;

        //TODO
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        printf(">> PC, r_vol:%d, l_vol:%d\n", r_vol, l_vol);
        /* app_audio_set_volume(APP_AUDIO_STATE_MUSIC, (r_vol + l_vol) / 2, 1); */
        pcspk_set_volume_by_taskq();
#endif
        break;
    default:
        break;
    }
}


static int (*mic_tx_handler)(void *, void *, int) = NULL;
static void *mic_tx_handler_priv = NULL;
static u32 mic_close_tid = 0;
static u32 mic_samplerate;
static u32 mic_bitwidth;
static u8  mic_channel;
static u32 mic_timestamp;

int uac_mic_stream_read(u8 *buf, u32 len)
{
    if (mic_stream_is_open == 0) {
        memset(buf, 0, len);
        return len;
    }
#if 0//48K 1ksin
    const s16 sin_48k[] ALIGNED(4) = {
        0, 2139, 4240, 6270, 8192, 9974, 11585, 12998,
        14189, 15137, 15826, 16244, 16384, 16244, 15826, 15137,
        14189, 12998, 11585, 9974, 8192, 6270, 4240, 2139,
        0, -2139, -4240, -6270, -8192, -9974, -11585, -12998,
        -14189, -15137, -15826, -16244, -16384, -16244, -15826, -15137,
        -14189, -12998, -11585, -9974, -8192, -6270, -4240, -2139
    };
    const u8 sin_16k[] ALIGNED(4) = {
        0x00, 0x00, 0xFB, 0x30, 0x81, 0x5A, 0x41, 0x76,
        0xFE, 0x7F, 0x41, 0x76, 0x82, 0x5A, 0xFC, 0x30,
        0x00, 0x00, 0x04, 0xCF, 0x7E, 0xA5, 0xC0, 0x89,
        0x01, 0x80, 0xBF, 0x89, 0x7E, 0xA5, 0x04, 0xCF,
    };
    s16 *sin_wav = NULL;
    if (mic_samplerate == 48000) {
        sin_wav = (s16 *)sin_48k;
    } else if (mic_samplerate == 16000) {
        sin_wav = (s16 *)sin_16k;
    } else {
        sin_wav = (s16 *)sin_48k;
        /* ASSERT(0, "unsupport samplerate: %d", mic_samplerate); */
    }
    if (mic_channel == 1) {
        memcpy(buf, sin_wav, len);
    } else if (mic_channel == 2) {
        u16 *l_ch = (u16 *)buf;
        u16 *r_ch = (u16 *)buf;
        r_ch++;
        for (int i = 0; i < len / 4; i++) {
            *l_ch = sin_wav[i];
            *r_ch = sin_wav[i];
            l_ch += 2;
            r_ch += 2;
        }
    } else if (mic_channel == 4) {
        u16 *ch0 = (u16 *)buf;
        u16 *ch1 = (u16 *)buf;
        u16 *ch2 = (u16 *)buf;
        u16 *ch3 = (u16 *)buf;
        ch1 += 1, ch2 += 2, ch3 += 3;
        for (int i = 0; i < len / 8; i++) {
            ch0[i * 4] = sin_wav[i];
            ch1[i * 4] = sin_wav[i];
            ch2[i * 4] = sin_wav[i];
            ch3[i * 4] = sin_wav[i];
        }
    } else if (mic_channel == 8) {
        u16 *ch0 = (u16 *)buf;
        u16 *ch1 = (u16 *)buf;
        u16 *ch2 = (u16 *)buf;
        u16 *ch3 = (u16 *)buf;
        u16 *ch4 = (u16 *)buf;
        u16 *ch5 = (u16 *)buf;
        u16 *ch6 = (u16 *)buf;
        u16 *ch7 = (u16 *)buf;
        ch1 += 1, ch2 += 2, ch3 += 3, ch4 += 4, ch5 += 5, ch6 += 6, ch7 += 7;
        for (int i = 0; i < len / 16; i++) {
            ch0[i * 8] = sin_wav[i];
            ch1[i * 8] = sin_wav[i];
            ch2[i * 8] = sin_wav[i];
            ch3[i * 8] = sin_wav[i];
            ch4[i * 8] = sin_wav[i];
            ch5[i * 8] = sin_wav[i];
            ch6[i * 8] = sin_wav[i];
            ch7[i * 8] = sin_wav[i];
        }
    }
    return len;
#else
    if (mic_tx_handler) {
        return mic_tx_handler(mic_tx_handler_priv, buf, len);
    } else {
        //putchar('N');
        memset(buf, 0, len);
    }
    return len;
#endif
}

void set_uac_mic_tx_handler(void *priv, int (*tx_handler)(void *, void *, int))
{
    mic_tx_handler = tx_handler;
    mic_tx_handler_priv = priv;
}

void uac_mic_stream_get_volume(u16 *vol)
{
    *vol = uac_get_mic_vol(0);
}

u8 uac_get_mic_stream_status(void)
{
    return mic_stream_is_open;
}

u32 uac_mic_stream_open(u32 samplerate, u32 ch, u32 bitwidth)
{
    u32 last_sr = 0;

#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    last_sr = pc_mic_get_fmt_sample_rate();
    pc_mic_set_fmt(ch, bitwidth, samplerate);
#endif

    if (mic_stream_is_open) {
        if (mic_close_tid) {
            sys_hi_timeout_del(mic_close_tid);
            mic_close_tid = 0;
        }
        if (mic_samplerate != samplerate ||
            mic_channel != ch ||
            mic_bitwidth != bitwidth) {
            mic_tx_handler = NULL;	//添加测试
            mic_samplerate = samplerate;
            mic_bitwidth = bitwidth;
            mic_channel = ch;
#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
            //重启mic recorder
            pc_mic_recoder_restart_by_taskq();
            app_send_message(APP_MSG_PC_AUDIO_MIC_CLOSE, 0);
            app_send_message(APP_MSG_PC_AUDIO_MIC_OPEN,
                             (int)(((bitwidth == 24 ? 1 : 0) << 28) |
                                   (ch << 24) |
                                   samplerate));
#endif
        }
        mic_timestamp = jiffies;
        return 0;
    }

    mic_tx_handler = NULL;	//这里需要赋值为NULL
    mic_samplerate = samplerate;
    mic_bitwidth = bitwidth;
    mic_channel = ch;

    log_info("%s", __func__);

    mic_stream_is_open = 1;

#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    log_info("## Func:%s, Line:%d, Open Mic!!\n", __func__, __LINE__);
    pc_mic_recoder_open_by_taskq();
    app_send_message(APP_MSG_PC_AUDIO_MIC_OPEN,
                     (int)(((bitwidth == 24 ? 1 : 0) << 28) |
                           (ch << 24) |
                           samplerate));
#endif

    mic_timestamp = jiffies;
    return 0;
}

static void uac_mic_stream_close_delay(void *priv)
{
    int release = (u8)priv;
    mic_close_tid = 0;
    log_info("%s", __func__);
    mic_stream_is_open = 0;

#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    log_info("## Func:%s, Line:%d, Close Mic!!\n", __func__, __LINE__);
    if (release) {
        pc_mic_recoder_close();
    } else {
        pc_mic_recoder_close_by_taskq();
    }
    app_send_message(APP_MSG_PC_AUDIO_MIC_CLOSE, 0);
#endif
}

void uac_mic_stream_close(int release)
{
    if (mic_stream_is_open == 0) {
        return ;
    }
    //FIXME:
    //未知原因出现频繁开关mic，导致出现audio或者蓝牙工作异常，
    //收到mic关闭命令后延时150ms再发消息通知audio模块执行关闭动作
    //如果在150ms之内继续收到usb下发的关闭命令，则继续推迟150ms。
    if (jiffies_to_msecs(jiffies - mic_timestamp) >= 150 || release) {
        if (mic_close_tid) {
            sys_hi_timeout_del(mic_close_tid);
            mic_close_tid = 0;
        }
        uac_mic_stream_close_delay((void *)release);
    } else {
        if (mic_close_tid == 0) {
            mic_close_tid = sys_hi_timeout_add(NULL, uac_mic_stream_close_delay, 150);
        } else {
            sys_hi_timeout_modify(mic_close_tid, 150);
        }
    }
}

