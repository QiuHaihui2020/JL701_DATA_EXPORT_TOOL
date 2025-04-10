#ifndef __EFFECTS_ADJ__H
#define __EFFECTS_ADJ__H


#include "system/includes.h"
#include "config/config_interface.h"
#include "crc.h"
#include "fs/resfile.h"
#include "AudioEffect_DataType.h"
#include "media/framework/include/jlstream.h"
#include "media_memory.h"
#include "media/audio_general.h"
#include "audio_drc_common.h"

#define TOOL_CTRL_BYPASS  	(0 << 4)
#define USER_CTRL_BYPASS  	(1 << 4)
#define BYPASS_CTRL_MODE(x) (x >> 4)

#define TOOL_CTRL_BYPASS  	(0 << 4)
#define USER_CTRL_BYPASS  	(1 << 4)
#define BYPASS_CTRL_MODE(x) (x >> 4)


#define TOOL_PARAM_SET    0
#define PRIVATE_PARAM_SET 1

struct node_param {//单节点名称
    char name[16];
};

struct eff_default_parm {
    char name[16];
    char cfg_index;//使用配置项的序号，指定默认配置项
    char mode_index;//节点与多模式关联时，该变量用于获取相应模式下的节点参数,模式序号（如，蓝牙模式下，无多子模式，mode_index 是0）
    void *private_param;//用于指定特定的默认配置
};



struct cfg_info {
    u16 offset;
    u16 size;
    u16 uuid;
    u8 subid;
};
#define indicator_check_points 4
#define indicator_read_points   40
struct indicator_upload_data {
    u8 len;
    u8 type;
    u8 index;
    u8 reserve;
    float min;
    float max;
    int nbit;//16bit、32bit
    int peak[indicator_read_points];
} __attribute__((packed));


// 配置文件格式
//文件头+节点配置头+节点数据区
struct node_file_head {
    u16 version;
    char reserved[14];
} __attribute__((packed));

#define EQ_NODE_CFG_TYPE      0x1//EQ节点配置
#define DRC_NODE_CFG_TYPE     0x2//DRC节点配置
#define FORM_NODE_CFG_TYPE    0x3//表单节点配置
#define PIPELINE_CFG_TYPE     0x4//参数组数据
#define UUID_MAP_CFG_TYPE     0x5//uuid映射表
#define DRC_ADVANCE_NODE_CFG_TYPE 0x6//drc advance 节点配置

struct node_cfg_format {
    u8 type;             //0x1 EQ配置  0x2 DRC配置  0x3 表单配置  0x4 参数组数据 0x5 uuid映射表
    u16 uuid;
    u8 subid;
    u8 mode_index;       //模式序号
    u16 node_name_uuid;  //节点名称的crc16
    u16 config_name_uuid;//配置名称的crc16
    u8 config_index;     //配置序号
    u32 offset;          //参数偏移位置
    u16 size;            //参数大小
    char reserved[16];   //保留位
} __attribute__((packed));

struct group_param {
    u16 uuid;            //流模块uuid
    u8 num;              //参数组个数
    u8 reserve;
    u8 data[0];          //参数组名称的uuid
} __attribute__((packed));

#define EFF_CONFIG_ID     (0x21) //调音注册的cmd
#define REPLY_TO_TOOL     (0)    //应答工具使用的标识
#define EFF_ADJ_CMD       (0x500)//调音命令
#define EFF_UPLOAD_CMD    (0x501)//数据上报命令，用于发送数据给上位机
#define EFF_MODE_CMD      (0x502)//模式切换命令，上位机发送当前模式序号给小机
#define EFF_INDICATOR_CMD (0x503)//指示器主动来获取数据
#define EFF_CRC_CMD       (0x504)//stream.bin crc校验命令
#define EFF_FORM_CMD      (0x506)//表单节点获取当前值命令
#define EFF_ONLINE_CMD    (0x507)//检查需要在线调试的节点
#define EFF_DNSFB_COEFF_CMD    (0x509)//更新DNSFB_coeff eq命令

#define EFF_NODE_MERGE_UPDATE          BIT(0)
#define EFF_MANUAL_ADJ_NODE            BIT(1)


enum {
    ERR_NONE                    = 1, //处理成功,返回"OK"
    ERR_COMM                    = 0, //通用错误,例如模块未打开,返回"ER"
    ERR_ACCEPTABLE              = -1, //可接受的错误,由case内部处理,返回内部数据
    EFF_ERR_TYPE_PTR_NULL       = -2,//模块打开失败,返回"ER_FLOW_OPEN_BUF"
    EFF_ERR_TYPE_ALGORITHM_NULL = -3,//算法返回错误,返回"ER_FLOW_OPEN_ALGORITHM"
    EFF_ERR_CRC                 = -4,//CRC校验失败
    EFF_ERR_RUN                 = -5,//音效run失败
};

