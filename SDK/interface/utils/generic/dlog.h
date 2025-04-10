#ifndef _DLOG_H_
#define _DLOG_H_

struct dlog_str_tab_s {
    u32 arg_num;
    u32 dlog_str_addr;
    u32 arg_type_bit_map;
} __attribute__((packed));

struct dlog_flash_op_s {
    int (*dlog_get_flash_zone)(u32 *addr, u32 *len);
    int (*dlog_flash_zone_erase)(u16 erase_sector, u8 sector_num);
    int (*dlog_flash_write)(void *buf, u16 len, u32 offset);
    int (*dlog_flash_read)(void *buf, u16 len, u32 offset);
};

/*----------------------------------------------------------------------------*/
/**@brief dlog功能初始化
  @param  void
  @return 成功返回 0, 失败返回 < 0
  @note
 */
/*----------------------------------------------------------------------------*/
int dlog_init(void);

/*----------------------------------------------------------------------------*/
/**@brief dlog功能的关闭
  @param  void
  @return void
  @note
 */
/*----------------------------------------------------------------------------*/
void dlog_close(void);

/*----------------------------------------------------------------------------*/
/**@brief 更新 ram 的数据到 flash 中
  @param  void
  @return 成功返回 0, 失败返回 < 0
  @note
 */
/*----------------------------------------------------------------------------*/
int dlog_flush2flash(void);

/*----------------------------------------------------------------------------*/
/**@brief 读取flash中的dlog信息
  @param  buf    : 返回读取的数据
  @param  len    : 需要读取的数据长度
  @param  offset : 读取数据的偏移(偏移是相对于flash区域起始地址)
  @return 返回读取到的数据长度,0表示已读取完数据
  @note
 */
/*----------------------------------------------------------------------------*/
u16 dlog_read_flash_info(u8 *buf, u16 len, u32 offset);

/*----------------------------------------------------------------------------*/
/**@brief 使能将dlog数据写到外部flash时,注册必要的接口给库调用
  @param  op: 接口的结构体句柄
  @return void
  @note
 */
/*----------------------------------------------------------------------------*/
void dlog_ex_flash_op_register(struct dlog_flash_op_s *op);

// 仅声明, 非外部调用接口
int dlog_print(const struct dlog_str_tab_s *str_tab, u8 arg_num, u32 arg_bit_map, ...);

extern const int config_dlog_enable;

// 超过 20 个参数就会计算出错
#define VA_ARGS_NUM_PRIV(P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13, P14, P15, P16, P17, P18 ,P19, P20, Pn, ...) Pn
#define VA_ARGS_NUM(...)   VA_ARGS_NUM_PRIV(-1, ##__VA_ARGS__, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define VA_ARGS_TYPE_BIT_SIZE_PRIV(P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13, P14, P15, P16, P17, Pn, ...)  \
    ( (_Generic(P2,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 0) \
    | (_Generic(P3,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 2) \
    | (_Generic(P4,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 4) \
    | (_Generic(P5,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 6) \
    | (_Generic(P6,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 8) \
    | (_Generic(P7,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 10) \
    | (_Generic(P8,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 12) \
    | (_Generic(P9,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 14) \
    | (_Generic(P10, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 16) \
    | (_Generic(P11, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 18) \
    | (_Generic(P12, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 20) \
    | (_Generic(P13, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 22) \
    | (_Generic(P14, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 24) \
    | (_Generic(P15, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 26) \
    | (_Generic(P16, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 28) \
    | (_Generic(P17, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                    float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                    char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) << 30))

#define VA_ARGS_TYPE_BIT_SIZE(...)  VA_ARGS_TYPE_BIT_SIZE_PRIV(-1, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)


#define VA_ARGS_TYPE_CHECK_PRIV(P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13, P14, P15, P16, P17, Pn, ...) \
    _Static_assert(_Generic(P2,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P3,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P4,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P5,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P6,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P7,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P8,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P9,  char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P10, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P11, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P12, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P13, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P14, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P15, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P16, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n"); \
    _Static_assert(_Generic(P17, char:0, unsigned char:0, short:0, unsigned short:0, int: 0, unsigned int: 0, \
                                float: 1, double: 1, long long int: 1, unsigned long long int: 1, \
                                char *: 2, unsigned char *: 2, const char *: 2, const unsigned char *: 2, default: 3) != 3, "arg type error!\n");

#define VA_ARGS_TYPE_CHECK(...)  VA_ARGS_TYPE_CHECK_PRIV(-1, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define dlog_printf(format, ...) \
    if(config_dlog_enable){ \
        _Static_assert(VA_ARGS_NUM_PRIV(-1, ##__VA_ARGS__, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0) <= 16, "log err : printf arg num too much\n"); \
        VA_ARGS_TYPE_CHECK_PRIV(-1, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0); \
        __attribute__((used,section(".dlog.rodata.string")))  static const char dlog_printf_str[] = format; \
        __attribute__((used,section(".dlog.rodata.str_tab"))) static const struct dlog_str_tab_s dlog_printf_str_tab = \
        { \
            .arg_num = (u32)VA_ARGS_NUM_PRIV(-1, ##__VA_ARGS__, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0), \
            .dlog_str_addr = (u32)dlog_printf_str, \
            .arg_type_bit_map = (u32)VA_ARGS_TYPE_BIT_SIZE_PRIV(-1, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0) \
        }; \
        dlog_print((const struct dlog_str_tab_s *)&dlog_printf_str_tab, \
                VA_ARGS_NUM_PRIV(-1, ##__VA_ARGS__, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0), \
                VA_ARGS_TYPE_BIT_SIZE_PRIV(-1, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0), ##__VA_ARGS__); \
    };

#endif
