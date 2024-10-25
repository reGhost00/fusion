#pragma once
#include <stdint.h>
// custom
#include "logger.h"

#if defined(__GNUC__) || defined(__clang__)
#define FU_LIKELY(expr) __builtin_expect(!!(expr), 1)
#define FU_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define FU_LIKELY(expr) (expr)
#define FU_UNLIKELY(expr) (expr)
#endif

typedef int (*FUCompareFunc)(const void* a, const void* b);
typedef int (*FUCompareDataFunc)(const void* a, const void* b, void* usd);
typedef bool (*FUEqualFunc)(const void* a, const void* b);
typedef uint64_t (*FUHashFunc)(const void* key);
typedef void (*FUFunc)(void* data, void* usd);
typedef bool (*FUFuncRevBool)(void* data, void* usd);
typedef char* (*FUFuncRevStr)(void* data, void* usd);
typedef void* (*FUFuncRevPtr)(void* data, void* usd);

typedef void (*FUNotify)(void* usd);
/**
 * GCopyFunc:
 * @src: (not nullable): A pointer to the data which should be copied
 * @data: Additional data
 *
 * A function of this signature is used to copy the node data
 * when doing a deep-copy of a tree.
 *
 * Returns: (not nullable): A pointer to the copy
 *
 * Since: 2.4
 */
typedef void* (*FUCopyFunc)(const void* src, void* dest);

/**
 * GFreeFunc:
 * @data: a data pointer
 *
 * Declares a type of function which takes an arbitrary
 * data pointer argument and has no return value. It is
 * not currently used in GLib or GTK.
 */
typedef void (*FUFreeFunc)(void* data);

typedef struct _FUError {
    int code;
    char* message;
} FUError;

FUError* fu_error_new_take(int code, char** msg);
FUError* fu_error_new_from_code(int code);
void fu_error_free(FUError* err);

static inline void fu_void0() { }
static inline void fu_void1(void* p) { }

#define fu_max(a, b) (((a) ^ ((a) ^ (b))) & -(a < b))
#define fu_min(a, b) (((b) ^ ((b) ^ (a))) & -(b < a))

#ifndef NDEBUG
#ifndef FU_ERROR_GUARD_DISABLED
#define fu_return_if_fail(expr)                      \
    do {                                             \
        if (FU_UNLIKELY(!(expr))) {                  \
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
#else
#define fu_return_if_fail(expr)
#define fu_return_val_if_fail(expr, val)
#endif // FU_ERROR_GUARD_DISABLED
#ifndef FU_ERROR_GUARD_MSG_DISABLED
#define fu_return_if_fail_with_message(expr, msg)  \
    do {                                           \
        if (FU_UNLIKELY(!(expr))) {                \
            fu_warning("%s: %s\n", __func__, msg); \
            return;                                \
        }                                          \
    } while (0)

#define fu_return_val_if_fail_with_message(expr, msg, val) \
    do {                                                   \
        if (FU_UNLIKELY(!(expr))) {                        \
            fu_warning("%s: %s\n", __func__, msg);         \
            return val;                                    \
        }                                                  \
    } while (0)
#else
#define fu_return_if_fail_with_message(expr, msg)
#define fu_return_val_if_fail_with_message(expr, msg, val)
#endif // FU_ERROR_GUARD_MSG_DISABLED
#ifndef FU_CHECK_CALLBACK_DISABLED
#define fu_return_if_true(expr, cb, args) \
    do {                                  \
        if (FU_UNLIKELY(expr)) {          \
            cb(args);                     \
            return;                       \
        }                                 \
    } while (0)

#define fu_return_val_if_true(expr, cb, val) \
    do {                                     \
        if (FU_UNLIKELY(expr)) {             \
            cb();                            \
            return val;                      \
        }                                    \
    } while (0)
#else
#define fu_return_if_true(expr, cb, args)
#define fu_return_val_if_true(expr, cb, val)
#endif // FU_CHECK_CALLBACK_DISABLED
#ifndef FU_CHECK_CALLBACK_MSG_DISABLED
#define fu_return_if_true_with_message(expr, msg)  \
    do {                                           \
        if (FU_UNLIKELY(expr)) {                   \
            fu_warning("%s: %s\n", __func__, msg); \
            return;                                \
        }                                          \
    } while (0)

#define fu_return_val_if_true_with_message(expr, msg, val) \
    do {                                                   \
        if (FU_UNLIKELY(expr)) {                           \
            fu_warning("%s: %s\n", __func__, msg);         \
            return val;                                    \
        }                                                  \
    } while (0)
#else
#define fu_return_if_true_with_message(expr, msg)
#define fu_return_val_if_true_with_message(expr, msg, val)
#endif // FU_CHECK_CALLBACK_MSG_DISABLED
#ifndef FU_CONTROL_FLOW_DISABLED
#define fu_goto_if_fail(expr, tar)                   \
    do {                                             \
        if (FU_UNLIKELY(!(expr))) {                  \
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

#else
#define fu_goto_if_fail(expr, tar)
#define fu_continue_if_fail(expr)
#endif // FU_CONTROL_FLOW_DISABLED
#else // NDEBUG
#ifdef FU_ERROR_GUARD_ENABLED
#define fu_return_if_fail(expr)   \
    do {                          \
        if (FU_UNLIKELY(!(expr))) \
            return;               \
    } while (0)
#define fu_return_if_fail_with_message(expr, msg) \
    do {                                          \
        if (FU_UNLIKELY(!(expr)))                 \
            return;                               \
    } while (0)
#define fu_return_val_if_fail(expr, val) \
    do {                                 \
        if (FU_UNLIKELY(!(expr)))        \
            return (val);                \
    } while (0)
#define fu_return_val_if_fail_with_message(expr, msg, val) \
    do {                                                   \
        if (FU_UNLIKELY(!(expr)))                          \
            return (val);                                  \
    } while (0)
#else
#define fu_return_if_fail(expr)
#define fu_return_if_fail_with_message(expr, msg)
#define fu_return_val_if_fail(expr, val)
#define fu_return_val_if_fail_with_message(expr, msg, val)
#endif // FU_ERROR_GUARD_ENABLED
#ifdef FU_CONTROL_FLOW_ENABLED
#define fu_return_if_true(expr, cb, args) \
    do {                                  \
        if (FU_UNLIKELY(expr))            \
            return;                       \
    } while (0)
#define fu_return_val_if_true(expr, cb, val) \
    do {                                     \
        if (FU_UNLIKELY(expr)) {             \
            return val;                      \
        }                                    \
    } while (0)
#define fu_return_if_true_with_message(expr, msg) \
    do {                                          \
        if (FU_UNLIKELY(expr))                    \
            return;                               \
    } while (0)
#define fu_return_val_if_true_with_message(expr, msg, val) \
    do {                                                   \
        if (FU_UNLIKELY(expr))                             \
            return val;                                    \
    } while (0)
#define fu_goto_if_fail(expr, tar) \
    do {                           \
        if (FU_UNLIKELY(!(expr)))  \
            goto tar;              \
    } while (0)
#define fu_continue_if_fail(expr) \
    do {                          \
        if (FU_UNLIKELY(!(expr))) \
            continue;             \
    } while (0)

#else
#define fu_return_if_true(expr, cb, args)
#define fu_return_val_if_true(expr, cb, val)
#define fu_return_if_true_with_message(expr, msg)
#define fu_return_val_if_true_with_message(expr, msg, val)
#define fu_goto_if_fail(expr, tar)
#define fu_continue_if_fail(expr)
#endif // FU_CONTROL_FLOW_ENABLED
#endif // NDEBUG