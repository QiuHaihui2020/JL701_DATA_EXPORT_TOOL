#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".spatial_effect.data.bss")
#pragma data_seg(".spatial_effect.data")
#pragma const_seg(".spatial_effect.text.const")
#pragma code_seg(".spatial_effect.text")
#endif
/*****************************************************************
>file name : spatial_audio.c
>create time : Mon 03 Jan 2022 01:58:51 PM CST
*****************************************************************/
#include "typedef.h"
#include "system/includes.h"
#include "audio_base.h"
#include "app_config.h"
#include "circular_buf.h"

#if TCFG_AUDIO_SPATIAL_EFFECT_ENABLE
#include "spatial_effect_tws.h"
#include "spatial_effect_imu.h"
#include "spatial_effect.h"
#include "spatial_effects_process.h"

#include "tech_lib/effect_surTheta_api.h"
#include "tech_lib/SpatialAudio_api.h"
#include "tech_lib/3DSpatial_ctrl.h"

/* 音效算法版本选择
 * 0：V1.0.0
 * 1：V2.0.0
 * */
extern const int CONFIG_SPATIAL_EFFECT_VERSION;

/************** V1.0.0版本音效算法配置 ***********************/
const int  voicechange_fft_PLATFORM = 2;

/*T0模式下有效
0：环境声更明显
1：最左最右的时候音量差异更明显*/
const int OLD_V_FLAG = 1;/*0 or 1*/
/*角度的映射音量密度
OLD_V_FLAG = 1时有效*/
const int P360_MRATE = 128;/*0 ~ 128*/

/*空间音频模式使能控制变量
 * 0: 关闭音效，保留角度跟踪
 * 1: 打开音效和角度跟踪
 * 2: 轻量级音效和角度跟踪*/
const int P360_REVERB_NE = 1;
/**************end  V1.0.0版本音效算法配置 ******************/

/************** V2.0.0版本音效算法配置 ***********************/
#define EFFECT_FRAMES_SIZE 512 //数据处理帧长，帧长越大，需要的tmpbuf越大
/**************end  V2.0.0版本音效算法配置 ******************/

static struct spatial_audio_context *spatial_hdl = NULL;

struct sound360td_algo {
    int angle;
    const void *ops;
    u8 *tmpbuf;
    unsigned int run_buf[0];
};

struct spatial_calculator {
    void *privtate_data;
    u8 work_buf[0];
};
int  anglevolume = 150;
int  angleresetflag = 0;

/*在线更新参数的标志*/
static u8 param_update_flag = 0;
static spatial_effect_cfg_t effect_cfg;

static char *version[2] = {"SPATIAL_EFFECT_V1", "SPATIAL_EFFECT_V2"};
void spatial_effect_param_dump(spatial_effect_cfg_t *effect_cfg)
{
    printf("version : %s", version[CONFIG_SPATIAL_EFFECT_VERSION]);
    printf("===== angle =====");
    puts("track_sensitivity :");
    put_float(effect_cfg->angle.track_sensitivity);
    puts("angle_reset_sensitivity :");
    put_float(effect_cfg->angle.angle_reset_sensitivity);
    if (CONFIG_SPATIAL_EFFECT_VERSION == SPATIAL_EFFECT_V2) {

        printf("===== pcm info =====");
        printf("IndataBit : %d", effect_cfg->pcm_info.IndataBit);
        printf("OutdataBit : %d", effect_cfg->pcm_info.OutdataBit);
        printf("IndataInc : %d", effect_cfg->pcm_info.IndataInc);
        printf("OutdataInc : %d", effect_cfg->pcm_info.OutdataInc);
        printf("Qval : %d", effect_cfg->pcm_info.Qval);

        printf("===== source_cfi =====");
        printf("CustomizedITD : %d", effect_cfg->scfi.CustomizedITD);
        printf("FarField : %d", effect_cfg->scfi.FarField);
        printf("sampleRate : %d", effect_cfg->scfi.sampleRate);

        printf("===== source_ang =====");
        printf("Azimuth_angle : %d", effect_cfg->sag.Azimuth_angle);
        printf("Elevation_angle : %d", effect_cfg->sag.Elevation_angle);
        puts("radius :");
        put_float(effect_cfg->sag.radius);
        printf("bias_angle : %d", effect_cfg->sag.bias_angle);

        printf("===== core =====");
        puts("ListenerHeadRadius :");
        put_float(effect_cfg->cor.ListenerHeadRadius);
        printf("soundSpeed : %d", effect_cfg->cor.soundSpeed);
        printf("channel : %d", effect_cfg->cor.channel);
    } else {
        printf("===== rp_parm =====");
        printf("trackKIND : %d", effect_cfg->rp_parm.trackKIND);
        printf("ReverbKIND : %d", effect_cfg->rp_parm.ReverbKIND);
        printf("reverbance : %d", effect_cfg->rp_parm.reverbance);
        printf("dampingval : %d", effect_cfg->rp_parm.dampingval);
    }

}

