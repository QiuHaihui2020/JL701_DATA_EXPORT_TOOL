#include "audio_raw_writer.h"
#include "system/includes.h"
#include "display/display_task.h"

#define RAW_MAX_CH              8               /*支持的最大通道数*/
/*
 * 每通道块缓冲，攒满才写盘。块越大写盘次数越少、越不容易在 SD 停顿时
 * 把上游 cbuf 撑爆。缓冲是按需申请且只申请一次，不占 .bss，可以放心用大块。
 */
#define RAW_BLK_SIZE            (16 * 1024)
#define RAW_MAX_REC_DIR         1000            /*rec_NNN 目录序号上限*/

/*
 * V2 包头，对应发送端 aec_uart_debug_v2.c 的 data_export_header。
 *
 * 用显式 1 字节对齐(packed)加占位成员把 20 字节布局逐字节写死，
 * 不依赖编译器的对齐规则，解析外部数据格式时这样最可控。
 *
 * 需要注意 rsvd 并非协议字段: 发送端源码里只有 7 个成员、没有占位符，
 * 它那个结构体是自然对齐的，由编译器自行在 len 与 timestamp 之间留出
 * 2 字节空洞，使总长为 20 而非 18。发送端是 cbuf_write(header, 20)
 * 把包头连同这个空洞整块写进发送流的，所以线上格式确实是 20 字节，
 * 这里必须用 rsvd 占住，否则 timestamp/total_len 会整体前移 2 字节。
 *
 * packed 同时解决了非对齐访问: 包长 20+len 随 len 变化，包在字节流里的
 * 起始位置根本不保证对齐，CRC 校验失败时还会退 1 字节重找同步头。
 * packed 结构体对齐要求为 1，编译器会对多字节成员生成逐字节访问，
 * 避开 pi32v2 上的非对齐访问异常。
 *
 * 唯一未处理的是字节序: 按成员取值依赖本机小端。收发是同一颗芯片、
 * 同一编译器，pi32v2 恒为小端，故当前无需转换。若将来要移植到大端平台，
 * 把 raw_v2_check()/raw_v2_dispatch() 里对 magic/ch/crc/len/total_len
 * 的取值换成显式小端组装即可，布局声明不用动。
 */
struct raw_v2_hdr_t {
    u16 magic;              /*byte 0 : 固定 RAW_V2_MAGIC*/
    u16 ch;                 /*byte 2 : 通道号*/
    u16 seqn;               /*byte 4 : 该通道的包序号*/
    u16 crc;                /*byte 6 : CRC16(data, len)，不含包头*/
    u16 len;                /*byte 8 : data 字节数，每包可不同*/
    u16 rsvd;               /*byte 10: 占位，对应发送端的编译器对齐空洞，内容是未初始化的栈垃圾，不可读取*/
    u32 timestamp;          /*byte 12: 发送时刻的 jiffies*/
    u32 total_len;          /*byte 16: 该通道累计字节数，含本包*/
} __attribute__((packed));

_Static_assert(sizeof(struct raw_v2_hdr_t) == 20, "v2 header must be 20 bytes");

#define RAW_V2_MAGIC            0x2B5A
#define RAW_V2_HDR_LEN          ((u32)sizeof(struct raw_v2_hdr_t))

/*==== 基准参数: 只需按发送端实际配置调整这四个，下面的缓冲尺寸会自动跟随 ====*/
#define RAW_V2_MAX_DATA         10240           /*单包数据长度上限，防 len 字段是垃圾*/
#define RAW_V2_TYP_DATA         512             /*常见单包数据长度，仅用于估算提前判定门限，不影响正确性*/
#define RAW_UART_FRAME_MAX      2048            /*单次喂入的净载荷上限*/
#define RAW_SNIFF_NEED_PKT      4               /*连续命中这么多个包即认定 V2*/

/*==== 以下尺寸全部由上面推导得出，不要手改 ====*/
#define RAW_V2_PKT_MAX          (RAW_V2_HDR_LEN + RAW_V2_MAX_DATA)
#define RAW_V2_PKT_TYP          (RAW_V2_HDR_LEN + RAW_V2_TYP_DATA)
/*
 * 解析缓冲要装得下一个最大包再加一次喂入量。否则大包会永远判为数据不足，
 * pos 不前进，缓冲塞满后反复 resync，一个包都解析不出来。
 */
