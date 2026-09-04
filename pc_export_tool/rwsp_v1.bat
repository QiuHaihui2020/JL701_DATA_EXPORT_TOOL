@echo off
setlocal enabledelayedexpansion

:: 保存当前控制台代码页，脚本结束时恢复，避免污染调用者的 cmd 窗口
for /f "tokens=2 delims=:" %%a in ('chcp') do set "OLD_CP=%%a"
set "OLD_CP=%OLD_CP: =%"
:: 本文件以 UTF-8(无 BOM) 保存，切到 65001 才能正常显示中文提示
chcp 65001 >nul

cd /d %~dp0

set /p "USER_DEV=请输入T卡盘符（如H）："

set /p "FILE_NUM=请输入%USER_DEV%:\JL_DEBUG\dbg_xxx.bin文件序号（如002）："

set "FILE_NAME=dbg_%FILE_NUM%.bin"

set "FILE_PATH=%USER_DEV%:\JL_DEBUG\%FILE_NAME%"
echo 文件完整路径：%FILE_PATH%

:: 检查文件是否存在
if exist "%FILE_PATH%" (
    echo 文件存在，正在解析...
) else (
    echo 错误：文件不存在！
    goto :END
)

:: 数据通道数
set  /p "CH_NUM=请输入写卡小板显示的ch大小（如3）："
:: 每通道的数据帧长，单位byte
set  /p "FRAME_LEN=请输入写卡小板显示的len大小（如512）："

:: 动态生成 unpack.exe 的参数（重复 CH_NUM 次 FRAME_LEN）
set ARGS=
for /l %%i in (1,1,%CH_NUM%) do (
    set ARGS=!ARGS! %FRAME_LEN%
)

echo 正在执行：unpack.exe %FILE_PATH% %CH_NUM%!ARGS!
:: 解析数据
unpack.exe %FILE_PATH% %CH_NUM%!ARGS!

:END
:: 恢复进入脚本前的代码页
chcp %OLD_CP% >nul
pause
