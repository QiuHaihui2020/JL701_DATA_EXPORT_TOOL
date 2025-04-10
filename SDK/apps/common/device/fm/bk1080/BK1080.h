/*--------------------------------------------------------------------------*/
/**@file     bk1080.h
   @brief    BK1080收音
   @details
   @author
   @date   2011-3-30
   @note
*/
/*----------------------------------------------------------------------------*/
#ifndef _BK_1080_H_
#define _BK_1080_H_

#if (TCFG_FM_BK1080_ENABLE && TCFG_APP_FM_EN)


#define XTAL_CLOCK			1
#define CHIP_DEV_ID 		0x80


u8 bk1080_init(void *priv);
void bk1080_setfreq(u16 curFreq);
u8 bk1080_set_fre(void *priv, u16 freq);
u8 bk1080_powerdown(void *priv);
u8 bk1080_mute(void *priv, u8 flag);
u8 bk1080_read_id(void *priv);
void bk1080_setch(u8 db);

extern int fm_get_device_en(char *logo);

#endif

#endif		/*  _BK_1080_H_ */
