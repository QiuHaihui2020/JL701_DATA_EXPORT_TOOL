#ifndef __MUSIC_PLAYER_H__
#define __MUSIC_PLAYER_H__

#include "system/includes.h"
#include "media/audio_decoder.h"
#include "dev_manager/dev_manager.h"
#include "file_operate/file_manager.h"
#include "file_player.h"


//解码错误码表
enum {
    MUSIC_PLAYER_ERR_NULL = 0x0,		//没有错误， 不需要做任何
    MUSIC_PLAYER_SUCC	  = 0x1,		//播放成功， 可以做相关显示、断点记忆等处理
    MUSIC_PLAYER_ERR_PARM,	    		//参数错误
    MUSIC_PLAYER_ERR_POINT,	    		//参数指针错误
    MUSIC_PLAYER_ERR_NO_RAM,			//没有ram空间错误
    MUSIC_PLAYER_ERR_DECODE_FAIL,		//解码器启动失败错误
    MUSIC_PLAYER_ERR_DEV_NOFOUND,		//没有找到指定设备
    MUSIC_PLAYER_ERR_DEV_OFFLINE,		//设备不在线错误
    MUSIC_PLAYER_ERR_DEV_READ,			//设备读错误
    MUSIC_PLAYER_ERR_FSCAN,				//设备扫描失败
    MUSIC_PLAYER_ERR_FILE_NOFOUND,		//没有找到文件
    MUSIC_PLAYER_ERR_RECORD_DEV,		//录音文件夹操作错误
    MUSIC_PLAYER_ERR_FILE_READ,			//文件读错误
};

//断点信息结构体
struct __breakpoint {
    //文件信息
    u32 fsize;
    u32 sclust;
    //解码信息
    struct audio_dec_breakpoint dbp;
};

//回调接口结构体
struct __player_cb {
    //解码成功回调
    void (*start)(void *priv, int parm);
    //解码结束回调
    void (*end)(void *priv, int parm);
    //解码错误回调
    void (*err)(void *priv, int parm);
};

//参数设置结构体
struct __player_parm {
    const struct __player_cb *cb;
    //其他扩展
    const struct __scan_callback *scan_cb;//扫盘打断回调
};

//music player总控制句柄
struct music_player {
    struct __dev 			*dev;//当前播放设备节点
    struct vfscan			*fsn;//设备扫描句柄
    FILE		 			*file;//当前播放文件句柄
    FILE		 			*lrc_file;//当前播放文件句柄
    void 					*priv;//music回调函数，私有参数
    struct __player_parm 	parm;//回调及参数配置
    void                    *le_audio;//广播音箱的句柄
    struct stream_enc_fmt fmt;//广播音箱的编码格式
};


//music_player初始化
struct music_player *music_player_creat(void);
//music_playe注册扫盘回调
bool music_player_reg_scandisk_callback(struct music_player *player_hd, const struct __scan_callback *scan_cb);
//music_playe注册解码状态回调
bool music_player_reg_dec_callback(struct music_player *player_hd, const struct __player_cb *cb);
//music_player释放接口
void music_player_destroy(struct music_player *player_hd);
//music_player解码器启动接口
int music_player_decode_start(struct music_player *player_hd, FILE *file, struct audio_dec_breakpoint *dbp);
//music_player获取当前播放设备断点信息
bool music_player_get_playing_breakpoint(struct music_player *player_hd, struct __breakpoint *bp, u8 flag);

//music_player播放结束处理
int music_player_end_deal(struct music_player *player_hd, int parm);
//music_player播放器解码错误处理
int music_player_decode_err_deal(struct music_player *player_hd, int event);
//music_player获取当前播放对应的music设备
char *music_player_get_cur_music_dev(struct music_player *player_hd);
//music_player获取当前播放设备下一个设备
char *music_player_get_dev_next(struct music_player *player_hd, u8 auto_next);
//music_player获取当前播放设备上一个设备
char *music_player_get_dev_prev(struct music_player *player_hd);
//music_player获取当前播放循环模式
u8 music_player_get_repeat_mode(void);
//music_player获取当前录音区分播放状态
bool music_player_get_record_play_status(struct music_player *player_hd);
//music_player从设备列表里面往前或往后找设备，并且过滤掉指定字符串的设备
char *music_player_get_dev_flit(struct music_player *player_hd, char *flit, u8 direct);
//music_player设置播放循环模式
int  music_player_set_repeat_mode(struct music_player *player_hd, u8 mode);
//music_player切换循环模式
int  music_player_change_repeat_mode(struct music_player *player_hd);
//music_player解码停止
void music_player_stop(struct music_player *player_hd, u8 fsn_release);
//music_player删除当前播放文件,并播放下一曲
int music_player_delete_playing_file(struct music_player *player_hd);
//music_player播放上一曲
int music_player_play_prev(struct music_player *player_hd);
//music_player播放下一曲
int music_player_play_next(struct music_player *player_hd);
//music_player播放第一曲
int music_player_play_first_file(struct music_player *player_hd, char *logo);
//music_player播放最后一曲
int music_player_play_last_file(struct music_player *player_hd, char *logo);
//music_player自动播放下一曲
int music_player_play_auto_next(struct music_player *player_hd);
//music_player上一个文件夹
int music_player_play_folder_prev(struct music_player *player_hd);
//music_player下一个文件夹
int music_player_play_folder_next(struct music_player *player_hd);
//music_player上一个设备
int music_player_play_devcie_prev(struct music_player *player_hd, struct __breakpoint *bp);
//music_player下一个设备
int music_player_play_devcie_next(struct music_player *player_hd, struct __breakpoint *bp);
//music_player断点播放指定设备
int music_player_play_by_breakpoint(struct music_player *player_hd, char *logo, struct __breakpoint *pb);
//music_player序号播放指定设备
int music_player_play_by_number(struct music_player *player_hd, char *logo, u32 number);
//music_player簇号播放指定设备
int music_player_play_by_sclust(struct music_player *player_hd, char *logo, u32 sclust);
//music_player路径播放指定设备
int music_player_play_by_path(struct music_player *player_hd, char *logo, const char *path);
//music_player录音区分切换播放
int music_player_play_record_folder(struct music_player *player_hd, char *logo, struct __breakpoint *bp);
//music_player扫盘
int music_player_scan_disk(struct music_player *player_hd);
//获取当前设备的物理设备
const char *music_player_get_phy_dev(struct music_player *player_hd, int *len);
//歌词分析
int music_player_lrc_analy_start(struct music_player *player_hd);

#endif//__MUSIC_PLAYER_H__

