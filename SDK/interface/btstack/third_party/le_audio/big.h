#ifndef __BIG_H__
#define __BIG_H__

#include "typedef.h"

/* Max BIG numbers. */
#define BIG_MAX_NUMS            1
/* Max BIS numbers in a BIG. */
#define BIG_MAX_BIS_NUMS        2
// TODO:
#define BIG_MAX_RX_BROADCAST_NUMS        2
#define BIG_MAX_TX_BROADCAST_NUMS        1

/* BIG operation status. */
typedef enum {
    BIG_OPST_SUCC,                              // Operation is successful.
    BIG_OPST_MEMORY_EXCEEDED,                   // Memory capacity exceeded.
    BIG_OPST_DISALLOWED,                        // Operation is disallowed.
    BIG_OPST_UNKNOWN_IDENTIFIER,                // Unknown link idenitfier.
    BIG_OPST_ALREADY_EXISTS,                    // Link already exists.
    BIG_OPST_LIMIT_EXCEEDED,                    // Links limit exceeded.
} BIG_OPST;

/* BIG events. */
typedef enum {
    // for tramsmitter
    BIG_EVENT_TRANSMITTER_CONNECT,              // Transmitter connect succ.
    BIG_EVENT_TRANSMITTER_DISCONNECT,           // Transmitter disconnect.
    BIG_EVENT_TRANSMITTER_ALIGN,                // Transmitter send align.
    BIG_EVENT_TRANSMITTER_READ_TX_SYNC,         // Transmitter get last tx clk.
    BIG_EVENT_TRANSMITTER_SYNC_DELAY,           // Transmitter sync delay.
    BIG_EVENT_TRANSMITTER_HB_SYNC,              // Transmitter HB sync.
    // for receiver
    BIG_EVENT_RECEIVER_CONNECT,                 // Receiver connect succ.
    BIG_EVENT_RECEIVER_DISCONNECT,              // Receiver disconnect.
    BIG_EVENT_PERIODIC_DISCONNECT,              // Periodic disconnect.

    // for app_user
    BIG_EVENT_CUSTOM_DATA_SYNC,
} BIG_EVENT;

/* BIG pair events. */
typedef enum {
    PAIR_EVENT_TX_PRI_CHANNEL_CREATE_SUCCESS,    // TX 生成连接通道
    PAIR_EVENT_TX_OPEN_PAIR_MODE_SUCCESS,        // TX 打开配对模式
    PAIR_EVENT_TX_CLOSE_PAIR_MODE_SUCCESS,       // TX 关闭配对模式
    PAIR_EVENT_RX_OPEN_PAIR_MODE_SUCCESS,        // RX 打开配对模式
    PAIR_EVENT_RX_CLOSE_PAIR_MODE_SUCCESS,       // RX 关闭配对模式
    PAIR_EVENT_RX_PRI_CHANNEL_CREATE_SUCCESS,    // RX 生成连接通道
} PAIR_EVENT;

typedef struct {
    uint8_t         format;
    uint8_t         sr;
    uint16_t        max_sdu;        // Max sdu packet size           (uints:octets).
    uint32_t        sdu_int_us;     // Sdu interval                  (uints:us).
} audio_param_t;

/* BIG handles. */
typedef struct {
    uint8_t         big_hdl;                    // BIG handle.
    uint8_t         big_custom_mode;            // 给上层识别正常模式和产测模式
    uint16_t        bis_hdl[BIG_MAX_BIS_NUMS];  // Max BIS handles in BIG.
    uint16_t        aux_hdl;                    // Auxiliary handle.
    uint32_t        big_sync_delay;             // BIG Sync Delay
    audio_param_t   enc;
} big_hdl_t;

/* BIG ISO stream parameter. */
typedef struct {
    uint32_t        ts;                         // TimeStamp.
    uint16_t        bis_hdl;                    // BIS handle.
    uint16_t        aux_hdl;                    // Auxiliary handle.
    //0b00 Valid data. The complete ISO_SDU was received correctly.
    //0b01 Possibly invalid data. The contents of the ISO_SDU may contain errors or
    //  part of the ISO_SDU may be missing. This is reported as "data with possible
    //  errors".
    //0b10 Part(s) of the ISO_SDU were not received correctly. This is reported as
    //  "lost data".
    uint8_t         packet_status_flag;
} big_stream_param_t;

/* BIG read TX sync info. */
typedef struct {
    uint16_t        bis_hdl;                    // BIS handle.
    uint32_t        tx_ts;                      // Last TX TimeStamp.
} bis_txsync_t;

/* BIG device callback. */
typedef struct {

    /* data callback */
    void (*receive_packet_cb)(const void *const buf, size_t length, void *priv);

    /* padv_data callback */
    int (*receive_padv_data_cb)(const void *const buf, size_t length, uint8_t big_hdl);

    /* adv_data callback */
    void (*receive_adv_data_cb)(const void *const buf, size_t length, uint8_t big_hdl);

    /* event callback */
    void (*event_cb)(const BIG_EVENT event, void *priv);

} big_callback_t;

typedef struct {
    /* pair event callback */
    void (*pair_event_cb)(const PAIR_EVENT event, void *priv);
} pair_callback_t;

