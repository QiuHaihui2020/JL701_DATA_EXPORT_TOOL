#ifndef POWER_INTERFACE_H
#define POWER_INTERFACE_H

#include "gpio.h"
#include "generic/typedef.h"

//-------------------------------------------------------
/* p33
 */
#include "power/p33/p33_sfr.h"
#include "power/p33/p33_app.h"
#include "power/p33/p33_api.h"

#include "asm/power/p11.h"

#include "asm/power/power_api.h"

#include "asm/power/power_port.h"

#include "power/power_wakeup.h"

#include "power/power_reset.h"

#include "asm/power/power_compat.h"

#include "asm/power/lp_ipc.h"

#include "power/power_app.h"

#include "asm/power/power_trim.h"

#include "power/rtc_app.h"

#include "power/wdt.h"

#include "power/lib_power_config.h"

#endif
