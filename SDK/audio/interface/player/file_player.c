#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".file_player.data.bss")
#pragma data_seg(".file_player.data")
#pragma const_seg(".file_player.text.const")
#pragma code_seg(".file_player.text")
#endif
#include "file_player.h"
#include "fs/resfile.h"
#include "os/os_api.h"
#include "system/init.h"
#include "system/includes.h"
#include "app_config.h"
#include "fs/fs.h"
#include "sdk_config.h"
#include "effects/audio_pitchspeed.h"
#include "audio_decoder.h"
#include "jldemuxer.h"
#include "clock_manager/clock_manager.h"
#include "audio_config_def.h"
#include "effects/audio_vbass.h"
#include "framework/include/decoder_node.h"
#include "le_audio_player.h"

#if TCFG_APP_MUSIC_EN

struct music_file_player_hdl {
    u8 player_id ;
    OS_MUTEX mutex;
    struct list_head head;
#if FILE_DEC_DEST_PLAY || FILE_DEC_REPEAT_EN
    struct file_player *cur_player ; //当前的音乐播放器句柄
#endif
};

static struct music_file_player_hdl g_file_player;

static int music_file_player_start(struct file_player *);
const struct stream_file_ops music_file_ops;

__attribute__((weak))
void music_event_to_user(int event, u16 fname_uuid)
{

}

void music_player_free(struct file_player *player)
{
    if (--player->ref == 0) {
        if (player->break_point_flag == 1) {
            free(player->break_point);
            player->break_point = NULL;
            player->break_point_flag = 0;
        }
        free(player);
#if FILE_DEC_DEST_PLAY || FILE_DEC_REPEAT_EN
        g_file_player.cur_player = NULL;
#endif
    }
}
extern FILE *music_player_get_file_hdl(void);
extern void music_player_remove_file_hdl(void);

extern int music_player_play_auto_next(void);
static void music_player_callback(void *_player_id, int event)
{
    struct file_player *player;

    printf("music_callback: %x, %d\n", event, (u8)_player_id);

    switch (event) {
    case STREAM_EVENT_START:
#if AUDIO_VBASS_LINK_VOLUME
        vbass_link_volume();
#endif
        if (list_empty(&(g_file_player.head))) {          //先判断是否为空防止触发异常
            break;
        }
        os_mutex_pend(&g_file_player.mutex, 0);
        player = list_first_entry(&(g_file_player.head), struct file_player, entry);
        if (player->player_id != (u8)_player_id) {
            os_mutex_post(&g_file_player.mutex);
            printf("player_id_not_match: %d\n", player->player_id);
            break;
        }
        os_mutex_post(&g_file_player.mutex);
        if (player->callback) {
            player->callback(player->priv, 0, STREAM_EVENT_START);
        }
        break;
    case STREAM_EVENT_PREEMPTED:
        break;
    case STREAM_EVENT_NEGOTIATE_FAILD:
    case STREAM_EVENT_STOP:
        if (list_empty(&(g_file_player.head))) {          //先判断是否为空防止触发异常
            break;
        }
        os_mutex_pend(&g_file_player.mutex, 0);
        player = list_first_entry(&(g_file_player.head), struct file_player, entry);
        if (player->player_id != (u8)_player_id) {
            os_mutex_post(&g_file_player.mutex);
            printf("player_id_not_match: %d\n", player->player_id);
            break;
        }
        //list_del(&player->entry);
        //jlstream_release(player->stream);

        //if (!list_empty(&(g_file_player.head))) {
        //    struct file_player *p = list_first_entry(&(g_file_player.head), struct file_player, entry);
        //    music_file_player_start(p);
        //}
        os_mutex_post(&g_file_player.mutex);
        if (player->callback) {
            player->callback(player->priv, player->read_err, STREAM_EVENT_STOP);

        }
        //music_player_free(player);
        /* int err = music_player_play_auto_next(); */

        break;
    }
}

