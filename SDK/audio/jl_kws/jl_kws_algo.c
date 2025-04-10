#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".jl_kws_algo.data.bss")
#pragma data_seg(".jl_kws_algo.data")
#pragma const_seg(".jl_kws_algo.text.const")
#pragma code_seg(".jl_kws_algo.text")
#endif
#include "jl_kws_common.h"
#include"jlsp_simple_dvad.h"

#if TCFG_KWS_VOICE_RECOGNITION_ENABLE

int CONFIG_YESNO_VAD_ENABLE = 0;

//===================================================//
//                    算法接口函数                   //
//===================================================//
#define KWS_FRAME_LEN 		320
//每帧固定320 bytes, 16k采样率, 10ms
int jlsp_wake_word_yesno_heap_size(int *private_heap_size, int *share_heap_size);
void *JL_kws_init(char *private_heap_buffer, int private_heap_size, char *share_heap_buffer, int share_heap_size,
                  float confidence_yes, float confidence_no, int smooth_yes, int smooth_no, int online, int init_noise);
int jl_detect_kws(void *m, dvad_config_t *cfg, char *pcm, int size);
void JL_kws_free(void *_jl_det_kws);
int jl_kws_get_need_buf_len(void);

#define ALIGN(a, b) \
	({ \
	 	int m = (u32)(a) & ((b)-1); \
		int rets = (u32)(a) + (m?((b)-m):0);	 \
		rets;\
	})


//===================================================//
//             IO 口DEBUG(测量算法时间)              //
//===================================================//
#define     KWS_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     KWS_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}

/*
 *Yes/No词条识别阈值:
 *(1)越大越难识别，相应的，词条识别匹配度就越高，即越不容易误触发
 *(2)越小越容易识别，相应的，词条识别匹配度就越低，即越容易误触发
 */
#define KWS_YES_THR		0.6f
#define KWS_NO_THR		0.6f

struct jl_kws_algo {
    void *kws_handle;
    u32 *frame_buf;
    char *private_heap;
    char *share_heap;
};

static struct jl_kws_algo __kws_algo = {0};

static void kws_algo_local_init(void)
{
    //===============================//
    //       申请算法需要的buf       //
    //===============================//
    int private_heap_size, share_heap_size;
    jlsp_wake_word_yesno_heap_size(&private_heap_size, &share_heap_size);
    kws_debug("private_heap_size:%d, share_heap_size:%d\n", private_heap_size, share_heap_size);
    __kws_algo.private_heap = (char *)zalloc(private_heap_size);
    __kws_algo.share_heap = (char *)zalloc(share_heap_size);

    //===============================//
    //       	 算法初始化          //
    //===============================//
    int cmn_online = 1;
    int smooth_yes = 25; //值越大对应的准确率越低，同时误触发也越低
    int smooth_no = 30; //值越大对应的准确率越低，同时误触发也越低
    float confidence_yes = KWS_YES_THR;
    float confidence_no = KWS_NO_THR;
    int init_noise = (45 << 8) | 50;
    __kws_algo.kws_handle = JL_kws_init(__kws_algo.private_heap, private_heap_size, __kws_algo.share_heap, share_heap_size,
                                        confidence_yes, confidence_no, smooth_yes, smooth_no, cmn_online, init_noise);
}

int jl_kws_algo_init(void)
{
    FILE *fp = NULL;
    void *buf_ptr = NULL;

    buf_ptr = zalloc(KWS_FRAME_LEN);
    kws_debug("KWS_FRAME_LEN = 0x%x", KWS_FRAME_LEN);
    if (!buf_ptr) {
        return JL_KWS_ERR_ALGO_NO_BUF;
    }
    __kws_algo.frame_buf = buf_ptr;

    kws_algo_local_init();

    return JL_KWS_ERR_NONE;
}


int jl_kws_algo_detect_run(u8 *buf, u32 len)
{
    int ret = 0;
    dvad_config_t dvad_cfg = {0};
    dvad_cfg.d2a_frame_con = 100;
    dvad_cfg.d2a_th_db = 20;
    dvad_cfg.dvad_gain_id = 10;
    dvad_cfg.d_frame_con = 100;
    dvad_cfg.d_high_con_th = 3;
    dvad_cfg.d_low_con_th = 6;
    dvad_cfg.d_stride1 = 3;
    dvad_cfg.d_stride2 = 7;
    /* local_irq_disable(); */
    /* KWS_IO_DEBUG_1(A,4); */
    if (__kws_algo.kws_handle) {
        ret = jl_detect_kws(__kws_algo.kws_handle, &dvad_cfg, (char *)buf, len);
    }
    /* KWS_IO_DEBUG_0(A,4); */
    /* local_irq_enable(); */

    if ((ret != KWS_VOICE_EVENT_YES) &&
        (ret != KWS_VOICE_EVENT_NO)) {
        ret = KWS_VOICE_EVENT_NONE;
    }

    return ret;
}

void *jl_kws_algo_get_frame_buf(u32 *buf_len)
{
    if (__kws_algo.frame_buf) {
        *buf_len = KWS_FRAME_LEN;
        return __kws_algo.frame_buf;
    }

    return NULL;
}

int jl_kws_algo_start(void)
{
    return JL_KWS_ERR_NONE;
}

void jl_kws_algo_stop(void)
{
    return;
}


void jl_kws_algo_close(void)
{
    kws_info("%s", __func__);

    if (__kws_algo.kws_handle) {
        JL_kws_free(__kws_algo.kws_handle);
        __kws_algo.kws_handle = NULL;
    }

    if (__kws_algo.frame_buf) {
        free(__kws_algo.frame_buf);
        __kws_algo.frame_buf = NULL;
    }

    if (__kws_algo.private_heap) {
        free(__kws_algo.private_heap);
        __kws_algo.private_heap = NULL;
    }

    if (__kws_algo.share_heap) {
        free(__kws_algo.share_heap);
        __kws_algo.share_heap = NULL;
    }
}

#endif /* #if TCFG_KWS_VOICE_RECOGNITION_ENABLE */
