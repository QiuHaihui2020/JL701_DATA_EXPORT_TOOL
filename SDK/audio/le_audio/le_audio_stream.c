/*************************************************************************************************/
/*!
*  \file      le_audio_stream.c
*
*  \brief
*
*  Copyright (c) 2011-2023 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#include "generic/atomic.h"
#include "le_audio_stream.h"
#include "audio_base.h"
#include "circular_buf.h"
#include "system/timer.h"
#define LE_AUDIO_TX_TEST        0

struct le_audio_stream_buf {
    void *addr;
    int size;
    cbuffer_t cbuf;
};

struct le_audio_tx_stream {
    struct le_audio_stream_buf buf;
    void *tick_priv;
    int (*tick_handler)(void *priv, int len, u32 timestamp);
    void *parent;
    u32 coding_type;

#if LE_AUDIO_TX_TEST
    u16 test_timer;
    int sdu_period_len;
    void *test_buf;
#endif
};

struct le_audio_rx_stream {
    struct le_audio_stream_buf buf;
    struct list_head frames;
    int sdu_period_len;
    int frames_len;
    int frames_max_size;
    u32 coding_type;
    atomic_t ref;
    void *parent;
    u32 timestamp;
    u8 online;
};

struct le_audio_stream_context {
    u16 conn;
    u8 start;
    u32 start_time;
    struct le_audio_stream_format fmt;
    struct le_audio_tx_stream *tx_stream;
    struct le_audio_rx_stream *rx_stream;
    spinlock_t lock;
    void *tx_tick_priv;
    int (*tx_tick_handler)(void *priv, int period, u32 timestamp);
    void *rx_tick_priv;
    void (*rx_tick_handler)(void *priv);
};

extern int bt_audio_reference_clock_select(void *addr, u8 network);
extern u32 bb_le_clk_get_time_us(void);
extern void ll_config_ctrler_clk(uint16_t handle, uint8_t sel);

#if (LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_V2)
extern const unsigned short JLA_V2_FRAMELEN_MASK;
#endif

void *le_audio_stream_create(u16 conn, struct le_audio_stream_format *fmt)
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)zalloc(sizeof(struct le_audio_stream_context));

    memcpy(&ctx->fmt, fmt, sizeof(ctx->fmt));

    printf("[le audio stream : %d, %d, 0x%x, %d, %d, %d]\n", ctx->fmt.nch, ctx->fmt.bit_rate, ctx->fmt.coding_type,
           ctx->fmt.frame_dms, ctx->fmt.sdu_period, ctx->fmt.sample_rate);
    spin_lock_init(&ctx->lock);
    ctx->conn = conn;
    ctx->start = 1;

    return ctx;
}

void le_audio_stream_free(void *le_audio)
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;

    if (ctx) {
        free(ctx);
    }
}

int le_audio_stream_clock_select(void *le_audio)
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;

    bt_audio_reference_clock_select(NULL, 2);
    ll_config_ctrler_clk((uint16_t)ctx->conn, 0); //蓝牙与同步关联 bis_hdl/cis_hdl
    return 0;
}

u32 le_audio_stream_current_time(void *le_audio)
{
    return bb_le_clk_get_time_us();
}

static int __le_audio_stream_tx_data_handler(void *stream, void *data, int len, u32 timestamp, int latency)
{
    struct le_audio_tx_stream *tx_stream = (struct le_audio_tx_stream *)stream;
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)tx_stream->parent;
    struct le_audio_rx_stream *rx_stream = ctx->rx_stream;
    u32 rlen = 0;
    u32 read_alloc_len = 0;

    /*putchar('A');*/
    if (!ctx->start) {
        u32 time_diff = (bb_le_clk_get_time_us() - ctx->start_time) & 0xfffffff;
        if (time_diff > 5000000) {
            return 0;
        }
        ctx->start = 1;
    }

    if (cbuf_get_data_len(&tx_stream->buf.cbuf) < len ||
        (rx_stream && cbuf_get_data_len(&rx_stream->buf.cbuf) < rx_stream->sdu_period_len)) {
        /*对于需要本地播放的必须满足播放与发送都有一个interval的数据*/
        /*y_printf("no data : %u, %d\n", timestamp, latency);*/
        return 0;
    }

    spin_lock(&ctx->lock);
    rlen = cbuf_read(&tx_stream->buf.cbuf, data, len);
    spin_unlock(&ctx->lock);
    /*printf("-%d, %d-\n", len, rlen);*/
    if (!rlen) {
        return 0;
    }

    if (ctx->tx_tick_handler) {
        ctx->tx_tick_handler(ctx->tx_tick_priv, ctx->fmt.sdu_period, timestamp);
    }

    if (tx_stream->tick_handler) {
        tx_stream->tick_handler(tx_stream->tick_priv, len, timestamp);
    }

    /*putchar('B');*/
    if (rx_stream) {
        spin_lock(&ctx->lock);
        void *addr = cbuf_read_alloc(&rx_stream->buf.cbuf, &read_alloc_len);
        if (read_alloc_len < rx_stream->sdu_period_len) {
            printf("local not align to tx.\n");
            return rlen;
        }
        if ((tx_stream->coding_type == AUDIO_CODING_LC3 || tx_stream->coding_type == AUDIO_CODING_JLA) &&
            rx_stream->coding_type == AUDIO_CODING_PCM) {
            timestamp = (timestamp + (ctx->fmt.frame_dms == 75 ? 4000L : 2500L)) & 0xfffffff;
#if (LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_V2)
        } else if (tx_stream->coding_type == AUDIO_CODING_JLA_V2 && rx_stream->coding_type == AUDIO_CODING_PCM) {
            u16 input_point = ctx->fmt.frame_dms * ctx->fmt.sample_rate / 1000 / 10;
            u32 time_diff = 0;
            if (!(JLA_V2_FRAMELEN_MASK & 0x8000) || input_point >= 160) { //最高位是0或者点数大于160,jla_v2的延时是1/4帧
                time_diff = ctx->fmt.frame_dms * 1000 / 10 /  4; //(us)
            } else {
                time_diff = ctx->fmt.frame_dms * 1000 / 10; //(us)
            }
            timestamp = (timestamp + time_diff) & 0xfffffff;
#endif
        }
        timestamp = (timestamp + latency) & 0xfffffff;
        le_audio_stream_rx_frame(rx_stream, addr, rx_stream->sdu_period_len, timestamp);
        cbuf_read_updata(&rx_stream->buf.cbuf, rx_stream->sdu_period_len);
        spin_unlock(&ctx->lock);
        /*printf("-%d-\n", rx_stream->sdu_period_len);*/
    }

    return rlen;
}

