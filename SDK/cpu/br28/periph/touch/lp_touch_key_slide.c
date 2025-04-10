#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lp_touch_key_slide.data.bss")
#pragma data_seg(".lp_touch_key_slide.data")
#pragma const_seg(".lp_touch_key_slide.text.const")
#pragma code_seg(".lp_touch_key_slide.text")
#endif

/******************************双通道滑动触摸识别的基础原理***********************************

1.单击，但如果超过设定的长按时间，那就为长按
chx  __________________________________________
chx  ___________                     __________
                |_______>180________|


2.单击，但如果超过设定的长按时间，那就为长按
chx  ___________                     __________
                |_______>180________|
chx  _______ <30                     <30 ______
            |___________________________|


3.单击，但如果超过设定的长按时间，那就为长按
chx  ___________                     __________
                |_______>180________|
chx  _______ <30                       >30   __
            |_______________________________|


4.单击，但如果超过设定的长按时间，那就为长按
chx  ___________                     __________
                |_______>180________|
chx  ___   >30                       <30 ______
        |_______________________________|


5.单击，但如果超过设定的长按时间，那就为长按
chx  ___________                     __________
                |_______>180________|
chx  ___   >30                         >30   __
        |___________________________________|


6.单击，但如果超过设定的长按时间，那就为长按
chx  ___________                         ______
                |_______________________|
chx  _______ <30        >180         _<30______
            |_______________________|


7.根据前后操作，有可能是滑动，也有可能是连击中的后单击，也有可能是无效操作
chx  ___________                 __________
                |_______________|
chx  _______ <30    <180     _<30__________
            |_______________|


8.滑动，t < 设定的长按的时间
chx  ___________                 __________
                |_______________|
chx  ___   >30       t       _<30__________
        |___________________|


9.滑动，t < 设定的长按的时间
chx  ___________                     ______
                |___________________|
chx  ___   >30       t       __>30_________
        |___________________|


10.滑动，t < 设定的长按的时间
chx  ___________                     ______
                |___________________|
chx  _______ <30     t       __>30_________
            |_______________|

*****************************************************************************/

enum falling_order_type {
    FALLING_NULL,
    FALLING_FIRST,
    FALLING_SECOND,
};
enum raising_order_type {
    RAISING_NULL,
    RAISING_FIRST,
    RAISING_SECOND,
};

enum time_interval_type {
    LONG_TIME,
    SHORT_TIME,
};

enum slide_key_type {
    SHORT_CLICK = 1,
    DOUBLE_CLICK,
    TRIPLE_CLICK,
    FOURTH_CLICK,
    FIRTH_CLICK,
    LONG_CLICK = 8,
    LONG_HOLD_CLICK = 9,
    LONG_UP_CLICK = 10,
    SLIDE_UP = 0x12,
    SLIDE_DOWN = 0x21,
};

struct touch_key_slide {
    u8 falling_order[2];
    u8 raising_order[2];
    u8 falling_time_interval;
    u8 raising_time_interval;
    u8 last_key_type;
    u8 send_keytone_flag;
    u8 fast_slide_cnt;
    u8 fast_slide_type;
    u16 slide_click_timeout_add;
    u16 falling_timeout_add;
    u16 raising_timeout_add;
    u16 click_idle_timeout_add;
    u16 first_short_click_timeout_add;
    u16 send_keytone_timeout_add;
};
static struct touch_key_slide slide_key;


#define FALLING_TIMEOUT_TIME    30              //两个按下边沿的时间间隔的阈值
#define RAISING_TIMEOUT_TIME    30              //两个抬起边沿的时间间隔的阈值
#define CLICK_IDLE_TIMEOUT_TIME 500             //如果该时间内，没有单击，那么该时间之后的第一次单击，识别为首次单击
#define FIRST_SHORT_CLICK_TIMEOUT_TIME  190     //只针对首次单击，要满足按够那么长时间。连击时，后面的单击没有该时间要求
#define SEND_KEYTONE_TIMEOUT_TIME   250         //长按时的按键音，在按下后的多长时间要响
#define SLIDE_CLICK_TIMEOUT_TIME    500         //识别为滑动之后，该时间内，如果有case7.也要识别为滑动
#define FAST_SLIDE_CNT_MAX          4           //一来就连续那么多次的case7.，就会识别为一次滑动。


