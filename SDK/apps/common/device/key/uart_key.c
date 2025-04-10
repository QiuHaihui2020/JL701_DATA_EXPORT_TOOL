#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".uart_key.data.bss")
#pragma data_seg(".uart_key.data")
#pragma const_seg(".uart_key.text.const")
#pragma code_seg(".uart_key.text")
#endif
#include "key_driver.h"
#include "uart.h"
#include "app_config.h"


#if TCFG_UART_KEY_ENABLE

static int uart_key_init(void)
{
    return 0;
}

static u8 uart_get_key_value(void)
{
    char c;
    u8 key_value;

    if (getbyte(&c) == 0) {
        return NO_KEY;
    }

    switch (c) {
    case 'm':
        key_value = KEY_MODE;
        break;
    case 'u':
        key_value = KEY_UP;
        break;
    case 'd':
        key_value = KEY_DOWN;
        break;
    case 'o':
        key_value = KEY_OK;
        break;
    case 'e':
        key_value = KEY_MENU;
        break;
    default:
        key_value = NO_KEY;
        break;
    }

    return key_value;
}



#endif /* #if TCFG_UART_KEY_ENABLE */

