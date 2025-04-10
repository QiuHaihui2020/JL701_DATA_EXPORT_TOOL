#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lp_touch_key_eartch.data.bss")
#pragma data_seg(".lp_touch_key_eartch.data")
#pragma const_seg(".lp_touch_key_eartch.text.const")
#pragma code_seg(".lp_touch_key_eartch.text")
#endif

#if TCFG_LP_EARTCH_KEY_ENABLE
#include "eartouch_manage.h"


extern u8 testbox_get_key_action_test_flag(void *priv);


void lp_touch_key_eartch_parm_init(void)
{
    u32 ch_idx = lpctmu_get_idx_by_cur_ch(__this->pdata->eartch_ch);
    const struct touch_key_cfg *key_cfg = &(__this->pdata->key_cfg[ch_idx]);
    const struct touch_key_algo_cfg *algo_cfg = &(key_cfg->algo_cfg[key_cfg->index]);

    M2P_CTMU_CH_CFG |= BIT(1);
    M2P_CTMU_EARTCH_CH = (__this->pdata->eartch_ref_ch << 4) | (__this->pdata->eartch_ch & 0xf);

    __this->eartch.trim_flag = 0;
    __this->eartch.inear_ok = 0;
    __this->eartch.trim_value = 10000;
    __this->eartch.last_state = CTMU_P2M_EARTCH_OUT_EVENT;
    __this->eartch.soft_inear_val = algo_cfg->algo_cfg2;
    __this->eartch.soft_outear_val = algo_cfg->algo_cfg2;

    u16 trim_value;
    int ret = syscfg_read(LP_KEY_EARTCH_TRIM_VALUE, &trim_value, sizeof(trim_value));
    if (ret > 0) {
        __this->eartch.inear_ok = 1;
        __this->eartch.trim_value = trim_value;
        log_debug("eartch_trim_value = %d", __this->eartch.trim_value);
    }
    M2P_CTMU_EARTCH_TRIM_VALUE_L = (__this->eartch.trim_value & 0xFF);
    M2P_CTMU_EARTCH_TRIM_VALUE_H = (__this->eartch.trim_value >> 8) & 0xFF;

    M2P_CTMU_INEAR_VALUE_L  = __this->eartch.soft_inear_val & 0xFF;
    M2P_CTMU_INEAR_VALUE_H  = __this->eartch.soft_inear_val >> 8;
    M2P_CTMU_OUTEAR_VALUE_L = __this->eartch.soft_outear_val & 0xFF;
    M2P_CTMU_OUTEAR_VALUE_H = __this->eartch.soft_outear_val >> 8;
}

static void lp_touch_key_eartch_notify_event(u8 state)
{
    eartouch_event_handler(state);
}

static void lp_touch_key_eartch_unlock_timeout_handle(void *priv)
{
    __this->key_ch_msg_lock = false;
    __this->key_ch_msg_lock_timer = 0;
}

static void lp_touch_key_eartch_event_handle(u8 eartch_state)
{
    if (!__this->eartch.inear_ok) {
        return;
    }

    u8 state;
    /* struct key_event e; */

    if (eartch_state == CTMU_P2M_EARTCH_IN_EVENT) {
        state = EARTOUCH_STATE_IN_EAR;
        /* e.event = 10; */
        log_debug("soft inear\n");
    } else if (eartch_state == CTMU_P2M_EARTCH_OUT_EVENT) {
        state = EARTOUCH_STATE_OUT_EAR;
        /* e.event = 11; */
        log_debug("soft outear");
    } else {
        return;
    }

#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
    if ((touch_bt_tool_enable) && (touch_bt_online_debug_key_event_handle)) {
        if (touch_bt_online_debug_key_event_handle(__this->pdata->eartch_ch, (void *)&e)) {
            return;
        }
    }
#endif

    __this->key_ch_msg_lock = true;

    if (__this->key_ch_msg_lock_timer == 0) {
        __this->key_ch_msg_lock_timer = sys_hi_timeout_add(NULL, lp_touch_key_eartch_unlock_timeout_handle, 3000);
    } else {
        sys_hi_timer_modify(__this->key_ch_msg_lock_timer, 3000);
    }

    lp_touch_key_eartch_notify_event(state);
}

