/*********************************************************************************************
    *   Filename        : cig_params.c

    *   Description     :

    *   Author          : Weixin Liang

    *   Email           : liangweixin@zh-jieli.com

    *   Last modifiled  : 2022-12-15 14:31

    *   Copyright:(c)JIELI  2011-2022  @ , All Rights Reserved.
*********************************************************************************************/
#include "app_config.h"
#include "app_main.h"
#include "audio_base.h"
#include "le_connected.h"
#include "wireless_trans.h"

#if LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN

/**************************************************************************************************
  Macros
**************************************************************************************************/

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/**************************************************************************************************
  Global Variables
**************************************************************************************************/
static cig_parameter_t cig_central_param = {
    .cb             = &cig_central_cb,
    .pair_en        = 0,
    .phy            = BIT(1),
    .aclIntMs       = 50,

    .vdr = {
        .tx_delay   = 1500,
        .cig_offset = 1500,
        .aclMaxPduCToP = 36,
        .aclMaxPduPToC = 27,
    },
};

static cig_parameter_t cig_perip_param = {
    .cb        = &cig_perip_cb,
    .pair_en        = 0,

    .vdr = {
        .tx_delay   = 1500,
        .aclMaxPduCToP = 36,
        .aclMaxPduPToC = 27,
    },
};


static cig_parameter_t *central_params = NULL;
static cig_parameter_t *perip_params = NULL;
static u16 cig_transmit_data_len = 0;
static u32 enc_output_frame_len = 0;
static u32 dec_input_buf_len = 0;
static u32 enc_output_buf_len = 0;
static u8 cur_app_task = -1;
static u8 cur_cig_role = 0;
static u8 platform_data_index = 0;
static struct connected_platform_data platform_data;
const static u8 platform_data_mapping[] = {
    APP_MODE_BT,
    APP_MODE_MUSIC,
    APP_MODE_LINEIN,
    APP_MODE_PC,
    APP_MODE_IIS,
    APP_MODE_MIC,
    APP_MODE_SPDIF,
    APP_MODE_FM,
    LE_AUDIO_1T3_MODE,
};

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*! \brief 每包编码数据长度 */
/* (int)((LE_AUDIO_CODEC_FRAME_LEN / 10) * (JLA_CODING_BIT_RATE / 1000 / 8) + 2) */
/* 如果码率超过96K,即帧长超过122,就需要将每次传输数据大小 修改为一帧编码长度 */
static u32 calcul_cig_enc_output_frame_len(u16 frame_len, u32 bit_rate)
{
    int len = 0;
#if (LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA || LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_V2)
    len = (frame_len * bit_rate / 1000 / 8 / 10 + 2);
#if JL_CC_CODED_EN
    len += (len & 1); //开fec 编码时会微调码率，使编出来一帧的数据长度是偶数,故如果计算出来是奇数需要+1;
#endif/*JL_CC_CODED_EN*/
#elif (LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_LL)
    len = jla_ll_enc_frame_len();
#elif(LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_LW)
    len = (frame_len * bit_rate / 1000 / 8 / 10);
#endif// WIERLESS_TRANS_CODING_TYPE == LIVE_AUDIO_CODING_JLA

    return len;
}

u32 get_cig_enc_output_frame_len(void)
{
    ASSERT(enc_output_frame_len, "enc_output_frame_len is 0");
    return enc_output_frame_len;
}

static u16 calcul_cig_transmit_data_len(u32 encode_output_frame_len, u16 period, u16 codec_frame_len)
{
    return (encode_output_frame_len * (period * 10 / 1000 / codec_frame_len));
}

u16 get_cig_transmit_data_len(void)
{
    ASSERT(cig_transmit_data_len, "cig_transmit_data_len is 0");
    return cig_transmit_data_len;
}

static u32 calcul_cig_enc_output_buf_len(u32 transmit_data_len)
{
    return (transmit_data_len * 2);
}

u32 get_cig_enc_output_buf_len(void)
{
    ASSERT(enc_output_buf_len, "enc_output_buf_len is 0");
    return enc_output_buf_len;
}

static u32 calcul_cig_dec_input_buf_len(u32 transmit_data_len)
{
    return (transmit_data_len * 10);
}

