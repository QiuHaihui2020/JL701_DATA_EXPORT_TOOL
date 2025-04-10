#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ntc_det.data.bss")
#pragma data_seg(".ntc_det.data")
#pragma const_seg(".ntc_det.text.const")
#pragma code_seg(".ntc_det.text")
#endif
//*********************************************************************************//
//                                NTC det 									       //
//*********************************************************************************//
#include "app_config.h"
#include "ntc_det_api.h"
#include "system/includes.h"
#include "asm/charge.h"
#include "asm/efuse.h"
#include "gpadc.h"
#include "app_msg.h"

#if NTC_DET_EN

#define NTC_DET_BAD_RES      0    //分压电阻损坏关闭检测

#ifndef NTC_DET_DUTY1
#define NTC_DET_DUTY1        5000 //检测周期
#endif

#ifndef NTC_DET_DUTY2
#define NTC_DET_DUTY2        10   //检测小周期
#endif

#ifndef NTC_DET_CNT
#define NTC_DET_CNT          3    //检测次数
#endif

#define NTC_DET_AD_CH        ntc_get_detect_ch()

#ifndef NTC_DET_UPPER
#define NTC_DET_UPPER        ntc_get_detect_upper(NTC_UPPER_RES)  //正常范围AD值上限，0度时
#endif

#ifndef NTC_DET_LOWER
#define NTC_DET_LOWER        ntc_get_detect_lower(NTC_LOWER_RES)  //正常范围AD值下限，45度时
#endif

#define NTC_IS_NORMAL(value, offset) (value >= NTC_DET_LOWER+(offset) && value <= NTC_DET_UPPER-(offset))
#define NTC_IS_BAD_RES(value) (value >= 1020 || value <= 5)

#define PULLUPRES_IS_TRIM           (efuse_get_io_pu_100k() != 0xFF)
#define GET_PULLUPRES_SIGN          ((efuse_get_io_pu_100k() & 0x80) ? -1 : 1)
#define GET_PULLUPRES_TRIM_SIZE     (efuse_get_io_pu_100k() & 0x7F)

enum {
    NTC_STATE_NORMAL = 0,
    NTC_STATE_ABNORMAL,
};

struct ntc_det_t {
    u16 normal_cnt : 4; //温度正常的次数
    u16 cnt : 4; //温度检测的次数
    u16 res_cnt : 4; //分压电阻脱落或损坏
    u16 state : 1; //是否超出范围
    u16 timer;
};
static struct ntc_det_t ntc_det = {0};
extern u8 get_charge_full_flag(void);

u16 ntc_det_working()
{
    return ntc_det.timer;
}

/* ***************************************************************************/
/**
 * \Brief :        获取内部上拉电阻校准阻值
 *
 * \Return :       内部上拉电阻校准值
 */
/* ***************************************************************************/
u32 ntc_get_inside_pull_up_res_trim_value(void)
{
    u32 res_value = (u32)((3072 - (524 + GET_PULLUPRES_TRIM_SIZE * GET_PULLUPRES_SIGN) * 3) / 0.015f);
    return res_value;
}

/* ***************************************************************************/
/**
 * \Brief :        获取NTC检测ADC通道
 *
 * \Return :       NTC检测通道
 */
/* ***************************************************************************/
u32 ntc_get_detect_ch(void)
{
    return adc_io2ch(NTC_DETECT_IO);
}

/* ***************************************************************************/
/**
 * \Brief :        获取正常温度范围AD值下限
 *
 * \Param :        ntc_res:0°时NTC的阻值
 *
 * \Return :       根据上拉电阻计算得到的0°时的采样值
 */
/* ***************************************************************************/
u32 ntc_get_detect_upper(u32 ntc_res)
{
    u32 pull_up_res;
    u32 upper_value;

    if (NTC_DET_PULLUP_TYPE && PULLUPRES_IS_TRIM) {
        pull_up_res = ntc_get_inside_pull_up_res_trim_value();
    } else {
        pull_up_res = 100000;
    }

    upper_value = 1023 * ntc_res / (ntc_res + pull_up_res);

    return upper_value;
}

/* ***************************************************************************/
/**
 * \Brief :        获取正常温度范围AD值下限
 *
 * \Param :        ntc_res:45°时NTC的阻值
 *
 * \Return :       根据上拉电阻计算得到的45°时的采样值
 */
