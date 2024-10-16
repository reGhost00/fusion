#pragma once
// #define FU_NO_DEBUG
#include <stdio.h>
#include <stdlib.h>
// custom
#include "types.h"

typedef struct _FUError {
    int code;
    char* message;
} FUError;

FUError* fu_error_new_take(int code, char** msg);
FUError* fu_error_new_from_code(int code);
void fu_error_free(FUError* err);
/**
 * fu_steal_pointer:
 * @pp: (not nullable): a pointer to a pointer
 *
 * Sets @pp to %NULL, returning the value that was there before.
 *
 * Conceptually, this transfers the ownership of the pointer from the
 * referenced variable to the "caller" of the macro (ie: "steals" the
 * reference).
 *
 * The return value will be properly typed, according to the type of @pp. */
static inline void* fu_steal_pointer(void* pp)
{
    void** ptr = (void**)pp;
    void* ref;

    ref = *ptr;
    *ptr = NULL;

    return ref;
}
#define fu_steal_pointer(pp) ((typeof(*pp))(fu_steal_pointer)(pp))
void fu_clear_pointer(void** pp, FUNotify destroy);
void* fu_memdup(const void* p, size_t size);
char* fu_strdup(const char* src);
char* fu_strndup(const char* str, size_t n);
char* fu_strnfill(size_t length, char ch);
char* fu_strdup_printf(const char* format, ...);

#define fu_max(a, b) (((a) ^ ((a) ^ (b))) & -(a < b))
#define fu_min(a, b) (((b) ^ ((b) ^ (a))) & -(b < a))

#define fu_continue_if_fail(expr) \
    do {                          \
        if (!(expr))              \
            continue;             \
    } while (0)

#if defined(FU_NO_TRACK_MEMORY) || defined(FU_NO_DEBUG)
#ifndef FU_NO_TRACK_MEMORY
#define FU_NO_TRACK_MEMORY
#endif
#include <mimalloc.h>
#define fu_malloc mi_malloc
#define fu_malloc0 mi_zalloc
#define fu_malloc_n mi_mallocn
#define fu_malloc0_n mi_calloc
#define fu_realloc mi_realloc
#define fu_free mi_free
#else
void* fu_malloc(size_t size);
void* fu_malloc0(size_t size);
void* fu_malloc_n(size_t count, size_t size);
void* fu_malloc0_n(size_t count, size_t size);
void* fu_realloc(void* p, size_t size);
void fu_free(void* p);
#endif

#ifndef FU_NO_DEBUG

