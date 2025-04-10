#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bt_name_parse.data.bss")
#pragma data_seg(".bt_name_parse.data")
#pragma const_seg(".bt_name_parse.text.const")
#pragma code_seg(".bt_name_parse.text")
#endif
#include "syscfg_id.h"
#include "gpio_config.h"
#include "gpadc.h"
#include "os/os_api.h"


struct bt_name_config {
    u8 len;
    char name[32];
    u16 value;
} __attribute__((packed));


int adc_get_ch_voltage(u8 adc_ch)
{
    return 0;
}

int bt_name_config_parse(char *bt_name)
{
    u8 buf[512];
    struct bt_name_config *config;

    int len = syscfg_read(CFG_ID_BT_NAME_SELECT, buf, 512);
    if (len < 0) {
        return 0;
    }

    int gpio = uuid2gpio((buf[2] << 8) | buf[1]);
    if (gpio == 0xff) {
        return 0;
    }
    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_HIGHZ);

    u32 adc_ch = adc_io2ch(gpio);
    ASSERT(adc_ch != 0xff);

    int adc_value = 0;
    for (int i = 0; i < 4; i++) {
        adc_value += adc_get_value_blocking(adc_ch);
    }
    adc_value /= 4;
    printf("bt_name_adc_value: %d\n", adc_value);

    int match = 0;

    for (int i = buf[0] + 1; i < len; i += sizeof(*config)) {
        config = (struct bt_name_config *)(buf + i);
        put_buf((u8 *)config, sizeof(*config));
        printf("i:%d %s %d\n", i, config->name, config->value);
        if (adc_value > config->value) {
            memcpy(bt_name, config->name, 32);
            match = 1;
            continue;
        }
    }

    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_HIGHZ);
    return match;
}
