#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".fm_manage.data.bss")
#pragma data_seg(".fm_manage.data")
#pragma const_seg(".fm_manage.text.const")
#pragma code_seg(".fm_manage.text")
#endif
#include "app_config.h"
#include "system/includes.h"
#include "fm_manage.h"
#include "rda5807/RDA5807.h"
#include "bk1080/BK1080.h"
#include "qn8035/QN8035.h"
#include "fm_inside/fm_inside.h"
#include "asm/audio_linein.h"
#include "audio_config.h"
#include "linein_player.h"
/* #include "audio_dec/audio_dec_fm.h" */
#if TCFG_APP_FM_EN


#define LOG_TAG_CONST       APP_FM
#define LOG_TAG             "[APP_FM]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static FM_INTERFACE *t_fm_hdl = NULL;
static FM_INTERFACE *fm_hdl = NULL;
static struct _fm_dev_info *fm_dev_info;
static struct _fm_dev_platform_data *fm_dev_platform_data;
int linein_dec_open(u8 source, u32 sample_rate);
void linein_dec_close(void);

int fm_dev_init(void *_data)
{
    fm_dev_platform_data = (struct _fm_dev_platform_data *)_data;
    return 0;
}

u8 fm_dev_iic_write(u8 w_chip_id, u8 register_address, u8 *buf, u32 data_len)
{
    u8 ret = 1;
    iic_start(fm_dev_info->iic_hdl);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, w_chip_id)) {
        ret = 0;
        log_e("\n fm iic wr err 0\n");
        goto __gcend;
    }

    delay_nops(fm_dev_info->iic_delay);

    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, register_address)) {
        ret = 0;
        log_e("\n fm iic wr err 1\n");
        goto __gcend;
    }
    u8 *pbuf = buf;

    while (data_len--) {
        delay_nops(fm_dev_info->iic_delay);

        if (0 == iic_tx_byte(fm_dev_info->iic_hdl, *pbuf++)) {
            ret = 0;
            log_e("\n fm iic wr err 2\n");
            goto __gcend;
        }
    }

__gcend:
    iic_stop(fm_dev_info->iic_hdl);

    return ret;
}

u8 fm_dev_iic_readn(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len)
{
    u8 read_len = 0;

    iic_start(fm_dev_info->iic_hdl);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, (r_chip_id & 0x01) ? (r_chip_id - 1) : (r_chip_id))) {
        log_e("\n fm iic rd err 0\n");
        read_len = 0;
        goto __gdend;
    }


    delay_nops(fm_dev_info->iic_delay);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, register_address)) {
        log_e("\n fm iic rd err 1\n");
        read_len = 0;
        goto __gdend;
    }

    delay_nops(fm_dev_info->iic_delay);
    iic_start(fm_dev_info->iic_hdl);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, r_chip_id)) {
        log_e("\n fm iic rd err 2\n");
        read_len = 0;
        goto __gdend;
    }

    delay_nops(fm_dev_info->iic_delay);

    for (; data_len > 1; data_len--) {
        *buf++ = iic_rx_byte(fm_dev_info->iic_hdl, 1);
        read_len ++;
    }

    *buf = iic_rx_byte(fm_dev_info->iic_hdl, 0);

__gdend:

    iic_stop(fm_dev_info->iic_hdl);
    delay_nops(fm_dev_info->iic_delay);

    return read_len;
}


int fm_manage_check_online(void)
{
    /* printf("fm check online dev \n"); */
    ////先找外挂fm
    list_for_each_fm(t_fm_hdl) {
#if TCFG_FM_OUTSIDE_ENABLE
        t_fm_hdl->fm_iic_init((void *)fm_dev_platform_data);
#endif
        /* printf("find %x %x %s\n", t_fm_hdl, t_fm_hdl->read_id((void *)fm_dev_info), t_fm_hdl->logo); */
        if (t_fm_hdl->read_id((void *)fm_dev_platform_data) &&
            (memcmp(t_fm_hdl->logo, "fm_inside", strlen((char *)(t_fm_hdl->logo))))) {
            printf("fm find dev %s \n", t_fm_hdl->logo);
            return 0;
        }
    }
    /* printf("no ex fm dev !!!\n"); */

    list_for_each_fm(t_fm_hdl) {
#if TCFG_FM_OUTSIDE_ENABLE
        t_fm_hdl->fm_iic_init((void *)fm_dev_platform_data);
#endif
        /* printf("# find %x %x %s\n", t_fm_hdl, t_fm_hdl->read_id((void *)fm_dev_info), t_fm_hdl->logo); */
        if (t_fm_hdl->read_id((void *)fm_dev_platform_data) &&
            (!memcmp(t_fm_hdl->logo, "fm_inside", strlen((char *)(t_fm_hdl->logo))))) {
            printf("fm find dev %s \n", t_fm_hdl->logo);
            return 0;
        }
    }
    return -1;
}

