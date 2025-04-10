#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".QN8035.data.bss")
#pragma data_seg(".QN8035.data")
#pragma const_seg(".QN8035.text.const")
#pragma code_seg(".QN8035.text")
#endif
/*--------------------------------------------------------------------------*/
/**@file     qn8035.c
  @brief    qn8035收音底层驱动
  @details
  @auth|=  @date   2011-11-24
  @note
 */
/*----------------------------------------------------------------------------*/

#include "app_config.h"
#include "system/includes.h"
#include "fm_manage.h"
#include "gpio_config.h"
#include "QN8035.h"

#if (TCFG_FM_QN8035_ENABLE && TCFG_APP_FM_EN && TCFG_FM_OUTSIDE_ENABLE)

#define USE_NEW_IIC_DRIVER  1

//#include "sdmmc_api.h"

//if antenna match circuit is used a inductor，macro USING_INDUCTOR will be set to 1
#define USING_INDUCTOR				0

#define INVERSE_IMR						1

//if using san noise floor as CCA algorithm,macro SCAN_NOISE_FLOOR_CCA will be set to 1
#define SCAN_NOISE_FLOOR_CCA 	1
//if using pilot as CCA algorithm,macro PILOT_CCA will be set to 1
//#define PILOT_CCA					0

//长天线配置:
//u8 _xdata qnd_PreNoiseFloor = 40,qnd_NoiseFloor = 40;
bool qn8035_online;
//短天线配置:


//提供给 qn8035 时钟IO 结构体
struct qn8035_clk_io_t {
    u8 get_io_flag;
    u8 clk_io;
    u8 clk_out_en;
};
static struct qn8035_clk_io_t qn8035_clk_io = {0};

static struct _fm_dev_info *fm_dev_info = NULL;

extern void mdelay(u32 ms);

#define delay_n10ms(x)      \
	mdelay(x*10)

/*----------------------------------------------------------------------------*/
/**@brief    QN8035 读寄存器函数
  @param    addr：寄存器地址
  @return   无
  @note     u8 QND_ReadReg(u8 addr)
//5_5_9_5
//4_5_8_5
//2_5_8_5
 */

#define	qn8035_rssi 	 	4      //越大台越少
#define qn8035_if2  		0x05    //越大台越多
#define qn8035_snr_th1  	0x08    //越大台越少 //
#define	qn8035_snr_th2 		0x05
#define	qn8035_pilot_cca  	1
#define	qn8035_inductor  	0
#define	qn8075_snr   		25
u8 qnd_PreNoiseFloor = 35;   //越大台越少
u8 qnd_NoiseFloor = 35;


static void qn8035_clk_io_init(void)
{
    if (qn8035_clk_io.get_io_flag == 0) {
        int clk_io_uuid = fm_qn8035_get_clk_io_uuid();
        if (clk_io_uuid == -1) {
            r_printf("Error: %s, %d, get clk_io uuid failed!\n", __func__, __LINE__);
            return;
        } else {
            qn8035_clk_io.clk_io = uuid2gpio((u16)clk_io_uuid);
            qn8035_clk_io.get_io_flag = 1;
            y_printf(">>>>>> qn8035_io_clk:%d\n", qn8035_clk_io.clk_io);
        }
    }
}

//引脚给 qn8035 模块提供24MHz 的时钟
static void qn8035_io_supply_24M_clk(void)
{
    qn8035_clk_io_init();
    /* local_irq_disable(); */
    if (qn8035_clk_io.clk_out_en == 0) {
        qn8035_clk_io.clk_out_en = 1;
        clk_out(qn8035_clk_io.clk_io, CLK_OUT_STD_24M, 1);
    }
    /* local_irq_enable(); */
}

static void qn8035_io_no_supply_clk(void)
{
    qn8035_clk_io_init();
    if (qn8035_clk_io.clk_out_en == 1) {
        qn8035_clk_io.clk_out_en = 0;
        gpio_och_disable_output_signal(qn8035_clk_io.clk_io, OUTPUT_CH_SIGNAL_CLOCK_OUT1);
    }
    /* gpio_setmode(qn8035_clk_io.clk_io/IO_GROUP_NUM, qn8035_clk_io.clk_io%IO_GROUP_NUM, PORT_OUTPUT_LOW); */
}

