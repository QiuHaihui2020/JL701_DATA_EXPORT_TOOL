#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "usb_config.h"
#include "usb/scsi.h"
#include "irq.h"
#include "init.h"
#include "gpio.h"
#include "timer.h"
#include "app_config.h"
#include "lbuf.h"

#ifdef CONFIG_ADAPTER_ENABLE
#include "adapter_usb_hid.h"
#endif//CONFIG_ADAPTER_ENABLE

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define     SET_INTERRUPT   ___interrupt


#define     MAX_EP_TX   5
#define     MAX_EP_RX   5

static usb_interrupt usb_interrupt_tx[USB_MAX_HW_NUM][MAX_EP_TX];
static usb_interrupt usb_interrupt_rx[USB_MAX_HW_NUM][MAX_EP_RX];
static usb_sof_ep_hander usb_sof_ep_tx[USB_MAX_HW_NUM][MAX_EP_TX];
static usb_sof_ep_hander usb_sof_ep_rx[USB_MAX_HW_NUM][MAX_EP_RX];

struct usb_config_var_t {
    u8 usb_setup_buffer[USB_SETUP_SIZE];
    struct usb_ep_addr_t usb_ep_addr;
    struct usb_setup_t usb_setup;
};
static struct usb_config_var_t *usb_config_var = {NULL};

#if USB_MALLOC_ENABLE
#else
static struct usb_config_var_t _usb_config_var SEC(.usb.data.bss.exchange);
#endif

extern void *usb_epbuf_alloc(const usb_dev usb_id, u32 ep, u32 size);
extern void usb_epbuf_free(const usb_dev usb_id, void *buf);

__attribute__((always_inline_when_const_args))
void *usb_alloc_ep_dmabuffer(const usb_dev usb_id, u32 ep, u32 dma_size)
{
    u8 *ep_buffer = NULL;

    //传进去的ep统一映射成bit7为1是rx，方便主从机共用函数
    ep_buffer = usb_epbuf_alloc(usb_id, ep ^ USB_DIR_IN, dma_size);
    ASSERT(ep_buffer, "%s() ep_buffer = NULL!!!, usb_id = %d, ep = %x, dma_size = %d\n", __func__, usb_id, ep, dma_size);

    log_info("usb%d slave, ep = %x, dma_size = %d, ep_buffer = %x\n", usb_id, ep, dma_size, (u32)ep_buffer);

    return ep_buffer;
}

void usb_free_ep_dmabuffer(const usb_dev usb_id, void *buf)
{
    usb_epbuf_free(usb_id, buf);
    log_info("usb%d slave free ep buffer %x\n", usb_id, (u32)buf);
}


static u8 sleep_flag = 0;
u8 get_sleep_flag()
{
    return sleep_flag ;
}

void set_sleep_flag(u8 sl_flag)
{
    sleep_flag = sl_flag ;
}

static void usb_resume_sign(void *priv)
{
    usb_dev usb_id = usb_device2id(priv);
    u32 reg = usb_read_power(usb_id);
    usb_write_power(usb_id, reg | BIT(2));//send resume
    os_time_dly(2);//10ms~20ms
    usb_write_power(usb_id, reg);//clean resume
}

static u8 usb_remote_wakeup_flag = USB_READY;//0:初始状态或suspend  1:从机已发送wakeup  2:主机已被唤醒
static u32 usb_remote_wakeup_cnt = 0;
void usb_remote_wakeup_detect(void *priv)
{
    if (usb_remote_wakeup_cnt == 0) {
        usb_remote_wakeup_flag = USB_SUSPEND;
        log_info("Wakeup fail!!! no SOF packet receive!\n");
    }
    if (usb_remote_wakeup_cnt > USB_REMOTE_WAKEUP_TIMEOUT_DETECT_TIMES - 200) {
        usb_remote_wakeup_flag = USB_READY;
        log_info("Receive %d SOF packet, USB ready!\n", usb_remote_wakeup_cnt);
    } else {
        usb_remote_wakeup_flag = USB_RESUME_WAIT;
        log_info("Receive %d SOF packet, please increase USB_REMOTE_WAKEUP_TIMEOUT_DETECT_TIMES\n", usb_remote_wakeup_cnt);
    }
}
void usb_remote_wakeup(const usb_dev usb_id)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);
    if (usb_device->bRemoteWakup) {
        sys_timeout_add(usb_device, usb_resume_sign, 1);
    }
    usb_remote_wakeup_flag = USB_RESUME_WAIT;
    usb_remote_wakeup_cnt = 0;
    log_info("slave remote_wakeup host signal has been sent");
    usb_sof_isr_reg(usb_id, 3, 0);
    sys_timeout_add(usb_device, usb_remote_wakeup_detect, USB_REMOTE_WAKEUP_TIMEOUT_DETECT_TIMES);
}
void usb_phy_resume(const usb_dev usb_id)
{
    usb_iomode(0);

    struct usb_device_t *usb_device = usb_id2device(usb_id);
    usb_write_faddr(usb_id, usb_device->baddr);

    if (usb_device->baddr == 0) {
        usb_device->bDeviceStates = USB_DEFAULT;
    } else {
        usb_device->bDeviceStates = USB_CONFIGURED;
    }

    usb_otg_resume(usb_id);
}

