#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_iis.data.bss")
#pragma data_seg(".audio_iis.data")
#pragma const_seg(".audio_iis.text.const")
#pragma code_seg(".audio_iis.text")
#endif
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "circular_buf.h"
#include "audio_iis.h"
#include "audio_link.h"
#include "effects/effects_adj.h"
#include "jlstream.h"
#include "gpio_config.h"
#include "audio_config.h"
#include "sync/audio_syncts.h"


#define IIS_LOG_ENABLE          0
#if IIS_LOG_ENABLE
#define LOG_TAG     "[IIS]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"
#else
#define log_info(fmt, ...)
#define log_error(fmt, ...)
#define log_debug(fmt, ...)
#endif

#ifdef SUPPORT_MS_EXTENSIONS
#pragma   code_seg(".audio_link.text")
#pragma   data_seg(".audio_link.data")
#pragma  const_seg(".audio_link.text.const")
#endif

#if TCFG_IIS_NODE_ENABLE



struct _iis_hdl *iis_hdl[2] = {NULL};



static spinlock_t lock[2];
//硬件模块的状态
#define IIS_MODULE_STATE_INIT        0
#define IIS_MODULE_STATE_OPEN        1
#define IIS_MODULE_STATE_START       2
#define IIS_MODULE_STATE_CLOSE       3
#define IIS_MODULE_STATE_UNINIT      4

//硬件通道的状态
#define IIS_STATE_INIT        0
#define IIS_STATE_OPEN        1
#define IIS_STATE_START       2
#define IIS_STATE_PREPARE     3
#define IIS_STATE_STOP        4
#define IIS_STATE_UNINIT

#define IIS_RX_DMA_LEN		(1024 * 4)

void audio_alink_lock(u8 module_idx)
{
    spin_lock(&lock[module_idx]);
}
void audio_alink_unlock(u8 module_idx)
{
    spin_unlock(&lock[module_idx]);
}
int alink_fifo_buffered_frames(struct _iis_hdl *hdl, u8 ch_idx);
int audio_iis_get_buffered_frames(void *hdl, u8 ch_idx)
{
    return alink_fifo_buffered_frames(hdl, ch_idx);
}
static int audio_iis_buf_frames_fade_out_32bit(void *_hdl, u8 ch_idx, int frames)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;

    u16 buffered_frames = alink_fifo_buffered_frames(hdl, ch_idx);
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)(hdl->hw_alink_ch[ch_idx]);
    u32 swp = alink_get_swptr(hw_channel_parm) / 2;

    s16 offset;
    s32 *ptr;
    int down_step;
    int value = 16384;
    int i, j;
    u8 deal_channel_num;



    if (frames > buffered_frames) {
        frames = buffered_frames;
    }

    if (frames < 1) {
        frames = 1;
    }
    offset = swp - frames;
    if (offset < 0) {
        offset += alink_get_dma_len(hdl->hw_alink) / 2;
    }
    down_step = (16384 / frames) + 1;

    for (i = 0; i < frames; i++) {
        if (value > 0) {
            value -= down_step;
            if (value < 0) {
                value = 0;
            }
        }

        deal_channel_num = hdl->channel;

        ptr = (s32 *)alink_get_addr(hw_channel_parm) + offset * deal_channel_num;
        for (j = 0; j < deal_channel_num; j++) {
            int tmp = *(ptr + j);
            *(ptr + j) = (long long)tmp * (long long)value / 16384;
        }

        if (++offset >= alink_get_dma_len(hdl->hw_alink) / 2) {
            offset = 0;
        }
    }

    return 0;
}

static int audio_iis_buf_frames_fade_out(void *_hdl, u8 ch_idx, int frames)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (!frames) {
        return 0;
    }
    if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
        return audio_iis_buf_frames_fade_out_32bit(hdl, ch_idx, frames);
    }
    u16 buffered_frames = alink_fifo_buffered_frames(hdl, ch_idx);
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)(hdl->hw_alink_ch[ch_idx]);
    u32 swp = alink_get_swptr(hw_channel_parm);

    s16 offset;
    s16 *ptr;
    int down_step;
    int value = 16384;
    int i, j;
    u8 deal_channel_num;



    if (frames > buffered_frames) {
        frames = buffered_frames;
    }

    if (frames < 1) {
        frames = 1;
    }
    offset = swp - frames;
    if (offset < 0) {
        offset += alink_get_dma_len(hdl->hw_alink);
    }
    down_step = (16384 / frames) + 1;
    for (i = 0; i < frames; i++) {
        if (value > 0) {
            value -= down_step;
            if (value < 0) {
                value = 0;
            }
        }

        deal_channel_num = hdl->channel;

        ptr = (s16 *)alink_get_addr(hw_channel_parm) + offset * deal_channel_num;
        for (j = 0; j < deal_channel_num; j++) {
            int tmp = *(ptr + j);
            *(ptr + j) = tmp * value / 16384;
        }

        if (++offset >= alink_get_dma_len(hdl->hw_alink)) {
            offset = 0;
        }
    }

    return 0;
}
/*
 *获取iis配置信息
 * */
int audio_iis_read_cfg(u8 module_idx, struct iis_file_cfg *cfg)
{
    int rlen = syscfg_read(module_idx ? CFG_ID_ALINK1_ID : CFG_ID_ALINK0_ID, cfg, sizeof(*cfg));
    return rlen;
}

//检查iis 硬件模块rx与tx是否同时开启,同时开启返回1，否则返回0
int audio_iis_check_hw_rx_and_tx_status(u8 module_idx)
{
    u8 tx_en = 0;
    u8 rx_en = 0;
    struct iis_file_cfg cfg = {0};
    int rlen = audio_iis_read_cfg(module_idx, &cfg);
    if (rlen == sizeof(cfg)) {
        for (int i = 0; i < 4; i++) {
            if (cfg.hwcfg[i].en) {
                if (cfg.hwcfg[i].dir) {
                    rx_en = 1;
                } else {
                    tx_en = 1;
                }
            }
        }
        if ((tx_en + rx_en) == 2) { //tx 与rx同时使能
            return 1;
        }
        if ((tx_en == 1) && (rx_en == 0)) { //只有tx 使能
            return 1;
        }
    }
    return 0;
}
/*
 *检查相应通道的输入输出方向,以及使能的状态
 * */
int audio_iis_check_hw_cfg_status(u8 module_idx, u8 ch_idx, u8 tar_dir)
{
    int find = false;
    int en = 0;
    u8 dir = 0;
    struct iis_file_cfg cfg = {0};
    int rlen = audio_iis_read_cfg(module_idx, &cfg);
    if (rlen == sizeof(cfg)) {
        for (int i = 0; i < 4; i++) {
            if (i != ch_idx) {
                continue;
            }
            if (cfg.hwcfg[i].en) {
                if (!cfg.hwcfg[i].dir) {
                    dir = ALINK_DIR_TX;
                } else {
                    dir = ALINK_DIR_RX;
                }
            }
            en = cfg.hwcfg[i].en;
            find = true;
            break;
        }
    }

    if (find) {
        if (!en) {
            ASSERT(0, "iis ch[%d] cfg disable\n", ch_idx);  //通道未使能
        }
        if (en && (dir != tar_dir)) {
            ASSERT(0, "iis ch[%d] %s cfg dir error\n", ch_idx, tar_dir ? "rx" : "tx");  //通道方向配置或者使用错误
        }

    } else {
        ASSERT(0, "iis module[%d] cfg error\n", module_idx);    //可视化配置界面模块参数结构与代码参数结构不一致
    }

    return find;
}

/*
 * audio_iis_init : 	申请iis需要的资源和初始化iis配置
 * param: module_idx:   br27 有两个ALINK模块，module可传入值为0 或 1
 *        sr:           采样率
 *        bit_width:    位宽配置 0：16bit 1:24bit
 *        dma_size:     决定iis buf大小，dma_len / 2 的值为中断起一次的长度len的值
 */