void spatial_effect_param_init()
{
    /*传感器角度计算参数*/
    effect_cfg.angle.track_sensitivity = 1.0f;//工具配置，头部跟踪灵敏度
    effect_cfg.angle.angle_reset_sensitivity = 0.01f;//工具配置，静止角度复位灵敏度

    /*V100音效参数*/
    effect_cfg.rp_parm.trackKIND = P360_T0;
    effect_cfg.rp_parm.ReverbKIND = P360_R1;
    effect_cfg.rp_parm.reverbance = 70;
    effect_cfg.rp_parm.dampingval = 70;

    /*V200音效参数*/
    effect_cfg.pcm_info.IndataBit = DATA_INT_16BIT;//节点协商确定
    effect_cfg.pcm_info.OutdataBit = DATA_INT_16BIT;//节点协商确定
    effect_cfg.pcm_info.IndataInc = 2;
    effect_cfg.pcm_info.OutdataInc = 2;
    effect_cfg.pcm_info.Qval = 15;

    effect_cfg.scfi.CustomizedITD = 1;
    effect_cfg.scfi.FarField = 1;
    effect_cfg.scfi.sampleRate = 44100;//节点协商确定

    effect_cfg.sag.Azimuth_angle = 0; //实时更新的角度
    effect_cfg.sag.Elevation_angle = 0;
    effect_cfg.sag.radius = 1;//工具配置，半径是声源到人的距离，调节远近的效果
    effect_cfg.sag.bias_angle = 40; //工具配置，偏角是调节声像的，偏角为0，听上去就是点声源，比如30度就听感上是60度范围的声像

    effect_cfg.cor.ListenerHeadRadius = 0.0875f;
    effect_cfg.cor.soundSpeed = 343;
    effect_cfg.cor.channel = 2;

    int len = spatial_effects_node_param_cfg_read(&effect_cfg, sizeof(effect_cfg));
    if (len != sizeof(effect_cfg)) {
        printf("spatial effects use default effect_cfg");
    }

    spatial_effect_param_dump(&effect_cfg);
}

void spatial_effect_online_updata(void *params)
{
    /* memcpy(&effect_cfg, (u8 *)params, sizeof(effect_cfg)); */
    spatial_effect_cfg_t *p = (spatial_effect_cfg_t *)params;

    effect_cfg.angle.track_sensitivity = p->angle.track_sensitivity;
    effect_cfg.angle.angle_reset_sensitivity = p->angle.angle_reset_sensitivity;

    effect_cfg.sag.radius = p->sag.radius;
    effect_cfg.sag.bias_angle = p->sag.bias_angle;

    /* effect_cfg.scfi.sampleRate = p->scfi.sampleRate; */

    effect_cfg.rp_parm.trackKIND = p->rp_parm.trackKIND;
    effect_cfg.rp_parm.ReverbKIND = p->rp_parm.ReverbKIND;
    effect_cfg.rp_parm.reverbance = p->rp_parm.reverbance;
    effect_cfg.rp_parm.dampingval = p->rp_parm.dampingval;
    spatial_effect_param_dump(&effect_cfg);
    param_update_flag = 1;
}

void get_spatial_effect_reverb_params(RP_PARM_CONIFG *params)
{
    memcpy(params, &effect_cfg.rp_parm, sizeof(effect_cfg.rp_parm));
}

static void *spatial_audio_effect_init(void)
{
    struct sound360td_algo *algo = NULL;

#if SPATIAL_AUDIO_EFFECT_ENABLE
    int buf_size = 0;
    angleresetflag = 1;
    param_update_flag = 0;

    spatial_effect_param_init();

    if (CONFIG_SPATIAL_EFFECT_VERSION == SPATIAL_EFFECT_V2) {
        Sound_3DSpatial *ops = get_SpatialEffect_ops();//获取句柄
        buf_size = ops->need_buf(effect_cfg.cor.channel);//获取bufsize
        printf("effect buf size %d", buf_size);

        algo = (struct sound360td_algo *)zalloc(sizeof(struct sound360td_algo) + buf_size);
        if (!algo) {
            return NULL;
        }

        /*tmpbuf的大小和输入处理的数据长度有关系*/
        int tmpbuf_len = ops->need_tmpbuf(EFFECT_FRAMES_SIZE, effect_cfg.pcm_info);
        printf("tmpbuf_len %d", tmpbuf_len);
        algo->tmpbuf = zalloc(tmpbuf_len);
        if (!algo->tmpbuf) {
            free(algo);
            return NULL;
        }
        int type = ops->init(algo->run_buf, &effect_cfg.cor, &effect_cfg.scfi, &effect_cfg.sag, &effect_cfg.pcm_info);
        if (type == af_DATABIT_NOTSUPPORT) {
            printf("[error] type == af_DATABIT_NOTSUPPORT !!! ");
        }

        algo->ops = ops;

    } else {
        PointSound360TD_FUNC_API *ops = get_PointSound360TD_func_api();
        PointSound360TD_PARM_SET params;
        /* RP_PARM_CONIFG  rp_obj; */

        /* memcpy(&rp_obj, &effect_cfg.rp_parm, sizeof(effect_cfg.rp_parm)); */

        params.theta = 0;
        params.volume = anglevolume;

        buf_size = ops->need_buf(P360TD_REV_K1);
        printf("effect buf size %d", buf_size);

        algo = (struct sound360td_algo *)zalloc(sizeof(struct sound360td_algo) + buf_size);
        if (!algo) {
            return NULL;
        }

        ops->open_config(algo->run_buf, &params, &effect_cfg.rp_parm);
        algo->angle = 0;
        algo->ops = ops;
    }
#endif /*SPATIAL_AUDIO_EFFECT_ENABLE*/

    return algo;
}

