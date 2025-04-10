#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".private_iis.data.bss")
#pragma data_seg(".private_iis.data")
#pragma const_seg(".private_iis.text.const")
#pragma code_seg(".private_iis.text")
#endif
#include "private_iis.h"
#include "app_config.h"
#include "gpio.h"
#include "audio_config.h"
#include "audio_dai/audio_link.h"
#include "system/includes.h"
#include "app_main.h"


#if TCFG_IIS_NODE_ENABLE

#define PRIVATE_IIS_MSG_CHECK_EN	1		//使能格式检查, 该函数需根据具体消息自定义
#define PRIVATE_IIS_RX_EN 			1

struct __private_iis_data {
    u32 msg[CMD_LEN / 4];
    u8 get_msg_flag;
    u8 msg_len;
};
static struct __private_iis_data private_iis_data = {0};

struct private_iis_hdl {
    ALINK_PARM alink_parm;
    struct alnk_hw_ch ch_cfg[4];
    void *hw_alink_ch[4];
    /* s16 *store_pcm_buf; */
    /* cbuffer_t cbuf; */
    int sample_rate;
    void *hw_alink;
    void *alink_ch[4];	//有四个通道
    u32 cmd;		//要求接收到的cmd
};
static struct private_iis_hdl *private_iis = NULL;


/********************************* private iis Rx *********************************/
// 这个格式检查需要根据具体消息自己定义！！, 也可以不做检查
static u8 private_iis_check_msg(void)
{
#if PRIVATE_IIS_MSG_CHECK_EN
    if (private_iis_data.msg[0] == PRIVATE_IIS_CMD && private_iis_data.msg[1] != private_iis_data.msg[0]) {
        return 1;
    }
    return 0;	//格式检查不过
#else
    return 1;
#endif
}


static void private_iis_in_data_handler(void *priv, void *_data, int len)
{

    s16 *data = (s16 *)_data;
    /* for(int i=0; i<len/2; i++) { */
    /* if(data[i] != 0) { */
    /* printf(">> data[%d] = %d\n", i, data[i]);	 */
    /* } */
    /* } */

    u8 *temp = memchr(data, private_iis->cmd, len);
    if (temp && private_iis_data.get_msg_flag == 0) {
        memcpy(private_iis_data.msg, temp, CMD_LEN);
        if (private_iis_check_msg()) {
            private_iis_data.get_msg_flag = 1;
            private_iis_data.msg_len = CMD_LEN;
        }
    }
    /* alink_set_shn(&private_iis->alink_ch[PRIVATE_IIS_RX_CH], len / 4); */
}

/*
 * 打开iis接收msg，用来接收私有数据, IIS 作从机 (AD 端)
 */
