#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".multi_protocol_main.data.bss")
#pragma data_seg(".multi_protocol_main.data")
#pragma const_seg(".multi_protocol_main.text.const")
#pragma code_seg(".multi_protocol_main.text")
#endif
#include "system/includes.h"
#include "clock_manager/clock_manager.h"
#include "multi_protocol_main.h"
#include "bt_tws.h"
#include "swift_pair_api.h"
#include "gfps_platform_api.h"
#include "btstack_rcsp_user.h"
#include "ble_rcsp_server.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & DMA_EN)
#include "dma_platform_api.h"
#endif


#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN | GFPS_EN | MMA_EN | FMNA_EN | REALME_EN | SWIFT_PAIR_EN | DMA_EN | ONLINE_DEBUG_EN | CUSTOM_DEMO_EN)) || ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN | LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)))
#define ATT_LOCAL_PAYLOAD_SIZE    (517)//(517)              //note: need >= 20
#define ATT_SEND_CBUF_SIZE        (512*2)                   //note: need >= 20,缓存大小，可修改
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_PAYLOAD_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 att_ram_buffer[ATT_RAM_BUFSIZE] __attribute__((aligned(4)));

typedef struct {
    // linked list - assert: first field
    void *offset_item;
    // data is contained in same memory
    u32        service_record_handle;
    u8         *service_record;
} sdp_protocal_item_t;
#define SDP_RECORD_REGISTER(handler) \
	const sdp_protocal_item_t  handler sec(.sdp_record_item)

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)

extern const u8 sdp_spp_service_data[];
SDP_RECORD_REGISTER(rcsp_sdp_record_item) = {
    .service_record = (u8 *)sdp_spp_service_data,
    .service_record_handle = 0x00010004,
};

// rcsp ble profile，用户可以此为基础添加需要ble服务和特征
const uint8_t rcsp_profile_data[] = {
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
#endif

#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
extern u8 get_bt_le_audio_config();
extern void le_audio_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en);
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & GFPS_EN)

#include "gfps_platform_api.h"

extern const u8 sdp_gfp_spp_service_data[];
SDP_RECORD_REGISTER(gfps_record_item) = {
    .service_record = (u8 *)sdp_gfp_spp_service_data,
    .service_record_handle = 0x00010039,
};

static const uint8_t google_model_id_used[3] = {0xF2, 0x63, 0x8D};
static const char google_public_key_used[64] = {
    0x5f, 0x17, 0xf0, 0x5a, 0x22, 0x03, 0xe2, 0xdc, 0x93, 0x1d, 0x5c, 0x00, 0xe8, 0xa4, 0x06, 0x7c,
    0x3c, 0x09, 0xa0, 0xad, 0x91, 0x82, 0x3e, 0x45, 0xda, 0xe7, 0xe5, 0x2f, 0x5e, 0xe9, 0x80, 0x1b,
    0xa6, 0xe2, 0x21, 0x9f, 0x65, 0x85, 0xa8, 0xab, 0x65, 0xf1, 0x47, 0x7e, 0x46, 0x7a, 0xdb, 0xb2,
    0x4b, 0x46, 0x72, 0xb8, 0x35, 0x89, 0x0b, 0x99, 0x57, 0xb7, 0x0e, 0xa9, 0x45, 0xe1, 0x7a, 0xbb
};
static const char google_private_key_used[32] = {
    0x86, 0x89, 0x57, 0x30, 0xFB, 0x95, 0x8A, 0x9E, 0x42, 0x57, 0x56, 0x97,
    0x39, 0x54, 0x77, 0xE7, 0x62, 0x44, 0x7A, 0xA1, 0x2B, 0xDF, 0x35, 0xFE,
    0xE9, 0xC5, 0x82, 0x28, 0x1B, 0xD7, 0xF5, 0xBE
};
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & MMA_EN)

extern void *xm_app_ble_hdl;

extern int  xm_all_init(void);
extern int  xm_all_exit(void);
extern void xm_ble_profile_init(void);
extern void xm_bt_ble_init(void);
extern void xm_bt_ble_exit(void);
extern void xm_bt_ble_adv_enable(u8 enable);
extern void mma_is_tws_master_callback_register(bool (*handler)(void));
extern int  xm_message_deal_handler(int id, int opcode, u8 *data, u32 len);
extern void mma_message_callback_register(int (*handler)(int id, int opcode, u8 *data, u32 len));

extern bool app_protocol_get_battery(u8 type, u8 *value);
extern void mma_get_battery_callback_register(bool (*handler)(u8 battery_type, u8 *value));
extern int mma_protocol_loop_process();
extern void mma_tx_resume_register(void (*handler)(void));
extern void mma_rx_resume_register(void (*handler)(void));

extern const u8 sdp_xiaoai_service_data[];
extern const u8 sdp_miui_service_data[];
SDP_RECORD_REGISTER(xm_record_item) = {
    .service_record = (u8 *)sdp_miui_service_data,
    .service_record_handle = 0x00010040,
};
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & REALME_EN)

