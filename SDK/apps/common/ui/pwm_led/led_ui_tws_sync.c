#include "generic/typedef.h"
#include "classic/tws_api.h"
#include "led_ui_api.h"
#include "app_config.h"
#include "system/timer.h"
#include "system/task.h"
#include "led_ui_tws_sync.h"

#define LOG_TAG             "[led_tws]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_PWMLED_ENABLE
#if TCFG_USER_TWS_ENABLE

//这个不能够在init的时候malloc优化，因为并不知道什么时候才切灯效

/* #define LED_SYNC_SWITCH_ENABLE */

/* #define LED_SYNC_SEND_DYNAMIC_TIME_ENABLE */


//ms
#define PLED_FISRT_SYNC_TIME        11000

//以下三个均是根据环境采样出来的经验偏差估计值，允许存在较大的误差
//经测试，人眼能够看出错开的最短时间,是亮灯时间100ms的一半
#define PLED_PERIOD_LIGHT_TIME 	50

//偏差特别小的时候，最长的一次通信同步时间
#define PLED_PERIOD_SYNC_MAX_TIME 	21000
//偏差特别大的时候，最短的通信同步时间
#define PLED_PERIOD_SYNC_MIN_TIME 	1000

/* 下面的变量可以可以init之后释放做内存空间优化 */
//只有一个变量没必要专门规划malloc内存来节省空间和降低代码灵活性，因为一直内存指针就4字节了
struct led_tws_sync_handler {
    u8 recv_enable;
    //一般就3次重试失败就重新初始化灯效同步
    u8 retry_cnt;
    u8 led_name;
    u16 sync_timer;
#ifdef LED_SYNC_SEND_DYNAMIC_TIME_ENABLE
    u32 prev_sync_time;
#endif
};

static struct led_tws_sync_handler led_tws_sync_ctl;
static void pwm_led_state_sync_read_send(void *priv);



//*********************************************************************************
/* 通信发起多次同步亮灯操作 */
enum {
    PWMLED_TWS_SYNC_CMD_SET  = 0,
    PWMLED_TWS_SYNC_CMD_READ,
    PWMLED_TWS_SYNC_CMD_STOP,
    PWMLED_TWS_SYNC_CMD_RESET,
};


struct pwm_led_send_data_t {
    u8 cmd;
    u8 led_name; //对应灯效的下标index
    /* u16 lrc; */
    u32 mclkn;
    struct pwm_led_status_t status;
};



#define TWS_FUNC_ID_PLED      TWS_FUNC_ID('P', 'L', 'E', 'D')


//*********************************************************************************

//释放资源
void pwm_led_state_sync_del(void)
{
    if (led_tws_sync_ctl.sync_timer) {
        sys_timer_del(led_tws_sync_ctl.sync_timer);
        led_tws_sync_ctl.sync_timer = 0;
        log_info("led_tws_sync_timer_del\n");
    }
}

//释放+发停止包
void pwm_led_state_sync_exit(u8 led_name)
{
    struct pwm_led_send_data_t sync_data;

    led_tws_sync_ctl.recv_enable = 0;
    sync_data.cmd = PWMLED_TWS_SYNC_CMD_STOP;
    sync_data.led_name = led_name;
    tws_api_send_data_to_sibling(&sync_data, sizeof(sync_data), TWS_FUNC_ID_PLED);
}


static void pwm_led_tws_sync_reset(u8 led_name)
{
    if (led_tws_sync_ctl.led_name == led_name) {
        led_tws_state_sync_start(led_name, 1, 300);
    }
}

