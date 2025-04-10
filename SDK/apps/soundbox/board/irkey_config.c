#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".irkey_config.data.bss")
#pragma data_seg(".irkey_config.data")
#pragma const_seg(".irkey_config.text.const")
#pragma code_seg(".irkey_config.text")
#endif
#include "app_config.h"
#include "gpio_config.h"
#include "key/irkey.h"
#include "key_driver.h"
#include "utils/syscfg_id.h"


#if TCFG_IRKEY_ENABLE

static struct irkey_platform_data g_irkey_platform_data;


const struct irkey_platform_data *get_irkey_platform_data()
{
    if (g_irkey_platform_data.enable) {
        return &g_irkey_platform_data;
    }

    g_irkey_platform_data.port = g_irkey_data.key_io;
    g_irkey_platform_data.num = ARRAY_SIZE(g_irkey_table);
    g_irkey_platform_data.IRff00_2_keynum = g_irkey_table;

    printf("irkey int: num = %d\n", g_irkey_platform_data.num);

    g_irkey_platform_data.enable    = 1;

    return &g_irkey_platform_data;
}

#endif


int get_irkey_io()
{
#if TCFG_IRKEY_ENABLE
    if (g_irkey_platform_data.enable) {
        return g_irkey_platform_data.port;
    }
#endif
    return -1;
}