void private_iis_rx_open(void)
{
    if (private_iis) {
        printf("private iis is running!\n");
        return;
    }
    struct private_iis_hdl *iis_in = NULL;
    iis_in = zalloc(sizeof(struct private_iis_hdl));
    if (!iis_in) {
        return;
    }

    private_iis = iis_in;

    iis_in->cmd = PRIVATE_IIS_CMD;

    iis_in->alink_parm.mode = ALINK_MD_IIS;
    iis_in->alink_parm.clk_mode = ALINK_CLK_FALL_UPDATE_RAISE_SAMPLE;
    iis_in->alink_parm.bitwide = ALINK_LEN_16BIT;
    iis_in->alink_parm.sclk_per_frame = ALINK_FRAME_32SCLK;
    iis_in->alink_parm.buf_mode = ALINK_BUF_CIRCLE;
    iis_in->alink_parm.dma_len = PRIVATE_IIS_DMA_LEN;
    iis_in->alink_parm.sample_rate = PRIVATE_IIS_RX_SR;

    iis_in->alink_parm.module = PRIVATE_IIS_RX_MODULE;
    iis_in->alink_parm.mclk_io = IO_PORTA_12;
    iis_in->alink_parm.sclk_io = IO_PORTA_13;
    iis_in->alink_parm.lrclk_io = IO_PORTA_14;
    iis_in->alink_parm.ch_cfg[PRIVATE_IIS_RX_CH].data_io = IO_PORTA_15;
    iis_in->alink_parm.role = ALINK_ROLE_SLAVE;		//从机

    iis_in->alink_parm.da2sync_ch = 0xff;
    //16 Bit
    iis_in->alink_parm.bitwide = ALINK_LEN_16BIT;
    iis_in->alink_parm.sclk_per_frame = ALINK_FRAME_32SCLK;
    /* iis_in->alink_parm.dma_len = irq_points * 16; */
    //24 Bit
    /* hdl->alink_parm.bitwide = ALINK_LEN_24BIT; */
    /* hdl->alink_parm.sclk_per_frame = ALINK_FRAME_64SCLK; */
    /* iis_in->alink_parm.dma_len = irq_points * 16 * 2; */


    /* for (int i = 0; i < 4; i++) { */
    iis_in->ch_cfg[0].dir = ALINK_DIR_RX;
    iis_in->ch_cfg[0].ch_ie = 1;
    iis_in->ch_cfg[0].isr_cb = private_iis_in_data_handler;
    iis_in->ch_cfg[0].private_data = NULL;
    iis_in->ch_cfg[0].data_io = IO_PORTA_15;
    /* } */

    iis_in->hw_alink = alink_init(&iis_in->alink_parm);
    u32 protect_pns = 1 * 1 * iis_in->alink_parm.sample_rate / 1000;
    alink_set_tx_pns(iis_in->hw_alink, protect_pns);
    iis_in->hw_alink_ch[0] = alink_channel_init_base(iis_in->hw_alink, &iis_in->ch_cfg[0], 0);

    if (iis_in->ch_cfg[0].dir == ALINK_DIR_TX) {
        u8 point_offset = 1;
        alink_set_shn(iis_in->hw_alink_ch[0], 1);//预设设置1个点，避免shn乱动的问题
    }

    alink_set_irq_handler(iis_in->hw_alink, &iis_in->ch_cfg[0], iis_in->ch_cfg[0].private_data, iis_in->ch_cfg[0].isr_cb);
    alink_start(iis_in->hw_alink);
    iis_in->sample_rate = PRIVATE_IIS_RX_SR;
}


u8 get_private_iis_get_msg_flag(void)
{
    return 	private_iis_data.get_msg_flag;
}

void show_private_msg(void)
{
    if (private_iis_data.get_msg_flag) {
        for (int i = 0; i < private_iis_data.msg_len / 4; i++) {
            y_printf(">>>msg[%d] = %d\n", i, private_iis_data.msg[i]);
        }
    } else {
        r_printf(">>>>> Show private msg failed!\n");
    }
}


void private_iis_rx_close(void)
{
    local_irq_disable();
    if (private_iis) {
        if (private_iis->hw_alink) {
            private_iis->ch_cfg[0].ch_idx = 0;
            private_iis->ch_cfg[0].private_data = NULL;
            alink_set_irq_handler(private_iis->hw_alink, &private_iis->ch_cfg[0], NULL, NULL);
            alink_channel_close(private_iis->hw_alink, private_iis->hw_alink_ch[0]);
            private_iis->hw_alink_ch[0] = NULL;
            int ret = alink_uninit_base(private_iis->hw_alink, NULL);
            if (ret) {
                private_iis->hw_alink = NULL;
            }
        }
        free(private_iis);
        private_iis = NULL;
    }
    local_irq_enable();
}


/********************************* private iis Tx *********************************/
static void private_iis_out_data_handler(void *priv, void *_data, int len)
{
    /* putchar('o'); */

    u32 *buf_u32 = (u32 *)_data;
    buf_u32[0] = private_iis->cmd;
    buf_u32[1] = PRIVATE_IIS_TX_LINEIN_SR;
    buf_u32[2] = PRIVATE_IIS_TX_LINEIN_CH_IDX;
    buf_u32[3] = PRIVATE_IIS_TX_LINEIN_GAIN;

    alink_set_shn(&private_iis->alink_ch[PRIVATE_IIS_TX_CH], len / 4);
}

/*
 * 打开iis发送 msg，用来发送私有数据, IIS 作主机 (Chip 端)
 */