static void pwmled_tws_sync_cmd_set_handle(struct pwm_led_send_data_t *pled_sync_remote_data)
{
    /* if 灯效不相同，就不支持同步,led_state_sync_handler同步灯效亮起函数保证了是相同的灯效*/

    //获取本地的蓝牙时间点(tws里面的cnt)标记
    u32 local_mclkn = tws_conn_get_mclkn(NULL);
    //tws里面的cnt 的差值转换为slot，slot *625 = 时间差(us)
    u32 how_long_ago =  bredr_clkn2offset(pled_sync_remote_data->mclkn, local_mclkn) * 625;

    //两个样机的时间差,us
    u32 sync_time = 0;
    /* 时间差和灯效同步信息写入pwm_led模块驱动 */
    s32 ret = pwm_led_set_sync(&(pled_sync_remote_data->status), how_long_ago, &sync_time);

    PWMLED_LOG_SUCC("local_mclkn：%d,>>>>>> recv remote_mclkn：%d >>>>>>", local_mclkn, pled_sync_remote_data->mclkn);
    PWMLED_LOG_INFO("ret = %d, how_long_ago：%dus , %ds,sync_time = %d", ret, how_long_ago, how_long_ago / 1000000, sync_time);

    //*********************************************************************************

    //设置失败，需要再发一包过来
    //失败就再定时用灯效周期的中间值再试一下
    if (ret == 1) {
        PWMLED_LOG_INFO("pwm_led_set_sync fail,request repeat cnt = %d", led_tws_sync_ctl.retry_cnt);

        led_tws_sync_ctl.retry_cnt++;
        if (led_tws_sync_ctl.retry_cnt == 3) {
            //3次都不成功，就重新初始化灯效
            PWMLED_LOG_DEBUG("pled_sync_remote_data->led_name: %d", pled_sync_remote_data->led_name);

            //添加控制变量在重新初始化灯效同步的后续一段时间内，忽略不处理,后续的上一个重试流程的对方包因为发包延迟等原因导致的包
            if (tws_api_get_role_async() == TWS_ROLE_MASTER) {
                pwm_led_tws_sync_reset(pled_sync_remote_data->led_name);
            } else {
                pled_sync_remote_data->cmd = PWMLED_TWS_SYNC_CMD_RESET;
                tws_api_send_data_to_sibling(pled_sync_remote_data,
                                             sizeof(struct pwm_led_send_data_t), TWS_FUNC_ID_PLED);
            }
            return;
        }

        //*********************************************************************************
        //复用变量
        pled_sync_remote_data->cmd = PWMLED_TWS_SYNC_CMD_READ;
        tws_api_send_data_to_sibling(pled_sync_remote_data,
                                     sizeof(struct pwm_led_send_data_t), TWS_FUNC_ID_PLED);

    } else {
#ifdef LED_SYNC_SEND_DYNAMIC_TIME_ENABLE
        /* if (get_rom_cfg_led_sync_send_dynamic_time_enable() == 1) { */

        //该处所做的同步操作是基于lrc时钟频率（不准）但是线性丢失精度的前提做的同步，如果因为环境温飘突变过大，则需要等待下一次同步周期才可以尝试运算时钟偏差补偿。

        //运算结果显示是属于快的时钟,在最小同步周期和最大同步周期之间比例取同步时间
        sync_time = sync_time / 1000;
        if (sync_time  > PLED_PERIOD_LIGHT_TIME) {
            sync_time = PLED_PERIOD_LIGHT_TIME;
        }

        u32 next_sync_time = PLED_PERIOD_LIGHT_TIME - sync_time;

        next_sync_time = next_sync_time * PLED_PERIOD_SYNC_MAX_TIME / PLED_PERIOD_LIGHT_TIME; //ms

        if (next_sync_time < PLED_PERIOD_SYNC_MIN_TIME) {
            next_sync_time = PLED_PERIOD_SYNC_MIN_TIME;
        }

        log_debug("cal next_sync_time = %d, %d", led_tws_sync_ctl.prev_sync_time, next_sync_time);
        if (led_tws_sync_ctl.prev_sync_time != next_sync_time) {
            led_tws_sync_ctl.prev_sync_time = next_sync_time;
            sys_timer_modify(led_tws_sync_ctl.sync_timer, next_sync_time);
        }
#endif
    }

}




static void pwm_led_tws_sibling_data_deal_in_task(void *_data)
{
    struct pwm_led_send_data_t *pled_sync_remote_data = (struct pwm_led_send_data_t *)_data;

    PWMLED_LOG_DEBUG("pled_sync_remote_data->cmd = %d", pled_sync_remote_data->cmd);

    if (pled_sync_remote_data->cmd == PWMLED_TWS_SYNC_CMD_STOP) {
        if (pled_sync_remote_data->led_name == led_tws_sync_ctl.led_name) {
            PWMLED_LOG_INFO("PWMLED_TWS_SYNC_CMD_STOP");
            led_tws_sync_ctl.recv_enable = 0;
            led_tws_sync_ctl.led_name = 0xff;
            pwm_led_state_sync_del();
        }
        goto __exit;
    }

    if (led_tws_sync_ctl.recv_enable == 0) {
        PWMLED_LOG_ERR("led_tws_sync_ctl.recv_enable == 0");
        goto __exit;
    }

    if (pled_sync_remote_data->led_name != led_tws_sync_ctl.led_name) {
        PWMLED_LOG_ERR("pled_sync_remote_data->led_name != led_ui_get_state_name()");
        pwm_led_state_sync_exit(pled_sync_remote_data->led_name);
        goto __exit;
    }

    switch (pled_sync_remote_data->cmd) {
    case PWMLED_TWS_SYNC_CMD_SET:
        PWMLED_LOG_INFO("PWMLED_TWS_SYNC_CMD_SET");
        pwmled_tws_sync_cmd_set_handle(pled_sync_remote_data);
        break;

    case PWMLED_TWS_SYNC_CMD_READ:
        PWMLED_LOG_INFO("PWMLED_TWS_SYNC_CMD_READ");
        pwm_led_state_sync_read_send(NULL);
        break;
    case PWMLED_TWS_SYNC_CMD_RESET:
        pwm_led_tws_sync_reset(pled_sync_remote_data->led_name);
        break;
    default:
        PWMLED_LOG_INFO("not surpport cmd");
        break;
    }

__exit:
    free(_data);
}