u8 QND_ReadReg(u8 addr)
{
    //旧版驱动
#if USE_NEW_IIC_DRIVER
    u8 byte = 0;

    u8 reg_addr[1] = {addr};
    u8 read_buf[1] = {0};
    int ret = hw_i2c_master_read_nbytes_from_device_reg(fm_dev_info->iic_hdl, 0x20, reg_addr, 1, read_buf, 1);
    if (ret < 0) {
        r_printf(">> %s, iic read failed! ret = %d\n", __func__, ret);
    }
    byte = read_buf[0];
    return byte;

#else
    u8  byte;
    iic_start(fm_dev_info->iic_hdl);                    //I2C启动
    iic_tx_byte(fm_dev_info->iic_hdl, 0x20); 		//写命令
    iic_tx_byte(fm_dev_info->iic_hdl, addr); 		//写地址
    /* iic_sendbyte(0x20);             //写命令 */
    /* iic_sendbyte(addr);         //写地址 */

    iic_start(fm_dev_info->iic_hdl);                    //写转为读命令，需要再次启动I2C
    iic_tx_byte(fm_dev_info->iic_hdl, 0x21); 		//读命令
    byte = iic_rx_byte(fm_dev_info->iic_hdl, 1);
    /* iic_sendbyte(0x21);             //读命令 */
    /* byte = iic_revbyte(1); */

    iic_stop(fm_dev_info->iic_hdl);                     //I2C停止
    return  byte;
#endif
}

//写成功返回1，失败返回0
u8 fm_dev_8035_iic_write(u8 w_chip_id, u8 register_address, u8 *buf, u32 data_len)
{
    u8 ret = 1;
    int err = 0;

#if  USE_NEW_IIC_DRIVER
    //w_chip_id : dev_addr
    // register_address : reg_addr
    u8 reg_addr[1] = {register_address};
    /* u8 reg_len = 1; */
    u8 *pbuf = buf;
    err = hw_i2c_master_write_nbytes_to_device_reg(fm_dev_info->iic_hdl, w_chip_id, reg_addr, 1, pbuf, (int)data_len);
    if (err < 0 || err != data_len) {
        ret = 0;
    }

#else
    iic_start(fm_dev_info->iic_hdl);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, w_chip_id)) {
        ret = 0;
        /* printf("\n fm iic wr err 0\n"); */
        goto __gcend;
    }

    delay_nops(fm_dev_info->iic_delay);
    if (0xff != register_address) {
        if (0 == iic_tx_byte(fm_dev_info->iic_hdl, register_address)) {
            ret = 0;
            /* printf("\n fm iic wr err 1:0x%x\n", register_address); */
            goto __gcend;
        }
    }
    u8 *pbuf = buf;

    while (data_len--) {
        delay_nops(fm_dev_info->iic_delay);

        if (0 == iic_tx_byte(fm_dev_info->iic_hdl, *pbuf++)) {
            ret = 0;
            /* printf("\n fm iic wr err 2\n"); */
            goto __gcend;
        }
    }

__gcend:
    iic_stop(fm_dev_info->iic_hdl);
#endif
    return ret;
}

u8 fm_dev_8035_iic_readn(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len)
{
    u8 read_len = 0;
    u8 tmp_da;
    /* i2c_master_read_nbytes_from_device_reg(fm_dev_info->iic_hdl, r_chip_id, register_address, buf, data_len); */

#if 1
    if (fm_dev_info == NULL) {
        printf("fm_dev_info  not init !!!!!");
    }

    iic_start(fm_dev_info->iic_hdl);
    if (0 == iic_tx_byte(fm_dev_info->iic_hdl, r_chip_id)) {
        printf("\n fm iic rd err 0\n");
        read_len = 0;
        goto __gdend;
    }


    delay_nops(fm_dev_info->iic_delay);
    if (0xff != register_address) {
        if (0 == iic_tx_byte(fm_dev_info->iic_hdl, register_address)) {
            printf("\n fm iic rd err 1\n");
            read_len = 0;
            goto __gdend;
        }
    }

    delay_nops(fm_dev_info->iic_delay);

    for (; data_len > 1; data_len--) {
        *buf++ = iic_rx_byte(fm_dev_info->iic_hdl, 1);
        read_len ++;
    }

    *buf = iic_rx_byte(fm_dev_info->iic_hdl, 0);
__gdend:
    printf("buf==%u", *buf);

    iic_stop(fm_dev_info->iic_hdl);
    delay_nops(fm_dev_info->iic_delay);
#endif

    return read_len;
}