void *audio_iis_init(struct alink_param params)
{
    struct _iis_hdl *hdl = zalloc(sizeof(struct _iis_hdl));
    u8 module_idx = params.module_idx;
    u8 bit_width  = params.bit_width;
    u32 dma_size  = params.dma_size;
    u32 sr        = params.sr;
    if (hdl) {
        hdl->fixed_pns = params.fixed_pns;
        log_debug("--- %s, module_idx:%d, dma_size:%d, sr:%d, mode:%d\n", __func__, module_idx, dma_size, sr, ALINK_BUF_CIRCLE);
        /* 获取配置文件内的参数 */
        int rlen = audio_iis_read_cfg(module_idx, &hdl->cfg);
        if (rlen == sizeof(hdl->cfg)) {
            hdl->io_cfg.mclk = uuid2gpio(hdl->cfg.io_mclk_uuid);
            hdl->io_cfg.sclk = uuid2gpio(hdl->cfg.io_sclk_uuid);
            hdl->io_cfg.lrclk = uuid2gpio(hdl->cfg.io_lrclk_uuid);
            hdl->io_cfg.data_io[0] = uuid2gpio(hdl->cfg.hwcfg[0].data_io_uuid);
            hdl->io_cfg.data_io[1] = uuid2gpio(hdl->cfg.hwcfg[1].data_io_uuid);
            hdl->io_cfg.data_io[2] = uuid2gpio(hdl->cfg.hwcfg[2].data_io_uuid);
            hdl->io_cfg.data_io[3] = uuid2gpio(hdl->cfg.hwcfg[3].data_io_uuid);
        } else {
            log_error("iis read params err:rlen %d %d\n", rlen, (int)sizeof(hdl->cfg));
            return NULL;
        }

        // iis共同的参数配置
        hdl->alink_parm.mode = ALINK_MD_IIS;
        hdl->alink_parm.clk_mode = ALINK_CLK_FALL_UPDATE_RAISE_SAMPLE;
        hdl->alink_parm.dma_len = dma_size;
        hdl->alink_parm.buf_mode = ALINK_BUF_CIRCLE;
        hdl->alink_parm.sample_rate = sr;
        hdl->alink_parm.module = hdl->cfg.module_idx;
        hdl->alink_parm.role = hdl->cfg.role;
        hdl->alink_parm.mclk_io = hdl->io_cfg.mclk;
        hdl->alink_parm.sclk_io = hdl->io_cfg.sclk;
        hdl->alink_parm.lrclk_io = hdl->io_cfg.lrclk;
        hdl->alink_parm.da2sync_ch = 0xff;
        if (bit_width == DATA_BIT_WIDE_24BIT) {
            hdl->alink_parm.bitwide = ALINK_LEN_24BIT;
            hdl->alink_parm.sclk_per_frame = ALINK_FRAME_64SCLK;
        } else {
            hdl->alink_parm.bitwide = ALINK_LEN_16BIT;
            hdl->alink_parm.sclk_per_frame = ALINK_FRAME_32SCLK;
        }
        for (int i = 0; i < 4; i++) {
            if (hdl->cfg.hwcfg[i].en) {
                hdl->ch_cfg[i].dir = hdl->cfg.hwcfg[i].dir ? ALINK_DIR_RX : ALINK_DIR_TX;
                hdl->ch_cfg[i].ch_ie = hdl->cfg.hwcfg[i].dir ? 1 : 0;
                hdl->ch_cfg[i].isr_cb = NULL;
                hdl->ch_cfg[i].private_data = NULL;
                hdl->ch_cfg[i].data_io = hdl->io_cfg.data_io[i];
                hdl->state[i] = IIS_STATE_INIT;
            }
            INIT_LIST_HEAD(&(hdl->sync_list[i]));
            INIT_LIST_HEAD(&(hdl->tx_irq_list[i]));
            INIT_LIST_HEAD(&(hdl->rx_irq_list[i]));
            os_mutex_create(&hdl->mutex[i]);
        }

        hdl->channel = 2;//iis硬件通道是立体声
        /* -------------------------------iis tx 接入audio cfifo----------------------------------- */
        spin_lock_init(&lock[hdl->alink_parm.module]);
        hdl->start = IIS_MODULE_STATE_INIT;
        log_debug("%s, iis  init succ! new sr : %d\n", __func__, sr);
    }
    return hdl;
}


static void iis_txx_handle(void *priv, void *addr, int len, u8 ch_idx)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)priv;
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)(hdl->hw_alink_ch[ch_idx]);
    audio_iis_buf_frames_fade_out(hdl, ch_idx, alink_fifo_buffered_frames(hdl, ch_idx));
    struct audio_iis_channel *ch;
    list_for_each_entry(ch, &hdl->tx_irq_list[ch_idx], entry) {
        if (ch->attr.ch_idx == ch_idx) {
            ch->fade_out = 1;
        }
    }
    alink_set_ch_ie(hw_channel_parm, 0);
    if (hdl->alink_parm.module) {
        putchar('V');
    } else {
        putchar('v');
    }
    putchar('H' + ch_idx);
    /* log_debug("iis hw[%d]ch[%d] fade out\n",hdl->alink_parm.module, ch_idx); */
}
static void iis_tx0_handle(void *priv, void *addr, int len)	//中断回调
{
    iis_txx_handle(priv, addr, len, 0);
}
static  void iis_tx1_handle(void *priv, void *addr, int len)	//中断回调
{
    iis_txx_handle(priv, addr, len, 1);
}
static  void iis_tx2_handle(void *priv, void *addr, int len)	//中断回调
{
    iis_txx_handle(priv, addr, len, 2);
}
static void iis_tx3_handle(void *priv, void *addr, int len)	//中断回调
{
    iis_txx_handle(priv, addr, len, 3);
}

static int iis_tx_handle[4] = {(int)iis_tx0_handle, (int)iis_tx1_handle, (int)iis_tx2_handle, (int)iis_tx3_handle};
static void iis_rxx_handle(void *priv, void *addr, int len, u8 ch_idx)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)priv;
    struct audio_iis_rx_output_hdl *p;
    list_for_each_entry(p, &hdl->rx_irq_list[ch_idx], entry) {
        p->handler(p->priv, addr, len);
    }
}
static void iis_rx0_handle(void *priv, void *addr, int len)	//中断回调
{
    iis_rxx_handle(priv, addr, len, 0);
}
static  void iis_rx1_handle(void *priv, void *addr, int len)	//中断回调
{
    iis_rxx_handle(priv, addr, len, 1);
}
static  void iis_rx2_handle(void *priv, void *addr, int len)	//中断回调
{
    iis_rxx_handle(priv, addr, len, 2);
}
static void iis_rx3_handle(void *priv, void *addr, int len)	//中断回调
{
    iis_rxx_handle(priv, addr, len, 3);
}

static int iis_rx_handle[4] = {(int)iis_rx0_handle, (int)iis_rx1_handle, (int)iis_rx2_handle, (int)iis_rx3_handle};

/*
 *iis 硬件初始化，通道配置
 *ch_idx:配置与蓝牙同步关联的目标通道,不需要关联时直接配0xff
 * */
void audio_iis_open(void *_hdl, u8 ch_idx)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (hdl) {
        spin_lock(&lock[hdl->alink_parm.module]);
        if (!hdl->hw_alink) {
            hdl->alink_parm.da2sync_ch = ch_idx;
            hdl->hw_alink = alink_init(&hdl->alink_parm);
            float pns_time_ms = hdl->fixed_pns ? hdl->fixed_pns : 0.5f;
            hdl->protect_pns = (hdl->channel * ((hdl->alink_parm.bitwide == ALINK_LEN_24BIT) ? 4 : 2) * pns_time_ms * hdl->alink_parm.sample_rate / 1000) / 4; //
            alink_set_tx_pns(hdl->hw_alink, hdl->protect_pns);
        }
        if (!hdl->hw_alink) {
            spin_unlock(&lock[hdl->alink_parm.module]);
            return;
        }
        for (int i = 0; i < 4; i++) { //将所有需要使用的通道初始化
            if (hdl->cfg.hwcfg[i].en) {
                if (!hdl->hw_alink_ch[i]) {
                    log_debug(">>>>> hw[%d] %x ch[%d] init %s\n", hdl->alink_parm.module, (int)hdl->hw_alink, i, (hdl->ch_cfg[i].dir == ALINK_DIR_TX) ? "out" : "in");
                    if (hdl->ch_cfg[i].dir == ALINK_DIR_TX) {
                        audio_iis_set_irq_handler(hdl, i, hdl, (void (*)(void *, void *, int))iis_tx_handle[i]);
                    }
                    hdl->hw_alink_ch[i] = alink_channel_init_base(hdl->hw_alink, &hdl->ch_cfg[i], i);
                    if (hdl->ch_cfg[i].dir == ALINK_DIR_TX) {
                        u8 point_offset = 1;
                        if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
                            point_offset = 2;
                        }
                        audio_iis_set_shn(hdl, i, 1 * point_offset * 4); //预设设置1个点，避免shn乱动的问题
                    }
                }
            }
        }
        hdl->start = IIS_MODULE_STATE_OPEN;
        spin_unlock(&lock[hdl->alink_parm.module]);
    }
}

/*对目标硬件通道配置回调函数
 *ch_idx:       目标硬件通道
 *priv:		    handle的私有句柄
 *handle:       注册的iis_rx中断
 */
void audio_iis_set_irq_handler(void *_hdl, u8 ch_idx, void *priv, void (*handle)(void *priv, void *addr, int len))
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (hdl && hdl->hw_alink) {
        spin_lock(&lock[hdl->alink_parm.module]);
        hdl->ch_cfg[ch_idx].ch_idx = ch_idx;
        hdl->ch_cfg[ch_idx].private_data = priv;
        hdl->ch_cfg[ch_idx].isr_cb = handle;
        alink_set_irq_handler(hdl->hw_alink, &hdl->ch_cfg[ch_idx], priv, handle);
        spin_unlock(&lock[hdl->alink_parm.module]);
    }
}
/*
 *iisrx需要多输出时可通过该处理注册输出回调
 * */
