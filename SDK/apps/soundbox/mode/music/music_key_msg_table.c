#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".music_key_msg_table.data.bss")
#pragma data_seg(".music_key_msg_table.data")
#pragma const_seg(".music_key_msg_table.text.const")
#pragma code_seg(".music_key_msg_table.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"


#if TCFG_ADKEY_ENABLE
//短按                  //长按                  //hold                       //长按抬起                  //双击                       //三击
#if (CONFIG_UI_STYLE != STYLE_JL_SOUNDBOX)
const int key_music_ad_num0_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_CHANGE_MODE,       APP_MSG_KEY_POWER_OFF,     APP_MSG_KEY_POWER_OFF_HOLD, APP_MSG_KEY_POWER_OFF_RELEASE,    APP_MSG_LE_BROADCAST_SW,   APP_MSG_NULL,
};
const int key_music_ad_num1_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_PP,       APP_MSG_MUSIC_CHANGE_DEV,   APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_VOCAL_REMOVE,   APP_MSG_NULL,
};
const int key_music_ad_num2_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_NEXT,     APP_MSG_VOL_UP,         APP_MSG_VOL_UP,         APP_MSG_NULL,           APP_MSG_MIC_EFFECT_ON_OFF,     APP_MSG_NULL,
};
const int key_music_ad_num3_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_PREV,     APP_MSG_VOL_DOWN,       APP_MSG_VOL_DOWN,       APP_MSG_NULL,           APP_MSG_MUSIC_PLAYE_PREV_FOLDER,         APP_MSG_NULL,
};
const int key_music_ad_num4_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_CHANGE_REPEAT,    APP_MSG_NULL,       APP_MSG_MUSIC_FF,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
#if TCFG_MIX_RECORD_ENABLE
const int key_music_ad_num5_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_PLAYE_PREV_FOLDER,     APP_MSG_MUSIC_DELETE_CUR_FILE,       APP_MSG_NULL,   APP_MSG_NULL,     APP_MSG_MUSIC_PLAY_REC_FOLDER_SWITCH,       APP_MSG_NULL,
};
#else
const int key_music_ad_num5_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_PLAYER_AB_REPEAT_SWITCH,     APP_MSG_NULL,       APP_MSG_MUSIC_FR,   APP_MSG_NULL,  APP_MSG_SWITCH_SOUND_EFFECT, APP_MSG_NULL,
};
#endif
const int key_music_ad_num6_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_SWITCH_MIC_EFFECT,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL, APP_MSG_MIC_EFFECT_ON_OFF, APP_MSG_NULL,
};
const int key_music_ad_num7_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_VOCAL_REMOVE,   APP_MSG_NULL,       APP_MSG_NULL,       APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
const int key_music_ad_num8_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MIC_VOL_UP,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
const int key_music_ad_num9_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,
};
#else  /*LCD按键*/
const int key_music_ad_num0_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_CHANGE_MODE,        APP_MSG_KEY_POWER_OFF,      APP_MSG_KEY_POWER_OFF_HOLD,    APP_MSG_KEY_POWER_OFF_RELEASE,           	 APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ad_num1_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_LCD_OK,             APP_MSG_LCD_MENU,           APP_MSG_NULL,               APP_MSG_NULL,                APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ad_num2_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_LCD_DOWN,           APP_MSG_LCD_VOL_DEC,        APP_MSG_LCD_VOL_DEC,        APP_MSG_NULL,                APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ad_num3_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_LCD_UP,             APP_MSG_LCD_VOL_INC,        APP_MSG_LCD_VOL_INC,        APP_MSG_NULL,                APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ad_num4_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,               APP_MSG_LCD_MODE,           APP_MSG_NULL,               APP_MSG_NULL,                APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ad_num5_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
const int key_music_ad_num6_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
const int key_music_ad_num7_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
const int key_music_ad_num8_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
const int key_music_ad_num9_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
#endif
#endif

