#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".hdmi_dete.data.bss")
#pragma data_seg(".hdmi_dete.data")
#pragma const_seg(".hdmi_dete.text.const")
#pragma code_seg(".hdmi_dete.text")
#endif

/*************************************************************
  此文件函数主要是 HDMI 插入检测
 **************************************************************/

#include "app_config.h"
#include "system/event.h"
#include "system/init.h"
#include "system/timer.h"
#include "asm/power_interface.h"
#include "gpadc.h"
#include "gpio.h"
#include "app_msg.h"
#include "gpio_config.h"
#include "hdmi_cec_api.h"
#include "app_main.h"
#include "spdif_file.h"


#if TCFG_APP_SPDIF_EN

#define HDMI_DETECT_CNT   6//滤波计算

struct hdmi_dev_opr {
    u8 cnt;//滤波计算
    u8 stu;//当前状态
    u16 timer;//定时器句柄
    u8 online: 1; //是否在线
    u8 active: 1; //进入sniff的判断标志
    u8 init: 1; //初始化完成标志
    u8 step: 3; //检测阶段
    u8 det_port;
    u8 cec_port;
    u8 det_mode;
};
static struct hdmi_dev_opr hdmi_dev_hdl = {0};
#define __this 	(&hdmi_dev_hdl)

/*----------------------------------------------------------------------------*/
/*@brief   获取hdmi是否在线
  @param
  @return  1:在线 0：不在线
  @note    app通过这个接口判断hdmi是否在线
 */
/*----------------------------------------------------------------------------*/
u8 hdmi_is_online(void)
{
    /* #if ((defined TCFG_LINEIN_DETECT_ENABLE) && (TCFG_LINEIN_DETECT_ENABLE == 0)) */
    /* return 1; */
    /* #else */
    return __this->online;
    /* #endif//TCFG_LINEIN_DETECT_ENABLE */

}

/*----------------------------------------------------------------------------*/
/*@brief   设置hdmi是否在线
  @param    1:在线 0：不在线
  @return  null
  @note    检测驱动通过这个接口设置hdmi是否在线
 */
/*----------------------------------------------------------------------------*/
void hdmi_set_online(u8 online)
{
    __this->online = online ? true : false;
}

/*----------------------------------------------------------------------------*/
/*@brief   设备上下线推送切模式消息
  @param    上下线消息
 */
/*----------------------------------------------------------------------------*/
void hdmi_event_notify(u8 arg)
{
    if (arg == DEVICE_EVENT_IN) {
        if (true != app_in_mode(APP_MODE_SPDIF)) {
#if (TCFG_BT_BACKGROUND_ENABLE)
            /*一些情况不希望退出蓝牙模式*/
            if (bt_background_check_if_can_enter()) {
                puts("bt_background can not enter\n");
            } else {
                app_send_message2(APP_MSG_GOTO_MODE, APP_MODE_SPDIF, 0);
            }
#else
            app_send_message2(APP_MSG_GOTO_MODE, APP_MODE_SPDIF, 0);
#endif
        }
        if (__this->cec_port != 0xff) {
            /* hdmi_cec_init(__this->cec_port, 1); */
            hdmi_cec_init(__this->cec_port, 0);
        }

    } else if (arg == DEVICE_EVENT_OUT) {
        hdmi_cec_close();
    }
}

/*----------------------------------------------------------------------------*/
/*@brief   检测前使能io
  @param    null
  @return  null
  @note    检测驱动检测前使能io ，检测完成后设为高阻 可以节约功耗
  (io检测、sd复用ad检测动态使用，单独ad检测不动态修改)
 */
/*----------------------------------------------------------------------------*/
static void hdmi_io_start(void)
{
    if (__this->init) {
        return ;
    }
    __this->init = 1;
    /* gpio_set_mode(IO_PORT_SPILT(__this->det_port), PORT_INPUT_PULLUP_10K); */
    gpio_set_mode(IO_PORT_SPILT(__this->det_port), PORT_INPUT_PULLDOWN_10K);
}

/*----------------------------------------------------------------------------*/
/*@brief   检测完成关闭使能io
  @param    null
  @return  null
  @note    检测驱动检测前使能io ，检测完成后设为高阻 可以节约功耗
  (io检测、sd复用ad检测动态使用，单独ad检测不动态修改)
 */
/*----------------------------------------------------------------------------*/
static void hdmi_io_stop(void)
{
    if (!__this->init) {
        return ;
    }
    __this->init = 0;
    gpio_set_mode(IO_PORT_SPILT(__this->det_port), PORT_HIGHZ);
    /* gpio_set_drive_strength(__this->det_port / 16, BIT(__this->det_port % 16), PORT_DRIVE_STRENGT_2p4mA); */
}

