#ifndef _AUDIO_EVENT_MANAGER_H_
#define _AUDIO_EVENT_MANAGER_H_

#include "generic/typedef.h"

enum audio_lib_event {
    AUDIO_LIB_EVENT_DAC_LOWPOWER,
    AUDIO_LIB_EVENT_VBG_TRIM_WTITE,
    AUDIO_LIB_EVENT_VBG_TRIM_READ,
};

int audio_event_notify(enum audio_lib_event event, void *priv, int arg);

#endif