static int music_file_read(void *file, u8 *buf, int len)
{
    int offset = 0;
    struct file_player *player = (struct file_player *)file;

    while (len) {
        if (!player->file) {
            break;
        }

        int rlen = 0;

#if TCFG_DEC_DECRYPT_ENABLE
        u32 addr;
        addr = ftell(player->file);
        rlen = fread(buf + offset, len, 1, player->file);
        if ((rlen > 0) && (rlen <= len)) {
            cryptanalysis_buff(&player->mply_cipher, buf + offset, addr, rlen); //解密了
        }
#else
        rlen = fread(buf + offset, len, 1, player->file);
#endif


        if (rlen < 0 || rlen == 0) {
            if (rlen == (-1)) {
                player->read_err  = 1; //file error
            } else {
                if (rlen != 0) {
                    player->read_err  = 2;    //disk error
                    return -1; //拔卡读不到数
                }
            }
            break;
        }
        player->read_err  = 0;
        offset += rlen;
        if ((len -= rlen) == 0) {
            break;
        }
    }

    return offset;
}

static int music_file_seek(void *file, int offset, int fromwhere)
{
    struct file_player *player = (struct file_player *)file;
    return fseek(player->file, offset, fromwhere);
}

int music_file_flen(void *file)
{
    struct file_player *player = (struct file_player *)file;
    u32 len = 0;
    if (player->file) {
        len = flen(player->file);
    }
    return len;
}

static int music_file_close(void *file)
{
    struct file_player *player = (struct file_player *)file;

    if (player->file) {
        if (music_player_get_file_hdl()) {
            fclose(player->file);
            music_player_remove_file_hdl();
        }
        player->file = NULL;
    }
    return 0;
}

static int music_file_get_fmt(void *file, struct stream_fmt *fmt)
{
    u8 name[16];
    struct file_player *player = (struct file_player *)file;

    fget_name(player->file, name, 16);
    struct stream_file_info info = {
        .file = player,
        .fname = (char *)name,
        .ops  = &music_file_ops,
        .scene = player->scene,
    };
    int err = jldemuxer_get_tone_file_fmt(&info, fmt);

    return err;
}

const struct stream_file_ops music_file_ops = {
    .read       = music_file_read,
    .seek       = music_file_seek,
    .close      = music_file_close,
    .get_fmt    = music_file_get_fmt,
};


int music_file_player_pp(struct file_player *music_player)
{
    if (music_player && music_player->stream) {
        if (music_player->status == FILE_PLAYER_PAUSE) {
#if 0
            //时间戳使能，多设备播放才需要配置，需接入播放同步节点
            u32 timestamp = audio_jiffies_usec() + 30 * 1000;
            jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_TIME_STAMP, timestamp);
#endif
        }
        jlstream_pp_toggle(music_player->stream, 50);
        if (music_player->status != FILE_PLAYER_STOP) {
            music_player->status = ((music_player->status == FILE_PLAYER_START) ? FILE_PLAYER_PAUSE : FILE_PLAYER_START);
        }
        return 0;
    } else {
        return -1;
    }
}

int music_file_player_ff(u16 step_s, struct file_player *music_player)
{
    if (music_player && music_player->stream) {
        jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_FF, step_s);
        return 0;
    } else {
        return -1;
    }
}

int music_file_player_fr(u16 step_s, struct file_player *music_player)
{
    if (music_player && music_player->stream) {
        jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_FR, step_s);
        return 0;
    } else {
        return -1;
    }
}

int music_file_get_cur_time(struct file_player *music_player)
{
    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {          //先判断是否为空
        os_mutex_post(&g_file_player.mutex);
        return -1;
    }
    if (!music_player) {
        music_player = list_first_entry(&(g_file_player.head), struct file_player, entry);
    }
    if (music_player && music_player->stream) {
        int ret = jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_CUR_TIME, 0);
        os_mutex_post(&g_file_player.mutex);
        return ret;
    }
    os_mutex_post(&g_file_player.mutex);
    return -1;
}

