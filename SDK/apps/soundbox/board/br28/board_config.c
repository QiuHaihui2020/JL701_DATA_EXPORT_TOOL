#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".board_config.data.bss")
#pragma data_seg(".board_config.data")
#pragma const_seg(".board_config.text.const")
#pragma code_seg(".board_config.text")
#endif
// V300挪去sdk_board_config.c
/* #include "gpadc.h" */
/* #include "fm_manage.h" */
/* #include "app_config.h" */
/* #include "app_power_config.h" */
/*  */
/* #if TCFG_APP_FM_EN */
/* #include "fm_manage.h" */
/*  */
/* FM_DEV_PLATFORM_DATA_BEGIN(fm_dev_data) */
/* .iic_hdl = 0, */
/*  .iic_delay = 50, */
/*   FM_DEV_PLATFORM_DATA_END(); */
/*  */
/* #endif */
/*  */
/* void board_init() */
/* { */
/*     board_power_init(); */
/*  */
/*     adc_init(); */
/*  */
/* #if TCFG_APP_FM_EN */
/*     y_printf(">> Func:%s, Line:%d, call: fm_dev_init Func!\n", __func__, __LINE__); */
/*     fm_dev_init((void *)(&fm_dev_data)); */
/* #endif */
/*  */
/* } */
