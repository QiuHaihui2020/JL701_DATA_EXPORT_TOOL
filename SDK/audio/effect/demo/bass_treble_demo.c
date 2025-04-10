#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bass_treble_demo.data.bss")
#pragma data_seg(".bass_treble_demo.data")
#pragma const_seg(".bass_treble_demo.text.const")
#pragma code_seg(".bass_treble_demo.text")
#endif

#include "effects/effects_adj.h"
#include "effects/audio_bass_treble_eq.h"
#include "node_uuid.h"


void user_bass_treble_udpate_parm_demo()
{
    /*
     *高低音参数增益更新
     * */
    {
        /*
         *低音
         * */
        char *node_name = "MusicBassTre";//节点的第一个参数，用户自定义
        struct bass_treble_parm par = {0};
        par.type = BASS_TREBLE_PARM_SET;  //参数更新类型
        par.gain.index = BASS_TREBLE_LOW; //低音
        par.gain.gain = -1;               //-1dB
        jlstream_set_node_param(NODE_UUID_BASS_TREBLE, node_name, &par, sizeof(par));
    }

    {
        /*
         *中音
         * */
        char *node_name = "MusicBassTre";//节点的第一个参数，用户自定义
        struct bass_treble_parm par = {0};
        par.type = BASS_TREBLE_PARM_SET;  //参数更新类型
        par.gain.index = BASS_TREBLE_MID; //低音
        par.gain.gain = -2;               //-2dB
        jlstream_set_node_param(NODE_UUID_BASS_TREBLE, node_name, &par, sizeof(par));
    }

    {
        /*
         *高音
         * */
        char *node_name = "MusicBassTre";//节点的第一个参数，用户自定义
        struct bass_treble_parm par = {0};
        par.type = BASS_TREBLE_PARM_SET;  //参数更新类型
        par.gain.index = BASS_TREBLE_HIGH; //低音
        par.gain.gain = -3;               //-3dB
        jlstream_set_node_param(NODE_UUID_BASS_TREBLE, node_name, &par, sizeof(par));
    }

    {
        /*
         *总增益更新
         * */
        char *node_name = "MusicBassTre";//节点的第一个参数，用户自定义
        struct bass_treble_parm par = {0};
        par.type = BASS_TREBLE_PARM_SET_GLOBAL_GAIN;
        par.global_gain = -4;             //-4dB
        jlstream_set_node_param(NODE_UUID_BASS_TREBLE, node_name, &par, sizeof(par));
    }
}

