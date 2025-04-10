#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_cvp_ais_3mic.data.bss")
#pragma data_seg(".audio_cvp_ais_3mic.data")
#pragma const_seg(".audio_cvp_ais_3mic.text.const")
#pragma code_seg(".audio_cvp_ais_3mic.text")
#endif
/*
 ***************************************************************************
 *							Audio CVP Develop
 *
 * Brief : Third-party algorithm integration 第三方算法集成
 * Notes : 1.可用内存
 *		   (1)free_ram_overlay:	静态复用内存(FREE_RAM_OVERLAY_SIZE)
 *		   (2)free_ram:	动态内存(mem_stats:physics memory size xxxx bytes)
 *		   (3)mux_rx_bulk:蓝牙复用空间(MUX_RX_BULK_MAX)
 *		   2.demo默认将输入数据copy到输出，相关处理只需在运算函数
 *		    audio_aec_run()实现即可
 *		   3.双mic ENC开发，需要在配置工具中通话第三方算法节点选择双麦算法
 *		   4.建议算法开发者使用宏定义将自己的代码模块包起来
 *		   6.开发阶段，默认使用芯片最高主频160MHz，可以通过修改AEC_CLK来修改
 			运行频率。
 ***************************************************************************
 */
#include "audio_cvp.h"
#include "aec_uart_debug.h"
#include "system/includes.h"
#include "media/includes.h"
#include "effects/eq_config.h"
#include "circular_buf.h"
#include "overlay_code.h"
#include "audio_config.h"
#include "debug.h"
#include "cvp_node.h"
#include "sdk_config.h"
#ifdef CONFIG_BOARD_AISPEECH_NR
#include "include/aispeech_enc.h"
#endif /*CONFIG_BOARD_AISPEECH_NR*/
#if TCFG_AUDIO_CVP_SYNC
#include "audio_cvp_sync.h"
#endif/*TCFG_AUDIO_CVP_SYNC*/
#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif/*TCFG_AUDIO_DUT_ENABLE*/

#if defined(TCFG_CVP_DEVELOP_ENABLE) && (TCFG_CVP_DEVELOP_ENABLE == CVP_CFG_AIS_3MIC)

#define AEC_CLK				(160 * 1000000L)	/*模块运行时钟(MaxFre:160MHz)*/
#define AEC_FRAME_POINTS	256					/*AEC处理帧长，跟mic采样长度关联*/
#define AEC_FRAME_SIZE		(AEC_FRAME_POINTS << 1)


/*数据输出开头丢掉的数据包数*/
#define CVP_OUT_DUMP_PACKET		15


//*********************************************************************************//
//                                预处理配置(Pre-process Config)               	   //
//*********************************************************************************//
/*预增益配置*/
#define CVP_PRE_GAIN_ENABLE				0	  //算法处理前预加数字增益放大使能
#define CVP_PRE_GAIN                    6.f   //算法前处理数字增益（Gain = 10^(dB_diff/20))

/*AEC输入buf复用mic_adc采样buf*/
#define MIC_BULK_MAX		3
struct mic_bulk {
    struct list_head entry;
    s16 *addr;
    u16 len;
    u16 used;
};

struct audio_aec_hdl {
    volatile u8 start;				//aec模块状态
    u8 mic_num;				//MIC的数量
    volatile u8 busy;
    volatile u8 ref_busy;
    volatile u8 ref_ok;				//aec模块状态
    u8 output_fade_in;		//aec输出淡入使能
    u8 output_fade_in_gain;	//aec输出淡入增益
    u8 output_sel;			//数据输出通道选择
    u16 dump_packet;		//前面如果有杂音，丢掉几包
    s16 *mic;				/*主mic数据地址*/
    s16 *mic_ref;			/*参考mic数据地址*/
    s16 *mic_ref_1;         /*参考mic数据地址*/
    s16 *pFar;			/*参考数据地址*/
    s16 *free_ram;			/*当前可用内存*/
    /* s16 spk_ref[AEC_FRAME_POINTS];	#<{(|扬声器参考数据|)}># */
    s16 *spk_ref;	/*扬声器参考数据*/
    s16 out[AEC_FRAME_POINTS];		/*运算输出地址*/
    OS_SEM sem;
    /*数据复用相关数据结构*/
    struct mic_bulk in_bulk[MIC_BULK_MAX];
    struct mic_bulk inref_bulk[MIC_BULK_MAX];
    struct mic_bulk inref_1_bulk[MIC_BULK_MAX];
    struct mic_bulk ref0_bulk[MIC_BULK_MAX];
    struct list_head in_head;
    struct list_head inref_head;
    struct list_head inref_1_head;
    struct list_head ref0_head;
    u8 output_way;
    u8 fm_tx_start;
    u8 ref_channel;
    u8 adc_ref_en;
    u16 ref_size;
    u32 ref_sr;
    int (*output_handle)(s16 *dat, u16 len);//输出回调函数
};
struct audio_aec_hdl *aec_hdl = NULL;
struct audio_aec_hdl  aec_hdl_mem AT(.aec_mem);

