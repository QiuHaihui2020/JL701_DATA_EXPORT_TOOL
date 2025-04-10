#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".spk_eq.data.bss")
#pragma data_seg(".spk_eq.data")
#pragma const_seg(".spk_eq.text.const")
#pragma code_seg(".spk_eq.text")
#endif

#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "effects/effects_adj.h"
#include "effects/audio_spk_eq.h"
#include "jlstream.h"
#include "user_cfg_id.h"
#include "online_db_deal.h"

#if TCFG_SPEAKER_EQ_NODE_ENABLE
static u8 spk_eq_read_from_ram = 0;
static float spk_eq_global_gain[2];
static struct eq_seg_info spk_eq_tab[20] = {
    //left
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,    0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,   0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,   0, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   0, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0, 0.7f},

    //right
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,    0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,   0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,   0, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   0, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0, 0.7f},
};

/*
 *spk_eq 系数更新接口,更新第几段eq系数
 *parm: *seg
 *seg->index:第几段(0~9)
 *seg->iir_type:滤波器类型(EQ_IIR_TYPE)
 *seg->freq:中心截止频率(20~20kHz)
 *seg->gain:增益（-12~13dB）
 * */
void spk_eq_seg_update(struct eq_seg_info *seg)
{
    //段系数更新
    struct spk_eq_seg_parm sparm;
    sparm.type  = UPDATE_SPK_EQ_SEG;
    sparm.left_right = 0;
    memcpy(&sparm.seg, seg, sizeof(struct eq_seg_info));
    int ret = jlstream_set_node_param(NODE_UUID_SPEAKER_EQ, "spk_eq", &sparm, sizeof(sparm));
    if (ret <= 0) {
        struct eq_seg_info *tar_seg = (struct eq_seg_info *)spk_eq_tab;
        memcpy(&tar_seg[sparm.seg.index], &sparm.seg, sizeof(struct eq_seg_info));
    }
}

/*
 *spk_eq 右声道系数更新接口,更新第几段eq系数
 *parm: *seg
 *seg->index:第几段(0~9)
 *seg->iir_type:滤波器类型(EQ_IIR_TYPE)
 *seg->freq:中心截止频率(20~20kHz)
 *seg->gain:增益（-12~13dB）
 * */
void spk_eq_seg_update_R(struct eq_seg_info *seg)
{
    //段系数更新
    struct spk_eq_seg_parm sparm;
    sparm.type  = UPDATE_SPK_EQ_SEG;
    sparm.left_right = 1;
    memcpy(&sparm.seg, seg, sizeof(struct eq_seg_info));
    int ret = jlstream_set_node_param(NODE_UUID_SPEAKER_EQ, "spk_eq", &sparm, sizeof(sparm));
    if (ret <= 0) {
        struct eq_seg_info *tar_seg = (struct eq_seg_info *)&spk_eq_tab[10];
        memcpy(&tar_seg[sparm.seg.index], &sparm.seg, sizeof(struct eq_seg_info));
    }
}

/*
 *spk_eq 左声道总增益更新
 * */
void spk_eq_global_gain_udapte(float global_gain)
{
    struct spk_eq_global_gain gparm;
    gparm.type = UPDATE_SPK_EQ_GLOBAL_GIAN;
    gparm.left_right = 0;
    gparm.global_gain = global_gain;
    printf("dbug 1\n");
    int ret = jlstream_set_node_param(NODE_UUID_SPEAKER_EQ, "spk_eq", &gparm, sizeof(gparm));
    spk_eq_global_gain[gparm.left_right] = gparm.global_gain;
}
/*
 *spk_eq 右声道总增益更新
 * */

void spk_eq_global_gain_udapte_R(float global_gain)
{
    struct spk_eq_global_gain gparm;
    gparm.type = UPDATE_SPK_EQ_GLOBAL_GIAN;
    gparm.left_right = 1;
    gparm.global_gain = global_gain;
    int ret = jlstream_set_node_param(NODE_UUID_SPEAKER_EQ, "spk_eq", &gparm, sizeof(gparm));
    spk_eq_global_gain[gparm.left_right] = gparm.global_gain;
}