extern const u8 sdp_realme_melody_spp_service_data[];
extern const u8 sdp_realme_link_spp_service_data[];

// realme link
SDP_RECORD_REGISTER(realme_link_record_item) = {
    .service_record = (u8 *)sdp_realme_link_spp_service_data,
    .service_record_handle = 0x00010040,
};

// melody
SDP_RECORD_REGISTER(realme_melody_record_item) = {
    .service_record = (u8 *)sdp_realme_melody_spp_service_data,
    .service_record_handle = 0x00010038,
};

extern int realme_protocol_init(void);
extern int realme_protocol_exit(void);

#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & DMA_EN)
extern int dma_protocol_init(void);
extern int dma_protocol_exit(void);
extern int dueros_process();
extern int dueros_send_process();

extern const u8 sdp_dueros_spp_service_data[];
extern const u8 sdp_dueros_ota_service_data[];
SDP_RECORD_REGISTER(dma_record_item) = {
    .service_record = (u8 *)sdp_dueros_spp_service_data,
    .service_record_handle = 0x00010036,
};
SDP_RECORD_REGISTER(dma_ota_record_item) = {
    .service_record = (u8 *)sdp_dueros_ota_service_data,
    .service_record_handle = 0x00010037,
};
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & ONLINE_DEBUG_EN)
extern const u8 sdp_spp_service_data[];
SDP_RECORD_REGISTER(spp_sdp_record_item) = {
    .service_record = (u8 *)sdp_spp_service_data,
    .service_record_handle = 0x00010004,
};
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & CUSTOM_DEMO_EN)

extern void custom_demo_all_init(void);
extern void custom_demo_all_exit(void);

// uuid:00001101-0000-1000-8000-00805F9B34FB
const u8 sdp_honor_spp_service_data[96] = {
    0x36, 0x00, 0x5B, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x40, 0x09, 0x00, 0x01, 0x36, 0x00,
    0x11, 0x1C, 0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B,
    0x34, 0xFB, 0x09, 0x00, 0x04, 0x36, 0x00, 0x0E, 0x36, 0x00, 0x03, 0x19, 0x01, 0x00, 0x36, 0x00,
    0x05, 0x19, 0x00, 0x03, 0x08, 0x01, 0x09, 0x00, 0x09, 0x36, 0x00, 0x17, 0x36, 0x00, 0x14, 0x1C,
    0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB,
    0x09, 0x01, 0x00, 0x09, 0x01, 0x00, 0x25, 0x06, 0x4A, 0x4C, 0x5F, 0x53, 0x50, 0x50, 0x00, 0x00,
};

SDP_RECORD_REGISTER(honor_sdp_record_item) = {
    .service_record = (u8 *)sdp_honor_spp_service_data,
    .service_record_handle = 0x00010040,
};
#endif

bool check_tws_master_role()
{
#if TCFG_USER_TWS_ENABLE
    return (tws_api_get_role() == TWS_ROLE_MASTER);
#endif
    return 1;
}

static void multi_protocol_resume(void)
{
    os_taskq_post_type("app_proto", MULTI_PROTOCOL_RX_DATA_EVENT, 0, NULL);
}
static void multi_protocol_send_resume(void)
{
    os_taskq_post_type("app_proto", MULTI_PROTOCOL_TX_DATA_EVENT, 0, NULL);
}

static void multi_protocol_tick_resume(void *ptr)
{
    multi_protocol_resume();///500ms resume for send retry
}

static void multi_protocol_loop_process(void *parm)
{
    int msg[8];
    int ret;
    //__this->tick_timer = sys_timer_add(NULL, app_protocol_tick_resume, 1000);
    printf("app_protocol_loop_process\n");
    while (1) {
        ret = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        switch (msg[0]) {
        case MULTI_PROTOCOL_RX_DATA_EVENT:
            /* printf("multi rx loop\n"); */
#if (THIRD_PARTY_PROTOCOLS_SEL & MMA_EN)
            mma_protocol_loop_process();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & DMA_EN)
            dueros_process();
#endif
            break;
        case MULTI_PROTOCOL_TX_DATA_EVENT:
            /* printf("multi tx loop\n"); */
#if (THIRD_PARTY_PROTOCOLS_SEL & MMA_EN)
            mma_protocol_loop_process();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & DMA_EN)
            dueros_send_process();
#endif
            break;
        case MULTI_PROTOCOL_TASK_EXIT:
            os_sem_post((OS_SEM *)msg[1]);
            os_time_dly(10000);
            break;
        default:
            printf("err multi protocal msg\n");
            break;
        }
    }
}

static void multi_protocol_profile_init(void)
{
    printf("multi_protocol_profile_init 0x%x %d %d\n", THIRD_PARTY_PROTOCOLS_SEL, TCFG_BT_SUPPORT_SPP, TCFG_USER_BLE_ENABLE);

    // BLE/SPP 公共的状态回调

#if TCFG_BT_SUPPORT_SPP
    app_spp_init();
    app_spp_state_update_callback_regitster(multi_protocol_state_update_callback);
#endif

#if TCFG_USER_BLE_ENABLE
    le_device_db_init();

#if (THIRD_PARTY_PROTOCOLS_SEL & FMNA_EN)
    app_ble_sm_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_BONDING, 7, 0);
