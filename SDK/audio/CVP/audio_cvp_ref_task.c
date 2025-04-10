#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_cvp_ref_task.data.bss")
#pragma data_seg(".audio_cvp_ref_task.data")
#pragma const_seg(".audio_cvp_ref_task.text.const")
#pragma code_seg(".audio_cvp_ref_task.text")
#endif
/*
 ****************************************************************
 *							AUDIO CVP REF SRC
 * File  : audio_cvp_ref_src.c
 * By    :
 * Notes : CVP回音消除的外部参考数变采样
 *
 ****************************************************************
 */
#include "audio_cvp.h"
#include "system/includes.h"
#include "media/includes.h"
#include "circular_buf.h"
#include "Resample_api.h"
#include "media/audio_general.h"
#include "effects/convert_data.h"
#include "jlstream.h"
#include "esco_player.h"
#include "pc_spk_player.h"

#define CVP_REF_SRC_TASK_NAME "CVP_RefTask"

#define ALINK_CH_IDX    0   //iis输出使用的通道

#define CVP_REF_SRC_FRAME_SIZE  (480 * 2)//512
typedef struct {
    volatile u8 state;
    volatile u8 busy;
    RS_STUCT_API *sw_src_api;
    u32 *sw_src_buf;
    u16 input_rate;
    u16 output_rate;
    u8 channel;
    s16 *ref_tmp_buf;
    cbuffer_t cbuf;
    u8 ref_buf[2048 * 10];//和输入的数据大小有关系
    u8 align_flag;
    u8 bit_width;
    u8 data_multiple;//输入输出数据倍数
} cvp_ref_src_t;
static cvp_ref_src_t *cvp_ref_src = NULL;

extern void iis_node_write_callback_add(const char *name, u8 is_after_write_over, u8 scene, void (*cb)(void *, int));
extern void iis_node_write_callback_del(const char *name);

#define IIS_READ_MAGIC  0x55AA
static int iis_read_pos = IIS_READ_MAGIC;
/*获取iis dma里面还有多少数据没有播放*/
static u32 get_alink_data_len(u8 ch_idx)
{
    u32 len = 0;
    switch (ch_idx) {
    case 0:
        len = (JL_ALNK0->LEN - JL_ALNK0->SHN0) * 4;
        break;
    case 1:
        len = (JL_ALNK0->LEN - JL_ALNK0->SHN1) * 4;
        break;
    case 2:
        len = (JL_ALNK0->LEN - JL_ALNK0->SHN2) * 4;
        break;
    case 3:
        len = (JL_ALNK0->LEN - JL_ALNK0->SHN3) * 4;
        break;
    }

    return len;
}

/*iis硬件写指针位置*/
static u32 get_alink_hwptr(u8 ch_idx)
{
    u32 val = 0;
    switch (ch_idx) {
    case 0:
        val = JL_ALNK0->HWPTR0;
        break;
    case 1:
        val = JL_ALNK0->HWPTR1;
        break;
    case 2:
        val = JL_ALNK0->HWPTR2;
        break;
    case 3:
        val = JL_ALNK0->HWPTR3;
        break;
    }

    return val;

}

/*重新对齐iis延时*/
int audio_cvp_ref_data_align_reset(void)
{
    iis_read_pos = IIS_READ_MAGIC;
    cvp_ref_src_t *hdl = cvp_ref_src;
    if (hdl) {
        hdl->align_flag = 0;
        cbuf_clear(&hdl->cbuf);

    }
    return 0;
}

/*对齐iis的数据延时，在第一次mic数据来时调用对齐*/
void audio_cvp_ref_data_align()
{
    cvp_ref_src_t *hdl = cvp_ref_src;
    /* !iis_read_pos ： iis有数据了才么开始对齐
       !hdl->align_flag ： 表示还没有做对齐*/
    if (hdl && !hdl->align_flag && !iis_read_pos) {
        int iis_data_len = get_alink_data_len(ALINK_CH_IDX);
        int cbuf_total_len = cbuf_get_data_len(&hdl->cbuf);
        printf("hdl->cbuf len %d ", cbuf_total_len);
        int need_read_len = cbuf_total_len - iis_data_len;
        printf("adc_iis_data_align: %d %d", iis_data_len, need_read_len);
        if (need_read_len >= 0) {
            cbuf_read_updata(&hdl->cbuf, need_read_len);
        } else {
            cbuf_write_updata(&hdl->cbuf, need_read_len * (-1));
        }
        hdl->align_flag = 1;
        //有参考数据进来，并且对齐后，取消忽略参考数据
        audio_cvp_ioctl(CVP_OUTWAY_REF_IGNORE, 0, NULL);
    }
}


