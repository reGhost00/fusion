#ifdef FU_OS_WINDOW
#pragma once
#include "misc.window.h"

//
// 字符编码相关
// window 专有

char* fu_wchar_to_utf8(const wchar_t* wstr, size_t* len);
wchar_t* fu_utf8_to_wchar(const char* str, size_t* len);
//
// 时间相关

#ifndef NDEBUG
void fu_os_print_error_from_code(const char* prefix, const DWORD code);

#define fu_os_print_error(prefix)                              \
    do {                                                       \
        fu_os_print_error_from_code((prefix), GetLastError()); \
    } while (0)

#define fu_os_return_if_fail(expr)       \
    do {                                 \
        if (FU_UNLIKELY(!(expr))) {      \
            fu_os_print_error(__func__); \
            return;                      \
        }                                \
    } while (0)

#define fu_os_return_val_if_fail(expr, val) \
    do {                                    \
        if (FU_UNLIKELY(!(expr))) {         \
            fu_os_print_error(__func__);    \
            return val;                     \
        }                                   \
    } while (0)

#define fu_os_goto_if_fail(expr, tar)    \
    do {                                 \
        if (FU_UNLIKELY(!(expr))) {      \
            fu_os_print_error(__func__); \
            goto tar;                    \
        }                                \
    } while (0)
#else
#define fu_os_print_error(prefix)
#define fu_os_print_error_from_code(prefix, code)
#define fu_os_return_if_fail(expr) ((void)(expr))
#define fu_os_return_val_if_fail(expr, val) ((void)(expr))
#define fu_os_goto_if_fail(expr, tar) ((void)(expr))

#endif
#endif