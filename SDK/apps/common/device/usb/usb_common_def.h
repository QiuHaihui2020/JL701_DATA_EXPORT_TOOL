#ifndef __USB_COMMON_DEFINE_H__
#define __USB_COMMON_DEFINE_H__

///<<<注意此文件不要放函数声明, 只允许宏定义, 并且差异化定义可以根据需求在对应板卡中重新定义, 除非新增，否则不要直接修改这里
///<<<注意此文件不要放函数声明, 只允许宏定义, 并且差异化定义可以根据需求在对应板卡中重新定义, 除非新增，否则不要直接修改这里
///<<<注意此文件不要放函数声明, 只允许宏定义, 并且差异化定义可以根据需求在对应板卡中重新定义, 除非新增，否则不要直接修改这里
//


/// board文件没有定义的宏,在这里定义,防止编译报warning
#ifndef TCFG_PC_ENABLE
#define TCFG_PC_ENABLE  0
#endif
#ifndef TCFG_USB_HOST_ENABLE
#define TCFG_USB_HOST_ENABLE  0
#endif
#ifndef TCFG_OTG_USB_DEV_EN
#define TCFG_OTG_USB_DEV_EN         0b01//USB0 = BIT(0)  USB1 = BIT(1)
#endif
#ifndef TCFG_USB_APPLE_DOCK_EN
#define TCFG_USB_APPLE_DOCK_EN 0
#endif
#ifndef TCFG_CHARGE_ENABLE
#define TCFG_CHARGE_ENABLE     0
#endif
#ifndef TCFG_USB_PORT_CHARGE
#define TCFG_USB_PORT_CHARGE   0
#endif
#ifndef TCFG_USB_MIC_ECHO_ENABLE
#define TCFG_USB_MIC_ECHO_ENABLE   0
#endif
#ifndef TCFG_USB_MIC_DATA_FROM_MICEFFECT
#define TCFG_USB_MIC_DATA_FROM_MICEFFECT   0
#endif
#ifndef TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
#define TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0   0
#endif
#ifndef TCFG_ONLY_PC_ENABLE     //只有pc模式
#define TCFG_ONLY_PC_ENABLE 0
#endif
#ifndef TCFG_TYPE_C_ENABLE      //应用于type-c场景
#define TCFG_TYPE_C_ENABLE  0
#endif
#ifndef TCFG_USB_CDC_BACKGROUND_RUN
#define TCFG_USB_CDC_BACKGROUND_RUN 0
#endif
#ifndef TCFG_FUSB_PLL_TRIM
#define TCFG_FUSB_PLL_TRIM   0
#endif
#ifndef USB_MEM_USE_OVERLAY
#define USB_MEM_USE_OVERLAY  0
#endif
#ifndef USB_EPBUF_ALLOC_USE_LBUF
#define USB_EPBUF_ALLOC_USE_LBUF    0
#endif
#ifndef USB_ROOT2
#define USB_ROOT2   0
#endif
#ifndef USB_MALLOC_ENABLE
#define USB_MALLOC_ENABLE           1
#endif
#ifndef USB_HOST_ASYNC
#define USB_HOST_ASYNC              1
#endif
#ifndef USB_H_MALLOC_ENABLE
#define USB_H_MALLOC_ENABLE         1
#endif


/********************************/

#if TCFG_CHARGE_ENABLE && TCFG_USB_PORT_CHARGE
#define TCFG_OTG_MODE_CHARGE                (OTG_CHARGE_MODE)
#else
#define TCFG_OTG_MODE_CHARGE                0
#endif

#if (TCFG_PC_ENABLE)
#define TCFG_PC_UPDATE                      1
#define TCFG_OTG_MODE_SLAVE                 (OTG_SLAVE_MODE)
#else
#define TCFG_PC_UPDATE                      0
#define TCFG_OTG_MODE_SLAVE                 0
#endif

#if (TCFG_USB_HOST_ENABLE)
#define TCFG_OTG_MODE_HOST                  (OTG_HOST_MODE)
#else
#define TCFG_OTG_MODE_HOST                  0
#endif

#if TCFG_USB_HOST_ENABLE
#ifndef TCFG_USB_HOST_MOUNT_RETRY
#define TCFG_USB_HOST_MOUNT_RETRY           3
#endif
#ifndef TCFG_USB_HOST_MOUNT_RESET
#define TCFG_USB_HOST_MOUNT_RESET           40
#endif
#ifndef TCFG_USB_HOST_MOUNT_TIMEOUT
#define TCFG_USB_HOST_MOUNT_TIMEOUT         50
#endif
#ifndef TCFG_UDISK_ENABLE
#define TCFG_UDISK_ENABLE                   1
#endif
#ifndef TCFG_HOST_AUDIO_ENABLE
#define TCFG_HOST_AUDIO_ENABLE              0
#endif
#ifndef TCFG_HID_HOST_ENABLE
#define TCFG_HID_HOST_ENABLE                0
#endif
#ifndef TCFG_AOA_ENABLE
#define TCFG_AOA_ENABLE                     0
#endif
#ifndef TCFG_ADB_ENABLE
#define TCFG_ADB_ENABLE                     0
#endif

