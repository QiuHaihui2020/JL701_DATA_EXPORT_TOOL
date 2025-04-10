#ifndef _JL_KWS_API_H_
#define _JL_KWS_API_H_

int jl_kws_speech_recognition_open(void);
int jl_kws_speech_recognition_start(void);
void jl_kws_speech_recognition_stop(void);
void jl_kws_speech_recognition_close(void);

void jl_kws_main_user_demo(void);

//for jl_kws audio extern input
u8 kws_aec_get_state(void);
void kws_aec_data_output(void *priv, s16 *data, int len);

extern int audio_phone_call_kws_start(void);
extern int audio_phone_call_kws_close(void);
extern void audio_smart_voice_detect_close(void);

#endif /* #ifndef _JL_KWS_API_H_ */