#if TCFG_IRKEY_ENABLE
//短按                  //长按                  //hold                       //长按抬起                  //双击                       //三击
const int key_music_ir_num0_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_KEY_POWER_OFF_INSTANTLY,    	APP_MSG_NULL,    	APP_MSG_NULL, APP_MSG_NULL,           	 APP_MSG_NULL,   APP_MSG_NULL,
};
const int key_music_ir_num1_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_CHANGE_MODE,       APP_MSG_NULL,    APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
};
const int key_music_ir_num2_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_SYS_MUTE,     APP_MSG_NULL,         APP_MSG_NULL,         APP_MSG_NULL,            APP_MSG_NULL,      APP_MSG_NULL,
};
const int key_music_ir_num3_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_PP,     APP_MSG_NULL,       APP_MSG_NULL,       APP_MSG_NULL,            APP_MSG_NULL,           APP_MSG_NULL,
};
const int key_music_ir_num4_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_PREV,     APP_MSG_NULL,       APP_MSG_MUSIC_FR,       APP_MSG_NULL,            APP_MSG_NULL,           APP_MSG_NULL,
};
const int key_music_ir_num5_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_NEXT,  APP_MSG_NULL,       APP_MSG_MUSIC_FF,       APP_MSG_NULL,            APP_MSG_NULL,           APP_MSG_NULL,
};
const int key_music_ir_num6_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_CHANGE_EQ,    APP_MSG_NULL,       APP_MSG_NULL,       APP_MSG_NULL,            APP_MSG_NULL,           APP_MSG_NULL,
};
const int key_music_ir_num7_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_VOL_DOWN,  APP_MSG_VOL_DOWN,       APP_MSG_VOL_DOWN,       APP_MSG_NULL,           APP_MSG_NULL,           APP_MSG_NULL,
};
const int key_music_ir_num8_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_VOL_UP,     APP_MSG_VOL_UP,       APP_MSG_VOL_UP,       APP_MSG_NULL,            APP_MSG_NULL,           APP_MSG_NULL,
};
const int key_music_ir_num9_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_IR_NUM0,    APP_MSG_NULL,      APP_MSG_NULL,      APP_MSG_NULL,           APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ir_num10_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_CHANGE_REPEAT,    APP_MSG_NULL,      APP_MSG_NULL,      APP_MSG_NULL,           APP_MSG_MUSIC_PLAYE_NEXT_FOLDER,          APP_MSG_NULL,
};
const int key_music_ir_num11_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_CHANGE_DEV,    APP_MSG_NULL,      APP_MSG_NULL,      APP_MSG_NULL,           APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ir_num12_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_IR_NUM1,    APP_MSG_NULL,      APP_MSG_NULL,      APP_MSG_NULL,           APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ir_num13_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_IR_NUM2,    APP_MSG_NULL,      APP_MSG_NULL,      APP_MSG_NULL,           APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ir_num14_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_IR_NUM3,    APP_MSG_NULL,      APP_MSG_NULL,      APP_MSG_NULL,           APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ir_num15_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_IR_NUM4,    APP_MSG_NULL,      APP_MSG_NULL,      APP_MSG_NULL,           APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ir_num16_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_IR_NUM5,    APP_MSG_NULL,      APP_MSG_NULL,      APP_MSG_NULL,           APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ir_num17_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_IR_NUM6,    APP_MSG_NULL,      APP_MSG_NULL,      APP_MSG_NULL,           APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ir_num18_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_IR_NUM7,    APP_MSG_NULL,      APP_MSG_NULL,      APP_MSG_NULL,           APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ir_num19_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_IR_NUM8,    APP_MSG_NULL,      APP_MSG_NULL,      APP_MSG_NULL,           APP_MSG_NULL,          APP_MSG_NULL,
};
const int key_music_ir_num20_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_IR_NUM9,    APP_MSG_NULL,      APP_MSG_NULL,      APP_MSG_NULL,           APP_MSG_NULL,          APP_MSG_NULL,
};

#endif

#if TCFG_IOKEY_ENABLE
//短按                  //长按                  //hold                       //长按抬起                  //双击                       //三击
const int key_music_io_num0_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_CHANGE_MODE,       APP_MSG_KEY_POWER_OFF,     APP_MSG_KEY_POWER_OFF_HOLD,    APP_MSG_KEY_POWER_OFF_RELEASE,    APP_MSG_NULL,   APP_MSG_NULL,
};
const int key_music_io_num1_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_PP,       APP_MSG_MUSIC_CHANGE_DEV,   APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,   APP_MSG_NULL,
};
const int key_music_io_num2_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_NEXT,     APP_MSG_VOL_UP,         APP_MSG_VOL_UP,         APP_MSG_NULL,           APP_MSG_MUSIC_PLAYE_NEXT_FOLDER,     APP_MSG_NULL,
};
const int key_music_io_num3_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_MUSIC_PREV,     APP_MSG_VOL_DOWN,       APP_MSG_VOL_DOWN,       APP_MSG_NULL,           APP_MSG_MUSIC_PLAYE_PREV_FOLDER,         APP_MSG_NULL,
};
const int key_music_io_num4_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
const int key_music_io_num5_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
const int key_music_io_num6_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
const int key_music_io_num7_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
const int key_music_io_num8_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
const int key_music_io_num9_msg_table[KEY_ACTION_MAX] = {
    APP_MSG_NULL,     APP_MSG_NULL,       APP_MSG_NULL,   APP_MSG_NULL,        APP_MSG_NULL,       APP_MSG_NULL,
};
#endif

