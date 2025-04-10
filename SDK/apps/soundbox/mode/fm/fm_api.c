#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".fm_api.data.bss")
#pragma data_seg(".fm_api.data")
#pragma const_seg(".fm_api.text.const")
#pragma code_seg(".fm_api.text")
#endif
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "app_task.h"
#include "tone_player.h"
#include "app_main.h"
#include "ui_manage.h"
#include "vm.h"
#include "key_event_deal.h"
#include "user_cfg.h"
#include "fm.h"
#include "fm_manage.h"
#include "fm_player.h"
#include "fm_rw.h"
#include "ui/ui_api.h"
#include "clock.h"
#include "includes.h"
#include "fm_api.h"
#include "bt_tws.h"
#include "ui/ui_style.h"
#include "audio_config.h"
/* #include "audio_dec/audio_dec_fm.h" */
#include "fm/fm_inside/fm_inside.h"
#include "app_tone.h"
#include "mic_effect.h"
#include "wireless_trans.h"
#include "le_audio_recorder.h"
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#include "le_connected.h"
#include "app_le_connected.h"
#include "le_audio_stream.h"
#include "le_audio_player.h"
#include "rcsp_device_status.h"
#include "rcsp_fm_func.h"
#include "rcsp_config.h"
#include "bt_key_func.h"
#include "btstack_rcsp_user.h"

#define LOG_TAG             "[APP_FM]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_APP_FM_EN

#define TCFG_FM_SC_REVERB_ENABLE 1//搜索过程关闭混响

/*************************************************************
  此文件函数主要是fm模式 实现函数

  任务内初始化：
  fm_api_init 主要申请空间、读取vm信息操作

  释放资源：
  fm_api_release

  注意：在逻辑操作上，sdk大部分是使用了虚拟频点,即(1,、2、3、4)代替了8750、8760、8770等真实的频率
  转换规则：真实 = REAL_FREQ(虚拟)
  虚拟 = VIRTUAL_FREQ(真实)
 **************************************************************/

struct fm_opr {
    void *dev;
    u8 volume: 7;
    u8 fm_dev_mute : 1;
    u8 scan_flag;//搜索标志位,客户增加了自己了搜索标志位也要对应增加
    u16 fm_freq_cur;		// 这是虚拟频率,从1计算  real_freq = fm_freq_cur + 874
    u16 fm_freq_channel_cur;//当前的台号,从1计算
    u16 fm_total_channel;//总共台数
    s16 scan_fre;//搜索过程的虚拟频率,因为--会少于0，使用带符号
    u16 fm_freq_temp;		// 这是虚拟频率,从1计算  real_freq = fm_freq_cur + 874，用来做记录
};

#define  SCANE_ALL         (0x01)
#define  SEMI_SCANE_DOWN   (0x02)//半自动搜索标志位
#define  SEMI_SCANE_UP     (0x03)


volatile u8 fm_mute_flag = 0;
static struct fm_opr *fm_hdl = NULL;
#define __this 	(fm_hdl)
/* extern void fm_dec_pause_out(u8 pause); */

/*----------------------------------------------------------------------------*/
/**@brief    fm 的 mute 操作
  @param    1: mute  0:解mute
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
static void fm_app_mute(u8 mute)
{
    if (!__this) {
        return;
    }
    if (mute) {
        if (!__this->fm_dev_mute) {
            fm_manage_mute(1);
            __this->fm_dev_mute = 1;
            fm_mute_flag = 1;
        }
    } else {
        if (__this->fm_dev_mute) {
            fm_manage_mute(0);
            __this->fm_dev_mute = 0;
            fm_mute_flag = 0;
        }
    }
    /* fm_dec_pause_out(mute); */
}