/*获取算法输出的通道值*/
static u8 get_Sound_3DSpatial_output_channal(u8 mapping_channel)
{
    u8 out_channel = 0;
    switch (mapping_channel) {
    case AUDIO_CH_L:
        out_channel = 0;
        break;
    case AUDIO_CH_R:
        out_channel = 1;
        break;
    case AUDIO_CH_LR:
        out_channel = 2;
        break;
    default:
        break;
    }
    return out_channel;
}

static int spatial_audio_effect_handler(void *effect, void *data, int len, u8 mapping_channel)
{
    struct sound360td_algo *algo = (struct sound360td_algo *)effect;

#if SPATIAL_AUDIO_EFFECT_ENABLE
    if ((algo == NULL) || (len == 0)) {
        return 0;
    }
    int frames = len >> 2;//输入单通道的点数

    if (CONFIG_SPATIAL_EFFECT_VERSION == SPATIAL_EFFECT_V2) {
        Sound_3DSpatial *ops = (Sound_3DSpatial *)algo->ops;
        int data_len = len;
        s8 *in_data = data;
        s8 *out_data = data;
        int offset;
        if (effect_cfg.pcm_info.IndataBit == DATA_INT_32BIT) {
            offset = 4;
        } else {
            offset = 2;
        }
        u8 out_channel = get_Sound_3DSpatial_output_channal(mapping_channel);
        /*分包处理，优化tmpbuf的大小*/
        while (data_len) {
            if (data_len > EFFECT_FRAMES_SIZE) {
                /*算法里面做了双变单的处理，
                 * out_channel 0输出左声道，1输出右声道，2输出双声道数据*/
                ops->run(algo->run_buf, algo->tmpbuf, in_data, out_data, EFFECT_FRAMES_SIZE / (2 * offset), out_channel);
                in_data += EFFECT_FRAMES_SIZE;
                if (out_channel != 2) {
                    /*输出单声道时*/
                    out_data += (EFFECT_FRAMES_SIZE / 2);
                } else {
                    /*输出双声道时*/
                    out_data += EFFECT_FRAMES_SIZE;
                }
                data_len -= EFFECT_FRAMES_SIZE;
            } else {
                /*算法里面做了双变单的处理，
                 * out_channel 0输出左声道，1输出右声道，2输出双声道数据*/
                ops->run(algo->run_buf, algo->tmpbuf, in_data, out_data, data_len / (2 * offset), out_channel);
                data_len = 0;
            }
        }
        if (out_channel != 2) {
            len >>= 1;
        }

    } else {
        PointSound360TD_FUNC_API *ops = (PointSound360TD_FUNC_API *)algo->ops;
        ops->run(algo->run_buf, data, data, frames);
    }
#endif /*SPATIAL_AUDIO_EFFECT_ENABLE*/
    return len;
}

static void spatial_audio_effect_close(void *effect)
{
    struct sound360td_algo *algo = (struct sound360td_algo *)effect;

#if SPATIAL_AUDIO_EFFECT_ENABLE
    if (algo) {
        if (algo->tmpbuf) {
            free(algo->tmpbuf);
            algo->tmpbuf = NULL;
        }
        free(algo);
        algo = NULL;
    }
#endif /*SPATIAL_AUDIO_EFFECT_ENABLE*/
}

/*
 * 空间效果参数配置接口
 */
static void spatial_audio_effect_params_setup(void *effect, int angle)
{
    struct sound360td_algo *algo = (struct sound360td_algo *)effect;
    if (algo == NULL) {
        return;
    }

#if SPATIAL_AUDIO_EFFECT_ENABLE

    /*角度发生变化 || 在线更新参数*/
    if ((angle != algo->angle) || param_update_flag) {
        if (CONFIG_SPATIAL_EFFECT_VERSION == SPATIAL_EFFECT_V2) {
            param_update_flag = 0;
            effect_cfg.sag.Azimuth_angle = angle;
            algo->angle = angle;
            updata_position(algo->run_buf, &effect_cfg.cor, &effect_cfg.sag);
        } else {
            PointSound360TD_FUNC_API *ops = (PointSound360TD_FUNC_API *)algo->ops;
            PointSound360TD_PARM_SET params;
            //TODO
            if (param_update_flag) {
                /*在线更新参数,有杂音*/
                params.theta = angle;
                params.volume = anglevolume;
                ops->open_config(algo->run_buf, &params, &effect_cfg.rp_parm);
            } else {
                /*更新角度参数，无杂音*/
                params.theta = angle;
                params.volume = anglevolume;
                ops->init(algo->run_buf, &params);
            }
            algo->angle = angle;
        }
        param_update_flag = 0;
    }
#endif /*SPATIAL_AUDIO_EFFECT_ENABLE*/
}