int music_file_get_total_time(struct file_player *music_player)
{
    if (music_player && music_player->stream) {
        return jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_TOTAL_TIME, 0);
    }
    return -1;
}

int music_file_get_player_status(struct file_player *music_player)
{
    enum play_status status = FILE_PLAYER_STOP;  //播放结束
    if (!(list_empty(&(g_file_player.head))) && music_player && music_player->stream) {
        return music_player->status;
    }
    return  status;
}

//变调接口
int music_file_pitch_up(struct file_player *music_player)
{
    float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};
    music_player->music_pitch_mode ++;
    if (music_player->music_pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        music_player->music_pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    printf("play pitch up+++%d\n", music_player->music_pitch_mode);
    int ret = music_file_set_pitch(music_player, music_player->music_pitch_mode);
    ret = (ret == true) ? music_player->music_pitch_mode : -1;
    return ret;
}

int music_file_pitch_down(struct file_player *music_player)
{
    music_player->music_pitch_mode --;
    if (music_player->music_pitch_mode < 0) {
        music_player->music_pitch_mode = 0;
    }
    printf("play pitch down---%d\n", music_player->music_pitch_mode);
    int ret = music_file_set_pitch(music_player, music_player->music_pitch_mode);
    ret = (ret == true) ? music_player->music_pitch_mode : -1;
    return ret;
}

int music_file_set_pitch(struct file_player *music_player, enum _pitch_level pitch_mode)
{
    float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};
    if (pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    pitch_speed_param_tool_set pitch_param = {
        .pitch = pitch_param_table[pitch_mode],
        .speed = 1,
    };
    if (music_player) {
        music_player->music_pitch_mode = pitch_mode;
        return jlstream_node_ioctl(music_player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&pitch_param);
    }
    return -1;
}

int music_file_speed_up(struct file_player *music_player) //倍速播放接口
{
    float speed_param_table[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};
    music_player->music_speed_mode ++;
    if (music_player->music_speed_mode > ARRAY_SIZE(speed_param_table) - 1) {
        music_player->music_speed_mode = ARRAY_SIZE(speed_param_table) - 1;
    }

    printf("play speed up+++%d\n", music_player->music_speed_mode);
    int ret = music_file_set_speed(music_player, music_player->music_speed_mode);
    ret = (ret == true) ? music_player->music_speed_mode : -1;
    return ret;
}

int music_file_speed_down(struct file_player *music_player) //慢速播放接口
{
    music_player->music_speed_mode --;
    if (music_player->music_speed_mode < 0) {
        music_player->music_speed_mode = 0;
    }
    printf("play speed down---%d\n", music_player->music_speed_mode);
    int ret = music_file_set_speed(music_player, music_player->music_speed_mode);
    ret = (ret == true) ? music_player->music_speed_mode : -1;
    return ret;
}


int music_file_set_speed(struct file_player *music_player, enum _speed_level speed_mode) //设置播放速度
{
    float speed_param_table[] = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0};

    if (speed_mode > ARRAY_SIZE(speed_param_table) - 1) {
        speed_mode = ARRAY_SIZE(speed_param_table) - 1;
    }
    pitch_speed_param_tool_set speed_param = {
        .pitch = 0,
        .speed = speed_param_table[speed_mode],
    };
    if (music_player) {
        printf("set play speed ---%d, %d\n", music_player->music_speed_mode, (int)(speed_param_table[music_player->music_speed_mode] * 100));
        music_player->music_speed_mode = speed_mode;
        return jlstream_node_ioctl(music_player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&speed_param);
    }
    return -1;
}

#if FILE_DEC_AB_REPEAT_EN
#define AUDIO_AB_REPEAT_CODING_TYPE		(AUDIO_CODING_MP3 | AUDIO_CODING_WMA | AUDIO_CODING_WAV | AUDIO_CODING_FLAC | AUDIO_CODING_APE | AUDIO_CODING_DTS)
enum {
    AB_REPEAT_STA_NON = 0,
    AB_REPEAT_STA_ASTA,
    AB_REPEAT_STA_BSTA,
};