#define AEC_REF_CBUF_SIZE         (AEC_FRAME_POINTS * 6)
#define AEC_REF_CBUF_DOOR_SIZE    (AEC_REF_CBUF_SIZE / 2)
static cbuffer_t *aec_ref_cbuf = NULL;

extern void aec_code_movable_load(void);
extern void aec_code_movable_unload(void);

__attribute__((weak))u32 usb_mic_is_running()
{
    return 0;
}

void audio_aec_ref_start(u8 en)
{
    if (aec_hdl) {
        if (en != aec_hdl->fm_tx_start) {
            /* if (esco_adc_mic_en() == 0) { */
            aec_hdl->fm_tx_start = en;
            y_printf("fm_tx_start:%d\n", en);
            /* } */
        }
    }
}

/*通话上行同步输出回调*/
int audio_aec_sync_buffer_set(s16 *data, int len)
{
    return cvp_node_output_handle(data, len);
}

/*
*********************************************************************
*                  Audio AEC Output Handle
* Description: AEC模块数据输出回调
* Arguments  : data 输出数据地址
*			   len	输出数据长度
* Return	 : 数据输出消耗长度
* Note(s)    : None.
*********************************************************************
*/
static int audio_aec_output(s16 *data, u16 len)
{
    u16 wlen = 0;
    if (aec_hdl && aec_hdl->start) {
#if TCFG_AUDIO_CVP_SYNC
        audio_cvp_sync_run(data, len);
        return len;
#endif/*TCFG_AUDIO_CVP_SYNC*/

        if (aec_hdl->dump_packet) {
            aec_hdl->dump_packet--;
            memset(data, 0, len);
        } else  {
            if (aec_hdl->output_fade_in) {
                s32 tmp_data;
                //printf("fade:%d\n",aec_hdl->output_fade_in_gain);
                for (int i = 0; i < len / 2; i++) {
                    tmp_data = data[i];
                    data[i] = tmp_data * aec_hdl->output_fade_in_gain >> 7;
                }
                aec_hdl->output_fade_in_gain += 12;
                if (aec_hdl->output_fade_in_gain >= 128) {
                    aec_hdl->output_fade_in = 0;
                }
            }
        }

        return cvp_node_output_handle(data, len);
    }
    return wlen;
}

/*
 *跟踪系统内存使用情况:physics memory size xxxx bytes
 *正常的系统运行过程，应该至少有3k bytes的剩余空间给到系统调度开销
 */
static void sys_memory_trace(void)
{
    static int cnt = 0;
    if (cnt++ > 200) {
        cnt = 0;
        mem_stats();
    }
}

#include "Resample_api.h"
static RS_STUCT_API *sw_src_api = NULL;
static unsigned int *sw_src_buf = NULL;
#include "asm/audio_src.h"
static struct audio_src_handle *ref_hw_src = NULL;;
static u16 ref_hw_src_len = 0;

static int ref_hw_src_output(void *p, s16 *data, u16 len)
{
    struct audio_aec_hdl *aec = (struct audio_aec_hdl *)p;
    if (aec) {
        memcpy(aec->pFar + (ref_hw_src_len >> 1), data, len);
        ref_hw_src_len += len;
    }
    return len;
}
static int sw_src_init(u8 nch, u16 insample, u16 outsample)
{
    if (CONST_REF_SRC == 1) {
        if (insample != outsample) {
            sw_src_api = get_rs16_context();
            g_printf("sw_src_api:0x%x\n", sw_src_api);
            ASSERT(sw_src_api);
            u32 sw_src_need_buf = sw_src_api->need_buf();
            g_printf("sw_src_buf:%d\n", sw_src_need_buf);
            sw_src_buf = malloc(sw_src_need_buf);
            ASSERT(sw_src_buf);
            RS_PARA_STRUCT rs_para_obj;
            rs_para_obj.nch = nch;

            rs_para_obj.new_insample = insample;//48000;
            rs_para_obj.new_outsample = outsample;//16000;
            rs_para_obj.dataTypeobj.IndataBit = 0;
            rs_para_obj.dataTypeobj.OutdataBit = 0;
            rs_para_obj.dataTypeobj.IndataInc = (nch == 1) ? 1 : 2;
            rs_para_obj.dataTypeobj.OutdataInc = (nch == 1) ? 1 : 2;
            rs_para_obj.dataTypeobj.Qval = 15;
            printf("sw src,ch = %d, in = %d,out = %d\n", rs_para_obj.nch, rs_para_obj.new_insample, rs_para_obj.new_outsample);
            sw_src_api->open(sw_src_buf, &rs_para_obj);
        }
    } else if (CONST_REF_SRC == 2) {
        if (insample != outsample) {
            printf("hw src ch %d, in %d, out %d\n", nch, insample, outsample);
            ref_hw_src = zalloc(sizeof(struct audio_src_handle));
            if (ref_hw_src) {
                u8 channel = nch;
                audio_hw_src_open(ref_hw_src, channel, AUDIO_RESAMPLE_SYNC_OUTPUT);
                audio_hw_src_set_rate(ref_hw_src, insample, outsample);
                audio_src_set_output_handler(ref_hw_src, aec_hdl, (int (*)(void *, void *, int))ref_hw_src_output);
                printf("audio hw src open succ %x", (int)ref_hw_src);
            } else {
                printf("ref_hw_src malloc fail !!!\n");
            }
        }
    }

    return 0;
}

