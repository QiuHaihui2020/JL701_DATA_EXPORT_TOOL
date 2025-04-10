#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adkey_config.data.bss")
#pragma data_seg(".adkey_config.data")
#pragma const_seg(".adkey_config.text.const")
#pragma code_seg(".adkey_config.text")
#endif
#include "app_config.h"
#include "utils/syscfg_id.h"
#include "gpio_config.h"
#include "key/adkey.h"
#include "key_driver.h"
#include "adkey_config.h"


#if TCFG_ADKEY_ENABLE

static struct adkey_platform_data g_adkey_platform_data;



const struct adkey_platform_data *get_adkey_platform_data()
{
    const struct adkey_info *info = &g_adkey_data;
    u16 key_numbers;

    if (g_adkey_platform_data.enable) {
        return &g_adkey_platform_data;
    }

    key_numbers = ARRAY_SIZE(adkey_res_table);

    g_adkey_platform_data.adkey_pin = info->key_io;
    g_adkey_platform_data.extern_up_en = info->pull_up_type;
    g_adkey_platform_data.ad_channel = adc_io2ch(g_adkey_platform_data.adkey_pin);
    g_adkey_platform_data.long_press_enable = info->long_press_reset_enable;
    g_adkey_platform_data.long_press_time = info->long_press_reset_time;

    if (g_adkey_platform_data.ad_channel == 0xff) {
        puts("ERR:Please check that the ad key is correct\n");
        return NULL;
    }

    printf("key_numbers: %d\n", key_numbers);
    printf("adkey_pin: %d\n", g_adkey_platform_data.adkey_pin);
    printf("extern_up_en: %d\n", g_adkey_platform_data.extern_up_en);
    printf("pull_up_value: %d\n", info->pull_up_value);
    printf("ad_channel: %d\n", g_adkey_platform_data.ad_channel);
    printf("long_press_enable:%d time:%ds\n", g_adkey_platform_data.long_press_enable, g_adkey_platform_data.long_press_time);

    u16 ordering_res[CONFIG_ADKEY_MAX_NUM][2];

    //记录电阻与序号
    for (u16 i = 0; i < key_numbers; i++) {
        ordering_res[i][0] =  adkey_res_table[i].res_value;
        ordering_res[i][1] =  i;
        /* printf("ordering_res:%d,s_number:%d\n", ordering_res[i][0], ordering_res[i][1]); */
    }
    //按电阻从小到大排序,序号要跟随电阻值
    for (u16 i = 0; i < key_numbers - 1; i++) {
        for (u16 j = 0; j < key_numbers - 1 - i; j++) {
            if (ordering_res[j][0] > ordering_res[j + 1][0]) {
                u16 tmp = ordering_res[j][0];
                ordering_res[j][0] = ordering_res[j + 1][0];
                ordering_res[j + 1][0] = tmp;

                tmp = ordering_res[j][1];
                ordering_res[j][1] = ordering_res[j + 1][1];
                ordering_res[j + 1][1] = tmp;
            }
        }
    }

    /*for test查看排序后的数据*/
    /* for (i = 0; i < key_numbers; i++) { */
    /*     printf("after ordering res:%d,s_number:%d\n", ordering_res[i][0], ordering_res[i][1]); */
    /* } */

    //配置 ad判断值与key值
    u16 tmp_value1, tmp_value2;
    for (u16 i = 0; i < key_numbers; i++) {
        tmp_value1 = info->max_ad_value * ordering_res[i][0] / (ordering_res[i][0] + info->pull_up_value);
        if (i == key_numbers - 1) {
            tmp_value2 = info->max_ad_value;
        } else {
            tmp_value2 = info->max_ad_value * ordering_res[i + 1][0] / (ordering_res[i + 1][0] + info->pull_up_value);
        }
        g_adkey_platform_data.ad_value[i] = (tmp_value1 + tmp_value2) / 2;
        g_adkey_platform_data.key_value[i] = adkey_res_table[ordering_res[i][1]].key_value;

        printf("ADkey:%d,judge_advalue:%d,key_value:%d\n", i, g_adkey_platform_data.ad_value[i], g_adkey_platform_data.key_value[i]);
    }

    g_adkey_platform_data.enable    = 1;

    return &g_adkey_platform_data;
}


bool is_adkey_press_down()
{
#if TCFG_ADKEY_ENABLE
    if (g_adkey_platform_data.enable == 0) {
        return false;
    }

    if (adc_get_value(g_adkey_platform_data.ad_channel) < 10) {
        return true;
    }
#endif
    return false;
}

int get_adkey_io()
{
#if TCFG_ADKEY_ENABLE
    if (g_adkey_platform_data.enable) {
        return g_adkey_platform_data.adkey_pin;
    }
#endif
    return -1;
}

#endif

