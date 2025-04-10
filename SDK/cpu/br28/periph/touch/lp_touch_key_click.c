#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lp_touch_key_click.data.bss")
#pragma data_seg(".lp_touch_key_click.data")
#pragma const_seg(".lp_touch_key_click.text.const")
#pragma code_seg(".lp_touch_key_click.text")
#endif

static void lp_touch_key_short_click_time_out_handle(void *priv)
{
    u8 ch = *((u8 *)priv);
    u32 ch_idx = lpctmu_get_idx_by_cur_ch(ch);
    const struct touch_key_cfg *key_cfg = &(__this->pdata->key_cfg[ch_idx]);
    struct touch_key_arg *arg = &(__this->arg[ch_idx]);

    struct key_event e;
    switch (arg->click_cnt) {
    case 1:
        e.event = KEY_ACTION_CLICK;
        break;
    case 2:
        e.event = KEY_ACTION_DOUBLE_CLICK;
        break;
    case 3:
        e.event = KEY_ACTION_TRIPLE_CLICK;
        break;
    case 4:
        e.event = KEY_ACTION_FOURTH_CLICK;
        break;
    case 5:
        e.event = KEY_ACTION_FIRTH_CLICK;
        break;
    default:
        e.event = KEY_ACTION_NO_KEY;
        break;
    }
    e.value = key_cfg->key_value;

    log_debug("notify key:%d short event, cnt: %d", ch_idx, arg->click_cnt);
    lp_touch_key_notify_key_event(&e, ch);

    arg->short_timer = 0;
    arg->last_key = 0;
    arg->click_cnt = 0;
}

static void lp_touch_key_short_click_handle(u32 ch_idx)
{
    const struct touch_key_cfg *key_cfg = &(__this->pdata->key_cfg[ch_idx]);
    struct touch_key_arg *arg = &(__this->arg[ch_idx]);
    arg->last_key =  TOUCH_KEY_SHORT_CLICK;
    if (arg->short_timer == 0) {
        arg->click_cnt = 1;
        arg->short_timer = usr_timeout_add((void *)&key_cfg->key_ch, lp_touch_key_short_click_time_out_handle, __this->pdata->short_click_check_time, 1);
    } else {
        arg->click_cnt++;
        usr_timer_modify(arg->short_timer, __this->pdata->short_click_check_time);
    }
}

static void lp_touch_key_raise_click_handle(u32 ch_idx)
{
    const struct touch_key_cfg *key_cfg = &(__this->pdata->key_cfg[ch_idx]);
    struct touch_key_arg *arg = &(__this->arg[ch_idx]);

    struct key_event e = {0};
    if (arg->last_key >= TOUCH_KEY_LONG_CLICK) {
        e.event = KEY_ACTION_UP;
        e.value = key_cfg->key_value;
        lp_touch_key_notify_key_event(&e, key_cfg->key_ch);

        arg->last_key =  TOUCH_KEY_NULL;
        log_debug("notify key HOLD UP event");
    } else {
        lp_touch_key_short_click_handle(ch_idx);
    }
}

static void lp_touch_key_long_click_handle(u32 ch_idx)
{
    const struct touch_key_cfg *key_cfg = &(__this->pdata->key_cfg[ch_idx]);
    struct touch_key_arg *arg = &(__this->arg[ch_idx]);
    arg->last_key = TOUCH_KEY_LONG_CLICK;

    struct key_event e;
    e.event = KEY_ACTION_LONG;
    e.value = key_cfg->key_value;

    lp_touch_key_notify_key_event(&e, key_cfg->key_ch);
}

static void lp_touch_key_hold_click_handle(u32 ch_idx)
{
    const struct touch_key_cfg *key_cfg = &(__this->pdata->key_cfg[ch_idx]);
    struct touch_key_arg *arg = &(__this->arg[ch_idx]);
    arg->last_key =  TOUCH_KEY_HOLD_CLICK;

    struct key_event e;
    e.event = KEY_ACTION_HOLD;
    e.value = key_cfg->key_value;

    lp_touch_key_notify_key_event(&e, key_cfg->key_ch);
}

static void lp_touch_key_slide_up_handle(u32 ch_idx)
{
    const struct touch_key_cfg *key_cfg = &(__this->pdata->key_cfg[ch_idx]);
    struct key_event e;
    e.event = KEY_SLIDER_UP;
    e.value = __this->pdata->slide_mode_key_value;
    lp_touch_key_notify_key_event(&e, key_cfg->key_ch);
}

static void lp_touch_key_slide_down_handle(u32 ch_idx)
{
    const struct touch_key_cfg *key_cfg = &(__this->pdata->key_cfg[ch_idx]);
    struct key_event e;
    e.event = KEY_SLIDER_DOWN;
    e.value = __this->pdata->slide_mode_key_value;
    lp_touch_key_notify_key_event(&e, key_cfg->key_ch);
}