static int sw_src_run(s16 *indata, s16 *outdata, u16 len)
{
    int outlen = len;
    if (CONST_REF_SRC == 1) {
        if (sw_src_api && sw_src_buf) {
            outlen = sw_src_api->run(sw_src_buf, indata, len >> 1, outdata);
            /* ASSERT(outlen <= (sizeof(outdata) >> 1));  */
            outlen = outlen << 1;
            /* printf("%d\n",outlen); */
        }
    } else if (CONST_REF_SRC == 2) {
        if (ref_hw_src) {
            ref_hw_src_len = 0;;
            outlen = audio_src_resample_write(ref_hw_src, indata, len);
            outlen = ref_hw_src_len;
        }
    }
    return outlen;
}

static void sw_src_exit(void)
{
    if (CONST_REF_SRC == 1) {
        if (sw_src_buf) {
            free(sw_src_buf);
            sw_src_buf = NULL;
            sw_src_api = NULL;
        }
    } else if (CONST_REF_SRC == 2) {
        printf("[HW]ref_src_exit\n");
        if (ref_hw_src) {
            audio_hw_src_close(ref_hw_src);
            free(ref_hw_src);
            ref_hw_src = NULL;
        }
    }
}

/*
*********************************************************************
*                  Audio AEC RUN
* Description: AEC数据处理核心
* Arguments  : in 		主mic数据
*			   inref	参考mic数据(双mic降噪有用)
*			   ref		speaker参考数据
*			   out		数据输出
*			   points   数据点数，单位short
* Return	 : 数据运算输出长度
* Note(s)    : 在这里实现AEC_core
*********************************************************************
*/
static int audio_aec_run(s16 *in, s16 *inref, s16 *inref1, s16 *ref, s16 *out, u16 points)
{
    int out_size = 0;
    putchar('.');

#if CVP_PRE_GAIN_ENABLE
    GainProcess_16Bit(in, in, CVP_PRE_GAIN, 1, 1, 1, points);
    GainProcess_16Bit(inref, inref, CVP_PRE_GAIN, 1, 1, 1, points);
    //    GainProcess_16Bit_test(inref1, inref1, 0.f, 1, 1, 1, points);
#endif/*CVP_PRE_GAIN_ENABLE*/

#ifdef CONFIG_BOARD_AISPEECH_NR
    out_size = Aispeech_NR_run(in, inref, inref1, ref, out, points);
#else
    memcpy(out, in, (points << 1));
    //memcpy(out, inref, (points << 1));
    out_size = points << 1;
#endif /*CONFIG_BOARD_AISPEECH_NR*/

#if TCFG_AUDIO_DUT_ENABLE
    switch (aec_hdl->output_sel) {
    case CVP_3MIC_OUTPUT_SEL_MASTER:
        memcpy(out, in, (points << 1));
        break;
    case CVP_3MIC_OUTPUT_SEL_SLAVE:
        memcpy(out, inref, (points << 1));
        break;
    case CVP_3MIC_OUTPUT_SEL_FBMIC:
        memcpy(out, inref1, (points << 1));
        break;
    default:
        break;
    }
#endif/*TCFG_AUDIO_DUT_ENABLE*/

    sys_memory_trace();
    return out_size;
}