#define RAW_PARSE_BUF_SIZE      (RAW_V2_PKT_MAX + RAW_UART_FRAME_MAX)
/*嗅探缓冲要装得下 NEED_PKT 个最大包，否则大包场景凑不够连击会误判成 V1*/
#define RAW_SNIFF_BUF_SIZE      (RAW_SNIFF_NEED_PKT * RAW_V2_PKT_MAX + RAW_UART_FRAME_MAX)
/*典型包长下凑够连击所需的量，用于尽早判定，不必等缓冲填满*/
#define RAW_SNIFF_MIN_LEN       (RAW_SNIFF_NEED_PKT * RAW_V2_PKT_TYP)

/*派生量已保证下面几条恒成立，留着是防止有人把它们改回硬编码值*/
_Static_assert(RAW_PARSE_BUF_SIZE >= RAW_V2_PKT_MAX + RAW_UART_FRAME_MAX,
               "parse buf too small for max packet");
_Static_assert(RAW_SNIFF_BUF_SIZE >= RAW_SNIFF_NEED_PKT * RAW_V2_PKT_MAX,
               "sniff buf too small to detect large packets");
_Static_assert(RAW_SNIFF_MIN_LEN <= RAW_SNIFF_BUF_SIZE,
               "sniff min len exceeds buf size");

#define V2_CHK_INCOMPLETE       (-1)            /*数据不足，无法判定*/

struct raw_ch_t {
    FILE *fp;
    u8   *blk;              /*块缓冲*/
    u32   blk_used;
    u32   written;          /*已提交的字节数(含补零)，V2 对齐用*/
};

struct raw_writer_t {
    u8   fmt;
    u8   ch_cnt;
    u8   v1_ch;
    u16  v1_single;
    u32  v1_pos;            /*当前在 V1 交织块内的字节偏移*/
    char dir[64];

    struct raw_ch_t ch[RAW_MAX_CH];

    u8  *sniff_buf;
    u32  sniff_len;

    u8  *parse_buf;
    u32  parse_len;

    u32  err_cnt;
};

static struct raw_writer_t *s_rw = NULL;

/*
 * 句柄与两个大缓冲一律静态分配，不走 zalloc/free。
 *
 * 它们的大小都是编译期固定的，却会随每次插拔反复申请释放。SD 卡插拔频繁时
 * 这会把内存切碎，之后 SD 驱动申请 DMA 缓冲就可能落到非物理连续区，
 * 在 sdx_source.c 里断言 "sdx dat dma memory not in phy_memory" 直接死机
 * (实测连续插拔数次必现)。静态分配后内存布局恒定，不再产生碎片。
 */
static struct raw_writer_t s_rw_inst;

/*
 * 大缓冲仍从 heap 申请，但只申请一次、之后永不释放。
 *
 * 不能改成静态数组: 静态数组占的是 .bss，而 .bss 排在 data_code 之前，
 * 一旦变大会把整个 RAM 布局往后推、挤掉物理内存池，SD 驱动就拿不到
 * 位于 phy 区的 DMA 缓冲，照样断言 "sdx dat dma memory not in phy_memory"。
 * 而"只申请一次"同样能消除插拔反复 zalloc/free 造成的碎片。
 */
static u8 *s_sniff_buf = NULL;
static u8 *s_parse_buf = NULL;
static u8 *s_ch_blk[RAW_MAX_CH] = { NULL };

/*----------------------------------- 通道文件 -----------------------------------*/

/*按需建立通道文件与块缓冲。V2 的通道数由包头决定，只能用到才建*/
static struct raw_ch_t *raw_ch_get(u8 idx)
{
    struct raw_writer_t *rw = s_rw;
    struct raw_ch_t *c;
    char path[80];

    if (!rw || (idx >= RAW_MAX_CH)) {
        return NULL;
    }
    c = &rw->ch[idx];
    if (c->fp) {
        return c;
    }