void audio_iis_add_rx_output_handler(struct _iis_hdl *hdl, struct audio_iis_rx_output_hdl *output)
{
    struct audio_iis_rx_output_hdl *p;

    spin_lock(&lock[hdl->alink_parm.module]);

    if (list_empty(&hdl->rx_irq_list[output->ch_idx])) {
        audio_iis_set_irq_handler(hdl, output->ch_idx, hdl, (void (*)(void *, void *, int))iis_rx_handle[output->ch_idx]);
    }

    list_for_each_entry(p, &hdl->rx_irq_list[output->ch_idx], entry) {
        if ((u32)p == (u32)output) {
            goto __exit;
        }
    }

    list_add_tail(&output->entry, &hdl->rx_irq_list[output->ch_idx]);

__exit:
    spin_unlock(&lock[hdl->alink_parm.module]);
}

/*
 *iisrx删除已注册输出回调
 * */
void audio_iis_del_rx_output_handler(struct _iis_hdl *hdl, struct audio_iis_rx_output_hdl *output)
{
    struct audio_iis_rx_output_hdl *p;

    spin_lock(&lock[hdl->alink_parm.module]);

    list_for_each_entry(p, &hdl->rx_irq_list[output->ch_idx], entry) {
        if ((u32)p == (u32)output) {
            list_del(&output->entry);
            break;
        }
    }
    if (list_empty(&hdl->rx_irq_list[output->ch_idx])) {
        audio_iis_set_irq_handler(hdl, output->ch_idx, hdl, (void (*)(void *, void *, int))NULL);
    }

    spin_unlock(&lock[hdl->alink_parm.module]);
}

/*
 *iisrx需要多输出时可通过该处理注册输出回调
 * */
void audio_iis_add_multi_channel_rx_output_handler(struct _iis_hdl *hdl, struct audio_iis_rx_output_hdl *_output)
{
    struct audio_iis_rx_output_hdl *p;

    spin_lock(&lock[hdl->alink_parm.module]);
    for (int i = 0; i < 4; i++) {
        struct audio_iis_rx_output_hdl *output = &_output[i];
        if (output->enable) {
            if (list_empty(&hdl->rx_irq_list[output->ch_idx])) {
                audio_iis_set_irq_handler(hdl, output->ch_idx, hdl, (void (*)(void *, void *, int))iis_rx_handle[output->ch_idx]);
            }
            int next = 0;
            list_for_each_entry(p, &hdl->rx_irq_list[output->ch_idx], entry) {
                if ((u32)p == (u32)output) {
                    next = 1;
                    break;
                }
            }
            if (next) {
                continue;
            }

            list_add_tail(&output->entry, &hdl->rx_irq_list[output->ch_idx]);
        }
    }
    spin_unlock(&lock[hdl->alink_parm.module]);
}

/*
 *iisrx删除已注册输出回调
 * */
void audio_iis_del_multi_channel_rx_output_handler(struct _iis_hdl *hdl, struct audio_iis_rx_output_hdl *_output)
{
    struct audio_iis_rx_output_hdl *p;

    spin_lock(&lock[hdl->alink_parm.module]);
    for (int i = 0; i < 4; i++) {
        struct audio_iis_rx_output_hdl *output = &_output[i];
        if (output->enable) {
            list_for_each_entry(p, &hdl->rx_irq_list[output->ch_idx], entry) {
                if ((u32)p == (u32)output) {
                    list_del(&p->entry);
                    break;
                }
            }
            if (list_empty(&hdl->rx_irq_list[output->ch_idx])) {
                audio_iis_set_irq_handler(hdl, output->ch_idx, hdl, (void (*)(void *, void *, int))NULL);
            }
        }
    }
    spin_unlock(&lock[hdl->alink_parm.module]);
}
/*
 *iis模块启动
 * */
int audio_iis_start(void *_hdl)
{
    int ret = 0;
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (hdl && hdl->hw_alink) {
        spin_lock(&lock[hdl->alink_parm.module]);
        if (hdl->start == IIS_MODULE_STATE_OPEN) {
            hdl->start = IIS_MODULE_STATE_START;
            alink_start(hdl->hw_alink);
            spin_unlock(&lock[hdl->alink_parm.module]);
            udelay(100);
            spin_lock(&lock[hdl->alink_parm.module]);
            for (int i = 0; i < 4; i++) {//将 tx的io挪到alink_start之后初始化
                if (hdl->cfg.hwcfg[i].en) {
                    if (hdl->ch_cfg[i].dir == ALINK_DIR_TX) {
                        alink_channel_io_init(hdl->hw_alink, &hdl->ch_cfg[i], i);
                    }
                }
            }
            spin_unlock(&lock[hdl->alink_parm.module]);
            ret = 1;
        } else {
            spin_unlock(&lock[hdl->alink_parm.module]);
        }
    }
    return ret;
}

/*
 *iis 通道关闭
 * */
int audio_iis_stop(void *_hdl, u8 ch_idx)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (!hdl) {
        return 0;
    }
    u8 hw_ch_num = 0;
    for (int i = 0; i < 4; i++) {
        if (hdl->cfg.hwcfg[i].en) {
            hw_ch_num++;
        }
    }
    if (hdl->hw_alink_ch[ch_idx] && (hdl->ch_cfg[ch_idx].dir == ALINK_DIR_TX)) {
        u32 cnt = 0;
        while (alink_get_hwptr(hdl->hw_alink_ch[ch_idx]) != alink_get_swptr(hdl->hw_alink_ch[ch_idx])) { //等待iis把数据消耗完成，再往后操作swptr = 0, hwptr = 0, 硬件才能回去初始态
            os_time_dly(1);
            cnt++;
            if (cnt > 10) {
                break;
            }
        }
    }
    if (hw_ch_num != 1 ||  hdl->cfg.role == ALINK_ROLE_MASTER) { //iis 做主机时不关硬件,避免iix tx主机播完提示音关掉iis,导致从机无时钟,下次再播提示音时,从机声音变调
        return 0;//iis模块内通道使能 大于等于2个，默认不关闭硬件
    }

    if (hdl && hdl->hw_alink) {
        spin_lock(&lock[hdl->alink_parm.module]);
        if (hdl->ch_cfg[ch_idx].dir == ALINK_DIR_RX) {
            if (!list_empty(&hdl->rx_irq_list[ch_idx])) { //rx busy
                spin_unlock(&lock[hdl->alink_parm.module]);
                return 0;
            }
        }
        log_debug("iis hw[%d] ch[%d] %s close %d\n", hdl->alink_parm.module, ch_idx, (hdl->ch_cfg[ch_idx].dir == ALINK_DIR_TX) ? "out" : "in");
        if (hdl->hw_alink_ch[ch_idx]) {
            alink_channel_close(hdl->hw_alink, hdl->hw_alink_ch[ch_idx]);
            hdl->hw_alink_ch[ch_idx] = NULL;
        }
        spin_unlock(&lock[hdl->alink_parm.module]);
    }
    return 0;
}
/*
 *iis硬件关闭
 * */
int audio_iis_close(void *_hdl)
{
    int ret = 0;
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (hdl && hdl->hw_alink) {
        spin_lock(&lock[hdl->alink_parm.module]);
        ret =  alink_uninit_base(hdl->hw_alink, NULL);
        if (ret) {
            hdl->hw_alink = NULL;
            hdl->start = IIS_MODULE_STATE_CLOSE;
        }
        spin_unlock(&lock[hdl->alink_parm.module]);
    }
    return ret;
}
/*
 *iis资源释放
 * */
int audio_iis_uninit(void *_hdl)
{
    int ret = 0;
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (hdl) {
        u8 module_idx = hdl->alink_parm.module;
        spin_lock(&lock[module_idx]);
        if (hdl->start == IIS_MODULE_STATE_CLOSE) {//硬件通道全部关闭后，才可释放
            hdl->start = IIS_MODULE_STATE_UNINIT;
            free(hdl);
            hdl = NULL;
            ret = 1;
            log_debug("hw[%d], iis  release succ\n", module_idx);
        }
        spin_unlock(&lock[module_idx]);
    }
    return ret;
}

/*
 * 设置rx 起中断的点数
 */
