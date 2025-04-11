#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".idle_app_msg_handler.data.bss")
#pragma data_seg(".idle_app_msg_handler.data")
#pragma const_seg(".idle_app_msg_handler.text.const")
#pragma code_seg(".idle_app_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "dev_status.h"
#include "init.h"
#include "idle.h"
#include "oled/oled.h"
#include "user_cfg_id.h"
#include "display/display_task.h"

int idle_app_msg_handler(int *msg)
{
    char *logo = NULL;
    char *evt_logo = NULL;
    int ret = 0;


    if (false == app_in_mode(APP_MODE_IDLE)) {
        return -1;
    }
    printf("idle_app_msg type: %d\n", msg[0]);

    switch (msg[0]) {
    case APP_MSG_MUSIC_PP:
    case APP_MSG_MUSIC_PREV:
    case APP_MSG_MUSIC_NEXT:
        idle_key_event_handler(msg[0]);
        break;
    case APP_MSG_KEY_POWER_ON:
    case APP_MSG_KEY_POWER_ON_HOLD:
        idle_key_poweron_deal(msg[0]);
        break;
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
    case DEVICE_EVENT_FROM_USB_HOST:
        if (msg[1] == DEVICE_EVENT_OUT) {
            ///下线处理,设备管理卸载
            printf("[idle]DEVICE_EVENT_OUT\n");
            oled_dispaly_task_post(OLED_DISPLAY_SD_OFF, NULL);
            extern void audio_uart_exit();
            audio_uart_exit();
        } else {
            ///上线处理, 设备管理挂载
            printf("[idle]OLED_DISPLAY_SD_ON\n");
            oled_dispaly_task_post(OLED_DISPLAY_SD_ON, NULL);
            extern void audio_uart_init();
            audio_uart_init();
            extern int SD_test(void);
            /* SD_test(); */
        }

        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

int idle_app_device_event_handler(int *msg)
{
    int ret = 0;
    const char *logo = NULL;
    const char *usb_msg = NULL;
    u8 app  = 0xff ;
    u8 alarm_flag = 0;
    switch (msg[0]) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
    case DEVICE_EVENT_FROM_USB_HOST:
        ret = dev_status_event_filter(msg);///解码设备上下线， 设备挂载等处理
        printf("idle ret %d", ret);
        if (ret == true) {
            if (msg[1] == DEVICE_EVENT_OUT) {
                ///下线处理,设备管理卸载
                printf("[idle]DEVICE_EVENT_OUT\n");
            } else {
                ///上线处理, 设备管理挂载
                printf("[idle]DEVICE_EVENT_IN\n");
            }
        }
        break;
    default:
        /* printf("unknow SYS_DEVICE_EVENT!!, %x\n", (u32)event->arg); */
        ret = -1;
        break;
    }

    return ret;
}

static u8 ch_len_switch = 0;
int idle_key_event_handler(int key_msg)
{
    printf("key_msg : %d\n",key_msg);
    int ret = 0;
    u8 ch = 3;
    u16 len = 512;
    u32 baud = 2000000;
    u8 strnum[2];
    u8 strnum_1[4];
    u8 strnum_2[7];

    switch (key_msg) {
    case APP_MSG_MUSIC_PP:
        printf("APP_MSG_MUSIC_PP \n");
        ch_len_switch++;
        if (ch_len_switch > 3) {
            ch_len_switch = 0;
        }
        
        printf("ch_len_switch: %d\n", ch_len_switch);
        oled_dispaly_task_post(OLED_DISPLAY_SET_NEXT, (int *)(ch_len_switch));
#if 0
        if (ch_len_switch == 0) {
            //sec
            OLED_P8x16Str(8, 4, "sec:");
            OLED_P8x16Str(40, 4, "0         ");
            //ch
            OLED_P8x16Str(24, 6, ":");
            //len
            OLED_P8x16Str(88, 6, ":");
        } else if (ch_len_switch == 1) {
            //ch
            OLED_P8x16Str(24, 6, "_");
            //len
            OLED_P8x16Str(88, 6, ":");
            //baud
            syscfg_read(CFG_UART_PCM_RX_BAUD_RATE, &baud, 4);
            sprintf(strnum_2, "%07d", baud);
            OLED_P8x16Str(8, 4, "baud:");
            OLED_P8x16Str(56, 4, strnum_2);
            /* OLED_P8x16Str(48, 4, ":"); */
        } else if (ch_len_switch == 2) {
            OLED_P8x16Str(24, 6, ":");
            OLED_P8x16Str(88, 6, "_");
            OLED_P8x16Str(40, 4, ":");
            //baud
            /* syscfg_read(CFG_UART_PCM_RX_BAUD_RATE, &baud, 4); */
            /* sprintf(strnum_2, "%07d", baud); */
            /* OLED_P8x16Str(8, 4, "baud:"); */
            /* OLED_P8x16Str(56, 4, strnum_2); */
        } else if (ch_len_switch == 3) {
            OLED_P8x16Str(24, 6, ":");
            OLED_P8x16Str(88, 6, ":");
            OLED_P8x16Str(40, 4, "_");
        }
#endif
        break;
    case APP_MSG_MUSIC_PREV:
        printf("APP_MSG_MUSIC_PREV \n");
        if (ch_len_switch == 1) {
            ret = syscfg_read(CFG_UART_PCM_RX_CH, &ch, 1);
            if (ret < 0) {
                printf("pcm channel read err\n");
                ch = 3;
            }
            ch -= 1;
            if (ch < 1) {
                ch = 1;
            }
            printf("pcm channel : %02d\n", ch);
            syscfg_write(CFG_UART_PCM_RX_CH, &ch, 1);
            oled_dispaly_task_post(OLED_DISPLAY_CH, (int *)(ch));

        } else if (ch_len_switch == 2) {

            ret = syscfg_read(CFG_UART_PCM_RX_SIG_SIZE, &len, 2);
            if (ret < 0) {
                printf("pcm channel read err\n");
                len = 512;
            }
            len -= 4;
            if (len < 4) {
                len = 4;
            }
            printf("pcm_rx_single_size : %04d\n", len);
            syscfg_write(CFG_UART_PCM_RX_SIG_SIZE, &len, 2);
            oled_dispaly_task_post(OLED_DISPLAY_LEN, (int *)(len));

        } else if (ch_len_switch == 3) {
            ret = syscfg_read(CFG_UART_PCM_RX_BAUD_RATE, &baud, 4);
            if (ret < 0) {
                printf("uart_baud_rate read err\n");
                baud = 2000000;
            }
            baud -= 1000000;
            if (baud < 1000000) {
                baud = 1000000;
            }
            printf("uart_baud_rate : %d\n", baud);
            syscfg_write(CFG_UART_PCM_RX_BAUD_RATE, &baud, 4);
            oled_dispaly_task_post(OLED_DISPLAY_BAUD, (int *)(baud));

        }

        break;
    case APP_MSG_MUSIC_NEXT:
        printf("APP_MSG_MUSIC_PREV \n");
        if (ch_len_switch == 1) {
            ret = syscfg_read(CFG_UART_PCM_RX_CH, &ch, 1);
            if (ret < 0) {
                printf("pcm channel read err\n");
                ch = 3;
            }
            ch += 1;
            printf("pcm channel : %02d\n", ch);
            syscfg_write(CFG_UART_PCM_RX_CH, &ch, 1);
            oled_dispaly_task_post(OLED_DISPLAY_CH, (int *)(ch));

        } else if (ch_len_switch == 2) {

            ret = syscfg_read(CFG_UART_PCM_RX_SIG_SIZE, &len, 2);
            if (ret < 0) {
                printf("pcm_rx_single_size read err\n");
                len = 512;
            }
            len += 4;
            printf("pcm_rx_single_size : %04d\n", len);
            syscfg_write(CFG_UART_PCM_RX_SIG_SIZE, &len, 2);
            oled_dispaly_task_post(OLED_DISPLAY_LEN, (int *)(len));

        } else if (ch_len_switch == 3) {
            ret = syscfg_read(CFG_UART_PCM_RX_BAUD_RATE, &baud, 4);
            if (ret < 0) {
                printf("uart_baud_rate read err\n");
                baud = 2000000;
            }
            baud += 1000000;
            if (baud > 9000000) {
                baud = 9000000;
            }
            printf("uart_baud_rate : %d\n", baud);
            syscfg_write(CFG_UART_PCM_RX_BAUD_RATE, &baud, 4);
            oled_dispaly_task_post(OLED_DISPLAY_BAUD, (int *)(baud));

        }
        break;
    }

    return 0;
}