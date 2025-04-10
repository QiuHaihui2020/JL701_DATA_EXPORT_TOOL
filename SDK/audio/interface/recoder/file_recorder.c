#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".file_recorder.data.bss")
#pragma data_seg(".file_recorder.data")
#pragma const_seg(".file_recorder.text.const")
#pragma code_seg(".file_recorder.text")
#endif
#include "file_recorder.h"
#include "media/includes.h"
#include "app_config.h"

#if TCFG_APP_RECORD_EN || TCFG_MIX_RECORD_ENABLE

static struct list_head g_recorder_head = LIST_HEAD_INIT(g_recorder_head);

static void file_recorder_callback(void *private_data, int event)
{
    struct file_recorder *recorder, *n;

    printf("file_recorder_callback: %x \n", event);

    list_for_each_entry_safe(recorder, n, &g_recorder_head, entry) {
        if (recorder->stream->id == (int)private_data) {
            if (recorder->callback) {
                recorder->callback(recorder->priv, event);
            }
            break;
        }
    }
}

struct file_recorder *file_recorder_open(int pipeline_uuid, int snode_uuid)
{
    struct file_recorder *recorder;
    struct jlstream *stream;

    recorder = zalloc(sizeof(*recorder));
    if (!recorder) {
        return NULL;
    }

    stream = jlstream_pipeline_parse(pipeline_uuid, snode_uuid);
    if (!stream) {
        free(recorder);
        return NULL;
    }
    recorder->stream = stream;

    jlstream_node_ioctl(stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 256);
    jlstream_set_scene(stream, STREAM_SCENE_RECODER);
    jlstream_add_thread(stream, NULL);

    int id = stream->id;
    jlstream_set_callback(stream, (void *)id, file_recorder_callback);

    list_add(&recorder->entry, &g_recorder_head);

    return recorder;
}

static int recorder_fread(void *_recorder, u8 *buf, int len)
{
    struct file_recorder *recorder = (struct file_recorder *)_recorder;

    if (!recorder->file) {
        return 0;
    }
    if (recorder->fops) {
        return recorder->fops->read(recorder->file, buf, len);
    }
    return fread(buf, len, 1, (FILE *)recorder->file);
}

static int recorder_fseek(void *_recorder, int offset, int from)
{
    struct file_recorder *recorder = (struct file_recorder *)_recorder;
    if (!recorder->file) {
        return 0;
    }
    if (recorder->fops) {
        return recorder->fops->seek(recorder->file, offset, from);
    }
    return fseek((FILE *)recorder->file, offset, from);
}

static int recorder_fwrite(void *_recorder, u8 *buf, int len)
{
    int wlen;
    struct file_recorder *recorder = (struct file_recorder *)_recorder;
    if (!recorder->file) {
        return 0;
    }
    if (recorder->fops) {
        wlen = recorder->fops->write(recorder->file, buf, len);
    } else {
        wlen = fwrite(buf, len, 1, (FILE *)recorder->file);
    }
    return wlen;
}

static int recorder_fclose(void *_recorder)
{
    struct file_recorder *recorder = (struct file_recorder *)_recorder;
    if (!recorder->file) {
        return 0;
    }
    if (recorder->fops) {
        if (!recorder->fops->close) {
            return -EINVAL;
        }
        return recorder->fops->close(recorder->file);
    }
    return fclose((FILE *)recorder->file);
}

static const struct stream_file_ops recorder_fops = {
    .read = recorder_fread,
    .seek = recorder_fseek,
    .write = recorder_fwrite,
};

FILE *file_recorder_open_file(struct file_recorder *recorder, const char *fname)
{
    recorder->file = fopen(fname, "w+");
    if (!recorder->file) {
        printf("open file %s faild\n", fname);
        return NULL;
    }
    jlstream_set_enc_file(recorder->stream, recorder, &recorder_fops);
    printf("open file %s suss\n", fname);

    return (FILE *)recorder->file;
}

int file_recorder_set_file(struct file_recorder *recorder, void *file,
                           const struct stream_file_ops *fops)
{
    recorder->file = file;
    recorder->fops = fops;
    jlstream_set_enc_file(recorder->stream, recorder, &recorder_fops);
    return 0;
}

void *file_recorder_change_file(struct file_recorder *recorder, void *new_file)
{
    void *file = recorder->file;
    recorder->file = new_file;
    return file;
}

int file_recorder_get_fmt(struct file_recorder *recorder, struct stream_enc_fmt *fmt)
{
    return jlstream_ioctl(recorder->stream, NODE_IOC_GET_ENC_FMT, (int)fmt);
}

int file_recorder_set_fmt(struct file_recorder *recorder, struct stream_enc_fmt *fmt)
{
    return jlstream_ioctl(recorder->stream, NODE_IOC_SET_ENC_FMT, (int)fmt);
}

void file_recorder_set_callback(struct file_recorder *recorder, void *priv,
                                file_recorder_cb_t callback)
{
    recorder->priv = priv;
    recorder->callback = callback;
}

void file_recorder_seamless_set(struct file_recorder *recorder, struct seamless_recording *seamless)
{
    jlstream_node_ioctl(recorder->stream, NODE_UUID_ENCODER, NODE_IOC_SET_SEAMLESS, (int)seamless);
}

int file_recorder_start(struct file_recorder *recorder)
{
    int err = jlstream_start(recorder->stream);
    return err;
}

int file_recorder_stop(struct file_recorder *recorder, bool close_file)
{
    int err = 0;

    if (!recorder) {
        return -EINVAL;
    }
    jlstream_stop(recorder->stream, 50);
    if (close_file) {
        err = recorder_fclose(recorder);
    }
    return err;
}

int file_recorder_release(struct file_recorder *recorder)
{
    if (!recorder) {
        return -EINVAL;
    }

    jlstream_release(recorder->stream);
    list_del(&recorder->entry);
    free(recorder);
    jlstream_event_notify(STREAM_EVENT_CLOSE_RECODER, (int)"recorder");
    return 0;
}

int file_recorder_close(struct file_recorder *recorder, bool close_file)
{
    int err = 0;

    if (!recorder) {
        return -EINVAL;
    }
    jlstream_stop(recorder->stream, 50);
    jlstream_release(recorder->stream);

    if (close_file) {
        err = recorder_fclose(recorder);
    }

    list_del(&recorder->entry);
    free(recorder);
    jlstream_event_notify(STREAM_EVENT_CLOSE_RECODER, (int)"recorder");
    return err;
}

int file_recorder_get_enc_time(struct file_recorder *recorder)
{
    int ret = 0;
    if (recorder && recorder->file) {
        ret = jlstream_node_ioctl(recorder->stream, NODE_UUID_ENCODER, NODE_IOC_GET_CUR_TIME, 0);
    }
    return ret;
}

#endif

