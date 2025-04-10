#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_dut_control.data.bss")
#pragma data_seg(".audio_dut_control.data")
#pragma const_seg(".audio_dut_control.text.const")
#pragma code_seg(".audio_dut_control.text")
#endif
/*
 ****************************************************************************
 *							Audio DUT Control
 *	音频产测标准控制流程
 *
 ****************************************************************************
 */
#include "audio_dut_control.h"
#include "online_db_deal.h"
#include "system/includes.h"
#include "app_config.h"
#include "audio_cvp.h"
#include "asm/audio_adc.h"
#include "app_main.h"
#include "audio_config.h"
#include "a2dp_player.h"
#include "math.h"
#include "esco_recoder.h"
#include "adc_file.h"
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif

#if 1
#define audio_dut_log	printf
#define audio_dut_put_float put_float
#else
#define audio_dut_log(...)
#define audio_dut_put_float(...)
#endif

#define AUDIO_DUT_BIG_ENDDIAN 		1		//大端数据输出
#define AUDIO_DUT_LITTLE_ENDDIAN	2		//小端数据输出
#define AUDIO_DUT_OUTPUT_WAY			AUDIO_DUT_LITTLE_ENDDIAN

static int audio_dut_new_tx_packet(u8 cmd, u8 *data, int len);

#if TCFG_AUDIO_DUT_ENABLE

extern void put_float(double fv);

audio_dut_t *audio_dut_hdl = NULL;
static int audio_dut_app_online_parse(u8 *packet, u8 size, u8 *ext_data, u16 ext_size);
int audio_dut_new_rx_packet(u8 *dat, u8 len);
int audio_dut_spp_rx_packet(u8 *dat, u8 len);

void audio_dut_new_send_data_handler(void (*handler)(u8 ack, u8 *packet, u8 size))
{
    if (audio_dut_hdl) {
        audio_dut_hdl->ack_hdl = handler;
    }
}

#if TCFG_AUDIO_ANC_ENABLE
/*未经过算法处理，主要用于分析ANC MIC的频响*/
int cvp_dut_bypass_mic_ch_sel(u8 mic_num)
{
    if (audio_dut_hdl) {
        if (mic_num < AUDIO_ADC_MAX_NUM) {
            audio_dut_hdl->mic_ch_sel = BIT(mic_num);
        }
    }
    return 0;
}
#endif

u8 cvp_dut_mic_ch_get(void)
{
    if (audio_dut_hdl) {
        return audio_dut_hdl->mic_ch_sel;
    }
    return 0;
}

u8 cvp_dut_mode_get(void)
{
    if (audio_dut_hdl) {
        return audio_dut_hdl->mode;
    }
    return 0;
}

void cvp_dut_mode_set(u8 mode)
{
    if (audio_dut_hdl) {
        if (audio_dut_hdl->reset_flag) {
            return;
        }
        audio_dut_hdl->mode = mode;
        if (audio_dut_hdl->mode == CVP_DUT_MODE_ALGORITHM) {
            //还原设置
            audio_adc_file_set_mic_en_map(audio_dut_hdl->esco_cfg_mic_ch);
        }
    }
    audio_dut_log("%s mode %d\n", __func__, mode);
}

void audio_dut_init(void)
{
    audio_dut_log("audio_dut init");
    if (audio_dut_hdl == NULL) {
        audio_dut_hdl = zalloc(sizeof(audio_dut_t));
    }
    //开机默认设置成算法模式
    audio_dut_hdl->mode = CVP_DUT_MODE_ALGORITHM;
    //记录stream.bin 的通话MIC CH配置，用于还原
    audio_dut_hdl->esco_cfg_mic_ch = audio_adc_file_get_mic_en_map();
    app_online_db_register_handle(DB_PKT_TYPE_DMS, audio_dut_app_online_parse);
}

void audio_dut_unit(void)
{
    free(audio_dut_hdl);
    audio_dut_hdl = NULL;
}

static int audio_dut_app_online_parse(u8 *packet, u8 size, u8 *ext_data, u16 ext_size)
{
    if (audio_dut_hdl) {
        audio_dut_hdl->parse_seq  = ext_data[1];
        int ret = audio_dut_spp_rx_packet(packet, size);
        if (ret) {
            audio_dut_new_rx_packet(packet, size);
        }
    }
    return 0;
}

/***************************************************************************
  							CVP SPP产测 NEW STRUCT
***************************************************************************/

