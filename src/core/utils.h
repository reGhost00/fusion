#ifndef _FU_UTILS_H_
#define _FU_UTILS_H_
// #define FU_NO_TRACK_MEMORY

#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#ifdef FU_OS_WINDOW
#include <windows.h>
#endif

typedef struct _FUError {
    int code;
    char* message;
} FUError;

FUError* fu_error_new_take(int code, char** msg);
FUError* fu_error_new_from_code(int code);
void fu_error_free(FUError* err);

#define fu_continue_if_fail(expr) \
    do {                          \
        if (!(expr))              \
            continue;             \
    } while (0)
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
#endif
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

#define fu_max(a, b) ((a) ^ ((a) ^ (b)) & -(a < b))
#define fu_min(a, b) ((b) ^ ((b) ^ (a)) & -(b < a))

// #define FU_NO_TRACK_MEMORY
#ifdef FU_NO_TRACK_MEMORY
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
void fu_clear_pointer(void** pp, FUNotify destroy);
void* fu_memdup(const void* p, size_t size);
char* fu_strdup(const char* src);
char* fu_strndup(const char* str, size_t n);
char* fu_strnfill(size_t length, char ch);
char* fu_stpcpy(char* dest, const char* src);
char* fu_strdup_printf(const char* format, ...);

#ifdef FU_OS_WINDOW

char* fu_wchar_to_utf8(const wchar_t* wstr, size_t* len);
wchar_t* fu_utf8_to_wchar(const char* str, size_t* len);
#ifndef FU_NO_DEBUG
#define fu_winapi_print_error(prefix)                                                                                                                                                                      \
    do {                                                                                                                                                                                                   \
        DWORD err = GetLastError();                                                                                                                                                                        \
        wchar_t* strW = NULL;                                                                                                                                                                              \
        size_t len;                                                                                                                                                                                        \
        if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR) & strW, 0, NULL)) { \
            char* strU = fu_wchar_to_utf8(strW, &len);                                                                                                                                                     \
            fprintf(stderr, "%s %ld %s\n", prefix, err, strU);                                                                                                                                             \
            LocalFree(strW);                                                                                                                                                                               \
            fu_free(strU);                                                                                                                                                                                 \
        } else                                                                                                                                                                                             \
            fprintf(stderr, "%s %ld\n", prefix, err);                                                                                                                                                      \
    } while (0)
#else
#define fu_winapi_print_error(prefix) ((void)0)
#endif

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

#endif

#endif