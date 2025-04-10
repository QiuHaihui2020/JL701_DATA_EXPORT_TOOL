@echo off
SET PROJ_DOWNLOAD_PATH=%1
if %2==format_flash (
    SET FORMAT_ALL_ENABLE=1
    SET FORMAT_VM_ENABLE=0
)
if %2==format_vm (
    SET FORMAT_VM_ENABLE=1
    SET FORMAT_ALL_ENABLE=0
)
if %2==download  (
    SET FORMAT_ALL_ENABLE=0
    SET FORMAT_VM_ENABLE=0
)

cpu\br28\tools\download.bat
