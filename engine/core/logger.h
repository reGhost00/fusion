#pragma once
#define fu_critical(message, ...) fu_log(EFU_LOG_LEVEL_CRITICAL, message, ##__VA_ARGS__)
#define fu_error(message, ...) fu_log(EFU_LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#define fu_abort_if_true(expr)                        \
    do {                                              \
        if (FU_UNLIKELY(expr)) {                      \
            fu_critical("%s: %s\n", __func__, #expr); \
            abort();                                  \
        }                                             \
    } while (0)
#define fu_abort_if_fail(expr)                        \
    do {                                              \
        if (FU_UNLIKELY(!(expr))) {                   \
            fu_critical("%s: %s\n", __func__, #expr); \
            abort();                                  \
        }                                             \
    } while (0)
#ifdef FU_ENABLE_DEBUG_MESSAGE
#ifndef FU_DISABLE_ERROR_GUARD
#define fu_return_if_fail(expr)                      \
    do {                                             \
        if (FU_UNLIKELY(!(expr))) {                  \
            fu_warning("%s: %s\n", __func__, #expr); \
            return;                                  \
        }                                            \
    } while (0)

#define fu_return_if_true(expr)                      \
    do {                                             \
        if (FU_UNLIKELY(expr)) {                     \
            fu_warning("%s: %s\n", __func__, #expr); \
            return;                                  \
        }                                            \
    } while (0)

#define fu_return_val_if_fail(expr, val)             \
    do {                                             \
        if (FU_UNLIKELY(!(expr))) {                  \
            fu_warning("%s: %s\n", __func__, #expr); \
            return (val);                            \
        }                                            \
    } while (0)

#define fu_return_val_if_true(expr, val)             \
    do {                                             \
        if (FU_UNLIKELY(expr)) {                     \
            fu_warning("%s: %s\n", __func__, #expr); \
            return (val);                            \
        }                                            \
    } while (0)
#endif // FU_DISABLE_ERROR_GUARD
#ifndef FU_DISABLE_CONTROL_FLOW
#define fu_goto_if_fail(expr, tar)                   \
    do {                                             \
        if (FU_UNLIKELY(!(expr))) {                  \
            fu_warning("%s: %s\n", __func__, #expr); \
            goto tar;                                \
        }                                            \
    } while (0)

#define fu_goto_if_true(expr, tar)                   \
    do {                                             \
        if (FU_UNLIKELY(expr)) {                     \
            fu_warning("%s: %s\n", __func__, #expr); \
            goto tar;                                \
        }                                            \
    } while (0)

#define fu_continue_if_fail(expr)                    \
    do {                                             \
        if (FU_UNLIKELY(!(expr))) {                  \
            fu_warning("%s: %s\n", __func__, #expr); \
            continue;                                \
        }                                            \
    } while (0)

#define fu_continue_if_true(expr)                    \
    do {                                             \
        if (FU_UNLIKELY(expr)) {                     \
            fu_warning("%s: %s\n", __func__, #expr); \
            continue;                                \
        }                                            \
    } while (0)

#define fn_exec_if_true(expr)                          \
    if (FU_UNLIKELY(!(expr)))                          \
        fu_warning("%s %s failed\n", __func__, #expr); \
    else

#define fu_return_call_if_fail(expr, fn, args)       \
    do {                                             \
        if (FU_UNLIKELY(!(expr))) {                  \
            fu_warning("%s: %s\n", __func__, #expr); \
            return fn(args);                         \
        }                                            \
    } while (0)
#define fu_return_call_if_true(expr, fn, args)       \
    do {                                             \
        if (FU_UNLIKELY(expr)) {                     \
            fu_warning("%s: %s\n", __func__, #expr); \
            return fn(args);                         \
        }                                            \
    } while (0)
#endif // FU_DISABLE_CONTROL_FLOW
#define fu_warning(message, ...) fu_log(EFU_LOG_LEVEL_WARNING, message, ##__VA_ARGS__)
#define fu_info(message, ...) fu_log(EFU_LOG_LEVEL_INFO, message, ##__VA_ARGS__)
#define fu_debug(message, ...) fu_log(EFU_LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)
#define fu_print_func_name(...) (printf("%s "__VA_ARGS__ \
                                        "\n",            \
    __func__))

#elif FU_DISABLE_DEBUG_MESSAGE // FU_ENABLE_DEBUG_MESSAGE
#ifdef FU_ENABLE_ERROR_GUARD
#define fu_return_if_fail(expr)   \
    do {                          \
        if (FU_UNLIKELY(!(expr))) \
            return;               \
    } while (0)

#define fu_return_if_true(expr) \
    do {                        \
        if (FU_UNLIKELY(expr))  \
            return;             \
    } while (0)

#define fu_return_val_if_fail(expr, val) \
    do {                                 \
        if (FU_UNLIKELY(!(expr)))        \
            return (val);                \
    } while (0)

#define fu_return_val_if_true(expr, val) \
    do {                                 \
        if (FU_UNLIKELY(expr))           \
            return (val);                \
    } while (0)

#endif // FU_ENABLE_ERROR_GUARD
#ifdef FU_ENABLE_CONTROL_FLOW
#define fu_goto_if_fail(expr, tar) \
    do {                           \
        if (FU_UNLIKELY(!(expr)))  \
            goto tar;              \
    } while (0)

#define fu_goto_if_true(expr, tar) \
    do {                           \
        if (FU_UNLIKELY(expr))     \
            goto tar;              \
    } while (0)

#define fu_continue_if_fail(expr) \
    do {                          \
        if (FU_UNLIKELY(!(expr))) \
            continue;             \
    } while (0)

#define fu_continue_if_true(expr) \
    do {                          \
        if (FU_UNLIKELY(expr))    \
            continue;             \
    } while (0)

#define fu_return_call_if_fail(expr, fn, args) \
    do {                                       \
        if (FU_UNLIKELY(!(expr))) {            \
            return fn(args);                   \
        }                                      \
    } while (0)
#define fu_return_call_if_true(expr, fn, args) \
    do {                                       \
        if (FU_UNLIKELY(expr)) {               \
            return fn(args);                   \
        }                                      \
    } while (0)
#define fn_exec_if_true(expr) if (FU_UNLIKELY(expr))
#endif // FU_ENABLE_CONTROL_FLOW
#define fu_warning(message, ...)
#define fu_info(message, ...)
#define fu_debug(message, ...)
#endif // FU_DISABLE_DEBUG_MESSAGE // FU_ENABLE_DEBUG_MESSAGE
#ifndef fu_return_if_fail
#define fu_return_if_fail(expr)
#define fu_return_if_true(expr)
#define fu_return_val_if_fail(expr, val)
#define fu_return_val_if_true(expr, val)
#endif
#ifndef fu_goto_if_fail
#define fu_goto_if_fail(expr, tar)
#define fu_goto_if_true(expr, tar)
#endif
#ifndef fu_continue_if_fail
#define fu_continue_if_fail(expr)
#define fu_continue_if_true(expr)
#endif
#ifndef fn_exec_if_true
#define fn_exec_if_true(expr)
#define fu_return_call_if_fail(expr, fn, args)
#define fu_return_call_if_true(expr, fn, args)
#endif
#ifndef fu_warning
#define fu_warning(message, ...)
#define fu_info(message, ...)
#define fu_debug(message, ...)
#define fu_print_func_name(...)
#endif

/** @brief Represents levels of logging */

typedef enum _EFULogLevel {
    EFU_LOG_LEVEL_CRITICAL,
    EFU_LOG_LEVEL_ERROR,
    EFU_LOG_LEVEL_WARNING,
    EFU_LOG_LEVEL_INFO,
    EFU_LOG_LEVEL_DEBUG,
    EFU_LOG_LEVEL_CNT
} EFULogLevel;

#ifdef __cplusplus
extern "C" {
#endif
// A function pointer for a console to hook into the logger.
typedef void (*FULogFunc)(EFULogLevel level, const char* message);

void fu_log_set_handler(FULogFunc handler);
void fu_log_reset_handler();

void fu_log(EFULogLevel level, const char* format, ...);

#ifdef __cplusplus
}
#endif