static void audio_cvp_ref_src_run(s16 *data, int len)
{
    cvp_ref_src_t *hdl = cvp_ref_src;
    u16 ref_len = len;
    if (hdl) {
        if (hdl->bit_width) {
            audio_convert_data_32bit_to_16bit_round((s32 *)data, (s16 *)data, ref_len >> 2);
            ref_len >>= 1;
        }

        if (hdl->channel == 2) {
            /* putchar('2'); */
            /*双变单*/
            for (int i = 0; i < (ref_len >> 2); i++) {
                data[i] = ((int)data[2 * i] + (int)data[2 * i + 1]) / 2;
            }
            ref_len >>= 1;
        }
        if (hdl->sw_src_api) {
            /* putchar('s'); */
            ref_len = hdl->sw_src_api->run(hdl->sw_src_buf, data, ref_len >> 1, data);
            ref_len <<= 1;
        }
        /* printf("ref_len %d : %d", ref_len, len); */
        audio_aec_refbuf(data, NULL, ref_len);
    }
}

static void audio_cvp_ref_src_task(void *p)
{
    int msg[16];
    cvp_ref_src_t *hdl = NULL;
    while (1) {

        os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        hdl = cvp_ref_src;
        if (hdl && hdl->state) {
            s16 *data = (s16 *)msg[1];
            int len = msg[2];
            /* putchar('r'); */
            hdl->busy = 1;
            int cbuf_data_len = cbuf_get_data_len(&hdl->cbuf);
            /*判断cbuf的缓存够一帧数据，并且参考数据可写长度大于1帧时*/
            while (cbuf_data_len >= CVP_REF_SRC_FRAME_SIZE && (get_audio_cvp_output_way_writable_len() * hdl->data_multiple) >= CVP_REF_SRC_FRAME_SIZE) {
                cbuf_data_len -= CVP_REF_SRC_FRAME_SIZE;
                cbuf_read(&hdl->cbuf, hdl->ref_tmp_buf, CVP_REF_SRC_FRAME_SIZE);
                audio_cvp_ref_src_run((s16 *)hdl->ref_tmp_buf, CVP_REF_SRC_FRAME_SIZE);
            }
            hdl->busy = 0;
        }
    }
}

int audio_cvp_ref_src_data_fill(void *p, s16 *data, int len)
{
    cvp_ref_src_t *hdl = cvp_ref_src;
    int ret = 0;
    if ((!esco_player_runing()
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
         && !pc_spk_player_runing()
#endif
        ) || (len == 0)) {
        return 0;
    }

    if (hdl && hdl->state) {
        if (0 == cbuf_write(&hdl->cbuf, data, len) && hdl->align_flag) {
            cbuf_clear(&hdl->cbuf);
            hdl->align_flag = 0;
            printf("ref src cbuf wfail!!");
        }
        if (hdl->align_flag) {
            if (cbuf_get_data_len(&hdl->cbuf) >= CVP_REF_SRC_FRAME_SIZE) {
                ret = os_taskq_post_msg(CVP_REF_SRC_TASK_NAME, 2, (int)data, len);
            }
        }
    }
    return ret;
}

static void iis_write_callback(void *data, int len)
{
    cvp_ref_src_t *hdl = cvp_ref_src;
    if (hdl) {
        if (iis_read_pos == IIS_READ_MAGIC) {
            u32 iis_hwptr = get_alink_hwptr(ALINK_CH_IDX);
            if (iis_hwptr) {
                iis_read_pos = 0;
                printf("JL_ALNK HWPTR %d, data_len %d", iis_hwptr, len);
            }
        }
        if (iis_read_pos == 0) {
            if (esco_player_runing()
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
                || pc_spk_player_runing()
#endif
               ) {
                audio_cvp_ref_start(1);
            }
        }
        audio_cvp_ref_src_data_fill(NULL, data, len);
    }
}

