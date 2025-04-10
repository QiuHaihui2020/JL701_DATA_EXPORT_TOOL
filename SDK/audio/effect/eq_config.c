#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".eq_config.data.bss")
#pragma data_seg(".eq_config.data")
#pragma const_seg(".eq_config.text.const")
#pragma code_seg(".eq_config.text")
#endif

#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "sdk_config.h"
#include "effects/eq_config.h"
#include "effects/effects_adj.h"
#include "audio_config.h"
#include "jlstream.h"

/*
 *双核EQ选配策略
 *0: 自动选配,适合无混响环境，或者有混响但有eq的数据流不大于2条的环境
 *1：混响固定选配到EQ1,其余eq选配到EQ0
 * */
#if defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE
const int audio_eq_core_sel = 1;
#else
const int audio_eq_core_sel = 0;
#endif

#if TCFG_EQ_ENABLE

#define   mSECTION_MAX   eq_get_table_nsection(eq_mode)
#define EQ_FADE_TIME 10   //新版sdk该宏定义只是本文件内eq淡入更新系数相关应用的使能位，旧版本则是更新系数timer的运行周期单位ms
#define EQ_FADE_STEP 0.1f //增益的淡入步进，单位dB(0.01~1)
#define EQ_FADE_FREQ_STEP  100  //中心截止频率的淡入步进Hz(1~255)
#define EQ_FADE_END_CALLBACK_EN   0//淡出结束增加回调处理，可用于debug 当前eq用的系数


#define MODE_INDEX_DEFAULT 0

const struct eq_seg_info eq_tab_normal[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,    0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,   0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,   0, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   0, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0, 0.7f},
};

const struct eq_seg_info eq_tab_rock[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    -2, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    2, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,    4, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   -2, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  -2, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   4, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  4, 0.7f},
};

const struct eq_seg_info eq_tab_pop[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,     3, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     1, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,   -2, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   -4, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  -4, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  -2, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   1, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  2, 0.7f},
};

const struct eq_seg_info eq_tab_classic[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,     0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     8, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    8, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,    4, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,    0, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,   0, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   2, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  2, 0.7f},

};

const struct eq_seg_info eq_tab_country[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    -2, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,    2, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,    2, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,   0, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   4, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  4, 0.7f},
};

const struct eq_seg_info eq_tab_jazz[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,     0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,     0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,    0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,    4, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,    4, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,   4, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,   2, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,   3, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000,  4, 0.7f},
};
struct eq_seg_info eq_tab_custom[] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 31,    0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 62,    0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 125,   0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 250,   0, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 500,   0, 0.7f},
    {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0, 0.7f},
    {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0, 0.7f},
    {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0, 0.7f},
    {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0, 0.7f},
    {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0, 0.7f},
};

// 默认系数表,用户可修改
const struct eq_seg_info *eq_type_tab[EQ_MODE_MAX] = {
    eq_tab_normal, eq_tab_rock, eq_tab_pop, eq_tab_classic, eq_tab_jazz, eq_tab_country, eq_tab_custom
};
// 默认系数表，每个表对应的总增益,用户可修改
float global_gain_tab[EQ_MODE_MAX] = {0, 0, 0, 0, 0, 0, 0};
static const u8 tab_section_num[] = {
    ARRAY_SIZE(eq_tab_normal), ARRAY_SIZE(eq_tab_rock),
    ARRAY_SIZE(eq_tab_pop), ARRAY_SIZE(eq_tab_classic),
    ARRAY_SIZE(eq_tab_jazz), ARRAY_SIZE(eq_tab_country),
    ARRAY_SIZE(eq_tab_custom)
};



/*
 *mode:枚举型EQ_MODE
 *return 返回对应系数表的段数
 * */
u8 eq_get_table_nsection(EQ_MODE mode)
{
    if (mode >= ARRAY_SIZE(eq_type_tab)) {
        log_e("mode error %d\n", mode);
        return 0;
    }
    if (mode >= ARRAY_SIZE(tab_section_num)) {
        log_e("mode error %d\n", mode);
        return 0;
    }

    return tab_section_num[mode];
}