u8 qn8035_iic_write(u8 w_chip_id, u8 register_address, u8 *buf, u32 data_len)
{
    u16 rewrite_cnt = 0;
__rewrite:
    u8 ret = fm_dev_8035_iic_write(w_chip_id, register_address, buf, data_len);
    if (ret == 0) {
        if (rewrite_cnt <= 50) {
            rewrite_cnt++;
            goto __rewrite;
        }
    }
    if (ret == 0) {
        /* r_printf("%s, %d, qn8035 iic write failed!\n", __func__, __LINE__); */
    }
    return ret;
}

u8 qn8035_iic_readn(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len)
{
    u8 ret = fm_dev_8035_iic_readn(r_chip_id, register_address, buf, data_len);
    return ret;
}

/*----------------------------------------------------------------------------*/
/**@brief    QN8035 写寄存器函数
  @param    addr：寄存器地址 data：写入数据
  @return   无
  @note     void QND_WriteReg(u8 addr,u8 data)
 */
/*----------------------------------------------------------------------------*/
void QND_WriteReg(u8 addr, u8 data)
{
    qn8035_iic_write(0x20, addr, &data, 1);
}


/**********************************************************************
  void QNF_SetCh(UINT16 start,UINT16 stop,UINT8 step)
 **********************************************************************
Description: set channel frequency
Parameters:
freq:  channel frequency to be set,frequency unit is 10KHZ
Return Value:
None
 **********************************************************************/
void QNF_SetCh(u16 start, u16 stop, u8 step)
{
    u8 temp;

    start = FREQ2CHREG(start);
    stop = FREQ2CHREG(stop);
    //writing lower 8 bits of CCA channel start index
    QND_WriteReg(CH_START, (u8)start);
    //writing lower 8 bits of CCA channel stop index
    QND_WriteReg(CH_STOP, (u8)stop);
    //writing lower 8 bits of channel index
    QND_WriteReg(CH, (u8)start);
    //writing higher bits of CCA channel start,stop and step index
    temp = (u8)((start >> 8) & CH_CH);
    temp |= ((u8)(start >> 6) & CH_CH_START);
    temp |= ((u8)(stop >> 4) & CH_CH_STOP);
    temp |= (step << 6);
    QND_WriteReg(CH_STEP, temp);
}


/**********************************************************************
  int QND_Delay()
 **********************************************************************
Description: Delay for some ms, to be customized according to user
application platform

Parameters:
ms: ms counts
Return Value:
None

 **********************************************************************/
void QND_Delay(u16 ms)
{
    //	Delay(25*ms);   		//rc
    //	delay16(1500*ms);
    delay_n10ms(1);
}

void qn8035_mute1(bool On)
{
    QND_WriteReg(0x4a,  On ? 0x30 : 0x10);
    //QND_WriteReg(0x4a, 0x30);
}
#define QNF_SetMute(x,y) 	qn8035_mute(x,y)
/**********************************************************************
  void QNF_SetMute(u8 On)
 **********************************************************************
Description: set register specified bit

Parameters:
On:        1: mute, 0: unmute
Return Value:
None
 **********************************************************************/
u8 qn8035_mute(void *priv, u8 On)
{
    qn8035_io_no_supply_clk();
    u8 regVal;
    /* if (fm_dev_info == NULL)  */
    {
        fm_dev_info = (struct _fm_dev_info *)priv;
    }
#if 1
    //旧版 mute
    QND_WriteReg(REG_DAC, On ? 0x1B : 0x10);
#else
    //新版 mute
    regVal = QN_IIC_read(VOL_CTL) & 0x7F;
    if (On) {
        regVal |= BIT(7);
    }
    QND_WriteReg(VOL_CTL, regVal);
#endif
    qn8035_io_supply_24M_clk();
    return 0;
}

