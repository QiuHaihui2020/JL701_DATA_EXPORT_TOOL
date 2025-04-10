#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".spectrum_demo.data.bss")
#pragma data_seg(".spectrum_demo.data")
#pragma const_seg(".spectrum_demo.text.const")
#pragma code_seg(".spectrum_demo.text")
#endif

#include "effects/effects_adj.h"
#include "effects/spectrum/spectrum_fft.h"
#include "node_uuid.h"



void user_spectrum_udpate_parm_demo()
{
    spectrum_effect_param_tool_set cfg = {0};
    /*
     *解析配置文件内效果配置
     * */
    char mode_index       = 0;               //模式序号（当前节点无多参数组，mode_index是0）
    char *node_name       = "spec_name";       //节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
    char cfg_index        = 0;               //目标配置项序号（当前节点无多参数组，cfg_index是0）
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err\n");
        return;
    }
    /*
     *将配置文件内获取得到的参数更新到目标节点
     * */
    jlstream_set_node_param(NODE_UUID_NOISEGATE, node_name, &cfg, sizeof(cfg));
}


void user_spectrum_get_param()
{
    struct spectrum_parm  cfg = {0};

    char *node_name       = "Spectrum2";     //节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
    int ret = jlstream_get_node_param(NODE_UUID_SPECTRUM, node_name, &cfg, sizeof(cfg));
    if (ret == sizeof(cfg)) {
        for (int i = 0; i < cfg.db_num; i++) {
            //输出db_num个 db值
            printf("db_data db[%02d] %04d %05d\n", i, cfg.db_data[i], (int)cfg.center_freq[i]);
        }
    }
}


#include "effects/spectrum/spectrum_eq.h"
//spectrum advance节点频谱值获取
///注册频谱能量计算的回调函数，有注册则可以通过该回调返回频谱之，见函数audio_spectrum_advance_handler. SDK默认使用回调函数输出频谱值
//没注册频谱能量计算的回调函数情况下,也可直接通过jlstream_get_node_param接口获取
void user_spectrum_advance_get_param()
{
    struct spectrum_advance_parm cfg = {0};
    char *node_name       = "spectrumAdv";     //节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
    int ret = jlstream_get_node_param(NODE_UUID_SPECTRUM_ADVANCE, node_name, &cfg, sizeof(cfg));
    if (ret == sizeof(cfg)) {
        for (int i = 0; i < cfg.section; i++) {
            //输出db_num个 db值
            printf("left:dB[%d] %d, right:dB[%d] %d\n", i, (int)cfg.db_data[0][i], i, (int)cfg.db_data[1][i]);//dB值是浮点
        }
    }
}