void audio_iis_set_rx_irq_points(void *_hdl, int irq_points)
{
    /* return; */
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (hdl) {
        spin_lock(&lock[hdl->alink_parm.module]);
        if (hdl->alink_parm.buf_mode == ALINK_BUF_CIRCLE) {
            u32 point_size = (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) ? 4 : 2;
            int rx_pns = irq_points * point_size * hdl->channel / 4; //pns寄存器以4byte为单位
            if (hdl->start == IIS_MODULE_STATE_OPEN) {
                alink_set_rx_pns(hdl->hw_alink, rx_pns);
            } else {
                hdl->alink_parm.rx_pns = rx_pns;
            }
        }
        spin_unlock(&lock[hdl->alink_parm.module]);
        log_debug("iis set rx irq_points %d\n", irq_points);
    } else {
        printf("alink_rx is not open !!!");
    }
}
int audio_iis_get_hwptr(void *_hdl, u8 ch_idx)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    spin_lock(&lock[hdl->alink_parm.module]);
    int hwptr = alink_get_hwptr(hdl->hw_alink_ch[ch_idx]);
    spin_unlock(&lock[hdl->alink_parm.module]);
    return hwptr * 4;
}
int audio_iis_get_swptr(void *_hdl, u8 ch_idx)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    spin_lock(&lock[hdl->alink_parm.module]);
    int swptr = alink_get_swptr(hdl->hw_alink_ch[ch_idx]);
    spin_unlock(&lock[hdl->alink_parm.module]);
    return swptr * 4;
}
/*
 * 选择循环buffer的方式时，需要告诉寄存器收到了len个数据
 */
void audio_iis_set_shn(void *_hdl, u8 ch_idx, int len)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (hdl && hdl->hw_alink_ch[ch_idx]) {
        spin_lock(&lock[hdl->alink_parm.module]);
        u32 shn = alink_get_shn(hdl->hw_alink_ch[ch_idx]);
        alink_set_shn(hdl->hw_alink_ch[ch_idx], len / 4);
        spin_unlock(&lock[hdl->alink_parm.module]);
        if (shn < (len / 4)) {
            printf("=====iis write error=====:shn %d, %d, w shh %d %d\n", shn, shn / 2, len / 8, len / 4);
        }
    }
}
__NODE_CACHE_CODE(iis)
void audio_iis_multi_set_shn(struct multi_ch *mch, u8 master_ch_idx, int len[4])
{
    struct audio_iis_channel *master_ch = (void *)mch->ch[master_ch_idx];
    struct _iis_hdl *hdl = (struct _iis_hdl *)master_ch->iis;
    if (hdl) {
        spin_lock(&lock[hdl->alink_parm.module]);
        for (int i = 0; i < 4; i++) {
            if (mch->ch[i] && hdl->hw_alink_ch[i] && (hdl->ch_cfg[i].dir == ALINK_DIR_TX)) {
                if (!len[i] || !(len[i] >> 2)) {
                    /* printf("mshn set error [%d] %d\n", i, len[i]); */
                    continue;
                }
                alink_set_shn_with_no_sync(hdl->hw_alink_ch[i], len[i] >> 2);
            }
        }
        __asm_csync();
        spin_unlock(&lock[hdl->alink_parm.module]);
    }
}
/*
 *通道是否初始化
 * */
int audio_iis_ch_alive(void *_hdl, u8 ch_idx)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    int ret = 0 ;
    if (hdl && hdl->hw_alink) {
        spin_lock(&lock[hdl->alink_parm.module]);
        ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hdl->hw_alink;
        if (hw_alink_parm->ch_cfg[ch_idx].ch_en) {
            ret = 1;
        }
        spin_unlock(&lock[hdl->alink_parm.module]);
    }
    return ret;
}
/*
 *硬件是否启动
 * */
u8 audio_iis_is_working(void *_hdl)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (!hdl) {
        return 0;
    }
    spin_lock(&lock[hdl->alink_parm.module]);
    if ((hdl->start != IIS_MODULE_STATE_INIT) && (hdl->start != IIS_MODULE_STATE_CLOSE)) {
        spin_unlock(&lock[hdl->alink_parm.module]);
        return 1;
    }
    spin_unlock(&lock[hdl->alink_parm.module]);

    return 0;
}
/*
 *采样率获取
 * */
int audio_iis_get_sample_rate(void *_hdl)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (!hdl) {
        return 0;
    }
    return hdl->alink_parm.sample_rate;

}
void audio_iis_set_da2sync_ch(void *_hdl, u8 ch_idx)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (!hdl) {
        return;
    }
    hdl->alink_parm.da2sync_ch = ch_idx;
    alink_set_da2sync_ch(&hdl->alink_parm);
}

/* -------------------------------iis tx 接入audio cfifo----------------------------------- */

#define AUDIO_CFIFO_IDLE      0
#define AUDIO_CFIFO_INITED    1
#define AUDIO_CFIFO_START     2
#define AUDIO_CFIFO_CLOSE     3

int audio_iis_syncts_enter_update_frame(struct audio_iis_channel *ch);
int audio_iis_syncts_exit_update_frame(struct audio_iis_channel *ch);
int audio_iis_channel_buffered_frames(struct audio_iis_channel *ch);



/*
 * Audio DAC申请加入一个新的DAC通道
 */
int audio_iis_new_channel(void *_hdl, void *iis_ch)
{
    struct audio_iis_channel *ch = (struct audio_iis_channel *)iis_ch;
    if (!_hdl || !ch) {
        return -EINVAL;
    }
    memset(ch, 0x0, sizeof(struct audio_iis_channel));
    ch->iis = _hdl;
    ch->state = AUDIO_CFIFO_IDLE;
    return 0;
}

void audio_iis_channel_start(void *iis_ch)
{
    struct audio_iis_channel *ch = (struct audio_iis_channel *)iis_ch;
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        log_e("IIS channel start error! iis_hdl is NULL\n");
        return;
    }
    os_mutex_pend(&hdl->mutex[ch->attr.ch_idx], 0);

    spin_lock(&lock[hdl->alink_parm.module]);
    if (!audio_iis_is_working(hdl)) {
        log_debug("iis hw[%d] open\n", hdl->alink_parm.module);
        audio_iis_open(hdl, ch->attr.ch_idx);
        audio_iis_start(hdl);
        for (int i = 0; i < 4; i++) { //将所有需要使用的通道初始化
            if (hdl->cfg.hwcfg[i].en) {
                hdl->state[i] = IIS_STATE_OPEN;
                hdl->fifo_state[i] = AUDIO_CFIFO_IDLE;
            }
        }
    }
    spin_unlock(&lock[hdl->alink_parm.module]);

    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)(hdl->hw_alink_ch[ch->attr.ch_idx]);
    if (!hw_channel_parm) {
        log_error("iis hw[%d], ch[%d] uninit\n", hdl->alink_parm.module, ch->attr.ch_idx);
        os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);
        return;
    }
    if (hdl->fifo_state[ch->attr.ch_idx] != AUDIO_CFIFO_INITED) {
        log_debug("cfifo init hw[%d] ch[%d]\n", hdl->alink_parm.module, ch->attr.ch_idx);
        if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
            hdl->cfifo[ch->attr.ch_idx].bit_wide = DATA_BIT_WIDE_24BIT;
        }
        ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hdl->hw_alink;
        audio_cfifo_init(&hdl->cfifo[ch->attr.ch_idx], hw_channel_parm->buf, hw_alink_parm->dma_len, hdl->alink_parm.sample_rate, hdl->channel);
        hdl->fifo_state[ch->attr.ch_idx] = AUDIO_CFIFO_INITED;
        if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
            audio_cfifo_set_readlock_samples(&hdl->cfifo[ch->attr.ch_idx], 1);
        }
    }
    if (hdl->state[ch->attr.ch_idx] != IIS_STATE_START) {
        alink_set_swptr(hw_channel_parm, 0);//解决 iis 不关硬件情况下，再次读shn是个随机值问题
        log_debug(" channel add ch->attr.ch_idx %d, ch->fifo %x, cfifo %x\n", ch->attr.ch_idx, (int)&ch->fifo, (int)&hdl->cfifo[ch->attr.ch_idx]);

        audio_cfifo_channel_add(&hdl->cfifo[ch->attr.ch_idx], &ch->fifo, ch->attr.delay_time, ch->attr.write_mode);

        ch->state = AUDIO_CFIFO_START;
        hdl->state[ch->attr.ch_idx] = IIS_STATE_START;
        spin_lock(&lock[hdl->alink_parm.module]);
        list_add(&ch->entry, &hdl->tx_irq_list[ch->attr.ch_idx]);
        spin_unlock(&lock[hdl->alink_parm.module]);
    } else {
        if (ch->state != AUDIO_CFIFO_START) {
            audio_cfifo_channel_add(&hdl->cfifo[ch->attr.ch_idx], &ch->fifo, ch->attr.delay_time, ch->attr.write_mode);
            ch->state = AUDIO_CFIFO_START;
            spin_lock(&lock[hdl->alink_parm.module]);
            list_add(&ch->entry, &hdl->tx_irq_list[ch->attr.ch_idx]);
            spin_unlock(&lock[hdl->alink_parm.module]);
        }
    }
    os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);
}

