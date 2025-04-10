#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

enum audio_msg {
    AUDIO_EVENT_VOL_MAX = 0x0,
    AUDIO_EVENT_VOL_MIN,
    AUDIO_EVENT_ANC_ON,
    AUDIO_EVENT_ANC_OFF,
    AUDIO_EVENT_ANC_TRANS,
    AUDIO_EVENT_A2DP_NO_ENERGY,
};

void audio_anc_event_to_user(int mode);

void audio_event_to_user(int event);


#endif
