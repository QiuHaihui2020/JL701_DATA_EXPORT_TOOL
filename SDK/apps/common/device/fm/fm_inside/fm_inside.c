#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".fm_inside.data.bss")
#pragma data_seg(".fm_inside.data")
#pragma const_seg(".fm_inside.text.const")
#pragma code_seg(".fm_inside.text")
#endif
#include "app_config.h"
#include "system/includes.h"
#include "fm_manage.h"
#include "asm/fm_inside_api.h"
#include "asm/dac.h"
#include "audio_config.h"
#include "media/audio_base.h"
#include "overlay_code.h"
#include "clock.h"
#include "fm_inside.h"
#include "fm_player.h"
#include "mic_effect.h"
#include "fm_api.h"
#include "tone_player.h"
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
#include "app_le_broadcast.h"
#endif

#if TCFG_APP_FM_EN
#if(TCFG_FM_INSIDE_ENABLE == ENABLE)



///--------------------------FM_INSIDE_API------------------------
static u16 fm_inside_agc_timer = 0;
static u8 fm_inside_dac_clk = 0;
u8 fm_get_scan_flag(void);
static void __fm_inside_agc_trim(void *priv)
{
    if (!fm_get_scan_flag()) {
        fm_inside_agc_trim(0xff);
    }
}


u8 fm_inside_init(void *priv)
{
    puts("fm_insice_init\n");

    fm_inside_dac_clk = audio_dac_clk_get();
    overlay_load_code(OVERLAY_FM);

    fm_inside_on();  //fm analog init
    fm_inside_set_stereo(TCFG_FM_INSIDE_STEREO_ENABLE);  //0 mono, 1 stereo.
#if TCFG_FM_INSIDE_AGC_ENABLE
    fm_inside_agc_en_set(1);
    fm_inside_agc_timer = sys_timer_add(NULL, __fm_inside_agc_trim, 1000);
#else
    fm_inside_agc_trim(TCFG_FM_INSIDE_AGC_LEVEL);
    fm_inside_agc_en_set(0);
#endif

    return 0;
}

void fm_inside_dac_clk_set(u32 freq)
{
    u8 target_dac_clk = 0;
    u8 cur_dac_clk = audio_dac_clk_get();

    if ((freq % 6000) == 0) {
        target_dac_clk = 1;
    } else {
        target_dac_clk = 0;
    }

    if (target_dac_clk != cur_dac_clk) {
#if TCFG_MIC_EFFECT_ENABLE
        tone_player_stop();
        mic_effect_player_pause(1);
#endif
        if (fm_player_runing()) {
            fm_player_close();
            audio_dac_clk_switch(target_dac_clk);
            fm_player_open();
        } else {
            audio_dac_clk_switch(target_dac_clk);
        }
        /* printf("freq = %d, cur_dac_clk = %d, target_dac_clk = %d, dac_clk_get = %d\n", freq, cur_dac_clk, target_dac_clk, audio_dac_clk_get()); */
#if TCFG_MIC_EFFECT_ENABLE
        if (!get_fm_scan_status()) {
            mic_effect_player_pause(0);
        }
#endif
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
        app_broadcast_reset_transmitter();
#endif
    }
}


bool fm_inside_set_fre(void *priv, u16 fre)
{
    u8 ret;
    u32 freq ;
    /* printf("[%d]", fre); */
    freq = fre * 10;
    fm_inside_dac_clk_set(freq);
    ret =  fm_inside_freq_set(freq);
    return ret;
}

bool fm_inside_read_id(void *priv)
{
    /* if (fm_get_device_en("fm_inside")) { */
    /* return (bool)fm_inside_id_read(); */
    /* } */
    /* return FALSE; */
    return (bool)fm_inside_id_read();
}

u8 fm_inside_powerdown(void *priv)
{
    sys_timer_del(fm_inside_agc_timer);
    fm_inside_off();
#if TCFG_MIC_EFFECT_ENABLE
    tone_player_stop();
    mic_effect_player_pause(1);
#endif

    audio_dac_clk_switch(fm_inside_dac_clk);
    /* dac_channel_off(FM_INSI_CHANNEL, FADE_ON); */

#if TCFG_MIC_EFFECT_ENABLE
    mic_effect_player_pause(0);
#endif

    return 0;
}

u8 fm_inside_mute(void *priv, u8 flag)
{
    fm_inside_mute_set(flag);
    return 0;
}


REGISTER_FM(fm_inside) = {
    .logo    = "fm_inside",
    .init    = fm_inside_init,
    .close   = fm_inside_powerdown,
    .set_fre = fm_inside_set_fre,
    .mute    = fm_inside_mute,
    .read_id = fm_inside_read_id,
};


#endif // FM_INSIDE
#endif