/*----------------------------------------------------------------------------*/
/*@brief   检测是否在线
  @param    驱动句柄
  @return    1:有设备插入 0：没有设备插入
  @note    检测驱动检测前使能io ，检测完成后设为高阻 可以节约功耗
  (io检测、sd复用ad检测动态使用，单独ad检测不动态修改)
 */
/*----------------------------------------------------------------------------*/
static int hdmi_sample_detect(void *arg)
{
    u8 cur_stu;
    hdmi_io_start();
    /* cur_stu = gpio_read(__this.det_port) ? false : true; */
    cur_stu = gpio_read(__this->det_port) ? true : false;
    hdmi_io_stop();
    /* cur_stu	= !cur_stu; */
    /* putchar('A' + cur_stu); */
    return cur_stu;
}

/*----------------------------------------------------------------------------*/
/*@brief   注册的定时器回调检测函数
  @param    驱动句柄
  @return    null
  @note    定时进行检测
 */
/*----------------------------------------------------------------------------*/
static void hdmi_detect(void *arg)
{
    int cur_stu;
    extern u8 bt_trim_status_get();
    if (bt_trim_status_get()) {
        sys_timer_modify(__this->timer, 2000);//bt trim 延后检测
        return;
    }

    if (__this->step == 0) {
        __this->step = 1;
        sys_timer_modify(__this->timer, 20);//猜测是检测状态变化的时候改变定时器回调时间
        return ;
    }
    cur_stu = hdmi_sample_detect(arg);
    if (!__this->active) {
        __this->step = 0;
        sys_timer_modify(__this->timer, 500);//猜测是检测状态不变化的时候改变定时器回调时间
    }

    if (cur_stu != __this->stu) {
        __this->stu = cur_stu;
        __this->cnt = 0;
        __this->active = 1;
    } else {
        __this->cnt ++;
    }

    if (__this->cnt < HDMI_DETECT_CNT) {//滤波计算
        return;
    }

    /* putchar(cur_stu); */
    __this->active = 0;//检测一次完成

    if ((hdmi_is_online()) && (!__this->stu)) {
        hdmi_set_online(false);
        hdmi_event_notify(DEVICE_EVENT_OUT);//拔出不处理
    } else if ((!hdmi_is_online()) && (__this->stu)) {
        hdmi_set_online(true);
        hdmi_event_notify(DEVICE_EVENT_IN);//发布上线消息
    }
    /* printf("hdmi status[%d][%d]", __this->online, __this->stu); */
    return;
}

void hdmi_detect_timer_add()
{
    if (!__this) {
        return;
    }
    if (__this->timer == 0) {
        __this->timer = sys_timer_add(NULL, hdmi_detect, 50);
    }
}
void hdmi_detect_timer_del()
{
    if (__this && __this->timer) {
        hdmi_set_online(false);
        sys_timer_del(__this->timer);
        __this->timer = 0;
    }
    if (__this && __this->det_mode == HDMI_DET_BY_CEC) {
        hdmi_cec_close();
    }
}


static int hdmi_dete_init(const struct dev_node *node,  void *arg)
{
    memset(__this, 0, sizeof(*__this));
    audio_spdif_file_init();
    struct spdif_file_cfg *p_spdif_cfg;	//spdif的配置参数信息
    p_spdif_cfg = audio_spdif_file_get_cfg();
    if (p_spdif_cfg->hdmi_det_mode != HDMI_DET_UNUSED) {
        puts("\n HDMI_DET_BY_IO \n");
        __this->det_port = uuid2gpio(p_spdif_cfg->hdmi_det_port);
        __this->cec_port = uuid2gpio(p_spdif_cfg->cec_io_port);
        //for test
        //__this.det_port = IO_PORTC_06;
        if (__this->det_port != 0xff) {
            hdmi_set_online(false);
            __this->timer = sys_timer_add(NULL, hdmi_detect, 50);
            __this->det_mode = HDMI_DET_BY_IO;
        }
    } else {
        hdmi_set_online(false);
    }
    return 0;
}

const struct device_operations hdmi_dev_ops = {
    .init = hdmi_dete_init,
};

/*----------------------------------------------------------------------------*/
/*@brief   注册的定功耗检测函数
  @param
  @return    null
  @note   用于防止检测一次未完成进入了低功耗
 */
/*----------------------------------------------------------------------------*/
static u8 hdmi_dev_idle_query(void)
{
    return !__this->active;
}

REGISTER_LP_TARGET(hdmi_dev_lp_target) = {
    .name = "hdmi_dev",
    .is_idle = hdmi_dev_idle_query,
};

#endif