/* ***************************************************************************/
u32 ntc_get_detect_lower(u32 ntc_res)
{
    u32 pull_up_res;
    u32 lower_value;

    if (NTC_DET_PULLUP_TYPE && PULLUPRES_IS_TRIM) {
        pull_up_res = ntc_get_inside_pull_up_res_trim_value();
    } else {
        pull_up_res = 100000;
    }

    lower_value = 1023 * ntc_res / (ntc_res + pull_up_res);

    return lower_value;
}

static void ntc_det_timer_deal(void *priv)
{
    u32 value;

#if NTC_DET_CNT
    if (ntc_det.cnt == 0) {
        sys_timer_modify(ntc_det.timer, NTC_DET_DUTY2);
    }
#endif
    value = adc_get_value(NTC_DET_AD_CH);
    printf("%d", value);

    ntc_det.cnt++;
    if (NTC_IS_NORMAL(value, ntc_det.state * 8)) { //温度恢复一定范围后才算正常，防止临界状态
        ntc_det.normal_cnt++;
    } else if (NTC_IS_BAD_RES(value)) {
        ntc_det.res_cnt++;
    }
    if (ntc_det.cnt >= NTC_DET_CNT) {
        if (ntc_det.normal_cnt > NTC_DET_CNT / 2) {
            if (ntc_det.state == NTC_STATE_ABNORMAL) {
                printf("temperature recover, start charge");
                ntc_det.state = NTC_STATE_NORMAL;
                charge_start();
            }
        }
#if NTC_DET_BAD_RES
        else if (ntc_det.res_cnt > NTC_DET_CNT / 2) {
            printf("bad res, stop det");
            ntc_det_stop();
        }
#endif
        else {
            if (ntc_det.state == NTC_STATE_NORMAL) {
                printf("temperature is abnormall, stop charge");
                ntc_det.state = NTC_STATE_ABNORMAL;
                charge_close();
                CHARGE_EN(0);
            }
            /* power_set_soft_poweroff(); */
        }
        ntc_det.cnt = 0;
        ntc_det.res_cnt = 0;
        ntc_det.normal_cnt = 0;
        sys_timer_modify(ntc_det.timer, NTC_DET_DUTY1);
    }
}

void ntc_det_start(void)
{
    if (ntc_det.timer == 0) {
        printf("ntc det start");
        memset(&ntc_det, 0, sizeof(ntc_det));
        if (NTC_DET_PULLUP_TYPE && PULLUPRES_IS_TRIM) {
            /* 内部上拉 */
            printf("PULLUP_RES: %d", ntc_get_inside_pull_up_res_trim_value());
            gpio_set_mode(IO_PORT_SPILT(IO_PORTC_05), PORT_INPUT_PULLUP_100K);
        } else {
            /* 外部上拉 */
            gpio_set_mode(IO_PORT_SPILT(NTC_POWER_IO), PORT_OUTPUT_HIGH);

            gpio_set_mode(IO_PORT_SPILT(NTC_DETECT_IO), PORT_HIGHZ);
        }
        adc_add_sample_ch(NTC_DET_AD_CH);
        printf("NTC_DET_UPPER: %d NTC_DET_LOWER: %d", NTC_DET_UPPER, NTC_DET_LOWER);
        ntc_det.timer = sys_timer_add(NULL, ntc_det_timer_deal, NTC_DET_DUTY1);
    }
}

void ntc_det_stop(void)
{
    if (!get_charge_full_flag() && get_charge_online_flag() && ntc_det.state == NTC_STATE_ABNORMAL) {
        printf("charge protecting, wait recover");
        return;
    }
    if (ntc_det.timer) {
        printf("ntc det stop");
        sys_timer_del(ntc_det.timer);
        ntc_det.timer = 0;
        adc_delete_ch(NTC_DET_AD_CH);
        gpio_set_mode(IO_PORT_SPILT(NTC_POWER_IO), PORT_HIGHZ);
    }
}

#if TCFG_CHARGE_ENABLE

static int ntc_msg_entry(int *msg)
{
    switch (msg[0]) {
    case CHARGE_EVENT_CHARGE_START:
        ntc_det_start();
        break;
    case CHARGE_EVENT_CHARGE_CLOSE:
        ntc_det_stop();
        break;
    case CHARGE_EVENT_LDO5V_KEEP:
        ntc_det_stop();
    }
    return 0;
}

APP_MSG_PROB_HANDLER(ntc_event_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BATTERY,
    .handler    = ntc_msg_entry,
};

#endif

#endif

