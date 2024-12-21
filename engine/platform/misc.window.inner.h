#ifdef FU_OS_WINDOW
#pragma once
#include "misc.window.h"

//
// 时间相关

#ifndef NDEBUG
#ifdef __cplusplus
extern "C" {
#endif
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
#ifdef __cplusplus
}
#endif
#else
#define fu_os_print_error(prefix)
#define fu_os_print_error_from_code(prefix, code)
#define fu_os_return_if_fail(expr) ((void)(expr))
#define fu_os_return_val_if_fail(expr, val) ((void)(expr))
#define fu_os_goto_if_fail(expr, tar) ((void)(expr))

#endif
#endif