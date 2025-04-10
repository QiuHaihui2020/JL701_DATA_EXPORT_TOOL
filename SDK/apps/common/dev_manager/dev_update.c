#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".dev_update.data.bss")
#pragma data_seg(".dev_update.data")
#pragma const_seg(".dev_update.text.const")
#pragma code_seg(".dev_update.text")
#endif
#include "dev_update.h"
#include "dev_manager.h"
#include "update/update.h"
#include "update/update_loader_download.h"
#include "app_config.h"
#include "btcontroller_modules.h"
#include "trim.h"

#include "update/update_interactive_uart.h"

#if defined(CONFIG_SD_UPDATE_ENABLE) || defined(CONFIG_USB_UPDATE_ENABLE)
#define DEV_UPDATE_EN		1
#else
#define DEV_UPDATE_EN		0
#endif


extern bool uart_update_send_update_ready(char *file_path);
extern bool get_uart_update_sta(void);
extern void latch_reset(void);

static char update_path[48] = {0};
extern const char updata_file_name[];

struct __update_dev_reg {
    char *logo;
    int type;
    union {
        UPDATA_SD sd;
        UPDATE_UDISK udisk;
    } u;
};


#if TCFG_SD0_ENABLE
static const struct __update_dev_reg sd0_update = {
    .logo = "sd0",
    .type = SD0_UPDATA,
    .u.sd.control_type = SD_CONTROLLER_0,
#ifdef TCFG_SD0_PORT_CMD
    .u.sd.control_io_clk = TCFG_SD0_PORT_CLK,
    .u.sd.control_io_cmd = TCFG_SD0_PORT_CMD,
    .u.sd.control_io_dat = TCFG_SD0_PORT_DA0,
#else
#if (TCFG_SD0_PORTS=='A')
    .u.sd.control_io = SD0_IO_A,
#elif (TCFG_SD0_PORTS=='B')
    .u.sd.control_io = SD0_IO_B,
#elif (TCFG_SD0_PORTS=='C')
    .u.sd.control_io = SD0_IO_C,
#elif (TCFG_SD0_PORTS=='D')
    .u.sd.control_io = SD0_IO_D,
#elif (TCFG_SD0_PORTS=='E')
    .u.sd.control_io = SD0_IO_E,
#elif (TCFG_SD0_PORTS=='F')
    .u.sd.control_io = SD0_IO_F,
#endif
#endif
    .u.sd.power = 1,
};
#endif//TCFG_SD0_ENABLE

#if TCFG_SD1_ENABLE
static const struct __update_dev_reg sd1_update = {
    .logo = "sd1",
    .type = SD1_UPDATA,
    .u.sd.control_type = SD_CONTROLLER_1,
#if (TCFG_SD1_PORTS=='A')
    .u.sd.control_io = SD1_IO_A,
#else
    .u.sd.control_io = SD1_IO_B,
#endif
    .u.sd.power = 1,

};
#endif//TCFG_SD1_ENABLE

#if TCFG_UDISK_ENABLE
static const struct __update_dev_reg udisk_update = {
    .logo = "udisk0",
    .type = USB_UPDATA,
};
#endif//TCFG_UDISK_ENABLE


static const struct __update_dev_reg *update_dev_list[] = {
#if TCFG_UDISK_ENABLE
    &udisk_update,
#endif//TCFG_UDISK_ENABLE
#if TCFG_SD0_ENABLE
    &sd0_update,
#endif//
#if TCFG_SD1_ENABLE
    &sd1_update,
#endif//TCFG_SD1_ENABLE
};

void *dev_update_get_parm(int type)
{
    struct __update_dev_reg *parm = NULL;
    for (int i = 0; i < ARRAY_SIZE(update_dev_list); i++) {
        if (update_dev_list[i]->type == type) {
            parm = (struct __update_dev_reg *)update_dev_list[i];
        }
    }

    if (parm == NULL) {
        return NULL;
    }
    return (void *)&parm->u.sd;
}


struct strg_update {
    void *fd;
    char *update_path;
    u32 offset_addr;
    u8 update_target;
};
static struct strg_update strg_update = {0};
#define __this 		(&strg_update)