    /*首次用到该通道时申请，之后跨插拔一直复用，不再释放*/
    if (!s_ch_blk[idx]) {
        s_ch_blk[idx] = zalloc(RAW_BLK_SIZE);
        if (!s_ch_blk[idx]) {
            printf("[raw] ch%d blk alloc fail\n", idx);
            return NULL;
        }
    }
    c->blk = s_ch_blk[idx];
    c->blk_used = 0;
    sprintf(path, "%s/%d.raw", rw->dir, idx);
    c->fp = fopen(path, "w+");
    if (!c->fp) {
        printf("[raw] open %s fail\n", path);
        c->blk = NULL;
        return NULL;
    }
    rw->ch_cnt++;
    printf("[raw] create %s\n", path);
    return c;
}

static void raw_ch_flush(struct raw_ch_t *c)
{
    int wlen;

    if (!c->fp || !c->blk || (c->blk_used == 0)) {
        return;
    }
    wlen = fwrite(c->blk, c->blk_used, 1, c->fp);
    if (wlen != (int)c->blk_used) {
        printf("[raw] sd write err %d/%d\n", wlen, c->blk_used);
        if (s_rw) {
            s_rw->err_cnt++;
        }
    }
    c->blk_used = 0;
}

/*
 * 往通道追加数据。pad 为真时写入零而忽略 data，
 * 用于丢包补位，两者的缓冲/落盘路径完全一致。
 */
static void raw_ch_put(u8 idx, const u8 *data, u32 len, u8 pad)
{
    struct raw_ch_t *c = raw_ch_get(idx);
    u32 space, n;

    if (!c) {
        return;
    }
    while (len) {
        space = RAW_BLK_SIZE - c->blk_used;
        n = (len < space) ? len : space;
        if (pad) {
            memset(&c->blk[c->blk_used], 0, n);
        } else {
            memcpy(&c->blk[c->blk_used], data, n);
            data += n;
        }
        c->blk_used += n;
        c->written  += n;
        len -= n;
        if (c->blk_used >= RAW_BLK_SIZE) {
            raw_ch_flush(c);
        }
    }
}

/*----------------------------------- V1 定长交织 -----------------------------------*/

/*
 * V1 无自描述信息，通道归属完全由字节在交织块内的位置决定，
 * 所以 v1_pos 必须跨帧连续累加。丢帧由上游往 cbuf 补等长的零来维持对齐，
 * 这里只管按位置切分。
 */
static void raw_v1_input(const u8 *data, u32 len)
{
    struct raw_writer_t *rw = s_rw;
    u32 blk = (u32)rw->v1_ch * rw->v1_single;
    u8  idx;
    u32 off, n;

    if (blk == 0) {
        return;
    }
    while (len) {
        idx = (u8)(rw->v1_pos / rw->v1_single);
        off = rw->v1_pos % rw->v1_single;
        n   = rw->v1_single - off;      /*本通道还差多少字节凑满*/
        if (n > len) {
            n = len;
        }
        raw_ch_put(idx, data, n, 0);
        data += n;
        len  -= n;
        rw->v1_pos = (rw->v1_pos + n) % blk;
    }
}

/*----------------------------------- V2 包流 -----------------------------------*/

/*
 * 判定 buf 处是否为合法 V2 包
 * @return >0 包总长(含包头)，0 不合法，V2_CHK_INCOMPLETE 数据不足
 */
static int raw_v2_check(const u8 *buf, u32 avail)
{
    const struct raw_v2_hdr_t *h = (const struct raw_v2_hdr_t *)buf;

    if (avail < RAW_V2_HDR_LEN) {
        return V2_CHK_INCOMPLETE;
    }
    if (h->magic != RAW_V2_MAGIC) {
        return 0;
    }
    if ((h->len == 0) || (h->len > RAW_V2_MAX_DATA)) {
        return 0;
    }
    if (avail < RAW_V2_HDR_LEN + h->len) {
        return V2_CHK_INCOMPLETE;
    }
    /*CRC 只覆盖数据区不含包头，与发送端 CRC16(buf, size) 一致*/
    if (CRC16(&buf[RAW_V2_HDR_LEN], h->len) != h->crc) {
        return 0;
    }
    return (int)(RAW_V2_HDR_LEN + h->len);
}

