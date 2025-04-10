#ifndef __CIG_H__
#define __CIG_H__

#include "typedef.h"

/* Max CIG numbers. */
#define CIG_MAX_NUMS            1
/* Max CIS numbers in a CIG. */
#define CIG_MAX_CIS_NUMS        2

/* CIG operation status. */
typedef enum {
    CIG_OPST_SUCC,                              // Operation is successful.
    CIG_OPST_MEMORY_EXCEEDED,                   // Memory capacity exceeded.
    CIG_OPST_DISALLOWED,                        // Operation is disallowed.
    CIG_OPST_UNKNOWN_IDENTIFIER,                // Unknown link idenitfier.
    CIG_OPST_ALREADY_EXISTS,                    // Link already exists.
    CIG_OPST_LIMIT_EXCEEDED,                    // Links limit exceeded.
} CIG_OPST;

/* CIG events. */
typedef enum {
    // for CIS
    CIG_EVENT_CIS_CONNECT,
    CIG_EVENT_CIS_DISCONNECT,
    CIG_EVENT_CENTRAL_CONNECT,                  // Transmitter connect succ.
    CIG_EVENT_CENTRAL_DISCONNECT,               // Transmitter disconnect.
    CIG_EVENT_PERIP_CONNECT,                    // Receiver connect succ.
    CIG_EVENT_PERIP_DISCONNECT,                 // Receiver disconnect.
    CIG_EVENT_TRANSMITTER_ALIGN,                // Transmitter send align.
    CIG_EVENT_TRANSMITTER_READ_TX_SYNC,         // Transmitter get last tx clk.
    CIG_EVENT_TRANSMITTER_SYNC_DELAY,           // Transmitter sync delay.
    // for ACL
    CIG_EVENT_ACL_CONNECT,
    CIG_EVENT_ACL_DISCONNECT,

    //for phone
    CIG_EVENT_PHONE_CONNECT,
    CIG_EVENT_PHONE_DISCONNECT,
} CIG_EVENT;

/* CIG handles. */
typedef struct {
    uint8_t         cig_hdl;                    // CIG handle.
    uint16_t        cis_hdl;                    // Current CIS handle in CIG.
    uint16_t        acl_hdl;                    // ACL handle.
    uint32_t        cig_sync_delay;             // CIG Sync Delay
    uint32_t        cis_sync_delay;             // CIS Sync Delay
    uint16_t        Max_PDU_C_To_P;
    uint16_t        Max_PDU_P_To_C;
    uint8_t         flush_timeout_C_to_P;
    uint8_t         flush_timeout_P_to_C;
    uint16_t        reserved;
    uint32_t        isoIntervalUs;
} cig_hdl_t;

/* CIG ISO stream parameter. */
typedef struct {
    uint32_t        ts;                         // TimeStamp.
    uint16_t        cis_hdl;                    // CIS handle.
    uint16_t        acl_hdl;                    // ACL handle.
    //0b00 Valid data. The complete ISO_SDU was received correctly.
    //0b01 Possibly invalid data. The contents of the ISO_SDU may contain errors or
    //  part of the ISO_SDU may be missing. This is reported as "data with possible
    //  errors".
    //0b10 Part(s) of the ISO_SDU were not received correctly. This is reported as
    //  "lost data".
    uint8_t         packet_status_flag;
} cig_stream_param_t;

/* ACL info. */
typedef struct {
    uint16_t        acl_hdl;                    // ACL handle.
    uint64_t        pri_ch;                     // Privacy connect channel.
    uint8_t			conn_type;					//testbox or ordinary dev
} cis_acl_info_t;

/* CIG read TX sync info. */
typedef struct {
    uint16_t        cis_hdl;                    // CIS handle.
    uint32_t        tx_ts;                      // Last TX TimeStamp.
} cis_txsync_t;

/* CIG device callback. */
typedef struct {

    /* data callback */
    void (*receive_packet_cb)(const void *const buf, size_t length, void *priv);

    /* event callback */
    void (*event_cb)(const CIG_EVENT event, void *priv);

} cig_callback_t;