static u16 strg_f_open(void)
{
    if (!__this->update_path) {
        printf("file path err ");
        return false;
    }

    if (__this->fd) {
        return true;
        /* fclose(__this->fd);
        __this->fd = NULL; */
    }
    __this->fd = fopen(__this->update_path, "r");
    if (!__this->fd) {
        printf("file open err ");
        return false;
    }
    return true;
}

static u16 strg_f_read(void *fp, u8 *buff, u16 len)
{
    if (!__this->fd) {
        return (u16) - 1;
    }

    len = fread(buff, len, 1, __this->fd);
    return len;
}

static int strg_f_seek(void *fp, u8 type, u32 offset)
{
    if (!__this->fd) {
        return (int) - 1;
    }

    if (type == SEEK_SET) {
        offset += __this->offset_addr;
    }
    int ret = fseek(__this->fd, offset, type);
    /* return 0; // 0k */
    return ret;
}
static u16 strg_f_stop(u8 err)
{
    if (__this->fd) {
        fclose(__this->fd);
        __this->fd = NULL;
    }
    return true;
}

static int strg_update_set_file_path_and_hdl(char *update_path, void *fd)
{
    __this->update_path = update_path;
    __this->fd = fd;

    return true;
}

static const update_op_api_t strg_dev_update_op = {
    .f_open = strg_f_open,
    .f_read = strg_f_read,
    .f_seek = strg_f_seek,
    .f_stop = strg_f_stop,
};

void update_param_ext_fill(UPDATA_PARM *p, u8 ext_type, u8 *ext_data, u8 ext_len);

static void dev_update_set_offset_addr(u32 offset)
{
    __this->offset_addr = offset;
    strg_f_seek(__this->fd, SEEK_SET, 0); //确定好偏移
}

static void dev_update_param_private_handle(UPDATA_PARM *p)
{
    // cppcheck-suppress unreadVariable
    u16 up_type = p->parm_type;

#if (defined CONFIG_SD_UPDATE_ENABLE) && (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE)
    if ((up_type == SD0_UPDATA) || (up_type == SD1_UPDATA)) {
        int sd_start = (u32)p->parm_priv;
        void *sd = NULL;
        sd = dev_update_get_parm(up_type);
        if (sd) {
            memcpy((void *)sd_start, sd, UPDATE_PRIV_PARAM_LEN);
        } else {
            memset((void *)sd_start, 0, UPDATE_PRIV_PARAM_LEN);
        }

        char io_port_stirng[4 * 3 + 1] = {0};
        sprintf(&io_port_stirng[0], "P%c%02d", TCFG_SD0_PORT_CLK / 16 + 'A', TCFG_SD0_PORT_CLK % 16);
        sprintf(&io_port_stirng[4], "P%c%02d", TCFG_SD0_PORT_CMD / 16 + 'A', TCFG_SD0_PORT_CMD % 16);
        sprintf(&io_port_stirng[8], "P%c%02d", TCFG_SD0_PORT_DA0 / 16 + 'A', TCFG_SD0_PORT_DA0 % 16);
        update_param_ext_fill(p, EXT_SD_IO_INFO, (u8 *)io_port_stirng, sizeof(io_port_stirng));
    }
#endif

#if (defined CONFIG_USB_UPDATE_ENABLE) && TCFG_UDISK_ENABLE
    if (up_type == USB_UPDATA) {
        printf("usb updata ");
        UPDATE_UDISK *usb_start = (UPDATE_UDISK *)p->parm_priv;
        memset((void *)usb_start, 0, UPDATE_PRIV_PARAM_LEN);
        void *usb = dev_update_get_parm(up_type);
        if (usb) {
            memcpy((void *)usb_start, usb, UPDATE_PRIV_PARAM_LEN);
#ifdef CONFIG_CPU_BR27
            usb_start->u.param.rx_ldo_trim = trim_get_usb_rx_ldo();
            usb_start->u.param.tx_ldo_trim = trim_get_usb_tx_ldo();
#endif
        }
    }
#endif

#ifdef CONFIG_UPDATE_JUMP_TO_MASK
    memcpy(p->file_patch, STRCHANGE(CONFIG_SD_LATCH_IO), 32);
#else
    memcpy(p->file_patch, updata_file_name, strlen(updata_file_name));
#endif

#if defined(CONFIG_UPDATE_MACHINE_NUM) && CONFIG_UPDATE_MUTIL_CPU_UART
    u8 machine_num[16] = {0};
    u32 len = update_get_machine_num((u8 *)machine_num, sizeof(machine_num));
    if (len && (len + 1) <= 16) {
        update_param_ext_fill(p, EXT_MUTIL_UPDATE_NAME, (u8 *)machine_num, len + 1); //多出一个0，便于strlen获取长度
    }
#endif
}