static u8  ab_repeat_status = AB_REPEAT_STA_NON;	// AB复读状态
/*----------------------------------------------------------------------------*/
/**@brief    设置AB点复读命令
   @param    ab_cmd: 命令
   @param    ab_mode: 参数
   @return   1: 设置成功 0:设置失败
   @note
*/
/*----------------------------------------------------------------------------*/
static int music_file_ab_repeat_set(int ab_cmd, int ab_mode, struct file_player *music_player)
{
    int err = false;
    if (!music_player) {
        return false;
    }
    printf("ab repat, cmd:0x%x, mode:%d \n", ab_cmd, ab_mode);
    struct audio_ab_repeat_mode_param rpt = {0};
    rpt.value = ab_mode;
    switch (ab_cmd) {
    case AUDIO_IOCTRL_CMD_SET_BREAKPOINT_A:
        printf(" SET BREAKPOINT A");
        err = jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_BP_A, (int)&rpt);
        break;
    case AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B:
        printf(" SET BREAKPOINT B");
        err = jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_BP_B, (int)&rpt);
        break;
    case AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE:
        printf(" CANCEL AB REPEAT");
        err = jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_SET_AB_REPEAT, (int)&rpt);
        break;
    default:
        break;
    }
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    切换AB点复读状态
   @param
   @return   true:成功
   @note
*/
/*----------------------------------------------------------------------------*/
int music_file_ab_repeat_switch(struct file_player *music_player)
{
    if (!music_player) {
        return false;
    }
    switch (music_player->ab_repeat_status) {
    case AB_REPEAT_STA_NON:
        if (music_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_A, 0, music_player)) {
            music_player->ab_repeat_status = AB_REPEAT_STA_ASTA;
        }
        break;
    case AB_REPEAT_STA_ASTA:
        if (music_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B, 0, music_player)) {
            music_player->ab_repeat_status = AB_REPEAT_STA_BSTA;
        }
        break;
    case AB_REPEAT_STA_BSTA:
        if (music_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE, AB_REPEAT_MODE_CUR, music_player)) {
            music_player->ab_repeat_status = AB_REPEAT_STA_NON;
        }
        break;
    }
    printf("music_file_ab_repeat_switch = %d\n", music_player->ab_repeat_status);
    return true;
}

/*----------------------------------------------------------------------------*/
/**@brief    关闭AB点复读
   @param
   @return   true:成功
   @note
*/
/*----------------------------------------------------------------------------*/
int music_file_ab_repeat_close(struct file_player *music_player)
{
    if (!music_player) {
        return false;
    }

    if (music_player->ab_repeat_status == AB_REPEAT_STA_NON) {
        return true;
    }

    if (music_player->ab_repeat_status == AB_REPEAT_STA_ASTA) {
        struct stream_fmt fmt;
        jlstream_node_ioctl(music_player->stream, NODE_UUID_SOURCE, NODE_IOC_GET_FMT, (int)&fmt);
        switch (fmt.coding_type) {
        case AUDIO_CODING_FLAC:
        case AUDIO_CODING_DTS:
        case AUDIO_CODING_APE:
            music_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B, 0, music_player);
            break;
        }
    }

    music_file_ab_repeat_set(AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE, AB_REPEAT_MODE_CUR, music_player);
    music_player->ab_repeat_status = AB_REPEAT_STA_NON;
    return true;
}

#endif /*FILE_DEC_AB_REPEAT_EN*/


int music_file_get_breakpoints(struct audio_dec_breakpoint *bp, struct file_player *music_player)
{
    if (music_player) {
        return jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_GET_BP, (int)bp);
    }
    return -1;
}