void usb_phy_suspend(const usb_dev usb_id)
{
    gpio_set_mode(PORTUSB, PORT_PIN_0, PORT_INPUT_PULLUP_10K); //dp
    /* musb_read_usb(0, MUSB_INTRUSB); */

    usb_otg_suspend(usb_id, OTG_KEEP_STATE);
}

u32 usb_get_suspend_resume_status(const usb_dev usb_id)
{
    switch (usb_remote_wakeup_flag) {
    case USB_READY:
        //log_info("USB READY\n");
        putchar('R');
        break;
    case USB_SUSPEND:
        log_info("USB SUSPEND\n");
        break;
    case USB_RESUME_WAIT:
        log_info("USB remote_wakeup send, RESUME WAIT\n");
        break;
    case USB_RESUME_OK://保留状态，未使用
        log_info("USB RESUME OK\n");
        break;
    default:
        break;
    }
    return usb_remote_wakeup_flag;
}

void usb_isr(const usb_dev usb_id)
{
    u32 intr_usb, intr_usbe;
    u32 intr_tx, intr_txe;
    u32 intr_rx, intr_rxe;

    /* __asm__ volatile("ssync"); */
    usb_read_intr(usb_id, &intr_usb, &intr_tx, &intr_rx);
    usb_read_intre(usb_id, &intr_usbe, &intr_txe, &intr_rxe);
    struct usb_device_t *usb_device = usb_id2device(usb_id);

    intr_usb &= intr_usbe;
    intr_tx &= intr_txe;
    intr_rx &= intr_rxe;

    if (intr_usb & INTRUSB_SUSPEND) {
        log_error("usb suspend");
        usb_remote_wakeup_flag = USB_SUSPEND;
        set_sleep_flag(1);
#if USB_SUSPEND_RESUME
        usb_phy_suspend(usb_id);
#endif

#if USB_SUSPEND_RESUME_SYSTEM_NO_SLEEP
        printf("\n NULL \n");
#endif
    }
    if (intr_usb & INTRUSB_RESET_BABBLE) {
        log_error("usb reset");
        usb_reset_interface(usb_device);
#if TCFG_FUSB_PLL_TRIM
        log_info("FUSB_PLL_AUTO_TRIM RUN\n");
        /* fusb_pll_trim(USB_TRIM_HAND, 10); */
        fusb_pll_trim(USB_TRIM_AUTO, 10);
#endif

#if USB_SUSPEND_RESUME || USB_SUSPEND_RESUME_SYSTEM_NO_SLEEP
        u32 reg = usb_read_power(usb_id);
        usb_write_power(usb_id, (reg | INTRUSB_SUSPEND | INTRUSB_RESUME));//enable suspend resume
#endif
        usb_reset_host_type(usb_id);
    }

    if (intr_usb & INTRUSB_RESUME) {
        log_error("usb resume");
#if USB_SUSPEND_RESUME
        usb_phy_resume(usb_id);
#endif
    }

    if (intr_tx & BIT(0)) {
        if (usb_interrupt_rx[usb_id][0]) {
            usb_interrupt_rx[usb_id][0](usb_device, 0);
        } else {
#if USB_SUSPEND_RESUME || USB_SUSPEND_RESUME_SYSTEM_NO_SLEEP
            if (usb_remote_wakeup_flag == USB_RESUME_WAIT) {
                if (usb_device->bsetup_phase == USB_EP0_STAGE_SETUP) {
                    usb_remote_wakeup_flag = USB_READY;
                    log_info("receive setup packet");
                }
            }
#endif
            usb_control_transfer(usb_device);
        }
    }

    for (int i = 1; i < MAX_EP_TX; i++) {
        if (intr_tx & BIT(i)) {
            if (usb_interrupt_tx[usb_id][i]) {
                usb_interrupt_tx[usb_id][i](usb_device, i);
            }
        }
    }

    for (int i = 1; i < MAX_EP_RX; i++) {
        if (intr_rx & BIT(i)) {
            if (usb_interrupt_rx[usb_id][i]) {
                usb_interrupt_rx[usb_id][i](usb_device, i);
            }
        }
    }
    __asm__ volatile("csync");
}

void usb_sof_isr(const usb_dev usb_id)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);
    u32 frame = usb_read_sofframe(usb_id);
    usb_sof_clr_pnd(usb_id);
    static u32 sof_count = 0;
#if USB_SUSPEND_RESUME || USB_SUSPEND_RESUME_SYSTEM_NO_SLEEP
    usb_remote_wakeup_cnt++;