#if SCAN_NOISE_FLOOR_CCA
/***********************************************************************
Description: scan a noise floor from 87.5M to 108M by step 200K
Parameters:
Return Value:
1: scan a noise floor successfully.
0: chip would not normally work.
 **********************************************************************/
u8 QND_ScanNoiseFloor(u16 start, u16 stop)
{
    u8 regValue;
    u8 timeOut = 255; //time out is 2.55S

    QND_WriteReg(CCA_SNR_TH_1, 0x00);
    QND_WriteReg(CCA_SNR_TH_2, 0x05);
    QND_WriteReg(0x40, 0xf0);
    //config CCS frequency rang by step 200KHZ
    QNF_SetCh(start, stop, 2);
    /*
       QND_WriteReg(CH_START,0x26);
       QND_WriteReg(CH_STOP,0xc0);
       QND_WriteReg(CH_STEP,0xb8);
     */
    //enter CCA mode,channel index is decided by internal CCA
    QND_WriteReg(SYSTEM1, 0x12);
    while (1) {
        regValue = QN_IIC_read(SYSTEM1);
        //if it seeks a potential channel, the loop will be quited
        if ((regValue & CHSC) == 0) {
            break;
        }
        delay_n10ms(1);   //delay 10ms
        //if it was time out,chip would not normally work.
        if ((timeOut--) == 0) {
            QND_WriteReg(0x40, 0x70);
            return 0;
        }
    }
    QND_WriteReg(0x40, 0x70);
    qnd_NoiseFloor = QN_IIC_read(0x3f);
    if (((qnd_PreNoiseFloor - qnd_NoiseFloor) > 2) || ((qnd_NoiseFloor - qnd_PreNoiseFloor) > 2)) {
        qnd_PreNoiseFloor = qnd_NoiseFloor;
    }
    //TRACE("NF:%d,timeOut:%d\n",qnd_NoiseFloor,255-timeOut);
    return 1;
}
#endif
/**********************************************************************
  void QNF_SetRegBit(u8 reg, u8 bitMask, u8 data_val)
 **********************************************************************
Description: set register specified bit

Parameters:
reg:        register that will be set
bitMask:    mask specified bit of register
data_val:    data will be set for specified bit
Return Value:
None
 ***********************************************************************/
void QND_RXSetTH(void)
{
    u8 rssi_th;

    rssi_th = qnd_PreNoiseFloor + qn8035_rssi - 28 ;  //10	越小台多0-
    ///increase reference PLL charge pump current.
    QND_WriteReg(REG_REF, 0x7a);
    //NFILT program is enabled
    QND_WriteReg(0x1b, 0x78);
    //using Filter3
    QND_WriteReg(CCA1, 0x75);
    //setting CCA IF counter error range value(768).
    QND_WriteReg(CCA_CNT2, qn8035_if2); //0x03	  大台多 1-5
    //#if PILOT_CCA
    if (qn8035_pilot_cca) {
        QND_WriteReg(PLT1, 0x00);
    }
    //#endif
    //selection the time of CCA FSM wait SNR calculator to settle:20ms
    //0x00:	    20ms(default)
    //0x40:	    40ms
    //0x80:	    60ms
    //0xC0:	    100m
    //    QNF_SetRegBit(CCA_SNR_TH_1 , 0xC0, 0x00);
    //selection the time of CCA FSM wait RF front end and AGC to settle:20ms
    //0x00:     10ms
    //0x40:     20ms(default)
    //0x80:     40ms
    //0xC0:     60ms
    //    QNF_SetRegBit(CCA_SNR_TH_2, 0xC0, 0x40);
    //    QNF_SetRegBit(CCA, 30);  //setting CCA RSSI threshold is 30
    QND_WriteReg(CCA_SNR_TH_2, qn8035_snr_th2); //0xc5
    QND_WriteReg(CCA, (QN_IIC_read(CCA) & 0xc0) | rssi_th);
    //#if PILOT_CCA
    if (qn8035_pilot_cca) {
        QND_WriteReg(CCA_SNR_TH_1, qn8035_snr_th1);    //setting SNR threshold for CCA
    } else {
        QND_WriteReg(CCA_SNR_TH_1, qn8035_snr_th1);    //小台多 8-12 //setting SNR threshold for CCA  9
    }
    //#endif
}



