#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".energy_detect_demo.data.bss")
#pragma data_seg(".energy_detect_demo.data")
#pragma const_seg(".energy_detect_demo.text.const")
#pragma code_seg(".energy_detect_demo.text")
#endif

#include "effects/effects_adj.h"
#include "media/audio_energy_detect.h"
#include "node_uuid.h"

void user_energy_detect_get_parm_demo()
{
    struct audio_get_energy energy = {0};
    memcpy(energy.name, "enerydet_name", strlen("enerydet_name"));//节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
    energy.tar_ch = BIT(0);//通道0
    int ret = jlstream_get_node_param(NODE_UUID_ENERGY_DETECT, energy.name, &energy, sizeof(energy));
    if (ret == sizeof(energy)) {
        printf("energy %d\n", energy.energy);
    }
}