static void raw_v2_dispatch(const u8 *pkt)
{
    struct raw_writer_t *rw = s_rw;
    const struct raw_v2_hdr_t *h = (const struct raw_v2_hdr_t *)pkt;
    struct raw_ch_t *c;
    u32 expect;

    if (h->ch >= RAW_MAX_CH) {
        rw->err_cnt++;
        return;
    }
    c = raw_ch_get((u8)h->ch);
    if (!c) {
        return;
    }
    /*
     * total_len 是含本包的通道累计长度，故本包应落在 total_len-len 处。
     * 出现正向缺口即中途丢了包，补零保持各通道时间轴对齐，
     * 否则做延时/回声分析时通道之间会整体错位。
     */
    expect = h->total_len - h->len;
    if (expect > c->written) {
        raw_ch_put((u8)h->ch, NULL, expect - c->written, 1);
        rw->err_cnt++;
    }
    raw_ch_put((u8)h->ch, &pkt[RAW_V2_HDR_LEN], h->len, 0);
}

/*
 * V2 的包长是 20+len，len 由发送端每次 fill 时给定、包与包之间可以不同，
 * 而传输层是按固定长度分片发出的，两者没有任何对齐关系，包头随时可能跨帧。
 * 所以只能维护跨帧的解析缓冲做流式拆包，不能按帧边界假设包边界。
 */
static void raw_v2_input(const u8 *data, u32 len)
{
    struct raw_writer_t *rw = s_rw;
    u32 space, n, pos;
    int ret;

    while (len) {
        space = RAW_PARSE_BUF_SIZE - rw->parse_len;
        if (space == 0) {
            /*塞满了还拆不出包，说明流已失步，丢弃重新找同步头*/
            printf("[raw] v2 resync\n");
            rw->err_cnt++;
            rw->parse_len = 0;
            continue;
        }
        n = (len < space) ? len : space;
        memcpy(&rw->parse_buf[rw->parse_len], data, n);
        rw->parse_len += n;
        data += n;
        len  -= n;

        pos = 0;
        while (pos < rw->parse_len) {
            ret = raw_v2_check(&rw->parse_buf[pos], rw->parse_len - pos);
            if (ret == V2_CHK_INCOMPLETE) {
                break;                  /*等更多数据再判*/
            }
            if (ret > 0) {
                raw_v2_dispatch(&rw->parse_buf[pos]);
                pos += (u32)ret;
            } else {
                pos++;                  /*不是包头，右移一字节继续找*/
            }
        }
        if (pos) {
            rw->parse_len -= pos;
            if (rw->parse_len) {
                memcpy(rw->parse_buf, &rw->parse_buf[pos], rw->parse_len);
            }
        }
    }
}

/*----------------------------------- 格式嗅探 -----------------------------------*/

/*
 * 统计嗅探缓冲里首尾相接、CRC 全部正确的最长连续包数。
 * 单包 CRC 撞中的概率是 1/65536，连续多包相乘后
 * 足以排除 V1 音频数据里偶然出现 0x2B5A 字节对造成的误判。
 * magic 不匹配时 raw_v2_check 立即返回，不会真去算 CRC，故整体开销很低。
 */
static u8 raw_sniff_max_run(void)
{
    struct raw_writer_t *rw = s_rw;
    u32 start, pos;
    u8  hit, best = 0;
    int ret;

    for (start = 0; start + RAW_V2_HDR_LEN <= rw->sniff_len; start++) {
        pos = start;
        hit = 0;
        while (hit < RAW_SNIFF_NEED_PKT) {
            ret = raw_v2_check(&rw->sniff_buf[pos], rw->sniff_len - pos);
            if (ret <= 0) {
                break;
            }
            pos += (u32)ret;
            hit++;
        }
        if (hit > best) {
            best = hit;
            if (best >= RAW_SNIFF_NEED_PKT) {
                printf("[raw] sniff: V2 run %d at offset %d\n", best, start);
                break;
            }
        }
    }
    return best;
}

