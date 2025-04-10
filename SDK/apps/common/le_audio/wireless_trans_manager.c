#include "wireless_trans_manager.h"
#include "list.h"

#define WIRELESS_DEV_ENTER_CRITICAL()  //local_irq_disable()
#define WIRELESS_DEV_EXIT_CRITICAL()   //local_irq_enable()

/* --------------------------------------------------------------------------*/
/**
 * @brief 无线数据传输接口
 *
 * @param name:dev name for find dev
 * @param buf: data buffer
 * @param len: data length
 * @param priv:dev operating parameters
 *
 * @return 0:succ, other:err
 */
/* ----------------------------------------------------------------------------*/
int wireless_trans_transmit(const char *name, const void *const buf, size_t len, void *priv)
{
    int ret = -1;
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->ioctrl) {
                ret = wireless_trans->ioctrl(WIRELESS_DEV_OP_SEND_PACKET, buf, len, priv);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return ret;
        }
    }
    WIRELESS_DEV_EXIT_CRITICAL();
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播同步数据设置接口
 *
 * @param name:dev name for find dev
 * @param buf: data buffer
 * @param len: data length
 * @param priv:dev operating parameters
 *
 * @return 0:succ, other:err
 */
/* ----------------------------------------------------------------------------*/
int wireless_trans_status_sync(const char *name, const void *const buf, size_t len, void *priv)
{
    int ret = -1;
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->ioctrl) {
                ret = wireless_trans->ioctrl(WIRELESS_DEV_OP_STATUS_SYNC, priv, buf, len);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return ret;
        }
    }
    WIRELESS_DEV_EXIT_CRITICAL();
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取当前设备的即时时钟
 *
 * @param name:dev name for fine dev
 * @param clk:pointer for get clock
 *
 * @return 0:succ, other:err
 */
/* ----------------------------------------------------------------------------*/
int wireless_trans_get_cur_clk(const char *name, void *clk)
{
    int ret = -1;
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->ioctrl) {
                ret = wireless_trans->ioctrl(WIRELESS_DEV_OP_GET_BLE_CLK, clk);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return ret;
        }
    }
    WIRELESS_DEV_EXIT_CRITICAL();
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 用于获取发送方最近一次发数时钟
 *
 * @param name:dev name for fine dev
 *
 * @return 0:succ, other:err
 */
/* ----------------------------------------------------------------------------*/
int wireless_trans_get_last_tx_clk(const char *name, void *priv)
{
    int ret = -1;
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->ioctrl) {
                ret = wireless_trans->ioctrl(WIRELESS_DEV_OP_GET_TX_SYNC, priv);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return ret;
        }
    }
    WIRELESS_DEV_EXIT_CRITICAL();
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief init dev private params
 *
 * @param name:dev name for find dev
 * @param priv:dev operating parameters
 *
 * @return 0:succ, other:err
 */
/* ----------------------------------------------------------------------------*/
int wireless_trans_init(const char *name, void *priv)
{
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->init) {
                wireless_trans->init(priv);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return 0;
        }
    }

    WIRELESS_DEV_EXIT_CRITICAL();
    return -1;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief uninit dev private params
 *
 * @param name:dev name for find dev
 * @param priv:dev operating parameters
 *
 * @return 0:succ, other:err
 */
/* ----------------------------------------------------------------------------*/
int wireless_trans_uninit(const char *name, void *priv)
{
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->uninit) {
                wireless_trans->uninit(priv);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return 0;
        }
    }

    WIRELESS_DEV_EXIT_CRITICAL();
    return -1;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief startup dev
 *
 * @param name:dev name for find dev
 * @param priv:dev operating parameters
 *
 * @return 0:succ, other:err
 */
/* ----------------------------------------------------------------------------*/
int wireless_trans_open(const char *name, void *priv)
{
    int ret = -1;
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->open) {
                ret = wireless_trans->open(priv);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return ret;
        }
    }

    WIRELESS_DEV_EXIT_CRITICAL();
    return ret;
}

int wireless_trans_trigger_latch_time(const char *name, void *priv)
{
    int ret = 0;
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->ioctrl) {
                ret = wireless_trans->ioctrl(WIRELESS_DEV_OP_SET_SYNC, (u16)priv, 1);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return ret;
        }
    }
    WIRELESS_DEV_EXIT_CRITICAL();
    return ret;
}

int wireless_trans_get_latch_time_us(const char *name, u16 *us_1_12th, u32 *clk_us, u32 *event, void *priv)
{
    int ret = 0;
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->ioctrl) {
                ret = wireless_trans->ioctrl(WIRELESS_DEV_OP_GET_SYNC, (u16)priv, us_1_12th, clk_us, event);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return ret;
        }
    }
    WIRELESS_DEV_EXIT_CRITICAL();
    return ret;
}

int wireless_trans_audio_sync_enable(const char *name, void *priv, u8 enable)
{
    int ret = 0;
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->ioctrl) {
                ret = wireless_trans->ioctrl(WIRELESS_DEV_OP_SET_SYNC, (u16)priv, 0);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return ret;
        }
    }
    WIRELESS_DEV_EXIT_CRITICAL();
    return ret;

}
/* --------------------------------------------------------------------------*/
/**
 * @brief stop and close dev
 *
 * @param name:dev name for find dev
 * @param priv:dev operating parameters
 *
 * @return 0:succ, other:err
 */
/* ----------------------------------------------------------------------------*/
int wireless_trans_close(const char *name, void *priv)
{
    int ret = -1;
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->close) {
                ret = wireless_trans->close(priv);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return ret;
        }
    }

    WIRELESS_DEV_EXIT_CRITICAL();
    return ret;
}

int wireless_trans_enter_pair(const char *name, u8 mode, void *priv, u32 pair_code)
{
    int ret = -1;
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->ioctrl) {
                ret = wireless_trans->ioctrl(WIRELESS_DEV_OP_ENTER_PAIR, mode, priv, pair_code);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return ret;
        }
    }
    WIRELESS_DEV_EXIT_CRITICAL();
    return ret;
}

int wireless_trans_exit_pair(const char *name, void *priv)
{
    int ret = -1;
    const wireless_trans_ops *wireless_trans;
    WIRELESS_DEV_ENTER_CRITICAL();
    list_for_each_wireless_trans(wireless_trans) {
        if (!strcmp(name, wireless_trans->name)) {
            if (wireless_trans->ioctrl) {
                ret = wireless_trans->ioctrl(WIRELESS_DEV_OP_EXIT_PAIR, priv);
            }
            WIRELESS_DEV_EXIT_CRITICAL();
            return ret;
        }
    }
    WIRELESS_DEV_EXIT_CRITICAL();
    return ret;
}