/*----------------------------------------------------------------------------*/
/**@brief    fm 获取vm 保存信息
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/

static void fm_read_info_init(void)
{
    FM_INFO fm_info = {0};

    fm_vm_check();//校验mask,不符合mask进行清0
    fm_read_info(&fm_info);//获取vm信息

    __this->fm_freq_cur = fm_info.curFreq;//上一次的保存的频率(虚拟频率)
    printf("__this->fm_freq_cur = 0x%x\n", __this->fm_freq_cur);
    __this->fm_freq_channel_cur	= fm_info.curChanel;//上一次保存的频道
    printf("__this->fm_freq_channel_cur = 0x%x\n", __this->fm_freq_channel_cur);
    __this->fm_total_channel = fm_info.total_chanel;//总台数
    printf("__this->fm_total_channel = 0x%x\n", __this->fm_total_channel);

    if (__this->fm_freq_cur == 0 && __this->fm_freq_channel_cur && __this->fm_total_channel) {
        __this->fm_freq_cur = get_fre_via_channel(__this->fm_freq_channel_cur);//上次记录操作是频道，则获取频道对应的频率
        fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));
    } else {
        fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));//上次记录操作的是频率
    }

}

/*----------------------------------------------------------------------------*/
/**@brief    fm ui 更新
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
static  void __fm_ui_reflash_main()//刷新主页
{
    UI_REFLASH_WINDOW(true);
    UI_MSG_POST("fm_fre", NULL);
}

static  void __fm_ui_show_station()//显示台号
{
    UI_SHOW_MENU(MENU_FM_STATION, 1000, __this->fm_total_channel, NULL);
    /* ui_menu_reflash(true); */
    UI_MSG_POST("fm_fre", NULL);
}

static  void __fm_ui_cur_station()//显示当前台号
{
    UI_SHOW_MENU(MENU_FM_STATION, 1000, __this->fm_freq_channel_cur, NULL);
    /* ui_menu_reflash(true); */
    UI_MSG_POST("fm_fre", NULL);
}

