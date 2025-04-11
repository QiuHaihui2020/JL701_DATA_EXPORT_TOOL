@echo off
setlocal enabledelayedexpansion

cd /d %~dp0

set /p USER_DEV=请输入T卡盘符（如H）:

set /p "FILE_NUM=请输入%USER_DEV%:\JL_DEBUG\dbg_xxx.bin文件序号（如002）："

set "FILE_NAME=dbg_%FILE_NUM%.bin"

set "FILE_PATH=%USER_DEV%:\JL_DEBUG\%FILE_NAME%"
echo 文件完整路径：%FILE_PATH%

:: 检查文件是否存在
if exist "%FILE_PATH%" (
    echo 文件存在，正在解析...
) else (
    echo 错误：文件不存在！
    pause
    exit
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

pause