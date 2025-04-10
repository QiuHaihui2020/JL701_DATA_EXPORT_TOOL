#include "rdec_soft.h"
#include "gptimer.h"
#include "clock.h"
#include "gpio.h"
#include "debug.h"

extern void wdt_clear();

void rdec_soft_demo()
{
    printf("rdec_soft_demo()\n");
    const struct rdec_soft_config rdec_config = {
        .rdec_a = IO_PORTA_00,
        .rdec_b = IO_PORTA_01,
        .filter_us = 1000,
        .mode = RDEC_PHASE_2,
        .tid = TIMER2,
    };
    u32 rdec_id = rdec_soft_init(&rdec_config);
    rdec_soft_start(rdec_id);

    while (1) {
        int value = rdec_soft_get_value(rdec_id);
        if (value != 0) {
            printf("( %d )", value);
        }
        mdelay(100);
        wdt_clear();
    }
}