struct audio_dut_packet_t {
    u8 cmd;		//命令
    u16 len;	//参数长度
    u8 dat[0];	//参数
} __attribute__((packed));

struct audio_dut_head_t {
    u16 magic;	//标识符
    u16 crc;	//CRC, 从packet开始计算
    struct audio_dut_packet_t packet;
} __attribute__((packed));

/*
	CVP 命令处理函数
	param: struct audio_dut_head_t 类型的数据
	return 执行结果
*/
int audio_dut_new_event_deal(void *packet)
{
    struct audio_dut_packet_t *hdl = (struct audio_dut_packet_t *)packet;
    u16 len = hdl->len;
    int wlen = 0;
    int ret = AUDIO_DUT_ACK_SUCCESS;
    audio_dut_log("AUDIO_DUT CMD:0X%x,LEN:%d\n", hdl->cmd, len);
    put_buf((u8 *)packet, hdl->len + sizeof(struct audio_dut_packet_t));
    u8 last_mode, reset_flag;
    if (audio_dut_hdl) {
__again:
        reset_flag = 0;
        last_mode = audio_dut_hdl->mode;
        switch (hdl->cmd) {
#if TCFG_AUDIO_DUAL_MIC_ENABLE
//双麦ENC DUT 事件处理
        case CVP_DUT_DMS_MASTER_MIC:
            audio_dut_log("CMD:CVP_DUT_DMS_MASTER_MIC\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_aec_output_sel(DMS_OUTPUT_SEL_MASTER, 0);
            break;
        case CVP_DUT_DMS_SLAVE_MIC:
            audio_dut_log("CMD:CVP_DUT_DMS_SLAVE_MIC\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_aec_output_sel(DMS_OUTPUT_SEL_SLAVE, 0);
            break;
        case CVP_DUT_DMS_OPEN_ALGORITHM:
            audio_dut_log("CMD:CVP_DUT_DMS_OPEN_ALGORITHM\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_aec_output_sel(DMS_OUTPUT_SEL_DEFAULT, 1);
            break;
#elif TCFG_AUDIO_TRIPLE_MIC_ENABLE
        case CVP_DUT_3MIC_MASTER_MIC:
            audio_dut_log("CMD:CVP_DUT_3MIC_MASTER_MIC\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_aec_output_sel(1, 0);
            break;
        case CVP_DUT_3MIC_SLAVE_MIC:
            audio_dut_log("CMD:CVP_DUT_3MIC_SLAVE_MIC\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_aec_output_sel(2, 0);
            break;
        case CVP_DUT_3MIC_FB_MIC:
            audio_dut_log("CMD:CVP_DUT_3MIC_FB_MIC\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_aec_output_sel(3, 0);
            break;
        case CVP_DUT_3MIC_OPEN_ALGORITHM:
            audio_dut_log("CMD:CVP_DUT_3MIC_OPEN_ALGORITHM\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_aec_output_sel(0, 0);
            break;
#else
//单麦SMS/DNS DUT 事件处理
        case CVP_DUT_DMS_MASTER_MIC:
            audio_dut_log("CMD:SMS/CVP_DUT_DMS_MASTER_MIC\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_cvp_toggle_set(0);
            break;
        case CVP_DUT_DMS_OPEN_ALGORITHM:
            audio_dut_log("CMD:SMS/CVP_DUT_DMS_OPEN_ALGORITHM\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_cvp_toggle_set(1);
            break;
#endif/*TCFG_AUDIO_DUAL_MIC_ENABLE*/

#if TCFG_AUDIO_ANC_ENABLE
        case CVP_DUT_FF_MIC:	//TWS_FF_MIC测试 或者 头戴式FF_L_MIC测试
            audio_dut_log("CMD:CVP_DUT_FF_MIC\n");
            audio_dut_hdl->mode = CVP_DUT_MODE_BYPASS;
            if (TCFG_AUDIO_ANC_CH & ANC_L_CH) {
                cvp_dut_bypass_mic_ch_sel(TCFG_AUDIO_ANCL_FF_MIC);
            } else {
                cvp_dut_bypass_mic_ch_sel(TCFG_AUDIO_ANCR_FF_MIC);
            }
            reset_flag = 1;
            break;
        case CVP_DUT_FB_MIC:	//TWS_FB_MIC测试 或者 头戴式FB_L_MIC测试
            audio_dut_log("CMD:CVP_DUT_FB_MIC\n");
            audio_dut_hdl->mode = CVP_DUT_MODE_BYPASS;
            if (TCFG_AUDIO_ANC_CH & ANC_L_CH) {
                cvp_dut_bypass_mic_ch_sel(TCFG_AUDIO_ANCL_FB_MIC);
            } else {
                cvp_dut_bypass_mic_ch_sel(TCFG_AUDIO_ANCR_FB_MIC);
            }
            reset_flag = 1;
            break;
        case CVP_DUT_HEAD_PHONE_R_FF_MIC:	//头戴式FF_R_MIC测试
            audio_dut_log("CMD:CVP_DUT_HEAD_PHONE_R_FF_MIC\n");
            audio_dut_hdl->mode = CVP_DUT_MODE_BYPASS;
            if (TCFG_AUDIO_ANC_CH == (ANC_L_CH | ANC_R_CH)) {
                cvp_dut_bypass_mic_ch_sel(TCFG_AUDIO_ANCR_FF_MIC);
            }
            reset_flag = 1;
            break;
        case CVP_DUT_HEAD_PHONE_R_FB_MIC:	//头戴式FB_R_MIC测试
            audio_dut_log("CMD:CVP_DUT_HEAD_PHONE_R_FB_MIC\n");
            audio_dut_hdl->mode = CVP_DUT_MODE_BYPASS;
            if (TCFG_AUDIO_ANC_CH == (ANC_L_CH | ANC_R_CH)) {
                cvp_dut_bypass_mic_ch_sel(TCFG_AUDIO_ANCR_FB_MIC);
            }
            reset_flag = 1;
            break;
        case CVP_DUT_MODE_ALGORITHM_SET:				//CVP恢复正常模式
            audio_dut_log("CMD:CVP_DUT_RESUME\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            break;
#endif/*TCFG_AUDIO_ANC_ENABLE*/

//JL自研算法指令API
#if !TCFG_CVP_DEVELOP_ENABLE
        case CVP_DUT_NS_OPEN:
            audio_dut_log("CMD:CVP_DUT_NS_OPEN\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_cvp_ioctl(CVP_NS_SWITCH, 1, NULL);	//降噪关
            break;
        case CVP_DUT_NS_CLOSE:
            audio_dut_log("CMD:CVP_DUT_NS_CLOSE\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_cvp_ioctl(CVP_NS_SWITCH, 0, NULL); //降噪开
            break;

        /*通话算法模块开关*/
        case CVP_DUT_CVP_IOCTL:
            audio_dut_log("CMD:CVP_DUT_CVP_IOCTL\n");
            u8 ioctl_cmd = hdl->dat[0];
            u8 toggle = hdl->dat[1];
            audio_dut_log(" io ctl cmd %d, toggle %d", ioctl_cmd, toggle);
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_cvp_ioctl(ioctl_cmd, toggle, NULL);
            break;
#endif/*TCFG_CVP_DEVELOP_ENABLE*/

        case CVP_DUT_OPEN_ALGORITHM:
            audio_dut_log("CMD:CVP_DUT_OPEN_ALGORITHM\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_cvp_toggle_set(1);
            break;
        case CVP_DUT_CLOSE_ALGORITHM:
            audio_dut_log("CMD:CVP_DUT_CLOSE_ALGORITHM\n");
            cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
            audio_cvp_toggle_set(0);
            break;
        case CVP_DUT_POWEROFF:
            audio_dut_log("CMD:CVP_DUT_POWEROFF\n");
            int msg[3];
            msg[0] = (int)sys_enter_soft_poweroff;
            msg[1] = 1;
            msg[2] = POWEROFF_NORMAL;
            os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
            break;

            /*enc麦克风校准补偿*/
#if TCFG_AUDIO_MIC_ARRAY_TRIM_ENABLE
        case CVP_ENC_MIC_DIFF_CMP_SET://主MIC - 副MIC
            audio_dut_log("CMD:CVP_ENC_MIC_DIFF_CMP_SET\n");
            float diff;
            memcpy((u8 *)&diff, hdl->dat, len);
            audio_dut_log("diff:");
            audio_dut_put_float(diff);
            float dms_mic_diff = log10(app_var.enc_degradation) * 20;
            app_var.audio_mic_array_diff_cmp = pow(10, (diff + dms_mic_diff) / 20);
            wlen = syscfg_write(CFG_MIC_ARRAY_DIFF_CMP_VALUE, (u8 *)(&app_var.audio_mic_array_diff_cmp), sizeof(app_var.audio_mic_array_diff_cmp));
            if (wlen != sizeof(app_var.audio_mic_array_diff_cmp)) {
                printf("mic array diff cap vm write fail !!!");
                ret = AUDIO_DUT_ACK_VM_WRITE_ERR;
            }
            audio_dut_log("mic_array_diff_cmp:");
            audio_dut_put_float(app_var.audio_mic_array_diff_cmp);
            break;
        case CVP_ENC_MIC_DIFF_CMP_EN:
            audio_dut_log("CMD:CVP_ENC_MIC_DIFF_CMP_EN\n");
            u8 en;
            memcpy(&en, hdl->dat, len);
            app_var.audio_mic_array_trim_en = en;
            wlen = syscfg_write(CFG_MIC_ARRAY_TRIM_EN, (u8 *)(&app_var.audio_mic_array_trim_en), sizeof(app_var.audio_mic_array_trim_en));
            if (wlen != sizeof(app_var.audio_mic_array_trim_en)) {
                audio_dut_log("mic array trim en vm write fail !!!");
                ret = AUDIO_DUT_ACK_VM_WRITE_ERR;
            }
            audio_dut_log("mic array trim en: %d\n", app_var.audio_mic_array_trim_en);
            break;
        case CVP_ENC_MIC_DIFF_CMP_CLEAN:
            audio_dut_log("CMD:CVP_ENC_MIC_DIFF_CMP_CLEAN\n");
            app_var.audio_mic_array_diff_cmp = 1.0f;
            app_var.audio_mic_cmp.talk = 1.0f;
            app_var.audio_mic_cmp.ff = 1.0f;
            app_var.audio_mic_cmp.fb = 1.0f;

            wlen = syscfg_write(CFG_MIC_ARRAY_DIFF_CMP_VALUE, (u8 *)(&app_var.audio_mic_array_diff_cmp), sizeof(app_var.audio_mic_array_diff_cmp));
            if (wlen != sizeof(app_var.audio_mic_array_diff_cmp)) {
                audio_dut_log("mic array diff cap vm write fail !!!");
                ret = AUDIO_DUT_ACK_VM_WRITE_ERR;
            }

            wlen = syscfg_write(CFG_MIC_TARGET_DIFF_CMP, (u8 *)(&app_var.audio_mic_cmp), sizeof(audio_mic_cmp_t));
            if (wlen != sizeof(audio_mic_cmp_t)) {
                printf("mic array diff cap vm write fail !!!");
                ret = AUDIO_DUT_ACK_VM_WRITE_ERR;
            }
            audio_dut_log("CMD:CVP_ENC_MIC_DIFF_CMP_CLEAN\n");
            break;
        case CVP_ENC_TARGET_MIC_DIFF_SET:
            audio_dut_log("CMD:CVP_ENC_TARGET_MIC_DIFF_SET\n");
            float talk_diff, ff_diff, fb_diff;
            memcpy((u8 *)&talk_diff, hdl->dat, 4);
            memcpy((u8 *)&ff_diff, hdl->dat + 4, 4);
            memcpy((u8 *)&fb_diff, hdl->dat + 8, 4);
            audio_dut_log("talk_diff:");
            audio_dut_put_float(talk_diff);
            audio_dut_log("ff_diff:");
            audio_dut_put_float(ff_diff);
            audio_dut_log("fb_diff:");
            audio_dut_put_float(fb_diff);
            app_var.audio_mic_cmp.talk = pow(10, (talk_diff) / 20);
            app_var.audio_mic_cmp.ff = pow(10, (ff_diff) / 20);
            app_var.audio_mic_cmp.fb = pow(10, (fb_diff) / 20);
            wlen = syscfg_write(CFG_MIC_TARGET_DIFF_CMP, (u8 *)(&app_var.audio_mic_cmp), sizeof(audio_mic_cmp_t));
            if (wlen != sizeof(audio_mic_cmp_t)) {
                printf("mic array diff cap vm write fail !!!");
                ret = AUDIO_DUT_ACK_VM_WRITE_ERR;
            }
            break;

        case CVP_ENC_TARGET_MIC_DIFF_GET:
            audio_dut_log("CMD:CVP_ENC_TARGET_MIC_DIFF_GET\n");
            float mic_diff[3];
            mic_diff[0] = 20 * log10_float(app_var.audio_mic_cmp.talk);
            mic_diff[1] = 20 * log10_float(app_var.audio_mic_cmp.ff);
            mic_diff[2] = 20 * log10_float(app_var.audio_mic_cmp.fb);
            audio_dut_log("talk_diff:dB");
            audio_dut_put_float(mic_diff[0]);
            audio_dut_log("ff_diff:dB");
            audio_dut_put_float(mic_diff[1]);
            audio_dut_log("fb_diff:dB");
            audio_dut_put_float(mic_diff[2]);

            audio_dut_new_tx_packet(AUDIO_DUT_ACK_DATA, (u8 *)mic_diff, sizeof(mic_diff));
            ret = AUDIO_DUT_ACK_DATA;	//此条命令回复指定数据
            break;
#endif /*TCFG_AUDIO_MIC_ARRAY_TRIM_ENABLE*/

        case AUDIO_DUT_DEC_DUT_SET:
            audio_dut_log("AUDIO_DUT_DEC_DUT_SET: %d\n", hdl->dat[0]);
            audio_dut_hdl->dec_dut_enable = hdl->dat[0];
            a2dp_player_reset();
            break;
        default:
            audio_dut_log("CMD:UNKNOW CMD!!!\n");
            ret = AUDIO_DUT_ACK_ERR_UNKNOW;
        }
        if (last_mode != audio_dut_hdl->mode) {
            reset_flag = 1;		//模式被修改，则复位通话
        }
        if (reset_flag) {
            audio_dut_hdl->reset_flag = 1;
            esco_recoder_reset();
            audio_dut_hdl->reset_flag = 0;
            //由于算法相关的命令在esco_recoder_reset后会失效，因此需要重新跑一遍
            if (audio_dut_hdl->mode == CVP_DUT_MODE_ALGORITHM) {
                goto __again;
            }
        }
    }
    return ret;
}

//len = 参数数据长度(不包括cmd)
static int audio_dut_new_tx_packet(u8 cmd, u8 *data, int len)
{
    if (audio_dut_hdl) {
        if (audio_dut_hdl->ack_hdl) {
            audio_dut_hdl->ack_hdl(cmd, data, len);
            return 0;
        }

        int packet_len = sizeof(struct audio_dut_head_t) + len;
        struct audio_dut_head_t *head = (struct audio_dut_head_t *)malloc(packet_len);
        head->magic = AUDIO_DUT_NEW_SPP_MAGIC;
        head->packet.cmd = cmd;
        head->packet.len = len;
        head->crc = CRC16(&head->packet, packet_len - 4);
        if (len) {
            memcpy(head->packet.dat, data, len);
        }
        audio_dut_log("tx new_dat_packet %d\n", packet_len);
        //put_buf((u8 *)head, packet_len);
        app_online_db_send(DB_PKT_TYPE_DMS, (u8 *)head, packet_len);
        free(head);
    }
    return 0;
}

int audio_dut_new_rx_packet(u8 *dat, u8 len)
{
    struct audio_dut_head_t *head = (struct audio_dut_head_t *)dat;
    if (audio_dut_hdl) {
        audio_dut_log("rx new_dat_packet");
        //put_buf(dat, len);
        u16 crc = CRC16(&head->packet, len - 4);
        if (head->magic == AUDIO_DUT_NEW_SPP_MAGIC) {
            if (head->crc == crc || head->crc == 0x1) {
                int ret = audio_dut_new_event_deal(&head->packet);
                if (ret == AUDIO_DUT_ACK_DATA) {					//执行成功，需要返回指定数据
                    return 0;
                }
                audio_dut_new_tx_packet(ret, NULL, 0);			//反馈收到命令
                if (ret == AUDIO_DUT_ACK_SUCCESS) {
                    return 0;
                }
            } else {
                audio_dut_log("ERR head->crc %x != %x\n", head->crc, crc);
                audio_dut_new_tx_packet(AUDIO_DUT_ACK_CRC_ERR, NULL, 0);		//反馈收到命令
            }
        }
    }
    return 1;
}

/*
   DEC产测状态获取
	param: music_decide 1 需判断播歌状态； 0 不需判断
*/
u8 audio_dec_dut_en_get(u8 music_decide)
{
    if (!audio_dut_hdl) {
        return 0;
    }
    if (music_decide && !a2dp_player_runing()) {
        //当前非播歌状态
        return 0;
    }
    return audio_dut_hdl->dec_dut_enable;
}

#endif /*TCFG_AUDIO_DUT_ENABLE*/