#else

#if TCFG_BLE_BRIDGE_EDR_ENALBE
    app_ble_sm_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING, 7, 0);
#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
    app_ble_sm_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_BONDING | SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_MITM_PROTECTION, 7, 0);
    le_audio_sm_setup_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_BONDING | SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_MITM_PROTECTION, 7, 0);
#else
    app_ble_sm_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_MITM_PROTECTION | SM_AUTHREQ_BONDING, 7, 0);
#endif

#endif

    app_ble_init();

    app_ble_state_update_callback_regitster(multi_protocol_state_update_callback);

    ble_op_multi_att_send_init(att_ram_buffer, ATT_RAM_BUFSIZE, ATT_LOCAL_PAYLOAD_SIZE);
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SINK_EN)
    extern int auracast_delegator_init(u8 addr_type);
    auracast_delegator_init(0xfd);//BD_ADDR_TYPE_EDR_ATT
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    bt_rcsp_interface_init(rcsp_profile_data);
    rcsp_ble_profile_init();

#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
    if (get_bt_le_audio_config()) { // RCSP 与 CIS 共用 ACL
        app_ble_no_profile_flag_set(rcsp_server_ble_hdl, 1);
    }
#endif
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & GFPS_EN)
    gfps_ble_profile_init();
    gfps_is_tws_master_callback_register(check_tws_master_role);
    gfps_message_callback_register(gfps_message_deal_handler);
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & MMA_EN)
    xm_ble_profile_init();
    mma_is_tws_master_callback_register(check_tws_master_role);
    mma_message_callback_register(xm_message_deal_handler);
    //mma_get_battery_callback_register(app_protocol_get_battery);
    mma_tx_resume_register(multi_protocol_resume);
    mma_rx_resume_register(multi_protocol_resume);
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & REALME_EN)
    realme_protocol_init();
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & FMNA_EN)
    extern void ancs_ams_set_ios_pair_request_enable(u8 enable);
    ancs_ams_set_ios_pair_request_enable(0);
    extern void fmy_bt_ble_init(void);
    fmy_bt_ble_init();
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & (MMA_EN | DMA_EN))
    task_create(multi_protocol_loop_process, NULL, "app_proto");
#endif

}

void multi_protocol_bt_init(void)
{
    printf("################# multi_protocol init");

    multi_protocol_profile_init();

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    rcsp_bt_ble_init();
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & GFPS_EN)
    gfps_set_model_id((uint8_t *)google_model_id_used);
    gfps_set_anti_spoofing_public_key((char *)google_public_key_used);
    gfps_set_anti_spoofing_private_key((char *)google_private_key_used);
#if CONFIG_ANC_ENABLE
    gfps_hearable_controls_enable(1);
    gfps_hearable_controls_update(GFPS_ANC_ALL_MODE, GFPS_ANC_ALL_MODE, GFPS_OFF_MODE);
#endif
    gfps_all_init();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & MMA_EN)
    xm_all_init();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & SWIFT_PAIR_EN)
    swift_pair_all_init();
    swift_pair_enter_pair_mode();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & DMA_EN)
    dma_tx_resume_register(multi_protocol_send_resume);
    dma_rx_resume_register(multi_protocol_resume);
    dma_protocol_init();
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & ONLINE_DEBUG_EN)
    extern void online_spp_init(void);
    online_spp_init();
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & CUSTOM_DEMO_EN)
    custom_demo_all_init();
#endif
}

void multi_protocol_bt_exit(void)
{
    printf("################# multi_protocol exit");
#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    rcsp_bt_ble_exit();
    bt_rcsp_interface_exit();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & GFPS_EN)
    gfps_all_exit();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & MMA_EN)
    xm_all_exit();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & REALME_EN)
    realme_protocol_exit();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & FMNA_EN)
    extern void fmy_bt_ble_exit(void);
    fmy_bt_ble_exit();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & SWIFT_PAIR_EN)
    swift_pair_exit_pair_mode();
    swift_pair_all_exit();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & DMA_EN)
    dma_protocol_exit();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & ONLINE_DEBUG_EN)
    extern void online_spp_exit(void);
    online_spp_exit();
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & CUSTOM_DEMO_EN)
    custom_demo_all_exit();
#endif

    app_ble_exit();
    app_spp_exit();

#if (THIRD_PARTY_PROTOCOLS_SEL & (MMA_EN | DMA_EN))
    task_kill("app_proto");
#endif
}

void bt_ble_adv_enable(u8 enable)
{
#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    rcsp_bt_ble_adv_enable(enable);
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & GFPS_EN)
    gfps_bt_ble_adv_enable(enable);
#endif
}


//编译问题加几个空的
_WEAK_ void bt_ble_init(void)
{
}
_WEAK_ void bt_ble_exit(void)
{
}

#endif
