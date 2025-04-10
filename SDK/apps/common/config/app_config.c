#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".app_config.data.bss")
#pragma data_seg(".app_config.data")
#pragma const_seg(".app_config.text.const")
#pragma code_seg(".app_config.text")
#endif
#include "system/includes.h"
#include "app_config.h"
#include "config/config_interface.h"
#include "fs/resfile.h"
#include "sdk_config.h"

#define LOG_TAG     "[APP-CONFIG]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#include "debug.h"


extern void dec_eq_test();
static int sdfile_test(const char *name);
int jl_cfg_dec_test(void);

ci_transport_config_uart_t config = {
    CI_TRANSPORT_CONFIG_UART,
#if (defined(TCFG_CFG_TOOL_ENABLE)&&TCFG_CFG_TOOL_ENABLE)
    1000000,
#else
    115200,
#endif
    0,
    0,
    NULL,
};

#if (TCFG_ONLINE_ENABLE || TCFG_CFG_TOOL_ENABLE)
#if (TCFG_COMM_TYPE == TCFG_UART_COMM)
int config_online_init()
{
    log_info("-------online Config Start-------");
    config_layer_init(ci_transport_uart_instance(), &config);
    return 0;
}
__initcall(config_online_init);
#endif
#endif

void config_test(void)
{

#if 1
    log_info("-------EQ online Config Start-------");

    /* sys_clk_set(SYS_48M); */

    //Setup dev input
    config_layer_init(ci_transport_uart_instance(), &config);

    dec_eq_test();
    while (1) {
        extern void clr_wdt();
        clr_wdt();
    }
#endif

#if 0
    log_info("-------SDFile test Start------");

#ifdef SDFILE_DEV

#define SDFILE_NAME1 SDFILE_ROOT_PATH"xx.bin"

    void *mnt = mount(SDFILE_DEV, SDFILE_MOUNT_PATH, "jlfs", 0, NULL);
    if (mnt == NULL) {
        log_error("mount jlfs failed");
    }

    sdfile_test(SDFILE_NAME1);

#endif  /*SDFILE_DEV*/

#endif

#if 0
    log_info("--------JL CFG test Start------");

    jl_cfg_dec_test();
#endif
    while (1);
}

static int sdfile_test(const char *name)
{
    log_d("SDFILE file path: %s", name);
    RESFILE *fp = NULL;
    u8 buf[16];
    u8 len;

    fp = resfile_open(name);
    if (!fp) {
        log_d("sdfile open file ERR!");
        return -1;
    }

    len = resfile_read(fp, buf, sizeof(buf));

    if (len != sizeof(buf)) {
        log_d("0-read file ERR!");
        goto _end;
    }

    put_buf(buf, sizeof(buf));

    resfile_seek(fp, 16, RESFILE_SEEK_SET);

    len = resfile_read(fp, buf, sizeof(buf));

    if (len != sizeof(buf)) {
        log_d("1-read file ERR!");
        goto _end;
    }

    put_buf(buf, sizeof(buf));

    log_d("SDFILE ok!");

_end:
    if (fp) {
        resfile_close(fp);
    }


    return 0;
}