#define USE_SDK_DEFAULT_EQ_TAB   1//使用sdk内置的默认系数表，
#define USE_FILE_DEFAULT_EQ_TAB  2//使用效果文件内内置的默认系数表

static u8 eq_mode = 0;
static u8 use_eq_tab_mark = 0;//使用过eq_mode_sw或者eq_mode_set接口, use_eq_tab_mark = 1
//使用过eq_file_cfg_switch接口, use_eq_tab_mark = 2
static u8 eq_cfg_num = 0;//记录音乐eq配置项序号
static char music_eq_name[][16] = {"MusicEqBt", "Eq0Media"};//耳机蓝牙模式音乐eq节点,音箱蓝牙音乐eq节点定义的名称

#if EQ_FADE_END_CALLBACK_EN
int audio_fade_end_callback(struct audio_eq *eq, void *priv)
{
    printf("================fade end===============\n");
    printf("eq global_gain %d.%02d\n", (int)eq->global_gain, __builtin_abs((int)((eq->global_gain - (int)eq->global_gain) * 100)));
    printf("eq seg_num %d\n", eq->cur_nsection);
    for (int i = 0; i < eq->cur_nsection; i++) {
        struct eq_seg_info *seg = (struct eq_seg_info *)&eq->seg[i];
        printf("eq idx:%d, iir:%d, freq:%d, gain:0x%08x, %d.%02d, q:0x%08x, %d.%02d\n",
               seg->index, seg->iir_type, seg->freq, *(int *)&seg->gain, (int)seg->gain, __builtin_abs((int)((seg->gain - (int)seg->gain) * 100)), *(int *)&seg->gain, (int)seg->q, __builtin_abs((int)((seg->q - (int)seg->q) * 100)));
    }
    return 0;
}
#endif

