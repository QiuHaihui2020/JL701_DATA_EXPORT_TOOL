
#ifndef BT_AUDIO_ENERGY_DET_API_H
#define BT_AUDIO_ENERGY_DET_API_H


int aac_decoder_energy_detect_open(void *packet, u16 frame_len);
int aac_decoder_energy_detect_run(void *packet, u16 frame_len, u32 *energy);
void aac_decoder_energy_detect_close();
int ldac_decoder_energy_detect_open(void *packet, u16 frame_len);
int ldac_decoder_energy_detect_run(void *packet, u16 frame_len, u32 *energy);
void ldac_decoder_energy_detect_close();

unsigned int sbc_cal_energy(u8 *codedata, int len);

#endif