/*----------------------------------------------------------------------------*/
/**@brief    fm 搜台完毕开混响
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
static void __fm_reverb_resume()
{
#if ((TCFG_MIC_EFFECT_ENABLE) &&(TCFG_FM_SC_REVERB_ENABLE))
    mic_effect_player_pause(0);
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    fm 搜台前关混响
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
static void __fm_reverb_pause()
{
#if ((TCFG_MIC_EFFECT_ENABLE) &&(TCFG_FM_SC_REVERB_ENABLE))
    mic_effect_player_pause(1);
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    fm 下一个台
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
static void __set_fm_station()
{
    fm_app_mute(1);
    __this->fm_freq_cur = get_fre_via_channel(__this->fm_freq_channel_cur);
    fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));
    fm_last_ch_save(__this->fm_freq_channel_cur);
    fm_app_mute(0);
}

static void __set_fm_frq()
{
    fm_app_mute(1);
    fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));
    fm_last_freq_save(REAL_FREQ(__this->fm_freq_cur));
    fm_app_mute(0);
}

static void __fm_scan_all(void *priv)
{
    if ((!__this) || (__this->scan_flag != SCANE_ALL)) {
        return;
    }
    wdt_clear();//搜台时间较长，每次切频点要清看门狗
    if (__this->scan_fre > VIRTUAL_FREQ(REAL_FREQ_MAX)) {
        __this->scan_fre = VIRTUAL_FREQ(REAL_FREQ_MIN);
        //搜索完毕跑这里
        fm_app_mute(1);

        /////////////////////////////////////
        //这里设置搜索完毕的默认台号
        if (__this->fm_freq_channel_cur) { //搜索到台就自动跳转1第一台
            __this->fm_freq_cur = get_fre_via_channel(1);
            __this->fm_freq_channel_cur = 1;
        } else {
            __this->fm_freq_cur = 1;    //搜索不了就默认第一个
        }
        ///////////////////////////
        fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));
        __this->scan_flag = 0;
        fm_app_mute(0);
        app_send_message(APP_MSG_FM_REFLASH, 0);
        __fm_reverb_resume();
        fm_last_ch_save(1);
#if RCSP_MODE
        rcsp_device_status_update(FM_FUNCTION_MASK, BIT(FM_INFO_ATTR_STATUS) | BIT(FM_INFO_ATTR_FRE));
#endif
        return;
    }

    fm_app_mute(1);

    if (fm_manage_set_fre(REAL_FREQ(__this->scan_fre))) {
        __this->fm_freq_cur  = __this->scan_fre;
        __this->fm_total_channel++;
        __this->fm_freq_channel_cur = __this->fm_total_channel;//++;
        save_fm_point(REAL_FREQ(__this->scan_fre));
        sys_timeout_add(NULL, __fm_scan_all, 1500); //播放一秒
        app_send_message(APP_MSG_FM_STATION, __this->fm_total_channel);
        fm_app_mute(0);
    } else {
        __this->fm_freq_cur  = __this->scan_fre;
        sys_timeout_add(NULL, __fm_scan_all, 20);

        app_send_message(APP_MSG_FM_REFLASH, 0);
    }
    __this->scan_fre++;

}

static void __fm_semi_scan(void *priv)//半自动收台
{
    if ((!__this) || (__this->scan_flag != SEMI_SCANE_UP && __this->scan_flag != SEMI_SCANE_DOWN)) {
        return;
    }
    wdt_clear();//搜台时间较长，每次切频点要清看门狗
    if (__this->scan_flag == SEMI_SCANE_DOWN) {
        __this->scan_fre++;
    } else {
        __this->scan_fre--;
    }

    if (__this->scan_fre > VIRTUAL_FREQ(REAL_FREQ_MAX)) {
        __this->scan_fre = VIRTUAL_FREQ(REAL_FREQ_MIN);
#if TCFG_FM_INSIDE_ENABLE
        save_scan_freq_org(REAL_FREQ(__this->scan_fre) * 10);
#endif
    }

    if (__this->scan_fre <  VIRTUAL_FREQ(REAL_FREQ_MIN)) {
        __this->scan_fre = VIRTUAL_FREQ(REAL_FREQ_MAX);
#if TCFG_FM_INSIDE_ENABLE
        save_scan_freq_org(REAL_FREQ(__this->scan_fre) * 10);
#endif
    }

    if (__this->scan_fre == __this->fm_freq_temp) {
        fm_app_mute(1);
        __this->fm_freq_cur = __this->fm_freq_temp;
        fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));
        fm_app_mute(0);
        __this->scan_flag = 0;
        app_send_message(APP_MSG_FM_REFLASH, 0);
        __fm_reverb_resume();
        return;
    }

    fm_app_mute(1);

    if (fm_manage_set_fre(REAL_FREQ(__this->scan_fre))) {
        __this->fm_freq_cur  = __this->scan_fre;
        save_fm_point(REAL_FREQ(__this->scan_fre));//保存当前频点
        __this->fm_freq_channel_cur = get_channel_via_fre(REAL_FREQ(__this->scan_fre));//获取当前台号
        __this->fm_total_channel = get_total_mem_channel();//获取新的总台数
        fm_app_mute(0);
        app_send_message(APP_MSG_FM_STATION, __this->fm_total_channel);
        __fm_reverb_resume();
        __this->scan_flag = 0;
        return;
    } else {
        app_send_message(APP_MSG_FM_REFLASH, 0);
        __this->fm_freq_cur  = __this->scan_fre; //影响半自动搜台结束条件,先进行注释,app问题要重新解决
        sys_timeout_add(NULL, __fm_semi_scan, 20);
    }

}

void fm_delete_freq()
{
    if (!__this || __this->scan_flag) {
        return;
    }
    delete_fm_point(__this->fm_freq_cur);
}

void fm_scan_up()//半自动收台
{
    log_info("KEY_FM_SCAN_UP\n");
    if (!__this || __this->scan_flag) {
        return;
    }
    __this->scan_fre =  __this->fm_freq_cur;
    __this->scan_flag = SEMI_SCANE_UP;
    __this->fm_freq_temp = __this->fm_freq_cur;
    __fm_reverb_pause();
    fm_app_mute(1);

#if TCFG_FM_INSIDE_ENABLE
    fm_inside_trim(REAL_FREQ(__this->scan_fre) * 10);
#endif
    sys_timeout_add(NULL, __fm_semi_scan, 20);
}

void fm_scan_down()//半自动收台
{
    log_info("KEY_FM_SCAN_DOWN\n");
    if (!__this || __this->scan_flag) {
        return;
    }
    __this->scan_fre = __this->fm_freq_cur;
    __this->scan_flag = SEMI_SCANE_DOWN;
    __this->fm_freq_temp = __this->fm_freq_cur;

    __fm_reverb_pause();
    fm_app_mute(1);

#if TCFG_FM_INSIDE_ENABLE
    fm_inside_trim(REAL_FREQ(__this->scan_fre) * 10);
#endif

    sys_timeout_add(NULL, __fm_semi_scan, 20);

}

void fm_scan_stop(void)
{
    if (__this && __this->scan_flag) {
        __this->scan_flag = 0;
        os_time_dly(1);
        __set_fm_station();
        __fm_reverb_resume();
    }
}

void fm_scan_all()
{
    log_info("KEY_FM_SCAN_ALL\n");
    if (!__this) {
        return;
    }

    if (__this->scan_flag) {
        fm_scan_stop();
        return;
    }

    clear_all_fm_point();

    __this->fm_freq_cur  = 1;;
    __this->fm_total_channel = 0;
    __this->fm_freq_channel_cur = 0;

    __this->scan_fre = VIRTUAL_FREQ(REAL_FREQ_MIN);
    __this->scan_flag = SCANE_ALL;

    __fm_reverb_pause();
    fm_app_mute(1);

#if TCFG_FM_INSIDE_ENABLE
    fm_inside_trim(REAL_FREQ(__this->scan_fre) * 10);
#endif
    sys_timeout_add(NULL, __fm_scan_all, 20);
}

void fm_clear_all_station()
{
    log_info("fm_clear_all_station\n");
    if (!__this) {
        return;
    }

    if (__this->scan_flag) {
        fm_scan_stop();
        return;
    }

    clear_all_fm_point();

    __this->fm_freq_cur  = 1;;
    __this->fm_total_channel = 0;
    __this->fm_freq_channel_cur = 0;
    fm_manage_set_fre(REAL_FREQ(__this->fm_freq_cur));
    app_send_message(APP_MSG_FM_REFLASH, 0);
}

static int le_audio_fm_volume_pp(void)
{
    int ret = 0;

    if (__this->fm_dev_mute) {
        ret = le_audio_scene_deal(LE_AUDIO_MUSIC_STOP);
#if (LEA_BIG_FIX_ROLE==1)
        if (__this->fm_local_audio_resume_onoff) {
            __this->fm_local_audio_resume_onoff = 0;
        }
#endif
    } else {
        ret = le_audio_scene_deal(LE_AUDIO_MUSIC_START);
#if (LEA_BIG_FIX_ROLE==1)
        if (__this->fm_local_audio_resume_onoff == 0) {
            __this->fm_local_audio_resume_onoff = 1;
        }
#endif
    }

    return ret;
}

void fm_volume_pp(void)
{
    log_info("KEY_MUSIC_PP\n");
    if (!__this || __this->scan_flag) {
        return ;
    }
    //广播角色为接收端，不让控制fm的播放、暂停
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
#if (LEA_BIG_FIX_ROLE == 2) && !TCFG_KBOX_1T3_MODE_EN
    //固定为接收端
    u8 fm_volume_mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
    if (get_broadcast_role()) {	//如果连接上了
        fm_volume_mute_mark ^= 1;
        audio_app_mute_en(fm_volume_mute_mark);
    } else {
        //没有连接下，如果上次是mute的状态，那么不mute
        if (fm_volume_mute_mark == 1) {
            fm_volume_mute_mark ^= 1;
            __this->fm_dev_mute = 1;
            audio_app_mute_en(fm_volume_mute_mark);
        }
        if (__this->fm_dev_mute == 0) {
            fm_app_mute(1);
            fm_player_close();
        } else {
            fm_app_mute(0);
            fm_player_open();
        }
    }
#elif (LEA_BIG_FIX_ROLE == 1)
    //固定为发送端
    if (__this->fm_dev_mute == 0) {
        fm_app_mute(1);
        fm_player_close();
    } else {
        fm_app_mute(0);
        if (get_broadcast_role() != 1) {
            //如果 fm 广播没有打开的情况下, 才去进行player的开关
            //加这个判断是为了解决：fm广播作发送端时，按pp键，重复多开fm player导致广播异常
            fm_player_open();
        }
    }
#else
    //加上这条判断为了解决的问题是：固定为接收端，FM播放中开广播进入接收状态，接收状态下点击pp键，然后关闭广播，点击pp键，无法播放
    if (get_broadcast_role() != 2) {
        if (__this->fm_dev_mute == 0) {
            fm_app_mute(1);
            fm_player_close();
        } else {
            fm_app_mute(0);
            if (get_broadcast_role() != 1) {
                //如果 fm 广播没有打开的情况下, 才去进行player的开关
                //加这个判断是为了解决：fm广播作发送端时，按pp键，重复多开fm player导致广播异常
                fm_player_open();
            }
        }
    } else {
        // 作为接收端时, 按下播放键时需要恢复播放, 转为发送端
        if (__this->fm_dev_mute == 0) {
            fm_app_mute(1);
            fm_player_close();
        } else {
            fm_app_mute(0);
            fm_player_open();
        }
    }
#endif
#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
#if (LEA_BIG_FIX_ROLE == 2)
    //固定为接收端
    u8 fm_volume_mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
    if (get_auracast_role()) {	//如果连接上了
        fm_volume_mute_mark ^= 1;
        audio_app_mute_en(fm_volume_mute_mark);
    } else {
        //没有连接下，如果上次是mute的状态，那么不mute
        if (fm_volume_mute_mark == 1) {
            fm_volume_mute_mark ^= 1;
            __this->fm_dev_mute = 1;
            audio_app_mute_en(fm_volume_mute_mark);
        }
        if (__this->fm_dev_mute == 0) {
            fm_app_mute(1);
            fm_player_close();
        } else {
            fm_app_mute(0);
            fm_player_open();
        }
    }
#elif (LEA_BIG_FIX_ROLE == 1)
    //固定为发送端
    if (__this->fm_dev_mute == 0) {
        fm_app_mute(1);
        fm_player_close();
    } else {
        fm_app_mute(0);
        if (get_auracast_role() != 1) {
            //如果 fm 广播没有打开的情况下, 才去进行player的开关
            //加这个判断是为了解决：fm广播作发送端时，按pp键，重复多开fm player导致广播异常
            fm_player_open();
        }
    }
#else
    //加上这条判断为了解决的问题是：固定为接收端，FM播放中开广播进入接收状态，接收状态下点击pp键，然后关闭广播，点击pp键，无法播放
    if (get_auracast_role() != 2) {
        if (__this->fm_dev_mute == 0) {
            fm_app_mute(1);
            fm_player_close();
        } else {
            fm_app_mute(0);
            if (get_auracast_role() != 1) {
                //如果 fm 广播没有打开的情况下, 才去进行player的开关
                //加这个判断是为了解决：fm广播作发送端时，按pp键，重复多开fm player导致广播异常
                fm_player_open();
            }
        }
    } else {
        // 作为接收端时, 按下播放键时需要恢复播放, 转为发送端
        if (__this->fm_dev_mute == 0) {
            fm_app_mute(1);
            fm_player_close();
        } else {
            fm_app_mute(0);
            fm_player_open();
        }
    }
#endif
#else
    if (__this->fm_dev_mute == 0) {
        fm_app_mute(1);
    } else {
        fm_app_mute(0);
    }
#endif

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    if (get_broadcast_role()) {
        le_audio_fm_volume_pp();
    } else {
        if (__this->fm_dev_mute) {
            update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_STOP);
        } else {
            update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_START);
        }
    }
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    if (get_auracast_role()) {
        le_audio_fm_volume_pp();
    } else {
        if (__this->fm_dev_mute) {
            update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_STOP);
        } else {
            update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_START);
        }
    }
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    if ((app_get_connected_role() == APP_CONNECTED_ROLE_TRANSMITTER) ||
        (app_get_connected_role() == APP_CONNECTED_ROLE_DUPLEX)) {
        le_audio_fm_volume_pp();
    } else {
        if (__this->fm_dev_mute) {
            update_app_connected_deal_scene(LE_AUDIO_MUSIC_STOP);
        } else {
            update_app_connected_deal_scene(LE_AUDIO_MUSIC_START);
        }
    }
#endif

}

void fm_prev_freq()
{
    log_info("KEY_FM_PREV_FREQ\n");
    if (!__this || __this->scan_flag) {
        return;
    }

    if (__this->fm_freq_cur <= VIRTUAL_FREQ(REAL_FREQ_MIN)) {
        __this->fm_freq_cur = VIRTUAL_FREQ(REAL_FREQ_MAX);
    } else {
        __this->fm_freq_cur -= 1;
    }
    __set_fm_frq();
    app_send_message(APP_MSG_FM_REFLASH, 0);
    log_info("KEY_FM_PREV_FREQ\n");
}

void fm_next_freq()
{
    log_info("KEY_FM_NEXT_FREQ\n");
    if (!__this || __this->scan_flag) {
        return;
    }
    if (__this->fm_freq_cur >= VIRTUAL_FREQ(REAL_FREQ_MAX)) {
        __this->fm_freq_cur = VIRTUAL_FREQ(REAL_FREQ_MIN);
    } else {
        __this->fm_freq_cur += 1;
    }
    __set_fm_frq();
    app_send_message(APP_MSG_FM_REFLASH, 0);

}

void fm_volume_up()
{
    u8 vol = 0;
    log_info("KEY_VOL_UP\n");
    if (!__this || __this->scan_flag) {
        return;
    }

#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN))
    if (bt_rcsp_device_conn_num() && JL_rcsp_get_auth_flag() && (app_get_current_mode()->name != APP_MODE_BT)) {
        bt_key_rcsp_vol_up();
    } else {
        app_audio_volume_up(1);
    }
#else
    app_audio_volume_up(1);
#endif
    log_info("fm vol+: %d", app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
    if (app_audio_get_volume(APP_AUDIO_CURRENT_STATE) == app_audio_get_max_volume()) {
        if (tone_player_runing() == 0) {
#if TCFG_MAX_VOL_PROMPT
            play_tone_file(get_tone_files()->max_vol);
#endif
        }
    }

    vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    app_send_message(APP_MSG_VOL_CHANGED, vol);

}

void fm_volume_down()
{
    u8 vol = 0;
    log_info("KEY_VOL_DOWN\n");
    if (!__this || __this->scan_flag) {
        return;
    }
#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN))
    if (bt_rcsp_device_conn_num() && JL_rcsp_get_auth_flag() && (app_get_current_mode()->name != APP_MODE_BT)) {
        bt_key_rcsp_vol_down();
    } else {
        app_audio_volume_down(1);
    }
#else
    app_audio_volume_down(1);
#endif
    log_info("fm vol-: %d", app_audio_get_volume(APP_AUDIO_STATE_MUSIC));

    vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    app_send_message(APP_MSG_VOL_CHANGED, vol);

}

void fm_prev_station()
{
    log_info("KEY_FM_PREV_STATION\n");

    if (!__this || __this->scan_flag || (!__this->fm_total_channel)) {
        return;
    }

    if (__this->fm_freq_channel_cur <= 1) {
        __this->fm_freq_channel_cur = __this->fm_total_channel;
    } else {
        __this->fm_freq_channel_cur -= 1;
    }
    __set_fm_station();
    app_send_message(APP_MSG_FM_STATION, __this->fm_freq_channel_cur);
}

void fm_next_station()
{
    log_info("KEY_FM_NEXT_STATION\n");
    if (!__this || __this->scan_flag || (!__this->fm_total_channel)) {
        return;
    }

    if (__this->fm_freq_channel_cur >= __this->fm_total_channel) {
        __this->fm_freq_channel_cur = 1;
    } else {
        __this->fm_freq_channel_cur += 1;
    }

    __set_fm_station();
    app_send_message(APP_MSG_FM_STATION, __this->fm_freq_channel_cur);
}

/*----------------------------------------------------------------------------*/
/**@brief    fm 入口初始化
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
void fm_api_init()
{
    fm_hdl = (struct fm_opr *)malloc(sizeof(struct fm_opr));
    memset(fm_hdl, 0x00, sizeof(struct fm_opr));
    if (fm_hdl == NULL) {
        puts("fm_state_machine fm_hdl malloc err !\n");
    }
    __this->fm_dev_mute = 0;
    fm_app_mute(1);
    fm_read_info_init();
    os_time_dly(1);
    fm_app_mute(0);

#if RCSP_MODE
    rcsp_device_status_update(FM_FUNCTION_MASK, BIT(FM_INFO_ATTR_STATUS) | BIT(FM_INFO_ATTR_FRE));
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    fm 任务资源释放
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
void fm_api_release()
{
    if (!__this) {
        return;
    }

    if (__this->scan_flag) {
        __this->scan_flag = 0;
        __fm_reverb_resume();
    }

    if (__this != NULL) {
        free(__this);
        __this = NULL;
    }
}

u8 fm_get_scan_flag(void)
{
    if (!__this) {
        return 0;
    }
    return __this->scan_flag;
}

u8 fm_get_fm_dev_mute(void)
{
    if (!__this) {
        return 0;
    }
    return __this->fm_dev_mute;
}

u8 fm_get_cur_channel(void)
{
    if (!__this) {
        return 0;
    }

    return (u8)__this->fm_freq_channel_cur;
}

u16 fm_get_cur_fre(void)
{
    if (!__this) {
        return 0;
    }
    if (__this->fm_freq_cur > 1080) {
        __this->fm_freq_cur /= 10;
    }
    return	(__this->fm_freq_cur % 874) + 874;
}

u8 fm_get_mode(void)
{
    u32 freq_min = REAL_FREQ_MIN / VIRTUAL_FREQ_STEP;
    // cppcheck-suppress knownConditionTrueFalse
    if (freq_min < 875) {
        return 1;
    } else {
        return 0;
    }
}

void fm_sel_station(u8 channel)
{
    if (!__this) {
        return;
    }

    if (channel > __this->fm_total_channel)	{
        printf("channel sel err!\n");
        return;
    }
    __this->fm_freq_channel_cur = channel;
    __set_fm_station();
}

u8 fm_set_fre(u16 fre)
{
    if (!__this) {
        return -1;
    }

    if ((fre < REAL_FREQ_MIN) || (fre > REAL_FREQ_MAX)) {
        return -1;
    }
    __this->fm_freq_cur = VIRTUAL_FREQ(fre);
    __set_fm_frq();
    return 0;
}

u8 get_fm_scan_status(void)
{
    return __this->scan_flag;
}

#if TCFG_FM_INSIDE_ENABLE
//FM发射模式下扫描强台，避开强台发射
//在最低优先级的线程运行、关闭解调、关闭FM解码
void txmode_fm_inside_freq_scan(void)
{
    u16 scan_cnt = 0;
    fm_vm_check();
    fm_inside_init(NULL);//初始化FM模块
    save_scan_freq_org(REAL_FREQ_MIN * 10); //搜台的起始频点
    for (scan_cnt = 1; scan_cnt <= MAX_CHANNEL; scan_cnt++) {
        wdt_clear();//搜台时间较长，每次切频点要清看门狗
        if (fm_inside_set_fre(NULL, REAL_FREQ(scan_cnt))) {  //真 判为有台
            save_fm_point(REAL_FREQ(scan_cnt));//保存频点到VM

            //REAL_FREQ(scan_cnt) * 10 ---->87.5M对应87500
            printf("get_freq = %d\n", REAL_FREQ(scan_cnt) * 10);
        }
    }
    fm_inside_powerdown(NULL);//关闭FM模块
}

//FM发射模式下获取扫描到的强台
//REAL_FREQ(get_fre_via_channel(freq_channel)) * 10 ---->87.5M对应87500
void txmode_fm_inside_freq_get(void)
{
    u16 freq_channel = 0;
    FM_INFO info;
    fm_vm_check();
    fm_read_info(&info);//读取扫完台的信息
    for (freq_channel = 1; freq_channel < info.total_chanel; freq_channel++) {
        printf("freq_channel_%d : %dKHz\n", freq_channel,
               REAL_FREQ(get_fre_via_channel(freq_channel)) * 10);
    }
}
#endif

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))

static int get_fm_play_status(void)
{
    if (get_le_audio_app_mode_exit_flag()) {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))) && (LEA_BIG_FIX_ROLE==1)
    if (__this->fm_local_audio_resume_onoff) {
        __this->fm_dev_mute = 0;
        return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
    } else {
        __this->fm_dev_mute = 1;
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }
#endif

    if (__this->fm_dev_mute == 0) {
        return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
    } else {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }
}

static int fm_local_audio_open(void)
{
    if (1) {//(get_fm_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //打开本地播放
        fm_player_open();
#if (defined TCFG_PITCH_SPEED_NODE_ENABLE && FM_PLAYBACK_PITCH_KEEP)
        audio_pitch_default_parm_set(app_var.pitch_mode);
        fm_file_pitch_mode_init(app_var.pitch_mode);
#endif
    }
    return 0;
}

static int fm_local_audio_close(void)
{
    if (fm_player_runing()) {
        //关闭本地播放
        fm_player_close();

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==1)
        if (get_broadcast_role()) {
            __this->fm_local_audio_resume_onoff = 1;
        }
#endif
#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN)) && (LEA_BIG_FIX_ROLE==1)
        if (get_auracast_role()) {
            __this->fm_local_audio_resume_onoff = 1;
        }
#endif
    } else {
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==1)
        if (get_broadcast_role()) {
            __this->fm_local_audio_resume_onoff = 0;
        }
#endif
#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN)) && (LEA_BIG_FIX_ROLE==1)
        if (get_auracast_role()) {
            __this->fm_local_audio_resume_onoff = 0;
        }
#endif
    }
    return 0;
}

static void *fm_tx_le_audio_open(void *args)
{
    int err;
    void *le_audio = NULL;

    if (1) {//(get_fm_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //打开广播音频播放
        struct le_audio_stream_params *params = (struct le_audio_stream_params *)args;
        le_audio = le_audio_stream_create(params->conn, &params->fmt);
        err = le_audio_fm_recorder_open((void *)&params->fmt, le_audio, params->latency);
        if (err != 0) {
            ASSERT(0, "recorder open fail");
        }
#if LEA_LOCAL_SYNC_PLAY_EN
        err = le_audio_player_open(le_audio, params);
        if (err != 0) {
            ASSERT(0, "player open fail");
        }
#endif
        __this->volume = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    }

    if (__this->fm_dev_mute) {
        fm_app_mute(0);
    }

    return le_audio;
}

static int fm_tx_le_audio_close(void *le_audio)
{
    if (!le_audio) {
        return -EPERM;
    }

    //关闭广播音频播放
#if LEA_LOCAL_SYNC_PLAY_EN
    le_audio_player_close(le_audio);
#endif
    le_audio_fm_recorder_close();
    le_audio_stream_free(le_audio);

    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, __this->volume, 1);

    return 0;
}

static int fm_rx_le_audio_open(void *rx_audio, void *args)
{
    int err;
    struct le_audio_player_hdl *rx_audio_hdl = (struct le_audio_player_hdl *)rx_audio;

    if (!rx_audio) {
        return -EPERM;
    }

    //打开广播音频播放
    struct le_audio_stream_params *params = (struct le_audio_stream_params *)args;
    rx_audio_hdl->le_audio = le_audio_stream_create(params->conn, &params->fmt);
    rx_audio_hdl->rx_stream = le_audio_stream_rx_open(rx_audio_hdl->le_audio, params->fmt.coding_type);
    err = le_audio_player_open(rx_audio_hdl->le_audio, params);
    if (err != 0) {
        ASSERT(0, "player open fail");
    }

    return 0;
}

static int fm_rx_le_audio_close(void *rx_audio)
{
    struct le_audio_player_hdl *rx_audio_hdl = (struct le_audio_player_hdl *)rx_audio;

    if (!rx_audio) {
        return -EPERM;
    }

    //关闭广播音频播放
    le_audio_player_close(rx_audio_hdl->le_audio);
    le_audio_stream_rx_close(rx_audio_hdl->rx_stream);
    le_audio_stream_free(rx_audio_hdl->le_audio);

    return 0;
}


const struct le_audio_mode_ops le_audio_fm_ops = {
    .local_audio_open = fm_local_audio_open,
    .local_audio_close = fm_local_audio_close,
    .tx_le_audio_open = fm_tx_le_audio_open,
    .tx_le_audio_close = fm_tx_le_audio_close,
    .rx_le_audio_open = fm_rx_le_audio_open,
    .rx_le_audio_close = fm_rx_le_audio_close,
    .play_status = get_fm_play_status,
};

#endif
#endif