//eq效果表切换
int eq_mode_sw(void)
{
    eq_mode++;
    if (eq_mode >= ARRAY_SIZE(eq_type_tab)) {
        eq_mode = 0;
    }
    struct eq_seg_info *seg = (struct eq_seg_info *)eq_type_tab[eq_mode];

    u8 nsection = eq_get_table_nsection(eq_mode);
    if (nsection > mSECTION_MAX) {
        log_e("ERROR nsection:%d > mSECTION_MAX:%d ", nsection, mSECTION_MAX);
        return -1;//
    }

    if (!nsection) {
        log_e("nsection is %d\n", nsection);
        return -1;
    }

    for (int i = 0; i < ARRAY_SIZE(music_eq_name); i++) {
        //music eq运行时，直接设置更新
        struct eq_adj eff = {0};
        eff.type = EQ_GLOBAL_GAIN_CMD;
        eff.param.global_gain =  global_gain_tab[eq_mode];
        eff.fade_parm.fade_time = EQ_FADE_TIME;
        eff.fade_parm.fade_step = EQ_FADE_STEP;
        eff.fade_parm.f_fade_step = EQ_FADE_FREQ_STEP;
#if EQ_FADE_END_CALLBACK_EN
        eff.fade_parm.priv = NULL;
        eff.fade_parm.callback = audio_fade_end_callback;
#endif

        int ret = jlstream_set_node_param(NODE_UUID_EQ, music_eq_name[i], &eff, sizeof(eff));
        if (ret == false) {
            continue ;
        }

        eff.type = EQ_SEG_NUM_CMD;
        eff.param.seg_num = nsection ;
        ret = jlstream_set_node_param(NODE_UUID_EQ, music_eq_name[i], &eff, sizeof(eff));
        if (ret == false) {
            continue ;
        }

        for (int j = 0; j < nsection; j++) {
            eff.type = EQ_SEG_CMD;
            memcpy(&eff.param.seg, &seg[j], sizeof(struct eq_seg_info));
            jlstream_set_node_param(NODE_UUID_EQ, music_eq_name[i], &eff, sizeof(eff));
        }
    }
    use_eq_tab_mark = USE_SDK_DEFAULT_EQ_TAB;
    return 0;
}
//指定设置某个eq效果表
int eq_mode_set(EQ_MODE mode)
{
    if (mode >= ARRAY_SIZE(eq_type_tab)) {
        log_e("mode err %d\n", mode);
        return -1;//
    }

    eq_mode = mode;
    struct eq_seg_info *seg = (struct eq_seg_info *)eq_type_tab[eq_mode];
    u8 nsection = eq_get_table_nsection(eq_mode);
    if (nsection > mSECTION_MAX) {
        log_e("ERROR nsection:%d > mSECTION_MAX:%d ", nsection, mSECTION_MAX);
        return -1;//
    }
    if (!nsection) {
        log_e("nsection is %d\n", nsection);
        return -1;
    }

    for (int i = 0; i < ARRAY_SIZE(music_eq_name); i++) {
        //music eq运行时，直接设置更新
        struct eq_adj eff = {0};
        eff.type = EQ_GLOBAL_GAIN_CMD;
        eff.param.global_gain =  global_gain_tab[eq_mode];
        eff.fade_parm.fade_time = EQ_FADE_TIME;
        eff.fade_parm.fade_step = EQ_FADE_STEP;
        eff.fade_parm.f_fade_step = EQ_FADE_FREQ_STEP;
#if EQ_FADE_END_CALLBACK_EN
        eff.fade_parm.priv = NULL;
        eff.fade_parm.callback = audio_fade_end_callback;
#endif

        int ret = jlstream_set_node_param(NODE_UUID_EQ, music_eq_name[i], &eff, sizeof(eff));
        if (ret == false) {
            continue ;
        }
        eff.type = EQ_SEG_NUM_CMD;
        eff.param.seg_num = nsection ;
        ret = jlstream_set_node_param(NODE_UUID_EQ, music_eq_name[i], &eff, sizeof(eff));
        if (ret == false) {
            continue ;
        }

        for (int j = 0; j < nsection; j++) {
            eff.type = EQ_SEG_CMD;
            memcpy(&eff.param.seg, &seg[j], sizeof(struct eq_seg_info));
            jlstream_set_node_param(NODE_UUID_EQ, music_eq_name[i], &eff, sizeof(eff));
        }
    }

    use_eq_tab_mark = USE_SDK_DEFAULT_EQ_TAB;
    return 0;
}
//返回某个eq效果模式标号
EQ_MODE eq_mode_get_cur(void)
{
    return eq_mode;
}

/*----------------------------------------------------------------------------*/
/**@brief   设置custom系数表的某一段系数
  @param   seg->index：第几段(从0开始)
  @param   seg->iir_type:滤波器类型(EQ_IIR_TYPE_HIGH_PASS, EQ_IIR_TYPE_LOW_PASS, EQ_IIR_TYPE_BAND_PASS, EQ_IIR_TYPE_HIGH_SHELF,EQ_IIR_TYPE_LOW_SHELF)
  @param   seg->freq:中心截止频率(20~22kHz)
  @param   seg->gain:总增益(-18~18)
  @param   seg->q : q值（0.3~30）
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_set_custom_seg(struct eq_seg_info *seg)
{
    struct eq_seg_info *tar_seg = eq_tab_custom;
    u8 index = seg->index;
    if (index > ARRAY_SIZE(eq_tab_custom)) {
        log_e("index %d > max_nsection %d", index, ARRAY_SIZE(eq_tab_custom));
        return -1;
    }
    memcpy(&tar_seg[index], seg, sizeof(struct eq_seg_info));
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    获取某个模式eq表内，某一段eq的信息
   @param
   @param
   @return  返回eq信息
   @note
*/
/*----------------------------------------------------------------------------*/
struct eq_seg_info *eq_mode_get_seg(EQ_MODE mode, u8 index)
{
    if (mode >= ARRAY_SIZE(eq_type_tab)) {
        log_e("mode error %d\n", mode);
        return NULL;
    }
    if (index >= eq_get_table_nsection(mode)) {
        log_e("index error %d\n", index);
        return NULL;
    }

