#ifndef __RESOURCE_MULTIPLEX_H__
#define __RESOURCE_MULTIPLEX_H__

#include "system/includes.h"
#include "spinlock.h"


#define RESOURCE_MULTIPLEX_MAX      8

#define RESOURCE_MUX_CBFUNC_MAX     4

#define RESOURCE_POOL_SIZE          4


typedef enum {
    DO_AFTER_RESOURCE_BUSY,
    FORGO_WHEN_RESOURCE_BUSY,
    WAIT_RESOURCE_FREE,
} request_mode_enum;


typedef struct resource_multiplex_cbfunc_info {
    void *cbfunc;
    void *cbfunc_arg;
    u32 cbfunc_mode;
    u32 cbfunc_owner;
} src_mux_cbfunc_info;


typedef struct resource_multiplex_info {
    u32 resource_user;
    u32 resource_last_user;
    u32 resource_num;
    u32 resource[RESOURCE_POOL_SIZE];
    u32 resource_waiting;
    spinlock_t lock;
    u32 locked;
    OS_SEM resource_mux_sem;
    src_mux_cbfunc_info pre_cbfunc_info[RESOURCE_MUX_CBFUNC_MAX];
    src_mux_cbfunc_info post_cbfunc_info[RESOURCE_MUX_CBFUNC_MAX];
    src_mux_cbfunc_info free_cbfunc_info;
} resource_mux_info;




resource_mux_info *get_resource_multiplex_info(u32 mux_resource);

int resource_multiplex_is_registered(u32 mux_resource);

int resource_multiplex_register(u32 resource_num, u32 *mux_resource);

int resource_multiplex_check_user_different(u32 mux_resource, u32 requester);

int resource_multiplex_check_last_user_different(u32 mux_resource, u32 requester);

void resource_multiplex_register_free_cbfunc(u32 mux_resource, src_mux_cbfunc_info *cbfunc_info);

void resource_multiplex_register_pre_cbfunc(u32 mux_resource, src_mux_cbfunc_info *cbfunc_info);

void resource_multiplex_register_post_cbfunc(u32 mux_resource, src_mux_cbfunc_info *cbfunc_info);

void resource_multiplex_remove_pre_cbfunc(u32 mux_resource, u32 cbfunc_owner);

void resource_multiplex_remove_post_cbfunc(u32 mux_resource, u32 cbfunc_owner);

void resource_multiplex_remove_free_cbfunc(u32 mux_resource, u32 cbfunc_owner);

int resource_multiplex_has_requester_waiting(u32 mux_resource, u32 requester);

int resource_multiplex_check_request(u32 mux_resource, u32 requester, u32 request_mode, src_mux_cbfunc_info *cbfunc_info);

void resource_multiplex_free(u32 mux_resource, u32 requester);



#endif
