#ifndef __LP_TOUCH_KEY_TOOL__
#define __LP_TOUCH_KEY_TOOL__

#include "typedef.h"


//spp
int lp_touch_key_online_debug_init(void);
int lp_touch_key_online_debug_send(u32 ch, u16 val);
int lp_touch_key_online_debug_key_event_handle(u32 ch, void *e);


#endif