static void raw_sniff_input(const u8 *data, u32 len)
{
    struct raw_writer_t *rw = s_rw;
    u8  *buf;
    u32  blen, space, n;
    u8   run;

    space = RAW_SNIFF_BUF_SIZE - rw->sniff_len;
    n = (len < space) ? len : space;
    if (n) {
        memcpy(&rw->sniff_buf[rw->sniff_len], data, n);
        rw->sniff_len += n;
    }

    if (rw->sniff_len < RAW_SNIFF_MIN_LEN) {
        return;
    }
    run = raw_sniff_max_run();
    if (run >= RAW_SNIFF_NEED_PKT) {
        rw->fmt = RAW_FMT_V2;
    } else if (rw->sniff_len < RAW_SNIFF_BUF_SIZE) {
        return;                         /*还没满，再多攒一点看能不能凑够连击*/
    } else if (run) {
        /*
         * 缓冲已满却凑不够连击，说明单包很大、装不下更多个。
         * 此时哪怕只命中一个包，其 CRC 正确也已是够强的证据。
         */
        rw->fmt = RAW_FMT_V2;
        printf("[raw] sniff: V2 by short run %d\n", run);
    } else {
        rw->fmt = RAW_FMT_V1;
        printf("[raw] sniff: V1 assumed, ch %d len %d\n", rw->v1_ch, rw->v1_single);
    }
    oled_dispaly_task_post(OLED_DISPLAY_FMT, (int *)((int)rw->fmt));

    /*嗅探期暂存的数据要按判定结果全部回灌，一个字节都不能丢*/
    buf  = rw->sniff_buf;
    blen = rw->sniff_len;
    rw->sniff_buf = NULL;
    rw->sniff_len = 0;
    if (rw->fmt == RAW_FMT_V2) {
        raw_v2_input(buf, blen);
    } else {
        raw_v1_input(buf, blen);
    }
    /*sniff_buf 是静态数组，回灌完只解除引用，不能 free*/

    /*本帧没能塞进嗅探缓冲的尾巴，此时格式已定，按新路径直接处理*/
    if (n < len) {
        raw_writer_input(&data[n], len - n);
    }
}

/*----------------------------------- 对外接口 -----------------------------------*/

enum {
    RAW_DIR_UNUSED = 0,     /*没有任何 .raw，该序号没用过*/
    RAW_DIR_EMPTY,          /*有 .raw 但全是 0 字节，可以覆盖*/
    RAW_DIR_USED,           /*有非空 .raw，里面是有效数据*/
};

/*
 * 判断 rec_NNN 目录的占用情况。
 * 必须遍历所有通道号而不能只看 0.raw: V2 的通道号来自包头、不保证从 0 开始，
 * 只探测 0.raw 会把只含 1.raw 的目录误判成未使用，进而覆盖掉有效数据。
 * 遇到第一个非空文件就返回，所以正常目录通常只需一次 fopen。
 */
static u8 raw_dir_state(const char *dir)
{
    char probe[80];
    FILE *f;
    u8 i, found = 0;

    for (i = 0; i < RAW_MAX_CH; i++) {
        sprintf(probe, "%s/%d.raw", dir, i);
        f = fopen(probe, "r");
        if (!f) {
            continue;
        }
        found = 1;
        if (flen(f) != 0) {
            fclose(f);
            return RAW_DIR_USED;
        }
        fclose(f);
    }
    return found ? RAW_DIR_EMPTY : RAW_DIR_UNUSED;
}

/*
 * 取一个可用的 rec_NNN 目录。
 * 全是 0 字节的目录直接复用而不递增序号 —— 文件建好后要攒满一个块才落盘，
 * 这期间断电或拔卡就会留下一堆 0 字节文件，不复用的话会白占序号。
 * 这与改造前 audio_dbg_file_open() 复用 0 字节 bin 的行为一致。
 */