void audio_iis_multi_channel_start(struct multi_ch *mch)
{
    struct _iis_hdl *hdl = NULL;
    int master_ch_idx = 0xff;
    for (int i = 0 ; i < 4; i++) {
        struct audio_iis_channel *ch_h = mch->ch[i];
        if (ch_h) {
            hdl = (struct _iis_hdl *)ch_h->iis;
            if (!hdl) {
                continue;
            } else {
                if (master_ch_idx == 0xff) {
                    master_ch_idx = i;
                }
                break;
            }
        }
    }
    if (!hdl) {
        return ;
    }
    u8 point_offset = 1;
    if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
        point_offset = 2;
    }
    spin_lock(&lock[hdl->alink_parm.module]);
    if (!audio_iis_is_working(hdl)) {
        log_debug("iis hw[%d] open\n", hdl->alink_parm.module);
        audio_iis_open(hdl, master_ch_idx);
        audio_iis_start(hdl);
        for (int i = 0; i < 4; i++) { //将所有需要使用的通道初始化
            if (hdl->cfg.hwcfg[i].en) {
                hdl->state[i] = IIS_STATE_OPEN;
                hdl->fifo_state[i] = AUDIO_CFIFO_IDLE;
            }
        }
    }
    spin_unlock(&lock[hdl->alink_parm.module]);
    for (int i = 0; i < 4; i++) {
        struct audio_iis_channel *ch = (struct audio_iis_channel *)mch->ch[i];
        if (ch && hdl->ch_cfg[i].dir == ALINK_DIR_TX) {
            struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)(hdl->hw_alink_ch[ch->attr.ch_idx]);
            if (!hw_channel_parm) {
                log_error("iis hw[%d], ch[%d] uninit\n", hdl->alink_parm.module, ch->attr.ch_idx);
                continue;
            }
            if (hdl->fifo_state[ch->attr.ch_idx] != AUDIO_CFIFO_INITED) {
                log_debug("cfifo init hw[%d] ch[%d]\n", hdl->alink_parm.module, ch->attr.ch_idx);
                if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
                    hdl->cfifo[ch->attr.ch_idx].bit_wide = DATA_BIT_WIDE_24BIT;
                }

                ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hdl->hw_alink;
                audio_cfifo_init(&hdl->cfifo[ch->attr.ch_idx], hw_channel_parm->buf, hw_alink_parm->dma_len, hdl->alink_parm.sample_rate, hdl->channel);
                hdl->fifo_state[ch->attr.ch_idx] = AUDIO_CFIFO_INITED;
                if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
                    audio_cfifo_set_readlock_samples(&hdl->cfifo[ch->attr.ch_idx], 1);
                }
            }
            if (hdl->state[ch->attr.ch_idx] != IIS_STATE_START) {
                alink_set_swptr(hw_channel_parm, 0);//解决 iis 不关硬件情况下，再次读shn是个随机值问题
                log_debug(" channel add ch->attr.ch_idx %d, ch->fifo %x, cfifo %x\n", ch->attr.ch_idx, (int)&ch->fifo, (int)&hdl->cfifo[ch->attr.ch_idx]);
                audio_cfifo_channel_add(&hdl->cfifo[ch->attr.ch_idx], &ch->fifo, ch->attr.delay_time, ch->attr.write_mode);
                ch->state = AUDIO_CFIFO_START;
                hdl->state[ch->attr.ch_idx] = IIS_STATE_START;
                spin_lock(&lock[hdl->alink_parm.module]);
                list_add(&ch->entry, &hdl->tx_irq_list[ch->attr.ch_idx]);
                spin_unlock(&lock[hdl->alink_parm.module]);
            }
        }
    }


    int fifo_rp_max = 0;
    int fifo_rp[4] = {0};
    int sample_size[4] = {0};

    for (int i = 0; i < 4; i ++) {
        struct audio_iis_channel *ch = (struct audio_iis_channel *)mch->ch[i];
        if (ch && hdl->ch_cfg[i].dir == ALINK_DIR_TX) {
            struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)(hdl->hw_alink_ch[ch->attr.ch_idx]);
            if (!hw_channel_parm) {
                log_error("iis hw[%d], ch[%d] uninit\n", hdl->alink_parm.module, ch->attr.ch_idx);
                continue;
            }

            if (ch->state != AUDIO_CFIFO_START) {
                fifo_rp[i] = audio_cfifo_get_fifo_rp(&hdl->cfifo[ch->attr.ch_idx]);
                sample_size[i] = audio_cfifo_get_fifo_sample_size(&hdl->cfifo[ch->attr.ch_idx]);
                if (fifo_rp_max < fifo_rp[i]) {
                    fifo_rp_max = fifo_rp[i];
                    if (fifo_rp_max >= sample_size[i]) {
                        fifo_rp_max -= sample_size[i];
                    }
                }
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        struct audio_iis_channel *ch = (struct audio_iis_channel *)mch->ch[i];
        if (ch && hdl->ch_cfg[i].dir == ALINK_DIR_TX) {
            struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)(hdl->hw_alink_ch[ch->attr.ch_idx]);
            if (!hw_channel_parm) {
                log_error("iis hw[%d], ch[%d] uninit\n", hdl->alink_parm.module, ch->attr.ch_idx);
                continue;
            }
            if (ch->state != AUDIO_CFIFO_START) {
                audio_cfifo_channel_add_with_fifo_rp(&hdl->cfifo[ch->attr.ch_idx], &ch->fifo, ch->attr.delay_time, ch->attr.write_mode, fifo_rp_max);//ch_fifo首地址使用fifo_rp对齐
                ch->state = AUDIO_CFIFO_START;
                spin_lock(&lock[hdl->alink_parm.module]);
                list_add(&ch->entry, &hdl->tx_irq_list[ch->attr.ch_idx]);
                spin_unlock(&lock[hdl->alink_parm.module]);
            }
        }
    }
}
int audio_iis_channel_close(void *iis_ch)
{
    int ret = 0;
    struct audio_iis_channel *ch = (struct audio_iis_channel *)iis_ch;
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return 0;
    }
    os_mutex_pend(&hdl->mutex[ch->attr.ch_idx], 0);
    if (ch->state != AUDIO_CFIFO_IDLE && ch->state != AUDIO_CFIFO_CLOSE) {
        if (ch->state == AUDIO_CFIFO_START) {
            audio_cfifo_channel_del(&ch->fifo);
            ch->state = AUDIO_CFIFO_CLOSE;
            spin_lock(&lock[hdl->alink_parm.module]);
            list_del(&ch->entry);
            spin_unlock(&lock[hdl->alink_parm.module]);
        }
    }
    if (hdl->fifo_state[ch->attr.ch_idx] != AUDIO_CFIFO_IDLE) {
        if (audio_cfifo_channel_num(&hdl->cfifo[ch->attr.ch_idx]) == 0) {
            hdl->unread_samples[ch->attr.ch_idx] = 0;
            hdl->state[ch->attr.ch_idx] = IIS_STATE_STOP;
            hdl->fifo_state[ch->attr.ch_idx] = AUDIO_CFIFO_CLOSE;
            audio_iis_stop(hdl, ch->attr.ch_idx);
            ret = audio_iis_close(hdl);
            log_debug("hw[%d] ch[%d] fifo close ret %d\n", hdl->alink_parm.module, ch->attr.ch_idx, ret);
        }
    }
    os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);
    return ret;
}

int alink_fifo_buffered_frames(struct _iis_hdl *hdl, u8 ch_idx)
{
    if (!hdl) {
        return 0;
    }
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)(hdl->hw_alink_ch[ch_idx]);
    if (!hw_channel_parm) {
        return 0;
    }

    u8 point_offset = 1;
    if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
        point_offset = 2;
    }
#if 0
    int unread_samples = alink_get_dma_len(hdl->hw_alink) - alink_get_shn(hw_channel_parm) ;
    if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
        unread_samples = unread_samples / point_offset - 1;
    } else {
        unread_samples = unread_samples  - 1;
    }
#else
    //24bit:shn是奇数时需先除2，再参与计算，才能避免0.5个正在读的点被忽略，导致写多1个点的情况
    int unread_samples = alink_get_dma_len(hdl->hw_alink) / point_offset - alink_get_shn(hw_channel_parm) / point_offset - 1;
#endif
    /* printf("ch_idx %d, dma_len %d, shn[%d] %d, unread_samples %d\n", ch_idx, alink_get_dma_len(hdl->hw_alink), ch_idx, alink_get_shn(hw_channel_parm), unread_samples); */

    if (unread_samples < 0) {
        unread_samples = 0;
    }

    return unread_samples;
}

