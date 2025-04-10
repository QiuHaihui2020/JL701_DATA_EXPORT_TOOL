#ifndef HDMI_CEC_API_H
#define HDMI_CEC_API_H

#include "typedef.h"

void hdmi_cec_init(u8 cec_port, u8 cec_det_en);
int hdmi_cec_send_volume(u32 vol);
void hdmi_cec_close(void);

u32 hdmi_ddc_get_cec_physical_address(void);
void hdmi_detect_timer_add();
void hdmi_detect_timer_del();

extern const struct device_operations hdmi_dev_ops;
#endif
