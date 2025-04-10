#ifndef A2DP_MEDIA_CODEC_H
#define A2DP_MEDIA_CODEC_H


#include "generic/typedef.h"
#include "classic/hci_lmp.h"

#ifndef A2DP_CODEC_SBC
#define A2DP_CODEC_SBC			0x00
#define A2DP_CODEC_MPEG12		0x01
#define A2DP_CODEC_MPEG24		0x02
#define A2DP_CODEC_ATRAC          0x03
#define A2DP_CODEC_NON_A2DP	      0xFF

#define A2DP_CODEC_LDAC			0xB
#define A2DP_CODEC_LHDC			0xE
#define A2DP_CODEC_LHDC_V5		0xC
#endif

#define seqn_after(a, b)        ((s16)((s16)(b) - (s16)(a)) < 0)
#define seqn_before(a, b)       seqn_after(b, a)


void *a2dp_open_media_file(u8 *addr);

void a2dp_media_close(u8 *addr);

void a2dp_close_media_file(void *file);

void a2dp_media_start_play(void *_file);

void a2dp_media_stop_play(void *_file);

int a2dp_media_set_rx_notify(void *_file, void *priv, void (*notify)(void *));

extern int a2dp_media_get_packet(void *_file, struct a2dp_media_frame *frame);

extern int a2dp_media_try_get_packet(void *_file, struct a2dp_media_frame *frame);

extern void a2dp_media_put_packet(void *_file, void *packet);

extern int a2dp_media_get_remain_buffer_size();

extern int a2dp_media_get_remain_play_time(void *_file, u8 include_tws);

extern int a2dp_media_get_total_data_len(void *_file);

extern int a2dp_media_get_packet_num(void *_file);

extern int a2dp_media_clear_packet_before_seqn(void *_file, u16 seqn_number);

extern void *a2dp_media_fetch_packet(void *_file, int *len, void *prev_packet);

extern void *a2dp_media_fetch_packet_and_wait(void *_file, int *len, void *prev_packet, int msec);

extern void a2dp_media_free_packet(void *_file, void *_packet);

extern int a2dp_media_channel_exist(void *_file);

extern int a2dp_media_is_clearing_frame(void *_file);

extern int a2dp_media_get_codec_type(void *_file);
extern int a2dp_media_get_sample_rate(void *_file);
extern int a2dp_media_get_bit_wide(void *_file);
extern int a2dp_media_get_codec_version(void *_file);

extern int a2dp_media_get_rtp_header_len(u8 coding_type, u8 *buf, int len);

extern int sbc_frame_length(u8 data, u8 bitpool);

extern void a2dp_media_mute(u8 *addr);
extern bool a2dp_media_unmute(u8 *addr);
extern bool a2dp_media_is_mute(u8 *addr);

extern int aac_energy_check(u8 *packet, u16 size);
extern void aac_decoder_energy_det_close();

void a2dp_media_update_delay_report_time(void *_file, u16 time);











#endif