/*获取vm加速度计偏置*/
int read_vm_acc_cel_param(acc_cel_t *acc_cel)
{
    int ret = 0;
    ret = syscfg_read(CFG_IMU_ACC_OFFEST_ID, acc_cel, sizeof(acc_cel_t));
    if (ret != sizeof(acc_cel_t)) {
        printf("vm acc_cel read fail !!!");
        acc_cel->acc_offx = -48.231f;
        acc_cel->acc_offy = -57.035f;
        acc_cel->acc_offz = -84.097f;
    } else {
        printf("vm acc_cel read succ !!!");
    }
    printf("acc_offx : ");
    put_float(acc_cel->acc_offx);
    printf("acc_offy : ");
    put_float(acc_cel->acc_offy);
    printf("acc_offz : ");
    put_float(acc_cel->acc_offz);
    return 0;
}

/*获取vm陀螺仪偏置*/
int read_vm_gyro_cel_param(gyro_cel_t *gyro_cel)
{
    int ret = 0;
    ret = syscfg_read(CFG_IMU_GYRO_OFFEST_ID, gyro_cel, sizeof(gyro_cel_t));
    if (ret != sizeof(gyro_cel_t)) {
        printf("vm gyro_cel read fail !!!");
        gyro_cel->gyro_x = -1.694f;
        gyro_cel->gyro_y = -0.521f;
        gyro_cel->gyro_z = -0.078f;
    } else {
        printf("vm gyro_cel read succ !!!");
    }
    printf("gyro_x : ");
    put_float(gyro_cel->gyro_x);
    printf("gyro_y : ");
    put_float(gyro_cel->gyro_y);
    printf("gyro_z : ");
    put_float(gyro_cel->gyro_z);
    return 0;
}

/*配置传感器算法参数*/
int space_motion_param_init(info_spa_t *info_spa, spatial_config_t *conf, tranval_t *tranval)
{
#if TCFG_ICM42670P_ENABLE
    if (info_spa) {
        info_spa->fs = 99.f;
        info_spa->len = 6;
        info_spa->sensitivity = 16.4f;
        info_spa->range = 16;
    }
    if (conf) {
        conf->beta = 0.1f;
        conf->cel_val = 0.14f;
        conf->SerialTime = 0.2f;
        conf->time = 1.0f;
        conf->val = 0.02f;
        conf->sensval = 0.02f;
    }
    if (tranval) {
        tranval->trans_x[0] =  0;
        tranval->trans_x[1] =  1;
        tranval->trans_x[2] =  0;
        tranval->trans_y[0] =  0;
        tranval->trans_y[1] =  0;
        tranval->trans_y[2] = -1;
        tranval->trans_z[0] = -1;
        tranval->trans_z[1] =  0;
        tranval->trans_z[2] =  0;
    }

#elif TCFG_LSM6DSL_ENABLE
    if (info_spa) {
        info_spa->fs = 104.f;
        info_spa->len = 6;
        info_spa->sensitivity = 14.28f;
        info_spa->range = 16;
    }
    if (conf) {
        conf->beta = 0.1f;
        conf->cel_val = 0.12f;
        conf->SerialTime = 0.24f;
        conf->time = 1.f;
        conf->val = 0.02f;
        conf->sensval = 0.01f;
    }
    if (tranval) {
        tranval->trans_x[0] =  0;
        tranval->trans_x[1] =  1;
        tranval->trans_x[2] =  0;
        tranval->trans_y[0] =  0;
        tranval->trans_y[1] =  0;
        tranval->trans_y[2] = -1;
        tranval->trans_z[0] = -1;
        tranval->trans_z[1] =  0;
        tranval->trans_z[2] =  0;
    }

#elif TCFG_MPU6887P_ENABLE
    if (info_spa) {
        info_spa->fs = 100.f;
        info_spa->len = 6;
        info_spa->sensitivity = 16.4f;
        info_spa->range = 16;
    }
    if (conf) {
        conf->beta = 0.1f;
        conf->cel_val = 0.12f;
        conf->SerialTime = 0.2f;
        conf->time = 1.0f;
        conf->val = 0.07f;
        conf->sensval = 0.01f;
    }
    if (tranval) {
        tranval->trans_x[0] = -1;
        tranval->trans_x[1] =  0;
        tranval->trans_x[2] =  0;
        tranval->trans_y[0] =  0;
        tranval->trans_y[1] =  0;
        tranval->trans_y[2] = -1;
        tranval->trans_z[0] =  0;
        tranval->trans_z[1] = -1;
        tranval->trans_z[2] =  0;
    }

#else /*TCFG_MPU6050_EN*/
    if (info_spa) {
        info_spa->fs = 100.f;
        info_spa->len = 6;
        info_spa->sensitivity = 16.4f;
        info_spa->range = 16;
    }
    if (conf) {
        conf->beta = 0.1f;
        conf->cel_val = 0.08f;
        conf->SerialTime = 0.2f;
        conf->time = 1.0f;
        conf->val = 0.02f;
        conf->sensval = 0.01f;
    }
    if (tranval) {
        tranval->trans_x[0] = -1;
        tranval->trans_x[1] =  0;
        tranval->trans_x[2] =  0;
        tranval->trans_y[0] =  0;
        tranval->trans_y[1] =  0;
        tranval->trans_y[2] = -1;
        tranval->trans_z[0] =  0;
        tranval->trans_z[1] = -1;
        tranval->trans_z[2] =  0;
    }

#endif
    return 0;
}

