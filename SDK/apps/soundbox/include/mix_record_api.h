#ifndef MIX_RECORD_INTERFACE_H
#define MIX_RECORD_INTERFACE_H

void mix_recorder_start();
void mix_recorder_stop();
int get_mix_recorder_status(void);
int mix_record_device_msg_handler(int *msg);
#endif