void private_iis_tx_open(void)
{
    if (private_iis) {
        printf("private iis is running!\n");
        return;
    }
    struct private_iis_hdl *iis_out = NULL;
    iis_out = zalloc(sizeof(struct private_iis_hdl));
    if (!iis_out) {
        return;
    }

    private_iis = iis_out;

    iis_out->cmd = PRIVATE_IIS_CMD;

    iis_out->alink_parm.mode = ALINK_MD_IIS;
    iis_out->alink_parm.clk_mode = ALINK_CLK_FALL_UPDATE_RAISE_SAMPLE;
    iis_out->alink_parm.bitwide = ALINK_LEN_16BIT;
    iis_out->alink_parm.sclk_per_frame = ALINK_FRAME_32SCLK;
    iis_out->alink_parm.buf_mode = ALINK_BUF_CIRCLE;
    iis_out->alink_parm.dma_len = PRIVATE_IIS_DMA_LEN;
    iis_out->alink_parm.sample_rate = PRIVATE_IIS_TX_SR;

    iis_out->alink_parm.module = PRIVATE_IIS_TX_MODULE;
    iis_out->alink_parm.mclk_io = IO_PORTA_12;
    iis_out->alink_parm.sclk_io = IO_PORTA_13;
    iis_out->alink_parm.lrclk_io = IO_PORTA_14;
    iis_out->alink_parm.ch_cfg[PRIVATE_IIS_TX_CH].data_io = IO_PORTA_15;
    iis_out->alink_parm.role = ALINK_ROLE_MASTER;		//主机
    iis_out->sample_rate = PRIVATE_IIS_TX_SR;

    iis_out->alink_parm.da2sync_ch = 0xff;
    iis_out->alink_parm.bitwide = ALINK_LEN_16BIT;
    iis_out->alink_parm.sclk_per_frame = ALINK_FRAME_32SCLK;

    iis_out->ch_cfg[0].dir = ALINK_DIR_TX;
    iis_out->ch_cfg[0].ch_ie = 1;
    iis_out->ch_cfg[0].isr_cb = private_iis_out_data_handler;
    iis_out->ch_cfg[0].private_data = NULL;
    iis_out->ch_cfg[0].data_io = IO_PORTA_15;

    iis_out->hw_alink = alink_init(&iis_out->alink_parm);

    u32 protect_pns = 1 * 1 * iis_out->alink_parm.sample_rate / 1000;
    alink_set_tx_pns(iis_out->hw_alink, protect_pns);
    iis_out->hw_alink_ch[0] = alink_channel_init_base(iis_out->hw_alink, &iis_out->ch_cfg[0], 0);
    if (iis_out->ch_cfg[0].dir == ALINK_DIR_TX) {
        u8 point_offset = 1;
        alink_set_shn(iis_out->hw_alink_ch[0], 1);//预设设置1个点，避免shn乱动的问题
    }
    alink_set_irq_handler(iis_out->hw_alink, &iis_out->ch_cfg[0], iis_out->ch_cfg[0].private_data, iis_out->ch_cfg[0].isr_cb);
    alink_start(iis_out->hw_alink);

}


void private_iis_tx_close(void)
{
    local_irq_disable();
    if (private_iis) {
        if (private_iis->hw_alink) {
            /* alink_uninit(private_iis->hw_alink); */
            private_iis->ch_cfg[0].ch_idx = 0;
            private_iis->ch_cfg[0].private_data = NULL;
            alink_set_irq_handler(private_iis->hw_alink, &private_iis->ch_cfg[0], NULL, NULL);
            alink_channel_close(private_iis->hw_alink, private_iis->hw_alink_ch[0]);
            private_iis->hw_alink_ch[0] = NULL;
            int ret = alink_uninit_base(private_iis->hw_alink, NULL);
            if (ret) {
                private_iis->hw_alink = NULL;
            }
        }
        free(private_iis);
        private_iis = NULL;
    }
    local_irq_enable();
}


// ************* test ************
static void private_iis_test_timer_cb(void *p)
{
    static u32 tick = 0;
    tick++;
    printf(">>>>>>>>>>>>>>> tick : %d\n", tick);
    if (get_private_iis_get_msg_flag()) {
        y_printf("(test) >>>>>>> GET MSG!\n");
        show_private_msg();
        private_iis_rx_close();
    }
}
void private_iis_rx_test(void)
{
    y_printf("-------------------- Enter %s !", __func__);
    private_iis_rx_open();
    sys_timer_add(NULL, private_iis_test_timer_cb, 1000);
}

void private_iis_tx_test(void)
{
    y_printf("-------------------- Enter %s !", __func__);
    private_iis_tx_open();
    os_time_dly(100);
    private_iis_tx_close();
}

#endif