struct effects_online_adjust {
    u16 uuid;
} __attribute__((packed));
//支持调音的节点，需使用该操作注册
#define REGISTER_ONLINE_ADJUST_TARGET(online) \
	static struct effects_online_adjust online sec(.effects_online_adjust)

extern struct effects_online_adjust effects_online_adjust_begin[];
extern struct effects_online_adjust effects_online_adjust_end[];

#define list_for_online_adjust_target(p) \
	    for (p = effects_online_adjust_begin; p < effects_online_adjust_end; p++)

/*
 *节点通过uuid 与name获取在线调试音效的参数
 * */
int get_eff_online_param(u32 uuid, char *name, void *packet);

/*
 *获取需要指定得默认配置
 * */
int get_eff_default_param(int arg);

/*
 *表单节点参数信息获取(获取目标参数存储地址，与大小)
 *mode_index:参数组序号（模式序号）
 *name:音效节点名字（节点内用户自定义）
 *cfg_index:目标配置项（0,1,2....）
 *info:音效节点的配置项信息（存储的目标地址，配置项大小）
 *return: 0 返回成功， 非0：返回失败
 * */
int jlstream_read_form_node_info_base(char mode_index, const char *name, char cfg_index, struct cfg_info *info);
/*
 *根据文件内偏移，获取目标配置参数
 *return: 实际长度: 返回成功， 0：返回失败
 * */
int jlstream_read_form_cfg_data(struct cfg_info *info, void *data);

/*
 *表单节点参数获取
 *mode_index:参数组序号（模式序号）
 *name:音效节点名字（节点内用户自定义）
 *cft_index:目标配置项（0,1,2....）
 *data:目标参数返回地址
 *return: 实际长度: 返回成功， 0：返回失败
 * */
int jlstream_read_form_data(char mode_index, const char *name, char cfg_index, void *data);

/*
 *通过uuid与subid直接获取节点参数,节点内如有多份参数，需通过STREAM_EVENT_GET_NODE_PARM case内指定目标参数序号
 *uuid:节点uuid
 *subid:节点subid
 *data:目标参数返回地址
 *node_name:节点配置名称返回地址
 *return: 实际长度: 返回成功， 0：返回失败
 * */
int jlstream_read_node_data_new(u16 uuid, u8 subid, void *data, char *node_name);

/*
 *通过uuid与subid、cfg_index直接获取节点参数
 *uuid:节点uuid
 *subid:节点subid
 *cfg_index:节点内的配置序号
 *data:目标参数返回地址
 *node_name:节点配置名称返回地址
 *return: 实际长度: 返回成功， 0：返回失败
 * */
int jlstream_read_node_data_by_cfg_index(u16 uuid, u8 subid, char cfg_index, void *data, char *node_name);

/*
 *通过uuid与subid直接获取节点info信息,节点内如有多份参数，需通过STREAM_EVENT_GET_NODE_PARM case内指定目标参数序号
 *uuid:节点uuid
 *subid:节点subid
 *data:目标参数返回地址
 *node_name:节点配置名称返回地址
 *info:节点信息保存的地址
 *return: 1:读取info信息成功,0:读取info信息失败
 * */
int jlstream_read_node_info_data(u16 uuid, u8 subid, char *node_name, struct cfg_info *info);

/*
 *节点info参数获取
 *mode_index:参数组序号（模式序号）
 *name:音效节点名字（节点内用户自定义）
 *cft_index:目标配置项（0,1,2....）
 *return: 1:读取info信息成功,0:读取info信息失败
 * */
int jlstream_read_info_data(char mode_index, const char *name, char cfg_index, struct cfg_info *info);
/*
 *通过uuid获取参数组个数
 *pipeline_uuid:流模块uuid
 *group_param_num:返回参数组个数
 *return 0:成功， 非0：失败
 * */
int jlstream_read_pipeline_param_group_num(u16 pipeline_uuid, u8 *group_param_num);


/*
 *通过uuid和name获取长度为len的在线调试参数param
 * */
int jlstream_read_effects_online_param(u32 uuid, char *name, void *param, u16 len);

/*
 *小机主动往上位机发数据接口
 * */
void eff_node_send_packet(u32 id, u8 sq, u8 *packet, int size);
/*
 *name_son:子节点名字
 *name_father:父节点名字
 *name_out:计算得到节点的实际名字 16byte
 * */
void jlstream_module_node_get_name(char *name_son, char *name_father, char *name_out);

/*
 *获取流程图中节点数据
 * */
int jlstream_read_pipeline_data(u16 pipeline, u8 **pipeline_data);
#endif/*__EFFECTS_ADJ__H*/