/*----------------------------------------------------------------------------*/
/**@brief    QN0835 初始化
  @param    无
  @return   无
  @note     u8 qn8035_init(void *priv)
 */
/*----------------------------------------------------------------------------*/
u8 qn8035_init(void *priv)
{
    /* if (fm_dev_info == NULL)  */
    {
        fm_dev_info = (struct _fm_dev_info *)priv;
    }
    puts("qn8035_init\n");
    QND_WriteReg(0x00, 0x81);
    delay_n10ms(1);

    /*********User sets chip working clock **********/
    //Following is where change the input clock wave type,as sine-wave or square-wave.
    //default set is 32.768KHZ square-wave input.
    QND_WriteReg(0x01, QND_SINE_WAVE_CLOCK);
    //QND_WriteReg(0x58,0x93);
    //Following is where change the input clock frequency.
    QND_WriteReg(XTAL_DIV0, QND_XTAL_DIV0);
    QND_WriteReg(XTAL_DIV1, QND_XTAL_DIV1);
    QND_WriteReg(XTAL_DIV2, QND_XTAL_DIV2);
    /********User sets chip working clock end ********/

    QND_WriteReg(0x54, 0x47);//mod PLL setting
    //select SNR as filter3,SM step is 2db
    QND_WriteReg(0x19, 0xc4);
    QND_WriteReg(0x33, 0x9e);//set HCC and SM Hystersis 5db
    QND_WriteReg(0x2d, 0xd6);//notch filter threshold adjusting
    QND_WriteReg(0x43, 0x10);//notch filter threshold enable
    QND_WriteReg(0x47, 0x39);
    //QND_WriteReg(0x57, 0x21);//only for qn8035B test
    //enter receiver mode directly
    QND_WriteReg(0x00, 0x11);
    //Enable the channel condition filter3 adaptation,Let ccfilter3 adjust freely
    QND_WriteReg(0x1d, 0xa9);
    QND_WriteReg(0x4f, 0x40);//dsiable auto tuning
    QND_WriteReg(0x34, SMSTART_VAL); ///set SMSTART
    QND_WriteReg(0x35, SNCSTART_VAL); ///set SNCSTART
    QND_WriteReg(0x36, HCCSTART_VAL); ///set HCCSTART
    QNF_SetMute((void *)fm_dev_info, 0);

    //dac_channel_on(FM_IIC_CHANNEL, FADE_ON);

    //QN8035 模块需要外供时钟, 时钟为24M
    qn8035_io_supply_24M_clk();
    delay_n10ms(2);
    // 解mute让qn8035出声
    qn8035_mute(priv, 1);
    delay_n10ms(2);
    qn8035_mute(priv, 0);

    return 0;
}


/*----------------------------------------------------------------------------*/
/**@brief    关闭 QN0835的电源
  @param    无
  @return   无
  @note     u8 QN8035_powerdown(void *priv)
 */
/*----------------------------------------------------------------------------*/
u8 qn8035_powerdown(void *priv)
{
    qn8035_io_no_supply_clk();
    /* if (fm_dev_info == NULL)  */
    {
        fm_dev_info = (struct _fm_dev_info *)priv;
    }
    puts("qn8035_powerdown\n");
    //	QND_SetSysMode(0);
    QNF_SetMute((void *)fm_dev_info, 1);
    QND_WriteReg(SYSTEM1, 0x20);

    qn8035_io_no_supply_clk();
    //dac_channel_off(FM_IIC_CHANNEL, FADE_ON);
    return 0;
}
/**********************************************************************
  void QND_TuneToCH(UINT16 ch)
 **********************************************************************
Description: Tune to the specific channel.
Parameters:
ch:Set the frequency (10kHz) to be tuned,
eg: 101.30MHz will be set to 10130.
Return Value:
None
 **********************************************************************/
