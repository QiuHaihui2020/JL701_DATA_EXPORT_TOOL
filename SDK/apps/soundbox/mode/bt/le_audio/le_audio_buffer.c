/*************************************************************************************************/
/*!
*  \file      le_audio_buffer.c
*
*  \brief
*
*  Copyright (c) 2011-2024 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#include "le_audio_buffer.h"
#include "system/spinlock.h"

struct le_audio_buffer {
    int max_size;
    int interval_len;
    int len;
    struct list_head list;
    struct le_audio_frame *frame;
    spinlock_t lock;
};

void *le_audio_buffer_open(int interval_len, int size)
{
    struct le_audio_buffer *buffer = (struct le_audio_buffer *)zalloc(sizeof(struct le_audio_buffer));

    if (!buffer) {
        return NULL;
    }

    ASSERT(interval_len <= size);
    buffer->max_size = size;
    buffer->interval_len = interval_len;

    INIT_LIST_HEAD(&buffer->list);

    spin_lock_init(&buffer->lock);

    return buffer;
}

int le_audio_push_frame(void *_buffer, void *data, int len, u32 timestamp)
{
    struct le_audio_buffer *buffer = (struct le_audio_buffer *)_buffer;
    struct le_audio_frame *frame = NULL;

    if (!buffer) {
        return 0;
    }

    spin_lock(&buffer->lock);
    if (buffer->frame) {
        frame = buffer->frame;
        if (frame->len + len > buffer->interval_len) {
            r_printf("frame len error : %d, %d\n", len, buffer->interval_len);
        }
    } else {
        if (buffer->len + len > buffer->max_size) {
            spin_unlock(&buffer->lock);
            return 0;
        }
        frame = (struct le_audio_frame *)malloc(sizeof(struct le_audio_frame) + buffer->interval_len);
        frame->data = (u8 *)(frame + 1);
        frame->len = 0;
        frame->timestamp = timestamp;
        /*printf("<%d, %d>\n", len, buffer->interval_len);*/
    }

    memcpy(frame->data + frame->len, data, len);

    buffer->len += len;
    frame->len += len;
    if (frame->len >= buffer->interval_len) {
        list_add_tail(&frame->entry, &buffer->list);
        buffer->frame = NULL;
    } else {
        buffer->frame = frame;
    }

    spin_unlock(&buffer->lock);

    return len;
}

struct le_audio_frame *le_audio_get_frame(void *_buffer)
{
    struct le_audio_buffer *buffer = (struct le_audio_buffer *)_buffer;
    struct le_audio_frame *frame;

    spin_lock(&buffer->lock);
    if (list_empty(&buffer->list)) {
        spin_unlock(&buffer->lock);
        return NULL;
    }
    frame = list_first_entry(&buffer->list, struct le_audio_frame, entry);
    list_del(&frame->entry);
    spin_unlock(&buffer->lock);

    return frame;
}

void le_audio_free_frame(void *_buffer, struct le_audio_frame *frame)
{
    struct le_audio_buffer *buffer = (struct le_audio_buffer *)_buffer;

    spin_lock(&buffer->lock);
    buffer->len -= frame->len;
    free(frame);
    spin_unlock(&buffer->lock);
}

void le_audio_buffer_close(void *_buffer)
{
    struct le_audio_buffer *buffer = (struct le_audio_buffer *)_buffer;
    struct le_audio_frame *frame, *n;

    if (!buffer) {
        return;
    }

    spin_lock(&buffer->lock);
    if (buffer->frame) {
        free(buffer->frame);
    }
    list_for_each_entry_safe(frame, n, &buffer->list, entry) {
        __list_del_entry(&frame->entry);
        free(frame);
    }
    spin_unlock(&buffer->lock);

    free(buffer);
}
