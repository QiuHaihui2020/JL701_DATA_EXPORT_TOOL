
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "jlstream.h"
#include "effects/effects_adj.h"
#include "effects_dev.h"

#define LOG_TAG_CONST EFFECTS
#define LOG_TAG     "[EFFECTS_ADJ]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"


#ifdef SUPPORT_MS_EXTENSIONS
#pragma   bss_seg(".effect_dev.data.bss")
#pragma  data_seg(".effect_dev.data")
#pragma const_seg(".effect_dev.text.const")
#pragma  code_seg(".effect_dev.text")
#endif
#if (TCFG_EFFECT_DEV0_NODE_ENABLE || TCFG_EFFECT_DEV1_NODE_ENABLE || TCFG_EFFECT_DEV2_NODE_ENABLE || TCFG_EFFECT_DEV2_NODE_ENABLE || TCFG_EFFECT_DEV3_NODE_ENABLE || TCFG_EFFECT_DEV4_NODE_ENABLE)


void effect_dev_init(struct packet_ctrl *hdl, u32 process_points_per_ch)
{
    if (process_points_per_ch) {
        hdl->frame_len = process_points_per_ch * hdl->in_ch_num * (2 << hdl->bit_width);
        hdl->remain_buf = zalloc(hdl->frame_len);
    }
}

void effect_dev_process(struct packet_ctrl *hdl, struct stream_iport *iport,  struct stream_note *note)
{
    struct stream_frame *frame;
    struct stream_node *node = iport->node;

    if (hdl->remain_buf) {
        frame = jlstream_pull_frame(iport, note);		//从iport读取数据
        if (!frame) {
            return;
        }

        /*算法出来一帧的数据长度，byte*/
        hdl->remain_len += frame->len;  //记录目前还有多少数据
        u16 tmp_remain = 0;             //记录上一次剩余多少
        u8 pack_frame_num = hdl->remain_len / hdl->frame_len;//每次数据需要跑多少帧
        u16 pack_frame_len = pack_frame_num * hdl->frame_len;       //记录本次需要跑多少数据
        u16 out_frame_len = hdl->out_ch_num * pack_frame_len / hdl->in_ch_num;
        u16 offset = 0;
        u16 out_offset = 0;

        if (pack_frame_num) {
            if (!hdl->pack_frame) {
                /*申请资源存储输入*/
                hdl->pack_frame = malloc(pack_frame_len);
                if (!hdl->pack_frame) {
                    return;
                }
            }
            if (!hdl->out_frame) {
                /*申请资源存储输出输出*/
                hdl->out_frame = jlstream_get_frame(node->oport, out_frame_len);
                if (!hdl->out_frame) {
                    return;
                }
            }
            tmp_remain = hdl->remain_len - frame->len;//上一次剩余的数据大小
            /*拷贝上一次剩余的数据*/
            memcpy(hdl->pack_frame, hdl->remain_buf, tmp_remain);
            /*拷贝本次数据*/
            memcpy((void *)((int)hdl->pack_frame + tmp_remain), frame->data, pack_frame_len - tmp_remain);
            if (hdl->out_ch_num == hdl->in_ch_num) {
                memcpy(hdl->out_frame->data, hdl->pack_frame, pack_frame_len);
            }
            while (pack_frame_num--) {
                hdl->effect_run(hdl->node_hdl, (s16 *)((int)hdl->pack_frame + offset), (s16 *)((int)hdl->out_frame->data + out_offset), hdl->frame_len);
                offset += hdl->frame_len;
                out_offset = hdl->out_ch_num * offset / hdl->in_ch_num;
            }
            hdl->remain_len -= pack_frame_len;//剩余数据长度
            hdl->out_frame->len = out_frame_len;
            jlstream_push_frame(node->oport, hdl->out_frame);	//将数据推到oport
            hdl->out_frame = NULL;
            free(hdl->pack_frame);
            hdl->pack_frame = NULL;

            /*保存剩余不够一帧的数据*/
            memcpy(hdl->remain_buf, (void *)((int)frame->data + frame->len - hdl->remain_len), hdl->remain_len);
            jlstream_free_frame(frame);	//释放iport资源
        } else {
            /*当前数据不够跑一帧算法时*/
            memcpy((void *)((int)hdl->remain_buf + hdl->remain_len - frame->len), frame->data, frame->len);
            jlstream_free_frame(frame);	//释放iport资源
        }
    } else {
        struct stream_frame *out_frame;
        frame = jlstream_pull_frame(iport, note);		//从iport读取数据
        if (!frame) {
            return;
        }

        if (hdl->out_ch_num != hdl->in_ch_num) {
            int out_len = hdl->out_ch_num * frame->len / hdl->in_ch_num;
            out_frame = jlstream_get_frame(node->oport, out_len);
            if (!out_frame) {
                return;
            }
            hdl->effect_run(hdl->node_hdl, (s16 *)frame->data, (s16 *)out_frame->data, frame->len);
            out_frame->len = out_len;
            jlstream_push_frame(node->oport, out_frame);	//将数据推到oport
            jlstream_free_frame(frame);	//释放iport资源
        } else {
            hdl->effect_run(hdl->node_hdl, (s16 *)frame->data, (s16 *)frame->data, frame->len);
            jlstream_push_frame(node->oport, frame);	//将数据推到oport
        }
    }
}
void effect_dev_close(struct packet_ctrl *hdl)
{
    hdl->remain_len = 0;
    if (hdl->remain_buf) {
        free(hdl->remain_buf);
        hdl->remain_buf = NULL;
    }
    if (hdl->pack_frame) {
        free(hdl->pack_frame);
        hdl->pack_frame = NULL;
    }

    if (hdl->out_frame) {
        jlstream_free_frame(hdl->out_frame);
        hdl->out_frame = NULL;
    }

}
#endif


