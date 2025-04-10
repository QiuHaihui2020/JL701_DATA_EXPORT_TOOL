#ifndef __ADV_ANS_VOICE_H__
#define __ADV_ANS_VOICE_H__

#define ANC_VOICE_TYPE_MAX		3

int anc_voice_setting_init(void);
int anc_voice_setting_sync(void);

// void deal_anc_voice(u8 *anc_setting, u8 write_vm, u8 tws_sync);
void set_anc_voice_info(u8 *anc_info);
int get_anc_voice_info(u8 *anc_info);

u8 anc_voice_info_get(u8 *data, u16 len);
u8 anc_voice_info_fetch_all_get(u8 *data, u16 len);
int anc_voice_info_set(u8 *data, u16 len);
int update_anc_voice_key_opt(void);
void rcsp_adv_voice_mode_update(u8 mode);

// mode - 模式，0 - 关闭模式，1 - 降噪模式，2 - 通透模式
// // type - 类型，0 - 左耳，1 - 右耳
// // buf  - 获取对应值的buffer
// // 返回值：返回数据，0是失败
u16 rcsp_adv_anc_voice_value_get(u8 mode);

#endif