/*
*********************************************************************
*                  Audio AEC Task
* Description: AEC任务
* Arguments  : priv	私用参数
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
static void audio_aec_task(void *priv)
{
    printf("==Audio AEC Task==\n");
    struct mic_bulk *bulk = NULL;
    struct mic_bulk *bulk_ref = NULL;
    struct mic_bulk *bulk_ref_1 = NULL;
    struct mic_bulk *ref0_bulk = NULL;
    u8 pend = 1;
    while (1) {
        if (aec_hdl->output_way == 1) {
            if (!list_empty(&aec_hdl->in_head) && (aec_ref_cbuf->data_len >= aec_hdl->ref_size)) {
                cbuf_read(aec_ref_cbuf, aec_hdl->spk_ref, aec_hdl->ref_size);
            } else {
                os_sem_pend(&aec_hdl->sem, 0);
                continue;
            }
        } else {
            if (pend) {
                os_sem_pend(&aec_hdl->sem, 0);
            }
        }
        pend = 1;
        if (aec_hdl->start) {
            if ((!list_empty(&aec_hdl->in_head)) &&
                (!list_empty(&aec_hdl->ref0_head))) {
                aec_hdl->busy = 1;
                local_irq_disable();
                /*1.获取主mic数据*/
                bulk = list_first_entry(&aec_hdl->in_head, struct mic_bulk, entry);
                list_del(&bulk->entry);
                aec_hdl->mic = bulk->addr;
                if (aec_hdl->mic_num >= 2) {
                    /*获取参考mic数据*/
                    bulk_ref = list_first_entry(&aec_hdl->inref_head, struct mic_bulk, entry);
                    list_del(&bulk_ref->entry);
                    aec_hdl->mic_ref = bulk_ref->addr;
                }
                if (aec_hdl->mic_num >= 3) {
                    /*获取参考mic数据*/
                    bulk_ref_1 = list_first_entry(&aec_hdl->inref_1_head, struct mic_bulk, entry);
                    list_del(&bulk_ref_1->entry);
                    aec_hdl->mic_ref_1 = bulk_ref_1->addr;
                }
                /*获取参考数据*/
                aec_hdl->pFar = aec_hdl->spk_ref;
                if (aec_hdl->output_way == 0) {
                    ref0_bulk = list_first_entry(&aec_hdl->ref0_head, struct mic_bulk, entry);
                    list_del(&ref0_bulk->entry);
                    aec_hdl->pFar = (s16 *)ref0_bulk->addr;
                }
                local_irq_enable();

                int rlen = aec_hdl->ref_size;
                /*dac参考数据是立体声数据时，合成一个声道*/
                if (aec_hdl->ref_channel == 2) {
                    for (int i = 0; i < rlen / 4; i++) {
                        aec_hdl->pFar[i] = (short)(((int)aec_hdl->pFar[i * 2] + (int)aec_hdl->pFar[i * 2 + 1]) >> 1);
                    }
                    rlen = rlen >> 1;
                }

                /*参考数据变采样*/
                if (CONST_REF_SRC) {
                    rlen = sw_src_run(aec_hdl->pFar, aec_hdl->pFar, rlen);
                }
                /*4.算法处理*/
                int out_len = audio_aec_run(aec_hdl->mic, aec_hdl->mic_ref, aec_hdl->mic_ref_1, aec_hdl->pFar, aec_hdl->out, AEC_FRAME_POINTS);

                /*5.结果输出*/
                aec_hdl->output_handle(aec_hdl->out, out_len);

                /*6.数据导出*/
                if (CONST_AEC_EXPORT) {
                    aec_uart_fill(0, aec_hdl->mic, 512);		//主mic数据
                    aec_uart_fill(1, aec_hdl->mic_ref, 512);	//副mic数据
                    aec_uart_fill(2, aec_hdl->mic_ref_1, 512);  //副mic数据
                    aec_uart_fill(3, aec_hdl->pFar, 512);    //扬声器数据
                    aec_uart_fill(4, aec_hdl->out, out_len); //算法运算结果
                    aec_uart_write();
                }
                bulk->used = 0;
                if (aec_hdl->mic_num >= 2) {
                    bulk_ref->used = 0;
                }
                if (aec_hdl->mic_num >= 3) {
                    bulk_ref_1->used = 0;
                }
                if (aec_hdl->output_way == 0) {
                    ref0_bulk->used = 0;
                }
                aec_hdl->busy = 0;
                pend = 0;
            }
        }
    }
}

