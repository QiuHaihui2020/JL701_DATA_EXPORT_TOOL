#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".record_app_msg_handler.data.bss")
#pragma data_seg(".record_app_msg_handler.data")
#pragma const_seg(".record_app_msg_handler.text.const")
#pragma code_seg(".record_app_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"



#if TCFG_APP_RECORD_EN


//*----------------------------------------------------------------------------*/
/**@brief    mic录音启动
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void record_mic_start(void)
{
#if 0
    struct record_file_fmt fmt = {0};
    /* char logo[] = {"sd0"}; */		//可以指定设备
    char folder[] = {REC_FOLDER_NAME};         //录音文件夹名称
    char filename[] = {"AC69****"};     //录音文件名，不需要加后缀，录音接口会根据编码格式添加后缀

#if (TCFG_NOR_REC)
    char logo[] = {"rec_nor"};		//外挂flash录音
#elif (FLASH_INSIDE_REC_ENABLE)
    char logo[] = {"rec_sdfile"};		//内置flash录音
#else
    char *logo = dev_manager_get_phy_logo(dev_manager_find_active(0));//普通设备录音，获取最后活动设备
#endif

    fmt.dev = logo;
    fmt.folder = folder;
    fmt.filename = filename;
#if (RECORDER_MIX_EN)
    //如果开了混合录音这里获取编码类型是为了使得保存的录音文件格式一致，主要指针对695
    fmt.coding_type = recorder_mix_get_coding_type();
#else
    fmt.coding_type = AUDIO_CODING_MP3; //编码格式：AUDIO_CODING_WAV, AUDIO_CODING_MP3
#endif/*RECORDER_MIX_EN*/
    fmt.channel = 1;                    //声道数： 1：单声道 2：双声道
    fmt.cut_head_time = 300;            //录音文件去头时间,单位ms
    fmt.cut_tail_time = 300;            //录音文件去尾时间,单位ms
    fmt.limit_size = 3000;              //录音文件大小最小限制， 单位byte
    fmt.gain = 8;
#if (TCFG_MIC_EFFECT_ENABLE && (RECORDER_MIX_EN == 0))
    fmt.sample_rate = MIC_EFFECT_SAMPLERATE;            //采样率：8000，16000，32000，44100
    fmt.source = ENCODE_SOURCE_MIX;     //录音输入源
#else
    fmt.sample_rate = 44100;            //采样率：8000，16000，32000，44100
    fmt.source = ENCODE_SOURCE_MIC;     //录音输入源
#endif//TCFG_MIC_EFFECT_ENABLE
    fmt.err_callback = NULL;
    int ret = recorder_encode_start(&fmt);
    if (ret) {
        log_e("record_mic_start fail !!, dev = %s\n", logo);
    } else {
        log_i("record_mic_start succ !!, dev = %s\n", logo);
    }
#endif
}
//*----------------------------------------------------------------------------*/
/**@brief    mic 录音停止
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void record_mic_stop(void)
{
    /* recorder_encode_stop(); */
}

//*----------------------------------------------------------------------------*/
/**@brief    mic 录音与回放切换
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void record_key_pp()
{
#if 0
    if (recorder_is_encoding()) {
        log_i("mic record stop && replay\n");
        record_mic_stop();
        record_file_play();
    } else {
        record_file_close();
        record_mic_start();
        log_i("mic record start\n");
    }
#endif
}


#endif

