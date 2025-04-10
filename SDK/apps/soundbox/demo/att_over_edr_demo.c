#include "system/includes.h"
#include "app_config.h"
#include "app_msg.h"
#include "btcontroller_config.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"
#include "btstack/le/ble_api.h"
#include "btstack/le/att.h"

#define ATT_OVER_EDR_DEMO_EN          1//enalbe att_over_edr_demo

/*
开启功能需要在apps/common/config/bt_profile_config.c中修改
const u8 adt_profile_support = 1;
edr att不接入中间层：
att_read_callback和att_write_callback与ble使用相同
发数接口：
void set_adt_send_handle(u16 handle_val)  //用于切换发送handle
bt_cmd_prepare(USER_CTRL_ADT_SEND_DATA, 15, (u8 *)send_data);

edr att接入中间层：
使用中间层的注册接口，实现发送和接收数据；
if (rcsp_adt_support) {
if (rcsp_server_edr_att_hdl == NULL) {
    rcsp_server_edr_att_hdl = app_ble_hdl_alloc();
}
    app_ble_adv_address_type_set(rcsp_server_edr_att_hdl, 0);
    app_ble_att_connect_type_set(rcsp_server_edr_att_hdl, BD_ADDR_TYPE_EDR_ATT);
    app_ble_hdl_uuid_set(rcsp_server_edr_att_hdl, EDR_ATT_HDL_UUID);
    app_ble_profile_set(rcsp_server_edr_att_hdl, rcsp_profile_data);
    app_ble_callback_register(rcsp_server_edr_att_hdl);
}

注意：edr连接之后app连接需要开ble广播出来用于发现设备，广播类型可连接和不可连接都可以
app连接成功后可关闭广播。
*/

#if ATT_OVER_EDR_DEMO_EN
#define log_info(x, ...)  printf("\n[###edr_att_demo@@@]" x " ", ## __VA_ARGS__)
extern const u8 adt_profile_support;
u8 rcsp_adt_support = 1; //edr att是否接入rcsp

extern void att_event_handler_register(void (*handler)(u8 packet_type, u16 channel, u8 *packet, u16 size));
extern void adt_profile_init(u16 psm, uint8_t const *db, att_read_callback_t rcallback, att_write_callback_t wcallback);
extern void bredr_adt_init();

const u8 sdp_att_service_data[60] = {                           //
    0x36, 0x00, 0x31, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x21, 0x09, 0x00, 0x01, 0x35, 0x03,
    0x19, 0x18, 0x01, 0x09, 0x00, 0x04, 0x35, 0x13, 0x35, 0x06, 0x19, 0x01, 0x00, 0x09, 0x00, 0x1F,
    0x35, 0x09, 0x19, 0x00, 0x07, 0x09, 0x00, 0x01, 0x09, 0x00, 0x04, 0x09, 0x00, 0x05, 0x35, 0x03,
    0x19, 0x10, 0x02, 0x00                    //                //
};
const u8 sdp_att_service_data1[60] = {
    0x36, 0x00, 0x31, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x22, 0x09, 0x00, 0x01, 0x35, 0x03,
    0x19, 0xAE, 0x00, 0x09, 0x00, 0x04, 0x35, 0x13, 0x35, 0x06, 0x19, 0x01, 0x00, 0x09, 0x00, 0x1F,
    0x35, 0x09, 0x19, 0x00, 0x07, 0x09, 0x00, 0x05, 0x09, 0x00, 0x0a, 0x09, 0x00, 0x05, 0x35, 0x03,
    0x19, 0x10, 0x02, 0x00
};
const u8 sdp_att_service_data2[60] = {
    0x36, 0x00, 0x31, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x23, 0x09, 0x00, 0x01, 0x35, 0x03,
    0x19, 0xBF, 0x00, 0x09, 0x00, 0x04, 0x35, 0x13, 0x35, 0x06, 0x19, 0x01, 0x00, 0x09, 0x00, 0x1F,
    0x35, 0x09, 0x19, 0x00, 0x07, 0x09, 0x00, 0x0b, 0x09, 0x00, 0x10, 0x09, 0x00, 0x05, 0x35, 0x03,
    0x19, 0x10, 0x02, 0x00
};