/*
*********************************************************************
*                  Audio AEC Open
* Description: 初始化AEC模块
* Arguments  : init_param 	sr 			采样率(8000/16000)
*              				ref_sr  	参考采样率
*			   enablebit	使能模块(AEC/NLP/AGC/ANS...)
*			   out_hdl		自定义回调函数，NULL则用默认的回调
* Return	 : 0 成功 其他 失败
* Note(s)    : 该接口是对audio_aec_init的扩展，支持自定义使能模块以及
*			   数据输出回调函数
*********************************************************************
*/
int audio_aec_open(struct audio_aec_init_param_t *init_param, s16 enablebit, int (*out_hdl)(s16 *data, u16 len))
{
    printf("audio_aec_init,sr = %d\n", init_param->sample_rate);
    s16 sample_rate = init_param->sample_rate;
    u32 ref_sr = init_param->ref_sr;
    mem_stats();
    if (aec_hdl) {
        printf("audio aec is already open!\n");
        return -1;
    }
    overlay_load_code(OVERLAY_AEC);
    aec_code_movable_load();

    /* aec_hdl = zalloc(sizeof(struct audio_aec_hdl)); */
    /* if (aec_hdl == NULL) { */
    /*     printf("aec_hdl malloc failed"); */
    /*     return -ENOMEM; */
    /* } */
    /* audio_overlay_load_code(OVERLAY_AEC); */

    memset(&aec_hdl_mem, 0, sizeof(aec_hdl_mem));
    aec_hdl = &aec_hdl_mem;
    printf("aec_hdl size:%ld\n", sizeof(struct audio_aec_hdl));
    /* clk_set("sys", AEC_CLK); */

    aec_hdl->mic_num = init_param->mic_num;

    aec_hdl->dump_packet = CVP_OUT_DUMP_PACKET;
    aec_hdl->output_fade_in = 1;
    aec_hdl->output_fade_in_gain = 0;

#if TCFG_AUDIO_DUT_ENABLE
    aec_hdl->output_sel = CVP_3MIC_OUTPUT_SEL_DEFAULT;
#endif/*TCFG_AUDIO_DUT_ENABLE*/

#if TCFG_AUDIO_CVP_SYNC
    audio_cvp_sync_open(init_param->sample_rate);
#endif/*TCFG_AUDIO_CVP_SYNC*/


    aec_hdl->output_way = 0;
    aec_hdl->fm_tx_start = 0;
    aec_hdl->ref_channel = 1;
    aec_hdl->adc_ref_en = 0;

    if (ref_sr) {
        aec_hdl->ref_sr  = ref_sr;
    } else {
        aec_hdl->ref_sr  = usb_mic_is_running();
    }
    if (aec_hdl->ref_sr == 0) {
        if (TCFG_ESCO_DL_CVSD_SR_USE_16K && (sample_rate == 8000)) {
            aec_hdl->ref_sr = 16000;	//CVSD 下行为16K
        } else {
            aec_hdl->ref_sr = sample_rate;
        }
    }

    if (out_hdl) {
        aec_hdl->output_handle = out_hdl;
    } else {
        aec_hdl->output_handle = audio_aec_output;
    }

    /*不支持参考数据和输入数据不成倍数关系的运算*/
    if (aec_hdl->ref_sr % sample_rate) {
        printf("AEC error: ref_sr:%d,in_sr:%d\n", aec_hdl->ref_sr, sample_rate);
        return -ENOEXEC;
    }

    if (aec_hdl->ref_channel != 2) {
        aec_hdl->ref_channel = 1;
    }

    INIT_LIST_HEAD(&aec_hdl->in_head);
    INIT_LIST_HEAD(&aec_hdl->inref_head);
    INIT_LIST_HEAD(&aec_hdl->inref_1_head);
    if (aec_hdl->output_way == 0) {
        INIT_LIST_HEAD(&aec_hdl->ref0_head);
    }
    /*adc回采参考数据时，复用adc的buf*/
    aec_hdl->ref_size = AEC_FRAME_POINTS * sizeof(short) * (aec_hdl->ref_sr / sample_rate) * aec_hdl->ref_channel;
    printf("aec ref_size:%d\n", aec_hdl->ref_size);

    if (aec_hdl->adc_ref_en == 0) {
        if (aec_hdl->output_way == 0) {
            aec_hdl->spk_ref = zalloc(aec_hdl->ref_size * MIC_BULK_MAX);
        } else {
            aec_hdl->spk_ref = zalloc(aec_hdl->ref_size);
        }
    }
    if (aec_hdl->output_way == 1) {
        u16 aec_ref_buf_size = AEC_REF_CBUF_SIZE * (aec_hdl->ref_sr / sample_rate) * aec_hdl->ref_channel;
        u8 *aec_ref_buf = zalloc(aec_ref_buf_size + sizeof(cbuffer_t));
        aec_ref_cbuf = (cbuffer_t *)aec_ref_buf;
        printf("aec ref cbuf:%d\n", aec_ref_buf_size);
        if (aec_ref_cbuf) {
            cbuf_init(aec_ref_cbuf, aec_ref_buf + sizeof(cbuffer_t), aec_ref_buf_size);
            /* cbuf_write(aec_ref_cbuf, aec->output, 128); */
        } else {
            printf("aec cbuf zalloc failed\n");
        }
    }

#ifdef CONFIG_BOARD_AISPEECH_NR
    u32 memPoolLen = Aispeech_NR_getmemsize(sample_rate);
    printf("ais memPoolLen=%d\r\n", memPoolLen);
    /* printf("--------------version %s ---\r\n", (char *)SEVC_API_Version()); */

    aec_hdl->free_ram = malloc(memPoolLen);

    char *pcMemPool = (char *)aec_hdl->free_ram;
    if (NULL == pcMemPool) {
        printf("ais calloc fail \r\n");
        return -1;
    }
    printf("ais pcMemPool malloc ok\r\n");
    Aispeech_NR_init(pcMemPool, memPoolLen, sample_rate);
#endif /*CONFIG_BOARD_AISPEECH_NR*/

    os_sem_create(&aec_hdl->sem, 0);
    task_create(audio_aec_task, NULL, "aec");

    if (CONST_AEC_EXPORT) {
        aec_uart_open(5, 512);
    }
    audio_dac_read_reset();
    if (CONST_REF_SRC) {
        sw_src_init(1, aec_hdl->ref_sr, sample_rate);
    }
    aec_hdl->start = 1;

    mem_stats();
#if	0
    aec_hdl->free_ram = malloc(1024 * 63);
    mem_stats();
#endif
    printf("audio_aec_open succ\n");
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Init
* Description: 初始化AEC模块
* Arguments  : init_param 初始化参数句柄
* Return	 : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_aec_init(struct audio_aec_init_param_t *init_param)
{
    return audio_aec_open(init_param, -1, NULL);
}


/*
*********************************************************************
*                  Audio AEC Close
* Description: 关闭AEC模块
* Arguments  : None.
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_aec_close(void)
{
    printf("audio_aec_close:%x", (u32)aec_hdl);
    if (aec_hdl) {
        aec_hdl->start = 0;

        while (aec_hdl->busy) {
            os_time_dly(2);
        }
        task_kill("aec");

#if TCFG_AUDIO_CVP_SYNC
        //在AEC关闭之后再关，否则还会跑cvp_sync_run,导致越界
        audio_cvp_sync_close();
#endif/*TCFG_AUDIO_CVP_SYNC*/

        if (CONST_AEC_EXPORT) {
            aec_uart_close();
        }

#ifdef CONFIG_BOARD_AISPEECH_NR
        Aispeech_NR_deinit();
        if (aec_hdl->free_ram) {
            free(aec_hdl->free_ram);
            aec_hdl->free_ram = NULL;
        }
#endif /*CONFIG_BOARD_AISPEECH_NR*/

        if (CONST_REF_SRC) {
            sw_src_exit();
        }
        if (aec_ref_cbuf) {
            free(aec_ref_cbuf);
            aec_ref_cbuf = NULL;
        }

        if (aec_hdl->spk_ref) {
            free(aec_hdl->spk_ref);
            aec_hdl->spk_ref = NULL;
        }

        local_irq_disable();
        /* free(aec_hdl); */
        aec_hdl = NULL;
        local_irq_enable();
        aec_code_movable_unload();
        printf("audio_aec_close succ\n");
    }
}

