#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rdec.data.bss")
#pragma data_seg(".rdec.data")
#pragma const_seg(".rdec.text.const")
#pragma code_seg(".rdec.text")
#endif
#include "asm/includes.h"
#include "gpio.h"
#include "asm/rdec.h"
#include "spinlock.h"

#define RDEC_DEBUG_ENABLE   1
#if RDEC_DEBUG_ENABLE
#define rdec_debug(fmt, ...) 	printf("[RDEC] "fmt, ##__VA_ARGS__)
#else
#define rdec_debug(...)
#endif

extern u32 clk_get(const char *name);
#define RDEC_CLK    clk_get("lsb")
DEFINE_SPINLOCK(rdec_lock);

static struct rdec_info_t *rdec_info[RDEC_NUM_MAX] = {NULL};

static JL_RDEC_TypeDef *rdec_get_reg(rdec_ch ch)
{
    ASSERT((u32)ch < RDEC_CH_MAX, "func:%s(), line:%d\n", __func__, __LINE__);
    JL_RDEC_TypeDef *reg = NULL;
    reg = (JL_RDEC_TypeDef *)(RDEC_BASE_ADDR + ch * RDEC_OFFSET);
    return reg;
}

/* static void clock_critical_enter(void) */
/* { */
/*     return; */
/* } */
/* static void clock_critical_exit(void) */
/* { */
/*     return; */
/* } */
/* CLOCK_CRITICAL_HANDLE_REG(rdec, clock_critical_enter, clock_critical_exit) */

static void rdec_reg_log_info(int rdec_cfg_id)
{
    JL_RDEC_TypeDef *rdec_reg = rdec_info[rdec_cfg_id]->rdecx;
    rdec_debug("rdec%d->con = 0x%x\n", rdec_cfg_id, rdec_reg->CON);
    rdec_debug("rdec%d->dat = 0x%x\n", rdec_cfg_id, rdec_reg->DAT);
    rdec_debug("rdec%d->smp = 0x%x\n", rdec_cfg_id, rdec_reg->SMP);
    /* rdec_debug("rdec%d->dbe = 0x%x\n", rdec_cfg_id, rdec_reg->DBE); */
    rdec_debug("rdec%d_clk = %d\n", rdec_cfg_id, RDEC_CLK);
}

static void rdec_cfg_info_load(int rdec_cfg_id)
{
    ASSERT(rdec_info[rdec_cfg_id] != NULL, "func:%s(), line:%d\n", __func__, __LINE__);
    JL_RDEC_TypeDef *rdec_reg = rdec_info[rdec_cfg_id]->rdecx;
    u16 con = 0;
    u8 smp = 0;
    /* SFR(con, QDEC_INT_MODE, 1, 1); */
    SFR(con, QDEC_MODE, 1, rdec_info[rdec_cfg_id]->rdec_cfg.mode);
    SFR(con, QDEC_CPND, 1, 1);
    u8 clk = rdec_info[rdec_cfg_id]->rdec_cfg.clk;
    if (clk != (u8) - 1) {
        ASSERT((clk <= 0xf) && (clk >= 0), "func:%s(), line:%d\n", __func__, __LINE__);
        SFR(con, QDEC_SPND, 4, clk); //0.68ms  1364hz
    } else {
        SFR(con, QDEC_SPND, 4, 0xf); //0.68ms  1364hz
    }
    SFR(con, QDEC_POL, 1, 0);
    SFR(smp, QDEC_SMP, 8, 0b1000);

    spin_lock(&rdec_lock);
    rdec_reg->CON = con;
    rdec_reg->SMP = smp;
    spin_unlock(&rdec_lock);

    gpio_set_mode(IO_PORT_SPILT(rdec_info[rdec_cfg_id]->rdec_cfg.port_0), PORT_INPUT_PULLUP_10K);
    gpio_set_mode(IO_PORT_SPILT(rdec_info[rdec_cfg_id]->rdec_cfg.port_1), PORT_INPUT_PULLUP_10K);
    gpio_set_function(IO_PORT_SPILT(rdec_info[rdec_cfg_id]->rdec_cfg.port_0), PORT_FUNC_RDEC0_PORT0 + 2 * rdec_cfg_id);
    gpio_set_function(IO_PORT_SPILT(rdec_info[rdec_cfg_id]->rdec_cfg.port_1), PORT_FUNC_RDEC0_PORT1 + 2 * rdec_cfg_id);
}

