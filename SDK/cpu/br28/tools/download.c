// *INDENT-OFF*
#include "app_config.h"

#ifdef __SHELL__

##!/bin/sh

${OBJDUMP} -D -address-mask=0x7ffffff -print-imm-hex -print-dbg -mcpu=r3 $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
${OBJCOPY} -O binary -j .data  $1.elf data.bin
${OBJCOPY} -O binary -j .moveable_slot $1.elf mov_slot.bin
${OBJCOPY} -O binary -j .data_code $1.elf data_code.bin
${OBJCOPY} -O binary -j .overlay_aec $1.elf aec.bin
${OBJCOPY} -O binary -j .overlay_aac $1.elf aac.bin
${OBJCOPY} -O binary -j .psr_data_code $1.elf psr_data_code.bin

${OBJDUMP} -section-headers -address-mask=0x7ffffff $1.elf > segment_list.txt
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 >  symbol_tbl.txt


/opt/utils/report_segment_usage --sdk_path ${ROOT} \
--tbl_file symbol_tbl.txt \
--seg_file segment_list.txt \
--map_file $1.map \
--module_depth "{\"apps\":1,\"lib\":2,\"[lib]\":2}"

cat text.bin data.bin mov_slot.bin data_code.bin aec.bin aac.bin psr_data_code.bin > app.bin

cat segment_list.txt
/* if [ -f version ]; then */
    /* host-client -project ${NICKNAME}$2 -f app.bin version $1.elf p11_code.bin br28loader.bin br28loader.uart uboot.boot uboot.boot_debug ota.bin ota_debug.bin isd_config.ini */
/* else */
    /* host-client -project ${NICKNAME}$2 -f app.bin $1.elf  p11_code.bin br28loader.bin br28loader.uart uboot.boot uboot.boot_debug ota.bin ota_debug.bin isd_config.ini */

/* fi */

/opt/utils/strip-ini -i isd_config.ini -o isd_config.ini

files="app.bin ${CPU}loader.* uboot*  ota*.bin p11_code.bin isd_config.ini isd_download.exe fw_add.exe ufw_maker.exe"


host-client -project ${NICKNAME}$2 -f ${files} $1.elf

#else

@echo off
Setlocal enabledelayedexpansion
@echo ********************************************************************************
@echo           SDK BR28
@echo ********************************************************************************
@echo %date%


cd /d %~dp0

set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe
set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe
set ELFFILE=sdk.elf
set bankfiles=
set LZ4_PACKET=.\lz4_packet.exe

REM %OBJDUMP% -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
%OBJCOPY% -O binary -j .text %ELFFILE% text.bin
%OBJCOPY% -O binary -j .data %ELFFILE% data.bin
%OBJCOPY% -O binary -j .data_code %ELFFILE% data_code.bin
%OBJCOPY% -O binary -j .overlay_aec %ELFFILE% aec.bin
%OBJCOPY% -O binary -j .overlay_aac %ELFFILE% aac.bin
%OBJCOPY% -O binary -j .overlay_aptx %ELFFILE% aptx.bin

%OBJCOPY% -O binary -j .common %ELFFILE% common.bin

for /L %%i in (0,1,20) do (
            %OBJCOPY% -O binary -j .overlay_bank%%i %ELFFILE% bank%%i.bin
                set bankfiles=!bankfiles! bank%%i.bin 0x0
        )


%LZ4_PACKET% -dict text.bin -input common.bin 0 aec.bin 0 aac.bin 0 !bankfiles! -o bank.bin

%OBJDUMP% -section-headers -address-mask=0x1ffffff %ELFFILE%
REM %OBJDUMP% -t %ELFFILE% >  symbol_tbl.txt

copy /b text.bin + data.bin + mov_slot.bin + data_code.bin + aec.bin + aac.bin + psr_data_code.bin app.bin


del !bankfiles! common.bin text.bin data.bin bank.bin

#if TCFG_TONE_EN_ENABLE
set TONE_EN_ENABLE=1
#else
set TONE_EN_ENABLE=0
#endif

#if TCFG_TONE_ZH_ENABLE
set TONE_ZH_ENABLE=1
#else
set TONE_ZH_ENABLE=0
#endif

#if RCSP_MODE
set RCSP_EN=1
#endif

#if TCFG_UI_ENABLE && TCFG_SPI_LCD_ENABLE
set UI_RESOURCE_EN=1
#else
set	UI_RESOURCE_EN=0
#endif

#ifdef CONFIG_EARPHONE_CASE
#if TCFG_AUDIO_ANC_EAR_ADAPTIVE_EN
copy anc_ext.bin download\earphone\ALIGN_DIR\.
#else
del download\earphone\ALIGN_DIR\anc_ext.bin
#endif
call download/earphone/download.bat
#endif

#ifdef CONFIG_SOUNDBOX_CASE
call download/soundbox/download.bat
#endif
#endif
