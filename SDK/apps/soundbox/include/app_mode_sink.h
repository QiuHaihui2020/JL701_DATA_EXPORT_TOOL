#ifndef APP_MODE_SINK_H
#define APP_MODE_SINK_H

#include "app_mode_manager/app_mode_manager.h"

extern const struct key_remap_table sink_mode_key_table[];

struct app_mode *app_enter_sink_mode(int arg);
void app_sink_set_sink_mode_rsp_arg(enum app_mode_t arg);

#endif

