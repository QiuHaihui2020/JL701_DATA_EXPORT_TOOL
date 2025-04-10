#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".frame_length_adaptive.data.bss")
#pragma data_seg(".frame_length_adaptive.data")
#pragma const_seg(".frame_length_adaptive.text.const")
#pragma code_seg(".frame_length_adaptive.text")
#endif
#include "frame_length_adaptive.h"


/*设置每次处理的帧长大小 和 处理回调*/
struct fixed_frame_len_handle *audio_fixed_frame_len_init(int frame_len, int (*output_handle)(void *priv, u8 *in, u8 *out, int len), void *priv)
{
    struct fixed_frame_len_handle *hdl = zalloc(sizeof(struct fixed_frame_len_handle));
    printf("audio_fixed_frame_len_init, frame_len: %d", frame_len);
    ASSERT(hdl);
    hdl->frame_len = frame_len;
    hdl->process_buf = zalloc(hdl->frame_len);
    ASSERT(hdl->process_buf);
    hdl->priv = priv;
    hdl->output_handle = output_handle;
    hdl->remain_len = 0;
    hdl->start = 1;
    return hdl;
}

/*根据输入数据长度，获取需要输出数据大小*/
int get_fixed_frame_len_output_len(struct fixed_frame_len_handle *hdl, int in_len)
{
    int out_len = 0;
    if (hdl && hdl->start) {
        out_len = ((hdl->remain_len + in_len) / hdl->frame_len) * hdl->frame_len;
    }
    return out_len;
}

/*按照设置的帧长处理数据输出*/
int audio_fixed_frame_len_run(struct fixed_frame_len_handle *hdl, u8 *in, u8 *out, int len)
{
    if (hdl && hdl->start) {
        int tmp_remain = hdl->remain_len;//上一次剩余的数据大小
        hdl->remain_len = hdl->remain_len + len;//记录目前还有多少数据
        u8 pack_frame_num = hdl->remain_len / hdl->frame_len;//每次数据需要跑多少帧
        u16 pack_frame_len = pack_frame_num * hdl->frame_len;       //记录本次需要跑多少数据
        int in_offset = 0, out_offset = 0;
        if (pack_frame_num) {
            memcpy(hdl->process_buf + tmp_remain, in, hdl->frame_len - tmp_remain);
            while (pack_frame_num--) {
                out_offset += hdl->output_handle(hdl->priv, hdl->process_buf, out + out_offset, hdl->frame_len);
                in_offset += hdl->frame_len;
                if ((hdl->remain_len - in_offset) >= hdl->frame_len) {
                    memcpy(hdl->process_buf, in + len - (hdl->remain_len - in_offset), hdl->frame_len);
                }
            }
            hdl->remain_len -= pack_frame_len;//剩余数据长度;
            /*本次处理还有剩余时*/
            if (hdl->remain_len) {
                memcpy(hdl->process_buf, in + len - hdl->remain_len, hdl->remain_len);
            }

            return out_offset;
        } else {
            /*当前数据不够跑一帧算法时*/
            memcpy(hdl->process_buf + hdl->remain_len - len, in, len);
        }
    }
    return 0;
}

void audio_fixed_frame_len_exit(struct fixed_frame_len_handle *hdl)
{
    if (hdl && hdl->start) {
        if (hdl->process_buf) {
            free(hdl->process_buf);
            hdl->process_buf = NULL;
        }
        free(hdl);
    }
}

int frame_length_adaptive_run(void *_hdl, void *in_data, void *out_data, u16 in_len)
{
    struct frame_length_adaptive_hdl *hdl = (struct frame_length_adaptive_hdl *)_hdl;
    if (!hdl) {
        return 0;
    }
    int	wlen, rlen = 0;
    wlen = cbuf_write(&hdl->sw_cbuf, in_data, in_len);
    if (wlen != in_len) {
        putchar('w');
    }
    if (cbuf_get_data_len(&hdl->sw_cbuf) >= hdl->adj_len) {
        rlen = cbuf_read(&hdl->sw_cbuf, out_data, hdl->adj_len);
        if (rlen != hdl->adj_len) {
            putchar('W');
        }
    }
    return rlen;
}

/* adj_len : 需要输出的数据长度
 * in_len : 输入的数据长度
 * */
struct frame_length_adaptive_hdl *frame_length_adaptive_open(int adj_len, int in_len)
{
    struct frame_length_adaptive_hdl *hdl = zalloc(sizeof(struct frame_length_adaptive_hdl));
    if (!hdl) {
        return NULL;
    }
    printf("frame_length_adaptive open---------adj_len = %d\n", adj_len);
    int cbuf_len = adj_len > in_len ? adj_len : in_len;
    hdl->adj_len = adj_len;
    hdl->sw_buf = zalloc(cbuf_len * 2);
    cbuf_init(&hdl->sw_cbuf, hdl->sw_buf, cbuf_len * 2);

    u32 len;
    void *obuf = cbuf_write_alloc(&hdl->sw_cbuf, &len); //先填一帧的数据,
    memset(obuf, 0, adj_len);
    cbuf_write_updata(&hdl->sw_cbuf, adj_len);

    return hdl;
}

void frame_length_adaptive_close(void *_hdl)
{
    struct frame_length_adaptive_hdl *hdl = (struct frame_length_adaptive_hdl *)_hdl;
    if (hdl) {
        if (hdl->sw_buf) {
            free(hdl->sw_buf);
        }
        free(hdl);
    }
}


