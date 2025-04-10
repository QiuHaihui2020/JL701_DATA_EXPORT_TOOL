#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".gpio_file_parse.data.bss")
#pragma data_seg(".gpio_file_parse.data")
#pragma const_seg(".gpio_file_parse.text.const")
#pragma code_seg(".gpio_file_parse.text")
#endif
#include "gpio_config.h"


#define __gpio_mask(gpio) \
	BIT((gpio) % IO_GROUP_NUM)


static const u16 gpio_uuid_table[][2] = {
    { IO_PORTA_00, 0xc2e1 },
    { IO_PORTA_01, 0xc2e2 },
    { IO_PORTA_02, 0xc2e3 },
    { IO_PORTA_03, 0xc2e4 },
    { IO_PORTA_04, 0xc2e5 },
    { IO_PORTA_05, 0xc2e6 },
    { IO_PORTA_06, 0xc2e7 },
#ifdef IO_PORTA_07
    { IO_PORTA_07, 0xc2e8 },
#endif
#ifdef IO_PORTA_08
    { IO_PORTA_08, 0xc2e9 },
#endif
#ifdef IO_PORTA_09
    { IO_PORTA_09, 0xc2ea },
#endif
#ifdef IO_PORTA_10
    { IO_PORTA_10, 0xc302 },
#endif
#ifdef IO_PORTA_11
    { IO_PORTA_11, 0xc303 },
#endif
#ifdef IO_PORTA_12
    { IO_PORTA_12, 0xc304 },
#endif
#ifdef IO_PORTA_13
    { IO_PORTA_13, 0xc305 },
#endif
#ifdef IO_PORTA_14
    { IO_PORTA_14, 0xc306 },
#endif
#ifdef IO_PORTA_15
    { IO_PORTA_15, 0xc307 },
#endif
    { IO_PORTB_00, 0x4f42 },
    { IO_PORTB_01, 0x4f43 },
    { IO_PORTB_02, 0x4f44 },
    { IO_PORTB_03, 0x4f45 },
    { IO_PORTB_04, 0x4f46 },
#ifdef IO_PORTB_05
    { IO_PORTB_05, 0x4f47 },
#endif
#ifdef IO_PORTB_06
    { IO_PORTB_06, 0x4f48 },
#endif
#ifdef IO_PORTB_07
    { IO_PORTB_07, 0x4f49 },
#endif
#ifdef IO_PORTB_08
    { IO_PORTB_08, 0x4f4a },
#endif
#ifdef IO_PORTB_09
    { IO_PORTB_09, 0x4f4b },
#endif
#ifdef IO_PORTB_10
    { IO_PORTB_10, 0x4f63 },
#endif
#ifdef IO_PORTB_11
    { IO_PORTB_11, 0x4f64 },
#endif
#ifdef IO_PORTB_12
    { IO_PORTB_12, 0x4f65 },
#endif
#ifdef IO_PORTB_13
    { IO_PORTB_13, 0x4f66 },
#endif
#ifdef IO_PORTB_14
    { IO_PORTB_14, 0x4f67 },
#endif
#ifdef IO_PORTB_15
    { IO_PORTB_15, 0x4f68 },
#endif
    { IO_PORTC_00, 0xdba3 },
    { IO_PORTC_01, 0xdba4 },
    { IO_PORTC_02, 0xdba5 },
    { IO_PORTC_03, 0xdba6 },
    { IO_PORTC_04, 0xdba7 },
    { IO_PORTC_05, 0xdba8 },
#ifdef IO_PORTC_06
    { IO_PORTC_06, 0xdba9 },
#endif
#ifdef IO_PORTC_07
    { IO_PORTC_07, 0xdbaa },
#endif
#ifdef IO_PORTC_08
    { IO_PORTC_08, 0xdbab },
#endif
#ifdef IO_PORTC_09
    { IO_PORTC_09, 0xdbac },
#endif
#ifdef IO_PORTC_10
    { IO_PORTC_10, 0xdbc4 },
#endif
#ifdef IO_PORTC_11
    { IO_PORTC_11, 0xdbc5 },
#endif
#ifdef IO_PORTC_12
    { IO_PORTC_12, 0xdbc6 },
#endif
#ifdef IO_PORTC_13
    { IO_PORTC_13, 0xdbc7 },
#endif
#ifdef IO_PORTC_14
    { IO_PORTC_14, 0xdbc8 },
#endif
#ifdef IO_PORTC_15
    { IO_PORTC_15, 0xdbc9 },
#endif
    { IO_PORTD_00, 0x6804 },
    { IO_PORTD_01, 0x6805 },
    { IO_PORTD_02, 0x6806 },
    { IO_PORTD_03, 0x6807 },
#ifdef IO_PORTD_04
    { IO_PORTD_04, 0x6808 },
#endif
#ifdef IO_PORTD_05
    { IO_PORTD_05, 0x6809 },
#endif
#ifdef IO_PORTD_06
    { IO_PORTD_06, 0x680a },
#endif
#ifdef IO_PORTD_07
    { IO_PORTD_07, 0x680b },
#endif
#ifdef IO_PORTE_00
    { IO_PORTE_00, 0xf465 },
#endif
#ifdef IO_PORTE_01
    { IO_PORTE_01, 0xf466 },
#endif
#ifdef IO_PORTE_02
    { IO_PORTE_02, 0xf467 },
#endif
#ifdef IO_PORTE_03
    { IO_PORTE_03, 0xf468 },
#endif
#ifdef IO_PORTE_04
    { IO_PORTE_04, 0xf469 },
#endif
#ifdef IO_PORTE_05
    { IO_PORTE_05, 0xf46a },
#endif
#ifdef IO_PORTE_06
    { IO_PORTE_06, 0xf46b },
#endif
#ifdef IO_PORTG_00
    { IO_PORTG_00, 0x0d27 },
#endif
#ifdef IO_PORTG_01
    { IO_PORTG_01, 0x0d28 },
#endif
#ifdef IO_PORTG_02
    { IO_PORTG_02, 0x0d29 },
#endif
#ifdef IO_PORTG_03
    { IO_PORTG_03, 0x0d2a },
#endif
#ifdef IO_PORTG_04
    { IO_PORTG_04, 0x0d2b },
#endif
#ifdef IO_PORTG_05
    { IO_PORTG_05, 0x0d2c },
#endif
#ifdef IO_PORTG_06
    { IO_PORTG_06, 0x0d2d },
#endif
#ifdef IO_PORTG_07
    { IO_PORTG_07, 0x0d2e },
#endif
#ifdef IO_PORTG_08
    { IO_PORTG_08, 0x0d2f },
#endif
    { IO_PORT_DP,  0x2cd4 },
    { IO_PORT_DM,  0x2cd1 },
#ifdef IO_PORT_LDOIN
    { IO_PORT_LDOIN, 0x7596 },
#endif
};