#if FILE_DEC_REPEAT_EN //无缝循环播放
/*----------------------------------------------------------------------------*/
/**@brief    循环播放回调接口
   @param    *priv: 私有参数
   @return   0：循环播放
   @return   非0：结束循环
   @note
*/
/*----------------------------------------------------------------------------*/
static int file_dec_repeat_cb(void *priv)
{
    struct file_player *music_player = priv;
    y_printf("file_dec_repeat_cb\n");
    if (music_player->repeat_num) {
        music_player->repeat_num--;
    } else {
        printf("file_dec_repeat_cb end\n");
        return -1;
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    设置循环播放次数
   @param    repeat_num: 循环次数
   @return   true：成功
   @return   false：失败
   @note
*/
/*----------------------------------------------------------------------------*/
static	struct audio_repeat_mode_param rep = {0};
int file_dec_repeat_set(u8 repeat_num, u32 coding_type)
{
    struct file_player *music_player = g_file_player.cur_player; //当前的音乐播放器句柄
    if (!music_player) {
        return false;
    }

    switch (music_player->stream->coding_type) {
    case AUDIO_CODING_MP3:
    case AUDIO_CODING_WAV: {
        music_player->repeat_num = repeat_num;
        /* struct audio_repeat_mode_param rep = {0}; */
        rep.flag = 1; //使能
        rep.headcut_frame = 2; //依据需求砍掉前面几帧，仅mp3格式有效
        rep.tailcut_frame = 2; //依据需求砍掉后面几帧，仅mp3格式有效
        rep.repeat_callback = file_dec_repeat_cb;
        rep.callback_priv = music_player;
        rep.repair_buf = &music_player->repair_buf;
        jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_REPEAT, (int)&rep);
        /* audio_decoder_ioctrl(&dec->file_dec.decoder, AUDIO_IOCTRL_CMD_REPEAT_PLAY, &rep); */
    }
    return true;
    }
    return false;
}
#endif


#if FILE_DEC_DEST_PLAY
/*----------------------------------------------------------------------------*/
/**@brief    跳到指定位置开始播放，播放到目标时间后回调
   @param    start_time: 要跳转过去播放的起始时间
   @param    dest_time: 要跳转过去播放的目标时间
   @param    *cb: 到达目标时间后回调
   @param    *cb_priv: 回调参数
   @return   true：成功
   @return   false：失败
   @note
*/
/*----------------------------------------------------------------------------*/
struct audio_dest_time_play_param param = {0};
int file_dec_set_start_dest_play(u32 start_time, u32 dest_time, u32(*cb)(void *), void *cb_priv, u32 coding_type, struct file_player *music_player)
{
    if (!music_player) {
        return false;
    }
    switch (coding_type) {
    case AUDIO_CODING_MP3: {
        /* struct audio_dest_time_play_param param = {0}; */
        param.start_time = start_time;
        param.dest_time = dest_time;
        param.callback_func = cb;
        param.callback_priv = cb_priv;
        jlstream_node_ioctl(music_player->stream, NODE_UUID_DECODER, NODE_IOC_DECODER_DEST_PLAY, (int)&param);
    }
    return true;
    }
    return false;
}

/*----------------------------------------------------------------------------*/
/**@brief    跳到指定位置开始播放
   @param    start_time: 要跳转过去播放的起始时间
   @return   true：成功
   @return   false：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int file_dec_set_start_play(u32 start_time, u32 coding_type)
{
    return file_dec_set_start_dest_play(start_time, 0x7fffffff, NULL, NULL, g_file_player.cur_player->stream->coding_type, g_file_player.cur_player);
}
#endif


#if TCFG_DEC_ID3_V1_ENABLE || TCFG_DEC_ID3_V2_ENABLE
static int file_name_match_mp3(char *file_name)
{
    const char *ext_name;
    for (int i = 0; file_name[i] != '\0'; i++) {
        if (file_name[i] == '.') {
            ext_name = file_name + i + 1;
            goto __match;
        }
    }
    return 0;
__match:
    if (!strncasecmp(ext_name, "mp3", 3)) {
        return 1;
    }
    return 0;
}
#endif

/*----------------------------------------------------------------------------*/
/**@brief    获取id3信息
   @return   true：成功
   @return   false：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int file_dec_id3_post(struct file_player *player)
{
    if (!player) {
        return false;
    }
    u32 id3_buf[8 + 13 + 8];
    struct id3_info info = {0};
    info.set.ptr = id3_buf;
    info.set.max_len = sizeof(id3_buf);
    info.set.n_items = 6;
    info.set.item_limit_len = 64;
    info.set.frame_id_att[0] = 0x3f; // 0x3E;  0x3f;
    int ret = jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_GET_ID3, (int)&info);
    if (ret == 0) {
        for (int i = 0; i < info.set.n_items; i++) {
            if ((info.data.len_list[i] != 0) && info.data.mptr[i] != 0) {
                printf("%d\t :  %s\n", i, info.data.mptr[i]);        //显示字符串.
                put_buf(info.data.mptr[i], info.data.len_list[i]);  //HEX
            }
        }
    }
    return true;
}

static int music_file_player_start(struct file_player *player)
{
    int err = -EINVAL;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"music");

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_MUSIC);
    if (!player->stream) {
        goto __exit0;
    }
    int player_id = player->player_id;
    jlstream_set_callback(player->stream, (void *)player_id, music_player_callback);
    jlstream_set_scene(player->stream, player->scene);
    jlstream_set_coexist(player->stream, player->coexist);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_BP, (int)player->break_point);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_FILE_LEN, (int)music_file_flen(player));

#if 0
    //时间戳使能，多设备播放才需要配置，需接入播放同步节点
    u32 timestamp = audio_jiffies_usec() + 30 * 1000;
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_SET_TIME_STAMP, timestamp);
#endif

    if (player->callback) {
        err = player->callback(player->priv, 0, STREAM_EVENT_INIT);
        if (err) {
            goto __exit1;
        }
    }

    jlstream_set_dec_file(player->stream, player, &music_file_ops);

#if TCFG_DEC_ID3_V1_ENABLE || TCFG_DEC_ID3_V2_ENABLE
    u8 name[12 + 1] = {0}; //8.3+\0
    fget_name(player->file, name, sizeof(name));
    if (file_name_match_mp3((char *)name)) {
#if TCFG_DEC_ID3_V1_ENABLE
        if (player->p_mp3_id3_v1) {
            id3_obj_post(&player->p_mp3_id3_v1);
        }
        player->p_mp3_id3_v1 = id3_v1_obj_get(player->file);
#endif
#if TCFG_DEC_ID3_V2_ENABLE
        if (player->p_mp3_id3_v2) {
            id3_obj_post(&player->p_mp3_id3_v2);
        }
        player->p_mp3_id3_v2 = id3_v2_obj_get(player->file);
#endif
    } else {
#if (TCFG_DEC_ID3_V1_ENABLE || TCFG_DEC_ID3_V2_ENABLE)
        file_dec_id3_post(player);
#endif
    }
#endif
    err = jlstream_start(player->stream);

    if (err) {
        goto __exit1;
    }

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    list_del(&player->entry);

    if (player->callback) {
        /* err = player->callback(player->priv, 0,STREAM_EVENT_NONE); */
    }
    music_player_free(player);
    return err;
}