/* BIG device parameter. */
typedef struct {
    uint8_t               pair_name[28];        // Pair name.
    const big_callback_t  *cb;                  // Callback of BIG.
    uint8_t               big_hdl;              // BIG handle.
    uint8_t               num_bis;              // Total number of BISes in the BIG.
    uint8_t               ext_phy;              // Primary   adv phy, 1:1M, 3:Coded.
    uint8_t               form;                 // APP form.
    uint8_t               enc;                  // 0:Unencrypted,1:Encrypted.
    uint8_t               bc[16];               // Broadcast Code, encrypt BIS payloads.
    uint32_t              pri_ch;               // Privacy connect channel.
    uint16_t              scan_int;             // Scan interval           (uints:slot)
    uint16_t              scan_win;             // Scan window             (uints:slot)
    uint32_t              scan_offset;          // Scan offset             (uints:us)
    uint16_t 		      hb_rx_timeout;        // hb_rx_timeout			 (uints:10ms).
    uint8_t               big_custom_mode;      // 给上层识别正常模式和产测模式

    union {
        struct {
            uint8_t  rtn;                   // The number of retransmitted times.
            uint8_t  phy;                   // BIS phy, BIT(0):1M, BIT(1):2M, BIT(2):Coded.
            uint8_t  aux_phy;               // Secondary adv phy, 1:1M, 2:2M, 3:Coded.
            uint8_t  packing;               // 0:Sequential, 1:Interleaved.
            uint8_t  framing;               // 0:Unframed,   1:Framed.
            uint16_t max_sdu;               // Max sdu packet size           (uints:octets).
            uint16_t eadv_int_slot;         // Primary/Secondary adv interval(uints:slot).
            uint16_t padv_int_slot;         // Periodic adv interval         (uints:slot).
            uint32_t mtl;                   // Max transport latency         (uints:us).
            uint32_t sdu_int_us;            // Sdu interval                  (uints:us).
            // TODO:
            uint8_t  num_br;                //broadcast number
            uint8_t  broadcast_name[BIG_MAX_TX_BROADCAST_NUMS][28];     // Broadcast name.
            uint8_t  enc[BIG_MAX_TX_BROADCAST_NUMS];                    // 0:Unencrypted,1:Encrypted.
            uint8_t  bc[BIG_MAX_TX_BROADCAST_NUMS][16];                 // Broadcast Code, encrypt BIS payloads.

            struct {
                uint32_t sync_offset;       // Sync offset                   (uints:us).
                uint32_t tx_delay;          // Tx delay                      (uints:us).
                uint32_t big_offset;        // Big offset                    (uints:us).
                uint32_t aux_offset;        // Aux offset                    (uints:us).
                uint8_t  adv_cnt;           // ADV counter
                uint8_t  max_pdu;           // Max pdu packet size           (uints:octets).
            } vdr;
        } tx;

        struct {
            uint8_t  bis[BIG_MAX_BIS_NUMS]; // Index of a BIS               (range:1~0x1f)
            uint16_t ext_scan_int;          // Primary scan interval        (uints:slot)
            uint16_t ext_scan_win;          // Primary scan window          (uints:slot)
            uint32_t bsync_to_ms;           // Sync timeout for the BIG     (uints:ms).
            uint32_t psync_to_ms;           // Sync timeout for the Period  (uints:ms).
            bool     psync_keep;            // Keep Period sync.
            // TODO:
            uint8_t  num_br;  //broadcast number
            uint8_t  broadcast_name[BIG_MAX_RX_BROADCAST_NUMS][28];  // Broadcast name.
            uint8_t  enc[BIG_MAX_RX_BROADCAST_NUMS];            // 0:Unencrypted,1:Encrypted.
            uint8_t  bc[BIG_MAX_RX_BROADCAST_NUMS][16];         // Broadcast Code, encrypt BIS payloads.
        } rx;

        struct {
            uint32_t        sync_to_ms;     // Sync timeout                  (uints:ms).
            audio_param_t   enc;
            uint8_t         rtn;            // The number of retransmitted times.
            uint8_t         nInterval;
            uint16_t        pairSwMs;       // Pair switch time               (uints:ms).
        } bst;
    };
} big_parameter_t;

/* BIG device ops. */
typedef struct {

    /* BIG init. */
    int (*init)(void);

    /* BIG uninit. */
    int (*uninit)(void);

    /* BIG open. */
    int (*open)(big_parameter_t *param);

    /* BIG close. */
    int (*close)(uint8_t big_hdl);

    /* Set btctrler's sync clock enable. */
    void (*sync_set)(uint16_t bis_hdl, bool sel);

    /* Get current packet's clock. */
    void (*sync_get)(uint16_t bis_hdl, uint16_t *us_1per12, uint32_t *clk_us, uint32_t *evt_cnt);

    /* For transmitter, send SDU packet. */
    int (*send_packet)(const uint8_t *packet, size_t size, big_stream_param_t *param);

    /* Get current btctrler's clock. */
    void (*get_ble_clk)(uint32_t *clk);

    /* For transmitter, get bis last sent packet's clock. */
    int (*get_tx_sync)(void *priv);

    /* BIS mode. */
    /* For transmitter, set padv data. */
    void (*padv_set_data)(void *p, uint8_t *data, size_t size);

    /* Enter pair. */
    void (*enter_pair)(uint8_t pair_mode, pair_callback_t *cb, uint32_t user_pair_code);

    /* Exit pair. */
    void (*exit_pair)(uint8_t big_hdl);

    /* Get current remote dev rssi. */
    s8(*get_rssi)(uint16_t hdl);

    /* BST mode. */
    /* Get pair code. */
    void (*get_pair_code)(uint8_t *pair_code, uint8_t privacy);
    void (*set_pair_code)(uint8_t *pair_code);
} big_ops_t;

/* Big api ops for upper layer. */
extern const big_ops_t big_tx_ops;
extern const big_ops_t big_rx_ops;

extern const big_ops_t *big_rx_ops_get(void);
extern const big_ops_t *big_tx_ops_get(void);

extern void big_receive_iso_cbk_register(void (*cbk)(const void *const buf, size_t length, void *priv));

#endif /* __BIG_H__ */

