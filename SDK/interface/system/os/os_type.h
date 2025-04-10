#ifndef __OS_TYPE_H
#define __OS_TYPE_H


#define OS_TICKS_PER_SEC          100

#define Q_MSG           0x100000
#define Q_EVENT         0x200000
#define Q_CALLBACK      0x300000
#define Q_USER          0x400000

#define OS_DEL_NO_PEND               0u
#define OS_DEL_ALWAYS                1u

#define OS_TASK_DEL_REQ           0x01u
#define OS_TASK_DEL_RES           0x02u
#define OS_TASK_DEL_OK            0x03u


#define  OS_TASK_SELF        (char *)0x1
#define  OS_TASK_FATHER      (char *)0x2

#ifdef CONFIG_UCOS_ENABLE

typedef struct {
    unsigned char OSEventType;
    int aa;
    void *bb;
    unsigned char value;
    unsigned char prio;
    unsigned short cc;
} OS_SEM, OS_MUTEX, OS_QUEUE;


#elif defined CONFIG_FREE_RTOS_ENABLE

struct StaticMiniListItem {
    uint32_t tick;
    void *pvDummy2[2];
};

struct StaticList {
    unsigned long uxDummy1;
    void *pvDummy2;
    struct StaticMiniListItem xDummy3;
};

typedef struct {
    void *pvDummy1[3];

    union {
        void *pvDummy2;
        unsigned long uxDummy2;
    } u;

    struct StaticList xDummy3[2];
    unsigned long uxDummy4[3];
    uint8_t ucDummy5[2];

    uint8_t ucDummy6;

    unsigned long uxDummy8;
    uint8_t ucDummy9;

} OS_SEM, OS_MUTEX, OS_QUEUE;


#else
#error "no_os_defined"
#endif


#endif