int music_player_init(struct file_player *player, void *file, struct audio_dec_breakpoint *dbp)
{
    player->ref         = 1;
    if (!file) {
        printf("music_player_faild\n");
        return -EINVAL;
    }
    player->file        = file;
    player->scene       = STREAM_SCENE_MUSIC;
    player->player_id   = g_file_player.player_id++;
    player->coexist     = STREAM_COEXIST_AUTO;

    if (dbp == NULL) {
        player->break_point = zalloc(sizeof(struct audio_dec_breakpoint) + BREAKPOINT_DATA_LEN);
        player->break_point->data_len = BREAKPOINT_DATA_LEN;
        player->break_point_flag = 1;
    } else {
        player->break_point = dbp;
        player->break_point_flag = 0;
    }
    player->music_speed_mode  = PLAY_SPEED_1; //固定开始的时候使用1倍速播放 */
    player->music_pitch_mode = PITCH_0;  //固定开始时不变调
#if TCFG_DEC_DECRYPT_ENABLE
    cipher_init(&player->mply_cipher, TCFG_DEC_DECRYPT_KEY);
    void cipher_check_decode_file(CIPHER * pcipher, void *file);
    cipher_check_decode_file(&player->mply_cipher, file);
#endif


    INIT_LIST_HEAD(&player->entry);

    return 0;
}