/*
*********************************************************************
*                  Audio AEC Status
* Description: AEC模块当前状态
* Arguments  : None.
* Return	 : 0 关闭 其他 打开
* Note(s)    : None.
*********************************************************************
*/
u8 audio_aec_status(void)
{
    if (aec_hdl) {
        return aec_hdl->start;
    }
    return 0;
}

int cvp_develop_read_ref_data(void)
{
    u16 rlen = -1;
    u8 i;
    int err = 0;
    if (aec_hdl == NULL) {
        return 0;
    } else if (aec_hdl->start == 0) {
        return 0;
    }
    if ((aec_hdl->output_way) || (aec_hdl->adc_ref_en)) {
        return 0;
    }
    for (i = 0; i < MIC_BULK_MAX; i++) {
        if (aec_hdl->ref0_bulk[i].used == 0) {
            rlen = audio_dac_read(60,
                                  aec_hdl->spk_ref + ((aec_hdl->ref_size >> 1) * i),
                                  aec_hdl->ref_size / aec_hdl->ref_channel,
                                  aec_hdl->ref_channel);
            break;
        }
    }
    if (i < MIC_BULK_MAX) {
        //AEC_D("bulk:%d-%d\n",i,len);
        aec_hdl->ref0_bulk[i].addr = (aec_hdl->spk_ref + ((aec_hdl->ref_size >> 1) * i));
        aec_hdl->ref0_bulk[i].used = 0x55;
        aec_hdl->ref0_bulk[i].len = rlen;
        list_add_tail(&aec_hdl->ref0_bulk[i].entry, &aec_hdl->ref0_head);
    } else {
        printf(">>>far_in_full\n");
        /*align reset*/
        struct mic_bulk *bulk;
        list_for_each_entry(bulk, &aec_hdl->ref0_head, entry) {
            bulk->used = 0;
            __list_del_entry(&bulk->entry);
        }
        return -1;
    }
    return rlen;
}