#define fu_return_if_fail(expr)                                  \
    do {                                                         \
        if (FU_UNLIKELY(!(expr))) {                              \
            fprintf(stderr, "%s Failed: %s\n", __func__, #expr); \
            return;                                              \
        }                                                        \
    } while (0)

#define fu_return_if_fail_with_message(expr, msg)              \
    do {                                                       \
        if (FU_UNLIKELY(!(expr))) {                            \
            fprintf(stderr, "%s Failed: %s\n", __func__, msg); \
            return;                                            \
        }                                                      \
    } while (0)

#define fu_return_val_if_fail(expr, val)                         \
    do {                                                         \
        if (FU_UNLIKELY(!(expr))) {                              \
            fprintf(stderr, "%s Failed: %s\n", __func__, #expr); \
            return (val);                                        \
        }                                                        \
    } while (0)

#define fu_return_val_if_fail_with_message(expr, msg, val) \
    do {                                                   \
        if (FU_UNLIKELY(!(expr))) {                        \
            fprintf(stderr, "%s: %s\n", __func__, msg);    \
            return val;                                    \
        }                                                  \
    } while (0)

#define fu_goto_if_fail(expr, tar)                               \
    do {                                                         \
        if (FU_UNLIKELY(!(expr))) {                              \
            fprintf(stderr, "%s Failed: %s\n", __func__, #expr); \
            goto tar;                                            \
        }                                                        \
    } while (0)

#else
#define fu_return_if_fail(expr) ((void)(expr))

#define fu_return_if_fail_with_message(expr, msg) \
    do {                                          \
        if (FU_UNLIKELY(!(expr)))                 \
            return;                               \
    } while (0)

#define fu_return_val_if_fail(expr, val) ((void)(expr))

#define fu_return_val_if_fail_with_message(expr, msg, val) \
    do {                                                   \
        if (FU_UNLIKELY(!(expr)))                          \
            return val;                                    \
    } while (0)

#define fu_goto_if_fail(expr, tar) ((void)(expr))
#endif // FU_NO_DEBUG

#ifdef FU_OS_WINDOW
#include <windows.h>

char* fu_wchar_to_utf8(const wchar_t* wstr, size_t* len);
wchar_t* fu_utf8_to_wchar(const char* str, size_t* len);
#ifndef FU_NO_DEBUG

void fu_winapi_print_error_from_code(const char* prefix, const DWORD code);

#define fu_winapi_print_error(prefix)                              \
    do {                                                           \
        fu_winapi_print_error_from_code((prefix), GetLastError()); \
    } while (0)

#define fu_winapi_return_if_fail(expr)       \
    do {                                     \
        if (FU_UNLIKELY(!(expr))) {          \
            fu_winapi_print_error(__func__); \
            return;                          \
        }                                    \
    } while (0)
#define fu_winapi_return_val_if_fail(expr, val) \
    do {                                        \
        if (FU_UNLIKELY(!(expr))) {             \
            fu_winapi_print_error(__func__);    \
            return val;                         \
        }                                       \
    } while (0)

#define fu_winapi_goto_if_fail(expr, tar)    \
    do {                                     \
        if (FU_UNLIKELY(!(expr))) {          \
            fu_winapi_print_error(__func__); \
            goto tar;                        \
        }                                    \
    } while (0)

#else

#define fu_winapi_print_error(prefix) ((void)0)
#define fu_winapi_print_error_from_code(prefix, code) ((void)0)
#define fu_winapi_return_if_fail(expr) ((void)(expr))
#define fu_winapi_return_val_if_fail(expr, val) ((void)(expr))
#define fu_winapi_goto_if_fail(expr, tar) ((void)(expr))

#endif // FU_NO_DEBUG
#endif // FU_OS_WINDOW
#ifdef FU_OS_LINUX
#include <errno.h>
//
// Linux force to use UTF-8
// no need to use wchar
#define fu_wchar_to_utf8(wstr, len) (wstr)
#define fu_utf8_to_wchar(str, len) (str)
#ifndef FU_NO_DEBUG

void fu_winapi_print_error_from_code(const char* prefix, const DWORD code);

#define fu_winapi_print_error(prefix)                    \
    do {                                                 \
        fu_winapi_print_error_with_code((prefix), errno) \
    } while (0)

#define fu_winapi_return_if_fail(expr)       \
    do {                                     \
        if (FU_UNLIKELY(!(expr))) {          \
            fu_winapi_print_error(__func__); \
            return;                          \
        }                                    \
    } while (0)
#define fu_winapi_return_val_if_fail(expr, val) \
    do {                                        \
        if (FU_UNLIKELY(!(expr))) {             \
            fu_winapi_print_error(__func__);    \
            return val;                         \
        }                                       \
    } while (0)

#define fu_winapi_goto_if_fail(expr, tar)    \
    do {                                     \
        if (FU_UNLIKELY(!(expr))) {          \
            fu_winapi_print_error(__func__); \
            goto tar;                        \
        }                                    \
    } while (0)

#else

#define fu_winapi_print_error(prefix) ((void)0)
#define fu_winapi_print_error_from_code(prefix, code) ((void)0)
#define fu_winapi_return_if_fail(expr) ((void)(expr))
#define fu_winapi_return_val_if_fail(expr, val) ((void)(expr))
#define fu_winapi_goto_if_fail(expr, tar) ((void)(expr))

#endif // FU_NO_DEBUG
#endif // FU_OS_LINUX