static struct file_player *music_player_create(void *file, struct audio_dec_breakpoint *dbp)
{
    struct file_player *player;

    player = zalloc(sizeof(*player));
    if (!player) {
        return NULL;
    }

#if FILE_DEC_DEST_PLAY || FILE_DEC_REPEAT_EN
    g_file_player.cur_player = player;
#endif

    int err = music_player_init(player, file, dbp);
    if (err) {
        music_player_free(player);
        return NULL;
    }

    return player;
}

struct file_player *music_player_add(struct file_player *player)
{
    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {
        int err = music_file_player_start(player);
        if (err) {
            os_mutex_post(&g_file_player.mutex);
            return NULL;
        }
        player->status = FILE_PLAYER_START;
    }
    list_add_tail(&player->entry, &(g_file_player.head));
    os_mutex_post(&g_file_player.mutex);
    return player;
}

struct file_player *music_file_play(FILE *file, struct audio_dec_breakpoint *dbp)
{
    struct file_player *player;
    player = music_player_create(file, dbp);
    if (!player) {
        return NULL;
    }
    return music_player_add(player);
}

struct file_player *music_file_play_callback(FILE *file, void *priv, music_player_cb_t callback, struct audio_dec_breakpoint *dbp)
{
    struct file_player *player;
    player = music_player_create(file, dbp);
    if (!player) {
        return NULL;
    }
    player->priv        = priv;
    player->callback    = callback;
    return music_player_add(player);
}

int music_player_runing()
{
    local_irq_disable();
    int ret = list_empty(&(g_file_player.head)) ? 0 : 1;
    local_irq_enable();
    return ret;
}

void music_file_player_stop()
{
    struct file_player *player, *n;

    os_mutex_pend(&g_file_player.mutex, 0);

    list_for_each_entry_safe(player, n, &(g_file_player.head), entry) {
        __list_del_entry(&player->entry);
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && LEA_LOCAL_SYNC_PLAY_EN
        if (player->le_audio) {
            le_audio_player_close(player->le_audio);
        }
#endif
        if (player->stream) {
            jlstream_stop(player->stream, 50);
            jlstream_release(player->stream);
        }
#if TCFG_DEC_ID3_V1_ENABLE
        if (player->p_mp3_id3_v1) {
            id3_obj_post(&player->p_mp3_id3_v1);
        }
#endif
#if TCFG_DEC_ID3_V2_ENABLE
        if (player->p_mp3_id3_v2) {
            id3_obj_post(&player->p_mp3_id3_v2);
        }
#endif

        music_player_free(player);
    }

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"music");
    os_mutex_post(&g_file_player.mutex);
}

struct file_player *get_music_file_player(void) //返回第一个打开的音乐播放器指针
{
    struct file_player *player = NULL;
    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {          //先判断是否为空
        os_mutex_post(&g_file_player.mutex);
        return NULL;
    }
    player = list_first_entry(&(g_file_player.head), struct file_player, entry);
    os_mutex_post(&g_file_player.mutex);
    return player;
}