static void pwm_led_tws_sibling_data_deal(void *_data, u16 len, bool rx)
{
    struct pwm_led_send_data_t *pled_sync_remote_data = (struct pwm_led_send_data_t *)_data;

    if (rx) {
        u8 *data = malloc(len);
        if (!data) {
            return;
        }
        memcpy(data, _data, len);
        int msg[3];
        msg[0] = (int)pwm_led_tws_sibling_data_deal_in_task;
        msg[1] = 1;
        msg[2] = (int)data;
        int err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        if (err != OS_NO_ERR) {
            free(data);
        }
    }
}

REGISTER_TWS_FUNC_STUB(pled_sync_stub) = {
    .func_id = TWS_FUNC_ID_PLED,
    .func    = pwm_led_tws_sibling_data_deal,
};



//=========================================================//
//读取本地计数-->发给对方
//=========================================================//
static void pwm_led_state_sync_read_send(void *priv)
{
    struct pwm_led_send_data_t local_data;

    if (tws_api_get_tws_state() & TWS_STA_SIBLING_DISCONNECTED) {
        //如果tws断开了，就删除timer
        led_tws_sync_ctl.recv_enable = 0;
        led_tws_sync_ctl.led_name = 0xff;
        pwm_led_state_sync_del();
        return;
    }

    local_data.led_name = led_tws_sync_ctl.led_name;
    local_data.cmd = PWMLED_TWS_SYNC_CMD_SET;

    pwm_led_get_sync_status(&(local_data.status));

    local_data.mclkn = tws_conn_get_mclkn(NULL);

    //开启处理包，现在可以处理接收到的新一轮流程的包.必须放这里，等一下timer的一段时间
    led_tws_sync_ctl.recv_enable = 1;

    tws_api_send_data_to_sibling(&local_data, sizeof(struct pwm_led_send_data_t), TWS_FUNC_ID_PLED);
}


//*********************************************************************************

/*
 * LED状态同步
 */
static void led_state_sync_handler(int data, int result)
{
    u8 name = (data >> 8) & 0xFF;
    u8 time = (data >> 0) & 0xFF;

    log_debug("led_name=%d, time=%x, result = %d\n", name, time, result);

    if (result == TWS_SYNC_CALL_RX) {
        led_ui_set_state(name, DISP_CLEAR_OTHERS | DISP_TWS_SYNC_RX);
        led_tws_sync_ctl.led_name = name;
    } else {
        if (time == 1 && led_tws_sync_ctl.led_name != name) {
            pwm_led_state_sync_exit(name);
            return;
        }
        led_ui_state_machine_run();
    }
    if (time == 0 || result == -TWS_SYNC_CALL_TX) {
        led_tws_sync_ctl.led_name = 0xff;
        pwm_led_state_sync_del();
        return;
    }

    //清除重试状态
    led_tws_sync_ctl.retry_cnt = 0;

#ifdef LED_SYNC_SEND_DYNAMIC_TIME_ENABLE
    /* 初始化变量 */
    led_tws_sync_ctl.prev_sync_time = -1;
#endif

    //11s通信同步一次灯效，允许systimer产生20s误差
    if (led_tws_sync_ctl.sync_timer) {
        sys_timer_modify(led_tws_sync_ctl.sync_timer, PLED_FISRT_SYNC_TIME);
    } else {
        led_tws_sync_ctl.sync_timer = sys_timer_add(NULL, pwm_led_state_sync_read_send,
                                      PLED_FISRT_SYNC_TIME);
    }
}


TWS_SYNC_CALL_REGISTER(tws_led_sync) = {
    .uuid = 0x2A1E3095,
    .task_name = "app_core",
    .func = led_state_sync_handler,
};

void led_tws_state_sync_start(u8 led_name, u8 time, int msec)
{
    if (tws_api_get_role_async() == TWS_ROLE_MASTER) {
        log_info("led_tws_sync_start: %d\n", led_name);
        led_tws_sync_ctl.led_name = led_name;
        int data = (led_name << 8) | time;
        tws_api_sync_call_by_uuid(0x2A1E3095, data, msec);
    }
}

void led_tws_state_sync_stop()
{
    if (led_tws_sync_ctl.led_name != 0xff) {
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            pwm_led_state_sync_exit(led_tws_sync_ctl.led_name);
        }
        led_tws_sync_ctl.led_name = 0xff;
    }
    pwm_led_state_sync_del();
}


#endif
#endif