static int raw_pick_dir(struct raw_writer_t *rw, const char *root_path, const char *folder)
{
    int i;
    u8  st;

    for (i = 0; i < RAW_MAX_REC_DIR; i++) {
        sprintf(rw->dir, "%s%s/rec_%03d", root_path, folder, i);
        st = raw_dir_state(rw->dir);
        if (st == RAW_DIR_UNUSED) {
            printf("[raw] use new dir %s\n", rw->dir);
            return 0;
        }
        if (st == RAW_DIR_EMPTY) {
            printf("[raw] reuse empty dir %s\n", rw->dir);
            return 0;
        }
    }
    printf("[raw] no free rec dir\n");
    return -1;
}

int raw_writer_open(const char *root_path, const char *folder,
                    u8 v1_channel, u16 v1_single_size)
{
    struct raw_writer_t *rw;

    if (s_rw) {
        return -1;
    }
    if (!root_path || !folder || (v1_channel == 0) ||
        (v1_channel > RAW_MAX_CH) || (v1_single_size == 0)) {
        printf("[raw] open bad param\n");
        return -1;
    }

    /*两个大缓冲只在首次 open 时申请，之后跨插拔复用，插拔过程不再动 heap*/
    if (!s_sniff_buf) {
        s_sniff_buf = zalloc(RAW_SNIFF_BUF_SIZE);
    }
    if (!s_parse_buf) {
        s_parse_buf = zalloc(RAW_PARSE_BUF_SIZE);
    }
    if (!s_sniff_buf || !s_parse_buf) {
        printf("[raw] buf alloc fail\n");
        return -1;
    }

    rw = &s_rw_inst;
    memset(rw, 0, sizeof(*rw));
    rw->fmt       = RAW_FMT_SNIFFING;
    rw->v1_ch     = v1_channel;
    rw->v1_single = v1_single_size;
    rw->sniff_buf = s_sniff_buf;
    rw->parse_buf = s_parse_buf;

    if (raw_pick_dir(rw, root_path, folder) < 0) {
        return -1;
    }
    s_rw = rw;
    return 0;
}

void raw_writer_input(const u8 *data, u32 len)
{
    struct raw_writer_t *rw = s_rw;

    if (!rw || !data || (len == 0)) {
        return;
    }
    switch (rw->fmt) {
    case RAW_FMT_SNIFFING:
        raw_sniff_input(data, len);
        break;
    case RAW_FMT_V1:
        raw_v1_input(data, len);
        break;
    case RAW_FMT_V2:
        raw_v2_input(data, len);
        break;
    default:
        break;
    }
}

void raw_writer_flush(void)
{
    struct raw_writer_t *rw = s_rw;
    u8 i;

    if (!rw) {
        return;
    }
    /*raw_ch_flush 内部会跳过没有残留数据的通道，不会产生多余的写盘*/
    for (i = 0; i < RAW_MAX_CH; i++) {
        raw_ch_flush(&rw->ch[i]);
    }
}

void raw_writer_close(u8 flush)
{
    struct raw_writer_t *rw = s_rw;
    struct raw_ch_t *c;
    u8 i;

    if (!rw) {
        return;
    }
    for (i = 0; i < RAW_MAX_CH; i++) {
        c = &rw->ch[i];
        if (c->fp) {
            /*
             * 卡被拔掉时调用方会传 flush=0: 挂载点已经失效，
             * 这一整块 fwrite 写不进去，还可能卡在 SD 驱动里。
             * fclose 仍要调用，否则 FILE 句柄泄漏。
             */
            if (flush) {
                raw_ch_flush(c);        /*落盘残留，否则尾部数据会丢*/
            }
            fclose(c->fp);
            c->fp = NULL;
        }
        c->blk = NULL;      /*静态数组，只解除引用*/
    }
    /*句柄与这两个缓冲都是静态的，只解除引用，不能 free*/
    rw->sniff_buf = NULL;
    rw->parse_buf = NULL;
    s_rw = NULL;
    printf("[raw] closed\n");
}

u8 raw_writer_get_fmt(void)
{
    return s_rw ? s_rw->fmt : RAW_FMT_SNIFFING;
}

u8 raw_writer_get_ch_cnt(void)
{
    return s_rw ? s_rw->ch_cnt : 0;
}

u32 raw_writer_get_err(void)
{
    return s_rw ? s_rw->err_cnt : 0;
}