void QND_TuneToCH(u16 channel)
{
    u8 reg;
    u16 ch;
    ch = channel * 10;
    //increase reference PLL charge pump current.
    QND_WriteReg(REG_REF, 0x7a);

    /********** QNF_RXInit ****************/
    QND_WriteReg(0x1b, 0x70);	//Let NFILT adjust freely
    QND_WriteReg(0x2c, 0x52);
    QND_WriteReg(0x45, 0x50);	//Set aud_thrd will affect ccfilter3's tap number
    QND_WriteReg(0x40, 0x70);	//set SNR as SM,SNC,HCC MPX
    QND_WriteReg(0x41, 0xca);
    /********** End of QNF_RXInit ****************/
#if INVERSE_IMR
    reg = QN_IIC_read(CCA) & ~0x40;
    if ((ch == 9340) || (ch == 9390) || (ch == 9530) || (ch == 9980) || (ch == 10480)) {
        reg |= 0x40;	// inverse IMR.
    } else {
        reg &= ~0x40;
    }
    QND_WriteReg(CCA, reg);
#endif
    QNF_SetMute((void *)fm_dev_info, 1);
    QNF_SetCh(ch, ch, 1);
    //enable CCA mode with user write into frequency
    QND_WriteReg(0x00, 0x13);
    //#if USING_INDUCTOR
    if (qn8035_inductor) {
        //Auto tuning
        QND_WriteReg(0x4f, 0x80);
        reg = QN_IIC_read(0x4f);
        reg >>= 1;
        QND_WriteReg(0x4f, reg);
    }
    //#endif
    ///avoid the "POP" noise.
    //QND_Delay(CH_SETUP_DELAY_TIME);
    //QND_Delay(500);

    delay_n10ms(10);

    ///decrease reference PLL charge pump current.
    QND_WriteReg(REG_REF, 0x70);
    QNF_SetMute((void *)fm_dev_info, 0);
}



/*----------------------------------------------------------------------------*/
/**@brief    设置一个频点QN0835
  @param    fre 频点  875~1080
  @return   1：当前频点有台，0：当前频点无台
  @note     bool set_fre_QN8035(u16 freq)
 */
/*----------------------------------------------------------------------------*/
bool QND_RXValidCH(u16 freq)
{
    printf("set_8035_frq %d ", freq);
    u8 regValue;
    u8 timeOut;
    u8 isValidChannelFlag;
    //	UINT8 snr,snc,temp1,temp2;
#if PILOT_CCA
    u8 snr, readCnt, stereoCount = 0;
#endif
    /* freq = freq * 10; */
#if SCAN_NOISE_FLOOR_CCA
    switch (freq) {
    case 8750:
        QND_ScanNoiseFloor(8750, 8800);
        QND_RXSetTH();
        break;
    case 8810:
        QND_ScanNoiseFloor(8810, 9200);
        QND_RXSetTH();
        break;
    case 9210:
        QND_ScanNoiseFloor(9210, 9600);
        QND_RXSetTH();
        break;
    case 9610:
        QND_ScanNoiseFloor(9610, 10000);
        QND_RXSetTH();
        break;
    case 10010:
        QND_ScanNoiseFloor(10010, 10400);
        QND_RXSetTH();
        break;
    case 10410:
        QND_ScanNoiseFloor(10410, 10800);
        QND_RXSetTH();
        break;
    default:
        //QND_Delay(350);
        break;
    }
#endif
    QNF_SetCh(freq, freq, 1);
    //#if USING_INDUCTOR
    if (qn8035_inductor) {
        //Auto tuning
        QND_WriteReg(0x00, 0x11);
        QND_WriteReg(0x4f, 0x80);
        regValue = QN_IIC_read(0x4f);
        regValue = (regValue >> 1);
        QND_WriteReg(0x4f, regValue);
    }

    //#endif
    //entering into RX mode and CCA mode,channels index decide by CCA.
    QND_WriteReg(0x00, 0x12);
    timeOut = 20;  // time out is 100ms
    while (1) {
        regValue = QN_IIC_read(SYSTEM1);
        //if it seeks a potential channel, the loop will be quited
        if ((regValue & CHSC) == 0) {
            break;
        }
        delay_n10ms(1);   //delay 5ms
        //if it was time out,chip would not normally work.
        if ((timeOut--) == 0) {
            return 0;
        }
    }
    //reading out the rxcca_fail flag of RXCCA status
    isValidChannelFlag = (QN_IIC_read(STATUS1) & RXCCA_FAIL ? 0 : 1);
    if (isValidChannelFlag) {
        //#if PILOT_CCA
        if (qn8035_pilot_cca) {

            for (readCnt = 10; readCnt > 0; readCnt--) {
                delay_n10ms(2);
                stereoCount += ((QN_IIC_read(STATUS1) & ST_MO_RX) ? 0 : 1);
                if (stereoCount >= 3) {
                    return 1;
                }
            }


            delay_n10ms(10);
            snr = QN_IIC_read(SNR);
            if (snr > qn8075_snr) {
                return 1;
            }
        } else {
            return 1;
        }
        //#endif
    }
    return 0;
}
/**/
u8 qn8035_set_fre(void *priv, u16 fre)
{
    qn8035_io_no_supply_clk();
    /* if (fm_dev_info == NULL)  */
    {
        fm_dev_info = (struct _fm_dev_info *)priv;
    }

    if (QND_RXValidCH(fre)) {
        //QND_TuneToCH(fre);
        qn8035_io_supply_24M_clk();
        return TRUE;
    }
    qn8035_io_supply_24M_clk();
    return FALSE;
}


