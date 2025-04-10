#ifndef _VM_H_
#define _VM_H_

#include "typedef.h"

#include "ioctl.h"

#define IOCTL_SET_VM_INFO               _IOW('V', 1, u8)
#define IOCTL_GET_VM_INFO               _IOW('V', 2, u8)

/* --------------------------------------------------------------------------*/
/**
 * @brief VM Struct
 */
/* ----------------------------------------------------------------------------*/
typedef u16 vm_hdl;

struct vm_table {
    u16  index;
    u16 value_byte;
    int value;      //cache value which value_byte <= 4
};


/* --------------------------------------------------------------------------*/
/**
 * @brief VM Error Code
 */
/* ----------------------------------------------------------------------------*/
typedef enum _vm_err {
    VM_ERR_NONE = 0,
    VM_INDEX_ERR = -0x100,
    VM_INDEX_EXIST,     //0xFF
    VM_DATA_LEN_ERR,    //0xFE
    VM_READ_NO_INDEX,   //0xFD
    VM_READ_DATA_ERR,   //0xFC
    VM_WRITE_OVERFLOW,  //0xFB
    VM_NOT_INIT,
    VM_INIT_ALREADY,
    VM_DEFRAG_ERR,
    VM_ERR_INIT,
    VM_ERR_PROTECT,
    VM_RAM_NOT_INIT, //vm_ram_storage_enable宏没有开启，没有初始化成功
    VM_RAM_MALLOC_ERR, // 申请内存失败
    VM_RAM2FLASH_WRITE_ERR, //ram的内容写到flash有错误，可能是ram的内容大于flash空间
    VM_RAM_FLUSH_NONE //没有配置项可以写到flash

} VM_ERR;

struct vm_manage_id {
    vm_hdl id;
};

#define VM_MANAGE_ID_REG(name, hdl) \
	const struct vm_manage_id vm_##name \
		 SEC_USED(.vm_manage_id_text) = {hdl};

extern struct vm_manage_id vm_reg_id_begin[];
extern struct vm_manage_id vm_reg_id_end[];

#define list_for_each_vm_id(h) \
	for (h=vm_reg_id_begin; h<vm_reg_id_end; h++)


/* --------------------------------------------------------------------------*/
/**
 * @brief vm_open
 *
 * @param index
 *
 * @return handle
 */
/* ----------------------------------------------------------------------------*/
s32 vm_open(u16 index);


/* --------------------------------------------------------------------------*/
/**
 * @brief vm_close
 *
 * @param hdl
 *
 * @return Error Code
 */
/* ----------------------------------------------------------------------------*/
VM_ERR vm_close(vm_hdl hdl);


/* --------------------------------------------------------------------------*/
/**
* @brief vm_eraser
 *
 * @return Error Code
 */
/* ----------------------------------------------------------------------------*/
VM_ERR vm_eraser(void);


/* --------------------------------------------------------------------------*/
/**
* @brief vm_check
 *
 * @param level
 */
/* ----------------------------------------------------------------------------*/
void vm_check_all(u8 level);    //level : default 0


/* --------------------------------------------------------------------------*/
/**
 * @brief vm_read
 *
 * @param hdl
 * @param data_buf
 * @param len
 *
 * @return len
 */
/* ----------------------------------------------------------------------------*/
s32 vm_read(vm_hdl hdl, void *data_buf, u16 len);


/* --------------------------------------------------------------------------*/
/**
 * @brief vm_write
 *
 * @param hdl
 * @param data_buf
 * @param len
 *
 * @return len
 */
/* ----------------------------------------------------------------------------*/
s32 vm_write(vm_hdl hdl, const void *data_buf, u16 len);


/* --------------------------------------------------------------------------*/
/**
 * @brief sfc_erase_zone
 *
 * @param addr
 * @param len
 *
 * @return true or false
 */
/* ----------------------------------------------------------------------------*/
bool sfc_erase_zone(u32 addr, u32 len);


/* --------------------------------------------------------------------------*/
/**
 * @brief 获取当前vm_ram_storage是否开启
 *
 * @return true=开启 or false=关闭
 */
/* --------------------------------------------------------------------------*/
u8 get_vm_ram_storage_enable(void);

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取当前vm_ram_storage_in_irq_enable是否开启
 *
 * @return true=开启 or false=关闭
 */
/* --------------------------------------------------------------------------*/
u8 get_vm_ram_storage_in_irq_enable(void);

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取VM配置项暂存RAM的内存使用情况
 * @return 返回全部数据+管理结构体加起来的字节数总和
 */
/* --------------------------------------------------------------------------*/
int get_vm_ram_data_total_size(void);

/* --------------------------------------------------------------------------*/
/**
 * @brief 刷新vm_ram缓存到vm_flash。（该接口不允许在中断内调用）
 * @param off:是否关闭写vm到ram的功能
 * @return VM错误码
 */
/* --------------------------------------------------------------------------*/
VM_ERR vm_flush2flash(u8 off);

/* --------------------------------------------------------------------------*/
/**
 * @brief 关闭写vm到ram功能,只有复位/重启才可重新写vm
 *
 * @return VM错误码
 */
/* --------------------------------------------------------------------------*/
VM_ERR vm_write_ram_off(void);

void vm_api_write_mult(u16 start_id, u16 end_id, void *buf, u16 len, u32 delay);
int vm_api_read_mult(u16 start_id, u16 end_id, void *buf, u16 len);


#endif  //_VM_H_

