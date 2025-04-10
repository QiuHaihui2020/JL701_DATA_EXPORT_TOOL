#ifndef _DMA_PLATFORM_API_H_
#define _DMA_PLATFORM_API_H_

#include "system/includes.h"
#include "dma/dma_api.h"

// app api
extern bool dma_get_battery(u8 type, u8 *value);
extern int dma_app_speech_start(void);
extern void dma_app_speech_stop(void);
extern int dma_protocol_init(void);
extern int dma_protocol_exit(void);

// lib api
extern int dueros_process();
extern int dma_all_init(void);
extern int dma_all_exit(void);
extern int dma_all_disconnecd(void);
extern void dma_message_callback_register(int (*handler)(int id, int opcode, u8 *data, u32 len));
extern void dma_check_status_callback_register(int (*handler)(int state_flag));
extern void dma_tx_resume_register(void (*handler)(void));
extern void dma_rx_resume_register(void (*handler)(void));
extern int dma_ble_adv_enable(u8 enable);
extern int dma_speech_data_send(u8 *buf, u16 len);
extern int dueros_send_process(void);
extern void dma_set_product_id_key(void *data);
extern int dma_pair_state();
extern int dma_start_voice_recognition(int en);
extern void dueros_dma_manufacturer_info_init();
extern int dma_disconnect(void *addr);
extern int dma_update_tws_state_to_lib(int state);
extern void dma_get_battery_callback_register(bool (*handler)(u8 battery_type, u8 *value));
extern void dma_set_pid(u32 pid);
extern int dma_tws_data_deal(u8 *data, int len);
extern int dma_ble_set_mac_addr(u8 *ble_addr);

// other api
extern int ai_mic_is_busy(void); //mic正在被使用
extern int ai_mic_rec_start(void); //启动mic和编码模块
extern int ai_mic_rec_close(void); //停止mic和编码模块
extern bool bt_is_sniff_close(void);
//语音识别接口
extern int mic_rec_pram_init(/* const char **name,  */u32 enc_type, u8 opus_type, u16(*speech_send)(u8 *buf, u16 len), u16 frame_num, u16 cbuf_size);

extern void dma_user_set_dev_info(struct _DmaUserInformation *user_info);

#endif