void QND_SetVol(u8 vol)
{
    u8 sVol;
    u8 regVal;
    sVol = vol * 3 + 2;
    regVal = QN_IIC_read(VOL_CTL);
    regVal = (regVal & 0xC0) | (sVol / 6) | ((5 - (sVol % 6)) << 3);
    //regVal = (regVal&0xC0)|0x02|0x06;
    QND_WriteReg(VOL_CTL, regVal);
}

u8 qn8035_iic_init(void *priv)
{
    fm_dev_info = (struct _fm_dev_info *)priv;

    int ret = hw_iic_init(fm_dev_info->iic_hdl, get_hw_iic_config(fm_dev_info->iic_hdl));

    return 1;
}

u8 qn8035_iic_uninit(void *priv)
{
    fm_dev_info = (struct _fm_dev_info *)priv;

    int ret = hw_iic_uninit(fm_dev_info->iic_hdl);

    return 1;
}

/*----------------------------------------------------------------------------*/
/**@brief   FM模块检测，获取QN0835 模块ID
  @param   无
  @return  检测到QN0835模块返回1，否则返回0
  @note    u8 QN8035_Read_ID(void *priv)
 */
/*----------------------------------------------------------------------------*/
u8 qn8035_read_id(void *priv)
{
    if (fm_get_device_en("qn8035")) {
        u8 cChipID;
        u16 read_cnt = 0;
        /* if (fm_dev_info == NULL)  */
        {
            fm_dev_info = (struct _fm_dev_info *)priv;
        }

__re_read:
        cChipID = QN_IIC_read(CID2);
        cChipID &= 0xfc;
        if (0x84 == cChipID) {
            qn8035_online = 1;
            return 1;
        } else {
            qn8035_online = 0;
            if (read_cnt <= 3) {
                read_cnt++;
                goto __re_read;
            }
            qn8035_iic_uninit(fm_dev_info);
            return 0;
        }
    }
    qn8035_iic_uninit(fm_dev_info);
    return 0;
}

void qn8035_setch(u8 db)
{
    QND_RXSetTH();
}


REGISTER_FM(qn8035) = {
    .logo          = "qn8035",
    .fm_iic_init   = qn8035_iic_init,
    .fm_iic_uninit = qn8035_iic_uninit,
    .init          = qn8035_init,
    .close         = qn8035_powerdown,
    .set_fre       = qn8035_set_fre,
    .mute          = qn8035_mute,
    .read_id       = qn8035_read_id,
};

#endif