static void *space_motion_calculator_open(void)
{
    struct spatial_calculator *c = NULL;

#if SPACE_MOTION_CALCULATOR_ENABLE
    //传感器算法参数初始化
    info_spa_t info_spa;
    spatial_config_t conf;
    tranval_t tranval;
    space_motion_param_init(&info_spa, &conf, &tranval);
    //陀螺仪偏置
    gyro_cel_t gyro_cel;
    read_vm_gyro_cel_param(&gyro_cel);
    //加速度偏置
    acc_cel_t acc_cel;
    read_vm_acc_cel_param(&acc_cel);

    int buf_size = get_Spatial_buf(info_spa.len);

    c = (struct spatial_calculator *)zalloc(sizeof(struct spatial_calculator) + buf_size);
    if (!c) {
        return NULL;
    }

    init_Spatial(c->work_buf, &info_spa, &tranval, &conf, &gyro_cel, &acc_cel);
#endif /*SPACE_MOTION_CALCULATOR_ENABLE*/

    return c;
}

static void space_motion_calculator_close(void *calculator)
{
#if SPACE_MOTION_CALCULATOR_ENABLE
    if (calculator) {
        free(calculator);
        calculator = NULL;
    }
#endif /*SPACE_MOTION_CALCULATOR_ENABLE*/
}

/*角度归0回正*/
void spatial_audio_angle_reset()
{
    angleresetflag = 1;
}

/*
 * 空间位置检测处理
 */
static int space_motion_detect(void *calculator, short *data, int len)
{
    struct spatial_calculator *c = (struct spatial_calculator *)calculator;
    int heading = 0;
    if ((c == NULL) || (len == 0)) {
        return 0;
    }

#if SPACE_MOTION_CALCULATOR_ENABLE

    /*
     * TODO : 这里是空间运动检测处理
     */
    if (angleresetflag) {
        Spatial_reset(c->work_buf);
        angleresetflag = 0;
    }

    int group_num = (len >> 1) / 6;
    while (group_num--) {
        Spatial_cacl(c->work_buf, data);
        data += 6;
        heading = get_Spa_angle(c->work_buf,
                                effect_cfg.angle.track_sensitivity,
                                effect_cfg.angle.angle_reset_sensitivity);

        int flag = Spatial_stra(c->work_buf, ANGLE_RESET_TIME, 0.5f); /*建议范围0.1~0.6；值越小，越接近静止条件下的回正，0.1为静止下的参考值*/
        if (flag == 1) {
            Spatial_reset(c->work_buf);
            flag = 0;
        }
    }
    /* printf("%d\n", heading); */
#endif /*SPACE_MOTION_CALCULATOR_ENABLE*/
    return heading;

}

#if SPATIAL_AUDIO_ANGLE_TWS_SYNC
static void spatial_tws_data_handler(void *priv, void *data, int len)
{
    struct spatial_audio_context *ctx = (struct spatial_audio_context *)priv;
    memcpy(&ctx->tws_angle, data, sizeof(ctx->tws_angle));
}
#endif /*SPATIAL_AUDIO_ANGLE_TWS_SYNC*/

void spatial_audio_sensor_sleep(u8 en)
{
    if (spatial_hdl && spatial_hdl->sensor) {
        space_motion_sensor_sleep(spatial_hdl->sensor, en);
    }
}