#else
#undef  TCFG_UDISK_ENABLE
#define TCFG_UDISK_ENABLE                   0
#undef  TCFG_HOST_AUDIO_ENABLE
#define TCFG_HOST_AUDIO_ENABLE              0
#undef  TCFG_HID_HOST_ENABLE
#define TCFG_HID_HOST_ENABLE                0
#undef  TCFG_AOA_ENABLE
#define TCFG_AOA_ENABLE                     0
#undef  TCFG_ADB_ENABLE
#define TCFG_ADB_ENABLE                     0
#endif

#if TCFG_PC_ENABLE
#define TCFG_USB_SLAVE_ENABLE               1
#ifndef TCFG_USB_SLAVE_MSD_ENABLE
#define TCFG_USB_SLAVE_MSD_ENABLE           0
#endif
#ifndef TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
#define TCFG_USB_SLAVE_AUDIO_SPK_ENABLE     0
#endif
#ifndef TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
#define TCFG_USB_SLAVE_AUDIO_MIC_ENABLE     0
#endif
#ifndef TCFG_USB_SLAVE_HID_ENABLE
#define TCFG_USB_SLAVE_HID_ENABLE           0
#endif
#ifndef TCFG_USB_SLAVE_CDC_ENABLE
#define TCFG_USB_SLAVE_CDC_ENABLE           0
#endif
#ifndef TCFG_USB_CUSTOM_HID_ENABLE
#define TCFG_USB_CUSTOM_HID_ENABLE          0
#endif
#ifndef TCFG_USB_SLAVE_MIDI_ENABLE
#define TCFG_USB_SLAVE_MIDI_ENABLE          0
#endif
#ifndef TCFG_USB_SLAVE_PRINTER_ENABLE
#define TCFG_USB_SLAVE_PRINTER_ENABLE       0
#endif

#else  /* TCFG_PC_ENABLE == 0*/
#define TCFG_USB_SLAVE_ENABLE               0
#undef  TCFG_USB_SLAVE_MSD_ENABLE
#define TCFG_USB_SLAVE_MSD_ENABLE           0
#undef  TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
#define TCFG_USB_SLAVE_AUDIO_SPK_ENABLE     0
#undef  TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
#define TCFG_USB_SLAVE_AUDIO_MIC_ENABLE     0
#undef  TCFG_USB_SLAVE_HID_ENABLE
#define TCFG_USB_SLAVE_HID_ENABLE           0
#undef  TCFG_USB_SLAVE_CDC_ENABLE
#define TCFG_USB_SLAVE_CDC_ENABLE           0
#undef  TCFG_USB_CUSTOM_HID_ENABLE
#define TCFG_USB_CUSTOM_HID_ENABLE          0
#undef  TCFG_USB_SLAVE_MIDI_ENABLE
#define TCFG_USB_SLAVE_MIDI_ENABLE          0
#undef  TCFG_USB_SLAVE_PRINTER_ENABLE
#define TCFG_USB_SLAVE_PRINTER_ENABLE       0
#endif

#define TCFG_OTG_SLAVE_ONLINE_CNT           3
#define TCFG_OTG_SLAVE_OFFLINE_CNT          2

#define TCFG_OTG_HOST_ONLINE_CNT            2
#define TCFG_OTG_HOST_OFFLINE_CNT           3

#ifndef TCFG_OTG_MODE
#define TCFG_OTG_MODE                       (TCFG_OTG_MODE_HOST|TCFG_OTG_MODE_SLAVE|TCFG_OTG_MODE_CHARGE)
#endif

#define TCFG_OTG_DET_INTERVAL               250

#if (TCFG_USB_CDC_BACKGROUND_RUN)

#undef  TCFG_USB_SLAVE_CDC_ENABLE
#define TCFG_USB_SLAVE_CDC_ENABLE           1
#undef  TCFG_USB_SLAVE_ENABLE
#define TCFG_USB_SLAVE_ENABLE               1

#if ((TCFG_OTG_MODE & OTG_SLAVE_MODE) == 0)
#undef TCFG_OTG_MODE
#define TCFG_OTG_MODE                       (TCFG_OTG_MODE_HOST|OTG_SLAVE_MODE|TCFG_OTG_MODE_CHARGE)
#endif

#endif

#endif
