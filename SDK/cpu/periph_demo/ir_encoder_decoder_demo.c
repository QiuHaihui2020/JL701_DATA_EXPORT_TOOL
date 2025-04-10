#include "ir_encoder.h"
#include "ir_decoder.h"
#include "clock.h"
#include "gpio.h"
#include "debug.h"
#include "gptimer.h"

extern void wdt_clear();

void ir_encoder_demo()
{
    printf("ir_encoder_demo()\n");
    ir_encoder_init(IO_PORTA_01, 38000, 3000); //红外信号输出IO_PORTA_00, 频率38000, 占空比30.00%
    /* ir_encoder_init(IO_PORTA_00, 38000, 10000); //红外信号输出IO_PORTA_00, 频率38000, 占空比100.00% */

    u8 addr = 0x4c; //设置红外地址 0x4c
    u8 cmd = 0x4a; //设置红外命令 0x4a
    u8 repeat_en = 1; //重复发送使能
    ir_encoder_tx(addr, cmd, repeat_en);
    udelay(2 * 1000 * 1000);
    ir_encoder_repeat_stop(); //停止发送
    gptimer_dump();
    while (1) {
        wdt_clear();
    }
}

const struct gptimer_config ir_decode_config = {
    .capture.filter = 0,//38000,
    .capture.max_period = 110 * 1000, //110ms
    .capture.port = PORTA,
    .capture.pin = BIT(1),
    .irq_cb = NULL,
    .irq_priority = 3,
    //根据红外模块的 idle 电平状态，选择边沿触发方式
    /* .mode = GPTIMER_MODE_CAPTURE_EDGE_FALL, */
    .mode = GPTIMER_MODE_CAPTURE_EDGE_RISE,
};
void ir_decoder_demo()
{
    printf("ir_decoder_demo()\n");
    ir_decoder_init(&ir_decode_config); //红外信号接收IO_PORTA_O1
    u32 ir_key; //接收到的红外码
    while (1) {
        ir_key = ir_decoder_get_data();
        if (ir_key != (u32) - 1) {
            printf("ir_key:0x%x\n", ir_key);
        }
        udelay(5 * 100 * 1000);
        wdt_clear();
    }
}

void ir_encoder_decoder_demo()
{
    printf("ir_encoder_decoder_demo()\n");
    //设置红外发送
    u8 addr = 0;
    u8 cmd = 0x80;
    u8 repeat_en = 0;
    /* ir_encoder_init(IO_PORTA_00, 38000, 3000); //红外信号输出IO_PORTA_00, 频率38000, 占空比30.00% */
    ir_encoder_init(IO_PORTA_00, 38000, 10000); //红外信号输出IO_PORTA_00, 频率0, 占空比100.00%

    //设置红外接收
    ir_decoder_init(&ir_decode_config); //红外信号接收IO_PORTA_O1

    u32 ir_key; //接收到的红外码
    u32 time_cnt = 0;
    while (1) {
        ir_key = ir_decoder_get_data(); //NEC数据排列格式:(命令反码 + 命令码 + 地址反码 +地址码)
        //提供以下接口使用, 获取接口每调用一次,内部接收到的数据会清掉
        /* u32 ir_cmd = ir_decoder_get_command_value(); */
        /* u32 ir_cmd = ir_decoder_get_command_value_uncheck(); */
        /* u32 ir_addr = ir_decoder_get_address_value(); */
        /* u32 ir_addr = ir_decoder_get_address_value_uncheck(); */
        if (ir_key != (u32) - 1) {
            printf("ir_key:0x%x\n", ir_key);
        }
        if (time_cnt % 2) {
            ir_encoder_tx(addr, cmd, repeat_en);
            addr++;
            cmd++;
        }
        time_cnt++;
        udelay(1 * 1000 * 1000);
        wdt_clear();
    }
}


