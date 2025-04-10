#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".undef_func.data.bss")
#pragma data_seg(".undef_func.data")
#pragma const_seg(".undef_func.text.const")
#pragma code_seg(".undef_func.text")
#endif
#include "app_main.h"
#include "usb/usb.h"
#include "vm.h"