u8 uuid2gpio(u16 uuid)
{
    for (int i = 0; i < ARRAY_SIZE(gpio_uuid_table); i++) {
        if (gpio_uuid_table[i][1] == uuid) {
            return gpio_uuid_table[i][0];
        }
    }
    return 0xff;
}

/* void gpio_set_config(struct gpio_config_t *config) */
/* { */
/* #if 1 */
/* #ifndef CONFIG_CPU_BR27 */
/*     struct port_reg *g; */
/*     g = gpio2reg(config->gpio); */
/*     if (!g) { */
/*         return; */
/*     } */
/*     u32 mask = __gpio_mask(config->gpio); */
/*  */
/*     //printf("gpio_set: %d, %d\n", config->gpio, config->mode); */
/*  */
/*     local_irq_disable(); */
/*  */
/*     switch (config->mode) { */
/*     case GPIO_HI_Z: */
/*         g->die &= ~mask; */
/*         g->dieh &= ~mask; */
/*         g->pu0 &= ~mask; */
/*         g->pd0 &= ~mask; */
/*         g->dir |= mask; */
/*         break; */
/*     case GPIO_OUTPUT_0: */
/*         g->out &= ~mask; */
/*         g->dir &= ~mask; */
/*         break; */
/*     case GPIO_OUTPUT_TOGGLE: */
/*         if (g->out & mask) { */
/*             g->out &= ~mask; */
/*             g->dir &= ~mask; */
/*             break; */
/*         } */
/*     #<{(| fall-throught |)}># */
/*     case GPIO_OUTPUT_1: */
/*         g->out |= mask; */
/*         g->dir &= ~mask; */
/*         if (config->hd & BIT(0)) { */
/*             g->hd0 |= mask; */
/*         } else { */
/*             g->hd0 &= ~mask; */
/*         } */
/*         if (config->hd & BIT(1)) { */
/*             g->hd0 |= mask; */
/*         } else { */
/*             g->hd0 &= ~mask; */
/*         } */
/*         break; */
/*     case GPIO_INPUT_PU: */
/*         g->pu0 |= mask; */
/*         g->pd0 &= ~mask; */
/*         g->dir |= mask; */
/*         g->die |= mask; */
/*         break; */
/*     case GPIO_INPUT_PD: */
/*         g->pu0 &= ~mask; */
/*         g->pd0 |= mask; */
/*         g->dir |= mask; */
/*         g->die |= mask; */
/*         break; */
/*     case GPIO_INPUT_FLOATING: */
/*         g->pu0 &= ~mask; */
/*         g->pd0 &= ~mask; */
/*         g->dir |= mask; */
/*         g->die |= mask; */
/*         break; */
/*     } */
/*  */
/*     local_irq_enable(); */
/* #endif */
/* #else */
/* #ifndef CONFIG_CPU_BR27 */
/*     local_irq_disable(); */
/* 	gpio_set_mode(config->gpio/16, BIT(config->gpio%16),config->mode); */
/* 	if(config->mode == PORT_OUTPUT_HIGH){ */
/* 		gpio_set_drive_strength(config->gpio/16, BIT(config->gpio%16),config->hd); */
/* 	} */
/*     local_irq_enable(); */
/* #endif */
/* #endif */
/* } */