    struct eq_seg_info *seg = (struct eq_seg_info *)eq_type_tab[mode];
    return &seg[index];
}

int get_cur_eq_tab(float *global_gain, struct eq_seg_info **seg)
{
    if (use_eq_tab_mark == USE_SDK_DEFAULT_EQ_TAB) {
        *global_gain = global_gain_tab[eq_mode];
        *seg = (struct eq_seg_info *)eq_type_tab[eq_mode];
        return tab_section_num[eq_mode];
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief   获取custom系数表的增益、频率
  @param   index:哪一段
  @param   freq:中心截止频率
  @param   gain:增益
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_set_custom_info(u16 index, int freq, float gain)
{
    struct eq_seg_info *seg = eq_mode_get_seg(EQ_MODE_CUSTOM, index);//获取某段eq系数
    if (!seg) {
        return -1;
    }
    seg->freq = freq;//修改freq gain
    seg->gain = gain;
    eq_mode_set_custom_seg(seg);//重设系数

    eq_mode_set(EQ_MODE_CUSTOM);//设置更新系数
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief   设置用custom系数表一段eq的增益
  @param   index:哪一段
  @param   gain:增益
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_set_custom_param(u16 index, int gain)
{
    struct eq_seg_info *seg = eq_mode_get_seg(EQ_MODE_CUSTOM, index);//获取某段eq系数
    if (!seg) {
        return -1;
    }
    seg->gain = gain;
    eq_mode_set_custom_seg(seg);//重设系数

    eq_mode_set(EQ_MODE_CUSTOM);//设置更新系数
    return 0;
}

s8 eq_mode_get_gain(EQ_MODE mode, u16 index)
{
    struct eq_seg_info *seg = eq_mode_get_seg(mode, index);
    if (!seg) {
        return 0;
    }
    return seg->gain;
}
/*----------------------------------------------------------------------------*/
/**@brief   获取某eq系数表一段eq的中心截止频率
  @param   mode:EQ_MODE_NORMAL, EQ_MODE_ROCK,EQ_MODE_POP,EQ_MODE_CLASSIC,EQ_MODE_JAZZ,EQ_MODE_COUNTRY, EQ_MODE_CUSTOM
  @param   index:哪一段
  @return  中心截止频率
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_get_freq(EQ_MODE mode, u16 index)
{
    struct eq_seg_info *seg = eq_mode_get_seg(mode, index);
    if (!seg) {
        return 0;
    }
    return seg->freq;
}
/*----------------------------------------------------------------------------*/
/**@brief   设置用custom系数表一段eq的增益
  @param   index:哪一段
  @param   gain:增益
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
void set_global_gain(EQ_MODE mode, float global_gain)
{
    global_gain_tab[mode] = global_gain;
    struct eq_adj eff = {0};

    for (int i = 0; i < ARRAY_SIZE(music_eq_name); i++) {
        eff.type = EQ_GLOBAL_GAIN_CMD;
        eff.param.global_gain =  global_gain_tab[mode];
        eff.fade_parm.fade_time = EQ_FADE_TIME;
        eff.fade_parm.fade_step = EQ_FADE_STEP;
        eff.fade_parm.f_fade_step = EQ_FADE_FREQ_STEP;
#if EQ_FADE_END_CALLBACK_EN
        eff.fade_parm.priv = NULL;
        eff.fade_parm.callback = audio_fade_end_callback;
#endif

        int ret = jlstream_set_node_param(NODE_UUID_EQ, music_eq_name[i], &eff, sizeof(eff));
        if (ret == true) {
            break;
        }
    }
}
/*
 *获取指定偏移内的eq配置
 *info结构通过jlstream_read_form_node_info_base接口获取
 * */
void eq_file_cfg_update(char *name, struct cfg_info info)
{
    struct eq_tool *tab = zalloc(info.size);
    int len = jlstream_read_form_cfg_data(&info, tab);

#if 0
    printf("tab->global_gain 0x%x\n", *(int *)&tab->global_gain);
    printf("tab->seg_num %d\n", tab->seg_num);
    for (int i = 0; i < tab->seg_num; i++) {
        struct eq_seg_info *seg = (struct eq_seg_info *)&tab->seg[i];
        printf("idx:%d, iir:%d, freq:%d, gain:0x%08x, q:0x%08x ", seg->index, seg->iir_type, seg->freq, *(int *)&seg->gain, *(int *)&seg->q);
    }
#endif
    if (!len) {
        printf("user eq cfg parm read err %d, %d\n", len, info.size);
        return;
    }
    if (tab->seg_num > AUDIO_EQ_MAX_SECTION) {
        printf("error:info.max_nsection(%d) > max(%d)\n", tab->seg_num, AUDIO_EQ_MAX_SECTION);
        return;
    }


    //运行时，直接设置更新
    struct eq_adj eff = {0};
    eff.type = EQ_GLOBAL_GAIN_CMD;
    eff.param.global_gain =  tab->global_gain;
    eff.fade_parm.fade_time = EQ_FADE_TIME;
    eff.fade_parm.fade_step = EQ_FADE_STEP;
    eff.fade_parm.f_fade_step = EQ_FADE_FREQ_STEP;
#if EQ_FADE_END_CALLBACK_EN
    eff.fade_parm.priv = NULL;
    eff.fade_parm.callback = audio_fade_end_callback;
#endif

    jlstream_set_node_param(NODE_UUID_EQ, name, &eff, sizeof(eff));

    eff.type = EQ_SEG_NUM_CMD;
    eff.param.seg_num = tab->seg_num;
    jlstream_set_node_param(NODE_UUID_EQ, name, &eff, sizeof(eff));



    for (int j = 0; j < tab->seg_num; j++) {
        eff.type = EQ_SEG_CMD;
        memcpy(&eff.param.seg, &tab->seg[j], sizeof(struct eq_seg_info));
        jlstream_set_node_param(NODE_UUID_EQ, name, &eff, sizeof(eff));
    }



    free(tab);
}

/*
 *调用本接口，可切换到eq节点内指定的配置项
 *name:eq节点的名字
 *cfg_index:eq配置项的序号
 * */
void eq_file_cfg_switch(char *name, char cfg_index)
{
    struct cfg_info info = {0};
    if (jlstream_read_form_node_info_base(MODE_INDEX_DEFAULT, name, cfg_index, &info)) {
        printf("user read eq node info err\n");
        return;
    }
    eq_file_cfg_update(name, info);

    if (!strncmp(name, "MusicEq", 7)) {
        eq_cfg_num = cfg_index;
        use_eq_tab_mark = USE_FILE_DEFAULT_EQ_TAB;
    }

}

// 使用过eq_file_cfg_switch接口时，本接口可获取musicEq的序号
int get_cur_eq_num(char *cfg_index)
{
    int ret = 0;
    if (use_eq_tab_mark == USE_FILE_DEFAULT_EQ_TAB) {
        *cfg_index = eq_cfg_num;
        ret = 1;
    }
    return ret;
}





void test_eq_switch(void *p)
{
    static u8 cnt = 0 ;
    cnt++;
    if (cnt == 3) {
        puts("-------------------------0\n");
        eq_file_cfg_switch("MusicEqBt", 0);
    } else if (cnt == 6) {
        puts("-------------------------1\n");
        eq_file_cfg_switch("MusicEqBt", 1);
        cnt = 0;
    }
}

#endif