static void dev_update_before_jump_handle(int up_type)
{
#if CONFIG_UPDATE_JUMP_TO_MASK
    y_printf(">>>[test]:latch reset update\n");
    latch_reset();
#else
    cpu_reset();
#endif
}

static void dev_update_state_cbk(int type, u32 state, void *priv)
{
    update_ret_code_t *ret_code = (update_ret_code_t *)priv;

    switch (state) {
    case UPDATE_CH_EXIT:
        if ((0 == ret_code->stu) && (0 == ret_code->err_code)) {
            update_mode_api_v2(type,
                               dev_update_param_private_handle,
                               dev_update_before_jump_handle);
        } else {
            printf("update fail, cpu reset!!!\n");
            cpu_reset();
        }
        break;
    }
}

u16 dev_update_check(char *logo)
{
    if ((update_success_boot_check() == true) || !CONFIG_UPDATE_STORAGE_DEV_EN) {
        return UPDATA_NON;
    }

    __this->offset_addr = 0;
    __this->update_target = 0;

    struct __dev *dev = dev_manager_find_spec(logo, 0);
    if (dev) {
#if DEV_UPDATE_EN
        //<查找设备升级配置
        struct __update_dev_reg *parm = NULL;
        for (int i = 0; i < ARRAY_SIZE(update_dev_list); i++) {
            if (0 == strcmp(update_dev_list[i]->logo, logo)) {
                parm = (struct __update_dev_reg *)update_dev_list[i];
            }
        }
        if (parm == NULL) {
            printf("dev update without parm err!!!\n");
            return UPDATA_PARM_ERR;
        }

        //最新设备升级只支持fat32(省flash空间)，此处截断
        struct vfs_partition *part = fget_partition((const char *)dev_manager_get_root_path(dev));
        if (part) {
            // fs_type: 1为fat12 、2 为fat16 、3为fat32 、4为exfat
            if (part->fs_type != 3) {
                printf(">>>[test]:dev update only support fat32 !!!\n");
                return UPDATA_PARM_ERR;
            }
        } else {
            printf(">>>[test]:dev part err!!!\n");
            return UPDATA_PARM_ERR;
        }

        //<尝试按照路径打开升级文件
        char *updata_file = (char *)updata_file_name;
        if (*updata_file == '/') {
            updata_file ++;
        }
        memset(update_path, 0, sizeof(update_path));
        sprintf(update_path, "%s%s", dev_manager_get_root_path(dev), updata_file);
        printf("update_path: %s\n", update_path);
        FILE *fd = fopen(update_path, "r");
        if (!fd) {
            //没有升级文件， 继续跑其他解码相关的流程
            printf("open update file err!!!\n");
            return UPDATA_DEV_ERR;
        }
        strg_update_set_file_path_and_hdl(update_path, (void *)fd);

        update_mode_info_t info = {
            .type = parm->type,
            .state_cbk = dev_update_state_cbk,
            .p_op_api = &strg_dev_update_op,
            .task_en = 0,
        };

#if CONFIG_UPDATE_MUTIL_CPU_UART
        update_interactive_task_start(&info, dev_update_set_offset_addr, 0);
#else

#if (TCFG_UPDATE_UART_IO_EN) && (TCFG_UPDATE_UART_ROLE)
        uart_update_send_update_ready(update_path);
        while (get_uart_update_sta()) {
            os_time_dly(10);
        }
#else
        //进行升级
        app_active_update_task_init(&info);

#endif
#endif
#endif//DEV_UPDATE_EN
    }
    return UPDATA_READY;
}



