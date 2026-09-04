/**
* 注意点：
* 0.此文件变化，在工具端会自动同步修改到工具配置中
* 1.功能块通过【---------xxx------】和 【#endif // xxx 】，是工具识别的关键位置，请勿随意改动
* 2.目前工具暂不支持非文件已有的C语言语法，此文件应使用文件内已有的语法增加业务所需的代码，避免产生不必要的bug
* 3.修改该文件出现工具同步异常或者导出异常时，请先检查文件内语法是否正常
**/


// ------------公共配置.json------------
const int CONFIG_LEA_PRODUCT_TEST_EN = 0; // 产测使能
const int CONFIG_LEA_PRODUCT_TEST_RSSI = 50; // 信号强度控制
// ------------公共配置.json------------

#if TCFG_IO_CFG_AT_POWER_ON
const struct gpio_cfg_item g_io_cfg_at_poweron [] =  {
    {
        .gpio = IO_PORTA_06,
        .mode = PORT_OUTPUT_LOW,
        .hd = PORT_DRIVE_STRENGT_64p0mA
    },
    {
        .gpio = IO_PORTA_07,
        .mode = PORT_OUTPUT_HIGH,
        .hd = PORT_DRIVE_STRENGT_64p0mA
    }
};
#endif // TCFG_IO_CFG_AT_POWER_ON

#if TCFG_IO_CFG_AT_POWER_OFF
const struct gpio_cfg_item g_io_cfg_at_poweroff [] =  {
    
};
#endif // TCFG_IO_CFG_AT_POWER_OFF

#if TCFG_IOKEY_ENABLE
const struct iokey_info g_iokey_info [] =  {
    {
        .key_value = KEY_IO_NUM0,
        .key_io = IO_PORTB_01,
        .detect = 0,
        .long_press_reset_enable = 0,
        .long_press_reset_time = 8
    },
    {
        .key_value = KEY_IO_NUM1,
        .key_io = IO_PORTB_02,
        .detect = 0,
        .long_press_reset_enable = 0,
        .long_press_reset_time = 8
    },
    {
        .key_value = KEY_IO_NUM2,
        .key_io = IO_PORTB_02,
        .detect = 0,
        .long_press_reset_enable = 0,
        .long_press_reset_time = 8
    },
    {
        .key_value = KEY_IO_NUM3,
        .key_io = IO_PORTA_11,
        .detect = 0,
        .long_press_reset_enable = 0,
        .long_press_reset_time = 8
    }
};
#endif // TCFG_IOKEY_ENABLE

#if TCFG_ADKEY_ENABLE
const struct adkey_res_value adkey_res_table [] =  {
    {
        .key_value = KEY_AD_NUM0,
        .res_value = 0
    },
    {
        .key_value = KEY_AD_NUM1,
        .res_value = 330
    },
    {
        .key_value = KEY_AD_NUM2,
        .res_value = 510
    },
    {
        .key_value = KEY_AD_NUM3,
        .res_value = 1000
    }
};

const struct adkey_info g_adkey_data =  {
    .key_io = IO_PORTB_01,
    .pull_up_type = 1,
    .pull_up_value = 220,
    .max_ad_value = 1023,
    .long_press_reset_enable = 0,
    .long_press_reset_time = 8,
    .res_table = adkey_res_table
};
#endif // TCFG_ADKEY_ENABLE

#if TCFG_IRKEY_ENABLE
const struct ff00_2_keynum g_irkey_table [] =  {
    {
        .key_value = KEY_IR_NUM0,
        .source_value = 0x45
    },
    {
        .key_value = KEY_IR_NUM1,
        .source_value = 0x46
    },
    {
        .key_value = KEY_IR_NUM2,
        .source_value = 0x47
    },
    {
        .key_value = KEY_IR_NUM3,
        .source_value = 0x44
    },
    {
        .key_value = KEY_IR_NUM4,
        .source_value = 0x40
    },
    {
        .key_value = KEY_IR_NUM5,
        .source_value = 0x43
    },
    {
        .key_value = KEY_IR_NUM6,
        .source_value = 0x07
    },
    {
        .key_value = KEY_IR_NUM7,
        .source_value = 0x15
    },
    {
        .key_value = KEY_IR_NUM8,
        .source_value = 0x09
    },
    {
        .key_value = KEY_IR_NUM9,
        .source_value = 0x16
    },
    {
        .key_value = KEY_IR_NUM10,
        .source_value = 0x19
    },
    {
        .key_value = KEY_IR_NUM11,
        .source_value = 0x0d
    },
    {
        .key_value = KEY_IR_NUM12,
        .source_value = 0x0c
    },
    {
        .key_value = KEY_IR_NUM13,
        .source_value = 0x18
    },
    {
        .key_value = KEY_IR_NUM14,
        .source_value = 0x5e
    },
    {
        .key_value = KEY_IR_NUM15,
        .source_value = 0x08
    },
    {
        .key_value = KEY_IR_NUM16,
        .source_value = 0x1c
    },
    {
        .key_value = KEY_IR_NUM17,
        .source_value = 0x5a
    },
    {
        .key_value = KEY_IR_NUM18,
        .source_value = 0x42
    },
    {
        .key_value = KEY_IR_NUM19,
        .source_value = 0x52
    },
    {
        .key_value = KEY_IR_NUM20,
        .source_value = 0x4a
    }
};

const struct irkey_info g_irkey_data =  {
    .key_io = IO_PORTC_08,
    .irkey_table = g_irkey_table
};
#endif // TCFG_IRKEY_ENABLE

#if TCFG_BATTERY_CURVE_ENABLE
const struct battery_curve g_battery_curve_table [] =  {
    {
        .voltage = 3300,
        .percent = 0
    },
    {
        .voltage = 3450,
        .percent = 5
    },
    {
        .voltage = 3680,
        .percent = 10
    },
    {
        .voltage = 3740,
        .percent = 20
    },
    {
        .voltage = 3770,
        .percent = 30
    },
    {
        .voltage = 3790,
        .percent = 40
    },
    {
        .voltage = 3820,
        .percent = 50
    },
    {
        .voltage = 3870,
        .percent = 60
    },
    {
        .voltage = 3920,
        .percent = 70
    },
    {
        .voltage = 3980,
        .percent = 80
    },
    {
        .voltage = 4060,
        .percent = 90
    },
    {
        .voltage = 4120,
        .percent = 100
    }
};
#endif // TCFG_BATTERY_CURVE_ENABLE