void iis_tx_delay_debug()
{
    for (int i = 0; i < 4; i++) {
        struct _iis_hdl *hdl = (struct _iis_hdl *)iis_hdl[0];

        if (hdl && hdl->ch_cfg[i].dir == ALINK_DIR_TX && hdl->cfg.hwcfg[i].en) {
            int frames = alink_fifo_buffered_frames(hdl, i);
            int delay = frames * 1000 / audio_iis_get_sample_rate(hdl);;
            printf("---IIS0 ch[%d] %d ms\n", i, delay);
        }
    }
    for (int i = 0; i < 4; i++) {
        struct _iis_hdl *hdl = (struct _iis_hdl *)iis_hdl[1];
        if (hdl && hdl->ch_cfg[i].dir == ALINK_DIR_TX && hdl->cfg.hwcfg[i].en) {
            int frames = alink_fifo_buffered_frames(hdl, i);
            int delay = frames * 1000 / audio_iis_get_sample_rate(hdl);;
            printf("---IIS1 ch[%d] %d ms\n", i, delay);
        }
    }

}
int audio_iis_channel_latch_time(struct audio_iis_channel *ch, u32 *latch_time, u32(*get_time)(u32 reference_network), u32 reference_network)
{
    int buffered_frames = 0;
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return 0;
    }
    os_mutex_pend(&hdl->mutex[ch->attr.ch_idx], 0);
    spin_lock(&lock[hdl->alink_parm.module]);
    *latch_time = get_time(reference_network);
    buffered_frames = alink_fifo_buffered_frames(hdl, ch->attr.ch_idx) - audio_cfifo_channel_unread_diff_samples(&ch->fifo);
    if (buffered_frames < 0) {
        buffered_frames = 0;
    }
    spin_unlock(&lock[hdl->alink_parm.module]);
    os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);
    return buffered_frames;
}


int audio_iis_channel_buffered_frames(struct audio_iis_channel *ch)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return 0;
    }

    int buffered_frames = 0;

    os_mutex_pend(&hdl->mutex[ch->attr.ch_idx], 0);
    if (ch->state != AUDIO_CFIFO_START) {
        goto ret_value;
    }


    buffered_frames = alink_fifo_buffered_frames(hdl, ch->attr.ch_idx) - audio_cfifo_channel_unread_diff_samples(&ch->fifo);
    if (buffered_frames < 0) {
        buffered_frames = 0;
    }

ret_value:
    os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);

    return buffered_frames;
}

int audio_iis_data_len(void *iis_ch)
{
    struct audio_iis_channel *ch = (struct audio_iis_channel *)iis_ch;
    return audio_iis_channel_buffered_frames(ch);
}

int audio_iis_link_to_syncts_check(void *_hdl, void *syncts)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)_hdl;
    if (!hdl) {
        return 0;
    }

    struct audio_iis_sync_node *node = NULL;
    for (int i = 0; i < 4; i++) { //检查所有iis通道，当前synsts是否已经挂载上，挂载上就不需要重复挂载
        list_for_each_entry(node, &hdl->sync_list[i], entry) {
            if ((u32)node->hdl == (u32)syncts) {
                return 1;
            }
            /* if (hdl->alink_parm.module != sound_pcm_device_get(node->hdl)){检查当前模块是否已经挂载上 */
            /* return;*/
            /* } */
        }
    }
    return 0;
}

int iis_adapter_link_to_syncts_check(void *syncts)
{
    int ret = 0;
    for (int i = 0; i < 2; i++) {
        ret += audio_iis_link_to_syncts_check(iis_hdl[i], syncts);
    }
    return ret;
}
int dac_adapter_link_to_syncts_check(void *syncts);
int audio_iis_add_syncts_with_timestamp(void *iis_ch, void *syncts, u32 timestamp)
{
    struct audio_iis_channel *ch = (struct audio_iis_channel *)iis_ch;
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return 0;
    }

    /* if (sound_pcm_get_syncts_network(syncts) != AUDIO_NETWORK_LOCAL){ */
    struct audio_iis_sync_node *node = NULL;
    os_mutex_pend(&hdl->mutex[ch->attr.ch_idx], 0);

    if (iis_adapter_link_to_syncts_check(syncts)) {
        log_debug("syncts has beed link to iis");
        os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);
        return 0;
    }
#if TCFG_DAC_NODE_ENABLE
    if (dac_adapter_link_to_syncts_check(syncts)) {
        log_debug("syncts has beed link to dac");
        os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);
        return 0;
    }
#endif
    for (int i = 0; i < 4; i++) { //检查所有iis通道，当前synsts是否已经挂载上，挂载上就不需要重复挂载
        list_for_each_entry(node, &hdl->sync_list[i], entry) {
            if ((u32)node->hdl == (u32)syncts) {
                os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);
                return 0;
            }
            /* if (hdl->alink_parm.module != sound_pcm_device_get(node->hdl)){//检查当前模块是否已经挂载上 */
            /* return;// */
            /* } */
        }
    }
    /* }else{ */
    /* list_for_each_entry(node, &hdl->sync_list[ch->attr.ch_idx], entry) { */
    /* if ((u32)node->hdl == (u32)syncts) { */
    /* return ; */
    /* } */
    /* [> if (hdl->alink_parm.module != sound_pcm_device_get(node->hdl)){//检查当前模块是否已经挂载上 <] */
    /* [> return;// <] */
    /* [> } <] */
    /* } */
    /* } */

    log_debug(">>>>>>>>>>>>>>>>add iis syncts %x, hw[%d] ch_idx %d\n", (u32)syncts, hdl->alink_parm.module, ch->attr.ch_idx);
    if (sound_pcm_get_syncts_network(syncts) != AUDIO_NETWORK_LOCAL) { //本地时钟不需要关联
        audio_iis_set_da2sync_ch(hdl, ch->attr.ch_idx);//iis关联到同步
        sound_pcm_device_register(syncts, PCM_OUTSIDE_DAC);
    }

    node = (struct audio_iis_sync_node *)zalloc(sizeof(struct audio_iis_sync_node));
    node->hdl = syncts;
    node->network = sound_pcm_get_syncts_network(syncts);
    node->timestamp = timestamp;
    node->ch = ch;
    spin_lock(&lock[hdl->alink_parm.module]);
    list_add(&node->entry, &hdl->sync_list[ch->attr.ch_idx]);
    spin_unlock(&lock[hdl->alink_parm.module]);
    os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);
    return 1;
}
void audio_iis_remove_syncts_handle(void *iis_ch, void *syncts)
{
    struct audio_iis_channel *ch = (struct audio_iis_channel *)iis_ch;
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return ;
    }

    struct audio_iis_sync_node *node;

    os_mutex_pend(&hdl->mutex[ch->attr.ch_idx], 0);
    list_for_each_entry(node, &hdl->sync_list[ch->attr.ch_idx], entry) {
        if ((u32)node->ch != (u32)ch)  {
            continue;
        }
        if (node->hdl == syncts) {
            goto remove_node;
        }
    }
    os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);

    return;
remove_node:
    log_debug(">>>>>>>>>>>>>>>>>>>del iis syncts %x, hw[%d] ch_idx %d \n", (u32)syncts, hdl->alink_parm.module, ch->attr.ch_idx);

    spin_lock(&lock[hdl->alink_parm.module]);
    list_del(&node->entry);
    free(node);
    spin_unlock(&lock[hdl->alink_parm.module]);
    os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);
}

static int audio_iis_channel_use_syncts_frames(struct audio_iis_channel *ch, int frames)
{
    if (!ch) {
        return 0;
    }
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return 0;
    }

    struct audio_iis_sync_node *node;
    u8 have_syncts = 0;
    u8 point_offset = 1;
    if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
        point_offset = 2;
    }

    list_for_each_entry(node, &hdl->sync_list[ch->attr.ch_idx], entry) {
        if (!node->triggered) {
            continue;
        }
        /*
         * 这里需要将其他通道延时数据大于音频同步计数通道的延迟数据溢出值通知到音频同步
         * 锁存模块，否则会出现硬件锁存不同通道数据的溢出误差
         */
        int diff_samples = audio_cfifo_channel_unread_diff_samples(&((struct audio_iis_channel *)node->ch)->fifo);
        sound_pcm_update_overflow_frames(node->hdl, diff_samples);
        if ((u32)node->ch != (u32)ch) {
            continue;
        }
        sound_pcm_update_frame_num(node->hdl, frames);
        if (node->network == AUDIO_NETWORK_LOCAL && ch->state == AUDIO_CFIFO_START) {
            if (audio_syncts_latch_enable(node->hdl)) {
                struct alnk_hw_ch *hw = (struct alnk_hw_ch *)(hdl->hw_alink_ch[ch->attr.ch_idx]);
                spin_lock(&lock[hdl->alink_parm.module]);
                u32 swn = alink_get_shn(hw);
                int timeout = (1000000 / hdl->alink_parm.sample_rate) * (clk_get("sys") / 1000000) * 5 / 4;
                while (--timeout > 0 && ((swn == alink_get_shn(hw)) || ((swn != alink_get_shn(hw)) && (alink_get_shn(hw) % point_offset))));//等到消耗偶数个点后，才往后走，避免奇数个点的情况下更新给同步模块有1个点的偏差
                swn = alink_get_shn(hw);
                u32 time = audio_jiffies_usec();
                spin_unlock(&lock[hdl->alink_parm.module]);
                int buffered_frames = alink_get_dma_len(hdl->hw_alink) / point_offset - swn / point_offset - 1 - audio_cfifo_channel_unread_diff_samples(&ch->fifo);
                sound_pcm_update_frame_num_and_time(node->hdl, 0, time, buffered_frames);
            }
        }
        have_syncts = 1;
    }

    if (have_syncts) {
        struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)(hdl->hw_alink_ch[ch->attr.ch_idx]);

        u16 free_points = alink_get_shn(hw_channel_parm) / point_offset;
        int timeout = (1000000 / hdl->alink_parm.sample_rate) * (clk_get("sys") / 1000000);
        while (free_points == (alink_get_shn(hw_channel_parm) / point_offset) && (--timeout > 0));
    }

    return 0;
}


