#ifndef	_USB_H_
#define _USB_H_
#include "typedef.h"
#include "generic/ioctl.h"
#include "asm/usb_hw.h"


#ifndef USB_DIR_OUT
#define USB_DIR_OUT			0		/* to device */
#endif
#ifndef USB_DIR_IN
#define USB_DIR_IN			0x80		/* to host */
#endif

#define     EP0_SETUP_LEN           0x40




struct usb_ep_addr_t {
    u32 ep0_addr;
    u32 ep_usage;
    u32 ep_taddr[4];
    u32 ep_dual_taddr[4];
    u32 ep_raddr[4];
    u32 ep_dual_raddr[4];
    u32 ep_tsize[4];
    u32 ep_rsize[4];
} __attribute__((aligned(4)));

typedef struct {
    u32 stamp;
    u32 left_time;
} usb_ot_t;


typedef u8 usb_dev;

u32 usb_phy_status(const usb_dev id);
u32 usb_sie_status(const usb_dev id);
u32 usb_check_dpo(const usb_dev id);
u32 usb_check_dmo(const usb_dev id);
u32 usb_read_dp_se(const usb_dev id);
u32 usb_read_dm_se(const usb_dev id);
u16 usb_read_sofframe(const usb_dev id);
void usb_sie_enable(const usb_dev usb_id);
void usb_sie_disable(const usb_dev id);
void usb_write_ep_cnt(const usb_dev usb_id, u32 ep, u32 len);
u32 usb_g_dev_status(const usb_dev usb_id);
u32 usb_h_dev_status(const usb_dev usb_id);
void usb_set_low_speed(const usb_dev usb_id, u8 flag);
void usb_write_ep0(const usb_dev usb_id, const u8 *ptr, u32 len);
void usb_read_ep0(const usb_dev usb_id, u8 *ptr, u32 len);
u32 usb_get_dma_size(const usb_dev usb_id, u32 ep);
void usb_set_dma_tsize(const usb_dev usb_id, u32 ep, u32 size);
void usb_set_dma_rsize(const usb_dev usb_id, u32 ep, u32 size);
void *usb_get_dma_taddr(const usb_dev usb_id, u32 ep);
void usb_set_dma_taddr(const usb_dev usb_id, u32 ep, void *ptr);
void *usb_get_dma_raddr(const usb_dev usb_id, u32 ep);
void usb_set_dma_raddr(const usb_dev usb_id, u32 ep, void *ptr);
void usb_set_dma_dual_raddr(const usb_dev usb_id, u32 ep, void *ptr);
void usb_write_power(const usb_dev usb_id, u32 value);
u32 usb_read_power(const usb_dev usb_id);
u32 usb_read_devctl(const usb_dev usb_id);
void usb_write_devctl(const usb_dev usb_id, u32 value);
u32 usb_read_csr0(const usb_dev usb_id);
void usb_write_csr0(const usb_dev usb_id, u32 csr0);
void usb_ep0_ClrRxPktRdy(const usb_dev usb_id);
void usb_ep0_TxPktEnd(const usb_dev usb_id);
void usb_ep0_RxPktEnd(const usb_dev usb_id);
void usb_ep0_Set_Stall(const usb_dev usb_id);
u32 usb_read_count0(const usb_dev usb_id);
void usb_read_intre(const usb_dev usb_id,
                    u32 *const intr_usbe,
                    u32 *const intr_txe,
                    u32 *const intr_rxe);

void usb_read_intr(const usb_dev usb_id,
                   u32 *const intr_usb,
                   u32 *const intr_tx,
                   u32 *const intr_rx);
