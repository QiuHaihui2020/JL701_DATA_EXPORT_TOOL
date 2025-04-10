#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".crossover_demo.data.bss")
#pragma data_seg(".crossover_demo.data")
#pragma const_seg(".crossover_demo.text.const")
#pragma code_seg(".crossover_demo.text")
#endif

#include "effects/effects_adj.h"
#include "effects/audio_crossover.h"
#include "node_uuid.h"

void user_crossover_udpate_parm_demo()
{
    crossover_param_tool_set cfg = {0};
    /*
     *解析配置文件内效果配置
     * */
    char mode_index       = 0;               //模式序号（当前节点无多参数组，mode_index是0）
    char *node_name       = "cross_name";    //节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
    char cfg_index        = 0;               //目标配置项序号（当前节点无多参数组，cfg_index是0）
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err\n");
        return;
    }

    /*
     *将配置文件内获取得到的参数更新到目标节点
     * */
    if (cfg.parm.way_num == 3) {
        /*
         *三段分频器参数更新
         * */
        jlstream_set_node_param(NODE_UUID_CROSSOVER, node_name, &cfg, sizeof(cfg));//
    } else {
        /*
         *两段分频器参数更新
         * */
        jlstream_set_node_param(NODE_UUID_CROSSOVER_2BAND, node_name, &cfg, sizeof(cfg));//
    }
}