static void lp_touch_key_falling_timeout_handle(void *priv)
{
    slide_key.falling_timeout_add = 0;
}

static void lp_touch_key_raising_timeout_handle(void *priv)
{
    slide_key.raising_timeout_add = 0;
}

static void lp_touch_key_click_idle_timeout_handle(void *priv)
{
    slide_key.click_idle_timeout_add = 0;
}

static void lp_touch_key_first_short_click_timeout_handle(void *priv)
{
    slide_key.first_short_click_timeout_add = 0;
}

static void lp_touch_key_send_keytone_timeout_handle(void *priv)
{
    lp_touch_key_send_key_tone_msg();
    slide_key.send_keytone_timeout_add = 0;
    slide_key.send_keytone_flag = 1;
}

void lp_touch_key_slide_click_timeout_handle(void *priv)
{
    slide_key.slide_click_timeout_add = 0;
}

static void lp_touch_key_check_channel_falling_info(u32 ch)
{
    slide_key.falling_order[ch] = FALLING_FIRST;
    if (slide_key.falling_order[!ch] == FALLING_FIRST) {
        slide_key.falling_order[ch] = FALLING_SECOND;
    }
    slide_key.send_keytone_flag = 0;
    if (slide_key.send_keytone_timeout_add == 0) {
        slide_key.send_keytone_timeout_add = sys_hi_timeout_add(NULL, lp_touch_key_send_keytone_timeout_handle, SEND_KEYTONE_TIMEOUT_TIME);
    } else {
        sys_hi_timer_modify(slide_key.send_keytone_timeout_add, SEND_KEYTONE_TIMEOUT_TIME);
    }
    if ((slide_key.last_key_type != SHORT_CLICK) || (slide_key.click_idle_timeout_add == 0)) {
        if (slide_key.first_short_click_timeout_add == 0) {
            slide_key.first_short_click_timeout_add = sys_hi_timeout_add(NULL, lp_touch_key_first_short_click_timeout_handle, FIRST_SHORT_CLICK_TIMEOUT_TIME);
        } else {
            sys_hi_timer_modify(slide_key.first_short_click_timeout_add, FIRST_SHORT_CLICK_TIMEOUT_TIME);
        }
    }
    if (slide_key.falling_order[ch] == FALLING_FIRST) {
        if (slide_key.falling_timeout_add == 0) {
            slide_key.falling_timeout_add = sys_hi_timeout_add(NULL, lp_touch_key_falling_timeout_handle, FALLING_TIMEOUT_TIME);
        } else {
            sys_hi_timer_modify(slide_key.falling_timeout_add, FALLING_TIMEOUT_TIME);
        }
        slide_key.falling_time_interval = SHORT_TIME;
    } else if (slide_key.falling_order[ch] == FALLING_SECOND) {
        if (slide_key.falling_timeout_add) {
            sys_hi_timeout_del(slide_key.falling_timeout_add);
            slide_key.falling_timeout_add = 0;
            slide_key.falling_time_interval = SHORT_TIME;
        } else {
            slide_key.falling_time_interval = LONG_TIME;
        }
    }
}

