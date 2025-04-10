#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".multi_protocol_common.data.bss")
#pragma data_seg(".multi_protocol_common.data")
#pragma const_seg(".multi_protocol_common.text.const")
#pragma code_seg(".multi_protocol_common.text")
#endif
#include "system/includes.h"
#include "multi_protocol_main.h"
#include "classic/tws_api.h"
#include "bt_tws.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN | GFPS_EN | MMA_EN | FMNA_EN | REALME_EN | SWIFT_PAIR_EN | DMA_EN | ONLINE_DEBUG_EN | CUSTOM_DEMO_EN)) || ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN | LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)))


#if 0
#define log_info(x, ...)       printf("[MULTI_PROTOCOL]" x " ", ## __VA_ARGS__)
#define log_info_hexdump       put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#if TCFG_USER_TWS_ENABLE

/*******************************************************
               BLE/SPP 中间层句柄 TWS 同步
*******************************************************/

#define TWS_FUNC_ID_MULTI_PROTOCOL_TWS_SYNC \
	(((u8)('M' + 'U' + 'L' + 'T' + 'I') << (3 * 8)) | \
	 ((u8)('P' + 'R' + 'O' + 'T' + 'O') << (2 * 8)) | \
	 ((u8)('C' + 'O' + 'L') << (1 * 8)) | \
	 ((u8)('T' + 'W' + 'S') << (0 * 8)))

#define MULTI_PROTOCOL_TWS_SYNC_BLE     (0x01)
#define MULTI_PROTOCOL_TWS_SYNC_SPP     (0x02)

extern const bool config_tws_le_role_sw;    //是否支持ble跟随tws主从切换
void multi_protocol_tws_sync_send(void)
{
    u8 *temp_buf = 0;
    int size = 0;

    log_info("tws sync : %d %d\n", get_bt_tws_connect_status(), tws_api_get_role());
    if (!get_bt_tws_connect_status() || !(tws_api_get_role() == TWS_ROLE_MASTER)) {
        return;
    }

    if (config_tws_le_role_sw) {
        size = app_ble_all_sync_data_size();
        if (size) {
            temp_buf = zalloc(size + 1);
            temp_buf[0] = MULTI_PROTOCOL_TWS_SYNC_BLE;
            app_ble_all_sync_data_get(&temp_buf[1]);
            tws_api_send_data_to_sibling(temp_buf, size + 1, TWS_FUNC_ID_MULTI_PROTOCOL_TWS_SYNC);
            free(temp_buf);
        }
    }

    size = app_spp_all_sync_data_size();
    if (size) {
        temp_buf = zalloc(size + 1);
        temp_buf[0] = MULTI_PROTOCOL_TWS_SYNC_SPP;
        app_spp_all_sync_data_get(&temp_buf[1]);
        tws_api_send_data_to_sibling(temp_buf, size + 1, TWS_FUNC_ID_MULTI_PROTOCOL_TWS_SYNC);
        free(temp_buf);
    }
}

static void multi_protocol_tws_sync_in_task(u8 *data, int len)
{
    int i;
    log_info("multi_protocol_tws_sync_in_task %d\n", len);
    log_info_hexdump(data, len);
    switch (data[0]) {
    case MULTI_PROTOCOL_TWS_SYNC_BLE:
        app_ble_all_sync_data_set(&data[1], len - 1);
        break;
    case MULTI_PROTOCOL_TWS_SYNC_SPP:
        app_spp_all_sync_data_set(&data[1], len - 1);
        break;
    }
    free(data);
}

static void multi_protocol_tws_sync_in_irq(void *_data, u16 len, bool rx)
{
    int i;
    int argv[4];
    u8 *rx_data = NULL;
    log_info("multi_protocol_tws_sync_in_irq %d\n", len);
    log_info_hexdump(_data, len);
    if (get_bt_tws_connect_status()) {
        if (rx && (tws_api_get_role() == TWS_ROLE_SLAVE)) {
            rx_data = malloc(len);
            if (rx_data == NULL) {
                return;
            }
            memcpy(rx_data, _data, len);
            argv[0] = (int)multi_protocol_tws_sync_in_task;
            argv[1] = 2;
            argv[2] = (int)rx_data;
            argv[3] = (int)len;
            int ret = os_taskq_post_type("app_core", Q_CALLBACK, 4, argv);
            if (ret) {
                printf("%s taskq post err \n", __func__);
            }
        }
    }
}

REGISTER_TWS_FUNC_STUB(tws_rcsp_bt_hdl_sync) = {
    .func_id = TWS_FUNC_ID_MULTI_PROTOCOL_TWS_SYNC,
    .func = multi_protocol_tws_sync_in_irq,
};

/*******************************************************
               BLE/SPP 中间层句柄 TWS 同步 end
*******************************************************/

#endif

#endif