static void lp_touch_key_eartch_diff_trim(void)
{
    if (__this->eartch.trim_flag == 0) {
        return;
    }

    u8 state;
    /* u16 eartch_iir  = (P2M_CTMU_EARTCH_H_IIR_VALUE  << 8) | P2M_CTMU_EARTCH_L_IIR_VALUE; */
    u16 eartch_diff = (P2M_CTMU_EARTCH_H_DIFF_VALUE << 8) | P2M_CTMU_EARTCH_L_DIFF_VALUE;
    if (__this->eartch.trim_value == 0) {
        __this->eartch.trim_value = eartch_diff;
    } else {
        __this->eartch.trim_value = (eartch_diff + __this->eartch.trim_value) / 2;
    }
    log_debug("eartch ch trim value: %d, res = %d", __this->eartch.trim_value, eartch_diff);

    __this->eartch.trim_flag ++;

    if (__this->eartch.trim_flag > 20) {
        __this->eartch.trim_flag = 0;
        is_lpkey_active = 0;

        M2P_CTMU_CH_DEBUG &= ~BIT(__this->pdata->eartch_ch);
        int ret = syscfg_write(LP_KEY_EARTCH_TRIM_VALUE, &(__this->eartch.trim_value), sizeof(__this->eartch.trim_value));
        if (ret > 0) {
            M2P_CTMU_EARTCH_TRIM_VALUE_L = (__this->eartch.trim_value >> 0) & 0xff;
            M2P_CTMU_EARTCH_TRIM_VALUE_H = (__this->eartch.trim_value >> 8) & 0xff;
            state = EARTOUCH_STATE_TRIM_OK;
            __this->eartch.inear_ok = 1;
            log_debug("trim: %d\n", __this->eartch.inear_ok);
        } else {
            state = EARTOUCH_STATE_TRIM_ERR;
            __this->eartch.inear_ok = 0;
            log_debug("trim: %d\n", __this->eartch.inear_ok);
        }

        lp_touch_key_eartch_notify_event(state);
    }
}

static void lp_touch_key_eartch_event_deal(u32 ch)
{
    if (__this->pdata->eartch_en == 0) {
        return;
    }
    if (__this->pdata->eartch_ch != ch) {
        return;
    }

#if !TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
    lp_touch_key_eartch_diff_trim();
#endif

    if (P2M_CTMU_EARTCH_EVENT == __this->eartch.last_state) {
        return;
    }

    __this->eartch.last_state = P2M_CTMU_EARTCH_EVENT;
    lp_touch_key_eartch_event_handle(__this->eartch.last_state);
}

void lp_touch_key_testbox_inear_trim(u32 flag)
{
#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
    return;
#endif

    if (flag == 1) {
        __this->eartch.trim_flag = 1;
        __this->eartch.inear_ok = 0;
        __this->eartch.trim_value = 0;
        is_lpkey_active = 1;
        M2P_CTMU_CH_DEBUG |= BIT(__this->pdata->eartch_ch);
        log_info("__this->eartch.trim_flag = %d", __this->eartch.trim_flag);
    } else {
        __this->eartch.trim_flag = 0;
        M2P_CTMU_CH_DEBUG &= ~BIT(__this->pdata->eartch_ch);
        log_info("__this->eartch.trim_flag = %d", __this->eartch.trim_flag);
    }
}

static u32 lp_touch_key_testbox_remote_test(u32 ch, u32 event)
{
    if (__this->pdata->eartch_en == 0) {
        return 0;
    }
    if (__this->pdata->eartch_ch != ch) {
        return 0;
    }
    if (0 == testbox_get_key_action_test_flag(NULL)) {
        return 0;
    }
    u8 key_report = 0;
    if (event == (CTMU_P2M_CH0_FALLING_EVENT + ch * 8)) {
        key_report = 0xF1;
    } else if (event == (CTMU_P2M_CH0_RAISING_EVENT + ch * 8)) {
        key_report = 0xF2;
    } else {
        return 0;
    }

    bt_cmd_prepare(USER_CTRL_TEST_KEY, 1, &key_report); //音量加

    return 1;
}

#endif