static u32 lp_touch_key_check_channel_raising_info_and_key_type(u32 ch)
{
    u32 key_type = 0;
    slide_key.fast_slide_type = 0;
    if (slide_key.falling_order[ch] == FALLING_NULL) {
        return key_type;
    }
    slide_key.raising_order[ch] = RAISING_FIRST;
    if (slide_key.raising_order[!ch] == RAISING_FIRST) {
        slide_key.raising_order[ch] = RAISING_SECOND;
    }
    if (slide_key.falling_order[ch] > slide_key.falling_order[!ch]) {
        if (slide_key.raising_order[ch] == RAISING_FIRST) {
            key_type = SHORT_CLICK;             //case 1~5.
        } else {
            if (slide_key.falling_time_interval == SHORT_TIME) {
                if (slide_key.raising_timeout_add) {
                    sys_hi_timeout_del(slide_key.raising_timeout_add);
                    slide_key.raising_timeout_add = 0;
                    key_type = SHORT_CLICK;     //case 6~7.
                    if (ch == 0) {
                        slide_key.fast_slide_type = SLIDE_DOWN;
                    } else {
                        slide_key.fast_slide_type = SLIDE_UP;
                    }
                } else if (ch == 0) {
                    key_type = SLIDE_DOWN;      //case 10.
                } else {
                    key_type = SLIDE_UP;        //case 10.
                }
            } else if (ch == 0) {
                key_type = SLIDE_DOWN;          //caee 8~9.
            } else {
                key_type = SLIDE_UP;            //case 8~9.
            }
        }
    } else {
        if (slide_key.raising_order[ch] == RAISING_FIRST) {
            if (slide_key.falling_time_interval == SHORT_TIME) {
                if (slide_key.raising_timeout_add == 0) {
                    slide_key.raising_timeout_add = sys_hi_timeout_add(NULL, lp_touch_key_raising_timeout_handle, RAISING_TIMEOUT_TIME);
                } else {
                    sys_hi_timer_modify(slide_key.raising_timeout_add, RAISING_TIMEOUT_TIME);
                }
            } else if (ch == 0) {
                key_type = SLIDE_UP;            //case 8~9.
            } else {
                key_type = SLIDE_DOWN;          //case 8~9.
            }
        } else {
            if (slide_key.falling_time_interval == SHORT_TIME) {
                if (slide_key.raising_timeout_add) {
                    sys_hi_timeout_del(slide_key.raising_timeout_add);
                    slide_key.raising_timeout_add = 0;
                    key_type = SHORT_CLICK;     //case 6~7
                    if (ch == 0) {
                        slide_key.fast_slide_type = SLIDE_UP;
                    } else {
                        slide_key.fast_slide_type = SLIDE_DOWN;
                    }
                } else if (ch == 0) {
                    key_type = SLIDE_UP;        //case 10.
                } else {
                    key_type = SLIDE_DOWN;      //case 10.
                }
            } else if (ch == 0) {
                key_type = SLIDE_UP;            //case 8~9.
            } else {
                key_type = SLIDE_DOWN;          //case 8~9.
            }
        }
    }
    return key_type;
}

static u32 lp_touch_key_get_slide_event_ch(void)
{
    return __this->pdata->key_cfg[0].key_ch;
}