#endif
#if 0
    if ((sof_count++ % 1000) == 0) {
        log_d("sof 1s isr frame:%d", usb_read_sofframe(usb_id));
    }
#endif
    for (int i = 0; i < MAX_EP_TX; i++) {
        if (usb_sof_ep_tx[usb_id][i]) {
            usb_sof_ep_tx[usb_id][i](usb_device, i, frame);
        }
    }
    for (int i = 0; i < MAX_EP_RX; i++) {
        if (usb_sof_ep_rx[usb_id][i]) {
            usb_sof_ep_rx[usb_id][i](usb_device, i, frame);
        }
    }
}
void usb_suspend_check(void *p)
{
    usb_dev usb_id = (usb_dev)p;

    static u16 sof_frame = 0;
    u16 frame = usb_read_sofframe(usb_id);// sof frame 不更新，则usb进入断开或者suspend状态
    if (frame == sof_frame) {
        usb_phy_suspend(usb_id);
    }
    sof_frame = frame;
}

SET_INTERRUPT
void usb0_g_isr()
{
    usb_isr(0);
}
SET_INTERRUPT
void usb0_sof_isr()
{
    usb_sof_isr(0);
}

#if USB_MAX_HW_NUM == 2
SET_INTERRUPT
void usb1_g_isr()
{
    usb_isr(1);
}
SET_INTERRUPT
void usb1_sof_isr()
{
    usb_sof_isr(1);
}
#endif
__attribute__((always_inline_when_const_args))
u32 usb_g_set_intr_hander(const usb_dev usb_id, u32 ep, usb_interrupt hander)
{
    if (ep & USB_DIR_IN) {
        usb_interrupt_tx[usb_id][ep & 0xf] = hander;
    } else {
        usb_interrupt_rx[usb_id][ep] = hander;
    }
    return 0;
}
__attribute__((always_inline_when_const_args))
u32 usb_g_set_sof_hander(const usb_dev usb_id, u32 ep, usb_sof_ep_hander hander)
{
    if (ep & USB_DIR_IN) {
        usb_sof_ep_tx[usb_id][ep & 0xf] = hander;
    } else {
        usb_sof_ep_rx[usb_id][ep] = hander;
    }
    return 0;
}
void usb_g_isr_reg(const usb_dev usb_id, u8 priority, u8 cpu_id)
{
    if (usb_id == 0) {
        request_irq(IRQ_USB_CTRL_IDX, priority, usb0_g_isr, cpu_id);
    } else {
#if USB_MAX_HW_NUM == 2
        request_irq(IRQ_USB1_CTRL_IDX, priority, usb1_g_isr, cpu_id);
#endif
    }
}
void usb_sof_isr_reg(const usb_dev usb_id, u8 priority, u8 cpu_id)
{
    if (usb_id == 0) {
        request_irq(IRQ_USB_SOF_IDX, priority, usb0_sof_isr, cpu_id);
    } else {
#if USB_MAX_HW_NUM == 2
        request_irq(IRQ_USB1_SOF_IDX, priority, usb1_sof_isr, cpu_id);
#endif
    }
    usb_sofie_enable(usb_id);
}
u32 usb_config(const usb_dev usb_id)
{
    memset(usb_interrupt_rx[usb_id], 0, sizeof(usb_interrupt_rx[usb_id]));
    memset(usb_interrupt_tx[usb_id], 0, sizeof(usb_interrupt_tx[usb_id]));
    memset(usb_sof_ep_rx[usb_id], 0, sizeof(usb_interrupt_rx[usb_id]));
    memset(usb_sof_ep_tx[usb_id], 0, sizeof(usb_interrupt_tx[usb_id]));

    if (!usb_config_var) {
#if USB_MALLOC_ENABLE
        usb_config_var = (struct usb_config_var_t *)zalloc(sizeof(struct usb_config_var_t));
        if (!usb_config_var) {
            return -1;
        }
#else
        memset(&_usb_config_var, 0, sizeof(_usb_config_var));
        usb_config_var = &_usb_config_var;
#endif
    }
    log_debug("zalloc: usb_config_var = %x\n", usb_config_var);

    usb_var_init(usb_id, &(usb_config_var->usb_ep_addr));
    usb_setup_init(usb_id, &(usb_config_var->usb_setup), usb_config_var->usb_setup_buffer);
    return 0;
}

u32 usb_release(const usb_dev usb_id)
{
    log_debug("free zalloc: usb_config_var = %x\n", usb_id, usb_config_var);
    usb_var_init(usb_id, NULL);
    usb_setup_init(usb_id, NULL, NULL);
#if USB_MALLOC_ENABLE
    if (usb_config_var) {
        log_debug("free: usb_config_var = %x\n", usb_config_var);
        free(usb_config_var);
    }
#endif

    usb_config_var = NULL;

    return 0;
}