static void audio_iis_update_frame_begin(struct audio_iis_channel *ch, u32 frames)
{
    audio_iis_syncts_enter_update_frame(ch);
}

static void audio_iis_update_frame(struct audio_iis_channel *ch, u32 frames)
{
    audio_iis_channel_use_syncts_frames(ch, frames);
}

static void audio_iis_update_frame_end(struct audio_iis_channel *ch)
{
    audio_iis_syncts_exit_update_frame(ch);
}

static int audio_iis_channel_fifo_write(struct audio_iis_channel *ch, void *data, int len, u8 is_fixed_data)
{
    if (len == 0) {
        return 0;
    }
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return 0;
    }
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)(hdl->hw_alink_ch[ch->attr.ch_idx]);
    if (!hw_channel_parm) {
        return 0;
    }

    /* printf(">>>>>>>>>>write idx %d, cfifo %x, %x, fifo %x, hw_channel_parm %x\n",  */
    /* ch->attr.ch_idx, (int)&hdl->cfifo[ch->attr.ch_idx],(int)ch->fifo.fifo, (int)ch,(int)hw_channel_parm); */

    os_mutex_pend(&hdl->mutex[ch->attr.ch_idx], 0);
    u8 point_offset = 1;
    if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
        ch->fifo.bit_wide = DATA_BIT_WIDE_24BIT;
        point_offset = 2;
    }
    int w_len = 0;
    int frames = 0;
    int unread_samples = alink_fifo_buffered_frames(hdl, ch->attr.ch_idx);

    /* printf("ch_idx %d, read_update %d, %d, %d\n", ch->attr.ch_idx, hdl->unread_samples[ch->attr.ch_idx], unread_samples, hdl->unread_samples[ch->attr.ch_idx] - unread_samples); */
    audio_cfifo_read_update(&hdl->cfifo[ch->attr.ch_idx], hdl->unread_samples[ch->attr.ch_idx] - unread_samples);

    hdl->unread_samples[ch->attr.ch_idx] = unread_samples;

    if (is_fixed_data) {
        w_len = audio_cfifo_channel_write_fixed_data(&ch->fifo, (s16)data, len);
    } else {
        w_len = audio_cfifo_channel_write(&ch->fifo, data, len);
    }
    int fifo_frames = (w_len >> point_offset) / hdl->channel;
    int samples = audio_cfifo_get_unread_samples(&hdl->cfifo[ch->attr.ch_idx]) - hdl->unread_samples[ch->attr.ch_idx];

    ASSERT(samples >= 0, " samples : %d, %d\n", audio_cfifo_get_unread_samples(&hdl->cfifo[ch->attr.ch_idx]), hdl->unread_samples[ch->attr.ch_idx]);
    hdl->unread_samples[ch->attr.ch_idx] += samples;

    frames = samples;

    audio_iis_update_frame_begin(ch, fifo_frames);
    if (frames) {
        audio_iis_set_shn(hdl, ch->attr.ch_idx, frames * point_offset * 4);
    }
    audio_iis_update_frame(ch, fifo_frames);
    audio_iis_update_frame_end(ch);

    u32 swp = alink_get_swptr(hw_channel_parm);
    ASSERT(swp == audio_cfifo_get_write_offset(&hdl->cfifo[ch->attr.ch_idx])*point_offset, ", %d %d\n", swp, audio_cfifo_get_write_offset(&hdl->cfifo[ch->attr.ch_idx])*point_offset);

    int pns = ((hdl->protect_pns * 4) >> point_offset) / hdl->channel;
    if ((fifo_frames > (pns)) && !alink_get_ch_ie(hw_channel_parm)) {
        spin_lock(&lock[hdl->alink_parm.module]);
        alink_clr_ch_pnd(hw_channel_parm);
        alink_set_ch_ie(hw_channel_parm, 1);
        /* putchar('T'); */
        spin_unlock(&lock[hdl->alink_parm.module]);
    }

    os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);

    return w_len;
}

static int audio_iis_multi_channel_fifo_write_base(struct audio_iis_channel *ch, void *data, int len, u8 is_fixed_data, int *frames, int *fifo_frames, int unread_samples)
{
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return 0;
    }

    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)(hdl->hw_alink_ch[ch->attr.ch_idx]);
    if (!hw_channel_parm) {
        return 0;
    }
    int w_len = 0;
    u8 point_offset = 1;
    if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
        ch->fifo.bit_wide = DATA_BIT_WIDE_24BIT;
        point_offset = 2;
    }

    /* int unread_samples = alink_fifo_buffered_frames(hdl, ch->attr.ch_idx); */

    audio_cfifo_read_update(&hdl->cfifo[ch->attr.ch_idx], hdl->unread_samples[ch->attr.ch_idx] - unread_samples);

    hdl->unread_samples[ch->attr.ch_idx] = unread_samples;
    if (is_fixed_data) {
        w_len = audio_cfifo_channel_write_fixed_data(&ch->fifo, (s16)data, len);
    } else {
        w_len = audio_cfifo_channel_write(&ch->fifo, data, len);
    }

    *fifo_frames = (w_len >> point_offset) / hdl->channel;
    int samples = audio_cfifo_get_unread_samples(&hdl->cfifo[ch->attr.ch_idx]) - hdl->unread_samples[ch->attr.ch_idx];
    ASSERT(samples >= 0, " samples : %d, %d\n", audio_cfifo_get_unread_samples(&hdl->cfifo[ch->attr.ch_idx]), hdl->unread_samples[ch->attr.ch_idx]);
    hdl->unread_samples[ch->attr.ch_idx] += samples;
    *frames = samples;
    return w_len;
}

static int audio_iis_multi_channel_fifo_write(struct multi_ch *mch, int *data, int *len, u8 is_fixed_data, int *wlen)
{
    struct _iis_hdl *hdl = NULL;
    for (int i = 0 ; i < 4; i++) {
        struct audio_iis_channel *ch_h = mch->ch[i];
        if (ch_h) {
            hdl = (struct _iis_hdl *)ch_h->iis;
            if (!hdl) {
                continue;
            } else {
                break;
            }
        }
    }
    if (!hdl) {
        return 0;
    }

    u8 point_offset = 1;
    if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
        point_offset = 2;
    }
    int frames[4] = {0};
    int fifo_frames[4] = {0};
    int master_ch_idx = 0xff;
    int unread_samples[4] = {0};
    int unread_samples_max = 0;

    for (int i = 0; i < 4; i++) {
        if (mch->ch[i] && hdl->hw_alink_ch[i] && (hdl->ch_cfg[i].dir == ALINK_DIR_TX)) { //连续读，并使用先获取的ch的unread_samples
            unread_samples[i] = alink_fifo_buffered_frames(hdl, i);
            if (unread_samples_max < unread_samples[i]) {
                unread_samples_max = unread_samples[i];
            }
            if (master_ch_idx == 0xff) {
                master_ch_idx = i;
            }
        }
    }
    /* if (unread_samples[1] != unread_samples[2]){ */
    /* printf("check unread_samples %d, %d\n", unread_samples[1], unread_samples[2]); */
    /* } */

    for (int i = 0; i < 4; i++) { //连续填写dma
        if (mch->ch[i] && len[i] && hdl->hw_alink_ch[i] && (hdl->ch_cfg[i].dir == ALINK_DIR_TX)) { //连续写
            /* printf("============i %d, %x, %x, %x %x %x %x, %x \n",i, (int)data, (int)len, (int)frames, (int)&frames[i], (int)fifo_frames, (int)&fifo_frames[i], (int)wlen); */
            wlen[i] = audio_iis_multi_channel_fifo_write_base(mch->ch[i], (void *)data[i], len[i], is_fixed_data, &frames[i], &fifo_frames[i], unread_samples_max);

        }
    }
    if (master_ch_idx == 0xff) {
        printf("errr ch \n");
        return 0;
    }
    struct audio_iis_channel *master_ch = (void *)mch->ch[master_ch_idx];
    audio_iis_update_frame_begin(master_ch, fifo_frames[master_ch_idx]);
    int frame_len[4] = {0};
    for (int i = 0; i < 4; i++) {
        if (frames[i] && hdl->hw_alink_ch[i] && (hdl->ch_cfg[i].dir == ALINK_DIR_TX)) {//一起更新给多个通道
            frame_len[i] = frames[i] * point_offset * 4;
        }
    }
    audio_iis_multi_set_shn(mch, master_ch_idx, frame_len);
    audio_iis_update_frame(master_ch, fifo_frames[master_ch_idx]);
    audio_iis_update_frame_end(master_ch);

    for (int i = 0; i < 4; i++) {
        struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)(hdl->hw_alink_ch[i]);
        if (mch->ch[i] && hw_channel_parm && (hdl->ch_cfg[i].dir == ALINK_DIR_TX)) {
            u32 swp = alink_get_swptr(hw_channel_parm);
            ASSERT(swp == audio_cfifo_get_write_offset(&hdl->cfifo[i])*point_offset, ", ch[%d] %d %d, %d \n", i, swp, audio_cfifo_get_write_offset(&hdl->cfifo[i])*point_offset, point_offset);
            int pns = ((hdl->protect_pns * 4) >> point_offset) / hdl->channel;
            if ((fifo_frames[i] > (pns)) && !alink_get_ch_ie(hw_channel_parm)) {
                spin_lock(&lock[hdl->alink_parm.module]);
                alink_clr_ch_pnd(hw_channel_parm);
                alink_set_ch_ie(hw_channel_parm, 1);
                /* putchar('T'); */
                spin_unlock(&lock[hdl->alink_parm.module]);
            }
        }
    }
    return 1;
}
int audio_iis_channel_write(void *iis_ch, void *buf, int len)
{
    struct audio_iis_channel *ch = (struct audio_iis_channel *)iis_ch;
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return 0;
    }

    return audio_iis_channel_fifo_write(ch, buf, len, 0);
}
int audio_iis_multi_channel_write(struct multi_ch *mch, int *data, int *len, int *wlen)
{
    return audio_iis_multi_channel_fifo_write(mch, data, len, 0, wlen);
}