typedef struct {
    // linked list - assert: first field
    void *offset_item;
    // data is contained in same memory
    u32        service_record_handle;
    u8         *service_record;
} service_record_item_t;
#define SDP_RECORD_HANDLER_REGISTER(handler) \
    const service_record_item_t  handler \
    sec(.sdp_record_item)
SDP_RECORD_HANDLER_REGISTER(spp_att_record_item) = {
    .service_record = (u8 *)sdp_att_service_data,
    .service_record_handle = 0x00010021,
};
SDP_RECORD_HANDLER_REGISTER(spp_att_record_item1) = {
    .service_record = (u8 *)sdp_att_service_data1,
    .service_record_handle = 0x00010022,
};
SDP_RECORD_HANDLER_REGISTER(spp_att_record_item2) = {
    .service_record = (u8 *)sdp_att_service_data2,
    .service_record_handle = 0x00010023,
};

static const uint8_t self_define_profile_data[] = {
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

    /* CHARACTERISTIC,  ae03, WRITE_WITHOUT_RESPONSE | DYNAMIC, */
    // 0x000a CHARACTERISTIC ae03 WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x0a, 0x00, 0x03, 0x28, 0x04, 0x0b, 0x00, 0x03, 0xae,
    // 0x000b VALUE ae03 WRITE_WITHOUT_RESPONSE | DYNAMIC
    0x08, 0x00, 0x04, 0x01, 0x0b, 0x00, 0x03, 0xae,

    /* CHARACTERISTIC,  ae04, NOTIFY, */
    // 0x000c CHARACTERISTIC ae04 NOTIFY
    0x0d, 0x00, 0x02, 0x00, 0x0c, 0x00, 0x03, 0x28, 0x10, 0x0d, 0x00, 0x04, 0xae,
    // 0x000d VALUE ae04 NOTIFY
    0x08, 0x00, 0x10, 0x00, 0x0d, 0x00, 0x04, 0xae,
    // 0x000e CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x0e, 0x00, 0x02, 0x29, 0x00, 0x00,

    /* CHARACTERISTIC,  ae05, INDICATE, */
    // 0x000f CHARACTERISTIC ae05 INDICATE
    0x0d, 0x00, 0x02, 0x00, 0x0f, 0x00, 0x03, 0x28, 0x20, 0x10, 0x00, 0x05, 0xae,
    // 0x0010 VALUE ae05 INDICATE
    0x08, 0x00, 0x20, 0x00, 0x10, 0x00, 0x05, 0xae,
    // 0x0011 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x11, 0x00, 0x02, 0x29, 0x00, 0x00,

    /* CHARACTERISTIC,  ae10, READ | WRITE | DYNAMIC, */
    // 0x0012 CHARACTERISTIC ae10 READ | WRITE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x12, 0x00, 0x03, 0x28, 0x0a, 0x13, 0x00, 0x10, 0xae,
    // 0x0013 VALUE ae10 READ | WRITE | DYNAMIC
    0x08, 0x00, 0x0a, 0x01, 0x13, 0x00, 0x10, 0xae,

    // END
    0x00, 0x00,
};

#define ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE 0x0003
#define ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE 0x0006
#define ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE 0x0008
#define ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE 0x0009
#define ATT_CHARACTERISTIC_ae03_01_VALUE_HANDLE 0x000b
#define ATT_CHARACTERISTIC_ae04_01_VALUE_HANDLE 0x000d
#define ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE 0x000e
#define ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE 0x0010
#define ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE 0x0011
#define ATT_CHARACTERISTIC_ae10_01_VALUE_HANDLE 0x0013


//------
#define ATT_LOCAL_PAYLOAD_SIZE    (256)                   //note: need >= 20
#define ATT_SEND_CBUF_SIZE        (512)                   //note: need >= 20,缓存大小，可修改
#define ATT_RAM_BUFSIZE           (ATT_LOCAL_PAYLOAD_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 *att_ram_buffer = NULL;

extern void set_adt_send_handle(u16 handle_val);
//---------------

