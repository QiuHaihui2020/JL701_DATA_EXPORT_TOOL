#ifndef _DMA_API_H_
#define _DMA_API_H_

#include "system/includes.h"

struct _DmaUserInformation {
    char *name;
    /*
     *default "3P" //HEADPHONE, SPEAKER, DOCK, TWS_HEADSET 新增TWS耳机类型
     */
    char *device_type;
    /*
     *厂商
     */
    char *manufacturer;
    /*
     *型号
     */
    char *model;
    char *firmware_version;
    char *software_version;
    /*
     *设备支持的语音交互方式
    0:"PRESS_AND_HOLD", 1:"TAP", 2:"PHONE_WAKEUP"
     */
    u32 initiator_type;
    /*
     *类似06:36:3b:cd:c1:b0
     */
    char *classic_bluetooth_mac;
    /*
     *移动到了PairInformation，为了不引起兼容问题，以下13、14不要再使用了
     *string rand=13;//长度8位的随机字符串。注意：需要每次返回都不一样。具体逻辑请看README.md中的鉴权部分
     *string sign=14;//sign = sha256(rand + key + product_id + serial_number),  key需要从度秘平台获得。具体逻辑请看README.md中的鉴权部分
     */
    /*
     *如果这个字段为false，手机端会每1-2秒发一个GET_STATE的请求，作为设备的心跳包。设为true的时候，手机不发送心跳请求
     */
    u8 disable_heart_beat;
    /*
     *如果这个字段为true，手机端生成sign2的时候会做第二次hash, new_sign2 = sign_method ( sign2 + requestId + int2string(command))，具体逻辑请看README.md中的“高级安全”部分
     */
    u8 enable_advanced_security;
    /*
     *可选，默认为false；标识设备是否使用FM发射的方式播放音频。如果这个字段为true。app连接外设成功之后会触发fm发射频率设置的逻辑。
     */
    u8 support_fm;
    /*
     *可选，标识外设上负责ota软件的版本。ota的软件和实现功能的软件（software_version）是独立的，所以版本号也独立
     */
    char *ota_version;
    /*
     *如果这个字段为true，手机端不再强制要求当前a2dp输出的设备，和dma设备是同一个设备
     */
    u8 no_a2dp;
    /*
     *如果这个字段为true，设备不支持DMA的ForwardATCommand指令，这样小度app在尝试帮用户拨打电话的时候，不会使用ForwardATCommand，而是使用ios或安卓系统的拨号api
     */
    u8 no_at_command;
    /*
     *如果这个字段为true，设备支持DMA的Media能力。这样小度app在执行音量调整的时候，会发送IssueMediaControl指令。在小度app收到用户的“下一首”之类的query的时候，如果当前没有媒体播放，会尝试用IssueMediaControl指令控制手机上的其它播放器。
     */
    u8 support_media;
    /*
     *如果这个字段为true，设备支持DMA的静默升级能力
     */
    u8 support_sota;
    /*
     *是否是耳机类产品
     */
    u8 is_earphone;
    /*
     *是否支持双路录音模式，一般指两个Mic的产品，比如耳机类产品
     */
    u8 support_dual_record;
    /*
     *是否支持本地唤醒
     */
    u8 support_local_voice_wake;
    /*
     *是否支持本地文件灌测
     */
    u8 support_local_audio_file;
    /*
     *是否支持波束算法特征值
     */
    u8 support_model_beamforming_asr;
    /*
     * 是否支持云端映射serial_number
     */
    u8 support_mapping_serial_number;
    /*
     * 目前仅限tws耳机使用，云端映射serial_number
     */
    char *mapping_serial_number;
    /*
     * 目前仅限tws耳机使用，tws耳机左右耳serial_number不同，serial_number标识左耳耳机，serial_number1标识右耳耳机
     */
    char *serial_number1;
    /*
     * 目前仅限tws耳机使用，tws耳机左右耳mac地址不同，classic_bluetooth_mac标识左耳的mac，classic_bluetooth_mac1标识右耳的mac
     */
    char *classic_bluetooth_mac1;
    /*
     * 蓝牙外设是否支持打点，字段存在且值为true支持打点，其他情况不支持
     */
    u8 support_log;
    /*
     * 是否支持"下一首"唤醒词
     */
    u8 support_wakeword_next;
    /*
     * 支持的电量结构，0：不支持电量展示，2：仅支持双耳电量展示，3：支持双耳电量 + 盒仓电量 ，缺省值是0
     */
    uint64_t battery_structure;
    /*
     * 是否支持自定义触控，0：不支持，1：小度一代tws耳机风格，2：小度二代pro耳机风格，3：小度C版耳机风格
     */
    uint64_t touch_setting_type;
    /*
     * 是否支持设备端本地唤醒， 缺省值false
     */
    u8 support_local_wakeup;
    /*
     *是否支持ANC主动降噪, 缺省值false
     */
    u8 support_anc;
    /*
     * 是否支持耳塞贴合度检测，缺省值false
     */
    u8 support_fit_detection;
    /*
     * 是否支持耳机入耳检测，缺省值false
     */
    u8 support_in_ear_detection;
    /*
     * 是否支持查找耳机功能，缺失值false
     */
    u8 support_find_earphone;
};

#endif
