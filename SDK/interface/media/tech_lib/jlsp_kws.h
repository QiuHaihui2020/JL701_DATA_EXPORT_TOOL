#ifndef _JLSP_KWS_H_
#define _JLSP_KWS_H_




int jl_kws_model_get_heap_size(int model, int *model_size, int *private_heap_size, int *share_heap_size);

void *jl_kws_model_init(int model, char *private_heap_buffer, int private_heap_size, char *share_heap_buffer, int share_heap_size, int model_size, float *confidence, int online);
int jl_kws_model_reset(void *m);
int jl_kws_model_process(void *m, int model, char *pcm, int size);
int jl_kws_model_free(void *m);



#endif