int le_audio_stream_tx_data_handler(void *le_audio, void *data, int len, u32 timestamp, int latency)
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;

    spin_lock(&ctx->lock);
    if (ctx->tx_stream) {
        int rlen = __le_audio_stream_tx_data_handler(ctx->tx_stream, data, len, timestamp, latency);
        spin_unlock(&ctx->lock);
        return rlen;

    }
    spin_unlock(&ctx->lock);

    return 0;
}

#if LE_AUDIO_TX_TEST
static void le_audio_tx_test_timer(void *stream)
{
    struct le_audio_tx_stream *tx_stream = (struct le_audio_tx_stream *)stream;

    __le_audio_stream_tx_data_handler(tx_stream, tx_stream->test_buf, tx_stream->sdu_period_len, 0x12345678);
}
#endif

void le_audio_stream_set_tx_tick_handler(void *le_audio, void *priv, int (*tick_hanlder)(void *, int, u32))
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;

    if (!ctx) {
        return;
    }

    spin_lock(&ctx->lock);
    ctx->tx_tick_handler = tick_hanlder;
    ctx->tx_tick_priv = priv;
    spin_unlock(&ctx->lock);
}

void *le_audio_stream_tx_open(void *le_audio, int coding_type, void *priv, int (*tick_handler)(void *, int, u32))
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;
    struct le_audio_tx_stream *tx_stream = NULL;
    int frame_size = 0;

    if (ctx->tx_stream) {
        return ctx->tx_stream;
    }

    tx_stream = (struct le_audio_tx_stream *)zalloc(sizeof(struct le_audio_tx_stream));

    if (ctx->fmt.coding_type == AUDIO_CODING_LC3) {
        frame_size = ctx->fmt.frame_dms * ctx->fmt.bit_rate / 8 / 10000 ;
    } else if (ctx->fmt.coding_type == AUDIO_CODING_JLA) {
        frame_size = ctx->fmt.frame_dms * ctx->fmt.bit_rate / 8 / 10000 + 2;
    } else if (ctx->fmt.coding_type == AUDIO_CODING_JLA_V2) {
        frame_size = ctx->fmt.frame_dms * ctx->fmt.bit_rate / 8 / 10000 + 2;
    } else {
        //TODO : 其他格式的buffer设置
    }

    int sdu_period_len = (ctx->fmt.sdu_period / 100 / ctx->fmt.frame_dms) * frame_size;
    tx_stream->buf.size = sdu_period_len * 8;
    tx_stream->buf.addr = malloc(tx_stream->buf.size);
    ASSERT(tx_stream->buf.addr != NULL, "please check audio param");
    printf("tx stream buffer : 0x%x, %d\n", (u32)tx_stream->buf.addr, tx_stream->buf.size);
    cbuf_init(&tx_stream->buf.cbuf, tx_stream->buf.addr, tx_stream->buf.size);
    tx_stream->coding_type = coding_type;
    tx_stream->parent = ctx;
    tx_stream->tick_priv = priv;
    tx_stream->tick_handler = tick_handler;

    ctx->tx_stream = tx_stream;

