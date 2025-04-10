#ifndef _FILE_RECODER_H_
#define _FILE_RECODER_H_

#include "jlstream.h"
#include "encoder_node.h"
#include "fs/fs.h"



typedef void (*file_recorder_cb_t)(void *, enum stream_state);

struct file_recorder {
    struct list_head entry;
    void *file;
    void *priv;
    file_recorder_cb_t callback;
    const struct stream_file_ops *fops;
    struct jlstream *stream;
};

struct file_recorder *file_recorder_open(int pipeline_uuid, int snode_uuid);

int file_recorder_get_fmt(struct file_recorder *recorder, struct stream_enc_fmt *fmt);

int file_recorder_set_fmt(struct file_recorder *recorder, struct stream_enc_fmt *fmt);

FILE *file_recorder_open_file(struct file_recorder *recorder, const char *fname);

int file_recorder_set_file(struct file_recorder *recorder, void *file,
                           const struct stream_file_ops *fops);

void *file_recorder_change_file(struct file_recorder *recorder, void *new_file);

void file_recorder_set_callback(struct file_recorder *recorder, void *priv,
                                file_recorder_cb_t callback);


void file_recorder_seamless_set(struct file_recorder *recorder, struct seamless_recording *seamless);

int file_recorder_start(struct file_recorder *recorder);

int file_recorder_close(struct file_recorder *recorder, bool close_file);

int file_recorder_get_enc_time(struct file_recorder *recorder);

int file_recorder_stop(struct file_recorder *recorder, bool close_file);

int file_recorder_release(struct file_recorder *recorder);

#endif

