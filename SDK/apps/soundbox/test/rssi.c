#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rssi.data.bss")
#pragma data_seg(".rssi.data")
#pragma const_seg(".rssi.text.const")
#pragma code_seg(".rssi.text")
#endif
#include "btstack/avctp_user.h"
#include "app_config.h"
#include "bt_tws.h"
#include "app_main.h"


#if TCFG_USER_RSSI_TEST_EN


static void spp_send_rssi(int slave_rssi)
{
    u8 send_buf[8];

    send_buf[0] = 0xfe;
    send_buf[1] = 0xdc;
    send_buf[2] = 0x04;
    send_buf[3] = 0x02;
    if (tws_api_get_local_channel() == 'L') {
        get_rssi_api((char *)&send_buf[4], (char *)&send_buf[6]);
        send_buf[5] = (s8)slave_rssi;
    } else {
        get_rssi_api((char *)&send_buf[5], (char *)&send_buf[6]);
        send_buf[4] = (s8)slave_rssi;
    }
    send_buf[7] = 0xba;
    bt_cmd_prepare(USER_CTRL_SPP_SEND_DATA, sizeof(send_buf), send_buf);
    /* put_buf(send_buf, sizeof(send_buf)); */
    /* printf("get rssi, l:%d, r:%d, tws:%d", send_buf[4], send_buf[5], send_buf[6]); */
}

static void app_core_send_rssi(int slave_rssi)
{
    int msg[3];

    msg[0] = (int)spp_send_rssi;
    msg[1] = 1;
    msg[2] = slave_rssi;
    os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
}

#define TWS_FUNC_ID_RSSI_SYNC	TWS_FUNC_ID('R', 'S', 'S', 'I')

static void tws_get_rssi_handler(void *_data, u16 len, bool rx)
{
    u8 *data = (u8 *)_data;

    if (rx) {
        app_core_send_rssi(data[0]);
    }
}

REGISTER_TWS_FUNC_STUB(tws_get_rssi) = {
    .func_id = TWS_FUNC_ID_RSSI_SYNC,
    .func    = tws_get_rssi_handler,
};

int spp_get_rssi_handler(u8 *packet, u16 size)
{
    const u8 get_rssi_cmd[] = {0xfe, 0xdc, 0x01, 0x01, 0xba};
    u8 send_cmd_buffer[2];
    if (size == sizeof(get_rssi_cmd) && !memcmp(packet, get_rssi_cmd, size)) {
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            if (tws_api_get_role() == TWS_ROLE_SLAVE) {
                get_rssi_api((char *)&send_cmd_buffer[0], (char *)&send_cmd_buffer[1]);
                tws_api_send_data_to_sibling(send_cmd_buffer, sizeof(send_cmd_buffer),
                                             TWS_FUNC_ID_RSSI_SYNC);
            }
        } else {
            app_core_send_rssi(-127);
        }
        return 1;
    }
    return 0;
}

#endif
