#ifdef FU_OS_LINUX
#pragma once
#include "misc.linux.h"

#ifdef FU_ENABLE_DEBUG_MSG
void fu_os_print_error_from_code(const char* prefix, const int code);

#define fu_os_print_error(prefix)                     \
    do {                                              \
        fu_os_print_error_from_code((prefix), errno); \
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

#else
#define fu_os_print_error(prefix)
#define fu_os_print_error_from_code(prefix, code)
#define fu_os_return_if_fail(expr) ((void)(expr))
#define fu_os_return_val_if_fail(expr, val) ((void)(expr))

#endif
#endif