#if LE_AUDIO_TX_TEST
    tx_stream->test_timer = sys_hi_timer_add(tx_stream, le_audio_tx_test_timer, ctx->fmt.sdu_period / 1000);
    tx_stream->test_buf = malloc(sdu_period_len);
    tx_stream->sdu_period_len = sdu_period_len;
#else
    /* le_audio_set_tx_data_handler(ctx->conn, tx_stream, le_audio_stream_tx_data_handler); */
#endif

    return tx_stream;
}

void le_audio_stream_tx_close(void *stream)
{
    struct le_audio_tx_stream *tx_stream = (struct le_audio_tx_stream *)stream;
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)tx_stream->parent;

    spin_lock(&ctx->lock);
#if LE_AUDIO_TX_TEST
    if (tx_stream->test_timer) {
        sys_hi_timer_del(tx_stream->test_timer);
    }
    if (tx_stream->test_buf) {
        free(tx_stream->test_buf);
    }
#endif
    if (tx_stream->buf.addr) {
        free(tx_stream->buf.addr);
    }

    free(tx_stream);
    ctx->tx_stream = NULL;
    spin_unlock(&ctx->lock);
}

int le_audio_stream_tx_buffered_time(void *stream)
{
    struct le_audio_tx_stream *tx_stream = (struct le_audio_tx_stream *)stream;
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)tx_stream->parent;

    int frame_size = ctx->fmt.frame_dms * ctx->fmt.bit_rate / 8 / 10000 + 2;

    return cbuf_get_data_len(&tx_stream->buf.cbuf) / frame_size * ctx->fmt.frame_dms * 100;
}

int le_audio_stream_set_start_time(void *le_audio, u32 start_time)
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;

    if (ctx) {
        ctx->start_time = start_time;
        ctx->start = 0;
    }

    return 0;
}

int le_audio_stream_set_bit_width(void *le_audio, u8 bit_width)
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;
    if (ctx) {
        ctx->fmt.bit_width = bit_width;
        return 0;
    }
    return -1;
}

void *le_audio_stream_rx_open(void *le_audio, int coding_type)
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;
    struct le_audio_rx_stream *rx_stream = ctx->rx_stream;
    int frame_size = 0;

    if (rx_stream) {
        atomic_inc(&rx_stream->ref);
        return rx_stream;
    }

    rx_stream = (struct le_audio_rx_stream *)zalloc(sizeof(struct le_audio_rx_stream));
    if (!rx_stream) {
        return NULL;
    }

    INIT_LIST_HEAD(&rx_stream->frames);
    if (coding_type == AUDIO_CODING_LC3) {
        frame_size = ctx->fmt.frame_dms * ctx->fmt.bit_rate / 8 / 10000 ;
    } else if (coding_type == AUDIO_CODING_JLA) {
        frame_size = ctx->fmt.frame_dms * ctx->fmt.bit_rate / 8 / 10000 + 2;
    } else if (coding_type == AUDIO_CODING_JLA_V2) {
        frame_size = ctx->fmt.frame_dms * ctx->fmt.bit_rate / 8 / 10000 + 2;
    } else if (coding_type == AUDIO_CODING_PCM) {
        frame_size = ctx->fmt.frame_dms * ctx->fmt.sample_rate * ctx->fmt.nch * (ctx->fmt.bit_width ? 4 : 2) / 10000;
    }

    int iso_interval_len = (ctx->fmt.isoIntervalUs / 100 / ctx->fmt.frame_dms) * frame_size;
    /*如果存在flush timeout，那么缓冲需要大于flush timeout的数量*/
    rx_stream->frames_max_size = iso_interval_len * (ctx->fmt.flush_timeout ? (ctx->fmt.flush_timeout + 5) : 10);
    rx_stream->buf.size = rx_stream->frames_max_size;
    rx_stream->buf.addr = malloc(rx_stream->frames_max_size);
    rx_stream->sdu_period_len = iso_interval_len;
    cbuf_init(&rx_stream->buf.cbuf, rx_stream->buf.addr, rx_stream->buf.size);
    rx_stream->parent = ctx;
    rx_stream->coding_type = coding_type;
    rx_stream->online = 1;

    atomic_inc(&rx_stream->ref);
    ctx->rx_stream = rx_stream;

    return rx_stream;
}

