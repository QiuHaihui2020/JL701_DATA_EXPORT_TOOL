#include "display_task.h"
#include "oled/oled.h"
#include "oled/oledbmp.h"
#include "system/includes.h"
#include "user_cfg_id.h"
#include "audio_raw_writer.h"
#include "app_config.h"

#define OLED_DISPLAY_TASK_NAME    "od_dispaly"
/*vm参数读取失败时的默认参数*/
#define PCM_UART1_BAUDRATE			2000000		/*数据导出波特率,不用修改，和接收端设置一直*/
#define PCM_CH                      3
#define PCM_SINGLE_LEN              512

void oled_display_task(void *priv)
{
    int ret = 0;
    int msg[16];
    char strnum[16];
    u8 set_switch = 0;
    u8 run_tips_flag = 0;

    OLED_32x32_Chinese(1,48,&JL_LOGO[0][0]);
    OLED_P16x16Ch(30,5,1);//`显示杰理科技
	OLED_P16x16Ch(48,5,2);
	OLED_P16x16Ch(66,5,3);
	OLED_P16x16Ch(84,5,4);

    os_time_dly(100);
    OLED_Fill(0x00); //清屏

    OLED_P8x16Str(0, 0, "--");
    OLED_P8x16Str(28, 0, "PcmRxTool");
    OLED_P8x16Str(112, 0, "w+");
    OLED_P8x16Str(8, 2, "sd:off");
    OLED_P8x16Str(64, 2, "err:0");
    OLED_P8x16Str(8, 4, "baud:");
    OLED_P8x16Str(8, 6, "ch:");
    OLED_P8x16Str(64, 6, "len:");

    u32 uart_baud_rate = 2000000;
    u16 pcm_rx_single_size = 512;
    u8 pcm_channel = 3;

    /*读取通道数ch*/
    ret = syscfg_read(CFG_UART_PCM_RX_CH, &pcm_channel, 1);
    if (ret < 0) {
        printf("pcm channel read err, use default\n");
        pcm_channel = PCM_CH;
        syscfg_write(CFG_UART_PCM_RX_CH, &pcm_channel, 1);
        printf("use default pcm channel : %d\n", pcm_channel);
    }
    printf("=================================== pcm channel : %d\n", pcm_channel);
    oled_dispaly_task_post(OLED_DISPLAY_CH, (int *)((int)pcm_channel));

    /*读取单个通道的数据长度*/
    ret = syscfg_read(CFG_UART_PCM_RX_SIG_SIZE, &pcm_rx_single_size, 2);
    if (ret < 0) {
        printf("pcm_rx_single_size read err, use default\n");
        pcm_rx_single_size = PCM_SINGLE_LEN;
        syscfg_write(CFG_UART_PCM_RX_SIG_SIZE, &pcm_rx_single_size, 2);
    }
    printf("=================================== pcm_rx_single_size : %d\n", pcm_rx_single_size);
    oled_dispaly_task_post(OLED_DISPLAY_LEN, (int *)((int)pcm_rx_single_size));

    /*读取波特率*/
    ret = syscfg_read(CFG_UART_PCM_RX_BAUD_RATE, &uart_baud_rate, 4);
    if (ret < 0) {
        printf("uart_baud_rate read err, use default\n");
        uart_baud_rate = PCM_UART1_BAUDRATE;
        syscfg_write(CFG_UART_PCM_RX_BAUD_RATE, &uart_baud_rate, 4);
    }
    printf("=================================== uart_baud_rate : %d\n", uart_baud_rate);
    oled_dispaly_task_post(OLED_DISPLAY_BAUD, (int *)(uart_baud_rate));

    while (1) {
        ret = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }

        switch (msg[1]) {
        case OLED_DISPLAY_SD_ON:
#ifdef TCFG_LED_RED_GPIO
        gpio_set_mode(IO_PORT_SPILT(TCFG_LED_RED_GPIO), PORT_OUTPUT_HIGH);
#endif
            OLED_P8x16Str(32, 2, "on ");
            break;
        case OLED_DISPLAY_SD_OFF:
#ifdef TCFG_LED_RED_GPIO
        gpio_set_mode(IO_PORT_SPILT(TCFG_LED_RED_GPIO), PORT_OUTPUT_LOW);
#endif
            OLED_P8x16Str(32, 2, "off");
            break;
        case OLED_DISPLAY_CH:
            printf("=================================== display ch : %d\n", (u8)msg[2]);
            sprintf(strnum, "%02d", (u8)msg[2]);
            OLED_P8x16Str(40, 6, strnum);
            break;
        case OLED_DISPLAY_LEN:
            printf("=================================== display len : %d\n", (u16)msg[2]);
            sprintf(strnum, "%04d", (u16)msg[2]);
            OLED_P8x16Str(96, 6, strnum);
            break;
        case OLED_DISPLAY_BAUD:
            printf("=================================== display baud : %d\n", (u32)msg[2]);
            sprintf(strnum, "%07d", (u32)msg[2]);
            OLED_P8x16Str(56, 4, strnum);
            break;

        case OLED_DISPLAY_SET_NEXT:
            set_switch = (u8)msg[2];
            printf("set_switch: %d\n", set_switch);
            if (set_switch == 0) {
                printf("=================================== over change");
                //baud
                //OLED_P8x16Str(8, 4, "sec:");
                //OLED_P8x16Str(40, 4, "0         ");
                OLED_P8x16Str(40, 4, ":");
                //ch
                OLED_P8x16Str(24, 6, ":");
                //len
                OLED_P8x16Str(88, 6, ":");
                extern void audio_uart_exit(u8 sd_present);
                extern u8 audio_uart_init_runing();
                extern void audio_uart_init();
                if (audio_uart_init_runing()) {
                    /*改参数后重启，卡还在位，正常把残留数据落盘*/
                    audio_uart_exit(1);
                    audio_uart_init();
                }

            } else if (set_switch == 1) {
                printf("=================================== selete change ch\n");
                //ch
                OLED_P8x16Str(24, 6, "_");
                //len
                OLED_P8x16Str(88, 6, ":");
                //baud
                u32 baud;
                syscfg_read(CFG_UART_PCM_RX_BAUD_RATE, &baud, 4);
                sprintf(strnum, "%07d", baud);
                OLED_P8x16Str(8, 4, "baud:");
                OLED_P8x16Str(56, 4, strnum);
                /* OLED_P8x16Str(48, 4, ":"); */
            } else if (set_switch == 2) {
                printf("=================================== selete change len\n");

                OLED_P8x16Str(24, 6, ":");
                OLED_P8x16Str(88, 6, "_");
                OLED_P8x16Str(40, 4, ":");
                //baud
                /* syscfg_read(CFG_UART_PCM_RX_BAUD_RATE, &baud, 4); */
                /* sprintf(strnum_2, "%07d", baud); */
                /* OLED_P8x16Str(8, 4, "baud:"); */
                /* OLED_P8x16Str(56, 4, strnum_2); */
            } else if (set_switch == 3) {
                printf("=================================== selete change baud\n");

                OLED_P8x16Str(24, 6, ":");
                OLED_P8x16Str(88, 6, ":");
                OLED_P8x16Str(40, 4, "_");
            }
            break;
        case OLED_DISPLAY_FMT:
            /*嗅探未完成时保持 "--"，判定后固定显示 V1/V2*/
            if ((u8)msg[2] == RAW_FMT_V1) {
                OLED_P8x16Str(0, 0, "V1");
            } else if ((u8)msg[2] == RAW_FMT_V2) {
                OLED_P8x16Str(0, 0, "V2");
            } else {
                OLED_P8x16Str(0, 0, "--");
            }
            printf("=================================== display fmt : %d\n", (u8)msg[2]);
            break;
        case OLED_DISPLAY_LOST:
            sprintf(strnum, "%04d", ((u32)msg[2]) % 9999);
            OLED_P8x16Str(96, 2, strnum);
            break;
        case OLED_DISPLAY_RUN_TIPS:
            if (run_tips_flag) {
                run_tips_flag = 0;
#ifdef TCFG_LED_RED_GPIO
                gpio_set_mode(IO_PORT_SPILT(TCFG_LED_RED_GPIO), PORT_OUTPUT_LOW);
#endif
                OLED_P8x16Str(112, 0, "  ");
            } else {
                run_tips_flag = 1;
#ifdef TCFG_LED_RED_GPIO
                gpio_set_mode(IO_PORT_SPILT(TCFG_LED_RED_GPIO), PORT_OUTPUT_HIGH);
#endif
                OLED_P8x16Str(112, 0, "w+");
            }
         
            break;
        }

    }
    
}

int oled_dispaly_task_post(int msg, int *arg)
{
    int err = os_taskq_post_msg(OLED_DISPLAY_TASK_NAME, 2, msg, arg);
    if (err != OS_ERR_NONE) {
        printf("oled_dispaly_task_post err %d\n", err);
    }
    return 0;
}

int oled_display_init()
{
    OLED_Init();
    task_create(oled_display_task, NULL, OLED_DISPLAY_TASK_NAME);
    return 0;
}

int oled_display_exit()
{
    task_kill(OLED_DISPLAY_TASK_NAME);
    return 0;
}