int audio_iis_fill_slience_frames(void *iis_ch,  int frames)
{
    struct audio_iis_channel *ch = (struct audio_iis_channel *)iis_ch;
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return 0;
    }

    int wlen = 0;
    u16 point_offset = 1;
    if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
        point_offset = 2;
    }
    wlen = audio_iis_channel_fifo_write(ch, (void *)0, frames * hdl->channel << point_offset, 1);

    return (wlen >> point_offset) / hdl->channel;
}

int audio_iis_multi_channel_fill_slience_frames(struct multi_ch *mch, int frames, int *wlen)
{
    struct _iis_hdl *hdl = NULL;
    for (int i = 0 ; i < 4; i++) {
        struct audio_iis_channel *ch_h = mch->ch[i];
        if (ch_h) {
            hdl = (struct _iis_hdl *)ch_h->iis;
            if (!hdl) {
                continue;
            } else {
                break;
            }
        }
    }
    if (!hdl) {
        return 0;
    }

    u16 point_offset = 1;
    if (hdl->alink_parm.bitwide == ALINK_LEN_24BIT) {
        point_offset = 2;
    }

    int tmp[4] = {0};//补固定值0
    int len[4] = {0};
    int slience_len = frames * hdl->channel << point_offset;
    if (!slience_len || !frames) {
        printf("miis set slience_frames error %d %d\n", slience_len, frames);
        return 0;
    }
    len[0] = mch->ch[0] ? slience_len : 0;
    len[1] = mch->ch[1] ? slience_len : 0;
    len[2] = mch->ch[2] ? slience_len : 0;
    len[3] = mch->ch[3] ? slience_len : 0;

    int ret = audio_iis_multi_channel_fifo_write(mch, tmp, len, 1, wlen);
    for (int i = 0; i < 4; i++) {
        wlen[i] = (wlen[i] >> point_offset) / hdl->channel;
    }



    return ret;
}

void audio_iis_channel_set_attr(void *iis_ch, void *attr)
{
    struct audio_iis_channel *ch = (struct audio_iis_channel *)iis_ch;
    if (ch) {
        if (!attr) {
            return;
        }
        memcpy(&ch->attr, attr, sizeof(struct audio_iis_channel_attr));
    }
}

int audio_iis_syncts_enter_update_frame(struct audio_iis_channel *ch)
{
    struct audio_iis_sync_node *node;
    if (!ch) {
        return 0;
    }
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return 0;
    }
    int ret = 0;
    list_for_each_entry(node, &hdl->sync_list[ch->attr.ch_idx], entry) {
        ret = sound_pcm_enter_update_frame(node->hdl);
    }
    return ret;
}

int audio_iis_syncts_exit_update_frame(struct audio_iis_channel *ch)
{
    struct audio_iis_sync_node *node;
    if (!ch) {
        return 0;
    }
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return 0;
    }

    int ret = 0;
    list_for_each_entry(node, &hdl->sync_list[ch->attr.ch_idx], entry) {
        ret = sound_pcm_exit_update_frame(node->hdl);
    }

    return ret;
}


void audio_iis_force_use_syncts_frames(void *iis_ch, int frames, u32 timestamp)
{
    struct audio_iis_sync_node *node;
    struct audio_iis_channel *ch = (struct audio_iis_channel *)iis_ch;
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return ;
    }

    os_mutex_pend(&hdl->mutex[ch->attr.ch_idx], 0);
    list_for_each_entry(node, &hdl->sync_list[ch->attr.ch_idx], entry) {
        if ((u32)node->ch != (u32)ch)  {
            continue;
        }
        if (!node->triggered) {
            if (audio_syncts_support_use_trigger_timestamp(node->hdl)) {
                u8 timestamp_enable = audio_syncts_get_trigger_timestamp(node->hdl, &node->timestamp);
                if (!timestamp_enable) {
                    continue;
                }
            }
            int time_diff = node->timestamp - timestamp;
            if (time_diff > 0) {
                os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);
                return;
            }
            if (node->hdl) {
                sound_pcm_update_frame_num(node->hdl, frames);
            }
        }
    }
    os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);
}



void audio_iis_syncts_trigger_with_timestamp(void *iis_ch, u32 timestamp)
{
    struct audio_iis_sync_node *node;
    struct audio_iis_channel *ch = (struct audio_iis_channel *)iis_ch;
    struct _iis_hdl *hdl = (struct _iis_hdl *)ch->iis;
    if (!hdl) {
        return ;
    }

    int time_diff = 0;

    os_mutex_pend(&hdl->mutex[ch->attr.ch_idx], 0);
    list_for_each_entry(node, &hdl->sync_list[ch->attr.ch_idx], entry) {
        if (node->triggered || ((u32)node->ch != (u32)ch)) {
            continue;
        }

        if (audio_syncts_support_use_trigger_timestamp(node->hdl)) {
            u8 timestamp_enable = audio_syncts_get_trigger_timestamp(node->hdl, &node->timestamp);
            if (!timestamp_enable) {
                continue;
            }
        }
        //int one_sample = PCM_SAMPLE_ONE_SECOND / dac->sample_rate;
        time_diff = timestamp - node->timestamp;
        /*printf("----- %d, %u, %u-----\n", time_diff, node->timestamp, timestamp);*/
        if (time_diff >= 0) {
            /*
            if (node->hdl) {
                sound_pcm_update_frame_num(node->hdl, TIME_TO_PCM_SAMPLES(time_diff, dac->sample_rate));
            }
            */
            sound_pcm_syncts_latch_trigger(node->hdl);
            /*printf("------------>>>> sound pcm syncts latch trigger : %d, %u, %u---\n", time_diff, node->timestamp, timestamp);*/
            node->triggered = 1;
        }
    }
    os_mutex_post(&hdl->mutex[ch->attr.ch_idx]);
}
/*
 *tx与rx共同开启时，需使用tx 的dmabuf长度做申请,
 *tx dma 长度：tx_dma_buf_time_ms
 *rx irq点数:rx_irq_points
 *return :使用该长度
 * */
u32 audio_iis_fix_dma_len(u32 module_idx, u32 tx_dma_buf_time_ms, u16 rx_irq_points, u8 bit_width, u8 ch_num)
{
    u32 dma_len = IIS_RX_DMA_LEN;
    u32 point_size = bit_width ? 4 : 2;
    if (audio_iis_check_hw_rx_and_tx_status(module_idx)) { //tx与rx共同使能，或者只有tx，buf长度使用tx的长度
        dma_len = tx_dma_buf_time_ms * ((AUDIO_DAC_MAX_SAMPLE_RATE + 999) / 1000) * ch_num  * point_size;
    }

    u32 dma_points = dma_len / point_size / ch_num;
    dma_points = (dma_points + rx_irq_points - 1) / rx_irq_points * rx_irq_points; //做对齐处理，避免rx循环buf尾包数据分两次输出的情况
    dma_len = dma_points * point_size * ch_num;
    return dma_len;
}
#endif