void le_audio_stream_rx_close(void *stream)
{
    struct le_audio_rx_stream *rx_stream = (struct le_audio_rx_stream *)stream;
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)rx_stream->parent;

    if (!rx_stream) {
        return;
    }

    spin_lock(&ctx->lock);
    if (atomic_dec(&rx_stream->ref) == 0) {
        if (rx_stream->buf.addr) {
            free(rx_stream->buf.addr);
        }
        struct le_audio_frame *frame, *n;
        list_for_each_entry_safe(frame, n, &rx_stream->frames, entry) {
            __list_del_entry(&frame->entry);
            free(frame);
        }
        free(rx_stream);
        ctx->rx_stream = NULL;
    }
    spin_unlock(&ctx->lock);
}

int le_audio_stream_get_rx_format(void *le_audio, struct le_audio_stream_format *fmt)
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;
    struct le_audio_rx_stream *rx_stream = (struct le_audio_rx_stream *)ctx->rx_stream;

    if (!rx_stream) {
        return -EINVAL;
    }
    fmt->coding_type = rx_stream->coding_type;
    fmt->nch = ctx->fmt.nch;
    fmt->sample_rate = ctx->fmt.sample_rate;
    fmt->frame_dms = ctx->fmt.frame_dms;
    fmt->bit_rate = ctx->fmt.bit_rate;
    //y_printf("le_audio param : %x,%d,%d,%d,%d\n", fmt->coding_type, fmt->nch, fmt->sample_rate, fmt->frame_dms, fmt->bit_rate);

    return 0;
}

int le_audio_stream_get_dec_ch_mode(void *le_audio)
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;

    if (!ctx) {
        return -EINVAL;
    }

    return ctx->fmt.dec_ch_mode;
}

void le_audio_stream_set_rx_tick_handler(void *le_audio, void *priv, void (*tick_handler)(void *priv))
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;

    if (ctx) {
        spin_lock(&ctx->lock);
        ctx->rx_tick_priv = priv;
        ctx->rx_tick_handler = tick_handler;
        spin_unlock(&ctx->lock);
    }
}

int le_audio_stream_tx_write(void *stream, void *data, int len)
{
    struct le_audio_tx_stream *tx_stream = (struct le_audio_tx_stream *)stream;

    int wlen = cbuf_write(&tx_stream->buf.cbuf, (u8 *)data, len);
    if (wlen < len) {
        /* putchar('t'); */
    }

    return wlen;
}

int le_audio_stream_rx_write(void *stream, void *data, int len)
{
    struct le_audio_rx_stream *rx_stream = (struct le_audio_rx_stream *)stream;

    int wlen = cbuf_write(&rx_stream->buf.cbuf, data, len);
    if (wlen < len) {
        /* putchar('r'); */
    }

    return wlen;
}

int le_audio_stream_tx_drain(void *stream)
{
    struct le_audio_tx_stream *tx_stream = (struct le_audio_tx_stream *)stream;
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)tx_stream->parent;

    /*r_printf("tx buffered : %d\n", cbuf_get_data_len(&tx_stream->buf.cbuf));*/
    spin_lock(&ctx->lock);
    cbuf_clear(&tx_stream->buf.cbuf);
    spin_unlock(&ctx->lock);
    return 0;
}

int le_audio_stream_rx_drain(void *stream)
{
    struct le_audio_rx_stream *rx_stream = (struct le_audio_rx_stream *)stream;
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)rx_stream->parent;

    /*r_printf("rx buffered : %d\n", cbuf_get_data_len(&rx_stream->buf.cbuf));*/
    spin_lock(&ctx->lock);
    cbuf_clear(&rx_stream->buf.cbuf);
    spin_unlock(&ctx->lock);
    return 0;
}

