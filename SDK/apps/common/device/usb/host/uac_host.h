#ifndef  __UAC_HOST_H__
#define  __UAC_HOST_H__

#pragma pack(1)

#define UAC_MAX_CFG 4
struct audio_streaming_config {
    u8 alterset; //配置选择控制
    u8 ch; //单双通道
    u8 bit_res; //位宽
    u32 sample_freq[6]; //采样率
    u8 target_ep; //从机描述符的端点
    u8 interval;
    u8 ep_attr;  //端点属性
    u32 max_packet_size; //最大传输包长
    u32 cur_sample_freq; //当前采样率
};

typedef struct {
    struct audio_streaming_config as_cfg[UAC_MAX_CFG];
    u8 ctrl_intr_num;  //控制接口
    u8 intr_num; //接口号
    u8 feature_id; //特征id
    u8 controls[3]; //控制位信息
    u8 host_ep; //主机分配的端点
    u8 cur_alterseting; //当前使用的配置
    void (*isr_hanlder)(void *priv);
    void *priv;
} UAC_INFO;

//内部使用API
int usb_audio_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf);
void uac_host_info_dump();

//外部调用API
void uac_host_init();
void uac_host_deinit();
void uac_host_mount(u32 usb_id);
void uac_host_unmount(u32 usb_id);

u32 uac_host_open_spk(usb_dev usb_id, u32 ch, u32 sample_freq, u32 bit_res);
u32 uac_host_close_spk(usb_dev usb_id);
int uac_host_set_spk_volume(usb_dev usb_id, u16 vol);
u32 uac_host_write_spk_cbuf(u8 *buf, u32 len);
u32 uac_host_get_spk_cbuf_len(); //获取spk_cbuf当前长度
u32 uac_host_get_spk_cbuf_size(); //获取spk_cbuf总长度
u32 uac_host_query_spk_info(u32 ch, u32 sample_freq, u32 bit_res); //查询spk是否支持该配置 0:支持, 1:不支持
u32 uac_host_set_spk_hanlder(void (*isr_hanlder)(void *priv), void *priv); //设置spk回调函数

u32 uac_host_open_mic(usb_dev usb_id, u32 ch, u32 sample_freq, u32 bit_res);
u32 uac_host_close_mic(usb_dev usb_id);
int uac_host_set_mic_volume(usb_dev usb_id, u16 vol);
u32 uac_host_read_mic_cbuf(u8 *buf, u32 len);
u32 uac_host_get_mic_cbuf_len();//获取mic_cbuf当前长度
u32 uac_host_get_mic_cbuf_size();//获取mic_cbuf总长度
u32 uac_host_query_mic_info(u32 ch, u32 sample_freq, u32 bit_res); //查询mic是否支持该配置 0:支持, 1:不支持
u32 uac_host_set_mic_hanlder(void (*isr_hanlder)(void *priv), void *priv); //设置mic中断的回调函数


extern int audio_usb_host_init(const char *audio);
extern void audio_usb_host_release(void);
extern void audio_host_spk_fade_in(void);


#endif
