#include "sdk_config.h"
#include "app_msg.h"
// #include "earphone.h"
#include "bt_tws.h"
#include "app_main.h"
#include "btstack/avctp_user.h"
#include "multi_protocol_main.h"
#include "custom_protocol.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & CUSTOM_DEMO_EN)

void *custom_demo_ble_hdl = NULL;
void *custom_demo_spp_hdl = NULL;

/*************************************************
                  BLE 相关内容
*************************************************/

const uint8_t custom_demo_profile_data[] = {
    //////////////////////////////////////////////////////
    //
    // 0x0001 PRIMARY_SERVICE  1800
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x28, 0x00, 0x18,

    /* CHARACTERISTIC,  2a00, READ | WRITE | DYNAMIC, */
    // 0x0002 CHARACTERISTIC 2a00 READ | WRITE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x02, 0x00, 0x03, 0x28, 0x0a, 0x03, 0x00, 0x00, 0x2a,
    // 0x0003 VALUE 2a00 READ | WRITE | DYNAMIC
    0x08, 0x00, 0x0a, 0x01, 0x03, 0x00, 0x00, 0x2a,

    //////////////////////////////////////////////////////
    //
    // 0x0004 PRIMARY_SERVICE  ae00
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x28, 0x00, 0xae,

    /* CHARACTERISTIC,  ae01, WRITE_WITHOUT_RESPONSE | DYNAMIC, */
    // 0x0005 CHARACTERISTIC ae01 WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x05, 0x00, 0x03, 0x28, 0x04, 0x06, 0x00, 0x01, 0xae,
    // 0x0006 VALUE ae01 WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x08, 0x00, 0x04, 0x01, 0x06, 0x00, 0x01, 0xae,

    /* CHARACTERISTIC,  ae02, NOTIFY, */
    // 0x0007 CHARACTERISTIC ae02 NOTIFY
    0x0d, 0x00, 0x02, 0x00, 0x07, 0x00, 0x03, 0x28, 0x10, 0x08, 0x00, 0x02, 0xae,
    // 0x0008 VALUE ae02 NOTIFY
    0x08, 0x00, 0x10, 0x00, 0x08, 0x00, 0x02, 0xae,
    // 0x0009 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x09, 0x00, 0x02, 0x29, 0x00, 0x00,

    // END
    0x00, 0x00,
};

//
// characteristics <--> handles
//
#define ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE 0x0003
#define ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE 0x0006
#define ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE 0x0008
#define ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE 0x0009

static u16 custom_adv_interval_min = 150;

static void custom_cbk_packet_handler(void *hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    u16 con_handle;
    // printf("cbk packet_type:0x%x, packet[0]:0x%x, packet[2]:0x%x", packet_type, packet[0], packet[2]);
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case ATT_EVENT_CAN_SEND_NOW:
            printf("ATT_EVENT_CAN_SEND_NOW");
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                con_handle = little_endian_read_16(packet, 4);
                printf("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x", con_handle);
                // reverse_bd_addr(&packet[8], addr);
                put_buf(&packet[8], 6);
                break;
            default:
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            printf("HCI_EVENT_DISCONNECTION_COMPLETE: %0x", packet[5]);
            custom_demo_adv_enable(1);
            break;
        default:
            break;
        }
        break;
    }
    return;
}

static uint16_t custom_att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;
    printf("<-------------read_callback, handle= 0x%04x,buffer= %08x", handle, (u32)buffer);
    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        const char *gap_name = bt_get_local_name();
        att_value_len = strlen(gap_name);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &gap_name[offset], buffer_size);
            att_value_len = buffer_size;
            printf("\n------read gap_name: %s", gap_name);
        }
        break;
    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = att_get_ccc_config(handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;
    default:
        break;
    }
    printf("att_value_len= %d", att_value_len);
    return att_value_len;
    return 0;
}

static int custom_att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;
    u16 handle = att_handle;
    printf("<-------------write_callback, handle= 0x%04x,size = %d", handle, buffer_size);
    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;
    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:
        printf("rx(%d):\n", buffer_size);
        put_buf(buffer, buffer_size);
        // test
        custom_demo_ble_send(buffer, buffer_size);
        break;
    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
        printf("\nwrite ccc:%04x, %02x\n", handle, buffer[0]);
        att_set_ccc_config(handle, buffer[0]);
        break;
    default:
        break;
    }
    return 0;
}

static u8 custom_fill_adv_data(u8 *adv_data)
{
    u8 offset = 0;
    const char *name_p = bt_get_local_name();
    int name_len = strlen(name_p);
    offset += make_eir_packet_data(&adv_data[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)name_p, name_len);
    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return 0;
    }
    return offset;
}

static u8 custom_fill_rsp_data(u8 *rsp_data)
{
    return 0;
}