static int __music_player_init()
{
    INIT_LIST_HEAD(&g_file_player.head);
    os_mutex_create(&g_file_player.mutex);
    return 0;
}
__initcall(__music_player_init);

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
static int le_audio_music_file_player_start(struct file_player *player)
{
    int err = -EINVAL;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"music_le_audio");
    /* u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"music"); */

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_MUSIC);
    if (!player->stream) {
        goto __exit0;
    }
    int player_id = player->player_id;
    jlstream_set_callback(player->stream, (void *)player_id, music_player_callback);
    jlstream_set_scene(player->stream, player->scene);
    jlstream_set_coexist(player->stream, player->coexist);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_BP, (int)player->break_point);
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER,
                        NODE_IOC_SET_FILE_LEN, (int)music_file_flen(player));
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE,
                        NODE_IOC_SET_TASK, (int)"music_file");
    jlstream_node_ioctl(player->stream, NODE_UUID_LE_AUDIO_SOURCE, NODE_IOC_SET_BTADDR, (int)player->le_audio);
    jlstream_ioctl(player->stream, NODE_IOC_SET_ENC_FMT, (int)player->fmt);
#if 0
//时间戳使能，多设备播放才需要配置，需接入播放同步节点
    u32 timestamp = audio_jiffies_usec() + 30 * 1000;
    jlstream_node_ioctl(player->stream, NODE_UUID_DECODER, NODE_IOC_SET_TIME_STAMP, timestamp);
#endif

    if (player->callback) {
        err = player->callback(player->priv, 0, STREAM_EVENT_INIT);
        if (err) {
            goto __exit1;
        }
    }

    jlstream_set_dec_file(player->stream, player, &music_file_ops);
#if TCFG_DEC_ID3_V1_ENABLE || TCFG_DEC_ID3_V2_ENABLE
    u8 name[12 + 1] = {0}; //8.3+\0
    fget_name(player->file, name, sizeof(name));
    if (file_name_match_mp3((char *)name)) {
#if TCFG_DEC_ID3_V1_ENABLE
        if (player->p_mp3_id3_v1) {
            id3_obj_post(&player->p_mp3_id3_v1);
        }
        player->p_mp3_id3_v1 = id3_v1_obj_get(player->file);
#endif
#if TCFG_DEC_ID3_V2_ENABLE
        if (player->p_mp3_id3_v2) {
            id3_obj_post(&player->p_mp3_id3_v2);
        }
        player->p_mp3_id3_v2 = id3_v2_obj_get(player->file);
#endif
    }
#endif
    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    list_del(&player->entry);
    if (player->callback) {
        /* err = player->callback(player->priv, 0,STREAM_EVENT_NONE); */
    }
    music_player_free(player);
    return err;
}

struct file_player *le_audio_music_player_add(struct file_player *player)
{
    os_mutex_pend(&g_file_player.mutex, 0);
    if (list_empty(&(g_file_player.head))) {
        int err = le_audio_music_file_player_start(player);
        if (err) {
            os_mutex_post(&g_file_player.mutex);
            return NULL;
        }
#if LEA_LOCAL_SYNC_PLAY_EN
        if (player->le_audio) {
            err = le_audio_player_open(player->le_audio, NULL);
            if (err != 0) {
                ASSERT(0, "player open fail");
            }
        }
#endif
        player->status = FILE_PLAYER_START;
    }
    list_add_tail(&player->entry, &(g_file_player.head));
    os_mutex_post(&g_file_player.mutex);
    return player;
}

struct file_player *le_audio_music_file_play_callback(FILE *file, void *priv, music_player_cb_t callback, struct audio_dec_breakpoint *dbp, void *le_audio, void *fmt)
{
    struct file_player *player;
    player = music_player_create(file, dbp);
    if (!player) {
        return NULL;
    }
    player->priv        = priv;
    player->callback    = callback;
    player->le_audio    = le_audio;
    player->fmt         = fmt;
    return le_audio_music_player_add(player);
}
#endif
#endif

