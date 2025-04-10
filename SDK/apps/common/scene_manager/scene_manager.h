#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "generic/typedef.h"

struct scene_param {
    u8 uuid;
    u8 arg_len;
    u8 *arg;
    void *msg;
    int user_type[2];
};

typedef bool (*scene_event_func_t)(struct scene_param *param);

struct scene_event {
    int event;
    scene_event_func_t match;
};


typedef bool (*scene_state_func_t)(struct scene_param *param);

struct scene_state {
    scene_state_func_t match;
};


typedef void (*scene_action_func_t)(struct scene_param *param);

struct scene_action {
    scene_action_func_t action;
};


struct scene_ability {
    int uuid;
    const struct scene_event *event;
    const struct scene_state *state;
    const struct scene_action *action;
};

#define REGISTER_SCENE_ABILITY(ability) \
    const struct scene_ability __##ability sec(.scene_ability)

extern const struct scene_ability scene_ability_begin[];
extern const struct scene_ability scene_ability_end[];

#define list_for_each_scene_ability(ability) \
    for (ability = scene_ability_begin; ability < scene_ability_end; ability++)


void scene_mgr_event_match(u8 uuid, int _event, void *arg);


#endif

