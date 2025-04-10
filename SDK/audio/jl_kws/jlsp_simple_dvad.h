#ifndef __JLSP_SIMPLE_DVAD_H__
#define __JLSP_SIMPLE_DVAD_H__

typedef struct  {
    int d_low_con_th;
    int d_high_con_th;
    int d2a_th_db;
    int d2a_frame_con;
    int dvad_gain_id;
    int d_frame_con;
    int d_stride1;
    int d_stride2;
} dvad_config_t;

void JLSP_simple_dvad_get_heap_size(int *private_heap_size);

void *JLSP_simple_dvad_init(char *private_heap, int private_heap_size, short init_noise);

void JLSP_simple_dvad_reset(void *dvad_obj);

int JLSP_simple_dvad_process(void *dvad_obj, char *inbu, int inlen, dvad_config_t *cfg);

void JLSP_simple_dvad_free(void *dvad_obj);

#endif /* #ifndef __VAD_ALGO_H__ */

