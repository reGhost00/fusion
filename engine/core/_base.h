#pragma once
/* NOTE CAREFULLY:
 *
 * This file is the lowest-level part of GLib.
 *
 * Other lowlevel parts of GLib (threads, slice allocator, g_malloc,
 * messages, etc) call into these functions and macros to get work done.
 *
 * As such, these functions can not call back into any part of GLib
 * without risking recursion.
 */

#include <stdint.h>
#if defined(__GNUC__) || defined(__clang__)
#ifndef FU_COMPILER_GCC_CLANG
#define FU_COMPILER_GCC_CLANG
#endif
#ifndef FU_COMPILER_C23
#include <stdbool.h>
#ifndef static_assert
#define static_assert _Static_assert
#endif
#ifndef typeof
#define typeof(t) __typeof__(t)
#endif
#ifndef offsetof
#define offsetof(s, m) __builtin_offsetof(s, m)
#endif
#ifndef alignof
#define alignof(x) __alignof__(x)
#endif
#ifndef alignas
#define alignas(x) __attribute__((aligned(x)))
#endif
#endif
#define FU_LIKELY(expr) __builtin_expect(!!(expr), 1)
#define FU_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define FU_LIKELY(expr) (expr)
#define FU_UNLIKELY(expr) (expr)
#endif

#define fu_max(a, b) (a > b ? a : b)
#define fu_min(a, b) (a < b ? a : b)
#define FU_N_ELEMENTS(x) (sizeof(x) / sizeof(x[0]))

typedef struct _FUObject FUObject;
typedef struct _FUAsyncResult FUAsyncResult;
typedef void (*FUAsyncReadyCallback)(FUObject* obj, FUAsyncResult* res, void* usd);

typedef int (*FUCompareFunc)(const void* a, const void* b);
typedef int (*FUCompareDataFunc)(const void* a, const void* b, void* usd);
typedef bool (*FUEqualFunc)(const void* a, const void* b);
typedef uint64_t (*FUHashFunc)(const void* key);
typedef void (*FUFunc)(void* data, void* usd);
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
