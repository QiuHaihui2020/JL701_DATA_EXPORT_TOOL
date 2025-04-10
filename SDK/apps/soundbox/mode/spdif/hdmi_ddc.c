#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".hdmi_ddc.data.bss")
#pragma data_seg(".hdmi_ddc.data")
#pragma const_seg(".hdmi_ddc.text.const")
#pragma code_seg(".hdmi_ddc.text")
#endif
#include "hdmi_cec.h"
#include "app_config.h"
#include "clock.h"
#include "app_config.h"
#include "gpio.h"
#include "iic_api.h"
#include "spdif_file.h"
#include "gpio_config.h"

#if (TCFG_SPDIF_ENABLE)


/* #define     TCFG_HDMI_IIC_SCL_PORT  IO_PORTC_04 */
/* #define     TCFG_HDMI_IIC_SDA_PORT  IO_PORTA_07 */

u8 iic_num = 0;
int hdmi_ddc_read(u8 *buffer, u8 addr, u32 len)
{
    return i2c_master_read_nbytes_from_device_reg(iic_num, 0xa0, &addr, 1, buffer, len);
}


void audio_block_decode(u8 *data, int length)
{
    char *audio_format[] = {
        "reserved",
        "Linear pulse-code modulation (LPCM)",
        "AC-3",
        "MPEG-1 (Layers 1 and 2)",
        "MP3",
        "MPEG-2",
        "AAC LC",
        "DTS",
        "ATRAC",
        "DSD (One-Bit) Audio, Super Audio CD",
        "DD+",
        "DTS-HD",
        "MAT/MLP/Dolby TrueHD",
        "DST Audio",
        "Microsoft WMA Pro",
        "Extension, see Byte 3"
    };

    int audio_format_code = data[1] >> 3;
    int num_of_channels = (data[1] & 0x7) + 1;
    printf("    %s\n", audio_format[audio_format_code]);
    printf("      Max channels: %d\n", num_of_channels);

    char *freq[] = {
        "res",
        "192",
        "176",
        "96",
        "88",
        "48",
        "44.1",
        "32"
    };
    printf("      Supported sample rates (kHz): ");
    for (int i = 0; i < 7; i++) {
        if (data[2] & (1 << i)) {
            printf("%s ", freq[6 - i]);
        }
    }


    char *bits[] = {
        "24",
        "20",
        "16"
    };
    printf("      Supported sample sizes (bits): ");
    for (int i = 0; i < 3; i++) {
        if (data[3] & (1 << i)) {
            printf("%s ", bits[2 - i]);
        }
    }
}
u32 hdmi_ddc_get_cec_physical_address(void)
{
    struct spdif_file_cfg *p_spdif_cfg;	//spdif的配置参数信息
    p_spdif_cfg = audio_spdif_file_get_cfg();


    if (uuid2gpio(p_spdif_cfg->iic_scl_port) == 0xff || uuid2gpio(p_spdif_cfg->iic_sda_port) == 0xff) {
        printf("un config iic port\n");
        return 0;
    }
    struct iic_master_config iic_cfg = {
        .scl_io = uuid2gpio(p_spdif_cfg->iic_scl_port),
        .sda_io = uuid2gpio(p_spdif_cfg->iic_sda_port),
        .io_mode = PORT_INPUT_PULLUP_10K,
        .hdrive = PORT_DRIVE_STRENGT_8p0mA,
        .master_frequency = 100000,
    };
    iic_num = p_spdif_cfg->iic_num;
    printf("\n iic scl[%x]sda[%x] \n", iic_cfg.scl_io, iic_cfg.sda_io);
    iic_init(iic_num, &iic_cfg);


    u8 *edid_data = (u8 *)malloc(128);
    memset(edid_data, 0, 128);

    for (int i = 0; i < 128; i += 8) {
        if (hdmi_ddc_read(edid_data + i, 0x80 + i, 8) == IIC_OK) {
            put_buf(edid_data + i, 8);
        }
    }
    iic_uninit(0);



    if (edid_data[0] == 0x02) {
        printf("CTA-861 Extension Block\n");
    }

    if (edid_data[1] == 0x03) {
        printf("  Revision: 3\n");
    }

    int byte_number = edid_data[2];
    if (edid_data[3] & (1 << 4)) {
        printf("  Supports YCbCr 4:4:2\n");
    }

    if (edid_data[3] & (1 << 5)) {
        printf("  Supports YCbCr 4:4:4\n");
    }

    if (edid_data[3] & (1 << 6)) {
        printf("  Basic audio support\n");
    }

    if (edid_data[3] & (1 << 7)) {
        printf("  Underscans IT Video Formats by default\n");
    }

    printf(" Native detailed modes:%d\n", edid_data[3] & 0xf);


    int offset = 4;
    u32 addres = 0;

    while (offset < byte_number) {
        int block_head = edid_data[offset];
        int block_type = block_head >> 5;
        int block_length = block_head & 0x1f;

        if (block_length == 0) {
            break;
        }

        if (block_type == 1) {
            printf("Audio Data Block:\n");
            audio_block_decode(edid_data + offset, block_length);

        } else if (block_type == 3) {

            printf("Vendor-Specific Data Block OUI %02X-%02X-%02X\n",
                   edid_data[offset + 1], edid_data[offset + 2], edid_data[offset + 3]);

            if (edid_data[offset + 1] == 0x03 &&
                edid_data[offset + 2] == 0x0c &&
                edid_data[offset + 3] == 0x00) {

                printf("     Source physical address: %02X %02X\n",
                       edid_data[offset + 4], edid_data[offset + 5]);
                addres = (edid_data[offset + 4] << 8) | edid_data[offset + 5] ;
            }
        }

        offset += block_length;
        offset += 1;
    }

    free(edid_data);

    return addres;
}

#endif