/*
*********************************************************************
*                  Audio AEC Input
* Description: AEC源数据输入
* Arguments  : buf	输入源数据地址
*			   len	输入源数据长度
* Return	 : None.
* Note(s)    : 输入一帧数据，唤醒一次运行任务处理数据，默认帧长256点
*********************************************************************
*/
void audio_aec_inbuf(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {

        if (aec_hdl->output_way == 1) {
            if ((aec_hdl->ref_ok == 0) || (aec_hdl->fm_tx_start == 0)) {
                if (aec_hdl->ref_ok && !aec_hdl->fm_tx_start) {
                    printf("[aec]fm_tx_start == 0\n");
                }
                return;
            }
        }

        int i = 0;
        for (i = 0; i < MIC_BULK_MAX; i++) {
            if (aec_hdl->in_bulk[i].used == 0) {
                break;
            }
        }

        if (i < MIC_BULK_MAX) {
            aec_hdl->in_bulk[i].addr = buf;
            aec_hdl->in_bulk[i].used = 0x55;
            aec_hdl->in_bulk[i].len = len;
            list_add_tail(&aec_hdl->in_bulk[i].entry, &aec_hdl->in_head);
        } else {
            printf(">>>aec_in_full\n");
            /*align reset*/
            if (aec_hdl->output_way == 0) {
                audio_dac_read_reset();
            }
            struct mic_bulk *bulk;
            list_for_each_entry(bulk, &aec_hdl->in_head, entry) {
                bulk->used = 0;
                __list_del_entry(&bulk->entry);
            }
            return;
        }
        os_sem_set(&aec_hdl->sem, 0);
        os_sem_post(&aec_hdl->sem);
    }
}

/*
*********************************************************************
*                  Audio AEC Input Reference
* Description: AEC源参考数据输入
* Arguments  : buf	输入源数据地址
*			   len	输入源数据长度
* Return	 : None.
* Note(s)    : 双mic ENC的参考mic数据输入,单mic的无须调用该接口
*********************************************************************
*/
void audio_aec_inbuf_ref(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
        if (aec_hdl->output_way == 1) {
            if ((aec_hdl->ref_ok == 0) || (aec_hdl->fm_tx_start == 0)) {
                if (aec_hdl->ref_ok && !aec_hdl->fm_tx_start) {
                    printf("[aec]fm_tx_start == 0\n");
                }
                return;
            }
        }

        int i = 0;
        for (i = 0; i < MIC_BULK_MAX; i++) {
            if (aec_hdl->inref_bulk[i].used == 0) {
                break;
            }
        }
        if (i < MIC_BULK_MAX) {
            aec_hdl->inref_bulk[i].addr = buf;
            aec_hdl->inref_bulk[i].used = 0x55;
            aec_hdl->inref_bulk[i].len = len;
            list_add_tail(&aec_hdl->inref_bulk[i].entry, &aec_hdl->inref_head);
        } else {
            printf(">>>aec_inref_full\n");
            /*align reset*/
            if (aec_hdl->output_way == 0) {
                audio_dac_read_reset();
            }
            struct mic_bulk *bulk;
            list_for_each_entry(bulk, &aec_hdl->inref_head, entry) {
                bulk->used = 0;
                __list_del_entry(&bulk->entry);
            }
            return;
        }
    }
}

/*
*********************************************************************
*                  Audio AEC Input Reference
* Description: AEC源参考数据输入
* Arguments  : buf	输入源数据地址
*			   len	输入源数据长度
* Return	 : None.
* Note(s)    : 双mic ENC的参考mic数据输入,单mic的无须调用该接口
*********************************************************************
*/
void audio_aec_inbuf_ref_1(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
        if (aec_hdl->output_way == 1) {
            if ((aec_hdl->ref_ok == 0) || (aec_hdl->fm_tx_start == 0)) {
                if (aec_hdl->ref_ok && !aec_hdl->fm_tx_start) {
                    printf("[aec]fm_tx_start == 0\n");
                }
                return;
            }
        }

        int i = 0;
        for (i = 0; i < MIC_BULK_MAX; i++) {
            if (aec_hdl->inref_1_bulk[i].used == 0) {
                break;
            }
        }
        if (i < MIC_BULK_MAX) {
            aec_hdl->inref_1_bulk[i].addr = buf;
            aec_hdl->inref_1_bulk[i].used = 0x55;
            aec_hdl->inref_1_bulk[i].len = len;
            list_add_tail(&aec_hdl->inref_1_bulk[i].entry, &aec_hdl->inref_1_head);
        } else {
            printf(">>>aec_inref_1_full\n");
            /*align reset*/
            struct mic_bulk *bulk;
            list_for_each_entry(bulk, &aec_hdl->inref_1_head, entry) {
                bulk->used = 0;
                __list_del_entry(&bulk->entry);
            }
            return;
        }
    }
}