u32 get_cig_dec_input_buf_len(void)
{
    ASSERT(dec_input_buf_len, "dec_input_buf_len is 0");
    return dec_input_buf_len;
}

u32 get_cig_sdu_period_us(void)
{
    return platform_data.args[platform_data_index].sdu_interval;
}

u32 get_cig_mtl_time(void)
{
    int mtl = 1;
    return mtl;
}

void set_cig_hdl(u8 role, u8 cig_hdl)
{
    if ((role == CONNECTED_ROLE_CENTRAL) && central_params) {
        central_params->cig_hdl = cig_hdl;
    } else if ((role == CONNECTED_ROLE_PERIP) && perip_params) {
        perip_params->cig_hdl = cig_hdl;
    }
}

int get_cig_audio_coding_nch(void)
{
    return platform_data.nch;
}
int get_cig_audio_coding_sample_rate(void)
{
    return platform_data.sample_rate;
}

int get_cig_audio_coding_bit_rate(void)
{
    return platform_data.args[platform_data_index].bitrate;
}

int get_cig_audio_coding_frame_duration(void)
{
    return platform_data.frame_len;
}

int get_cig_tx_rtn(void)
{
    int rtn = 0;

#if (LEA_CIG_TRANS_MODE == 1)

#if (LEA_CIG_CONNECT_MODE == 2)
    rtn = platform_data.args[platform_data_index].rtnPToC;
#else
    rtn = platform_data.args[platform_data_index].rtnCToP;
#endif

#else

    if (cur_cig_role == CONNECTED_ROLE_CENTRAL) {
        rtn = platform_data.args[platform_data_index].rtnCToP;
    } else if (cur_cig_role == CONNECTED_ROLE_PERIP) {
        rtn = platform_data.args[platform_data_index].rtnPToC;
    }

#endif

    return rtn;
}

int get_cig_tx_delay(void)
{
    int tx_delay = 0;

    if ((cur_cig_role == CONNECTED_ROLE_CENTRAL) && central_params) {
        tx_delay = central_params->vdr.tx_delay;
    } else if ((cur_cig_role == CONNECTED_ROLE_PERIP) && perip_params) {
        tx_delay = perip_params->vdr.tx_delay;
    }

    return tx_delay;
}

u32 get_cig_tx_latency(void)
{
    return platform_data.args[platform_data_index].tx_latency;
}

u32 get_cig_play_latency(void)
{
    return platform_data.args[platform_data_index].play_latency;
}

u8 get_cis_num(u8 role)
{
    u8 num = 0;
    if ((role == CONNECTED_ROLE_CENTRAL) && central_params) {
        num = central_params->num_cis;
    } else if ((role == CONNECTED_ROLE_PERIP) && perip_params) {
        num = 1;
    }
    return num;
}

static const struct connected_platform_data *get_connected_platform_data(u8 mode)
{
    u8 find = 0;
    int len = syscfg_read(CFG_CIG_PARAMS, platform_data.args, sizeof(platform_data.args));
    if (len <= 0) {
        r_printf("ERR:Can not read the connected config\n");
        return NULL;
    }

    for (platform_data_index = 0; platform_data_index < ARRAY_SIZE(platform_data_mapping); platform_data_index++) {
        if (mode == platform_data_mapping[platform_data_index]) {
            find = 1;
            break;
        }
    }

    if (!find) {
        r_printf("ERR:Can not find the connected config\n");
        return NULL;
    }

    platform_data.nch = LE_AUDIO_CODEC_CHANNEL;
    platform_data.frame_len = LE_AUDIO_CODEC_FRAME_LEN;
    platform_data.sample_rate = LE_AUDIO_CODEC_SAMPLERATE;
    platform_data.coding_type = LE_AUDIO_CODEC_TYPE;

    /* put_buf((const u8 *)&(platform_data.args[platform_data_index]), sizeof(struct connected_cfg_args)); */
    g_printf("sdu_interval:%d", platform_data.args[platform_data_index].sdu_interval);
    g_printf("tx_latency:%d\n", platform_data.args[platform_data_index].tx_latency);
    g_printf("play_latency:%d\n", platform_data.args[platform_data_index].play_latency);
    g_printf("rtnCToP:%d", platform_data.args[platform_data_index].rtnCToP);
    g_printf("rtnPToC:%d", platform_data.args[platform_data_index].rtnPToC);
    g_printf("mtlCToP:%d", platform_data.args[platform_data_index].mtlCToP);
    g_printf("mtlPToC:%d", platform_data.args[platform_data_index].mtlPToC);
    g_printf("bitrate:%d", platform_data.args[platform_data_index].bitrate);
    g_printf("nch:%d", platform_data.nch);
    g_printf("frame_len:%d", platform_data.frame_len);
    g_printf("sample_rate:%d", platform_data.sample_rate);
    g_printf("coding_type:0x%x", platform_data.coding_type);

    return &platform_data;
}