extern int set_bt_media_imu_angle(int angle);
void aud_spatial_sensor_task(void *priv)
{

    struct spatial_audio_context *ctx = priv;
    int angle = 0;
    while (1) {
        os_time_dly(2);
        if (!ctx->sensor_task_state) {
            continue;
        }
        ctx->sensor_task_busy = 1;
#if SPATIAL_AUDIO_SENSOR_ENABLE
        int sensor_data_len = 0;
        if (ctx->sensor && ctx->head_tracked) {
            sensor_data_len = space_motion_data_read(ctx->sensor, ctx->data, 1024);
        }
#endif /*SPATIAL_AUDIO_SENSOR_ENABLE*/

        /*传感器有数据才做角度计算和角度同步*/
#if SPATIAL_AUDIO_SENSOR_ENABLE
        if (sensor_data_len) {
#if SPACE_MOTION_CALCULATOR_ENABLE
            if (ctx->calculator) {
                if (ctx->head_tracked) {
                    angle = space_motion_detect(ctx->calculator, (s16 *)ctx->data, sensor_data_len);
                } else {
                    angle = 0;
                }
            }
#endif /*SPACE_MOTION_CALCULATOR_ENABLE*/

#if SPATIAL_AUDIO_ANGLE_TWS_SYNC
            if (ctx->tws_conn) {
                spatial_tws_audio_data_sync(ctx->tws_conn, (void *)&angle, sizeof(angle));
            }
            /* printf("%d", ctx->tws_angle); */
#endif /*SPATIAL_AUDIO_ANGLE_TWS_SYNC*/
        }
#endif /*SPATIAL_AUDIO_SENSOR_ENABLE*/

#if SPATIAL_AUDIO_EFFECT_ENABLE
        if (ctx->effect) {
#if SPATIAL_AUDIO_ANGLE_TWS_SYNC
            spatial_audio_effect_params_setup(ctx->effect, ctx->tws_angle);
#else
            spatial_audio_effect_params_setup(ctx->effect, angle);
#endif /*SPATIAL_AUDIO_ANGLE_TWS_SYNC*/
        }
#endif /*SPATIAL_AUDIO_EFFECT_ENABLE*/

#if (TCFG_SPATIAL_EFFECT_ONLINE_ENABLE)
#if SPATIAL_AUDIO_ANGLE_TWS_SYNC
        set_bt_media_imu_angle(ctx->tws_angle);
#else
        set_bt_media_imu_angle(angle);
#endif /*SPATIAL_AUDIO_ANGLE_TWS_SYNC*/
#endif /*TCFG_SPATIAL_EFFECT_ONLINE_ENABLE*/
        ctx->sensor_task_busy = 0;
    }
}

int aud_spatial_sensor_init(struct spatial_audio_context *spatial_ctx)
{
    int err = 0;
    struct spatial_audio_context *ctx = spatial_ctx;
    if (!ctx) {
        return 0;
    }

#if SPATIAL_AUDIO_SENSOR_ENABLE
    ctx->head_tracked = ((get_a2dp_spatial_audio_mode() == SPATIAL_EFX_TRACKED) ? 1 : 0);
    mem_stats();
    ctx->sensor = space_motion_detect_open();
    mem_stats();
    if (ctx->head_tracked) {
        spatial_audio_sensor_sleep(0);
    } else {
        spatial_audio_sensor_sleep(1);
    }
#endif /*SPATIAL_AUDIO_SENSOR_ENABLE*/

#if SPACE_MOTION_CALCULATOR_ENABLE
    ctx->calculator = space_motion_calculator_open();
    mem_stats();
#endif /*SPACE_MOTION_CALCULATOR_ENABLE*/

#if SPATIAL_AUDIO_ANGLE_TWS_SYNC
    ctx->tws_conn = spatial_tws_create_connection(100, (void *)ctx, spatial_tws_data_handler);
    mem_stats();
#endif /*SPATIAL_AUDIO_ANGLE_TWS_SYNC*/

    ctx->sensor_task_busy = 0;
    ctx->sensor_task_state = 1;
    task_create(aud_spatial_sensor_task, ctx, SPATIAL_SENSOR_TASK_NAME);
    return err;
}

int aud_spatial_sensor_exit(struct spatial_audio_context *spatial_ctx)
{
    int err = 0;
    struct spatial_audio_context *ctx = spatial_ctx;
    if (!ctx) {
        return 0;
    }
    ctx->sensor_task_state = 0;
    while (ctx->sensor_task_busy) {
        putchar('w');
        os_time_dly(1);
    }
    task_kill(SPATIAL_SENSOR_TASK_NAME);

#if SPATIAL_AUDIO_SENSOR_ENABLE
    if (ctx->sensor) {
        space_motion_detect_close(ctx->sensor);
    }
#endif /*SPATIAL_AUDIO_SENSOR_ENABLE*/

#if SPACE_MOTION_CALCULATOR_ENABLE
    if (ctx->calculator) {
        space_motion_calculator_close(ctx->calculator);
    }
#endif /*SPACE_MOTION_CALCULATOR_ENABLE*/

#if SPATIAL_AUDIO_ANGLE_TWS_SYNC
    if (ctx->tws_conn) {
        spatial_tws_delete_connection(ctx->tws_conn);
    }
#endif /*SPATIAL_AUDIO_ANGLE_TWS_SYNC*/

    printf("aud_spatial_sensor_exit\n");
    return err;
}

