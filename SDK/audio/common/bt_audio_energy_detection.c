#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bt_audio_energy_detection.data.bss")
#pragma data_seg(".bt_audio_energy_detection.data")
#pragma const_seg(".bt_audio_energy_detection.text.const")
#pragma code_seg(".bt_audio_energy_detection.text")
#endif
#include "system/includes.h"
#include "btstack/a2dp_media_codec.h"
#include "bt_audio_energy_detection.h"
#include "app_config.h"
#include "tech_lib/bt_audio_energy_det_api.h"

/*data[0]:0x9c data[1]和data[2]: 用于获取编码信息和计算帧长 */
#define SBC_CHECK_FRAME_INFO_DATA_LEN  3

int sbc_energy_detect(u8 *packet, u16 size)
{
    int length;
    int frame_len;
    int ret;
    u8 *beg;

    length = a2dp_media_get_rtp_header_len(A2DP_CODEC_SBC, packet, size);
    beg = packet + length;
    if ((*beg != 0x9c) || ((size - length) < SBC_CHECK_FRAME_INFO_DATA_LEN)) {
        return 0;
    }
    //printf("<bitpool:%d>",beg[2]);
    frame_len = sbc_frame_length(beg[1], beg[2]);
    if (beg + frame_len > packet + size) {
        return 0;
    }

    ret = sbc_cal_energy(beg, frame_len);
    while (ret < 0) {
        beg += frame_len;
        if (beg + frame_len > packet + size) {
            return 0;
        }
        ret = sbc_cal_energy(beg, frame_len);
    }
    /* printf("sbc --- energy = %d\n",ret); */
    return ret;
}

static int aac_energy_detect(u8 *packet, u16 size)
{
    int rtp_length;
    int frame_len;
    u32 ret = 0;
    int err;
    u8 *beg;

    rtp_length = a2dp_media_get_rtp_header_len(A2DP_CODEC_MPEG24, packet, size);
    beg = packet + rtp_length;
    frame_len = size - rtp_length;

    err = aac_decoder_energy_detect_open(beg, frame_len);
    if (err) {
        return 0;
    }
    err = aac_decoder_energy_detect_run(beg, frame_len, &ret);
    if (err) {
        return 0;
    }
    /* printf("aac --- energy = %d\n",ret); */
    return ret;
}

static int ldac_energy_detect(u8 *packet, u16 size)
{
    int rtp_length;
    int frame_len;
    u32 ret = 0;
    int err;
    u8 *beg;

    rtp_length = a2dp_media_get_rtp_header_len(A2DP_CODEC_LDAC, packet, size);
    beg = packet + rtp_length;
    frame_len = size - rtp_length;
    err = ldac_decoder_energy_detect_open(beg, frame_len);
    if (err) {
        return 0;
    }
    err = ldac_decoder_energy_detect_run(beg, frame_len, &ret);
    //printf("ldac --- energy = %d\n",ret);
    if (err) {
        return 0;
    }
    return ret;

}

int bt_audio_energy_detect_run(u8 codec_type, void *packet, u16 frame_len)
{
#ifdef CONFIG_FPGA_ENABLE
    return 0;
#else
    if (codec_type == A2DP_CODEC_SBC) {
#if TCFG_A2DP_SBC_SILENCE_DETECT_ENABLE
        return sbc_energy_detect(packet, frame_len);
#endif

    } else if (codec_type == A2DP_CODEC_MPEG24) {
#if TCFG_A2DP_AAC_SILENCE_DETECT_ENABLE
        return aac_energy_detect(packet, frame_len);
#endif
    } else if (codec_type == A2DP_CODEC_LDAC) {
#if TCFG_A2DP_LDAC_SILENCE_DETECT_ENABLE
        return ldac_energy_detect(packet, frame_len);
#endif
    } else {
        //other format todo
    }
    return 0;
#endif
}

void bt_audio_energy_detect_close(u8 codec_type)
{
#ifdef CONFIG_FPGA_ENABLE
    return ;
#else
    if (codec_type == A2DP_CODEC_MPEG24) {
#if TCFG_A2DP_AAC_SILENCE_DETECT_ENABLE
        aac_decoder_energy_detect_close();
#endif
    } else if (codec_type == A2DP_CODEC_LDAC) {
#if TCFG_A2DP_LDAC_SILENCE_DETECT_ENABLE
        ldac_decoder_energy_detect_close();
#endif
    }
#endif
}



