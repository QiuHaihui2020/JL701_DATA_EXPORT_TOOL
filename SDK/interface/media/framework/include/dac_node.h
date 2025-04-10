#ifndef _DAC_NODE_H_
#define _DAC_NODE_H_

enum DAC_NODE_STATE {
    DAC_NODE_STATE_START = 0,
    DAC_NODE_STATE_STOP,
};

struct dac_node_cb_hdl {
    enum DAC_NODE_STATE state;
    u8 scene;
    u8 channle_mode;
    int sample_rate;
};

void dac_try_power_on_thread();

/*
*********************************************************************
*                  			DAC NODE WRITE CALLBACK ADD
* Description: 注册DAC节点回调函数(仅写数回调)
* Arguments  : name 			回调函数名称
*		       scene 			回调函数触发场景
*		       cb         		DAC节点写数回调
* Return	 : null
*********************************************************************
*/
void dac_node_write_callback_add(const char *name, u8 scene, void (*cb)(void *, int));

/*
*********************************************************************
*                  			DAC NODE WRITE CALLBACK DEL
* Description: 删除DAC节点回调函数
* Arguments  : name 			被删除回调函数名称
* Return	 : null
*********************************************************************
*/
void dac_node_write_callback_del(const char *name);

/*
*********************************************************************
*                  			DAC NODE CALLBACK ADD
* Description: 注册DAC节点回调函数, 包含状态回调/写数回调
* Arguments  : name 			回调函数名称
*		       target_scene 	回调函数触发场景, 0XFF表示任意场景均触发
*		       state_cb         DAC节点状态回调
*		       write_cb         DAC节点写数回调
* Return	 : null
*********************************************************************
*/
void dac_node_callback_add(const char *name, u8 target_scene, void (*state_cb)(struct dac_node_cb_hdl *hdl), void (*write_cb)(void *, int));

/*
*********************************************************************
*                  			DAC NODE CALLBACK DEL
* Description: 删除DAC节点回调函数
* Arguments  : name 			被删除回调函数名称
* Return	 : null
*********************************************************************
*/
void dac_node_callback_del(const char *name);

#endif