/*
*********************************************************************
*                  Audio AEC Reference
* Description: AEC模块参考数据输入
* Arguments  : buf	输入参考数据地址
*			   len	输入参考数据长度
* Return	 : None.
* Note(s)    : 声卡设备是DAC，默认不用外部提供参考数据
*********************************************************************
*/
void audio_aec_far_refbuf(s16 *buf, u16 len);
void audio_aec_refbuf(s16 *data0, s16 *data1, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
        if ((aec_hdl != NULL) && (aec_hdl->adc_ref_en == 1) && (aec_hdl->output_way == 0)) {
            /*使用adc回采时*/
            if (data0) {
                audio_aec_far_refbuf(data0, len);
            }
            return;
        }

        /*使用外部参考数据时*/
        if (aec_hdl->output_way != 1) {
            return;
        }
        aec_hdl->ref_busy = 1;

        aec_hdl->ref_ok = 1;
        if (0 == cbuf_write(aec_ref_cbuf, data0, len)) {
            printf("aec wfail:%d\n", len);
        }
#if 1
        static u16 aec_ref_max = 0;
        if (aec_ref_max < aec_ref_cbuf->data_len) {
            aec_ref_max = aec_ref_cbuf->data_len;
            printf("aec_ref_max:%d\n", aec_ref_max);
        }
#endif

        if (!list_empty(&aec_hdl->in_head) && (aec_ref_cbuf->data_len >= aec_hdl->ref_size)) {
            os_sem_set(&aec_hdl->sem, 0);
            os_sem_post(&aec_hdl->sem);
        }
        aec_hdl->ref_busy = 0;
    }
}

/*adc ref0*/
void audio_aec_far_refbuf(s16 *buf, u16 len)
{
    u16 wlen;
    u8 i;
    int err = 0;

    if ((aec_hdl == NULL) || (aec_hdl->adc_ref_en == 0) || (aec_hdl->output_way == 1)) {
        return;
    }
#if 0
    /*AEC暂时挂起，有其他功能要做，比如播题示音，这个时候不用做AEC*/
    if ((aec->state & AEC_STATE_SUSPEND) || (aec->state & AEC_STATE_RESET)) {
        return err;
    }
#endif


    //AEC_D("A:%d\n", len);
    for (i = 0; i < MIC_BULK_MAX; i++) {
        if (aec_hdl->ref0_bulk[i].used == 0) {
            break;
        }
    }
    if (i < MIC_BULK_MAX) {
        //AEC_D("bulk:%d-%d\n",i,len);
        aec_hdl->ref0_bulk[i].addr = buf;
        aec_hdl->ref0_bulk[i].used = 0x55;
        aec_hdl->ref0_bulk[i].len = len;
        list_add_tail(&aec_hdl->ref0_bulk[i].entry, &aec_hdl->ref0_head);
    } else {
        printf(">>>aec_in_ref0_full\n");
        /*align reset*/
        struct mic_bulk *bulk;
        list_for_each_entry(bulk, &aec_hdl->ref0_head, entry) {
            bulk->used = 0;
            __list_del_entry(&bulk->entry);
        }
        return;
    }
}

/*
*********************************************************************
*                  Audio AEC Output Sel
* Description: AEC输出数据选择
* Arguments  : sel 选择输出/算法输出/talk/ff/fb原始数据
*			   agc NULL
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_aec_output_sel(u8 sel, u8 agc)
{
    if (aec_hdl) {
        aec_hdl->output_sel = sel;
    }
}

/*
*********************************************************************
*                  Audio CVP Toggle Set
* Description: CVP模块算法开关使能
* Arguments  : toggle 0 关闭算法 1 打开算法
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
int audio_cvp_toggle_set(u8 toggle)
{
    if (aec_hdl) {
        aec_hdl->output_sel = (toggle) ? 0 : 1;
        return 0;
    }
    return 1;
}

/*
*********************************************************************
*                  			Audio CVP IOCTL
* Description: CVP功能配置
* Arguments  : cmd 		操作命令
*		       value 	操作数
*		       priv 	操作内存地址
* Return	 : 0 成功 其他 失败
* Note(s)    : (1)比如动态开关降噪NS模块:
*********************************************************************
*/
int audio_cvp_ioctl(int cmd, int value, void *priv)
{
    return -1;
}

#endif /*TCFG_CVP_DEVELOP_ENABLE*/