int le_audio_stream_rx_frame(void *stream, void *data, int len, u32 timestamp)
{
    struct le_audio_rx_stream *rx_stream = (struct le_audio_rx_stream *)stream;
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)rx_stream->parent;
    struct le_audio_frame *frame = NULL;

    if (rx_stream->frames_len + len > rx_stream->frames_max_size) {
        /*printf("frame no buffer.\n");*/
        spin_lock(&ctx->lock);
        if (!list_empty(&rx_stream->frames)) {
            frame = list_first_entry(&rx_stream->frames, struct le_audio_frame, entry);
            list_del(&frame->entry);
            rx_stream->frames_len -= frame->len;
            free(frame);
        }
        spin_unlock(&ctx->lock);
        putchar('H');
        return 0;
    }
    frame = malloc(sizeof(struct le_audio_frame) + len);
    if (!frame) {
        return 0;
    }
    frame->len = len;
    frame->data = (u8 *)(frame + 1);
    frame->timestamp = timestamp;
    rx_stream->timestamp = timestamp;
    memcpy(frame->data, data, len);

    spin_lock(&ctx->lock);
    list_add_tail(&frame->entry, &rx_stream->frames);
    rx_stream->frames_len += len;
    if (ctx->rx_tick_handler) {
        ctx->rx_tick_handler(ctx->rx_tick_priv);
    }
    spin_unlock(&ctx->lock);
    return len;
}

static int le_audio_stream_rx_fill_frame(struct le_audio_rx_stream *rx_stream)
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)rx_stream->parent;

    if (rx_stream->coding_type == AUDIO_CODING_LC3 || rx_stream->coding_type == AUDIO_CODING_JLA || rx_stream->coding_type == AUDIO_CODING_JLA_V2) {
        rx_stream->timestamp = (rx_stream->timestamp + ctx->fmt.sdu_period) & 0xfffffff;
        u8 jla_err_frame[2] = {0x02, 0x00};
        u8 err_packet[10] = {0};
        u8 frame_num = ctx->fmt.sdu_period / 100 / ctx->fmt.frame_dms;
        for (int i = 0; i < frame_num; i++) {
            memcpy(&err_packet[i * 2], jla_err_frame, 2);
        }
        le_audio_stream_rx_frame(rx_stream, err_packet, frame_num * 2, rx_stream->timestamp);
    }

    return 0;
}

void le_audio_stream_rx_disconnect(void *stream)
{
    struct le_audio_rx_stream *rx_stream = (struct le_audio_rx_stream *)stream;
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)rx_stream->parent;

    spin_lock(&ctx->lock);
    rx_stream->online = 0;
    le_audio_stream_rx_fill_frame(rx_stream);
    spin_unlock(&ctx->lock);
}

struct le_audio_frame *le_audio_stream_get_frame(void *le_audio)
{
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;
    struct le_audio_rx_stream *rx_stream = (struct le_audio_rx_stream *)ctx->rx_stream;
    struct le_audio_frame *frame = NULL;

    if (!ctx || !rx_stream) {
        return NULL;
    }

    spin_lock(&ctx->lock);
get_frame:
    if (!list_empty(&rx_stream->frames)) {
        frame = list_first_entry(&rx_stream->frames, struct le_audio_frame, entry);
        list_del(&frame->entry);
    }

    if (!frame && !rx_stream->online) {
        if (rx_stream->coding_type == AUDIO_CODING_LC3 || rx_stream->coding_type == AUDIO_CODING_JLA || rx_stream->coding_type == AUDIO_CODING_JLA_V2) {
            le_audio_stream_rx_fill_frame(rx_stream);
            goto get_frame;
        }
    }

    spin_unlock(&ctx->lock);

    return frame;
}

int le_audio_stream_get_frame_num(void *le_audio)
{
    if (!le_audio) {
        return 0;
    }
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;
    struct le_audio_rx_stream *rx_stream = ctx->rx_stream;

    if (!rx_stream) {
        return 0;
    }

    return rx_stream->frames_len / rx_stream->sdu_period_len;
}

void le_audio_stream_free_frame(void *le_audio, struct le_audio_frame *frame)
{
    if (!le_audio) {
        return;
    }
    struct le_audio_stream_context *ctx = (struct le_audio_stream_context *)le_audio;
    struct le_audio_rx_stream *rx_stream = ctx->rx_stream;
    if (!rx_stream) {
        return;
    }

    spin_lock(&ctx->lock);
    rx_stream->frames_len -= frame->len;
    spin_unlock(&ctx->lock);
    free(frame);
}