void *spatial_audio_open(void)
{
    printf("%s", __func__);
    struct spatial_audio_context *ctx;

    if (spatial_hdl) {
        printf("spatial_audio is alreadly open !!!");
        return spatial_hdl;
    }
    mem_stats();
#if SPATIAL_AUDIO_SENSOR_ENABLE
    ctx = (struct spatial_audio_context *)zalloc(sizeof(struct spatial_audio_context) + 1024);
#else
    ctx = (struct spatial_audio_context *)zalloc(sizeof(struct spatial_audio_context));
#endif
    if (!ctx) {
        return NULL;
    }
    spatial_hdl = ctx;

    aud_spatial_sensor_init(ctx);

#if SPATIAL_AUDIO_EFFECT_ENABLE
    ctx->effect = spatial_audio_effect_init();
    mem_stats();
#endif /*SPATIAL_AUDIO_EFFECT_ENABLE*/

    /*空间音频正常的声道映射为左右声道*/
    ctx->mapping_channel = AUDIO_CH_MIX;
    return ctx;
}

int spatial_audio_set_mapping_channel(void *sa, u8 channel)
{
    struct spatial_audio_context *ctx = (struct spatial_audio_context *)sa;
    if (ctx) {
        ctx->mapping_channel = channel;
    }

    return 0;
}

void spatial_audio_close(void *sa)
{
    printf("%s", __func__);
    struct spatial_audio_context *ctx = (struct spatial_audio_context *)sa;
    if (!ctx) {
        printf("spatial_audio_close err !!!");
        return;
    }

    aud_spatial_sensor_exit(ctx);

#if SPATIAL_AUDIO_EFFECT_ENABLE
    if (ctx->effect) {
        spatial_audio_effect_close(ctx->effect);
    }
#endif /*SPATIAL_AUDIO_EFFECT_ENABLE*/

    free(ctx);
    ctx = NULL;
    spatial_hdl = NULL;
}

int spatial_audio_remapping_data_handler(u8 mapping_channel, u8 bit_width, void *data, int len)
{
    int pcm_frames;
    int i = 0;
    int tmp = 0;
    if (bit_width) {

        s32 *pcm_buf_32 = (s32 *)data;
        pcm_frames = len >> 3;
        switch (mapping_channel) {
        case AUDIO_CH_L:
            for (i = 0; i < pcm_frames; i++) {
                pcm_buf_32[i] = pcm_buf_32[i * 2];
            }
            len /= 2;
            break;
        case AUDIO_CH_R:
            for (i = 0; i < pcm_frames; i++) {
                pcm_buf_32[i] = pcm_buf_32[i * 2 + 1];
            }
            len /= 2;
            break;
        case AUDIO_CH_MIX:
            for (i = 0; i < pcm_frames; i++) {
                tmp = pcm_buf_32[i * 2] + pcm_buf_32[i * 2 + 1];
                pcm_buf_32[i] = tmp / 2;
            }
            len /= 2;
            break;
        default:
            break;
        }
    } else {
        s16 *pcm_buf = (s16 *)data;
        pcm_frames = len >> 2;
        switch (mapping_channel) {
        case AUDIO_CH_L:
            for (i = 0; i < pcm_frames; i++) {
                pcm_buf[i] = pcm_buf[i * 2];
            }
            len /= 2;
            break;
        case AUDIO_CH_R:
            for (i = 0; i < pcm_frames; i++) {
                pcm_buf[i] = pcm_buf[i * 2 + 1];
            }
            len /= 2;
            break;
        case AUDIO_CH_MIX:
            for (i = 0; i < pcm_frames; i++) {
                tmp = pcm_buf[i * 2] + pcm_buf[i * 2 + 1];
                pcm_buf[i] = tmp / 2;
            }
            len /= 2;
            break;
        default:
            break;
        }
    }

    return len;
}

static int spatial_audio_data_handler(struct spatial_audio_context *ctx, void *data, int len)
{
#if SPATIAL_AUDIO_EFFECT_ENABLE
    if (ctx->effect) {
        len = spatial_audio_effect_handler(ctx->effect, data, len, ctx->mapping_channel);
    }

    /*V2算法里面做了双变单的切换*/
    if (CONFIG_SPATIAL_EFFECT_VERSION == SPATIAL_EFFECT_V1) {
        len = spatial_audio_remapping_data_handler(ctx->mapping_channel, effect_cfg.pcm_info.IndataBit, data, len);
    }

#else
    len = spatial_audio_remapping_data_handler(ctx->mapping_channel, effect_cfg.pcm_info.IndataBit, data, len);
#endif
    return len;
}

void spatial_audio_head_tracked_en(struct spatial_audio_context *ctx, u8 en)
{
    printf("head_tracked = %d\n", en);
    if (ctx) {
        angleresetflag = 1;
        ctx->head_tracked = en;
    }
}

u8 get_spatial_audio_head_tracked(struct spatial_audio_context *ctx)
{
    if (ctx) {
        printf("head_tracked = %d\n", ctx->head_tracked);
        return ctx->head_tracked;
    } else {
        return 0;
    }
}

