#ifndef __ADV_WORK_SETTING_H__
#define __ADV_WORK_SETTING_H__

typedef enum {
    RCSPWorkModeNone						= 0,
    RCSPWorkModeNormal						= 1,
    RCSPWorkModeGame						= 2,
} RCSPWorkMode;

// 设置工作模式
void rcsp_set_work_mode(RCSPWorkMode work_mode);

#endif
