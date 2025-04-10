#ifndef _AUDIO_SPK_EQ_H_
#define _AUDIO_SPK_EQ_H_

#include "spinlock.h"
#include "math.h"
#include "effects/audio_eq.h"


struct spk_eq_seg_parm {
    u8 type;//0:seg 1:gain 2:获取系数表地址
    u8 left_right;//0:left  1:right
    struct eq_seg_info seg;
};

struct spk_eq_global_gain {
    u8 type;//0:seg 1:gain
    u8 left_right;//0:left  1:right
    float global_gain;
};

struct spk_eq_get_seg_tab {
    u8 type;//0:seg 1:gain 2:获取系数表地址 3:获取总增益
    u16 tab_size;
    struct eq_seg_info *seg;//包含左右声道
};

struct spk_eq_get_global_gain {
    u8 type;//0:seg 1:gain 2:获取系数表地址 3:获取总增益
    float global_gain[2]; //0:left 1:right
};


struct spk_eq_get_parm {
    char name[16];
    u8 type;// 2:获取系数表地址，4:获取vm数据
    struct spk_eq_get_seg_tab seg_tab;
    struct spk_eq_get_global_gain g_gain;
};

int spk_eq_read_from_vm(void *priv);

void spk_eq_set_send_data_handler(void (*handler)(u8 seq, u8 *packet, u8 size));
int spk_eq_app_online_parse(u8 *packet, u8 size, u8 *ext_data, u16 ext_size);
int spk_eq_spp_rx_packet(u8 *packet, u8 len);
int spk_eq_read_seg_l(u8 **buf);
int spk_eq_read_seg_r(u8 **buf);

#define UPDATE_SPK_EQ_SEG         0
#define UPDATE_SPK_EQ_GLOBAL_GIAN 1
#define GET_SPK_EQ_SEG_TAB        2
#define GET_SPK_EQ_GLOBAL_GAIN    3
#define GET_SPK_EQ_VM_DATA        4

#endif