int custom_demo_adv_enable(u8 enable)
{
    uint8_t adv_type = ADV_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    uint8_t advData[ADV_RSP_PACKET_MAX] = {0};
    uint8_t rspData[ADV_RSP_PACKET_MAX] = {0};
    uint8_t len = 0;

    if (enable == app_ble_adv_state_get(custom_demo_ble_hdl)) {
        return 0;
    }
    if (enable) {
        app_ble_set_adv_param(custom_demo_ble_hdl, custom_adv_interval_min, adv_type, adv_channel);
        len = custom_fill_adv_data(advData);
        if (len) {
            app_ble_adv_data_set(custom_demo_ble_hdl, advData, len);
        }
        len = custom_fill_rsp_data(rspData);
        if (len) {
            app_ble_rsp_data_set(custom_demo_ble_hdl, rspData, len);
        }
    }
    app_ble_adv_enable(custom_demo_ble_hdl, enable);
    return 0;
}

int custom_demo_ble_send(u8 *data, u32 len)
{
    int ret = 0;
    int i;
    printf("custom_demo_ble_send len = %d", len);
    put_buf(data, len);
    ret = app_ble_att_send_data(custom_demo_ble_hdl, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
    if (ret) {
        printf("send fail\n");
    }
    return ret;
}
/*************************************************
                  BLE 相关内容 end
*************************************************/

/*************************************************
                  SPP 相关内容
*************************************************/
static void custom_spp_state_callback(void *hdl, void *remote_addr, u8 state)
{
    int i;
    int bond_flag = 0;
    switch (state) {
    case SPP_USER_ST_CONNECT:
        printf("custom spp connect#########\n");
        // 将 custom_demo_spp_hdl 绑定到连接上的设备地址，否则后续会收到所有已连接设备地址的事件和数据
        app_spp_set_filter_remote_addr(custom_demo_spp_hdl, remote_addr);
        break;
    case SPP_USER_ST_DISCONN:
        printf("custom spp disconnect#########\n");
        break;
    };
}

static void custom_spp_recieve_callback(void *hdl, void *remote_addr, u8 *buf, u16 len)
{
    printf("custom_spp_recieve_callback len=%d\n", len);
    put_buf(buf, len);

    // test send
    custom_demo_spp_send(buf, len);
}

int custom_demo_spp_send(u8 *data, u32 len)
{
    return app_spp_data_send(custom_demo_spp_hdl, data, len);
}

/*************************************************
                  SPP 相关内容 end
*************************************************/

void custom_demo_all_init(void)
{
    printf("custom_demo_all_init\n");
    const uint8_t *edr_addr = bt_get_mac_addr();
    printf("edr addr:");
    put_buf((uint8_t *)edr_addr, 6);

    // BLE init
    if (custom_demo_ble_hdl == NULL) {
        custom_demo_ble_hdl = app_ble_hdl_alloc();
        if (custom_demo_ble_hdl == NULL) {
            printf("custom_demo_ble_hdl alloc err !\n");
            return;
        }
        app_ble_set_mac_addr(custom_demo_ble_hdl, (void *)edr_addr);
        app_ble_profile_set(custom_demo_ble_hdl, custom_demo_profile_data);
        app_ble_att_read_callback_register(custom_demo_ble_hdl, custom_att_read_callback);
        app_ble_att_write_callback_register(custom_demo_ble_hdl, custom_att_write_callback);
        app_ble_att_server_packet_handler_register(custom_demo_ble_hdl, custom_cbk_packet_handler);
        app_ble_hci_event_callback_register(custom_demo_ble_hdl, custom_cbk_packet_handler);
        app_ble_l2cap_packet_handler_register(custom_demo_ble_hdl, custom_cbk_packet_handler);

        custom_demo_adv_enable(1);
    }
    // BLE init end

    // SPP init
    if (custom_demo_spp_hdl == NULL) {
        custom_demo_spp_hdl = app_spp_hdl_alloc(0x0);
        if (custom_demo_spp_hdl == NULL) {
            printf("custom_demo_spp_hdl alloc err !\n");
            return;
        }
        app_spp_recieve_callback_register(custom_demo_spp_hdl, custom_spp_recieve_callback);
        app_spp_state_callback_register(custom_demo_spp_hdl, custom_spp_state_callback);
        app_spp_wakeup_callback_register(custom_demo_spp_hdl, NULL);
    }
    // SPP init end
}

void custom_demo_all_exit(void)
{
    printf("custom_demo_all_exit\n");

    // BLE exit
    if (app_ble_get_hdl_con_handle(custom_demo_ble_hdl)) {
        app_ble_disconnect(custom_demo_ble_hdl);
    }
    app_ble_hdl_free(custom_demo_ble_hdl);
    custom_demo_ble_hdl = NULL;

    // SPP init
    if (NULL != app_spp_get_hdl_remote_addr(custom_demo_spp_hdl)) {
        app_spp_disconnect(custom_demo_spp_hdl);
    }
    app_spp_hdl_free(custom_demo_spp_hdl);
    custom_demo_spp_hdl = NULL;
}

#endif