/*
*********************************************************************
*                  Audio CVP Ref Src
* Description: 打开外部参考数据变采样
* Arguments  : scene	场景
*			   insr	    输入采样率
*			   outsr	输出采样率
*			   nch	    输入数据的通道数
* Return	 : None.
* Note(s)    : 声卡设备是DAC，默认不用外部提供参考数据
*********************************************************************
*/
int audio_cvp_ref_src_open(u8 scene, u32 insr, u32 outsr, u8 nch)
{
    printf("audio_cvp_ref_src_open");
    if (cvp_ref_src) {
        printf("aec_ref_src alreadly open !!!");
        return -1;
    }
    cvp_ref_src_t *hdl = zalloc(sizeof(cvp_ref_src_t));
    if (hdl == NULL) {
        printf("aec_ref_src malloc fail !!!");
        return -1;
    }

    cbuf_init(&hdl->cbuf, hdl->ref_buf, sizeof(hdl->ref_buf));
    int err = task_create(audio_cvp_ref_src_task, NULL, CVP_REF_SRC_TASK_NAME);
    if (err != OS_NO_ERR) {
        printf("task create error!");
        free(hdl);
        hdl = NULL;
        return -1;
    }

    audio_cvp_set_output_way(1);

    iis_node_write_callback_add(CVP_REF_SRC_TASK_NAME, 1, scene, iis_write_callback);

    hdl->input_rate = insr;
    hdl->output_rate = outsr;
    hdl->channel = nch;
    hdl->bit_width = audio_general_out_dev_bit_width();

    hdl->sw_src_api = get_rs16_context();
    printf("sw_src_api:0x%x\n", (int)(hdl->sw_src_api));
    ASSERT(hdl->sw_src_api);
    int sw_src_need_buf = hdl->sw_src_api->need_buf();
    printf("sw_src_buf:%d\n", sw_src_need_buf);
    hdl->sw_src_buf = zalloc(sw_src_need_buf);
    ASSERT(hdl->sw_src_buf, "sw_src_buf zalloc fail");
    RS_PARA_STRUCT rs_para_obj;
    rs_para_obj.nch = 1;
    rs_para_obj.dataTypeobj.IndataBit = 0;
    rs_para_obj.dataTypeobj.OutdataBit = 0;
    rs_para_obj.dataTypeobj.IndataInc = 1;
    rs_para_obj.dataTypeobj.OutdataInc = 1;
    rs_para_obj.dataTypeobj.Qval = 15;

    int multiple_cnt;//倍数
    if ((insr % 8000) == 0) {
        multiple_cnt = insr / 8000;
        rs_para_obj.new_insample = 6250;
        if (outsr == 16000) {
            //48k->16k
            rs_para_obj.new_outsample = 2080 * (6 / multiple_cnt);
        } else {
            //48k->8k
            rs_para_obj.new_outsample = (2080 / 2) * (6 / multiple_cnt);
        }
    } else {
        multiple_cnt = insr / 11025;
        rs_para_obj.new_insample = 12000;
        if (outsr == 16000) {
            //44.1k->16k
            rs_para_obj.new_outsample = (17 * 256) * (4 / multiple_cnt);
        } else {
            //44.1k->8k
            rs_para_obj.new_outsample = (17 * 256 / 2) * (4 / multiple_cnt);
        }
    }
    printf("sw src,ch = %d, in = %d,out = %d\n", rs_para_obj.nch, rs_para_obj.new_insample, rs_para_obj.new_outsample);
    hdl->sw_src_api->open(hdl->sw_src_buf, &rs_para_obj);
    hdl->data_multiple = (insr / outsr) * nch * (1 << hdl->bit_width);
    printf("indata nch %d, bitw %d, data_multiple %d", hdl->channel, hdl->bit_width, hdl->data_multiple);
    hdl->ref_tmp_buf = zalloc(CVP_REF_SRC_FRAME_SIZE);
    audio_cvp_ref_data_align_reset();
    hdl->state = 1;
    cvp_ref_src = hdl;
    return 0;
}

void audio_cvp_ref_src_close()
{
    cvp_ref_src_t *hdl = cvp_ref_src;
    if (hdl) {
        hdl->state = 0;
        while (hdl->busy) {
            putchar('w');
            os_time_dly(2);
        }
        int err = task_kill(CVP_REF_SRC_TASK_NAME);
        if (err) {
            printf("kill task %s: err=%d\n", CVP_REF_SRC_TASK_NAME, err);
        }

        iis_node_write_callback_del(CVP_REF_SRC_TASK_NAME);
        if (hdl->sw_src_api) {
            hdl->sw_src_api = NULL;
        }
        if (hdl->sw_src_buf) {
            free(hdl->sw_src_buf);
            hdl->sw_src_buf = NULL;
        }
        if (hdl->ref_tmp_buf) {
            free(hdl->ref_tmp_buf);
            hdl->ref_tmp_buf = NULL;
        }
        free(hdl);
        hdl = NULL;
        cvp_ref_src = NULL;
    }
}