int get_fm_manage_hd(void)
{
    ////先找外挂fm
    list_for_each_fm(t_fm_hdl) {
        /* printf("find %x %x %s\n", t_fm_hdl, t_fm_hdl->read_id((void *)fm_dev_info), t_fm_hdl->logo); */
#if TCFG_FM_OUTSIDE_ENABLE
        t_fm_hdl->fm_iic_init((void *)fm_dev_info);
#endif
        if (t_fm_hdl->read_id((void *)fm_dev_info) &&
            (memcmp(t_fm_hdl->logo, "fm_inside", strlen((char *)(t_fm_hdl->logo))))) {
            if (fm_get_device_en((char *)t_fm_hdl->logo)) {
                fm_hdl = t_fm_hdl;
                y_printf(">>>> fm find dev %s \n", t_fm_hdl->logo);
                return 0;
            }
        }
    }
    /* printf("no ex fm dev !!!\n"); */

    list_for_each_fm(t_fm_hdl) {
        /* printf("# find %x %x %s\n", t_fm_hdl, t_fm_hdl->read_id((void *)fm_dev_info), t_fm_hdl->logo); */
#if TCFG_FM_OUTSIDE_ENABLE
        t_fm_hdl->fm_iic_init((void *)fm_dev_info);
#endif
        if (t_fm_hdl->read_id((void *)fm_dev_info) &&
            (!memcmp(t_fm_hdl->logo, "fm_inside", strlen((char *)(t_fm_hdl->logo))))) {
            if (fm_get_device_en((char *)t_fm_hdl->logo)) {
                fm_hdl = t_fm_hdl;
                y_printf(">>>> fm find dev %s \n", t_fm_hdl->logo);
                return 0;
            }
        }
    }
    r_printf(">>> %s, %d, FM device no found!\n", __func__, __LINE__);
    return -1;
}

int fm_manage_init()
{
    fm_dev_info = (struct _fm_dev_info *)malloc(sizeof(struct _fm_dev_info));
    if (fm_dev_info == NULL) {
        printf("fm_dev_init info malloc err!");
        return -1;
    }

    memset(fm_dev_info, 0x00, sizeof(struct _fm_dev_info));

    if (fm_dev_platform_data == NULL) {
        r_printf("Error, %s, %d, fm_dev_platform_data is NULL !\n", __func__, __LINE__);
        return -1;
    }
    fm_dev_info->iic_hdl = fm_dev_platform_data->iic_hdl;
    fm_dev_info->iic_delay = fm_dev_platform_data->iic_delay;
    if (!fm_hdl) {
        if (get_fm_manage_hd()) {
            r_printf("\n fm manager could not find dev \n");
            return -1;
        }
    }

    //找到的logo放在 fm_dev_info->logo
    memcpy(fm_dev_info->logo, fm_hdl->logo, strlen((char *)(fm_hdl->logo)));
    fm_file_set_logo((char *)(fm_dev_info->logo), strlen((char *)(fm_hdl->logo)));
    printf("fm_manage found dev %s\n", fm_dev_info->logo);

    if (fm_hdl) {
        fm_hdl->init((void *)fm_dev_info);
    }
    return 0;
}



int fm_manage_start()
{
    if (!fm_hdl || !fm_dev_info) {
        printf("fm_manage could not find dev\n");
        return -1;
    }
    if (memcmp(fm_dev_info->logo, "fm_inside", strlen((char *)(fm_dev_info->logo)))) {//不开启内部收音开启linein
        //外挂FM
    } else {
        // 内置FM
    }
    return 0;
}

static u16 cur_fmrx_freq = 0;
bool fm_manage_set_fre(u16 fre)
{
    if (fm_hdl) {
        cur_fmrx_freq = fre;
        return fm_hdl->set_fre((void *)fm_dev_info, fre);
    }
    return 0;
}

u16 fm_manage_get_fre()
{
    if (fm_hdl) {
        return  cur_fmrx_freq;
    }
    return 0;
}

void fm_manage_close(void)
{
    if (fm_hdl) {
        fm_hdl->close((void *)fm_dev_info);
#if TCFG_FM_OUTSIDE_ENABLE
        fm_hdl->fm_iic_uninit((void *)fm_dev_info);
#endif
        fm_hdl = NULL;
    }

    if (fm_dev_info != NULL) {
        free(fm_dev_info);
        fm_dev_info = NULL;
    }
}


void fm_manage_mute(u8 mute)
{
    if (fm_hdl) {
        fm_hdl->mute((void *)fm_dev_info, mute);
    }
}

#endif

