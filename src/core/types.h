#ifndef __G_MACROS_H__
#define __G_MACROS_H__

#include <stdint.h>
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

#endif /* __G_MACROS_H__ */