int spk_eq_save_to_vm(void)
{
    int ret_tmp = 0;
    struct spk_eq_get_seg_tab seg_tab = {0};
    seg_tab.type = GET_SPK_EQ_SEG_TAB;//获取系数表
    int ret = jlstream_get_node_param(NODE_UUID_SPEAKER_EQ, "spk_eq", (void *)&seg_tab, sizeof(seg_tab));
    if (ret <= 0) {
        seg_tab.seg = (struct eq_seg_info *)spk_eq_tab;
        seg_tab.tab_size = sizeof(spk_eq_tab);
    }

    ret = syscfg_write(CFG_SPK_EQ_SEG_SAVE, seg_tab.seg, seg_tab.tab_size);
    if (ret <= 0) {
        printf("spk_eq tab write to vm err, ret %d\n", ret);
        ret_tmp = -1;
    }

    struct spk_eq_get_global_gain g_gain = {0};
    g_gain.type = GET_SPK_EQ_GLOBAL_GAIN;//获取总增益
    ret = jlstream_get_node_param(NODE_UUID_SPEAKER_EQ, "spk_eq", (void *)&g_gain, sizeof(g_gain));
    if (ret <= 0) {
        memcpy(g_gain.global_gain, spk_eq_global_gain, sizeof(spk_eq_global_gain));
    }
    ret = syscfg_write(CFG_SPK_EQ_GLOBAL_GAIN_SAVE, g_gain.global_gain, sizeof(g_gain.global_gain));
    if (ret <= 0) {
        printf("spk_eq global gain write to vm err ret %d\n", ret);
        ret_tmp = -1;
    }
    return ret_tmp;
}
/*
 *spk_eq 系数表从vm中读取
 * */
int spk_eq_read_from_vm(void *priv)
{
    struct spk_eq_get_parm *spk_parm = (struct spk_eq_get_parm *)priv;
    if (spk_parm->type == GET_SPK_EQ_SEG_TAB) {
        spk_parm->seg_tab.seg = (struct eq_seg_info *)spk_eq_tab;
        spk_parm->seg_tab.tab_size = sizeof(spk_eq_tab) / 2;
        //printf("-------spk tab %x, %d\n", (int)spk_eq_tab, spk_parm->seg_tab.tab_size);
    } else if (spk_parm->type == GET_SPK_EQ_VM_DATA) {
        struct spk_eq_get_seg_tab *seg_tab = (struct spk_eq_get_seg_tab *)&spk_parm->seg_tab;
        struct spk_eq_get_global_gain *g_gain = (struct spk_eq_get_global_gain *)&spk_parm->g_gain;

        if (spk_eq_read_from_ram) {
            memcpy(g_gain->global_gain, spk_eq_global_gain, sizeof(spk_eq_global_gain));
            return 0;
        }
        spk_eq_read_from_ram = 1;
        int ret = syscfg_read(CFG_SPK_EQ_SEG_SAVE, seg_tab->seg, seg_tab->tab_size);
        if (ret <= 0) {
            printf("skp_eq read from vm err\n");
            return -1;
        }
        ret = syscfg_read(CFG_SPK_EQ_GLOBAL_GAIN_SAVE, g_gain->global_gain, sizeof(g_gain->global_gain));
        if (ret <= 0) {
            printf("spk_eq global gain read from vm err\n");
            return -1;
        }
    }
    return 0;
}

typedef struct {
    u16 magic;     //0x3344
    u16 crc;       //data crc
    u8 data[32];   //data
} SPK_EQ_PACK;

#define CMD_SEG    0x1
#define CMD_GLOBAL 0x2
#define CMD_SAVE_PARM 0x3
#define CMD_RESET_PARM 0x4
/*
 *右声道更新命令
 * */
#define CMD_SEG_R        0x5
#define CMD_GLOBAL_R     0x6

#define CMD_SUPPORT_GLOBAL_GAIN     0x7


#define CMD_READ_SEG_L   0x8//左声道或者单声道获取系数表
#define CMD_READ_SEG_R   0x9//右声道获取系数表

#define CMD_READ_GLOBAL_L  0xa //左声道或者单声道获取总增益
#define CMD_READ_GLOBAL_R  0xb //右声道获取总增益
#define SPK_EQ_CRC_EN  0//是否使能crc校验
static u8 parse_seq = 0;

void (*send_data_handler)(u8 seq, u8 *packet, u8 size);