int spatial_audio_filter(void *sa, void *data, int len)
{
    struct spatial_audio_context *ctx = (struct spatial_audio_context *)sa;

    if ((!ctx) || (!len)) {
        putchar('e');
        return 0;
    }

    return spatial_audio_data_handler(ctx, data, len);
}

#if SPATIAL_AUDIO_EXPORT_DATA

static int spatial_audio_export_init(struct spatial_audio_context *ctx)
{
    int audio_buf_size = 40 * 1024;
    int space_buf_size = 8 * 1024;
    ctx->space_fifo_buf = (u8 *)malloc(1024);
    ctx->audio_buf = (u8 *)malloc(audio_buf_size);
    /*ctx->space_buf = (u8 *)malloc(space_buf_size);*/
    cbuf_init(&ctx->audio_cbuf, ctx->audio_buf, audio_buf_size);
    /*cbuf_init(&ctx->space_cbuf, ctx->space_buf, space_buf_size);*/

#if SPATIAL_AUDIO_EXPORT_MODE == 0
    ctx->audio_file = fopen("storage/sd0/C/spatial_audio/aud***.raw", "w+");
    /*ctx->space_file = fopen("storage/sd0/C/spatial_audio/spa***.raw", "w+"); */
#elif SPATIAL_AUDIO_EXPORT_MODE == 1
    aec_uart_init();
#endif
    ctx->export = 1;
    return 0;
}

static void spatial_audio_export_release(struct spatial_audio_context *ctx)
{
    ctx->export = 0;

    if (ctx->audio_buf) {
        free(ctx->audio_buf);
        ctx->audio_buf = NULL;
    }

    if (ctx->space_buf) {
        free(ctx->space_buf);
        ctx->space_buf = NULL;
    }

    if (ctx->space_fifo_buf) {
        free(ctx->space_fifo_buf);
        ctx->space_fifo_buf = NULL;
    }

#if SPATIAL_AUDIO_EXPORT_MODE == 0
    if (ctx->audio_file) {
        fclose(ctx->audio_file);
    }

    if (ctx->space_file) {
        fclose(ctx->space_file);
    }
#elif SPATIAL_AUDIO_EXPORT_MODE == 1
    aec_uart_close();
#endif
}

static int audio_data_export_handler(struct spatial_audio_context *ctx)
{
    int err = -EINVAL;
#if SPATIAL_AUDIO_EXPORT_MODE == 0
    if (ctx->audio_file) {
        if (cbuf_get_data_len(&ctx->audio_cbuf) >= 512) {
            fwrite(ctx->audio_file, cbuf_get_readptr(&ctx->audio_cbuf), 512);
            cbuf_read_updata(&ctx->audio_cbuf, 512);
            err = 0;
        } else {
            //TODO
        }
    }

#if 0
    if (ctx->space_file) {
        if (cbuf_get_data_len(&ctx->space_cbuf) >= 512) {
            fwrite(ctx->space_file, cbuf_get_readptr(&ctx->space_cbuf), 512);
            cbuf_read_updata(&ctx->space_cbuf, 512);
            err = 0;
        } else {
            //TODO
        }
    }
#endif
#elif SPATIAL_AUDIO_EXPORT_MODE == 1
    u8 send = 0;
    if (cbuf_get_data_len(&ctx->audio_cbuf) >= 512) {
        aec_uart_fill(0, cbuf_get_readptr(&ctx->audio_cbuf), 512);
        cbuf_read_updata(&ctx->audio_cbuf, 512);
        err = 0;
        send = 1;
    }

    if (!send && cbuf_get_data_len(&ctx->space_cbuf) >= 512) {
        aec_uart_fill(0, cbuf_get_readptr(&ctx->space_cbuf), 512);
        cbuf_read_updata(&ctx->space_cbuf, 512);
        err = 0;
        send = 1;
    }

    if (send) {
        putchar('t');
        aec_uart_write();
    }
#endif

    return err;
}

void audio_export_task(void *arg)
{
    int msg[16];
    int res;
    int pend_taskq = 1;
    struct spatial_audio_context *ctx = (struct spatial_audio_context *)arg;

    while (1) {
        if (pend_taskq) {
            res = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        } else {
            res = os_taskq_accept(ARRAY_SIZE(msg), msg);
        }

        if (res == OS_TASKQ) {
            switch (msg[1]) {
            case 0:
                spatial_audio_export_init(ctx);
                break;
            case 1:
                spatial_audio_export_release(ctx);
                os_sem_post((OS_SEM *)msg[2]);
                pend_taskq = 1;
                break;
            case 2: //音频数据
                break;
            case 3: //空间移动信息
                break;
            }
        }

        if (ctx->export) {
            if (0 != audio_data_export_handler(ctx)) {
                pend_taskq = 1;
            } else {
                pend_taskq = 0;
            }
        }

    }
}
#endif

#endif/*TCFG_AUDIO_SPATIAL_EFFECT_ENABLE*/