static u32 lp_touch_key_check_slide_key_type(u32 event, u32 ch)
{
    u32 key_type = 0;
    u32 event_ch = lp_touch_key_get_slide_event_ch();
    if (event == (CTMU_P2M_CH0_FALLING_EVENT + ch * 8)) {
        if (ch == event_ch) {
            lp_touch_key_check_channel_falling_info(0);
        } else {
            lp_touch_key_check_channel_falling_info(1);
        }
    } else if (event == (CTMU_P2M_CH0_RAISING_EVENT + ch * 8)) {
        if (ch == event_ch) {
            if ((slide_key.last_key_type == LONG_CLICK) || (slide_key.last_key_type == LONG_HOLD_CLICK)) {
                key_type = LONG_UP_CLICK;       //一定是长按抬起
            } else {
                key_type = lp_touch_key_check_channel_raising_info_and_key_type(0);
            }
        } else {
            key_type = lp_touch_key_check_channel_raising_info_and_key_type(1);
        }
    } else if (event == (CTMU_P2M_CH0_LONG_KEY_EVENT + event_ch * 8)) {//长按只判断通道号小的那个按键
        key_type = LONG_CLICK;
    } else if (event == (CTMU_P2M_CH0_HOLD_KEY_EVENT + event_ch * 8)) {//长按只判断通道号小的那个按键
        key_type = LONG_HOLD_CLICK;
    }

    if (key_type) {
        slide_key.falling_order[0] = FALLING_NULL;
        slide_key.falling_order[1] = FALLING_NULL;
        slide_key.raising_order[0] = RAISING_NULL;
        slide_key.raising_order[1] = RAISING_NULL;
        slide_key.falling_time_interval = 0;
        slide_key.last_key_type = key_type;
        if (slide_key.send_keytone_timeout_add) {
            sys_hi_timeout_del(slide_key.send_keytone_timeout_add);
            slide_key.send_keytone_timeout_add = 0;
        }
        if (key_type == SHORT_CLICK) {                  //case 6~7 的进一步处理
            if (slide_key.first_short_click_timeout_add) {
                sys_hi_timeout_del(slide_key.first_short_click_timeout_add);
                slide_key.first_short_click_timeout_add = 0;

                if (slide_key.slide_click_timeout_add) {
                    sys_hi_timeout_del(slide_key.slide_click_timeout_add);
                    slide_key.slide_click_timeout_add = 0;
                    if (slide_key.fast_slide_type) {
                        key_type = slide_key.fast_slide_type;
                        slide_key.last_key_type = key_type;
                    } else {
                        key_type = 0xff;
                        slide_key.last_key_type = key_type;
                    }
                    slide_key.fast_slide_cnt = 0;
                } else {
                    if (slide_key.fast_slide_type) {
                        slide_key.fast_slide_cnt ++;
                        if (slide_key.fast_slide_cnt >= FAST_SLIDE_CNT_MAX) {
                            slide_key.fast_slide_cnt = 0;
                            key_type = slide_key.fast_slide_type;
                            slide_key.last_key_type = key_type;
                        } else {
                            key_type = 0xff;
                            slide_key.last_key_type = key_type;
                        }
                    } else {
                        slide_key.fast_slide_cnt = 0;
                        key_type = 0xff;
                        slide_key.last_key_type = key_type;
                    }
                }
            } else {
                if (slide_key.send_keytone_flag == 0) {
                    lp_touch_key_send_key_tone_msg();
                }
                if (slide_key.slide_click_timeout_add) {
                    sys_hi_timeout_del(slide_key.slide_click_timeout_add);
                    slide_key.slide_click_timeout_add = 0;
                }
                slide_key.fast_slide_cnt = 0;
            }
            if (slide_key.click_idle_timeout_add == 0) {
                slide_key.click_idle_timeout_add = sys_hi_timeout_add(NULL, lp_touch_key_click_idle_timeout_handle, CLICK_IDLE_TIMEOUT_TIME);
            } else {
                sys_hi_timer_modify(slide_key.click_idle_timeout_add, CLICK_IDLE_TIMEOUT_TIME);
            }
        } else {
            slide_key.fast_slide_cnt = 0;
        }

        if ((key_type == SLIDE_UP) || (key_type == SLIDE_DOWN)) {
            if (slide_key.slide_click_timeout_add == 0) {
                slide_key.slide_click_timeout_add = sys_hi_timeout_add(NULL, lp_touch_key_slide_click_timeout_handle, SLIDE_CLICK_TIMEOUT_TIME);
            } else {
                sys_hi_timer_modify(slide_key.slide_click_timeout_add, SLIDE_CLICK_TIMEOUT_TIME);
            }
        }
    }

    return key_type;
}

static void lp_touch_key_send_slide_key_type_event(u32 key_type)
{
    switch (key_type) {
    case SHORT_CLICK:           //单击
        lp_touch_key_short_click_handle(0);
        break;
    case LONG_CLICK:            //长按
#if CTMU_CHECK_LONG_CLICK_BY_RES
        if (lp_touch_key_check_long_click_by_ctmu_res(0)) {
            slide_key.last_key_type = 0;
            return;
        }
#endif
        lp_touch_key_long_click_handle(0);
        break;
    case LONG_HOLD_CLICK:       //长按保持
#if CTMU_CHECK_LONG_CLICK_BY_RES
        if (lp_touch_key_check_long_click_by_ctmu_res(0)) {
            slide_key.last_key_type = 0;
            return;
        }
#endif
        lp_touch_key_hold_click_handle(0);
        break;
    case LONG_UP_CLICK:         //长按抬起
        lp_touch_key_raise_click_handle(0);
        break;
    case SLIDE_UP:              //向上滑动
        lp_touch_key_slide_up_handle(0);
        break;
    case SLIDE_DOWN:            //向下滑动
        lp_touch_key_slide_down_handle(0);
        break;
    default:
        break;
    }
}