int rdec_init(struct rdec_config *rdec_cfg)
{
    JL_RDEC_TypeDef *RDECx = rdec_get_reg(rdec_cfg->ch);

    int cfg_id = -1;
    for (u32 i = 0; i < RDEC_NUM_MAX; i++) {
        if (rdec_info[i]) {
            continue;
        }
        if (rdec_info[i] == NULL) {
            rdec_info[i] = (struct rdec_info_t *)malloc(sizeof(struct rdec_info_t));
            ASSERT(rdec_info[i] != NULL, "func:%s(), line:%d\n", __func__, __LINE__);
            cfg_id = i;
            break;
        }
    }
    if (cfg_id == -1) {
        rdec_debug("rdec_config_id is null!!!\n");
        return cfg_id;
    }
    rdec_info[cfg_id]->rdecx = RDECx;
    memcpy(&rdec_info[cfg_id]->rdec_cfg, rdec_cfg, sizeof(struct rdec_config));
    rdec_debug("rdec_info[%d]->rdecx = 0x%x\n", cfg_id, (u32)rdec_info[cfg_id]->rdecx);
    rdec_debug("rdec_info[%d]->rdec_cfg.ch = %d\n", cfg_id, (u32)rdec_info[cfg_id]->rdec_cfg.ch);
    rdec_debug("rdec_info[%d]->rdec_cfg.mode = %d\n", cfg_id, (u32)rdec_info[cfg_id]->rdec_cfg.mode);
    rdec_debug("rdec_info[%d]->rdec_cfg.port_0 = %d\n", cfg_id, (u32)rdec_info[cfg_id]->rdec_cfg.port_0);
    rdec_debug("rdec_info[%d]->rdec_cfg.port_1 = %d\n", cfg_id, (u32)rdec_info[cfg_id]->rdec_cfg.port_1);
    rdec_debug("rdec_info[%d]->rdec_cfg.clk = %d\n", cfg_id, (u32)rdec_info[cfg_id]->rdec_cfg.clk);
    return cfg_id;
}

void rdec_deinit(int rdec_cfg_id)
{
    ASSERT(rdec_info[rdec_cfg_id] != NULL, "func:%s(), line:%d\n", __func__, __LINE__);
    JL_RDEC_TypeDef *rdec_reg = rdec_info[rdec_cfg_id]->rdecx;
    spin_lock(&rdec_lock);
    rdec_reg->CON = BIT(QDEC_CPND);
    rdec_reg->SMP = 0;
    spin_unlock(&rdec_lock);
    gpio_disable_function(IO_PORT_SPILT(rdec_info[rdec_cfg_id]->rdec_cfg.port_0), PORT_FUNC_RDEC0_PORT0 + 2 * rdec_cfg_id);
    gpio_disable_function(IO_PORT_SPILT(rdec_info[rdec_cfg_id]->rdec_cfg.port_1), PORT_FUNC_RDEC0_PORT1 + 2 * rdec_cfg_id);
    free(rdec_info[rdec_cfg_id]);
    memset(rdec_info[rdec_cfg_id], 0, sizeof(struct rdec_info_t));
    rdec_info[rdec_cfg_id] = NULL;
}

void rdec_start(int rdec_cfg_id)
{
    ASSERT(rdec_info[rdec_cfg_id] != NULL, "func:%s(), line:%d\n", __func__, __LINE__);
    JL_RDEC_TypeDef *rdec_reg = rdec_info[rdec_cfg_id]->rdecx;
    rdec_cfg_info_load(rdec_cfg_id);
    spin_lock(&rdec_lock);
    SFR(rdec_reg->CON, QDEC_EN, 1, 1);
    spin_unlock(&rdec_lock);
    rdec_reg_log_info(rdec_cfg_id);
}

void rdec_pause(int rdec_cfg_id)
{
    ASSERT(rdec_info[rdec_cfg_id] != NULL, "func:%s(), line:%d\n", __func__, __LINE__);
    JL_RDEC_TypeDef *rdec_reg = rdec_info[rdec_cfg_id]->rdecx;
    spin_lock(&rdec_lock);
    SFR(rdec_reg->CON, QDEC_EN, 1, 0);
    spin_unlock(&rdec_lock);
}

void rdec_rsume(int rdec_cfg_id)
{
    rdec_start(rdec_cfg_id);
}

s8 rdec_get_value(int rdec_cfg_id)
{
    ASSERT(rdec_info[rdec_cfg_id] != NULL, "func:%s(), line:%d\n", __func__, __LINE__);
    JL_RDEC_TypeDef *rdec_reg = rdec_info[rdec_cfg_id]->rdecx;
    s8 dat = 0;
    if (rdec_reg->CON & BIT(QDEC_PND)) {
        rdec_reg->CON |= BIT(QDEC_CPND);
        asm("csync");
        dat = rdec_reg->DAT;
    }
    return dat;
}



#if 0
//测试使用代码
#include "timer.h"
int usr_cfg_id;
int data = 0;
static void rdec_timer_test_func()
{
    rdec_debug("Youbaibai rdec_timer_test_func\n");
    s8 dat = rdec_get_value(usr_cfg_id);
    data += dat;
    rdec_debug("data = %d, dat = %d\n", data, dat);
}
void rdec_test_func()
{
    rdec_debug("Youbaibai rdec_test_func\n");
    usr_timer_add(NULL, rdec_timer_test_func, 1000, 1); //1s读取一次编码器值
    struct rdec_config usr_rdec_cfg = {
        .ch = RDEC_0,
        .mode = RDEC_PHASE_2,
        .port_0 = IO_PORTA_02,
        .port_1 = IO_PORTA_04,
        .clk = -1,
    };
    usr_cfg_id =  rdec_init(&usr_rdec_cfg);
    rdec_start(usr_cfg_id);
    /* rdec_pause(usr_cfg_id); */

}
#endif