/* CIG device parameter. */
typedef struct {
    uint8_t               pair_name[28];  // Pair name.
    const cig_callback_t  *cb;            // Callback of CIG.
    bool                  pair_en;        // Enable pair mode.
    uint8_t               cig_hdl;        // CIG handle.

    uint32_t             pair_public_code;  //公共配对码
    uint32_t             pair_bind_code;    //要绑定的私有配对码
    uint16_t             pair_sw_time;      //公共配对码和私有配对码scan切换周期
    s8                   pair_rssi;         //配对功能允许的有效rssi范围

    union {
        // for central
        struct {
            uint8_t         num_cis;        // Total number of CISes in the CIG.
            uint8_t         phy;            // CIS phy, BIT(0):1M, BIT(1):2M, BIT(2):Coded.
            uint32_t        sduIntUsCToP;   // Sdu interval Central to Peripheral   (uints:us).
            uint32_t        sduIntUsPToC;   // Sdu interval Peripheral to Central   (uints:us).
            uint16_t        mtlCToP;        // Max transport latency C to P         (uints:ms).
            uint16_t        mtlPToC;        // Max transport latency P to C         (uints:ms).
            uint16_t        scan_int;       // Scan interval                        (uints:slot)
            uint16_t        scan_win;       // Scan window                          (uints:slot)
            uint16_t        aclIntMs;       // ACL connect interval                 (uints:ms).
            uint16_t        aclToMs;        // Timeout for the ACL                  (uints:ms).

            struct {
                uint8_t     rtnCToP;        // Retransmitted times from Central to Peripheral.
                uint8_t     rtnPToC;        // Retransmitted times from Peripheral to Central.
                uint16_t    maxSduCToP;     // Max sdu Central to Peripheral    (uints:octets).
                uint16_t    maxSduPToC;     // Max sdu Peripheral to Central    (uints:octets).
                uint64_t    pri_ch;         // Privacy connect channel.
            } cis[CIG_MAX_CIS_NUMS];
        };

        // for peripheral
        struct {
            uint64_t        pri_ch;         // Privacy connect channel.
        } perip;
    };

    struct {
        uint32_t    tx_delay;       // Tx delay                      (uints:us).
        uint32_t    cig_offset;     // CIG offset                    (uints:us).
        uint8_t     max_pdu;        // Max pdu packet size           (uints:octets).
        uint8_t     aclMaxPduCToP;  // Central ACL max transmit PDU.
        uint8_t     aclMaxPduPToC;  // Peripheral ACL max transmit PDU.
    } vdr;
} cig_parameter_t;

/* CIG device ops. */
typedef struct {

    /* CIG init. */
    int (*init)(void);

    /* CIG uninit. */
    int (*uninit)(void);

    /* CIG open. */
    int (*open)(cig_parameter_t *param);

    /* CIG close. */
    int (*close)(uint8_t cig_hdl);

    /* Set btctrler's sync clock enable. */
    void (*sync_set)(uint16_t cis_hdl, bool sel);

    /* Get current packet's clock. */
    void (*sync_get)(uint16_t cis_hdl, uint16_t *us_1per12, uint32_t *clk_us, uint32_t *evt_cnt);

    /* For transmitter, send SDU packet. */
    int (*send_packet)(const uint8_t *packet, size_t size, cig_stream_param_t *param);

    /* Get current btctrler's clock. */
    void (*get_ble_clk)(uint32_t *clk);

    /* For transmitter, get CIS last sent packet's clock. */
    int (*get_tx_sync)(void *priv);

} cig_ops_t;

/* CIG api ops for upper layer. */
extern const cig_ops_t cig_central_ops;
extern const cig_ops_t cig_perip_ops;

extern const cig_ops_t *cig_central_ops_get(void);
extern const cig_ops_t *cig_perip_ops_get(void);

extern void cig_perip_receive_iso_cbk_register(void (*cbk)(const void *const buf, size_t length, void *priv));
extern void cig_central_receive_iso_cbk_register(void (*cbk)(const void *const buf, size_t length, void *priv)) ;


#endif /* __CIG_H__ */
