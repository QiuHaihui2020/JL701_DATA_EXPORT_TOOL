#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".eq_demo.data.bss")
#pragma data_seg(".eq_demo.data")
#pragma const_seg(".eq_demo.text.const")
#pragma code_seg(".eq_demo.text")
#endif

#include "effects/effects_adj.h"
#include "effects/eq_config.h"
#include "node_uuid.h"


/*
 *用户自定义系数更新例子
 * */
void user_eq_update_custom_parm_demo()
{
    {
        /*
         *更新用户自定义总增益
         * */
        struct eq_adj eff = {0};
        char *node_name = "eq_name";//节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
        eff.type = EQ_GLOBAL_GAIN_CMD;      //参数类型：总增益
        eff.param.global_gain =  -5;       //设置总增益 -5dB
        eff.fade_parm.fade_time = 10;      //ms，淡入timer执行的周期
        eff.fade_parm.fade_step = 0.1f;    //淡入步进
        int ret = jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));
        if (ret) {
            puts("ok\n");
        }
    }

    {
        /*
         *更新系数表段数(如系数表段数发生改变，需要用以下处理，更新系数表段数)
         * */
        struct eq_adj eff = {0};
        char *node_name = "eq_name";//节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
        eff.type = EQ_SEG_NUM_CMD;         //参数类型：系数表段数
        eff.param.seg_num = 10 ;           //系数表段数
        eff.fade_parm.fade_time = 10;      //ms，淡入timer执行的周期
        eff.fade_parm.fade_step = 0.1f;    //淡入步进
        int ret = jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));
        if (ret) {
            puts("ok\n");
        }
    }
    {
        /*
         *更新用户自定义的系数表
         * */
        struct eq_seg_info eq_tab_normal[] = {
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

        struct eq_adj eff = {0};
        char *node_name = "eq_name";//节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
        eff.type = EQ_SEG_CMD;             //参数类型：滤波器参数
        eff.fade_parm.fade_time = 10;      //ms，淡入timer执行的周期
        eff.fade_parm.fade_step = 0.1f;    //淡入步进
        int nsection = ARRAY_SIZE(eq_tab_normal);//滤波器段数
        for (int j = 0; j < nsection; j++) {
            memcpy(&eff.param.seg, &eq_tab_normal[j], sizeof(struct eq_seg_info));//滤波器参数
            jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));
        }
    }

}

void user_eq_update_file_parm_demo()
{
    /*
     *解析配置文件内效果配置
     * */
    struct cfg_info info  = {0};             //节点配置相关信息（参数存储的目标地址、配置项大小）
    char mode_index       = 0;               //模式序号（当前节点无多参数组，mode_index是0）
    char *node_name       = "eq_name";       //节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
    char cfg_index        = 0;               //目标配置项序号（当前节点无多参数组，cfg_index是0）
    int ret = jlstream_read_form_node_info_base(mode_index, node_name, cfg_index, &info);
    if (!ret) {
        struct eq_tool *tab = zalloc(info.size);
        if (!jlstream_read_form_cfg_data(&info, tab)) {
            printf("user eq cfg parm read err\n");
            free(tab);
            return;
        }

        //运行时，直接设置更新
        struct eq_adj eff = {0};
        eff.type = EQ_GLOBAL_GAIN_CMD;
        eff.param.global_gain =  tab->global_gain;
        eff.fade_parm.fade_time = 10;        //ms，淡入timer执行的周期
        eff.fade_parm.fade_step = 0.1f;      //淡入步进
        jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));//更新总增益

        eff.type = EQ_SEG_NUM_CMD;
        eff.param.seg_num = tab->seg_num;
        jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));//更新滤波器段数

        for (int j = 0; j < tab->seg_num; j++) {
            eff.type = EQ_SEG_CMD;
            memcpy(&eff.param.seg, &tab->seg[j], sizeof(struct eq_seg_info));
            jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));//更新滤波器系数
        }
        free(tab);
    }
}