void usb_write_intr_usbe(const usb_dev usb_id, u32 intr_usbe);
void usb_set_intr_txe(const usb_dev usb_id, const u32 ep);
void usb_clr_intr_txe(const usb_dev usb_id, const u32 ep);
void usb_set_intr_rxe(const usb_dev usb_id, const u32 ep);
void usb_clr_intr_rxe(const usb_dev usb_id, const u32 ep);
void usb_write_faddr(const usb_dev usb_id, u32 addr);
void usb_write_txcsr(const usb_dev usb_id, const u32 ep, u32 txcsr);
u32 usb_read_txcsr(const usb_dev usb_id, const u32 ep);
void usb_write_rxcsr(const usb_dev usb_id, const u32 ep, u32 rxcsr);
u32 usb_read_rxcsr(const usb_dev usb_id, const u32 ep);
void usb_write_rxmaxp(const usb_dev usb_id, const u32 ep, u32 value);
void usb_write_txmaxp(const usb_dev usb_id, const u32 ep, u32 value);
void usb_write_rxtype(const usb_dev usb_id, const u32 ep, u32 value);
void usb_write_txtype(const usb_dev usb_id, const u32 ep, u32 value);
int usb_read_rxcount(const usb_dev usb_id, u32 ep);
u32 usb_g_ep_read64byte_fast(const usb_dev usb_id, const u32 ep, u8 *ptr, u32 len);
u32 usb_g_ep_read(const usb_dev usb_id, const u32 ep, u8 *ptr, u32 len, u32 block);
u32 usb_g_ep_write(const usb_dev usb_id, u32 ep, const u8 *ptr, u32 len);
u32 usb_g_ep_config(const usb_dev usb_id, u32 ep, u32 type, u32 ie, u8 *ptr, u32 dma_size);
void usb_g_sie_init(const usb_dev usb_id);
void usb_g_hold(const usb_dev usb_id);
u32 usb_get_ep_num(const usb_dev usb_id, u32 ep_dir, u32 type);
u32 usb_free_ep_num(const usb_dev usb_id, u32 ep);
u32 usb_h_ep_config(const usb_dev usb_id, u32 ep, u32 type, u32 ie, u32 interval, u8 *ptr, u32 dma_size);
u32 usb_h_ep_write(const usb_dev usb_id, u8 host_ep, u16 txmaxp, u8 target_ep, const u8 *ptr, u32 len, u32 xfer);
int usb_h_ep_write_async(const usb_dev id, u8 host_ep, u16 txmaxp, u8 target_ep, const u8 *ptr, u32 len, u32 xfer, u32 kstart);
u32 usb_h_ep_read(const usb_dev usb_id, u8 host_ep, u16 rxmaxp, u8 target_ep, u8 *ptr, u32 len, u32 xfer);
int usb_h_ep_read_async(const usb_dev id, u8 host_ep, u8 target_ep, u8 *ptr, u32 len, u32 xfer, u32 kstart);
void usb_h_sie_init(const usb_dev usb_id);
void usb_h_sie_close(const usb_dev usb_id);
u32 usb_h_chirp_and_reset(const usb_dev id, u32 reset_delay, u32 timeout);
void usb_h_sie_reset(const usb_dev usb_id);
void usb_hotplug_disable(const usb_dev usb_id);
void usb_hotplug_enable(const usb_dev usb_id, u32 mode);
void usb_pdchkdp_disable(const usb_dev usb_id);
void usb_pdchkdp_enable(const usb_dev usb_id);
void usb_sie_close(const usb_dev usb_id);
void usb_sie_close_all(void);
void usb_var_init(const usb_dev usb_id, void *ptr);
void usb_var_release(const usb_dev usb_id);
void usb_enable_ep(const usb_dev usb_id, u32 eps);
void usb_disable_ep(const usb_dev usb_id, u32 eps);
u32 usb_get_ep_status(const usb_dev usb_id, u32 epx);
void usb_sofie_enable(const usb_dev id);
void usb_sofie_disable(const usb_dev id);
u32 usb_read_sofpnd(const usb_dev id);
void usb_sof_clr_pnd(const usb_dev id);
void usb_ep0_Set_ignore(const usb_dev id, u32 addr);
void usb_recover_io_status(const usb_dev id);
void usb_write_rxinterval(const usb_dev id, const u32 ep, u32 value);
void usb_write_txinterval(const usb_dev id, const u32 ep, u32 value);
void usb_write_txfuncaddr(const usb_dev id, const u32 ep, const u32 devnum);
void usb_write_rxfuncaddr(const usb_dev id, const u32 ep, const u32 devnum);
void usb_h_force_reset(const usb_dev usb_id);
void usb_lowpower_enter_sleep(void);
void usb_lowpower_exit_sleep(void);


#endif