const struct key_remap_table music_mode_key_table[] = {
#if TCFG_ADKEY_ENABLE
    { .key_value = KEY_AD_NUM0, .remap_table = key_music_ad_num0_msg_table },
    { .key_value = KEY_AD_NUM1, .remap_table = key_music_ad_num1_msg_table },
    { .key_value = KEY_AD_NUM2, .remap_table = key_music_ad_num2_msg_table },
    { .key_value = KEY_AD_NUM3, .remap_table = key_music_ad_num3_msg_table },
    { .key_value = KEY_AD_NUM4, .remap_table = key_music_ad_num4_msg_table },
    { .key_value = KEY_AD_NUM5, .remap_table = key_music_ad_num5_msg_table },
    { .key_value = KEY_AD_NUM6, .remap_table = key_music_ad_num6_msg_table },
    { .key_value = KEY_AD_NUM7, .remap_table = key_music_ad_num7_msg_table },
    { .key_value = KEY_AD_NUM8, .remap_table = key_music_ad_num8_msg_table },
    { .key_value = KEY_AD_NUM9, .remap_table = key_music_ad_num9_msg_table },
#endif
#if TCFG_IRKEY_ENABLE
    { .key_value = KEY_IR_NUM0, .remap_table = key_music_ir_num0_msg_table },
    { .key_value = KEY_IR_NUM1, .remap_table = key_music_ir_num1_msg_table },
    { .key_value = KEY_IR_NUM2, .remap_table = key_music_ir_num2_msg_table },
    { .key_value = KEY_IR_NUM3, .remap_table = key_music_ir_num3_msg_table },
    { .key_value = KEY_IR_NUM4, .remap_table = key_music_ir_num4_msg_table },
    { .key_value = KEY_IR_NUM5, .remap_table = key_music_ir_num5_msg_table },
    { .key_value = KEY_IR_NUM6, .remap_table = key_music_ir_num6_msg_table },
    { .key_value = KEY_IR_NUM7, .remap_table = key_music_ir_num7_msg_table },
    { .key_value = KEY_IR_NUM8, .remap_table = key_music_ir_num8_msg_table },
    { .key_value = KEY_IR_NUM9, .remap_table = key_music_ir_num9_msg_table },
    { .key_value = KEY_IR_NUM10, .remap_table = key_music_ir_num10_msg_table },
    { .key_value = KEY_IR_NUM11, .remap_table = key_music_ir_num11_msg_table },
    { .key_value = KEY_IR_NUM12, .remap_table = key_music_ir_num12_msg_table },
    { .key_value = KEY_IR_NUM13, .remap_table = key_music_ir_num13_msg_table },
    { .key_value = KEY_IR_NUM14, .remap_table = key_music_ir_num14_msg_table },
    { .key_value = KEY_IR_NUM15, .remap_table = key_music_ir_num15_msg_table },
    { .key_value = KEY_IR_NUM16, .remap_table = key_music_ir_num16_msg_table },
    { .key_value = KEY_IR_NUM17, .remap_table = key_music_ir_num17_msg_table },
    { .key_value = KEY_IR_NUM18, .remap_table = key_music_ir_num18_msg_table },
    { .key_value = KEY_IR_NUM19, .remap_table = key_music_ir_num19_msg_table },
    { .key_value = KEY_IR_NUM20, .remap_table = key_music_ir_num20_msg_table },
#endif
#if TCFG_IOKEY_ENABLE
    { .key_value = KEY_IO_NUM0, .remap_table = key_music_io_num0_msg_table },
    { .key_value = KEY_IO_NUM1, .remap_table = key_music_io_num1_msg_table },
    { .key_value = KEY_IO_NUM2, .remap_table = key_music_io_num2_msg_table },
    { .key_value = KEY_IO_NUM3, .remap_table = key_music_io_num3_msg_table },
    { .key_value = KEY_IO_NUM4, .remap_table = key_music_io_num4_msg_table },
    { .key_value = KEY_IO_NUM5, .remap_table = key_music_io_num5_msg_table },
    { .key_value = KEY_IO_NUM6, .remap_table = key_music_io_num6_msg_table },
    { .key_value = KEY_IO_NUM7, .remap_table = key_music_io_num7_msg_table },
    { .key_value = KEY_IO_NUM8, .remap_table = key_music_io_num8_msg_table },
    { .key_value = KEY_IO_NUM9, .remap_table = key_music_io_num9_msg_table },
#endif
    { .key_value = 0xff }
};