void begin_to_connect_att(void)
{
    printf("[Y]%s\n", __FUNCTION__);
    if (adt_profile_support) {
        log_info("------------USER_CTRL_ADT_CONNECT-------------\n");
        bt_cmd_prepare(USER_CTRL_ADT_CONNECT, 0, NULL);
    }
}

void test_send_data_timer(void *priv)
{
    printf("[Y]%s\n", __FUNCTION__);
    int error = 0;
    putchar('A');
    u8 send_data[16] = "---123456789---";
    bt_cmd_prepare(USER_CTRL_ADT_SEND_DATA, 15, (u8 *)send_data);
    log_info("==%d==\n", error);
}
static char gap_device_name[20] = "jl_att_test";
static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    printf("[Y]%s\n", __FUNCTION__);
    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;
    /* log_info("READ Callback, handle %04x, offset %u, buffer size %u\n", handle, offset, buffer_size); */
    log_info("read_callback, handle= 0x%04x,buffer= %08x\n", handle, (u32)buffer);
    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        att_value_len = strlen(gap_device_name);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &gap_device_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("\n------read gap_name: %s \n", gap_device_name);
        }
        break;
    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = att_get_ccc_config(handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;
    default:
        break;
    }
    log_info("att_value_len= %d\n", att_value_len);
    return att_value_len;
}


static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    printf("[Y]%s\n", __FUNCTION__);
    int result = 0;
    u16 tmp16;
    u16 handle = att_handle;
    log_info("att write_callback, handle= 0x%04x\n", handle);
    switch (handle) {
    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:
        log_info("-dma_rx():");
        put_buf(buffer, buffer_size);
        break;
    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
        log_info("---ii---write ccc:%04x, %02x\n", handle, buffer[0]);
        att_set_ccc_config(handle, buffer[0]);
        set_adt_send_handle(ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE);
        sys_timer_add(NULL, test_send_data_timer, 1000);
        break;
    case ATT_CHARACTERISTIC_ae04_01_VALUE_HANDLE:
        log_info("-dma_rx():");
        put_buf(buffer, buffer_size);
        break;
    case ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE:
        att_set_ccc_config(handle, buffer[0]);
        break;
    default:
        break;
    }
    return 0;
}


enum {
    ATT_EVENT_CONNECT = 1,
    ATT_EVENT_DISCONNECT,
    ATT_EVENT_MONITOR_START,//监听开始
    ATT_EVENT_PACKET_HANDLER = 7,
};
static void att_packet_handler(u8 packet_type, u16 channel, u8 *packet, u16 size)
{
    printf("[Y]%s\n", __FUNCTION__);
    switch (packet_type) {
    case ATT_EVENT_CONNECT:
        ASSERT(att_ram_buffer == NULL, "att_pool is err\n");
        att_ram_buffer = malloc(ATT_RAM_BUFSIZE);
        ASSERT(att_ram_buffer, "att_pool is not ok\n");
        ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, channel, att_ram_buffer, ATT_RAM_BUFSIZE, ATT_LOCAL_PAYLOAD_SIZE);
        break;
    case ATT_EVENT_DISCONNECT:
        if (att_ram_buffer) {
            free(att_ram_buffer);
        }
        break;
    case ATT_EVENT_MONITOR_START:
    case ATT_EVENT_PACKET_HANDLER:
        break;
    }
}

void att_profile_init()
{
    bredr_adt_init();
#if 0
    printf("------------------------------tag30\n");
    printf("[Y]%s\n", __FUNCTION__);
    if (rcsp_adt_support) {
        printf("------------------------------tag31\n");
        bredr_adt_init();
    } else {
        printf("------------------------------tag32\n");
        att_event_handler_register(att_packet_handler);
        adt_profile_init(0, self_define_profile_data, att_read_callback, att_write_callback);
    }
#endif
}

static int edr_att_btstack_event_handler(int *msg)
{
    printf("[Y]%s\n", __FUNCTION__);
    struct bt_event *bt = (struct bt_event *)msg;

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        att_profile_init();
        break;
    }
    return 0;
}
APP_MSG_HANDLER(edr_att_stack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = edr_att_btstack_event_handler,
};

#endif

