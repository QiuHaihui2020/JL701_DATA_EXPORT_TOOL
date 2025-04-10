#ifndef APP_MODE_MANAGER_H
#define APP_MODE_MANAGER_H

#include "generic/typedef.h"
#include "generic/list.h"


struct app_mode_ops {
    int (*enter)(int param);
    int (*exit)();
    int (*try_enter)(int param);
    int (*try_exit)();
    int (*goto_background)();
    int (*goto_foreground)();
    int (*msg_handler)(int *msg);
};

struct app_mode {
    u8 name;
    u8 index;
    u8 has_try_exited;
    struct list_head entry;
    const struct app_mode_ops *ops;
};


#define REGISTER_APP_MODE(mode_name) \
    struct app_mode __##mode_name sec(.app_mode)

extern struct app_mode app_mode_begin[];
extern struct app_mode app_mode_end[];

#define list_for_each_app_mode(mode) \
    for (mode = app_mode_begin; mode < app_mode_end; mode++)


struct app_mode *app_get_current_mode();

void app_set_current_mode(struct app_mode *mode);

struct app_mode *app_get_mode_by_name(u8 name);

bool app_have_mode(void);

bool app_in_mode(u8 name);

struct app_mode *app_next_mode(struct app_mode *mode);

struct app_mode *app_get_next_mode();

struct app_mode *app_get_prev_mode();

bool app_try_exit_curr_mode();

bool app_try_enter_mode(struct app_mode *mode, int param);

int app_goto_next_mode();

int app_goto_prev_mode();

int app_goto_mode(u8 mode_name, int param);

int app_push_mode(u8 mode_name);

int app_goto_foreground(u8 mode_name);

int app_goto_background(u8 mode_name);

u8 app_pop_mode();

int app_exit_mode(struct app_mode *mode);

int app_exit_current_mode();



#endif