static int spk_eq_ack_packet(u8 seq, u8 *packet, u8 size)
{
    if (send_data_handler) {
        send_data_handler(seq, packet, size);
    } else {
        return app_online_db_ack(seq, packet, size);
    }
    return 0;
}
int spk_eq_spp_rx_packet(u8 *packet, u8 len)
{

    SPK_EQ_PACK pack = {0};//packet;
    memcpy(&pack, packet, len);
    if (pack.magic != 0x3344) {
        printf("magic err 0x%x\n", pack.magic);
        return -1;
    }
    struct eq_seg_info seg = {0};
    float global_gain = 0;
    u8 cmd = pack.data[0];
    u16 crc;
    printf("cmd %d\n", cmd);
    switch (cmd) {
    case CMD_SEG:
#if SPK_EQ_CRC_EN
        crc = CRC16(pack.data, len - 4);
        if (crc != pack.crc) {
            printf("spk_seg pack crc err %x, %x\n", crc, pack.crc);
            return -1;
        }
#endif
        memcpy(&seg, &pack.data[1], sizeof(struct eq_seg_info));
        spk_eq_seg_update(&seg);

        printf("idx:%d, iir:%d, frq:%d, gain:0x%x, q:0x%x \n", seg.index, seg.iir_type, seg.freq, *(int *)&seg.gain, *(int *)&seg.q);
        break;
    case CMD_GLOBAL:
#if SPK_EQ_CRC_EN
        crc = CRC16(pack.data, len - 4);
        if (crc != pack.crc) {
            printf("spk global gain info pack crc err %x, %x\n", crc, pack.crc);
            return -1;
        }
#endif
        memcpy(&global_gain, &pack.data[1], sizeof(float));
        spk_eq_global_gain_udapte(global_gain);
        printf("global_gain 0x%x\n", *(int *)&global_gain);
        break;
    case CMD_SEG_R:
#if SPK_EQ_CRC_EN
        crc = CRC16(pack.data, len - 4);
        if (crc != pack.crc) {
            printf("spk_seg_r pack crc err %x, %x\n", crc, pack.crc);
            return -1;
        }
#endif
        memcpy(&seg, &pack.data[1], sizeof(struct eq_seg_info));
        spk_eq_seg_update_R(&seg);

        printf("R idx:%d, iir:%d, frq:%d, gain:0x%x, q:0x%x \n", seg.index, seg.iir_type, seg.freq, *(int *)&seg.gain, *(int *)&seg.q);
        break;
    case CMD_GLOBAL_R:
#if SPK_EQ_CRC_EN
        crc = CRC16(pack.data, len - 4);
        if (crc != pack.crc) {
            printf("spk global gain info pack crc err %x, %x\n", crc, pack.crc);
            return -1;
        }
#endif
        memcpy(&global_gain, &pack.data[1], sizeof(float));
        spk_eq_global_gain_udapte_R(global_gain);
        printf("R global_gain 0x%x\n", *(int *)&global_gain);
        break;
    case  CMD_SAVE_PARM:
        return spk_eq_save_to_vm();
        break;
    case CMD_RESET_PARM:
        struct eq_seg_info seg;
        for (int i = 0; i < 10; i++) {
            seg.index = i;
            seg.freq = 100;
            seg.iir_type = EQ_IIR_TYPE_BAND_PASS;
            seg.gain = 0.0f;
            seg.q = 0.7f;
            spk_eq_seg_update(&seg);
            spk_eq_seg_update_R(&seg);
        }
        spk_eq_global_gain_udapte(0);
        spk_eq_global_gain_udapte_R(0);
        return spk_eq_save_to_vm();
        break;
    case CMD_SUPPORT_GLOBAL_GAIN:
        //suuport global_gain
        break;
    case CMD_READ_SEG_L:
        spk_eq_ack_packet(parse_seq, (u8 *)spk_eq_tab, sizeof(spk_eq_tab) / 2);
        return 1;
    case CMD_READ_SEG_R:
        spk_eq_ack_packet(parse_seq, (u8 *)&spk_eq_tab[10], sizeof(spk_eq_tab) / 2);
        return 1;
    case CMD_READ_GLOBAL_L:
        spk_eq_ack_packet(parse_seq, (u8 *)&spk_eq_global_gain[0], 4);
        return 1;
    case CMD_READ_GLOBAL_R:
        spk_eq_ack_packet(parse_seq, (u8 *)&spk_eq_global_gain[1], 4);
        return 1;
    default:
        printf("crc %d\n", pack.crc);
        printf("cmd err %x\n", cmd);
        return -1;
    }
    return 0;
}

int spk_eq_read_seg_l(u8 **buf)
{
    *buf = (u8 *)(&spk_eq_tab[0]);
    return sizeof(spk_eq_tab) / 2;
}

int spk_eq_read_seg_r(u8 **buf)
{
#if AUDIO_SPK_EQ_STERO
    *buf = (u8 *)(&spk_eq_tab[10]);
    return sizeof(spk_eq_tab) / 2;
#else
    return 0;
#endif
}

int spk_eq_app_online_parse(u8 *packet, u8 size, u8 *ext_data, u16 ext_size)
{
    if (ext_data) {
        parse_seq = ext_data[1];
    } else {
        parse_seq = 0xff;
    }
    printf("parse_seq %x\n", parse_seq);
    int ret = spk_eq_spp_rx_packet(packet, size);
    if (!ret) {
        u8 ack[] = "OK";
        spk_eq_ack_packet(parse_seq, ack, sizeof(ack));
    } else if (ret != 1) {
        u8 ack[] = "ER";
        spk_eq_ack_packet(parse_seq, ack, sizeof(ack));
    }
    return 0;
}

void spk_eq_set_send_data_handler(void (*handler)(u8 seq, u8 *packet, u8 size))
{
    send_data_handler = handler;
}

int spk_eq_init(void)
{
#if APP_ONLINE_DEBUG
    app_online_db_register_handle(DB_PKT_TYPE_SPK_EQ, spk_eq_app_online_parse);
#endif
    return 0;
}
__initcall(spk_eq_init);
#endif