cig_parameter_t *set_cig_params(u8 app_task, u8 role, u8 pair_without_addr)
{
    int ret;
    u64 pair_addr;
    cur_app_task = app_task;
    cur_cig_role = role;

#if TCFG_KBOX_1T3_MODE_EN
    app_task = LE_AUDIO_1T3_MODE;
#endif

    const struct connected_platform_data *data = get_connected_platform_data(app_task);

    if (role == CONNECTED_ROLE_CENTRAL) {
        central_params = &cig_central_param;
        memcpy(central_params->pair_name, get_le_audio_pair_name(), sizeof(central_params->pair_name));
#if (LEA_TX_CHANNEL_SEPARATION && (LE_AUDIO_CODEC_CHANNEL == 2))
        data->nch = 1;
        data->args[platform_data_index].bit_rate /= 2;
#endif
        enc_output_frame_len = calcul_cig_enc_output_frame_len(data->frame_len, data->args[platform_data_index].bitrate);
        cig_transmit_data_len = calcul_cig_transmit_data_len(enc_output_frame_len, data->args[platform_data_index].sdu_interval, data->frame_len);
        dec_input_buf_len = calcul_cig_dec_input_buf_len(cig_transmit_data_len);
        enc_output_buf_len = calcul_cig_enc_output_buf_len(cig_transmit_data_len);
        if (central_params) {

#if (LEA_CIG_CONNECT_MODE == 1)
            central_params->num_cis = 1;
#else
            central_params->num_cis = 2;
#endif

#if (LEA_CIG_TRANS_MODE == 1)
#if (LEA_CIG_CONNECT_MODE == 2)
            central_params->mtlPToC = data->args[platform_data_index].sdu_interval * data->args[platform_data_index].mtlPToC / 1000;
            central_params->sduIntUsPToC = data->args[platform_data_index].sdu_interval;
            central_params->cis[0].rtnPToC = data->args[platform_data_index].rtnPToC - 1;
            central_params->cis[1].rtnPToC = data->args[platform_data_index].rtnPToC - 1;
            central_params->cis[0].maxSduPToC = cig_transmit_data_len;
            central_params->cis[1].maxSduPToC = cig_transmit_data_len;
#else
            central_params->mtlCToP = data->args[platform_data_index].sdu_interval * data->args[platform_data_index].mtlCToP / 1000;
            central_params->sduIntUsCToP = data->args[platform_data_index].sdu_interval;
            central_params->cis[0].rtnCToP = data->args[platform_data_index].rtnCToP - 1;
            central_params->cis[1].rtnCToP = data->args[platform_data_index].rtnCToP - 1;
            central_params->cis[0].maxSduCToP = cig_transmit_data_len;
            central_params->cis[1].maxSduCToP = cig_transmit_data_len;
#endif
#endif  //#if (LEA_CIG_TRANS_MODE == 1)

#if (LEA_CIG_TRANS_MODE == 2)
            central_params->mtlCToP = data->args[platform_data_index].sdu_interval * data->args[platform_data_index].mtlCToP / 1000;
            central_params->mtlPToC = data->args[platform_data_index].sdu_interval * data->args[platform_data_index].mtlPToC / 1000;
            central_params->sduIntUsCToP = data->args[platform_data_index].sdu_interval;
            central_params->sduIntUsPToC = data->args[platform_data_index].sdu_interval;
#if (LEA_TX_CHANNEL_SEPARATION && (LE_AUDIO_CODEC_CHANNEL == 2))
            u8 packet_num;
            u16 single_ch_trans_data_len;
            packet_num = cig_transmit_data_len / enc_output_frame_len;
            single_ch_trans_data_len = cig_transmit_data_len / 2 + packet_num;
            central_params->cis[0].rtnCToP = data->args[platform_data_index].rtnCToP - 1;
            central_params->cis[0].rtnPToC = data->args[platform_data_index].rtnPToC - 1;
            central_params->cis[1].rtnCToP = data->args[platform_data_index].rtnCToP - 1;
            central_params->cis[1].rtnPToC = data->args[platform_data_index].rtnPToC - 1;
            central_params->cis[0].maxSduCToP = single_ch_trans_data_len;
            central_params->cis[0].maxSduPToC = single_ch_trans_data_len;
            central_params->cis[1].maxSduCToP = single_ch_trans_data_len;
            central_params->cis[1].maxSduPToC = single_ch_trans_data_len;
#else
            central_params->cis[0].rtnCToP = data->args[platform_data_index].rtnCToP - 1;
            central_params->cis[0].rtnPToC = data->args[platform_data_index].rtnPToC - 1;
            central_params->cis[1].rtnCToP = data->args[platform_data_index].rtnCToP - 1;
            central_params->cis[1].rtnPToC = data->args[platform_data_index].rtnPToC - 1;
            central_params->cis[0].maxSduCToP = cig_transmit_data_len;
            central_params->cis[0].maxSduPToC = cig_transmit_data_len;
            central_params->cis[1].maxSduCToP = cig_transmit_data_len;
            central_params->cis[1].maxSduPToC = cig_transmit_data_len;
#endif
#endif  //#if (LEA_CIG_TRANS_MODE == 2)

        }

        if (pair_without_addr) {
            central_params->cis[0].pri_ch = 0;
            central_params->cis[1].pri_ch = 0;
        } else {
            ret = syscfg_read(VM_WIRELESS_PAIR_CODE0, &pair_addr, sizeof(pair_addr));
            if (ret <= 0) {
                pair_addr = 0;
            }
            central_params->cis[0].pri_ch = pair_addr;
            g_printf("cis0.pri_ch:");
            put_buf((u8 *)&central_params->cis[0].pri_ch, sizeof(central_params->cis[0].pri_ch));
            ret = syscfg_read(VM_WIRELESS_PAIR_CODE1, &pair_addr, sizeof(pair_addr));
            if (ret <= 0) {
                pair_addr = 0;
            }
            central_params->cis[1].pri_ch = pair_addr;
            g_printf("cis1.pri_ch:");
            put_buf((u8 *)&central_params->cis[1].pri_ch, sizeof(central_params->cis[1].pri_ch));
        }
        return central_params;
    }

    if (role == CONNECTED_ROLE_PERIP) {
        perip_params = &cig_perip_param;
        memcpy(perip_params->pair_name, get_le_audio_pair_name(), sizeof(perip_params->pair_name));
        enc_output_frame_len = calcul_cig_enc_output_frame_len(data->frame_len, data->args[platform_data_index].bitrate);
        cig_transmit_data_len = calcul_cig_transmit_data_len(enc_output_frame_len, data->args[platform_data_index].sdu_interval, data->frame_len);
        dec_input_buf_len = calcul_cig_dec_input_buf_len(cig_transmit_data_len);
        if (pair_without_addr) {
            perip_params->perip.pri_ch = 0;
        } else {
            ret = syscfg_read(VM_WIRELESS_PAIR_CODE0, &pair_addr, sizeof(pair_addr));
            if (ret <= 0) {
                pair_addr = 0;
            }
            perip_params->perip.pri_ch = pair_addr;
            g_printf("perip.pri_ch:");
            put_buf((u8 *)&perip_params->perip.pri_ch, sizeof(perip_params->perip.pri_ch));
        }
        return perip_params;
    }

    return NULL;
}

void reset_cig_params(void)
{
    cur_app_task = -1;
    cur_cig_role = 0;
    central_params = 0;
    perip_params = 0;
}

#endif

